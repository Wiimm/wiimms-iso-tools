
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

#ifndef WIT_LIB_SF_H
#define WIT_LIB_SF_H 1

#include "lib-wdf.h"
#include "lib-wia.h"
#include "lib-ciso.h"
#include "lib-gcz.h"
#include "libwbfs.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			typedef functions		///////////////
///////////////////////////////////////////////////////////////////////////////

struct SuperFile_t;

typedef enumError (*ReadFunc)
	( struct SuperFile_t * sf, off_t off, void * buf, size_t count );

typedef off_t (*DataBlockFunc)
	( struct SuperFile_t * sf, off_t off, size_t hint_align, off_t * block_size );

typedef void (*FileMapFunc)
	( struct SuperFile_t * sf, FileMap_t *fm );

typedef enumError (*WriteFunc)
	( struct SuperFile_t * sf, off_t off, const void * buf, size_t count );

typedef enumError (*ZeroFunc)
	( struct SuperFile_t * sf, off_t off, size_t count );

typedef enumError (*FlushFunc)
	( struct SuperFile_t * sf );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct IOData_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[IOData_t]]

typedef struct IOData_t
{
    enumOFT	oft;			// open file type

    ReadFunc	read_func;		// read function
    DataBlockFunc data_block_func;	// get next data block
    FileMapFunc	file_map_func;		// create a file map

    WriteFunc	write_func;		// write function
    WriteFunc	write_sparse_func;	// sparse write function
    ZeroFunc	write_zero_func;	// zero data write function

    FlushFunc	flush_func;		// flush output

} IOData_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct SuperFile_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[SuperFile_t]]

typedef struct SuperFile_t
{
	// parameters, set by user

	WFile_t f;			// file handling struct
	int  indent;			// indent of progress and summary
	bool show_progress;		// true: show progress info
	bool show_summary;		// true: show summary statistics
	bool show_msec;			// true: show milli seconds in statistics
	bool allow_fst;			// true: allow reading of fst

	// additional info

	struct SuperFile_t *src;	// NULL or pointer to source to get info
	bool  raw_mode;			// true: force raw mode

	// internal values: progress

	int  progress_trigger;		// progress is only printed if value>0
	int  progress_trigger_init;	// if printed: init 'progress_trigger' with this value
	u32  progress_start_time;	// time of start
	u32  progress_last_view_sec;	// time of last progress viewing
	u32  progress_max_wd;		// max width used for progress output
	ccp  progress_verb;		// default is "copied"
	bool progress_summary;		// print summary (delayed) on closing
	u64  progress_add_total;	// add this value to the total (management data)

	u64  progress_last_done;	// last p_done value of PrintProgressSF() call
	u64  progress_last_total;	// last p_total value of PrintProgressSF() call
	u64  progress_data_size;	// value of DefineProgressChunkSF() call
	u64  progress_chunk_size;	// value of DefineProgressChunkSF() call

	// internal values: file handling

	u64 file_size;			// the size of the (virtual) ISO image
	u64 min_file_size;		// if set: Call SetMinSizeSF() before closing
	u64 max_virt_off;		// maximal used offset of virtual image
	u64 source_size;		// if >0: size of source
					//  => display compression ratio in summary stat

	// read and write support

	IOData_t iod;			// open file mode & read+write functions
	enumOFT oft_orig;		// OFT of original source if source is FST
	ReadFunc std_read_func;		// standard read function

	// Wii disc support
	
	bool discs_loaded;		// true: discs already loaded -> don't try again
	wd_disc_t * disc1;		// NULL or pointer to unpatched wii disc
	wd_disc_t * disc2;		// NULL or pointer to patched wii disc

	// WDF support

	wdf_controller_t * wdf;		// WDF controller

	// WIA support

	wia_controller_t * wia;		// WIA controller

	// CISO support

	CISO_Info_t ciso;		// CISO info data

	// WBFS support (read only)

	struct WBFS_t * wbfs;		// a WBFS
	u32 wbfs_fragments;		// 0=unknown, >0:number of fragments
	id6_t wbfs_id6;			// ID6 of wbfs inode

	// GCZ support

	GCZ_t * gcz;			// GCT header

	// FST support
	
	struct WiiFst_t * fst;		// a FST
	MemMap_t modified_list;		// sections that are modified while
					// reading data. This data should
					// be rewritten to the destination
					// before closing the files.

} SuperFile_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////               interface for SuperFile_t         ///////////////
///////////////////////////////////////////////////////////////////////////////

// initialize the super file
void InitializeSF ( SuperFile_t * sf );

// remove all dynamic data
void CleanSF ( SuperFile_t * sf );

// close file + remove all dynamic data
enumError CloseSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref );

enumError Close2SF
(
    SuperFile_t		* sf,		// file to close
    SuperFile_t		* remove_sf,	// not NULL & 'sf' finished without error:
					// close & remove it before renaming 'sf'
    bool		preserve	// true: force preserve time
);

// reset == CloseSF() + reset all but user settings
enumError ResetSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref );

// remove == ResetSF() + remove all files
enumError RemoveSF ( SuperFile_t * sf );

// status
bool IsOpenSF ( const SuperFile_t * sf );
bool IsWritableSF ( const SuperFile_t * sf );

// dynamic SF
SuperFile_t * AllocSF();
SuperFile_t * FreeSF ( SuperFile_t * sf ); // returns always NULL

// setup oft and modifier
enumOFT SetupIOD ( SuperFile_t * sf, enumOFT force, enumOFT def );

// setup reading
enumError SetupReadSF   ( SuperFile_t * sf );		// all files
enumError SetupReadISO  ( SuperFile_t * sf );		// only iso images
enumError SetupReadWBFS ( SuperFile_t * sf );		// setup wbfs/disc reading

enumError OpenSF
(
	SuperFile_t * sf,
	ccp fname,
	bool allow_non_iso,
	bool open_modify
);

enumError CreateSF
(
    SuperFile_t		* sf,		// file to setup
    ccp			fname,		// NULL or filename
    enumOFT		oft,		// output file mode
    enumIOMode		iomode,		// io mode
    int			overwrite	// overwrite mode
);

int IsFileSelected ( wd_iterator_t *it );

void CloseDiscSF
(
    SuperFile_t		* sf		// valid pointer
);

wd_disc_t * OpenDiscSF
(
    SuperFile_t		* sf,		// valid file pointer
    bool		load_part_data,	// true: load partition data
    bool		print_err	// true: print error message if open fails
);

enumError PatchSF
(
    SuperFile_t		* sf,		// valid file pointer
    enumError		err_on_patch	// error message if patched
);

enumError RewriteModifiedSF
(
    SuperFile_t		* fi,		// valid input file
    SuperFile_t		* fo,		// NULL or output file
    struct WBFS_t	* wbfs,		// NULL or output WBFS
    u64			off		// offset: write_off := read_off + off
);

enumError PreallocateSF
(
    SuperFile_t		* sf,		// file to operate
    u64			base_off,	// offset of pre block
    u64			base_size,	// size of pre block
					// base_off+base_size == address fpr block #0
    u32			block_size,	// size in wii sectors of 1 container block
    u32			min_hole_size	// the minimal allowed hole size in 32K sectors
);

enumError SetupWriteSF
(
    SuperFile_t		* sf,		// file to setup
    enumOFT		oft		// force OFT mode of 'sf' 
);

enumError SetupWriteWBFS( SuperFile_t * sf );		// setup wbfs/disc writing

// filename helpers

void SubstFileNameSF
(
    SuperFile_t	* fo,		// output file
    SuperFile_t	* fi,		// input file
    ccp		dest_arg	// arg to parse for escapes
);

int SubstFileNameBuf
(
    char	* fname_buf,	// output buf
    size_t	fname_size,	// size of output buf
    SuperFile_t	* fi,		// input file -> id6, fname
    ccp		fname,		// pure filename. if NULL: extract from 'fi'
    ccp		dest_arg,	// arg to parse for escapes
    enumOFT	oft		// output file type
);

int SubstFileName
(
    char	* fname_buf,	// output buf
    size_t	fname_size,	// size of output buf
    ccp		id6,		// id6
    ccp		disc_name,	// name of disc
    ccp		src_file,	// full path to source file
    ccp		fname,		// pure filename, no path. If NULL: extract from 'src_file'
    ccp		dest_arg,	// arg to parse for escapes
    enumOFT	oft		// output file type
);


//--- main read and write functions

enumError ReadZero	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadSF	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadDirectSF	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadISO	( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError ReadWBFS	( SuperFile_t * sf, off_t off, void * buf, size_t count );

enumError WriteSF	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteSparseSF	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteISO	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteSparseISO( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteWBFS	( SuperFile_t * sf, off_t off, const void * buf, size_t count );

enumError WriteZeroSF	( SuperFile_t * sf, off_t off, size_t count );
enumError WriteZeroISO	( SuperFile_t * sf, off_t off, size_t count );
enumError WriteZeroWBFS	( SuperFile_t * sf, off_t off, size_t count );

enumError FlushSF	( SuperFile_t * sf );
enumError FlushFile	( SuperFile_t * sf );

enumError SetSizeSF	( SuperFile_t * sf, off_t off );
enumError SetMinSizeSF	( SuperFile_t * sf, off_t off );
enumError MarkMinSizeSF ( SuperFile_t * sf, off_t off );
u64       GetGoodMinSize( bool is_gc );

//-----------------------------------------------------------------------------
// data block functions

off_t DataBlockStandard
(
    SuperFile_t		* sf,		// valid file
    off_t		off,		// file offset
    size_t		hint_align,	// if >1: hint for a aligment factor
    off_t		* block_size	// not null: return block size
);

off_t DataBlockWBFS
(
    SuperFile_t		* sf,		// valid file
    off_t		off,		// file offset
    size_t		hint_align,	// if >1: hint for a aligment factor
    off_t		* block_size	// not null: return block size
);

off_t DataBlockSF
(
    SuperFile_t		* sf,		// valid file
    off_t		off,		// file offset
    size_t		align,		// if >1: round results to multiple of 'align'
    off_t		* block_size	// not null: return block size
);

off_t UnionDataBlockSF
(
    SuperFile_t		* sf1,		// first file
    SuperFile_t		* sf2,		// second file
    off_t		off,		// file offset
    size_t		align,		// if >1: round results to multiple of 'align'
    off_t		* block_size	// not null: return block size
);

uint GetFileMapSF
(
    SuperFile_t		* sf,		// valid super files
    FileMap_t		* mm,		// memory list
    bool		init_mm		// true: initialize 'mm', false: reset 'mm'
);

//-----------------------------------------------------------------------------
// mem list functions

uint GetFileMapSF
(
    SuperFile_t		* sf,		// valid super files
    FileMap_t		* mm,		// memory list
    bool		init_mm		// true: initialize 'mm', false: reset 'mm'
);

void FileMapISO  ( SuperFile_t * sf, FileMap_t *fm );
void FileMapWDF  ( SuperFile_t * sf, FileMap_t *fm );
void FileMapWBFS ( SuperFile_t * sf, FileMap_t *fm );
void FileMapCISO ( SuperFile_t * sf, FileMap_t *fm );

//-----------------------------------------------------------------------------
// standard read and write wrappers

enumError ReadSwitchSF		( SuperFile_t * sf, off_t off, void * buf, size_t count );
enumError WriteSwitchSF		( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteSparseSwitchSF	( SuperFile_t * sf, off_t off, const void * buf, size_t count );
enumError WriteZeroSwitchSF	( SuperFile_t * sf, off_t off, size_t count );

// libwbfs read and write wrappers

int WrapperReadSF	  ( void * p_sf, u32 offset, u32 count, void * iobuf );
int WrapperReadDirectSF	  ( void * p_sf, u32 offset, u32 count, void * iobuf );
int WrapperWriteSF	  ( void * p_sf, u32 lba,    u32 count, void * iobuf );
int WrapperWriteSparseSF  ( void * p_sf, u32 lba,    u32 count, void * iobuf );

enumError SparseHelper
	( SuperFile_t * sf, off_t off, const void * buf, size_t count,
	  WriteFunc func, size_t min_chunk_size );

///////////////////////////////////////////////////////////////////////////////

// progress and statistics
void CopyProgressSF ( SuperFile_t * dest, SuperFile_t * src );
void PrintProgressSF ( u64 done, u64 total, void * param );
void ClearProgressLineSF ( SuperFile_t * sf );
void PrintSummarySF ( SuperFile_t * sf );

void DefineProgressChunkSF
(
    SuperFile_t		* sf,		// valid file
    u64			data_size,	// the relevant data size
    u64			chunk_size	// size of chunk to write
);

void PrintProgressChunkSF
(
    SuperFile_t		* sf,		// valid file
    u64			chunk_done	// size of progresses data
);

// find file type
enumFileType AnalyzeFT ( WFile_t * f );
enumFileType AnalyzeMemFT ( const void * buf_hd_sect_size, off_t file_size );
enumError XPrintErrorFT ( XPARM WFile_t * f, enumFileType err_mask );

ccp GetNameFT ( enumFileType ftype, int ignore );
ccp GetContainerNameFT ( enumFileType ftype, ccp answer_if_no_container );
enumOFT FileType2OFT ( enumFileType ftype );
wd_disc_type_t FileType2DiscType ( enumFileType ftype );

u32 CountUsedIsoBlocksSF ( SuperFile_t * sf, const wd_select_t * psel );

enumError CopyImage
(
    SuperFile_t		* fi,		// valid input file
    SuperFile_t		* fo,		// valid output file
    enumOFT		oft,		// oft, if 'OFT_UNKNOWN' it is detected automatically
    int			overwrite,	// overwrite mode
    bool		preserve,	// true: force preserve time
    bool		remove_source	// true: remove source on success
);

enumError CopyImageName
(
    SuperFile_t		* fi,		// valid input file
    ccp			path1,		// NULL or part 1 of path
    ccp			path2,		// NULL or part 2 of path
    enumOFT		oft,		// oft, if 'OFT_UNKNOWN' it is detected automatically
    int			overwrite,	// overwrite mode
    bool		preserve,	// true: force preserve time
    bool		remove_source	// true: remove source on success
);

enumError NormalizeExtractPath
(
    char		* dest_dir,	// result: pointer to path buffer
    size_t		dest_dir_size,	// size of 'dest_dir'
    ccp			source_dest,	// source for destination path
    int			overwrite	// overwrite mode
);

enumError ExtractImage
(
    SuperFile_t		* fi,		// valid input file
    ccp			dest_dir,	// destination directory terminated with '/'
    int			overwrite,	// overwrite mode
    bool		preserve	// true: copy time to extracted files
);

// copy functions
enumError CopySF  ( SuperFile_t * in, SuperFile_t * out );
enumError CopyRaw ( SuperFile_t * in, SuperFile_t * out );

enumError CopyRawData
(
    SuperFile_t	* in,
    SuperFile_t	* out,
    off_t	off,
    off_t	size
);

enumError CopyRawData2
(
    SuperFile_t	* in,
    off_t	in_off,
    SuperFile_t	* out,
    off_t	out_off,
    off_t	copy_size
);

enumError CopyWDF	( SuperFile_t * in, SuperFile_t * out );
enumError CopyWIA	( SuperFile_t * in, SuperFile_t * out );
enumError CopyWBFSDisc	( SuperFile_t * in, SuperFile_t * out );
enumError AppendF	(      WFile_t * in, SuperFile_t * out, off_t in_off, size_t count );
enumError AppendSparseF	(      WFile_t * in, SuperFile_t * out, off_t in_off, size_t count );
enumError AppendSF	( SuperFile_t * in, SuperFile_t * out, off_t in_off, size_t count );
enumError AppendZeroSF	( SuperFile_t * out, off_t count );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    diff			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[Diff_t]]

struct WritePatch_t;

typedef struct Diff_t
{
    //--- options

    int  verbose;		// verbose level, <0: quiet
    int  info_level;		// level of diff output
    int  block_size;		// size of 1 block, setup value
    int  active_block_size;	// size of 1 block, active value

    //--- flags

    bool diff_iso;		// true: diff wii/gc iso images
    bool source_differ;		// true: mismatch found in source
    bool file_differ;		// true: mismatch found in file
    bool mismatch_marked;	// true: mismatch externaly marked

    //--- file info

    SuperFile_t	* f1;		// first file
    SuperFile_t	* f2;		// second file
    ccp	 file_prefix;		// NULL or prefix for 'file_name'
    ccp	 file_name;		// NULL or name of file in file mode

    //--- output streams

    FILE * logfile;		// file for diff messages, stdout is default
    struct WritePatch_t *patch;	// NULL or patch data, must be closed & freed

    //--- source statistics

    u32 source_count;		// number of sources
    u32 source_differ_count;	// number of mismatched sources
    u32 source_differ_limit;	// limit of mismatched sources

    //--- file statistics

    u32 file_count;		// number of files in current source
    u32 file_differ_count;	// number of mismatched files in current source
    u32 file_differ_limit;	// limit of mismatched sources in current source
    u32 total_file_count;	// total number of files
    u32 total_file_differ_count;// total number of mismatched files

    //--- mismatch statistics

    u32 mismatch_count;		// number of mismatches in current file
    u32 mismatch_limit;		// limit of mismatches in current file
    u64 total_mismatch_count;	// total number of mismatches

    //--- data for DiffData()

    char  data1[16];		// copy of mismatched data of previous call to
    char  data2[16];		//    -> needed for complete hexdumps
    off_t data_off;		// offset of 'data*'
    int	  data_size;		// length of 'data*'

    u32	  chunk_count;		// number of compared chunks
    off_t next_off;		// result: next offset to check

} Diff_t;

//-----------------------------------------------------------------------------

void SetupDiff
(
    // use globals: verbose, print_sections,
    //		    opt_limit, opt_file_limit, opt_block_size

    Diff_t		* diff,		// diff structure to setup
    int			long_count	// relevant long count
);

//-----------------------------------------------------------------------------

enumError CloseDiff
(
    // close files and free dynamic data structures

    Diff_t		* diff		// NULL or diff structure to reset
);

//-----------------------------------------------------------------------------

bool OpenDiffSource
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    SuperFile_t		* f1,		// first file
    SuperFile_t		* f2,		// second file
    bool		diff_iso	// true: diff iso images
);

//-----------------------------------------------------------------------------

bool CloseDiffSource
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    bool		silent		// true: suppress printing status messages
);

//-----------------------------------------------------------------------------

bool OpenDiffFile
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    ccp			file_prefix,	// NULL or prefix of file_name
    ccp			file_name	// file name
);

//-----------------------------------------------------------------------------

bool CloseDiffFile
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    bool		silent		// true: suppress printing status messages
);

//-----------------------------------------------------------------------------

bool DiffData
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    off_t		off,		// offset of 'data*'
    size_t		iosize,		// size of 'data*', NULL for terminatiing
    ccp			data1,		// first data
    ccp			data2,		// second data
    int			mode		// 0:in file, 1:term, 2:complete
);

//-----------------------------------------------------------------------------

enumError GetDiffStatus
(
    Diff_t		* diff		// valid diff structure
);

//-----------------------------------------------------------------------------

enumError DiffSF
(
    Diff_t		* diff,		// valid diff structure
    bool		force_raw_mode	// true: force raw mode
);

//-----------------------------------------------------------------------------

enumError DiffRawSF
(
    Diff_t		* diff		// valid diff structure
);

//-----------------------------------------------------------------------------

struct FilePattern_t;
enumError DiffFilesSF
(
    Diff_t		* diff,		// valid diff structure
    struct FilePattern_t * pat,		// NULL or file pattern
    wd_ipm_t		pmode		// prefix mode
);

//-----------------------------------------------------------------------------

enumError DiffPatchSF
(
    Diff_t		* diff,		// valid diff structure
    struct FilePattern_t * pat,		// NULL or file pattern
    wd_ipm_t		pmode		// prefix mode
);

///////////////////////////////////////////////////////////////////////////////

enumError oldDiffSF
(
	SuperFile_t	* f1,
	SuperFile_t	* f2,
	int		long_count,
	bool		force_raw_mode
);

enumError oldDiffRawSF
(
	SuperFile_t	* f1,
	SuperFile_t	* f2,
	int		long_count
);

enumError oldDiffFilesSF
(
	SuperFile_t	* f1,
	SuperFile_t	* f2,
	int		long_count,
	struct FilePattern_t *pat,
	wd_ipm_t	pmode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_SF_H

