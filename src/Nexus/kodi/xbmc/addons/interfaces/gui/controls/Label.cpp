/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Label.h"

#include "ServiceBroker.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-dev-kit/include/kodi/gui/controls/Label.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUILabelControl.h"
#include "guilib/GUIWindowManager.h"
#include "utils/log.h"

namespace ADDON
{

void Interface_GUIControlLabel::Init(AddonGlobalInterface* addonInterface)
{
  addonInterface->toKodi->kodi_gui->control_label =
      new AddonToKodiFuncTable_kodi_gui_control_label();

  addonInterface->toKodi->kodi_gui->control_label->set_visible = set_visible;
  addonInterface->toKodi->kodi_gui->control_label->set_label = set_label;
  addonInterface->toKodi->kodi_gui->control_label->get_label = get_label;
}

void Interface_GUIControlLabel::DeInit(AddonGlobalInterface* addonInterface)
{
  delete addonInterface->toKodi->kodi_gui->control_label;
}

void Interface_GUIControlLabel::set_visible(KODI_HANDLE kodiBase,
                                            KODI_GUI_CONTROL_HANDLE handle,
                                            bool visible)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUILabelControl* control = static_cast<CGUILabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlLabel::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return;
  }

  control->SetVisible(visible);
}

void Interface_GUIControlLabel::set_label(KODI_HANDLE kodiBase,
                                          KODI_GUI_CONTROL_HANDLE handle,
                                          const char* label)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUILabelControl* control = static_cast<CGUILabelControl*>(handle);
  if (!addon || !control || !label)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlLabel::{} - invalid handler data (kodiBase='{}', handle='{}', "
              "label='{}') on addon '{}'",
              __func__, kodiBase, handle, static_cast<const void*>(label),
              addon ? addon->ID() : "unknown");
    return;
  }

  CGUIMessage msg(GUI_MSG_LABEL_SET, control->GetParentID(), control->GetID());
  msg.SetLabel(label);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, control->GetParentID());
}

char* Interface_GUIControlLabel::get_label(KODI_HANDLE kodiBase, KODI_GUI_CONTROL_HANDLE handle)
{
  CAddonDll* addon = static_cast<CAddonDll*>(kodiBase);
  CGUILabelControl* control = static_cast<CGUILabelControl*>(handle);
  if (!addon || !control)
  {
    CLog::Log(LOGERROR,
              "Interface_GUIControlLabel::{} - invalid handler data (kodiBase='{}', handle='{}') "
              "on addon '{}'",
              __func__, kodiBase, handle, addon ? addon->ID() : "unknown");
    return nullptr;
  }

  return strdup(control->GetDescription().c_str());
}

} /* namespace ADDON */
