#include "App.h"
#include "util/AutoGui.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// TODO: make this dpi scaling aware
// doesn't work when our scaling factor is different (the screen coordinates change)
wordblitz::Cursor MapCursorToScreen(
    const int cx, const int cy, 
    const int top, const int left,
    const float xscale, const float yscale,
    GridCropper &grid)
{
    int gx = grid.offset.x + cx*grid.spacing.x + grid.size.x/2;
    int gy = grid.offset.y + cy*grid.spacing.y + grid.size.y/2;

    int dx = left + static_cast<int>(xscale * (float)gx);
    int dy = top  + static_cast<int>(yscale * (float)gy);

    return {dx, dy};
}

void App::TracerThread() {
    while (m_is_tracer_thread_alive) {
        // tracing active
        if (m_is_tracing) {
            auto lock = std::shared_lock(m_traces_mutex);
            
            bool is_first_trace = true;
            for (auto &trace: m_traces) {
                if (!m_is_tracing) {
                    break;
                }
                if (trace.status == wordblitz::TraceStatus::COMPLETE) {
                    continue;
                }
                // if trace is incomplete or in progress, we continue off with it
                const int path_length = static_cast<int>(trace.path.size());
                int curr_length = 0;

                // trace our path
                for (auto &cursor: trace.path) {
                    const float xscale = (float)m_screen_width/(float)m_params->inter_buffer_size.x;
                    const float yscale = (float)m_screen_height/(float)m_params->inter_buffer_size.y;

                    auto screen_pos = MapCursorToScreen(
                        cursor.x, cursor.y, 
                        m_capture_position.top, m_capture_position.left, 
                        xscale, yscale,
                        m_params->cropper_characters);

                    if (!m_is_tracing) {
                        break;
                    }

                    util::SetCursorPosition(screen_pos.x, screen_pos.y);

                    // first character should drag
                    if (curr_length == 0) {
                        trace.status = wordblitz::TraceStatus::IN_PROGRESS;
                        util::SendMouseEvent(MOUSEEVENTF_LEFTDOWN, screen_pos.x, screen_pos.y);
                        // on the first trace, we delay for the application to catch up
                        if (is_first_trace) {
                            is_first_trace = false;
                            Sleep(300);
                        }
                    } else if (curr_length == path_length-1) {
                        util::SendMouseEvent(MOUSEEVENTF_LEFTUP, screen_pos.x, screen_pos.y);
                        trace.status = wordblitz::TraceStatus::COMPLETE;
                    }
                    Sleep(m_tracer_speed_ms);
                    curr_length++;
                }
            }
            // once done, lower the trace flag
            m_is_tracing = false;
        }
        Sleep(30);
    }
}