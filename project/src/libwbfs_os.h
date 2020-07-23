
/***************************************************************************
 *                    __            __ _ ___________                       *
 *                    \ \          / /| |____   ____|                      *
 *                     \ \        / / | |    | |                           *
 *                      \ \  /\  / /  | |    | |                           *
 *                       \ \/  \/ /   | |    | |                           *
 *                        \  /\  /    | |    | |                           *
 *                         \/  \/     |_|    |_|                           *
 *                                                                         *
 *                           Wiimms ISO Tools                              *
 *                         https://wit.wiimm.de/                           *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit https://wit.wiimm.de/ for project details and sources.          *
 *                                                                         *
 *   Copyright (c) 2009-2017 by Dirk Clemens <wiimm@wiimm.de>              *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#ifndef LIBWBFS_OS_H
#define LIBWBFS_OS_H

// system includes
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// project dependent includes
#include "dclib/dclib-types.h"
#include "dclib/dclib-debug.h"
#include "lib-error.h"
#include "crypt.h"

// error messages
#define wbfs_fatal(...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_FATAL,__VA_ARGS__)
#define wbfs_error(...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_WBFS,__VA_ARGS__)
#define wbfs_warning(...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_WARNING,__VA_ARGS__)

// alloc and free memory space
#define wbfs_malloc(s) MALLOC(s)
#define wbfs_calloc(n,s) CALLOC(n,s)
#define wbfs_free(x) FREE(x)

// alloc and free memory space suitable for disk io
#define wbfs_ioalloc(x) MALLOC(x)
#define wbfs_iofree(x) FREE(x)

// endianess functions
#define wbfs_ntohs(x)  ntohs(x)
#define wbfs_ntohl(x)  ntohl(x)
#define wbfs_ntoh64(x) ntoh64(x)
#define wbfs_htons(x)  htons(x)
#define wbfs_htonl(x)  htonl(x)
#define wbfs_hton64(x) hton64(x)

// memory functions
#define wbfs_memcmp(x,y,z) memcmp(x,y,z)
#define wbfs_memcpy(x,y,z) memcpy(x,y,z)
#define wbfs_memset(x,y,z) memset(x,y,z)

// time function
#define wbfs_time() ((u64)time(0))

#endif // LIBWBFS_OS_H
