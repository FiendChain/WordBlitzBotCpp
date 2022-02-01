#pragma once

#include "buffer_graphics.h"
#include <shared_mutex>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>

class TextureEditContext;

class Texture 
{
public:
    struct Vec2D {
        int x;
        int y;
    };
private:
    ID3D11ShaderResourceView* m_view;
    ID3D11Texture2D* m_texture;

    ID3D11Device* m_dx11_device;
    ID3D11DeviceContext* m_dx11_context;
    int m_width;
    int m_height;

    std::shared_mutex m_mutex;
public:
    Texture(ID3D11Device* dx11_device, ID3D11DeviceContext* dx11_context,
            const int width, const int height);
    ~Texture();
    TextureEditContext CreateEditContext();
    ID3D11Texture2D* GetTexture() { return m_texture; }
    ID3D11ShaderResourceView* GetView() { return m_view; }
    inline Vec2D GetSize() const { return {m_width, m_height}; }
    inline int GetWidth() const { return m_width; }
    inline int GetHeight() const { return m_height; }
private:
    friend class TextureEditContext;
};

class TextureEditContext 
{
public:
    struct Vec2D {
        int x;
        int y;
    };
private:
    D3D11_MAPPED_SUBRESOURCE m_mapped_resource;
    Texture *m_texture;
    int m_subresource;
    int m_row_stride;
    std::shared_lock<std::shared_mutex> m_lock;
private:
    TextureEditContext(Texture *texture);
    friend class Texture;
public:
    ~TextureEditContext();
    RGBA<uint8_t> *GetBuffer();
    Vec2D GetBufferSize();
    inline int GetRowStride() const { return m_row_stride; }

};