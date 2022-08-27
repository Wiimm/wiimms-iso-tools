
/***************************************************************************
 *                                                                         *
 *                     _____     ____                                      *
 *                    |  __ \   / __ \   _     _ _____                     *
 *                    | |  \ \ / /  \_\ | |   | |  _  \                    *
 *                    | |   \ \| |      | |   | | |_| |                    *
 *                    | |   | || |      | |   | |  ___/                    *
 *                    | |   / /| |   __ | |   | |  _  \                    *
 *                    | |__/ / \ \__/ / | |___| | |_| |                    *
 *                    |_____/   \____/  |_____|_|_____/                    *
 *                                                                         *
 *                       Wiimms source code library                        *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *        Copyright (c) 2012-2022 by Dirk Clemens <wiimm@wiimm.de>         *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#ifndef DC_LIB_DOL_H
#define DC_LIB_DOL_H 1

#define _GNU_SOURCE 1
#include "dclib-basics.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			DOL: defs & structs		///////////////
///////////////////////////////////////////////////////////////////////////////

#define DOL_N_TEXT_SECTIONS	 7
#define DOL_N_DATA_SECTIONS	11
#define DOL_N_SECTIONS		18

#define DOL_IDX_BSS		DOL_N_SECTIONS		// alias for BSS
#define DOL_IDX_ENTRY		(DOL_N_SECTIONS+1)	// alias for ENTRY
#define DOL_NN_SECTIONS		(DOL_N_SECTIONS+2)	// N + aliase

#define DOL_HEADER_SIZE		0x100

//-----------------------------------------------------------------------------

#define ASM_NOP			0x60000000

//-----------------------------------------------------------------------------

static int IsDolTextSection ( uint section )
	{ return section < DOL_N_TEXT_SECTIONS; }

static bool IsDolDataSection ( uint section )
	{ return section - DOL_N_TEXT_SECTIONS < DOL_N_DATA_SECTIONS; }

//-----------------------------------------------------------------------------
// [[dol_header_t]]

typedef struct dol_header_t
{
    /* 0x00 */	u32 sect_off [DOL_N_SECTIONS];	// file offset
    /* 0x48 */	u32 sect_addr[DOL_N_SECTIONS];	// virtual address
    /* 0x90 */	u32 sect_size[DOL_N_SECTIONS];	// section size
    /* 0xd8 */	u32 bss_addr;			// BSS address
    /* 0xdc */	u32 bss_size;			// BSS size
    /* 0xe0 */	u32 entry_addr;			// entry point
    /* 0xe4 */	u8  padding[DOL_HEADER_SIZE-0xe4];
}
__attribute__ ((packed)) dol_header_t;

///////////////////////////////////////////////////////////////////////////////
// [[dol_sect_info_t]]

typedef struct dol_sect_info_t
{
    int		section;	// section index, <0: invalid
    char	name[4];	// name of section
    u32		off;		// file offset of data
    u32		addr;		// image address
    u32		size;		// size of section
    bool	sect_valid;	// true: section is valid
    bool	data_valid;	// true: data is valid (size>0 and inside DOL file)
    bool	hash_valid;	// true: 'hash' is valid
    u8		*data;		// NULL (data_valid=0) or pointer to data (data_valid=1)
    sha1_hash_t	hash;		// SHA1 hash of data
}
dol_sect_info_t;

///////////////////////////////////////////////////////////////////////////////
// [[dol_sect_select_t]]

typedef struct dol_sect_select_t
{
    int		find_mode;	// mode for xFindFirstFreeDolSection(), <0: skip this
    int		sect_idx;	// >=0: section, <0: auto select
    char	name[4];	// a name based on 'sect_idx' and 'find_mode'
    u32		addr;		// address of section, if 0: use DATA[0x10]
    ccp		fname;		// file name
    bool	use_param;	// true: write params (entry and '.dol' section)

    u8		*data;		// not NULL: data, alloced
    uint	size;		// >0: size of 'data'
    u32		offset;		// last used file offset (temporary)
}
dol_sect_select_t;

///////////////////////////////////////////////////////////////////////////////
// [[dol_sect_addr_t]]

typedef struct dol_sect_addr_t
{
    //--- source info
    u32		addr;		// address of data
    u32		size;		// size of data

    //--- search info
    int		section;	// section index, <0: invalid
    char	sect_name[4];	// empty or name of found section
    u32		sect_addr;	// NULL or section address
    u32		sect_offset;	// NULL or offset in found section
    u32		sect_size;	// NULL or size of found section
}
dol_sect_addr_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			DOL: Interface			///////////////
///////////////////////////////////////////////////////////////////////////////

void ntoh_dol_header ( dol_header_t * dest, const dol_header_t * src );
void hton_dol_header ( dol_header_t * dest, const dol_header_t * src );
bool IsDolHeader ( const void *data, uint data_size, uint file_size );

extern const char dol_section_name[DOL_NN_SECTIONS+1][4];
static inline ccp GetDolSectionName ( uint section )
{
    return section < DOL_NN_SECTIONS ? dol_section_name[section] : 0;
}

extern const sizeof_info_t sizeof_info_dol[];

///////////////////////////////////////////////////////////////////////////////

bool GetDolSectionInfo
(
    dol_sect_info_t	* ret_info,	// valid pointer: returned data
    const dol_header_t	*dol_head,	// valid DOL header
    uint		file_size,	// (file) size of 'data'
    uint		section		// 0 .. DOL_NN_SECTIONS
					//	(7*TEXT, 11*DATA, BSS, ENTRY )
);

//-----------------------------------------------------------------------------

bool FindFirstFreeDolSection
(
    // returns TRUE on found

    dol_sect_info_t	*info,	// result
    const dol_header_t	*dh,	// DOL header to analyse
    uint		mode	// 0: search all sections (TEXT first)
				// 1: search only TEXT sections
				// 2: search only DATA sections
				// 3: search all sections (DATA first)
);

//-----------------------------------------------------------------------------

bool FindDolSectionBySelector
(
    // returns TRUE on found

    dol_sect_info_t		*info,	// result
    const dol_header_t		*dh,	// DOL header to analyse
    const dol_sect_select_t	*dss	// DOL section selector
);

//-----------------------------------------------------------------------------

u32 FindFreeSpaceDOL
(
    // return address or NULL on not-found

    const dol_header_t	*dol_head,	// valid DOL header
    u32			addr_beg,	// first possible address
    u32			addr_end,	// last possible address
    u32			size,		// minimal size
    u32			align,		// aligning
    u32			*space		// not NULL: store available space here
);

//-----------------------------------------------------------------------------

u32 FindFreeBssSpaceDOL
(
    // return address or NULL on not-found

    const dol_header_t	*dol_head,	// valid DOL header
    u32			size,		// min size
    u32			align,		// aligning
    u32			*space		// not NULL: store available space here
);

///////////////////////////////////////////////////////////////////////////////

void DumpDolHeader
(
    FILE		* f,		// dump to this file
    int			indent,		// indention
    const dol_header_t	*dol_head,	// valid DOL header
    uint		file_size,	// NULL or size of file
    uint		print_mode	// bit field:
					//  1: print file map
					//  2: print virtual mem map
					//  4: print delta table
);

///////////////////////////////////////////////////////////////////////////////

int GetDolSectionAddr
(
    // returns dsa->section
    dol_sect_addr_t	*dsa,		// result: section address info
    const dol_header_t	*dol_head,	// valid DOL header
    u32			addr,		// address to search
    u32			size		// >0: wanted size
);

//-----------------------------------------------------------------------------

ccp GetDolSectionAddrInfo
(
    // returns GetCircBuf() or EmptyString on error
    const dol_header_t	*dol_head,	// valid DOL header
    u32			addr,		// address to search
    u32			size		// >0: wanted size
);

///////////////////////////////////////////////////////////////////////////////

u32 GetDolOffsetByAddr
(
    const dol_header_t	*dol_head,	// valid DOL header
    u32			addr,		// address to search
    u32			size,		// >0: wanted size
    u32			*valid_size	// not NULL: return valid size
);

///////////////////////////////////////////////////////////////////////////////

u32 GetDolAddrByOffset
(
    const dol_header_t	*dol_head,	// valid DOL header
    u32			off,		// offset to search
    u32			size,		// >0: wanted size
    u32			*valid_size	// not NULL: return valid size
);

///////////////////////////////////////////////////////////////////////////////

uint AddDolAddrByOffset
(
    const dol_header_t	*dol_head,	// valid DOL header
    MemMap_t		*mm,		// valid destination mem map, not cleared
    bool		use_tie,	// TRUE: use InsertMemMapTie()
    u32			off,		// offset to search
    u32			size		// size, may overlay multiple sections
);

///////////////////////////////////////////////////////////////////////////////

uint TranslateDolOffsets
(
    const dol_header_t	*dol_head,	// valid DOL header
    MemMap_t		*mm,		// valid destination mem map, not cleared
    bool		use_tie,	// TRUE: use InsertMemMapTie()
    const MemMap_t	*mm_off		// NULL or mem map with offsets
);

///////////////////////////////////////////////////////////////////////////////

uint TranslateAllDolOffsets
(
    const dol_header_t	*dol_head,	// valid DOL header
    MemMap_t		*mm,		// valid destination mem map, not cleared
    bool		use_tie		// TRUE: use InsertMemMapTie()
);

///////////////////////////////////////////////////////////////////////////////

uint TranslateDolSections
(
    const dol_header_t	*dol_head,	// valid DOL header
    MemMap_t		*mm,		// valid destination mem map, not cleared
    bool		use_tie,	// TRUE: use InsertMemMapTie()
    uint		dol_sections	// bitfield of sections
);

///////////////////////////////////////////////////////////////////////////////

uint CountOverlappedDolSections
(
    // returns the number of overlapped section by [addr,size]

    const dol_header_t	*dol_head,	// valid DOL header
    u32			addr,		// address to search
    u32			size		// size of addr to verify
);

///////////////////////////////////////////////////////////////////////////////

u32 FindDolAddressOfVBI
(
    // search in available data and return NULL if not found

    cvp			data,		// DOL data beginning with dol_header_t
    uint		data_size	// size of 'data'
);

///////////////////////////////////////////////////////////////////////////////

char * ScanDolSectionName
(
    // returns the first uninterpreted character

    ccp			arg,		// comma/space separated names
    int			*ret_section,	// not NULL: return section index
					//	<0: error / >=0: section index
    enumError		*ret_err	// not NULL: return error code
);

//-----------------------------------------------------------------------------

uint ScanDolSectionList
(
    // return a bitfield as section list

    ccp			arg,		// comma/space separated names
    enumError		*ret_err	// not NULL: return status
);

//-----------------------------------------------------------------------------

ccp DolSectionList2Text ( uint sect_list );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    add/remove DOL sections		///////////////
///////////////////////////////////////////////////////////////////////////////

uint CompressDOL
(
    // return: size of compressed DOL (inline replaced)

    dol_header_t	*dol,		// valid dol header
    FILE		*log		// not NULL: print memmap to this file
);

//-----------------------------------------------------------------------------

uint RemoveDolSections
(
    // return: 0:if not modifed, >0:size of compressed DOL

    dol_header_t	*dol,		// valid dol header
    uint		stay,		// bit field of sections (0..17): SET => don't remove
    uint		*res_stay,	// not NULL: store bit field of vlaid sections here
    FILE		*log		// not NULL: print memmap to this file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Gecko Code Handler		///////////////
///////////////////////////////////////////////////////////////////////////////

#define GCH_MAGIC_NUM 0x47044348

#define GCT_MAGIC8_NUM	0x00d0c0de00d0c0deull
#define GCT_TERM_NUM	0xf000000000000000ull
#define GCT_SEP_NUM1	0xf0000001u
#define GCT_REG_OFFSET	8

///////////////////////////////////////////////////////////////////////////////
// [[gch_header_t]]

typedef struct gch_header_t
{
    // big endian!

    u32	magic;			// magic: 'G\x{04}CH' (Gecko Code Handler)
    u32	addr;			// address of new section
    u32	size;			// size of code section, file size must be >= 'size'
				// a cheat section may follow
    u32	vbi_entry;		// >0: entry point for VBI interrupts

    u8  data[];			// usually start of code handler
}
__attribute__ ((packed)) gch_header_t;

///////////////////////////////////////////////////////////////////////////////

bool IsValidGCH
(
    const gch_header_t *gh,		// valid header
    uint		file_size	// size of file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Wiimms Code Handler		///////////////
///////////////////////////////////////////////////////////////////////////////

#define WCH_MAGIC_NUM 0x57054348

///////////////////////////////////////////////////////////////////////////////
// [[wch_segment_t]]

typedef struct wch_segment_t
{
    // big endian!

    u32 type;			// 'T': text section
				// 'D': data section
				// 'S': any section, try data first, then text
				// 'P': patch data => patch main.dol
				//  0 : end of segment list

    u32 addr;			// address, where the data is stored
    u32 size;			// size of data

    u32	main_entry;		// >0: `main entry point´
    u32	main_entry_old;		// >0: store old `main entry point´ at this address
    u32	vbi_entry;		// >0: `vbi entry point´
    u32	vbi_entry_old;		// >0: store old `vbi entry point´ at this address

    u8 data[];			// data, aligned by ALIGN(size,4)
    // wch_segment_t		// next segment, aligned by ALIGN(size,4)
}
__attribute__ ((packed)) wch_segment_t;

///////////////////////////////////////////////////////////////////////////////
// [[wch_header_t]]

typedef struct wch_header_t
{
    // big endian!

    u32	magic;			// magic: 'W\x{05}CH' (Gecko Code Handler)
    u32 version;		// 0: raw, 1: bzip2
    u32	size;			// total size of (uncompressed) segment data

    wch_segment_t segment[];	// first segment
}
__attribute__ ((packed)) wch_header_t;

///////////////////////////////////////////////////////////////////////////////
// [[wch_control_t]]

#define WCH_MAX_SEG 50

typedef struct wch_control_t
{
    u8			*temp_data;	// not NULL: temporary data, alloced
    uint		temp_size;	// not NULL: size of 'temp_data'

    bool		is_valid;	// true: data is valid
    wch_header_t	wh;		// decoded header, local endian

    uint		n_seg;		// number of segments excluding terminator
    wch_segment_t seg[WCH_MAX_SEG+1];	// decoded segment headers, local endian
					//  + special terminator segment (type==0)
    u8 *seg_data[WCH_MAX_SEG+1];	// pointer to decoded data for each segment
}
wch_control_t;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef enumError (*DecompressWCH_func)
(
    // return TRUE on success

    wch_control_t	*wc,		// valid data, includes type info
    void		*data,		// source data to decompress
    uint		size		// size of 'data'
);

///////////////////////////////////////////////////////////////////////////////

ccp DecodeWCH
(
    // returns NULL on success, or a short error message

    wch_control_t	*wc,		// store only, (initialized)
    void		*data,		// source data, modified if type != 0
    uint		size,		// size of 'data'
    DecompressWCH_func	decompress	// NULL or decompression function
);

void ResetWCH ( wch_control_t * wc );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DC_LIB_DOL_H 1
