#include "gui.h"
#include "util/AutoGui.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


#include <algorithm>
#include <fmt/core.h>

static void RenderControls(App &app);
static void RenderConfig(App &app);
static void RenderStatistics(App &app);

static void RenderModelView(App &app);
static void RenderScreenImage(App &app);
static void RenderModelImage(App &app);

static void RenderGridCropper(App &app, GridCropper &cropper, const int width, const int height, const char *id);
static void RenderGridData(App &app);

void RenderApp(App &app) {
    app.Update();
    RenderControls(app);
    RenderConfig(app);
    RenderStatistics(app);
    RenderModelView(app);
    RenderGridData(app);
}

void RenderControls(App &app) {
    const auto screen_size = util::GetScreenSize();

    ImGui::Begin("Controls");

    ImGui::Checkbox("Is render (F2)", &app.m_is_render_running);

    auto &ss_texture = app.m_screenshot_texture; 
    ImGui::Text("pointer = %p", ss_texture.view);
    ImGui::Text("texture             = %d x %d", ss_texture.width, ss_texture.height);

    auto max_buffer_size = app.m_mss->GetMaxSize();
    ImGui::Text("max_screenshot_size = %d x %d", max_buffer_size.x, max_buffer_size.y);

    ImGui::Separator();
    if (ImGui::Button("Calculate")) {
        app.m_model->Update();
    }

    ImGui::End();
    
}

void RenderConfig(App &app) {
    const auto screen_size = util::GetScreenSize();
    static int screen_width = app.m_screen_width;
    static int screen_height = app.m_screen_height;

    ImGui::Begin("Config");
    ImGui::DragInt("Top", &app.m_capture_position.top, 1, 0, screen_size.height);
    ImGui::DragInt("Left", &app.m_capture_position.left, 1, 0, screen_size.width);
    ImGui::DragInt("screen width", &screen_width, 1, 0, screen_size.width);
    ImGui::DragInt("screen height", &screen_height, 1, 0, screen_size.height);
    if ((screen_width != app.m_screen_width) || 
        (screen_height != app.m_screen_height)) 
    {
        if (ImGui::Button("Submit changes")) {
            app.SetScreenshotSize(screen_width, screen_height);
        }
    }

    // render the controls
    ImGui::Separator();
    auto &p = *(app.m_params);
    ImGui::Text("Bonuses");
    RenderGridCropper(app, p.cropper_bonuses, app.m_screen_width, app.m_screen_height, "##bonus_cropper");
    ImGui::Text("Characters");
    RenderGridCropper(app, p.cropper_characters, app.m_screen_width, app.m_screen_height, "##characters_cropper");
    ImGui::Text("Values");
    RenderGridCropper(app, p.cropper_values, app.m_screen_width, app.m_screen_height, "##values_cropper");

    ImGui::End(); 
}

void RenderStatistics(App &app) {
    ImGui::Begin("Statistics");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}



void RenderModelView(App &app) {
    const auto screen_size = util::GetScreenSize();

    ImGui::Begin("Render");
    if (ImGui::BeginTabBar("Views")) {
        if (ImGui::BeginTabItem("Screen view")) {
            RenderScreenImage(app);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Model view")) {
            RenderModelImage(app);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
} 

void RenderScreenImage(App &app) {
    static float zoom_scale = 1.0f;
    static auto last_mouse_pos = ImGui::GetMousePos();
    static auto is_drag_enabled = false;
    const auto screen_size = util::GetScreenSize();

    ImGui::DragFloat("Scale", &zoom_scale, 0.001f, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    if (app.m_is_render_running) app.UpdateScreenshotTexture();

    auto &texture = app.m_screenshot_texture;
    ImGui::Image(
        (void*)texture.view, 
        ImVec2(texture.width*zoom_scale, texture.height*zoom_scale));

    // create dragging controls for the image
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 10.0f)) {
            is_drag_enabled = true;
        }
    } 
    
    if (is_drag_enabled) {
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            is_drag_enabled = false;
        } else {
            auto latest_pos = ImGui::GetMousePos();
            ImVec2 delta;
            delta.x = latest_pos.x - last_mouse_pos.x;
            delta.y = latest_pos.y - last_mouse_pos.y;
            auto &p = app.m_capture_position;
            p.top   = std::clamp(p.top  - (int)(delta.y), 0, screen_size.height);
            p.left  = std::clamp(p.left - (int)(delta.x), 0, screen_size.width);
        }
    }
    last_mouse_pos = ImGui::GetMousePos();
}

void RenderModelImage(App &app) {
    static float zoom_scale = 10.0f;
    static auto last_mouse_pos = ImGui::GetMousePos();
    static auto is_drag_enabled = false;
    const auto screen_size = util::GetScreenSize();

    ImGui::DragFloat("Scale", &zoom_scale, 0.001f, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    if (app.m_is_render_running) app.UpdateModelTexture();

    auto &texture = app.m_resize_texture;
    ImGui::Image(
        (void*)texture.view, 
        ImVec2(texture.width*zoom_scale, texture.height*zoom_scale));

    // create dragging controls for the image
    if (ImGui::IsItemHovered()) {
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 10.0f)) {
            is_drag_enabled = true;
        }
    } 
    
    if (is_drag_enabled) {
        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            is_drag_enabled = false;
        } else {
            auto latest_pos = ImGui::GetMousePos();
            ImVec2 delta;
            delta.x = latest_pos.x - last_mouse_pos.x;
            delta.y = latest_pos.y - last_mouse_pos.y;
            auto &p = app.m_capture_position;
            p.top   = std::clamp(p.top  - (int)(delta.y), 0, screen_size.height);
            p.left  = std::clamp(p.left - (int)(delta.x), 0, screen_size.width);
        }
    }
    last_mouse_pos = ImGui::GetMousePos();
}

void RenderGridCropper(App &app, GridCropper &cropper, const int width, const int height, const char *label) {
    ImGui::PushID(label);
    ImGui::BeginGroup();
    const ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ClampOnInput;
    ImGui::DragInt2("Offset",   (int*)(&cropper.offset), 1, 0, height, "%d", flags);
    ImGui::DragInt2("Size",     (int*)(&cropper.size), 1, 0, height, "%d", flags);
    ImGui::DragInt2("Spacing",  (int*)(&cropper.spacing), 1, 0, height, "%d", flags);

    ImGui::EndGroup();
    ImGui::PopID();
}

void RenderGridData(App &app) {
    auto &g = app.m_params->grid;

    ImGui::Begin("Grid");
    if (ImGui::Button("Calculate")) {
        app.m_model->Update();
    }

    ImGui::Separator();

    const auto GetModifierLabel = [](wordblitz::CellModifier m) {
        switch (m) {
        case wordblitz::CellModifier::MOD_2L:   return "2L";
        case wordblitz::CellModifier::MOD_2W:   return "2W";
        case wordblitz::CellModifier::MOD_3L:   return "3L";
        case wordblitz::CellModifier::MOD_3W:   return "3W";
        case wordblitz::CellModifier::MOD_NONE: 
        default:
            return " ";
        }
    };

    constexpr int num_bonuses = 5;
    const wordblitz::CellModifier all_bonuses[num_bonuses] = {
        wordblitz::CellModifier::MOD_NONE,
        wordblitz::CellModifier::MOD_2L,
        wordblitz::CellModifier::MOD_2W,
        wordblitz::CellModifier::MOD_3L,
        wordblitz::CellModifier::MOD_3W,
    };

    ImGuiTableFlags table_flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg;
    if (ImGui::BeginTable("##grid_data", g.sqrt_size, table_flags)) {
        ImGui::TableNextRow();
        for (int y = 0; y < g.sqrt_size; y++) {
            for (int x = 0; x < g.sqrt_size; x++) {
                const int i = g.GetIndex(x, y);
                auto &cell = g.GetCell(i);

                ImGui::TableNextColumn();
                ImGui::PushID(i);
                ImGui::BeginGroup();

                ImGui::DragInt("##value", &cell.value, 1, 0, 20);
                // ImGui::InputText("##character", &cell.c, 1);
                ImGui::Text("%c", cell.c);

                const char *label = GetModifierLabel(cell.modifier);
                if (ImGui::BeginCombo("##bonuses", label)) {
                    for (int i = 0; i < num_bonuses; i++) {
                        bool is_selected = all_bonuses[i] == cell.modifier;
                        ImGui::PushID(i);
                        if (ImGui::Selectable(GetModifierLabel(cell.modifier), &is_selected)) {
                            cell.modifier = all_bonuses[i];
                        }
                        ImGui::PopID();
                    }
                    
                    ImGui::EndCombo();
                }

                ImGui::EndGroup();
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }


    ImGui::End();
}