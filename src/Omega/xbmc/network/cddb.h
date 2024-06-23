/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <map>
#ifndef TARGET_POSIX
#include <strstream>
#endif
#include "storage/cdioSupport.h"

#include "utils/ScopeGuard.h"

namespace CDDB
{

//Can be removed if/when removing Xcddb::queryCDinfo(int real_track_count, toc cdtoc[])
//#define IN_PROGRESS           -1
//#define QUERY_OK             7
//#define E_INEXACT_MATCH_FOUND      211
//#define W_CDDB_already_shook_hands      402
//#define E_CDDB_Handshake_not_successful 431

#define E_TOC_INCORRECT           2
#define E_NETWORK_ERROR_OPEN_SOCKET     3
#define E_NETWORK_ERROR_SEND       4
#define E_WAIT_FOR_INPUT         5
#define E_PARAMETER_WRONG         6
#define E_NO_MATCH_FOUND        202

#define CDDB_PORT 8880


struct toc
{
  int min;
  int sec;
  int frame;
};


class Xcddb
{
public:
  Xcddb();
  virtual ~Xcddb();
  void setCDDBIpAddress(const std::string& ip_address);
  void setCacheDir(const std::string& pCacheDir );

//  int queryCDinfo(int real_track_count, toc cdtoc[]);
  bool queryCDinfo(MEDIA_DETECT::CCdInfo* pInfo, int inexact_list_select);
  bool queryCDinfo(MEDIA_DETECT::CCdInfo* pInfo);
  int getLastError() const;
  const char * getLastErrorText() const;
  const std::string& getYear() const;
  const std::string& getGenre() const;
  const std::string& getTrackArtist(int track) const;
  const std::string& getTrackTitle(int track) const;
  void getDiskArtist(std::string& strdisk_artist) const;
  void getDiskTitle(std::string& strdisk_title) const;
  const std::string& getTrackExtended(int track) const;
  uint32_t calc_disc_id(int nr_of_tracks, toc cdtoc[]);
  const std::string& getInexactArtist(int select) const;
  const std::string& getInexactTitle(int select) const;
  bool queryCache( uint32_t discid );
  bool writeCacheFile( const char* pBuffer, uint32_t discid );
  bool isCDCached( int nr_of_tracks, toc cdtoc[] );
  bool isCDCached( MEDIA_DETECT::CCdInfo* pInfo );

protected:
  std::string m_strNull;
#if defined(TARGET_WINDOWS)
  using CAutoPtrSocket = KODI::UTILS::CScopeGuard<SOCKET, INVALID_SOCKET, decltype(closesocket)>;
#else
  using CAutoPtrSocket = KODI::UTILS::CScopeGuard<int, -1, decltype(close)>;
#endif
  CAutoPtrSocket m_cddb_socket;
  const static int recv_buffer = 4096;
  int m_lastError;
  std::map<int, std::string> m_mapTitles;
  std::map<int, std::string> m_mapArtists;
  std::map<int, std::string> m_mapExtended_track;

  std::map<int, std::string> m_mapInexact_cddb_command_list;
  std::map<int, std::string> m_mapInexact_artist_list;
  std::map<int, std::string> m_mapInexact_title_list;


  std::string m_strDisk_artist;
  std::string m_strDisk_title;
  std::string m_strYear;
  std::string m_strGenre;

  void addTitle(const char *buffer);
  void addExtended(const char *buffer);
  void parseData(const char *buffer);
  bool Send( const void *buffer, int bytes );
  bool Send( const char *buffer);
  std::string Recv(bool wait4point);
  bool openSocket();
  bool closeSocket();
  struct toc cdtoc[100];
  int cddb_sum(int n);
  void addInexactList(const char *list);
  void addInexactListLine(int line_cnt, const char *line, int len);
  const std::string& getInexactCommand(int select) const;
  std::string GetCacheFile(uint32_t disc_id) const;
  /*! \brief Trim and convert some text to UTF8
   \param untrimmedText original text to trim and convert
   \return a utf8 version of the trimmed text
   */
  std::string TrimToUTF8(const std::string &untrimmed);

  std::string m_cddb_ip_address;
  std::string cCacheDir;
};
}
