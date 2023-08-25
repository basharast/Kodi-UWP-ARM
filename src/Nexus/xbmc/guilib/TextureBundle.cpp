/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "TextureBundle.h"

#include "guilib/TextureBundleXBT.h"

class CTexture;

CTextureBundle::CTextureBundle() : m_tbXBT{false}, m_useXBT{false}
{
}

CTextureBundle::CTextureBundle(bool useXBT) : m_tbXBT{useXBT}, m_useXBT{useXBT}
{
}

bool CTextureBundle::HasFile(const std::string& Filename)
{
  if (m_useXBT)
  {
    return m_tbXBT.HasFile(Filename);
  }

  if (m_tbXBT.HasFile(Filename))
  {
    m_useXBT = true;
    return true;
  }

  return false;
}

std::vector<std::string> CTextureBundle::GetTexturesFromPath(const std::string& path)
{
  if (m_useXBT)
    return m_tbXBT.GetTexturesFromPath(path);
  else
    return {};
}

bool CTextureBundle::LoadTexture(const std::string& filename,
                                 std::unique_ptr<CTexture>& texture,
                                 int& width,
                                 int& height)
{
  if (m_useXBT)
    return m_tbXBT.LoadTexture(filename, texture, width, height);
  else
    return false;
}

bool CTextureBundle::LoadAnim(const std::string& filename,
                              std::vector<std::pair<std::unique_ptr<CTexture>, int>>& textures,
                              int& width,
                              int& height,
                              int& nLoops)
{
  if (m_useXBT)
    return m_tbXBT.LoadAnim(filename, textures, width, height, nLoops);
  else
    return false;
}

void CTextureBundle::Close()
{
  m_tbXBT.CloseBundle();
}

void CTextureBundle::SetThemeBundle(bool themeBundle)
{
  m_tbXBT.SetThemeBundle(themeBundle);
}

std::string CTextureBundle::Normalize(std::string name)
{
  return CTextureBundleXBT::Normalize(std::move(name));
}
