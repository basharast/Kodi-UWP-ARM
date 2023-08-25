/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIHelpers.h"

#include "dialogs/GUIDialogYesNo.h"
#include "guilib/LocalizeStrings.h"

using namespace ADDON;
using namespace ADDON::GUI;

bool CHelpers::DialogAddonLifecycleUseAsk(const std::shared_ptr<const IAddon>& addonInfo)
{
  int header_nr;
  int text_nr;
  switch (addonInfo->LifecycleState())
  {
    case AddonLifecycleState::BROKEN:
      header_nr = 24164;
      text_nr = 24165;
      break;
    case AddonLifecycleState::DEPRECATED:
      header_nr = 24166;
      text_nr = 24167;
      break;
    default:
      header_nr = 0;
      text_nr = 0;
      break;
  }
  if (header_nr > 0)
  {
    std::string header = StringUtils::Format(g_localizeStrings.Get(header_nr), addonInfo->ID());
    std::string text =
        StringUtils::Format(g_localizeStrings.Get(text_nr), addonInfo->LifecycleStateDescription());
    if (!CGUIDialogYesNo::ShowAndGetInput(header, text))
      return false;
  }

  return true;
}
