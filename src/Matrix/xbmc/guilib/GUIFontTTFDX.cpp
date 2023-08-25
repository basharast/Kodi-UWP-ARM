/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFontTTFDX.h"

#include "GUIFontManager.h"
#include "GUIShaderDX.h"
#include "TextureDX.h"
#include "rendering/dx/DeviceResources.h"
#include "rendering/dx/RenderContext.h"
#include "utils/log.h"

// stuff for freetype
#include <ft2build.h>

using namespace Microsoft::WRL;

#ifdef TARGET_WINDOWS_STORE
#define generic GenericFromFreeTypeLibrary
#endif

#include FT_FREETYPE_H
#include FT_GLYPH_H

CGUIFontTTF* CGUIFontTTF::CreateGUIFontTTF(const std::string& fileName)
{
  return new CGUIFontTTFDX(fileName);
}

CGUIFontTTFDX::CGUIFontTTFDX(const std::string& strFileName) : CGUIFontTTF(strFileName)
{
  m_speedupTexture = nullptr;
  m_vertexBuffer   = nullptr;
  m_vertexWidth    = 0;
  m_buffers.clear();
  DX::Windowing()->Register(this);
}

CGUIFontTTFDX::~CGUIFontTTFDX(void)
{
  DX::Windowing()->Unregister(this);

  if (m_speedupTexture)
  {
    delete m_speedupTexture;
    m_speedupTexture = nullptr;
  }
  m_vertexBuffer = nullptr;
  m_staticIndexBuffer = nullptr;
  if (!m_buffers.empty())
  {
    std::for_each(m_buffers.begin(), m_buffers.end(), [](CD3DBuffer* buf) {
      if (buf) delete buf;
    });
  }
  m_buffers.clear();
  m_staticIndexBufferCreated = false;
  m_vertexWidth = 0;
}

bool CGUIFontTTFDX::FirstBegin()
{
  if (!DX::DeviceResources::Get()->GetD3DContext())
    return false;

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();
  pGUIShader->Begin(SHADER_METHOD_RENDER_FONT);

  return true;
}

void CGUIFontTTFDX::LastEnd()
{
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();
  if (!pContext)
    return;

  typedef CGUIFontTTF::CTranslatedVertices trans;
  bool transIsEmpty = std::all_of(m_vertexTrans.begin(), m_vertexTrans.end(),
                                  [](trans& _) { return _.vertexBuffer->size <= 0; });
  // no chars to render
  if (m_vertex.empty() && transIsEmpty)
    return;

  CreateStaticIndexBuffer();

  unsigned int offset = 0;
  unsigned int stride = sizeof(SVertex);

  CGUIShaderDX* pGUIShader = DX::Windowing()->GetGUIShader();
  // Set font texture as shader resource
  pGUIShader->SetShaderViews(1, m_speedupTexture->GetAddressOfSRV());
  // Enable alpha blend
  DX::Windowing()->SetAlphaBlendEnable(true);
  // Set our static index buffer
  pContext->IASetIndexBuffer(m_staticIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
  // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
  pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  if (!m_vertex.empty())
  {
    // Deal with vertices that had to use software clipping
    if (!UpdateDynamicVertexBuffer(&m_vertex[0], m_vertex.size()))
      return;

    // Set the dynamic vertex buffer to active in the input assembler
    pContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    // Do the actual drawing operation, split into groups of characters no
    // larger than the pre-determined size of the element array
    size_t size = m_vertex.size() / 4;
    for (size_t character = 0; size > character; character += ELEMENT_ARRAY_MAX_CHAR_INDEX)
    {
      size_t count = size - character;
      count = std::min<size_t>(count, ELEMENT_ARRAY_MAX_CHAR_INDEX);

      // 6 indices and 4 vertices per character
      pGUIShader->DrawIndexed(count * 6, 0, character * 4);
    }
  }

  if (!transIsEmpty)
  {
    // Deal with the vertices that can be hardware clipped and therefore translated

    // Store current GPU transform
    XMMATRIX view = pGUIShader->GetView();
    // Store current scissor
    CRect scissor = CServiceBroker::GetWinSystem()->GetGfxContext().StereoCorrection(CServiceBroker::GetWinSystem()->GetGfxContext().GetScissors());

    for (size_t i = 0; i < m_vertexTrans.size(); i++)
    {
      // ignore empty buffers
      if (m_vertexTrans[i].vertexBuffer->size == 0)
        continue;

      // Apply the clip rectangle
      CRect clip = DX::Windowing()->ClipRectToScissorRect(m_vertexTrans[i].clip);
      // Intersect with current scissors
      clip.Intersect(scissor);

      // skip empty clip, a little improvement to not render invisible text
      if (clip.IsEmpty())
        continue;

      DX::Windowing()->SetScissors(clip);

      // Apply the translation to the model view matrix
      XMMATRIX translation = XMMatrixTranslation(m_vertexTrans[i].translateX, m_vertexTrans[i].translateY, m_vertexTrans[i].translateZ);
      pGUIShader->SetView(XMMatrixMultiply(translation, view));

      CD3DBuffer* vbuffer = reinterpret_cast<CD3DBuffer*>(m_vertexTrans[i].vertexBuffer->bufferHandle);
      // Set the static vertex buffer to active in the input assembler
      ID3D11Buffer* buffers[1] = { vbuffer->Get() };
      pContext->IASetVertexBuffers(0, 1, buffers, &stride, &offset);

      // Do the actual drawing operation, split into groups of characters no
      // larger than the pre-determined size of the element array
      for (size_t character = 0; m_vertexTrans[i].vertexBuffer->size > character; character += ELEMENT_ARRAY_MAX_CHAR_INDEX)
      {
        size_t count = m_vertexTrans[i].vertexBuffer->size - character;
        count = std::min<size_t>(count, ELEMENT_ARRAY_MAX_CHAR_INDEX);

        // 6 indices and 4 vertices per character
        pGUIShader->DrawIndexed(count * 6, 0, character * 4);
      }
    }

    // restore scissor
    DX::Windowing()->SetScissors(scissor);

    // Restore the original transform
    pGUIShader->SetView(view);
  }

  pGUIShader->RestoreBuffers();
}

CVertexBuffer CGUIFontTTFDX::CreateVertexBuffer(const std::vector<SVertex> &vertices) const
{
  CD3DBuffer* buffer = nullptr;
  if (!vertices.empty()) // do not create empty buffers, leave buffer as nullptr, it will be ignored on drawing stage
  {
    buffer = new CD3DBuffer();
    if (!buffer->Create(D3D11_BIND_VERTEX_BUFFER, vertices.size(), sizeof(SVertex), DXGI_FORMAT_UNKNOWN, D3D11_USAGE_IMMUTABLE, &vertices[0]))
      CLog::LogF(LOGERROR, "Failed to create vertex buffer.");
    else
      AddReference((CGUIFontTTFDX*)this, buffer);
  }

  return CVertexBuffer(reinterpret_cast<void*>(buffer), vertices.size() / 4, this);
}

void CGUIFontTTFDX::AddReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer)
{
  font->m_buffers.push_back(pBuffer);
}

void CGUIFontTTFDX::DestroyVertexBuffer(CVertexBuffer &buffer) const
{
  if (nullptr != buffer.bufferHandle)
  {
    CD3DBuffer* vbuffer = reinterpret_cast<CD3DBuffer*>(buffer.bufferHandle);
    ClearReference((CGUIFontTTFDX*)this, vbuffer);
    if (vbuffer)
      delete vbuffer;
    buffer.bufferHandle = 0;
  }
}

void CGUIFontTTFDX::ClearReference(CGUIFontTTFDX* font, CD3DBuffer* pBuffer)
{
  std::list<CD3DBuffer*>::iterator it = std::find(font->m_buffers.begin(), font->m_buffers.end(), pBuffer);
  if (it != font->m_buffers.end())
    font->m_buffers.erase(it);
}

CTexture* CGUIFontTTFDX::ReallocTexture(unsigned int& newHeight)
{
  assert(newHeight != 0);
  assert(m_textureWidth != 0);
  if(m_textureHeight == 0)
  {
    delete m_texture;
    m_texture = nullptr;
    delete m_speedupTexture;
    m_speedupTexture = nullptr;
  }
  m_staticCache.Flush();
  m_dynamicCache.Flush();

  CDXTexture* pNewTexture = new CDXTexture(m_textureWidth, newHeight, XB_FMT_A8);
  CD3DTexture* newSpeedupTexture = new CD3DTexture();
  if (!newSpeedupTexture->Create(m_textureWidth, newHeight, 1, D3D11_USAGE_DEFAULT, DXGI_FORMAT_R8_UNORM))
  {
    delete newSpeedupTexture;
    delete pNewTexture;
    return nullptr;
  }

  // There might be data to copy from the previous texture
  if (newSpeedupTexture && m_speedupTexture)
  {
    CD3D11_BOX rect(0, 0, 0, m_textureWidth, m_textureHeight, 1);
    ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
    pContext->CopySubresourceRegion(newSpeedupTexture->Get(), 0, 0, 0, 0, m_speedupTexture->Get(), 0, &rect);
  }

  if (m_texture)
  {
    delete m_texture;
    m_texture = nullptr;
  }
  if (m_speedupTexture)
  {
    delete m_speedupTexture;
    m_speedupTexture = nullptr;
  }
  m_textureHeight = newHeight;
  m_textureScaleY = 1.0f / m_textureHeight;
  m_speedupTexture = newSpeedupTexture;

  return pNewTexture;
}

bool CGUIFontTTFDX::CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2)
{
  FT_Bitmap bitmap = bitGlyph->bitmap;

  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetImmediateContext();
  if (m_speedupTexture && m_speedupTexture->Get() && pContext && bitmap.buffer)
  {
    CD3D11_BOX dstBox(x1, y1, 0, x2, y2, 1);
    pContext->UpdateSubresource(m_speedupTexture->Get(), 0, &dstBox, bitmap.buffer, bitmap.pitch, 0);
    return true;
  }

  return false;
}

void CGUIFontTTFDX::DeleteHardwareTexture()
{
}

bool CGUIFontTTFDX::UpdateDynamicVertexBuffer(const SVertex* pSysMem, unsigned int vertex_count)
{
  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  ComPtr<ID3D11DeviceContext> pContext = DX::DeviceResources::Get()->GetD3DContext();

  if (!pDevice || !pContext)
    return false;

  unsigned width = sizeof(SVertex) * vertex_count;
  if (width > m_vertexWidth) // create or re-create
  {
    CD3D11_BUFFER_DESC bufferDesc(width, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    D3D11_SUBRESOURCE_DATA initData;
    ZeroMemory(&initData, sizeof(D3D11_SUBRESOURCE_DATA));
    initData.pSysMem = pSysMem;

    if (FAILED(pDevice->CreateBuffer(&bufferDesc, &initData, m_vertexBuffer.ReleaseAndGetAddressOf())))
    {
      CLog::LogF(LOGERROR, "Failed to create the vertex buffer.");
      return false;
    }

    m_vertexWidth = width;
  }
  else
  {
    D3D11_MAPPED_SUBRESOURCE resource;
    if (FAILED(pContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource)))
    {
      CLog::LogF(LOGERROR, "Failed to update the vertex buffer.");
      return false;
    }
    memcpy(resource.pData, pSysMem, width);
    pContext->Unmap(m_vertexBuffer.Get(), 0);
  }
  return true;
}

void CGUIFontTTFDX::CreateStaticIndexBuffer(void)
{
  if (m_staticIndexBufferCreated)
    return;

  ComPtr<ID3D11Device> pDevice = DX::DeviceResources::Get()->GetD3DDevice();
  if (!pDevice)
    return;

  uint16_t index[ELEMENT_ARRAY_MAX_CHAR_INDEX][6];
  for (size_t i = 0; i < ELEMENT_ARRAY_MAX_CHAR_INDEX; i++)
  {
    index[i][0] = 4 * i;
    index[i][1] = 4 * i + 1;
    index[i][2] = 4 * i + 2;
    index[i][3] = 4 * i + 2;
    index[i][4] = 4 * i + 3;
    index[i][5] = 4 * i + 0;
  }

  CD3D11_BUFFER_DESC desc(sizeof(index), D3D11_BIND_INDEX_BUFFER, D3D11_USAGE_IMMUTABLE);
  D3D11_SUBRESOURCE_DATA initData = { 0 };
  initData.pSysMem = index;

  if (SUCCEEDED(pDevice->CreateBuffer(&desc, &initData, m_staticIndexBuffer.ReleaseAndGetAddressOf())))
    m_staticIndexBufferCreated = true;
}

bool CGUIFontTTFDX::m_staticIndexBufferCreated = false;
ComPtr<ID3D11Buffer> CGUIFontTTFDX::m_staticIndexBuffer = nullptr;

void CGUIFontTTFDX::OnDestroyDevice(bool fatal)
{
  m_staticIndexBufferCreated = false;
  m_vertexWidth = 0;
  m_staticIndexBuffer = nullptr;
  m_vertexBuffer = nullptr;
}

void CGUIFontTTFDX::OnCreateDevice(void)
{
}
