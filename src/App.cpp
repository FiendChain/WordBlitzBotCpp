#include "App.h"

#include <algorithm>
#include <memory>

#include "model.h"
#include "buffer_graphics.h"
#include "util/MSS.h"
#include "util/AutoGui.h"
#include "util/KeyListener.h"

App::App(ID3D11Device *dx11_device, ID3D11DeviceContext *dx11_context)
{
    m_mss = std::make_shared<util::MSS>();
    m_params = std::make_shared<AppParams>(4);
    {
        auto model_bonuses      = std::make_unique<BonusesModel>    ("assets/models/bonuses.tflite");
        auto model_characters   = std::make_unique<CharacterModel>  ("assets/models/characters.tflite");
        auto model_values       = std::make_unique<ValuesModel>     ("assets/models/two_digit_classifier.tflite");
        m_model = std::make_shared<UnifiedModel>(
            model_bonuses,
            model_characters,
            model_values,
            m_params,
            m_mss);
    }
    {
        auto &p = *m_params;
        p.cropper_bonuses = {
            {13,34},
            {28,20},
            {107,107}
        };
        p.cropper_characters = {
            {41,61},
            {45, 51},
            {107,107}
        };
        p.cropper_values = {
            {82,48},
            {23,23},
            {107,107}
        };
    }

    m_dx11_device = dx11_device;
    m_dx11_context = dx11_context;

    // setup screen shotter
    m_capture_position = {435,544};
    SetScreenshotSize(512, 512);

    {
        auto &m = m_model->m_model_characters;
        auto resize_buffer_size = m->GetInputSize();
        m_resize_texture = CreateTexture(resize_buffer_size.x, resize_buffer_size.y);
    }

    m_is_render_running = true;

    // create application bindings
    util::InitGlobalListener();

    // util::AttachKeyboardListener(VK_F1, [this](WPARAM type) {
    //     if (type == WM_KEYDOWN) {
    //         bool v = m_player->GetIsRunning();
    //         m_player->SetIsRunning(!v);
    //     }
    // });
}

void App::SetScreenshotSize(const int width, const int height) {
    m_screen_width = width;
    m_screen_height = height;
    m_mss->SetSize(width, height);
    m_screenshot_texture = CreateTexture(width, height);
}

void App::Update() {
    m_mss->Grab(m_capture_position.top, m_capture_position.left);
}

App::TextureWrapper App::CreateTexture(const int width, const int height) {
    App::TextureWrapper wrapper;
    wrapper.texture = NULL;
    wrapper.view = NULL;
    wrapper.width = width;
    wrapper.height = height;

    // Create texture
    // We are creating a dynamic texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    auto status = m_dx11_device->CreateTexture2D(&desc, NULL, &wrapper.texture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    m_dx11_device->CreateShaderResourceView(wrapper.texture, &srvDesc, &wrapper.view);

    return wrapper;
}


void App::UpdateScreenshotTexture() {
    auto bitmap = m_mss->GetBitmap();
    auto buffer_size = bitmap.GetSize();
    auto buffer_max_size = m_mss->GetMaxSize();
    DIBSECTION &sec = bitmap.GetBitmap();
    BITMAP &bmp = sec.dsBm;
    uint8_t *buffer = (uint8_t*)(bmp.bmBits);

    // setup dx11 to modify texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    const UINT subresource = 0;
    m_dx11_context->Map(m_screenshot_texture.texture, subresource, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    // update texture from screen shotter
    int row_width = mappedResource.RowPitch / 4;

    RGBA<uint8_t> *dst_buffer = (RGBA<uint8_t> *)(mappedResource.pData);
    RGBA<uint8_t> *src_buffer = (RGBA<uint8_t> *)(buffer);

    for (int x = 0; x < buffer_size.x; x++) {
        for (int y = 0; y < buffer_size.y; y++) {
            int i = x + y*row_width;
            int j = x + (buffer_max_size.y-y-1)*buffer_max_size.x;
            dst_buffer[i] = src_buffer[j];
        }
    }

    auto &p = *m_params;
    auto RenderGrid = [&p, &dst_buffer, &buffer_size, &row_width](GridCropper &cropper, const RGBA<uint8_t> &pen_color, const int pen_width=1) {
        for (int x = 0; x < p.sqrt_grid_size; x++) {
            for (int y = 0; y < p.sqrt_grid_size; y++) {
                const int i = p.grid.GetIndex(x, y);
                auto &cell = p.grid.GetCell(i);

                const int x_start = cropper.offset.x + x*cropper.spacing.x;
                const int y_start = cropper.offset.y + y*cropper.spacing.y;
                const auto size = cropper.size;

                DrawRectInBuffer(
                    x_start, y_start, x_start+size.x, y_start+size.y, 
                    pen_color, pen_width, 
                    dst_buffer, buffer_size.x, buffer_size.y, row_width);
            }
        }
    };

    // render boxes of croppers
    const RGBA<uint8_t> bonus_color         = {255,0,0,255};
    const RGBA<uint8_t> characters_color    = {0,255,0,255};
    const RGBA<uint8_t> values_color        = {0,0,255,255};
    RenderGrid(p.cropper_bonuses, bonus_color);
    RenderGrid(p.cropper_characters, characters_color);
    RenderGrid(p.cropper_values, values_color);

    m_dx11_context->Unmap(m_screenshot_texture.texture, subresource);
}

void App::UpdateModelTexture() {
    auto &m = m_model->m_model_characters;
    auto buffer_size = m->GetInputSize();
    RGBA<uint8_t> *buffer = m->GetResizeBuffer();

    // setup dx11 to modify texture
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    const UINT subresource = 0;
    m_dx11_context->Map(m_resize_texture.texture, subresource, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

    // update texture from screen shotter
    int row_width = mappedResource.RowPitch / 4;

    RGBA<uint8_t> *dst_buffer = (RGBA<uint8_t> *)(mappedResource.pData);
    RGBA<uint8_t> *src_buffer = (RGBA<uint8_t> *)(buffer);

    for (int x = 0; x < buffer_size.x; x++) {
        for (int y = 0; y < buffer_size.y; y++) {
            int i = x + y*row_width;
            int j = x + (buffer_size.y-y-1)*buffer_size.x;
            dst_buffer[i] = src_buffer[j];
        }
    }

    m_dx11_context->Unmap(m_resize_texture.texture, subresource);   
}