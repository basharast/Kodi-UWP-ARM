/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/dialogs/GUIDialogSettingsManualBase.h"

class CFileItem;

namespace PERIPHERALS
{
class CGUIDialogPeripheralSettings : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogPeripheralSettings();
  ~CGUIDialogPeripheralSettings() override;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage& message) override;

  virtual void SetFileItem(const CFileItem* item);

protected:
  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override;
  void OnResetSettings() override;
  void SetupView() override;

  // specialization of CGUIDialogSettingsManualBase
  void InitializeSettings() override;

  CFileItem* m_item;
  bool m_initialising = false;
  std::map<std::string, std::shared_ptr<CSetting>> m_settingsMap;
};
} // namespace PERIPHERALS
