/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RPProcessInfoWin.h"

#include "cores/RetroPlayer/rendering/VideoRenderers/RPWinRenderer.h"

using namespace KODI;
using namespace RETRO;

CRPProcessInfoWin::CRPProcessInfoWin() : CRPProcessInfo("Windows")
{
}

CRPProcessInfo* CRPProcessInfoWin::Create()
{
  return new CRPProcessInfoWin();
}

void CRPProcessInfoWin::Register()
{
  CRPProcessInfo::RegisterProcessControl(CRPProcessInfoWin::Create);
}
