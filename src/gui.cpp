#include "gui.h"
#include "util/AutoGui.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"


#include <algorithm>
#include <fmt/core.h>
#include <ctype.h>

class ImageZoomGizmo 
{
private:
    bool m_is_drag_enabled;
    ImVec2 m_prev_mouse_pos;
public:
    ImageZoomGizmo() {
        m_is_drag_enabled = false;
        m_prev_mouse_pos = ImGui::GetMousePos();
    }

    bool UpdateOnItem(
        int& width, int& height,
        const int min_x, const int max_x, const int min_y, const int max_y)
    {
        bool is_changed = false;
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left, 10.0f)) {
            m_is_drag_enabled = true;
        }

        ImGuiIO& io = ImGui::GetIO();
        int mouse_wheel = static_cast<int>(io.MouseWheel);
        if (mouse_wheel != 0) {
            int delta = mouse_wheel*mouse_wheel*mouse_wheel;
            width = std::clamp(width+delta, min_x, max_x);
            height = std::clamp(height+delta, min_y, max_y);
            is_changed = true;
        } 

        return is_changed;
    }

    bool UpdateAlways(
        int& top, int& left, 
        const int min_x, const int max_x, const int min_y, const int max_y) 
    {
        bool is_changed = false;
        if (m_is_drag_enabled) {
            if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                m_is_drag_enabled = false;
            } else {
                auto new_pos = ImGui::GetMousePos();
                ImVec2 delta;
                delta.x = new_pos.x - m_prev_mouse_pos.x;
                delta.y = new_pos.y - m_prev_mouse_pos.y;
                left = std::clamp(left - (int)(delta.x), min_x, max_x);
                top = std::clamp(top - (int)(delta.y), min_y, max_y);
                is_changed = true;
            }
        }

        m_prev_mouse_pos = ImGui::GetMousePos();
        return is_changed;
    }
};

// our gui components
static void RenderQuickControls(App &app);
static void RenderConfig(App &app);
static void RenderBuffers(App &app);
static void RenderGridData(App &app);
static void RenderTraces(App &app);
static void RenderErrorList(App &app);
static void RenderStatistics(App &app);

static void RenderInterImage(App &app);
static void RenderScreenImage(App &app);
static void RenderModelImage(App &app);
static void RenderGridCropper(App &app, GridCropper &cropper, const int width, const int height, const char *id);


// all gui components
void RenderApp(App &app) {
    app.Update();
    RenderConfig(app);
    RenderStatistics(app);
    RenderBuffers(app);
    RenderGridData(app);
    RenderTraces(app);
    RenderErrorList(app);
}

void RenderConfig(App &app) {
    const auto screen_size = util::GetScreenSize();

    ImGui::Begin("Config");
    {
        auto &pos = app.GetCapturePosition();
        ImGui::DragInt("Top", &pos.top, 1, 0, screen_size.height);
        ImGui::DragInt("Left", &pos.left, 1, 0, screen_size.width);
    }

    {
        auto size = app.GetScreenshotSize();
        bool is_changed = false;
        is_changed = ImGui::DragInt("screen width", &size.x, 1, 0, screen_size.width) || is_changed;
        is_changed = ImGui::DragInt("screen height", &size.y, 1, 0, screen_size.height) || is_changed;
        if (is_changed) {
            app.SetScreenshotSize(size.x, size.y);
        }
    }

    auto &p = app.GetParams();
    static auto previous_size = p.inter_buffer_size;
    bool is_changed = 
        (previous_size.x != p.inter_buffer_size.x) || 
        (previous_size.y != p.inter_buffer_size.y);

    // render the controls
    ImGui::Separator();
    ImGui::Text("Intermediate buffer");
    ImGui::DragInt("width", &p.inter_buffer_size.x, 1, 0, screen_size.width);
    ImGui::DragInt("height", &p.inter_buffer_size.y, 1, 0, screen_size.height);
    if (is_changed && ImGui::Button("Apply changes")) {
        app.ApplyInterbufferSize();
        previous_size = p.inter_buffer_size;
    }
    {
        auto size = app.GetScreenshotSize();
        ImGui::Text("Bonuses");
        RenderGridCropper(app, p.cropper_bonuses, size.x, size.y, "##bonus_cropper");
        ImGui::Text("Characters");
        RenderGridCropper(app, p.cropper_characters, size.x, size.y, "##characters_cropper");
        ImGui::Text("Values");
        RenderGridCropper(app, p.cropper_values, size.x, size.y, "##values_cropper");
    }

    ImGui::End(); 
}

void RenderStatistics(App &app) {
    ImGui::Begin("Statistics");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Separator();
    {
        auto &texture = app.GetScreenshotTexture(); 
        ImGui::Text("pointer = %p", texture.GetView());
        ImGui::Text("texture = %d x %d", texture.GetWidth(), texture.GetHeight());
    }
    {
        auto size = app.GetMSS().GetMaxSize();
        ImGui::Text("max_screenshot_size = %d x %d", size.x, size.y);
    }
    ImGui::Separator();
    ImGui::Text("node pool size = %d", app.GetNodePoolSize());
    ImGui::Text("node pool capacity = %d", app.GetNodePoolCapacity());

    ImGui::End();
}

void RenderBuffers(App &app) {
    const auto screen_size = util::GetScreenSize();

    ImGui::Begin("Render");
    if (ImGui::BeginTabBar("Views")) {
        if (ImGui::BeginTabItem("Inter view")) {
            RenderInterImage(app);
            ImGui::EndTabItem();
        }
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
    const auto screen_size = util::GetScreenSize();
    static auto scroller = ImageZoomGizmo();

    ImGui::DragFloat("Scale", &zoom_scale, 0.001f, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    if (app.GetIsRendering()) app.UpdateScreenshotTexture();

    auto &texture = app.GetScreenshotTexture();
    ImGui::Image(
        (void*)texture.GetView(), 
        ImVec2(texture.GetWidth()*zoom_scale, texture.GetHeight()*zoom_scale));

    if (ImGui::IsItemHovered()) {
        auto size = app.GetScreenshotSize();
        if (scroller.UpdateOnItem(
            size.x, size.y, 
            0, screen_size.width, 0, screen_size.height))
        {
            app.SetScreenshotSize(size.x, size.y);
        }
    } 

    auto& pos = app.GetCapturePosition();
    scroller.UpdateAlways(
        pos.top, pos.left,
        0, screen_size.width, 0, screen_size.height);
}

void RenderInterImage(App &app) {
    static float zoom_scale = 1.0f;
    const auto screen_size = util::GetScreenSize();
    static auto scroller = ImageZoomGizmo();

    ImGui::DragFloat("Scale", &zoom_scale, 0.001f, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    if (app.GetIsRendering()) app.UpdateInterTexture();

    auto &texture = app.GetInterTexture();
    ImGui::Image(
        (void*)texture.GetView(), 
        ImVec2(texture.GetWidth()*zoom_scale, texture.GetHeight()*zoom_scale));

    if (ImGui::IsItemHovered()) {
        auto size = app.GetScreenshotSize();
        if (scroller.UpdateOnItem(
            size.x, size.y, 
            0, screen_size.width, 0, screen_size.height))
        {
            app.SetScreenshotSize(size.x, size.y);
        }
    } 

    auto& pos = app.GetCapturePosition();
    scroller.UpdateAlways(
        pos.top, pos.left,
        0, screen_size.width, 0, screen_size.height);
}

void RenderModelImage(App &app) {
    static float zoom_scale = 10.0f;
    const auto screen_size = util::GetScreenSize();
    static auto scroller = ImageZoomGizmo();

    ImGui::DragFloat("Scale", &zoom_scale, 0.001f, 0.1f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
    if (app.GetIsRendering()) app.UpdateModelTexture();

    auto &texture = app.GetModelTexture();
    ImGui::Image(
        (void*)texture.GetView(), 
        ImVec2(texture.GetWidth()*zoom_scale, texture.GetHeight()*zoom_scale));

    if (ImGui::IsItemHovered()) {
        auto size = app.GetScreenshotSize();
        if (scroller.UpdateOnItem(
            size.x, size.y, 
            0, screen_size.width, 0, screen_size.height))
        {
            app.SetScreenshotSize(size.x, size.y);
        }
    } 

    auto& pos = app.GetCapturePosition();
    scroller.UpdateAlways(
        pos.top, pos.left,
        0, screen_size.width, 0, screen_size.height);
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
    auto &g = app.GetParams().grid;

    ImGui::Begin("Grid");
    RenderQuickControls(app);

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
                ImGui::Text("%c", toupper(cell.c));

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

struct TraceCount {
    int incomplete = 0;
    int complete = 0;
    int in_progress = 0;
};

void RenderTraces(App &app) {
    auto lock = std::shared_lock(app.GetTraceMutex());
    auto &traces = app.GetTraces();
    static TraceCount counts;

    auto label = fmt::format(
        "Traces ({}/{})###traces_window", 
        counts.complete, traces.size());
    ImGui::Begin(label.c_str());

    RenderQuickControls(app);

    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;
    ImGui::BeginChild("##traces_list");
    if (ImGui::BeginTable("Traces", 2, flags)) {
        ImGui::TableSetupColumn("Word",  ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Score", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        TraceCount new_counts;

        for (auto &t: traces) {
            ImU32 row_color;
            switch (t.status) {
            case wordblitz::TraceStatus::COMPLETE: 
                new_counts.complete++;
                row_color = ImGui::GetColorU32(ImVec4(0.0f, 1.0f, 0.0f, 0.35f));
                break;
            case wordblitz::TraceStatus::IN_PROGRESS: 
                new_counts.in_progress++;
                row_color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 1.0f, 0.35f));
                break;
            case wordblitz::TraceStatus::INCOMPLETE: 
                new_counts.incomplete++;
                row_color = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 0.0f, 0.35f));
                break;
            }
            ImGui::TableNextRow();
            ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, row_color);

            ImGui::TableNextColumn();
            ImGui::Text(t.word.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%d", t.value);
            // TODO: add color for status indicator
        }

        counts = new_counts;
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::End();
}

void RenderQuickControls(App &app) {
    ImGui::BeginGroup();
    {
        bool v = app.GetIsRendering();
        if (ImGui::Checkbox("Is render (F2)", &v)) {
            app.SetIsRendering(v);
        }
    }
    {
        bool v = app.GetIsGrabbing();
        if (ImGui::Checkbox("Is grab (F3)", &v)) {
            app.SetIsGrabbing(v);
        }
    }
    {
        bool v = app.GetIsTracing();
        if (ImGui::Checkbox("Is tracing (F4)", &v)) {
            app.SetIsTracing(v);
        }
    }
    {
        int ms_tracer = app.GetTracerSpeedMillis();
        ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp | ImGuiSliderFlags_ClampOnInput;
        if (ImGui::DragInt("Trace speed (ms)", &ms_tracer, 1, 10, 1000, "%d", flags)) {
            app.SetTracerSpeedMillis(ms_tracer);
        }
    }

    if (ImGui::Button("Read")) {
        app.ReadScreen();
    }
    ImGui::SameLine();
    if (ImGui::Button("Calculate")) {
        app.UpdateTraces();
    }
    ImGui::SameLine();
    if (ImGui::Button("Trace")) {
        app.SetIsTracing(true);
    }
    ImGui::EndGroup();
}

void RenderErrorList(App &app) {
    auto &errors = app.GetErrorList();
    auto it = errors.begin();

    ImGui::Begin("Errors");
    if (errors.size() == 0) {
        ImGui::Text("No errors");
    }

    int id = 0;
    while (it != errors.end()) {
        ImGui::PushID(id++);
        ImGui::BeginGroup();
        bool is_delete = ImGui::Button("X");
        ImGui::SameLine();
        ImGui::TextWrapped((*it).c_str());
        ImGui::EndGroup();

        if (is_delete) {
            it = errors.erase(it);
        } else {
            it++;
        }
        ImGui::PopID();
    }
    ImGui::End();
}