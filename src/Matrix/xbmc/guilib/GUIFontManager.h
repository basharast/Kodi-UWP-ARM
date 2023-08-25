/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFontManager.h
\brief
*/

#include "IMsgTargetCallback.h"
#include "utils/Color.h"
#include "utils/GlobalsHandling.h"
#include "windowing/GraphicContext.h"

#include <utility>
#include <vector>

// Forward
class CGUIFont;
class CGUIFontTTF;
class CXBMCTinyXML;
class TiXmlNode;
class CSetting;
struct StringSettingOption;

struct OrigFontInfo
{
   int size;
   float aspect;
   std::string fontFilePath;
   std::string fileName;
   RESOLUTION_INFO sourceRes;
   bool preserveAspect;
   bool border;
};

/*!
 \ingroup textures
 \brief
 */
class GUIFontManager : public IMsgTargetCallback
{
public:
  GUIFontManager(void);
  ~GUIFontManager(void) override;

  bool OnMessage(CGUIMessage &message) override;

  void Unload(const std::string& strFontName);
  void LoadFonts(const std::string &fontSet);
  CGUIFont* LoadTTF(const std::string& strFontName, const std::string& strFilename, UTILS::Color textColor, UTILS::Color shadowColor, const int iSize, const int iStyle, bool border = false, float lineSpacing = 1.0f, float aspect = 1.0f, const RESOLUTION_INFO *res = NULL, bool preserveAspect = false);
  CGUIFont* GetFont(const std::string& strFontName, bool fallback = true);

  /*! \brief return a default font
   \param border whether the font should be a font with an outline
   \return the font.  NULL if no default font can be found.
   */
  CGUIFont* GetDefaultFont(bool border = false);

  void Clear();
  void FreeFontFile(CGUIFontTTF* pFont);

  static void SettingOptionsFontsFiller(const std::shared_ptr<const CSetting>& setting,
                                        std::vector<StringSettingOption>& list,
                                        std::string& current,
                                        void* data);

protected:
  void ReloadTTFFonts();
  static void RescaleFontSizeAndAspect(float *size, float *aspect, const RESOLUTION_INFO &sourceRes, bool preserveAspect);
  void LoadFonts(const TiXmlNode* fontNode);
  CGUIFontTTF* GetFontFile(const std::string& strFontFile);
  static void GetStyle(const TiXmlNode *fontNode, int &iStyle);

  std::vector<CGUIFont*> m_vecFonts;
  std::vector<CGUIFontTTF*> m_vecFontFiles;
  std::vector<OrigFontInfo> m_vecFontInfo;
  RESOLUTION_INFO m_skinResolution;
  bool m_canReload;
};

/*!
 \ingroup textures
 \brief
 */
XBMC_GLOBAL_REF(GUIFontManager, g_fontManager);
#define g_fontManager XBMC_GLOBAL_USE(GUIFontManager)

