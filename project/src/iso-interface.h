
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

#ifndef WIT_ISO_INTERFACE_H
#define WIT_ISO_INTERFACE_H 1

#include "dclib/dclib-types.h"
#include "lib-sf.h"
#include "patch.h"
#include "dclib-utf8.h"
#include "match-pattern.h"
#include "libwbfs/libwbfs.h"
#include "libwbfs/rijndael.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  wii iso discs                  ///////////////
///////////////////////////////////////////////////////////////////////////////
// [[WDiscInfo_t]]

typedef struct WDiscInfo_t
{
    wd_header_t		dhead;
    wd_disc_type_t	disc_type;	// disc type
    wd_disc_attrib_t	disc_attrib;	// disc attrib
    u32			magic2;

    int			disc_index;
    int			slot;
    char		id6[7];
    char		part_info[5];
    u64			size;
    u64			iso_size;
    u32			used_blocks;
    ccp			title;		// pointer to title DB
    u32			n_part;		// number of partitions
    u32			wbfs_fragments;	// number of wbfs fragments

} WDiscInfo_t;

//-----------------------------------------------------------------------------
// [[WDiscListItem_t]]

typedef struct WDiscListItem_t
{
    u32			size_mib;	// size of the source in MiB
    u32			used_blocks;	// number of used ISO blocks
    char		id6[7];		// ID6
    char		name64[65];	// disc name from header
    char		part_info[5];	// string like 'DUC?'
    ccp			title;		// ptr into title DB (not alloced)
    u16			part_index;	// WBFS partition index
    u16			n_part;		// number of partitions
    s16			wbfs_slot;	// slot number
    u16			wbfs_fragments;	// number of wbfs fragments
    enumFileType	ftype;		// the type of the file
    ccp			fname;		// filename, alloced
    FileAttrib_t	fatt;		// file attributes: size, itime, mtime, ctime, atime

} WDiscListItem_t;

//-----------------------------------------------------------------------------
// [[WDiscList_t]]

typedef struct WDiscList_t
{
	WDiscListItem_t * first_disc;
	u32 used;
	u32 size;
	u32 total_size_mib;
	SortMode sort_mode;

} WDiscList_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 dump files			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_ISO
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode,	// what should be printed
    int			dump_level	// dump level: 0..3, ignored if show_mode is set
);

//-----------------------------------------------------------------------------

enumError Dump_DOL
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    uint		dump_mode	// bit field:
					//  1: print header
					//  2: print file position table
					//  4: print virtual position table
					//  8: print virtual-to-file translation table
);

//-----------------------------------------------------------------------------

enumError Dump_CERT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    int			print_ext	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
);

enumError Dump_CERT_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const u8		* cert_data,	// valid pointer to cert data
    size_t		cert_size,	// size of 'cert_data'
    int			print_ext	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
);

enumError Dump_CERT
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const cert_chain_t	* cc,		// valid pinter to cert chain
    int			print_ext	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
);

void Dump_CERT_Item
(
    FILE		* f,		// valid output stream
    int			indent,		// normalized indent of output
    const cert_item_t	* item,		// valid item pointer
    int			cert_index,	// >=0: print title with certificate index
    int			print_ext,	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
    const cert_chain_t	* cc		// not NULL: verify signature
);

//-----------------------------------------------------------------------------

enumError Dump_TIK_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_TIK_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_ticket_t	* tik,		// valid pointer to ticket
    cert_stat_t		sig_status	// not NULL: cert status of signature
);

//-----------------------------------------------------------------------------

enumError Dump_TMD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

enumError Dump_TMD_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_tmd_t	* tmd,		// valid pointer to ticket
    int			n_content,	// number of loaded wd_tmd_content_t elements
    cert_stat_t		sig_status	// not NULL: cert status of signature
);

//-----------------------------------------------------------------------------

enumError Dump_HEAD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

//-----------------------------------------------------------------------------

enumError Dump_BOOT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

//-----------------------------------------------------------------------------

enumError Dump_FST_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode	// what should be printed
);

enumError Dump_FST_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_fst_item_t	* ftab_data,	// the FST data
    size_t		ftab_size,	// size of FST data
    ccp			fname,		// NULL or source hint for error message
    wd_pfst_t		pfst		// print fst mode
);

//-----------------------------------------------------------------------------

enumError Dump_PATCH
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 source iterator                 ///////////////
///////////////////////////////////////////////////////////////////////////////

struct Iterator_t;
typedef enumError (*IteratorFunc) ( SuperFile_t * sf, struct Iterator_t * it );
extern int opt_source_auto;

//-----------------------------------------------------------------------------

typedef enum enumAction
{
	ACT_WARN,	// ignore with message, default
	ACT_IGNORE,	// ignore without message
	ACT_ALLOW,	// allow, default for 'act_wbfs_disc'
	ACT_EXPAND,	// allow and expand (wbfs+fst only)

} enumAction;

//-----------------------------------------------------------------------------
// [[Iterator_t]]

typedef struct Iterator_t
{
	int		depth;		// current directory depth
	int		max_depth;	// maximal directory depth
	bool		expand_dir;	// true: expand directories
	IteratorFunc	func;		// call back function
	ccp		real_path;	// pointer to real_path;

	// options

	bool		open_modify;	// open in modify mode
	bool		newer;		// newer option set, caller needs mtime
	enumAction	act_non_exist;	// action for non existing files
	enumAction	act_non_iso;	// action for non iso files
	enumAction	act_known;	// action for non iso files but well known files
	enumAction	act_wbfs;	// action for wbfs files with n(disc) != 1
	enumAction	act_wbfs_disc;	// action for a wbfs disc (selector used), default=ALLOW
	enumAction	act_fst;	// action for fst
	enumAction	act_gc;		// action for GameCube discs
	enumAction	act_open;	// action for open output files

	// source info

	IdField_t	source_list;	// collected files
	int		source_index;	// informative: index of current file
	bool		auto_processed;	// auto scanning of partitions done

	// statistics

	u32		num_of_scans;	// number of scanned files and dirs
	u32		num_of_files;	// number of found files
	u32		num_of_dirs;	// number of found directories
	u32		num_empty_dirs;	// number of empty base directories

	// progress

	bool	scan_progress;		// true: log each found image
	bool	progress_enabled;	// true: show progress info
	u32	progress_last_sec;	// time of last progress viewing
	ccp	progress_t_file;	// text, default = 'supported file'
	ccp	progress_t_files;	// text, default = 'supported files'

	// user defined parameters, ignored by SourceIterator()

	ShowMode	show_mode;	// active show mode, initialized by opt_show_mode
	bool		diff_it;	// DIFF instead of COPY/CONVERT
	bool		convert_it;	// CONVERT instead of COPY
	bool		update;		// update option set
	bool		overwrite;	// overwrite option set
	bool		remove_source;	// remove option set
	int		real_filename;	// set real filename without any selector
	int		long_count;	// long counter for output
	int		user_mode;	// any user mode
	uint		job_count;	// job counter
	uint		job_total;	// total jobs
	uint		rm_count;	// remove counter
	uint		done_count;	// done counter
	uint		diff_count;	// diff counter
	uint		exists_count;	// 'file already exists' counter
	u64		sum;		// any summary value
	WDiscList_t	* wlist;	// pointer to WDiscList_t to collect data
	struct WBFS_t	* wbfs;		// open WBFS
	dev_t		open_dev;	// dev_t of open output file
	ino_t		open_ino;	// ino_t of open output file

} Iterator_t;

//-----------------------------------------------------------------------------

void InitializeIterator ( Iterator_t * it );
void ResetIterator ( Iterator_t * it );

enumError SourceIterator
(
	Iterator_t * it,
	int warning_mode,
	bool current_dir_is_default,
	bool collect_fnames
);

enumError SourceIteratorCollected
(
    Iterator_t		* it,		// iterator info
    u8			mask,		// not NULL: execute only items
					// with: ( mask & itme->flag ) != 0
    int			warning_mode,	// warning mode if no source found
					// 0:off, 1:only return status, 2:print error
    bool		ignore_err	// false: abort on error > ERR_WARNING 
);

enumError SourceIteratorWarning ( Iterator_t * it, enumError max_err, bool silent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool allow_fst;		// FST diabled by default
extern bool ignore_setup;	// ignore file 'setup.txt' while composing
extern bool opt_links;		// find linked files and create hard links
extern bool opt_user_bin;	// enable management of "sys/user.bin"

extern wd_select_t part_selector;

extern u8 wdisc_usage_tab [WII_MAX_SECTORS];
extern u8 wdisc_usage_tab2[WII_MAX_SECTORS];

enumError ScanPartSelector
(
    wd_select_t * select,	// valid partition selector
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
);

int ScanOptPartSelector ( ccp arg );
u32 ScanPartType ( ccp arg, ccp err_text_extend );

enumError ScanPartTabAndType
(
    u32		* res_ptab,		// NULL or result: partition table
    bool	* res_ptab_valid,	// NULL or result: partition table is valid
    u32		* res_ptype,		// NULL or result: partition type
    bool	* res_ptype_valid,	// NULL or result: partition type is valid
    ccp		arg,			// argument to analyze
    ccp		err_text_extend		// text to extent error messages
);

//-----------------------------------------------------------------------------

extern wd_ipm_t prefix_mode;
extern int opt_flat;

wd_ipm_t ScanPrefixMode ( ccp arg );
void SetupNeekMode();

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Iso Mapping			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumIsoMapType
{
	IMT_ID,			// copy ID with respect to '.' as 'unchanged'
	IMT_DATA,		// raw data
	IMT_FILE,		// data := filename
	IMT_PART_FILES,		// files of partition
	IMT_PART,		// a partition
	IMT__N

} enumIsoMapType;

//-----------------------------------------------------------------------------
// [[IsoMappingItem_t]]

struct WiiFstPart_t;

typedef struct IsoMappingItem_t
{
	enumIsoMapType	imt;		// map type
	u64		offset;		// offset
	u64		size;		// size
	struct WiiFstPart_t *part;	// NULL or relevant partition
	void		*data;		// NULL or pointer to data
	bool		data_alloced;	// true if data must be freed.
	char		info[30];	// comment for dumps

} IsoMappingItem_t;

//-----------------------------------------------------------------------------
// [[IsoMapping_t]]

typedef struct IsoMapping_t
{
	IsoMappingItem_t *field;	// NULL or pointer to first item
	u32 used;			// used items
	u32 size;			// alloced items

} IsoMapping_t;

//-----------------------------------------------------------------------------

void InitializeIM ( IsoMapping_t * im );
void ResetIM ( IsoMapping_t * im );
IsoMappingItem_t * InsertIM
	( IsoMapping_t * im, enumIsoMapType imt, u64 offset, u64 size );

void PrintIM ( IsoMapping_t * im, FILE * f, int indent );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			file index list			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[FileIndexItem_t]]

typedef struct FileIndexItem_t
{
    dev_t		dev;		// ID of device containing file
    ino_t		ino;		// inode number
    struct WiiFstFile_t * file;		// pointer to file;
        
} FileIndexItem_t;

//-----------------------------------------------------------------------------
// [[FileIndexData_t]]

typedef struct FileIndexData_t
{
    struct FileIndexData_t	*next;	// next data element
    uint			unused;	// num of unused elements in 'data'
    FileIndexItem_t		data[];	// item  pool

} FileIndexData_t;

//-----------------------------------------------------------------------------
// [[FileIndex_t]]

typedef struct FileIndex_t
{
    // The field is soreted by 1. 'ino' and 2. 'dev'
    // because 'dev' is almost the same

    FileIndexData_t	* data;		// data pool
    FileIndexItem_t	** sort;	// sorted pointers to 'field'
    uint		used;		// num of used elements in 'field' & 'sort'
    uint		size;		// num of alloced elements in 'field' & 'sort'

} FileIndex_t;

//-----------------------------------------------------------------------------

void InitializeFileIndex ( FileIndex_t * fidx );
void ResetFileIndex ( FileIndex_t * fidx );

FileIndexItem_t * FindFileIndex ( FileIndex_t * fidx, dev_t dev, ino_t ino );
FileIndexItem_t * InsertFileIndex
	( FileIndex_t * fidx, bool * found, dev_t dev, ino_t ino );

void DumpFileIndex ( FILE *f, int indent, const FileIndex_t * fidx );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Wii FST			///////////////
///////////////////////////////////////////////////////////////////////////////

// search and load 'sys/fst+.bin'  =>  0=off, 1=readonly, 2=read+create
#define SUPPORT_FST_PLUS	0

#define FST_SETUP_FILE		"setup.txt"
#define FST_EXCLUDE_FILE	"exclude.fst"
#define FST_INCLUDE_FILE	"include.fst"
#define FST_ALIGN_FILE		"align-files.txt"

enum // some const
{
	PARTITION_NAME_SIZE	= 20,

	DATA_PART_FOUND		= 1,
	UPDATE_PART_FOUND	= 2,
	CHANNEL_PART_FOUND	= 4,
};

extern ccp SpecialFilesFST[]; // NULL terminated list

//-----------------------------------------------------------------------------
// [[WiiFstFile_t]]

typedef struct WiiFstFile_t
{
    u16			icm;			// wd_icm_t
    u32			offset4;		// offset in 4*bytes steps
    u32			size;			// size of file
    ccp			path;			// alloced path name
    u8			* data;			// raw data

    // if ( icm == WD_ICM_FILE ) 'data' is used as link to hardlink list

} WiiFstFile_t;

//-----------------------------------------------------------------------------
// [[WiiFstPart_t]]

typedef struct WiiFstPart_t
{
    //----- wd interface

    wd_part_t		* part;			// NULL or partition pointer

    //----- partition info

    u32			part_type;		// partition type
    u64			part_off;		// offset of partition relative to disc start
    enumOFT		image_type;		// image type, read from setup
    u8			key[WII_KEY_SIZE];	// partition key
    aes_key_t		akey;			// partition aes key
    ccp			path;			// prefix path to partition
    wd_part_control_t	* pc;			// ticket + cert + tmd + h3;

    //----- files

    WiiFstFile_t	* file;			// alloced list of files
    u32			file_used;		// number of used elements in 'file'
    u32			file_size;		// number of allocated elements in 'file'
    SortMode		sort_mode;		// current sort mode
    StringField_t	exclude_list;		// exclude this files on composing
    StringField_t	include_list;		// list of files with trailing '.'
    FileAttrib_t	max_fatt;		// max file attributes
    u64			total_file_size;	// total size of all files

    //----- generator data

    u8			* ftab;			// file table (fst.bin)
    u32			ftab_size;		// size of file table
    IsoMapping_t	im;			// iso mapping
    FileIndex_t		fidx;			// file index for searching links

    //----- status

    int			done;			// set if operation was done

} WiiFstPart_t;

//-----------------------------------------------------------------------------
// [[WiiFst_t]]

typedef struct WiiFst_t
{
	//--- wd interface

	wd_disc_t	* disc;			// NULL or disc pointer

	//--- partitions

	WiiFstPart_t	* part;			// partition infos
	u32		part_used;		// number of used elements in 'part'
	u32		part_size;		// number of allocated elements in 'part'
	WiiFstPart_t	* part_active;		// active partition

	//--- statistics

	u32		total_file_count;	// number of all files of all partition
	u64		total_file_size;	// total size of all files of all partition

	//--- file statistics
	
	u32		files_served;		// total number of files served
	u32		dirs_served;		// total number of files served
	u32		max_path_len;		// max path len of file[].path
	u32		fst_max_off4;		// maximal offset4 value of all files
	u32		fst_max_size;		// maximal size value of all files

	//--- generator data

	IsoMapping_t	im;			// iso mapping
	wd_header_t	dhead;			// disc header
	wd_region_t	region;			// region settings

	u8		*cache;			// cache with WII_GROUP_SECTORS elements
	WiiFstPart_t	*cache_part;		// partition of valid cache data
	u32		cache_group;		// partition group of cache data

	enumEncoding	encoding;		// the encoding mode

} WiiFst_t;

//-----------------------------------------------------------------------------
// [[WiiFstInfo_t]]

typedef struct WiiFstInfo_t
{
	SuperFile_t	* sf;			// NULL or pointer to input file
	WiiFst_t	* fst;			// NULL or pointer to file system
	WiiFstPart_t	* part;			// NULL or pointer to partion
	WiiFstFile_t	* last_file;		// NULL or last file -> detect links

	u32		total_count;		// total files to proceed
	u32		done_count;		// preceeded files
	u32		fw_done_count;		// field width of 'done_count'
	u64		done_size;		// done file size for progress statistics
	u64		total_size;		// total file size for progress statistics
	u32		not_created_count;	// number of not created files+dirs

	FileAttrib_t	* set_time;		// NULL or set time attrib
	bool		overwrite;		// allow ovwerwriting
	bool		copy_image;		// true: copy image to 'game.iso'
	bool		link_image;		// true: try 'link' before 'copy'
	int		verbose;		// the verbosity level

	WiiParamField_t	align_info;		// store align infos here

} WiiFstInfo_t;

//-----------------------------------------------------------------------------

void InitializeFST ( WiiFst_t * fst );
void ResetFST ( WiiFst_t * fst );
void ResetPartFST ( WiiFstPart_t * part );

WiiFstPart_t * AppendPartFST ( WiiFst_t * fst );
WiiFstFile_t * AppendFileFST ( WiiFstPart_t * part );
WiiFstFile_t * FindFileFST ( WiiFstPart_t * part, u32 offset4 );

int CollectFST
(
    WiiFst_t		* fst,		// valid fst pointer
    wd_disc_t		* disc,		// valid disc pointer
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir,	// true: ignore directories
    int			ignore_files,	// >0: ignore all real files
					// >1: ignore fst.bin + main.dol too
    wd_ipm_t		prefix_mode,	// prefix mode
    bool		store_prefix	// true: store full path incl. prefix
);

int CollectFST_BIN
(
    WiiFst_t		* fst,		// valid fst pointer
    const wd_fst_item_t * ftab_data,	// the FST data
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir	// true: ignore directories
);

void DumpFilesFST
(
    FILE		* f,		// NULL or output file
    int			indent,		// indention of the output
    WiiFst_t		* fst,		// valid FST pointer
    wd_pfst_t		pfst,		// print fst mode
    ccp			prefix		// NULL or path prefix
);

u32 ScanPartFST ( WiiFstPart_t * part, ccp path, u32 cur_offset4, wd_boot_t * boot );

u64 GenPartFST
(
    SuperFile_t		* sf,		// valid file pointer
    WiiFstPart_t	* part,		// valid partition pointer
    ccp			path,		// path of partition directory
    u64			good_off,	// standard partition offset
    u64			min_off		// minimal possible partition offset
);

//-----------------------------------------------------------------------------

enumError CreateFST	( WiiFstInfo_t *wfi, ccp dest_path );
enumError CreatePartFST	( WiiFstInfo_t *wfi, ccp dest_path );
enumError CreateFileFST	( WiiFstInfo_t *wfi, ccp dest_path, WiiFstFile_t * file );

enumError ReadFileFST4
(
    WiiFstPart_t	* part,	// valid fst partition pointer
    const WiiFstFile_t	*file,	// valid fst file pointer
    u32			off4,	// file offset/4
    void		* buf,	// destination buffer with at least 'size' bytes
    u32			size	// number of bytes to read
);

enumError ReadFileFST
(
    WiiFstPart_t	* part,	// valid fst partition pointer
    const WiiFstFile_t	*file,	// valid fst file pointer
    u64			off,	// file offset
    void		* buf,	// destination buffer with at least 'size' bytes
    u32			size	// number of bytes to read
);

//-----------------------------------------------------------------------------

SortMode SortFST ( WiiFst_t * fst, SortMode sort_mode, SortMode default_sort_mode );
SortMode SortPartFST ( WiiFstPart_t * part, SortMode sort_mode, SortMode default_sort_mode );
void ReversePartFST ( WiiFstPart_t * part );

//-----------------------------------------------------------------------------

enumFileType IsFST     ( ccp path, char * id6_result );
enumFileType IsFSTPart ( ccp path, char * id6_result );

int SearchPartitionsFST
(
	ccp base_path,
	char * id6_result,	// NULL or result pointer
	char * data_part,	// NULL or result pointer
	char * update_part,	// NULL or result pointer
	char * channel_part	// NULL or result pointer
);

void PrintFstIM ( WiiFst_t * fst, FILE * f, int indent, bool print_part, ccp title );

enumError SetupReadFST ( SuperFile_t * sf );
enumError UpdateSignatureFST ( WiiFst_t * fst );

enumError ReadFST ( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadPartFST ( SuperFile_t * sf, WiiFstPart_t * part,
			off_t off, void * buf, size_t count );
enumError ReadPartGroupFST ( SuperFile_t * sf, WiiFstPart_t * part,
				u32 group_no, void * buf, u32 n_groups );

void EncryptSectorGroup
(
    const aes_key_t	* akey,
    wd_part_sector_t	* sect0,
    size_t		n_sectors,
    void		* h3
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Verify_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[Verify_t]]

typedef struct Verify_t
{
	// data structures

	SuperFile_t		* sf;
	u8			* usage_tab;
	wd_part_t		* part;

	// options, default are global options

	wd_select_t		* psel;		// NULL or partition selector
	int			verbose;	// general verbosity level
	int			long_count;	// verbosity for each message
	int			max_err_msg;	// max message per partition

	// statistical values, used for messages

	int			indent;		// indention of messages
	int			disc_index;	// disc index
	int			disc_total;	// number of total discs
	ccp			fname;		// printed filename

	// internal data

	u32			group;		// index of current sector group
	int			info_indent;	// indention of following messages

} Verify_t;

//-----------------------------------------------------------------------------

void InitializeVerify ( Verify_t * ver, SuperFile_t * sf );
void ResetVerify ( Verify_t * ver );
enumError VerifyPartition ( Verify_t * ver );
enumError VerifyDisc ( Verify_t * ver );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Skeletonize			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Skeletonize
(
    SuperFile_t		* fi,		// valid input file
    ccp			path,		// NULL or path for logging
    int			disc_index,	// 1 based index of disc
    int			disc_total	// total numbers of discs
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                        END                      ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_ISO_INTERFACE_H
