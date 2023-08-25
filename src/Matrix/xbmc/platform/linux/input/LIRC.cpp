/*
 *  Copyright (C) 2007-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LIRC.h"

#include "AppInboundProtocol.h"
#include "ServiceBroker.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <fcntl.h>
#include <lirc/lirc_client.h>
#include <sys/socket.h>
#include <sys/un.h>

CLirc::CLirc() : CThread("Lirc")
{
}

CLirc::~CLirc()
{
  {
    CSingleLock lock(m_critSection);
    if (m_fd > 0)
      shutdown(m_fd, SHUT_RDWR);
  }
  StopThread();
}

void CLirc::Start()
{
  Create();
  SetPriority(GetMinPriority());
}

void CLirc::Process()
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    throw std::runtime_error("CSettingsComponent needs to exist before starting CLirc");

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    throw std::runtime_error("CSettings needs to exist before starting CLirc");

  while (!settings->IsLoaded())
    CThread::Sleep(1000);

  m_profileId = settingsComponent->GetProfileManager()->GetCurrentProfileId();
  m_irTranslator.Load("Lircmap.xml");

  // make sure work-around (CheckDaemon) uses the same socket path as lirc_init
  const char* socket_path = getenv("LIRC_SOCKET_PATH");
  socket_path = socket_path ? socket_path : "/var/run/lirc/lircd";
  setenv("LIRC_SOCKET_PATH", socket_path, 0);

  while (!m_bStop)
  {
    {
      CSingleLock lock(m_critSection);

      // lirc_client is buggy because it does not close socket, if connect fails
      // work around by checking if daemon is running before calling lirc_init
      if (!CheckDaemon())
      {
        CSingleExit lock(m_critSection);
        CThread::Sleep(1000);
        continue;
      }

      m_fd = lirc_init(const_cast<char*>("kodi"), 0);
      if (m_fd <= 0)
      {
        CSingleExit lock(m_critSection);
        CThread::Sleep(1000);
        continue;
      }
    }

    char *code;
    while (!m_bStop)
    {
      int ret = lirc_nextcode(&code);
      if (ret < 0)
      {
        lirc_deinit();
        CThread::Sleep(1000);
        break;
      }
      if (code != nullptr)
      {
        int profileId = settingsComponent->GetProfileManager()->GetCurrentProfileId();
        if (m_profileId != profileId)
        {
          m_profileId = profileId;
          m_irTranslator.Load("Lircmap.xml");
        }
        ProcessCode(code);
        free(code);
      }
    }
  }

  lirc_deinit();
}

void CLirc::ProcessCode(char *buf)
{
  // Parse the result. Sample line:
  // 000000037ff07bdd 00 OK mceusb
  char scanCode[128];
  char buttonName[128];
  char repeatStr[4];
  char deviceName[128];
  sscanf(buf, "%s %s %s %s", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0]);

  // Some template LIRC configuration have button names in apostrophes or quotes.
  // If we got a quoted button name, strip 'em
  unsigned int buttonNameLen = strlen(buttonName);
  if (buttonNameLen > 2 &&
      ((buttonName[0] == '\'' && buttonName[buttonNameLen-1] == '\'') ||
      ((buttonName[0] == '"' && buttonName[buttonNameLen-1] == '"'))))
  {
    memmove(buttonName, buttonName + 1, buttonNameLen - 2);
    buttonName[buttonNameLen - 2] = '\0';
  }

  int button = m_irTranslator.TranslateButton(deviceName, buttonName);

  char *end = nullptr;
  long repeat = strtol(repeatStr, &end, 16);
  if (!end || *end != 0)
    CLog::Log(LOGERROR, "LIRC: invalid non-numeric character in expression %s", repeatStr);

  if (repeat == 0)
  {
    CLog::Log(LOGDEBUG, "LIRC: - NEW %s %s %s %s (%s)", &scanCode[0], &repeatStr[0], &buttonName[0], &deviceName[0], buttonName);
    m_firstClickTime = XbmcThreads::SystemClockMillis();

    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = 0;
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->OnEvent(newEvent);
    return;
  }
  else if (repeat > CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_remoteDelay)
  {
    XBMC_Event newEvent;
    newEvent.type = XBMC_BUTTON;
    newEvent.keybutton.button = button;
    newEvent.keybutton.holdtime = XbmcThreads::SystemClockMillis() - m_firstClickTime;
    std::shared_ptr<CAppInboundProtocol> appPort = CServiceBroker::GetAppPort();
    if (appPort)
      appPort->OnEvent(newEvent);
  }
}

bool CLirc::CheckDaemon()
{
  const char* socket_path = getenv("LIRC_SOCKET_PATH");

  struct sockaddr_un addr_un;
  if (strlen(socket_path) + 1 > sizeof(addr_un.sun_path))
  {
    return false;
  }

  addr_un.sun_family = AF_UNIX;
  strcpy(addr_un.sun_path, socket_path);

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1)
  {
    return false;
  }

  if (connect(fd, reinterpret_cast<struct sockaddr*>(&addr_un), sizeof(addr_un)) == -1)
  {
    close(fd);
    return false;
  }

  close(fd);
  return true;
}
