/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "platform/posix/GPUInfoPosix.h"

class CGPUInfoFreebsd : public CGPUInfoPosix
{
public:
  CGPUInfoFreebsd() = default;
  ~CGPUInfoFreebsd() = default;

private:
  bool SupportsPlatformTemperature() const override;
  bool GetGPUPlatformTemperature(CTemperature& temperature) const override;
};
