/*
 * This file is part of libdvdread.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>

#include "dvdread/dvd_reader.h"
#include "logger.h"

void DVDReadLog( void *priv, const dvd_logger_cb *logcb,
                 dvd_logger_level_t level, const char *fmt, ... )
{
    va_list list;
    va_start(list, fmt);
    if(logcb && logcb->pf_log)
        logcb->pf_log(priv, level, fmt, list);
    else
    {
        FILE *stream = (level == DVD_LOGGER_LEVEL_ERROR) ? stderr : stdout;
        fprintf(stream, "libdvdread: ");
        vfprintf(stream, fmt, list);
        fprintf(stream, "\n");
    }
    va_end(list);
}
