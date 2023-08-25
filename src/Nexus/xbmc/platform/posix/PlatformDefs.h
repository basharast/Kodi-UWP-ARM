/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#if defined(TARGET_DARWIN)
#include <stdio.h>
#include <sched.h>
#include <AvailabilityMacros.h>
#ifndef __STDC_FORMAT_MACROS
  #define __STDC_FORMAT_MACROS
#endif
#include <sys/sysctl.h>
#include <mach/mach.h>
#if defined(TARGET_DARWIN_OSX)
#include <libkern/OSTypes.h>
#endif

#elif defined(TARGET_FREEBSD)
#include <stdio.h>
#include <sys/sysctl.h>
#else
#include <sys/sysinfo.h>
#endif

#include <sys/time.h>
#include <time.h>

#if defined(__ppc__) || defined(__powerpc__)
#define PIXEL_ASHIFT 0
#define PIXEL_RSHIFT 8
#define PIXEL_GSHIFT 16
#define PIXEL_BSHIFT 24
#else
#define PIXEL_ASHIFT 24
#define PIXEL_RSHIFT 16
#define PIXEL_GSHIFT 8
#define PIXEL_BSHIFT 0
#endif

#include <stdint.h>

#define _fdopen fdopen
#define _vsnprintf vsnprintf

#define __stdcall
#define __cdecl
#define WINAPI      __stdcall
#undef APIENTRY
struct CXHandle; // forward declaration
typedef CXHandle* HANDLE;

typedef void* HINSTANCE;
typedef void* HMODULE;

typedef unsigned int  DWORD;
#define INVALID_HANDLE_VALUE     ((HANDLE)~0U)

#define MAXWORD   0xffff

typedef union _LARGE_INTEGER
{
  struct {
    DWORD LowPart;
    int32_t HighPart;
  } u;
  long long QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

 typedef union _ULARGE_INTEGER {
  struct {
      DWORD LowPart;
      DWORD HighPart;
  } u;
  unsigned long long QuadPart;
} ULARGE_INTEGER;

// Network
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (~0)
#define closesocket(s)  close(s)
#define ioctlsocket(s, f, v) ioctl(s, f, v)
#define WSAGetLastError() (errno)
#define WSAECONNRESET ECONNRESET

typedef int SOCKET;

// Thread
typedef int (*LPTHREAD_START_ROUTINE)(void *);

// File
#define O_BINARY 0
#define _O_TRUNC O_TRUNC
#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY

#if defined(TARGET_DARWIN) || defined(TARGET_FREEBSD)
  #define stat64 stat
  #define __stat64 stat
  #define fstat64 fstat
  typedef int64_t off64_t;
  #if defined(TARGET_FREEBSD)
    #define statfs64 statfs
  #endif
#else
  #define __stat64 stat64
#endif

struct _stati64 {
  dev_t st_dev;
  ino_t st_ino;
  unsigned short st_mode;
  short          st_nlink;
  short          st_uid;
  short          st_gid;
  dev_t st_rdev;
  long long  st_size;
  time_t _st_atime;
  time_t _st_mtime;
  time_t _st_ctime;
};

#define FILE_BEGIN              0
#define FILE_CURRENT            1
#define FILE_END                2

#define _S_IFREG  S_IFREG
#define _S_IFDIR  S_IFDIR
#define MAX_PATH PATH_MAX

// CreateFile defines
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define FILE_READ_DATA   ( 0x0001 )
#define FILE_WRITE_DATA  ( 0x0002 )
