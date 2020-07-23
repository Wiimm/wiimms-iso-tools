
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

#ifndef WIT_PATCH_H
#define WIT_PATCH_H 1

#include <stdio.h>
#include "dclib/dclib-types.h"
#include "lib-std.h"
#include "wiidisc.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

extern int opt_hook; // true: force relocation hook

//-----------------------------------------------------------------------------

typedef enum enumEncoding
{
	// some flags

	ENCODE_F_FAST		= 0x0001, // fast encoding wanted
	ENCODE_F_ENCRYPT	= 0x0002, // encryption and any signing wanted

	// the basic jobs

	ENCODE_CLEAR_HASH	= 0x0010, // clear hash area of each sector
	ENCODE_CALC_HASH	= 0x0020, // calc hash values for each sector
	ENCODE_DECRYPT		= 0x0100, // decrypt sectors
	ENCODE_ENCRYPT		= 0x0200, // encrypt sectors
	ENCODE_NO_SIGN		= 0x1000, // clear signing area
	ENCODE_SIGN		= 0x2000, // fake sign
	
	// the masks

	ENCODE_M_HASH		= ENCODE_CLEAR_HASH | ENCODE_CALC_HASH,
	ENCODE_M_CRYPT		= ENCODE_DECRYPT | ENCODE_ENCRYPT,
	ENCODE_M_SIGN		= ENCODE_NO_SIGN | ENCODE_SIGN,
	
	ENCODE_MASK		= ENCODE_M_HASH | ENCODE_M_CRYPT | ENCODE_M_SIGN
				| ENCODE_F_FAST | ENCODE_F_ENCRYPT,

	ENCODE_DEFAULT		= 0,	// initial value

} enumEncoding;

extern enumEncoding encoding;
enumEncoding ScanEncoding ( ccp arg );
int ScanOptEncoding ( ccp arg );
enumEncoding SetEncoding
	( enumEncoding val, enumEncoding set_mask, enumEncoding default_mask );

//-----------------------------------------------------------------------------

typedef enum enumRegion
{
	REGION_JAP	= 0,	// Japan
	REGION_USA	= 1,	// USA
	REGION_EUR	= 2,	// Europe
	REGION_KOR	= 4,	// Korea

	REGION__AUTO	= 0x7ffffffe,	// auto detect
	REGION__FILE	= 0x7fffffff,	// try to load from file
	REGION__ERROR	= -1,		// hint: error while scanning

} enumRegion;

extern enumRegion opt_region;
enumRegion ScanRegion ( ccp arg );
int ScanOptRegion ( ccp arg );
ccp GetRegionName ( enumRegion region, ccp unkown_value );

//-----------------------------------------------------------------------------

typedef struct RegionInfo_t
{
	enumRegion reg;	    // the region
	bool mandatory;	    // is 'reg' mandatory?
	char name4[4];	    // short name with maximal 4 characters
	ccp name;	    // long region name

} RegionInfo_t;

const RegionInfo_t * GetRegionInfo ( char region_code );

//-----------------------------------------------------------------------------

extern enumRegion opt_common_key;
wd_ckey_index_t ScanCommonKey ( ccp arg );
int ScanOptCommonKey ( ccp arg );

//-----------------------------------------------------------------------------

extern u64 opt_ios;
extern bool opt_ios_valid;

bool ScanSysVersion ( u64 * ios, ccp arg );
int ScanOptIOS ( ccp arg );

//-----------------------------------------------------------------------------

extern bool opt_http;
extern ccp  opt_domain;
int ScanOptDomain ( bool http, ccp domain );
int patch_main ( wd_disc_t * disc );

//-----------------------------------------------------------------------------

extern wd_modify_t opt_modify;
wd_modify_t ScanModify ( ccp arg );
int ScanOptModify ( ccp arg );

//-----------------------------------------------------------------------------

extern ccp modify_name;
extern ccp modify_id;
extern ccp modify_disc_id;
extern ccp modify_boot_id;
extern ccp modify_ticket_id;
extern ccp modify_tmd_id;
extern ccp modify_wbfs_id;

int ScanOptName ( ccp arg );
int ScanOptId ( ccp arg );
int ScanOptDiscId ( ccp arg );
int ScanOptBootId ( ccp arg );
int ScanOptTicketId ( ccp arg );
int ScanOptTmdId ( ccp arg );
int ScanOptWbfsId ( ccp arg );
void NormalizeIdOptions();

bool PatchId
(
    void	*dest_id,	// destination with 'maxlen' byte
    ccp		patch_id,	// NULL or patch string
    int		skip,		// 'patch_id' starts at index 'skip'
    int		maxlen		// length of destination ID
);

bool CopyPatchId
(
    void	*dest,		// destination with 'maxlen' byte
    const void	*src,		// source of ID. If NULL or empty: clear dest
    ccp		patch_id,	// NULL or patch string
    int		maxlen,		// length of destination ID
    bool	null_term	// true: Add an additional 0 byte to end of dest
);

bool CopyPatchWbfsId ( char *dest_id6, const void * source_id6 );
bool CopyPatchDiscId ( char *dest_id6, const void * source_id6 );

bool PatchIdCond ( void * id, int skip, int maxlen, wd_modify_t condition );
bool PatchName ( void * name, wd_modify_t condition );
bool PatchDiscHeader ( void * dhead, const void * patch_id, const void * patch_name );

//-----------------------------------------------------------------------------

extern wd_trim_mode_t opt_trim;

wd_trim_mode_t ScanTrim
(
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
);

int ScanOptTrim
(
    ccp arg			// argument to scan
);

//-----------------------------------------------------------------------------

extern u32  opt_align1;
extern u32  opt_align2;
extern u32  opt_align3;
extern u32  opt_align_part;
extern bool opt_align_files;

int ScanOptAlign ( ccp arg );
int ScanOptAlignPart ( ccp arg );

//-----------------------------------------------------------------------------

extern u64 opt_disc_size;

int ScanOptDiscSize ( ccp arg );

//-----------------------------------------------------------------------------

extern StringField_t add_file;
extern StringField_t repl_file;

int ScanOptFile ( ccp arg, bool add );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			write patch files		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct WritePatch_t
{
    //--- file

    ccp		fname;			// filename for error messages
					//  => never NULL, is freed on closing
    FILE	* file;			// output file

} WritePatch_t;

///////////////////////////////////////////////////////////////////////////////

void SetupWritePatch
(
    WritePatch_t	* pat		// patch data structure
);

//-----------------------------------------------------------------------------

enumError CloseWritePatch
(
    WritePatch_t	* pat		// patch data structure
);

//-----------------------------------------------------------------------------

enumError CreateWritePatch
(
    WritePatch_t	* pat,		// patch data structure
    ccp			filename	// filename of output file
);

//-----------------------------------------------------------------------------

enumError CreateWritePatchF
(
    WritePatch_t	* pat,		// patch data structure
    FILE		* file,		// open output file
    ccp			filename	// NULL or known filename
);

///////////////////////////////////////////////////////////////////////////////

enumError WritePatchComment
(
    WritePatch_t	* pat,		// patch data structure
    ccp			format,		// format string
    ...					// arguments for 'format'
)
__attribute__ ((__format__(__printf__,2,3)));

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read patch files		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct ReadPatch_t
{
    //--- status
    
    bool	is_valid;		// until now source is a valid patch file
    bool	is_compatible;		// source has supported format
    u32		version;		// patch file version
    u32		compatible;		// patch file down compatible to version

    //--- file

    ccp		fname;			// filename for error messages
					//  => never NULL, is freed on closing
    FILE	* file;			// input file

    //--- read buffer

    char	read_buf[1024];		// read chache
    off_t	read_buf_off;		// file offset of first byte in 'read_buf'
    int		read_buf_len;		// valid data of 'read_buf'

    //--- current object in read_buf

    wpat_type_t	cur_type;		// current object type
    u32		cur_size;		// size of current object
					// 'read_buf' is filled as much as possible

} ReadPatch_t;

///////////////////////////////////////////////////////////////////////////////

void SetupReadPatch
(
    ReadPatch_t		* pat		// patch data structure
);

//-----------------------------------------------------------------------------

enumError CloseReadPatch
(
    ReadPatch_t		* pat		// patch data structure
);

//-----------------------------------------------------------------------------

enumError OpenReadPatch
(
    ReadPatch_t		* pat,		// patch data structure
    ccp			filename	// filename of input file
);

//-----------------------------------------------------------------------------

enumError OpenReadPatchF
(
    ReadPatch_t		* pat,		// patch data structure
    FILE		* file,		// open input file
    ccp			filename	// NULL or known filename
);

//-----------------------------------------------------------------------------

enumError SupportedVersionReadPatch
(
    ReadPatch_t		* pat,		// patch data structure
    bool		silent		// true: suppress error message
);

///////////////////////////////////////////////////////////////////////////////

enumError GetNextReadPatch
(
    ReadPatch_t		* pat		// patch data structure
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_PATCH_H
