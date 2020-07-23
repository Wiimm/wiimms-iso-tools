
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

#ifndef WIT_WBFS_INTERFACE_H
#define WIT_WBFS_INTERFACE_H 1

#include <stdio.h>

#include "dclib/dclib-types.h"
#include "lib-sf.h"
#include "iso-interface.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  some constants		///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
	MIN_WBFS_SIZE		=  10000000, // minimal WBFS partition size
	MIN_WBFS_SIZE_MIB	=       100, // minimal WBFS partition size
	MAX_WBFS		=	999, // maximal number of WBFS partitions
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   partitions			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumPartMode
{
	PM_UNKNOWN,		// not analyzed yet
	PM_IGNORE,		// ignore this entry
	PM_CANT_READ,		// can't read file
	PM_WRONG_TYPE,		// type must be regular or block
	PM_NO_WBFS_MAGIC,	// no WBFS Magic found
	PM_WBFS_MAGIC_FOUND,	// WBFS Magic found, further test needed
	PM_WBFS_INVALID,	// WBFS Magic found, but not a legal WBFS
//	PM_WBFS_CORRUPTED,	// WBFS with errors found
	PM_WBFS,		// WBFS found, no errors detected

} enumPartMode;

//-----------------------------------------------------------------------------

typedef enum enumPartSource
{
	PS_PARAM,	// set by --part or by parameter
	PS_AUTO,	// set by scanning because --auto is set
	PS_AUTO_IGNORE,	// like PS_AUTO, but ignore if can't be opened
	PS_ENV,		// set by scanninc env 'WWT_WBFS'

} enumPartSource;

//-----------------------------------------------------------------------------

typedef struct PartitionInfo_t
{
	int  part_index;

	ccp  path;
	ccp  real_path;
	enumFileMode filemode;
	bool is_checked;
	bool ignore;
	u32  hss;
	u64  file_size;
	u64  disk_usage;
	u32  wbfs_hss;
	u32  wbfs_wss;
	u64  wbfs_size;
	enumPartMode part_mode;
	enumPartSource source;

	struct WDiscList_t   * wlist;
	struct PartitionInfo_t * next;

} PartitionInfo_t;

//-----------------------------------------------------------------------------

extern int wbfs_count;

extern PartitionInfo_t *  first_partition_info;
extern PartitionInfo_t ** append_partition_info;

extern int pi_count;
extern PartitionInfo_t * pi_list[MAX_WBFS+1];
extern struct WDiscList_t pi_wlist;
extern u32 pi_free_mib;

extern int opt_part;
extern int opt_auto;
extern int opt_all;

PartitionInfo_t * CreatePartitionInfo ( ccp path, enumPartSource source );
int  AddPartition ( ccp arg, int unused );
int  ScanPartitions ( bool all );
void AddEnvPartitions();
enumError AnalyzePartitions ( FILE * outfile, bool non_found_is_ok, bool scan_wbfs );
void ScanPartitionGames();

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    wbfs structs		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct WBFS_t
{
    // handles

	SuperFile_t * sf;	// attached super file
	bool sf_alloced;	// true: 'sf' is alloced
	bool is_growing;	// true: wbfs is of type growing
	bool disc_sf_opened;	// true: OpenWDiscSF() opened a disc
	bool cache_candidate;	// true: wbfs is a cache candidate
	bool is_wbfs_file;	// true: is a wbfs file (exact 1 disc at slot #0)
	wbfs_t * wbfs;		// the pure wbfs handle
	wbfs_disc_t * disc;	// the wbfs disc handle
	int disc_slot;		// >=0: last opened slot

    // infos calced by CalcWBFSUsage()

	u32 used_discs;
	u32 free_discs;
	u32 total_discs;

	u32 free_blocks;
	u32 used_mib;
	u32 free_mib;
	u32 total_mib;

} WBFS_t;

extern bool wbfs_cache_enabled;

//-----------------------------------------------------------------------------

typedef struct CheckDisc_t
{
	char id6[7];		// id of the disc
	char no_blocks;		// no valid blocks assigned
	u16  bl_fbt;		// num of blocks marked free in fbt
	u16  bl_overlap;	// num of blocks that overlaps other discs
	u16  bl_invalid;	// num of blocks with invalid blocks
	u16  err_count;		// total count of errors

	char no_iinfo_count;	// no inode defined (not a error)

} CheckDisc_t;

//-----------------------------------------------------------------------------

typedef struct CheckWBFS_t
{
    // handles

	WBFS_t * wbfs;		// attached WBFS

    // data

	off_t  fbt_off;		// offset of fbt
	size_t fbt_size;	// size of fbt
	u32 * cur_fbt;		// current free blocks table (1 bit per block)
	u32 * good_fbt;		// calculated free blocks table (1 bit per block)

	u8  * ubl;		// used blocks (1 byte per block), copy of fbt
	u8  * blc;		// block usage counter
	CheckDisc_t * disc;	// disc list

    // statistics

	u32 err_fbt_used;	// number of wrong used marked blocks
	u32 err_fbt_free;	// number of wrong free marked blocks
	u32 err_fbt_free_wbfs0;	// number of 'err_fbt_free' depend on WBFS v0
	u32 err_no_blocks;	// total num of 'no_blocks' errors
	u32 err_bl_overlap;	// total num of 'bl_overlap' errors
	u32 err_bl_invalid;	// total num of 'bl_invalid' errors
	u32 err_total;		// total of all above

	u32 no_iinfo_count;	// total number of missing inode infos (informative)
	u32 invalid_disc_count;	// total number of invalid games

	enumError err;		// status: OK | WARNING | WBFS_INVALID

} CheckWBFS_t;

///////////////////////////////////////////////////////////////////////////////

extern wbfs_balloc_mode_t opt_wbfs_alloc;
int ScanOptWbfsAlloc ( ccp arg );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   Analyze WBFS			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumAnalyzeWBFS
{
	AW_NONE,		// invalid

	AW_HEADER,		// WBFS header found
	AW_INODES,		// Inodes with WBFS info found
	AW_DISCS,		// Discs found
	AW_CALC,		// Calculation
	AW_OLD_CALC,		// Old and buggy calculation

	AW__N,			// Number of previous values

	AW_MAX_RECORD = 20	// max number of stored records

} enumAnalyzeWBFS;

//-----------------------------------------------------------------------------

typedef struct AWRecord_t
{
	enumAnalyzeWBFS status;		// status of search
	char    title[11];		// short title of record
	char    info[30];		// additional info

	bool	magic_found;		// true: magic found
	u8	wbfs_version;		// source: wbfs_head::wbfs_version

	u32	hd_sectors;		// source: wbfs_head::n_hd_sec
	u32	hd_sector_size;		// source: wbfs_head::hd_sec_sz_s
	u32	wbfs_sectors;		// source:    wbfs_t::n_wbfs_sec
	u32	wbfs_sector_size;	// source: wbfs_head::wbfs_sec_sz_s
	u32	max_disc;		// source:    wbfs_t::max_disc
	u32	disc_info_size;		// source:    wbfs_t::disc_info_sz
	
} AWRecord_t;

//-----------------------------------------------------------------------------

typedef struct AWData_t
{
	uint n_record;			// number of used records
	AWRecord_t rec[AW_MAX_RECORD];	// results of sub search

} AWData_t;

//-----------------------------------------------------------------------------

int AnalyzeWBFS      ( AWData_t * ad, WFile_t * f );
int PrintAnalyzeWBFS
(
    FILE		* out,		// valid output stream
    int			indent,		// indent of output
    AWData_t		* awd,		// valid pointer
    int			print_calc	// 0: suppress calculated values
					// 1: print calculated values if other values available
					// 2: print calculated values
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  ID handling			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ScanParamID6
(
    StringField_t	* select_list,	// append all results to this list
    const ParamList_t	* param		// first param of a list to check
);

int AppendListID6 // returns number of inserted ids
(
    StringField_t	* id6_list,	// append all selected IDs in this list
    const StringField_t	* select_list,	// selector list
    WBFS_t		* wbfs		// open WBFS file
);

int AppendWListID6 // returns number of inserted ids
(
    StringField_t	* id6_list,	// append all selected IDs in this list
    const StringField_t	* select_list,	// selector list
    WDiscList_t		* wlist,	// valid list
    bool		add_to_title_db	// true: add to title DB if unkown
);

bool MatchRulesetID
(
    const StringField_t	* select_list,	// selector list
    ccp			id		// id to compare
);

bool MatchPatternID // returns TRUE if pattern and id match
(
    ccp			pattern,	// pattern, '.' is a wildcard
    ccp			id		// id to compare
);

enumError CheckParamRename ( bool rename_id, bool allow_plus, bool allow_index );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    discs & wbfs interface		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CloseWBFSCache();

void InitializeWBFS	( WBFS_t * w );
enumError ResetWBFS	( WBFS_t * w );
enumError OpenParWBFS	( WBFS_t * w, SuperFile_t * sf, bool print_err, wbfs_param_t * par );
enumError SetupWBFS	( WBFS_t * w, SuperFile_t * sf, bool print_err,
			  int sector_size, bool recover );
enumError CreateGrowingWBFS
			( WBFS_t * w, SuperFile_t * sf, off_t size, int sector_size );

enumError OpenWBFS
(
	WBFS_t		*w,		// valid data structure
	ccp		filename,	// filename to open
	bool		open_modify,	// true: open read+write
	bool		print_err,	// true: pprint error messages
	wbfs_param_t	*par		// NULL or parameter record
);

enumError FormatWBFS	( WBFS_t * w, ccp filename, bool print_err,
			  wbfs_param_t * par, int sector_size, bool recover );
enumError RecoverWBFS	( WBFS_t * w, ccp fname, bool testmode );
enumError TruncateWBFS	( WBFS_t * w );

enumError CalcWBFSUsage	( WBFS_t * w );
enumError SyncWBFS	( WBFS_t * w, bool force_sync );
enumError ReloadWBFS	( WBFS_t * w );

enumError OpenPartWBFS	( WBFS_t * w, struct PartitionInfo_t *  info, bool open_modify );
enumError GetFirstWBFS	( WBFS_t * w, struct PartitionInfo_t ** info, bool open_modify );
enumError GetNextWBFS	( WBFS_t * w, struct PartitionInfo_t ** info, bool open_modify );

void LogOpenedWBFS
(
    WBFS_t		* w,		// valid and opened WBFS
    int			count,		// wbfs counter, 1 based
					// if NULL: neither 'count' nor 'total' are printed
    int			total,		// total wbfs count to handle
					// if NULL: don't print info
    ccp			path		// path of sourcefile
					// if NULL: use 'w->sf->f.fname' (real path)
);

void LogCloseWBFS
(
    WBFS_t		* w,		// valid and opened WBFS
    int			count,		// wbfs counter, 1 based
					// if NULL: neither 'count' nor 'total' are printed
    int			total,		// total wbfs count to handle
					// if NULL: don't print info
    ccp			path		// path of sourcefile
					// if NULL: use 'w->sf->f.fname' (real path)
);

uint      CountWBFS	();
uint	  GetIdWBFS	( WBFS_t * w, IdField_t * idf );

enumError DumpWBFS
(
    WBFS_t	* wbfs,			// valid WBFS
    FILE	* f,			// valid output file
    int		indent,			// indention of output
    ShowMode	show_mode,		// what should be printed
    int		dump_level,		// dump level: 0..3, ignored if show_mode is set
    int		view_invalid_discs,	// view invalid discs too
    CheckWBFS_t	* ck			// not NULL: dump only discs with errors
);

extern StringField_t wbfs_part_list;
u32 FindWBFSPartitions();

//-----------------------------------------------------------------------------

void InitializeCheckWBFS ( CheckWBFS_t * ck );
void ResetCheckWBFS	 ( CheckWBFS_t * ck );
enumError CheckWBFS ( CheckWBFS_t * ck, WBFS_t * wbfs,
			int prt_sections, int verbose, FILE * f, int indent );
enumError AutoCheckWBFS	 ( WBFS_t * w, bool ignore_check, int indent, int prt_sections );

enumError PrintCheckedWBFS ( CheckWBFS_t * ck, FILE * f, int indent, int prt_sections );

enumError RepairWBFS
	( CheckWBFS_t * ck, int testmode, RepairMode rm, int verbose, FILE * f, int indent );
enumError CheckRepairWBFS
	( WBFS_t * w, int testmode, RepairMode rm, int verbose, FILE * f, int indent );
enumError RepairFBT
	( WBFS_t * w, int testmode, FILE * f, int indent );

// returns true if 'good_ftb' differ from 'cur_ftb'
bool CalcFBT ( CheckWBFS_t * ck );

//-----------------------------------------------------------------------------

void InitializeWDiscInfo     ( WDiscInfo_t * dinfo );
enumError ResetWDiscInfo     ( WDiscInfo_t * dinfo );
enumError GetWDiscInfo	     ( WBFS_t * w, WDiscInfo_t * dinfo, int disc_index );
enumError GetWDiscInfoBySlot ( WBFS_t * w, WDiscInfo_t * dinfo, u32 disc_slot );
enumError FindWDiscInfo	     ( WBFS_t * w, WDiscInfo_t * dinfo, ccp id6 );

enumError LoadIsoHeader	( WBFS_t * w, wd_header_t * iso_header, wbfs_disc_t * disc );

void CalcWDiscInfo ( WDiscInfo_t * winfo, SuperFile_t * sf /* NULL possible */ );

enumError DumpWDiscInfo
	( WDiscInfo_t * dinfo, wd_header_t * iso_header, FILE * f, int indent );

//-----------------------------------------------------------------------------

WDiscList_t * GenerateWDiscList ( WBFS_t * w, int part_index );
void InitializeWDiscList ( WDiscList_t * wlist );
void ResetWDiscList ( WDiscList_t * wlist );
void FreeWDiscList ( WDiscList_t * wlist );

WDiscListItem_t *  AppendWDiscList ( WDiscList_t * wlist, WDiscInfo_t * winfo );
void CopyWDiscInfo ( WDiscListItem_t * item, WDiscInfo_t * winfo );

void ReverseWDiscList	( WDiscList_t * wlist );
void SortWDiscList	( WDiscList_t * wlist, enum SortMode sort_mode,
			  enum SortMode default_sort_mode, int unique );

void PrintSectWDiscListItem ( FILE * out, WDiscListItem_t * witem, ccp def_fname );

//-----------------------------------------------------------------------------

enumError OpenWDiscID6	( WBFS_t * w, ccp id6 );
enumError OpenWDiscIndex( WBFS_t * w, u32 index );
enumError OpenWDiscSlot	( WBFS_t * w, u32 slot, bool force_open );
enumError OpenWDiscSF	( WBFS_t * w );
enumError CloseWDiscSF	( WBFS_t * w );
enumError CloseWDisc	( WBFS_t * w );
enumError ExistsWDisc	( WBFS_t * w, ccp id6 );

wd_header_t * GetWDiscHeader ( WBFS_t * w );

enumError AddWDisc	( WBFS_t * w, SuperFile_t * sf, const wd_select_t * psel );

enumError RemoveWDisc
(
    WBFS_t		* w,		// valid WBFS descriptor
    ccp			id6,		// id6 to remove. If NULL: remove 'slot'
    int			slot,		// slot index, only used if 'discid==NULL'
    int			free_slot_only	// true: do not free blocks
);

enumError RenameWDisc	( WBFS_t * w, ccp new_id6, ccp new_title,
	bool change_wbfs_head, bool change_iso_head, int verbose, int testmode );

int RenameISOHeader ( void * data, ccp fname,
	ccp new_id6, ccp new_title, int verbose, int testmode );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_WBFS_INTERFACE_H
