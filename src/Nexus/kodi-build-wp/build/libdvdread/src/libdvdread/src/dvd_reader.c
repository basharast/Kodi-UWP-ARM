/*
 * Copyright (C) 2001-2004 Billy Biggs <vektor@dumbterm.net>,
 *                         Håkan Hjort <d95hjort@dtek.chalmers.se>,
 *                         Björn Englund <d4bjorn@dtek.chalmers.se>
 *
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"
#include <sys/types.h>      /* off_t */
#include <sys/stat.h>       /* stat */
#include <sys/time.h>       /* For the timing of dvdcss_title crack. */
#include <fcntl.h>          /* open */
#include <stdlib.h>         /* free */
#include <stdio.h>          /* fprintf */
#include <errno.h>          /* errno, EIN* */
#include <string.h>         /* memcpy, strlen */
#include <unistd.h>         /* pclose */
#include <limits.h>         /* PATH_MAX */
#if HAVE_DIRENT_H
#include <dirent.h>         /* opendir, readdir */
#endif
#include <ctype.h>          /* isalpha */
#ifndef WIN32
#include <paths.h>
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
#define getenv(x) NULL
#endif
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__bsdi__) || defined(__APPLE__)
# define SYS_BSD 1
#endif

#if defined(__sun)
# include <sys/mnttab.h>
#elif defined(__APPLE__)
# include <sys/param.h>
# include <sys/ucred.h>
# include <sys/mount.h>
#elif defined(SYS_BSD)
# include <fstab.h>
#elif defined(__linux__)
# include <mntent.h>
# include <paths.h>
#endif

#include "dvdread/dvd_udf.h"
#include "dvdread/dvd_reader.h"
#include "dvd_input.h"
#include "dvdread_internal.h"
#include "md5.h"
#include "dvdread/ifo_read.h"

#if defined(_WIN32)
# include <windows.h>
# include "msvc/contrib/win32_cs.h"
#endif

/* misc win32 helpers */

#ifdef _WIN32
# ifndef HAVE_GETTIMEOFDAY
   /* replacement gettimeofday implementation */
# include <sys/timeb.h>
static inline int _private_gettimeofday( struct timeval *tv, void *tz )
{
  struct timeb t;
  ftime( &t );
  tv->tv_sec = t.time;
  tv->tv_usec = t.millitm * 1000;
  return 0;
}
#  define gettimeofday(TV, TZ) _private_gettimeofday((TV), (TZ))
#endif
#endif /* _WIN32 */


typedef struct stat dvdstat_t;
static inline int dvdstat(const char *file, dvdstat_t *st) {
  return stat(file, st);
}

#define DEFAULT_UDF_CACHE_LEVEL 1

struct dvd_reader_device_s {
  /* Basic information. */
  int isImageFile;

  /* Hack for keeping track of the css status.
   * 0: no css, 1: perhaps (need init of keys), 2: have done init */
  int css_state;
  int css_title; /* Last title that we have called dvdinpute_title for. */

  /* Information required for an image file. */
  dvd_input_t dev;

  /* Information required for a directory path drive. */
  char *path_root;

  /* Filesystem cache */
  int udfcache_level; /* 0 - turned off, 1 - on */
  void *udfcache;
};

#define TITLES_MAX 9

struct dvd_file_s {
  /* Basic information. */
  dvd_reader_t *ctx;

  /* Hack for selecting the right css title. */
  int css_title;

  /* Information required for an image file. */
  uint32_t lb_start;
  uint32_t seek_pos;

  /* Information required for a directory path drive. */
  size_t title_sizes[ TITLES_MAX ];
  dvd_input_t title_devs[ TITLES_MAX ];

  /* Calculated at open-time, size in blocks. */
  ssize_t filesize;

  /* Cache of the dvd_file. If not NULL, the cache corresponds to the whole
   * dvd_file. Used only for IFO and BUP. */
  unsigned char *cache;
};

/**
 * Set the level of caching on udf
 * level = 0 (no caching)
 * level = 1 (caching filesystem info)
 */
int DVDUDFCacheLevel(dvd_reader_t *reader, int level)
{
  dvd_reader_device_t *dev = reader->rd;

  if(level > 0) {
    level = 1;
  } else if(level < 0) {
    return dev->udfcache_level;
  }

  dev->udfcache_level = level;

  return level;
}

void *GetUDFCacheHandle(dvd_reader_t *reader)
{
  dvd_reader_device_t *dev = reader->rd;

  return dev->udfcache;
}

void SetUDFCacheHandle(dvd_reader_t *reader, void *cache)
{
  dvd_reader_device_t *dev = reader->rd;

  dev->udfcache = cache;
}



/* Loop over all titles and call dvdcss_title to crack the keys. */
static int initAllCSSKeys( dvd_reader_t *ctx )
{
  dvd_reader_device_t *dvd = ctx->rd;
  struct timeval all_s, all_e;
  struct timeval t_s, t_e;
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint32_t start, len;
  int title;

  const char *nokeys_str = getenv("DVDREAD_NOKEYS");
  if(nokeys_str != NULL)
    return 0;

  Log2(ctx,"Attempting to retrieve all CSS keys" );
  Log2(ctx,"This can take a _long_ time, please be patient" );
  gettimeofday(&all_s, NULL);

  for( title = 0; title < 100; title++ ) {
    gettimeofday( &t_s, NULL );
    if( title == 0 ) {
      strcpy( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
    } else {
      sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, 0 );
    }
    start = UDFFindFile( ctx, filename, &len );
    if( start != 0 && len != 0 ) {
      /* Perform CSS key cracking for this title. */
      Log3(ctx,"Get key for %s at 0x%08x",filename, start );
      if( dvdinput_title( dvd->dev, (int)start ) < 0 ) {
        Log1(ctx,"Error cracking CSS key for %s (0x%08x)", filename, start);
      }
      gettimeofday( &t_e, NULL );
      Log3(ctx,"Elapsed time %ld", (long int) t_e.tv_sec - t_s.tv_sec );
    }

    if( title == 0 ) continue;

    gettimeofday( &t_s, NULL );
    sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, 1 );
    start = UDFFindFile( ctx, filename, &len );
    if( start == 0 || len == 0 ) break;

    /* Perform CSS key cracking for this title. */
    Log3(ctx,"Get key for %s at 0x%08x",filename, start );
    if( dvdinput_title( dvd->dev, (int)start ) < 0 ) {
      Log1(ctx,"Error cracking CSS key for %s (0x%08x)", filename, start);
    }
    gettimeofday( &t_e, NULL );
    Log3(ctx,"Elapsed time %ld", (long int) t_e.tv_sec - t_s.tv_sec );
  }
  title--;

  Log3(ctx,"Found %d VTS's", title );
  gettimeofday(&all_e, NULL);
  Log3(ctx,"Elapsed time %ld", (long int) all_e.tv_sec - all_s.tv_sec );

  return 0;
}



/**
 * Open a DVD image or block device file or use stream_cb functions.
 */
static dvd_reader_device_t *DVDOpenImageFile( dvd_reader_t *ctx,
                                              const char *location,
                                        dvd_reader_stream_cb *stream_cb,
                                       int have_css )
{
  dvd_reader_device_t *dvd;
  dvd_input_t dev;

  dev = dvdinput_open( ctx->priv, &ctx->logcb, location, stream_cb );
  if( !dev ) {
    Log0(ctx,"Can't open %s for reading", location );
    return NULL;
  }

  dvd = calloc( 1, sizeof( dvd_reader_device_t ) );
  if( !dvd ) {
    dvdinput_close(dev);
    return NULL;
  }
  dvd->isImageFile = 1;
  dvd->dev = dev;

  dvd->udfcache_level = DEFAULT_UDF_CACHE_LEVEL;

  if( have_css ) {
    /* Only if DVDCSS_METHOD = title, a bit if it's disc or if
     * DVDCSS_METHOD = key but region mismatch. Unfortunately we
     * don't have that information. */

    dvd->css_state = 1; /* Need key init. */
  }

  return dvd;
}

static dvd_reader_device_t *DVDOpenPath( const char *path_root )
{
  dvd_reader_device_t *dvd;

  dvd = calloc( 1, sizeof( dvd_reader_device_t ) );
  if( !dvd ) return NULL;
  dvd->path_root = strdup( path_root );
  if(!dvd->path_root) {
    free(dvd);
    return NULL;
  }
  dvd->udfcache_level = DEFAULT_UDF_CACHE_LEVEL;

  return dvd;
}

#if defined(__sun)
/* /dev/rdsk/c0t6d0s0 (link to /devices/...)
   /vol/dev/rdsk/c0t6d0/??
   /vol/rdsk/<name> */
static char *sun_block2char( const char *path )
{
  char *new_path;

  /* Must contain "/dsk/" */
  if( !strstr( path, "/dsk/" ) ) return (char *) strdup( path );

  /* Replace "/dsk/" with "/rdsk/" */
  new_path = malloc( strlen(path) + 2 );
  if(!new_path) return NULL;
  strcpy( new_path, path );
  strcpy( strstr( new_path, "/dsk/" ), "" );
  strcat( new_path, "/rdsk/" );
  strcat( new_path, strstr( path, "/dsk/" ) + strlen( "/dsk/" ) );

  return new_path;
}
#endif

#if defined(SYS_BSD)
/* FreeBSD /dev/(r)(a)cd0c (a is for atapi), recommended to _not_ use r
   update: FreeBSD and DragonFly no longer uses the prefix so don't add it.
   OpenBSD /dev/rcd0c, it needs to be the raw device
   NetBSD  /dev/rcd0[d|c|..] d for x86, c (for non x86), perhaps others
   Darwin  /dev/rdisk0,  it needs to be the raw device
   BSD/OS  /dev/sr0c (if not mounted) or /dev/rsr0c ('c' any letter will do)
   returns a string allocated with strdup. It should be freed when no longer
   used. */
static char *bsd_block2char( const char *path )
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
  return (char *) strdup( path );
#else
  char *new_path;

  /* If it doesn't start with "/dev/" or does start with "/dev/r" exit */
  if( strncmp( path, "/dev/",  5 ) || !strncmp( path, "/dev/r", 6 ) )
    return (char *) strdup( path );

  /* Replace "/dev/" with "/dev/r" */
  new_path = malloc( strlen(path) + 2 );
  if(!new_path) return NULL;
  strcpy( new_path, "/dev/r" );
  strcat( new_path, path + strlen( "/dev/" ) );

  return new_path;
#endif /* __FreeBSD__ || __DragonFly__ */
}
#endif

static dvd_reader_t *DVDOpenCommon( void *priv,
                                    const dvd_logger_cb *logcb,
                                    const char *ppath,
                                    dvd_reader_stream_cb *stream_cb )
{
  dvdstat_t fileinfo;
  int ret, have_css, cdir = -1;
  char *dev_name = NULL;
  char *path = NULL, *new_path = NULL, *path_copy = NULL;
  dvd_reader_t *ctx = calloc(1, sizeof(*ctx));
  if(!ctx)
      return NULL;

  ctx->priv = priv;
  if(logcb)
    ctx->logcb = *logcb;

#if defined(_WIN32) || defined(__OS2__)
      int len;
#endif

  /* Try to open DVD using stream_cb functions */
  if( priv != NULL && stream_cb != NULL )
  {
    have_css = dvdinput_setup( ctx->priv, &ctx->logcb );
    ctx->rd = DVDOpenImageFile( ctx, NULL, stream_cb, have_css );
    if(!ctx->rd)
    {
        free(ctx);
        return NULL;
    }
    return ctx;
  }

  if( ppath == NULL )
    goto DVDOpen_error;

  path = strdup(ppath);
  if( path == NULL )
    goto DVDOpen_error;

  /* Try to open libdvdcss or fall back to standard functions */
  have_css = dvdinput_setup( ctx->priv, &ctx->logcb );

#if defined(_WIN32) || defined(__OS2__)
  /* Strip off the trailing \ if it is not a drive */
  len = strlen(path);
  if ((len > 1) &&
      (path[len - 1] == '\\')  &&
      (path[len - 2] != ':'))
  {
    path[len-1] = '\0';
  }
#endif

  ret = dvdstat( path, &fileinfo );

  if( ret < 0 ) {

    /* maybe "host:port" url? try opening it with acCeSS library */
    if( strchr(path,':') ) {
      ctx->rd = DVDOpenImageFile( ctx, path, NULL, have_css );
      free(path);
      if(!ctx->rd)
      {
          free(ctx);
          return NULL;
      }
      return ctx;
    }

    /* If we can't stat the file, give up */
    Log0(ctx, "Can't stat %s", path );
    perror("");
    goto DVDOpen_error;
  }

  /* First check if this is a block/char device or a file*/
  if( S_ISBLK( fileinfo.st_mode ) ||
      S_ISCHR( fileinfo.st_mode ) ||
      S_ISREG( fileinfo.st_mode ) ) {

    /**
     * Block devices and regular files are assumed to be DVD-Video images.
     */
#if defined(__sun)
    dev_name = sun_block2char( path );
#elif defined(SYS_BSD)
    dev_name = bsd_block2char( path );
#else
    dev_name = strdup( path );
#endif
    if(!dev_name)
        goto DVDOpen_error;
    ctx->rd = DVDOpenImageFile( ctx, dev_name, NULL, have_css );
    free( dev_name );
    free(path);
    if(!ctx->rd)
    {
        free(ctx);
        return NULL;
    }
    return ctx;
  } else if( S_ISDIR( fileinfo.st_mode ) ) {
#if defined(SYS_BSD)
    struct fstab* fe;
#elif defined(__sun) || defined(__linux__)
    FILE *mntfile;
#endif

    /* XXX: We should scream real loud here. */
    if( !(path_copy = strdup( path ) ) )
      goto DVDOpen_error;

#if !defined(WIN32) && !defined(_XBMC) /* don't have fchdir, and getcwd( NULL, ... ) is strange */
              /* Also WIN32 does not have symlinks, so we don't need this bit of code. */
              /* XBMC also doesn't need symlink resolution */

    /* Resolve any symlinks and get the absolute dir name. */
    {
        new_path = realpath( path_copy, NULL );
        if( new_path == NULL ) {
          goto DVDOpen_error;
        }
        free(path_copy);
        path_copy = new_path;
        new_path = NULL;
    }
#endif

    /**
     * If we're being asked to open a directory, check if that directory
     * is the mount point for a DVD-ROM which we can use instead.
     */

    if( strlen( path_copy ) > 1 ) {
      if( path_copy[ strlen( path_copy ) - 1 ] == '/' ) {
        path_copy[ strlen( path_copy ) - 1 ] = '\0';
      }
    }

#if defined(_WIN32) || defined(__OS2__)
    if( strlen( path_copy ) > 9 ) {
      if( !strcasecmp( &(path_copy[ strlen( path_copy ) - 9 ]),
                       "\\video_ts"))
        path_copy[ strlen( path_copy ) - (9-1) ] = '\0';
    }
#endif
    if( strlen( path_copy ) > 9 ) {
      if( !strcasecmp( &(path_copy[ strlen( path_copy ) - 9 ]),
                       "/video_ts" ) ) {
        path_copy[ strlen( path_copy ) - 9 ] = '\0';
      }
    }

    if(path_copy[0] == '\0') {
      free( path_copy );
      if( !(path_copy = strdup( "/" ) ) )
        goto DVDOpen_error;
    }

#if defined(__APPLE__)
    struct statfs s[128];
    int r = getfsstat(NULL, 0, MNT_NOWAIT);
    if (r > 0) {
        if (r > 128)
            r = 128;
        r = getfsstat(s, r * sizeof(s[0]), MNT_NOWAIT);
        int i;
        for (i=0; i<r; i++) {
            if (!strcmp(path_copy, s[i].f_mntonname)) {
                dev_name = bsd_block2char(s[i].f_mntfromname);
                Log3(ctx, "Attempting to use device %s"
                          " mounted on %s for CSS authentication",
                        dev_name,
                        s[i].f_mntonname);
                ctx->rd = DVDOpenImageFile( ctx, dev_name, NULL, have_css );
                break;
            }
        }
    }
#elif defined(SYS_BSD)
    if( ( fe = getfsfile( path_copy ) ) ) {
      dev_name = bsd_block2char( fe->fs_spec );
      Log3(ctx, "Attempting to use device %s"
               " mounted on %s for CSS authentication",
               dev_name,
               fe->fs_file );
      ctx->rd = DVDOpenImageFile( ctx, dev_name, NULL, have_css );
    }
#elif defined(__sun)
    mntfile = fopen( MNTTAB, "r" );
    if( mntfile ) {
      struct mnttab mp;
      int res;

      while( ( res = getmntent( mntfile, &mp ) ) != -1 ) {
        if( res == 0 && !strcmp( mp.mnt_mountp, path_copy ) ) {
          dev_name = sun_block2char( mp.mnt_special );
          Log3(ctx, "Attempting to use device %s"
                   " mounted on %s for CSS authentication",
                   dev_name,
                   mp.mnt_mountp );
          ctx->rd = DVDOpenImageFile( ctx, dev_name, NULL, have_css );
          break;
        }
      }
      fclose( mntfile );
    }
#elif defined(__linux__)
    mntfile = fopen( _PATH_MOUNTED, "r" );
    if( mntfile ) {

#ifdef HAVE_GETMNTENT_R
      struct mntent *me, mbuf;
      char buf [8192];
      while( ( me = getmntent_r( mntfile, &mbuf, buf, sizeof(buf) ) ) ) {
#else
      struct mntent *me;
      while( ( me = getmntent( mntfile ) ) ) {
#endif
        if( !strcmp( me->mnt_dir, path_copy ) ) {
          Log3(ctx, "Attempting to use device %s"
                   " mounted on %s for CSS authentication",
                   me->mnt_fsname,
                   me->mnt_dir );
          ctx->rd = DVDOpenImageFile( ctx, me->mnt_fsname, NULL, have_css );
          dev_name = strdup(me->mnt_fsname);
          break;
        }
      }
      fclose( mntfile );
    }
#elif defined(_WIN32) || defined(__OS2__)
#ifdef __OS2__
    /* Use DVDOpenImageFile() only if it is a drive */
    if(isalpha(path[0]) && path[1] == ':' &&
        ( !path[2] ||
          ((path[2] == '\\' || path[2] == '/') && !path[3])))
#endif
    ctx->rd = DVDOpenImageFile( ctx, path, NULL, have_css );
#endif

#if !defined(_WIN32) && !defined(__OS2__)
    if( !dev_name ) {
      Log0(ctx, "Couldn't find device name." );
    } else if( !ctx->rd ) {
      Log0(ctx, "Device %s inaccessible, "
                "CSS authentication not available.", dev_name );
    }
#else
    if( !ctx->rd ) {
        Log0(ctx, "Device %s inaccessible, "
                 "CSS authentication not available.", path );
    }
#endif

    free( dev_name );
    dev_name = NULL;
    free( path_copy );
    path_copy = NULL;

    /**
     * If we've opened a drive, just use that.
     */
    if(ctx->rd)
    {
        free(path);
        return ctx;
    }
    /**
     * Otherwise, we now try to open the directory tree instead.
     */
    ctx->rd = DVDOpenPath( path );
    free( path );
    if(!ctx->rd)
    {
        free(ctx);
        return NULL;
    }
    return ctx;
  }

DVDOpen_error:
  /* If it's none of the above, screw it. */
  Log0(ctx, "Could not open %s", path );
  free( path );
  free( path_copy );
  if ( cdir >= 0 )
    close( cdir );
  free( new_path );
  return NULL;
}

dvd_reader_t *DVDOpen( const char *ppath )
{
    return DVDOpenCommon( NULL, NULL, ppath, NULL );
}

dvd_reader_t *DVDOpenStream( void *stream,
                             dvd_reader_stream_cb *stream_cb )
{
    return DVDOpenCommon( stream, NULL, NULL, stream_cb );
}

dvd_reader_t *DVDOpen2( void *priv, const dvd_logger_cb *logcb,
                        const char *ppath )
{
    return DVDOpenCommon( priv, logcb, ppath, NULL );
}

dvd_reader_t *DVDOpenStream2( void *priv, const dvd_logger_cb *logcb,
                              dvd_reader_stream_cb *stream_cb )
{
    return DVDOpenCommon( priv, logcb, NULL, stream_cb );
}

void DVDClose( dvd_reader_t *dvd )
{
  if( dvd ) {
    if( dvd->rd->dev ) dvdinput_close( dvd->rd->dev );
    if( dvd->rd->path_root ) free( dvd->rd->path_root );
    if( dvd->rd->udfcache ) FreeUDFCache( dvd->rd->udfcache );
    free( dvd->rd );
    free( dvd );
  }
}

/**
 * Open an unencrypted file on a DVD image file.
 */
static dvd_file_t *DVDOpenFileUDF( dvd_reader_t *ctx, const char *filename,
                                   int do_cache )
{
  uint32_t start, len;
  dvd_file_t *dvd_file;

  start = UDFFindFile( ctx, filename, &len );
  if( !start ) {
    Log0(ctx, "DVDOpenFileUDF:UDFFindFile %s failed", filename );
    return NULL;
  }

  dvd_file = calloc( 1, sizeof( dvd_file_t ) );
  if( !dvd_file ) {
    Log0(ctx, "DVDOpenFileUDF:malloc failed" );
    return NULL;
  }
  dvd_file->ctx = ctx;
  dvd_file->lb_start = start;
  dvd_file->filesize = len / DVD_VIDEO_LB_LEN;

  /* Read the whole file in cache (unencrypted) if asked and if it doesn't
   * exceed 128KB */
  if( do_cache && len < 64 * DVD_VIDEO_LB_LEN ) {
    int ret;

    dvd_file->cache = malloc( len );
    if( !dvd_file->cache )
        return dvd_file;

    ret = InternalUDFReadBlocksRaw( ctx, dvd_file->lb_start,
                                    dvd_file->filesize, dvd_file->cache,
                                    DVDINPUT_NOFLAGS );
    if( ret != dvd_file->filesize ) {
        free( dvd_file->cache );
        dvd_file->cache = NULL;
    }
  }

  return dvd_file;
}

/**
 * Searches for <file> in directory <path>, ignoring case.
 * Returns 0 and full filename in <filename>.
 *     or -1 on file not found.
 *     or -2 on path not found.
 */
static int findDirFile( const char *path, const char *file, char *filename )
{
#if defined(_XBMC)
  struct stat fileinfo;

	// no emulated opendir function in xbmc, so we'll
	// check if the file exists by stat'ing it ...
  sprintf(filename, "%s%s%s", path, ((path[strlen(path) - 1] == '/') ? "" : "/"), file);

  if (stat(filename, &fileinfo) == 0) return 0;

#else
  DIR *dir;
  struct dirent *ent;

  dir = opendir( path );
  if( !dir ) return -2;

  while( ( ent = readdir( dir ) ) != NULL ) {
    if( !strcasecmp( ent->d_name, file ) ) {
      sprintf( filename, "%s%s%s", path,
               ( ( path[ strlen( path ) - 1 ] == '/' ) ? "" : "/" ),
               ent->d_name );
      closedir(dir);
      return 0;
    }
  }
  closedir(dir);
#endif // _XBMC
  return -1;
}

static int findDVDFile( dvd_reader_t *dvd, const char *file, char *filename )
{
  const char *nodirfile;
  int ret;

  /* Strip off the directory for our search */
  if( !strncasecmp( "/VIDEO_TS/", file, 10 ) ) {
    nodirfile = &(file[ 10 ]);
  } else {
    nodirfile = file;
  }

  ret = findDirFile( dvd->rd->path_root, nodirfile, filename );
  if( ret < 0 ) {
    char video_path[ PATH_MAX + 1 ];

    /* Try also with adding the path, just in case. */
    sprintf( video_path, "%s/VIDEO_TS/", dvd->rd->path_root );
    ret = findDirFile( video_path, nodirfile, filename );
    if( ret < 0 ) {
      /* Try with the path, but in lower case. */
      sprintf( video_path, "%s/video_ts/", dvd->rd->path_root );
      ret = findDirFile( video_path, nodirfile, filename );
      if( ret < 0 ) {
        return 0;
      }
    }
  }

  return 1;
}

/**
 * Open an unencrypted file from a DVD directory tree.
 */
static dvd_file_t *DVDOpenFilePath( dvd_reader_t *ctx, const char *filename )
{
  char full_path[ PATH_MAX + 1 ];
  dvd_file_t *dvd_file;
  dvdstat_t fileinfo;
  dvd_input_t dev;

  /* Get the full path of the file. */
  if( !findDVDFile( ctx, filename, full_path ) ) {
    Log0(ctx, "DVDOpenFilePath:findDVDFile %s failed", filename );
    return NULL;
  }

  dev = dvdinput_open( ctx->priv, &ctx->logcb, full_path, NULL );
  if( !dev ) {
    Log0(ctx, "DVDOpenFilePath:dvdinput_open %s failed", full_path );
    return NULL;
  }

  dvd_file = calloc( 1, sizeof( dvd_file_t ) );
  if( !dvd_file ) {
    Log0(ctx, "DVDOpenFilePath:dvd_file malloc failed" );
    dvdinput_close(dev);
    return NULL;
  }
  dvd_file->ctx = ctx;

  if( dvdstat( full_path, &fileinfo ) < 0 ) {
    Log0(ctx, "Can't stat() %s.", filename );
    free( dvd_file );
    dvdinput_close( dev );
    return NULL;
  }
  dvd_file->title_sizes[ 0 ] = fileinfo.st_size / DVD_VIDEO_LB_LEN;
  dvd_file->title_devs[ 0 ] = dev;
  dvd_file->filesize = dvd_file->title_sizes[ 0 ];

  return dvd_file;
}

static dvd_file_t *DVDOpenVOBUDF( dvd_reader_t *ctx, int title, int menu )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint32_t start, len;
  dvd_file_t *dvd_file;

  if( title == 0 ) {
    strcpy( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
  } else {
    sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, menu ? 0 : 1 );
  }
  start = UDFFindFile( ctx, filename, &len );
  if( start == 0 ) return NULL;

  dvd_file = calloc( 1, sizeof( dvd_file_t ) );
  if( !dvd_file ) return NULL;
  dvd_file->ctx = ctx;
  /*Hack*/ dvd_file->css_title = title << 1 | menu;
  dvd_file->lb_start = start;
  dvd_file->filesize = len / DVD_VIDEO_LB_LEN;

  /* Calculate the complete file size for every file in the VOBS */
  if( !menu ) {
    int cur;

    for( cur = 2; cur < 10; cur++ ) {
      sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, cur );
      if( !UDFFindFile( ctx, filename, &len ) ) break;
      dvd_file->filesize += len / DVD_VIDEO_LB_LEN;
    }
  }

  if( ctx->rd->css_state == 1 /* Need key init */ ) {
    initAllCSSKeys( ctx );
    ctx->rd->css_state = 2;
  }
  /*
  if( dvdinput_title( dvd_file->dvd->dev, (int)start ) < 0 ) {
      Log0(ctx, "Error cracking CSS key for %s", filename );
  }
  */

  return dvd_file;
}

static dvd_file_t *DVDOpenVOBPath( dvd_reader_t *ctx, int title, int menu )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  char full_path[ PATH_MAX + 1 ];
  dvdstat_t fileinfo;
  dvd_file_t *dvd_file;

  dvd_file = calloc( 1, sizeof( dvd_file_t ) );
  if( !dvd_file ) return NULL;
  dvd_file->ctx = ctx;
  /*Hack*/ dvd_file->css_title = title << 1 | menu;

  if( menu ) {
    dvd_input_t dev;

    if( title == 0 ) {
      strcpy( filename, "VIDEO_TS.VOB" );
    } else {
      sprintf( filename, "VTS_%02i_0.VOB", title );
    }
    if( !findDVDFile( ctx, filename, full_path ) ) {
      free( dvd_file );
      return NULL;
    }

    dev = dvdinput_open( ctx->priv, &ctx->logcb, full_path, NULL );
    if( dev == NULL ) {
      free( dvd_file );
      return NULL;
    }

    if( dvdstat( full_path, &fileinfo ) < 0 ) {
      Log0(ctx, "Can't stat() %s.", filename );
      dvdinput_close(dev);
      free( dvd_file );
      return NULL;
    }
    dvd_file->title_sizes[ 0 ] = fileinfo.st_size / DVD_VIDEO_LB_LEN;
    dvd_file->title_devs[ 0 ] = dev;
    dvdinput_title( dvd_file->title_devs[0], 0);
    dvd_file->filesize = dvd_file->title_sizes[ 0 ];

  } else {
    int i;

    for( i = 0; i < TITLES_MAX; ++i ) {

      sprintf( filename, "VTS_%02i_%i.VOB", title, i + 1 );
      if( !findDVDFile( ctx, filename, full_path ) ) {
        break;
      }

      if( dvdstat( full_path, &fileinfo ) < 0 ) {
        Log0(ctx, "Can't stat() %s.", filename );
        break;
      }

      dvd_file->title_sizes[ i ] = fileinfo.st_size / DVD_VIDEO_LB_LEN;
      dvd_file->title_devs[ i ] = dvdinput_open( ctx->priv, &ctx->logcb, full_path, NULL );
      dvdinput_title( dvd_file->title_devs[ i ], 0 );
      dvd_file->filesize += dvd_file->title_sizes[ i ];
    }
    if( !dvd_file->title_devs[ 0 ] ) {
      free( dvd_file );
      return NULL;
    }
  }

  return dvd_file;
}

dvd_file_t *DVDOpenFile( dvd_reader_t *ctx, int titlenum,
                         dvd_read_domain_t domain )
{
  dvd_reader_device_t *dvd = ctx->rd;
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  int do_cache = 0;

  /* Check arguments. */
  if( dvd == NULL || titlenum < 0 )
    return NULL;

  switch( domain ) {
  case DVD_READ_INFO_FILE:
    if( titlenum == 0 ) {
      strcpy( filename, "/VIDEO_TS/VIDEO_TS.IFO" );
    } else {
      sprintf( filename, "/VIDEO_TS/VTS_%02i_0.IFO", titlenum );
    }
    do_cache = 1;
    break;
  case DVD_READ_INFO_BACKUP_FILE:
    if( titlenum == 0 ) {
      strcpy( filename, "/VIDEO_TS/VIDEO_TS.BUP" );
    } else {
      sprintf( filename, "/VIDEO_TS/VTS_%02i_0.BUP", titlenum );
    }
    do_cache = 1;
    break;
  case DVD_READ_MENU_VOBS:
    if( dvd->isImageFile ) {
      return DVDOpenVOBUDF( ctx, titlenum, 1 );
    } else {
      return DVDOpenVOBPath( ctx, titlenum, 1 );
    }
    break;
  case DVD_READ_TITLE_VOBS:
    if( titlenum == 0 ) return NULL;
    if( dvd->isImageFile ) {
      return DVDOpenVOBUDF( ctx, titlenum, 0 );
    } else {
      return DVDOpenVOBPath( ctx, titlenum, 0 );
    }
    break;
  default:
    Log1(ctx, "Invalid domain for file open." );
    return NULL;
  }

  if( dvd->isImageFile ) {
    return DVDOpenFileUDF( ctx, filename, do_cache );
  } else {
    return DVDOpenFilePath( ctx, filename );
  }
}

void DVDCloseFile( dvd_file_t *dvd_file )
{
  dvd_reader_device_t *dvd = dvd_file->ctx->rd;
  if( dvd_file && dvd ) {
    if( !dvd->isImageFile ) {
      int i;

      for( i = 0; i < TITLES_MAX; ++i ) {
        if( dvd_file->title_devs[ i ] ) {
          dvdinput_close( dvd_file->title_devs[i] );
        }
      }
    }

    free( dvd_file->cache );
    free( dvd_file );
    dvd_file = NULL;
  }
}

static int DVDFileStatVOBUDF( dvd_reader_t *dvd, int title,
                              int menu, dvd_stat_t *statbuf )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint32_t size;
  off_t tot_size;
  off_t parts_size[ 9 ];
  int nr_parts = 0;
  int n;

  if( title == 0 )
    strcpy( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
  else
    sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, menu ? 0 : 1 );

  if( !UDFFindFile( dvd, filename, &size ) )
    return -1;

  tot_size = size;
  nr_parts = 1;
  parts_size[ 0 ] = size;

  if( !menu ) {
    int cur;

    for( cur = 2; cur < 10; cur++ ) {
      sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, cur );
      if( !UDFFindFile( dvd, filename, &size ) )
        break;

      parts_size[ nr_parts ] = size;
      tot_size += size;
      nr_parts++;
    }
  }

  statbuf->size = tot_size;
  statbuf->nr_parts = nr_parts;
  for( n = 0; n < nr_parts; n++ )
    statbuf->parts_size[ n ] = parts_size[ n ];

  return 0;
}


static int DVDFileStatVOBPath( dvd_reader_t *dvd, int title,
                               int menu, dvd_stat_t *statbuf )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  char full_path[ PATH_MAX + 1 ];
  dvdstat_t fileinfo;
  off_t tot_size;
  off_t parts_size[ 9 ];
  int nr_parts = 0;
  int n;

  if( title == 0 )
    strcpy( filename, "VIDEO_TS.VOB" );
  else
    sprintf( filename, "VTS_%02d_%d.VOB", title, menu ? 0 : 1 );

  if( !findDVDFile( dvd, filename, full_path ) )
    return -1;

  if( dvdstat( full_path, &fileinfo ) < 0 ) {
    Log1(dvd, "Can't stat() %s.", filename );
    return -1;
  }

  tot_size = fileinfo.st_size;
  nr_parts = 1;
  parts_size[ 0 ] = fileinfo.st_size;

  if( !menu ) {
    int cur;
    for( cur = 2; cur < 10; cur++ ) {
      sprintf( filename, "VTS_%02d_%d.VOB", title, cur );
      if( !findDVDFile( dvd, filename, full_path ) )
        break;

      if( dvdstat( full_path, &fileinfo ) < 0 ) {
        Log1(dvd, "Can't stat() %s.", filename );
        break;
      }

      parts_size[ nr_parts ] = fileinfo.st_size;
      tot_size += parts_size[ nr_parts ];
      nr_parts++;
    }
  }

  statbuf->size = tot_size;
  statbuf->nr_parts = nr_parts;
  for( n = 0; n < nr_parts; n++ )
    statbuf->parts_size[ n ] = parts_size[ n ];

  return 0;
}


int DVDFileStat( dvd_reader_t *reader, int titlenum,
                 dvd_read_domain_t domain, dvd_stat_t *statbuf )
{
  dvd_reader_device_t *dvd = reader->rd;
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  dvdstat_t fileinfo;
  uint32_t size;

  /* Check arguments. */
  if( dvd == NULL || titlenum < 0 ) {
    errno = EINVAL;
    return -1;
  }

  switch( domain ) {
  case DVD_READ_INFO_FILE:
    if( titlenum == 0 )
      strcpy( filename, "/VIDEO_TS/VIDEO_TS.IFO" );
    else
      sprintf( filename, "/VIDEO_TS/VTS_%02i_0.IFO", titlenum );

    break;
  case DVD_READ_INFO_BACKUP_FILE:
    if( titlenum == 0 )
      strcpy( filename, "/VIDEO_TS/VIDEO_TS.BUP" );
    else
      sprintf( filename, "/VIDEO_TS/VTS_%02i_0.BUP", titlenum );

    break;
  case DVD_READ_MENU_VOBS:
    if( dvd->isImageFile )
      return DVDFileStatVOBUDF( reader, titlenum, 1, statbuf );
    else
      return DVDFileStatVOBPath( reader, titlenum, 1, statbuf );

    break;
  case DVD_READ_TITLE_VOBS:
    if( titlenum == 0 )
      return -1;

    if( dvd->isImageFile )
      return DVDFileStatVOBUDF( reader, titlenum, 0, statbuf );
    else
      return DVDFileStatVOBPath( reader, titlenum, 0, statbuf );

    break;
  default:
    Log1(reader, "Invalid domain for file stat." );
    errno = EINVAL;
    return -1;
  }

  if( dvd->isImageFile ) {
    if( UDFFindFile( reader, filename, &size ) ) {
      statbuf->size = size;
      statbuf->nr_parts = 1;
      statbuf->parts_size[ 0 ] = size;
      return 0;
    }
  } else {
    char full_path[ PATH_MAX + 1 ];

    if( findDVDFile( reader, filename, full_path ) ) {
      if( dvdstat( full_path, &fileinfo ) < 0 )
        Log1(reader, "Can't stat() %s.", filename );
      else {
        statbuf->size = fileinfo.st_size;
        statbuf->nr_parts = 1;
        statbuf->parts_size[ 0 ] = statbuf->size;
        return 0;
      }
    }
  }
  return -1;
}

/* Internal, but used from dvd_udf.c */
int InternalUDFReadBlocksRaw( const dvd_reader_t *ctx, uint32_t lb_number,
                      size_t block_count, unsigned char *data,
                      int encrypted )
{
  int ret;

  if( !ctx->rd->dev ) {
    Log0(ctx, "Fatal error in block read." );
    return -1;
  }

  ret = dvdinput_seek( ctx->rd->dev, (int) lb_number );
  if( ret != (int) lb_number ) {
    Log1(ctx, "Can't seek to block %u", lb_number );
    return ret;
  }

  ret = dvdinput_read( ctx->rd->dev, (char *) data,
                       (int) block_count, encrypted );
  return ret;
}

/* This is using a single input and starting from 'dvd_file->lb_start' offset.
 *
 * Reads 'block_count' blocks from 'dvd_file' at block offset 'offset'
 * into the buffer located at 'data' and if 'encrypted' is set
 * descramble the data if it's encrypted.  Returning either an
 * negative error or the number of blocks read. */
static int DVDReadBlocksUDF( const dvd_file_t *dvd_file, uint32_t offset,
                             size_t block_count, unsigned char *data,
                             int encrypted )
{
  /* If the cache is present and we don't need to decrypt, use the cache to
   * feed the data */
  if( dvd_file->cache && (encrypted & DVDINPUT_READ_DECRYPT) == 0 ) {
    /* Check if we don't exceed the cache (or file) size */
    if( block_count + offset > (size_t) dvd_file->filesize )
      return 0;

    /* Copy the cache at a specified offset into data. offset and block_count
     * must be converted into bytes */
    memcpy( data, dvd_file->cache + (off_t)offset * (off_t)DVD_VIDEO_LB_LEN,
            (off_t)block_count * (off_t)DVD_VIDEO_LB_LEN );

    /* return the amount of blocks copied */
    return block_count;
  } else {
    /* use dvdinput access */
    return InternalUDFReadBlocksRaw( dvd_file->ctx, dvd_file->lb_start + offset,
                             block_count, data, encrypted );
  }
}

/* This is using possibly several inputs and starting from an offset of '0'.
 *
 * Reads 'block_count' blocks from 'dvd_file' at block offset 'offset'
 * into the buffer located at 'data' and if 'encrypted' is set
 * descramble the data if it's encrypted.  Returning either an
 * negative error or the number of blocks read. */
static int DVDReadBlocksPath( const dvd_file_t *dvd_file, unsigned int offset,
                              size_t block_count, unsigned char *data,
                              int encrypted )
{
  const dvd_reader_t *ctx = dvd_file->ctx;
  int i;
  int ret, ret2, off;

  ret = 0;
  ret2 = 0;
  for( i = 0; i < TITLES_MAX; ++i ) {
    if( !dvd_file->title_sizes[ i ] ) return 0; /* Past end of file */

    if( offset < dvd_file->title_sizes[ i ] ) {
      if( ( offset + block_count ) <= dvd_file->title_sizes[ i ] ) {
        off = dvdinput_seek( dvd_file->title_devs[ i ], (int)offset );
        if( off < 0 || off != (int)offset ) {
          Log1(ctx, "Can't seek to block %u", offset );
          return off < 0 ? off : 0;
        }
        ret = dvdinput_read( dvd_file->title_devs[ i ], data,
                             (int)block_count, encrypted );
        break;
      } else {
        size_t part1_size = dvd_file->title_sizes[ i ] - offset;
        /* FIXME: Really needs to be a while loop.
         * (This is only true if you try and read >1GB at a time) */

        /* Read part 1 */
        off = dvdinput_seek( dvd_file->title_devs[ i ], (int)offset );
        if( off < 0 || off != (int)offset ) {
          Log1(ctx, "Can't seek to block %u", offset );
          return off < 0 ? off : 0;
        }
        ret = dvdinput_read( dvd_file->title_devs[ i ], data,
                             (int)part1_size, encrypted );
        if( ret < 0 ) return ret;
        /* FIXME: This is wrong if i is the last file in the set.
         * also error from this read will not show in ret. */

        /* Does the next part exist? If not then return now. */
        if( i + 1 >= TITLES_MAX || !dvd_file->title_devs[ i + 1 ] )
          return ret;

        /* Read part 2 */
        off = dvdinput_seek( dvd_file->title_devs[ i + 1 ], 0 );
        if( off < 0 || off != 0 ) {
          Log1(ctx, "Can't seek to block %d", 0 );
          return off < 0 ? off : 0;
        }
        ret2 = dvdinput_read( dvd_file->title_devs[ i + 1 ],
                              data + ( part1_size
                                       * (int64_t)DVD_VIDEO_LB_LEN ),
                              (int)(block_count - part1_size),
                              encrypted );
        if( ret2 < 0 ) return ret2;
        break;
      }
    } else {
      offset -= dvd_file->title_sizes[ i ];
    }
  }

  return ret + ret2;
}

/* This is broken reading more than 2Gb at a time is ssize_t is 32-bit. */
ssize_t DVDReadBlocks( dvd_file_t *dvd_file, int offset,
                       size_t block_count, unsigned char *data )
{
  dvd_reader_t *ctx = dvd_file->ctx;
  dvd_reader_device_t *dvd = ctx->rd;
  int ret;

  /* Check arguments. */
  if( dvd_file == NULL || offset < 0 || data == NULL )
    return -1;

  /* Hack, and it will still fail for multiple opens in a threaded app ! */
  if( dvd->css_title != dvd_file->css_title ) {
      dvd->css_title = dvd_file->css_title;
    if( dvd->isImageFile ) {
      dvdinput_title( dvd->dev, (int)dvd_file->lb_start );
    }
    /* Here each vobu has it's own dvdcss handle, so no need to update
    else {
      dvdinput_title( dvd_file->title_devs[ 0 ], (int)dvd_file->lb_start );
    }*/
  }

  if( dvd->isImageFile ) {
    ret = DVDReadBlocksUDF( dvd_file, (uint32_t)offset,
                            block_count, data, DVDINPUT_READ_DECRYPT );
  } else {
    ret = DVDReadBlocksPath( dvd_file, (unsigned int)offset,
                             block_count, data, DVDINPUT_READ_DECRYPT );
  }

  return (ssize_t)ret;
}

int32_t DVDFileSeek( dvd_file_t *dvd_file, int32_t offset )
{
  /* Check arguments. */
  if( dvd_file == NULL || offset < 0 )
    return -1;

  if( offset > dvd_file->filesize * DVD_VIDEO_LB_LEN ) {
    return -1;
  }
  dvd_file->seek_pos = (uint32_t) offset;
  return offset;
}

int DVDFileSeekForce(dvd_file_t *dvd_file, int offset, int force_size)
{
  dvd_reader_t *ctx = dvd_file->ctx;
  dvd_reader_device_t *dvd = ctx->rd;
  /* Check arguments. */
  if( dvd_file == NULL || offset <= 0 )
      return -1;

  if( dvd->isImageFile ) {
    if( force_size < 0 )
      force_size = (offset - 1) / DVD_VIDEO_LB_LEN + 1;
    if( dvd_file->filesize < force_size ) {
      dvd_file->filesize = force_size;
      free(dvd_file->cache);
      dvd_file->cache = NULL;
      Log2(ctx, "Ignored size of file indicated in UDF.");
    }
  }

  if( offset > dvd_file->filesize * DVD_VIDEO_LB_LEN )
    return -1;

  dvd_file->seek_pos = (uint32_t) offset;
  return offset;
}

ssize_t DVDReadBytes( dvd_file_t *dvd_file, void *data, size_t byte_size )
{
  dvd_reader_t *ctx = dvd_file->ctx;
  dvd_reader_device_t *dvd = ctx->rd;
  unsigned char *secbuf_base, *secbuf;
  unsigned int numsec, seek_sector, seek_byte;
  int ret;

  /* Check arguments. */
  if( dvd_file == NULL || data == NULL || (ssize_t)byte_size < 0 )
    return -1;

  seek_sector = dvd_file->seek_pos / DVD_VIDEO_LB_LEN;
  seek_byte   = dvd_file->seek_pos % DVD_VIDEO_LB_LEN;

  numsec = ( ( seek_byte + byte_size ) / DVD_VIDEO_LB_LEN ) +
    ( ( ( seek_byte + byte_size ) % DVD_VIDEO_LB_LEN ) ? 1 : 0 );

  secbuf_base = malloc( numsec * DVD_VIDEO_LB_LEN + 2048 );
  if( !secbuf_base ) {
    Log0(ctx, "Can't allocate memory for file read" );
    return 0;
  }
  secbuf = (unsigned char *)(((uintptr_t)secbuf_base & ~((uintptr_t)2047)) + 2048);

  if( dvd->isImageFile ) {
    ret = DVDReadBlocksUDF( dvd_file, (uint32_t) seek_sector,
                            (size_t) numsec, secbuf, DVDINPUT_NOFLAGS );
  } else {
    ret = DVDReadBlocksPath( dvd_file, seek_sector,
                             (size_t) numsec, secbuf, DVDINPUT_NOFLAGS );
  }

  if( ret != (int) numsec ) {
    free( secbuf_base );
    return ret < 0 ? ret : 0;
  }

  memcpy( data, &(secbuf[ seek_byte ]), byte_size );
  free( secbuf_base );

  DVDFileSeekForce(dvd_file, dvd_file->seek_pos + byte_size, -1);
  return byte_size;
}

ssize_t DVDFileSize( dvd_file_t *dvd_file )
{
  /* Check arguments. */
  if( dvd_file == NULL )
    return -1;

  return dvd_file->filesize;
}

int DVDDiscID( dvd_reader_t *dvd, unsigned char *discid )
{
  struct md5_s ctx;
  int title;
  int title_sets;
  int nr_of_files = 0;
  ifo_handle_t *vmg_ifo;

  /* Check arguments. */
  if( dvd == NULL || discid == NULL )
    return 0;

  vmg_ifo = ifoOpen( dvd, 0 );
  if( !vmg_ifo ) {
    Log0(dvd, "DVDDiscId, failed to open VMG IFO" );
    return -1;
  }

  title_sets = vmg_ifo->vmgi_mat->vmg_nr_of_title_sets + 1;
  ifoClose( vmg_ifo );

  if( title_sets > 10 )
  	title_sets = 10;

  /* Go through the first IFO:s, in order, up until the tenth,
   * and md5sum them, i.e  VIDEO_TS.IFO and VTS_0?_0.IFO */
  InitMD5( &ctx );
  for( title = 0; title < title_sets; title++ ) {
    dvd_file_t *dvd_file = DVDOpenFile( dvd, title, DVD_READ_INFO_FILE );
    if( dvd_file != NULL ) {
      ssize_t bytes_read;
      ssize_t file_size = dvd_file->filesize * DVD_VIDEO_LB_LEN;
      char *buffer_base = malloc( file_size + 2048 );

      if( buffer_base == NULL ) {
          DVDCloseFile( dvd_file );
          Log0(dvd, "DVDDiscId, failed to allocate memory for file read" );
          return -1;
      }

      char *buffer = (char *)(((uintptr_t)buffer_base & ~((uintptr_t)2047)) + 2048);

      bytes_read = DVDReadBytes( dvd_file, buffer, file_size );
      if( bytes_read != file_size ) {
          Log1(dvd, "DVDDiscId read returned %zd bytes"
                   ", wanted %zd", bytes_read, file_size );
          DVDCloseFile( dvd_file );
          free( buffer_base );
          return -1;
      }

      AddMD5( &ctx, buffer, file_size );

      DVDCloseFile( dvd_file );
      free( buffer_base );
      nr_of_files++;
    }
  }
  EndMD5( &ctx );
  memcpy( discid, ctx.buf, 16 );
  if(!nr_of_files)
    return -1;

  return 0;
}


int DVDISOVolumeInfo( dvd_reader_t *ctx,
                      char *volid, unsigned int volid_size,
                      unsigned char *volsetid, unsigned int volsetid_size )
{
  dvd_reader_device_t *dvd = ctx->rd;
  unsigned char *buffer, *buffer_base;
  int ret;

  /* Check arguments. */
  if( dvd == NULL )
    return 0;

  if( dvd->dev == NULL ) {
    /* No block access, so no ISO... */
    return -1;
  }

  buffer_base = malloc( DVD_VIDEO_LB_LEN + 2048 );

  if( buffer_base == NULL ) {
    Log0(ctx, "DVDISOVolumeInfo, failed to "
             "allocate memory for file read" );
    return -1;
  }

  buffer = (unsigned char *)(((uintptr_t)buffer_base & ~((uintptr_t)2047)) + 2048);

  ret = InternalUDFReadBlocksRaw( ctx, 16, 1, buffer, 0 );
  if( ret != 1 ) {
    Log0(ctx, "DVDISOVolumeInfo, failed to "
             "read ISO9660 Primary Volume Descriptor" );
    free( buffer_base );
    return -1;
  }

  if( (volid != NULL) && (volid_size > 0) ) {
    unsigned int n;
    for(n = 0; n < 32; n++) {
      if(buffer[40+n] == 0x20) {
        break;
      }
    }

    if(volid_size > n+1) {
      volid_size = n+1;
    }

    memcpy(volid, &buffer[40], volid_size-1);
    volid[volid_size-1] = '\0';
  }

  if( (volsetid != NULL) && (volsetid_size > 0) ) {
    if(volsetid_size > 128) {
      volsetid_size = 128;
    }
    memcpy(volsetid, &buffer[190], volsetid_size);
  }
  free( buffer_base );
  return 0;
}


int DVDUDFVolumeInfo( dvd_reader_t *ctx,
                      char *volid, unsigned int volid_size,
                      unsigned char *volsetid, unsigned int volsetid_size )
{
  int ret;
  /* Check arguments. */
  if( ctx == NULL || ctx->rd == NULL )
    return -1;

  if( ctx->rd->dev == NULL ) {
    /* No block access, so no UDF VolumeSet Identifier */
    return -1;
  }

  if( (volid != NULL) && (volid_size > 0) ) {
    ret = UDFGetVolumeIdentifier(ctx, volid, volid_size);
    if(!ret) {
      return -1;
    }
  }
  if( (volsetid != NULL) && (volsetid_size > 0) ) {
    ret =  UDFGetVolumeSetIdentifier(ctx, volsetid, volsetid_size);
    if(!ret) {
      return -1;
    }
  }

  return 0;
}
