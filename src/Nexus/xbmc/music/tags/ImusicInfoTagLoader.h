/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class EmbeddedArt;

namespace MUSIC_INFO
{
  class CMusicInfoTag;
  class IMusicInfoTagLoader
  {
  public:
    IMusicInfoTagLoader(void) = default;
    virtual ~IMusicInfoTagLoader() = default;

    virtual bool Load(const std::string& strFileName, CMusicInfoTag& tag, EmbeddedArt *art = NULL) = 0;
  };
}
