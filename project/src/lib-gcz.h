
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

#ifndef WIT_LIB_GCZ_H
#define WIT_LIB_GCZ_H 1

#include "dclib/dclib-types.h"
#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			GCZ file layout			///////////////
///////////////////////////////////////////////////////////////////////////////

// *** file layout ***
//
//	+-----------------------+
//	| GCZ_Head_t		|  filer header
//	+-----------------------+
//	| u64[]			|  table: offset for each block
//	| u32[]			|  table: checksum for each block
//	+-----------------------+
//	| data block		|  stored in any order
//	| ...			|
//	+-----------------------+

//
///////////////////////////////////////////////////////////////////////////////
///////////////			GCZ definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

#define GCZ_MAGIC_NUM		0xb10bc001
#define GCZ_TYPE		1
#define GCZ_DEF_BLOCK_SIZE	0x4000

//
///////////////////////////////////////////////////////////////////////////////
///////////////			GCZ_Head_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct GCZ_Head_t // little endian
{
  /* 0x00 */	u32 magic;		// GCZ_MAGIC_NUM
  /* 0x04 */	u32 sub_type;		// GCZ_TYPE
  /* 0x08 */	u64 compr_size;		// size of compressed data
  /* 0x10 */	u64 image_size;		// uncompressed data size
  /* 0x18 */	u32 block_size;		// size of one block
  /* 0x1c */	u32 num_blocks;		// number of blocks
  /* 0x20 */
}
__attribute__ ((packed)) GCZ_Head_t;
 
//
///////////////////////////////////////////////////////////////////////////////
///////////////			    GCZ_t			///////////////
///////////////////////////////////////////////////////////////////////////////

struct wd_disc_t;

typedef struct GCZ_t // little endian
{
    GCZ_Head_t		head;		// GCT header
    u64			*offset;	// offset list, alloced
    u32			*checksum;	// checksum list, *NOT* alloced
    u64			data_offset;	// offset of data in file

    //--- source disc support

    struct wd_disc_t	*disc;		// NULL or pointer to source disc
    bool		fast;		// enable fast mode, only true if 'disc' avaialable

    //--- current block and data

    u32			block;		// block of 'data', ~0: invalid
    u8			*data;		// last decompressed block, alloced
    u8			*cdata;		// space for compressed data, *NOT* alloced

    //--- data of a zero block

    u8			*zero_data;	// NULL or alloced
    uint		zero_size;	// alloced size of 'zero_data'
    u32			zero_checksum;	// checksum of zero block
}
GCZ_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Interface			///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool opt_gcz_zip;
extern u32  opt_gcz_block_size;
int ScanOptGCZBlock ( ccp arg );

///////////////////////////////////////////////////////////////////////////////

bool IsValidGCZ
(
    const void		*data,		// valid pointer to data
    uint		data_size,	// size of data to analyze
    u64			file_size,	// NULL or known file size
    GCZ_Head_t		*head		// not NULL: store header (local endian) here
);

///////////////////////////////////////////////////////////////////////////////

void ResetGCZ ( GCZ_t *gcz );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SuperFile_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////
// WFile_t level reading

enumError LoadHeadGCZ
(
    GCZ_t		*gcz,		// pointer to data, will be initalized
    WFile_t		*f		// file to read
);

//-----------------------------------------------------------------------------

enumError LoadDataGCZ
(
    GCZ_t		*gcz,		// pointer to data, will be initalized
    WFile_t		*f,		// source file
    off_t		off,		// file offset
    void		*buf,		// destination buffer
    size_t		count		// number of bytes to read
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// GCZ reading support

struct SuperFile_t;

enumError SetupReadGCZ
(
    struct SuperFile_t	* sf		// file to setup
);

//-----------------------------------------------------------------------------

enumError ReadGCZ
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// file offset
    void		* buf,		// destination buffer
    size_t		count		// number of bytes to read
);

//-----------------------------------------------------------------------------

off_t DataBlockGCZ
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// virtual file offset
    size_t		hint_align,	// if >1: hint for a aligment factor
    off_t		* block_size	// not null: return block size
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// GCZ writing support

enumError SetupWriteGCZ
(
    struct SuperFile_t	* sf,		// file to setup
    u64			src_file_size	// NULL or source file size
);

//-----------------------------------------------------------------------------

enumError TermWriteGCZ
(
    struct SuperFile_t	* sf		// file to terminate
);

//-----------------------------------------------------------------------------

enumError WriteGCZ
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError WriteZeroGCZ
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError FlushGCZ
(
    struct SuperFile_t	* sf		// destination file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_GCZ_H

