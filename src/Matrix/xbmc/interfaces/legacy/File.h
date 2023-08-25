/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "AddonString.h"
#include "LanguageHook.h"
#include "commons/Buffer.h"
#include "filesystem/File.h"

#include <algorithm>

namespace XBMCAddon
{

  namespace xbmcvfs
  {

    //
    /// \defgroup python_file File
    /// \ingroup python_xbmcvfs
    /// @{
    /// @brief <b>Kodi's file class.</b>
    ///
    /// \python_class{ xbmcvfs.File(filepath, [mode]) }
    ///
    /// @param filepath             string Selected file path
    /// @param mode                 [opt] string Additional mode options (if no mode is supplied, the default is Open for Read).
    ///   |  Mode  | Description                     |
    ///   |:------:|:--------------------------------|
    ///   |   w    | Open for write                  |
    ///
    ///
    ///--------------------------------------------------------------------------
    /// @python_v19 Added context manager support
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// f = xbmcvfs.File(file, 'w')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    /// **Example (v19 and up):**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// with xbmcvfs.File(file, 'w') as f:
    ///   ..
    /// ..
    /// ~~~~~~~~~~~~~
    //
    class File : public AddonClass
    {
      XFILE::CFile* file;
    public:
      inline File(const String& filepath, const char* mode = NULL) : file(new XFILE::CFile())
      {
        DelayedCallGuard dg(languageHook);
        if (mode && strncmp(mode, "w", 1) == 0)
          file->OpenForWrite(filepath,true);
        else
          file->Open(filepath, XFILE::READ_NO_CACHE);
      }

      inline ~File() override { delete file; }

#if !defined(DOXYGEN_SHOULD_USE_THIS)
      inline File* __enter__() { return this; };
      inline void __exit__() { close(); };
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ read([bytes]) }
      /// Read file parts as string.
      ///
      /// @param bytes              [opt] How many bytes to read - if not
      ///                               set it will read the whole file
      /// @return                       string
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// b = f.read()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file) as file:
      ///   b = f.read()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      read(...);
#else
      inline String read(unsigned long numBytes = 0)
      {
        XbmcCommons::Buffer b = readBytes(numBytes);
        return b.getString(numBytes == 0 ? b.remaining() : std::min((unsigned long)b.remaining(),numBytes));
      }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ readBytes(numbytes) }
      /// Read bytes from file.
      ///
      /// @param numbytes           How many bytes to read [opt]- if not set
      ///                               it will read the whole file
      /// @return                       bytearray
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// b = f.readBytes()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file) as f:
      ///   b = f.readBytes()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      readBytes(...);
#else
      XbmcCommons::Buffer readBytes(unsigned long numBytes = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ write(buffer) }
      /// To write given data in file.
      ///
      /// @param buffer             Buffer to write to file
      /// @return                       True on success.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file, 'w')
      /// result = f.write(buffer)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file, 'w') as f:
      ///   result = f.write(buffer)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      write(...);
#else
      bool write(XbmcCommons::Buffer& buffer);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ size() }
      /// Get the file size.
      ///
      /// @return                       The file size
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// s = f.size()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file) as f:
      ///   s = f.size()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      size();
#else
      inline long long size() { DelayedCallGuard dg(languageHook); return file->GetLength(); }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ seek(seekBytes, iWhence) }
      /// Seek to position in file.
      ///
      /// @param seekBytes          position in the file
      /// @param iWhence            [opt] where in a file to seek from[0 beginning,
      ///                           1 current , 2 end position]
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 Function changed. param **iWhence** is now optional.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// result = f.seek(8129, 0)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file) as f:
      ///   result = f.seek(8129, 0)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      seek(...);
#else
      inline long long seek(long long seekBytes, int iWhence = SEEK_SET) { DelayedCallGuard dg(languageHook); return file->Seek(seekBytes,iWhence); }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ tell() }
      /// Get the current position in the file.
      ///
      /// @return                       The file position
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v19 New function added
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// s = f.tell()
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file) as f:
      ///   s = f.tell()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      tell();
#else
      inline long long tell() { DelayedCallGuard dg(languageHook); return file->GetPosition(); }
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_file
      /// @brief \python_func{ close() }
      /// Close opened file.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// f = xbmcvfs.File(file)
      /// f.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      /// **Example (v19 and up):**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// with xbmcvfs.File(file) as f:
      ///   ..
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      close();
#else
      inline void close() { DelayedCallGuard dg(languageHook); file->Close(); }
#endif

#ifndef SWIG
      inline const XFILE::CFile* getFile() const { return file; }
#endif

    };
    //@}
  }
}
