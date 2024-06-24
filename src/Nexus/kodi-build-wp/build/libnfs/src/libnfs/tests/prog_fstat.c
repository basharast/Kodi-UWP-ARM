/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/* 
   Copyright (C) by Ronnie Sahlberg <ronniesahlberg@gmail.com> 2017
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "libnfs.h"

void usage(void)
{
	fprintf(stderr, "Usage: prog_fstat <url> <cwd> <path>\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	struct nfs_context *nfs;
	struct nfsfh *nfsfh;
	struct nfs_url *url;
	struct nfs_stat_64 st;

	if (argc != 4) {
		usage();
	}

	nfs = nfs_init_context();
	if (nfs == NULL) {
		printf("failed to init context\n");
		exit(1);
	}

	url = nfs_parse_url_full(nfs, argv[1]);
	if (url == NULL) {
		fprintf(stderr, "%s\n", nfs_get_error(nfs));
		exit(1);
	}

	if (nfs_mount(nfs, url->server, url->path) != 0) {
 		fprintf(stderr, "Failed to mount nfs share : %s\n",
			nfs_get_error(nfs));
		exit(1);
	}

	if (nfs_chdir(nfs, argv[2]) != 0) {
 		fprintf(stderr, "Failed to chdir to \"%s\" : %s\n",
			argv[2], nfs_get_error(nfs));
                exit(1);
	}

	if (nfs_open(nfs, argv[3], O_RDONLY, &nfsfh)) {
 		fprintf(stderr, "Failed to open file : %s\n",
			nfs_get_error(nfs));
		exit(1);
	}

	if (nfs_fstat64(nfs, nfsfh, &st)) {
 		fprintf(stderr, "Failed to stat file : %s\n",
			nfs_get_error(nfs));
		exit(1);
	}

	printf("nfs_ino:%" PRIu64 "\n", st.nfs_ino);
	printf("nfs_mode:%" PRIo64 "\n", st.nfs_mode);
	printf("nfs_nlink:%" PRIu64 "\n", st.nfs_nlink);
	printf("nfs_uid:%" PRIu64 "\n", st.nfs_uid);
	printf("nfs_gid:%" PRIu64 "\n", st.nfs_gid);
	printf("nfs_size:%" PRIu64 "\n", st.nfs_size);
	printf("nfs_atime:%" PRIu64 "\n", st.nfs_atime);
	printf("nfs_mtime:%" PRIu64 "\n", st.nfs_mtime);
	printf("nfs_ctime:%" PRIu64 "\n", st.nfs_ctime);

	nfs_destroy_url(url);
	nfs_close(nfs, nfsfh);
	nfs_destroy_context(nfs);

	return 0;
}
