/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CAppParamParser;
class CAdvancedSettings;
class CProfileManager;
class CSettings;

class CSettingsComponent
{
public:
  CSettingsComponent();
  virtual ~CSettingsComponent();

  /*!
   * @brief Initialize all subcomponents with system default values (loaded from code, system settings files, ...).
   * @param params The command line params passed to the app.
   */
  void Init(const CAppParamParser &params);

  /*!
   * @brief Initialize all subcomponents with user values (loaded from user settings files, according to active profile).
   * @return true on success, false otherwise.
   */
  bool Load();

  /*!
   * @brief Deinitialize all subcomponents.
   */
  void Deinit();

  /*!
   * @brief Get access to the settings subcomponent.
   * @return the settings subcomponent.
   */
  std::shared_ptr<CSettings> GetSettings();

  /*!
   * @brief Get access to the advanced settings subcomponent.
   * @return the advanced settings subcomponent.
   */
  std::shared_ptr<CAdvancedSettings> GetAdvancedSettings();

  /*!
   * @brief Get access to the profiles manager subcomponent.
   * @return the profiles manager subcomponent.
   */
  std::shared_ptr<CProfileManager> GetProfileManager();

private:
  bool InitDirectoriesLinux(bool bPlatformDirectories);
  bool InitDirectoriesOSX(bool bPlatformDirectories);
  bool InitDirectoriesWin32(bool bPlatformDirectories);
  void CreateUserDirs() const;

  enum class State
  {
    DEINITED,
    INITED,
    LOADED
  };
  State m_state = State::DEINITED;

  std::shared_ptr<CSettings> m_settings;
  std::shared_ptr<CAdvancedSettings> m_advancedSettings;
  std::shared_ptr<CProfileManager> m_profileManager;
};
