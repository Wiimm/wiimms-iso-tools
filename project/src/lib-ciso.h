
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

#ifndef WIT_LIB_CISO_H
#define WIT_LIB_CISO_H 1

#include "dclib/dclib-types.h"
#include "libwbfs.h"
#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   CISO options			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumChunkMode
{
	CHUNK_MODE_ANY,		// allow any values
	CHUNK_MODE_32KIB,	// force multiple of 32KiB
	CHUNK_MODE_POW2,	// force multiple of 32KiB and power of 2
	CHUNK_MODE_ISO,		// force values good for iso images and loaders

} enumChunkMode;

extern enumChunkMode opt_chunk_mode;
extern u32  opt_chunk_size;
extern bool force_chunk_size;
extern u32  opt_max_chunks;

 // returns '1' on error, '0' else
int ScanChunkMode ( ccp source );
int ScanMaxChunks ( ccp source );
int ScanChunkSize ( ccp source );

u32 CalcBlockSizeCISO ( u32 * result_n_blocks, off_t file_size );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   CISO support			///////////////
///////////////////////////////////////////////////////////////////////////////

enum // some constants
{
    CISO_HEAD_SIZE		= 0x8000,		// total header size
    CISO_MAP_SIZE		= CISO_HEAD_SIZE - 8,	// map size
    CISO_MIN_BLOCK_SIZE		= WII_SECTOR_SIZE,	// minimal allowed block size
    CISO_MAX_BLOCK_SIZE		= 0x80000000,		// maximal allowed block size

    CISO_WR_MIN_BLOCK_SIZE	= 1*MiB,		// minimal block size if writing
    CISO_WR_MAX_BLOCK		= 0x2000,		// maximal blocks if writing
    CISO_WR_MIN_HOLE_SIZE	= 0x1000,		// min hole size for sparse cheking
};

typedef u16 CISO_Map_t;
#define CISO_UNUSED_BLOCK ((CISO_Map_t)~0)

//-----------------------------------------------------------------------------

typedef struct CISO_Head_t
{
	u8  magic[4];		// "CISO"
	u32 block_size;		// stored as litte endian (not network byte order)
	u8  map[CISO_MAP_SIZE];	// 0=unused, 1=used

} __attribute__ ((packed)) CISO_Head_t;

//-----------------------------------------------------------------------------

typedef struct CISO_Info_t
{
	u32 block_size;		// the block size
	u32 used_blocks;	// number of used blocks
	u32 needed_blocks;	// number of needed blocks
	u32 map_size;		// number of alloced elements for 'map'
	CISO_Map_t * map;	// NULL or map with 'map_size' elements
	off_t max_file_off;	// maximal file offset
	off_t max_virt_off;	// maximal virtiual iso offset

} CISO_Info_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////              interface for CISO files           ///////////////
///////////////////////////////////////////////////////////////////////////////

// Initialize CISO_Info_t, copy and validate data from CISO_Head_t if not NULL
enumError InitializeCISO ( CISO_Info_t * ci, CISO_Head_t * ch );

// Setup CISO_Info_t, copy and validate data from CISO_Head_t if not NULL
enumError SetupCISO ( CISO_Info_t * ci, CISO_Head_t * ch );

// free all dynamic data and Clear data
void ResetCISO ( CISO_Info_t * ci );

//-----------------------------------------------------------------------------

#undef SUPERFILE
#define SUPERFILE struct SuperFile_t
SUPERFILE;

// CISO reading support
enumError SetupReadCISO	( SUPERFILE * sf );
enumError ReadCISO	( SUPERFILE * sf, off_t off, void * buf, size_t size );
off_t     DataBlockCISO	( SUPERFILE * sf, off_t off, size_t hint_align, off_t * block_size );

// CISO writing support
enumError SetupWriteCISO ( SUPERFILE * sf );
enumError TermWriteCISO	 ( SUPERFILE * sf );
enumError WriteCISO	 ( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteSparseCISO( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteZeroCISO	 ( SUPERFILE * sf, off_t off, size_t size );

#undef SUPERFILE

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_CISO_H

