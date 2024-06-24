/*
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

#ifndef LIBDVDREAD_DVDREAD_INTERNAL_H
#define LIBDVDREAD_DVDREAD_INTERNAL_H

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef _WIN32
# include <unistd.h>
#endif /* _WIN32 */

#include "dvdread/dvd_reader.h"
#include "dvdread/ifo_types.h"
#include "logger.h"

#define container_of(ptr, type, member) \
    ((type *)(((char *)(ptr)) - offsetof(type, member)))

struct dvd_reader_s
{
    dvd_reader_device_t *rd;
    void *priv; /* User provided context */
    dvd_logger_cb logcb;
    /* Set 100 flags for BUP fallback, most signifiant left
       [0] for upper remaining VTS, [1] for the first Main + 63 VTS */
    uint64_t ifoBUPflags[2];
};

struct ifo_handle_private_s
{
    ifo_handle_t handle;
    dvd_reader_t *ctx;
    dvd_file_t *file;
};

enum TagIdentifier {
  /* ECMA 167 3/7.2.1 */
  PrimaryVolumeDescriptor           = 1,
  AnchorVolumeDescriptorPointer     = 2,
  VolumeDescriptorPointer           = 3,
  ImplementationUseVolumeDescriptor = 4,
  PartitionDescriptor               = 5,
  LogicalVolumeDescriptor           = 6,
  UnallocatedSpaceDescriptor        = 7,
  TerminatingDescriptor             = 8,
  LogicalVolumeIntegrityDescriptor  = 9,
  /* ECMA 167 4/7.2.1 */
  FileSetDescriptor                 = 256,
  FileIdentifierDescriptor          = 257,
  AllocationExtentDescriptor        = 258,
  IndirectEntry                     = 259,
  TerminalEntry                     = 260,
  FileEntry                         = 261,
  ExtendedAttributeHeaderDescriptor = 262,
  UnallocatedSpaceEntry             = 263,
  SpaceBitmapDescriptor             = 264,
  PartitionIntegrityEntry           = 265,
  ExtendedFileEntry                 = 266,
};

int InternalUDFReadBlocksRaw(const dvd_reader_t *, uint32_t lb_number,
                     size_t block_count, unsigned char *data, int encrypted);

void *GetUDFCacheHandle(dvd_reader_t *);
void SetUDFCacheHandle(dvd_reader_t *, void *cache);
void FreeUDFCache(void *cache);

#endif /* LIBDVDREAD_DVDREAD_INTERNAL_H */
