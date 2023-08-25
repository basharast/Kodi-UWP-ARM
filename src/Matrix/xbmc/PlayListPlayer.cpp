/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListPlayer.h"

#include "Application.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/VideoDatabaseFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

using namespace PLAYLIST;
using namespace KODI::MESSAGING;

CPlayListPlayer::CPlayListPlayer(void)
{
  m_PlaylistMusic = new CPlayList(PLAYLIST_MUSIC);
  m_PlaylistVideo = new CPlayList(PLAYLIST_VIDEO);
  m_PlaylistEmpty = new CPlayList;
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;
  m_bPlaybackStarted = false;
  m_iCurrentPlayList = PLAYLIST_NONE;
  for (REPEAT_STATE& repeatState : m_repeatState)
    repeatState = REPEAT_NONE;
  m_iFailedSongs = 0;
  m_failedSongsStart = 0;
}

CPlayListPlayer::~CPlayListPlayer(void)
{
  Clear();
  delete m_PlaylistMusic;
  delete m_PlaylistVideo;
  delete m_PlaylistEmpty;
}

bool CPlayListPlayer::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_PREV_ITEM && !IsSingleItemNonRepeatPlaylist())
  {
    PlayPrevious();
    return true;
  }
  else if (action.GetID() == ACTION_NEXT_ITEM && !IsSingleItemNonRepeatPlaylist())
  {
    PlayNext();
    return true;
  }
  else
    return false;
}

bool CPlayListPlayer::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_NOTIFY_ALL:
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      // update the items in our playlist(s) if necessary
      for (int i = PLAYLIST_MUSIC; i <= PLAYLIST_VIDEO; i++)
      {
        CPlayList &playlist = GetPlaylist(i);
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        playlist.UpdateItem(item.get());
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STOPPED:
    {
      if (m_iCurrentPlayList != PLAYLIST_NONE && m_bPlaybackStarted)
      {
        CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
        CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
        Reset();
        m_iCurrentPlayList = PLAYLIST_NONE;
        return true;
      }
    }
    break;
  case GUI_MSG_PLAYBACK_STARTED:
    {
      m_bPlaybackStarted = true;
    }
    break;
  }

  return false;
}

int CPlayListPlayer::GetNextSong(int offset) const
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return -1;

  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;

  int song = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == PLAYLIST_MUSIC)
    return song + offset;

  // wrap around in the case of repeating
  if (RepeatedOne(m_iCurrentPlayList))
    return song;

  song += offset;
  if (song >= playlist.size() && Repeated(m_iCurrentPlayList))
    song %= playlist.size();

  return song;
}

int CPlayListPlayer::GetNextSong()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return -1;
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return -1;
  int iSong = m_iCurrentSong;

  // party mode
  if (g_partyModeManager.IsEnabled() && GetCurrentPlaylist() == PLAYLIST_MUSIC)
    return iSong + 1;

  // if repeat one, keep playing the current song if its valid
  if (RepeatedOne(m_iCurrentPlayList))
  {
    // otherwise immediately abort playback
    if (m_iCurrentSong >= 0 && m_iCurrentSong < playlist.size() && playlist[m_iCurrentSong]->GetProperty("unplayable").asBoolean())
    {
      CLog::Log(LOGERROR,"Playlist Player: RepeatOne stuck on unplayable item: %i, path [%s]", m_iCurrentSong, playlist[m_iCurrentSong]->GetPath().c_str());
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return -1;
    }
    return iSong;
  }

  // if we've gone beyond the playlist and repeat all is enabled,
  // then we clear played status and wrap around
  iSong++;
  if (iSong >= playlist.size() && Repeated(m_iCurrentPlayList))
    iSong = 0;

  return iSong;
}

bool CPlayListPlayer::PlayNext(int offset, bool bAutoPlay)
{
  int iSong = GetNextSong(offset);
  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);

  if ((iSong < 0) || (iSong >= playlist.size()) || (playlist.GetPlayable() <= 0))
  {
    if(!bAutoPlay)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(34201));

    CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
    CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
    Reset();
    m_iCurrentPlayList = PLAYLIST_NONE;
    return false;
  }

  return Play(iSong, "", false);
}

bool CPlayListPlayer::PlayPrevious()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  int iSong = m_iCurrentSong;

  if (!RepeatedOne(m_iCurrentPlayList))
    iSong--;

  if (iSong < 0 && Repeated(m_iCurrentPlayList))
    iSong = playlist.size() - 1;

  if (iSong < 0 || playlist.size() <= 0)
  {
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(34202));
    return false;
  }

  return Play(iSong, "", false, true);
}

bool CPlayListPlayer::IsSingleItemNonRepeatPlaylist() const
{
  const CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  return (playlist.size() <= 1 && !RepeatedOne(m_iCurrentPlayList) && !Repeated(m_iCurrentPlayList));
}

bool CPlayListPlayer::Play()
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return false;

  return Play(0, "");
}

bool CPlayListPlayer::PlaySongId(int songId)
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return Play();

  for (int i = 0; i < playlist.size(); i++)
  {
    if (playlist[i]->HasMusicInfoTag() && playlist[i]->GetMusicInfoTag()->GetDatabaseId() == songId)
      return Play(i, "");
  }
  return Play();
}

bool CPlayListPlayer::Play(const CFileItemPtr& pItem, const std::string& player)
{
  int playlist;
  bool isVideo{pItem->IsVideo()};
  bool isAudio{pItem->IsAudio()};
  if (isVideo && isAudio && pItem->HasProperty("playlist_type_hint"))
  {
    // If an extension is set in both audio / video lists (e.g. playlist .strm),
    // is not possible detect the type of playlist then we rely on the hint
    playlist = pItem->GetProperty("playlist_type_hint").asInteger32(PLAYLIST_NONE);
  }
  else if (isAudio)
    playlist = PLAYLIST_MUSIC;
  else if (isVideo)
    playlist = PLAYLIST_VIDEO;
  else
  {
    CLog::Log(
        LOGWARNING,
        "Playlist Player: ListItem type must be audio or video, use ListItem::setInfo to specify!");
    return false;
  }

  ClearPlaylist(playlist);
  Reset();
  SetCurrentPlaylist(playlist);
  Add(playlist, pItem);

  return Play(0, player);
}

bool CPlayListPlayer::Play(int iSong,
                           const std::string& player,
                           bool bAutoPlay /* = false */,
                           bool bPlayPrevious /* = false */)
{
  if (m_iCurrentPlayList == PLAYLIST_NONE)
    return false;

  CPlayList& playlist = GetPlaylist(m_iCurrentPlayList);
  if (playlist.size() <= 0)
    return false;
  if (iSong < 0)
    iSong = 0;
  if (iSong >= playlist.size())
    iSong = playlist.size() - 1;

  // check if the item itself is a playlist, and can be expanded
  // only allow a few levels, this could end up in a loop
  // if they refer to each other in a loop
  for (int i=0; i<5; i++)
  {
    if(!playlist.Expand(iSong))
      break;
  }

  m_iCurrentSong = iSong;
  CFileItemPtr item = playlist[m_iCurrentSong];
  if (item->IsVideoDb() && !item->HasVideoInfoTag())
    *(item->GetVideoInfoTag()) = XFILE::CVideoDatabaseFile::GetVideoTag(CURL(item->GetDynPath()));

  playlist.SetPlayed(true);

  m_bPlaybackStarted = false;

  unsigned int playAttempt = XbmcThreads::SystemClockMillis();
  bool ret = g_application.PlayFile(*item, player, bAutoPlay);
  if (!ret)
  {
    CLog::Log(LOGERROR,"Playlist Player: skipping unplayable item: %i, path [%s]", m_iCurrentSong, CURL::GetRedacted(item->GetDynPath()).c_str());
    playlist.SetUnPlayable(m_iCurrentSong);

    // abort on 100 failed CONSECTUTIVE songs
    if (!m_iFailedSongs)
      m_failedSongsStart = playAttempt;
    m_iFailedSongs++;
    const std::shared_ptr<CAdvancedSettings> advancedSettings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
    if ((m_iFailedSongs >= advancedSettings->m_playlistRetries && advancedSettings->m_playlistRetries >= 0)
        || ((XbmcThreads::SystemClockMillis() - m_failedSongsStart  >= static_cast<unsigned int>(advancedSettings->m_playlistTimeout) * 1000) &&
            advancedSettings->m_playlistTimeout))
    {
      CLog::Log(LOGDEBUG,"Playlist Player: one or more items failed to play... aborting playback");

      // open error dialog
      HELPERS::ShowOKDialogText(CVariant{16026}, CVariant{16027});

      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      Reset();
      GetPlaylist(m_iCurrentPlayList).Clear();
      m_iCurrentPlayList = PLAYLIST_NONE;
      m_iFailedSongs = 0;
      m_failedSongsStart = 0;
      return false;
    }

    // how many playable items are in the playlist?
    if (playlist.GetPlayable() > 0)
    {
      return bPlayPrevious ? PlayPrevious() : PlayNext();
    }
    // none? then abort playback
    else
    {
      CLog::Log(LOGDEBUG,"Playlist Player: no more playable items... aborting playback");
      CGUIMessage msg(GUI_MSG_PLAYLISTPLAYER_STOPPED, 0, 0, m_iCurrentPlayList, m_iCurrentSong);
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
      Reset();
      m_iCurrentPlayList = PLAYLIST_NONE;
      return false;
    }
  }

  // reset the start offset of this item
  if (item->m_lStartOffset == STARTOFFSET_RESUME)
    item->m_lStartOffset = 0;

  //! @todo - move the above failure logic and the below success logic
  //!        to callbacks instead so we don't rely on the return value
  //!        of PlayFile()

  // consecutive error counter so reset if the current item is playing
  m_iFailedSongs = 0;
  m_failedSongsStart = 0;
  m_bPlayedFirstFile = true;
  return true;
}

void CPlayListPlayer::SetCurrentSong(int iSong)
{
  if (iSong >= -1 && iSong < GetPlaylist(m_iCurrentPlayList).size())
    m_iCurrentSong = iSong;
}

int CPlayListPlayer::GetCurrentSong() const
{
  return m_iCurrentSong;
}

int CPlayListPlayer::GetCurrentPlaylist() const
{
  return m_iCurrentPlayList;
}

void CPlayListPlayer::SetCurrentPlaylist(int iPlaylist)
{
  if (iPlaylist == m_iCurrentPlayList)
    return;

  // changing the current playlist while party mode is on
  // disables party mode
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  m_iCurrentPlayList = iPlaylist;
  m_bPlayedFirstFile = false;
}

void CPlayListPlayer::ClearPlaylist(int iPlaylist)
{
  // clear our applications playlist file
  g_application.m_strPlayListFile.clear();

  CPlayList& playlist = GetPlaylist(iPlaylist);
  playlist.Clear();

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

CPlayList& CPlayListPlayer::GetPlaylist(int iPlaylist)
{
  switch ( iPlaylist )
  {
  case PLAYLIST_MUSIC:
    return *m_PlaylistMusic;
    break;
  case PLAYLIST_VIDEO:
    return *m_PlaylistVideo;
    break;
  default:
    m_PlaylistEmpty->Clear();
    return *m_PlaylistEmpty;
    break;
  }
}

const CPlayList& CPlayListPlayer::GetPlaylist(int iPlaylist) const
{
  switch ( iPlaylist )
  {
  case PLAYLIST_MUSIC:
    return *m_PlaylistMusic;
    break;
  case PLAYLIST_VIDEO:
    return *m_PlaylistVideo;
    break;
  default:
    // NOTE: This playlist may not be empty if the caller of the non-const version alters it!
    return *m_PlaylistEmpty;
    break;
  }
}

int CPlayListPlayer::RemoveDVDItems()
{
  int nRemovedM = m_PlaylistMusic->RemoveDVDItems();
  int nRemovedV = m_PlaylistVideo->RemoveDVDItems();

  return nRemovedM + nRemovedV;
}

void CPlayListPlayer::Reset()
{
  m_iCurrentSong = -1;
  m_bPlayedFirstFile = false;
  m_bPlaybackStarted = false;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

bool CPlayListPlayer::HasPlayedFirstFile() const
{
  return m_bPlayedFirstFile;
}

bool CPlayListPlayer::Repeated(int iPlaylist) const
{
  if (iPlaylist >= PLAYLIST_MUSIC && iPlaylist <= PLAYLIST_VIDEO)
    return (m_repeatState[iPlaylist] == REPEAT_ALL);
  return false;
}

bool CPlayListPlayer::RepeatedOne(int iPlaylist) const
{
  if (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_VIDEO)
    return (m_repeatState[iPlaylist] == REPEAT_ONE);
  return false;
}

void CPlayListPlayer::SetShuffle(int iPlaylist, bool bYesNo, bool bNotify /* = false */)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;

  // disable shuffle in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    return;

  // do we even need to do anything?
  if (bYesNo != IsShuffled(iPlaylist))
  {
    // save the order value of the current song so we can use it find its new location later
    int iOrder = -1;
    CPlayList &playlist = GetPlaylist(iPlaylist);
    if (m_iCurrentSong >= 0 && m_iCurrentSong < playlist.size())
      iOrder = playlist[m_iCurrentSong]->m_iprogramCount;

    // shuffle or unshuffle as necessary
    if (bYesNo)
      playlist.Shuffle();
    else
      playlist.UnShuffle();

    if (bNotify)
    {
      std::string shuffleStr = StringUtils::Format("%s: %s", g_localizeStrings.Get(191).c_str(), g_localizeStrings.Get(bYesNo ? 593 : 591).c_str()); // Shuffle: All/Off
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559),  shuffleStr);
    }

    // find the previous order value and fix the current song marker
    if (iOrder >= 0)
    {
      int iIndex = playlist.FindOrder(iOrder);
      if (iIndex >= 0)
        m_iCurrentSong = iIndex;
      // if iIndex < 0, something unexpected happened
      // so dont do anything
    }
  }

  // its likely that the playlist changed   
  if (CServiceBroker::GetGUI() != nullptr)
  {
    CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }

  AnnouncePropertyChanged(iPlaylist, "shuffled", IsShuffled(iPlaylist));
}

bool CPlayListPlayer::IsShuffled(int iPlaylist) const
{
  // even if shuffled, party mode says its not
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    return false;

  if (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_VIDEO)
    return GetPlaylist(iPlaylist).IsShuffled();

  return false;
}

void CPlayListPlayer::SetRepeat(int iPlaylist, REPEAT_STATE state, bool bNotify /* = false */)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;

  // disable repeat in party mode
  if (g_partyModeManager.IsEnabled() && iPlaylist == PLAYLIST_MUSIC)
    state = REPEAT_NONE;

  // notify the user if there was a change in the repeat state
  if (m_repeatState[iPlaylist] != state && bNotify)
  {
    int iLocalizedString;
    if (state == REPEAT_NONE)
      iLocalizedString = 595; // Repeat: Off
    else if (state == REPEAT_ONE)
      iLocalizedString = 596; // Repeat: One
    else
      iLocalizedString = 597; // Repeat: All
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559), g_localizeStrings.Get(iLocalizedString));
  }

  m_repeatState[iPlaylist] = state;

  CVariant data;
  switch (state)
  {
  case REPEAT_ONE:
    data = "one";
    break;
  case REPEAT_ALL:
    data = "all";
    break;
  default:
    data = "off";
    break;
  }

  // its likely that the playlist changed   
  if (CServiceBroker::GetGUI() != nullptr)
  {
    CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }

  AnnouncePropertyChanged(iPlaylist, "repeat", data);
}

REPEAT_STATE CPlayListPlayer::GetRepeat(int iPlaylist) const
{
  if (iPlaylist == PLAYLIST_MUSIC || iPlaylist == PLAYLIST_VIDEO)
    return m_repeatState[iPlaylist];
  return REPEAT_NONE;
}

void CPlayListPlayer::ReShuffle(int iPlaylist, int iPosition)
{
  // playlist has not played yet so shuffle the entire list
  // (this only really works for new video playlists)
  if (!GetPlaylist(iPlaylist).WasPlayed())
  {
    GetPlaylist(iPlaylist).Shuffle();
  }
  // we're trying to shuffle new items into the currently playing playlist
  // so we shuffle starting at two positions below the current item
  else if (iPlaylist == m_iCurrentPlayList)
  {
    if (
      (g_application.GetAppPlayer().IsPlayingAudio() && iPlaylist == PLAYLIST_MUSIC) ||
      (g_application.GetAppPlayer().IsPlayingVideo() && iPlaylist == PLAYLIST_VIDEO)
      )
    {
      GetPlaylist(iPlaylist).Shuffle(m_iCurrentSong + 2);
    }
  }
  // otherwise, shuffle from the passed position
  // which is the position of the first new item added
  else
  {
    GetPlaylist(iPlaylist).Shuffle(iPosition);
  }
}

void CPlayListPlayer::Add(int iPlaylist, const CPlayList& playlist)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(playlist);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, const CFileItemPtr &pItem)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(pItem);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
}

void CPlayListPlayer::Add(int iPlaylist, const CFileItemList& items)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Add(items);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::Insert(int iPlaylist, const CPlayList& playlist, int iIndex)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Insert(playlist, iIndex);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
  else if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Insert(int iPlaylist, const CFileItemPtr &pItem, int iIndex)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Insert(pItem, iIndex);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
  else if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;
}

void CPlayListPlayer::Insert(int iPlaylist, const CFileItemList& items, int iIndex)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  int iSize = list.size();
  list.Insert(items, iIndex);
  if (list.IsShuffled())
    ReShuffle(iPlaylist, iSize);
  else if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iIndex)
    m_iCurrentSong++;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::Remove(int iPlaylist, int iPosition)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;
  CPlayList& list = GetPlaylist(iPlaylist);
  list.Remove(iPosition);
  if (m_iCurrentPlayList == iPlaylist && m_iCurrentSong >= iPosition)
    m_iCurrentSong--;

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::Clear()
{
  if (m_PlaylistMusic)
    m_PlaylistMusic->Clear();
  if (m_PlaylistVideo)
    m_PlaylistVideo->Clear();
  if (m_PlaylistEmpty)
    m_PlaylistEmpty->Clear();
}

void CPlayListPlayer::Swap(int iPlaylist, int indexItem1, int indexItem2)
{
  if (iPlaylist != PLAYLIST_MUSIC && iPlaylist != PLAYLIST_VIDEO)
    return;

  CPlayList& list = GetPlaylist(iPlaylist);
  if (list.Swap(indexItem1, indexItem2) && iPlaylist == m_iCurrentPlayList)
  {
    if (m_iCurrentSong == indexItem1)
      m_iCurrentSong = indexItem2;
    else if (m_iCurrentSong == indexItem2)
      m_iCurrentSong = indexItem1;
  }

  // its likely that the playlist changed
  CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
}

void CPlayListPlayer::AnnouncePropertyChanged(int iPlaylist, const std::string &strProperty, const CVariant &value)
{
  if (strProperty.empty() || value.isNull() ||
     (iPlaylist == PLAYLIST_VIDEO && !g_application.GetAppPlayer().IsPlayingVideo()) ||
     (iPlaylist == PLAYLIST_MUSIC && !g_application.GetAppPlayer().IsPlayingAudio()))
    return;

  CVariant data;
  data["player"]["playerid"] = iPlaylist;
  data["property"][strProperty] = value;
  CServiceBroker::GetAnnouncementManager()->Announce(ANNOUNCEMENT::Player, "OnPropertyChanged",
                                                     data);
}

int PLAYLIST::CPlayListPlayer::GetMessageMask()
{
  return TMSG_MASK_PLAYLISTPLAYER;
}

void PLAYLIST::CPlayListPlayer::OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_PLAYLISTPLAYER_PLAY:
    if (pMsg->param1 != -1)
      Play(pMsg->param1, "");
    else
      Play();
    break;

  case TMSG_PLAYLISTPLAYER_PLAY_SONG_ID:
    if (pMsg->param1 != -1)
    {
      bool *result = (bool*)pMsg->lpVoid;
      *result = PlaySongId(pMsg->param1);
    }
    else
      Play();
    break;

  case TMSG_PLAYLISTPLAYER_NEXT:
    PlayNext();
    break;

  case TMSG_PLAYLISTPLAYER_PREV:
    PlayPrevious();
    break;

  case TMSG_PLAYLISTPLAYER_ADD:
    if (pMsg->lpVoid)
    {
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      Add(pMsg->param1, (*list));
      delete list;
    }
    break;

  case TMSG_PLAYLISTPLAYER_INSERT:
    if (pMsg->lpVoid)
    {
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);
      Insert(pMsg->param1, (*list), pMsg->param2);
      delete list;
    }
    break;

  case TMSG_PLAYLISTPLAYER_REMOVE:
    if (pMsg->param1 != -1)
      Remove(pMsg->param1, pMsg->param2);
    break;

  case TMSG_PLAYLISTPLAYER_CLEAR:
    ClearPlaylist(pMsg->param1);
    break;

  case TMSG_PLAYLISTPLAYER_SHUFFLE:
    SetShuffle(pMsg->param1, pMsg->param2 > 0);
    break;

  case TMSG_PLAYLISTPLAYER_REPEAT:
    SetRepeat(pMsg->param1, (PLAYLIST::REPEAT_STATE)pMsg->param2);
    break;

  case TMSG_PLAYLISTPLAYER_GET_ITEMS:
    if (pMsg->lpVoid)
    {
      PLAYLIST::CPlayList playlist = GetPlaylist(pMsg->param1);
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      for (int i = 0; i < playlist.size(); i++)
        list->Add(std::make_shared<CFileItem>(*playlist[i]));
    }
    break;

  case TMSG_PLAYLISTPLAYER_SWAP:
    if (pMsg->lpVoid)
    {
      auto indexes = static_cast<std::vector<int>*>(pMsg->lpVoid);
      if (indexes->size() == 2)
        Swap(pMsg->param1, indexes->at(0), indexes->at(1));
      delete indexes;
    }
    break;

  case TMSG_MEDIA_PLAY:
  {
    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    // first check if we were called from the PlayFile() function
    if (pMsg->lpVoid && pMsg->param2 == 0)
    {
      // Discard the current playlist, if TMSG_MEDIA_PLAY gets posted with just a single item.
      // Otherwise items may fail to play, when started while a playlist is playing.
      Reset();

      CFileItem *item = static_cast<CFileItem*>(pMsg->lpVoid);
      g_application.PlayFile(*item, "", pMsg->param1 != 0);
      delete item;
      return;
    }

    //g_application.StopPlaying();
    // play file
    if (pMsg->lpVoid)
    {
      CFileItemList *list = static_cast<CFileItemList*>(pMsg->lpVoid);

      if (list->Size() > 0)
      {
        int playlist = PLAYLIST_MUSIC;
        for (int i = 0; i < list->Size(); i++)
        {
          if ((*list)[i]->IsVideo())
          {
            playlist = PLAYLIST_VIDEO;
            break;
          }
        }

        ClearPlaylist(playlist);
        SetCurrentPlaylist(playlist);
        if (list->Size() == 1 && !(*list)[0]->IsPlayList())
        {
          CFileItemPtr item = (*list)[0];
          // if the item is a plugin we need to resolve the URL to ensure the infotags are filled.
          // resolve only for a maximum of 5 times to avoid deadlocks (plugin:// paths can resolve to plugin:// paths)
          for (int i = 0; URIUtils::IsPlugin(item->GetDynPath()) && i < 5; ++i)
          {
            if (!XFILE::CPluginDirectory::GetPluginResult(item->GetDynPath(), *item, true))
              return;
          }
          if (item->IsAudio() || item->IsVideo())
            Play(item, pMsg->strParam);
          else
            g_application.PlayMedia(*item, pMsg->strParam, playlist);
        }
        else
        {
          // Handle "shuffled" option if present
          if (list->HasProperty("shuffled") && list->GetProperty("shuffled").isBoolean())
            SetShuffle(playlist, list->GetProperty("shuffled").asBoolean(), false);
          // Handle "repeat" option if present
          if (list->HasProperty("repeat") && list->GetProperty("repeat").isInteger())
            SetRepeat(playlist, (PLAYLIST::REPEAT_STATE)list->GetProperty("repeat").asInteger(), false);

          Add(playlist, (*list));
          Play(pMsg->param1, pMsg->strParam);
        }
      }

      delete list;
    }
    else if (pMsg->param1 == PLAYLIST_MUSIC || pMsg->param1 == PLAYLIST_VIDEO)
    {
      if (GetCurrentPlaylist() != pMsg->param1)
        SetCurrentPlaylist(pMsg->param1);

      CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, pMsg->param2);
    }
  }
  break;

  case TMSG_MEDIA_RESTART:
    g_application.Restart(true);
    break;

  case TMSG_MEDIA_STOP:
  {
    // restore to previous window if needed
    bool stopSlideshow = true;
    bool stopVideo = true;
    bool stopMusic = true;
    if (pMsg->param1 >= PLAYLIST_MUSIC && pMsg->param1 <= PLAYLIST_PICTURE)
    {
      stopSlideshow = (pMsg->param1 == PLAYLIST_PICTURE);
      stopVideo = (pMsg->param1 == PLAYLIST_VIDEO);
      stopMusic = (pMsg->param1 == PLAYLIST_MUSIC);
    }

    if ((stopSlideshow && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_SLIDESHOW) ||
      (stopVideo && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO) ||
      (stopVideo && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME) ||
      (stopMusic && CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VISUALISATION))
      CServiceBroker::GetGUI()->GetWindowManager().PreviousWindow();

    g_application.ResetScreenSaver();
    g_application.WakeUpScreenSaverAndDPMS();

    // stop playing file
    if (g_application.GetAppPlayer().IsPlaying())
      g_application.StopPlaying();
  }
  break;

  case TMSG_MEDIA_PAUSE:
    if (g_application.GetAppPlayer().HasPlayer())
    {
      g_application.ResetScreenSaver();
      g_application.WakeUpScreenSaverAndDPMS();
      g_application.GetAppPlayer().Pause();
    }
    break;

  case TMSG_MEDIA_UNPAUSE:
    if (g_application.GetAppPlayer().IsPausedPlayback())
    {
      g_application.ResetScreenSaver();
      g_application.WakeUpScreenSaverAndDPMS();
      g_application.GetAppPlayer().Pause();
    }
    break;

  case TMSG_MEDIA_PAUSE_IF_PLAYING:
    if (g_application.GetAppPlayer().IsPlaying() && !g_application.GetAppPlayer().IsPaused())
    {
      g_application.ResetScreenSaver();
      g_application.WakeUpScreenSaverAndDPMS();
      g_application.GetAppPlayer().Pause();
    }
    break;

  case TMSG_MEDIA_SEEK_TIME:
  {
    CApplicationPlayer& player = g_application.GetAppPlayer();
    if (player.IsPlaying() || player.IsPaused())
      player.SeekTime(pMsg->param3);

    break;
  }
  default:
    break;
  }
}
