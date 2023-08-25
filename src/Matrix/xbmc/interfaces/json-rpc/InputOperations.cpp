/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputOperations.h"

#include "Application.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "input/ButtonTranslator.h"
#include "input/Key.h"
#include "input/actions/ActionTranslator.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/Variant.h"

using namespace JSONRPC;
using namespace KODI::MESSAGING;

//! @todo the breakage of the screensaver should be refactored
//! to one central super duper place for getting rid of
//! 1 million dupes
bool CInputOperations::handleScreenSaver()
{
  g_application.ResetScreenSaver();
  if (g_application.WakeUpScreenSaverAndDPMS())
    return true;

  return false;
}

JSONRPC_STATUS CInputOperations::SendAction(int actionID, bool wakeScreensaver /* = true */, bool waitResult /* = false */)
{
  if(!wakeScreensaver || !handleScreenSaver())
  {
    g_application.ResetSystemIdleTimer();
    CGUIComponent* gui = CServiceBroker::GetGUI();
    if (gui)
      gui->GetAudioManager().PlayActionSound(actionID);

    if (waitResult)
      CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(actionID)));
    else
      CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(new CAction(actionID)));
  }
  return ACK;
}

JSONRPC_STATUS CInputOperations::activateWindow(int windowID)
{
  if(!handleScreenSaver())
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTIVATE_WINDOW, windowID, 0);

  return ACK;
}

JSONRPC_STATUS CInputOperations::SendText(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  if (CGUIKeyboardFactory::SendTextToActiveKeyboard(parameterObject["text"].asString(), parameterObject["done"].asBoolean()))
    return ACK;

  CGUIWindow *window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow(CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
  if (!window)
    return ACK;

  CGUIMessage msg(GUI_MSG_SET_TEXT, 0, window->GetFocusedControlID());
  msg.SetLabel(parameterObject["text"].asString());
  msg.SetParam1(parameterObject["done"].asBoolean() ? 1 : 0);
  CApplicationMessenger::GetInstance().SendGUIMessage(msg, window->GetID());

  return ACK;
}

JSONRPC_STATUS CInputOperations::ExecuteAction(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  unsigned int action;
  if (!CActionTranslator::TranslateString(parameterObject["action"].asString(), action))
    return InvalidParams;

  return SendAction(action);
}

JSONRPC_STATUS CInputOperations::ButtonEvent(const std::string& method,
                                             ITransportLayer* transport,
                                             IClient* client,
                                             const CVariant& parameterObject,
                                             CVariant& result)
{
  std::string button = parameterObject["button"].asString();
  std::string keymap = parameterObject["keymap"].asString();
  int holdtime = static_cast<int>(parameterObject["holdtime"].asInteger());
  if (holdtime < 0)
  {
    return InvalidParams;
  }

  uint32_t keycode = CButtonTranslator::TranslateString(keymap, button);
  if (keycode == 0)
  {
    return InvalidParams;
  }

  XBMC_Event* newEvent = new XBMC_Event;
  newEvent->type = XBMC_BUTTON;
  newEvent->keybutton.button = keycode;
  newEvent->keybutton.holdtime = holdtime;

  CApplicationMessenger::GetInstance().PostMsg(TMSG_EVENT, -1, -1, static_cast<void*>(newEvent));

  return ACK;
}

JSONRPC_STATUS CInputOperations::Left(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_MOVE_LEFT);
}

JSONRPC_STATUS CInputOperations::Right(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_MOVE_RIGHT);
}

JSONRPC_STATUS CInputOperations::Down(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_MOVE_DOWN);
}

JSONRPC_STATUS CInputOperations::Up(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_MOVE_UP);
}

JSONRPC_STATUS CInputOperations::Select(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_SELECT_ITEM);
}

JSONRPC_STATUS CInputOperations::Back(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_NAV_BACK);
}

JSONRPC_STATUS CInputOperations::ContextMenu(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_CONTEXT_MENU);
}

JSONRPC_STATUS CInputOperations::Info(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_SHOW_INFO);
}

JSONRPC_STATUS CInputOperations::Home(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return activateWindow(WINDOW_HOME);
}

JSONRPC_STATUS CInputOperations::ShowCodec(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return MethodNotFound;
}

JSONRPC_STATUS CInputOperations::ShowOSD(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_SHOW_OSD);
}

JSONRPC_STATUS CInputOperations::ShowPlayerProcessInfo(const std::string &method, ITransportLayer *transport, IClient *client, const CVariant &parameterObject, CVariant &result)
{
  return SendAction(ACTION_PLAYER_PROCESS_INFO);
}
