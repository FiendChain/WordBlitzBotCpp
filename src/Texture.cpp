#include "Texture.h"

Texture::Texture(
    ID3D11Device* dx11_device, ID3D11DeviceContext* dx11_context,
    int width, int height) 
: m_dx11_device(dx11_device), m_dx11_context(dx11_context)
{
    m_width = width;
    m_height = height;

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

    auto status = m_dx11_device->CreateTexture2D(&desc, NULL, &m_texture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    m_dx11_device->CreateShaderResourceView(m_texture, &srvDesc, &m_view);
}

TextureEditContext Texture::CreateEditContext() {
    return TextureEditContext(this);
}

Texture::~Texture() {
    auto lock = std::unique_lock(m_mutex);
    m_view->Release();
    m_texture->Release();
}

TextureEditContext::TextureEditContext(Texture *texture) 
: m_lock(texture->m_mutex)
{
    m_texture = texture;
    m_subresource = 0;
    m_texture->m_dx11_context->Map(
        m_texture->GetTexture(), m_subresource, 
        D3D11_MAP_WRITE_DISCARD, 0, &m_mapped_resource);
    
    m_row_stride = m_mapped_resource.RowPitch / 4;
}

TextureEditContext::~TextureEditContext() {
    m_texture->m_dx11_context->Unmap(m_texture->GetTexture(), m_subresource);   
}

TextureEditContext::Vec2D TextureEditContext::GetBufferSize() {
    auto s = m_texture->GetSize();
    return {s.x, s.y};
}

RGBA<uint8_t> *TextureEditContext::GetBuffer() {
    return (RGBA<uint8_t>*)(m_mapped_resource.pData);
}