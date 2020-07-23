
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

#ifndef WIT_LIB_WIA_H
#define WIT_LIB_WIA_H 1

#include "dclib/dclib-types.h"
#include "lib-std.h"
#include "libwbfs/wiidisc.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WIA file layout			///////////////
///////////////////////////////////////////////////////////////////////////////

// *** file layout ***
//
//	+-----------------------+
//	| wia_file_head_t	|  data structure will never changed
//	+-----------------------+
//	| wia_disc_t		|  disc data
//	+-----------------------+
//	| wia_part_t #0		|  fixed size partition header 
//	| wia_part_t #1		|  wia_disc_t::n_part elements
//	| ...			|   -> data stored in wia_group_t elements
//	+-----------------------+
//	| data chunks		|  stored in any order
//	| ...			|
//	+-----------------------+


// *** data chunks (any order) ***
//
//	+-----------------------+
//	| compressor data	|  data for compressors (e.g properties)
//	| compressed chunks	|
//	+-----------------------+


// *** compressed chunks (any order) ***
//
//	+-----------------------+
//	| raw data table	|
//	| group table		|
//	| raw data		|  <- divided in groups (e.g partition .header)
//	| partition data	|  <- divided in groups
//	+-----------------------+


// *** partition data (Wii format) ***
//
//	+-----------------------+
//	| wia_except_list	|  wia_group_t::n_exceptions hash exceptions
//	+-----------------------+
//	| (compressed) data	|  compression dependent data
//	+-----------------------+


// *** other data (non partition data) ***
//
//	+-----------------------+
//	| (compressed) data	|  compression dependent data
//	+-----------------------+

//
///////////////////////////////////////////////////////////////////////////////
///////////////			WIA definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

// First a magic is defined to identify a WIA clearly. The magic should never be
// a possible Wii ISO image identification. Wii ISO images starts with the ID6.
// And so the WIA magic contains one contral character (CTRL-A) within.

#define WIA_MAGIC		"WIA\1"
#define WIA_MAGIC_SIZE		4

//-----------------------------------------------------
// Format of version number: AABBCCDD = A.BB | A.BB.CC
// If D != 0x00 && D != 0xff => append: 'beta' D
//-----------------------------------------------------

#define WIA_VERSION			0x01000000  // current writing version
#define WIA_VERSION_COMPATIBLE		0x00090000  // down compatible
#define WIA_VERSION_READ_COMPATIBLE	0x00080000  // read compatible
#define PrintVersionWIA PrintVersion

// the minimal size of holes in bytes that will be detected.
#define WIA_MIN_HOLE_SIZE	0x400

// the maximal supported iso size
#define WIA_MAX_SUPPORTED_ISO_SIZE (50ull*GiB)

// the minimal chunk size (if ever, multiple of this are supported)
#define WIA_BASE_CHUNK_SIZE	WII_GROUP_SIZE
#define WIA_DEF_CHUNK_FACTOR	 20
#define WIA_MAX_CHUNK_FACTOR	100

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_file_head_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_file_head_t
{
    // This data structure is never changed.
    // If additional info is needed, others structures will be expanded.
    // All values are stored in network byte order (big endian)

    char		magic[WIA_MAGIC_SIZE];	// 0x00: WIA_MAGIC, what else!

    u32			version;		// 0x04: WIA_VERSION
    u32			version_compatible;	// 0x08: compatible down to

    u32			disc_size;		// 0x0c: size of wia_disc_t
    sha1_hash_t		disc_hash;		// 0x10: hash of wia_disc_t

    u64			iso_file_size;		// 0x24: size of ISO image
    u64			wia_file_size;		// 0x2c: size of WIA file

    sha1_hash_t		file_head_hash;		// 0x34: hash of wia_file_head_t

} __attribute__ ((packed)) wia_file_head_t;	// 0x48 = 72 = sizeof(wia_file_head_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_disc_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_disc_t
{
    // All values are stored in network byte order (big endian)

    //--- base infos

    u32			disc_type;		// 0x00: wd_disc_type_t
    u32			compression;		// 0x04: wd_compression_t
    u32			compr_level;		// 0x08: method dependent compr.level
						//       This value is informative only
    u32			chunk_size;		// 0x0c: used chunk_size, 2 MiB is standard

    //--- disc header, first 0x80 bytes of source for easy detection 

    u8			dhead[0x80];		// 0x10: 128 bytes of disc header
						//	 for a fast data access
    //--- partition data, direct copy => hash needed

    u32			n_part;			// 0x90: number or partitions
    u32			part_t_size;		// 0x94: size of 1 element of wia_part_t
    u64			part_off;		// 0x98: file offset wia_part_t[n_part]
    sha1_hash_t		part_hash;		// 0xa0: hash of wia_part_t[n_part]

    //--- raw data, compressed

    u32			n_raw_data;		// 0xb4: number of wia_raw_data_t elements 
    u64			raw_data_off;		// 0xb8: offset of wia_raw_data_t[n_raw_data]
    u32			raw_data_size;		// 0xc0: conpressed size of raw data

    //--- group header, compressed

    u32			n_groups;		// 0xc4: number of wia_group_t elements
    u64			group_off;		// 0xc8: offset of wia_group_t[n_groups]
    u32			group_size;		// 0xd0: conpressed size of groups

    //--- compressor dependent data (e.g. LZMA properties)

    u8			compr_data_len;		// 0xd4: used length of 'compr_data'
    u8			compr_data[7];		// 0xd5: compressor specific data 

} __attribute__ ((packed)) wia_disc_t;		// 0xdc = 220 = sizeof(wia_disc_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wia_part_data_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// Sub structire for wia_part_t

typedef struct wia_part_data_t
{
    // All values are stored in network byte order (big endian)

    u32			first_sector;		// 0x00: first data sector
    u32			n_sectors;		// 0x04: number of sectors

    u32			group_index;		// 0x08: index of first wia_group_t 
    u32			n_groups;		// 0x0c: number of wia_group_t elements
    
} __attribute__ ((packed)) wia_part_data_t;	// 0x10 = 16 = sizeof(wia_part_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_part_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// This data structure is only used for Wii like partitions but nor for GC.
// It supports decrypting and hash removing.

typedef struct wia_part_t
{
    // All values are stored in network byte order (big endian)

    u8			part_key[WII_KEY_SIZE];	// 0x00: partition key => build aes key

    wia_part_data_t	pd[2];			// 0x10: 2 partition data segments
						//   segment 0 is small and defined
						//   for management data (boot .. fst).
						//   segment 1 takes the remaining data

} __attribute__ ((packed)) wia_part_t;		// 0x30 = 48 = sizeof(wia_part_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_raw_data_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_raw_data_t
{
    // All values are stored in network byte order (big endian)

    u64			raw_data_off;		// 0x00: disc offset of raw data
    u64			raw_data_size;		// 0x08: size of raw data
    
    u32			group_index;		// 0x10: index of first wia_group_t 
    u32			n_groups;		// 0x14: number of wia_group_t elements

} __attribute__ ((packed)) wia_raw_data_t;	// 0x18 = 24 = sizeof(wia_raw_data_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_group_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// Each group points to a data segment list. Size of last element is null.
// Partition data groups are headed by a hash exeption list (wia_except_list_t).

typedef struct wia_group_t
{
    // All values are stored in network byte order (big endian)

    u32			data_off4;		// 0x00: file offset/4 of data
    u32			data_size;		// 0x04: file size of data

} __attribute__ ((packed)) wia_group_t;		// 0x08 = 8 = sizeof(wia_group_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_exception_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_exception_t
{
    // All values are stored in network byte order (big endian)

    u16			offset;			// 0x00: sector offset of hash
    sha1_hash_t		hash;			// 0x02: hash value

} __attribute__ ((packed)) wia_exception_t;	// 0x16 = 22 = sizeof(wia_exception_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_except_list_t	///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_except_list_t
{
    u16			n_exceptions;		// 0x00: number of hash exceptions
    wia_exception_t	exception[0];		// 0x02: hash exceptions

} __attribute__ ((packed)) wia_except_list_t;	// 0x02 = 2 = sizeof(wia_except_list_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_segment_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_segment_t
{
    // All values are stored in network byte order (big endian)
    // Segments are used to reduce the size of uncompressed data (method PURGE)
    // This is done by eleminatin holes (zeroed areas)

    u32			offset;			// 0x00: offset relative to group
    u32			size;			// 0x04: size of 'data'
    u8			data[0];		// 0x08: data

} __attribute__ ((packed)) wia_segment_t;	// 0x08 = 8 = sizeof(wia_segment_t)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wia_controller_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct wia_controller_t
{
    wd_disc_t		* wdisc;	// valid if writing

    wia_file_head_t	fhead;		// header in local endian
    wia_disc_t		disc;		// disc data

    wia_part_t		* part;		// NULL or pointer to partition info

    wia_raw_data_t	* raw_data;	// NULL or pointer to raw data list
    u32			raw_data_used;	// number of used 'group' elements
    u32			raw_data_size;	// number of alloced 'group' elements
    wia_raw_data_t	* growing;	// NULL or pointer to last element of 'raw_data'
					// used for writing behind expected file size

    wia_group_t		* group;	// NULL or pointer to group list
    u32			group_used;	// number of used 'group' elements
    u32			group_size;	// number of alloced 'group' elements

    wd_memmap_t		memmap;		// memory mapping

    bool		encrypt;	// true: encrypt data if reading
    bool		is_writing;	// false: read a WIA / true: write a WIA
    bool		is_valid;	// true: WIA is valid

    u32			chunk_size;	// chunk size in use
    u32			chunk_sectors;	// sector per chunk
    u32			chunk_groups;	// sector groups per chunk
    u32			memory_usage;	// calculated memory usage (RAM)

    u64			write_data_off;	// writing file offset for the next data


    //----- group data cache

    u8			* gdata;	// group data
    u32			gdata_size;	// alloced size of 'gdata'
    u32			gdata_used;	// relevant size of 'gdata'
    int			gdata_group;	// index of current group, -1:invalid
    int			gdata_part;	// partition index of current group data

    aes_key_t		akey;		// akey of 'gdata_part'
    wd_part_sector_t	empty_sector;	// empty encrypted sector, calced with 'akey'

} wia_controller_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    interface			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetWIA
(
    wia_controller_t	* wia		// NULL or valid pointer
);

//-----------------------------------------------------------------------------

bool IsWIA
(
    const void		* data,		// data to check
    size_t		data_size,	// size of data
    void		* id6_result,	// not NULL: store ID6 (6 bytes without null term)
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wd_compression_t	* compression	// not NULL: store compression
);

//-----------------------------------------------------------------------------

u32 CalcMemoryUsageWIA
(
    wd_compression_t	compression,	// compression method
    int			compr_level,	// valid are 1..9 / 0: use default value
    u32			chunk_size,	// wanted chunk size
    bool		is_writing	// false: reading mode, true: writing mode
);

//-----------------------------------------------------------------------------

int CalcDefaultSettingsWIA
(
    wd_compression_t	* compression,	// NULL or compression method
    int			* compr_level,	// NULL or compression level
    u32			* chunk_size	// NULL or wanted chunk size
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SuperFile_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////
// WIA reading support

struct SuperFile_t;

enumError SetupReadWIA
(
    struct SuperFile_t	* sf		// file to setup
);

//-----------------------------------------------------------------------------

enumError ReadWIA
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// file offset
    void		* buf,		// destination buffer
    size_t		count		// number of bytes to read
);

//-----------------------------------------------------------------------------

off_t DataBlockWIA
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// virtual file offset
    size_t		hint_align,	// if >1: hint for a aligment factor
    off_t		* block_size	// not null: return block size
);

//-----------------------------------------------------------------------------
// WIA writing support

enumError SetupWriteWIA
(
    struct SuperFile_t	* sf,		// file to setup
    u64			src_file_size	// NULL or source file size
);

//-----------------------------------------------------------------------------

enumError TermWriteWIA
(
    struct SuperFile_t	* sf		// file to terminate
);

//-----------------------------------------------------------------------------

enumError WriteWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError WriteSparseWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError WriteZeroWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    size_t		count		// number of bytes to write
);

//-----------------------------------------------------------------------------

enumError FlushWIA
(
    struct SuperFile_t	* sf		// destination file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_file_head ( wia_file_head_t * dest, const wia_file_head_t * src );
void wia_hton_file_head ( wia_file_head_t * dest, const wia_file_head_t * src );

void wia_ntoh_disc ( wia_disc_t * dest, const wia_disc_t * src );
void wia_hton_disc ( wia_disc_t * dest, const wia_disc_t * src );

void wia_ntoh_part ( wia_part_t * dest, const wia_part_t * src );
void wia_hton_part ( wia_part_t * dest, const wia_part_t * src );

void wia_ntoh_raw_data ( wia_raw_data_t * dest, const wia_raw_data_t * src );
void wia_hton_raw_data ( wia_raw_data_t * dest, const wia_raw_data_t * src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_WIA_H
