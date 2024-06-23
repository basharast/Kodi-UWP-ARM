/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Resource.h"

#include <memory>

namespace ADDON
{

class CGameResource : public CResource
{
public:
  explicit CGameResource(const AddonInfoPtr& addonInfo);
  ~CGameResource() override = default;

  // implementation of CResource
  bool IsAllowed(const std::string& file) const override { return true; }
};

}
