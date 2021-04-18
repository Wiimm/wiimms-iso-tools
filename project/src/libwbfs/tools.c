
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

#include "tools.h"
#include "file-formats.h"

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
)
{
    fflush(stdout);

    ccp msg_prefix, msg_name;
    if ( err > ERR_ERROR )
    {
	msg_prefix = "!!! ";
	msg_name   = "FATAL ERROR";
    }
    else if ( err > ERR_WARNING )
    {
	msg_prefix = "!! ";
	msg_name   = "ERROR";
    }
    else if ( err > ERR_OK )
    {
	msg_prefix = "! ";
	msg_name   = "WARNING";
    }
    else
    {
	msg_prefix = "";
	msg_name   = "SUCCESS";
    }

    if ( err > ERR_OK )
	fprintf(stderr,"%s%s in %s() @ %s#%d\n",
	    msg_prefix, msg_name, func, file, line );

    if (format)
    {
	fputs(msg_prefix,stderr);
	va_list arg;
	va_start(arg,format);
	vfprintf(stderr,format,arg);
	va_end(arg);
	if ( format[strlen(format)-1] != '\n' )
	    fputc('\n',stderr);
    }

    fflush(stderr);
    
    if ( err > ERR_ERROR )
	exit(ERR_FATAL);

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			aligning			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 wd_align32
(
    u32		number,		// object of aligning
    u32		align,		// NULL or valid align factor
    int		align_mode	// <0: round down, =0: round math, >0 round up
)
{
    if ( align > 1 )
    {
	const u32 mod = number % align;
	if (mod)
	{
	    if ( align_mode < 0 )
		number -= mod;
	    else if  ( align_mode > 0 )
		number += align - mod;
	    else if ( mod < align/2 )
		number -= mod;
	    else
		number += align - mod;
	}
    }

    return number;
}

///////////////////////////////////////////////////////////////////////////////

u64 wd_align64
(
    u64		number,		// object of aligning
    u64		align,		// NULL or valid align factor
    int		align_mode	// <0: round down, =0: round math, >0 round up
)
{
    if ( align > 1 )
    {
	const u64 mod = number % align;
	if (mod)
	{
	    if ( align_mode < 0 )
		number -= mod;
	    else if  ( align_mode > 0 )
		number += align - mod;
	    else if ( mod < align/2 )
		number -= mod;
	    else
		number += align - mod;
	}
    }

    return number;
}

///////////////////////////////////////////////////////////////////////////////

u64 wd_align_part
(
    u64		number,		// object of aligning
    u64		align,		// NULL or valid align factor
    bool	is_gamecube	// hint for automatic calculation (align==0)
)
{
    if (!align)
	align = is_gamecube ? GC_GOOD_PART_ALIGN : WII_SECTOR_SIZE;
    return wd_align64(number,align,1);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			print size			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp wd_size_tab_1000[WD_SIZE_N_MODES+1] =
{
    0,		// WD_SIZE_DEFAULT
    0,		// WD_SIZE_AUTO

    "B",	// WD_SIZE_BYTES
    "kB",	// WD_SIZE_K
    "MB",	// WD_SIZE_M
    "GB",	// WD_SIZE_G
    "TB",	// WD_SIZE_T
    "PB",	// WD_SIZE_P
    "EB",	// WD_SIZE_E

    0		// all others
};

//-----------------------------------------------------------------------------

ccp wd_size_tab_1024[WD_SIZE_N_MODES+1] =
{
    0,		// WD_SIZE_DEFAULT
    0,		// WD_SIZE_AUTO

    "B",	// WD_SIZE_BYTES
    "KiB",	// WD_SIZE_K
    "MiB",	// WD_SIZE_M
    "GiB",	// WD_SIZE_G
    "TiB",	// WD_SIZE_T
    "PiB",	// WD_SIZE_P
    "EiB",	// WD_SIZE_E

    0		// all others
};

///////////////////////////////////////////////////////////////////////////////

ccp wd_get_size_unit // get a unit for column headers
(
    wd_size_mode_t	mode,		// print mode
    ccp			if_invalid	// output for invalid modes
)
{
    const bool force_1000 = ( mode & WD_SIZE_M_BASE ) == WD_SIZE_F_1000;

    switch ( mode & WD_SIZE_M_MODE )
    {
	//---- SI and IEC units

	case WD_SIZE_DEFAULT:
	case WD_SIZE_AUTO:	return "size";
	case WD_SIZE_BYTES:	return "bytes";

	case WD_SIZE_K:		return force_1000 ? "kB" : "KiB";
	case WD_SIZE_M:		return force_1000 ? "MB" : "MiB";
	case WD_SIZE_G:		return force_1000 ? "GB" : "GiB";
	case WD_SIZE_T:		return force_1000 ? "TB" : "TiB";
	case WD_SIZE_P:		return force_1000 ? "PB" : "PiB";
	case WD_SIZE_E:		return force_1000 ? "EB" : "EiB";
	
	case WD_SIZE_HD_SECT:	return "HDS";
	case WD_SIZE_WD_SECT:	return "WDS";
	case WD_SIZE_GC:	return "GC";
	case WD_SIZE_WII:	return "Wii";
    }
    
    return if_invalid;
}

///////////////////////////////////////////////////////////////////////////////

int wd_get_size_fw // get a good value field width
(
    wd_size_mode_t	mode,		// print mode
    int			min_fw		// minimal fw => return max(calc_fw,min_fw);
					// this value is also returned for invalid modes
)
{
    int fw = mode & (WD_SIZE_F_AUTO_UNIT|WD_SIZE_F_NO_UNIT) ? 0 : 4;

    switch ( mode & WD_SIZE_M_MODE )
    {
	case WD_SIZE_DEFAULT:
	case WD_SIZE_AUTO:
	    if ( !(mode & WD_SIZE_F_NO_UNIT) )
		fw = 4;
	    fw += 4;
	    break;

	case WD_SIZE_BYTES:	fw += 10; break;
	case WD_SIZE_K:		fw +=  7; break;
	case WD_SIZE_M:		fw +=  4; break;
	case WD_SIZE_G:		fw +=  3; break;
	case WD_SIZE_T:		fw +=  3; break;
	case WD_SIZE_P:		fw +=  3; break;
	case WD_SIZE_E:		fw +=  3; break;
	
	case WD_SIZE_HD_SECT:	fw +=  8; break;
	case WD_SIZE_WD_SECT:	fw +=  6; break;
	case WD_SIZE_GC:	fw +=  4; break;
	case WD_SIZE_WII:	fw +=  4; break;

	default:		fw = min_fw;
    }
    
    return fw > min_fw ? fw : min_fw;
}

///////////////////////////////////////////////////////////////////////////////

char * wd_print_size
(
    char		* buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    bool		aligned,	// true: print exact 8 chars for num+unit
    wd_size_mode_t	mode		// print mode
)
{
    DASSERT( WD_SIZE_N_MODES <= WD_SIZE_M_MODE + 1 );

    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    const bool force_1000 = ( mode & WD_SIZE_M_BASE ) == WD_SIZE_F_1000;

    switch ( mode & WD_SIZE_M_MODE )
    {
	//---- SI and IEC units

	case WD_SIZE_BYTES:
	    snprintf(buf,buf_size, aligned ? "%6llu B" : "%llu B", size );
	    break;

	case WD_SIZE_K:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu kB" : "%llu kB",
			( size + KB_SI/2 ) / KB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu KiB" : "%llu KiB",
			( size + KiB/2 ) / KiB );
	    break;

	case WD_SIZE_M:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu MB" : "%llu MB",
			( size + MB_SI /2 ) / MB_SI  );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu MiB" : "%llu MiB",
			( size + MiB /2 ) / MiB  );
	    break;

	case WD_SIZE_G:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu GB" : "%llu GB",
			( size + GB_SI/2 ) / GB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu GiB" : "%llu GiB",
			( size + GiB/2 ) / GiB );
	    break;

	case WD_SIZE_T:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu TB" : "%llu TB",
			( size + TB_SI/2 ) / TB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu TiB" : "%llu TiB",
			( size + TiB/2 ) / TiB );
	    break;

	case WD_SIZE_P:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu PB" : "%llu PB",
			( size + PB_SI/2 ) / PB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu PiB" : "%llu PiB",
			( size + PiB/2 ) / PiB );
	    break;

	case WD_SIZE_E:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu EB" : "%llu EB",
			( size + EB_SI/2 ) / EB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu EiB" : "%llu EiB",
			( size + EiB/2 ) / EiB );
	    break;


	//----- special formats

	case WD_SIZE_HD_SECT:
	    snprintf(buf,buf_size, aligned ? "%4llu HDS" : "%llu HDS",
			( size + HD_SECTOR_SIZE/2 ) / HD_SECTOR_SIZE );
	    break;

	case WD_SIZE_WD_SECT:
	    snprintf(buf,buf_size, aligned ? "%4llu WDS" : "%llu WDS",
			( size + WII_SECTOR_SIZE/2 ) / WII_SECTOR_SIZE );
	    break;

	case WD_SIZE_GC:
	    snprintf(buf,buf_size, aligned ? "%5.2f GC" : "%4.2f GC",
			(double)size / GC_DISC_SIZE );
	    break;

	case WD_SIZE_WII:
	    snprintf(buf,buf_size,"%4.2f Wii",
			(double)size / WII_SECTOR_SIZE / WII_SECTORS_SINGLE_LAYER );
	    break;


	//----- default == auto

	default:
	    buf = force_1000
			? wd_print_size_1000(buf,buf_size,size,aligned)
			: wd_print_size_1024(buf,buf_size,size,aligned);
	    if ( !(mode&WD_SIZE_F_NO_UNIT) )
		return buf;

    }

    if ( mode & (WD_SIZE_F_AUTO_UNIT|WD_SIZE_F_NO_UNIT) )
    {
	char * ptr = buf;
	while ( *ptr == ' ' )
	    ptr++;
	while ( *ptr && *ptr != ' ' )
	    ptr++;
	*ptr  = 0;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * wd_print_size_1000
(
    char		* buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    bool		aligned		// true: print exact 4+4 chars for num+unit
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    u64 num;
    wd_size_mode_t unit;

    u64 mb = (size+MB_SI/2)/MB_SI; // maybe an overflow => extra if
    if ( mb < 10000 && size < EB_SI )
    {
	u64 kb = (size+KB_SI/2)/KB_SI;
	if ( kb < 10 )
	{
	    num  = size;
	    unit = WD_SIZE_BYTES;
	}
	else if ( kb < 10000 )
	{
	    num  = kb;
	    unit = WD_SIZE_K;
	}
	else
	{
	    num  = mb;
	    unit = WD_SIZE_M;
	}
    }
    else
    {
	mb = size / MB_SI; // recalc because of possible overflow
	u64 tb = (mb+MB_SI/2)/MB_SI;
	if ( tb < 10000 )
	{
	    if ( tb < 10 )
	    {
		num  = (mb+KB_SI/2)/KB_SI;
		unit = WD_SIZE_G;
	    }
	    else
	    {
		num  = tb;
		unit = WD_SIZE_T;
	    }
	}
	else
	{
	    u64 pb = (mb+GB_SI/2)/GB_SI;
	    if ( pb < 10000 )
	    {
		num  = pb;
		unit = WD_SIZE_P;
	    }
	    else
	    {
		num  = (mb+TB_SI/2)/TB_SI;
		unit = WD_SIZE_E;
	    }
	}
    }

    if ( num && !( num % 1000 ) && wd_size_tab_1000[unit+1] )
    {
	unit++;
	num /= 1000;
    }

    if (aligned)
	snprintf(buf,buf_size,"%4llu %-3s",num,wd_size_tab_1000[unit]);
    else
	snprintf(buf,buf_size,"%llu %s",num,wd_size_tab_1000[unit]);

    return buf;
};

///////////////////////////////////////////////////////////////////////////////

char * wd_print_size_1024
(
    char		* buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    bool		aligned		// true: print exact 4+4 chars for num+unit
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    u64 num;
    wd_size_mode_t unit;

    u64 mib = (size+MiB/2)/MiB; // maybe an overflow => extra if
    if ( mib < 10000 && size < EiB )
    {
	u64 kib = (size+KiB/2)/KiB;
	if ( kib < 10 )
	{
	    num  = size;
	    unit = WD_SIZE_BYTES;
	}
	else if ( kib < 10000 )
	{
	    num  = kib;
	    unit = WD_SIZE_K;
	}
	else
	{
	    num  = mib;
	    unit = WD_SIZE_M;
	}
    }
    else
    {
	mib = size / MiB; // recalc because of possible overflow
	u64 tib = (mib+MiB/2)/MiB;
	if ( tib < 10000 )
	{
	    if ( tib < 10 )
	    {
		num  = (mib+KiB/2)/KiB;
		unit = WD_SIZE_G;
	    }
	    else
	    {
		num  = tib;
		unit = WD_SIZE_T;
	    }
	}
	else
	{
	    u64 pib = (mib+GiB/2)/GiB;
	    if ( pib < 10000 )
	    {
		num  = pib;
		unit = WD_SIZE_P;
	    }
	    else
	    {
		num  = (mib+TiB/2)/TiB;
		unit = WD_SIZE_E;
	    }
	}
    }

    if ( num && !( num & 0x3ff ) && wd_size_tab_1024[unit+1] )
    {
	unit++;
	num /= 0x400;
    }

    if (aligned)
	snprintf(buf,buf_size,"%4llu %-3s",num,wd_size_tab_1024[unit]);
    else
	snprintf(buf,buf_size,"%llu %s",num,wd_size_tab_1024[unit]);

    return buf;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			printing helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

int wd_normalize_indent
(
    int			indent		// base vlaue to normalize
)
{
    return indent < 0 ? 0 : indent < 50 ? indent : 50;
}

///////////////////////////////////////////////////////////////////////////////

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
)
{
    ASSERT(f);
    ASSERT(tab);

    enum { line_count = 64 };
    char buf[2*line_count];
    char skip_buf[2*line_count];

    indent = wd_normalize_indent(indent);
    const int addr_fw = snprintf(buf,sizeof(buf),"%llx",(u64)addr_factor*size);
    indent += addr_fw; // add address field width

    const u8 * ptr = tab;
    const u8 * tab_end = ptr + size - 1;
    const u8 * tab_min = ptr + used;
    if ( tab_min > ptr )
	tab_min--;
    while ( !*tab_end && tab_end > tab_min )
	tab_end--;
    tab_end++;

    int skip_count = 0;
    u64 skip_addr = 0;

    while ( ptr < tab_end )
    {
	const u64 addr = (u64)( ptr - tab ) * addr_factor;
	
	char * dest = buf;
	const u8 * line_end = ptr + line_count;
	if ( line_end > tab_end )
	    line_end = tab_end;

	int pos = 0, last_count = 0;
	const u8 cmp_val = *ptr;
	while ( ptr < line_end )
	{
	    if ( !( pos++ & 15 ) )
		*dest++ = ' ';
	    last_count += *ptr == cmp_val;
	    *dest++ = chartab[*ptr++];
	}
	*dest = 0;
	DASSERT( dest < buf + sizeof(buf) );
	if ( last_count < line_count )
	{
	    if (skip_count)
	    {
		if ( skip_count == 1 )
		    fprintf(f,"%*llx:%s\n",indent,skip_addr,skip_buf);
		else
		    fprintf(f,"%*llx:%s *%5u\n",indent,skip_addr,skip_buf,skip_count);
		skip_count = 0;
	    }
	    fprintf(f,"%*llx:%s\n",indent,addr,buf);
	}
	else if (!skip_count++)
	{
	    memcpy(skip_buf,buf,sizeof(skip_buf));
	    skip_addr = addr;
	}
    }

    if ( skip_count == 1 )
	fprintf(f,"%*llx:%s\n",indent,skip_addr,skip_buf);
    else if (skip_count)
	fprintf(f,"%*llx:%s *%5u\n",indent,skip_addr,skip_buf,skip_count);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			fake functions			///////////////
///////////////////////////////////////////////////////////////////////////////

unsigned char * wbfs_sha1_fake
	( const unsigned char *d, size_t n, unsigned char *md )
{
    static unsigned char m[WII_HASH_SIZE];
    if (!md)
	md = m;
    memset(md,0,sizeof(*md));
    return md;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

