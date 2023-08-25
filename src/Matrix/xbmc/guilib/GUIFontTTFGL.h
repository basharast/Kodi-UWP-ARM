/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFontTTF.h"

#include <string>
#include <vector>

#include "system_gl.h"

class CGUIFontTTFGL : public CGUIFontTTF
{
public:
  explicit CGUIFontTTFGL(const std::string& strFileName);
  ~CGUIFontTTFGL(void) override;

  bool FirstBegin() override;
  void LastEnd() override;

  CVertexBuffer CreateVertexBuffer(const std::vector<SVertex> &vertices) const override;
  void DestroyVertexBuffer(CVertexBuffer &bufferHandle) const override;
  static void CreateStaticVertexBuffers(void);
  static void DestroyStaticVertexBuffers(void);

protected:
  CTexture* ReallocTexture(unsigned int& newHeight) override;
  bool CopyCharToTexture(FT_BitmapGlyph bitGlyph, unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2) override;
  void DeleteHardwareTexture() override;

  static GLuint m_elementArrayHandle;

private:
  unsigned int m_updateY1;
  unsigned int m_updateY2;

  enum TextureStatus
  {
    TEXTURE_VOID = 0,
    TEXTURE_READY,
    TEXTURE_REALLOCATED,
    TEXTURE_UPDATED,
  };

  TextureStatus m_textureStatus;

  static bool m_staticVertexBufferCreated;
};

