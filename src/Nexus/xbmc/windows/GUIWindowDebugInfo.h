/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIDialog.h"
#ifdef TARGET_POSIX
#include "platform/posix/PosixResourceCounter.h"
#endif

class CGUITextLayout;

class CGUIWindowDebugInfo :
      public CGUIDialog
{
public:
  CGUIWindowDebugInfo();
  ~CGUIWindowDebugInfo() override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnMessage(CGUIMessage &message) override;
protected:
  void UpdateVisibility() override;
private:
  CGUITextLayout *m_layout;
#ifdef TARGET_POSIX
  CPosixResourceCounter m_resourceCounter;
#endif
};
