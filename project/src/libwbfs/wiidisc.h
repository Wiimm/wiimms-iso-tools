
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

#ifndef WIIDISC_H
#define WIIDISC_H

#include "cert.h"
#include "rijndael.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Introduction			///////////////
///////////////////////////////////////////////////////////////////////////////

/****************************************************************************
 *									    *
 *  This is a new wiidisc lib written by Dirk Clemens. It support GameCube  *
 *  and Wii ISO images. If the name of a offset or size variable ends with  *
 *  a '4' than the real value must be calculated with: (u64)value4<<2	    *
 *  But this not true for GameCube discs. GameCube discs store the	    *
 *  unshifted value.							    *
 *									    *
 ****************************************************************************/

//
///////////////////////////////////////////////////////////////////////////////
///////////////			conditionals			///////////////
///////////////////////////////////////////////////////////////////////////////

// search and load 'sys/user.bin'  =>  0=off, 1=readonly, 2=read+create
#define SUPPORT_USER_BIN	2

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_part_type_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_part_type_t // known partition types
{
    WD_PART_DATA	=  0,	// DATA partition
    WD_PART_UPDATE	=  1,	// UPDATE partition
    WD_PART_CHANNEL	=  2,	// CHANNEL partition

} wd_part_type_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_ckey_index_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef TEST
  #define SUPPORT_CKEY_DEVELOP 1
#else
  #define SUPPORT_CKEY_DEVELOP 0
#endif

typedef enum wd_ckey_index_t // known partition types
{
    WD_CKEY_STANDARD,		// standard common key
    WD_CKEY_KOREA,		// common key for korea
 #ifdef SUPPORT_CKEY_DEVELOP
    WD_CKEY_DEVELOPER,		// common key type 'developer'
 #endif
    WD_CKEY__N			// number of common keys

} wd_ckey_index_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_select_mode_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_select_mode_t // modes of selection
{
    WD_SM_IGNORE,		// ignore this selection

    //--- allow modes

    WD_SM_ALLOW_PTYPE,		// allow partition type
    WD_SM_ALLOW_PTAB,		// allow partition table
    WD_SM_ALLOW_INDEX,		// allow partition #index in absolut partition count
    WD_SM_ALLOW_LT_INDEX,	// allow partitions < #index in absolut partition count
    WD_SM_ALLOW_GT_INDEX,	// allow partitions > #index in absolut partition count
    WD_SM_ALLOW_PTAB_INDEX,	// allow partition #index in given table
    WD_SM_ALLOW_ID,		// allow ID
    WD_SM_ALLOW_GC_BOOT,	// allow GameCube boot partition
    WD_SM_ALLOW_ALL,		// allow all partitions

    WD_SM_N_MODE,		// number of modes

    //--- the deny flag and masks

    WD_SM_F_DENY = 0x100,	// this bit is set for DENY modes
    WD_SM_M_MODE = 0x0ff,	// mask for base modes

    //--- deny modes

    WD_SM_DENY_PTYPE		= WD_SM_F_DENY | WD_SM_ALLOW_PTYPE,
    WD_SM_DENY_PTAB		= WD_SM_F_DENY | WD_SM_ALLOW_PTAB,
    WD_SM_DENY_INDEX		= WD_SM_F_DENY | WD_SM_ALLOW_INDEX,
    WD_SM_DENY_LT_INDEX		= WD_SM_F_DENY | WD_SM_ALLOW_LT_INDEX,
    WD_SM_DENY_GT_INDEX		= WD_SM_F_DENY | WD_SM_ALLOW_GT_INDEX,
    WD_SM_DENY_PTAB_INDEX	= WD_SM_F_DENY | WD_SM_ALLOW_PTAB_INDEX,
    WD_SM_DENY_ID		= WD_SM_F_DENY | WD_SM_ALLOW_ID,
    WD_SM_DENY_GC_BOOT		= WD_SM_F_DENY | WD_SM_ALLOW_GC_BOOT,
    WD_SM_DENY_ALL		= WD_SM_F_DENY | WD_SM_ALLOW_ALL,

} wd_select_mode_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_trim_mode_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_trim_mode_t
{
	//--- main trimming modes

	WD_TRIM_DEFAULT	= 0x001,	// default mode (no user value)

	WD_TRIM_DISC	= 0x002,	// trim disc: move whole partitions
	WD_TRIM_PART	= 0x004,	// trim partition: move sectors
	WD_TRIM_FST	= 0x008,	// trim filesystem: move files

	WD_TRIM_NONE	= 0x000,
	WD_TRIM_ALL	= WD_TRIM_DISC | WD_TRIM_PART | WD_TRIM_FST,
	WD_TRIM_FAST	= WD_TRIM_DISC | WD_TRIM_PART,

	//--- trimming flags

	WD_TRIM_F_END	= 0x100,	// flags for WD_TRIM_DISC: move to disc end

	WD_TRIM_M_FLAGS	= 0x100,

	//--- all valid bits

	WD_TRIM_M_ALL	= WD_TRIM_DEFAULT | WD_TRIM_ALL | WD_TRIM_M_FLAGS

} wd_trim_mode_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_icm_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_icm_t // iterator call mode
{
    WD_ICM_OPEN_PART,		// called for each openening of a partition
    WD_ICM_CLOSE_PART,		// called if partitions closed

    WD_ICM_DIRECTORY,		// item is a directory; this is the first non hint message

    WD_ICM_FILE,		// item is a partition file -> extract with key
    WD_ICM_COPY,		// item is a disc area -> disc raw copy
    WD_ICM_DATA,		// item contains pure data

} wd_icm_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_ipm_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_ipm_t // iterator prefix mode
{
    WD_IPM_DEFAULT,		// not defined -> like AUTO in wiidisc
    WD_IPM_AUTO,		// for single partitions: WD_IPM_PART_NAME, else: WD_IPM_POINT

    WD_IPM_NONE,		// no prefix: ""
    WD_IPM_SLASH,		// prefix with "/"
    WD_IPM_POINT,		// prefix with "./"
    WD_IPM_PART_ID,		// prefix with 'P' and partition id: "P%u/"
    WD_IPM_PART_NAME,		// prefix with partition name or "P<id>": "NAME/"
    WD_IPM_PART_INDEX,		// prefix with 'P' and table + partition index: "P%u.%u/"
    WD_IPM_COMBI,		// WD_IPM_PART_INDEX + ID|NAME: "P%u.%u-%s/"

} wd_ipm_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_pfst_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_pfst_t // print-fst mode
{
    WD_PFST_HEADER	= 0x01,  // print table header
    WD_PFST_PART	= 0x02,  // print partition intro
    WD_PFST_UNUSED	= 0x04,  // print unused area
    WD_PFST_OFFSET	= 0x08,  // print offset
    WD_PFST_SIZE_HEX	= 0x10,  // print size in hex
    WD_PFST_SIZE_DEC	= 0x20,  // print size in dec

    WD_PFST__ALL	= 0x3f,

    WD_PFST__OFF_SIZE	= WD_PFST_UNUSED
			| WD_PFST_OFFSET
			| WD_PFST_SIZE_HEX
			| WD_PFST_SIZE_DEC,

} wd_pfst_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    enum wd_sector_status_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_sector_status_t
{
    //----- cleared status

    WD_SS_HASH_CLEARED		= 0x0001,   // hash area cleared with any constant
    WD_SS_DATA_CLEARED		= 0x0002,   // data area cleared with any constant
    WD_SS_SECTOR_CLEARED	= 0x0004,   // complete sector cleared with any constant
					    // if set: WD_SS_HASH_CLEARED
					    //	     & WD_SS_DATA_CLEARED are set too

    WD_SS_HASH_ZEROED		= 0x0010,   // hash area zeroed ==> WD_SS_HASH_CLEARED
    WD_SS_DATA_ZEROED		= 0x0020,   // data area zeroed ==> WD_SS_DATA_CLEARED

    WD_SS_M_CLEARED		= WD_SS_HASH_CLEARED
				| WD_SS_DATA_CLEARED
				| WD_SS_SECTOR_CLEARED
				| WD_SS_HASH_ZEROED
				| WD_SS_DATA_ZEROED,

    //----- encryption status

    WD_SS_ENCRYPTED		= 0x0040,   // sector encrypted (decrypted H0 hash is ok)
    WD_SS_DECRYPTED		= 0x0080,   // sector decrypted (H0 hash is ok)

    WD_SS_M_CRYPT		= WD_SS_ENCRYPTED
				| WD_SS_DECRYPTED,

    //----- flags

    WD_SS_F_PART_DATA		= 0x0100,   // sector contains partition data
    WD_SS_F_SCRUBBED		= 0x0200,   // sector is scrubbed
    WD_SS_F_SKELETON		= 0x0400,   // sector is marked 'SKELETON'

    WD_SS_M_FLAGS		= WD_SS_F_PART_DATA
				| WD_SS_F_SCRUBBED
				| WD_SS_F_SKELETON,

    //----- error status

    WD_SS_INVALID_SECTOR	= 0x1000,   // invalid sector index
    WD_SS_READ_ERROR		= 0x2000,   // error while reading sector

    WD_SS_M_ERROR		= WD_SS_INVALID_SECTOR
				| WD_SS_READ_ERROR,

} wd_sector_status_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_scrubbed_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_scrubbed_t // scrubbing mode
{
    WD_SCRUBBED_NONE	= 0,
    WD_SCRUBBED_HASH	= 1,
    WD_SCRUBBED_DATA	= 2,
    WD_SCRUBBED_ALL	= WD_SCRUBBED_HASH | WD_SCRUBBED_DATA,

} wd_scrubbed_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_usage_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_usage_t // usage table values
{
    WD_USAGE_UNUSED,		// block is not used, always = 0
    WD_USAGE_DISC,		// block is used for disc management
    WD_USAGE_PART_0,		// index for first partition

    WD_USAGE__MASK	= 0x7f,	// mask for codes above
    WD_USAGE_F_CRYPT	= 0x80,	// flag: encryption candidate

} wd_usage_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_pname_mode_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_pname_mode_t // partition name modes
{
    WD_PNAME_NAME_NUM_9,	// NAME+NUM if possible, ID or NUM else, fw=9
    WD_PNAME_COLUMN_9,		// NAME+NUM|NUM|ID, fw=9
    WD_PNAME_NUM,		// ID or NUM
    WD_PNAME_NAME,		// NAME if possible, ID or NUM else
    WD_PNAME_P_NAME,		// NAME if possible, 'P-'ID or 'P'NUM else
    WD_PNAME_NUM_INFO,		// NUM + '[INFO]'

    WD_PNAME__N			// number of supported formats

} wd_pname_mode_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_patch_mode_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_patch_mode_t // patching modes
{
    WD_PAT_IGNORE	= 0,	// non used value, always zero

    WD_PAT_ZERO,		// zero data area
    WD_PAT_DATA,		// known raw data
    WD_PAT_PART_TICKET,		// partition TICKET (-> fake sign)
    WD_PAT_PART_TMD,		// partition TMD (-> fake sign)

    WD_PAT__N			// number of valid codes

} wd_patch_mode_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_reloc_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_reloc_t // relocation modes
{
    WD_RELOC_M_DELTA	=    0x7ffff,	// mask for source sector delta

    WD_RELOC_M_PART	=  0x3f80000,	// mask for partition index
    WD_RELOC_S_PART	=         19,	// shift for partition index

    WD_RELOC_F_ENCRYPT	= 0x04000000,	// set if partition data, encrypted
    WD_RELOC_F_DECRYPT	= 0x08000000,	// set if partition data, decrypted
    WD_RELOC_F_COPY	= 0x10000000,	// set if source must be copied
    WD_RELOC_F_PATCH	= 0x20000000,	// set if patching is needed
    WD_RELOC_F_HASH	= 0x40000000,	// set if hash calculation is needed
    WD_RELOC_F_LAST	= 0x80000000,	// set if must be copied after all other
    WD_RELOC_M_FLAGS	= 0xfc000000,	// sum of all flags

    WD_RELOC_M_CRYPT	= WD_RELOC_F_ENCRYPT | WD_RELOC_F_DECRYPT,

    WD_RELOC_M_DISCLOAD	= WD_RELOC_M_DELTA
			| WD_RELOC_M_PART
			| WD_RELOC_F_ENCRYPT
			| WD_RELOC_F_DECRYPT
			| WD_RELOC_F_COPY,

    WD_RELOC_M_PARTLOAD	= WD_RELOC_M_DELTA
			| WD_RELOC_M_PART
			| WD_RELOC_F_ENCRYPT
			| WD_RELOC_F_DECRYPT,

} wd_reloc_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_modify_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_modify_t // objects to modify
{
    //--- standard values

    WD_MODIFY_DISC	= 0x001,  // modify disc header
    WD_MODIFY_BOOT	= 0x002,  // modify boot.bin
    WD_MODIFY_TICKET	= 0x004,  // modify ticket.bin
    WD_MODIFY_TMD	= 0x008,  // modify tmd.bin

    //--- for external extensions

    WD_MODIFY_WBFS	= 0x010,  // modify WBFS inode [obsolete?]

    //--- special flags

    WD_MODIFY__NONE	=     0,  // modify nothing
    WD_MODIFY__AUTO	= 0x100,  // automatic mode
    WD_MODIFY__ALWAYS	= 0x200,  // this bit is always set

    //--- summary

    WD_MODIFY__TT	= WD_MODIFY_TICKET|WD_MODIFY_TMD,
    WD_MODIFY__ALL	= 0x01f, // modify all
    WD_MODIFY__MASK	= WD_MODIFY__ALL | WD_MODIFY__AUTO | WD_MODIFY__ALWAYS,

} wd_modify_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			callback functions		///////////////
///////////////////////////////////////////////////////////////////////////////

struct wd_disc_t;
struct wd_part_t;
struct wd_iterator_t;
struct wd_memmap_t;
struct wd_memmap_item_t;

//-----------------------------------------------------------------------------
// Callback read function. Returns 0 on success and any other value on failutre.

typedef int (*wd_read_func_t)
(
    void		* read_data,	// user defined data
    u32			offset4,	// offset/4 to read
    u32			count,		// num of bytes to read
    void		* iobuf		// buffer, alloced with wbfs_ioalloc
);

//-----------------------------------------------------------------------------
// Callback definition for memory dump with wd_print_mem()

typedef void (*wd_mem_func_t)
(
    void		* param,	// user defined parameter
    u64			offset,		// offset of object
    u64			size,		// size of object
    ccp			info		// info about object
);

//-----------------------------------------------------------------------------
// Callback definition for file iteration. if return != 0 => abort

typedef int (*wd_file_func_t)
(
    struct wd_iterator_t	*it	// iterator struct with all infos
);

//-----------------------------------------------------------------------------
// Callback definition for wd_insert_memmap_disc_part() & wd_insert_memmap_part()

typedef void (*wd_memmap_func_t)
(
    void			* param,	// user defined parameter
    struct wd_memmap_t		* patch,	// valid pointer to patch object
    struct wd_memmap_item_t	* item		// valid pointer to inserted item
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wd_select_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_select_item_t]]

typedef struct wd_select_item_t // a select sub item
{
    wd_select_mode_t	mode;		// select mode
    u32			table;		// partition table
    u32			part;		// partition type or index

} wd_select_item_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_select_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_select_t]]

typedef struct wd_select_t // a selector
{
    bool		whole_disc;	// flag: copy whole disc, ignore others
    bool		whole_part;	// flag: copy whole partitions

    u32			used;		// number of used elements in list
    u32			size;		// number of alloced elements in list
    wd_select_item_t	* list;		// list of select items

} wd_select_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_memmap_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_memmap_item_t]]

typedef struct wd_memmap_item_t // patching data item
{
    u32			mode;		// memmap mode (e.g. wd_patch_mode_t)
    u32			index;		// user defined index (e.g. partition index)
    u64			offset;		// offset
    u64			size;		// size
    void		* data;		// NULL or pointer to data/filename
    bool		data_alloced;	// true if data must be freed
    char		info[51];	// comment for dumps

} wd_memmap_item_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_memmap_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_memmap_t]]

typedef struct wd_memmap_t // patching data structure
{
    wd_memmap_item_t	* item;		// pointer to first item
    u32			used;		// number of used items
    u32			size;		// number of allocated items

} wd_memmap_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_file_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_file_t]]

typedef struct wd_file_t
{
    u32			src_off4;		// offset of source in 4*bytes steps
    u32			dest_off4;		// offset of dest in 4*bytes steps
    u32			size;			// size of file
    ccp			iso_path;		// path within iso image, never NULL
    ccp			file_path;		// NULL or path of file to load

    // For directories all values but iso_path are NULL
    // and the 'iso_path' terminates with a slash ('/')
    // For files the value of 'dest_off4' is never NULL.
    // Use the function wd_file_is_directory() to distinguish between both.

} wd_file_t;


//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_file_list_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_file_list_t]]

typedef struct wd_file_list_t
{
    wd_file_t		* file;			// list with 'size' files
    u32			used;			// number of used and
						// initialized elements in 'list'
    u32			size;			// number of alloced elements in 'list'

} wd_file_list_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_part_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_part_t]]

typedef struct wd_part_t
{
    //----- base infos

    int			index;		// zero based index within wd_disc_t
    wd_usage_t		usage_id;	// marker for usage_table
    int			ptab_index;	// zero based index of owning partition table
    int			ptab_part_index;// zero based index within owning partition table
    u32			part_type;	// partition type

    u32			part_off4;	// offset/4 of partition relative to disc start
    u64			max_marked;	// max marked offset relative to disc start
    u64			part_size;	// total size of partition

    struct wd_disc_t	* disc;		// pointer to disc
    wd_memmap_t		patch;		// patching data


    //----- partition status

    bool		is_loaded;	// true if this partition info was loaded
    bool		is_valid;	// true if this partition is valid
    bool		is_enabled;	// true if this partition is enabled
    bool		is_ok;		// true if is_loaded && is_valid && is_enabled
    bool		is_encrypted;	// true if this partition is encrypted
    bool		is_overlay;	// true if this partition overlays other partitions
    bool		is_gc;		// true for GC partition => no crypt, no hash

    wd_sector_status_t	sector_stat;	// sector status: OR'ed for different sectors
    bool		scrub_test_done;// true if scrubbing test already done


    //----- to do status

    bool		sign_ticket;	// true if ticket must be signed
    bool		sign_tmd;	// true if tmd must be signed
    bool		h3_dirty;	// true if h3 is dirty -> calc h4 and sign tmd


    //----- partition data, only valid if 'is_valid' is true

    wd_part_header_t	ph		// partition header (incl. ticket), host endian
	    __attribute__ ((aligned(4)));
    wd_tmd_t		* tmd;		// NULL or pointer to tmd, size = ph.tmd_size
    u8			* cert;		// NULL or pointer to cert, size = ph.cert_size
    u8			* h3;		// NULL or pointer to h3, size = WII_H3_SIZE

    char		* setup_txt;	// NULL or pointer to content of file setup.txt
    u32			setup_txt_len;	// = strlen(setup_txt)
    char		* setup_sh;	// NULL or pointer to content of file setup.sh
    u32			setup_sh_len;	// = strlen(setup_sh)
    char		* setup_bat;	// NULL or pointer to content of file setup.bat
    u32			setup_bat_len;	// = strlen(setup_bat)

    cert_chain_t	cert_chain;	// list of ceritificates
    cert_stat_t		cert_tik_stat;	// certificate state of ticket, NULL=not set
    cert_stat_t		cert_tmd_stat;	// certificate state of tmd, NULL=not set

    u8			key[WII_KEY_SIZE];
					// partition key, needed to build aes key
    u32			data_off4;	// offset/4 of partition data relative to disc start
    u32			data_sector;	// index of first data sector
    u32			end_mgr_sector;	// index of last management sector + 1
    u32			end_sector;	// index of last data sector + 1

    wd_boot_t		boot;		// copy of boot.bin, host endian
    u32			dol_size;	// size of main.dol
    u32			apl_size;	// size of apploader.img
    u32			region;		// gamecube region code, extract of bi2

 #if SUPPORT_USER_BIN > 0
    u32			ubin_off4;	// NULL or offset/4 of user.bin
    u32			ubin_size;	// NULL or sife of user.bin
 #endif

    wd_fst_item_t	* fst;		// pointer to fst data
    u32			fst_n;		// number or elements in fst
    u32			fst_max_off4;	// informative: maximal offset4 value of all files
    u32			fst_max_size;	// informative: maximal size value of all files
    u32			fst_dir_count;	// informative: number or directories in fst
    u32			fst_file_count;	// informative: number or real files in fst

} wd_part_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_disc_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_disc_t]]

typedef struct wd_disc_t
{
    //----- open parameters

    wd_read_func_t	read_func;	// read function, always valid
    void		* read_data;	// data pointer for read function
    u64			iso_size;	// size of file, 0=unknown
    int			open_count;	// open counter
    int			force;		// >0: force opening even if invalid

    //----- errror support

    char		error_term[200]; // termination string for error messages

    //----- raw data

    wd_disc_type_t	disc_type;	// disc type
    wd_disc_attrib_t	disc_attrib;	// disc attrib

    wd_header_t		dhead;		// copy of disc header
    wd_region_t		region;		// copy of disc region settings
    wd_ptab_info_t	ptab_info[WII_MAX_PTAB];
					// copy of partion table info
    wd_ptab_entry_t	*ptab_entry;	// collected copy of all partion entries
    u32			magic2;		// second magic @ WII_MAGIC2_OFF

    //----- user data

    ccp			image_type;	// NULL or string of image type
    ccp			image_ext;	// NULL or recommended image extension

    //----- patching data

    bool		whole_disc;	// selection flag: copy whole disc (raw mode)
    bool		whole_part;	// selection flag: copy whole partitions
    wd_trim_mode_t	trim_mode;	// active trim mode
    u32			trim_align;	// alignment value for trimming
    wd_memmap_t		patch;		// patching data
    wd_ptab_t		ptab;		// partition tables / GC: copy of dhead
    wd_reloc_t		* reloc;	// relocation data

    //----- partitions

    int			n_ptab;		// number of valid partition tables
    int			n_part;		// number of partitions
    wd_part_t		* part;		// list of partitions (N='n_part')
    wd_part_t		* data_part;	// NULL or pointer to first DATA partition
    wd_part_t		* update_part;	// NULL or pointer to first UPDATE partition
    wd_part_t		* channel_part;	// NULL or pointer to first CHANNEL partition
    wd_part_t		* main_part;	// NULL or pointer to main partition
					// This is the first of:
					//   first DATA partition
					//   first UPDATE partition
					//   first CHANNEL partition
					//   first partition at all
    u32			invalid_part;	// number of invalid partitions
    u32			fst_n;		// informative: number or elements in fst
    u32			fst_max_off4;	// informative: maximal offset4 value of all files
    u32			fst_max_size;	// informative: maximal size value of all files
    u32			fst_dir_count;	// informative: number or directories in fst
    u32			fst_file_count;	// informative: number or real files in fst
    u32			system_menu;	// version of system menu file in update partition

    cert_stat_t		cert_summary;	// summary of partitions stats
    wd_sector_status_t	sector_stat;	// sector status: OR'ed for different sectors
    bool		scrub_test_done;// true if scrubbing test already done
    bool		have_overlays;	// overlayed partitions found
    bool		patch_ptab_recommended;
					// informative: patch ptab is recommended

    //----- usage table

    u8			usage_table[WII_MAX_SECTORS];
					// usage table of disc
    int			usage_max;	// ( max used index of 'usage_table' ) + 1

    //----- block cache

    u8			block_buf[WII_SECTOR_DATA_SIZE];
					// cache for partition blocks
    u32			block_num;	// block number of last loaded 'block_buf'
    wd_part_t		* block_part;	// partition of last loaded 'block_buf'

    //----- akey cache

    aes_key_t		akey;		// aes key for 'akey_part'
    wd_part_t		* akey_part;	// partition of 'akey'

    //----- temp buffer

    u8	temp_buf[2*WII_SECTOR_SIZE];	// temp buffer for reading operations

    u32		cache_sector;		// sector number of 'cache'
    u8		cache[WII_SECTOR_SIZE];	// cache for wd_read_and_patch()

    wd_part_t	* group_cache_part;	// parttion of 'group_cache'
    u32		  group_cache_sector;	// sector number of 'group_cache'
    u8		* group_cache;		// cache for sector groups


} wd_disc_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_iterator_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_iterator_t]]

typedef struct wd_iterator_t
{
    //----- global parameters

    void		* param;	// user defined parameter
    wd_disc_t		* disc;		// valid disc pointer
    wd_part_t		* part;		// valid disc partition pointer

    //----- settings

    bool		select_mode;	// true: run in select mode
					//   func() return 0: don't select
					//   func() return 1: select and mark
					//   func() return other: abort

    //----- file specific parameters

    wd_icm_t		icm;		// iterator call mode
    u32			off4;		// offset/4 to read (GC: offset/1)
    u32			size;		// size of object
    const void		* data;		// NULL or pointer to data
    wd_fst_item_t	* fst_item;	// NULL or pointer to FST item

    //----- filename

    wd_ipm_t		prefix_mode;	// prefix mode
    char		prefix[20];	// prefix of path
    int			prefix_len;	// := strlen(prefix)
    char		path[2000];	// full path including prefix
    char		*fst_name;	// pointer := path + prefix_len

} wd_iterator_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_print_fst_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[wd_print_fst_t]]

typedef struct wd_print_fst_t
{
	//----- base data

	FILE		* f;		// valid output file
	int		indent;		// indention of the output
	wd_pfst_t	mode;		// print mode

	//----- field widths and opther helper data

	int		fw_unused;	// field width or 0 if hidden
	int		fw_offset;	// field width or 0 if hidden
	int		fw_size_dec;	// field width or 0 if hidden
	int		fw_size_hex;	// field width or 0 if hidden

	u64		last_end;	// 'offset + size' of last printed element
	wd_icm_t	last_icm;	// 'icm' of last printed element

	//----- filter function, used by wd_print_fst_item_wrapper()

	wd_file_func_t	filter_func;	// NULL or filter function
	void		* filter_param;	// user defined parameter

} wd_print_fst_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: common key & encryption	///////////////
///////////////////////////////////////////////////////////////////////////////

const u8 * wd_get_common_key
(
    wd_ckey_index_t	ckey_index	// index of common key
);

//-----------------------------------------------------------------------------

const u8 * wd_set_common_key
(
    wd_ckey_index_t	ckey_index,	// index of common key
    const void		* new_key	// the new key
);

//-----------------------------------------------------------------------------

void wd_decrypt_title_key
(
    const wd_ticket_t	* tik,		// pointer to ticket
    u8			* title_key	// the result
);

//-----------------------------------------------------------------------------

void wd_encrypt_title_key
(
    wd_ticket_t		* tik,		// pointer to ticket (modified)
    const u8		* title_key	// valid pointer to wanted title key
);

//-----------------------------------------------------------------------------

void wd_decrypt_sectors
(
    wd_part_t		* part,		// NULL or pointer to partition
    const aes_key_t	* akey,		// aes key, if NULL use 'part'
    const void		* sect_src,	// pointer to first source sector
    void		* sect_dest,	// pointer to first destination sector
    void		* hash_dest,	// if not NULL: store hash tables here
    u32			n_sectors	// number of wii sectors to decrypt
);

//-----------------------------------------------------------------------------

void wd_encrypt_sectors
(
    wd_part_t		* part,		// NULL or pointer to partition
    const aes_key_t	* akey,		// aes key, if NULL use 'part'
    const void		* sect_src,	// pointer to first source sector
    const void		* hash_src,	// if not NULL: source of hash tables
    void		* sect_dest,	// pointer to first destination sector
    u32			n_sectors	// number of wii sectors to encrypt
);

//-----------------------------------------------------------------------------

void wd_split_sectors
(
    const void		* sect_src,	// pointer to first source sector
    void		* data_dest,	// pointer to data destination
    void		* hash_dest,	// pointer to hash destination
    u32			n_sectors	// number of wii sectors to decrypt
);

//-----------------------------------------------------------------------------

void wd_join_sectors
(
    const void		* data_src,	// pointer to data source
    const void		* hash_src,	// pointer to hash source
    void		* sect_dest,	// pointer to first destination sector
    u32			n_sectors	// number of wii sectors to encrypt
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: names, ids, titles, ...	///////////////
///////////////////////////////////////////////////////////////////////////////

extern const char * wd_disc_type_name[];
extern const char * wd_part_name[];

//-----------------------------------------------------------------------------

const char * wd_get_disc_type_name
(
    wd_disc_type_t	disc_type,	// disc type
    ccp			not_found	// result if no name found
);

//-----------------------------------------------------------------------------

const char * wd_get_part_name
(
    u32			ptype,		// partition type
    ccp			not_found	// result if no name found
);

//-----------------------------------------------------------------------------

char * wd_print_part_name
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			ptype,		// partition type
    wd_pname_mode_t	mode		// print mode
);

//-----------------------------------------------------------------------------
// returns a pointer to a printable ID, teminated with 0

char * wd_print_id
(
    const void		* id,		// ID to convert in printable format
    size_t		id_len,		// len of 'id'
    void		* buf		// Pointer to buffer, size >= id_len + 1
					// If NULL, a local circulary static buffer is used
);

//-----------------------------------------------------------------------------
// returns a pointer to a printable ID with colors, teminated with 0

char * wd_print_id_col
(
    const void		* id,		// NULL | EMPTY | ID to convert
    size_t		id_len,		// len of 'id'
    const void		* ref_id,	// reference ID
    const ColorSet_t	*colset		// NULL or color set
);

//-----------------------------------------------------------------------------
// rename ID and title of a ISO header

int wd_rename
(
    void		* data,		// pointer to ISO data
    ccp			new_id,		// if !NULL: take the first 6 chars as ID
    ccp			new_title	// if !NULL: take the first 0x39 chars as title
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: read functions		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_read_raw
(
    wd_disc_t		* disc,		// valid disc pointer
    u32			disc_offset4,	// disc offset/4
    void		* dest_buf,	// destination buffer
    u32			read_size,	// number of bytes to read 
    wd_usage_t		usage_id	// not 0: mark usage usage_table with this value
);

//-----------------------------------------------------------------------------

enumError wd_read_part_raw
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			offset4,	// offset/4 to partition start
    void		* dest_buf,	// destination buffer
    u32			read_size,	// number of bytes to read 
    bool		mark_block	// true: mark block in 'usage_table'
);

//-----------------------------------------------------------------------------

enumError wd_read_part_block
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			block_num,	// block number of partition
    u8			* block,	// destination buf
    bool		mark_block	// true: mark block in 'usage_table'
);

//-----------------------------------------------------------------------------

enumError wd_read_part
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			data_offset4,	// partition data offset/4
    void		* dest_buf,	// destination buffer
    u32			read_size,	// number of bytes to read 
    bool		mark_block	// true: mark block in 'usage_table'
);

//-----------------------------------------------------------------------------

void wd_mark_part
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			data_offset4,	// partition data offset/4
    u32			data_size	// number of bytes to mark
);

//-----------------------------------------------------------------------------

u64 wd_calc_disc_offset
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u64			data_offset4	// partition data offset
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get sector status		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_sector_status_t wd_get_part_sector_status
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			block_num,	// block number of disc
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

wd_sector_status_t wd_get_disc_sector_status
(
    wd_disc_t		* disc,		// valid disc pointer
    u32			block_num,	// block number of partition
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

ccp wd_log_sector_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_sector_status_t	sect_stat	// sector status
);

//-----------------------------------------------------------------------------

ccp wd_print_sector_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_sector_status_t	sect_stat,	// sector status
    cert_stat_t		sig_stat	// not NULL: add signature status
);

//-----------------------------------------------------------------------------

int wd_is_block_encrypted
(
    // returns -1 on read error

    wd_part_t		* part,		// valid pointer to a disc partition
    u32			block_num,	// block number of partition
    int			unknown_result,	// result if status is unknown
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

bool wd_is_part_scrubbed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

bool wd_is_disc_scrubbed
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		silent		// true: don't print error messages
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: open and close		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_disc_t * wd_open_disc
(
    wd_read_func_t	read_func,	// read function, always valid
    void		* read_data,	// data pointer for read function
    u64			iso_size,	// size of iso file, unknown if 0
    ccp			file_name,	// used for error messages if not NULL
    int			force,		// force level
    enumError		* error_code	// store error code if not NULL
);

//-----------------------------------------------------------------------------

wd_disc_t * wd_dup_disc
(
    wd_disc_t		* disc		// NULL or a valid disc pointer
);

//-----------------------------------------------------------------------------

void wd_close_disc
(
    wd_disc_t		* disc		// NULL or valid disc pointer
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: get status		///////////////
///////////////////////////////////////////////////////////////////////////////

bool wd_disc_has_ptab
(
    wd_disc_t		*disc		// valid disc pointer
);

//-----------------------------------------------------------------------------

bool wd_disc_has_region_setting
(
    wd_disc_t		*disc		// valid disc pointer
);

//-----------------------------------------------------------------------------

bool wd_part_has_ticket
(
    wd_part_t		*part		// valid disc partition pointer
);

//-----------------------------------------------------------------------------

bool wd_part_has_tmd
(
    wd_part_t		*part		// valid disc partition pointer
);

//-----------------------------------------------------------------------------

bool wd_part_has_cert
(
    wd_part_t		*part		// valid disc partition pointer
);

//-----------------------------------------------------------------------------

bool wd_part_has_h3
(
    wd_part_t		*part		// valid disc partition pointer
);

//-----------------------------------------------------------------------------

uint wd_disc_is_encrypted
(
    // return
    //    0: no partition is encrypted
    //    N: some (=N) partitions are encrypted, but not all
    // 1000: all partitions are encrypted
    
    wd_disc_t		* disc		// disc to check, valid
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: get partition		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_index
(
	wd_disc_t	* disc,		// valid disc pointer
	int		index,		// zero based partition index within wd_disc_t
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//-----------------------------------------------------------------------------

wd_part_t * wd_get_part_by_usage
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		usage_id,	// value of 'usage_table'
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//-----------------------------------------------------------------------------

wd_part_t * wd_get_part_by_ptab
(
	wd_disc_t	* disc,		// valid disc pointer
	int		ptab_index,	// zero based index of partition table
	int		ptab_part_index,// zero based index within owning partition table
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//-----------------------------------------------------------------------------

wd_part_t * wd_get_part_by_type
(
	wd_disc_t	* disc,		// valid disc pointer
	u32		part_type,	// find first partition with this type
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: load partition data		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_load_part
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		load_cert,	// true: load cert data too
    bool		load_h3,	// true: load h3 data too
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

enumError wd_load_all_part
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		load_cert,	// true: load cert data too
    bool		load_h3,	// true: load h3 data too
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

enumError wd_calc_fst_statistics
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		sum_all		// false: summarize only enabled partitions
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: certificate support		///////////////
///////////////////////////////////////////////////////////////////////////////

const cert_chain_t * wd_load_cert_chain
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
);

//-----------------------------------------------------------------------------

cert_stat_t wd_get_cert_ticket_stat
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
);

//-----------------------------------------------------------------------------

cert_stat_t wd_get_cert_tmd_stat
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
);

//-----------------------------------------------------------------------------

ccp wd_get_sig_status_short_text
(
    cert_stat_t		sig_stat
);

//-----------------------------------------------------------------------------

ccp wd_get_sig_status_text
(
    cert_stat_t		sig_stat
);

//-----------------------------------------------------------------------------

char * wd_print_sig_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent,		// true: don't print errors while loading data
    bool		add_enc_info	// true: append " Partition is *crypted."
);

//-----------------------------------------------------------------------------

char * wd_print_part_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
);

//-----------------------------------------------------------------------------

enumError wd_calc_part_status
(
    wd_part_t		* part,		// valid pointer to a disc partition
    bool		silent		// true: don't print error messages
);

//-----------------------------------------------------------------------------

enumError wd_calc_disc_status
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		silent		// true: don't print error messages
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: select partitions		///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_initialize_select
(
    wd_select_t		* select	// valid pointer to a partition selector
);

//-----------------------------------------------------------------------------

void wd_reset_select
(
    wd_select_t		* select	// valid pointer to a partition selector
);

//-----------------------------------------------------------------------------

wd_select_item_t * wd_append_select_item
(
    wd_select_t		* select,	// valid pointer to a partition selector
    wd_select_mode_t	mode,		// select mode of new item
    u32			table,		// partition table of new item
    u32			part		// partition type or index of new item
);

//-----------------------------------------------------------------------------

void wd_copy_select
(
    wd_select_t		* dest,		// valid pointer to a partition selector
    const wd_select_t	* source	// NULL or pointer to a partition selector
);

//-----------------------------------------------------------------------------

void wd_move_select
(
    wd_select_t		* dest,		// valid pointer to a partition selector
    wd_select_t		* source	// NULL or pointer to a partition selector
);

//-----------------------------------------------------------------------------

void wd_print_select
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_select_t		* select	// valid pointer to a partition selector
);

//-----------------------------------------------------------------------------

bool wd_is_part_selected
(
    wd_part_t		* part,		// valid pointer to a disc partition
    const wd_select_t	* select	// NULL or pointer to a partition selector
);

//-----------------------------------------------------------------------------

bool wd_select // return true if selection changed
(
    wd_disc_t		* disc,		// valid disc pointer
    const wd_select_t	* select	// NULL or pointer to a partition selector
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: usage table		///////////////
///////////////////////////////////////////////////////////////////////////////

extern const char wd_usage_name_tab[256];
extern const u8   wd_usage_class_tab[256];

//-----------------------------------------------------------------------------

u8 * wd_calc_usage_table
(
    wd_disc_t		* disc		// valid disc partition pointer
);

//-----------------------------------------------------------------------------

u8 * wd_filter_usage_table
(
    wd_disc_t		* disc,		// valid disc pointer
    u8			* usage_table,	// NULL or result. If NULL -> MALLOC()
    const wd_select_t	* select	// NULL or a new selector
);

//-----------------------------------------------------------------------------

u32 wd_pack_disc_usage_table // returns the index if the 'last_used_sector + 1'
(
    u8			* dest_table,	// valid pointer to destination table
    wd_disc_t		* disc,		// valid pointer to a disc
    u32			block_size,	// if >1: count every 'block_size'
					//        continuous sectors as one block
    const wd_select_t	* select	// NULL or a new selector
);

//-----------------------------------------------------------------------------

u32 wd_pack_usage_table // returns the index if the 'last_used_sector + 1'
(
    u8			* dest_table,	// valid pointer to destination table
    const u8		* usage_table,	// valid pointer to usage table
    u32			block_size	// if >1: count every 'block_size'
					//        continuous sectors as one block
);

//-----------------------------------------------------------------------------

u64 wd_count_used_disc_size
(
    wd_disc_t		* disc,		// valid pointer to a disc
    int			block_size,	// if >1: count every 'block_size'
					//        continuous sectors as one block
					//        and return the block count
					// if <0: like >1, but give the result as multiple
					//        of WII_SECTOR_SIZE and reduce the count
					//        for non needed sectors at the end.
    const wd_select_t	* select	// NULL or a new selector
);

//-----------------------------------------------------------------------------

u32 wd_count_used_disc_blocks
(
    wd_disc_t		* disc,		// valid pointer to a disc
    int			block_size,	// if >1: count every 'block_size'
					//        continuous sectors as one block
					//        and return the block count
					// if <0: like >1, but give the result as multiple
					//        of WII_SECTOR_SIZE and reduce the count
					//        for non needed sectors at the end.
    const wd_select_t	* select	// NULL or a new selector
);

//-----------------------------------------------------------------------------

u32 wd_count_used_blocks
(
    const u8		* usage_table,	// valid pointer to usage table
    int			block_size	// if >1: count every 'block_size'
					//        continuous sectors as one block
					//        and return the block count
					// if <0: like >1, but give the result as multiple
					//        of WII_SECTOR_SIZE and reduce the count
					//        for non needed sectors at the end.
);

//-----------------------------------------------------------------------------

bool wd_is_block_used
(
    const u8		* usage_table,	// valid pointer to usage table
    u32			block_index,	// index of block
    u32			block_size	// if >1: number of sectors per block
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: file iteration		///////////////
///////////////////////////////////////////////////////////////////////////////

int wd_iterate_files
(
    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
    void		* param,	// user defined parameter
    int			ignore_files,	// >0: ignore all real files
					// >1: ignore fst.bin + main.dol too
    wd_ipm_t		prefix_mode	// prefix mode
);

//-----------------------------------------------------------------------------

int wd_iterate_fst_files
(
    const wd_fst_item_t	*fst_base,	// valid pointer to FST data
    wd_file_func_t	func,		// call back function
    void		* param		// user defined parameter
);

//-----------------------------------------------------------------------------

int wd_remove_disc_files
(
    // Call wd_remove_part_files() for each enabled partition.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was removed
);

//-----------------------------------------------------------------------------

int wd_remove_part_files
(
    // Remove files and directories from internal FST copy.
    // Only empty directories are removed. If at least 1 files/dir
    // is removed the new FST.BIN is added to the patching map.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_part_t		* part,		// valid pointer to a partition
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was removed
);

//-----------------------------------------------------------------------------

int wd_zero_disc_files
(
    // Call wd_remove_part_files() for each enabled partition.
    // Returns 0 if nothing is zeroed, 1 if at least one file is zeroed
    // Other values are abort codes from func()

    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was zeroed
);

//-----------------------------------------------------------------------------

int wd_zero_part_files
(
    // Zero files from internal FST copy (set offset and size to NULL).
    // If at least 1 file is zeroed the new FST.BIN is added to the patching map.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_part_t		* part,		// valid pointer to a partition
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was zeroed
);

//-----------------------------------------------------------------------------

int wd_select_disc_files
(
    // Call wd_remove_part_files() for each enabled partition.
    // Returns 0 if nothing is ignored, 1 if at least one file is ignored
    // Other values are abort codes from func()

    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param		// user defined parameter
);

//-----------------------------------------------------------------------------

int wd_select_part_files
(
    // Calculate the usage map for the partition by using the internal
    // and perhaps modified FST.BIN. If func() is not NULL ignore system
    // and real files (/sys/... and /files/...) for return value 1.
    // If at least 1 file is zeroed the new FST.BIN is added to the patching map.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_part_t		* part,		// valid pointer to a partition
    wd_file_func_t	func,		// call back function
					// If NULL nor file is ignored and only
					// the usage map is re calculated.
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param		// user defined parameter
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: print files		///////////////
///////////////////////////////////////////////////////////////////////////////

extern const char wd_sep_200[201]; // 200 * '-' + NULL

//-----------------------------------------------------------------------------

void wd_initialize_print_fst
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_pfst_t	mode,		// mode for setup
	FILE		* f,		// NULL or output file
	int		indent,		// indention of the output
	u32		max_off4,	// NULL or maximal offset4 value of all files
	u32		max_size	// NULL or maximal size value of all files
);

//-----------------------------------------------------------------------------

void wd_print_fst_header
(
	wd_print_fst_t	* pf,		// valid pointer
	int		max_name_len	// max name len, needed for separator line
);

//-----------------------------------------------------------------------------

void wd_print_fst_item
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_part_t	* part,		// valid pointer to a disc partition
	wd_icm_t	icm,		// iterator call mode
	u32		offset4,	// offset/4 to read
	u32		size,		// size of object
	ccp		fname1,		// NULL or file name, part 1
	ccp		fname2		// NULL or file name, part 2
);

//-----------------------------------------------------------------------------

void wd_print_fst
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid pointer to a disc
	wd_ipm_t	prefix_mode,	// prefix mode
	wd_pfst_t	pfst_mode,	// print mode
	wd_file_func_t	filter_func,	// NULL or filter function
	void		* filter_param	// user defined parameter
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: memmap helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_reset_memmap 
(
    wd_memmap_t		* mm		// NULL or patching data
);

//-----------------------------------------------------------------------------

wd_memmap_item_t * wd_find_memmap
(
    wd_memmap_t		* mm,		// patching data
    u32			mode,		// memmap mode (e.g. wd_patch_mode_t)
    u64			offset,		// offset of object
    u64			size		// size of object
);

//-----------------------------------------------------------------------------

wd_memmap_item_t * wd_insert_memmap
(
    wd_memmap_t		* mm,		// patching data
    u32			mode,		// memmap mode (e.g. wd_patch_mode_t)
    u64			offset,		// offset of object
    u64			size		// size of object
);

//-----------------------------------------------------------------------------

wd_memmap_item_t * wd_insert_memmap_alloc
(
    wd_memmap_t		* mm,		// patching data
    u32			mode,		// memmap mode (e.g. wd_patch_mode_t)
    u64			offset,		// offset of object
    u64			size		// size of object
);

//-----------------------------------------------------------------------------

int wd_insert_memmap_disc_part 
(
    wd_memmap_t		* mm,		// patching data
    wd_disc_t		* disc,		// valid disc pointer

    wd_memmap_func_t	func,		// not NULL: Call func() for each inserted item
    void		* param,	// user defined parameter for 'func()'

    // creation modes:
    // value WD_PAT_IGNORE means: do not create such entires

    wd_patch_mode_t	wii_head_mode,	// value for the Wii partition header
    wd_patch_mode_t	wii_mgr_mode,	// value for the Wii partition mgr data
    wd_patch_mode_t	wii_data_mode,	// value for the Wii partition data
    wd_patch_mode_t	gc_mgr_mode,	// value for the GC partition mgr header
    wd_patch_mode_t	gc_data_mode	// value for the GC partition header
);

//-----------------------------------------------------------------------------

int wd_insert_memmap_part 
(
    wd_memmap_t		* mm,		// patching data
    wd_part_t		* part,		// valid pointer to a disc partition

    wd_memmap_func_t	func,		// not NULL: Call func() for each inserted item
    void		* param,	// user defined parameter for 'func()'

    // creation modes:
    // value WD_PAT_IGNORE means: do not create such entires

    wd_patch_mode_t	wii_head_mode,	// value for the Wii partition header
    wd_patch_mode_t	wii_mgr_mode,	// value for the Wii partition mgr data
    wd_patch_mode_t	wii_data_mode,	// value for the Wii partition data
    wd_patch_mode_t	gc_mgr_mode,	// value for the GC partition mgr header
    wd_patch_mode_t	gc_data_mode	// value for the GC partition header
);

//-----------------------------------------------------------------------------

void wd_print_memmap
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_memmap_t		* mm		// patching data
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: patch helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_insert_patch_ticket
(
    wd_part_t		* part		// valid pointer to a disc partition
);

//-----------------------------------------------------------------------------

wd_memmap_item_t * wd_insert_patch_tmd
(
    wd_part_t		* part		// valid pointer to a disc partition
);

//-----------------------------------------------------------------------------

wd_memmap_item_t * wd_insert_patch_fst
(
    wd_part_t		* part		// valid pointer to a disc partition
);

//-----------------------------------------------------------------------------

void wd_print_disc_patch
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid pointer to a disc
    bool		print_title,	// true: print table titles
    bool		print_part	// true: print partitions too
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: read and patch		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_read_and_patch
(
    wd_disc_t		* disc,		// valid disc pointer
    u64			offset,		// offset to read
    void		* dest_buf,	// destination buffer
    u32			count		// number of bytes to read
);

//-----------------------------------------------------------------------------

void wd_calc_group_hashes
(
    const u8		* group_data,	// group data space
    u8			* group_hash,	// group hash space
    u8			* h3,		// NULL or H3 element to change
    const u8		dirty[WII_GROUP_SECTORS]
					// NULL or 'dirty sector' flags
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     interface: patching		///////////////
///////////////////////////////////////////////////////////////////////////////

u32 wd_get_ptab_sector
(
    wd_disc_t		* disc		// valid disc pointer
);

//-----------------------------------------------------------------------------

bool wd_patch_ptab // result = true if something changed
(
    wd_disc_t		* disc,		// valid disc pointer
    void		* sector_data,	// valid pointer to sector data
					//   GC:  GC_MULTIBOOT_PTAB_OFF + GC_MULTIBOOT_PTAB_SIZE
					//   Wii: WII_MAX_PTAB_SIZE
    bool		force_patch	// false: patch only if needed
);

//-----------------------------------------------------------------------------

bool wd_patch_id
(
    void		* dest_id,	// destination, size=id_len, not 0 term
    const void		* source_id,	// NULL or source id, length=id_len
    const void		* new_id,	// NULL or new ID / 0 term / '.': don't change
    u32			id_len		// max length of id
);

//-----------------------------------------------------------------------------

bool wd_patch_disc_header // result = true if something changed
(
    wd_disc_t		* disc,		// valid pointer to a disc
    ccp			new_id,		// NULL or new ID / '.': don't change
    ccp			new_name	// NULL or new disc name
);

//-----------------------------------------------------------------------------

bool wd_patch_region // result = true if something changed
(
    wd_disc_t		* disc,		// valid pointer to a disc
    u32			new_region	// new region id
);

//-----------------------------------------------------------------------------

bool wd_patch_common_key // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    wd_ckey_index_t	ckey_index	// new common key index
);

//-----------------------------------------------------------------------------

bool wd_patch_part_id // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    wd_modify_t		modify,		// objects to modify
    ccp			new_disc_id,	// NULL or new disc ID / '.': don't change
    ccp			new_boot_id,	// NULL or new boot ID / '.': don't change
    ccp			new_ticket_id,	// NULL or new ticket ID / '.': don't change
    ccp			new_tmd_id	// NULL or new tmd ID / '.': don't change
);

//-----------------------------------------------------------------------------

bool wd_patch_part_name // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    ccp			new_name,	// NULL or new disc name
    wd_modify_t		modify		// objects to modify
);

//-----------------------------------------------------------------------------

bool wd_patch_part_system // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u64			system		// new system id (IOS)
);

///////////////////////////////////////////////////////////////////////////////
// [[wd_patch_main_t]]

typedef struct wd_patch_main_t
{
    wd_disc_t	*disc;		// valid pointer to disc
    wd_part_t	*part;		// valid pointer to partition, if file found

    bool	patch_main;	// true: patch 'sys/main.dol'
    bool	patch_staticr;	// true: patch 'files/rel/staticr.rel'

    wd_memmap_item_t *main;	// NULL or found 'sys/main.dol'
    wd_memmap_item_t *staticr;	// NULL or found 'files/rel/staticr.rel'
}
wd_patch_main_t;

int wd_patch_main
(
    wd_patch_main_t	*pm,		// result only, will be initialized
    wd_disc_t		*disc,		// valid disc
    bool		patch_main,	// true: patch 'sys/main.dol'
    bool		patch_staticr	// true: patch 'files/rel/staticr.rel'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     interface: relocation		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_reloc_t * wd_calc_relocation
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		encrypt,	// true: encrypt partition data
    wd_trim_mode_t	trim_mode,	// trim mode
    u32			trim_align,	// alignment value for trimming
    const wd_select_t	* select,	// NULL or a new selector
    bool		force		// true: force new calculation
);

//-----------------------------------------------------------------------------

void wd_print_relocation
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const wd_reloc_t	* reloc,	// valid pointer to relocation table
    bool		print_title	// true: print table titles
);

//-----------------------------------------------------------------------------

void wd_print_disc_relocation
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid pointer to a disc
    bool		print_title	// true: print table titles
);

//-----------------------------------------------------------------------------

wd_trim_mode_t wd_get_relococation_trim
(
    wd_trim_mode_t	trim_mode,	// trim mode to check
    u32			* trim_align,	// NULL or trim alignment (modify)
    wd_disc_type_t	disc_type	// type of disc for align calculation
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     interface: file relocation		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_file_list_t * wd_initialize_file_list
(
    wd_file_list_t	* fl		// NULL or working file list
					// If NULL: allocate a file list
);

//-----------------------------------------------------------------------------

void wd_reset_file_list
(
    wd_file_list_t	* fl,		// NULL or working file list to reset (free data)
    bool		free_fl		// true: call 'FREE(fl)'
);

//-----------------------------------------------------------------------------

wd_file_list_t * wd_create_file_list
(
    wd_file_list_t	* fl,		// NULL or working file list
					// If NULL: allocate a file list
    wd_fst_item_t	* fst,		// valid fst data structure
    bool		fst_is_gc	// true: 'fst' is a GameCube source
);

//-----------------------------------------------------------------------------

wd_file_t * wd_insert_file
(
    wd_file_list_t	* fl,		// working file list
    u32			src_off4,	// source offset
    u32			size,		// size of file
    ccp			iso_path	// valid path of iso file
);

//-----------------------------------------------------------------------------

wd_file_t * wd_insert_directory
(
    wd_file_list_t	* fl,		// working file list
    ccp			dir_path	// valid path of iso directory
);

//-----------------------------------------------------------------------------

wd_fst_item_t * wd_create_file_list_fst
(
    wd_file_list_t	* fl,		// working file list
    u32			* n_fst,	// not NULL: store number of created fst elements
    bool		create_gc_fst,	// true: create a GameCube fst

    u32			align1,		// minimal alignment
    u32			align2,		// alignment for files with size >= align2
    u32			align3		// alignment for files with size >= align3
					//   All aligment values are rounded 
					//   up to the next power of 2.
					//   The values are normalized (increased) 
					//   to align1 <= align2 <= align3
);

//-----------------------------------------------------------------------------

bool wd_is_directory
(
    wd_file_t		* file		// pointer to a file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: dump data structures		///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_print_disc
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid disc pointer
    int			dump_level	// dump level
					//   >0: print extended part info 
					//   >1: print usage table
);

//-----------------------------------------------------------------------------

void wd_print_disc_usage_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid disc pointer
    bool		print_all	// false: ignore const lines
);

//-----------------------------------------------------------------------------

void wd_print_usage_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const u8		* usage_tab,	// valid pointer, size = WII_MAX_SECTORS
    u64			iso_size,	// NULL or size of iso file
    bool		print_all	// false: ignore const lines
);

//-----------------------------------------------------------------------------

void wd_print_mem
(
    wd_disc_t		* disc,		// valid disc pointer
    wd_mem_func_t	func,		// valid function pointer
    void		* param		// user defined parameter
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIIDISC_H

