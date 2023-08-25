/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "guilib/guiinfo/VideoGUIInfo.h"

#include "Application.h"
#include "FileItem.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/DataCacheCore.h"
#include "cores/VideoPlayer/VideoRenderers/BaseRenderer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "guilib/WindowIDs.h"
#include "guilib/guiinfo/GUIInfo.h"
#include "guilib/guiinfo/GUIInfoHelper.h"
#include "guilib/guiinfo/GUIInfoLabels.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"

#include <math.h>

using namespace KODI::GUILIB;
using namespace KODI::GUILIB::GUIINFO;

int CVideoGUIInfo::GetPercentPlayed(const CVideoInfoTag* tag) const
{
  CBookmark bookmark = tag->GetResumePoint();
  if (bookmark.IsPartWay())
    return std::lrintf(static_cast<float>(bookmark.timeInSeconds) / bookmark.totalTimeInSeconds * 100.0f);
  else
    return 0;
}

bool CVideoGUIInfo::InitCurrentItem(CFileItem *item)
{
  if (item && item->IsVideo())
  {
    // special case where .strm is used to start an audio stream
    if (item->IsInternetStream() && g_application.GetAppPlayer().IsPlayingAudio())
      return false;

    CLog::Log(LOGDEBUG,"CVideoGUIInfo::InitCurrentItem(%s)", CURL::GetRedacted(item->GetPath()).c_str());

    // Find a thumb for this file.
    if (!item->HasArt("thumb"))
    {
      CVideoThumbLoader loader;
      loader.LoadItem(item);
    }

    // find a thumb for this stream
    if (item->IsInternetStream())
    {
      if (!g_application.m_strPlayListFile.empty())
      {
        CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
        CFileItem thumbItem(g_application.m_strPlayListFile,false);

        CVideoThumbLoader loader;
        if (loader.FillThumb(thumbItem))
          item->SetArt("thumb", thumbItem.GetArt("thumb"));
      }
    }
    return true;
  }
  return false;
}

bool CVideoGUIInfo::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  // For videoplayer "offset" and "position" info labels check playlist
  if (info.GetData1() && ((info.m_info >= VIDEOPLAYER_OFFSET_POSITION_FIRST &&
      info.m_info <= VIDEOPLAYER_OFFSET_POSITION_LAST) ||
      (info.m_info >= PLAYER_OFFSET_POSITION_FIRST && info.m_info <= PLAYER_OFFSET_POSITION_LAST)))
    return GetPlaylistInfo(value, info);

  const CVideoInfoTag* tag = item->GetVideoInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // PLAYER_* / VIDEOPLAYER_* / LISTITEM_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case PLAYER_PATH:
      case PLAYER_FILENAME:
      case PLAYER_FILEPATH:
        if (item->HasMusicInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;

        value = tag->m_strFileNameAndPath;
        if (value.empty())
          value = item->GetPath();
        value = GUIINFO::GetFileInfoLabelValueFromPath(info.m_info, value);
        return true;
      case PLAYER_TITLE:
        value = tag->m_strTitle;
        return !value.empty();
      case VIDEOPLAYER_TITLE:
        value = tag->m_strTitle;
        return !value.empty();
      case LISTITEM_TITLE:
        value = tag->m_strTitle;
        return true;
      case VIDEOPLAYER_ORIGINALTITLE:
      case LISTITEM_ORIGINALTITLE:
        value = tag->m_strOriginalTitle;
        return true;
      case VIDEOPLAYER_GENRE:
      case LISTITEM_GENRE:
        value = StringUtils::Join(tag->m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case VIDEOPLAYER_DIRECTOR:
      case LISTITEM_DIRECTOR:
        value = StringUtils::Join(tag->m_director, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case VIDEOPLAYER_IMDBNUMBER:
      case LISTITEM_IMDBNUMBER:
        value = tag->GetUniqueID();
        return true;
      case VIDEOPLAYER_DBID:
      case LISTITEM_DBID:
        if (tag->m_iDbId > -1)
        {
          value = StringUtils::Format("%i", tag->m_iDbId);
          return true;
        }
        break;
      case VIDEOPLAYER_TVSHOWDBID:
      case LISTITEM_TVSHOWDBID:
        if (tag->m_iIdShow > -1)
        {
          value = StringUtils::Format("%i", tag->m_iIdShow);
          return true;
        }
        break;
      case VIDEOPLAYER_UNIQUEID:
      case LISTITEM_UNIQUEID:
        if (!info.GetData3().empty())
          value = tag->GetUniqueID(info.GetData3());
        return true;
      case VIDEOPLAYER_RATING:
      case LISTITEM_RATING:
      {
        float rating = tag->GetRating(info.GetData3()).rating;
        if (rating > 0.f)
        {
          value = StringUtils::FormatNumber(rating);
          return true;
        }
        break;
      }
      case VIDEOPLAYER_RATING_AND_VOTES:
      case LISTITEM_RATING_AND_VOTES:
      {
        CRating rating = tag->GetRating(info.GetData3());
        if (rating.rating >= 0.f)
        {
          if (rating.rating > 0.f && rating.votes == 0)
            value = StringUtils::FormatNumber(rating.rating);
          else if (rating.votes > 0)
            value = StringUtils::Format(g_localizeStrings.Get(20350).c_str(),
                                        StringUtils::FormatNumber(rating.rating).c_str(),
                                        StringUtils::FormatNumber(rating.votes).c_str());
          else
            break;
          return true;
        }
        break;
      }
      case VIDEOPLAYER_USER_RATING:
      case LISTITEM_USER_RATING:
        if (tag->m_iUserRating > 0)
        {
          value = StringUtils::Format("%i", tag->m_iUserRating);
          return true;
        }
        break;
      case VIDEOPLAYER_VOTES:
      case LISTITEM_VOTES:
        value = StringUtils::FormatNumber(tag->GetRating(info.GetData3()).votes);
        return true;
      case VIDEOPLAYER_YEAR:
      case LISTITEM_YEAR:
        if (tag->HasYear())
        {
          value = StringUtils::Format("%i", tag->GetYear());
          return true;
        }
        break;
      case VIDEOPLAYER_PREMIERED:
      case LISTITEM_PREMIERED:
      {
        CDateTime dateTime;
        if (tag->m_firstAired.IsValid())
          dateTime = tag->m_firstAired;
        else if (tag->HasPremiered())
          dateTime = tag->GetPremiered();

        if (dateTime.IsValid())
        {
          value = dateTime.GetAsLocalizedDate();
          return true;
        }
        break;
      }
      case VIDEOPLAYER_PLOT:
        value = tag->m_strPlot;
        return true;
      case VIDEOPLAYER_TRAILER:
      case LISTITEM_TRAILER:
        value = tag->m_strTrailer;
        return true;
      case VIDEOPLAYER_PLOT_OUTLINE:
      case LISTITEM_PLOT_OUTLINE:
        value = tag->m_strPlotOutline;
        return true;
      case VIDEOPLAYER_EPISODE:
      case LISTITEM_EPISODE:
      {
        int iEpisode = -1;
        if (tag->m_iEpisode > 0)
        {
          iEpisode = tag->m_iEpisode;
        }

        if (iEpisode >= 0)
        {
          value = StringUtils::Format("%d", iEpisode);
          return true;
        }
        break;
      }
      case VIDEOPLAYER_SEASON:
      case LISTITEM_SEASON:
        if (tag->m_iSeason >= 0)
        {
          value = StringUtils::Format("%d", tag->m_iSeason);
          return true;
        }
        break;
      case VIDEOPLAYER_TVSHOW:
      case LISTITEM_TVSHOW:
        value = tag->m_strShowTitle;
        return true;
      case VIDEOPLAYER_STUDIO:
      case LISTITEM_STUDIO:
        value = StringUtils::Join(tag->m_studio, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case VIDEOPLAYER_COUNTRY:
      case LISTITEM_COUNTRY:
        value = StringUtils::Join(tag->m_country, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case VIDEOPLAYER_MPAA:
      case LISTITEM_MPAA:
        value = tag->m_strMPAARating;
        return true;
      case VIDEOPLAYER_TOP250:
      case LISTITEM_TOP250:
        if (tag->m_iTop250 > 0)
        {
          value = StringUtils::Format("%i", tag->m_iTop250);
          return true;
        }
        break;
      case VIDEOPLAYER_CAST:
      case LISTITEM_CAST:
        value = tag->GetCast();
        return true;
      case VIDEOPLAYER_CAST_AND_ROLE:
      case LISTITEM_CAST_AND_ROLE:
        value = tag->GetCast(true);
        return true;
      case VIDEOPLAYER_ARTIST:
      case LISTITEM_ARTIST:
        value = StringUtils::Join(tag->m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case VIDEOPLAYER_ALBUM:
      case LISTITEM_ALBUM:
        value = tag->m_strAlbum;
        return true;
      case VIDEOPLAYER_WRITER:
      case LISTITEM_WRITER:
        value = StringUtils::Join(tag->m_writingCredits, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case VIDEOPLAYER_TAGLINE:
      case LISTITEM_TAGLINE:
        value = tag->m_strTagLine;
        return true;
      case VIDEOPLAYER_LASTPLAYED:
      case LISTITEM_LASTPLAYED:
      {
        const CDateTime dateTime = tag->m_lastPlayed;
        if (dateTime.IsValid())
        {
          value = dateTime.GetAsLocalizedDate();
          return true;
        }
        break;
      }
      case VIDEOPLAYER_PLAYCOUNT:
      case LISTITEM_PLAYCOUNT:
        if (tag->GetPlayCount() > 0)
        {
          value = StringUtils::Format("%i", tag->GetPlayCount());
          return true;
        }
        break;

      /////////////////////////////////////////////////////////////////////////////////////////////
      // LISTITEM_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case LISTITEM_DURATION:
      {
        int iDuration = tag->GetDuration();
        if (iDuration > 0)
        {
          value = StringUtils::SecondsToTimeString(iDuration, static_cast<TIME_FORMAT>(info.GetData4()));
          return true;
        }
        break;
      }
      case LISTITEM_TRACKNUMBER:
        if (tag->m_iTrack > -1 )
        {
          value = StringUtils::Format("%i", tag->m_iTrack);
          return true;
        }
        break;
      case LISTITEM_PLOT:
        {
          std::shared_ptr<CSettingList> setting(std::dynamic_pointer_cast<CSettingList>( 
            CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS)));
          if (tag->m_type != MediaTypeTvShow && tag->m_type != MediaTypeVideoCollection &&
              tag->GetPlayCount() == 0 && setting &&
              ((tag->m_type == MediaTypeMovie &&
                !CSettingUtils::FindIntInList(
                    setting, CSettings::VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_MOVIES)) ||
               (tag->m_type == MediaTypeEpisode &&
                !CSettingUtils::FindIntInList(
                    setting, CSettings::VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_TVSHOWEPISODES))))
          {
            value = g_localizeStrings.Get(20370);
          }
          else
          {
            value = tag->m_strPlot;
          }
          return true;
        }
      case LISTITEM_STATUS:
        value = tag->m_strStatus;
        return true;
      case LISTITEM_TAG:
        value = StringUtils::Join(tag->m_tags, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
        return true;
      case LISTITEM_SET:
        value = tag->m_set.title;
        return true;
      case LISTITEM_SETID:
        if (tag->m_set.id > 0)
        {
          value = StringUtils::Format("%d", tag->m_set.id);
          return true;
        }
        break;
      case LISTITEM_ENDTIME_RESUME:
      {
        const CDateTimeSpan duration(0, 0, 0, tag->GetDuration() - tag->GetResumePoint().timeInSeconds);
        value = (CDateTime::GetCurrentDateTime() + duration).GetAsLocalizedTime("", false);
        return true;
      }
      case LISTITEM_ENDTIME:
      {
        const CDateTimeSpan duration(0, 0, 0, tag->GetDuration());
        value = (CDateTime::GetCurrentDateTime() + duration).GetAsLocalizedTime("", false);
        return true;
      }
      case LISTITEM_DATE_ADDED:
        if (tag->m_dateAdded.IsValid())
        {
          value = tag->m_dateAdded.GetAsLocalizedDate();
          return true;
        }
        break;
      case LISTITEM_DBTYPE:
        value = tag->m_type;
        return true;
      case LISTITEM_APPEARANCES:
        if (tag->m_relevance > -1)
        {
          value = StringUtils::Format("%i", tag->m_relevance);
          return true;
        }
        break;
      case LISTITEM_PERCENT_PLAYED:
        value = StringUtils::Format("%d", GetPercentPlayed(tag));
        return true;
      case LISTITEM_VIDEO_CODEC:
        value = tag->m_streamDetails.GetVideoCodec();
        return true;
      case LISTITEM_VIDEO_RESOLUTION:
        value = CStreamDetails::VideoDimsToResolutionDescription(tag->m_streamDetails.GetVideoWidth(), tag->m_streamDetails.GetVideoHeight());
        return true;
      case LISTITEM_VIDEO_ASPECT:
        value = CStreamDetails::VideoAspectToAspectDescription(tag->m_streamDetails.GetVideoAspect());
        return true;
      case LISTITEM_AUDIO_CODEC:
        value = tag->m_streamDetails.GetAudioCodec();
        return true;
      case LISTITEM_AUDIO_CHANNELS:
      {
        int iChannels = tag->m_streamDetails.GetAudioChannels();
        if (iChannels > 0)
        {
          value = StringUtils::Format("%i", iChannels);
          return true;
        }
        break;
      }
      case LISTITEM_AUDIO_LANGUAGE:
        value = tag->m_streamDetails.GetAudioLanguage();
        return true;
      case LISTITEM_SUBTITLE_LANGUAGE:
        value = tag->m_streamDetails.GetSubtitleLanguage();
        return true;
      case LISTITEM_FILENAME:
      case LISTITEM_FILE_EXTENSION:
        if (item->IsVideoDb())
          value = URIUtils::GetFileName(tag->m_strFileNameAndPath);
        else if (item->HasMusicInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          value = URIUtils::GetFileName(item->GetPath());

        if (info.m_info == LISTITEM_FILE_EXTENSION)
        {
          std::string strExtension = URIUtils::GetExtension(value);
          value = StringUtils::TrimLeft(strExtension, ".");
        }
        return true;
      case LISTITEM_FOLDERNAME:
      case LISTITEM_PATH:
        if (item->IsVideoDb())
        {
          if (item->m_bIsFolder)
            value = tag->m_strPath;
          else
            URIUtils::GetParentPath(tag->m_strFileNameAndPath, value);
        }
        else if (item->HasMusicInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          URIUtils::GetParentPath(item->GetPath(), value);

        value = CURL(value).GetWithoutUserDetails();

        if (info.m_info == LISTITEM_FOLDERNAME)
        {
          URIUtils::RemoveSlashAtEnd(value);
          value = URIUtils::GetFileName(value);
        }
        return true;
      case LISTITEM_FILENAME_AND_PATH:
        if (item->IsVideoDb())
          value = tag->m_strFileNameAndPath;
        else if (item->HasMusicInfoTag()) // special handling for music videos, which have both a videotag and a musictag
          break;
        else
          value = item->GetPath();

        value = CURL(value).GetWithoutUserDetails();
        return true;
    }
  }

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // VIDEOPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case VIDEOPLAYER_PLAYLISTLEN:
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_VIDEO)
      {
        value = GUIINFO::GetPlaylistLabel(PLAYLIST_LENGTH);
        return true;
      }
      break;
    case VIDEOPLAYER_PLAYLISTPOS:
      if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() == PLAYLIST_VIDEO)
      {
        value = GUIINFO::GetPlaylistLabel(PLAYLIST_POSITION);
        return true;
      }
      break;
    case VIDEOPLAYER_VIDEO_ASPECT:
      value = CStreamDetails::VideoAspectToAspectDescription(CServiceBroker::GetDataCacheCore().GetVideoDAR());
      return true;
    case VIDEOPLAYER_STEREOSCOPIC_MODE:
      value = CServiceBroker::GetDataCacheCore().GetVideoStereoMode();
      return true;
    case VIDEOPLAYER_SUBTITLES_LANG:
      value = m_subtitleInfo.language;
      return true;
      break;
    case VIDEOPLAYER_COVER:
      if (g_application.GetAppPlayer().IsPlayingVideo())
      {
        if (fallback)
          *fallback = "DefaultVideoCover.png";

        value = item->HasArt("thumb") ? item->GetArt("thumb") : "DefaultVideoCover.png";
        return true;
      }
      break;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_STEREOSCOPIC_MODE:
      value = item->GetProperty("stereomode").asString();
      if (value.empty() && tag)
        value = CStereoscopicsManager::NormalizeStereoMode(tag->m_streamDetails.GetStereoMode());
      return true;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // VIDEOPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case VIDEOPLAYER_VIDEO_CODEC:
      value = m_videoInfo.codecName;
      return true;
    case VIDEOPLAYER_VIDEO_RESOLUTION:
      value = CStreamDetails::VideoDimsToResolutionDescription(m_videoInfo.width, m_videoInfo.height);
      return true;
    case VIDEOPLAYER_AUDIO_CODEC:
      value = m_audioInfo.codecName;
      return true;
    case VIDEOPLAYER_AUDIO_CHANNELS:
    {
      int iChannels = m_audioInfo.channels;
      if (iChannels > 0)
      {
        value = StringUtils::Format("%i", iChannels);
        return true;
      }
      break;
    }
    case VIDEOPLAYER_AUDIO_BITRATE:
    {
      int iBitrate = m_audioInfo.bitrate;
      if (iBitrate > 0)
      {
        value = StringUtils::Format("%li", std::lrint(static_cast<double>(iBitrate) / 1000.0));
        return true;
      }
      break;
    }
    case VIDEOPLAYER_VIDEO_BITRATE:
    {
      int iBitrate = m_videoInfo.bitrate;
      if (iBitrate > 0)
      {
        value = StringUtils::Format("%li", std::lrint(static_cast<double>(iBitrate) / 1000.0));
        return true;
      }
      break;
    }
    case VIDEOPLAYER_AUDIO_LANG:
      value = m_audioInfo.language;
      return true;
  }

  return false;
}

bool CVideoGUIInfo::GetPlaylistInfo(std::string& value, const CGUIInfo& info) const
{
  PLAYLIST::CPlayList& playlist = CServiceBroker::GetPlaylistPlayer().GetPlaylist(PLAYLIST_VIDEO);
  if (playlist.size() < 1)
    return false;

  int index = info.GetData2();
  if (info.GetData1() == 1)
  { // relative index (requires current playlist is PLAYLIST_VIDEO)
    if (CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist() != PLAYLIST_VIDEO)
      return false;

    index = CServiceBroker::GetPlaylistPlayer().GetNextSong(index);
  }

  if (index < 0 || index >= playlist.size())
    return false;

  const CFileItemPtr playlistItem = playlist[index];
  // try to set a thumbnail
  if (!playlistItem->HasArt("thumb"))
  {
    CVideoThumbLoader loader;
    loader.LoadItem(playlistItem.get());
    // still no thumb? then just the set the default cover
    if (!playlistItem->HasArt("thumb"))
      playlistItem->SetArt("thumb", "DefaultVideoCover.png");
  }
  if (info.m_info == VIDEOPLAYER_PLAYLISTPOS)
  {
    value = StringUtils::Format("%i", index + 1);
    return true;
  }
  else if (info.m_info == VIDEOPLAYER_COVER)
  {
    value = playlistItem->GetArt("thumb");
    return true;
  }

  return GetLabel(value, playlistItem.get(), 0, CGUIInfo(info.m_info), nullptr);
}

bool CVideoGUIInfo::GetFallbackLabel(std::string& value,
                                     const CFileItem* item,
                                     int contextWindow,
                                     const CGUIInfo& info,
                                     std::string* fallback)
{
  // No fallback for videoplayer "offset" and "position" info labels
  if (info.GetData1() && ((info.m_info >= VIDEOPLAYER_OFFSET_POSITION_FIRST &&
      info.m_info <= VIDEOPLAYER_OFFSET_POSITION_LAST) ||
      (info.m_info >= PLAYER_OFFSET_POSITION_FIRST && info.m_info <= PLAYER_OFFSET_POSITION_LAST)))
    return false;

  const CVideoInfoTag* tag = item->GetVideoInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // VIDEOPLAYER_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case VIDEOPLAYER_TITLE:
        value = item->GetLabel();
        if (value.empty())
          value = CUtil::GetTitleFromPath(item->GetPath());
        return true;
      default:
        break;
    }
  }
  return false;
}

bool CVideoGUIInfo::GetInt(int& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  if (!gitem->IsFileItem())
    return false;

  const CFileItem *item = static_cast<const CFileItem*>(gitem);
  const CVideoInfoTag* tag = item->GetVideoInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // LISTITEM_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case LISTITEM_PERCENT_PLAYED:
        value = GetPercentPlayed(tag);
        return true;
    }
  }

  return false;
}

bool CVideoGUIInfo::GetBool(bool& value, const CGUIListItem *gitem, int contextWindow, const CGUIInfo &info) const
{
  if (!gitem->IsFileItem())
    return false;

  const CFileItem *item = static_cast<const CFileItem*>(gitem);
  const CVideoInfoTag* tag = item->GetVideoInfoTag();
  if (tag)
  {
    switch (info.m_info)
    {
      /////////////////////////////////////////////////////////////////////////////////////////////
      // VIDEOPLAYER_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case VIDEOPLAYER_HAS_INFO:
        value = !tag->IsEmpty();
        return true;

      /////////////////////////////////////////////////////////////////////////////////////////////
      // LISTITEM_*
      /////////////////////////////////////////////////////////////////////////////////////////////
      case LISTITEM_IS_RESUMABLE:
        value = tag->GetResumePoint().timeInSeconds > 0;
        return true;
      case LISTITEM_IS_COLLECTION:
        value = tag->m_type == MediaTypeVideoCollection;
        return true;
    }
  }

  switch (info.m_info)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // VIDEOPLAYER_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case VIDEOPLAYER_CONTENT:
    {
      std::string strContent = "files";
      if (tag)
      {
        if (tag->m_type == MediaTypeMovie)
          strContent = "movies";
        else if (tag->m_type == MediaTypeEpisode)
          strContent = "episodes";
        else if (tag->m_type == MediaTypeMusicVideo)
          strContent = "musicvideos";
      }
      value = StringUtils::EqualsNoCase(info.GetData3(), strContent);
      return value; // if no match for this provider, other providers shall be asked.
    }
    case VIDEOPLAYER_USING_OVERLAYS:
      value = (CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_VIDEOPLAYER_RENDERMETHOD) == RENDER_OVERLAYS);
      return true;
    case VIDEOPLAYER_ISFULLSCREEN:
      value = CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO ||
              CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_FULLSCREEN_GAME;
      return true;
    case VIDEOPLAYER_HASMENU:
      value = g_application.GetAppPlayer().HasMenu();
      return true;
    case VIDEOPLAYER_HASTELETEXT:
      if (g_application.GetAppPlayer().GetTeletextCache())
      {
        value = true;
        return true;
      }
      break;
    case VIDEOPLAYER_HASSUBTITLES:
      value = g_application.GetAppPlayer().GetSubtitleCount() > 0;
      return true;
    case VIDEOPLAYER_SUBTITLESENABLED:
      value = g_application.GetAppPlayer().GetSubtitleVisible();
      return true;
    case VIDEOPLAYER_IS_STEREOSCOPIC:
      value = !CServiceBroker::GetDataCacheCore().GetVideoStereoMode().empty();
      return true;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // LISTITEM_*
    ///////////////////////////////////////////////////////////////////////////////////////////////
    case LISTITEM_IS_STEREOSCOPIC:
    {
      std::string stereoMode = item->GetProperty("stereomode").asString();
      if (stereoMode.empty() && tag)
        stereoMode = CStereoscopicsManager::NormalizeStereoMode(tag->m_streamDetails.GetStereoMode());
      if (!stereoMode.empty() && stereoMode != "mono")
        value = true;
      return true;
    }
  }

  return false;
}
