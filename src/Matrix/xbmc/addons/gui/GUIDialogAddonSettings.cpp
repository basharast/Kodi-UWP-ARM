/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogAddonSettings.h"

#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "addons/settings/AddonSettings.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "input/Key.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "view/ViewStateSettings.h"

#define CONTROL_BTN_LEVELS 20

using namespace KODI::MESSAGING;

CGUIDialogAddonSettings::CGUIDialogAddonSettings()
  : CGUIDialogSettingsManagerBase(WINDOW_DIALOG_ADDON_SETTINGS, "DialogAddonSettings.xml")
{
}

bool CGUIDialogAddonSettings::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId() == CONTROL_SETTINGS_CUSTOM_BUTTON)
      {
        OnResetSettings();
        return true;
      }
      break;
    }

    case GUI_MSG_SETTING_UPDATED:
    {
      const std::string& settingId = message.GetStringParam(0);
      const std::string& settingValue = message.GetStringParam(1);

      std::shared_ptr<CSetting> setting = GetSettingsManager()->GetSetting(settingId);
      if (setting != nullptr)
      {
        setting->FromString(settingValue);
        return true;
      }
      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManagerBase::OnMessage(message);
}

bool CGUIDialogAddonSettings::OnAction(const CAction& action)
{
  switch (action.GetID())
  {
    case ACTION_SETTINGS_LEVEL_CHANGE:
    {
      // Test if we can access the new level
      if (!g_passwordManager.CheckSettingLevelLock(
              CViewStateSettings::GetInstance().GetNextSettingLevel(), true))
        return false;

      CViewStateSettings::GetInstance().CycleSettingLevel();
      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      // try to keep the current position
      std::string oldCategory;
      if (m_iCategory >= 0 && m_iCategory < static_cast<int>(m_categories.size()))
        oldCategory = m_categories[m_iCategory]->GetId();

      SET_CONTROL_LABEL(CONTROL_BTN_LEVELS,
                        10036 +
                            static_cast<int>(CViewStateSettings::GetInstance().GetSettingLevel()));
      // only re-create the categories, the settings will be created later
      SetupControls(false);

      m_iCategory = 0;
      // try to find the category that was previously selected
      if (!oldCategory.empty())
      {
        for (int i = 0; i < static_cast<int>(m_categories.size()); i++)
        {
          if (m_categories[i]->GetId() == oldCategory)
          {
            m_iCategory = i;
            break;
          }
        }
      }

      CreateSettings();
      return true;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManagerBase::OnAction(action);
}

bool CGUIDialogAddonSettings::ShowForAddon(const ADDON::AddonPtr& addon,
                                           bool saveToDisk /* = true */)
{
  if (addon == nullptr)
    return false;

  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return false;

  if (!addon->HasSettings())
  {
    // addon does not support settings, inform user
    HELPERS::ShowOKDialogText(CVariant{24000}, CVariant{24030});
    return false;
  }

  // Create the dialog
  CGUIDialogAddonSettings* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonSettings>(
          WINDOW_DIALOG_ADDON_SETTINGS);
  if (dialog == nullptr)
    return false;

  dialog->m_addon = addon;
  dialog->m_saveToDisk = saveToDisk;
  dialog->Open();

  if (!dialog->IsConfirmed())
    return false;

  if (saveToDisk)
    addon->SaveSettings();

  return true;
}

void CGUIDialogAddonSettings::SaveAndClose()
{
  if (!g_passwordManager.CheckMenuLock(WINDOW_ADDON_BROWSER))
    return;

  // get the dialog
  CGUIDialogAddonSettings* dialog =
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogAddonSettings>(
          WINDOW_DIALOG_ADDON_SETTINGS);
  if (dialog == nullptr || !dialog->IsActive())
    return;

  // check if we need to save the settings
  if (dialog->m_saveToDisk && dialog->m_addon != nullptr)
    dialog->m_addon->SaveSettings();

  // close the dialog
  dialog->Close();
}

std::string CGUIDialogAddonSettings::GetCurrentAddonID() const
{
  if (m_addon == nullptr)
    return "";

  return m_addon->ID();
}

void CGUIDialogAddonSettings::SetupView()
{
  if (m_addon == nullptr || m_addon->GetSettings() == nullptr)
    return;

  auto settings = m_addon->GetSettings();
  if (!settings->IsLoaded())
    return;

  CGUIDialogSettingsManagerBase::SetupView();

  // set addon id as window property
  SetProperty("Addon.ID", m_addon->ID());

  // set heading
  SetHeading(StringUtils::Format("$LOCALIZE[10004] - %s",
                                 m_addon->Name().c_str())); // "Settings - AddonName"

  // set control labels
  SET_CONTROL_LABEL(CONTROL_SETTINGS_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CANCEL_BUTTON, 222);
  SET_CONTROL_LABEL(CONTROL_SETTINGS_CUSTOM_BUTTON, 409);
  SET_CONTROL_LABEL(CONTROL_BTN_LEVELS,
                    10036 + static_cast<int>(CViewStateSettings::GetInstance().GetSettingLevel()));
}

std::string CGUIDialogAddonSettings::GetLocalizedString(uint32_t labelId) const
{
  std::string label = g_localizeStrings.GetAddonString(m_addon->ID(), labelId);
  if (!label.empty())
    return label;

  return CGUIDialogSettingsManagerBase::GetLocalizedString(labelId);
}

std::string CGUIDialogAddonSettings::GetSettingsLabel(const std::shared_ptr<ISetting>& setting)
{
  if (setting == nullptr)
    return "";

  std::string label = GetLocalizedString(setting->GetLabel());
  if (!label.empty())
    return label;

  // try the addon settings
  label = m_addon->GetSettings()->GetSettingLabel(setting->GetLabel());
  if (!label.empty())
    return label;

  return CGUIDialogSettingsManagerBase::GetSettingsLabel(setting);
}

int CGUIDialogAddonSettings::GetSettingLevel() const
{
  return static_cast<int>(CViewStateSettings::GetInstance().GetSettingLevel());
}

std::shared_ptr<CSettingSection> CGUIDialogAddonSettings::GetSection()
{
  const auto settingsManager = GetSettingsManager();
  if (settingsManager == nullptr)
    return nullptr;

  const auto sections = settingsManager->GetSections();
  if (!sections.empty())
    return sections.front();

  return nullptr;
}

CSettingsManager* CGUIDialogAddonSettings::GetSettingsManager() const
{
  if (m_addon == nullptr || m_addon->GetSettings() == nullptr)
    return nullptr;

  return m_addon->GetSettings()->GetSettingsManager();
}

void CGUIDialogAddonSettings::OnSettingAction(const std::shared_ptr<const CSetting>& setting)
{
  if (m_addon == nullptr || m_addon->GetSettings() == nullptr)
    return;

  m_addon->GetSettings()->OnSettingAction(setting);
}
