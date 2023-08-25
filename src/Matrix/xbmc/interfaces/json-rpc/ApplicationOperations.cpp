/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationOperations.h"

#include "Application.h"
#include "CompileInfo.h"
#include "GUIInfoManager.h"
#include "InputOperations.h"
#include "LangInfo.h"
#include "Util.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <cmath>
#include <string.h>

using namespace JSONRPC;
using namespace KODI::MESSAGING;

JSONRPC_STATUS CApplicationOperations::GetProperties(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CVariant properties = CVariant(CVariant::VariantTypeObject);
  for (unsigned int index = 0; index < parameterObject["properties"].size(); index++)
  {
    std::string propertyName = parameterObject["properties"][index].asString();
    CVariant property;
    JSONRPC_STATUS ret;
    if ((ret = GetPropertyValue(propertyName, property)) != OK)
      return ret;

    properties[propertyName] = property;
  }

  result = properties;

  return OK;
}

JSONRPC_STATUS CApplicationOperations::SetVolume(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  bool up = false;
  if (parameterObject["volume"].isInteger())
  {
    int oldVolume = (int)g_application.GetVolumePercent();
    int volume = (int)parameterObject["volume"].asInteger();

    g_application.SetVolume((float)volume, true);

    up = oldVolume < volume;
  }
  else if (parameterObject["volume"].isString())
  {
    JSONRPC_STATUS ret;
    std::string direction = parameterObject["volume"].asString();
    if (direction.compare("increment") == 0)
    {
      ret = CInputOperations::SendAction(ACTION_VOLUME_UP, false, true);
      up = true;
    }
    else if (direction.compare("decrement") == 0)
    {
      ret = CInputOperations::SendAction(ACTION_VOLUME_DOWN, false, true);
      up = false;
    }
    else
      return InvalidParams;

    if (ret != ACK && ret != OK)
      return ret;
  }
  else
    return InvalidParams;

  CApplicationMessenger::GetInstance().PostMsg(TMSG_VOLUME_SHOW, up ? ACTION_VOLUME_UP : ACTION_VOLUME_DOWN);

  return GetPropertyValue("volume", result);
}

JSONRPC_STATUS CApplicationOperations::SetMute(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if ((parameterObject["mute"].isString() && parameterObject["mute"].asString().compare("toggle") == 0) ||
      (parameterObject["mute"].isBoolean() && parameterObject["mute"].asBoolean() != g_application.IsMuted()))
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(ACTION_MUTE)));
  else if (!parameterObject["mute"].isBoolean() && !parameterObject["mute"].isString())
    return InvalidParams;

  return GetPropertyValue("muted", result);
}

JSONRPC_STATUS CApplicationOperations::Quit(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);
  return ACK;
}

JSONRPC_STATUS CApplicationOperations::GetPropertyValue(const std::string &property, CVariant &result)
{
  if (property == "volume")
    result = static_cast<int>(std::lroundf(g_application.GetVolumePercent()));
  else if (property == "muted")
    result = g_application.IsMuted();
  else if (property == "name")
    result = CCompileInfo::GetAppName();
  else if (property == "version")
  {
    result = CVariant(CVariant::VariantTypeObject);
    result["major"] = CCompileInfo::GetMajor();
    result["minor"] = CCompileInfo::GetMinor();
    result["revision"] = CCompileInfo::GetSCMID();
    std::string tag = CCompileInfo::GetSuffix();
    if (StringUtils::StartsWithNoCase(tag, "alpha"))
    {
      result["tag"] = "alpha";
      result["tagversion"] = StringUtils::Mid(tag, 5);
    }
    else if (StringUtils::StartsWithNoCase(tag, "beta"))
    {
      result["tag"] = "beta";
      result["tagversion"] = StringUtils::Mid(tag, 4);
    }
    else if (StringUtils::StartsWithNoCase(tag, "rc"))
    {
      result["tag"] = "releasecandidate";
      result["tagversion"] = StringUtils::Mid(tag, 2);
    }
    else if (tag.empty())
      result["tag"] = "stable";
    else
      result["tag"] = "prealpha";
  }
  else if (property == "sorttokens")
  {
    result = CVariant(CVariant::VariantTypeArray); // Ensure no tokens returns as []
    std::set<std::string> sortTokens = g_langInfo.GetSortTokens();
    for (const auto& token : sortTokens)
      result.append(token);
  }
  else if (property == "language")
    result = g_langInfo.GetLocale().ToShortString();
  else
    return InvalidParams;

  return OK;
}
