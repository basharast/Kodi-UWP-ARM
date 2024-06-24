/*
 * Copyright (C) 2000, 2001 Håkan Hjort
 * Copyright (C) 2001 Rich Wareham <richwareham@users.sourceforge.net>
 *               2002-2004 the dvdnav project
 *
 * This file is part of libdvdnav, a DVD navigation library. It is modified
 * from a file originally part of the Ogle DVD player.
 *
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdnav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdint.h>

#include <dvdread/nav_types.h>
#include <dvdread/ifo_read.h>
#include "dvdnav/dvdnav.h"

#include "decoder.h"
#include "vm.h"
#include "play.h"
#include "getset.h"
#include "dvdnav_internal.h"
#include "logger.h"

#ifdef _MSC_VER
#include <io.h>   /* read() */
#endif /* _MSC_VER */

#ifdef __OS2__
#define INCL_DOS
#include <os2safe.h>
#include <os2.h>
#include <io.h>     /* setmode() */
#include <fcntl.h>  /* O_BINARY  */
#endif

#if DVDREAD_VERSION >= DVDREAD_VERSION_CODE(6,1,0)
static void dvd_reader_logger_handler( void *priv, dvd_logger_level_t level,
                                       const char *fmt, va_list list )
{
    vm_t *vm = priv;
    if(vm->logcb.pf_log)
        vm->logcb.pf_log(vm->priv, (dvdnav_logger_level_t) level, fmt, list );
}
#endif

static int dvd_reader_seek_handler(void *priv, uint64_t pos)
{
    vm_t *vm = priv;
    if(vm->streamcb.pf_seek)
        return vm->streamcb.pf_seek(vm->priv, pos);
    return 1;
}

static int dvd_reader_read_handler(void *priv, void *buffer, int count)
{
    vm_t *vm = priv;
    if(vm->streamcb.pf_read)
        return vm->streamcb.pf_read(vm->priv, buffer, count);
    return 1;
}

static int dvd_reader_readv_handler(void *priv, void *iovec, int count)
{
    vm_t *vm = priv;
    if(vm->streamcb.pf_readv)
        return vm->streamcb.pf_readv(vm->priv, iovec, count);
    return 1;
}

/*
#define DVDNAV_STRICT
*/

/* Local prototypes */

/* Process link - returns 1 if a hop has been performed */
static int process_command(vm_t *vm,link_t link_values);

/* Helper functions */
static void vm_close(vm_t *vm);

/* Debug functions */

#ifdef TRACE
void vm_position_print(vm_t *vm, vm_position_t *position) {
  Log3(vm, "But=%x Spu=%x Aud=%x Ang=%x Hop=%x vts=%x dom=%x cell=%x cell_restart=%x cell_start=%x still=%x block=%x",
  position->button,
  position->spu_channel,
  position->audio_channel,
  position->angle_channel,
  position->hop_channel,
  position->vts,
  position->domain,
  position->cell,
  position->cell_restart,
  position->cell_start,
  position->still,
  position->block);
}

static void vm_print_current_domain_state(vm_t *vm) {
  const char *domain;

  switch(vm->state.domain) {
    case DVD_DOMAIN_VTSTitle:  domain = "Video Title";        break;
    case DVD_DOMAIN_VTSMenu: domain = "Video Title Menu";   break;
    case DVD_DOMAIN_VMGM: domain = "Video Manager Menu"; break;
    case DVD_DOMAIN_FirstPlay:   domain = "First Play";         break;
    default:          domain = "Unknown";            break;
  }
  Log3(vm, "%s Domain: VTS:%d PGC:%d PG:%u CELL:%u BLOCK:%u VTS_TTN:%u TTN:%u TT_PGCN:%u",
                   domain,
                   vm->state.vtsN,
                   get_PGCN(vm),
                   vm->state.pgN,
                   vm->state.cellN,
                   vm->state.blockN,
                   vm->state.VTS_TTN_REG,
                   vm->state.TTN_REG,
                   vm->state.TT_PGCN_REG);
}
#endif

#ifdef __OS2__
#define open os2_open

static int os2_open(const char *name, int oflag)
{
  HFILE hfile;
  ULONG ulAction;
  ULONG rc;

  rc = DosOpenL(name, &hfile, &ulAction, 0, FILE_NORMAL,
                OPEN_ACTION_OPEN_IF_EXISTS | OPEN_ACTION_FAIL_IF_NEW,
                OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE | OPEN_FLAGS_DASD,
                NULL);

  if(rc)
    return -1;

  setmode(hfile, O_BINARY);

  return (int)hfile;
}
#endif

static void escaped_strcpy(char *dst, const char *src, size_t len)
{
    for(size_t i=0; i < len; i++ )
    {
      if(src[i] == 0)
      {
          dst[i] = 0;
          break;
      }
      else if(isprint(src[i]))
      {
        dst[i] = src[i];
      } else {
        dst[i] = ' ';
      }
    }
}

static int dvd_read_name(const vm_t *vm, char *name, char *serial, const char *device) {
  /* Because we are compiling with _FILE_OFFSET_BITS=64
   * all off_t are 64bit.
   */
  off_t off;
  ssize_t read_size = 0;
  int fd = -1;
  uint8_t data[DVD_VIDEO_LB_LEN];

  /* Read DVD name */
  if (device == NULL) {
    Log0(vm, "Device name string NULL");
    goto fail;
  }
  if ((fd = open(device, O_RDONLY)) == -1) {
    Log0(vm, "Unable to open device file %s.", device);
    goto fail;
  }

  if ((off = lseek( fd, 32 * (off_t) DVD_VIDEO_LB_LEN, SEEK_SET )) == (off_t) - 1) {
    Log0(vm, "Unable to seek to the title block %u.", 32);
    goto fail;
  }

  if( off != ( 32 * (off_t) DVD_VIDEO_LB_LEN ) ) {
    Log0(vm, "Can't seek to block %u", 32 );
    goto fail;
  }

  if ((read_size = read( fd, data, DVD_VIDEO_LB_LEN )) == -1) {
    Log0(vm, "Can't read name block. Probably not a DVD-ROM device.");
    goto fail;
  }

  close(fd);
  fd = -1;
  if (read_size != DVD_VIDEO_LB_LEN) {
    Log0(vm, "Can't read name block. Probably not a DVD-ROM device.");
    goto fail;
  }

  char buffer[49] = {0};
  strncpy(name, (const char*) &data[25], 48);
  name[48] = 0;
  escaped_strcpy(buffer, name, 48);
  Log2(vm, "DVD Title: %s", buffer);

  strncpy(serial, (const char*) &data[73], 14);
  serial[14] = 0;
  escaped_strcpy(buffer, serial, 14);
  Log2(vm, "DVD Serial Number: %s", buffer);

  escaped_strcpy(buffer, (const char *) &data[89], 128 - 89);
  Log2(vm, "DVD Title (Alternative): %s", buffer);

  return 1;

fail:
  if (fd >= 0)
    close(fd);

  return 0;
}

int ifoOpenNewVTSI(vm_t *vm, dvd_reader_t *dvd, int vtsN) {
  if(vm->state.vtsN == vtsN) {
    return 1; /*  We already have it */
  }

  if(vm->vtsi != NULL)
    ifoClose(vm->vtsi);

  vm->vtsi = ifoOpenVTSI(dvd, vtsN);
  if(vm->vtsi == NULL) {
    Log0(vm, "ifoOpenVTSI failed");
    return 0;
  }
  if(!ifoRead_VTS_PTT_SRPT(vm->vtsi)) {
    Log0(vm, "ifoRead_VTS_PTT_SRPT failed");
    return 0;
  }
  if(!ifoRead_PGCIT(vm->vtsi)) {
    Log0(vm, "ifoRead_PGCIT failed");
    return 0;
  }
  if(!ifoRead_PGCI_UT(vm->vtsi)) {
    Log0(vm, "ifoRead_PGCI_UT failed");
    return 0;
  }
  if(!ifoRead_VOBU_ADMAP(vm->vtsi)) {
    Log0(vm, "ifoRead_VOBU_ADMAP vtsi failed");
    return 0;
  }
  if(!ifoRead_TITLE_VOBU_ADMAP(vm->vtsi)) {
    Log0(vm, "ifoRead_TITLE_VOBU_ADMAP vtsi failed");
    return 0;
  }
  vm->state.vtsN = vtsN;

  return 1;
}


/* Initialisation & Destruction */

vm_t* vm_new_vm(void *priv, const dvdnav_logger_cb *logcb) {
  vm_t *vm = calloc(1, sizeof(vm_t));
  if(vm)
  {
    vm->priv = priv;
    if(logcb)
        vm->logcb = *logcb;
  }
  return vm;
}

void vm_free_vm(vm_t *vm) {
  vm_close(vm);
  free(vm);
}


/* IFO Access */

ifo_handle_t *vm_get_vmgi(vm_t *vm) {
  return vm->vmgi;
}

ifo_handle_t *vm_get_vtsi(vm_t *vm) {
  return vm->vtsi;
}


/* Reader Access */

dvd_reader_t *vm_get_dvd_reader(vm_t *vm) {
  return vm->dvd;
}


/* Basic Handling */

int vm_start(vm_t *vm) {
  if (vm->stopped) {
    if (!vm_reset(vm, NULL, NULL, NULL))
      return 0;

    vm->stopped = 0;
  }
  /* Set pgc to FP (First Play) pgc */
  set_FP_PGC(vm);
  process_command(vm, play_PGC(vm));
  return !vm->stopped;
}

void vm_stop(vm_t *vm) {
  vm->stopped = 1;
}

static void vm_close(vm_t *vm) {
  if(!vm)
    return;
  if(vm->vmgi) {
    ifoClose(vm->vmgi);
    vm->vmgi=NULL;
  }
  if(vm->vtsi) {
    ifoClose(vm->vtsi);
    vm->vtsi=NULL;
  }
  if(vm->dvd) {
    DVDClose(vm->dvd);
    vm->dvd=NULL;
  }
  vm->stopped = 1;
}

int vm_reset(vm_t *vm, const char *dvdroot,
             void *priv, dvdnav_stream_cb *stream_cb) {
  /*  Setup State */
  memset(vm->state.registers.SPRM, 0, sizeof(vm->state.registers.SPRM));
  memset(vm->state.registers.GPRM, 0, sizeof(vm->state.registers.GPRM));
  memset(vm->state.registers.GPRM_mode, 0, sizeof(vm->state.registers.GPRM_mode));
  memset(vm->state.registers.GPRM_mode, 0, sizeof(vm->state.registers.GPRM_mode));
  memset(vm->state.registers.GPRM_time, 0, sizeof(vm->state.registers.GPRM_time));
  vm->state.registers.SPRM[0]  = ('e'<<8)|'n'; /* Player Menu Language code */
  vm->state.AST_REG            = 15;           /* 15 why? */
  vm->state.SPST_REG           = 62;           /* 62 why? */
  vm->state.AGL_REG            = 1;
  vm->state.TTN_REG            = 1;
  vm->state.VTS_TTN_REG        = 1;
  /* vm->state.TT_PGCN_REG        = 0 */
  vm->state.PTTN_REG           = 1;
  vm->state.HL_BTNN_REG        = 1 << 10;
  vm->state.PTL_REG            = 15;           /* Parental Level */
  vm->state.registers.SPRM[12] = ('U'<<8)|'S'; /* Parental Management Country Code */
  vm->state.registers.SPRM[16] = ('e'<<8)|'n'; /* Initial Language Code for Audio */
  vm->state.registers.SPRM[18] = ('e'<<8)|'n'; /* Initial Language Code for Spu */
  vm->state.registers.SPRM[20] = 0x1;          /* Player Regional Code Mask. Region free! */
  vm->state.registers.SPRM[14] = 0x100;        /* Try Pan&Scan */
  vm->state.registers.SPRM[15] = 0x7CFC;       /* Audio capabilities - All defined audio types */

  vm->state.pgN                = 0;
  vm->state.cellN              = 0;
  vm->state.cell_restart       = 0;

  vm->state.domain             = DVD_DOMAIN_FirstPlay;
  vm->state.rsm_vtsN           = 0;
  vm->state.rsm_cellN          = 0;
  vm->state.rsm_blockN         = 0;

  vm->state.vtsN               = -1;

  vm->hop_channel                = 0;

  /* save target callbacks */
  if(stream_cb)
      vm->streamcb = *stream_cb;
  else
      vm->streamcb = (dvdnav_stream_cb) { NULL, NULL, NULL };

  /* bind local callbacks */
  vm->dvdstreamcb.pf_seek = vm->streamcb.pf_seek ? dvd_reader_seek_handler : NULL;
  vm->dvdstreamcb.pf_read = vm->streamcb.pf_read ? dvd_reader_read_handler : NULL;
  vm->dvdstreamcb.pf_readv = vm->streamcb.pf_readv ? dvd_reader_readv_handler : NULL;

  if (vm->dvd && (dvdroot || (priv && stream_cb))) {
    /* a new dvd device has been requested */
    vm_close(vm);
  }
  if (!vm->dvd) {
    /* dvdread stream callback handlers for redirection */
#if DVDREAD_VERSION >= DVDREAD_VERSION_CODE(6,1,0)
    dvd_logger_cb dvdread_logcb = { .pf_log = dvd_reader_logger_handler };
    /* Only install log handler if we have one ourself */
    dvd_logger_cb *p_dvdread_logcb = vm->logcb.pf_log ? &dvdread_logcb : NULL;
    if(dvdroot)
        vm->dvd = DVDOpen2(vm, p_dvdread_logcb, dvdroot);
    else if(vm->priv && vm->dvdstreamcb.pf_read)
        vm->dvd = DVDOpenStream2(vm, p_dvdread_logcb, &vm->dvdstreamcb);
#else
      if(dvdroot)
          vm->dvd = DVDOpen(dvdroot);
      else if(vm->priv && vm->dvdstreamcb.pf_read)
          vm->dvd = DVDOpenStream(vm, &vm->dvdstreamcb);
#endif
    if(!vm->dvd) {
      Log0(vm, "vm: failed to open/read the DVD");
      return 0;
    }
    vm->vmgi = ifoOpenVMGI(vm->dvd);
    if(!vm->vmgi) {
      Log0(vm, "vm: vm: failed to read VIDEO_TS.IFO");
      return 0;
    }
    if(!ifoRead_FP_PGC(vm->vmgi)) {
      Log0(vm, "vm: vm: ifoRead_FP_PGC failed");
      return 0;
    }
    if(!ifoRead_TT_SRPT(vm->vmgi)) {
      Log0(vm, "vm: vm: ifoRead_TT_SRPT failed");
      return 0;
    }
    if(!ifoRead_PGCI_UT(vm->vmgi)) {
      Log0(vm, "vm: vm: ifoRead_PGCI_UT failed");
      return 0;
    }
    if(!ifoRead_PTL_MAIT(vm->vmgi)) {
      Log0(vm, "vm: ifoRead_PTL_MAIT failed");
      /* return 0; Not really used for now.. */
    }
    if(!ifoRead_VTS_ATRT(vm->vmgi)) {
      Log0(vm, "vm: ifoRead_VTS_ATRT failed");
      /* return 0; Not really used for now.. */
    }
    if(!ifoRead_VOBU_ADMAP(vm->vmgi)) {
      Log0(vm, "vm: ifoRead_VOBU_ADMAP vgmi failed");
      /* return 0; Not really used for now.. */
    }
    /* ifoRead_TXTDT_MGI(vmgi); Not implemented yet */
    if(dvd_read_name(vm, vm->dvd_name, vm->dvd_serial, dvdroot) != 1) {
      Log1(vm, "vm: dvd_read_name failed");
    }
  }
  if (vm->vmgi) {
    int i, mask;
    char buffer[8 * 3 + 1];
    char *p = buffer;
    for (i = 1, mask = 1; i <= 8; i++, mask <<= 1)
    {
      if (((vm->vmgi->vmgi_mat->vmg_category >> 16) & mask) == 0)
      {
        sprintf(p, " %02d", i);
        p +=3;
      }
    }
    *p = 0;
    Log2(vm, "DVD disk reports itself with Region mask 0x%08x. Regions:%s",
      vm->vmgi->vmgi_mat->vmg_category, buffer);
  }
  return 1;
}


/* copying and merging */

vm_t *vm_new_copy(vm_t *source) {
  vm_t *target = vm_new_vm(source->priv, &source->logcb);
  int vtsN;
  int pgcN = get_PGCN(source);
  int pgN  = (source->state).pgN;

  if (target == NULL || pgcN == 0)
    goto fail;

  memcpy(target, source, sizeof(vm_t));

  /* open a new vtsi handle, because the copy might switch to another VTS */
  target->vtsi = NULL;
  vtsN = (target->state).vtsN;
  if (vtsN > 0) {
    (target->state).vtsN = 0;
    if (!ifoOpenNewVTSI(target, target->dvd, vtsN))
      goto fail;

    /* restore pgc pointer into the new vtsi */
    if (!set_PGCN(target, pgcN))
      goto fail;

    (target->state).pgN = pgN;
  }

  return target;

fail:
  if (target != NULL)
    vm_free_vm(target);

  return NULL;
}

void vm_merge(vm_t *target, vm_t *source) {
  if(target->vtsi)
    ifoClose(target->vtsi);
  memcpy(target, source, sizeof(vm_t));
  memset(source, 0, sizeof(vm_t));
}

void vm_free_copy(vm_t *vm) {
  if(vm->vtsi)
    ifoClose(vm->vtsi);
  free(vm);
}


/* regular playback */

void vm_position_get(vm_t *vm, vm_position_t *position) {
  position->button = vm->state.HL_BTNN_REG >> 10;
  position->vts = vm->state.vtsN;
  position->domain = vm->state.domain;
  position->spu_channel = vm->state.SPST_REG;
  position->audio_channel = vm->state.AST_REG;
  position->angle_channel = vm->state.AGL_REG;
  position->hop_channel = vm->hop_channel; /* Increases by one on each hop */
  position->cell = vm->state.cellN;
  position->cell_restart = vm->state.cell_restart;
  position->cell_start = vm->state.pgc->cell_playback[vm->state.cellN - 1].first_sector;
  position->still = vm->state.pgc->cell_playback[vm->state.cellN - 1].still_time;
  position->block = vm->state.blockN;

  /* handle PGC stills at PGC end */
  if (vm->state.cellN == vm->state.pgc->nr_of_cells)
    position->still += vm->state.pgc->still_time;
  /* still already determined */
  if (position->still)
    return;
  /* This is a rough fix for some strange still situations on some strange DVDs.
   * There are discs (like the German "Back to the Future" RC2) where the only
   * indication of a still is a cell playback time higher than the time the frames
   * in this cell actually take to play (like 1 frame with 1 minute playback time).
   * On the said BTTF disc, for these cells last_sector and last_vobu_start_sector
   * are equal and the cells are very short, so we abuse these conditions to
   * detect such discs. I consider these discs broken, so the fix is somewhat
   * broken, too. */
  if ((vm->state.pgc->cell_playback[vm->state.cellN - 1].last_sector ==
       vm->state.pgc->cell_playback[vm->state.cellN - 1].last_vobu_start_sector) &&
      (vm->state.pgc->cell_playback[vm->state.cellN - 1].last_sector -
       vm->state.pgc->cell_playback[vm->state.cellN - 1].first_sector < 1024)) {
    int time;
    int size = vm->state.pgc->cell_playback[vm->state.cellN - 1].last_sector -
               vm->state.pgc->cell_playback[vm->state.cellN - 1].first_sector;
    time  = (vm->state.pgc->cell_playback[vm->state.cellN - 1].playback_time.hour   >> 4  ) * 36000;
    time += (vm->state.pgc->cell_playback[vm->state.cellN - 1].playback_time.hour   & 0x0f) * 3600;
    time += (vm->state.pgc->cell_playback[vm->state.cellN - 1].playback_time.minute >> 4  ) * 600;
    time += (vm->state.pgc->cell_playback[vm->state.cellN - 1].playback_time.minute & 0x0f) * 60;
    time += (vm->state.pgc->cell_playback[vm->state.cellN - 1].playback_time.second >> 4  ) * 10;
    time += (vm->state.pgc->cell_playback[vm->state.cellN - 1].playback_time.second & 0x0f) * 1;
    if (!time || size / time > 30)
      /* datarate is too high, it might be a very short, but regular cell */
      return;
    if (time > 0xff) time = 0xff;
    position->still = time;
  }
}

void vm_get_next_cell(vm_t *vm) {
  process_command(vm, play_Cell_post(vm));
}


/* Jumping */

int vm_jump_pg(vm_t *vm, int pg) {
  vm->state.pgN = pg;
  process_command(vm, play_PG(vm));
  return 1;
}

int vm_jump_cell_block(vm_t *vm, int cell, int block) {
  vm->state.cellN = cell;
  process_command(vm, play_Cell(vm));
  /* play_Cell can jump to a different cell in case of angles */
  if (vm->state.cellN == cell)
    vm->state.blockN = block;
  return 1;
}

int vm_jump_title_program(vm_t *vm, int title, int pgcn, int pgn) {
  link_t link;

  if(!set_PROG(vm, title, pgcn, pgn))
    return 0;
  /* Some DVDs do not want us to jump directly into a title and have
   * PGC pre commands taking us back to some menu. Since we do not like that,
   * we do not execute PGC pre commands that would do a jump. */
  /* process_command(vm, play_PGC_PG(vm, vm->state.pgN)); */
  link = play_PGC_PG(vm, vm->state.pgN);
  if (link.command != PlayThis)
    /* jump occurred -> ignore it and play the PG anyway */
    process_command(vm, play_PG(vm));
  else
    process_command(vm, link);
  return 1;
}

int vm_jump_title_part(vm_t *vm, int title, int part) {
  link_t link;

  if(!set_PTT(vm, title, part))
    return 0;
  /* Some DVDs do not want us to jump directly into a title and have
   * PGC pre commands taking us back to some menu. Since we do not like that,
   * we do not execute PGC pre commands that would do a jump. */
  /* process_command(vm, play_PGC_PG(vm, vm->state.pgN)); */
  link = play_PGC_PG(vm, vm->state.pgN);
  if (link.command != PlayThis)
    /* jump occurred -> ignore it and play the PG anyway */
    process_command(vm, play_PG(vm));
  else
    process_command(vm, link);
  return 1;
}

int vm_jump_top_pg(vm_t *vm) {
  process_command(vm, play_PG(vm));
  return 1;
}

int vm_jump_next_pg(vm_t *vm) {
  if(vm->state.pgN >= vm->state.pgc->nr_of_programs) {
    /* last program -> move to TailPGC */
    process_command(vm, play_PGC_post(vm));
    return 1;
  } else {
    vm_jump_pg(vm, vm->state.pgN + 1);
    return 1;
  }
}

int vm_jump_prev_pg(vm_t *vm) {
  if (vm->state.pgN <= 1) {
    /* first program -> move to last program of previous PGC */
    if (vm->state.pgc->prev_pgc_nr && set_PGCN(vm, vm->state.pgc->prev_pgc_nr)) {
      process_command(vm, play_PGC(vm));
      vm_jump_pg(vm, vm->state.pgc->nr_of_programs);
      return 1;
    }
    return 0;
  } else {
    vm_jump_pg(vm, vm->state.pgN - 1);
    return 1;
  }
}

int vm_jump_up(vm_t *vm) {
  if(vm->state.pgc->goup_pgc_nr && set_PGCN(vm, vm->state.pgc->goup_pgc_nr)) {
    process_command(vm, play_PGC(vm));
    return 1;
  }
  return 0;
}

int vm_jump_menu(vm_t *vm, DVDMenuID_t menuid) {
  DVDDomain_t old_domain = vm->state.domain;

  switch (vm->state.domain) {
  case DVD_DOMAIN_FirstPlay: /* FIXME XXX $$$ What should we do here? */
    break;
  case DVD_DOMAIN_VTSTitle:
    set_RSMinfo(vm, 0, vm->state.blockN);
    /* FALL THROUGH */
  case DVD_DOMAIN_VTSMenu:
  case DVD_DOMAIN_VMGM:
    switch(menuid) {
    case DVD_MENU_Title:
    case DVD_MENU_Escape:
      if(vm->vmgi == NULL || vm->vmgi->pgci_ut == NULL)
        return 0;
      vm->state.domain = DVD_DOMAIN_VMGM;
      break;
    case DVD_MENU_Root:
    case DVD_MENU_Subpicture:
    case DVD_MENU_Audio:
    case DVD_MENU_Angle:
    case DVD_MENU_Part:
      if(vm->vtsi == NULL || vm->vtsi->pgci_ut == NULL)
        return 0;
      vm->state.domain = DVD_DOMAIN_VTSMenu;
      break;
    }
    if(get_PGCIT(vm) && set_MENU(vm, menuid)) {
      process_command(vm, play_PGC(vm));
      return 1;  /* Jump */
    } else
      vm->state.domain = old_domain;
    break;
  }

  return 0;
}

int vm_jump_resume(vm_t *vm) {
  link_t link_values = { LinkRSM, 0, 0, 0 };

  if (!vm->state.rsm_vtsN) /* Do we have resume info? */
    return 0;
  return !!process_command(vm, link_values);
}

int vm_exec_cmd(vm_t *vm, const vm_cmd_t *cmd) {
  link_t link_values;

  if(vmEval_CMD(cmd, 1, &vm->state.registers, &link_values))
    return process_command(vm, link_values);
  else
    return 0; /*  It updated some state that's all... */
}

/* link processing */

static int process_command(vm_t *vm, link_t link_values) {

  while(link_values.command != PlayThis) {

#ifdef TRACE
    Log3(vm, "Before printout starts:");
    vm_print_link(link_values);
    Log3(vm, "Link values %i %i %i %i", link_values.command,
            link_values.data1, link_values.data2, link_values.data3);
    vm_print_current_domain_state(vm);
    Log3(vm, "Before printout ends.");
#endif

    switch(link_values.command) {

    case LinkNoLink:
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      return 0;  /* no actual jump */

    case LinkTopC:
      /* Restart playing from the beginning of the current Cell. */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      link_values = play_Cell(vm);
      break;

    case LinkNextC:
      /* Link to Next Cell */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      vm->state.cellN += 1;
      link_values = play_Cell(vm);
      break;

    case LinkPrevC:
      /* Link to Previous Cell */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      assert(vm->state.cellN > 1);
      vm->state.cellN -= 1;
      link_values = play_Cell(vm);
      break;

    case LinkTopPG:
      /* Link to Top of current Program */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      link_values = play_PG(vm);
      break;

    case LinkNextPG:
      /* Link to Next Program */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      vm->state.pgN += 1;
      link_values = play_PG(vm);
      break;

    case LinkPrevPG:
      /* Link to Previous Program */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      assert(vm->state.pgN > 1);
      vm->state.pgN -= 1;
      link_values = play_PG(vm);
      break;

    case LinkTopPGC:
      /* Restart playing from beginning of current Program Chain */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      link_values = play_PGC(vm);
      break;

    case LinkNextPGC:
      /* Link to Next Program Chain */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      assert(vm->state.pgc->next_pgc_nr != 0);
      if(set_PGCN(vm, vm->state.pgc->next_pgc_nr))
        link_values = play_PGC(vm);
      else
        link_values.command = Exit;
      break;

    case LinkPrevPGC:
      /* Link to Previous Program Chain */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      assert(vm->state.pgc->prev_pgc_nr != 0);
      if(set_PGCN(vm, vm->state.pgc->prev_pgc_nr))
        link_values = play_PGC(vm);
      else
        link_values.command = Exit;
      break;

    case LinkGoUpPGC:
      /* Link to GoUp Program Chain */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      assert(vm->state.pgc->goup_pgc_nr != 0);
      if(set_PGCN(vm, vm->state.pgc->goup_pgc_nr))
        link_values = play_PGC(vm);
      else
        link_values.command = Exit;
      break;

    case LinkTailPGC:
      /* Link to Tail of Program Chain */
      /* BUTTON number:data1 */
      if(link_values.data1 != 0)
        vm->state.HL_BTNN_REG = link_values.data1 << 10;
      link_values = play_PGC_post(vm);
    break;

    case LinkRSM:
        /* Check and see if there is any rsm info!! */
        if (!vm->state.rsm_vtsN) {
          Log2(vm, "trying to resume without any resume info set");
          link_values.command = Exit;
          break;
        }

        vm->state.domain = DVD_DOMAIN_VTSTitle;
        if (!ifoOpenNewVTSI(vm, vm->dvd, vm->state.rsm_vtsN))
          assert(0);
        set_PGCN(vm, vm->state.rsm_pgcN);

        /* These should never be set in SystemSpace and/or MenuSpace */
        /* vm->state.TTN_REG = rsm_tt; ?? */
        /* vm->state.TT_PGCN_REG = vm->state.rsm_pgcN; ?? */
        int i;
        for(i = 0; i < 5; i++) {
          vm->state.registers.SPRM[4 + i] = vm->state.rsm_regs[i];
        }

        if(link_values.data1 != 0)
          vm->state.HL_BTNN_REG = link_values.data1 << 10;

        if(vm->state.rsm_cellN == 0) {
          assert(vm->state.cellN); /*  Checking if this ever happens */
          vm->state.pgN = 1;
          link_values = play_PG(vm);
        } else {
          /* vm->state.pgN = ?? this gets the right value in set_PGN() below */
          vm->state.cellN = vm->state.rsm_cellN;
          link_values.command = PlayThis;
          link_values.data1 = vm->state.rsm_blockN & 0xffff;
          link_values.data2 = vm->state.rsm_blockN >> 16;
          if(!set_PGN(vm)) {
            /* Were at the end of the PGC, should not happen for a RSM */
            assert(0);
            link_values.command = LinkTailPGC;
            link_values.data1 = 0;  /* No button */
          }
        }
      break;

    case LinkPGCN:
      /* Link to Program Chain Number:data1 */
      if(!set_PGCN(vm, link_values.data1))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case LinkPTTN:
      /* Link to Part of current Title Number:data1 */
      /* BUTTON number:data2 */
      /* PGC Pre-Commands are not executed */
      assert(vm->state.domain == DVD_DOMAIN_VTSTitle);
      if(link_values.data2 != 0)
        vm->state.HL_BTNN_REG = link_values.data2 << 10;
      if(!set_VTS_PTT(vm, vm->state.vtsN, vm->state.VTS_TTN_REG, link_values.data1))
        link_values.command = Exit;
      else
        link_values = play_PG(vm);
      break;

    case LinkPGN:
      /* Link to Program Number:data1 */
      /* BUTTON number:data2 */
      if(link_values.data2 != 0)
        vm->state.HL_BTNN_REG = link_values.data2 << 10;
      /* Update any other state, PTTN perhaps? */
      vm->state.pgN = link_values.data1;
      link_values = play_PG(vm);
      break;

    case LinkCN:
      /* Link to Cell Number:data1 */
      /* BUTTON number:data2 */
      if(link_values.data2 != 0)
        vm->state.HL_BTNN_REG = link_values.data2 << 10;
      /* Update any other state, pgN, PTTN perhaps? */
      vm->state.cellN = link_values.data1;
      link_values = play_Cell(vm);
      break;

    case Exit:
      vm->stopped = 1;
      return 0;

    case JumpTT:
      /* Jump to VTS Title Domain */
      /* Only allowed from the First Play domain(PGC) */
      /* or the Video Manager domain (VMG) */
      /* Stop SPRM9 Timer */
      /* Set SPRM1 and SPRM2 */
      assert(vm->state.domain == DVD_DOMAIN_VMGM || vm->state.domain == DVD_DOMAIN_FirstPlay); /* ?? */
      if(set_TT(vm, link_values.data1))
        link_values = play_PGC(vm);
      else
        link_values.command = Exit;
      break;

    case JumpVTS_TT:
      /* Jump to Title:data1 in same VTS Title Domain */
      /* Only allowed from the VTS Menu Domain(VTSM) */
      /* or the Video Title Set Domain(VTS) */
      /* Stop SPRM9 Timer */
      /* Set SPRM1 and SPRM2 */
      assert(vm->state.domain == DVD_DOMAIN_VTSMenu || vm->state.domain == DVD_DOMAIN_VTSTitle); /* ?? */
      if(!set_VTS_TT(vm, vm->state.vtsN, link_values.data1))
        link_values.command = Exit;
      else
        link_values = play_PGC(vm);
      break;

    case JumpVTS_PTT:
      /* Jump to Part:data2 of Title:data1 in same VTS Title Domain */
      /* Only allowed from the VTS Menu Domain(VTSM) */
      /* or the Video Title Set Domain(VTS) */
      /* Stop SPRM9 Timer */
      /* Set SPRM1 and SPRM2 */
      assert(vm->state.domain == DVD_DOMAIN_VTSMenu || vm->state.domain == DVD_DOMAIN_VTSTitle); /* ?? */
      if(!set_VTS_PTT(vm, vm->state.vtsN, link_values.data1, link_values.data2))
        link_values.command = Exit;
      else
        link_values = play_PGC_PG(vm, vm->state.pgN);
      break;

    case JumpSS_FP:
      /* Jump to First Play Domain */
      /* Only allowed from the VTS Menu Domain(VTSM) */
      /* or the Video Manager domain (VMG) */
      /* Stop SPRM9 Timer and any GPRM counters */
      assert(vm->state.domain == DVD_DOMAIN_VMGM || vm->state.domain == DVD_DOMAIN_VTSMenu); /* ?? */
      if (!set_FP_PGC(vm))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case JumpSS_VMGM_MENU:
      /* Jump to Video Manager domain - Title Menu:data1 or any PGC in VMG */
      /* Allowed from anywhere except the VTS Title domain */
      /* Stop SPRM9 Timer and any GPRM counters */
      assert(vm->state.domain != DVD_DOMAIN_VTSTitle); /* ?? */
      if(vm->vmgi == NULL || vm->vmgi->pgci_ut == NULL) {
        link_values.command = Exit;
        break;
      }
      vm->state.domain = DVD_DOMAIN_VMGM;
      if(!set_MENU(vm, link_values.data1))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case JumpSS_VTSM:
      /* Jump to a menu in Video Title domain, */
      /* or to a Menu is the current VTS */
      /* Stop SPRM9 Timer and any GPRM counters */
      /* ifoOpenNewVTSI:data1 */
      /* VTS_TTN_REG:data2 */
      /* get_MENU:data3 */
      if(link_values.data1 != 0) {
          assert(vm->state.domain == DVD_DOMAIN_VTSMenu ||
                  vm->state.domain == DVD_DOMAIN_VMGM || vm->state.domain == DVD_DOMAIN_FirstPlay); /* ?? */
        if (link_values.data1 != vm->state.vtsN) {
          /* the normal case */
          assert(vm->state.domain != DVD_DOMAIN_VTSMenu);
          if (!ifoOpenNewVTSI(vm, vm->dvd, link_values.data1))  /* Also sets vm->state.vtsN */
            vm->vtsi = NULL;
        } else {
          /* This happens on some discs like "Captain Scarlet & the Mysterons" or
           * the German RC2 of "Anatomie" in VTSM. */
        }

        if(vm->vtsi == NULL || vm->vtsi->pgci_ut == NULL) {
          link_values.command = Exit;
          break;
        }
        vm->state.domain = DVD_DOMAIN_VTSMenu;
      } else {
        /*  This happens on 'The Fifth Element' region 2. */
        assert(vm->state.domain == DVD_DOMAIN_VTSMenu);
      }
      /*  I don't know what title is supposed to be used for. */
      /*  Alien or Aliens has this != 1, I think. */
      /* assert(link_values.data2 == 1); */
      vm->state.VTS_TTN_REG = link_values.data2;
      /* TTN_REG (SPRM4), VTS_TTN_REG (SPRM5), TT_PGCN_REG (SPRM6) are linked, */
      /* so if one changes, the others must change to match it. */
      vm->state.TTN_REG     = get_TT(vm, vm->state.vtsN, vm->state.VTS_TTN_REG);
      if(!set_MENU(vm, link_values.data3))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case JumpSS_VMGM_PGC:
      /* set_PGCN:data1 */
      /* Stop SPRM9 Timer and any GPRM counters */
      assert(vm->state.domain != DVD_DOMAIN_VTSTitle); /* ?? */
      if(vm->vmgi == NULL || vm->vmgi->pgci_ut == NULL) {
        link_values.command = Exit;
        break;
      }
      vm->state.domain = DVD_DOMAIN_VMGM;
      if(!set_PGCN(vm, link_values.data1))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case CallSS_FP:
      /* set_RSMinfo:data1 */
      assert(vm->state.domain == DVD_DOMAIN_VTSTitle); /* ?? */
      /* Must be called before domain is changed */
      set_RSMinfo(vm, link_values.data1, /* We dont have block info */ 0);
      set_FP_PGC(vm);
      link_values = play_PGC(vm);
      break;

    case CallSS_VMGM_MENU:
      /* set_MENU:data1 */
      /* set_RSMinfo:data2 */
      assert(vm->state.domain == DVD_DOMAIN_VTSTitle); /* ?? */
      /* Must be called before domain is changed */
      if(vm->vmgi == NULL || vm->vmgi->pgci_ut == NULL) {
        link_values.command = Exit;
        break;
      }
      set_RSMinfo(vm, link_values.data2, /* We dont have block info */ 0);
      vm->state.domain = DVD_DOMAIN_VMGM;
      if(!set_MENU(vm, link_values.data1))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case CallSS_VTSM:
      /* set_MENU:data1 */
      /* set_RSMinfo:data2 */
      assert(vm->state.domain == DVD_DOMAIN_VTSTitle); /* ?? */
      /* Must be called before domain is changed */
      if(vm->vtsi == NULL || vm->vtsi->pgci_ut == NULL) {
        link_values.command = Exit;
        break;
      }
      set_RSMinfo(vm, link_values.data2, /* We dont have block info */ 0);
      vm->state.domain = DVD_DOMAIN_VTSMenu;
      if(!set_MENU(vm, link_values.data1))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case CallSS_VMGM_PGC:
      /* set_PGC:data1 */
      /* set_RSMinfo:data2 */
      assert(vm->state.domain == DVD_DOMAIN_VTSTitle); /* ?? */
      /* Must be called before domain is changed */
      if(vm->vmgi == NULL || vm->vmgi->pgci_ut == NULL) {
        link_values.command = Exit;
        break;
      }
      set_RSMinfo(vm, link_values.data2, /* We dont have block info */ 0);
      vm->state.domain = DVD_DOMAIN_VMGM;
      if(!set_PGCN(vm, link_values.data1))
        assert(0);
      link_values = play_PGC(vm);
      break;

    case PlayThis:
      /* Should never happen. */
      assert(0);
      break;
    }

#ifdef TRACE
    Log3(vm, "After printout starts:");
    vm_print_current_domain_state(vm);
    Log3(vm, "After printout ends.");
#endif

  }

  vm->state.blockN = link_values.data1 | (link_values.data2 << 16);
  return 1;
}

//return the ifo_handle_t describing required title, used to
//identify chapters
ifo_handle_t *vm_get_title_ifo(vm_t *vm, uint32_t title)
{
  uint8_t titleset_nr;
  if((title < 1) || (title > vm->vmgi->tt_srpt->nr_of_srpts))
    return NULL;
  titleset_nr = vm->vmgi->tt_srpt->title[title-1].title_set_nr;
  return ifoOpen(vm->dvd, titleset_nr);
}

void vm_ifo_close(ifo_handle_t *ifo)
{
  ifoClose(ifo);
}
