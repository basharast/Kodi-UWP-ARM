/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "threads/SystemClock.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowFullScreenDefines.h"
#include "Application.h"
#include "ServiceBroker.h"
#include "messaging/ApplicationMessenger.h"
#include "GUIInfoManager.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIProgressControl.h"
#include "guilib/GUILabelControl.h"
#include "video/dialogs/GUIDialogSubtitleSettings.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "video/dialogs/GUIDialogFullScreenInfo.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "FileItem.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "windowing/WinSystem.h"
#include "cores/IPlayer.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "video/ViewModeSettings.h"

#include <stdio.h>
#include <algorithm>
#if defined(TARGET_DARWIN)
#include "platform/posix/PosixResourceCounter.h"
#endif

using namespace KODI::GUILIB;
using namespace KODI::MESSAGING;

#if defined(TARGET_DARWIN)
static CPosixResourceCounter m_resourceCounter;
#endif

CGUIWindowFullScreen::CGUIWindowFullScreen()
    : CGUIWindow(WINDOW_FULLSCREEN_VIDEO, "VideoFullScreen.xml")
{
  m_viewModeChanged = true;
  m_dwShowViewModeTimeout = 0;
  m_bShowCurrentTime = false;
  m_loadType = KEEP_IN_MEMORY;
  // audio
  //  - language
  //  - volume
  //  - stream

  // video
  //  - Create Bookmark (294)
  //  - Cycle bookmarks (295)
  //  - Clear bookmarks (296)
  //  - jump to specific time
  //  - slider
  //  - av delay

  // subtitles
  //  - delay
  //  - language

  m_controlStats = new GUICONTROLSTATS;
}

CGUIWindowFullScreen::~CGUIWindowFullScreen(void)
{
  delete m_controlStats;
}

bool CGUIWindowFullScreen::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
  case ACTION_SHOW_OSD:
    ToggleOSD();
    return true;

  case ACTION_TRIGGER_OSD:
    TriggerOSD();
    return true;

  case ACTION_MOUSE_MOVE:
    if (action.GetAmount(2) || action.GetAmount(3))
    {
      if (!g_application.GetAppPlayer().IsInMenu())
      {
        TriggerOSD();
        return true;
      }
    }
    break;

  case ACTION_MOUSE_LEFT_CLICK:
    if (!g_application.GetAppPlayer().IsInMenu())
    {
      TriggerOSD();
      return true;
    }
    break;

  case ACTION_SHOW_GUI:
    {
      // switch back to the menu
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
      return true;
    }
    break;

  case ACTION_SHOW_OSD_TIME:
    m_bShowCurrentTime = !m_bShowCurrentTime;
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetShowTime(m_bShowCurrentTime);
    return true;
    break;

  case ACTION_SHOW_INFO:
    {
      CGUIDialogFullScreenInfo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogFullScreenInfo>(WINDOW_DIALOG_FULLSCREEN_INFO);
      if (pDialog)
      {
        CFileItem item(g_application.CurrentFileItem());
        pDialog->Open();
        return true;
      }
      break;
    }

  case ACTION_ASPECT_RATIO:
    { // toggle the aspect ratio mode (only if the info is onscreen)
      if (m_dwShowViewModeTimeout)
      {
        CVideoSettings vs = g_application.GetAppPlayer().GetVideoSettings();
        vs.m_ViewMode = CViewModeSettings::GetNextQuickCycleViewMode(vs.m_ViewMode);
        g_application.GetAppPlayer().SetRenderViewMode(vs.m_ViewMode, vs.m_CustomZoomAmount,
                                                   vs.m_CustomPixelRatio, vs.m_CustomVerticalShift,
                                                   vs.m_CustomNonLinStretch);
      }
      else
        m_viewModeChanged = true;
      m_dwShowViewModeTimeout = XbmcThreads::SystemClockMillis();
    }
    return true;
    break;
  case ACTION_SHOW_PLAYLIST:
    {
      CFileItem item(g_application.CurrentFileItem());
      if (item.HasPVRChannelInfoTag())
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
      else if (item.HasVideoInfoTag())
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_PLAYLIST);
      else if (item.HasMusicInfoTag())
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_MUSIC_PLAYLIST);
    }
    return true;
    break;
  case ACTION_BROWSE_SUBTITLE:
    {
      std::string path = CGUIDialogSubtitleSettings::BrowseForSubtitle();
      if (!path.empty())
        g_application.GetAppPlayer().AddSubtitle(path);
      return true;
    }
  default:
      break;
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowFullScreen::ClearBackground()
{
  if (g_application.GetAppPlayer().IsRenderingVideoLayer())
    CServiceBroker::GetWinSystem()->GetGfxContext().Clear(0);
}

void CGUIWindowFullScreen::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  // override the clear colour - we must never clear fullscreen
  m_clearBackground = 0;

  CGUIProgressControl* pProgress = dynamic_cast<CGUIProgressControl*>(GetControl(CONTROL_PROGRESS));
  if (pProgress)
  {
    if( pProgress->GetInfo() == 0 || !pProgress->HasVisibleCondition())
    {
      pProgress->SetInfo(PLAYER_PROGRESS);
      pProgress->SetVisibleCondition("player.displayafterseek");
      pProgress->SetVisible(true);
    }
  }

  CGUILabelControl* pLabel = dynamic_cast<CGUILabelControl*>(GetControl(LABEL_BUFFERING));
  if(pLabel && !pLabel->HasVisibleCondition())
  {
    pLabel->SetVisibleCondition("player.caching");
    pLabel->SetVisible(true);
  }

  pLabel = dynamic_cast<CGUILabelControl*>(GetControl(LABEL_CURRENT_TIME));
  if(pLabel && !pLabel->HasVisibleCondition())
  {
    pLabel->SetVisibleCondition("player.displayafterseek");
    pLabel->SetVisible(true);
    pLabel->SetLabel("$INFO(VIDEOPLAYER.TIME) / $INFO(VIDEOPLAYER.DURATION)");
  }
}

bool CGUIWindowFullScreen::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // check whether we've come back here from a window during which time we've actually
      // stopped playing videos
      if (message.GetParam1() == WINDOW_INVALID && !g_application.GetAppPlayer().IsPlayingVideo())
      { // why are we here if nothing is playing???
        CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();
        return true;
      }

      GUIINFO::CPlayerGUIInfo& guiInfo = CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider();
      guiInfo.SetShowInfo(false);
      m_bShowCurrentTime = false;
      guiInfo.SetDisplayAfterSeek(0); // Make sure display after seek is off.

      // switch resolution
      CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(true);

      // now call the base class to load our windows
      CGUIWindow::OnMessage(message);

      m_dwShowViewModeTimeout = 0;
      m_viewModeChanged = true;


      return true;
    }
  case GUI_MSG_WINDOW_DEINIT:
    {
      // close all active modal dialogs
      CServiceBroker::GetGUI()->GetWindowManager().CloseInternalModalDialogs(true);

      CGUIWindow::OnMessage(message);

      CServiceBroker::GetSettingsComponent()->GetSettings()->Save();

      CServiceBroker::GetWinSystem()->GetGfxContext().SetFullScreenVideo(false);

      return true;
    }
  case GUI_MSG_SETFOCUS:
  case GUI_MSG_LOSTFOCUS:
    if (message.GetSenderId() != WINDOW_FULLSCREEN_VIDEO) return true;
    break;
  }

  return CGUIWindow::OnMessage(message);
}

EVENT_RESULT CGUIWindowFullScreen::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_RIGHT_CLICK)
  { // no control found to absorb this click - go back to GUI
    OnAction(CAction(ACTION_SHOW_GUI));
    return EVENT_RESULT_HANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_FORWARD, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_BACK, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id >= ACTION_GESTURE_NOTIFY && event.m_id <= ACTION_GESTURE_END) // gestures
    return EVENT_RESULT_UNHANDLED;
  return EVENT_RESULT_UNHANDLED;
}

void CGUIWindowFullScreen::FrameMove()
{
  float playspeed = g_application.GetAppPlayer().GetPlaySpeed();
  if (playspeed != 1.0 && !g_application.GetAppPlayer().HasGame() &&
      !g_application.GetAppPlayer().IsPausedPlayback())
    CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetDisplayAfterSeek();

  if (!g_application.GetAppPlayer().HasPlayer())
    return;

  //----------------------
  // ViewMode Information
  //----------------------
  if (m_dwShowViewModeTimeout && XbmcThreads::SystemClockMillis() - m_dwShowViewModeTimeout > 2500)
  {
    m_dwShowViewModeTimeout = 0;
    m_viewModeChanged = true;
  }

  if (m_dwShowViewModeTimeout)
  {
    RESOLUTION_INFO res = CServiceBroker::GetWinSystem()->GetGfxContext().GetResInfo();

    {
      // get the "View Mode" string
      const std::string& strTitle = g_localizeStrings.Get(629);
      const auto& vs = g_application.GetAppPlayer().GetVideoSettings();
      int sId = CViewModeSettings::GetViewModeStringIndex(vs.m_ViewMode);
      const std::string& strMode = g_localizeStrings.Get(sId);
      std::string strInfo = StringUtils::Format("%s : %s", strTitle.c_str(), strMode.c_str());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW1);
      msg.SetLabel(strInfo);
      OnMessage(msg);
    }
    // show sizing information
    VideoStreamInfo info;
    g_application.GetAppPlayer().GetVideoStreamInfo(CURRENT_STREAM,info);
    {
      // Splitres scaling factor
      float xscale = (float)res.iScreenWidth  / (float)res.iWidth;
      float yscale = (float)res.iScreenHeight / (float)res.iHeight;

      std::string strSizing = StringUtils::Format(g_localizeStrings.Get(245).c_str(),
                                                 (int)info.SrcRect.Width(),
                                                 (int)info.SrcRect.Height(),
                                                 (int)(info.DestRect.Width() * xscale),
                                                 (int)(info.DestRect.Height() * yscale),
                                                 CDisplaySettings::GetInstance().GetZoomAmount(),
                                                 info.videoAspectRatio*CDisplaySettings::GetInstance().GetPixelRatio(),
                                                 CDisplaySettings::GetInstance().GetPixelRatio(),
                                                 CDisplaySettings::GetInstance().GetVerticalShift());
      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW2);
      msg.SetLabel(strSizing);
      OnMessage(msg);
    }
    // show resolution information
    {
      std::string strStatus;
      if (CServiceBroker::GetWinSystem()->IsFullScreen())
        strStatus = StringUtils::Format("%s %ix%i@%.2fHz - %s",
                                        g_localizeStrings.Get(13287).c_str(),
                                        res.iScreenWidth,
                                        res.iScreenHeight,
                                        res.fRefreshRate,
                                        g_localizeStrings.Get(244).c_str());
      else
        strStatus = StringUtils::Format("%s %ix%i - %s",
                                        g_localizeStrings.Get(13287).c_str(),
                                        res.iScreenWidth,
                                        res.iScreenHeight,
                                        g_localizeStrings.Get(242).c_str());

      CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), LABEL_ROW3);
      msg.SetLabel(strStatus);
      OnMessage(msg);
    }
  }

  if (m_viewModeChanged)
  {
    if (m_dwShowViewModeTimeout)
    {
      SET_CONTROL_VISIBLE(LABEL_ROW1);
      SET_CONTROL_VISIBLE(LABEL_ROW2);
      SET_CONTROL_VISIBLE(LABEL_ROW3);
      SET_CONTROL_VISIBLE(BLUE_BAR);
    }
    else
    {
      SET_CONTROL_HIDDEN(LABEL_ROW1);
      SET_CONTROL_HIDDEN(LABEL_ROW2);
      SET_CONTROL_HIDDEN(LABEL_ROW3);
      SET_CONTROL_HIDDEN(BLUE_BAR);
    }
    m_viewModeChanged = false;
  }
}

void CGUIWindowFullScreen::Process(unsigned int currentTime, CDirtyRegionList &dirtyregion)
{
  if (g_application.GetAppPlayer().IsRenderingGuiLayer())
    MarkDirtyRegion();

  m_controlStats->Reset();

  CGUIWindow::Process(currentTime, dirtyregion);

  //! @todo This isn't quite optimal - ideally we'd only be dirtying up the actual video render rect
  //!       which is probably the job of the renderer as it can more easily track resizing etc.
  m_renderRegion.SetRect(0, 0, (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetWidth(), (float)CServiceBroker::GetWinSystem()->GetGfxContext().GetHeight());
}

void CGUIWindowFullScreen::Render()
{
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), false);
  g_application.GetAppPlayer().Render(true, 255);
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(m_coordsRes, m_needsScaling);
  CGUIWindow::Render();
}

void CGUIWindowFullScreen::RenderEx()
{
  CGUIWindow::RenderEx();
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(CServiceBroker::GetWinSystem()->GetGfxContext().GetVideoResolution(), false);
  g_application.GetAppPlayer().Render(false, 255, false);
  CServiceBroker::GetWinSystem()->GetGfxContext().SetRenderingResolution(m_coordsRes, m_needsScaling);
}

void CGUIWindowFullScreen::SeekChapter(int iChapter)
{
  g_application.GetAppPlayer().SeekChapter(iChapter);

  // Make sure gui items are visible.
  CServiceBroker::GetGUI()->GetInfoManager().GetInfoProviders().GetPlayerInfoProvider().SetDisplayAfterSeek();
}

void CGUIWindowFullScreen::ToggleOSD()
{
  CGUIDialog *pOSD = GetOSD();
  if (pOSD)
  {
    if (pOSD->IsDialogRunning())
      pOSD->Close();
    else
      pOSD->Open();
  }

  MarkDirtyRegion();
}

void CGUIWindowFullScreen::TriggerOSD()
{
  CGUIDialog *pOSD = GetOSD();
  if (pOSD && !pOSD->IsDialogRunning())
  {
    if (!g_application.GetAppPlayer().IsPlayingGame())
      pOSD->SetAutoClose(3000);
    pOSD->Open();
  }
}

bool CGUIWindowFullScreen::HasVisibleControls()
{
  return m_controlStats->nCountVisible > 0;
}

CGUIDialog *CGUIWindowFullScreen::GetOSD()
{
  return CServiceBroker::GetGUI()->GetWindowManager().GetDialog(WINDOW_DIALOG_VIDEO_OSD);
}
