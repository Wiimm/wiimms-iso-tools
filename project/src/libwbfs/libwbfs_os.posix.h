
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
 *   Copyright (c) 2009 Kwiirk                                             *
 *   Copyright (c) 2009-2021 by Dirk Clemens <wiimm@wiimm.de>              *
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
       
//////////////////////////////////////////////////////////////////////////
/////                                                                ///// 
/////   This is a template file for 'libwbfs_os.h'                   /////
/////   Move it to your project directory and rename it.             /////
/////   Edit this file and customise it for your project and os.     /////
/////                                                                ///// 
/////   File 'libwbfs_defaults.h' defines more default macros.       ///// 
/////   Copy the macros into this file if other definitions needed.  ///// 
/////                                                                ///// 
//////////////////////////////////////////////////////////////////////////

#ifndef LIBWBFS_OS_H
#define LIBWBFS_OS_H

///////////////////////////////////////////////////////////////////////////////

// system includes
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

//#include "tools.h" 

///////////////////////////////////////////////////////////////////////////////
// uncomment if you have troubles with this names

//#define be16 wbfs_be16
//#define be32 wbfs_be32
//#define be64 wbfs_be64

//#define wbfs_count_usedblocks wbfs_get_free_block_count
//#define wbfs_open_disc wbfs_open_disc_by_id6

//#define ONLY_GAME_PARTITION WD_PART_DATA

///////////////////////////////////////////////////////////////////////////////
// type definitions -> comment out if already defined

// unsigned integer types
typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

// signed integer types
typedef signed char		s8;
typedef signed short		s16;
typedef signed int		s32;
typedef signed long long	s64;

// other types
typedef const char *		ccp;
typedef enum bool { false, true } __attribute__ ((packed)) bool;

///////////////////////////////////////////////////////////////////////////////
// error messages

typedef enum enumError
{
	ERR_OK,
	ERR_WARNING,		// separator: below = real errors and not warnings
	ERR_WDISC_NOT_FOUND,
	ERR_WPART_INVALID,
	ERR_WDISC_INVALID,
	ERR_READ_FAILED,
	ERR_ERROR,		// separator: below = hard/fatal errors => exit
	ERR_OUT_OF_MEMORY,
	ERR_FATAL

} enumError;

///////////////////////////////////////////////////////////////////////////////
// SHA1 not supported

#define SHA1 wbfs_sha1_fake

///////////////////////////////////////////////////////////////////////////////
// error messages

#define wbfs_warning(...) wd_print_error(__FUNCTION__,__FILE__,__LINE__,ERR_WARNING,__VA_ARGS__)
#define wbfs_error(...)   wd_print_error(__FUNCTION__,__FILE__,__LINE__,ERR_ERROR,__VA_ARGS__)
#define wbfs_fatal(...)   wd_print_error(__FUNCTION__,__FILE__,__LINE__,ERR_FATAL,__VA_ARGS__)
#define OUT_OF_MEMORY	  wd_print_error(__FUNCTION__,__FILE__,__LINE__,ERR_OUT_OF_MEMORY,"Out of memory")

// -> file 'libwbfs_defaults.h' contains macro WD_PRINT

///////////////////////////////////////////////////////////////////////////////
// alloc and free memory space

#define wbfs_malloc(s) MALLOC(s)
#define wbfs_calloc(n,s) CALLOC(n,s)
#define wbfs_free(x) FREE(x)

#define wbfs_ioalloc(x) MALLOC(x)
#define wbfs_iofree(x) FREE(x)

///////////////////////////////////////////////////////////////////////////////
// endianess functions

#define wbfs_ntohs(x)  ntohs(x)
#define wbfs_ntohl(x)  ntohl(x)
#define wbfs_ntoh64(x) ntoh64(x)
#define wbfs_htons(x)  htons(x)
#define wbfs_htonl(x)  htonl(x)
#define wbfs_hton64(x) hton64(x)

///////////////////////////////////////////////////////////////////////////////
// memory functions

#define wbfs_memcmp(x,y,z) memcmp(x,y,z)
#define wbfs_memcpy(x,y,z) memcpy(x,y,z)
#define wbfs_memset(x,y,z) memset(x,y,z)

///////////////////////////////////////////////////////////////////////////////
// time function

#define wbfs_time() ((u64)time(0))

///////////////////////////////////////////////////////////////////////////////

#endif // LIBWBFS_OS_H
