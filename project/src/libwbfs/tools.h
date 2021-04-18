
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
 *   Copyright (c) 2009-2021 by Dirk Clemens <wiimm@wiimm.de>              *
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

#ifndef TOOLS_H
#define TOOLS_H

#include "dclib/dclib-types.h"
#include "dclib/dclib-basics.h"
#include "dclib/lib-dol.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    setup			///////////////
///////////////////////////////////////////////////////////////////////////////

#include "libwbfs_os.h"		// os dependent definitions
#include "libwbfs_defaults.h"	// default settings

//
///////////////////////////////////////////////////////////////////////////////
///////////////			error messages			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_print_error
(
    ccp		func,		// calling function, use macro __FUNCTION__
    ccp		file,		// source file, use macro __FILE__
    uint	line,		// line number of source file, use macro __LINE__
    enumError	err,		// error code
    ccp		format,		// NULL or format string for fprintf() function.
    ...				// parameters for 'format'

) __attribute__ ((__format__(__printf__,5,6)));

//
///////////////////////////////////////////////////////////////////////////////
///////////////			aligning			///////////////
///////////////////////////////////////////////////////////////////////////////

// align macros for 2^n values

#define ALIGN32(d,a) ((d)+((a)-1)&~(u32)((a)-1))
#define ALIGN64(d,a) ((d)+((a)-1)&~(u64)((a)-1))

//-----------------------------------------------------------------------------

u32 wd_align32
(
    u32		number,		// object of aligning
    u32		align,		// NULL or valid align factor
    int		align_mode	// <0: round down, =0: round math, >0 round up
);

//-----------------------------------------------------------------------------

u64 wd_align64
(
    u64		number,		// object of aligning
    u64		align,		// NULL or valid align factor
    int		align_mode	// <0: round down, =0: round math, >0 round up
);

//-----------------------------------------------------------------------------

u64 wd_align_part
(
    u64		number,		// object of aligning
    u64		align,		// NULL or valid align factor
    bool	is_gamecube	// hint for automatic calculation (align==0)
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			print size			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum wd_size_mode_t
{
    //----- modes => used as index for wd_size_tab_1000[] & wd_size_tab_1024[]

    WD_SIZE_DEFAULT	= 0,	// special default value, fall back to AUTO
    WD_SIZE_AUTO,		// select unit automatically

    WD_SIZE_BYTES,		// force output in bytes
    WD_SIZE_K,			// force output in KB or KiB (kilo,kibi)
    WD_SIZE_M,			// force output in MB or MiB (mega,mebi)
    WD_SIZE_G,			// force output in GB or GiB (giga,gibi)
    WD_SIZE_T,			// force output in TB or TiB (tera,tebi)
    WD_SIZE_P,			// force output in PB or PiB (peta,pebi)
    WD_SIZE_E,			// force output in EB or EiB (exa, exbi)
				// zetta/zebi & yotta/yobi not supported because >2^64

    WD_SIZE_HD_SECT,		// force output as multiples of HD sector size (=512)
    WD_SIZE_WD_SECT,		// force output as multiples of WD sector size (=32768)
    WD_SIZE_GC,			// force output as float as multiples of GC discs
    WD_SIZE_WII,		// force output as float as multiples of Wii discs

    WD_SIZE_N_MODES,		// number of modes

    //----- flags

    WD_SIZE_F_1000	= 0x010,  // force output in SI units (kB=1000, MB=1000000,...)
    WD_SIZE_F_1024	= 0x020,  // force output in IEC units (KiB=1024, MiB=1024*1024,...)
    WD_SIZE_F_AUTO_UNIT	= 0x040,  // suppress output of unit for non AUTO mode
    WD_SIZE_F_NO_UNIT	= 0x080,  // suppress allways output of unit

    //----- masks

    WD_SIZE_M_MODE	= 0x00f,  // mask for modes
    WD_SIZE_M_BASE	= 0x030,  // mask for base
    WD_SIZE_M_ALL	= 0x0ff,  // all relevant bits

} wd_size_mode_t;

//-----------------------------------------------------------------------------

extern ccp wd_size_tab_1000[WD_SIZE_N_MODES+1];
extern ccp wd_size_tab_1024[WD_SIZE_N_MODES+1];

//-----------------------------------------------------------------------------

ccp wd_get_size_unit // get a unit for column headers
(
    wd_size_mode_t	mode,		// print mode
    ccp			if_invalid	// output for invalid modes
);

//-----------------------------------------------------------------------------

int wd_get_size_fw // get a good value field width
(
    wd_size_mode_t	mode,		// print mode
    int			min_fw		// minimal fw => return max(calc_fw,min_fw);
					// this value is also returned for invalid modes
);

//-----------------------------------------------------------------------------

char * wd_print_size
(
    char		* buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    bool		aligned,	// true: print exact 8 chars for num+unit
    wd_size_mode_t	mode		// print mode
);

//-----------------------------------------------------------------------------

char * wd_print_size_1000
(
    char		* buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    bool		aligned		// true: print exact 4+4 chars for num+unit
);

//-----------------------------------------------------------------------------

char * wd_print_size_1024
(
    char		* buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    bool		aligned		// true: print exact 4+4 chars for num+unit
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			printing helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

int wd_normalize_indent
(
    int			indent		// base value to normalize
);

//-----------------------------------------------------------------------------

void wd_print_byte_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const u8		* tab,		// valid pointer to byte table
    u32			used,		// print minimal 'used' values of 'tab'
    u32			size,		// size of 'tab'
    u32			addr_factor,	// each 'tab' element represents 'addr_factor' bytes
    const char		chartab[256],	// valid pointer to a char table
    bool		print_all	// false: ignore const lines
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			fake functions			///////////////
///////////////////////////////////////////////////////////////////////////////

unsigned char * wbfs_sha1_fake
	( const unsigned char *d, size_t n, unsigned char *md );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // TOOLS_H
