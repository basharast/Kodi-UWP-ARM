/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicFileDirectory.h"

#include "FileItem.h"
#include "URL.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

using namespace MUSIC_INFO;
using namespace XFILE;

CMusicFileDirectory::CMusicFileDirectory(void) = default;

CMusicFileDirectory::~CMusicFileDirectory(void) = default;

bool CMusicFileDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string strPath=url.Get();

  std::string strFileName;
  strFileName = URIUtils::GetFileName(strPath);
  URIUtils::RemoveExtension(strFileName);

  int iStreams = GetTrackCount(strPath);

  URIUtils::AddSlashAtEnd(strPath);

  for (int i=0; i<iStreams; ++i)
  {
    std::string strLabel = StringUtils::Format("%s - %s %2.2i", strFileName.c_str(), g_localizeStrings.Get(554).c_str(), i+1);
    CFileItemPtr pItem(new CFileItem(strLabel));
    strLabel = StringUtils::Format("%s%s-%i.%s", strPath.c_str(), strFileName.c_str(), i+1, m_strExt.c_str());
    pItem->SetPath(strLabel);

    /*
     * Try fist to load tag about related stream track. If them fails or not
     * available, take base tag for all streams (in this case the item names
     * are all the same).
     */
    MUSIC_INFO::CMusicInfoTag tag;
    if (Load(strLabel, tag, nullptr))
      *pItem->GetMusicInfoTag() = tag;
    else if (m_tag.Loaded())
      *pItem->GetMusicInfoTag() = m_tag;

    /*
     * Check track number not set and take stream entry number about.
     * NOTE: Audio decoder addons can also give a own track number.
     */
    if (pItem->GetMusicInfoTag()->GetTrackNumber() == 0)
      pItem->GetMusicInfoTag()->SetTrackNumber(i+1);
    items.Add(pItem);
  }

  return true;
}

bool CMusicFileDirectory::Exists(const CURL& url)
{
  return true;
}

bool CMusicFileDirectory::ContainsFiles(const CURL &url)
{
  const std::string pathToUrl(url.Get());
  if (GetTrackCount(pathToUrl) > 1)
    return true;

  return false;
}
