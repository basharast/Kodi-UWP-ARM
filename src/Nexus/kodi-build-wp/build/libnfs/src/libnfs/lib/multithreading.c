/* -*-  mode:c; tab-width:8; c-basic-offset:8; indent-tabs-mode:nil;  -*- */
/*
   Copyright (C) 2010 by Ronnie Sahlberg <ronniesahlberg@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef AROS
#include "aros_compat.h"
#endif

#ifdef PS2_EE
#include "ps2_compat.h"
#endif

#ifdef PS3_PPU
#include "ps3_compat.h"
#endif

#ifdef WIN32
#include <win32/win32_compat.h>
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "libnfs.h"
#include "libnfs-raw.h"
#include "libnfs-private.h"

#ifdef HAVE_MULTITHREADING

#ifdef HAVE_PTHREAD
#include <unistd.h>
#include <sys/syscall.h>

nfs_tid_t nfs_mt_get_tid(void)
{
#ifdef SYS_gettid
        pid_t tid = syscall(SYS_gettid);
        return tid;
#else
#error "SYS_gettid unavailable on this system"
#endif
}

static void *nfs_mt_service_thread(void *arg)
{
        struct nfs_context *nfs = (struct nfs_context *)arg;
	struct pollfd pfd;
	int revents;
	int ret;

        nfs->rpc->multithreading_enabled = 1;

	while (nfs->rpc->multithreading_enabled) {
		pfd.fd = nfs_get_fd(nfs);
		pfd.events = nfs_which_events(nfs);
		pfd.revents = 0;

		ret = poll(&pfd, 1, 0);
		if (ret < 0) {
			nfs_set_error(nfs, "Poll failed");
			revents = -1;
		} else {
			revents = pfd.revents;
		}
		if (nfs_service(nfs, revents) < 0) {
			if (revents != -1)
				nfs_set_error(nfs, "nfs_service failed");
		}
	}
        return NULL;
}

int nfs_mt_service_thread_start(struct nfs_context *nfs)
{
        if (pthread_create(&nfs->nfsi->service_thread, NULL,
                           &nfs_mt_service_thread, nfs)) {
                nfs_set_error(nfs, "Failed to start service thread");
                return -1;
        }
        while (nfs->rpc->multithreading_enabled == 0) {
                struct timespec ts = {0, 1000000};
                nanosleep(&ts, NULL);
        }
        return 0;
}

void nfs_mt_service_thread_stop(struct nfs_context *nfs)
{
        nfs->rpc->multithreading_enabled = 0;
        pthread_join(nfs->nfsi->service_thread, NULL);
}
        
int nfs_mt_mutex_init(libnfs_mutex_t *mutex)
{
        pthread_mutex_init(mutex, NULL);
        return 0;
}

int nfs_mt_mutex_destroy(libnfs_mutex_t *mutex)
{
        pthread_mutex_destroy(mutex);
        return 0;
}

int nfs_mt_mutex_lock(libnfs_mutex_t *mutex)
{
        return pthread_mutex_lock(mutex);
}

int nfs_mt_mutex_unlock(libnfs_mutex_t *mutex)
{
        return pthread_mutex_unlock(mutex);
}

int nfs_mt_sem_init(libnfs_sem_t *sem, int value)
{
        return sem_init(sem, 0, value);
}

int nfs_mt_sem_destroy(libnfs_sem_t *sem)
{
        return sem_destroy(sem);
}

int nfs_mt_sem_post(libnfs_sem_t *sem)
{
        return sem_post(sem);
}

int nfs_mt_sem_wait(libnfs_sem_t *sem)
{
        return sem_wait(sem);
}

#elif WIN32 
nfs_tid_t nfs_mt_get_tid(void)
{
    return GetCurrentThreadId();
}
static void* nfs_mt_service_thread(void* arg)
{
    struct nfs_context* nfs = (struct nfs_context*)arg;
    struct pollfd pfd;
    int revents;
    int ret;

    nfs->rpc->multithreading_enabled = 1;

    while (nfs->rpc->multithreading_enabled) {
        pfd.fd = nfs_get_fd(nfs);
        pfd.events = nfs_which_events(nfs);
        pfd.revents = 0;

        ret = poll(&pfd, 1, 0);
        if (ret < 0) {
            nfs_set_error(nfs, "Poll failed");
            revents = -1;
        }
        else {
            revents = pfd.revents;
        }
        if (nfs_service(nfs, revents) < 0) {
            if (revents != -1)
                nfs_set_error(nfs, "nfs_service failed");
        }
    }
    return NULL;
}

static DWORD WINAPI service_thread_init(LPVOID lpParam)
{
    HANDLE hStdout;
    struct nfs_context* nfs;

    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hStdout == INVALID_HANDLE_VALUE) {
        return 1;
    }
    nfs = (struct nfs_context *)lpParam;
    nfs_mt_service_thread(nfs);
    return 0;
}

int nfs_mt_service_thread_start(struct nfs_context* nfs)
{
    nfs->nfsi->service_thread = CreateThread(NULL, 1024*1024, service_thread_init, nfs, 0, NULL);
    if (nfs->nfsi->service_thread == NULL) {
        nfs_set_error(nfs, "Failed to start service thread");
        return -1;
    }
    while (nfs->rpc->multithreading_enabled == 0) {
        Sleep(100);
    }
    return 0;
}

void nfs_mt_service_thread_stop(struct nfs_context* nfs)
{
    nfs->rpc->multithreading_enabled = 0;
    while (WaitForSingleObject(nfs->nfsi->service_thread, INFINITE) != WAIT_OBJECT_0);
}

int nfs_mt_mutex_init(libnfs_mutex_t* mutex)
{
    *mutex = CreateSemaphoreA(NULL, 1, 1, NULL);
    return 0;
}

int nfs_mt_mutex_destroy(libnfs_mutex_t* mutex)
{
    CloseHandle(*mutex);
    return 0;
}

int nfs_mt_mutex_lock(libnfs_mutex_t* mutex)
{
    while (WaitForSingleObject(*mutex, INFINITE) != WAIT_OBJECT_0);
    return 0;
}

int nfs_mt_mutex_unlock(libnfs_mutex_t* mutex)
{
    ReleaseSemaphore(*mutex, 1, NULL);
    return 0;
}

int nfs_mt_sem_init(libnfs_sem_t* sem, int value)
{
    *sem = CreateSemaphoreA(NULL, 0, 16, NULL);
    return 0;
}

int nfs_mt_sem_destroy(libnfs_sem_t* sem)
{
    CloseHandle(*sem);
    return 0;
}

int nfs_mt_sem_post(libnfs_sem_t* sem)
{
    ReleaseSemaphore(*sem, 1, NULL);
    return 0;
}

int nfs_mt_sem_wait(libnfs_sem_t* sem)
{
    while (WaitForSingleObject(*sem, INFINITE) != WAIT_OBJECT_0);
    return 0;
}

#endif


#endif /* HAVE_MULTITHREADING */

