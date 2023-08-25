/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVideoInfo.h"

#include "ContextMenuManager.h"
#include "FileItem.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "Util.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogYesNo.h"
#include "filesystem/Directory.h"
#include "filesystem/VideoDatabaseDirectory.h"
#include "filesystem/VideoDatabaseDirectory/QueryParams.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIImage.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindow.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/Key.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/MusicDatabase.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "playlists/PlayListTypes.h"
#include "profiles/ProfileManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSourceSettings.h"
#include "settings/SettingUtils.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "storage/MediaManager.h"
#include "utils/FileExtensionProvider.h"
#include "utils/FileUtils.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "video/VideoDbUrl.h"
#include "video/VideoInfoScanner.h"
#include "video/VideoInfoTag.h"
#include "video/VideoLibraryQueue.h"
#include "video/VideoThumbLoader.h"
#include "video/tags/VideoTagLoaderFFmpeg.h"
#include "video/windows/GUIWindowVideoNav.h"

#include <iterator>
#include <string>

using namespace XFILE::VIDEODATABASEDIRECTORY;
using namespace XFILE;
using namespace KODI::MESSAGING;

#define CONTROL_IMAGE                3
#define CONTROL_TEXTAREA             4
#define CONTROL_BTN_TRACKS           5
#define CONTROL_BTN_REFRESH          6
#define CONTROL_BTN_USERRATING       7
#define CONTROL_BTN_PLAY             8
#define CONTROL_BTN_RESUME           9
#define CONTROL_BTN_GET_THUMB       10
#define CONTROL_BTN_PLAY_TRAILER    11
#define CONTROL_BTN_GET_FANART      12
#define CONTROL_BTN_DIRECTOR        13

#define CONTROL_LIST                50

// predicate used by sorting and set_difference
bool compFileItemsByDbId(const CFileItemPtr& lhs, const CFileItemPtr& rhs)
{
  return lhs->HasVideoInfoTag() && rhs->HasVideoInfoTag() && lhs->GetVideoInfoTag()->m_iDbId < rhs->GetVideoInfoTag()->m_iDbId;
}

CGUIDialogVideoInfo::CGUIDialogVideoInfo(void)
  : CGUIDialog(WINDOW_DIALOG_VIDEO_INFO, "DialogVideoInfo.xml"),
  m_movieItem(new CFileItem),
  m_castList(new CFileItemList)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogVideoInfo::~CGUIDialogVideoInfo(void)
{
  delete m_castList;
}

bool CGUIDialogVideoInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearCastList();

      if (m_startUserrating != m_movieItem->GetVideoInfoTag()->m_iUserRating)
      {
        CVideoDatabase db;
        if (db.Open())
        {
          m_hasUpdatedUserrating = true;
          db.SetVideoUserRating(m_movieItem->GetVideoInfoTag()->m_iDbId, m_movieItem->GetVideoInfoTag()->m_iUserRating, m_movieItem->GetVideoInfoTag()->m_type);
          db.Close();
        }
      }
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        if (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeTvShow)
        {
          bool bCanceled=false;
          if (CGUIDialogYesNo::ShowAndGetInput(CVariant{20377}, CVariant{20378}, bCanceled, CVariant{ "" }, CVariant{ "" }, CGUIDialogYesNo::NO_TIMEOUT))
          {
            m_bRefreshAll = true;
            CVideoDatabase db;
            if (db.Open())
            {
              db.SetPathHash(m_movieItem->GetVideoInfoTag()->m_strPath,"");
              db.Close();
            }
          }
          else
            m_bRefreshAll = false;

          if (bCanceled)
            return false;
        }
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      else if (iControl == CONTROL_BTN_PLAY)
      {
        Play();
      }
      else if (iControl == CONTROL_BTN_USERRATING)
      {
        OnSetUserrating();
      }
      else if (iControl == CONTROL_BTN_RESUME)
      {
        Play(true);
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetArt();
      }
      else if (iControl == CONTROL_BTN_PLAY_TRAILER)
      {
        PlayTrailer();
      }
      else if (iControl == CONTROL_BTN_GET_FANART)
      {
        OnGetFanart();
      }
      else if (iControl == CONTROL_BTN_DIRECTOR)
      {
        auto directors = m_movieItem->GetVideoInfoTag()->m_director;
        if (directors.size() == 0)
          return true;
        if (directors.size() == 1)
          OnSearch(directors[0]);
        else
        {
          auto pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
          if (pDlgSelect)
          {
            pDlgSelect->Reset();
            pDlgSelect->SetHeading(CVariant{22080});
            for (const auto &director: directors)
              pDlgSelect->Add(director);
            pDlgSelect->Open();

            int iItem = pDlgSelect->GetSelectedItem();
            if (iItem < 0)
              return true;
            OnSearch(directors[iItem]);
          }
        }
      }
      else if (iControl == CONTROL_LIST)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          OnMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= m_castList->Size())
            break;
          std::string strItem = m_castList->Get(iItem)->GetLabel();
          if (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeVideoCollection)
          {
            SetMovie(m_castList->Get(iItem).get());
            Close();
            Open();
          }
          else
            OnSearch(strItem);
        }
      }
    }
    break;
  case GUI_MSG_NOTIFY_ALL:
    {
      if (IsActive() && message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
      {
        CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
        if (item && m_movieItem->IsPath(item->GetPath()))
        { // Just copy over the stream details and the thumb if we don't already have one
          if (!m_movieItem->HasArt("thumb"))
            m_movieItem->SetArt("thumb", item->GetArt("thumb"));
          m_movieItem->GetVideoInfoTag()->m_streamDetails = item->GetVideoInfoTag()->m_streamDetails;
        }
        return true;
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogVideoInfo::OnInitWindow()
{
  m_bRefresh = false;
  m_bRefreshAll = true;
  m_hasUpdatedThumb = false;
  m_hasUpdatedUserrating = false;
  m_bViewReview = true;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  const std::string uniqueId = m_movieItem->GetProperty("xxuniqueid").asString();
  if (uniqueId.empty() || !StringUtils::StartsWithNoCase(uniqueId.c_str(), "xx"))
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_REFRESH,
        (profileManager->GetCurrentProfile().canWriteDatabases() ||
        g_passwordManager.bMasterUser));
  else
    CONTROL_DISABLE(CONTROL_BTN_REFRESH);
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB,
      (profileManager->GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) &&
      !StringUtils::StartsWithNoCase(m_movieItem->GetVideoInfoTag()->
      GetUniqueID().c_str(), "plugin"));
  // Disable video user rating button for plugins and sets as they don't have tables to save this
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_USERRATING, !m_movieItem->IsPlugin() && m_movieItem->GetVideoInfoTag()->m_type != MediaTypeVideoCollection);

  VideoDbContentType type = m_movieItem->GetVideoContentType();
  if (type == VideoDbContentType::TVSHOWS || type == VideoDbContentType::MOVIES)
    CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_FANART, (profileManager->
        GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser) &&
        !StringUtils::StartsWithNoCase(m_movieItem->GetVideoInfoTag()->
        GetUniqueID().c_str(), "plugin"));
  else
    CONTROL_DISABLE(CONTROL_BTN_GET_FANART);

  Update();

  CGUIDialog::OnInitWindow();
}

bool CGUIDialogVideoInfo::OnAction(const CAction &action)
{
  int userrating = m_movieItem->GetVideoInfoTag()->m_iUserRating;
  if (action.GetID() == ACTION_INCREASE_RATING)
  {
    SetUserrating(userrating + 1);
    return true;
  }
  else if (action.GetID() == ACTION_DECREASE_RATING)
  {
    SetUserrating(userrating - 1);
    return true;
  }
  else if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogVideoInfo::SetUserrating(int userrating) const
{
  userrating = std::max(userrating, 0);
  userrating = std::min(userrating, 10);
  if (userrating != m_movieItem->GetVideoInfoTag()->m_iUserRating)
  {
    m_movieItem->GetVideoInfoTag()->SetUserrating(userrating);

    // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_movieItem);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
}

void CGUIDialogVideoInfo::SetMovie(const CFileItem *item)
{
  *m_movieItem = *item;

  // setup cast list
  ClearCastList();

  // When the scraper throws an error, the video tag can be null here
  if (!item->HasVideoInfoTag())
    return;

  MediaType type = item->GetVideoInfoTag()->m_type;

  m_startUserrating = m_movieItem->GetVideoInfoTag()->m_iUserRating;

  if (type == MediaTypeMusicVideo)
  { // music video
    CMusicDatabase database;
    database.Open();
    const std::vector<std::string> &artists = m_movieItem->GetVideoInfoTag()->m_artist;
    for (std::vector<std::string>::const_iterator it = artists.begin(); it != artists.end(); ++it)
    {
      int idArtist = database.GetArtistByName(*it);
      std::string thumb = database.GetArtForItem(idArtist, MediaTypeArtist, "thumb");
      CFileItemPtr item(new CFileItem(*it));
      if (!thumb.empty())
        item->SetArt("thumb", thumb);
      item->SetArt("icon", "DefaultArtist.png");
      item->SetLabel2(g_localizeStrings.Get(29904));
      m_castList->Add(item);
    }
    // get performers in the music video (added as actors)
    for (CVideoInfoTag::iCast it = m_movieItem->GetVideoInfoTag()->m_cast.begin();
         it != m_movieItem->GetVideoInfoTag()->m_cast.end(); ++it)
    {
      // Check to see if we have already added this performer as the artist and skip adding if so
      auto haveArtist = std::find(std::begin(artists), std::end(artists), it->strName);
      if (haveArtist == artists.end()) // artist or performer not already in the list
      {
        CFileItemPtr item(new CFileItem(it->strName));
        if (!it->thumb.empty())
          item->SetArt("thumb", it->thumb);
        else if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
                     CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS))
        { // backward compatibility
          std::string thumb = CScraperUrl::GetThumbUrl(it->thumbUrl.GetFirstUrlByType());
          if (!thumb.empty())
          {
            item->SetArt("thumb", thumb);
            CServiceBroker::GetTextureCache()->BackgroundCacheImage(thumb);
          }
        }
        item->SetArt("icon", "DefaultActor.png");
        item->SetLabel(it->strName);
        item->SetLabel2(it->strRole);
        m_castList->Add(item);
      }
    }
  }
  else if (type == MediaTypeVideoCollection)
  {
    CVideoDatabase database;
    database.Open();
    database.GetMoviesNav(m_movieItem->GetPath(), *m_castList, -1, -1, -1, -1, -1, -1,
                          m_movieItem->GetVideoInfoTag()->m_set.id, -1,
                          SortDescription(), VideoDbDetailsAll);
    m_castList->Sort(SortBySortTitle, SortOrderDescending);
    CVideoThumbLoader loader;
    for (auto& item : *m_castList)
      loader.LoadItem(item.get());
  }
  else
  { // movie/show/episode
    for (CVideoInfoTag::iCast it = m_movieItem->GetVideoInfoTag()->m_cast.begin(); it != m_movieItem->GetVideoInfoTag()->m_cast.end(); ++it)
    {
      CFileItemPtr item(new CFileItem(it->strName));
      if (!it->thumb.empty())
        item->SetArt("thumb", it->thumb);
      else
      {
        const std::shared_ptr<CSettings> settings =
            CServiceBroker::GetSettingsComponent()->GetSettings();
        if (settings->GetInt(CSettings::SETTING_VIDEOLIBRARY_ARTWORK_LEVEL) !=
                CSettings::VIDEOLIBRARY_ARTWORK_LEVEL_NONE &&
            settings->GetBool(CSettings::SETTING_VIDEOLIBRARY_ACTORTHUMBS))
        { // backward compatibility
          std::string thumb = CScraperUrl::GetThumbUrl(it->thumbUrl.GetFirstUrlByType());
          if (!thumb.empty())
          {
            item->SetArt("thumb", thumb);
            CServiceBroker::GetTextureCache()->BackgroundCacheImage(thumb);
          }
        }
      }
      item->SetArt("icon", "DefaultActor.png");
      item->SetLabel(it->strName);
      item->SetLabel2(it->strRole);
      m_castList->Add(item);
    }
  }

  if (type == MediaTypeMovie)
  {
    // local trailers should always override non-local, so check
    // for a local one if the registered trailer is online
    if (m_movieItem->GetVideoInfoTag()->m_strTrailer.empty() ||
        URIUtils::IsInternetStream(m_movieItem->GetVideoInfoTag()->m_strTrailer))
    {
      std::string localTrailer = m_movieItem->FindTrailer();
      if (!localTrailer.empty())
      {
        m_movieItem->GetVideoInfoTag()->m_strTrailer = localTrailer;
        CVideoDatabase database;
        if (database.Open())
        {
          database.SetSingleValue(VideoDbContentType::MOVIES, VIDEODB_ID_TRAILER,
                                  m_movieItem->GetVideoInfoTag()->m_iDbId,
                                  m_movieItem->GetVideoInfoTag()->m_strTrailer);
          database.Close();
          CUtil::DeleteVideoDatabaseDirectoryCache();
        }
      }
    }
  }

  m_castList->SetContent(CMediaTypes::ToPlural(type));

  CVideoThumbLoader loader;
  loader.LoadItem(m_movieItem.get());
}

void CGUIDialogVideoInfo::Update()
{
  // setup plot text area
  std::shared_ptr<CSettingList> setting(std::dynamic_pointer_cast<CSettingList>( 
    CServiceBroker::GetSettingsComponent()->GetSettings()->GetSetting(CSettings::SETTING_VIDEOLIBRARY_SHOWUNWATCHEDPLOTS)));
  std::string strTmp = m_movieItem->GetVideoInfoTag()->m_strPlot;
  if (m_movieItem->GetVideoInfoTag()->m_type != MediaTypeTvShow)
    if (m_movieItem->GetVideoInfoTag()->GetPlayCount() == 0 && setting &&
        ((m_movieItem->GetVideoInfoTag()->m_type == MediaTypeMovie &&
          !CSettingUtils::FindIntInList(setting,
                                        CSettings::VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_MOVIES)) ||
         (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeEpisode &&
          !CSettingUtils::FindIntInList(
              setting, CSettings::VIDEOLIBRARY_PLOTS_SHOW_UNWATCHED_TVSHOWEPISODES))))
      strTmp = g_localizeStrings.Get(20370);

  StringUtils::Trim(strTmp);
  SetLabel(CONTROL_TEXTAREA, strTmp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_castList);
  OnMessage(msg);

  if (GetControl(CONTROL_BTN_TRACKS)) // if no CONTROL_BTN_TRACKS found - allow skinner full visibility control over CONTROL_TEXTAREA and CONTROL_LIST
  {
    if (m_bViewReview)
    {
      if (!m_movieItem->GetVideoInfoTag()->m_artist.empty())
      {
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 133);
      }
      else if (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeVideoCollection)
      {
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 20342);
      }
      else
      {
        SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 206);
      }

      SET_CONTROL_HIDDEN(CONTROL_LIST);
      SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
    }
    else
    {
      SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 207);

      SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
      SET_CONTROL_VISIBLE(CONTROL_LIST);
    }
  }

  // Check for resumability
  if (m_movieItem->GetVideoInfoTag()->GetResumePoint().timeInSeconds > 0.0)
    CONTROL_ENABLE(CONTROL_BTN_RESUME);
  else
    CONTROL_DISABLE(CONTROL_BTN_RESUME);

  CONTROL_ENABLE(CONTROL_BTN_PLAY);

  // update the thumbnail
  CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = static_cast<CGUIImage*>(pControl);
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_movieItem->GetArt("thumb"));
  }
  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  if (m_hasUpdatedThumb)
  {
    CGUIMessage reload(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(reload);
  }
}

bool CGUIDialogVideoInfo::NeedRefresh() const
{
  return m_bRefresh;
}

bool CGUIDialogVideoInfo::RefreshAll() const
{
  return m_bRefreshAll;
}

void CGUIDialogVideoInfo::OnSearch(std::string& strSearch)
{
  CGUIDialogProgress *progress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(WINDOW_DIALOG_PROGRESS);
  if (progress)
  {
    progress->SetHeading(CVariant{194});
    progress->SetLine(0, CVariant{strSearch});
    progress->SetLine(1, CVariant{""});
    progress->SetLine(2, CVariant{""});
    progress->Open();
    progress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (progress)
    progress->Close();

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (pDlgSelect)
    {
      pDlgSelect->Reset();
      pDlgSelect->SetHeading(CVariant{283});

      CVideoThumbLoader loader;
      for (int i = 0; i < items.Size(); i++)
      {
        if (items[i]->HasVideoInfoTag() &&
          items[i]->GetVideoInfoTag()->GetPlayCount() > 0)
          items[i]->SetLabel2(g_localizeStrings.Get(16102));

        loader.LoadItem(items[i].get());
        pDlgSelect->Add(*items[i]);
      }

      pDlgSelect->SetUseDetails(true);
      pDlgSelect->Open();

      int iItem = pDlgSelect->GetSelectedItem();
      if (iItem < 0)
        return;

      OnSearchItemFound(items[iItem].get());
    }
  }
  else
  {
    HELPERS::ShowOKDialogText(CVariant{194}, CVariant{284});
  }
}

void CGUIDialogVideoInfo::DoSearch(std::string& strSearch, CFileItemList& items) const
{
  CVideoDatabase db;
  if (!db.Open())
    return;

  CFileItemList movies;
  db.GetMoviesByActor(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    std::string label = movies[i]->GetVideoInfoTag()->m_strTitle;
    if (movies[i]->GetVideoInfoTag()->HasYear())
      label += StringUtils::Format(" ({})", movies[i]->GetVideoInfoTag()->GetYear());
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20338) + "] ", items);

  db.GetTvShowsByActor(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    std::string label = movies[i]->GetVideoInfoTag()->m_strShowTitle;
    if (movies[i]->GetVideoInfoTag()->HasYear())
      label += StringUtils::Format(" ({})", movies[i]->GetVideoInfoTag()->GetYear());
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20364) + "] ", items);

  db.GetEpisodesByActor(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    std::string label = movies[i]->GetVideoInfoTag()->m_strTitle + " (" +  movies[i]->GetVideoInfoTag()->m_strShowTitle + ")";
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20359) + "] ", items);

  db.GetMusicVideosByArtist(strSearch, movies);
  for (int i = 0; i < movies.Size(); ++i)
  {
    std::string label = StringUtils::Join(movies[i]->GetVideoInfoTag()->m_artist, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator) + " - " + movies[i]->GetVideoInfoTag()->m_strTitle;
    if (movies[i]->GetVideoInfoTag()->HasYear())
      label += StringUtils::Format(" ({})", movies[i]->GetVideoInfoTag()->GetYear());
    movies[i]->SetLabel(label);
  }
  CGUIWindowVideoBase::AppendAndClearSearchItems(movies, "[" + g_localizeStrings.Get(20391) + "] ", items);
  db.Close();

  // Search for music albums by artist with name matching search string
  CMusicDatabase music_database;
  if (!music_database.Open())
    return;

  if (music_database.SearchAlbumsByArtistName(strSearch, movies))
  {
    for (int i = 0; i < movies.Size(); ++i)
    {
      // Set type so that video thumbloader handles album art
      movies[i]->GetVideoInfoTag()->m_type = MediaTypeAlbum;
    }
    CGUIWindowVideoBase::AppendAndClearSearchItems(
        movies, "[" + g_localizeStrings.Get(36918) + "] ", items);
  }
  music_database.Close();
}

void CGUIDialogVideoInfo::OnSearchItemFound(const CFileItem* pItem)
{
  VideoDbContentType type = pItem->GetVideoContentType();

  CVideoDatabase db;
  if (!db.Open())
    return;

  CVideoInfoTag movieDetails;
  if (type == VideoDbContentType::MOVIES)
    db.GetMovieInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VideoDbContentType::EPISODES)
    db.GetEpisodeInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VideoDbContentType::TVSHOWS)
    db.GetTvShowInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  if (type == VideoDbContentType::MUSICVIDEOS)
    db.GetMusicVideoInfo(pItem->GetPath(), movieDetails, pItem->GetVideoInfoTag()->m_iDbId);
  db.Close();
  if (type == VideoDbContentType::MUSICALBUMS)
  {
    Close();
    CGUIDialogMusicInfo::ShowFor(const_cast<CFileItem*>(pItem));
    return; // No video info to refresh so just close the window and go back to the fileitem list
  }

  CFileItem item(*pItem);
  *item.GetVideoInfoTag() = movieDetails;
  SetMovie(&item);
  // refresh our window entirely
  Close();
  Open();
}

void CGUIDialogVideoInfo::ClearCastList()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
  OnMessage(msg);
  m_castList->Clear();
}

void CGUIDialogVideoInfo::Play(bool resume)
{
  if (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeTvShow)
  {
    std::string strPath;
    if (m_movieItem->IsPlugin())
    {
      strPath = m_movieItem->GetPath();
      Close();
      if (CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindow() == WINDOW_VIDEO_NAV)
      {
        CGUIMessage message(GUI_MSG_NOTIFY_ALL, CServiceBroker::GetGUI()->
                            GetWindowManager().GetActiveWindow(), 0, GUI_MSG_UPDATE, 0);
        message.SetStringParam(strPath);
        CServiceBroker::GetGUI()->GetWindowManager().SendMessage(message);
      }
      else
        CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV,strPath);
    }
    else
    {
      strPath = StringUtils::Format("videodb://tvshows/titles/{}/",
                                    m_movieItem->GetVideoInfoTag()->m_iDbId);
      Close();
      CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV,strPath);
    }
    return;
  }

  if (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeVideoCollection)
  {
    std::string strPath = StringUtils::Format("videodb://movies/sets/{}/?setid={}",
                                              m_movieItem->GetVideoInfoTag()->m_iDbId,
                                              m_movieItem->GetVideoInfoTag()->m_iDbId);
    Close();
    CServiceBroker::GetGUI()->GetWindowManager().ActivateWindow(WINDOW_VIDEO_NAV, strPath);
    return;
  }

  CGUIWindowVideoNav* pWindow = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowVideoNav>(WINDOW_VIDEO_NAV);
  if (pWindow)
  {
    // close our dialog
    Close(true);
    if (resume)
      m_movieItem->SetStartOffset(STARTOFFSET_RESUME);
    else if (!CGUIWindowVideoBase::ShowResumeMenu(*m_movieItem))
    {
      // The Resume dialog was closed without any choice
      Open();
      return;
    }
    m_movieItem->SetProperty("playlist_type_hint", PLAYLIST::TYPE_VIDEO);

    pWindow->PlayMovie(m_movieItem.get());
  }
}

namespace
{
// Add art types required in Kodi and configured by the user
void AddHardCodedAndExtendedArtTypes(std::vector<std::string>& artTypes, const CVideoInfoTag& tag)
{
  for (const auto& artType : CVideoThumbLoader::GetArtTypes(tag.m_type))
  {
    if (find(artTypes.cbegin(), artTypes.cend(), artType) == artTypes.cend())
      artTypes.emplace_back(artType);
  }
}

// Add art types currently assigned to the media item
void AddCurrentArtTypes(std::vector<std::string>& artTypes, const CVideoInfoTag& tag,
  CVideoDatabase& db)
{
  std::map<std::string, std::string> currentArt;
  db.GetArtForItem(tag.m_iDbId, tag.m_type, currentArt);
  for (const auto& art : currentArt)
  {
    if (!art.second.empty() && find(artTypes.cbegin(), artTypes.cend(), art.first) == artTypes.cend())
      artTypes.push_back(art.first);
  }
}

// Add art types that exist for other media items of the same type
void AddMediaTypeArtTypes(std::vector<std::string>& artTypes, const CVideoInfoTag& tag,
  CVideoDatabase& db)
{
  std::vector<std::string> dbArtTypes;
  db.GetArtTypes(tag.m_type, dbArtTypes);
  for (const auto& artType : dbArtTypes)
  {
    if (find(artTypes.cbegin(), artTypes.cend(), artType) == artTypes.cend())
      artTypes.push_back(artType);
  }
}

// Add art types from available but unassigned artwork for this media item
void AddAvailableArtTypes(std::vector<std::string>& artTypes, const CVideoInfoTag& tag,
  CVideoDatabase& db)
{
  for (const auto& artType : db.GetAvailableArtTypesForItem(tag.m_iDbId, tag.m_type))
  {
    if (find(artTypes.cbegin(), artTypes.cend(), artType) == artTypes.cend())
      artTypes.push_back(artType);
  }
}

std::vector<std::string> GetArtTypesList(const CVideoInfoTag& tag)
{
  CVideoDatabase db;
  db.Open();

  std::vector<std::string> artTypes;

  AddHardCodedAndExtendedArtTypes(artTypes, tag);
  AddCurrentArtTypes(artTypes, tag, db);
  AddMediaTypeArtTypes(artTypes, tag, db);
  AddAvailableArtTypes(artTypes, tag, db);

  db.Close();
  return artTypes;
}
}

std::string CGUIDialogVideoInfo::ChooseArtType(const CFileItem &videoItem)
{
  // prompt for choice
  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (!dialog || !videoItem.HasVideoInfoTag())
    return "";

  CFileItemList items;
  dialog->SetHeading(CVariant{13511});
  dialog->Reset();
  dialog->SetUseDetails(true);
  dialog->EnableButton(true, 13516);

  std::vector<std::string> artTypes = GetArtTypesList(*videoItem.GetVideoInfoTag());

  for (std::vector<std::string>::const_iterator i = artTypes.begin(); i != artTypes.end(); ++i)
  {
    const std::string& type = *i;
    CFileItemPtr item(new CFileItem(type, false));
    if (type == "banner")
      item->SetLabel(g_localizeStrings.Get(20020));
    else if (type == "fanart")
      item->SetLabel(g_localizeStrings.Get(20445));
    else if (type == "poster")
      item->SetLabel(g_localizeStrings.Get(20021));
    else if (type == "thumb")
      item->SetLabel(g_localizeStrings.Get(21371));
    else
      item->SetLabel(type);
    item->SetProperty("type", type);
    if (videoItem.HasArt(type))
      item->SetArt("thumb", videoItem.GetArt(type));
    items.Add(item);
  }

  dialog->SetItems(items);
  dialog->Open();

  if (dialog->IsButtonPressed())
  {
    // Get the new artwork name
    std::string strArtworkName;
    if (!CGUIKeyboardFactory::ShowAndGetInput(strArtworkName, CVariant{g_localizeStrings.Get(13516)}, false))
      return "";

    return strArtworkName;
  }

  return dialog->GetSelectedFileItem()->GetProperty("type").asString();
}

void CGUIDialogVideoInfo::OnGetArt()
{
  if (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeVideoCollection)
  {
    ManageVideoItemArtwork(m_movieItem, m_movieItem->GetVideoInfoTag()->m_type);
    return;
  }
  std::string type = ChooseArtType(*m_movieItem);
  if (type.empty())
    return; // cancelled

  //! @todo this can be removed once these are unified.
  if (type == "fanart")
    OnGetFanart();
  else
  {
    CFileItemList items;

    // Current thumb
    if (m_movieItem->HasArt(type))
    {
      CFileItemPtr item(new CFileItem("thumb://Current", false));
      item->SetArt("thumb", m_movieItem->GetArt(type));
      item->SetArt("icon", "DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13512));
      items.Add(item);
    }
    else if ((type == "poster" || type == "banner") && m_movieItem->HasArt("thumb"))
    { // add the 'thumb' type in
      CFileItemPtr item(new CFileItem("thumb://Thumb", false));
      item->SetArt("thumb", m_movieItem->GetArt("thumb"));
      item->SetArt("icon", "DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13512));
      items.Add(item);
    }

    std::string embeddedArt;
    if (URIUtils::HasExtension(m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath, ".mkv"))
    {
      CFileItem item(m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath, false);
      CVideoTagLoaderFFmpeg loader(item, nullptr, false);
      CVideoInfoTag tag;
      loader.Load(tag, false, nullptr);
      for (const auto& it : tag.m_coverArt)
      {
        if (it.m_type == type)
        {
          CFileItemPtr itemF(new CFileItem("thumb://Embedded", false));
          embeddedArt = CTextureUtils::GetWrappedImageURL(item.GetPath(), "video_" + type);
          itemF->SetArt("thumb", embeddedArt);
          itemF->SetLabel(g_localizeStrings.Get(13519));
          items.Add(itemF);
        }
      }
    }

    // Grab the thumbnails from the web
    m_movieItem->GetVideoInfoTag()->m_strPictureURL.Parse();
    std::vector<std::string> thumbs;
    int season = (m_movieItem->GetVideoInfoTag()->m_type == MediaTypeSeason) ? m_movieItem->GetVideoInfoTag()->m_iSeason : -1;
    m_movieItem->GetVideoInfoTag()->m_strPictureURL.GetThumbUrls(thumbs, type, season);

    for (unsigned int i = 0; i < thumbs.size(); ++i)
    {
      std::string strItemPath = StringUtils::Format("thumb://Remote{}", i);
      CFileItemPtr item(new CFileItem(strItemPath, false));
      item->SetArt("thumb", thumbs[i]);
      item->SetArt("icon", "DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13513));

      //! @todo Do we need to clear the cached image?
      //    CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
      items.Add(item);
    }

    std::string localThumb = CVideoThumbLoader::GetLocalArt(*m_movieItem, type);
    if (!localThumb.empty())
    {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetArt("thumb", localThumb);
      item->SetArt("icon", "DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13514));
      items.Add(item);
    }
    else
    { // no local thumb exists, so we are just using the IMDb thumb or cached thumb
      // which is probably the IMDb thumb.  These could be wrong, so allow the user
      // to delete the incorrect thumb
      CFileItemPtr item(new CFileItem("thumb://None", false));
      item->SetArt("icon", "DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13515));
      items.Add(item);
    }

    std::string result;
    VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("video"));
    AddItemPathToFileBrowserSources(sources, *m_movieItem);
    CServiceBroker::GetMediaManager().GetLocalDrives(sources);
    if (CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(13511), result) &&
        result != "thumb://Current") // user didn't choose the one they have
    {
      std::string newThumb;
      if (StringUtils::StartsWith(result, "thumb://Remote"))
      {
        int number = atoi(result.substr(14).c_str());
        newThumb = thumbs[number];
      }
      else if (result == "thumb://Thumb")
        newThumb = m_movieItem->GetArt("thumb");
      else if (result == "thumb://Local")
        newThumb = localThumb;
      else if (result == "thumb://Embedded")
        newThumb = embeddedArt;
      else if (CFileUtils::Exists(result))
        newThumb = result;
      else // none
        newThumb.clear();

      // update thumb in the database
      CVideoDatabase db;
      if (db.Open())
      {
        db.SetArtForItem(m_movieItem->GetVideoInfoTag()->m_iDbId, m_movieItem->GetVideoInfoTag()->m_type, type, newThumb);
        db.Close();
      }
      CUtil::DeleteVideoDatabaseDirectoryCache(); // to get them new thumbs to show
      m_movieItem->SetArt(type, newThumb);
      if (m_movieItem->HasProperty("set_folder_thumb"))
      { // have a folder thumb to set as well
        VIDEO::CVideoInfoScanner::ApplyThumbToFolder(m_movieItem->GetProperty("set_folder_thumb").asString(), newThumb);
      }
      m_hasUpdatedThumb = true;
    }
  }

  // Update our screen
  Update();

  // re-open the art selection dialog as we come back from
  // the image selection dialog
  OnGetArt();
}

// Allow user to select a Fanart
void CGUIDialogVideoInfo::OnGetFanart()
{
  CFileItemList items;

  // Ensure the fanart is unpacked
  m_movieItem->GetVideoInfoTag()->m_fanart.Unpack();

  if (m_movieItem->HasArt("fanart"))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetArt("thumb", m_movieItem->GetArt("fanart"));
    itemCurrent->SetArt("icon", "DefaultPicture.png");
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  std::string embeddedArt;
  if (URIUtils::HasExtension(m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath, ".mkv"))
  {
    CFileItem item(m_movieItem->GetVideoInfoTag()->m_strFileNameAndPath, false);
    CVideoTagLoaderFFmpeg loader(item, nullptr, false);
    CVideoInfoTag tag;
    loader.Load(tag, false, nullptr);
    for (const auto& it : tag.m_coverArt)
    {
      if (it.m_type == "fanart")
      {
        CFileItemPtr itemF(new CFileItem("fanart://Embedded", false));
        embeddedArt = CTextureUtils::GetWrappedImageURL(item.GetPath(), "video_fanart");
        itemF->SetArt("thumb", embeddedArt);
        itemF->SetLabel(g_localizeStrings.Get(13520));
        items.Add(itemF);
      }
    }
  }

  // Grab the thumbnails from the web
  for (unsigned int i = 0; i < m_movieItem->GetVideoInfoTag()->m_fanart.GetNumFanarts(); i++)
  {
    if (URIUtils::IsProtocol(m_movieItem->GetVideoInfoTag()->m_fanart.GetPreviewURL(i), "image"))
      continue;
    std::string strItemPath = StringUtils::Format("fanart://Remote{}", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    std::string thumb = m_movieItem->GetVideoInfoTag()->m_fanart.GetPreviewURL(i);
    item->SetArt("thumb", CTextureUtils::GetWrappedThumbURL(thumb));
    item->SetArt("icon", "DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20441));

    //! @todo Do we need to clear the cached image?
    //    CServiceBroker::GetTextureCache()->ClearCachedImage(thumb);
    items.Add(item);
  }

  CFileItem item(*m_movieItem->GetVideoInfoTag());
  std::string strLocal = item.GetLocalFanart();
  if (!strLocal.empty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetArt("thumb", strLocal);
    itemLocal->SetArt("icon", "DefaultPicture.png");
    itemLocal->SetLabel(g_localizeStrings.Get(20438));

    //! @todo Do we need to clear the cached image?
    CServiceBroker::GetTextureCache()->ClearCachedImage(strLocal);
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetArt("icon", "DefaultPicture.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  std::string result;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("video"));
  AddItemPathToFileBrowserSources(sources, item);
  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445) ||
    StringUtils::EqualsNoCase(result, "fanart://Current"))
    return;   // user cancelled

  if (StringUtils::EqualsNoCase(result, "fanart://Local"))
    result = strLocal;

  if (StringUtils::EqualsNoCase(result, "fanart://Embedded"))
  {
    unsigned int current = m_movieItem->GetVideoInfoTag()->m_fanart.GetNumFanarts();
    int found = -1;
    for (size_t i = 0; i < current; ++i)
      if (URIUtils::IsProtocol(m_movieItem->GetVideoInfoTag()->m_fanart.GetImageURL(), "image"))
        found = i;
    if (found != -1)
    {
      m_movieItem->GetVideoInfoTag()->m_fanart.AddFanart(embeddedArt, "", "");
      found = current;
    }

    m_movieItem->GetVideoInfoTag()->m_fanart.SetPrimaryFanart(found);

    CVideoDatabase db;
    if (db.Open())
    {
      db.UpdateFanart(*m_movieItem, m_movieItem->GetVideoContentType());
      db.Close();
    }
    result = embeddedArt;
  }

  if (StringUtils::StartsWith(result, "fanart://Remote"))
  {
    int iFanart = atoi(result.substr(15).c_str());
    // set new primary fanart, and update our database accordingly
    m_movieItem->GetVideoInfoTag()->m_fanart.SetPrimaryFanart(iFanart);
    CVideoDatabase db;
    if (db.Open())
    {
      db.UpdateFanart(*m_movieItem, m_movieItem->GetVideoContentType());
      db.Close();
    }
    result = m_movieItem->GetVideoInfoTag()->m_fanart.GetImageURL();
  }
  else if (StringUtils::EqualsNoCase(result, "fanart://None") || !CFileUtils::Exists(result))
    result.clear();

  // set the fanart image
  if (flip && !result.empty())
    result = CTextureUtils::GetWrappedImageURL(result, "", "flipped");
  CVideoDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_movieItem->GetVideoInfoTag()->m_iDbId, m_movieItem->GetVideoInfoTag()->m_type, "fanart", result);
    db.Close();
  }

  CUtil::DeleteVideoDatabaseDirectoryCache(); // to get them new thumbs to show
  m_movieItem->SetArt("fanart", result);
  m_hasUpdatedThumb = true;

  // Update our screen
  Update();
}

void CGUIDialogVideoInfo::OnSetUserrating() const
{
  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (dialog)
  {
    dialog->SetHeading(CVariant{ 38023 });
    dialog->Add(g_localizeStrings.Get(38022));
    for (int i = 1; i <= 10; i++)
      dialog->Add(StringUtils::Format("{}: {}", g_localizeStrings.Get(563), i));

    dialog->SetSelected(m_movieItem->GetVideoInfoTag()->m_iUserRating);

    dialog->Open();

    int iItem = dialog->GetSelectedItem();
    if (iItem < 0)
      return;

    SetUserrating(iItem);
  }
}

void CGUIDialogVideoInfo::PlayTrailer()
{
  Close(true);
  CGUIMessage msg(GUI_MSG_PLAY_TRAILER, 0, 0, 0, 0, m_movieItem);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
}

void CGUIDialogVideoInfo::SetLabel(int iControl, const std::string &strLabel)
{
  if (strLabel.empty())
  {
    SET_CONTROL_LABEL(iControl, 416);  // "Not available"
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

std::string CGUIDialogVideoInfo::GetThumbnail() const
{
  return m_movieItem->GetArt("thumb");
}

namespace
{
std::string GetItemPathForBrowserSource(const CFileItem& item)
{
  if (!item.HasVideoInfoTag())
    return "";

  std::string itemDir = item.GetVideoInfoTag()->m_basePath;
  //season
  if (itemDir.empty())
    itemDir = item.GetVideoInfoTag()->GetPath();

  CFileItem itemTmp(itemDir, false);
  if (itemTmp.IsVideo())
    itemDir = URIUtils::GetParentPath(itemDir);

  return itemDir;
}

void AddItemPathStringToFileBrowserSources(VECSOURCES& sources,
    const std::string& itemDir, const std::string& label)
{
  if (!itemDir.empty() && CDirectory::Exists(itemDir))
  {
    CMediaSource itemSource;
    itemSource.strName = label;
    itemSource.strPath = itemDir;
    sources.push_back(itemSource);
  }
}
} // namespace

void CGUIDialogVideoInfo::AddItemPathToFileBrowserSources(VECSOURCES& sources,
    const CFileItem& item)
{
  std::string itemDir = GetItemPathForBrowserSource(item);
  AddItemPathStringToFileBrowserSources(sources, itemDir, g_localizeStrings.Get(36041));
}

int CGUIDialogVideoInfo::ManageVideoItem(const std::shared_ptr<CFileItem>& item)
{
  if (item == nullptr || !item->IsVideoDb() || !item->HasVideoInfoTag() || item->GetVideoInfoTag()->m_iDbId < 0)
    return -1;

  CVideoDatabase database;
  if (!database.Open())
    return -1;

  const std::string &type = item->GetVideoInfoTag()->m_type;
  int dbId = item->GetVideoInfoTag()->m_iDbId;

  CContextButtons buttons;
  if (type == MediaTypeMovie || type == MediaTypeVideoCollection ||
      type == MediaTypeTvShow || type == MediaTypeEpisode ||
     (type == MediaTypeSeason && item->GetVideoInfoTag()->m_iSeason > 0) ||  // seasons without "all seasons" and "specials"
      type == MediaTypeMusicVideo)
    buttons.Add(CONTEXT_BUTTON_EDIT, 16105);

  if (type == MediaTypeMovie || type == MediaTypeTvShow)
    buttons.Add(CONTEXT_BUTTON_EDIT_SORTTITLE, 16107);

  if (type == MediaTypeMovie)
  {
    // only show link/unlink if there are tvshows available
    if (database.HasContent(VideoDbContentType::TVSHOWS))
    {
      buttons.Add(CONTEXT_BUTTON_LINK_MOVIE, 20384);
      if (database.IsLinkedToTvshow(dbId))
        buttons.Add(CONTEXT_BUTTON_UNLINK_MOVIE, 20385);
    }

    // set or change movie set the movie belongs to
    buttons.Add(CONTEXT_BUTTON_SET_MOVIESET, 20465);
  }

  if (type == MediaTypeEpisode &&
      item->GetVideoInfoTag()->m_iBookmarkId > 0)
    buttons.Add(CONTEXT_BUTTON_UNLINK_BOOKMARK, 20405);

  // movie sets
  if (item->m_bIsFolder && type == MediaTypeVideoCollection)
  {
    buttons.Add(CONTEXT_BUTTON_SET_MOVIESET_ART, 13511);
    buttons.Add(CONTEXT_BUTTON_MOVIESET_ADD_REMOVE_ITEMS, 20465);
  }

  // seasons
  if (item->m_bIsFolder && type == MediaTypeSeason)
    buttons.Add(CONTEXT_BUTTON_SET_SEASON_ART, 13511);

  // tags
  if (item->m_bIsFolder && type == "tag")
  {
    CVideoDbUrl videoUrl;
    if (videoUrl.FromString(item->GetPath()))
    {
      const std::string &mediaType = videoUrl.GetItemType();

      buttons.Add(
          CONTEXT_BUTTON_TAGS_ADD_ITEMS,
          StringUtils::Format(g_localizeStrings.Get(20460), GetLocalizedVideoType(mediaType)));
      buttons.Add(CONTEXT_BUTTON_TAGS_REMOVE_ITEMS, StringUtils::Format(g_localizeStrings.Get(20461).c_str(), GetLocalizedVideoType(mediaType).c_str()));
    }
  }

  if (type != MediaTypeSeason)
    buttons.Add(CONTEXT_BUTTON_DELETE, 646);

  //temporary workaround until the context menu ids are removed
  const int addonItemOffset = 10000;

  auto addonItems = CServiceBroker::GetContextMenuManager().GetAddonItems(*item, CContextMenuManager::MANAGE);
  for (size_t i = 0; i < addonItems.size(); ++i)
    buttons.Add(addonItemOffset + i, addonItems[i]->GetLabel(*item));

  bool result = false;
  int button = CGUIDialogContextMenu::ShowAndGetChoice(buttons);
  if (button >= 0)
  {
    switch (static_cast<CONTEXT_BUTTON>(button))
    {
      case CONTEXT_BUTTON_EDIT:
        result = UpdateVideoItemTitle(item);
        break;

      case CONTEXT_BUTTON_EDIT_SORTTITLE:
        result = UpdateVideoItemSortTitle(item);
        break;

      case CONTEXT_BUTTON_LINK_MOVIE:
        result = LinkMovieToTvShow(item, false, database);
        break;

      case CONTEXT_BUTTON_UNLINK_MOVIE:
        result = LinkMovieToTvShow(item, true, database);
        break;

      case CONTEXT_BUTTON_SET_MOVIESET:
      {
        CFileItemPtr selectedSet;
        if (GetSetForMovie(item.get(), selectedSet))
          result = SetMovieSet(item.get(), selectedSet.get());
        break;
      }

      case CONTEXT_BUTTON_UNLINK_BOOKMARK:
        database.DeleteBookMarkForEpisode(*item->GetVideoInfoTag());
        result = true;
        break;

      case CONTEXT_BUTTON_DELETE:
        result = DeleteVideoItem(item);
        break;

      case CONTEXT_BUTTON_SET_MOVIESET_ART:
      case CONTEXT_BUTTON_SET_SEASON_ART:
        result = ManageVideoItemArtwork(item, type);
        break;

      case CONTEXT_BUTTON_MOVIESET_ADD_REMOVE_ITEMS:
        result = ManageMovieSets(item);
        break;

      case CONTEXT_BUTTON_TAGS_ADD_ITEMS:
        result = AddItemsToTag(item);
        break;

      case CONTEXT_BUTTON_TAGS_REMOVE_ITEMS:
        result = RemoveItemsFromTag(item);
        break;

      default:
        if (button >= addonItemOffset)
          result = CONTEXTMENU::LoopFrom(*addonItems[button - addonItemOffset], item);
        break;
    }
  }

  database.Close();

  if (result)
    return button;

  return -1;
}

//Add change a title's name
bool CGUIDialogVideoInfo::UpdateVideoItemTitle(const std::shared_ptr<CFileItem>& pItem)
{
  if (pItem == nullptr || !pItem->HasVideoInfoTag())
    return false;

  // dont allow update while scanning
  if (CVideoLibraryQueue::GetInstance().IsScanningLibrary())
  {
    HELPERS::ShowOKDialogText(CVariant{257}, CVariant{14057});
    return false;
  }

  CVideoDatabase database;
  if (!database.Open())
    return false;

  int iDbId = pItem->GetVideoInfoTag()->m_iDbId;
  MediaType mediaType = pItem->GetVideoInfoTag()->m_type;

  CVideoInfoTag detail;
  std::string title;
  if (mediaType == MediaTypeMovie)
  {
    database.GetMovieInfo("", detail, iDbId, VideoDbDetailsNone);
    title = detail.m_strTitle;
  }
  else if (mediaType == MediaTypeVideoCollection)
  {
    database.GetSetInfo(iDbId, detail);
    title = detail.m_strTitle;
  }
  else if (mediaType == MediaTypeEpisode)
  {
    database.GetEpisodeInfo(pItem->GetPath(), detail, iDbId, VideoDbDetailsNone);
    title = detail.m_strTitle;
  }
  else if (mediaType == MediaTypeSeason)
  {
    database.GetSeasonInfo(iDbId, detail);
    title = detail.m_strSortTitle.empty() ? detail.m_strTitle : detail.m_strSortTitle;
  }
  else if (mediaType == MediaTypeTvShow)
  {
    database.GetTvShowInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath, detail, iDbId, 0, VideoDbDetailsNone);
    title = detail.m_strTitle;
  }
  else if (mediaType == MediaTypeMusicVideo)
  {
    database.GetMusicVideoInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath, detail, iDbId, VideoDbDetailsNone);
    title = detail.m_strTitle;
  }

  // get the new title
  if (!CGUIKeyboardFactory::ShowAndGetInput(title, CVariant{ g_localizeStrings.Get(16105) }, false))
    return false;

  if (mediaType == MediaTypeSeason)
  {
    detail.m_strSortTitle = title;
    std::map<std::string, std::string> artwork;
    database.SetDetailsForSeason(detail, artwork, detail.m_iIdShow, detail.m_iDbId);
  }
  else
  {
    detail.m_strTitle = title;
    VideoDbContentType iType = pItem->GetVideoContentType();
    database.UpdateMovieTitle(iDbId, detail.m_strTitle, iType);
  }

  return true;
}

bool CGUIDialogVideoInfo::CanDeleteVideoItem(const std::shared_ptr<CFileItem>& item)
{
  if (item == nullptr || !item->HasVideoInfoTag())
    return false;

  if (item->GetVideoInfoTag()->m_type == "tag")
    return true;

  CQueryParams params;
  CVideoDatabaseDirectory::GetQueryParams(item->GetPath(), params);

  return params.GetMovieId()   != -1 ||
         params.GetEpisodeId() != -1 ||
         params.GetMVideoId()  != -1 ||
         params.GetSetId()     != -1 ||
         (params.GetTvShowId() != -1 && params.GetSeason() <= -1 &&
          !CVideoDatabaseDirectory::IsAllItem(item->GetPath()));
}

bool CGUIDialogVideoInfo::DeleteVideoItemFromDatabase(const std::shared_ptr<CFileItem>& item,
                                                      bool unavailable /* = false */)
{
  if (item == nullptr || !item->HasVideoInfoTag() ||
      !CanDeleteVideoItem(item))
    return false;

  // dont allow update while scanning
  if (CVideoLibraryQueue::GetInstance().IsScanningLibrary())
  {
    HELPERS::ShowOKDialogText(CVariant{257}, CVariant{14057});
    return false;
  }

  CGUIDialogYesNo* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogYesNo>(WINDOW_DIALOG_YES_NO);
  if (pDialog == nullptr)
    return false;

  int heading = -1;
  VideoDbContentType type = item->GetVideoContentType();
  const std::string& subtype = item->GetVideoInfoTag()->m_type;
  if (subtype != "tag")
  {
    switch (type)
    {
      case VideoDbContentType::MOVIES:
        heading = 432;
        break;
      case VideoDbContentType::EPISODES:
        heading = 20362;
        break;
      case VideoDbContentType::TVSHOWS:
        heading = 20363;
        break;
      case VideoDbContentType::MUSICVIDEOS:
        heading = 20392;
        break;
      case VideoDbContentType::MOVIE_SETS:
        heading = 646;
        break;
      default:
        return false;
    }
  }
  else
  {
    heading = 10058;
  }

  pDialog->SetHeading(CVariant{heading});

  if (unavailable)
  {
    pDialog->SetLine(0, CVariant{g_localizeStrings.Get(662)});
    pDialog->SetLine(1, CVariant{g_localizeStrings.Get(663)});
  }
  else
  {
    pDialog->SetLine(0,
                     CVariant{StringUtils::Format(g_localizeStrings.Get(433), item->GetLabel())});
    pDialog->SetLine(1, CVariant{""});
  }
  pDialog->SetLine(2, CVariant{""});
  pDialog->Open();

  if (!pDialog->IsConfirmed())
    return false;

  CVideoDatabase database;
  database.Open();

  if (item->GetVideoInfoTag()->m_iDbId < 0)
    return false;

  if (subtype == "tag")
  {
    database.DeleteTag(item->GetVideoInfoTag()->m_iDbId, type);
    return true;
  }

  switch (type)
  {
    case VideoDbContentType::MOVIES:
      database.DeleteMovie(item->GetVideoInfoTag()->m_iDbId);
      break;
    case VideoDbContentType::EPISODES:
      database.DeleteEpisode(item->GetVideoInfoTag()->m_iDbId);
      break;
    case VideoDbContentType::TVSHOWS:
      database.DeleteTvShow(item->GetVideoInfoTag()->m_iDbId);
      break;
    case VideoDbContentType::MUSICVIDEOS:
      database.DeleteMusicVideo(item->GetVideoInfoTag()->m_iDbId);
      break;
    case VideoDbContentType::MOVIE_SETS:
      database.DeleteSet(item->GetVideoInfoTag()->m_iDbId);
      break;
    default:
      return false;
  }
  return true;
}

bool CGUIDialogVideoInfo::DeleteVideoItem(const std::shared_ptr<CFileItem>& item,
                                          bool unavailable /* = false */)
{
  if (item == nullptr)
    return false;

  // delete the video item from the database
  if (!DeleteVideoItemFromDatabase(item, unavailable))
    return false;

  const std::shared_ptr<CProfileManager> profileManager = CServiceBroker::GetSettingsComponent()->GetProfileManager();

  // check if the user is allowed to delete the actual file as well
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) &&
      (profileManager->GetCurrentProfile().getLockMode() == LOCK_MODE_EVERYONE ||
       !profileManager->GetCurrentProfile().filesLocked() ||
       g_passwordManager.IsMasterLockUnlocked(true)))
  {
    std::string strDeletePath = item->GetVideoInfoTag()->GetPath();

    if (StringUtils::EqualsNoCase(URIUtils::GetFileName(strDeletePath), "VIDEO_TS.IFO"))
    {
      strDeletePath = URIUtils::GetDirectory(strDeletePath);
      if (StringUtils::EndsWithNoCase(strDeletePath, "video_ts/"))
      {
        URIUtils::RemoveSlashAtEnd(strDeletePath);
        strDeletePath = URIUtils::GetDirectory(strDeletePath);
      }
    }
    if (URIUtils::HasSlashAtEnd(strDeletePath))
      item->m_bIsFolder = true;

    // check if the file/directory can be deleted
    if (CUtil::SupportsWriteFileOperations(strDeletePath))
    {
      item->SetPath(strDeletePath);

      // HACK: stacked files need to be treated as folders in order to be deleted
      if (item->IsStack())
        item->m_bIsFolder = true;

      CGUIComponent *gui = CServiceBroker::GetGUI();
      if (gui && gui->ConfirmDelete(item->GetPath()))
        CFileUtils::DeleteItem(item);
    }
  }

  CUtil::DeleteVideoDatabaseDirectoryCache();

  return true;
}

bool CGUIDialogVideoInfo::ManageMovieSets(const std::shared_ptr<CFileItem>& item)
{
  if (item == nullptr)
    return false;

  CFileItemList originalItems;
  CFileItemList selectedItems;

  if (!GetMoviesForSet(item.get(), originalItems, selectedItems) ||
      selectedItems.Size() == 0) // need at least one item selected
    return false;

  VECFILEITEMS original = originalItems.GetList();
  std::sort(original.begin(), original.end(), compFileItemsByDbId);
  VECFILEITEMS selected = selectedItems.GetList();
  std::sort(selected.begin(), selected.end(), compFileItemsByDbId);

  bool refreshNeeded = false;
  // update the "added" items
  VECFILEITEMS addedItems;
  set_difference(selected.begin(),selected.end(), original.begin(),original.end(), std::back_inserter(addedItems), compFileItemsByDbId);
  for (VECFILEITEMS::const_iterator it = addedItems.begin();  it != addedItems.end(); ++it)
  {
    if (SetMovieSet(it->get(), item.get()))
      refreshNeeded = true;
  }

  // update the "deleted" items
  CFileItemPtr clearItem(new CFileItem());
  clearItem->GetVideoInfoTag()->m_iDbId = -1; // -1 will be used to clear set
  VECFILEITEMS deletedItems;
  set_difference(original.begin(),original.end(), selected.begin(),selected.end(), std::back_inserter(deletedItems), compFileItemsByDbId);
  for (VECFILEITEMS::iterator it = deletedItems.begin();  it != deletedItems.end(); ++it)
  {
    if (SetMovieSet(it->get(), clearItem.get()))
      refreshNeeded = true;
  }

  return refreshNeeded;
}

bool CGUIDialogVideoInfo::GetMoviesForSet(const CFileItem *setItem, CFileItemList &originalMovies, CFileItemList &selectedMovies)
{
  if (setItem == nullptr || !setItem->HasVideoInfoTag())
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  std::string baseDir =
      StringUtils::Format("videodb://movies/sets/{}", setItem->GetVideoInfoTag()->m_iDbId);

  if (!CDirectory::GetDirectory(baseDir, originalMovies, "", DIR_FLAG_DEFAULTS) ||
      originalMovies.Size() <= 0) // keep a copy of the original members of the set
    return false;

  CFileItemList listItems;
  if (!videodb.GetSortedVideos(MediaTypeMovie, "videodb://movies", SortDescription(), listItems) || listItems.Size() <= 0)
    return false;

  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (dialog == nullptr)
    return false;

  listItems.Sort(SortByLabel, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  dialog->Reset();
  dialog->SetMultiSelection(true);
  dialog->SetHeading(CVariant{g_localizeStrings.Get(20457)});
  dialog->SetItems(listItems);
  std::vector<int> selectedIndices;
  for (int i = 0; i < originalMovies.Size(); i++)
  {
    for (int listIndex = 0; listIndex < listItems.Size(); listIndex++)
    {
      if (listItems.Get(listIndex)->GetVideoInfoTag()->m_iDbId == originalMovies[i]->GetVideoInfoTag()->m_iDbId)
      {
        selectedIndices.push_back(listIndex);
        break;
      }
    }
  }
  dialog->SetSelected(selectedIndices);
  dialog->EnableButton(true, 186);
  dialog->Open();

  if (dialog->IsConfirmed())
  {
    for (int i : dialog->GetSelectedItems())
      selectedMovies.Add(listItems.Get(i));
    return (selectedMovies.Size() > 0);
  }
  else
    return false;
}

bool CGUIDialogVideoInfo::GetSetForMovie(const CFileItem* movieItem,
                                         std::shared_ptr<CFileItem>& selectedSet)
{
  if (movieItem == nullptr || !movieItem->HasVideoInfoTag())
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  CFileItemList listItems;

  // " ignoreSingleMovieSets=false " as an option in the url is needed here
  // to override the gui-setting "Include sets containing a single movie"
  // and retrieve all moviesets

  std::string baseDir = "videodb://movies/sets/?ignoreSingleMovieSets=false";

  if (!CDirectory::GetDirectory(baseDir, listItems, "", DIR_FLAG_DEFAULTS))
    return false;
  listItems.Sort(SortByLabel, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  int currentSetId = 0;
  std::string currentSetLabel;

  if (movieItem->GetVideoInfoTag()->m_set.id > currentSetId)
  {
    currentSetId = movieItem->GetVideoInfoTag()->m_set.id;
    currentSetLabel = videodb.GetSetById(currentSetId);
  }

  if (currentSetId > 0)
  {
    // remove duplicate entry
    for (int listIndex = 0; listIndex < listItems.Size(); listIndex++)
    {
      if (listItems.Get(listIndex)->GetVideoInfoTag()->m_iDbId == currentSetId)
      {
        listItems.Remove(listIndex);
        break;
      }
    }
    // add clear item
    std::string strClear = StringUtils::Format(g_localizeStrings.Get(20467), currentSetLabel);
    CFileItemPtr clearItem(new CFileItem(strClear));
    clearItem->GetVideoInfoTag()->m_iDbId = -1; // -1 will be used to clear set
    listItems.AddFront(clearItem, 0);
    // add keep current set item
    std::string strKeep = StringUtils::Format(g_localizeStrings.Get(20469), currentSetLabel);
    CFileItemPtr keepItem(new CFileItem(strKeep));
    keepItem->GetVideoInfoTag()->m_iDbId = currentSetId;
    listItems.AddFront(keepItem, 1);
  }

  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (dialog == nullptr)
    return false;

  dialog->Reset();
  dialog->SetHeading(CVariant{g_localizeStrings.Get(20466)});
  dialog->SetItems(listItems);
  if (currentSetId >= 0)
  {
    for (int listIndex = 0; listIndex < listItems.Size(); listIndex++)
    {
      if (listItems.Get(listIndex)->GetVideoInfoTag()->m_iDbId == currentSetId)
      {
        dialog->SetSelected(listIndex);
        break;
      }
    }
  }
  dialog->EnableButton(true, 20468); // new set via button
  dialog->Open();

  if (dialog->IsButtonPressed())
  { // creating new set
    std::string newSetTitle;
    if (!CGUIKeyboardFactory::ShowAndGetInput(newSetTitle, CVariant{g_localizeStrings.Get(20468)}, false))
      return false;
    int idSet = videodb.AddSet(newSetTitle);
    std::map<std::string, std::string> movieArt, setArt;
    if (!videodb.GetArtForItem(idSet, MediaTypeVideoCollection, setArt))
    {
      videodb.GetArtForItem(movieItem->GetVideoInfoTag()->m_iDbId, MediaTypeMovie, movieArt);
      videodb.SetArtForItem(idSet, MediaTypeVideoCollection, movieArt);
    }
    CFileItemPtr newSet(new CFileItem(newSetTitle));
    newSet->GetVideoInfoTag()->m_iDbId = idSet;
    selectedSet = newSet;
    return true;
  }
  else if (dialog->IsConfirmed())
  {
    selectedSet = dialog->GetSelectedFileItem();
    return (selectedSet != nullptr);
  }
  else
    return false;
}

bool CGUIDialogVideoInfo::SetMovieSet(const CFileItem *movieItem, const CFileItem *selectedSet)
{
  if (movieItem == nullptr || !movieItem->HasVideoInfoTag() ||
      selectedSet == nullptr || !selectedSet->HasVideoInfoTag())
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  videodb.SetMovieSet(movieItem->GetVideoInfoTag()->m_iDbId, selectedSet->GetVideoInfoTag()->m_iDbId);
  return true;
}

bool CGUIDialogVideoInfo::GetItemsForTag(const std::string &strHeading, const std::string &type, CFileItemList &items, int idTag /* = -1 */, bool showAll /* = true */)
{
  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  MediaType mediaType = MediaTypeNone;
  std::string baseDir = "videodb://";
  std::string idColumn;
  if (type.compare(MediaTypeMovie) == 0)
  {
    mediaType = MediaTypeMovie;
    baseDir += "movies";
    idColumn = "idMovie";
  }
  else if (type.compare(MediaTypeTvShow) == 0)
  {
    mediaType = MediaTypeTvShow;
    baseDir += "tvshows";
    idColumn = "idShow";
  }
  else if (type.compare(MediaTypeMusicVideo) == 0)
  {
    mediaType = MediaTypeMusicVideo;
    baseDir += "musicvideos";
    idColumn = "idMVideo";
  }

  baseDir += "/titles/";
  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(baseDir))
    return false;

  CVideoDatabase::Filter filter;
  if (idTag > 0)
  {
    if (!showAll)
      videoUrl.AddOption("tagid", idTag);
    else
      filter.where = videodb.PrepareSQL("%s_view.%s NOT IN (SELECT tag_link.media_id FROM tag_link WHERE tag_link.tag_id = %d AND tag_link.media_type = '%s')", type.c_str(), idColumn.c_str(), idTag, type.c_str());
  }

  CFileItemList listItems;
  if (!videodb.GetSortedVideos(mediaType, videoUrl.ToString(), SortDescription(), listItems, filter) || listItems.Size() <= 0)
    return false;

  CGUIDialogSelect *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
  if (dialog == nullptr)
    return false;

  listItems.Sort(SortByLabel, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);

  dialog->Reset();
  dialog->SetMultiSelection(true);
  dialog->SetHeading(CVariant{strHeading});
  dialog->SetItems(listItems);
  dialog->EnableButton(true, 186);
  dialog->Open();

  for (int i : dialog->GetSelectedItems())
    items.Add(listItems.Get(i));
  return items.Size() > 0;
}

bool CGUIDialogVideoInfo::AddItemsToTag(const std::shared_ptr<CFileItem>& tagItem)
{
  if (tagItem == nullptr || !tagItem->HasVideoInfoTag())
    return false;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(tagItem->GetPath()))
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
    return true;

  std::string mediaType = videoUrl.GetItemType();
  mediaType = mediaType.substr(0, mediaType.length() - 1);

  CFileItemList items;
  std::string localizedType = GetLocalizedVideoType(mediaType);
  std::string strLabel = StringUtils::Format(g_localizeStrings.Get(20464), localizedType);
  if (!GetItemsForTag(strLabel, mediaType, items, tagItem->GetVideoInfoTag()->m_iDbId))
    return true;

  for (int index = 0; index < items.Size(); index++)
  {
    if (!items[index]->HasVideoInfoTag() || items[index]->GetVideoInfoTag()->m_iDbId <= 0)
      continue;

    videodb.AddTagToItem(items[index]->GetVideoInfoTag()->m_iDbId, tagItem->GetVideoInfoTag()->m_iDbId, mediaType);
  }

  return true;
}

bool CGUIDialogVideoInfo::RemoveItemsFromTag(const std::shared_ptr<CFileItem>& tagItem)
{
  if (tagItem == nullptr || !tagItem->HasVideoInfoTag())
    return false;

  CVideoDbUrl videoUrl;
  if (!videoUrl.FromString(tagItem->GetPath()))
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
    return true;

  std::string mediaType = videoUrl.GetItemType();
  mediaType = mediaType.substr(0, mediaType.length() - 1);

  CFileItemList items;
  std::string localizedType = GetLocalizedVideoType(mediaType);
  std::string strLabel = StringUtils::Format(g_localizeStrings.Get(20464), localizedType);
  if (!GetItemsForTag(strLabel, mediaType, items, tagItem->GetVideoInfoTag()->m_iDbId, false))
    return true;

  for (int index = 0; index < items.Size(); index++)
  {
    if (!items[index]->HasVideoInfoTag() || items[index]->GetVideoInfoTag()->m_iDbId <= 0)
      continue;

    videodb.RemoveTagFromItem(items[index]->GetVideoInfoTag()->m_iDbId, tagItem->GetVideoInfoTag()->m_iDbId, mediaType);
  }

  return true;
}

namespace
{
std::string FindLocalMovieSetArtworkFile(const CFileItemPtr& item, const std::string& artType)
{
  std::string infoFolder = VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(item->GetLabel());
  if (infoFolder.empty())
    return "";

  CFileItemList availableArtFiles;
  CDirectory::GetDirectory(infoFolder, availableArtFiles,
      CServiceBroker::GetFileExtensionProvider().GetPictureExtensions(),
      DIR_FLAG_NO_FILE_DIRS | DIR_FLAG_READ_CACHE | DIR_FLAG_NO_FILE_INFO);
  for (const auto& artFile : availableArtFiles)
  {
    std::string candidate = URIUtils::GetFileName(artFile->GetPath());
    URIUtils::RemoveExtension(candidate);
    if (StringUtils::EqualsNoCase(candidate, artType))
      return artFile->GetPath();
  }
  return "";
}
} // namespace

bool CGUIDialogVideoInfo::ManageVideoItemArtwork(const std::shared_ptr<CFileItem>& item,
                                                 const MediaType& type)
{
  if (item == nullptr || !item->HasVideoInfoTag() || type.empty())
    return false;

  CVideoDatabase videodb;
  if (!videodb.Open())
    return true;

  // Grab the thumbnails from the web
  CFileItemList items;
  CFileItemPtr noneitem(new CFileItem("thumb://None", false));
  std::string currentThumb;
  int idArtist = -1;
  std::string artistPath;
  std::string artistOldPath;
  std::string artType = "thumb";
  if (type == MediaTypeArtist)
  {
    CMusicDatabase musicdb;
    if (musicdb.Open())
    {
      idArtist = musicdb.GetArtistByName(item->GetLabel()); // Fails when name not unique
      if (idArtist >= 0 )
      {
        // Get artist paths - possible locations for thumb - while music db open
        musicdb.GetOldArtistPath(idArtist, artistOldPath);  // Old artist path, local to music files
        CArtist artist;
        musicdb.GetArtist(idArtist, artist); // Need name and mbid for artist folder name
        musicdb.GetArtistPath(artist, artistPath);  // Artist path in artist info folder

        currentThumb = musicdb.GetArtForItem(idArtist, MediaTypeArtist, "thumb");
        if (currentThumb.empty())
          currentThumb = videodb.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artType);
      }
    }
  }
  else if (type == "actor")
    currentThumb = videodb.GetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artType);
  else
  { // SEASON, SET
    artType = ChooseArtType(*item);
    if (artType.empty())
      return false;

    if (artType == "fanart" && type != MediaTypeVideoCollection)
      return OnGetFanart(item);

    if (item->HasArt(artType))
      currentThumb = item->GetArt(artType);
    else if ((artType == "poster" || artType == "banner") && item->HasArt("thumb"))
      currentThumb = item->GetArt("thumb");
  }

  if (!currentThumb.empty())
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetArt("thumb", currentThumb);
    item->SetLabel(g_localizeStrings.Get(13512));
    items.Add(item);
  }
  noneitem->SetArt("icon", "DefaultFolder.png");
  noneitem->SetLabel(g_localizeStrings.Get(13515));

  bool local = false;
  std::vector<std::string> thumbs;
  if (type != MediaTypeArtist)
  {
    CVideoInfoTag tag;
    if (type == MediaTypeSeason)
    {
      videodb.GetTvShowInfo("", tag, item->GetVideoInfoTag()->m_iIdShow);
      tag.m_strPictureURL.Parse();
      tag.m_strPictureURL.GetThumbUrls(thumbs, artType, item->GetVideoInfoTag()->m_iSeason);
    }
    else if (type == MediaTypeVideoCollection)
    {
      CFileItemList items;
      std::string baseDir =
          StringUtils::Format("videodb://movies/sets/{}", item->GetVideoInfoTag()->m_iDbId);
      if (videodb.GetMoviesNav(baseDir, items))
      {
        for (int i=0; i < items.Size(); i++)
        {
          CVideoInfoTag* pTag = items[i]->GetVideoInfoTag();
          pTag->m_strPictureURL.Parse();
          pTag->m_strPictureURL.GetThumbUrls(thumbs, "set." + artType, -1, true);
        }
      }
    }
    else
    {
      tag = *item->GetVideoInfoTag();
      tag.m_strPictureURL.Parse();
      tag.m_strPictureURL.GetThumbUrls(thumbs, artType);
    }

    for (size_t i = 0; i < thumbs.size(); i++)
    {
      CFileItemPtr item(new CFileItem(StringUtils::Format("thumb://Remote{0}", i), false));
      item->SetArt("thumb", thumbs[i]);
      item->SetArt("icon", "DefaultPicture.png");
      item->SetLabel(g_localizeStrings.Get(13513));
      items.Add(item);

      //! @todo Do we need to clear the cached image?
      //    CServiceBroker::GetTextureCache()->ClearCachedImage(thumbs[i]);
    }

    if (type == "actor")
    {
      std::string picturePath;
      std::string strThumb = URIUtils::AddFileToFolder(picturePath, "folder.jpg");
      if (CFileUtils::Exists(strThumb))
      {
        CFileItemPtr pItem(new CFileItem(strThumb,false));
        pItem->SetLabel(g_localizeStrings.Get(13514));
        pItem->SetArt("thumb", strThumb);
        items.Add(pItem);
        local = true;
      }
      else
        noneitem->SetArt("icon", "DefaultActor.png");
    }

    if (type == MediaTypeVideoCollection)
    {
      std::string localFile = FindLocalMovieSetArtworkFile(item, artType);
      if (!localFile.empty())
      {
        CFileItemPtr pItem(new CFileItem(localFile, false));
        pItem->SetLabel(g_localizeStrings.Get(13514));
        pItem->SetArt("thumb", localFile);
        items.Add(pItem);
        local = true;
      }
      else
        noneitem->SetArt("icon", "DefaultVideo.png");
    }
  }
  else
  {
    std::string strThumb;
    bool existsThumb = false;
    // First look for artist thumb in the primary location
    if (!artistPath.empty())
    {
      strThumb = URIUtils::AddFileToFolder(artistPath, "folder.jpg");
      existsThumb = CFileUtils::Exists(strThumb);
    }
    // If not there fall back local to music files (historic location for those album artists with a unique folder)
    if (!existsThumb && !artistOldPath.empty())
    {
      strThumb = URIUtils::AddFileToFolder(artistOldPath, "folder.jpg");
      existsThumb = CFileUtils::Exists(strThumb);
    }

    if (existsThumb)
    {
      CFileItemPtr pItem(new CFileItem(strThumb, false));
      pItem->SetLabel(g_localizeStrings.Get(13514));
      pItem->SetArt("thumb", strThumb);
      items.Add(pItem);
      local = true;
    }
    else
      noneitem->SetArt("icon", "DefaultArtist.png");
  }

  if (!local)
    items.Add(noneitem);

  std::string result;
  VECSOURCES sources=*CMediaSourceSettings::GetInstance().GetSources("video");
  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  if (type == MediaTypeVideoCollection)
  {
    AddItemPathStringToFileBrowserSources(sources,
        VIDEO::CVideoInfoScanner::GetMovieSetInfoFolder(item->GetLabel()),
        g_localizeStrings.Get(36041));
    AddItemPathStringToFileBrowserSources(sources,
        CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
            CSettings::SETTING_VIDEOLIBRARY_MOVIESETSFOLDER),
        "* " + g_localizeStrings.Get(20226));
  }
  else
    AddItemPathToFileBrowserSources(sources, *item);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(13511), result))
    return false;   // user cancelled

  if (result == "thumb://Current")
    result = currentThumb;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (result == "thumb://None")
    result.clear();
  else if (StringUtils::StartsWith(result, "thumb://Remote"))
  {
    int number = atoi(StringUtils::Mid(result, 14).c_str());
    result = thumbs[number];
  }

  // write the selected artwork to the database
  if (type == MediaTypeVideoCollection ||
      type == "actor" ||
      type == MediaTypeSeason ||
      (type == MediaTypeArtist && idArtist < 0))
    videodb.SetArtForItem(item->GetVideoInfoTag()->m_iDbId, item->GetVideoInfoTag()->m_type, artType, result);
  else
  {
    CMusicDatabase musicdb;
    if (musicdb.Open())
      musicdb.SetArtForItem(idArtist, MediaTypeArtist, artType, result);
  }

  item->SetArt(artType, result);

  CUtil::DeleteVideoDatabaseDirectoryCache();
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);

  return true;
}

std::string CGUIDialogVideoInfo::GetLocalizedVideoType(const std::string &strType)
{
  if (CMediaTypes::IsMediaType(strType, MediaTypeMovie))
    return g_localizeStrings.Get(20342);
  else if (CMediaTypes::IsMediaType(strType, MediaTypeTvShow))
    return g_localizeStrings.Get(20343);
  else if (CMediaTypes::IsMediaType(strType, MediaTypeEpisode))
    return g_localizeStrings.Get(20359);
  else if (CMediaTypes::IsMediaType(strType, MediaTypeMusicVideo))
    return g_localizeStrings.Get(20391);

  return "";
}

bool CGUIDialogVideoInfo::UpdateVideoItemSortTitle(const std::shared_ptr<CFileItem>& pItem)
{
  // dont allow update while scanning
  if (CVideoLibraryQueue::GetInstance().IsScanningLibrary())
  {
    HELPERS::ShowOKDialogText(CVariant{257}, CVariant{14057});
    return false;
  }

  CVideoDatabase database;
  if (!database.Open())
    return false;

  int iDbId = pItem->GetVideoInfoTag()->m_iDbId;
  CVideoInfoTag detail;
  VideoDbContentType iType = pItem->GetVideoContentType();
  if (iType == VideoDbContentType::MOVIES)
    database.GetMovieInfo("", detail, iDbId, VideoDbDetailsNone);
  else if (iType == VideoDbContentType::TVSHOWS)
    database.GetTvShowInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath, detail, iDbId, 0, VideoDbDetailsNone);

  std::string currentTitle;
  if (detail.m_strSortTitle.empty())
    currentTitle = detail.m_strTitle;
  else
    currentTitle = detail.m_strSortTitle;

  // get the new sort title
  if (!CGUIKeyboardFactory::ShowAndGetInput(currentTitle, CVariant{g_localizeStrings.Get(16107)}, false))
    return false;

  return database.UpdateVideoSortTitle(iDbId, currentTitle, iType);
}

bool CGUIDialogVideoInfo::LinkMovieToTvShow(const std::shared_ptr<CFileItem>& item,
                                            bool bRemove,
                                            CVideoDatabase& database)
{
  int dbId = item->GetVideoInfoTag()->m_iDbId;

  CFileItemList list;
  if (bRemove)
  {
    std::vector<int> ids;
    if (!database.GetLinksToTvShow(dbId, ids))
      return false;

    for (unsigned int i = 0; i < ids.size(); ++i)
    {
      CVideoInfoTag tag;
      database.GetTvShowInfo("", tag, ids[i], 0 , VideoDbDetailsNone);
      CFileItemPtr show(new CFileItem(tag));
      list.Add(show);
    }
  }
  else
  {
    database.GetTvShowsNav("videodb://tvshows/titles", list);

    // remove already linked shows
    std::vector<int> ids;
    if (!database.GetLinksToTvShow(dbId, ids))
      return false;

    for (int i = 0; i < list.Size(); )
    {
      size_t j;
      for (j = 0; j < ids.size(); ++j)
      {
        if (list[i]->GetVideoInfoTag()->m_iDbId == ids[j])
          break;
      }
      if (j == ids.size())
        i++;
      else
        list.Remove(i);
    }
  }

  int iSelectedLabel = 0;
  if (list.Size() > 1 || (!bRemove && !list.IsEmpty()))
  {
    list.Sort(SortByLabel, SortOrderAscending, CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_FILELISTS_IGNORETHEWHENSORTING) ? SortAttributeIgnoreArticle : SortAttributeNone);
    CGUIDialogSelect* pDialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(WINDOW_DIALOG_SELECT);
    if (pDialog)
    {
      pDialog->Reset();
      pDialog->SetItems(list);
      pDialog->SetHeading(CVariant{20356});
      pDialog->Open();
      iSelectedLabel = pDialog->GetSelectedItem();
    }
  }

  if (iSelectedLabel > -1 && iSelectedLabel < list.Size())
    return database.LinkMovieToTvshow(dbId, list[iSelectedLabel]->GetVideoInfoTag()->m_iDbId, bRemove);

  return false;
}

bool CGUIDialogVideoInfo::OnGetFanart(const std::shared_ptr<CFileItem>& videoItem)
{
  if (videoItem == nullptr || !videoItem->HasVideoInfoTag())
    return false;

  // update the db
  CVideoDatabase videodb;
  if (!videodb.Open())
    return false;

  CVideoThumbLoader loader;
  CFileItem item(*videoItem);
  loader.LoadItem(&item);

  CFileItemList items;
  if (item.HasArt("fanart"))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current", false));
    itemCurrent->SetArt("thumb", item.GetArt("fanart"));
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // add the none option
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetArt("icon", "DefaultVideo.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  std::string result;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("video"));
  CServiceBroker::GetMediaManager().GetLocalDrives(sources);
  AddItemPathToFileBrowserSources(sources, item);
  bool flip = false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445) ||
      StringUtils::EqualsNoCase(result, "fanart://Current"))
    return false;

  else if (StringUtils::EqualsNoCase(result, "fanart://None") || !CFileUtils::Exists(result))
    result.clear();
  if (!result.empty() && flip)
    result = CTextureUtils::GetWrappedImageURL(result, "", "flipped");

  videodb.SetArtForItem(item.GetVideoInfoTag()->m_iDbId, item.GetVideoInfoTag()->m_type, "fanart", result);

  // clear view cache and reload images
  CUtil::DeleteVideoDatabaseDirectoryCache();

  return true;
}

void CGUIDialogVideoInfo::ShowFor(const CFileItem& item)
{
  auto window = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIWindowVideoNav>(WINDOW_VIDEO_NAV);
  if (window)
  {
    ADDON::ScraperPtr info;
    window->OnItemInfo(item, info);
  }
}
