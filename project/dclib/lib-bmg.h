
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

#ifndef DC_LIB_BMG_H
#define DC_LIB_BMG_H 1

#define _GNU_SOURCE 1
#include "dclib-basics.h"
#include "dclib-file.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			basic consts			///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_ATTRIB_SIZE		40			// attrib size in bytes
#define BMG_ATTRIB_BUF_SIZE	(10+4*BMG_ATTRIB_SIZE)	// good buf size for attrib string
#define BMG_INF_MAX_SIZE	(4+BMG_ATTRIB_SIZE)	// max supported inf size
#define BMG_INF_LIMIT		1000			// max allowed inf size
#define BMG_LEGACY_BLOCK_SIZE	32			// block size for legacy files
#define BMG_MAX_SECTIONS	100			// max supported sections (sanity check)

// [[xbuf]]
#define BMG_MSG_BUF_SIZE	20050			// good buf size for messages

///////////////////////////////////////////////////////////////////////////////

#define BMG_MAGIC		"MESGbmg1"
#define BMG_MAGIC8_NUM		0x4d455347626d6731ull

#define BMG_TEXT_MAGIC		"#BMG"
#define BMG_TEXT_MAGIC_NUM	0x23424d47

#define BMG_UTF8_MAX		0xfffd
#define BMG_UTF8_MAX_DIFF	0x058f	// behind: from-right-to-left letters

#define BMG_NO_PREDEF_SLOT	0xffff	// only slots <this can be predefined
typedef u16 bmg_slot_t;			// type of slot

typedef struct bmg_t bmg_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MKW specific consts		///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    BMG_N_CHAT			=    96,
    BMG_N_RCUP			=     8,
    BMG_N_BCUP			=     2,
    BMG_N_TRACK			=    32,
    BMG_N_ARENA			=    10,

    MID_RCUP_BEG		= 0x23f0,
    MID_RCUP_END		= MID_RCUP_BEG + BMG_N_RCUP,
    MID_BCUP_BEG		= 0x2489,
    MID_BCUP_END		= MID_BCUP_BEG + BMG_N_BCUP,

    BMG_RCUP_TRACKS		=     4,
    BMG_BCUP_TRACKS		=     5,

    MID_ENGINE_BEG		= 0x0589,
    MID_ENGINE_END		= MID_ENGINE_BEG + 4,

    MID_CHAT_BEG		= 0x1194,
    MID_CHAT_END		= MID_CHAT_BEG + BMG_N_CHAT,


    //-- parameters

    MID_PARAM_IDENTIFY		= 0x3def,

    MID_PARAM_BEG		= 0x3ff0,
     MID_VERSUS_POINTS		= MID_PARAM_BEG,
    MID_PARAM_END		= 0x4000,


    //-- extended messages for CT/LE

    MID_X_MESSAGE_BEG		= 0x6000,
    MID_X_MESSAGE_END		= 0x6200,


    //-- CT-CODE

    BMG_N_CT_RCUP		= 0x3e,
    BMG_N_CT_BCUP		=    2,
    BMG_N_CT_TRACK		= 0x1fe,
    BMG_N_CT_ARENA		= BMG_N_ARENA,

    MID_CT_TRACK_BEG		= 0x4000,
    MID_CT_TRACK_END		= MID_CT_TRACK_BEG + BMG_N_CT_TRACK,
     MID_CT_ARENA_BEG		= MID_CT_TRACK_BEG + BMG_N_TRACK,
     MID_CT_ARENA_END		= MID_CT_ARENA_BEG + BMG_N_CT_ARENA,
     MID_CT_RANDOM		= MID_CT_TRACK_BEG + 0xff,

    MID_CT_CUP_BEG		= 0x4200,
     MID_CT_RCUP_BEG		= MID_CT_CUP_BEG,
     MID_CT_RCUP_END		= MID_CT_RCUP_BEG + BMG_N_CT_RCUP,
     MID_CT_BCUP_BEG		= MID_CT_RCUP_END,
     MID_CT_BCUP_END		= MID_CT_BCUP_BEG + BMG_N_CT_BCUP,
    MID_CT_CUP_END		= MID_CT_BCUP_END,

    MID_CT_CUP_REF_BEG		= 0x4300,
    MID_CT_CUP_REF_END		= MID_CT_CUP_REF_BEG + BMG_N_CT_TRACK,

    MID_CT_BEG			= MID_CT_TRACK_BEG,
    MID_CT_END			= MID_CT_CUP_REF_END,


    //-- LE-CODE

    BMG_N_LE_RCUP		=  0x400,
    BMG_N_LE_BCUP		=      2,
    BMG_N_LE_TRACK		= 0x1000,
    BMG_N_LE_ARENA		= BMG_N_ARENA,
    BMG_LE_FIRST_CTRACK		=   0x44,

    MID_LE_CUP_BEG		= 0x6800,
     MID_LE_RCUP_BEG		= MID_LE_CUP_BEG,
     MID_LE_RCUP_END		= MID_LE_RCUP_BEG + BMG_N_LE_RCUP,
     MID_LE_BCUP_BEG		= MID_LE_RCUP_END,
     MID_LE_BCUP_END		= MID_LE_BCUP_BEG + BMG_N_LE_BCUP,
    MID_LE_CUP_END		= MID_LE_BCUP_END,

    MID_LE_TRACK_BEG		= 0x7000,
    MID_LE_TRACK_END		= MID_LE_TRACK_BEG + BMG_N_LE_TRACK,
     MID_LE_ARENA_BEG		= MID_LE_TRACK_BEG + BMG_N_TRACK,
     MID_LE_ARENA_END		= MID_LE_ARENA_BEG + BMG_N_LE_ARENA,

    MID_LE_CUP_REF_BEG		= 0x8000,
    MID_LE_CUP_REF_END		= MID_LE_CUP_REF_BEG + BMG_N_LE_TRACK,

    MID_LE_BEG			= MID_LE_CUP_BEG,
    MID_LE_END			= MID_LE_CUP_REF_END,


    //-- max(CT,LE)

    BMG_MAX_RCUP		=  0x3fe,
    BMG_MAX_BCUP		=      2,
    BMG_MAX_TRACK		= 0x1000,
    BMG_MAX_ARENA		= BMG_N_ARENA,


    //-- tracks

    MID_TRACK1_BEG		= 0x2454,
    MID_TRACK1_END		= MID_TRACK1_BEG + BMG_N_TRACK,
    MID_TRACK2_BEG		= 0x2490,
    MID_TRACK2_END		= MID_TRACK2_BEG + BMG_N_TRACK,

    MID_ARENA1_BEG		= 0x24b8,
    MID_ARENA1_END		= MID_ARENA1_BEG + BMG_N_ARENA,
    MID_ARENA2_BEG		= 0x24cc,
    MID_ARENA2_END		= MID_ARENA2_BEG + BMG_N_ARENA,

    MID_RANDOM			= 0x1101,


    //--- misc

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
    MID__VIP_END		= MID_LE_END,
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

extern uint		opt_bmg_force_count;	// >0: force some operations
extern uint		opt_bmg_align;		// alignment of raw-bg sections
extern bool		opt_bmg_export;		// true: optimize for exports
extern bool		opt_bmg_no_attrib;	// true: suppress attributes
extern bool		opt_bmg_x_escapes;	// true: use x-scapes insetad of z-escapes
extern bool		opt_bmg_old_escapes;	// true: use only old escapes
extern int		opt_bmg_single_line;	// >0: single line, >1: suppress atrributes
extern bool		opt_bmg_inline_attrib;	// true: print attrinbutes inline
extern bool		opt_bmg_support_mkw;	// true: support MKW specific extensions
extern bool		opt_bmg_support_ctcode;	// true: support MKW/CT-CODE specific extensions
extern bool		opt_bmg_support_lecode;	// true: support MKW/LE-CODE specific extensions
extern uint		opt_bmg_rcup_fill_limit;// >0: limit racing cups to reduce BMG size
extern uint		opt_bmg_track_fill_limit;// >0: limit tracks to reduce BMG size
extern int		opt_bmg_colors;		// use c-escapes: 0:off, 1:auto, 2:on
extern ColNameLevelBMG	opt_bmg_color_name;
extern uint		opt_bmg_max_recurse;	// max recurse depth
extern bool		opt_bmg_allow_print;	// true: allow '$' to print a log message
extern bool		opt_bmg_use_slots;	// true: use predifined slots
extern bool		opt_bmg_use_raw_sections;// true: use raw sections

//-----------------------------------------------------------------------------

extern int		opt_bmg_endian;
extern int		opt_bmg_encoding;
extern uint		opt_bmg_inf_size;
extern OffOn_t		opt_bmg_mid;
extern bool		opt_bmg_force_attrib;
extern bool		opt_bmg_def_attrib;

extern u8		bmg_force_attrib[BMG_ATTRIB_SIZE];
extern u8		bmg_def_attrib[BMG_ATTRIB_SIZE];


// 'PatchKeysBMG->opt'
//	0: no param
//	1: file param
//	2: string param
extern const KeywordTab_t PatchKeysBMG[];

extern const sizeof_info_t sizeof_info_bmg[];

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CT/LE support			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ct_mode_t]]

typedef enum ct_mode_t
{
    CTM_NO_INIT,		// don't initialize, don't change value
    CTM_NINTENDO,		// original Nintendo mdoe with 32 tracks
    CTM_AUTO,			// use fallback
    CTM_OPTIONS,		// use options opt_bmg_support_{le,ct}code

    CTM_CTCODE_BASE = 0x10,	// base for all CT-CODEs
    CTM_CTCODE,			// CT-CODE (v1)

    CTM_LECODE_BASE = 0x20,	// base for all LE-CODEs
    CTM_LECODE1,		// LE-CODE phase 1 [[not longer used]]
    CTM_LECODE2,		// LE-CODE phase 2

    CTM_LECODE_MIN = CTM_LECODE2, // minimal LE-PHASE
    CTM_LECODE_DEF = CTM_LECODE2, // default LE-PHASE
    CTM_LECODE_MAX = CTM_LECODE2, // maximal LE-PHASE
}
ct_mode_t;

ct_mode_t NormalizeCtModeBMG ( ct_mode_t mode, ct_mode_t fallback );
ct_mode_t SetupCtModeBMG ( ct_mode_t mode );
ccp GetCtModeNameBMG ( ct_mode_t mode, bool full );

///////////////////////////////////////////////////////////////////////////////
// [[bmg_range_t]]

typedef struct bmg_range_t
{
    u32 beg;	// id of first MID
    u32 end;	// id of last MID+1
    u32 n;	// number of MIDs := end-beg
}
bmg_range_t;

///////////////////////////////////////////////////////////////////////////////
// [[ct_bmg_t]]

typedef struct ct_bmg_t
{
    ct_mode_t	ct_mode;	// normalized CT mode
    int		version;	// a version number
    uint	is_ct_code;	// 0:off, 1:CT-CODE, 2:CT+LE-CODE
    uint	le_phase;	// >0: LE-CODE enabled in phase #

    u32		identify;	// MID if ifentification
    bmg_range_t	param;		// MID range of parameters

    bmg_range_t	range;		// MID range of this section (min,max)
    bmg_range_t	rcup_name;	// MID range of racing cup names
    bmg_range_t	bcup_name;	// MID range of battle cup names (maybe part of rcup_name)
    bmg_range_t	track_name1;	// MID range of track names, first set
    bmg_range_t	track_name2;	// 0 or MID range of track names, second set
    bmg_range_t	arena_name1;	// MID range of arena names, first set
    bmg_range_t	arena_name2;	// 0 or MID range of arena names, second set
    bmg_range_t	cup_ref;	// MID range of battle track_to_cup references
    bmg_range_t	random;		// MID for RANDOM
}
ct_bmg_t;

ct_mode_t SetupCtBMG ( ct_bmg_t *ctb, ct_mode_t mode, ct_mode_t fallback );
static inline void ResetCtBMG ( ct_bmg_t *ctb ) {}
ccp GetCtBMGIdentification ( ct_bmg_t *ctb, bool full ); // message for MID_PARAM_IDENTIFY
void DumpCtBMG ( FILE *f, int indent, ct_bmg_t *ctb );

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

//-----------------------------------------------------------------------------

uint PrintString16BMG
(
    char		*buf,		// destination buffer
    uint		buf_size,	// size of 'buf'
    const u16		*src,		// source
    int			src_len,	// length of source in u16 words, -1:NULL term
    u16			utf8_max,	// max code printed as UTF-8
    uint		quote,		// 0:no quotes, 1:escape ", 2: print in quotes
    int			use_color	// >0: use \c{...}
);

// [[xbuf]] [[2do]] not implemented yet
uint PrintFastBuf16BMG
(
    FastBuf_t		*fb,		// destination buffer (data appended)
    const u16		*src,		// source
    int			src_len,	// length of source in u16 words, -1:NULL term
    u16			utf8_max,	// max code printed as UTF-8
    uint		quote,		// 0:no quotes, 1:escape ", 2: print in quotes
    int			use_color	// >0: use \c{...}
);

//-----------------------------------------------------------------------------

uint ScanString16BMG
(
    u16			*buf,		// destination buffer
    uint		buf_size,	// size of 'buf' in u16 words
    ccp			src,		// UTF-8 source
    int			src_len,	// length of source, -1: determine with strlen()
    const bmg_t		*bmg		// NULL or support for \m{...}
);

// [[xbuf]] [[2do]] not implemented yet
uint ScanFastBuf16BMG
(
    FastBuf_t		*fb,		// destination buffer (data appended)
    ccp			src,		// UTF-8 source
    int			src_len,	// length of source, -1: determine with strlen()
    const bmg_t		*bmg		// NULL or support for \m{...}
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_section_t]]

typedef struct bmg_section_t
{
 /*00*/  char		magic[4];	// a magic to identity the section
 /*04*/  be32_t		size;		// total size of the section
 /*08*/  u8		data[0];	// section data
}
__attribute__ ((packed)) bmg_section_t;

//-----------------------------------------------------------------------------
// [[bmg_endian_t]]

typedef enum bmg_endian_t
{
    BMG_AUTO_ENDIAN,
    BMG_BIG_ENDIAN,	// used in MKWii
    BMG_LITTLE_ENDIAN,

    BMG_DEFAULT_ENDIAN	= BMG_BIG_ENDIAN,
}
__attribute__ ((packed)) bmg_endian_t;

extern const KeywordTab_t TabEndianBMG[];
ccp GetEndianNameBMG	( int endian, ccp return_if_invalid );

//-----------------------------------------------------------------------------
// [[bmg_encoding_t]]

typedef enum bmg_encoding_t
{
    BMG_ENC_CP1252	= 1,
    BMG_ENC_UTF16BE	= 2,	// used in MKWii and AC
    BMG_ENC_SHIFT_JIS	= 3,
    BMG_ENC_UTF8	= 4,

    BMG_ENC__MIN	= BMG_ENC_CP1252,
    BMG_ENC__DEFAULT	= BMG_ENC_UTF16BE,
    BMG_ENC__MAX	= BMG_ENC_UTF8,
    BMG_ENC__UNDEFINED	= -1,	// only for option --bmg-encoding
}
__attribute__ ((packed)) bmg_encoding_t;

extern const KeywordTab_t TabEncodingBMG[];
ccp GetEncodingNameBMG	( int encoding, ccp return_if_invalid );
int CheckEncodingBMG	( int encoding, int return_if_invalid );

//-----------------------------------------------------------------------------
// [[bmg_header_t]]

typedef struct bmg_header_t
{
 /*00*/  char		magic[8];	// = BMG_MAGIC
 /*08*/  be32_t		size;		// total size of file
 /*0c*/  be32_t		n_sections;	// number of sections
 /*10*/  bmg_encoding_t	encoding;	// text encoding
 /*11*/  u8		unknown[15];	// unknown data
 /*20*/ //bmg_section_t	section[0];	// first section header
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
#define BMG_INF_DEFAULT_0C	0

//-----------------------------------------------------------------------------
// [[bmg_inf_item_t]]

typedef struct bmg_inf_item_t
{
 /*00*/  u32		offset;		// offset into text pool
 /*04*/  u8		attrib[0];	// attribute bytes
					// NKWii: MSByte used for font selection.
}
__attribute__ ((packed)) bmg_inf_item_t;

//-----------------------------------------------------------------------------
// [[bmg_inf_t]]

typedef struct bmg_inf_t
{
 /*00*/  char		magic[4];	// = BMG_INF_MAGIC
 /*04*/  be32_t		size;		// total size of the section
 /*08*/  be16_t		n_msg;		// number of messages
 /*0a*/  be16_t		inf_size;	// size of inf items
 /*0c*/  be32_t		unknown_0c;	// = BMG_INF_DEFAULT_0C = 0 for MKW
 /*10*/  bmg_inf_item_t	list[0];	// message list with (offset,attribute)
}
__attribute__ ((packed)) bmg_inf_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		struct bmg_dat_t + bmg_str_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_DAT_MAGIC "DAT1"
#define BMG_STR_MAGIC "STR1"

//-----------------------------------------------------------------------------
// [[bmg_dat_t]]

typedef struct bmg_dat_t
{
 /*00*/  char		magic[4];	// = BMG_DAT_MAGIC
 /*04*/  be32_t		size;		// total size of the section
 /*08*/  u8		text_pool[];
}
__attribute__ ((packed)) bmg_dat_t;

//-----------------------------------------------------------------------------

// [[bmg_str_t]]
typedef bmg_dat_t bmg_str_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_mid_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_MID_MAGIC "MID1"
#define BMG_MID_DEFAULT_0A  0x1000
#define BMG_MID_DEFAULT_0C  0

//-----------------------------------------------------------------------------
// [[bmg_mid_t]]

typedef struct bmg_mid_t
{
 /*00*/  char		magic[4];	// = BMG_MID_MAGIC
 /*04*/  be32_t		size;		// total size of the section
 /*08*/  be16_t		n_msg;		// number of messages
 /*0a*/  be16_t		unknown_0a;	// = BMG_MID_DEFAULT_0A = 0x1000 for MKW
 /*0c*/  be32_t		unknown_0c;	// = BMG_MID_DEFAULT_0C = 0 for MKW
 /*10*/  u32		mid[0];		// message id table
}
__attribute__ ((packed)) bmg_mid_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_flw_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_FLW_MAGIC "FLW1"

//-----------------------------------------------------------------------------
// [[bmg_flw_t]]

typedef struct bmg_flw_t
{
 /*00*/  char		magic[4];	// = BMG_FLW_MAGIC
 /*04*/  be32_t		size;		// total size of the section
 /*08*/  u8		unknown[];	// ??? [[2do]]
}
__attribute__ ((packed)) bmg_flw_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_fli_t		///////////////
///////////////////////////////////////////////////////////////////////////////

#define BMG_FLI_MAGIC "FLI1"

//-----------------------------------------------------------------------------
// [[bmg_fli_t]]

typedef struct bmg_fli_t
{
 /*00*/  char		magic[4];	// = BMG_FLI_MAGIC
 /*04*/  be32_t		size;		// total size of the section
 /*08*/  u8		unknown[];	// ??? [[2do]]
}
__attribute__ ((packed)) bmg_fli_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		bmg_sect_info_t & bmg_sect_list_t	///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_sect_info_t]]

typedef struct bmg_sect_info_t
{
    uint		offset;		// offset from beginning of file
    uint		sect_size;	// section size by section header
    uint		real_size;	// real section size
    uint		head_size;	// size of section header
    uint		elem_size;	// =0: unknown, >0: element size
    uint		n_elem_head;	// elem_size>0: number of elements by header
    uint		n_elem;		// 0: number of real elements
    bmg_section_t	*sect;		// pointer to raw source data
					// if NULL: list terminator
    bool		known;		// true: section known
    bool		supported;	// true: section fully supported
    bool		is_empty;	// true: complete data is NULL
//    bool		use_str1;	// true: SINF1 uses 2x 16-bit offsets to STR1
    ccp			info;		// short description
}
bmg_sect_info_t;

//-----------------------------------------------------------------------------
// [[bmg_sect_list_t]]

typedef struct bmg_sect_list_t
{
    uint		source_size;	// valid source size
    uint		this_size;	// total size of this including info[]
    const endian_func_t	*endian;	// endian functions, never NULL

    // all following pointers point into raw source data
    bmg_header_t	*header;	// NULL or pointer to BMG file header
    bmg_inf_t		*pinf;		// NULL or pointer to INF1 header
    bmg_dat_t		*pdat;		// NULL or pointer to DAT1 header
    bmg_str_t		*pstr;		// NULL or pointer to STR1 header
    bmg_mid_t		*pmid;		// NULL or pointer to MID1 header
    bmg_flw_t		*pflw;		// NULL or pointer to FLW1 header
    bmg_fli_t		*pfli;		// NULL or pointer to FLI1 header

    uint		n_sections;	// number if secton, get by bmg header
    uint		n_info;		// number of info records excluding terimination
    bmg_sect_info_t	info[];		// list of sections,
					// terminated by a record with sect==NULL
}
bmg_sect_list_t;

//-----------------------------------------------------------------------------

// result is alloced => FREE(result)
bmg_sect_list_t * ScanSectionsBMG
	( cvp data, uint size, const endian_func_t *endian ); // endian==0: auto detect

const bmg_sect_info_t * SearchSectionBMG
(
    const bmg_sect_list_t *sl,	// valid, created by ScanSectionsBMG()
    cvp			search,	// magic to search
    bool		abbrev,	// if not found: allow abbreviations
    bool		icase	// if not found: ignore case and try again
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_raw_section_t	///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_raw_section_t]]

typedef struct bmg_raw_section_t
{
    char		magic[4];	// section magic
    FastBuf_t		data;		// data buffer
    uint		total_size;	// total size of section
					// if 0: calculate it
    struct
     bmg_raw_section_t	*next;		// pointer to next element
}
bmg_raw_section_t;

//-----------------------------------------------------------------------------

bmg_raw_section_t * GetRawSectionBMG	( bmg_t *bmg, cvp magic );
bmg_raw_section_t * CreateRawSectionBMG	( bmg_t *bmg, cvp magic );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_item_t]]

typedef struct bmg_item_t
{
    u32		mid;			// message ID
    u32		cond;			// >0: message 'cond' must be defined
    bmg_slot_t	slot;			// BMG_NO_PREDEF_SLOT or predifined slot

    u16		attrib_used;		// used size of 'attrib'
    u8		attrib[BMG_ATTRIB_SIZE];// attribute buffer, copy from 'inf'
					// always padded with NULL

    u16		*text;			// pointer to text
    u16		len;			// length in u16 words of 'text'
    u16		alloced_size;		// alloced size in u16 words of 'text'
					// if 0: pointer to an other area
					//       ==> do not free or modify
}
__attribute__ ((packed)) bmg_item_t;

//-----------------------------------------------------------------------------

// special value for bmg_item_t::text
extern u16 bmg_null_entry[];	// = {0}

static inline bool isSpecialEntryBMG ( const u16 *text )
{
    return text == bmg_null_entry;
}

//-----------------------------------------------------------------------------

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

    ccp			fname;			// alloced filename of loaded file
    FileAttrib_t	fatt;			// file attribute
    bool		is_text_src;		// true: source was text
    const endian_func_t	* endian;		// endian functions, never NULL

    //--- container, used by SZS tools

    struct szs_file_t	*szs;			// NULL or pointer to containing SZS

    //--- multiple load support

    bool		src_is_arch;		// true: source is an archive
    uint		src_count;		// number of scanned sources
    enumError		max_err;		// max error
    uint		recurse_depth;		// current recurse depth

    //--- raw data

    u8			* data;			// raw data
    uint		data_size;		// size of 'data'
    bool		data_alloced;		// true: 'data' must be freed

    bmg_inf_t		* inf;			// pointer to 'INF1' data
    bmg_dat_t		* dat;			// pointer to 'DAT1' data
    bmg_mid_t		* mid;			// pointer to 'MID1' data

    //--- raw sections

    bmg_raw_section_t	*first_raw;		// NULL or pointer to chain of raw sections
    bmg_raw_section_t	*current_raw;		// NULL or pointer to current raw sections

    //--- section header data

    u32			unknown_inf_0c;		// section INF1 offset 0x0c
    u16			unknown_mid_0a;		// section MID1 offset 0x0a
    u32			unknown_mid_0c;		// section MID1 offset 0x0c

    //--- text map

    bmg_item_t		* item;			// item list
    uint		item_used;		// number of used items
    uint		item_size;		// number of alloced items

    //--- attributes and other parameters

    bmg_encoding_t	encoding;		// type of encoding
    uint		inf_size;		// size of each 'inf' element
    bool		have_mid;		// TRUE: enable MID1 creation
    bool		legacy;			// TRUE: legacy (GameCube) mode enabled
    u8			attrib[BMG_ATTRIB_SIZE];// attribute buffer for defaults
    u16			attrib_used;		// used size of 'attrib', ALIGN(4)
    u8			use_color_names;	// 0:off, 1:auto, 2:on
    u8			use_mkw_messages;	// 0:off, 1:auto, 2:on
    bool		param_defined;		// true: params above finally defined

    bool		have_predef_slots;	// temporary, set by HavePredifinedSlots()
    bool		use_slots;		// init by opt_bmg_use_slots
    bool		use_raw_sections;	// init by opt_bmg_use_raw_sections
    u32			active_cond;		// active condition (for text output)

    //--- raw data, created by CreateRawBMG()

    u8			* raw_data;		// NULL or data
    uint		raw_data_size;		// size of 'raw_data'

} bmg_t;

//-----------------------------------------------------------------------------

extern bmg_t *bmg_macros;

//-----------------------------------------------------------------------------

bmg_item_t * FindItemBMG ( const bmg_t *bmg, u32 mid );
bmg_item_t * FindAnyItemBMG ( const bmg_t *bmg, u32 mid );
bmg_item_t * InsertItemBMG ( bmg_t *bmg, u32 mid,
				u8 *attrib, uint attrib_used, bool *old_item );
bool DeleteItemBMG ( bmg_t *bmg, u32 mid );

void ResetAttribBMG
(
    const bmg_t		* bmg,		// pointer to bmg
    bmg_item_t		* item		// dest item
);

void CopyAttribBMG
(
    const bmg_t		* dest,		// pointer to destination bmg
    bmg_item_t		* dptr,		// dest item
    const bmg_item_t	* sptr		// source item
);

bool HavePredifinedSlots ( bmg_t *bmg );

//-----------------------------------------------------------------------------

void InitializeBMG ( bmg_t * bmg );
void ResetBMG ( bmg_t * bmg );
void AssignEncodingBMG ( bmg_t * bmg, int encoding );
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

enumError SaveRawFileBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    FILE		* f,		// valid output file
    ccp			fname		// NULL or filename for error messages
);

enumError SaveTextBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    ccp			fname,		// filename of destination
    FileMode_t		fmode,		// create-file mode
    bool		set_time,	// true: set time stamps
    bool		force_numeric,	// true: force numeric MIDs
    uint		brief_count	// >0: suppress syntax info
					// >1: suppress all comments
					// >2: suppress '#BMG' magic
);

enumError SaveTextFileBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    FILE		* f,		// valid output file
    ccp			fname,		// NULL or filename for error messages
    bool		force_numeric,	// true: force numeric MIDs
    uint		brief_count	// >0: suppress syntax info
					// >1: suppress all comments
					// >2: suppress '#BMG' magic
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_create_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_create_t]]

typedef struct bmg_create_t
{
    bmg_t	*bmg;			// current bmg
    const endian_func_t			// endian functions, never NULL
		*endian;		// init by bmg->endian, overridden by opt_bmg_endian


    FastBuf_t	inf;			// INF1 data
    FastBuf_t	dat;			// DAT1 data
    FastBuf_t	mid;			// MID1 data

    bool	have_mid;		// true: have MID1 section
    uint	n_msg;			// number of inserted messages

    //--- iterator -> GetFirstBI() + GetNextBI()

    int		slot;			// slot number
    bmg_item_t	*bi;			// next bi
    bmg_item_t	*bi_end;		// last bi

    bmg_item_t	**bi_list;		// list for predifined slots
    uint	bi_used;		// number of used elements in 'bi_list'
    uint	bi_size;		// number of alloced elements in 'bi_list'
}
bmg_create_t;

//-----------------------------------------------------------------------------

void InitializeCreateBMG( bmg_create_t *bc, bmg_t *bmg );
void ResetCreateBMG	( bmg_create_t *bc );

bmg_item_t * GetFirstBI ( bmg_create_t *bc );
bmg_item_t * GetNextBI	( bmg_create_t *bc );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_scan_mid_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[bmg_scan_mid_t]]

typedef struct bmg_scan_mid_t
{
    int		status;		// -1 for invalid, or 'n_mid'
    int		n_mid;		// number of valid members in mid (0..3)
    u32		mid[3];		// list of message IDs
				//	mid[0]: standard MID
				//	mid[1]: twin (track or arena)
				//	mid[2]: MID for LE_CODE or CT_CODE
    char	*scan_end;	// NULL or end of scanned 'source'
}
bmg_scan_mid_t;

//-----------------------------------------------------------------------------

int ScanBMGMID
(
    // returns 'scan->status'

    bmg_scan_mid_t	*scan,		// result, never NULL
    bmg_t		*bmg,		// NULL or bmg to update parameters
    ccp			src,		// source pointer
    ccp			src_end		// NULL or end of source
);

//-----------------------------------------------------------------------------
// [[obsolete]] wrapper for ScanBMGMID();

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
	BMG_PM_REGEX,		//	________use_parameter________	//
	BMG_PM_RM_REGEX,	//	________use_parameter________	//
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
	BMG_PM_LE_COPY,		//	___no_patch_file_available___	//
	BMG_PM_LE_FORCE_COPY,	//	___no_patch_file_available___	//
	BMG_PM_LE_FILL,		//	___no_patch_file_available___	//
	BMG_PM_X_COPY,		//	___no_patch_file_available___	//
	BMG_PM_X_FORCE_COPY,	//	___no_patch_file_available___	//
	BMG_PM_X_FILL,		//	___no_patch_file_available___	//
	BMG_PM_RM_FILLED,	//	___no_patch_file_available___	//
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

bool PatchCTCopyBMG	( bmg_t * bmg );
bool PatchLECopyBMG	( bmg_t * bmg );
bool PatchXCopyBMG	( bmg_t * bmg );

bool PatchCTForceCopyBMG( bmg_t * bmg );
bool PatchLEForceCopyBMG( bmg_t * bmg );
bool PatchXForceCopyBMG	( bmg_t * bmg );

bool PatchFillBMG
	( bmg_t *bmg, ct_mode_t ct_mode, uint rcup_limit, uint track_limit );
bool PatchXFillBMG ( bmg_t *bmg, uint rcup_limit, uint track_limit );
static inline bool PatchCTFillBMG ( bmg_t *bmg, uint rcup_limit, uint track_limit )
	{ return PatchFillBMG(bmg,CTM_CTCODE,rcup_limit,track_limit); }
static inline bool PatchLEFillBMG ( bmg_t *bmg, uint rcup_limit, uint track_limit )
	{ return PatchFillBMG(bmg,CTM_LECODE2,rcup_limit,track_limit); }

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
bool PatchIdentificationBMG ( bmg_t *bmg, ct_bmg_t *ctb );

//-----------------------------------------------------------------------------

ccp GetNameBMG ( const bmg_t *bmg, uint *name_len );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DC_LIB_BMG_H 1
