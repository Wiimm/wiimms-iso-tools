
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

#ifndef WIT_LIB_WDF_H
#define WIT_LIB_WDF_H 1

#include "dclib/dclib-types.h"
#include "lib-std.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WDF definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

// First a magic is defined to identify a WDF clearly. The magic should never be
// a possible Wii ISO image identification. Wii ISO images starts with the ID6.
// And so the WDF magic contains one control character (CTRL-A).

#define WDF_MAGIC		"WII\1DISC"
#define WDF_MAGIC_SIZE		8

#define WDF_DEF_VERSION		2	// default version
#define WDF_MAX_VERSION		2	// max supported version

#define WDF_DEF_ALIGN		4	// default alignment for WDF v2 and above
#define WDF_DEF_ALIGN_TEXT	"4"
#define WDF_MAX_ALIGN		GiB	// max allowed WDF alignment
#define WDF_MAX_ALIGN_TEXT	"1 GiB"

//--- WDF head sizes
#define WDF_VERSION1_SIZE	sizeof(wdf_header_t)
#define WDF_VERSION2_SIZE	sizeof(wdf_header_t)
#define WDF_VERSION3_SIZE	sizeof(wdf_header_t)

//--- WDF hole detection type
typedef u32 WDF_Hole_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wdf_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// This is the header of a WDF v1.
// Remember: Within a file the data is stored in network byte order (big endian)
// [[wdf_header_t]]

typedef struct wdf_header_t
{
	char magic[WDF_MAGIC_SIZE];	// WDF_MAGIC, what else!

	u32 wdf_version;		// WDF_DEF_VERSION

    #if 0 // first head definition, WDF v1 before 2012-09

	// split file support (never used, values were *,0,1)
	u32 split_file_id;		// for plausibility checks
	u32 split_file_index;		// zero based index of this file, always 0
	u32 split_file_total;		// number of split files, always 1

    #else

	u32 head_size;			// size of version related wdf_header_t

	u32 align_factor;		// info: Alignment forced on creation
					//	 Number is always power of 2
					//       If NULL: no alignment forced.  

	u32 wdf_compatible;		// this file is compatible down to version #

    #endif

	u64 file_size;			// the size of the virtual file

	u64 data_size;			// the ISO data size in this file
					// (without header and chunk table)

    #if 0 // first head definition, WDF v1 before 2012-09
	u32 chunk_split_file;		// which split file contains the chunk table
    #else
	u32 reserved;			// always 0
    #endif

	u32 chunk_n;			// total number of data chunks

	u64 chunk_off;			// the 'MAGIC + chunk_table' file offset
}
__attribute__ ((packed)) wdf_header_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct WDF*_Chunk_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// This is the chunk info of WDF.
// Remember: Within a file the data is stored in network byte order (big endian)
// [[wdf1_chunk_t]]

typedef struct wdf1_chunk_t
{
	// split_file_index is obsolete in WDF v2
	u32 ignored_split_file_index;	// which split file contains that chunk
					// (WDF v1: always 0, ignored)
	u64 file_pos;			// the virtual ISO file position
	u64 data_off;			// the data file offset
	u64 data_size;			// the data size

} __attribute__ ((packed)) wdf1_chunk_t;

//-----------------------------------------------------------------------------
// [[wdf2_chunk_t]]

typedef struct wdf2_chunk_t
{
	u64 file_pos;			// the virtual ISO file position
	u64 data_off;			// the data file offset
	u64 data_size;			// the data size

} __attribute__ ((packed)) wdf2_chunk_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface for wdf_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeWH ( wdf_header_t * wh );

// convert WH data, src + dest may point to same structure
void ConvertToNetworkWH ( wdf_header_t * dest, const wdf_header_t * src );
void ConvertToHostWH    ( wdf_header_t * dest, const wdf_header_t * src );

// helpers
bool FixHeaderWDF
(
    wdf_header_t	*wh,	    // dest header
    const wdf_header_t	*wh_src,    // src header, maybe NULL
    bool		setup	    // true: setup all parameters
);

static inline uint GetHeaderSizeWDF ( uint vers )
	{ return sizeof(wdf_header_t); }

///////////////////////////////////////////////////////////////////////////////
///////////////		    interface for WDF*_Chunk_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// convert WC.v1 data, src + dest may point to same structure
void ConvertToNetworkWC1 ( wdf1_chunk_t * dest, const wdf1_chunk_t * src );
void ConvertToHostWC1    ( wdf1_chunk_t * dest, const wdf1_chunk_t * src );

static inline uint GetChunkSizeWDF ( uint vers )
	{ return vers < 2 ? sizeof(wdf1_chunk_t) : sizeof(wdf2_chunk_t); }

//-----------------------------------------------------------------------------

// convert WC.v2 data, src + dest may point to same structure
void ConvertToNetworkWC2 ( wdf2_chunk_t * dest, const wdf2_chunk_t * src );
void ConvertToHostWC2    ( wdf2_chunk_t * dest, const wdf2_chunk_t * src );

//-----------------------------------------------------------------------------

void ConvertToNetworkWC1n
(
    wdf1_chunk_t	*dest,		// dest data, must not overlay 'src'
    const wdf2_chunk_t	*src,		// source data, network byte order
    uint		n_elem		// number of elements to convert
);

void ConvertToNetworkWC2n
(
    wdf2_chunk_t	*dest,		// dest data, may overlay 'src'
    const wdf2_chunk_t	*src,		// source data, network byte order
    uint		n_elem		// number of elements to convert
);

void ConvertToHostWC
(
    wdf2_chunk_t	*dest,		// dest data, may overlay 'src'
    const void		*src_data,	// source data, network byte order
    uint		wdf_version,	// related wdf version
    uint		n_elem		// number of elements to convert
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wdf_controller_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wdf_controller_t]]

typedef struct wdf_controller_t
{
    //--- base data

    wdf_header_t head;		// the WDF header

    wdf2_chunk_t *chunk;	// field with 'chunk_size' elements
    int chunk_size;		// number of elements in 'chunk'
    int chunk_used;		// number of used elements in 'chunk'

    //--- parameters

    u64 filesize_on_open;	// file size on start
    u64 min_data_off;		// min used data offset
    u64 max_data_off;		// max used data offset
    u64 max_virt_off;		// max virtual file offset
    u64 image_size;		// current image size
    u32 align;			// not NULL: force this align
    u32 min_hole_size;		// minimal size of an hole
}
wdf_controller_t;

//-----------------------------------------------------------------------------

void InitializeWDF ( wdf_controller_t * wdf );
void ResetWDF ( wdf_controller_t * wdf );

bool UpdateVersionWDF
(
    const wdf_controller_t * wdf_in,	// NULL or input file
    wdf_controller_t	   * wdf_out	// NULL or output file
);

enumOFT ProposeOFT_WDF ( enumOFT src_oft );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SuperFile_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////

#undef SUPERFILE
#define SUPERFILE struct SuperFile_t
SUPERFILE;

// WDF reading support
enumError SetupReadWDF	( SUPERFILE * sf );
enumError ReadWDF	( SUPERFILE * sf, off_t off, void * buf, size_t size );
off_t     DataBlockWDF	( SUPERFILE * sf, off_t off, size_t hint_align, off_t * block_size );

// WDF writing support
enumError SetupWriteWDF	( SUPERFILE * sf );
enumError TermWriteWDF	( SUPERFILE * sf );
enumError WriteWDF	( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteSparseWDF( SUPERFILE * sf, off_t off, const void * buf, size_t size );
enumError WriteZeroWDF	( SUPERFILE * sf, off_t off, size_t size );

// chunk management
wdf2_chunk_t * NeedChunkWDF ( SUPERFILE * sf, int at_index );

#undef SUPERFILE

//
///////////////////////////////////////////////////////////////////////////////
///////////////				options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern uint opt_wdf_version;
extern uint opt_wdf_align;
extern uint opt_wdf_min_holesize;
extern uint use_wdf_version;
extern uint use_wdf_align;

void SetupOptionsWDF();
int SetModeWDF ( uint c, ccp align );
int ScanOptAlignWDF ( ccp arg, ccp opt_name );

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_WDF_H

