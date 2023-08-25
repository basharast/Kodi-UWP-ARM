/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "Builtins.h"

#include "AddonBuiltins.h"
#include "ApplicationBuiltins.h"
#include "CECBuiltins.h"
#include "GUIBuiltins.h"
#include "GUIControlBuiltins.h"
#include "GUIContainerBuiltins.h"
#include "LibraryBuiltins.h"
#include "OpticalBuiltins.h"
#include "PictureBuiltins.h"
#include "PlayerBuiltins.h"
#include "ProfileBuiltins.h"
#include "PVRBuiltins.h"
#include "SkinBuiltins.h"
#include "SystemBuiltins.h"
#include "WeatherBuiltins.h"

#include "ServiceBroker.h"
#include "input/InputManager.h"
#include "powermanagement/PowerTypes.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#if defined(TARGET_ANDROID)
#include "AndroidBuiltins.h"
#endif

#if defined(TARGET_POSIX)
#include "PlatformDefs.h"
#endif

CBuiltins::CBuiltins()
{
  RegisterCommands<CAddonBuiltins>();
  RegisterCommands<CApplicationBuiltins>();
  RegisterCommands<CGUIBuiltins>();
  RegisterCommands<CGUIContainerBuiltins>();
  RegisterCommands<CGUIControlBuiltins>();
  RegisterCommands<CLibraryBuiltins>();
  RegisterCommands<COpticalBuiltins>();
  RegisterCommands<CPictureBuiltins>();
  RegisterCommands<CPlayerBuiltins>();
  RegisterCommands<CProfileBuiltins>();
  RegisterCommands<CPVRBuiltins>();
  RegisterCommands<CSkinBuiltins>();
  RegisterCommands<CSystemBuiltins>();
  RegisterCommands<CWeatherBuiltins>();

#if defined(HAVE_LIBCEC)
  RegisterCommands<CCECBuiltins>();
#endif

#if defined(TARGET_ANDROID)
  RegisterCommands<CAndroidBuiltins>();
#endif
}

CBuiltins::~CBuiltins() = default;

CBuiltins& CBuiltins::GetInstance()
{
  static CBuiltins sBuiltins;
  return sBuiltins;
}

bool CBuiltins::HasCommand(const std::string& execString)
{
  std::string function;
  std::vector<std::string> parameters;
  CUtil::SplitExecFunction(execString, function, parameters);
  StringUtils::ToLower(function);

  if (CServiceBroker::GetInputManager().HasBuiltin(function))
    return true;

  const auto& it = m_command.find(function);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || it->second.parameters <= parameters.size())
      return true;
  }

  return false;
}

bool CBuiltins::IsSystemPowerdownCommand(const std::string& execString)
{
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);

  // Check if action is resulting in system powerdown.
  if (execute == "reboot"    ||
      execute == "restart"   ||
      execute == "reset"     ||
      execute == "powerdown" ||
      execute == "hibernate" ||
      execute == "suspend" )
  {
    return true;
  }
  else if (execute == "shutdown")
  {
    switch (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))
    {
      case POWERSTATE_SHUTDOWN:
      case POWERSTATE_SUSPEND:
      case POWERSTATE_HIBERNATE:
        return true;

      default:
        return false;
    }
  }
  return false;
}

void CBuiltins::GetHelp(std::string &help)
{
  help.clear();

  for (const auto& it : m_command)
  {
    help += it.first;
    help += "\t";
    help += it.second.description;
    help += "\n";
  }
}

int CBuiltins::Execute(const std::string& execString)
{
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);

  const auto& it = m_command.find(execute);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || params.size() >= it->second.parameters)
      return it->second.Execute(params);
    else
    {
      CLog::Log(LOGERROR, "{0} called with invalid number of parameters (should be: {1}, is {2})",
                          execute.c_str(), it->second.parameters, params.size());
      return -1;
    }
  }
  else
    return CServiceBroker::GetInputManager().ExecuteBuiltin(execute, params);
}
