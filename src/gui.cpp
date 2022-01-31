#include "gui.h"
#include "util/AutoGui.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


#include <algorithm>

static void RenderControls(App &app);
static void RenderStatistics(App &app);

static void RenderModelView(App &app);
static void RenderScreenImage(App &app);
static void RenderModelImage(App &app);

void RenderApp(App &app) {
    app.Update();
    RenderControls(app);
    RenderStatistics(app);
    RenderModelView(app);
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

    static int screen_width = app.m_screen_width;
    static int screen_height = app.m_screen_height;
    ImGui::DragInt("screen width", &screen_width, 1, 0, screen_size.width);
    ImGui::DragInt("screen height", &screen_height, 1, 0, screen_size.height);
    if ((screen_width != app.m_screen_width) || 
        (screen_height != app.m_screen_height)) 
    {
        if (ImGui::Button("Submit changes")) {
            app.SetScreenshotSize(screen_width, screen_height);
        }
    }

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
    // {
    //     bool v = false;
    //     v = ImGui::DragInt("Top", &screen_pos.top, 1, 0, screen_size.height) || v;
    //     v = ImGui::DragInt("Left", &screen_pos.left, 1, 0, screen_size.width) || v;
    //     if (v) {
    //         player_controller->SetPosition(screen_pos.top, screen_pos.left);
    //     }
    // }

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
    // auto screen_pos = player_controller->GetPosition();

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
            // screen_pos.top   = std::clamp(screen_pos.top  - (int)(delta.y), 0, screen_size.height);
            // screen_pos.left  = std::clamp(screen_pos.left - (int)(delta.x), 0, screen_size.width);
            // player_controller->SetPosition(screen_pos.top, screen_pos.left);
        }
    }
    last_mouse_pos = ImGui::GetMousePos();
}

void RenderModelImage(App &app) {
    ImGui::Text("Placeholder model texture");
}