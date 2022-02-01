#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>

#include <memory>
#include <list>
#include <shared_mutex>
#include <thread>

#include "Texture.h"
#include "util/MSS.h"
#include "unified_model.h"
#include "wordblitz.h"
#include "wordtree.h"
#include "buffer_graphics.h"

typedef std::list<std::string> ErrorList;

class App
{
private:
    struct Position {
        int top;
        int left;
    };
    struct Vec2D {
        int x;
        int y;
    };
private:
    ID3D11Device *m_dx11_device; 
    ID3D11DeviceContext *m_dx11_context;
    wordtree::NodePool m_node_pool; 
    std::shared_ptr<UnifiedModel> m_model;
    std::shared_ptr<util::MSS> m_mss;
    std::shared_ptr<AppParams> m_params;

    // traces
    std::vector<wordblitz::TraceResult> m_traces;
    std::shared_mutex m_traces_mutex;
    bool m_is_tracing;
    bool m_is_tracer_thread_alive;
    int m_tracer_speed_ms;
    std::unique_ptr<std::thread> m_tracer_thread;
    
    Position m_capture_position;
    std::unique_ptr<Texture> m_screenshot_texture;
    int m_screen_width;
    int m_screen_height;

    std::unique_ptr<Texture> m_inter_texture;
    RGBA<uint8_t> *m_inter_buffer;
    int m_inter_width;
    int m_inter_height;

    std::unique_ptr<Texture> m_resize_texture;

    bool m_is_render_running;
    bool m_is_grabbing;
    ErrorList m_errors;
public:
    App(ID3D11Device *dx11_device, ID3D11DeviceContext *dx11_context);
    ~App();
    void Update();
    void UpdateTraces();
    void ReadScreen();

    void UpdateScreenshotTexture();
    void UpdateInterTexture();
    void UpdateModelTexture();

    inline bool GetIsRendering() const { return m_is_render_running; }
    inline void SetIsRendering(const bool v) { m_is_render_running = v; } 
    inline bool GetIsGrabbing() const { return m_is_grabbing; }
    inline void SetIsGrabbing(const bool v) { m_is_grabbing = v; } 

    inline util::MSS& GetMSS() { return *m_mss; }

    inline AppParams& GetParams() { return *m_params; }
    void SetScreenshotSize(const int width, const int height);
    inline Texture& GetScreenshotTexture() { return *m_screenshot_texture; }
    inline Vec2D GetScreenshotSize() const { 
        return {m_screen_width, m_screen_height}; 
    }
    inline Position& GetCapturePosition() { return m_capture_position; }

    inline Texture& GetInterTexture() { return *m_inter_texture; }
    inline Vec2D GetInterSize() const { return {m_inter_width, m_inter_height}; }
    void ApplyInterbufferSize();

    inline Texture& GetModelTexture() { return *m_resize_texture; }

    inline std::vector<wordblitz::TraceResult>& GetTraces() { return m_traces; }
    std::shared_mutex &GetTraceMutex() { return m_traces_mutex; }
    inline int GetTracerSpeedMillis() const { return m_tracer_speed_ms; }
    inline void SetTracerSpeedMillis(const int v) { m_tracer_speed_ms = v; }
    inline bool GetIsTracing() const { return m_is_tracing; }
    inline void SetIsTracing(const bool v) { m_is_tracing = v; }

    inline int GetNodePoolSize() { 
        return static_cast<int>(m_node_pool.size()); 
    }
    inline int GetNodePoolCapacity() { 
        return static_cast<int>(m_node_pool.capacity()); 
    }

    inline ErrorList &GetErrorList() { return m_errors; }
private:
    void GrabScreen();
    void TracerThread();
};

