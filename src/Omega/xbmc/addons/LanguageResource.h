/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"
#include "utils/Locale.h"

#include <set>

namespace ADDON
{
class CLanguageResource : public CResource
{
public:
  explicit CLanguageResource(const AddonInfoPtr& addonInfo);

  bool IsInUse() const override;

  void OnPostInstall(bool update, bool modal) override;

  bool IsAllowed(const std::string &file) const override;

  const CLocale& GetLocale() const { return m_locale; }

  const std::string& GetGuiCharset() const { return m_charsetGui; }
  bool ForceUnicodeFont() const { return m_forceUnicodeFont; }
  const std::string& GetSubtitleCharset() const { return m_charsetSubtitle; }

  const std::string& GetDvdMenuLanguage() const { return m_dvdLanguageMenu; }
  const std::string& GetDvdAudioLanguage() const { return m_dvdLanguageAudio; }
  const std::string& GetDvdSubtitleLanguage() const { return m_dvdLanguageSubtitle; }

  const std::set<std::string>& GetSortTokens() const { return m_sortTokens; }

  static std::string GetAddonId(const std::string& locale);

  static bool FindLegacyLanguage(const std::string &locale, std::string &legacyLanguage);

private:
  CLocale m_locale;

  std::string m_charsetGui;
  bool m_forceUnicodeFont;
  std::string m_charsetSubtitle;

  std::string m_dvdLanguageMenu;
  std::string m_dvdLanguageAudio;
  std::string m_dvdLanguageSubtitle;

  std::set<std::string> m_sortTokens;
};

}
