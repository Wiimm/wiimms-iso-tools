
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
 *        Copyright (c) 2012-2018 by Dirk Clemens <wiimm@wiimm.de>         *
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

#ifndef DC_LIB_BMG_H
#define DC_LIB_BMG_H 1

#define _GNU_SOURCE 1
#include "dclib-basics.h"
#include "dclib-file.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			basic consts			///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_ATTRIB_SIZE		20			// attrib size in bytes
#define BMG_ATTRIB_BUF_SIZE	(10+4*BMG_ATTRIB_SIZE)	// good buf size for attrib string
#define BMG_INF_MAX_SIZE	(4+BMG_ATTRIB_SIZE)	// max supported inf size
#define BMG_INF_LIMIT		1000			// max allowed inf size
#define BMG_MSG_BUF_SIZE	10050			// good buf size for messages

///////////////////////////////////////////////////////////////////////////////

#define BMG_MAGIC		"MESGbmg1"
#define BMG_MAGIC_NUM		0x4d455347626d6731ull

#define BMG_TEXT_MAGIC		"#BMG"
#define BMG_TEXT_MAGIC_NUM	0x23424d47

#define BMG_UTF8_MAX		0xfffd
#define BMG_UTF8_MAX_DIFF	0x058f	// behind: from-right-to-left letters

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MKW specific consts		///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    BMG_N_CHAT			=    96,
    BMG_N_TRACK			=    32,
    BMG_N_ARENA			=    10,
    BMG_N_CT_TRACK		= 0x120,

    MID_ENGINE_BEG		= 0x0589,
    MID_ENGINE_END		= MID_ENGINE_BEG + 4,

    MID_CHAT_BEG		= 0x1194,
    MID_CHAT_END		= MID_CHAT_BEG + BMG_N_CHAT,

    MID_RACING_CUP_BEG		= 0x23f0,
    MID_RACING_CUP_END		= 0x23f8,
    MID_BATTLE_CUP_BEG		= 0x2489,
    MID_BATTLE_CUP_END		= 0x248b,

    MID_PARAM_BEG		= 0x3ff0,
     MID_VERSUS_POINTS		= MID_PARAM_BEG,
    MID_PARAM_END		= 0x4000,

    MID_CT_TRACK_BEG		= 0x4000,
    MID_CT_TRACK_END		= MID_CT_TRACK_BEG + BMG_N_CT_TRACK,

    MID_CT_CUP_BEG		= 0x4200,
    MID_CT_CUP_END		= 0x4240,
     MID_CT_BATTLE_CUP_BEG	= 0x423e,	// part of MID_CT_CUP_*
     MID_CT_BATTLE_CUP_END	= 0x4240,

    MID_CT_TRACK_CUP_BEG	= 0x4300,
    MID_CT_TRACK_CUP_END	= MID_CT_TRACK_CUP_BEG + BMG_N_CT_TRACK,

    MID_CT_BEG			= MID_CT_TRACK_BEG,
    MID_CT_END			= MID_CT_TRACK_CUP_END,

    MID_TRACK1_BEG		= 0x2454,
    MID_TRACK1_END		= MID_TRACK1_BEG + BMG_N_TRACK,
    MID_TRACK2_BEG		= 0x2490,
    MID_TRACK2_END		= MID_TRACK2_BEG + BMG_N_TRACK,
    MID_TRACK3_BEG		= MID_CT_TRACK_BEG,
    MID_TRACK3_END		= MID_CT_TRACK_END,

    MID_ARENA1_BEG		= 0x24b8,
    MID_ARENA1_END		= MID_ARENA1_BEG + BMG_N_ARENA,
    MID_ARENA2_BEG		= 0x24cc,
    MID_ARENA2_END		= MID_ARENA2_BEG + BMG_N_ARENA,
    MID_ARENA3_BEG		= MID_CT_TRACK_BEG + BMG_N_TRACK,
    MID_ARENA3_END		= MID_ARENA3_BEG + BMG_N_ARENA,

    MID_GENERIC_BEG		= 0xfff0,
     MID_G_SMALL		= MID_GENERIC_BEG,
     MID_G_MEDIUM,
     MID_G_LARGE,
    MID_GENERIC_END,
    BMG_N_GENERIC		= MID_GENERIC_END - MID_GENERIC_BEG,
     //MID_VEHICLE		= 0xd66,
     MID_KARTS			= 0xd67,
     MID_BIKES			= 0xd68,

    MID__VIP_BEG		= MID_CHAT_BEG,
    MID__VIP_END		= MID_CT_TRACK_END,
    BMG__VIP_N			= MID__VIP_END - MID__VIP_BEG
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    global bmg options			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum ColNameLevelBMG // color names as escapes
{
	BMG_CNL_NONE,		// disable color names
	BMG_CNL_BASICS,		// use only old standard color names
	BMG_CNL_YOR,		// + use YOR* names

	BMG_CNL_ALL = BMG_CNL_YOR // default
}
ColNameLevelBMG;

extern uint		opt_bmg_align;		// alignment of raw-bg sections
extern bool		opt_bmg_export;		// true: optimize for exports
extern bool		opt_bmg_no_attrib;	// true: suppress attributes
extern bool		opt_bmg_x_escapes;	// true: use x-scapes insetad of z-escapes
extern bool		opt_bmg_old_escapes;	// true: use only old escapes
extern int		opt_bmg_single_line;	// >0: single line, >1: suppress atrributes
extern bool		opt_bmg_inline_attrib;	// true: print attrinbutes inline
extern bool		opt_bmg_support_mkw;	// true: support MKW specific extensions
extern bool		opt_bmg_support_ctcode;	// true: support MKW/CTCODE specific extensions
extern int		opt_bmg_colors;		// use c-escapes: 0:off, 1:auto, 2:on
extern ColNameLevelBMG	opt_bmg_color_name;
extern uint		opt_bmg_max_recurse;	// max recurse depth
extern bool		opt_bmg_allow_print;	// true: allow '$' to print a log message

//-----------------------------------------------------------------------------

extern uint		opt_bmg_inf_size;
extern bool		opt_bmg_force_attrib;
extern u8		bmg_force_attrib[BMG_ATTRIB_SIZE];
extern bool		opt_bmg_def_attrib;
extern u8		bmg_def_attrib[BMG_ATTRIB_SIZE];

// 'PatchKeysBMG->opt'
//	0: no param
//	1: file param
//	2: string param
extern const KeywordTab_t PatchKeysBMG[];

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    global string functions		///////////////
///////////////////////////////////////////////////////////////////////////////

// returns 1..N
uint GetWordLength16BMG ( const u16 *source );

uint GetLength16BMG
(
    const u16		* sourse,	// source
    uint		max_len		// max possible length
);

uint PrintString16BMG
(
    char		* buf,		// destination buffer
    uint		buf_size,	// size of 'buf'
    const u16		* src,		// source
    int			src_len,	// length of source in u16 words, -1:NULL term
    u16			utf8_max,	// max code printed as UTF-8
    uint		quote,		// 0:no quotes, 1:escape ", 2: print in quotes
    int			use_color	// >0: use \c{...}
);

uint ScanString16BMG
(
    u16			* buf,		// destination buffer
    uint		buf_size,	// size of 'buf' in u16 words
    ccp			src,		// UTF-8 source
    int			src_len		// length of source, -1: determine with strlen()
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_section_t]]

typedef struct bmg_section_t
{
    char		magic[4];	// a magic to identity the section
    be32_t		size;		// total size of the section
    u8			data[0];	// section data
}
__attribute__ ((packed)) bmg_section_t;


//-----------------------------------------------------------------------------
// [[bmg_header_t]]

typedef struct bmg_header_t
{
    char		magic[8];	// = BMG_MAGIC
    be32_t		size;		// total size of file
    be32_t		n_sections;	// number of sub sections
    be32_t		unknown1;	// = 0x02000000
    char		unknown2[0xc];	// unknown data
    //bmg_section_t	section[0];	// first section header
					// -> disabled because MVC
}
__attribute__ ((packed)) bmg_header_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_inf_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_INF_MAGIC		"INF1"
#define BMG_INF_STD_ATTRIB	0x01000000
#define BMG_INF_STD_SIZE	8

//-----------------------------------------------------------------------------
// [[bmg_inf_item_t]]

typedef struct bmg_inf_item_t
{
    u32			offset;		// offset into text pool
    u8			attrib[0];	// attribute bytes
					// NKWii: MSByte used for font selection.
}
__attribute__ ((packed)) bmg_inf_item_t;

//-----------------------------------------------------------------------------
// [[bmg_inf_t]]

typedef struct bmg_inf_t
{
    char		magic[4];	// = BMG_INF_MAGIC
    be32_t		size;		// total size of the section
    be16_t		n_msg;		// number of messages
    be16_t		inf_size;	// size of inf items
    be32_t		unknown;	// = 0

    bmg_inf_item_t	list[0];	// message list with (offset,attribute)
}
__attribute__ ((packed)) bmg_inf_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_dat_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_DAT_MAGIC "DAT1"

//-----------------------------------------------------------------------------

typedef struct bmg_dat_t
{
    char		magic[4];	// = BMG_DAT_MAGIC
    be32_t		size;		// total size of the section
    u8			text_pool[];
}
__attribute__ ((packed)) bmg_dat_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_mid_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_MID_MAGIC "MID1"

//-----------------------------------------------------------------------------

typedef struct bmg_mid_t
{
    char		magic[4];	// = BMG_MID_MAGIC
    be32_t		size;		// total size of the section
    be16_t		n_msg;		// number of messages
    be16_t		unknown1;	// = 0x1000
    be32_t		unknown2;	// = 0
    u32			mid[0];		// message id table
}
__attribute__ ((packed)) bmg_mid_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_item_t]]

typedef struct bmg_item_t
{
    u32		mid;			// message ID
    u32		cond;			// >0: message 'cond' must be defined

    u8		attrib[BMG_ATTRIB_SIZE];// attribute buffer, copy from 'inf'
					// always padded with NULL
    u16		attrib_used;		// used size of 'attrib'

    u16		*text;			// pointer to text
    u16		len;			// length in u16 words of 'text'
    u16		alloced_size;		// alloced size in u16 words of 'text'
					// if 0: pointer to an other area
					//       ==> do not free or modify
}
__attribute__ ((packed)) bmg_item_t;

//-----------------------------------------------------------------------------

// special value for bmg_item_t::text
extern u16 bmg_null_entry[];		// = {0}

void FreeItemBMG( bmg_item_t * bi );

void AssignItemTextBMG
(
    bmg_item_t		* bi,		// valid item
    ccp			utf8,		// UTF-8 text to store
    int			len		// length of 'utf8'.
					// if -1: detect length with strlen()
);

void AssignItemScanTextBMG
(
    bmg_item_t		* bi,		// valid item
    ccp			utf8,		// UTF-8 text to scan
    int			len		// length of 'utf8'.
					// if -1: detect length with strlen()
);

void AssignItemText16BMG
(
    bmg_item_t		* bi,		// valid item
    const u16		* text16,	// text to store
    int			len		// length of 'text16'.
					// if -1: detect length by finding
					//        first NULL value.
);

bool IsItemEqualBMG
(
    const bmg_item_t	* bi1,		// NULL or valid item
    const bmg_item_t	* bi2		// NULL or valid item
);

u16 * ModifiyItem
(
    // return a fixed 'ptr'
    bmg_item_t		* bi,		// valid item, prepare text modification
    const u16		* ptr		// NULL or a pointer within 'bi->text'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_t]]

struct szs_file_t;

typedef struct bmg_t
{
    //--- base info

    ccp			fname;		// alloced filename of loaded file
    FileAttrib_t	fatt;		// file attribute
    bool		is_text_src;	// true: source was text

    //--- container, used by SZS tools

    struct szs_file_t	*szs;		// NULL or pointer to containing SZS

    //--- multiple load support

    bool		src_is_arch;	// true: source is an archive
    uint		src_count;	// number of scanned sources
    enumError		max_err;	// max error
    uint		recurse_depth;	// current recurse depth

    //--- raw data

    u8			* data;		// raw data
    uint		data_size;	// size of 'data'
    bool		data_alloced;	// true: 'data' must be freed

    bmg_inf_t		* inf;		// pointer to 'INF1' data
    bmg_dat_t		* dat;		// pointer to 'DAT1' data
    bmg_mid_t		* mid;		// pointer to 'MID1' data

    //--- text map

    bmg_item_t		* item;		// item list
    uint		item_used;	// number of used items
    uint		item_size;	// number of alloced items

    //--- attributes and other parameters

    uint	inf_size;		// size of each 'inf' element
    u8		attrib[BMG_ATTRIB_SIZE];// attribute buffer for defaults
    u16		attrib_used;		// used size of 'attrib', ALIGN(4)
    u8		use_color_names;	// 0:off, 1:auto, 2:on
    u8		use_mkw_messages;	// 0:off, 1:auto, 2:on
    bool	param_defined;		// true: params above finally defined
    u32		active_cond;		// active condition (for text output)

    //--- raw data, created by CreateRawBMG()

    u8			* raw_data;		// NULL or data
    uint		raw_data_size;		// size of 'raw_data'

} bmg_t;

//-----------------------------------------------------------------------------

bmg_item_t * FindItemBMG ( const bmg_t *bmg, u32 mid );
bmg_item_t * FindAnyItemBMG ( const bmg_t *bmg, u32 mid );
bmg_item_t * InsertItemBMG ( bmg_t *bmg, u32 mid,
				u8 *attrib, uint attrib_used, bool *old_item );

//-----------------------------------------------------------------------------

void InitializeBMG ( bmg_t * bmg );
void ResetBMG ( bmg_t * bmg );
void AssignInfSizeBMG ( bmg_t * bmg, uint inf_size );

//-----------------------------------------------------------------------------

enumError ScanBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    bool		initialize,	// true: initialize 'bmg'
    ccp			fname,		// NULL or filename for error messages
    const u8		* data,		// data / if NULL: use internal data
    size_t		data_data	// size of 'data' if data not NULL
);

enumError ScanRawBMG ( bmg_t * bmg );
enumError ScanTextBMG ( bmg_t * bmg );

enumError LoadBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    bool		initialize,	// true: initialize 'bmg'
    ccp			parent_fname,	// NULL or filename of parent for dir-extract
    ccp			fname,		// filename of source
    FileMode_t		file_mode	// open modes
);

//-----------------------------------------------------------------------------

char * PrintAttribBMG
(
    char		*buf,		// destination buffer
    uint		buf_size,	// size of 'buf', BMG_ATTRIB_BUF_SIZE is good
    const u8		*attrib,	// NULL or attrib to print
    uint		attrib_used,	// used length of attrib
    const u8		*def_attrib,	// NULL or default attrib
    bool		force		// true: always print '[]'
);

int ScanMidBMG
(
    // returns:
    //	-1: invalid MID
    //	 0: empty line
    //   1: single mid found
    //   2: double mid found (track or arena)
    //   3: triple mid found (track or arena, CTCODE)

    bmg_t	*bmg,		// NULL or bmg to update parameters
    u32		*ret_mid1,	// return value: pointer to first MID
    u32		*ret_mid2,	// return value: pointer to second MID
    u32		*ret_mid3,	// return value: pointer to third MID
    ccp		src,		// source pointer
    ccp		src_end,	// NULL or end of source
    char	**scan_end	// return value: NULL or end of scanned 'source'
);

//-----------------------------------------------------------------------------

enumError CreateRawBMG
(
    bmg_t		* bmg		// pointer to valid BMG
);

enumError SaveRawBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    ccp			fname,		// filename of destination
    FileMode_t		fmode,		// create-file mode
    bool		set_time	// true: set time stamps
);

enumError SaveTextBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    ccp			fname,		// filename of destination
    FileMode_t		fmode,		// create-file mode
    bool		set_time,	// true: set time stamps
    bool		force_numeric,	// true: force numric MIDs
    uint		brief_count	// >0: suppress syntax info
					// >1: suppress all comments
					// >2: suppress '#BMG' magic
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    MKW hooks			///////////////
///////////////////////////////////////////////////////////////////////////////

extern int (*GetTrackIndexBMG) ( uint idx, int result_if_not_found );
extern int (*GetArenaIndexBMG) ( uint idx, int result_if_not_found );
extern ccp (*GetContainerNameBMG) ( const bmg_t * bmg );

extern int (*AtHookBMG) ( bmg_t * bmg, ccp line, ccp line_end );
// On return status:
//	 -1: continue with scanning line
//	 -2: continue with next line
//	>=0: close file and return status as error code

//
///////////////////////////////////////////////////////////////////////////////
///////////////			patch BMG			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[PatchModeBMG_t]]

typedef enum PatchModeBMG_t
{
				// this mode descibes the operation:
				//	source := source x patch

				//--------------------------------------//
				//	only	only	source	source	//
				//	in	in	!=	==	//
				//	source	patch	patch	patch	//
				//--------------------------------------//
	BMG_PM_PRINT,		//	source	-	print	print	//
	BMG_PM_FORMAT,		//	________use_parameter________	//
	BMG_PM_ID,		//	prefix	n.a.	n.a.	n.a.	//
	BMG_PM_ID_ALL,		//	prefix	n.a.	n.a.	n.a.	//
	BMG_PM_UNICODE,		//	___no_patch_file_available___	//
	BMG_PM_RM_ESCAPES,	//	___no_patch_file_available___	//
				//--------------------------------------//
	BMG_PM_REPLACE,		//	source	-	patch	both	//
	BMG_PM_INSERT,		//	source	patch	source	both	//
	BMG_PM_OVERWRITE,	//	source	patch	patch	both	//
	BMG_PM_DELETE,		//	source	-	-	-	//
	BMG_PM_MASK,		//	-	-	source	both	//
	BMG_PM_EQUAL,		//	-	-	-	both	//
	BMG_PM_NOT_EQUAL,	//	-	-	source	-	//
				//--------------------------------------//
	BMG_PM_GENERIC,		//	___no_patch_file_available___	//
	BMG_PM_RM_CUPS,		//	___no_patch_file_available___	//
	BMG_PM_CT_COPY,		//	___no_patch_file_available___	//
	BMG_PM_CT_FORCE_COPY,	//	___no_patch_file_available___	//
	BMG_PM_CT_FILL,		//	___no_patch_file_available___	//
				//--------------------------------------//

	//--- helpers
	BMG_PM__MASK_CMD	= 0xff,	// relevant bits of the command
	BMG_PM__SHIFT_OPT	= 8	// shift the parameter option by 8 bits

} PatchModeBMG_t;

///////////////////////////////////////////////////////////////////////////////

enumError PatchBMG
(
    bmg_t		*dest,		// pointer to destination bmg
    const bmg_t		*src,		// pointer to source bmg
    PatchModeBMG_t	patch_mode,	// patch mode
    ccp			format,		// NULL or format parameter
    bool		dup_strings	// true: alloc mem for copied strings (OVERWRITE)
);

//-----------------------------------------------------------------------------

bool PatchFormatBMG
(
    bmg_t		*dest,		// pointer to destination bmg
    const bmg_t		*src,		// pointer to source bmg
    ccp			format		// format parameter
);

bool PatchOverwriteBMG
(
    bmg_t		* dest,		// pointer to destination bmg
    const bmg_t		* src,		// pointer to source bmg
    bool		dup_strings	// true: allocate mem for copied strings
);

bool PatchPrintBMG	( bmg_t * dest, const bmg_t * src );
bool PatchReplaceBMG	( bmg_t * dest, const bmg_t * src );
bool PatchInsertBMG	( bmg_t * dest, const bmg_t * src );
bool PatchDeleteBMG	( bmg_t * dest, const bmg_t * src );
bool PatchMaskBMG	( bmg_t * dest, const bmg_t * src );
bool PatchEqualBMG	( bmg_t * dest, const bmg_t * src );
bool PatchNotEqualBMG	( bmg_t * dest, const bmg_t * src );
bool PatchGenericBMG	( bmg_t * dest, const bmg_t * src );

bool PatchRmCupsBMG	( bmg_t * bmg );
bool PatchCtCopyBMG	( bmg_t * bmg );
bool PatchCtForceCopyBMG( bmg_t * bmg );
bool PatchCtFillBMG	( bmg_t * bmg );

bool PatchIdBMG
(
    bmg_t		* bmg,		// pointer to destination bmg
    bool		prefix_all	// true: prefix all messages
);

bool PatchRmEscapesBMG
(
    bmg_t		* bmg,		// pointer to destination bmg
    bool		unicode,	// replace '\u{16bit}' by unicode chars
    bool		rm_escapes	// remove 1A escapes
);

// remove between mid1 and mid2-1
bool PatchRemoveBMG ( bmg_t * bmg, int mid1, int mid2 );
//bool PatchRemoveTracksBMG ( bmg_t * bmg, int track_limit );

//-----------------------------------------------------------------------------

ccp GetNameBMG ( const bmg_t *bmg, uint *name_len );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DC_LIB_BMG_H 1
