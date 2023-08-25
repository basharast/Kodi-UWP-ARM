/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItem.h"
#include "threads/CriticalSection.h"

typedef enum
{
  MOVIES = 0,
  TV_SHOWS = 1
} TVOSTopShelfItemsCategory;


class CTVOSTopShelf
{
public:
  static CTVOSTopShelf& GetInstance();
  void RunTopShelf();
  void SetTopShelfItems(CFileItemList& items, TVOSTopShelfItemsCategory category);
  void HandleTopShelfUrl(const std::string& url, const bool run);

private:
  CTVOSTopShelf() = default;
  ~CTVOSTopShelf() = default;

  static std::string m_url;
  static bool m_handleUrl;
};
