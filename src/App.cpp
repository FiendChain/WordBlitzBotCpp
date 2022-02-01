#include "App.h"

#include <algorithm>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>

#include <fmt/core.h>

#include "Texture.h"

#include "model.h"
#include "buffer_graphics.h"
#include "util/MSS.h"
#include "util/AutoGui.h"
#include "util/KeyListener.h"

App::App(ID3D11Device *dx11_device, ID3D11DeviceContext *dx11_context)
: m_dx11_device(dx11_device), 
  m_dx11_context(dx11_context)
{
    m_is_render_running = true;
    m_is_grabbing = true;

    m_is_tracing = false;
    m_is_tracer_thread_alive = true;
    m_tracer_speed_ms = 33;
    m_tracer_thread = std::make_unique<std::thread>([this]() {
        this->TracerThread();
    });

    m_mss = std::make_shared<util::MSS>();
    m_params = std::make_shared<AppParams>(4);
    {
        auto model_bonuses      = std::make_unique<BonusesModel>    ("assets/models/bonuses.tflite");
        auto model_characters   = std::make_unique<CharacterModel>  ("assets/models/characters.tflite");
        auto model_values       = std::make_unique<ValuesModel>     ("assets/models/two_digit_classifier.tflite");
        m_model = std::make_shared<UnifiedModel>(
            model_bonuses, model_characters, model_values, 
            m_params);
    }
    {
        std::ifstream fp;
        fp.open("assets/dicts/en.txt", std::ios::binary);
        if (!fp.is_open()) {
            throw std::runtime_error("Failed to load dictionary");
        }
        std::stringstream ss;
        ss << fp.rdbuf();
        fp.close();

        const auto &buf = ss.str();

        wordtree::ReadWordTree(buf.c_str(), static_cast<int>(buf.length()), m_node_pool, 20);
    }
    {
        m_params->cropper_bonuses = {
            {5,9},
            {23,17},
            {107,107}
        };
        m_params->cropper_characters = {
            {35,40},
            {40,40},
            {107,107}
        };
        m_params->cropper_values = {
            {73,24},
            {20,16},
            {107,107}
        };
        m_params->inter_buffer_size = {433,433};
    }

    // setup screen shotter
    m_capture_position = {463,558};
    SetScreenshotSize(433,433);

    {
        auto &m = m_model->m_model_characters;
        auto size = m->GetInputSize();
        m_resize_texture = std::make_unique<Texture>(
            m_dx11_device, m_dx11_context,
            size.x, size.y);
    }


    // create application bindings
    util::InitGlobalListener();
    util::AttachKeyboardListener(VK_F2, [this](WPARAM type) {
        if (type == WM_KEYDOWN) {
            SetIsRendering(!GetIsRendering());
        }
    });
    util::AttachKeyboardListener(VK_F3, [this](WPARAM type) {
        if (type == WM_KEYDOWN) {
            SetIsGrabbing(!GetIsGrabbing());
        }
    });
    util::AttachKeyboardListener(VK_F4, [this](WPARAM type) {
        if (type == WM_KEYDOWN) {
            SetIsTracing(!GetIsTracing());
        }
    });
}

App::~App() {
    // stop tracing and terminate tracer thread
    m_is_tracer_thread_alive = false;
    m_is_tracing = false;
    m_tracer_thread->join();
}

void App::Update() {
    // update the view window
    if (m_is_grabbing) {
        GrabScreen();
    }
}

void App::UpdateTraces() {
    auto &grid = m_params->grid;
    try {
        auto searches = wordblitz::SearchWordTree(m_node_pool, grid.characters, grid.sqrt_size);
        auto lock = std::unique_lock(m_traces_mutex);
        m_traces = wordblitz::GetTraceFromSearch(grid, searches);
    } catch (std::exception &ex) {
        m_errors.push_back(fmt::format(
            "Error when tracing: {}", 
            ex.what()
        ));
    }
}

void App::GrabScreen() {
    m_mss->Grab(m_capture_position.top, m_capture_position.left);
}

void App::ReadScreen() {
    auto bitmap = m_mss->GetBitmap();
    auto buffer_size = bitmap.GetSize();
    auto buffer_max_size = m_mss->GetMaxSize();
    DIBSECTION &sec = bitmap.GetBitmap();
    BITMAP &bmp = sec.dsBm;
    uint8_t *buffer = (uint8_t*)(bmp.bmBits);

    const int row_stride = buffer_max_size.x*sizeof(uint8_t)*4;
    uint8_t *data = buffer + (buffer_max_size.y-buffer_size.y)*row_stride;

    try {
        m_model->Update(
            data,
            buffer_size.x, buffer_size.y, row_stride);
    } catch (std::exception &ex) {
        m_errors.push_back(fmt::format(
            "Error when reading: {}", 
            ex.what()
        ));
    }
}

void App::SetScreenshotSize(const int width, const int height) {
    m_screen_width = width;
    m_screen_height = height;
    m_mss->SetSize(width, height);
    m_screenshot_texture = std::make_unique<Texture>(
        m_dx11_device, m_dx11_context,
        width, height);
}

void App::UpdateScreenshotTexture() {
    auto bitmap = m_mss->GetBitmap();
    auto buffer_size = bitmap.GetSize();
    auto buffer_max_size = m_mss->GetMaxSize();
    DIBSECTION &sec = bitmap.GetBitmap();
    BITMAP &bmp = sec.dsBm;
    uint8_t *buffer = (uint8_t*)(bmp.bmBits);

    const int src_row_stride = buffer_max_size.x*sizeof(uint8_t)*4;
    uint8_t *src_data = buffer + (buffer_max_size.y-buffer_size.y)*src_row_stride;
    RGBA<uint8_t> *src_buffer = (RGBA<uint8_t>*)(src_data);


    auto texture_context = m_screenshot_texture->CreateEditContext();
    const int dst_row_stride = texture_context.GetRowStride();
    auto dst_buffer = texture_context.GetBuffer();

    for (int x = 0; x < buffer_size.x; x++) {
        for (int y = 0; y < buffer_size.y; y++) {
            // screenshot is upside down
            int i = x + (buffer_size.y-y-1)*dst_row_stride;
            int j = x + y*buffer_max_size.x;
            dst_buffer[i] = src_buffer[j];
        }
    }

    // scale the grid to screenshot size
    const float xscale = (float)buffer_size.x / (float)m_params->inter_buffer_size.x;
    const float yscale = (float)buffer_size.y / (float)m_params->inter_buffer_size.y;

    auto RenderGrid = [xscale, yscale](
        RGBA<uint8_t> *buffer, const int width, const int height, const int row_stride,
        GridCropper &cropper, wordblitz::Grid &grid,
        const RGBA<uint8_t> &pen_color, const int pen_width=1)
    {
        for (int x = 0; x < grid.sqrt_size; x++) {
            for (int y = 0; y < grid.sqrt_size; y++) {
                const int i = grid.GetIndex(x, y);
                auto &cell = grid.GetCell(i);

                const int x_start = cropper.offset.x + x*cropper.spacing.x;
                const int y_start = cropper.offset.y + y*cropper.spacing.y;
                const auto size = cropper.size;

                const int x_rescale = (int)(xscale * (float)x_start);
                const int y_rescale = (int)(yscale * (float)y_start);
                const int width_rescale = (int)(xscale * (float)size.x);
                const int height_rescale = (int)(yscale * (float)size.y);

                DrawRectInBuffer(
                    x_rescale, y_rescale, x_rescale+width_rescale, y_rescale+height_rescale,
                    pen_color, pen_width, 
                    buffer, width, height, row_stride);
            }
        }
    };

    // render boxes of croppers
    const RGBA<uint8_t> bonus_color         = {255,0,0,255};
    const RGBA<uint8_t> characters_color    = {0,255,0,255};
    const RGBA<uint8_t> values_color        = {0,0,255,255};
    RenderGrid(
        dst_buffer, buffer_size.x, buffer_size.y, dst_row_stride, 
        m_params->cropper_bonuses, m_params->grid,
        bonus_color);
    RenderGrid(
        dst_buffer, buffer_size.x, buffer_size.y, dst_row_stride, 
        m_params->cropper_values, m_params->grid,
        values_color);
    RenderGrid(
        dst_buffer, buffer_size.x, buffer_size.y, dst_row_stride, 
        m_params->cropper_characters, m_params->grid,
        characters_color);
}

void App::UpdateModelTexture() {
    auto &m = m_model->m_model_characters;
    auto buffer_size = m->GetInputSize();
    RGBA<uint8_t> *buffer = m->GetResizeBuffer();

    auto texture_context = m_resize_texture->CreateEditContext();
    auto dst_buffer = texture_context.GetBuffer();
    const int row_stride = texture_context.GetRowStride();

    RGBA<uint8_t> *src_buffer = (RGBA<uint8_t> *)(buffer);
    for (int x = 0; x < buffer_size.x; x++) {
        for (int y = 0; y < buffer_size.y; y++) {
            int i = x + y*row_stride;
            int j = x + (buffer_size.y-y-1)*buffer_size.x;
            dst_buffer[i] = src_buffer[j];
        }
    }
}

