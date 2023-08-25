/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/Platform.h"

class CPlatformWin10 : public CPlatform
{
  public:
    /**\brief C'tor */
    CPlatformWin10() = default;

    /**\brief D'tor */
    ~CPlatformWin10() override = default;

    bool Init() override;
};
