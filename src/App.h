#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>

#include <memory>

#include "util/MSS.h"

class App
{
private:
    struct TextureWrapper {
        ID3D11ShaderResourceView* view;
        ID3D11Texture2D* texture;
        int width;
        int height;
    };
public:
    std::shared_ptr<util::MSS> m_mss;
    
    bool m_is_render_running;
    
    TextureWrapper m_screenshot_texture;
    int m_screen_width;
    int m_screen_height;

    ID3D11Device *m_dx11_device; 
    ID3D11DeviceContext *m_dx11_context;
public:
    App(ID3D11Device *dx11_device, ID3D11DeviceContext *dx11_context);
    void Update();
    void UpdateScreenshotTexture();
    void UpdateModelTexture();
    void SetScreenshotSize(const int width, const int height);
private:
    TextureWrapper CreateTexture(const int width, const int height);
};

