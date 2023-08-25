/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "TextureManager.h"
#include "utils/Color.h"
#include "utils/Geometry.h"
#include "guiinfo/GUIInfoColor.h"

// image alignment for <aspect>keep</aspect>, <aspect>scale</aspect> or <aspect>center</aspect>
#define ASPECT_ALIGN_CENTER  0
#define ASPECT_ALIGN_LEFT    1
#define ASPECT_ALIGN_RIGHT   2
#define ASPECT_ALIGNY_CENTER 0
#define ASPECT_ALIGNY_TOP    4
#define ASPECT_ALIGNY_BOTTOM 8
#define ASPECT_ALIGN_MASK    3
#define ASPECT_ALIGNY_MASK  ~3

class CAspectRatio
{
public:
  enum ASPECT_RATIO { AR_STRETCH = 0, AR_SCALE, AR_KEEP, AR_CENTER };
  CAspectRatio(ASPECT_RATIO aspect = AR_STRETCH)
  {
    ratio = aspect;
    align = ASPECT_ALIGN_CENTER | ASPECT_ALIGNY_CENTER;
    scaleDiffuse = true;
  };
  bool operator!=(const CAspectRatio &right) const
  {
    if (ratio != right.ratio) return true;
    if (align != right.align) return true;
    if (scaleDiffuse != right.scaleDiffuse) return true;
    return false;
  };

  ASPECT_RATIO ratio;
  uint32_t     align;
  bool         scaleDiffuse;
};

class CTextureInfo
{
public:
  CTextureInfo();
  explicit CTextureInfo(const std::string &file);
  CTextureInfo& operator=(const CTextureInfo& right) = default;
  bool       useLarge;
  CRect      border;          // scaled  - unneeded if we get rid of scale on load
  int        orientation;     // orientation of the texture (0 - 7 == EXIForientation - 1)
  std::string diffuse;         // diffuse overlay texture
  KODI::GUILIB::GUIINFO::CGUIInfoColor diffuseColor; // diffuse color
  std::string filename;        // main texture file
};

class CGUITexture
{
public:
  virtual ~CGUITexture() = default;
  static CGUITexture* CreateTexture(
      float posX, float posY, float width, float height, const CTextureInfo& texture);
  virtual CGUITexture* Clone() const = 0;

  static void DrawQuad(const CRect& coords,
                       UTILS::Color color,
                       CTexture* texture = nullptr,
                       const CRect* texCoords = nullptr);

  bool Process(unsigned int currentTime);
  void Render();

  void DynamicResourceAlloc(bool bOnOff);
  bool AllocResources();
  void FreeResources(bool immediately = false);
  void SetInvalid();

  bool SetVisible(bool visible);
  bool SetAlpha(unsigned char alpha);
  bool SetDiffuseColor(UTILS::Color color);
  bool SetPosition(float x, float y);
  bool SetWidth(float width);
  bool SetHeight(float height);
  bool SetFileName(const std::string &filename);
  void SetUseCache(const bool useCache = true);
  bool SetAspectRatio(const CAspectRatio &aspect);

  const std::string& GetFileName() const { return m_info.filename; };
  float GetTextureWidth() const { return m_frameWidth; };
  float GetTextureHeight() const { return m_frameHeight; };
  float GetWidth() const { return m_width; };
  float GetHeight() const { return m_height; };
  float GetXPosition() const { return m_posX; };
  float GetYPosition() const { return m_posY; };
  int GetOrientation() const;
  const CRect &GetRenderRect() const { return m_vertex; };
  bool IsLazyLoaded() const { return m_info.useLarge; };

  bool HitTest(const CPoint &point) const { return CRect(m_posX, m_posY, m_posX + m_width, m_posY + m_height).PtInRect(point); };
  bool IsAllocated() const { return m_isAllocated != NO; };
  bool FailedToAlloc() const { return m_isAllocated == NORMAL_FAILED || m_isAllocated == LARGE_FAILED; };
  bool ReadyToRender() const;

protected:
  CGUITexture(float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUITexture(const CGUITexture& left);

  bool CalculateSize();
  void LoadDiffuseImage();
  bool AllocateOnDemand();
  bool UpdateAnimFrame(unsigned int currentTime);
  void Render(float left,
              float top,
              float right,
              float bottom,
              float u1,
              float v1,
              float u2,
              float v2,
              float u3,
              float v3);
  static void OrientateTexture(CRect &rect, float width, float height, int orientation);
  void ResetAnimState();

  // functions that our implementation classes handle
  virtual void Allocate() {}; ///< called after our textures have been allocated
  virtual void Free() {};     ///< called after our textures have been freed
  virtual void Begin(UTILS::Color color) = 0;
  virtual void Draw(float* x,
                    float* y,
                    float* z,
                    const CRect& texture,
                    const CRect& diffuse,
                    int orientation) = 0;
  virtual void End() = 0;

  bool m_visible;
  UTILS::Color m_diffuseColor;

  float m_posX;         // size of the frame
  float m_posY;
  float m_width;
  float m_height;

  CRect m_vertex;       // vertex coords to render
  bool m_invalid;       // if true, we need to recalculate
  bool m_use_cache;
  unsigned char m_alpha;

  float m_frameWidth, m_frameHeight;          // size in pixels of the actual frame within the texture
  float m_texCoordsScaleU, m_texCoordsScaleV; // scale factor for pixel->texture coordinates

  // animations
  int m_currentLoop;
  unsigned int m_currentFrame;
  uint32_t m_lasttime;

  float m_diffuseU, m_diffuseV;           // size of the diffuse frame (in tex coords)
  float m_diffuseScaleU, m_diffuseScaleV; // scale factor of the diffuse frame (from texture coords to diffuse tex coords)
  CPoint m_diffuseOffset;                 // offset into the diffuse frame (it's not always the origin)

  bool m_allocateDynamically;
  enum ALLOCATE_TYPE { NO = 0, NORMAL, LARGE, NORMAL_FAILED, LARGE_FAILED };
  ALLOCATE_TYPE m_isAllocated;

  CTextureInfo m_info;
  CAspectRatio m_aspect;

  CTextureArray m_diffuse;
  CTextureArray m_texture;
};
