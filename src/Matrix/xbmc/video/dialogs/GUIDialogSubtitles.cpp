/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogSubtitles.h"

#include "Application.h"
#include "LangInfo.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/AddonManager.h"
#include "cores/IPlayer.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/AddonsDirectory.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "filesystem/StackDirectory.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIKeyboardFactory.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/lib/Setting.h"
#include "utils/JobManager.h"
#include "utils/LangCodeExpander.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

using namespace ADDON;
using namespace XFILE;

#define CONTROL_NAMELABEL            100
#define CONTROL_NAMELOGO             110
#define CONTROL_SUBLIST              120
#define CONTROL_SUBSEXIST            130
#define CONTROL_SUBSTATUS            140
#define CONTROL_SERVICELIST          150
#define CONTROL_MANUALSEARCH         160

/*! \brief simple job to retrieve a directory and store a string (language)
 */
class CSubtitlesJob: public CJob
{
public:
  CSubtitlesJob(const CURL &url, const std::string &language) : m_url(url), m_language(language)
  {
    m_items = new CFileItemList;
  }
  ~CSubtitlesJob() override
  {
    delete m_items;
  }
  bool DoWork() override
  {
    CDirectory::GetDirectory(m_url.Get(), *m_items, "", DIR_FLAG_DEFAULTS);
    return true;
  }
  bool operator==(const CJob *job) const override
  {
    if (strcmp(job->GetType(),GetType()) == 0)
    {
      const CSubtitlesJob* rjob = dynamic_cast<const CSubtitlesJob*>(job);
      if (rjob)
      {
        return m_url.Get() == rjob->m_url.Get() &&
               m_language == rjob->m_language;
      }
    }
    return false;
  }
  const CFileItemList *GetItems() const { return m_items; }
  const CURL &GetURL() const { return m_url; }
  const std::string &GetLanguage() const { return m_language; }
private:
  CURL           m_url;
  CFileItemList *m_items;
  std::string    m_language;
};

CGUIDialogSubtitles::CGUIDialogSubtitles(void)
    : CGUIDialog(WINDOW_DIALOG_SUBTITLES, "DialogSubtitles.xml")
    , m_subtitles(new CFileItemList)
    , m_serviceItems(new CFileItemList)
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogSubtitles::~CGUIDialogSubtitles(void)
{
  CancelJobs();
  delete m_subtitles;
  delete m_serviceItems;
}

bool CGUIDialogSubtitles::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    bool selectAction = (message.GetParam1() == ACTION_SELECT_ITEM ||
                         message.GetParam1() == ACTION_MOUSE_LEFT_CLICK);

    if (selectAction && iControl == CONTROL_SUBLIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SUBLIST);
      OnMessage(msg);

      int item = msg.GetParam1();
      if (item >= 0 && item < m_subtitles->Size())
        Download(*m_subtitles->Get(item));
      return true;
    }
    else if (selectAction && iControl == CONTROL_SERVICELIST)
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SERVICELIST);
      OnMessage(msg);

      int item = msg.GetParam1();
      if (item >= 0 && item < m_serviceItems->Size())
      {
        SetService(m_serviceItems->Get(item)->GetProperty("Addon.ID").asString());
        Search();
      }
      return true;
    }
    else if (iControl == CONTROL_MANUALSEARCH)
    {
      //manual search
      if (CGUIKeyboardFactory::ShowAndGetInput(m_strManualSearch, CVariant{g_localizeStrings.Get(24121)}, true))
      {
        Search(m_strManualSearch);
        return true;
      }
    }
  }
  else if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    // Resume the video if the user has requested it
    if (g_application.GetAppPlayer().IsPaused() && m_pausedOnRun)
      g_application.GetAppPlayer().Pause();

    CGUIDialog::OnMessage(message);

    ClearSubtitles();
    ClearServices();
    return true;
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogSubtitles::OnInitWindow()
{
  // Pause the video if the user has requested it
  m_pausedOnRun = false;
  if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SUBTITLES_PAUSEONSEARCH) && !g_application.GetAppPlayer().IsPaused())
  {
    g_application.GetAppPlayer().Pause();
    m_pausedOnRun = true;
  }

  FillServices();
  CGUIWindow::OnInitWindow();
  Search();
}

void CGUIDialogSubtitles::Process(unsigned int currentTime, CDirtyRegionList &dirtyregions)
{
  if (m_bInvalidated)
  {
    // take copies of our variables to ensure we don't hold the lock for long.
    std::string status;
    CFileItemList subs;
    {
      CSingleLock lock(m_critsection);
      status = m_status;
      subs.Assign(*m_subtitles);
    }
    SET_CONTROL_LABEL(CONTROL_SUBSTATUS, status);

    if (m_updateSubsList)
    {
      CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SUBLIST, 0, 0, &subs);
      OnMessage(message);
      if (!subs.IsEmpty())
      {
        // focus subtitles list
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_SUBLIST);
        OnMessage(msg);
      }
      m_updateSubsList = false;
    }

    int control = GetFocusedControlID();
    // nothing has focus
    if (!control)
    {
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_subtitles->IsEmpty() ?
                      CONTROL_SERVICELIST : CONTROL_SUBLIST);
      OnMessage(msg);
    }
    // subs list is focused but we have no subs
    else if (control == CONTROL_SUBLIST && m_subtitles->IsEmpty())
    {
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_SERVICELIST);
      OnMessage(msg);
    }
  }
  CGUIDialog::Process(currentTime, dirtyregions);
}

void CGUIDialogSubtitles::FillServices()
{
  ClearServices();

  VECADDONS addons;
  CServiceBroker::GetAddonMgr().GetAddons(addons, ADDON_SUBTITLE_MODULE);

  if (addons.empty())
  {
    UpdateStatus(NO_SERVICES);
    return;
  }

  std::string defaultService;
  const CFileItem &item = g_application.CurrentUnstackedItem();
  if (item.GetVideoContentType() == VIDEODB_CONTENT_TVSHOWS ||
      item.GetVideoContentType() == VIDEODB_CONTENT_EPISODES)
    // Set default service for tv shows
    defaultService = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_TV);
  else
    // Set default service for filemode and movies
    defaultService = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(CSettings::SETTING_SUBTITLES_MOVIE);

  std::string service = addons.front()->ID();
  for (VECADDONS::const_iterator addonIt = addons.begin(); addonIt != addons.end(); ++addonIt)
  {
    CFileItemPtr item(CAddonsDirectory::FileItemFromAddon(*addonIt, "plugin://" + (*addonIt)->ID(), false));
    m_serviceItems->Add(item);
    if ((*addonIt)->ID() == defaultService)
      service = (*addonIt)->ID();
  }

  // Bind our services to the UI
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SERVICELIST, 0, 0, m_serviceItems);
  OnMessage(msg);

  SetService(service);
}

bool CGUIDialogSubtitles::SetService(const std::string &service)
{
  if (service != m_currentService)
  {
    m_currentService = service;
    CLog::Log(LOGDEBUG, "New Service [%s] ", m_currentService.c_str());

    CFileItemPtr currentService = GetService();
    // highlight this item in the skin
    for (int i = 0; i < m_serviceItems->Size(); i++)
    {
      CFileItemPtr pItem = m_serviceItems->Get(i);
      pItem->Select(pItem == currentService);
    }

    SET_CONTROL_LABEL(CONTROL_NAMELABEL, currentService->GetLabel());

    if (currentService->HasAddonInfo())
    {
      std::string icon = URIUtils::AddFileToFolder(currentService->GetAddonInfo()->Path(), "logo.png");
      SET_CONTROL_FILENAME(CONTROL_NAMELOGO, icon);
    }

    if (g_application.GetAppPlayer().GetSubtitleCount() == 0)
      SET_CONTROL_HIDDEN(CONTROL_SUBSEXIST);
    else
      SET_CONTROL_VISIBLE(CONTROL_SUBSEXIST);

    return true;
  }
  return false;
}

const CFileItemPtr CGUIDialogSubtitles::GetService() const
{
  for (int i = 0; i < m_serviceItems->Size(); i++)
  {
    if (m_serviceItems->Get(i)->GetProperty("Addon.ID") == m_currentService)
      return m_serviceItems->Get(i);
  }
  return CFileItemPtr();
}

void CGUIDialogSubtitles::Search(const std::string &search/*=""*/)
{
  if (m_currentService.empty())
    return; // no services available

  UpdateStatus(SEARCHING);
  ClearSubtitles();

  CURL url("plugin://" + m_currentService + "/");
  if (!search.empty())
  {
    url.SetOption("action", "manualsearch");
    url.SetOption("searchstring", search);
  }
  else
    url.SetOption("action", "search");

  const std::shared_ptr<CSettings> settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  SettingConstPtr setting = settings->GetSetting(CSettings::SETTING_SUBTITLES_LANGUAGES);
  if (setting)
    url.SetOption("languages", setting->ToString());

  // Check for stacking
  if (g_application.CurrentFileItem().IsStack())
    url.SetOption("stack", "1");

  std::string preferredLanguage = settings->GetString(CSettings::SETTING_LOCALE_SUBTITLELANGUAGE);

  if(StringUtils::EqualsNoCase(preferredLanguage, "original"))
  {
    AudioStreamInfo info;
    std::string strLanguage;

    g_application.GetAppPlayer().GetAudioStreamInfo(CURRENT_STREAM, info);

    if (!g_LangCodeExpander.Lookup(info.language, strLanguage))
      strLanguage = "Unknown";

    preferredLanguage = strLanguage;
  }
  else if (StringUtils::EqualsNoCase(preferredLanguage, "default"))
    preferredLanguage = g_langInfo.GetEnglishLanguageName();

  url.SetOption("preferredlanguage", preferredLanguage);

  AddJob(new CSubtitlesJob(url, ""));
}

void CGUIDialogSubtitles::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  const CURL &url             = static_cast<CSubtitlesJob*>(job)->GetURL();
  const CFileItemList *items  = static_cast<CSubtitlesJob*>(job)->GetItems();
  const std::string &language = static_cast<CSubtitlesJob*>(job)->GetLanguage();
  if (url.GetOption("action") == "search" || url.GetOption("action") == "manualsearch")
    OnSearchComplete(items);
  else
    OnDownloadComplete(items, language);
  CJobQueue::OnJobComplete(jobID, success, job);
}

void CGUIDialogSubtitles::OnSearchComplete(const CFileItemList *items)
{
  CSingleLock lock(m_critsection);
  m_subtitles->Assign(*items);
  UpdateStatus(SEARCH_COMPLETE);
  m_updateSubsList = true;

  if (!items->IsEmpty() && g_application.GetAppPlayer().GetSubtitleCount() == 0 &&
    m_LastAutoDownloaded != g_application.CurrentFile() && CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(CSettings::SETTING_SUBTITLES_DOWNLOADFIRST))
  {
    CFileItemPtr item = items->Get(0);
    CLog::Log(LOGDEBUG, "%s - Automatically download first subtitle: %s", __FUNCTION__, item->GetLabel2().c_str());
    m_LastAutoDownloaded = g_application.CurrentFile();
    Download(*item);
  }

  SetInvalid();
}

void CGUIDialogSubtitles::UpdateStatus(STATUS status)
{
  CSingleLock lock(m_critsection);
  std::string label;
  switch (status)
  {
    case NO_SERVICES:
      label = g_localizeStrings.Get(24114);
      break;
    case SEARCHING:
      label = g_localizeStrings.Get(24107);
      break;
    case SEARCH_COMPLETE:
      if (!m_subtitles->IsEmpty())
        label = StringUtils::Format(g_localizeStrings.Get(24108).c_str(), m_subtitles->Size());
      else
        label = g_localizeStrings.Get(24109);
      break;
    case DOWNLOADING:
      label = g_localizeStrings.Get(24110);
      break;
    default:
      break;
  }
  if (label != m_status)
  {
    m_status = label;
    SetInvalid();
  }
}

void CGUIDialogSubtitles::Download(const CFileItem &subtitle)
{
  UpdateStatus(DOWNLOADING);

  // subtitle URL should be of the form plugin://<addonid>/?param=foo&param=bar
  // we just append (if not already present) the action=download parameter.
  CURL url(subtitle.GetURL());
  if (url.GetOption("action").empty())
    url.SetOption("action", "download");

  AddJob(new CSubtitlesJob(url, subtitle.GetLabel()));
}

void CGUIDialogSubtitles::OnDownloadComplete(const CFileItemList *items, const std::string &language)
{
  if (items->IsEmpty())
  {
    CFileItemPtr service = GetService();
    if (service)
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, service->GetLabel(), g_localizeStrings.Get(24113));
    UpdateStatus(SEARCH_COMPLETE);
    return;
  }

  SUBTITLE_STORAGEMODE storageMode = (SUBTITLE_STORAGEMODE) CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(CSettings::SETTING_SUBTITLES_STORAGEMODE);

  // Get (unstacked) path
  std::string strCurrentFile = g_application.CurrentUnstackedItem().GetDynPath();

  std::string strDownloadPath = "special://temp";
  std::string strDestPath;
  std::vector<std::string> vecFiles;

  std::string strCurrentFilePath;
  if (StringUtils::StartsWith(strCurrentFilePath, "http://"))
  {
    strCurrentFile = "TempSubtitle";
    vecFiles.push_back(strCurrentFile);
  }
  else
  {
    std::string subPath = CSpecialProtocol::TranslatePath("special://subtitles");
    if (!subPath.empty())
      strDownloadPath = subPath;

    /** Get item's folder for sub storage, special case for RAR/ZIP items
     * @todo We need some way to avoid special casing this all over the place
     * for rar/zip (perhaps modify GetDirectory?)
     */
    if (URIUtils::IsInRAR(strCurrentFile) || URIUtils::IsInZIP(strCurrentFile))
      strCurrentFilePath = URIUtils::GetDirectory(CURL(strCurrentFile).GetHostName());
    else
      strCurrentFilePath = URIUtils::GetDirectory(strCurrentFile);

    // Handle stacks
    if (g_application.CurrentFileItem().IsStack() && items->Size() > 1)
    {
      CStackDirectory::GetPaths(g_application.CurrentFileItem().GetPath(), vecFiles);
      // Make sure (stack) size is the same as the size of the items handed to us, else fallback to single item
      if (items->Size() != (int) vecFiles.size())
      {
        vecFiles.clear();
        vecFiles.push_back(strCurrentFile);
      }
    }
    else
    {
      vecFiles.push_back(strCurrentFile);
    }

    if (storageMode == SUBTITLE_STORAGEMODE_MOVIEPATH &&
        CUtil::SupportsWriteFileOperations(strCurrentFilePath))
    {
      strDestPath = strCurrentFilePath;
    }
  }

  // Use fallback?
  if (strDestPath.empty())
    strDestPath = strDownloadPath;

  // Extract the language and appropriate extension
  std::string strSubLang;
  g_LangCodeExpander.ConvertToISO6391(language, strSubLang);

  // Iterate over all items to transfer
  for (unsigned int i = 0; i < vecFiles.size() && i < (unsigned int) items->Size(); i++)
  {
    std::string strUrl = items->Get(i)->GetPath();
    std::string strFileName = URIUtils::GetFileName(vecFiles[i]);
    URIUtils::RemoveExtension(strFileName);

    // construct subtitle path
    std::string strSubExt = URIUtils::GetExtension(strUrl);
    std::string strSubName = StringUtils::Format("%s.%s%s", strFileName.c_str(), strSubLang.c_str(), strSubExt.c_str());

    // Handle URL encoding:
    std::string strDownloadFile = URIUtils::ChangeBasePath(strCurrentFilePath, strSubName, strDownloadPath);
    std::string strDestFile = strDownloadFile;

    if (!CFile::Copy(strUrl, strDownloadFile))
    {
      CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Error, strSubName, g_localizeStrings.Get(24113));
      CLog::Log(LOGERROR, "%s - Saving of subtitle %s to %s failed", __FUNCTION__, strUrl.c_str(), strDownloadFile.c_str());
    }
    else
    {
      if (strDestPath != strDownloadPath)
      {
        // Handle URL encoding:
        std::string strTryDestFile = URIUtils::ChangeBasePath(strCurrentFilePath, strSubName, strDestPath);

        /* Copy the file from temp to our final destination, if that fails fallback to download path
         * (ie. special://subtitles or use special://temp). Note that after the first item strDownloadPath equals strDestpath
         * so that all remaining items (including the .idx below) are copied directly to their final destination and thus all
         * items end up in the same folder
         */
        CLog::Log(LOGDEBUG, "%s - Saving subtitle %s to %s", __FUNCTION__, strDownloadFile.c_str(), strTryDestFile.c_str());
        if (CFile::Copy(strDownloadFile, strTryDestFile))
        {
          CFile::Delete(strDownloadFile);
          strDestFile = strTryDestFile;
          strDownloadPath = strDestPath; // Update download path so all the other items get directly downloaded to our final destination
        }
        else
        {
          CLog::Log(LOGWARNING, "%s - Saving of subtitle %s to %s failed. Falling back to %s", __FUNCTION__, strDownloadFile.c_str(), strTryDestFile.c_str(), strDownloadPath.c_str());
          strDestPath = strDownloadPath; // Copy failed, use fallback for the rest of the items
        }
      }
      else
      {
        CLog::Log(LOGDEBUG, "%s - Saved subtitle %s to %s", __FUNCTION__, strUrl.c_str(), strDownloadFile.c_str());
      }

      // for ".sub" subtitles we check if ".idx" counterpart exists and copy that as well
      if (StringUtils::EqualsNoCase(strSubExt, ".sub"))
      {
        strUrl = URIUtils::ReplaceExtension(strUrl, ".idx");
        if(CFile::Exists(strUrl))
        {
          std::string strSubNameIdx = StringUtils::Format("%s.%s.idx", strFileName.c_str(), strSubLang.c_str());
          // Handle URL encoding:
          strDestFile = URIUtils::ChangeBasePath(strCurrentFilePath, strSubNameIdx, strDestPath);
          CFile::Copy(strUrl, strDestFile);
        }
      }

      // Set sub for currently playing (stack) item
      if (vecFiles[i] == strCurrentFile)
        SetSubtitles(strDestFile);
    }
  }

  // Notify window manager that a subtitle was downloaded
  CGUIMessage msg(GUI_MSG_SUBTITLE_DOWNLOADED, 0, 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);

  // Close the window
  Close();
}

void CGUIDialogSubtitles::ClearSubtitles()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SUBLIST);
  OnMessage(msg);
  CSingleLock lock(m_critsection);
  m_subtitles->Clear();
}

void CGUIDialogSubtitles::ClearServices()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SERVICELIST);
  OnMessage(msg);
  m_serviceItems->Clear();
  m_currentService.clear();
}

void CGUIDialogSubtitles::SetSubtitles(const std::string &subtitle)
{
  g_application.GetAppPlayer().AddSubtitle(subtitle);
}
