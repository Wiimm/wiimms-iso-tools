
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

#ifndef LIBWBFS_H
#define LIBWBFS_H

#include "file-formats.h"
#include "wiidisc.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

///////////////////////////////////////////////////////////////////////////////

#define WBFS_MAGIC ( 'W'<<24 | 'B'<<16 | 'F'<<8 | 'S' )
#define WBFS_VERSION 1
#define WBFS_NO_BLOCK (~(u32)0)

///////////////////////////////////////////////////////////////////////////////

//  WBFS first wbfs_sector structure:
//
//  -----------
// | wbfs_head |  (hd_sec_sz)
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// | ...       |
// |	       |
//  -----------
// |	       |
// | disc_info |
// |	       |
//  -----------
// |	       |
// |freeblk_tbl|
// |	       |
//  -----------
//

//-----------------------------------------------------------------------------

// callback definition. Return 1 on fatal error
// (callback is supposed to make retries until no hopes..)

typedef int (*rw_sector_callback_t)(void*fp,u32 lba,u32 count,void*iobuf);
typedef void (*progress_callback_t) (u64 done, u64 total, void * callback_data );

//-----------------------------------------------------------------------------

typedef enum wbfs_slot_mode_t
{
    //--- the base info

    WBFS_SLOT_FREE	= 0x00,  // dics slot is unused
    WBFS_SLOT_VALID	= 0x01,  // dics slot is used and disc seems valid
    WBFS_SLOT_INVALID	= 0x02,  // dics slot is used but disc is invalid

    //--- some additionally flags

    WBFS_SLOT_F_SHARED	= 0x04,  // flag: disc is/was sharing a block with other disc
    WBFS_SLOT_F_FREED	= 0x08,  // flag: disc is/was using a 'marked free' block

    //--- more values

    WBFS_SLOT__MASK	= 0x0f,  // mask of all values above
    WBFS_SLOT__USER	= 0x10,  // first free bit for extern usage

} wbfs_slot_mode_t;

extern char wbfs_slot_mode_info[WBFS_SLOT__MASK+1];

//-----------------------------------------------------------------------------

typedef enum wbfs_balloc_mode_t // block allocation mode
{
    WBFS_BA_AUTO,	// let add disc the choice:
			// for small WBFS partitons (<20G) use WBFS_BA_FIRST
			// and all for other WBFS partitons use WBFS_BA_NO_FRAG

    WBFS_BA_FIRST,	// don't find large blocks and use always the first free block
    WBFS_BA_AVOID_FRAG,	// try to avoid fragments

    WBFS_BA_DEFAULT	= WBFS_BA_AUTO

} wbfs_balloc_mode_t;

//-----------------------------------------------------------------------------

typedef struct wbfs_t
{
    wbfs_head_t	* head;

    /* hdsectors, the size of the sector provided by the hosting hard drive */
    u32		hd_sec_sz;
    u8		hd_sec_sz_s;		// the power of two of the last number
    u32		n_hd_sec;		// the number of hd sector in the wbfs partition

    /* standard wii sector (0x8000 bytes) */
    u32		wii_sec_sz;		// always WII_SECTOR_SIZE
    u8		wii_sec_sz_s;		// always 15
    u32		n_wii_sec;
    u32		n_wii_sec_per_disc;	// always WII_MAX_SECTORS

    /* The size of a wbfs sector */
    u32		wbfs_sec_sz;
    u32		wbfs_sec_sz_s;
    u16		n_wbfs_sec;		// this must fit in 16 bit!
    u16		n_wbfs_sec_per_disc;	// size of the lookup table

    u32		part_lba;		// the lba of the wbfs header

    /* virtual methods to read write the partition */
    rw_sector_callback_t read_hdsector;
    rw_sector_callback_t write_hdsector;
    void	* callback_data;

    u16		max_disc;		// maximal number of possible discs
    u32		freeblks_lba;		// the hd sector of the free blocks table
    u32		freeblks_lba_count;	// number of hd sector used by free blocks table
    u32		freeblks_size4;		// size in u32 of free blocks table
    u32		freeblks_mask;		// mask for last used u32 of freeblks
    u32		* freeblks;		// if not NULL: copy of free blocks table

    u8		* block0;		// NULL or copy of wbfs block #0
    u8		* used_block;		// For each WBFS block 1 byte, N='n_wbfs_sec'
					//    0: unused			==> OK
					//    1: normal (=single) usage	==> OK
					//   >1: shared by #N discs	==> BAD!
					//  127: shared by >=127 discs	==> BAD!
					// >128: reserved for internal usage
					//  255: header block #0
    bool	used_block_dirty;	// true: 'used_block' must be written to disc
    wbfs_slot_mode_t	new_slot_err;	// new detected errors
    wbfs_slot_mode_t	all_slot_err;	// all detected errros
    wbfs_balloc_mode_t	balloc_mode;	// block allocation mode

    u16		disc_info_sz;

    u8		* tmp_buffer;		// pre-allocated buffer for unaligned read
    id6_t	*id_list;		// list with all disc ids

    bool	is_dirty;		// if >0: call wbfs_sync() on close
    u32		n_disc_open;		// number of open discs

} wbfs_t;

//-----------------------------------------------------------------------------

typedef struct wbfs_disc_t
{
    wbfs_t		* p;
    wbfs_disc_info_t	* header;	// pointer to wii header
    int			slot;		// disc slot, range= 0 .. wbfs_t::max_disc-1
    wd_disc_type_t	disc_type;	// disc type
    wd_disc_attrib_t	disc_attrib;	// disc attrib
    uint		disc_blocks;	// >0: number of blocks
    uint		n_fragments;	// >0: number of fragments
    bool		is_used;	// disc is marked as 'used'?
    bool		is_valid;	// disc has valid id and magic
    bool		is_deleted;	// disc has valid id and deleted_magic
    bool		is_iinfo_valid;	// disc has a valid wbfs_inode_info_t
    bool		is_creating;	// disc is in creation process
    bool		is_dirty;	// if >0: call wbfs_sync_disc_header() on close

} wbfs_disc_t;

//-----------------------------------------------------------------------------

be64_t	wbfs_setup_inode_info
	( wbfs_t * p, wbfs_inode_info_t * ii, bool is_valid, int is_changed );

int wbfs_is_inode_info_valid
(
    // if valid   -> return WBFS_INODE_INFO_VERSION
    // if invalid -> return 0

    const wbfs_t		* p,	// NULL or WBFS
    const wbfs_inode_info_t	* ii	// NULL or inode pointer
);

//-----------------------------------------------------------------------------

typedef struct wbfs_param_t // function parameters
{
  //----- parameters for wbfs_open_partition_param()

	// call back data
	rw_sector_callback_t	read_hdsector;		// read sector function
	rw_sector_callback_t	write_hdsector;		// write sector function

	// needed parameters
	u32			hd_sector_size;		// HD sector size (0=>512)
	u32			part_lba;		// partition LBA delta
	int			old_wii_sector_calc;	// >0 => use old & buggy calculation
	int			force_mode;		// >0 => no plausibility tests
	int			reset;			// >0 => format disc

	// only used if formatting disc (if reset>0)
	u32			num_hd_sector;		// num of HD sectors
	int			clear_inodes;		// >0 => clear inodes
	int			setup_iinfo;		// >0 => clear inodes & use iinfo
	int			wbfs_sector_size;	// >0 => force wbfs_sec_sz_s


  //----- parameters for wbfs_open_partition_param()
  
	// call back data
	wd_read_func_t		read_src_wii_disc;	// read wit sector [obsolete?]
	progress_callback_t	spinner;		// progress callback
	wbfs_balloc_mode_t	balloc_mode;		// block allocation mode


  //----- parameters for wbfs_add_disc_param()

	u64			iso_size;		// size of iso image in bytes
	wd_disc_t		*wd_disc;		// NULL or the source disc
	const wd_select_t	*psel;			// partition selector
	id6_t			wbfs_id6;		// not NULL: use this ID for inode


  //----- multi use parameters

	// call back data
	void			*callback_data;		// used defined data

	// inode info
	wbfs_inode_info_t	iinfo;			// additional infos


  //----- infos (output of wbfs framework)

	int			slot;			// >=0: slot of last added disc
	wbfs_disc_t		*open_disc;		// NULL or open disc

} wbfs_param_t;

//-----------------------------------------------------------------------------

#ifndef WIT // not used in WiT

wbfs_t * wbfs_open_partition
(
    rw_sector_callback_t	read_hdsector,
    rw_sector_callback_t	write_hdsector,
    void			* callback_data,
    int				hd_sector_size,
    int				num_hd_sector,
    u32				part_lba,
    int				reset
);

#endif

wbfs_t * wbfs_open_partition_param ( wbfs_param_t * par );

int wbfs_calc_size_shift
	( u32 hd_sec_sz_s, u32 num_hd_sector, int old_wii_sector_calc );

u32 wbfs_calc_sect_size ( u64 total_size, u32 hd_sec_size );

void wbfs_calc_geometry
(
    wbfs_t		* p,		// pointer to wbfs_t, p->head must be NULL or valid
    u32			n_hd_sec,	// total number of hd_sec in partition
    u32			hd_sec_sz,	// size of a hd/partition sector
    u32			wbfs_sec_sz	// size of a wbfs sector
);

//-----------------------------------------------------------------------------

void wbfs_close ( wbfs_t * p );
void wbfs_sync  ( wbfs_t * p );

wbfs_disc_t * wbfs_open_disc_by_id6  ( wbfs_t * p, u8 * id6 );
wbfs_disc_t * wbfs_open_disc_by_slot ( wbfs_t * p, u32 slot, int force_open );

wbfs_disc_t * wbfs_create_disc
(
    wbfs_t	* p,		// valid WBFS descriptor
    const void	* disc_header,	// NULL or disc header to copy
    const void	* disc_id	// NULL or ID6: check non existence
				// disc_id overwrites the id of disc_header
);

int wbfs_sync_disc_header ( wbfs_disc_t * d );
void wbfs_close_disc ( wbfs_disc_t * d );

wbfs_inode_info_t * wbfs_get_inode_info ( wbfs_t *p, wbfs_disc_info_t *info, int clear_mode );
wbfs_inode_info_t * wbfs_get_disc_inode_info ( wbfs_disc_t * d, int clear_mode );
	// clear_mode == 0 : don't clear
	// clear_mode == 1 : clear if invalid
	// clear_mode == 2 : clear always

uint wbfs_get_fragments
(
    const u16		* wlba_tab,	// valid wlba table in network byte order
    uint		tab_length,	// length of 'wlba_tab'
    uint		* disc_blocks	// not NULL: store number of disc blocks
);

uint wbfs_get_disc_fragments
(
    wbfs_disc_t		*d,		// valid wbfs disc
    uint		* disc_blocks	// not NULL: store number of disc blocks
);

//-----------------------------------------------------------------------------

int wbfs_rename_disc
(
    wbfs_disc_t		* d,		// pointer to an open disc
    const char		* new_id,	// if !NULL: take the first 6 chars as ID
    const char		* new_title,	// if !NULL: take the first 0x39 chars as title
    int			chg_wbfs_head,	// if !0: change ID/title of WBFS header
    int			chg_iso_head	// if !0: change ID/title of ISO header
);

//-----------------------------------------------------------------------------

int wbfs_touch_disc
(
    wbfs_disc_t		* d,		// pointer to an open disc
    u64			itime,		// if != 0: new itime
    u64			mtime,		// if != 0: new mtime
    u64			ctime,		// if != 0: new ctime
    u64			atime		// if != 0: new atime
);

//-----------------------------------------------------------------------------

void wbfs_print_block_usage
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const wbfs_t	* p,		// valid WBFS descriptor
    bool		print_all	// false: ignore const lines
);

//-----------------------------------------------------------------------------

extern const char wbfs_usage_name_tab[256];
 
void wbfs_print_usage_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const u8		* used_block,	// valid pointer to usage table
    u32			block_used_sz,	// size of 'used_block'
    u32			sector_size,	// wbfs sector size
    bool		print_all	// false: ignore const lines
);

//-----------------------------------------------------------------------------

typedef enum wbfs_check_t
{
    WBFS_CHK_DONE = 0,			// finish message

    WBFS_CHK_UNUSED_BLOCK,		// block marked used, but is not used [block]
    WBFS_CHK_MULTIUSED_BLOCK,		// block used multiple times [block,count]

    WBFS_CHK_INVALID_BLOCK,		// invalid block number [slot,id6,block]
    WBFS_CHK_FREE_BLOCK_USED,		// block marked free but used [slot,id6,block]
    WBFS_CHK_SHARED_BLOCK,		// usage of a shared block by [slot,id6,block,count]

} wbfs_check_t;

//-----------------------------------------------------------------------------

typedef void (*wbfs_check_func)
(
    wbfs_t		* p,		// valid WBFS descriptor
    wbfs_check_t	check_mode,	// modus
    int			slot,		// -1 or related slot index
    ccp			id6,		// NULL or pointer to disc ID6
    int			block,		// -1 or related block number
    uint		count,		// block usage count
    ccp			msg,		// clear text message
    uint		msg_len,	// strlen(msg)
    void		* param		// user defined parameter
);

//-----------------------------------------------------------------------------

int wbfs_calc_used_blocks
(
    wbfs_t		* p,		// valid WBFS descriptor
    bool		force_reload,	// true: definitely reload block #0
    bool		store_block0,	// true: don't free block0
    wbfs_check_func	func,		// call back function for errors
    void		* param		// user defined parameter
);

//-----------------------------------------------------------------------------

u32 wbfs_find_free_blocks
(
    // returns index of first free block or WBFS_NO_BLOCK if not enough blocks free

    wbfs_t	* p,		// valid WBFS descriptor
    u32		n_needed	// number of needed blocks
);

//-----------------------------------------------------------------------------

/*! @return the number of discs inside the paritition */
u32 wbfs_count_discs(wbfs_t*p);

u32 wbfs_alloc_block
(
    wbfs_t		* p,		// valid WBFS descriptor
    u32			start_block	// >0: start search at this block
);

enumError wbfs_get_disc_info
(
    wbfs_t		* p,		// valid wbfs descriptor
    u32			index,		// disc index: 0 .. num_dics-1
    u8			* header,	// header to store data
    int			header_size,	// size of 'header'
    u32			* slot_found,	// not NULL: store slot of found disc
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wd_disc_attrib_t	* disc_attrib,	// not NULL: store disc attrib
    u32			* size4,	// not NULL: store 'size>>2' of found disc
    u32			* n_fragments	// number of wbfs fragments
);

enumError wbfs_get_disc_info_by_slot
(
    wbfs_t		* p,		// valid wbfs descriptor
    u32			slot,		// disc index: 0 .. num_dics-1
    u8			* header,	// not NULL: header to store data
    int			header_size,	// size of 'header'
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wd_disc_attrib_t	* disc_attrib,	// not NULL: store disc attrib
    u32			* size4,	// not NULL: store 'size>>2' of found disc
    u32			* n_fragments	// number of wbfs fragments
);

/*! get the number of unuseds block of the partition.
  to be multiplied by p->wbfs_sec_sz (use 64bit multiplication) to have the number in bytes
*/
u32 wbfs_get_free_block_count ( wbfs_t * p );

/******************* write access  ******************/

id6_t * wbfs_load_id_list	( wbfs_t * p, int force_reload );
int  wbfs_find_slot		( wbfs_t * p, const u8 * disc_id );

u32 * wbfs_free_freeblocks	( wbfs_t * p );
u32 * wbfs_load_freeblocks	( wbfs_t * p );
void wbfs_free_block		( wbfs_t * p, u32 bl );
void wbfs_use_block		( wbfs_t * p, u32 bl );
u32 wbfs_find_last_used_block	( wbfs_t * p );

/*! add a wii dvd inside the partition
  @param read_src_wii_disc: a callback to access the wii dvd. offsets are in 32bit, len in bytes!
  @callback_data: private data passed to the callback
  @spinner: a pointer to a function that is regulary called to update a progress bar.
  @sel: selects which partitions to copy.
  @copy_1_1: makes a 1:1 copy, whenever a game would not use the wii disc format, and some data is hidden outside the filesystem.
 */
u32 wbfs_add_disc
(
    wbfs_t		* p,
    wd_read_func_t	read_src_wii_disc,
    void		* callback_data,
    progress_callback_t	spinner,
    const wd_select_t	* psel,
    int			copy_1_1
);

u32 wbfs_add_disc_param ( wbfs_t * p, wbfs_param_t * par );

u32 wbfs_add_phantom ( wbfs_t *p, ccp phantom_id, u32 wii_sectors );

// remove a disc from partition
u32 wbfs_rm_disc
(
    wbfs_t		* p,		// valid WBFS descriptor
    u8			* discid,	// id6 to remove. If NULL: remove 'slot'
    int			slot,		// slot index, only used if 'discid==NULL'
    int			free_slot_only	// true: do not free blocks
);

/*! trim the file-system to its minimum size
  This allows to use wbfs as a wiidisc container
 */
u32 wbfs_trim(wbfs_t*p);

/* OS specific functions provided by libwbfs_<os>.c */

wbfs_t *wbfs_try_open(char *disk, char *partition, int reset);
wbfs_t *wbfs_try_open_partition(char *fn, int reset);

void *wbfs_open_file_for_read(char*filename);
void *wbfs_open_file_for_write(char*filename);
int wbfs_read_file(void*handle, int len, void *buf);
void wbfs_close_file(void *handle);
void wbfs_file_reserve_space(void*handle,long long size);
void wbfs_file_truncate(void *handle,long long size);
int wbfs_read_wii_file(void *_handle, u32 _offset, u32 count, void *buf);
int wbfs_write_wii_sector_file(void *_handle, u32 lba, u32 count, void *buf);

///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // LIBWBFS_H
