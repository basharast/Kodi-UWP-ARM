/*
 * This code is based on dvdudf by:
 *   Christian Wolff <scarabaeus@convergence.de>.
 *
 * Modifications by:
 *   Billy Biggs <vektor@dumbterm.net>.
 *   Björn Englund <d4bjorn@dtek.chalmers.se>.
 *
 * dvdudf: parse and read the UDF volume information of a DVD Video
 * Copyright (C) 1999 Christian Wolff for convergence integrated media
 * GmbH The author can be reached at scarabaeus@convergence.de, the
 * project's page is at http://linuxtv.org/dvd/
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>

#include "dvdread_internal.h"
#include "dvdread/dvd_reader.h"
#include "dvdread/dvd_udf.h"

/* It's required to either fail or deliver all the blocks asked for. */
static int DVDReadLBUDF( dvd_reader_t *ctx, uint32_t lb_number,
                         size_t block_count, unsigned char *data,
                         int encrypted )
{
  size_t count = block_count;

  while(count > 0) {
    int ret;

    ret = InternalUDFReadBlocksRaw(ctx, lb_number, count, data + DVD_VIDEO_LB_LEN * (block_count - count), encrypted);

    if(ret <= 0) {
      /* One of the reads failed or nothing more to read, too bad.
       * We won't even bother returning the reads that went ok. */
      return ret;
    }

    count -= (size_t)ret;
    lb_number += (uint32_t)ret;
  }

  return block_count;
}


#ifndef NULL
#define NULL ((void *)0)
#endif

struct Partition {
  int valid;
  uint16_t Flags;
  uint16_t Number;
  char Contents[32];
  uint32_t AccessType;
  uint32_t Start;
  uint32_t Length;
};

struct AD {
  uint32_t Location;
  uint32_t Length;
  uint8_t  Flags;
  uint16_t Partition;
};

struct extent_ad {
  uint32_t location;
  uint32_t length;
};

struct avdp_t {
  struct extent_ad mvds;
  struct extent_ad rvds;
};

struct pvd_t {
  uint8_t VolumeIdentifier[32];
  uint8_t VolumeSetIdentifier[128];
};

struct lbudf {
  uint32_t lb;
  uint8_t *data;
  /* needed for proper freeing */
  uint8_t *data_base;
};

struct icbmap {
  uint32_t lbn;
  struct AD file;
  uint8_t filetype;
};

struct udf_cache {
  int avdp_valid;
  struct avdp_t avdp;
  int pvd_valid;
  struct pvd_t pvd;
  int partition_valid;
  struct Partition partition;
  int rooticb_valid;
  struct AD rooticb;
  int lb_num;
  struct lbudf *lbs;
  int map_num;
  struct icbmap *maps;
};

typedef enum {
  PartitionCache, RootICBCache, LBUDFCache, MapCache, AVDPCache, PVDCache
} UDFCacheType;

void FreeUDFCache(void *cache)
{
  struct udf_cache *c = (struct udf_cache *)cache;
  if(c == NULL)
    return;

  if(c->lbs) {
    int n;
    for(n = 0; n < c->lb_num; n++)
      free(c->lbs[n].data_base);
    free(c->lbs);
  }
  if(c->maps)
    free(c->maps);
  free(c);
}


static int GetUDFCache(dvd_reader_t *ctx, UDFCacheType type,
                       uint32_t nr, void *data)
{
  int n;
  struct udf_cache *c;

  if(DVDUDFCacheLevel(ctx, -1) <= 0)
    return 0;

  c = (struct udf_cache *)GetUDFCacheHandle(ctx);

  if(c == NULL)
    return 0;

  switch(type) {
  case AVDPCache:
    if(c->avdp_valid) {
      *(struct avdp_t *)data = c->avdp;
      return 1;
    }
    break;
  case PVDCache:
    if(c->pvd_valid) {
      *(struct pvd_t *)data = c->pvd;
      return 1;
    }
    break;
  case PartitionCache:
    if(c->partition_valid) {
      *(struct Partition *)data = c->partition;
      return 1;
    }
    break;
  case RootICBCache:
    if(c->rooticb_valid) {
      *(struct AD *)data = c->rooticb;
      return 1;
    }
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
        *(uint8_t **)data = c->lbs[n].data;
        return 1;
      }
    }
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
        *(struct icbmap *)data = c->maps[n];
        return 1;
      }
    }
    break;
  default:
    break;
  }

  return 0;
}

static int SetUDFCache(dvd_reader_t *ctx, UDFCacheType type,
                       uint32_t nr, void *data)
{
  int n;
  struct udf_cache *c;
  void *tmp;

  if(DVDUDFCacheLevel(ctx, -1) <= 0)
    return 0;

  c = (struct udf_cache *)GetUDFCacheHandle(ctx);

  if(c == NULL) {
    c = calloc(1, sizeof(struct udf_cache));
    /* Log3(ctx, "calloc: %d", sizeof(struct udf_cache)); */
    if(c == NULL)
      return 0;
    SetUDFCacheHandle(ctx, c);
  }


  switch(type) {
  case AVDPCache:
    c->avdp = *(struct avdp_t *)data;
    c->avdp_valid = 1;
    break;
  case PVDCache:
    c->pvd = *(struct pvd_t *)data;
    c->pvd_valid = 1;
    break;
  case PartitionCache:
    c->partition = *(struct Partition *)data;
    c->partition_valid = 1;
    break;
  case RootICBCache:
    c->rooticb = *(struct AD *)data;
    c->rooticb_valid = 1;
    break;
  case LBUDFCache:
    for(n = 0; n < c->lb_num; n++) {
      if(c->lbs[n].lb == nr) {
        /* replace with new data */
        c->lbs[n].data_base = ((uint8_t **)data)[0];
        c->lbs[n].data = ((uint8_t **)data)[1];
        c->lbs[n].lb = nr;
        return 1;
      }
    }
    c->lb_num++;
    tmp = realloc(c->lbs, c->lb_num * sizeof(struct lbudf));
    /*
    Log3(ctx, "realloc lb: %d * %d = %d",
    c->lb_num, sizeof(struct lbudf),
    c->lb_num * sizeof(struct lbudf));
    */
    if(tmp == NULL) {
      if(c->lbs) free(c->lbs);
      c->lb_num = 0;
      return 0;
    }
    c->lbs = tmp;
    c->lbs[n].data_base = ((uint8_t **)data)[0];
    c->lbs[n].data = ((uint8_t **)data)[1];
    c->lbs[n].lb = nr;
    break;
  case MapCache:
    for(n = 0; n < c->map_num; n++) {
      if(c->maps[n].lbn == nr) {
        /* replace with new data */
        c->maps[n] = *(struct icbmap *)data;
        c->maps[n].lbn = nr;
        return 1;
      }
    }
    c->map_num++;
    tmp = realloc(c->maps, c->map_num * sizeof(struct icbmap));
    /*
    Log3(ctx, "realloc maps: %d * %d = %d\n",
      c->map_num, sizeof(struct icbmap),
      c->map_num * sizeof(struct icbmap));
    */
    if(tmp == NULL) {
      if(c->maps) free(c->maps);
      c->map_num = 0;
      return 0;
    }
    c->maps = tmp;
    c->maps[n] = *(struct icbmap *)data;
    c->maps[n].lbn = nr;
    break;
  default:
    return 0;
  }

  return 1;
}


/* For direct data access, LSB first */
#define GETN1(p) ((uint8_t)data[p])
#define GETN2(p) ((uint16_t)data[p] | ((uint16_t)data[(p) + 1] << 8))
#define GETN3(p) ((uint32_t)data[p] | ((uint32_t)data[(p) + 1] << 8)    \
                  | ((uint32_t)data[(p) + 2] << 16))
#define GETN4(p) ((uint32_t)data[p]                     \
                  | ((uint32_t)data[(p) + 1] << 8)      \
                  | ((uint32_t)data[(p) + 2] << 16)     \
                  | ((uint32_t)data[(p) + 3] << 24))
/* This is wrong with regard to endianness */
#define GETN(p, n, target) memcpy(target, &data[p], n)

static int Unicodedecode( uint8_t *data, int len, char *target )
{
  int p = 1, i = 0;
  int err = 0;

  if( ( data[ 0 ] == 8 ) || ( data[ 0 ] == 16 ) ) do {
    if( data[ 0 ] == 16 ) err |= data[p++];  /* character cannot be converted to 8bit, return error */
    if( p < len ) {
      target[ i++ ] = data[ p++ ];
    }
  } while( p < len );

  target[ i ] = '\0';
  return !err;
}

static int UDFDescriptor( uint8_t *data, uint16_t *TagID )
{
  *TagID = GETN2(0);
  /* TODO: check CRC 'n stuff */
  return 0;
}

static int UDFExtentAD( uint8_t *data, uint32_t *Length, uint32_t *Location )
{
  *Length   = GETN4(0);
  *Location = GETN4(4);
  return 0;
}

static int UDFShortAD( uint8_t *data, struct AD *ad,
                       struct Partition *partition )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(4);
  ad->Partition = partition->Number; /* use number of current partition */
  return 0;
}

static int UDFLongAD( uint8_t *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(4);
  ad->Partition = GETN2(8);
  /* GETN(10, 6, Use); */
  return 0;
}

static int UDFExtAD( uint8_t *data, struct AD *ad )
{
  ad->Length = GETN4(0);
  ad->Flags = ad->Length >> 30;
  ad->Length &= 0x3FFFFFFF;
  ad->Location = GETN4(12);
  ad->Partition = GETN2(16);
  /* GETN(10, 6, Use); */
  return 0;
}

static int UDFICB( uint8_t *data, uint8_t *FileType, uint16_t *Flags )
{
  *FileType = GETN1(11);
  *Flags = GETN2(18);
  return 0;
}


static int UDFPartition( uint8_t *data, uint16_t *Flags, uint16_t *Number,
                         char *Contents, uint32_t *Start, uint32_t *Length )
{
  *Flags = GETN2(20);
  *Number = GETN2(22);
  GETN(24, 32, Contents);
  *Start = GETN4(188);
  *Length = GETN4(192);
  return 0;
}

/**
 * Reads the volume descriptor and checks the parameters.  Returns 0 on OK, 1
 * on error.
 */
static int UDFLogVolume( uint8_t *data )
{
  uint32_t lbsize;
  lbsize = GETN4(212);  /* should be 2048 */
  /* MT_L = GETN4(264);  */  /* should be 6 */
  /* N_PM = GETN4(268);  */  /* should be 1 */
  if (lbsize != DVD_VIDEO_LB_LEN)
    return 1;

  return 0;
}

static int UDFFileEntry( uint8_t *data, uint8_t *FileType,
                         struct Partition *partition, struct AD *ad )
{
  uint16_t flags;
  uint32_t L_EA, L_AD;
  unsigned int p;
  struct AD tmpad;

  UDFICB( &data[ 16 ], FileType, &flags );

  /* Init ad for an empty file (i.e. there isn't a AD, L_AD == 0 ) */
  ad->Length = GETN4( 60 ); /* Really 8 bytes a 56 */
  ad->Flags = 0;
  ad->Location = 0; /* what should we put here?  */
  ad->Partition = partition->Number; /* use number of current partition */

  L_EA = GETN4( 168 );
  L_AD = GETN4( 172 );

  if (176 + L_EA + L_AD > DVD_VIDEO_LB_LEN)
    return 0;

  p = 176 + L_EA;
  while( p < 176 + L_EA + L_AD ) {
    switch( flags & 0x0007 ) {
    case 0:
      UDFShortAD( &data[ p ], &tmpad, partition );
      if( ad->Location == 0 ) /* We do not support more than one AD */
          *ad = tmpad;        /* This was exploited by copy protections adding empty AD */
      p += 8;
      break;
    case 1: /* FIXME ? UDF 2.60 2.3.7.1 Only Short Allocation Descriptors shall be used */
      UDFLongAD( &data[ p ], ad );
      p += 16;
      break;
    case 2:
      UDFExtAD( &data[ p ], ad );
      p += 20;
      break;
    case 3:
      switch( L_AD ) {
      case 8:
        UDFShortAD( &data[ p ], ad, partition );
        break;
      case 16:
        UDFLongAD( &data[ p ], ad );
        break;
      case 20:
        UDFExtAD( &data[ p ], ad );
        break;
      }
      p += L_AD;
      break;
    default:
      p += L_AD;
      break;
    }
  }
  return 0;
}

static int UDFFileIdentifier( uint8_t *data, uint8_t *FileCharacteristics,
                              char *FileName, struct AD *FileICB )
{
  uint8_t L_FI;
  uint16_t L_IU;

  *FileCharacteristics = GETN1(18);
  L_FI = GETN1(19);
  UDFLongAD(&data[20], FileICB);
  L_IU = GETN2(36);
  if (L_FI) {
    if (!Unicodedecode(&data[38 + L_IU], L_FI, FileName)) FileName[0] = 0;
  } else FileName[0] = '\0';
  return 4 * ((38 + L_FI + L_IU + 3) / 4);
}

/**
 * Maps ICB to FileAD
 * ICB: Location of ICB of directory to scan
 * FileType: Type of the file
 * File: Location of file the ICB is pointing to
 * return 1 on success, 0 on error;
 */
static int UDFMapICB( dvd_reader_t *ctx, struct AD ICB, uint8_t *FileType,
                      struct Partition *partition, struct AD *File )
{
  uint8_t LogBlock_base[DVD_VIDEO_LB_LEN + 2048];
  uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  struct icbmap tmpmap;
  int ret;

  lbnum = partition->Start + ICB.Location;
  tmpmap.lbn = lbnum;
  if(GetUDFCache(ctx, MapCache, lbnum, &tmpmap)) {
    *FileType = tmpmap.filetype;
    memcpy(File, &tmpmap.file, sizeof(tmpmap.file));
    return 1;
  }

  do {
    ret = DVDReadLBUDF( ctx, lbnum++, 1, LogBlock, 0 );
    if( ret < 0 ) {
      return ret;
    }
    else if( ret == 0 ) {
      TagID = 0;
    }
    else {
      UDFDescriptor( LogBlock, &TagID );
    }

    if( TagID == FileEntry ) {
      UDFFileEntry( LogBlock, FileType, partition, File );
      memcpy(&tmpmap.file, File, sizeof(tmpmap.file));
      tmpmap.filetype = *FileType;
      SetUDFCache(ctx, MapCache, tmpmap.lbn, &tmpmap);
      return 1;
    };
  } while( ( lbnum <= partition->Start + ICB.Location + ( ICB.Length - 1 )
             / DVD_VIDEO_LB_LEN ) && ( TagID != FileEntry ) );

  return 0;
}

/**
 * Dir: Location of directory to scan
 * FileName: Name of file to look for
 * FileICB: Location of ICB of the found file
 * return 1 on success, 0 on error;
 */
static int UDFScanDir( dvd_reader_t *ctx, struct AD Dir, char *FileName,
                       struct Partition *partition, struct AD *FileICB,
                       int cache_file_info)
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint8_t directory_base[ 2 * DVD_VIDEO_LB_LEN + 2048];
  uint8_t *directory = (uint8_t *)(((uintptr_t)directory_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  uint8_t filechar;
  unsigned int p;
  uint8_t *cached_dir_base = NULL, *cached_dir;
  uint32_t dir_lba;
  struct AD tmpICB;
  int ret;

  /* Scan dir for ICB of file */
  lbnum = partition->Start + Dir.Location;

  if(DVDUDFCacheLevel(ctx, -1) > 0) {
    int found = 0;
    int in_cache = 0;

    /* caching */

    if(!GetUDFCache(ctx, LBUDFCache, lbnum, &cached_dir)) {
      dir_lba = (Dir.Length + DVD_VIDEO_LB_LEN) / DVD_VIDEO_LB_LEN;
      if((cached_dir_base = malloc(dir_lba * DVD_VIDEO_LB_LEN + 2048)) == NULL)
        return 0;
      cached_dir = (uint8_t *)(((uintptr_t)cached_dir_base & ~((uintptr_t)2047)) + 2048);
      ret = DVDReadLBUDF( ctx, lbnum, dir_lba, cached_dir, 0);
      if( ret <= 0 ) {
        free(cached_dir_base);
        cached_dir_base = NULL;
        cached_dir = NULL;
        if( ret < 0 )
            return ret;
      }
      /*
      if(cached_dir) {
        Log3(ctx, "malloc dir: %d",  dir_lba * DVD_VIDEO_LB_LEN);
      }
      */
      {
        uint8_t *data[2];
        data[0] = cached_dir_base;
        data[1] = cached_dir;
        SetUDFCache(ctx, LBUDFCache, lbnum, data);
      }
    } else
      in_cache = 1;

    if(cached_dir == NULL) {
      free(cached_dir_base);
      return 0;
    }

    p = 0;

    while( p < Dir.Length ) {

      UDFDescriptor( &cached_dir[ p ], &TagID );
      if( TagID == FileIdentifierDescriptor ) {
        p += UDFFileIdentifier( &cached_dir[ p ], &filechar,
                                filename, &tmpICB );
        if(cache_file_info && !in_cache) {
          uint8_t tmpFiletype;
          struct AD tmpFile;

          if( !strcasecmp( FileName, filename ) ) {
            memcpy(FileICB, &tmpICB, sizeof(tmpICB));
            found = 1;
          }
          if(!UDFMapICB(ctx, tmpICB, &tmpFiletype, partition, &tmpFile))
            return 0;

        } else {
          if( !strcasecmp( FileName, filename ) ) {
            memcpy(FileICB, &tmpICB, sizeof(tmpICB));
            return 1;
          }
        }
      } else {
        if(cache_file_info && (!in_cache) && found)
          return 1;
        return 0;
      }
    }
    if(cache_file_info && (!in_cache) && found)
      return 1;
    return 0;
  }

  if( DVDReadLBUDF( ctx, lbnum, 2, directory, 0 ) <= 0 )
    return 0;

  p = 0;
  while( p < Dir.Length ) {
    if( p > DVD_VIDEO_LB_LEN ) {
      ++lbnum;
      p -= DVD_VIDEO_LB_LEN;
      Dir.Length -= DVD_VIDEO_LB_LEN;
      if( DVDReadLBUDF( ctx, lbnum, 2, directory, 0 ) <= 0 ) {
        return 0;
      }
    }
    UDFDescriptor( &directory[ p ], &TagID );
    if( TagID == FileIdentifierDescriptor ) {
      p += UDFFileIdentifier( &directory[ p ], &filechar,
                              filename, FileICB );
      if( !strcasecmp( FileName, filename ) ) {
        return 1;
      }
    } else
      return 0;
  }

  return 0;
}


static int UDFGetAVDP( dvd_reader_t *ctx,
                       struct avdp_t *avdp)
{
  uint8_t Anchor_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *Anchor = (uint8_t *)(((uintptr_t)Anchor_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum, MVDS_location, MVDS_length;
  uint16_t TagID;
  uint32_t lastsector;
  int terminate;
  struct avdp_t;
  int ret;

  if(GetUDFCache(ctx, AVDPCache, 0, avdp))
    return 1;

  /* Find Anchor */
  lastsector = 0;
  lbnum = 256;   /* Try #1, prime anchor */
  terminate = 0;

  for(;;) {
    ret = DVDReadLBUDF( ctx, lbnum, 1, Anchor, 0 );
    if( ret < 0 ) {
      return ret;
    }
    else if( ret == 0 ) {
      TagID = 0;
    }
    else {
      UDFDescriptor( Anchor, &TagID );
    }
    if (TagID != AnchorVolumeDescriptorPointer) {
      /* Not an anchor */
      if( terminate ) return 0; /* Final try failed */

      if( lastsector ) {
        /* We already found the last sector.  Try #3, alternative
         * backup anchor.  If that fails, don't try again.
         */
        lbnum = lastsector;
        terminate = 1;
      } else {
        /* TODO: Find last sector of the disc (this is optional). */
        if( lastsector )
          /* Try #2, backup anchor */
          lbnum = lastsector - 256;
        else
          /* Unable to find last sector */
          return 0;
      }
    } else
      /* It's an anchor! We can leave */
      break;
  }
  /* Main volume descriptor */
  UDFExtentAD( &Anchor[ 16 ], &MVDS_length, &MVDS_location );
  avdp->mvds.location = MVDS_location;
  avdp->mvds.length = MVDS_length;

  /* Backup volume descriptor */
  UDFExtentAD( &Anchor[ 24 ], &MVDS_length, &MVDS_location );
  avdp->rvds.location = MVDS_location;
  avdp->rvds.length = MVDS_length;

  SetUDFCache(ctx, AVDPCache, 0, avdp);

  return 1;
}

/**
 * Looks for partition on the disc.  Returns 1 if partition found, 0 on error.
 *   partnum: Number of the partition, starting at 0.
 *   part: structure to fill with the partition information
 */
static int UDFFindPartition( dvd_reader_t *ctx, int partnum,
                             struct Partition *part )
{
  uint8_t LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum, MVDS_location, MVDS_length;
  uint16_t TagID;
  int i, volvalid, ret;
  struct avdp_t avdp;

  if(!UDFGetAVDP(ctx, &avdp))
    return 0;

  /* Main volume descriptor */
  MVDS_location = avdp.mvds.location;
  MVDS_length = avdp.mvds.length;

  part->valid = 0;
  volvalid = 0;
  i = 1;
  do {
    /* Find Volume Descriptor */
    lbnum = MVDS_location;
    do {

      ret = DVDReadLBUDF( ctx, lbnum++, 1, LogBlock, 0 );
      if( ret < 0 ) {
        return ret;
      }
      else if( ret == 0 ) {
        TagID = 0;
      }
      else {
        UDFDescriptor( LogBlock, &TagID );
      }

      if( ( TagID == PartitionDescriptor ) && ( !part->valid ) ) {
        /* Partition Descriptor */
        UDFPartition( LogBlock, &part->Flags, &part->Number,
                      part->Contents, &part->Start, &part->Length );
        part->valid = ( partnum == part->Number );
      } else if( ( TagID == LogicalVolumeDescriptor ) && ( !volvalid ) ) {
        /* Logical Volume Descriptor */
        if( UDFLogVolume( LogBlock ) ) {
          /* TODO: sector size wrong! */
        } else
          volvalid = 1;
      }

    } while( ( lbnum <= MVDS_location + ( MVDS_length - 1 )
               / DVD_VIDEO_LB_LEN ) && ( TagID != TerminatingDescriptor )
             && ( ( !part->valid ) || ( !volvalid ) ) );

    if( ( !part->valid) || ( !volvalid ) ) {
      /* Backup volume descriptor */
      MVDS_location = avdp.mvds.location;
      MVDS_length = avdp.mvds.length;
    }
  } while( i-- && ( ( !part->valid ) || ( !volvalid ) ) );

  /* We only care for the partition, not the volume */
  return part->valid;
}

uint32_t UDFFindFile( dvd_reader_t *ctx, const char *filename,
                      uint32_t *filesize )
{
  uint8_t LogBlock_base[ DVD_VIDEO_LB_LEN + 2048 ];
  uint8_t *LogBlock = (uint8_t *)(((uintptr_t)LogBlock_base & ~((uintptr_t)2047)) + 2048);
  uint32_t lbnum;
  uint16_t TagID;
  struct Partition partition;
  struct AD RootICB, File, ICB;
  char tokenline[ MAX_UDF_FILE_NAME_LEN ];
  uint8_t filetype;
  int ret;

  *filesize = 0;
  tokenline[0] = '\0';
  strncat(tokenline, filename, MAX_UDF_FILE_NAME_LEN - 1);
  memset(&ICB, 0, sizeof(ICB));

  if(!(GetUDFCache(ctx, PartitionCache, 0, &partition) &&
       GetUDFCache(ctx, RootICBCache, 0, &RootICB))) {
    /* Find partition, 0 is the standard location for DVD Video.*/
    if( !UDFFindPartition( ctx, 0, &partition ) ) return 0;
    SetUDFCache(ctx, PartitionCache, 0, &partition);

    /* Find root dir ICB */
    lbnum = partition.Start;
    do {
      ret = DVDReadLBUDF( ctx, lbnum++, 1, LogBlock, 0 );
      if( ret < 0 ) {
        return ret;
      }
      else if( ret == 0 ) {
        TagID = 0;
      }
      else {
        UDFDescriptor( LogBlock, &TagID );
      }

      /* File Set Descriptor */
      if( TagID == FileSetDescriptor )  /* File Set Descriptor */
        UDFLongAD( &LogBlock[ 400 ], &RootICB );
    } while( ( lbnum < partition.Start + partition.Length )
             && ( TagID != TerminatingDescriptor ) && ( TagID != FileSetDescriptor) );

    /* Sanity checks. */
    if( TagID != FileSetDescriptor )
      return 0;
    if( RootICB.Partition != 0 )
      return 0;
    SetUDFCache(ctx, RootICBCache, 0, &RootICB);
  }

  /* Find root dir */
  if( !UDFMapICB( ctx, RootICB, &filetype, &partition, &File ) )
    return 0;
  if( filetype != 4 )
    return 0;  /* Root dir should be dir */
  {
    int cache_file_info = 0;
    /* Tokenize filepath */
    char *token = strtok(tokenline, "/");

    while( token != NULL ) {
      if( !UDFScanDir( ctx, File, token, &partition, &ICB,
                       cache_file_info))
        return 0;
      if( !UDFMapICB( ctx, ICB, &filetype, &partition, &File ) )
        return 0;
      if(!strcmp(token, "VIDEO_TS"))
        cache_file_info = 1;
      token = strtok( NULL, "/" );
    }
  }

  /* Sanity check. */
  if( File.Partition != 0 )
    return 0;
  *filesize = File.Length;
  /* Hack to not return partition.Start for empty files. */
  if( !File.Location )
    return 0;
  else
    return partition.Start + File.Location;
}



/**
 * Gets a Descriptor .
 * Returns 1 if descriptor found, 0 on error.
 * id, tagid of descriptor
 * bufsize, size of BlockBuf (must be >= DVD_VIDEO_LB_LEN).
 */
static int UDFGetDescriptor( dvd_reader_t *ctx, int id,
                             uint8_t *descriptor, int bufsize)
{
  uint32_t lbnum, MVDS_location, MVDS_length;
  struct avdp_t avdp;
  uint16_t TagID;
  int i, desc_found = 0, ret;
  /* Find Anchor */
  lbnum = 256;   /* Try #1, prime anchor */
  if(bufsize < DVD_VIDEO_LB_LEN)
    return 0;

  if(!UDFGetAVDP(ctx, &avdp))
    return 0;

  /* Main volume descriptor */
  MVDS_location = avdp.mvds.location;
  MVDS_length = avdp.mvds.length;

  i = 1;
  do {
    /* Find  Descriptor */
    lbnum = MVDS_location;
    do {
      ret = DVDReadLBUDF( ctx, lbnum++, 1, descriptor, 0 );
      if( ret < 0 ) {
        return ret;
      }
      else if( ret == 0 ) {
        TagID = 0;
      }
      else {
        UDFDescriptor( descriptor, &TagID );
      }
      if( (TagID == id) && ( !desc_found ) )
        /* Descriptor */
        desc_found = 1;
    } while( ( lbnum <= MVDS_location + ( MVDS_length - 1 )
               / DVD_VIDEO_LB_LEN ) && ( TagID != TerminatingDescriptor )
             && ( !desc_found) );

    if( !desc_found ) {
      /* Backup volume descriptor */
      MVDS_location = avdp.rvds.location;
      MVDS_length = avdp.rvds.length;
    }
  } while( i-- && ( !desc_found )  );

  return desc_found;
}


static int UDFGetPVD(dvd_reader_t *ctx, struct pvd_t *pvd)
{
  uint8_t pvd_buf_base[DVD_VIDEO_LB_LEN + 2048];
  uint8_t *pvd_buf = (uint8_t *)(((uintptr_t)pvd_buf_base & ~((uintptr_t)2047)) + 2048);
  if(GetUDFCache(ctx, PVDCache, 0, pvd))
    return 1;

  if(!UDFGetDescriptor( ctx, 1, pvd_buf, DVD_VIDEO_LB_LEN))
    return 0;

  memcpy(pvd->VolumeIdentifier, &pvd_buf[24], 32);
  memcpy(pvd->VolumeSetIdentifier, &pvd_buf[72], 128);
  SetUDFCache(ctx, PVDCache, 0, pvd);
  return 1;
}

/**
 * Gets the Volume Identifier string, in 8bit unicode (latin-1)
 * volid, place to put the string
 * volid_size, size of the buffer volid points to
 * returns the size of buffer needed for all data
 */
int UDFGetVolumeIdentifier(dvd_reader_t *ctx, char *volid,
                           unsigned int volid_size)
{
  struct pvd_t pvd;
  unsigned int volid_len;

  /* get primary volume descriptor */
  if(!UDFGetPVD(ctx, &pvd))
    return 0;

  volid_len = pvd.VolumeIdentifier[31];
  if(volid_len > 31)
    /* this field is only 32 bytes something is wrong */
    volid_len = 31;
  if(volid_size > volid_len)
    volid_size = volid_len;
  Unicodedecode(pvd.VolumeIdentifier, volid_size, volid);

  return volid_len;
}

/**
 * Gets the Volume Set Identifier, as a 128-byte dstring (not decoded)
 * WARNING This is not a null terminated string
 * volsetid, place to put the data
 * volsetid_size, size of the buffer volsetid points to
 * the buffer should be >=128 bytes to store the whole volumesetidentifier
 * returns the size of the available volsetid information (128)
 * or 0 on error
 */
int UDFGetVolumeSetIdentifier(dvd_reader_t *ctx, uint8_t *volsetid,
                              unsigned int volsetid_size)
{
  struct pvd_t pvd;

  /* get primary volume descriptor */
  if(!UDFGetPVD(ctx, &pvd))
    return 0;


  if(volsetid_size > 128)
    volsetid_size = 128;

  memcpy(volsetid, pvd.VolumeSetIdentifier, volsetid_size);

  return 128;
}
