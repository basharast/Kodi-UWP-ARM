/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/GlobalsHandling.h"

#include <memory>

namespace ADDON {
class CAddonMgr;
class CBinaryAddonManager;
class CBinaryAddonCache;
class CVFSAddonCache;
class CServiceAddonManager;
class CRepositoryUpdater;
}

namespace ANNOUNCEMENT
{
  class CAnnouncementManager;
}

namespace PVR
{
  class CPVRManager;
}

namespace PLAYLIST
{
  class CPlayListPlayer;
}

class CContextMenuManager;
class XBPython;
class CDataCacheCore;
class IAE;
class CFavouritesService;
class CInputManager;
class CFileExtensionProvider;
class CNetworkBase;
class CWinSystemBase;
class CRenderSystemBase;
class CPowerManager;
class CWeatherManager;
class CPlayerCoreFactory;
class CDatabaseManager;
class CEventLog;
class CGUIComponent;
class CAppInboundProtocol;
class CSettingsComponent;
class CDecoderFilterManager;
class CMediaManager;
class CCPUInfo;
class CLog;

namespace KODI
{
namespace GAME
{
  class CControllerManager;
  class CGameServices;
}

namespace RETRO
{
  class CGUIGameRenderManager;
}
}

namespace PERIPHERALS
{
  class CPeripherals;
}

class CServiceBroker
{
public:
  CServiceBroker();
  ~CServiceBroker();

  static CLog& GetLogging();
  static void CreateLogging();

  static std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> GetAnnouncementManager();
  static void RegisterAnnouncementManager(std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> announcementManager);
  static void UnregisterAnnouncementManager();

  static ADDON::CAddonMgr &GetAddonMgr();
  static ADDON::CBinaryAddonManager &GetBinaryAddonManager();
  static ADDON::CBinaryAddonCache &GetBinaryAddonCache();
  static ADDON::CVFSAddonCache &GetVFSAddonCache();
  static XBPython &GetXBPython();
  static PVR::CPVRManager &GetPVRManager();
  static CContextMenuManager& GetContextMenuManager();
  static CDataCacheCore& GetDataCacheCore();
  static PLAYLIST::CPlayListPlayer& GetPlaylistPlayer();
  static KODI::GAME::CControllerManager& GetGameControllerManager();
  static KODI::GAME::CGameServices& GetGameServices();
  static KODI::RETRO::CGUIGameRenderManager& GetGameRenderManager();
  static PERIPHERALS::CPeripherals& GetPeripherals();
  static CFavouritesService& GetFavouritesService();
  static ADDON::CServiceAddonManager& GetServiceAddons();
  static ADDON::CRepositoryUpdater& GetRepositoryUpdater();
  static CInputManager& GetInputManager();
  static CFileExtensionProvider &GetFileExtensionProvider();
  static bool IsBinaryAddonCacheUp();
  static bool IsServiceManagerUp();
  static CNetworkBase& GetNetwork();
  static CPowerManager& GetPowerManager();
  static CWeatherManager& GetWeatherManager();
  static CPlayerCoreFactory &GetPlayerCoreFactory();
  static CDatabaseManager &GetDatabaseManager();
  static CEventLog &GetEventLog();
  static CMediaManager& GetMediaManager();

  static CGUIComponent* GetGUI();
  static void RegisterGUI(CGUIComponent *gui);
  static void UnregisterGUI();

  static void RegisterSettingsComponent(CSettingsComponent *settings);
  static void UnregisterSettingsComponent();
  static CSettingsComponent* GetSettingsComponent();

  static void RegisterWinSystem(CWinSystemBase *winsystem);
  static void UnregisterWinSystem();
  static CWinSystemBase* GetWinSystem();
  static CRenderSystemBase* GetRenderSystem();

  static IAE* GetActiveAE();
  static void RegisterAE(IAE *ae);
  static void UnregisterAE();

  static std::shared_ptr<CAppInboundProtocol> GetAppPort();
  static void RegisterAppPort(std::shared_ptr<CAppInboundProtocol> port);
  static void UnregisterAppPort();

  static void RegisterDecoderFilterManager(CDecoderFilterManager* manager);
  static CDecoderFilterManager* GetDecoderFilterManager();

  static std::shared_ptr<CCPUInfo> GetCPUInfo();
  static void RegisterCPUInfo(std::shared_ptr<CCPUInfo> cpuInfo);
  static void UnregisterCPUInfo();

private:
  std::unique_ptr<CLog> m_logging;
  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  CGUIComponent* m_pGUI;
  CWinSystemBase* m_pWinSystem;
  IAE* m_pActiveAE;
  std::shared_ptr<CAppInboundProtocol> m_pAppPort;
  CSettingsComponent* m_pSettingsComponent;
  CDecoderFilterManager* m_decoderFilterManager;
  std::shared_ptr<CCPUInfo> m_cpuInfo;
};

XBMC_GLOBAL_REF(CServiceBroker, g_serviceBroker);
#define g_serviceBroker XBMC_GLOBAL_USE(CServiceBroker)
