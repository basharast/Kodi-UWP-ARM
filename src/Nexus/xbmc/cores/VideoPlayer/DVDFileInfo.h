/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CDVDDemux;
class CStreamDetails;
class CStreamDetailSubtitle;
class CDVDInputStream;
class CTextureDetails;

class CDVDFileInfo
{
public:
  // Extract a thumbnail image from the media referenced by fileItem, optionally populating a streamdetails class with the data
  static bool ExtractThumb(const CFileItem& fileItem,
                           CTextureDetails &details,
                           CStreamDetails *pStreamDetails,
                           int64_t pos);

  // Probe the files streams and store the info in the VideoInfoTag
  static bool GetFileStreamDetails(CFileItem *pItem);
  static bool DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                     CDVDDemux* pDemux,
                                     CStreamDetails& details,
                                     const std::string& path = "");

  /** \brief Probe the file's internal and external streams and store the info in the StreamDetails parameter.
  *   \param[out] details The file's StreamDetails consisting of internal streams and external subtitle streams.
  */
  static bool DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                     CDVDDemux* pDemuxer,
                                     const std::vector<CStreamDetailSubtitle>& subs,
                                     CStreamDetails& details);

  static bool GetFileDuration(const std::string &path, int &duration);

  /** \brief Probe the streams of an external subtitle file and store the info in the StreamDetails parameter.
  *   \param[out] details The external subtitle file's StreamDetails.
  */
  static bool AddExternalSubtitleToDetails(const std::string &path, CStreamDetails &details, const std::string& filename, const std::string& subfilename = "");
};
