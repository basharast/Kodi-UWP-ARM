/*
 *  Copyright (c) 2002 Frodo
 *      Portions Copyright (c) by the authors of ffmpeg and xvid
 *  Copyright (C) 2002-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// File.h: interface for the CFile class.
//
//////////////////////////////////////////////////////////////////////

#include "IFileTypes.h"
#include "URL.h"
#include "utils/auto_buffer.h"

#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

#include "PlatformDefs.h"

class BitstreamStats;

namespace XFILE
{

using ::XUTILS::auto_buffer;
class IFile;

class IFileCallback
{
public:
  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) = 0;
  virtual ~IFileCallback() = default;
};

class CFileStreamBuffer;

class CFile
{
public:
  CFile();
  ~CFile();

  bool CURLCreate(const std::string &url);
  bool CURLAddOption(XFILE::CURLOPTIONTYPE type, const char* name, const char * value);
  bool CURLOpen(unsigned int flags);

  /**
  * Attempt to open an IFile instance.
  * @param file reference to CCurl file description
  * @param flags see IFileTypes.h
  * @return true on success, false otherwise
  *
  * Remarks: Open can only be called once. Calling
  * Open() on an already opened file will fail
  * except if flag READ_REOPEN is set and the underlying
  * file has an implementation of ReOpen().
  */
  bool Open(const CURL& file, const unsigned int flags = 0);
  bool Open(const std::string& strFileName, const unsigned int flags = 0);

  bool OpenForWrite(const CURL& file, bool bOverWrite = false);
  bool OpenForWrite(const std::string& strFileName, bool bOverWrite = false);

  ssize_t LoadFile(const CURL &file, auto_buffer& outputBuffer);

  /**
   * Attempt to read bufSize bytes from currently opened file into buffer bufPtr.
   * @param bufPtr  pointer to buffer
   * @param bufSize size of the buffer
   * @return number of successfully read bytes if any bytes were read and stored in
   *         buffer, zero if no bytes are available to read (end of file was reached)
   *         or undetectable error occur, -1 in case of any explicit error
   */
  ssize_t Read(void* bufPtr, size_t bufSize);
  bool ReadString(char *szLine, int iLineLength);
  /**
   * Attempt to write bufSize bytes from buffer bufPtr into currently opened file.
   * @param bufPtr  pointer to buffer
   * @param bufSize size of the buffer
   * @return number of successfully written bytes if any bytes were written,
   *         zero if no bytes were written and no detectable error occur,
   *         -1 in case of any explicit error
   */
  ssize_t Write(const void* bufPtr, size_t bufSize);
  void Flush();
  int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  int Truncate(int64_t iSize);
  int64_t GetPosition() const;
  int64_t GetLength();
  void Close();
  int GetChunkSize();
  const std::string GetProperty(XFILE::FileProperty type, const std::string &name = "") const;
  const std::vector<std::string> GetPropertyValues(XFILE::FileProperty type, const std::string &name = "") const;
  ssize_t LoadFile(const std::string &filename, auto_buffer& outputBuffer);

  static int DetermineChunkSize(const int srcChunkSize, const int reqChunkSize);

  BitstreamStats* GetBitstreamStats() { return m_bitStreamStats; }

  int IoControl(EIoControl request, void* param);

  IFile *GetImplementation() const { return m_pFile; }

  // CURL interface
  static bool Exists(const CURL& file, bool bUseCache = true);
  static bool Delete(const CURL& file);
  /**
  * Fills struct __stat64 with information about file specified by filename
  * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
  * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
  * information is available. Function may set st_size (file size), st_atime,
  * st_mtime, st_ctime (access, modification, creation times).
  * Any other flags and members of __stat64 that didn't updated with actual file
  * information will be set to zero (st_nlink can be set ether to 1 or zero).
  * @param file        specifies requested file
  * @param buffer      pointer to __stat64 buffer to receive information about file
  * @return zero of success, -1 otherwise.
  */
  static int  Stat(const CURL& file, struct __stat64* buffer);
  static bool Rename(const CURL& file, const CURL& urlNew);
  static bool Copy(const CURL& file, const CURL& dest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool SetHidden(const CURL& file, bool hidden);

  // string interface
  static bool Exists(const std::string& strFileName, bool bUseCache = true);
  /**
  * Fills struct __stat64 with information about file specified by filename
  * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
  * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
  * information is available. Function may set st_size (file size), st_atime,
  * st_mtime, st_ctime (access, modification, creation times).
  * Any other flags and members of __stat64 that didn't updated with actual file
  * information will be set to zero (st_nlink can be set ether to 1 or zero).
  * @param strFileName specifies requested file
  * @param buffer      pointer to __stat64 buffer to receive information about file
  * @return zero of success, -1 otherwise.
  */
  static int  Stat(const std::string& strFileName, struct __stat64* buffer);
  /**
  * Fills struct __stat64 with information about currently open file
  * For st_mode function will set correctly _S_IFDIR (directory) flag and may set
  * _S_IREAD (read permission), _S_IWRITE (write permission) flags if such
  * information is available. Function may set st_size (file size), st_atime,
  * st_mtime, st_ctime (access, modification, creation times).
  * Any other flags and members of __stat64 that didn't updated with actual file
  * information will be set to zero (st_nlink can be set ether to 1 or zero).
  * @param buffer      pointer to __stat64 buffer to receive information about file
  * @return zero of success, -1 otherwise.
  */
  int Stat(struct __stat64 *buffer);
  static bool Delete(const std::string& strFileName);
  static bool Rename(const std::string& strFileName, const std::string& strNewFileName);
  static bool Copy(const std::string& strFileName, const std::string& strDest, XFILE::IFileCallback* pCallback = NULL, void* pContext = NULL);
  static bool SetHidden(const std::string& fileName, bool hidden);
  double GetDownloadSpeed();

private:
  unsigned int        m_flags;
  CURL                m_curl;
  IFile*              m_pFile;
  CFileStreamBuffer*  m_pBuffer;
  BitstreamStats*     m_bitStreamStats;
};

// streambuf for file io, only supports buffered input currently
class CFileStreamBuffer
  : public std::streambuf
{
public:
  ~CFileStreamBuffer() override;
  explicit CFileStreamBuffer(int backsize = 0);

  void Attach(IFile *file);
  void Detach();

private:
  int_type underflow() override;
  std::streamsize showmanyc() override;
  pos_type seekoff(off_type, std::ios_base::seekdir,std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override;
  pos_type seekpos(pos_type, std::ios_base::openmode = std::ios_base::in | std::ios_base::out) override;

  IFile* m_file;
  char*  m_buffer;
  int    m_backsize;
  int    m_frontsize = 0;
};

// very basic file input stream
class CFileStream
  : public std::istream
{
public:
  explicit CFileStream(int backsize = 0);
  ~CFileStream() override;

  bool Open(const std::string& filename);
  bool Open(const CURL& filename);
  void Close();

  int64_t GetLength();
private:
  CFileStreamBuffer m_buffer;
  IFile*            m_file;
};

}
