
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

#define _GNU_SOURCE 1
#include <errno.h>

#include "lib-bmg.h"
#include "dclib-utf8.h"
#include "dclib-xdump.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			compiler options		///////////////
///////////////////////////////////////////////////////////////////////////////

#if 1
  #define ENABLE_BMG_REGEX 1
#else
  #define ENABLE_BMG_REGEX 0
#endif

#if ENABLE_BMG_REGEX
 #include "dclib-regex.h"
#endif

//-----------------------------------------------------------------------------

#if defined(TEST) || defined(DEBUG) || HAVE_WIIMM_EXT
  #define COMPARE_CREATE 1	// 0=off, 1=on
#else
  #define COMPARE_CREATE 0
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    global bmg options			///////////////
///////////////////////////////////////////////////////////////////////////////

uint		opt_bmg_force_count	= 0;	// >0: force some operations
uint		opt_bmg_align		= 0x20;	// alignment of raw-bg sections
bool		opt_bmg_export		= 0;	// true: optimize for exports
bool		opt_bmg_no_attrib	= 0;	// true: suppress attributes
bool		opt_bmg_x_escapes	= 0;	// true: use x-scapes insetad of z-escapes
bool		opt_bmg_old_escapes	= 0;	// true: use only old escapes
int		opt_bmg_single_line	= 0;	// >0: single line, >1: suppress atrributes
bool		opt_bmg_support_mkw	= 0;	// true: support MKW specific extensions
bool		opt_bmg_support_ctcode	= 0;	// true: support MKW/CT-CODE specific extensions
bool		opt_bmg_support_lecode	= 0;	// true: support MKW/LE-CODE specific extensions
uint		opt_bmg_rcup_fill_limit	= 0;	// >0: limit racing cups to reduce BMG size
uint		opt_bmg_track_fill_limit= 0;	// >0: limit tracks to reduce BMG size
bool		opt_bmg_inline_attrib	= true;	// true: print attrinbutes inline
int		opt_bmg_colors		= 1;	// use c-escapss: 0:off, 1:auto, 2:on
ColNameLevelBMG	opt_bmg_color_name	= BMG_CNL_ALL;
uint		opt_bmg_max_recurse	= 10;	// max recurse depth
bool		opt_bmg_allow_print	= 0;	// true: allow '$' to print a log message
bool		opt_bmg_use_slots	= 0;	// true: use predifined slots
bool		opt_bmg_use_raw_sections= 0;	// true: use raw sections

//-----------------------------------------------------------------------------

int		opt_bmg_endian		= BMG_AUTO_ENDIAN;
int		opt_bmg_encoding	= BMG_ENC__UNDEFINED;
uint		opt_bmg_inf_size	= 0;
OffOn_t		opt_bmg_mid		= OFFON_AUTO;
bool		opt_bmg_force_attrib	= false;
bool		opt_bmg_def_attrib	= false;

u8		bmg_force_attrib[BMG_ATTRIB_SIZE];
u8		bmg_def_attrib[BMG_ATTRIB_SIZE];

bmg_t		*bmg_macros		= 0;

//-----------------------------------------------------------------------------

const KeywordTab_t PatchKeysBMG[] =
{
	// 'opt'
	//  0: no param
	//  1: file param
	//  2: string param

	{ BMG_PM_PRINT,		"PRINT",	0,		1 },
	{ BMG_PM_FORMAT,	"FORMAT",	0,		2 },
    #if ENABLE_BMG_REGEX
	{ BMG_PM_REGEX,		"REGEXP",	0,		2 },
	{ BMG_PM_RM_REGEX,	"RM-REGEXP",	"RMREGEXP",	2 },
    #endif
	{ BMG_PM_ID,		"ID",		0,		0 },
	{ BMG_PM_ID_ALL,	"ID-ALL",	"IDALL",	0 },
	{ BMG_PM_UNICODE,	"UNICODE",	0,		0 },
	{ BMG_PM_RM_ESCAPES,	"RM-ESCAPES",	"RMESCAPES",	0 },

	{ BMG_PM_REPLACE,	"REPLACE",	0,		1 },
	{ BMG_PM_INSERT,	"INSERT",	0,		1 },
	{ BMG_PM_OVERWRITE,	"OVERWRITE",	0,		1 },
	{ BMG_PM_DELETE,	"DELETE",	0,		1 },
	{ BMG_PM_MASK,		"MASK",		0,		1 },
	{ BMG_PM_EQUAL,		"EQUAL",	0,		1 },
	{ BMG_PM_NOT_EQUAL,	"NOTEQUAL",	"NEQUAL",	1 },

	{ BMG_PM_GENERIC,	"GENERIC",	0,		0 },
	{ BMG_PM_RM_CUPS,	"RM-CUPS",	"RMCUPS",	0 },

	{ BMG_PM_CT_COPY,	"CT-COPY",	"CTCOPY",	0 },
	{ BMG_PM_CT_FORCE_COPY,	"CT-FORCE-COPY","CTFORCECOPY",	0 },
	{ BMG_PM_CT_FILL,	"CT-FILL",	"CTFILL",	0 },

	{ BMG_PM_LE_COPY,	"LE-COPY",	"LECOPY",	0 },
	{ BMG_PM_LE_FORCE_COPY,	"LE-FORCE-COPY","LEFORCECOPY",	0 },
	{ BMG_PM_LE_FILL,	"LE-FILL",	"LEFILL",	0 },

	{ BMG_PM_X_COPY,	"X-COPY",	"XCOPY",	0 },
	{ BMG_PM_X_FORCE_COPY,	"X-FORCE-COPY",	"XFORCECOPY",	0 },
	{ BMG_PM_X_FILL,	"X-FILL",	"XFILL",	0 },

	{ BMG_PM_RM_FILLED,	"RM-FILLED",	"XFILLED",	0 },

	{ 0,0,0,0 }
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////		       endian helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static const endian_func_t * get_bmg_endian ( const bmg_t * bmg )
{
    return opt_bmg_endian == BMG_LITTLE_ENDIAN
	 ? &le_func
	 : opt_bmg_endian == BMG_BIG_ENDIAN || !bmg
	 ? &be_func
	 : bmg->endian;
}

//-----------------------------------------------------------------------------

static const endian_func_t * get_endian_by_bh ( const bmg_header_t * bh, uint size )
{
    DASSERT(bh);
    if	(  be_func.n2hl(bh->size) <= size
	&& be_func.n2hl(bh->n_sections) <= BMG_MAX_SECTIONS
	)
    {
	return &be_func;
    }

    if	(  le_func.n2hl(bh->size) <= size
	&& le_func.n2hl(bh->n_sections) <= BMG_MAX_SECTIONS
	)
    {
	return &le_func;
    }

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		       print & scan string16		///////////////
///////////////////////////////////////////////////////////////////////////////

static const KeywordTab_t bmg_color[] =
{
    { 0x0000,	"OFF",		"NONE",		BMG_CNL_BASICS },
    { 0x0000,	"GRAY",		"GREY",		BMG_CNL_BASICS },
    { 0x0001,	"RED4",		0,		BMG_CNL_BASICS },
    { 0x0002,	"WHITE",	0,		BMG_CNL_BASICS },

    { 0x0008,	"TRANSP",	"TRANSPARENT",	BMG_CNL_BASICS },
    { 0x0008,	"CLEAR",	0,		BMG_CNL_BASICS },

    { 0x0010,	"YOR0",		"YR0",		BMG_CNL_YOR },
    { 0x0011,	"YOR1",		"YR1",		BMG_CNL_YOR },
    { 0x0012,	"YOR2",		"YR2",		BMG_CNL_YOR },
    { 0x0013,	"YOR3",		"YR3",		BMG_CNL_YOR },
    { 0x0014,	"YOR4",		"YR4",		BMG_CNL_YOR },
    { 0x0015,	"YOR5",		"YR5",		BMG_CNL_YOR },
    { 0x0016,	"YOR6",		"YR6",		BMG_CNL_YOR },
    { 0x0017,	"YOR7",		"YR7",		BMG_CNL_YOR },

    { 0x0020,	"RED2",		0,		BMG_CNL_BASICS },
    { 0x0021,	"BLUE1",	"BLUE",		BMG_CNL_BASICS },

    { 0x0030,	"YELLOW",	0,		BMG_CNL_BASICS },
    { 0x0031,	"BLUE2",	0,		BMG_CNL_BASICS },
    { 0x0032,	"RED3",		0,		BMG_CNL_BASICS },
    { 0x0033,	"GREEN",	0,		BMG_CNL_BASICS },

    { 0x0040,	"RED1",		"RED",		BMG_CNL_BASICS },

    {0,0,0,0}
};

///////////////////////////////////////////////////////////////////////////////

uint GetWordLength16BMG ( const u16 *source )
{
    DASSERT(source);
    u16 code = ntohs(*source);
    if ( code != 0x1a )
	return 1;
    const uint len = ( ((u8*)source)[2] + 1 & 0xfe ) / 2;
    return len ? len : 1;
}

///////////////////////////////////////////////////////////////////////////////

uint GetLength16BMG
(
    const u16		*source,	// source
    uint		max_len		// max possible length
)
{
    DASSERT(source);

    const u16 *src = source, *src_end = src + max_len;

    while ( src < src_end )
    {
	u16 code = ntohs(*src++);
	if (!code)
	    return src - source - 1;

	if ( code == 0x1a )
	{
	    // nintendos special escape sequence!
	    const uint len = *(u8*)src+1 & 0xfe;
	    if ( len > 0 )
		src += len - 1;
	}
    }

    const uint len = src - source;
    return len < max_len ? len : max_len;
}

///////////////////////////////////////////////////////////////////////////////
#undef TEST_MODE // 0:off, 1:compare, 2:replace
#if HAVE_WIIMM_EXT
  #define TEST_MODE 1
#else
  #define TEST_MODE 0
#endif
//--------------------------------------------------

uint PrintString16BMG
(
    char		*buf,		// destination buffer
    uint		buf_size,	// size of 'buf'
    const u16		*src,		// source
    int			src_len,	// length of source in u16 words, -1:NULL term
    u16			utf8_max,	// max code printed as UTF-8
    uint		quote,		// 0:no quotes, 1:escape ", 2: print in quotes
    int			use_color	// 0:no, 1:use, 2:use all
)
{
    DASSERT(buf);
    DASSERT( buf_size >= 100 );
    DASSERT(src);

 //--------------------------------------------------
 #if TEST_MODE > 1
 //--------------------------------------------------

    if ( buf_size < 2 )
    {
	*buf = 0;
	return 0;
    }

    struct { FastBuf_t b; char space[1000]; } fb;
    InitializeFastBuf(&fb,sizeof(fb));
    PrintFastBuf16BMG(&fb.b,src,src_len,utf8_max,quote,use_color);
    mem_t mem = GetFastBufMem(&fb.b);

    if ( mem.len >= buf_size )
	mem.len = buf_size - 1;
    memcpy(buf,mem.ptr,mem.len);
    buf[mem.len] = 0;
    ResetFastBuf(&fb.b);
    return mem.len;

 //--------------------------------------------------
 #else // TEST_MODE <= 1
 //--------------------------------------------------

  #if TEST_MODE == 1
    struct { FastBuf_t b; char space[1000]; } fb;
    InitializeFastBuf(&fb,sizeof(fb));
    PrintFastBuf16BMG(&fb.b,src,src_len,utf8_max,quote,use_color);
  #endif

    if ( buf_size < 30 )
    {
 #if TEST_MODE == 1
    ResetFastBuf(&fb.b);
 #endif
	*buf = 0;
	return 0;
    }

    #undef PRINT_UTF8_MAX
    //#define PRINT_UTF8_MAX 0xdb7f

    if ( utf8_max < 0x7f )
	utf8_max = 0x7f;
 #ifdef PRINT_UTF8_MAX
    else if ( utf8_max > PRINT_UTF8_MAX )
	utf8_max = PRINT_UTF8_MAX;
 #endif

    char *dest = buf, *last_u = 0;
    char *dest_end = dest + buf_size - 25;
    const u16 * src_end = src + ( src_len < 0 ? buf_size : src_len );

    if (quote>1)
	*dest++ = '"';

    while ( dest < dest_end && src < src_end )
    {
	u16 code = ntohs(*src++);
	if ( !code && src_len < 0 )
	    break;

	if ( code == 0x1a )
	{
	    // nintendos special escape sequence!
	    uint total_words = *(u8*)src+1 >> 1;
	    if ( total_words >= 3 && src + total_words - 1 <= src_end )
	    {
		//HexDump(stdout,1,0,2,2*total_words,src-1,2*total_words);
		const u16 code2 = ntohs(*src++);
		if ( code2 == 0x800 && use_color > 0 && be16(src) == 1 )
		{
		    const u16 color = be16(src+1);
		    const KeywordTab_t *kt;
		    for ( kt = bmg_color; kt->name1 && kt->id != color; kt++ )
			;
		    if ( kt->name1 && opt_bmg_color_name >= kt->opt )
		    {
			*dest++ = '\\';
			*dest++ = 'c';
			*dest++ = '{';
			ccp name = kt->name1;
			while (*name)
			    *dest++ = tolower((int)*name++);
			*dest++ = '}';
		    }
		    else
			dest += sprintf(dest,"\\c{%x}",color);
		    src += 2;
		    continue;
		}

		if ( code2 == 0x801 )
		{
		    if ( opt_bmg_old_escapes || last_u != dest )
			dest = snprintfE(dest,dest_end,"\\u{%x}",be32(src));
		    else
			dest = snprintfE(dest-1,dest_end,",%x}",be32(src));
		    last_u = dest;
		    src += 2;
		    continue;
		}

		if ( ( total_words > 6 || opt_bmg_x_escapes ) && opt_bmg_old_escapes )
		{
		    src -= 2;
		    while ( total_words-- > 0 )
			dest = snprintfE(dest,dest_end,"\\x{%x}",ntohs(*src++));
		    continue;
		}
		else if ( opt_bmg_x_escapes )
		{
		    if ( dest < dest_end )
		    {
			dest = StringCopyE(dest,dest_end,"\\x{1a");
			src--;
			while ( total_words-- > 1 )
			    dest = snprintfE(dest,dest_end,",%x",ntohs(*src++));
		    }
		    *dest++ = '}';
		    continue;
		}

		u64 xcode = 0;
		switch( total_words & 3 )
		{
		    case 3 & 3: xcode = be16(src); src += 1; break;
		    case 4 & 3: xcode = be32(src); src += 2; break;
		    case 5 & 3: xcode = be48(src); src += 3; break;
		    case 6 & 3: xcode = be64(src); src += 4; break;
		}

		dest = snprintfE(dest,dest_end,"\\z{%x,%llx",code2,xcode);
		while ( total_words > 6 )
		{
		    dest = snprintfE(dest,dest_end,",%llx",be64(src));
		    src += 4;
		    total_words -= 4;
		}
		*dest++ = '}';
		continue;
	    }
	}

	if ( code < ' ' || code == 0x7f || code == 0xff )
	{
	    *dest++ = '\\';
	    switch(code)
	    {
		case '\\': *dest++ = '\\'; break;
		case '\a': *dest++ = 'a'; break;
		case '\b': *dest++ = 'b'; break;
		case '\f': *dest++ = 'f'; break;
		case '\n': *dest++ = 'n'; break;
		case '\r': *dest++ = 'r'; break;
		case '\t': *dest++ = 't'; break;
		case '\v': *dest++ = 'v'; break;

		default:
		    *dest++ = ( code >> 6 & 3 ) | '0';
		    *dest++ = ( code >> 3 & 7 ) | '0';
		    *dest++ = ( code      & 7 ) | '0';
	    }
	}
     #ifdef PRINT_UTF8_MAX
	else if ( code > utf8_max )
	    dest += sprintf(dest,"\\x{%x}",code);
     #else
	else if ( quote && code == '"' )
	{
	    *dest++ = '\\';
	    *dest++ = '"';
	}
	else if ( code > utf8_max
		|| code >= 0xdb80 && code <= 0xf8ff	// surrogates & private use
		|| ( code & 0xffff ) >= 0xfffe		// invalid
		)
	{
	    dest = snprintfE(dest,dest_end,"\\x{%x}",code);
	}
     #endif
	else
	    dest = PrintUTF8Char(dest,code);
    }

    if ( dest > buf && dest[-1] == ' ' )
    {
	dest[-1] = '\\';
	*dest++ = '0';
	*dest++ = '4';
	*dest++ = '0';
    }

    if (quote>1)
	*dest++ = '"';

    *dest = 0;

  #if TEST_MODE == 1
    const mem_t mem	= GetFastBufMem(&fb.b);
    if ( mem.len < buf_size )
    {

	XDump_t xd;
	InitializeXDump(&xd);
	xd.f		= stderr;
	xd.indent	= 2;
	xd.assumed_size	= mem.len;
	XDiff( &xd, buf,dest-buf,true, mem.ptr,mem.len,true, 5,false );
    }
    ResetFastBuf(&fb.b);
  #endif

    return dest - buf;

 //--------------------------------------------------
 #endif // TEST_MODE <= 1
 //--------------------------------------------------
}

///////////////////////////////////////////////////////////////////////////////

uint PrintFastBuf16BMG
(
    // return the number of appended bytes

    FastBuf_t		*fb,		// destination buffer (data appended)
    const u16		*src,		// source
    int			src_len,	// length of source in u16 words, -1:NULL term
    u16			utf8_max,	// max code printed as UTF-8
    uint		quote,		// 0:no quotes, 1:escape ", 2: print in quotes
    int			use_color	// >0: use \c{...}
)
{
    DASSERT(fb);
    DASSERT(src);

    #undef PRINT_UTF8_MAX
    //#define PRINT_UTF8_MAX 0xdb7f

    if ( utf8_max < 0x7f )
	utf8_max = 0x7f;
 #ifdef PRINT_UTF8_MAX
    else if ( utf8_max > PRINT_UTF8_MAX )
	utf8_max = PRINT_UTF8_MAX;
 #endif

    const uint start_len = GetFastBufLen(fb);

    char buf[1300];
    char *dest		= buf;
    char *dest_term	= dest + sizeof(buf) - 1;
    char *dest_end	= dest_term - 270; // escape sequence with 256 bytes possible
    char *last_u	= 0;

    if ( src_len < 0 )
	src_len = GetLength16BMG(src,1000000);
    const u16 * src_end = src + src_len;

    if (quote>1)
	*dest++ = '"';

    while ( src < src_end )
    {
	if ( dest >= dest_end )
	{
	    AppendFastBuf(fb,buf,dest-buf);
	    dest = buf;
	    last_u = 0;
	}

	u16 code = ntohs(*src++);
	if ( !code && src_len < 0 )
	    break;

	if ( code == 0x1a )
	{
	    // nintendos special escape sequence!
	    uint total_words = *(u8*)src+1 >> 1;
	    if ( total_words >= 3 && src + total_words - 1 <= src_end )
	    {
		//HexDump(stdout,1,0,2,2*total_words,src-1,2*total_words);
		const u16 code2 = ntohs(*src++);
		if ( code2 == 0x800 && use_color > 0 && be16(src) == 1 )
		{
		    const u16 color = be16(src+1);
		    const KeywordTab_t *kt;
		    for ( kt = bmg_color; kt->name1 && kt->id != color; kt++ )
			;
		    if ( kt->name1 && opt_bmg_color_name >= kt->opt )
		    {
			*dest++ = '\\';
			*dest++ = 'c';
			*dest++ = '{';
			ccp name = kt->name1;
			while (*name)
			    *dest++ = tolower((int)*name++);
			*dest++ = '}';
		    }
		    else
			dest += sprintf(dest,"\\c{%x}",color);
		    src += 2;
		    continue;
		}

		if ( code2 == 0x801 )
		{
		    if ( opt_bmg_old_escapes || last_u != dest )
			dest = snprintfE(dest,dest_term,"\\u{%x}",be32(src));
		    else
			dest = snprintfE(dest-1,dest_term,",%x}",be32(src));
		    last_u = dest;
		    src += 2;
		    continue;
		}

		if ( ( total_words > 6 || opt_bmg_x_escapes ) && opt_bmg_old_escapes )
		{
		    src -= 2;
		    while ( total_words-- > 0 )
			dest = snprintfE(dest,dest_term,"\\x{%x}",ntohs(*src++));
		    continue;
		}
		else if ( opt_bmg_x_escapes )
		{
		    if ( dest < dest_term )
		    {
			dest = StringCopyE(dest,dest_term,"\\x{1a");
			src--;
			while ( total_words-- > 1 )
			    dest = snprintfE(dest,dest_term,",%x",ntohs(*src++));
		    }
		    *dest++ = '}';
		    continue;
		}

		u64 xcode = 0;
		switch( total_words & 3 )
		{
		    case 3 & 3: xcode = be16(src); src += 1; break;
		    case 4 & 3: xcode = be32(src); src += 2; break;
		    case 5 & 3: xcode = be48(src); src += 3; break;
		    case 6 & 3: xcode = be64(src); src += 4; break;
		}

		dest = snprintfE(dest,dest_term,"\\z{%x,%llx",code2,xcode);
		while ( total_words > 6 )
		{
		    dest = snprintfE(dest,dest_term,",%llx",be64(src));
		    src += 4;
		    total_words -= 4;
		}
		*dest++ = '}';
		continue;
	    }
	}

	if ( code < ' ' || code == 0x7f || code == 0xff )
	{
	    *dest++ = '\\';
	    switch(code)
	    {
		case '\\': *dest++ = '\\'; break;
		case '\a': *dest++ = 'a'; break;
		case '\b': *dest++ = 'b'; break;
		case '\f': *dest++ = 'f'; break;
		case '\n': *dest++ = 'n'; break;
		case '\r': *dest++ = 'r'; break;
		case '\t': *dest++ = 't'; break;
		case '\v': *dest++ = 'v'; break;

		default:
		    *dest++ = ( code >> 6 & 3 ) | '0';
		    *dest++ = ( code >> 3 & 7 ) | '0';
		    *dest++ = ( code      & 7 ) | '0';
	    }
	}
     #ifdef PRINT_UTF8_MAX
	else if ( code > utf8_max )
	    dest += sprintf(dest,"\\x{%x}",code);
     #else
	else if ( quote && code == '"' )
	{
	    *dest++ = '\\';
	    *dest++ = '"';
	}
	else if ( code > utf8_max
		|| code >= 0xdb80 && code <= 0xf8ff	// surrogates & private use
		|| ( code & 0xffff ) >= 0xfffe		// invalid
		)
	{
	    dest = snprintfE(dest,dest_term,"\\x{%x}",code);
	}
     #endif
	else
	    dest = PrintUTF8Char(dest,code);
    }

    if ( dest > buf && dest[-1] == ' ' )
    {
	dest[-1] = '\\';
	*dest++ = '0';
	*dest++ = '4';
	*dest++ = '0';
    }

    if (quote>1)
	*dest++ = '"';

    AppendFastBuf(fb,buf,dest-buf);
    return GetFastBufLen(fb) - start_len;
}

///////////////////////////////////////////////////////////////////////////////

#if 0
uint ScanFastBuf16BMG
(

    FastBuf_t		*fb,		// destination buffer
    ccp			src,		// UTF-8 source
    int			src_len,	// length of source, -1: determine with strlen()
    const bmg_t		*bmg		// NULL or support for \m{...}
);
#endif

uint ScanString16BMG
(
    u16			* buf,		// destination buffer
    uint		buf_size,	// size of 'buf' in u16 words
    ccp			src,		// UTF-8 source
    int			src_len,	// length of source, -1: determine with strlen()
    const bmg_t		*bmg		// NULL or support for \m{...}
)
{
    DASSERT(buf);
    DASSERT(src);

    u16 * dest = buf;
    u16 * dest_end = dest + buf_size;
    ccp src_end = src + ( src_len < 0 ? strlen(src) : src_len );

    while ( dest < dest_end && src < src_end )
    {
	if ( *src == '\\' && src+1 < src_end )
	{
	    src++;
	    switch(*src++)
	    {
		case '\\': *dest++ = htons('\\'); break;
		case 'a':  *dest++ = htons('\a'); break;
		case 'b':  *dest++ = htons('\b'); break;
		case 'f':  *dest++ = htons('\f'); break;
		case 'n':  *dest++ = htons('\n'); break;
		case 'r':  *dest++ = htons('\r'); break;
		case 't':  *dest++ = htons('\t'); break;
		case 'v':  *dest++ = htons('\v'); break;

		case 'm':
		case 'M':
		    if ( src < src_end && *src == '{' )
		    {
			const char mode = src[-1];
			src++;
			bmg_scan_mid_t scan;
			for(;;)
			{
			    while ( src < src_end && *src == ',' )
				src++;
			    if ( ScanBMGMID(&scan,0,src,src_end) < 1 )
				break;
			    src = scan.scan_end;

			    PRINT("\\M{%x} : bmg=%d\n",scan.mid[0],bmg!=0);

			    const bmg_item_t *dptr = 0;
			    if ( bmg && mode == 'm' )
				dptr = FindItemBMG(bmg,scan.mid[0]);
			    if ( !dptr && bmg_macros )
				dptr = FindItemBMG(bmg_macros,scan.mid[0]);

			    if (!dptr)
			    {
				char buf[16];
				snprintf(buf,sizeof(buf),"\\%c{%x}",mode,scan.mid[0]);
				ccp src = buf;
				while ( *src && dest < dest_end )
				    *dest++ = htons(*src++);
			    }
			    else if ( dptr->text )
			    {
				const u16 *copy = dptr->text;
				const u16 *copy_end = copy + dptr->len;
				while ( copy < copy_end && dest < dest_end )
				    *dest++ = *copy++;
			    }
			    if ( *src != ',' )
				break;
			}
			if ( *src == '}' )
			    src++;
		    }
		    break;

		case 'x':
		    {
			const bool have_brace = *src == '{';
			if (have_brace)
			    src++;
			for(;;)
			{
			    char *end;
			    u16 code = strtoul(src,&end,16);
			    src = end;
			    if ( dest < dest_end )
				*dest++ = htons(code);
			    if ( !have_brace || *src != ',' )
				break;
			    src++;
			}
			if ( have_brace && *src == '}' )
			    src++;
		    }
		    break;

		case 'z':
		    {
			const bool have_brace = *src == '{';
			if (have_brace)
			    src++;
			char * end;
			const u16 code1 = strtoul(src,&end,16);
			src = end;
			uint total_words = code1+0x100 >> 9;
			if ( dest + total_words <= dest_end )
			{
			    *dest++ = htons(0x1a);
			    *dest++ = htons(code1);
			    u64 xcode = 0;
			    if ( *src == ',' )
			    {
				xcode = strtoull(src+1,&end,16);
				src = end;
			    }
			    switch( total_words & 3 )
			    {
				case 3 & 3: write_be16(dest,xcode); dest += 1; break;
				case 4 & 3: write_be32(dest,xcode); dest += 2; break;
				case 5 & 3: write_be48(dest,xcode); dest += 3; break;
				case 6 & 3: write_be64(dest,xcode); dest += 4; break;
			    }
			    while ( total_words > 6 )
			    {
				total_words -= 4;
				if ( *src == ',' )
				{
				    xcode = strtoull(src+1,&end,16);
				    src = end;
				}
				else
				    xcode = 0;
				write_be64(dest,xcode);
				dest += 4;
			    }
			    while ( *src == ',' )
			    {
				strtoull(src+1,&end,16);
				src = end;
			    }
			}
			if ( have_brace && *src == '}' )
			    src++;
		    }
		    break;

		case 'u':
		    {
			const bool have_brace = *src == '{';
			if (have_brace)
			    src++;
			for(;;)		// >>>>>  comma list support since v1.44  <<<<<
			{
			    char * end;
			    u32 code = strtoul(src,&end,16);
			    src = end;
			    if ( dest + 4 <= dest_end )
			    {
				*dest++ = htons(0x001a);
				*dest++ = htons(0x0801);
				*dest++ = htons(code>>16);
				*dest++ = htons(code);
			    }
			    if ( !have_brace || *src != ',' )
				break;
			    src++;
			}
			if ( have_brace && *src == '}' )
			    src++;
		    }
		    break;

		case 'c':
		    {
			const bool have_brace = *src == '{';
			if (have_brace)
			    src++;

			u16 code = 0;
			if ( *src >= '0' && *src <= '9' )
			{
			    char *end;
			    code = strtoul(src,&end,16);
			    src = end;
			}
			else
			{
			    char keyname[50], *kp = keyname;
			    while (isalnum((int)*src))
			    {
				if ( kp < keyname + sizeof(keyname) - 1 )
				    *kp++ = *src;
				src++;
			    }
			    *kp = 0;

			    const KeywordTab_t *key = ScanKeyword(0,keyname,bmg_color);
			    if (key)
				code = key->id;
			    else
				code = strtoul(keyname,0,16);
			}
			if ( have_brace && *src == '}' )
			    src++;

			if ( dest + 4 <= dest_end )
			{
			    *dest++ = htons(0x001a);
			    *dest++ = htons(0x0800);
			    *dest++ = htons(0x0001);
			    *dest++ = htons(code);
			}
		    }
		    break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		    {
			src--;
			u16 code = 0;
			int idx;
			for ( idx = 0; idx < 3 && *src >= '0' && *src <= '7'; idx++ )
			    code = code << 3 | *src++ - '0';
			*dest++ = htons(code);
		    }
		    break;

		default:
		    *dest++ = htons('\\');
		    src--;
		    break;
	    }
	}
	else
	    *dest++ = htons(ScanUTF8AnsiChar(&src));
    }

    return dest - buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static unsigned long int strtoul16
(
    const u16		*  ptr16,
    u16			** end16,
    int			base
)
{
    DASSERT(ptr16);

    char buf[100];
    char * dest = buf;
    const u16 * p16 = ptr16;
    while ( dest < buf + sizeof(buf)-1 )
    {
	int ch = be16(p16++);
	if ( ch >= '0' && ch <= '9'
	    || ch >= 'a' && ch <= 'f'
	    || ch >= 'A' && ch <= 'F'
	    || ch == 'x'
	    || ch == 'X'
	    || isspace(ch)
	    )
	{
	    *dest++ = ch;
	}
	else
	    break;
    }
    DASSERT( dest < buf + sizeof(buf) );
    *dest = 0;

    char *end;
    unsigned long num = strtoul(buf,&end,base);
    if (end16)
	*end16 = (u16*)ptr16 + (end-buf);
    return num;
}

///////////////////////////////////////////////////////////////////////////////

static u16 * skip_ctrl16 ( const u16 * ptr )
{
    DASSERT(ptr);
    for(;;)
    {
	const u16 ch = be16(ptr);
	if ( !ch || ch > ' ' )
	    return (u16*)ptr;
	ptr++;
    }
}

///////////////////////////////////////////////////////////////////////////////

static u16 * ScanRange16U32
(
    const u16		* arg,		// text to scan (valid pointer)
    u32			* p_stat,	// NULL or return status:
					//   number of scanned numbers (0..2)
    u32			* p_n1,		// first return value (valid pointer)
    u32			* p_n2,		// second return value (valid pointer)
    u32			min,		// min allowed value (fix num)
    u32			max		// max allowed value (fix num)
)
{
    DASSERT(arg);
    DASSERT(p_n1);
    DASSERT(p_n2);
    TRACE("ScanRange16U32()\n");

    int stat = 0;

    u32 n1 = ~(u32)0, n2 = 0;
    arg = skip_ctrl16(arg);

    if ( be16(arg) == '-' )
	n1 = min;
    else
    {
	u16 * end;
	u32 num = strtoul16(arg,&end,0);
	if ( arg == end )
	    goto abort;

	stat = 1;
	arg = skip_ctrl16(end);
	n1 = num;
    }

    if ( be16(arg) != '-' )
    {
	stat = 1;
	n2 = n1;
	goto abort;
    }

    arg = skip_ctrl16(arg+1);
    u16 * end;
    n2 = strtoul16(arg,&end,0);
    if ( end == arg )
	n2 = max;
    stat = 2;
    arg = end;

 abort:

    if ( stat > 0 )
    {
	if ( n1 < min )
	    n1 = min;
	if ( n2 > max )
	    n2 = max;
    }

    if ( !stat || n1 > n2 )
    {
	stat = 0;
	n1 = ~(u32)0;
	n2 = 0;
    }

    if (p_stat)
	*p_stat = stat;
    *p_n1 = n1;
    *p_n2 = n2;

    arg = skip_ctrl16(arg);

    TRACE("END ScanRange16U32() stat=%u, n=%u..%u\n",stat,n1,n2);
    return (u16*)arg;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanSectionsBMG()		///////////////
///////////////////////////////////////////////////////////////////////////////

bmg_sect_list_t * ScanSectionsBMG
	( cvp data, uint size, const endian_func_t *endian )
{
    //--- initialize

    if ( !data || size < sizeof(bmg_header_t) || memcmp(data,BMG_MAGIC,8) )
	return 0;

    bmg_sect_list_t sl = {0};
    sl.header = (bmg_header_t*)data;

    if (!endian)
    {
	endian = get_endian_by_bh(sl.header,size);
	if (!endian)
	    return 0;
    }

    sl.endian		= endian;
    sl.source_size	= endian->n2hl(sl.header->size);
    sl.n_sections	= endian->n2hl(sl.header->n_sections);

    FastBuf_t fb;
    InitializeFastBufAlloc(&fb,10*sizeof(bmg_sect_info_t));

    bmg_sect_info_t si;


    //--- scan sections

    u8 *data_end = (u8*)data + size;
    bmg_section_t *sect = (bmg_section_t*)(sl.header+1);
    for(;;)
    {
	if ( (u8*)sect + sizeof(*sect) > data_end )
	    break;

	memset(&si,0,sizeof(si));
	si.offset = (u8*)sect - (u8*)data;
	si.sect_size = si.real_size = endian->n2hl(sect->size);
	if ( (u8*)sect + si.real_size > data_end )
	    si.real_size = data_end - (u8*)sect;

	si.sect = sect;
	si.info = "?";

	if (!memcmp(sect->magic,BMG_INF_MAGIC,sizeof(sect->magic)))
	{
	    sl.pinf		= (bmg_inf_t*)sect;
	    si.head_size	= sizeof(bmg_inf_t);
	    si.elem_size	= endian->n2hs(sl.pinf->inf_size);
	    si.n_elem_head	= endian->n2hs(sl.pinf->n_msg);
	    si.known		= true;
	    si.supported	= true;
	    si.info		= "offset & attributes";
	}
	else if (!memcmp(sect->magic,BMG_DAT_MAGIC,sizeof(sect->magic)))
	{
	    sl.pdat		= (bmg_dat_t*)sect;
	    si.head_size	= sizeof(bmg_dat_t);
	    si.known		= true;
	    si.supported	= true;
	    si.info		= "string pool";
	}
	else if (!memcmp(sect->magic,BMG_STR_MAGIC,sizeof(sect->magic)))
	{
	    sl.pstr		= (bmg_str_t*)sect;
	    si.head_size	= sizeof(bmg_str_t);
	    si.known		= true;
	    si.supported	= false;
	    si.info		= "secondary string pool";
	}
	else if (!memcmp(sect->magic,BMG_MID_MAGIC,sizeof(sect->magic)))
	{
	    sl.pmid		= (bmg_mid_t*)sect;
	    si.head_size	= sizeof(bmg_mid_t);
	    si.elem_size	= sizeof(u32);
	    si.n_elem_head	= endian->n2hs(sl.pinf->n_msg);
	    si.known		= true;
	    si.supported	= true;
	    si.info		= "message ids";
	}
	else if (!memcmp(sect->magic,BMG_FLW_MAGIC,sizeof(sect->magic)))
	{
	    sl.pflw		= (bmg_flw_t*)sect;
	    si.known		= true;
	}
	else if (!memcmp(sect->magic,BMG_FLI_MAGIC,sizeof(sect->magic)))
	{
	    sl.pfli		= (bmg_fli_t*)sect;
	    si.known		= true;
	}

	if (si.elem_size)
	{
	    si.n_elem = ( si.real_size - si.head_size ) / si.elem_size;
	    if ( si.n_elem_head && si.n_elem > si.n_elem_head )
		si.n_elem = si.n_elem_head;
	}

	if ( si.head_size && si.head_size < si.real_size )
	{
	    si.is_empty = true;
	    const u8 *ptr = (u8*)sect + si.head_size;
	    const u8 *end = (u8*)sect + si.real_size;
	    while ( ptr < end )
		if ( *ptr++ )
		{
		    si.is_empty = false;
		    break;
		}
	}

	sect = (bmg_section_t*)( (u8*)sect + si.real_size );
	AppendFastBuf(&fb,&si,sizeof(si));
	sl.n_info++;
    }


    //--- finalize

    memset(&si,0,sizeof(si));
    AppendFastBuf(&fb,&si,sizeof(si));

    mem_t mem			= GetFastBufMem(&fb);
    sl.this_size		= sizeof(sl) + mem.len;
    bmg_sect_list_t *dest	= MALLOC(sl.this_size);
    memcpy(dest,&sl,sizeof(sl));
    memcpy(dest->info,mem.ptr,mem.len);
    ResetFastBuf(&fb);
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

const bmg_sect_info_t * SearchSectionBMG
(
    const bmg_sect_list_t *sl,	// valid, created by ScanSectionsBMG()
    cvp			search,	// magic to search
    bool		abbrev,	// if not found: allow abbreviations _> strcmp()
    bool		icase	// if not found: ignore case and try again
)
{
    DASSERT(sl);

    // make sure, that magic is null terminated
    char magic[5];
    memcpy(magic,search,4);
    magic[4] = 0;


    //--- search exact

    const bmg_sect_info_t *si;
    for ( si = sl->info; si->sect; si++ )
	if (!memcmp(magic,si->sect->magic,4))
	    return si;


    //--- search abbrev

    const uint mlen = strlen(magic);
    if (!mlen)
	abbrev = false;

    if (abbrev)
    {
	for ( si = sl->info; si->sect; si++ )
	    if (!memcmp(magic,si->sect->magic,mlen))
		return si;
    }


    //--- search icase

    if (icase)
    {
	uint i;
	for ( i = 0; i < 4; i++ )
	    magic[i] = tolower((int)magic[i]);

	const bmg_sect_info_t *abbrev_found = 0;
	char buf[5];

	for ( si = sl->info; si->sect; si++ )
	{
	    for ( i = 0; i < 4; i++ )
		buf[i] = tolower((int)si->sect->magic[i]);

	    if (!memcmp(magic,buf,4))
		return si;
	    if ( abbrev && !memcmp(magic,buf,mlen) )
		abbrev_found = si;
	}
	return abbrev_found;
    }

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct bmg_raw_section_t		///////////////
///////////////////////////////////////////////////////////////////////////////

bmg_raw_section_t * GetRawSectionBMG ( bmg_t *bmg, cvp magic )
{
    DASSERT(bmg);
    DASSERT(magic);

    bmg_raw_section_t *raw;
    for ( raw = bmg->first_raw; raw; raw = raw->next )
	if (!memcmp(raw->magic,magic,sizeof(raw->magic)))
	{
	    PRINT("GetRawSectionBMG(%s), found\n",PrintID(magic,4,0));
	    return raw;
	}

    PRINT("GetRawSectionBMG(%s), not found\n",PrintID(magic,4,0));
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

bmg_raw_section_t * CreateRawSectionBMG ( bmg_t *bmg, cvp magic )
{
    bmg_raw_section_t **raw;
    for ( raw = &bmg->first_raw; *raw; raw = &(*raw)->next )
	if (!memcmp((*raw)->magic,magic,sizeof((*raw)->magic)))
	{
	    PRINT("CreateRawSectionBMG(%s), found\n",PrintID(magic,4,0));
	    return (*raw);
	}

    DASSERT(raw);
    PRINT("CreateRawSectionBMG(%s), create\n",PrintID(magic,4,0));
    bmg_raw_section_t *p = CALLOC(1,sizeof(*p));
    memcpy(p->magic,magic,sizeof(p->magic));
    InitializeFastBuf(&p->data,sizeof(p->data));
    return *raw = p;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			attribute helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetAttribBMG
(
    const bmg_t		* bmg,		// pointer to bmg
    bmg_item_t		* item		// dest item
)
{
    DASSERT(bmg);
    DASSERT(item);

    item->slot = BMG_NO_PREDEF_SLOT;
    item->attrib_used = bmg->attrib_used;
    memcpy(item->attrib,bmg->attrib,sizeof(item->attrib));
}

///////////////////////////////////////////////////////////////////////////////

void CopyAttribBMG
(
    const bmg_t		* dest,		// pointer to destination bmg
    bmg_item_t		* dptr,		// dest item
    const bmg_item_t	* sptr		// source item
)
{
    DASSERT(dest);
    DASSERT(dptr);
    DASSERT(sptr);

    DASSERT( dest->attrib_used <= BMG_ATTRIB_SIZE );
    DASSERT( sptr->attrib_used <= BMG_ATTRIB_SIZE );

    if ( sptr->slot != BMG_NO_PREDEF_SLOT )
	dptr->slot = sptr->slot;

    if ( opt_bmg_force_attrib )
    {
	//dptr->attrib_used = dest->attrib_used;
	memcpy(dptr->attrib,bmg_force_attrib,dptr->attrib_used);
    }
    else if ( sptr->attrib_used )
    {
	if ( dest->attrib_used <= sptr->attrib_used )
	    dptr->attrib_used = dest->attrib_used;
	else
	{
	    dptr->attrib_used = sptr->attrib_used;
	    memset( dptr->attrib+dptr->attrib_used, 0, BMG_ATTRIB_SIZE - sptr->attrib_used );
	}
	memcpy(dptr->attrib,sptr->attrib,dptr->attrib_used);
    }
}

///////////////////////////////////////////////////////////////////////////////

static char * scan_attrib ( u8 *dest, uint size, ccp src )
{
    DASSERT(dest);
    DASSERT( size <= BMG_ATTRIB_SIZE );
    memset(dest,0,BMG_ATTRIB_SIZE);

    while ( *src ==  ' ' || *src == '\t' )
	src++;
    const bool have_brackets = *src == '[';
    if ( have_brackets )
	src++;

    int idx = 0, add_idx = 0;
    for(;;)
    {
	while ( *src ==  ' ' || *src == '\t' )
	    src++;

	if ( *src == ',' )
	{
	    src++;
	    idx++;
	    add_idx = 0;
	}
	else if ( *src == '/' )
	{
	    src++;
	    idx = ALIGN32(idx+1,4);
	    add_idx = 0;
	}
	else if ( *src == '%' )
	{
	    src++;
	    char *end;
	    u32 num = str2ul(src,&end,16);
	    if ( src == end )
		break;

	    idx = ALIGN32(idx+add_idx,4);
	    noPRINT("U32[0x%02x/%02x]: %8x\n",idx,size,num);

	    char buf[4];
	    write_be32(buf,num);
	    const int max_copy = (int)size - (int)idx;
	    if ( max_copy > 0 )
		memcpy(dest+idx,buf, max_copy < 4 ? max_copy : 4 );
	    idx += 3;
	    add_idx = 1;
	    src = end;
	}
	else
	{
	    char *end;
	    u8 num = str2ul(src,&end,16);
	    if ( src == end )
		break;

	    idx += add_idx;
	    noPRINT("U8[0x%02x/%02x]: %8x\n",idx,size,num);
	    if ( idx < size )
		dest[idx] = num;
	    add_idx = 1;
	    src = end;
	}
    }
    //HexDump16(stderr,0,0,dest,size);

    if ( have_brackets && *src == ']' )
	src++;
    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

static void copy_param
(
    bmg_t		* dest,		// pointer to destination bmg
    const bmg_t		* src		// pointer to source bmg
)
{
    DASSERT(dest);
    DASSERT(src);

    if ( !dest->param_defined && src->param_defined )
    {
	memcpy(dest->attrib,src->attrib,sizeof(dest->attrib));
	dest->inf_size		= src->inf_size;
	dest->have_mid		= src->have_mid;
	dest->attrib_used	= src->attrib_used;
	dest->use_color_names	= src->use_color_names;
	dest->use_mkw_messages	= src->use_mkw_messages;
	dest->param_defined	= true;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// special values for bmg_item_t::text
u16 bmg_null_entry[] = {0};

//////////////////////////////////////////////////////////////////////////////

void FreeItemBMG( bmg_item_t * bi )
{
    DASSERT(bi);
    if (bi->text)
    {
	if ( bi->alloced_size && !isSpecialEntryBMG(bi->text) )
	    FREE( bi->text);
	bi->text = bmg_null_entry;
    }
    bi->len = bi->alloced_size = 0;
}

///////////////////////////////////////////////////////////////////////////////

void AssignItemText16BMG
(
    bmg_item_t	* bi,			// valid item
    const u16	* text16,		// text to store
    int		len			// length of 'text16'.
					// if -1: detect length by finding
					//        first NULL value.
)
{
    DASSERT(bi);
    if ( !text16 || isSpecialEntryBMG(text16) )
    {
	FreeItemBMG(bi);
	bi->text = (u16*)text16;
	return;
    }

    if ( len < 0 )
    {
	const u16 * ptr = text16;
	while (*ptr)
	    ptr++;
	len = ptr - text16;
    }

    if ( bi->text && bi->alloced_size )
    {
	if ( len+1 <= bi->alloced_size )
	{
	    memcpy(bi->text,text16,len*sizeof(*bi->text));
	    bi->text[len] = 0;
	    bi->len = len;
	    return;
	}
	FREE(bi->text);
    }

    bi->alloced_size = len + 1;
    bi->len = len;
    bi->text = CALLOC(bi->alloced_size,sizeof(*bi->text));
    memcpy(bi->text,text16,len*sizeof(*bi->text));
}

///////////////////////////////////////////////////////////////////////////////

void AssignItemScanTextBMG
(
    bmg_item_t		* bi,		// valid item
    ccp			utf8,		// UTF-8 text to scan
    int			len		// length of 'utf8'.
					// if -1: detect length with strlen()
)
{
    DASSERT(bi);

    if (!utf8)
	AssignItemText16BMG(bi,0,0);
    else
    {
	const uint bufsize = 10000;
	u16 buf[bufsize];
	len = ScanString16BMG(buf,bufsize,utf8,len,0);
	AssignItemText16BMG(bi,buf,len);
    }
}

///////////////////////////////////////////////////////////////////////////////

void AssignItemTextBMG
(
    bmg_item_t	* bi,			// valid item
    ccp		utf8,			// UTF-8 text to store
    int		len			// length of 'utf8'.
					// if -1: detect length with strlen()
)
{
    DASSERT(bi);

    if (!utf8)
	AssignItemText16BMG(bi,0,0);
    else
    {
	u16 buf[10000];
	u16 *ptr = buf;
	u16 *end = buf + sizeof(buf)/sizeof(*buf);

	if ( len < 0 )
	{
	    while ( ptr < end )
	    {
		ulong code = ScanUTF8AnsiChar(&utf8);
		if (!code)
		    break;
		*ptr++ = htons(code);
	    }
	}
	else
	{
	    ccp utf8_end = utf8 + len;
	    while ( utf8 < utf8_end && ptr < end )
		*ptr++ = htons(ScanUTF8AnsiCharE(&utf8,utf8_end));
	}
	AssignItemText16BMG(bi,buf,ptr-buf);
    }
}

///////////////////////////////////////////////////////////////////////////////

bool IsItemEqualBMG
(
    const bmg_item_t	* bi1,		// NULL or valid item
    const bmg_item_t	* bi2		// NULL or valid item
)
{
    if ( bi1 == bi2 )
	return true;
    if ( !bi1 || !bi2 )
	return false;

    return bi1->len		== bi2->len
	&& bi1->attrib_used	== bi2->attrib_used
	&& !memcmp(bi1->text,bi2->text,bi1->len*sizeof(*bi2->text))
	&& !memcmp(bi1->attrib,bi2->attrib,bi1->attrib_used);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static uint find_item_helper ( const bmg_t * bmg, u32 mid, bool * found )
{
    DASSERT(bmg);
    DASSERT(found);

    int beg = 0;
    int end = bmg->item_used - 1;
    while ( beg <= end )
    {
	const uint idx = (beg+end)/2;
	const bmg_item_t * item = bmg->item + idx;
	if ( mid < item->mid )
	    end = idx - 1 ;
	else if ( mid > item->mid )
	    beg = idx + 1;
	else
	{
	    *found = true;
	    return idx;
	}
    }

    *found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

bmg_item_t * FindItemBMG ( const bmg_t * bmg, u32 mid )
{
    DASSERT(bmg);

    bool found;
    uint idx = find_item_helper(bmg,mid,&found);
    DASSERT( idx < bmg->item_used || idx == bmg->item_used && !found );

    return found ? bmg->item + idx : 0;
}

///////////////////////////////////////////////////////////////////////////////

bmg_item_t * FindAnyItemBMG ( const bmg_t * bmg, u32 mid )
{
    DASSERT(bmg);

    bool found;
    uint idx = find_item_helper(bmg,mid,&found);
    DASSERT( idx < bmg->item_used || idx == bmg->item_used && !found );

    bmg_item_t * item = bmg->item + idx;
    if (!found)
    {
	bmg_item_t * item_end = bmg->item + bmg->item_used;
	while ( item < item_end && item->mid < mid )
	    item++;
    }
    return item;
}

///////////////////////////////////////////////////////////////////////////////

bmg_item_t * InsertItemBMG
	( bmg_t *bmg, u32 mid, u8 *attrib, uint attrib_used, bool *old_item )
{
    DASSERT(bmg);
    DASSERT( attrib_used <= BMG_ATTRIB_SIZE );

    if (opt_bmg_force_attrib)
    {
	attrib_used = bmg->attrib_used;
	attrib = bmg_force_attrib;
    }
    else if ( attrib_used > bmg->attrib_used )
	attrib_used = bmg->attrib_used;

    bmg->param_defined = true; // don't accept modifications on patch

    //if ( attrib && attrib_used ) HexDump16(stderr,0,0x100,attrib,attrib_used);

    bool found;
    uint idx = find_item_helper(bmg,mid,&found);
    DASSERT( idx < bmg->item_used || idx == bmg->item_used && !found );

    if (old_item)
	*old_item = found;

    if (found)
    {
	bmg_item_t * item = bmg->item + idx;
	if ( attrib && attrib_used )
	{
	    item->attrib_used = bmg->attrib_used;
	    memcpy(item->attrib,bmg->attrib,sizeof(item->attrib));
	    memcpy(item->attrib,attrib,attrib_used);
	}
	return item;
    }

    DASSERT( bmg->item_used <= bmg->item_size );
    if ( bmg->item_used == bmg->item_size )
    {
	bmg->item_size += 1000;
	bmg->item = REALLOC(bmg->item,bmg->item_size*sizeof(*bmg->item));
    }

    bmg_item_t * item = bmg->item + idx;
    memmove( item+1, item, (bmg->item_used-idx) * sizeof(*bmg->item) );
    bmg->item_used++;
    DASSERT( idx < bmg->item_used );
    memset(item,0,sizeof(*item));
    item->mid = mid;
    item->slot = BMG_NO_PREDEF_SLOT;

    memcpy(item->attrib,bmg->attrib,sizeof(item->attrib));
    if ( attrib && attrib_used )
    {
	item->attrib_used = bmg->attrib_used;
	memcpy(item->attrib,attrib,attrib_used);
    }
    return item;
}

///////////////////////////////////////////////////////////////////////////////

bool DeleteItemBMG ( bmg_t *bmg, u32 mid )
{
    DASSERT(bmg);

    bmg_item_t *dptr = FindItemBMG(bmg,mid);
    if (dptr)
    {
	FreeItemBMG(dptr);
	dptr->text = 0;
	ResetAttribBMG(bmg,dptr);
	return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool HavePredifinedSlots ( bmg_t *bmg )
{
    DASSERT(bmg);

    bmg_item_t *bi, *bi_end = bmg->item + bmg->item_used;
    for ( bi = bmg->item; bi < bi_end; bi++ )
	if ( bi->text && bi->slot != BMG_NO_PREDEF_SLOT )
	    return bmg->have_predef_slots = true;
    return bmg->have_predef_slots = false;
}

///////////////////////////////////////////////////////////////////////////////

u16 * ModifiyItem
(
    // return a fixed 'ptr'
    bmg_item_t		* bi,		// valid item, prepare text modification
    const u16		* ptr		// NULL or a pointer within 'bi->text'
)
{
    DASSERT(bi);

    if (!bi->alloced_size)
    {
	const uint size = (bi->len+1) * sizeof(*bi->text);
	u16 *text = MALLOC(size);
	memcpy(text,bi->text,size);
	if (ptr)
	    ptr = text + ( ptr - bi->text );
	bi->text = text;
	bi->alloced_size = bi->len;
    }
    return (u16*)ptr;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    MKW hooks			///////////////
///////////////////////////////////////////////////////////////////////////////

static int GetTrackIndexBMG0 ( uint idx, int result_on_invalid )
{
    static const u8 track_pos[BMG_N_TRACK] =
    {
	 8, 1, 2, 4,  0, 5, 6, 7,  9,15,11, 3, 14,10,12,13,
	16,20,25,26, 27,31,23,18, 21,30,29,17, 24,22,19,28
    };

    return idx < BMG_N_TRACK
		? track_pos[idx]
		: idx < BMG_N_CT_TRACK
		? idx
		: result_on_invalid;
}

int (*GetTrackIndexBMG) ( uint idx, int result_on_invalid ) = GetTrackIndexBMG0;

///////////////////////////////////////////////////////////////////////////////

static int GetArenaIndexBMG0 ( uint idx, int result_on_invalid )
{
    static const u8 arena_pos[BMG_N_ARENA] = { 1,0,3,2,4, 7,8,9,5,6 };

    return idx < BMG_N_ARENA
		? arena_pos[idx]
		: result_on_invalid;
}

int (*GetArenaIndexBMG) ( uint idx, int result_on_invalid ) = GetArenaIndexBMG0;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			bmg endian support		///////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t TabEndianBMG[] =
{
	{ BMG_AUTO_ENDIAN,	"AUTO",		"UNDEFINED",	0 },
	{ BMG_BIG_ENDIAN,	"BIG",		"BE",		0 },
	{ BMG_LITTLE_ENDIAN,	"LITTLE",	"LE",		0 },
	{ 0,0,0,0 }
};

//-----------------------------------------------------------------------------

ccp GetEndianNameBMG ( int endian, ccp return_if_invalid )
{
    switch (endian)
    {
	case BMG_AUTO_ENDIAN:	return "AUTO";
	case BMG_BIG_ENDIAN:	return "BIG";
	case BMG_LITTLE_ENDIAN:	return "LITTLE";
    }
    return return_if_invalid;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			bmg encoding support		///////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t TabEncodingBMG[] =
{
					// if opt==1: not implemented yet
	{ BMG_ENC_CP1252,	"CP-1252",	"CP1252",	0 },
	{ BMG_ENC_CP1252,	"1",		0,		0 },

	{ BMG_ENC_UTF16BE,	"UTF-16BE",	"UTF16BE",	0 },
	{ BMG_ENC_UTF16BE,	"2",		0,		0 },

	{ BMG_ENC_SHIFT_JIS,	"SHIFT-JIS",	"SHIFTJIS",	0 },
	{ BMG_ENC_SHIFT_JIS,	"3",		"SJIS",		0 },

	{ BMG_ENC_UTF8,		"UTF-8",	"UTF8",		0 },
	{ BMG_ENC_UTF8,		"4",		0,		0 },

	{ BMG_ENC__UNDEFINED,	"AUTO",		"UNDEFINED",	0 },
	{ 0,0,0,0 }
};

//-----------------------------------------------------------------------------

ccp GetEncodingNameBMG ( int encoding, ccp return_if_invalid )
{
    switch (encoding)
    {
	case BMG_ENC_CP1252:	return "CP1252";
	case BMG_ENC_UTF16BE:	return "UTF-16/be";
	case BMG_ENC_SHIFT_JIS:	return "Shift-JIS";
	case BMG_ENC_UTF8:	return "UTF-8";
    }
    return return_if_invalid;
}

//-----------------------------------------------------------------------------

int CheckEncodingBMG ( int encoding, int return_if_invalid )
{
    return encoding >= BMG_ENC__MIN && encoding <= BMG_ENC__MAX
		? encoding : return_if_invalid;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup/reset bmg_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void AssignEncodingBMG ( bmg_t * bmg, int encoding )
{
    if ( encoding >= BMG_ENC__MIN && encoding <= BMG_ENC__MAX )
	bmg->encoding = encoding;
}

///////////////////////////////////////////////////////////////////////////////

void AssignInfSizeBMG ( bmg_t * bmg, uint inf_size )
{
    bmg->param_defined = true;

    if ( inf_size < 4 || inf_size > BMG_INF_LIMIT )
    {
	ERROR0(ERR_WARNING,
		"INF data size is outside allowed range 4..%u => ignored: %s\n",
		BMG_INF_LIMIT, bmg->fname );
	return;
    }

    if ( inf_size > BMG_INF_MAX_SIZE )
	ERROR0(ERR_WARNING,
		"INF data size is %u bytes, but only %u bytes are supported."
		" The additional bytes are always assumed to be NULL: %s",
		inf_size, BMG_INF_MAX_SIZE, bmg->fname );

    bmg->inf_size = opt_bmg_inf_size ? opt_bmg_inf_size : inf_size;
    bmg->attrib_used = bmg->inf_size - 4;
    if ( bmg->attrib_used > sizeof(bmg->attrib) )
	bmg->attrib_used = sizeof(bmg->attrib);
    if (opt_bmg_def_attrib)
	memcpy(bmg->attrib,bmg_def_attrib,bmg->attrib_used);

    if ( inf_size != 8 )
    {
	// not MKW => change defaults
	if ( bmg->use_color_names == 2 )
	    bmg->use_color_names = 1;
	if ( bmg->use_mkw_messages == 2 )
	    bmg->use_mkw_messages = 1;
    }
}

///////////////////////////////////////////////////////////////////////////////

void InitializeBMG ( bmg_t * bmg )
{
    ASSERT( MID_VERSUS_POINTS >= MID_PARAM_BEG
	 && MID_VERSUS_POINTS <  MID_PARAM_END );

    DASSERT(bmg);
    memset(bmg,0,sizeof(*bmg));

    bmg->endian			= &be_func;
    bmg->have_mid		= 1;
    bmg->encoding		= BMG_ENC__DEFAULT;
    bmg->use_slots		= opt_bmg_use_slots;
    bmg->use_raw_sections	= opt_bmg_use_raw_sections;

    bmg->unknown_inf_0c		= BMG_INF_DEFAULT_0C;
    bmg->unknown_mid_0a		= BMG_MID_DEFAULT_0A;
    bmg->unknown_mid_0c		= BMG_MID_DEFAULT_0C;

    //-- setup for MKW

    if (opt_bmg_support_mkw)
    {
	bmg->use_color_names	= 1;
	bmg->use_mkw_messages	= 1;
	AssignInfSizeBMG(bmg,BMG_INF_STD_SIZE);

	if ( !opt_bmg_def_attrib && bmg->attrib_used == 4 )
	    write_be32(bmg->attrib,BMG_INF_STD_ATTRIB);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ResetBMG ( bmg_t * bmg )
{
    DASSERT(bmg);

    bmg_item_t * bi  = bmg->item;
    bmg_item_t * bi_end = bi + bmg->item_used;
    while ( bi < bi_end )
	FreeItemBMG(bi++);

    FREE(bmg->item);

    if (bmg->data_alloced)
	FREE(bmg->data);
    FREE(bmg->raw_data);
    FreeString(bmg->fname);

    bmg_raw_section_t *raw = bmg->first_raw;
    while (raw)
    {
	ResetFastBuf(&raw->data);
	bmg_raw_section_t *del = raw;
	raw = raw->next;
	FREE(del);
    }

    memset(bmg,0,sizeof(*bmg));
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanRawBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

static int CompareU32 ( const u32 *a, const u32 *b )
{
    return *a < *b ? -1 : *a > *b;
}

//-----------------------------------------------------------------------------

static u32 FindMajority32 ( cvp first, uint n_elem, uint delta )
{
    DASSERT( first || !n_elem );

    if ( n_elem < 3 )
	return n_elem ? *(u32*)first : 0;

    u32 *buf = MALLOC(n_elem*sizeof(*buf));

    uint i;
    for ( i = 0; i < n_elem; i++ )
    {
	buf[i] = *(u32*)first;
	first = (u32*)( (u8*)first + delta );
    }
    //HexDump16(stderr,0,0,buf,64);
    qsort(buf,n_elem,sizeof(*buf),(qsort_func)CompareU32);

    uint max_val = 0, max_count = 0;
    const u32 *ptr = buf;
    const u32 *end = ptr + n_elem;
    while ( ptr < end )
    {
	const u32 *beg = ptr;
	const u32 val = *ptr++;
	while ( ptr < end && *ptr == val )
	    ptr++;
	if ( ptr - beg > max_count )
	{
	    max_count = ptr - beg;
	    max_val = val;
	}
    }

    FREE(buf);
    return max_val;
}

///////////////////////////////////////////////////////////////////////////////

static void scan_raw_cp1252
	( bmg_item_t *bi, const u8 *start, const u8 *end, FastBuf_t *fb )
{
    u8 buf[4];
    const u8 *ptr;
    for ( ptr = start; ptr < end && *ptr; )
    {
	u16 code = *ptr++;
	if ( code >= 0x80 && code < 0xa0 )
	    code = TableCP1252_80[code-0x80];
	if ( code == 0x1a )
	{
	    const uint len = *ptr++;
	    if ( len > 2 )
	    {
		buf[0] = 0;
		buf[1] = 0x1a;
		buf[2] = len+1;
		AppendFastBuf(fb,buf,3);
		AppendFastBuf(fb,ptr,len-2);
		if (!(len&1))
		    AppendFastBuf(fb,buf,1);
		ptr += len - 2;
	    }
	}
	else
	{
	    code = htons(code);
	    AppendFastBuf(fb,&code,2);
	}
    }

    const mem_t mem = GetFastBufMem(fb);
    AssignItemText16BMG(bi,(u16*)mem.ptr,mem.len/2);
}

//-----------------------------------------------------------------------------

static void scan_raw_utf16 ( bmg_item_t *bi, const u8 *start, const u8 *end )
{
    const u8 *ptr;
    for ( ptr = start; ptr < end; ptr += 2 )
    {
	if (!ptr[0])
	{
	    if (!ptr[1])
		break;
	    if ( ptr[1] == 0x1a )
	    {
		ptr += 2;
		const uint skip = *ptr + 1 & 0x1e;
		if ( skip > 4 )
		    ptr += skip - 4;
	    }
	}
    }

    AssignItemText16BMG( bi, (u16*)start, ( ptr - start )/2 );
}

//-----------------------------------------------------------------------------

static void scan_raw_shift_jis
	( bmg_item_t *bi, const u8 *start, const u8 *end, FastBuf_t *fb )
{
    u8 buf[4];
    const u8 *ptr;
    for ( ptr = start; ptr < end; )
    {
	int code = ScanShiftJISChar(&ptr);
	if (!code)
	    break;
	if ( code == 0x1a )
	{
	    const uint len = *ptr++;
	    if ( len > 2 )
	    {
		buf[0] = 0;
		buf[1] = 0x1a;
		buf[2] = len+1;
		AppendFastBuf(fb,buf,3);
		AppendFastBuf(fb,ptr,len-2);
		if (!(len&1))
		    AppendFastBuf(fb,buf,1);
		ptr += len - 2;
	    }
	}
	else if ( code >= 0 )
	{
	    code = htons(code);
	    AppendFastBuf(fb,&code,2);
	}
    }

    const mem_t mem = GetFastBufMem(fb);
    AssignItemText16BMG(bi,(u16*)mem.ptr,mem.len/2);
}

//-----------------------------------------------------------------------------

static void scan_raw_utf8
	( bmg_item_t *bi, const u8 *start, const u8 *end, FastBuf_t *fb )
{
    u8 buf[4];
    const u8 *ptr;
    for ( ptr = start; ptr < end && *ptr; )
    {
	u32 code = ScanUTF8AnsiCharE((ccp*)&ptr,(ccp)end);
	if ( code >= 0x80 && code < 0xa0 )
	    code = TableCP1252_80[code -0x80];
	if ( code == 0x1a )
	{
	    const uint len = *ptr++;
	    if ( len > 2 )
	    {
		buf[0] = 0;
		buf[1] = 0x1a;
		buf[2] = len+1;
		AppendFastBuf(fb,buf,3);
		AppendFastBuf(fb,ptr,len-2);
		if (!(len&1))
		    AppendFastBuf(fb,buf,1);
		ptr += len - 2;
	    }
	}
	else
	{
	    code = htons(code);
	    AppendFastBuf(fb,&code,2);
	}
    }

    const mem_t mem = GetFastBufMem(fb);
    AssignItemText16BMG(bi,(u16*)mem.ptr,mem.len/2);
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanRawBMG ( bmg_t * bmg )
{
    DASSERT(bmg);
    DASSERT(bmg->data);
    DASSERT(bmg->endian);

    const bmg_header_t * bh = (bmg_header_t*)bmg->data;
    DASSERT(!memcmp(bh->magic,BMG_MAGIC,sizeof(bh->magic)));

    u32 data_size = bmg->endian->n2hl(bh->size);
    bmg->encoding = bh->encoding;
    if ( !bmg->encoding && data_size*BMG_LEGACY_BLOCK_SIZE <= bmg->data_size )
    {
	bmg->legacy = true;
	bmg->encoding = BMG_ENC_CP1252;
	data_size *= BMG_LEGACY_BLOCK_SIZE;
    }

    if ( data_size > bmg->data_size )
    {
	ERROR0(ERR_WARNING,
		"BMG is %u bytes shorter than announced: %s",
		data_size - bmg->data_size, bmg->fname );
	data_size = bmg->data_size;
    }

    const bmg_sect_list_t *sl = ScanSectionsBMG(bmg->data,data_size,bmg->endian);
    if ( !sl || !sl->header )
	return ERROR0(ERR_INVALID_DATA,"Invalid BMG data: %s",bmg->fname);


    //--- check encoding

    switch (bmg->encoding)
    {
     case BMG_ENC_CP1252:
     case BMG_ENC_UTF16BE:
     case BMG_ENC_SHIFT_JIS:
     case BMG_ENC_UTF8:
	//>>> ok! <<<
	break;

     default:
	if (!opt_bmg_force_count)
	    return ERROR0(ERR_ERROR,
			"Can't read BMG with encoding #%u (%s): %s\n",
			bmg->encoding, GetEncodingNameBMG(bmg->encoding,"?"),
			bmg->fname );
	ERROR0(ERR_WARNING,
			"Try to read BMG with unsupported encoding #%u (%s): %s\n",
			bmg->encoding, GetEncodingNameBMG(bmg->encoding,"?"),
			bmg->fname );
    }


    //--- iterate sections

    const bmg_sect_info_t *si;
    for ( si = sl->info; si->sect; si++ )
    {
	if ( si->real_size < si->sect_size )
	    ERROR0(ERR_WARNING,
			"Size of BMG section %s set to %u bytes (%u lesser than announced): %s\n",
			PrintID(si->sect->magic,4,0),
			si->real_size, si->sect_size - si->real_size, bmg->fname );

	if ( !si->supported )
	{
	    bmg_raw_section_t *raw = CreateRawSectionBMG(bmg,si->sect->magic);
	    DASSERT(raw);
	    AssignFastBuf( &raw->data, si->sect->data,
				si->real_size - sizeof(bmg_section_t) );
	    raw->total_size = si->real_size;
	    PRINT("SCAN/RAW SECTION »%s« data: %u = 0x%x, total: %u = 0x%x\n",
			PrintID(raw->magic,4,0),
			GetFastBufLen(&raw->data), GetFastBufLen(&raw->data),
			raw->total_size, raw->total_size );
	}
    }

    PRINT("%p %p %p\n",sl->pinf,sl->pdat,sl->pmid);
    const bmg_inf_t *pinf = bmg->inf = sl->pinf;
    const bmg_dat_t *pdat = bmg->dat = sl->pdat;
    const bmg_mid_t *pmid = bmg->mid = sl->pmid;

    if ( !sl->pinf || !sl->pdat )
	return ERROR0(ERR_INVALID_DATA,"Corrupted BMG file: %s\n",bmg->fname);

    bmg->unknown_inf_0c = bmg->endian->n2hl(pinf->unknown_0c);

    uint max_item;
    bmg->have_mid = pmid != 0;
    if (bmg->have_mid)
    {
	bmg->unknown_mid_0a = bmg->endian->n2hs(pmid->unknown_0a);
	bmg->unknown_mid_0c = bmg->endian->n2hl(pmid->unknown_0c);

	max_item = ( bmg->endian->n2hl(pmid->size) - sizeof(*pmid) ) / sizeof(*pmid->mid);
	if ( max_item > bmg->endian->n2hs(pmid->n_msg) )
	     max_item = bmg->endian->n2hs(pmid->n_msg);
	PRINT("BMG with MID1: N=%u [inf=%u,mid=%u], %s\n",
		max_item, bmg->endian->n2hs(pinf->n_msg), bmg->endian->n2hs(pmid->n_msg), bmg->fname );
    }
    else
    {
	max_item = bmg->endian->n2hs(pinf->n_msg);
	PRINT("BMG without MID1: N=%u, %s\n",max_item,bmg->fname);
    }

    const uint max_offset = ( bmg->endian->n2hl(pdat->size) - sizeof(*pdat) )
			  / sizeof(*pdat->text_pool);
    const u8 *text_end = pdat->text_pool + max_offset;

    const u8 *raw_inf = (u8*)pinf->list;
    const uint src_inf_size = bmg->endian->n2hs(pinf->inf_size);
    AssignInfSizeBMG(bmg,src_inf_size);


    //--- find default attributes

    if (!opt_bmg_def_attrib)
    {
	uint idx;
	for ( idx = 0; idx < bmg->attrib_used; idx += 4 )
	{
	    const u32 major = FindMajority32(raw_inf+idx+4,max_item,src_inf_size);
	    noPRINT("-> IDX: %2u -> %8x\n",idx,bmg->endian->n2hl(major));
	    memcpy(bmg->attrib+idx,&major,4);
	}
	//HexDump16(stderr,8,0x10,bmg->attrib,bmg->attrib_used);
    }


    //--- decode raw files

    struct { FastBuf_t b; char space[500]; } fb_mgr;
    InitializeFastBuf(&fb_mgr,sizeof(fb_mgr));
 #if HAVE_PRINT
    uint ins_count = 0, empty_count = 0;
 #endif

    uint prev_mid = 0;
    uint slot, unsort_count = 0, ffff_count = 0;
    for ( slot = 0; slot < max_item; slot++, raw_inf += src_inf_size )
    {
	bmg_inf_item_t *item = (bmg_inf_item_t*)raw_inf;
	const u32 offset = bmg->endian->n2hl(item->offset);

	u32 mid = pmid ? bmg->endian->n2hl(pmid->mid[slot]) : slot;
	if ( mid == 0xffff && !offset )
	    ffff_count++;
	if ( mid < prev_mid )
	    unsort_count++;
	prev_mid = mid;

 #ifdef TEST
	if ( offset >= max_offset )
	    return ERROR0(ERR_INVALID_DATA,
		"Invalid pointer at file offset 0x%zx (INF item #%u): %s\n",
			(u8*)&pinf->list[slot].offset - bmg->data, slot, bmg->fname);
 #else
	if ( offset >= max_offset )
	    return ERROR0(ERR_INVALID_DATA,"Corrupted BMG file: %s\n",bmg->fname);
 #endif

 #if HAVE_PRINT0
	{
	    bmg_item_t *bi = FindItemBMG(bmg,mid);
	    if (bi)
		PRINT("MID EXISTS: %x, slot:%x\n",mid,slot);
	}
 #endif

	bmg_item_t *bi = InsertItemBMG(bmg,mid,item->attrib,bmg->attrib_used,0);
	DASSERT(bi);
	bi->slot = slot < BMG_NO_PREDEF_SLOT ? slot : BMG_NO_PREDEF_SLOT;

	if (!offset)
	{
	 #if HAVE_PRINT
	    empty_count++;
	 #endif
	    AssignItemText16BMG(bi,bmg_null_entry,0);
	    continue;
	}

	switch (bmg->encoding)
	{
	 case BMG_ENC_CP1252:
	    ClearFastBuf(&fb_mgr.b);
	    scan_raw_cp1252( bi, pdat->text_pool + offset, text_end, &fb_mgr.b );
	    break;

	 case BMG_ENC_UTF16BE:
	    scan_raw_utf16( bi, pdat->text_pool + offset, text_end );
	    break;

	 case BMG_ENC_SHIFT_JIS:
	    ClearFastBuf(&fb_mgr.b);
	    scan_raw_shift_jis( bi, pdat->text_pool + offset, text_end, &fb_mgr.b );
	    break;

	 case BMG_ENC_UTF8:
	    ClearFastBuf(&fb_mgr.b);
	    scan_raw_utf8( bi, pdat->text_pool + offset, text_end, &fb_mgr.b );
	    break;

	 default:
	    // fallback for cast opt_bmg_force_count>0
	    DASSERT(opt_bmg_force_count);
	    ClearFastBuf(&fb_mgr.b);
	    scan_raw_cp1252( bi, pdat->text_pool + offset, text_end, &fb_mgr.b );
	    break;
	}

     #if HAVE_PRINT
	ins_count++;
     #endif
    }

 #if HAVE_PRINT
    PRINT("BMG: %u+%u=%u/%u messages inserted => %u | n(unsort)=%d, n(ffff)=%d\n",
		empty_count, ins_count, empty_count+ins_count,
		max_item, bmg->item_used,
		unsort_count, ffff_count );
 #endif
    ResetFastBuf(&fb_mgr.b);

    bmg->have_predef_slots = pmid && max_item > 1
				&& ( unsort_count > 1 || ffff_count > 1 );

    if (bmg->have_predef_slots)
    {
	bmg_item_t *bi = FindItemBMG(bmg,0xffff);
	if ( bi && isSpecialEntryBMG(bi->text) )
	    AssignItemText16BMG(bi,0,0);
    }
    else
    {
	// clear slots settings
	bmg_item_t *bi, *bi_end = bmg->item + bmg->item_used;
	for ( bi = bmg->item; bi < bi_end; bi++ )
	    bi->slot = BMG_NO_PREDEF_SLOT;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanTextBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

int (*AtHookBMG) ( bmg_t * bmg, ccp line, ccp line_end ) = 0;

///////////////////////////////////////////////////////////////////////////////

static char * GetNext ( ccp ptr, ccp end )
{
    DASSERT(ptr);
    DASSERT(end);

    while ( ptr < end && ( *ptr == ' ' || *ptr == '\t' || *ptr == '\r' ) )
	ptr++;
    return (char*)ptr;
}

///////////////////////////////////////////////////////////////////////////////
// [[xbuf]] GetText ( FastBuf_t *fb ccp ptr, ccp end, uint *len )

static char * GetText ( char *buf, uint bufsize, ccp ptr, ccp end, uint *len )
{
    DASSERT(buf);
    DASSERT(bufsize>10);

    char *dest = buf;
    char *dest_end = buf + bufsize - 1;

    ptr = GetNext(ptr,end);

    if ( ptr < end && ( *ptr == '\'' || *ptr == '\"' ) )
    {
	const char quote = *ptr++;
	while ( ptr < end && *ptr != quote )
	{
	    if ( dest < dest_end )
		*dest++ = *ptr;
	    ptr++;
	}
	if ( *ptr == quote )
	    ptr++;
    }
    else
    {
	while ( ptr < end && (uchar)*ptr > ' ' )
	{
	    if ( dest < dest_end )
		*dest++ = *ptr;
	    ptr++;
	}
    }

    *dest = 0;
    if (len)
	*len = dest - buf;
    return GetNext(ptr,end);
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanTextBMG ( bmg_t * bmg )
{
    DASSERT(bmg);
    TRACE("SCAN BMG/TEXT: %s\n",bmg->fname);
    bmg->is_text_src = true;

    ccp ptr = (ccp)bmg->data;
    ccp end = ptr + bmg->data_size;
    ptr += GetTextBOMLen(ptr,bmg->data_size);

    ccp bmg_name = 0;
// [[xbuf]]
    char xbuf[2*BMG_MSG_BUF_SIZE];
    enumError err_stat = ERR_OK;
    u32 cond = 0;

    ParamField_t assign;
    InitializeParamField(&assign);
    assign.free_data = true;

    for(;;)
    {
	while ( ptr < end && (uchar)*ptr <= ' ' )
	    ptr++;
	if ( ptr >= end )
	    break;

	ccp start = ptr = GetNext(ptr,end);
	while ( ptr < end && *ptr != '\r' && *ptr != '\n' )
	    ptr++;

	if ( *start == '@' )
	{
	    if (AtHookBMG)
	    {
		const int stat = AtHookBMG(bmg,start,ptr);
		if ( stat < -1 )
		    continue;
		if ( stat >= 0 )
		{
		    err_stat = stat;
		    break;
		}
	    }

	    start = GetNext(start+1,ptr);
	    const bool silent = start < ptr && *start == '!';
	    if (silent)
		start = GetNext(start+1,ptr);

	    if ( *start == '?' )
	    {
		// SYNTAX/IF: '@?' MID_COND

		u32 mid;
		ScanMidBMG(bmg,&mid,0,0,start+1,end,(char**)&start);
		if ( ptr >= end )
		    break;
		cond = mid;
		continue;
	    }

	    if ( *start == '=' )
	    {
		// SYNTAX/MATCH: '@=' PATTERN [':'] LINE

		uint len;
// [[xbuf]]
		start = GetText(xbuf,sizeof(xbuf),start+1,ptr,&len);
		if (len)
		{
// [[xbuf]]
		    PRINT("MATCH |%s|\n",xbuf);
		    if (!bmg_name)
		    {
			bmg_name = GetNameBMG(bmg,0);
			PRINT("NAME |%s|\n",bmg_name);
		    }

// [[xbuf?]]
		    if (MatchPattern(xbuf,bmg_name,'/'))
		    {
			ptr = GetNext(start,ptr);
			if ( *ptr == ':' )
			    ptr++;
		    }
		}
		continue;
	    }

	    if ( *start == '$' )
	    {
		// SYNTAX/ECHO: '@$' TEXT_TO_EOL

		if (opt_bmg_allow_print)
		{
		    start++;
		    printf("#%.*s\n",(int)(ptr-start),start);
		}
		continue;
	    }

	    const bool close_current = *start == '>';
	    if ( close_current || *start == '<' )
	    {
		// SYNTAX/GOTO:    '@' ['!'] '>' FILE
		// SYNTAX/INCLUDE: '@' ['!'] '<' FILE

		uint len;
// [[xbuf]]
		start = GetText(xbuf,sizeof(xbuf),start+1,ptr,&len);
		if ( len && bmg->recurse_depth < opt_bmg_max_recurse )
		{
		    PRINT("BMG/%s: %s\n",close_current?"GOTO":"INCLUDE",xbuf);
		    bmg_t bmg2;
// [[xbuf?]]
		    const enumError err
			= LoadBMG(&bmg2,true,bmg->fname,xbuf,silent?FM_SILENT:0);
		    if ( err == ERR_OK )
			PatchOverwriteBMG(bmg,&bmg2,true);
		    ResetBMG(&bmg2);
		    if ( close_current && err != ERR_CANT_OPEN )
			break;
		}
		continue;
	    }

	    char namebuf[100], *nptr = namebuf;
	    while ( start < ptr && *start > ' ' && *start < 'z' && *start != '=' )
	    {
		if ( nptr < namebuf + sizeof(namebuf) - 1 )
		    *nptr++ = toupper((int)*start);
		start++;
	    }
	    *nptr = 0;
	    start = GetNext(start,ptr);


	    //--- settings without mandatory '='

	    const bool have_equal = start < ptr && *start == '=';
	    if (have_equal)
		start = GetNext(start+1,ptr);

	    if (!strcmp(namebuf,"SECTION"))
	    {
		bmg->current_raw = 0;
		uint len = ScanEscapedString(namebuf,sizeof(namebuf),
				start, ptr-start, false, -1, 0 );
		if (!len)
		    continue;
		if ( len > 4 )
		{
		    ERROR0(ERR_WARNING,
			"@SECTION name exceeds 4 [%u] characters: %s\n",
			len, namebuf );
		    continue;
		}
		while ( len < 4 )
		    namebuf[len++] = 0;
		PRINT("@SECTION »%s« %02x %02x %02x %02x\n",
			PrintID(namebuf,4,0),
			namebuf[0], namebuf[1], namebuf[2], namebuf[3] );
		bmg->current_raw = CreateRawSectionBMG(bmg,namebuf);
		DASSERT(bmg->current_raw);
		ClearFastBuf(&bmg->current_raw->data);
		continue;
	    }

	    if (!strcmp(namebuf,"X"))
	    {
		if (!bmg->current_raw)
		    continue;

		// skip addr

		ccp p = start;
		while (isalnum((int)*p))
		    p++;
		if ( p > start && *p == ':' )
		    start = p+1;

		for (;;)
		{
		    start = GetNext(start,ptr);
		    if ( start >= ptr || *start == ':' )
			break;
		    char *end;
		    uint num = str2ul(start,&end,16);
		    if ( end == start )
			break;
		    start = end;
		    AppendCharFastBuf(&bmg->current_raw->data,num);
		}
		continue;
	    }


	    //--- settings with mandatory '='

	    if ( !have_equal || start >= ptr )
		continue;

	    noPRINT("PARAM: %s = |%.*s|\n",namebuf,(int)(ptr-start),start);
	    if (!strcmp(namebuf,"ENDIAN"))
		bmg->endian = str2l(start,0,10) > 0 ? &le_func : &be_func;
	    else if (!strcmp(namebuf,"LEGACY"))
		bmg->legacy = str2l(start,0,10) > 0;
	    else if (!strcmp(namebuf,"ENCODING"))
		AssignEncodingBMG(bmg,str2l(start,0,10));
	    else if (!strcmp(namebuf,"BMG-MID"))
		bmg->have_mid = str2ul(start,0,10) > 0;
	    else if (!strcmp(namebuf,"INF-SIZE"))
		AssignInfSizeBMG(bmg,str2ul(start,0,10));
	    else if (!strcmp(namebuf,"DEFAULT-ATTRIBS"))
	    {
		noPRINT("--- SCAN ATTRIB: %.*s\n",(int)(ptr-start),start);
		scan_attrib(bmg->attrib,bmg->attrib_used,start);
		//HEXDUMP16(0,0,bmg->attrib,bmg->attrib_used);
	    }
	    else if (!strcmp(namebuf,"UNKNOWN-INF32-0C"))
		bmg->unknown_inf_0c = str2ul(start,0,10);
	    else if (!strcmp(namebuf,"UNKNOWN-MID16-0A"))
		bmg->unknown_mid_0a = str2ul(start,0,10);
	    else if (!strcmp(namebuf,"UNKNOWN-MID32-0C"))
		bmg->unknown_mid_0c = str2ul(start,0,10);
	    continue;
	}


	//--- scan message ids

	u32 mid1 = 0, mid2 = 0, mid3 = 0;
	const int mid_stat
	    = ScanMidBMG(bmg,&mid1,&mid2,&mid3,start,end,(char**)&start);

	if ( mid_stat <= 0 ) // not a valid message line
	    continue;

	//--- scan predifined slot

	bmg_slot_t slot = BMG_NO_PREDEF_SLOT;
	if ( *start == '@' )
	{
	    char *end;
	    uint num = str2l(++start,&end,16);
	    PRINT0("SLOT[%#x] = %#x\n",mid1,num);
	    if ( end > start && num < BMG_NO_PREDEF_SLOT )
		slot = num;
	    start = GetNext(end,ptr);
	}


	//--- scan attributes

	u8 attrib_buf[BMG_ATTRIB_SIZE];
	memcpy(attrib_buf,bmg->attrib,sizeof(attrib_buf));
	uint attrib_used = 0;

	if ( *start == '[' )
	{
	    ccp aptr = scan_attrib(attrib_buf,bmg->attrib_used,start+1);
	    aptr = GetNext(aptr,ptr);
	    if ( *aptr == ']' )
	    {
		attrib_used = bmg->attrib_used;
		start = GetNext(aptr+1,ptr);
	    }
	}

	if ( *start == '/' || *start == '~' )
	{
	    if ( *start == '~' )
	    {
		attrib_used = bmg->attrib_used;
		start = GetNext(start+1,ptr);

		if ( *start == '[' )
		    scan_attrib(attrib_buf,attrib_used,start);
		else
		{
		    memset(attrib_buf,0,sizeof(attrib_buf));
		    const u32 attr = str2ul(start,0,10);
		    write_be32(attrib_buf,attr);
		}
	    }

	    bmg_item_t * bi = InsertItemBMG(bmg,mid1,attrib_buf,attrib_used,0);
	    DASSERT(bi);
	    bi->slot = slot;
	    AssignItemText16BMG(bi,bmg_null_entry,0);
	    if (mid2)
	    {
		bi = InsertItemBMG(bmg,mid2,attrib_buf,attrib_used,0);
		DASSERT(bi);
		bi->slot = slot;
		AssignItemText16BMG(bi,bmg_null_entry,0);
	    }
	    if (mid3)
	    {
		bi = InsertItemBMG(bmg,mid3,attrib_buf,attrib_used,0);
		DASSERT(bi);
		bi->slot = slot;
		AssignItemText16BMG(bi,bmg_null_entry,0);
	    }
	    continue;
	}

	if ( *start == ':' )
	{
	    start = GetNext(start+1,end);
	    ccp ptr = start;
	    while ( ptr < end && isalnum((int)*ptr) )
		ptr++;

	    if ( ptr > start )
	    {
		u32 *tab[] = { &mid1, &mid2, &mid3, 0 }, **tptr;
		for ( tptr = tab; *tptr; tptr++ )
		{
		    const u32 mid = **tptr;
		    if (mid)
		    {
			DeleteItemBMG(bmg,mid);

			char midbuf[20];
			snprintf(midbuf,sizeof(midbuf),"%x",mid);
			noPRINT("BMG ASSIGN: %5s = |%.*s|\n",
				midbuf,(int)(ptr-start),start);
			InsertParamField(&assign,midbuf,false,mid,MEMDUP(start,ptr-start));
		    }
		}
	    }
	    continue;
	}

	if ( *start != '=' ) // not a valid line
	    continue;

// [[xbuf]]
	// assume that xbuf is large enough
	//   => make only simple and silent buffer overrun tests

// [[xbuf?]]
	char * in_beg = xbuf + sizeof(xbuf)/2;
	char * in_end = xbuf + sizeof(xbuf) - 0x10;
	DASSERT( in_beg < in_end );
	char * in_ptr = in_beg;

	for(;;)
	{
	    if ( ptr[-1] == '\r' )
		ptr--;
	    start++;
	    if ( *start == ' ' )
		start++;
	    if ( start > ptr )
		 start = ptr;

	    int len = ptr - start;
	    if ( len > in_end - in_ptr )
		 len = in_end - in_ptr;
	    memcpy(in_ptr,start,len);
	    in_ptr += len;

	    while ( ptr < end && (uchar)*ptr <= ' ' )
		ptr++;
	    if ( ptr == end || *ptr != '+' )
		break;

	    start = ptr;
	    while ( ptr < end && *ptr != '\n' )
		ptr++;
	}
	*in_ptr = 0;

	const int in_len = in_ptr - in_beg;
	noPRINT("%x,%x,%x |%.*s|\n",mid1,mid2,mid3,in_len,in_beg);
// [[xbuf]] ScanFastBuf16BMG();
	u16 * dest_buf = (u16*)xbuf;
	const uint len16
	    = ScanString16BMG( dest_buf, sizeof(xbuf)/sizeof(u16), in_beg, in_len, bmg );

	bmg_item_t * bi = InsertItemBMG(bmg,mid1,attrib_buf,attrib_used,0);
	DASSERT(bi);
	bi->cond = cond;
	bi->slot = slot;
	AssignItemText16BMG(bi,dest_buf,len16);
	if (mid2)
	{
	    bi = InsertItemBMG(bmg,mid2,attrib_buf,attrib_used,0);
	    DASSERT(bi);
	    bi->cond = cond;
	    bi->slot = slot;
	    AssignItemText16BMG(bi,dest_buf,len16);
	}
	if (mid3)
	{
	    bi = InsertItemBMG(bmg,mid3,attrib_buf,attrib_used,0);
	    DASSERT(bi);
	    bi->cond = cond;
	    bi->slot = slot;
	    AssignItemText16BMG(bi,dest_buf,len16);
	}
    }


    //--- manage assignments

    bool dirty = assign.used > 0;
    while (dirty)
    {
	dirty = false;
	ParamFieldItem_t *pfi = assign.field, *end;
	for ( end = pfi + assign.used; pfi < end; pfi++ )
	{
	    if (!pfi->num)
		continue;

	    bmg_item_t *bi = FindItemBMG(bmg,pfi->num);
	    noPRINT("## %5x = %5s = |%s| -> found=%d, len=%d\n",
			pfi->num, pfi->key, (ccp)pfi->data, bi!=0, bi ? bi->len : -1 );
	    pfi->key = 0;
	    if ( bi && bi->len )
	    {
		pfi->num = 0;
		continue;
	    }

	    bmg_scan_mid_t scan;
	    ScanBMGMID(&scan,bmg,(ccp)pfi->data,0);
	    if ( scan.status < 1 )
	    {
		pfi->num = 0;
		continue;
	    }

	    uint i;
	    for ( i = 0; i < scan.n_mid; i++ )
	    {
		bmg_item_t *src = FindItemBMG(bmg,scan.mid[i]);
		if ( src && src->text )
		{
		    bmg_item_t *dest
			= InsertItemBMG(bmg,pfi->num,src->attrib,src->attrib_used,0);
		    DASSERT(dest);

		    // src is invalid now => search again
		    src = FindItemBMG(bmg,scan.mid[i]);
		    if (src)
		    {
			AssignItemText16BMG(dest,src->text,src->len);
			dest->attrib_used = src->attrib_used;
			memcpy(dest->attrib,src->attrib,dest->attrib_used);
			dirty = true;
			pfi->num = 0;
		    }
		    break;
		}
	    }
	}
    }
    ResetParamField(&assign);


    //--- terminate

    if (bmg->data_alloced)
	FREE(bmg->data);
    bmg->data = 0;
    bmg->data_size = 0;

    return err_stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    ScanBMG(), LoadBMG()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ScanBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    bool		initialize,	// true: initialize 'bmg'
    ccp			fname,		// NULL or filename for error messages
    const u8		* data,		// data / if NULL: use internal data
    size_t		data_size	// size of 'data' if data not NULL
)
{
    if (initialize)
	InitializeBMG(bmg);
    else if (data)
	ResetBMG(bmg);

    if (data)
    {
	bmg->data = (u8*)data;
	bmg->data_size = data_size;
    }

    if (fname)
	bmg->fname = STRDUP(fname);

    const bmg_header_t * bh = (bmg_header_t*)bmg->data;

    if (!memcmp(bh->magic,BMG_MAGIC,sizeof(bh->magic)))
    {
	bmg->endian = get_endian_by_bh(bh,bmg->data_size);
	if (bmg->endian)
	    return ScanRawBMG(bmg);
    }

    ccp ptr = (ccp)bmg->data + GetTextBOMLen(bmg->data,bmg->data_size);
    if ( !memcmp(ptr,BMG_TEXT_MAGIC,4) )
	return ScanTextBMG(bmg);

    return ERROR0(ERR_INVALID_DATA,"No BMG file: %s\n",bmg->fname);
}

///////////////////////////////////////////////////////////////////////////////

enumError LoadBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    bool		initialize,	// true: initialize 'bmg'
    ccp			parent_fname,	// NULL or filename of parent for dir-extract
    ccp			fname,		// filename of source
    FileMode_t		file_mode	// open modes
)
{
    DASSERT(bmg);
    DASSERT(fname);
    PRINT("LoadBMG(%p,init=%d) fname=%s\n", bmg, initialize, fname );

    if (initialize)
	InitializeBMG(bmg);
    else
	ResetBMG(bmg);

    if (!strcmp(fname,"0"))
    {
	bmg->is_text_src = false;
	bmg->fname = STRDUP(fname);
	return ERR_OK;
    }

    char path[PATH_MAX];
    if ( *fname != '/' && parent_fname )
    {
	ccp slash = strrchr(parent_fname,'/');
	char *dest;
	if (slash)
	    dest = StringCopySM(path,sizeof(path)-2,parent_fname,slash-parent_fname);
	else
	    dest = StringCopyS(path,sizeof(path)-2,parent_fname);
	*dest++ = '/';
	StringCopyE(dest,path+sizeof(path),fname);
	fname = path;
    }
    noPRINT(">>> fname: |%s|d\n",fname);

    u8 *data;
    uint size;
    enumError err = OpenReadFile( fname, 0, file_mode, 0,0,
				&data, &size, &bmg->fname, &bmg->fatt );
    if ( err == ERR_OK )
    {
	bmg->data	= data;
	bmg->data_size	= size;
	bmg->data_alloced = true;
	return ScanBMG(bmg,false,0,0,0);
    }

    FREE(data);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct bmg_create_t		///////////////
///////////////////////////////////////////////////////////////////////////////

static void grow_bi_list ( bmg_create_t *bc, uint slot )
{
    if ( bc->bi_size <= slot )
    {
	const uint esize = sizeof(*bc->bi_list);
	const uint old_size = bc->bi_size;
	const uint byte_size = ( slot + slot/8 + 10 ) * esize;
	bc->bi_size = GetGoodAllocSize(byte_size) / esize;
	bc->bi_list = REALLOC( bc->bi_list, bc->bi_size * esize );
	memset( bc->bi_list + old_size, 0, ( bc->bi_size - old_size ) * esize );
	PRINT("GROW BI-LIST: %u/%u/%u\n",slot,bc->bi_used,bc->bi_size);
    }
}

//-----------------------------------------------------------------------------

void InitializeCreateBMG ( bmg_create_t *bc, bmg_t *bmg )
{
    DASSERT(bc);
    DASSERT(bmg);

    memset(bc,0,sizeof(*bc));
    bc->bmg	 = bmg;
    bc->endian	 = get_bmg_endian(bmg);
    bc->have_mid = opt_bmg_mid == OFFON_AUTO
		 ? bmg->have_mid : opt_bmg_mid >= OFFON_ON;

    uint n_predef = 0, other_count = 0;
    bmg_item_t *bi, *bi_end = bmg->item + bmg->item_used;
    for ( bi = bmg->item; bi < bi_end; bi++ )
	if (bi->text)
	{
	    if ( bi->slot == BMG_NO_PREDEF_SLOT )
		other_count++;
	    else if ( n_predef <= bi->slot )
		n_predef = bi->slot + 1;
	}

    bmg->have_predef_slots = n_predef >= 0;
    if (n_predef)
    {
	grow_bi_list( bc, n_predef + other_count );

	uint next = n_predef;
	for ( bi = bc->bmg->item; bi < bi_end; bi++ )
	    if (bi->text)
	    {
		uint slot = bi->slot;
		if ( slot >= n_predef || bc->bi_list[slot] )
		{
		    slot = next++;
		    grow_bi_list(bc,slot);
		}
		PRINT0(">>> ASSIGN %4x = %4x\n",slot,bi->mid);
		bc->bi_list[slot] = bi;
	    }

	bc->bi_used = next;
	PRINT("BI-LIST: %u/%u/%u\n",next,bc->bi_used,bc->bi_size);
    }

    uint n_item = bc->bi_used;
    if ( n_item < bmg->item_used )
	 n_item = bmg->item_used;

    InitializeFastBufAlloc( &bc->inf, n_item * bmg->inf_size );
    InitializeFastBufAlloc( &bc->dat, n_item * 50 );
    InitializeFastBufAlloc( &bc->mid, n_item * sizeof(u32) );

    if (bc->have_mid)
    {
	uint i;
	for ( i = 0; i < bc->bi_used; i++ )
	    AppendBE32FastBuf(&bc->mid,0xffff);
    }
}

//-----------------------------------------------------------------------------

void ResetCreateBMG ( bmg_create_t *bc )
{
    DASSERT(bc);

    FREE(bc->bi_list);
    ResetFastBuf(&bc->inf);
    ResetFastBuf(&bc->dat);
    ResetFastBuf(&bc->mid);
    memset(bc,0,sizeof(0));
}

//-----------------------------------------------------------------------------

bmg_item_t * GetFirstBI ( bmg_create_t *bc )
{
    DASSERT(bc);
    DASSERT(bc->bmg);

    bc->bi	= bc->bmg->item;
    bc->bi_end	= bc->bi + bc->bmg->item_used;
    bc->slot	= -1;
    return GetNextBI(bc);
}

//-----------------------------------------------------------------------------

bmg_item_t * GetNextBI ( bmg_create_t *bc )
{
    DASSERT(bc);
    DASSERT(bc->bmg);
    DASSERT(bc->bmg->endian);

    bmg_t *bmg = bc->bmg;

    bmg_item_t *bi = 0;
    int slot = bc->slot + 1;
    if (bc->bi_list)
    {
	while ( slot < bc->bi_used && !bc->bi_list[slot] )
	    slot++;
	if ( slot < bc->bi_used )
	{
	    bi = bc->bi_list[slot];
	    PRINT0(">>> BI/LIST: %4x: %04x\n",slot,bi->mid);
	}
    }
    else if ( bc->bi < bc->bi_end )
    {
	bi = bc->bi++;
	PRINT0(">>> BI/DIRECT: %4x: %04x\n",slot,bi->mid);
    }

    if (!bi)
    {
	bc->slot = -1;
	return 0;
    }

    //--- found: assign data

    bc->slot = slot;

    u32 *pmid = (u32*)WriteFastBuf( &bc->mid, bc->slot*sizeof(u32),
				    0, sizeof(u32) );
    DASSERT(pmid);
    *pmid = bc->endian->h2nl(bi->mid);

    const uint inf_size = bmg->inf_size;
    bmg_inf_item_t *ii = (bmg_inf_item_t*)WriteFastBuf
		    ( &bc->inf, bc->slot * inf_size, 0, inf_size );
    DASSERT(ii);
    if ( !isSpecialEntryBMG(bi->text) )
	ii->offset = bc->endian->h2nl(GetFastBufLen(&bc->dat));
    memcpy(ii->attrib,bi->attrib,bmg->attrib_used);

    if ( bc->n_msg <= bc->slot )
	 bc->n_msg  = bc->slot + 1;
    return bi;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CreateRawBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void create_raw_cp1252 ( bmg_create_t *bc )
{
    DASSERT(bc);
    DASSERT(bc->bmg);
    DASSERT(!GetFastBufLen(&bc->dat));

    AppendCharFastBuf(&bc->dat,0);

    bmg_item_t *bi;
    for ( bi = GetFirstBI(bc); bi; bi = GetNextBI(bc) )
    {
	if ( !isSpecialEntryBMG(bi->text) )
	{
	    const u16 *ptr = bi->text;
	    const u16 *end = ptr + bi->len;
	    while ( ptr < end )
	    {
		u16 code = ntohs(*ptr++);
		if ( code == 0x1a )
		{
		    AppendCharFastBuf(&bc->dat,code);
		    uint len = *(u8*)ptr;
		    AppendCharFastBuf(&bc->dat,len-1);
		    AppendFastBuf(&bc->dat,(u8*)ptr+1,len-3);
		    ptr += (len-1)/2;
		}
		else if (code)
		{
		    if ( code >= 0x100 )
		    {
			uint i;
			for ( i = 0; i < 0x20; i++ )
			    if ( TableCP1252_80[i] == code )
			    {
				code = i + 0x80;
				break;
			    }
		    }
		    if ( code < 0x100 )
			AppendCharFastBuf(&bc->dat,code);
		}
	    }
	    AppendCharFastBuf(&bc->dat,0);
	}
    }
}

//-----------------------------------------------------------------------------

static void create_raw_utf16 ( bmg_create_t *bc )
{
    DASSERT(bc);
    DASSERT(bc->bmg);
    DASSERT(!GetFastBufLen(&bc->dat));

    AppendBE16FastBuf(&bc->dat,0);

    bmg_item_t *bi;
    for ( bi = GetFirstBI(bc); bi; bi = GetNextBI(bc) )
    {
	if ( !isSpecialEntryBMG(bi->text) )
	{
	    AppendFastBuf( &bc->dat, bi->text, bi->len*2 );
	    AppendBE16FastBuf(&bc->dat,0);
	}
    }
}

//-----------------------------------------------------------------------------

static void create_raw_shift_jis ( bmg_create_t *bc )
{
    DASSERT(bc);
    DASSERT(bc->bmg);
    DASSERT(!GetFastBufLen(&bc->dat));

    AppendCharFastBuf(&bc->dat,0);

    bmg_item_t *bi;
    for ( bi = GetFirstBI(bc); bi; bi = GetNextBI(bc) )
    {
	if ( !isSpecialEntryBMG(bi->text) )
	{
	    const u16 *ptr = bi->text;
	    const u16 *end = ptr + bi->len;
	    while ( ptr < end )
	    {
		const u16 code = ntohs(*ptr++);
		if ( code == 0x1a )
		{
		    AppendCharFastBuf(&bc->dat,code);
		    uint len = *(u8*)ptr;
		    AppendCharFastBuf(&bc->dat,len-1);
		    AppendFastBuf(&bc->dat,(u8*)ptr+1,len-3);
		    ptr += (len-1)/2;
		}
		else
		{
		    const int val = GetShiftJISChar(code);
		    if ( val >= 0x100 )
			AppendBE16FastBuf(&bc->dat,val);
		    else if ( val > 0 )
			AppendCharFastBuf(&bc->dat,val);
		}
	    }
	    AppendCharFastBuf(&bc->dat,0);
	}
    }
}

//-----------------------------------------------------------------------------

static void create_raw_utf8 ( bmg_create_t *bc )
{
    DASSERT(bc);
    DASSERT(bc->bmg);
    DASSERT(!GetFastBufLen(&bc->dat));

    AppendCharFastBuf(&bc->dat,0);

    bmg_item_t *bi;
    for ( bi = GetFirstBI(bc); bi; bi = GetNextBI(bc) )
    {
	if ( !isSpecialEntryBMG(bi->text) )
	{
	    const u16 *ptr = bi->text;
	    const u16 *end = ptr + bi->len;
	    while ( ptr < end )
	    {
		u16 code = ntohs(*ptr++);
		if ( code == 0x1a )
		{
		    AppendCharFastBuf(&bc->dat,code);
		    uint len = *(u8*)ptr;
		    AppendCharFastBuf(&bc->dat,len-1);
		    AppendFastBuf(&bc->dat,(u8*)ptr+1,len-3);
		    ptr += (len-1)/2;
		}
		else if (code)
		    AppendUTF8CharFastBuf(&bc->dat,code);
	    }
	    AppendCharFastBuf(&bc->dat,0);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError CreateRawBMG
(
    bmg_t		* bmg		// pointer to valid BMG
)
{
    DASSERT(bmg);
    DASSERT(bmg->endian);
    TRACE("CreateRawBMG()\n");


    //--- setup bmg_create_t

// [[opt-endian]]
    bmg_create_t bc;
    InitializeCreateBMG(&bc,bmg);
    PRINT("fb:inf: %s\n", GetFastBufStatus(&bc.inf) );
    PRINT("fb:dat: %s\n", GetFastBufStatus(&bc.dat) );
    PRINT("fb:mid: %s\n", GetFastBufStatus(&bc.mid) );


    //--- check encoding

    const int encoding = bmg->legacy
		? BMG_ENC_CP1252
		: CheckEncodingBMG(opt_bmg_encoding,bmg->encoding);
    switch (encoding)
    {
     case BMG_ENC_CP1252:
	create_raw_cp1252(&bc);
	break;

     case BMG_ENC_UTF16BE:
	create_raw_utf16(&bc);
	break;

     case BMG_ENC_SHIFT_JIS:
	SetupGetShiftJISCache();
	create_raw_shift_jis(&bc);
	break;

     case BMG_ENC_UTF8:
	create_raw_utf8(&bc);
	break;

     default:
	return ERROR0(ERR_ERROR,
			"Can't create BMG with encoding #%u (%s): %s\n",
			encoding, GetEncodingNameBMG(encoding,"?"), bmg->fname );
    }


    //--- count size of raw sections

    uint total_raw_size = 0, raw_count = 0;
    if (bmg->use_raw_sections)
    {
	bmg_raw_section_t *raw;
	for ( raw = bmg->first_raw; raw; raw = raw->next )
	{
	    const uint good_size = sizeof(bmg_section_t) + GetFastBufLen(&raw->data);
	    if ( raw->total_size < good_size )
		raw->total_size = ALIGN32(good_size, raw->next ? opt_bmg_align : 4 );
	    total_raw_size += raw->total_size;
	    raw_count++;
	}
    }


    //--- calculate sizes

    const mem_t inf = GetFastBufMem(&bc.inf);
    const mem_t mid = GetFastBufMem(&bc.mid);
    const mem_t dat = GetFastBufMem(&bc.dat);

    const int align	= bmg->legacy ? BMG_LEGACY_BLOCK_SIZE : opt_bmg_align;
    const u32 inf_size	= ALIGN32( sizeof(bmg_inf_t) + inf.len, align );
    const u32 dat_size	= ALIGN32( sizeof(bmg_dat_t) + dat.len, align );
    const u32 mid_size	= bc.have_mid
			? ALIGN32( sizeof(bmg_mid_t) + mid.len, align ) : 0;

    const u32 total_size = sizeof(bmg_header_t)
			 + inf_size + dat_size + mid_size
			 + total_raw_size;

    PRINT("SIZE(%u msg): %zx + %x[/%x] + %x + %x = %x\n",
		bc.n_msg, sizeof(bmg_header_t),
		inf_size,  bmg->inf_size, dat_size, mid_size, total_size );


    //--- alloc data

    FREE(bmg->raw_data);
    bmg->raw_data = CALLOC(total_size,1);
    bmg->raw_data_size = total_size;


    //---- setup bmg header

    bmg_header_t *bh	= (bmg_header_t*)bmg->raw_data;
    bh->size		= bc.endian->h2nl( bmg->legacy ? total_size/BMG_LEGACY_BLOCK_SIZE: total_size );
    bh->n_sections	= bc.endian->h2nl( ( bc.have_mid ? 3 : 2 ) + raw_count );
    bh->encoding	= bmg->legacy ? 0 : encoding;
    memcpy(bh->magic,BMG_MAGIC,sizeof(bh->magic));

    u8 *dest = (u8*)(bh+1);
    DASSERT( dest <= bmg->raw_data + total_size );


    //---- setup inf section

    bmg_inf_t *pinf	= (bmg_inf_t*)dest;
    pinf->size		= bc.endian->h2nl(inf_size);
    pinf->n_msg		= bc.endian->h2ns(bc.n_msg);
    pinf->inf_size	= bc.endian->h2ns(bmg->inf_size);
    pinf->unknown_0c	= bc.endian->h2nl(bmg->unknown_inf_0c);
    memcpy(pinf->magic,BMG_INF_MAGIC,sizeof(pinf->magic));
    memcpy(pinf->list,inf.ptr,inf.len);

    dest += inf_size;
    DASSERT( dest <= bmg->raw_data + total_size );


    //---- setup dat section

    bmg_dat_t	* pdat	= (bmg_dat_t*)dest;
    pdat->size		= bc.endian->h2nl(dat_size);
    memcpy(pdat->magic,BMG_DAT_MAGIC,sizeof(pdat->magic));
    memcpy(pdat->text_pool,dat.ptr,dat.len);

    dest += dat_size;
    DASSERT( dest <= bmg->raw_data + total_size );


    //---- setup mid section

    if (bc.have_mid)
    {
	bmg_mid_t *pmid		= (bmg_mid_t*)dest;
	pmid->size		= bc.endian->h2nl(mid_size);
	pmid->n_msg		= bc.endian->h2ns(bc.n_msg);
	pmid->unknown_0a	= bc.endian->h2ns(bmg->unknown_mid_0a);
	pmid->unknown_0c	= bc.endian->h2nl(bmg->unknown_mid_0c);
	memcpy(pmid->magic,BMG_MID_MAGIC,sizeof(pmid->magic));
	memcpy(pmid->mid,mid.ptr,mid.len);

	dest += mid_size;
	DASSERT( dest <= bmg->raw_data + total_size );
    }


    //---- copy raw sections

    if (total_raw_size)
    {
	bmg_raw_section_t *raw;
	for ( raw = bmg->first_raw; raw; raw = raw->next )
	{
	    PRINT("SAVE/RAW SECTION »%s« data: %u = 0x%x, total: %u = 0x%x\n",
			PrintID(raw->magic,4,0),
			GetFastBufLen(&raw->data), GetFastBufLen(&raw->data),
			raw->total_size, raw->total_size );

	    bmg_section_t *sect = (bmg_section_t*)dest;
	    memcpy(sect->magic,raw->magic,sizeof(sect->magic));
	    sect->size = bc.endian->h2nl(raw->total_size);
	    const mem_t mem = GetFastBufMem(&raw->data);
	    memcpy(sect->data,mem.ptr,mem.len);

	    dest += raw->total_size;
	    DASSERT( dest <= bmg->raw_data + total_size );
	}
    }


    //--- clean

    DASSERT( dest == bmg->raw_data + total_size );

    PRINT("fb:inf: %s\n", GetFastBufStatus(&bc.inf) );
    PRINT("fb:mid: %s\n", GetFastBufStatus(&bc.mid) );
    PRINT("fb:dat: %s\n", GetFastBufStatus(&bc.dat) );
    ResetCreateBMG(&bc);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SaveRawBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SaveRawBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    ccp			fname,		// filename of destination
    FileMode_t		fmode,		// create-file mode
    bool		set_time	// true: set time stamps
)
{
    DASSERT(bmg);
    DASSERT(fname);
    PRINT("SaveRawBMG(%s,%d)\n",fname,set_time);


    //--- create raw data

    enumError err = CreateRawBMG(bmg);
    if (err)
	return err;
    DASSERT(bmg->raw_data);
    DASSERT(bmg->raw_data_size);


    //--- write to file

    File_t F;
    err = CreateFile(&F,true,fname,fmode);
    if ( err > ERR_WARNING || !F.f )
	return err;
    SetFileAttrib(&F.fatt,&bmg->fatt,0);

    if ( fwrite(bmg->raw_data,1,bmg->raw_data_size,F.f) != bmg->raw_data_size )
	err = FILEERROR1(&F,ERR_WRITE_FAILED,"Write failed: %s\n",fname);
    return ResetFile(&F,set_time);
}

///////////////////////////////////////////////////////////////////////////////

enumError SaveRawFileBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    FILE		* f,		// valid output file
    ccp			fname		// NULL or filename for error messages
)
{
    DASSERT(bmg);


    //--- create raw data

    enumError err = CreateRawBMG(bmg);
    if (err)
	return err;
    DASSERT(bmg->raw_data);
    DASSERT(bmg->raw_data_size);


    //--- write to file

    if ( fwrite(bmg->raw_data,1,bmg->raw_data_size,f) != bmg->raw_data_size )
	err = ERROR1(ERR_WRITE_FAILED,"Write failed: %s\n",fname?fname:"?");
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SaveTextBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

char * PrintAttribBMG
(
    char		*buf,		// destination buffer
    uint		buf_size,	// size of 'buf', BMG_ATTRIB_BUF_SIZE is good
    const u8		*attrib,	// NULL or attrib to print
    uint		attrib_used,	// used length of attrib
    const u8		*def_attrib,	// NULL or default attrib
    bool		force		// true: always print '[]'
)
{
    DASSERT(buf);
    DASSERT( buf_size >= 10 );
    DASSERT( attrib_used <= BMG_ATTRIB_SIZE);

    static u8 all0[BMG_ATTRIB_SIZE] = {0};

    if ( !attrib
	|| !attrib_used
// [[BMG_ATTRIB_SIZE]]
	|| def_attrib && !memcmp(attrib,def_attrib,attrib_used) )
    {
	if (force)
	{
	    *buf++ = '[';
	    *buf++ = ']';
	}
	*buf = 0;
	return buf;
    }

    char *buf_end = buf + buf_size - 2;
    *buf++ = '[';

// [[BMG_ATTRIB_SIZE]]
    if (!memcmp(attrib,all0,attrib_used))
	*buf++ = '0';
    else
    {
	uint idx;
	for ( idx = 0; idx < attrib_used; idx += 4 )
	{
	    if ( idx && buf < buf_end )
		*buf++ = '/';

	    u32 attrib_val = be32(attrib+idx);
	    ccp sep = "";
	    while (attrib_val)
	    {
		const uint val = (attrib_val >> 24) & 0xff;
		if (val)
		    buf = snprintfE(buf,buf_end,"%s%x",sep,val);
		else if ( *sep && buf < buf_end )
		    *buf++ = *sep;
		sep = ",";
		attrib_val <<= 8;
	    }
	}
	while ( buf[-1] == '/' )
	    buf--;
    }
    *buf++ = ']';
    *buf = 0;
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

static void print_message
	( FILE *f, bmg_t *bmg, const bmg_item_t *bi, ccp mid_name )
{
    DASSERT(f);
    DASSERT(bmg);
    DASSERT(bi);

    if ( bmg->active_cond != bi->cond )
    {
	bmg->active_cond = bi->cond;
	fprintf(f,"@? %x\n",bmg->active_cond);
    }

    ccp slot;
    char slot_buf[10];
    if (!bmg->have_predef_slots)
	slot = EmptyString;
    else if ( bi->slot == BMG_NO_PREDEF_SLOT )
	slot = "      ";
    else
    {
	snprintf(slot_buf,sizeof(slot_buf)," @%04x",bi->slot);
	slot = slot_buf;
    }

    char attrib[100+BMG_ATTRIB_BUF_SIZE];
// [[xbuf]]
    char xbuf[BMG_MSG_BUF_SIZE];

    const bool print_attrib
		=  !opt_bmg_no_attrib
		&& opt_bmg_single_line < 2
		&& bi->attrib_used
		&& memcmp(bi->attrib,bmg->attrib,bmg->attrib_used);

    if ( print_attrib && !opt_bmg_inline_attrib )
    {
	if ( bmg->attrib_used == 4 )
	{
	    // classic, compatible with all versions
	    fprintf(f,"%s\t~%s 0x%08x\r\n",mid_name,slot,be32(bi->attrib));
	}
	else
	{
	    // use new attrib vector
	    PrintAttribBMG( attrib, sizeof(attrib),
				bi->attrib, bi->attrib_used, bmg->attrib, true );
	    fprintf(f,"%s\t~%s %s\r\n",mid_name,slot,attrib);
	}
    }

    if ( print_attrib && opt_bmg_inline_attrib )
    {
	char *dest = attrib + snprintfS(attrib,sizeof(attrib),"%s%s ",mid_name,slot);
	dest = PrintAttribBMG( dest, attrib+sizeof(attrib)-dest,
				bi->attrib, bi->attrib_used, bmg->attrib, false );
	*dest++ = ' ';
	*dest = 0;
	DASSERT( dest < attrib + sizeof(attrib) - 1 );
    }
    else
	if (*slot)
	    snprintf(attrib,sizeof(attrib),"%s%s ",mid_name,slot);
	else
	    snprintf(attrib,sizeof(attrib),"%s\t",mid_name);

    if ( bi->text == bmg_null_entry )
	fprintf(f,"%s/\r\n",attrib);
    else if ( bi->text )
    {
	fputs(attrib,f);
	const int use_colors = opt_bmg_colors != 1
				? opt_bmg_colors
				: bmg->use_color_names > 0;
// [[xbuf?]]
	uint len = PrintString16BMG( xbuf, sizeof(xbuf), bi->text, bi->len,
					BMG_UTF8_MAX, 0, use_colors );
// [[xbuf?]]
	ccp ptr = xbuf;
	ccp end = ptr + len;
	ccp sep = "=";
	do
	{
	    ccp start = ptr;
	    ccp temp_end = ptr + 72;
	    if ( temp_end > end )
		 temp_end = end;
	    ccp good_break = 0;
	    if ( opt_bmg_single_line || opt_bmg_export )
		ptr = end;

	    bool first_loop = true;
	    while ( ptr < temp_end && !good_break )
	    {
		for ( ; ptr <= temp_end; ptr++ )
		{
		    if ( ptr == end )
		    {
			good_break = ptr;
			break;
		    }

		    if ( *ptr == '\\' )
		    {
			if ( ++ptr >= temp_end )
			    break;
			if ( *ptr == 'n' )
			{
			    good_break = ++ptr;
			    break;
			}
			if ( *ptr != '\\' && ptr-start > 40 )
			{
			    good_break = ptr-1;
			    if (!first_loop)
				break;
			}
		    }
		    else if ( *ptr == ' ' && ptr-start > 40 )
		    {
			good_break = ptr;
			if (!first_loop)
			    break;
		    }
		}
		temp_end = end;
		first_loop = false;
	    }
	    if ( good_break && good_break < end - 8 )
		ptr = good_break;

	    fprintf(f,"%s %.*s\r\n", sep, (int)(ptr-start), start );
	    sep = "\t+";

	} while ( ptr < end );
    }
}

///////////////////////////////////////////////////////////////////////////////

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
)
{
    DASSERT(bmg);
    DASSERT(fname);
    PRINT("SaveTextBMG(%s,%d)\n",fname,set_time);

    //--- open file

    File_t F;
    enumError err = CreateFile(&F,true,fname,fmode);
    if ( err > ERR_WARNING || !F.f )
	return err;
    SetFileAttrib(&F.fatt,&bmg->fatt,0);

    err = SaveTextFileBMG(bmg,F.f,fname,force_numeric,brief_count);

    ResetFile(&F,set_time);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////

enumError SaveTextFileBMG
(
    bmg_t		* bmg,		// pointer to valid bmg
    FILE		* f,		// valid output file
    ccp			fname,		// NULL or filename for error messages
    bool		force_numeric,	// true: force numeric MIDs
    uint		brief_count	// >0: suppress syntax info
					// >1: suppress all comments
					// >2: suppress '#BMG' magic
)
{
    // use DOS/Windows line format -> unix can handle it ;)

    DASSERT(bmg);
    bmg->active_cond = 0;

    enum { BRIEF_ALL, BRIEF_NO_SYNTAX, BRIEF_NO_COMMENT, BRIEF_NO_MAGIC };

    bool already_printed[BMG__VIP_N];
    memset(already_printed,0,sizeof(already_printed));
    char mid_name[10], abuf[BMG_ATTRIB_BUF_SIZE];


    static ccp param_announce =
	"# All parameters begin with '@'. Unknown parameters are ignored on scanning.\r\n";

    static const char text_head[] =
    {
      "#BMG  <<<  The first 4 characters '#BMG' are the magic for a BMG text file.\r\n"
      "#     <<<  Don't remove them!\r\n"
      "#\r\n"
      "# Details about BMG text files are available in the Web:\r\n"
      "#  * Syntax and Semantics: https://szs.wiimm.de/doc/bmg/text\r\n"
      "#  * The BMG file format:  https://szs.wiimm.de/r/wiki/BMG\r\n"
      "#\r\n"
      "#\f\r\n"
      "#------------------------------------------------------------------------------\r\n"
    };

    static const char text_param[] =
    {
      "%s"
      "\r\n"
      "# The endianness of binary files: 0=big endian (default), 1=little endian.\r\n"
      "# Little endian is only tested for encoding UTF-8 (need examples of analysis).\r\n"
      "@ENDIAN = %u\r\n"
      "\r\n"
      "# If 1, then enable legacy (GameCube) mode for old binary BMG files.\r\n"
      "# If enabled, ENCODING is always CP1252.\r\n"
      "@LEGACY = %u\r\n"
      "\r\n"
      "# Define encoding of BMG: 1=CP1252, 2=UTF-16/be, 3=Shift-JIS, 4=UTF-8\r\n"
      "@ENCODING = %u\r\n"
      "\r\n"
      "# Create »MID1« section: 0=off, 1=on\r\n"
      "@BMG-MID = %u\r\n"
      "\r\n"
      "# Size of each element of section 'INF1' (MKW=8).\r\n"
      "# This setting has impact to attributes and MKW features.\r\n"
      "@INF-SIZE = 0x%02x\r\n"
      "\r\n"
      "# Default attribute values for this BMG (MKW=[1])\r\n"
      "@DEFAULT-ATTRIBS = %s\r\n"
      "\r\n"
      "# Use MKW specific color names: 0=off, 1=auto, 2=on\r\n"
      "@COLOR-NAMES = %u\r\n"
      "\r\n"
      "# Use MKW messages and track names: 0=off, 1=auto, 2=on\r\n"
      "@MKW-MESSAGES = %u\r\n"
      "\r\n"
      "#\f\r\n"
      "#------------------------------------------------------------------------------\r\n"
    };

    static const char unknown_param[] =
    {
      "%s"
      "\r\n"
      "# This part defines values for unknown parameters of section headers.\r\n"
      "@UNKNOWN-INF32-0C = %#10x	# 32 bit value of section INF1 offset 0x0c\r\n"
      "@UNKNOWN-MID16-0A = %#10x	# 16 bit value of section MID1 offset 0x0a\r\n"
      "@UNKNOWN-MID32-0C = %#10x	# 32 bit value of section MID1 offset 0x0c\r\n"
      "\r\n"
      "#\f\r\n"
      "#------------------------------------------------------------------------------\r\n"
    };


    const bool print_full = brief_count == BRIEF_ALL && !opt_bmg_export;
    if (print_full)
	fwrite(text_head,1,sizeof(text_head)-1,f);
    else if ( brief_count < BRIEF_NO_MAGIC )
	fprintf(f,"%s\r\n\r\n",BMG_TEXT_MAGIC);

    HavePredifinedSlots(bmg);
    bmg->legacy = bmg->legacy != 0;
    const endian_func_t *endian = get_bmg_endian(bmg);
    const int encoding = bmg->legacy
		? BMG_ENC_CP1252
		: CheckEncodingBMG(opt_bmg_encoding,bmg->encoding);

    const bool print_settings // print only, if unusual
		=  endian->is_le
		|| encoding != BMG_ENC__DEFAULT
		|| bmg->attrib_used != 4
		|| be32(bmg->attrib) != BMG_INF_STD_ATTRIB
		|| !bmg->have_mid
		|| !bmg->use_color_names
		|| !bmg->use_mkw_messages
		;

    if	(print_settings)
    {
	PrintAttribBMG(abuf,sizeof(abuf),bmg->attrib,bmg->attrib_used,0,true);
	if (print_full)
	{
	    fprintf(f,text_param,
			param_announce,
			endian->is_le, bmg->legacy,
			encoding, bmg->have_mid, bmg->inf_size, abuf,
			bmg->use_color_names, bmg->use_mkw_messages );
	    param_announce = "";
	}
	else if ( brief_count < BRIEF_NO_MAGIC )
	    fprintf(f,"@ENDIAN          = %u\r\n"
			"@LEGACY          = %u\r\n"
			"@ENCODING        = %u\r\n"
			"@BMG-MID         = %u\r\n"
			"@INF-SIZE        = 0x%02x\r\n"
			"@DEFAULT-ATTRIBS = %s\r\n"
			"@COLOR-NAMES     = %u\r\n"
			"@MKW-MESSAGES    = %u\r\n"
			"\r\n",
			endian->is_le, bmg->legacy,
			encoding, bmg->have_mid, bmg->inf_size, abuf,
			bmg->use_color_names, bmg->use_mkw_messages );
    }

    if ( print_settings
	|| bmg->unknown_inf_0c != BMG_INF_DEFAULT_0C
	|| bmg->unknown_mid_0a != BMG_MID_DEFAULT_0A
	|| bmg->unknown_mid_0c != BMG_MID_DEFAULT_0C
    )
    {
	if (print_full)
	    fprintf(f,unknown_param,
			param_announce,
			bmg->unknown_inf_0c,
			bmg->unknown_mid_0a, bmg->unknown_mid_0c );
	else if ( brief_count < BRIEF_NO_MAGIC )
	    fprintf(f,"@UNKNOWN-INF32-0C = %#x\r\n"
			"@UNKNOWN-MID16-0A = %#x\r\n"
			"@UNKNOWN-MID32-0C = %#x\r\n"
			"\r\n",
			bmg->unknown_inf_0c,
			bmg->unknown_mid_0a, bmg->unknown_mid_0c );
    }


    //--- messages

    ccp intro = "\r\n";
    static const char intro_other[] = "\r\n#--- other messages\r\n\r\n";

    const bool support_mkw
		=  opt_bmg_support_mkw
		&& !force_numeric
		&& !opt_bmg_export
		&& bmg->attrib_used == 4
		&& be32(bmg->attrib) == BMG_INF_STD_ATTRIB
		&& bmg->have_mid
		&& !bmg->have_predef_slots;

    if (support_mkw)
    {
	//--- find track names

	int idx;
	uint last_cup = 0;
	for ( idx = 0; idx < BMG_N_TRACK; idx++ )
	{
	    const int tidx = GetTrackIndexBMG(idx,-1);
	    if ( tidx < 0 )
		continue;

	    const bmg_item_t * bi1 = FindItemBMG(bmg,MID_TRACK1_BEG+tidx);
	    if (bi1)
	    {
		const bmg_item_t * bi2 = FindItemBMG(bmg,MID_TRACK2_BEG+tidx);
		if ( bi2 && IsItemEqualBMG(bi1,bi2) )
		{
		    const uint cup = idx / 4 + '1';
		    if ( brief_count < BRIEF_NO_COMMENT && last_cup != cup )
		    {
			if (!last_cup)
			{
			    intro = intro_other;
			    fprintf(f,"\r\n#--- standard track names\r\n\r\n");
			}
			else
			    fputs("\r\n",f);
			last_cup = cup;
		    }
		    snprintf(mid_name,sizeof(mid_name), "  T%c%c", cup, idx % 4 + '1' );
		    print_message(f,bmg,bi1,mid_name);

		    already_printed[ bi1->mid - MID__VIP_BEG ] = true;
		    already_printed[ bi2->mid - MID__VIP_BEG ] = true;

		    if (opt_bmg_support_lecode)
		    {
			bi2 = FindItemBMG(bmg,MID_LE_TRACK_BEG+tidx);
			if ( bi2 && IsItemEqualBMG(bi1,bi2) )
			    already_printed[ bi2->mid - MID__VIP_BEG ] = true;
		    }
		    else if (opt_bmg_support_ctcode)
		    {
			bi2 = FindItemBMG(bmg,MID_CT_TRACK_BEG+tidx);
			if ( bi2 && IsItemEqualBMG(bi1,bi2) )
			    already_printed[ bi2->mid - MID__VIP_BEG ] = true;
		    }
		}
	    }
	}


	//--- find battle names

	last_cup = 0;
	for ( idx = 0; idx < BMG_N_ARENA; idx++ )
	{
	    const int tidx = GetArenaIndexBMG(idx,-1);
	    if ( tidx < 0 )
		continue;

	    const bmg_item_t * bi1 = FindItemBMG(bmg,MID_ARENA1_BEG+tidx);
	    if (bi1)
	    {
		const bmg_item_t * bi2 = FindItemBMG(bmg,MID_ARENA2_BEG+tidx);
		if ( bi2 && IsItemEqualBMG(bi1,bi2) )
		{
		    const uint cup = idx / 5 + '1';
		    if ( brief_count < BRIEF_NO_COMMENT && last_cup != cup )
		    {
			if (!last_cup)
			{
			    intro = intro_other;
			    fprintf(f,"\r\n#--- battle track names\r\n\r\n");
			}
			else
			    fputs("\r\n",f);
			last_cup = cup;
		    }
		    snprintf(mid_name,sizeof(mid_name), "  U%c%c", cup, idx % 5 + '1' );
		    print_message(f,bmg,bi1,mid_name);

		    already_printed[ bi1->mid - MID__VIP_BEG ] = true;
		    already_printed[ bi2->mid - MID__VIP_BEG ] = true;

		    if (opt_bmg_support_lecode)
		    {
			bi2 = FindItemBMG(bmg,MID_LE_ARENA_BEG+tidx);
			if ( bi2 && IsItemEqualBMG(bi1,bi2) )
			    already_printed[ bi2->mid - MID__VIP_BEG ] = true;
		    }
		    else if (opt_bmg_support_ctcode)
		    {
			bi2 = FindItemBMG(bmg,MID_CT_ARENA_BEG+tidx);
			if ( bi2 && IsItemEqualBMG(bi1,bi2) )
			    already_printed[ bi2->mid - MID__VIP_BEG ] = true;
		    }
		}
	    }
	}


	//--- find chat strings

	const bmg_item_t * bi1 = FindAnyItemBMG(bmg,MID_CHAT_BEG);
	const bmg_item_t * bi1end = FindAnyItemBMG(bmg,MID_CHAT_END);
	if ( bi1 < bi1end )
	{
	    if ( brief_count < BRIEF_NO_COMMENT )
	    {
		intro = intro_other;
		fprintf(f,"\r\n#--- online chat\r\n\r\n");
	    }

	    for ( ; bi1 < bi1end; bi1++ )
	    {
		DASSERT( bi1->mid >= MID_CHAT_BEG && bi1->mid <= MID_CHAT_END );
		snprintf(mid_name,sizeof(mid_name),"  M%02u",bi1->mid-MID_CHAT_BEG+1);
		print_message(f,bmg,bi1,mid_name);
		already_printed[bi1->mid-MID__VIP_BEG] = true;
	    }
	}
    }


    //--- print messages not already printed

    struct sep_t { u32 mid; ccp info; };

    static const struct sep_t sep_std_tab[] =
    {
	{0,0}
    };

    static const struct sep_t sep_mkw_tab[] =
    {
	{ MID_ENGINE_BEG,		"engines" },
	{ MID_ENGINE_END,		0 },
	{ MID_CHAT_BEG,			"online chat" },
	{ MID_CHAT_END,			0 },

	{ MID_RCUP_BEG,			"racing cup names" },
	{ MID_RCUP_END,			0 },
	{ MID_TRACK1_BEG,		"track names, 1. copy" },
	{ MID_TRACK1_END,		0 },
	{ MID_BCUP_BEG,			"battle cup names" },
	{ MID_BCUP_END,			0 },
	{ MID_TRACK2_BEG,		"track names, 2. copy" },
	{ MID_TRACK2_END,		0 },

	{ MID_ARENA1_BEG,		"arena names, 1. copy" },
	{ MID_ARENA1_END,		0 },
	{ MID_ARENA2_BEG,		"arena names, 2. copy" },
	{ MID_ARENA2_END,		0 },

	{ MID_PARAM_IDENTIFY,		"identification for third party tools" },
	{ MID_PARAM_IDENTIFY,		0 },

	{ MID_PARAM_BEG,		"parameters for mkw-ana" },
	{ MID_PARAM_END,		0 },

	{ MID_CT_TRACK_BEG,		"CT-CODE: track names" },
	{ MID_CT_TRACK_END,		0 },
	{ MID_CT_CUP_BEG,		"CT-CODE: cup names" },
	{ MID_CT_CUP_END,		0 },
	{ MID_CT_CUP_REF_BEG,		"CT-CODE: track-to-cup reference" },
	{ MID_CT_CUP_REF_END,		0 },

	{ MID_X_MESSAGE_BEG,		"new messages for CT/LE-CODE" },
	{ MID_X_MESSAGE_END,		0 },

	{ MID_LE_CUP_BEG,		"LE-CODE: cup names" },
	{ MID_LE_CUP_END,		0 },
	{ MID_LE_TRACK_BEG,		"LE-CODE: track names" },
	{ MID_LE_TRACK_END,		0 },
	{ MID_LE_CUP_REF_BEG,		"LE-CODE: track-to-cup reference" },
	{ MID_LE_CUP_REF_END,		0 },

	{0,0}
    };

    #ifdef TEST
    {
	fflush(stdout);
	const struct sep_t *sep;
	for ( sep = sep_std_tab+1; sep->mid; sep++ )
	    if ( sep[-1].mid > sep->mid )
		fprintf(stderr,
		    "Wrong order in sep_std_tab[%zu]:\n\t%04x %s\n\t%04x %s\n",
			sep-sep_std_tab, sep[-1].mid, sep[-1].info, sep->mid, sep->info );
	for ( sep = sep_mkw_tab+1; sep->mid; sep++ )
	    if ( sep[-1].mid > sep->mid )
		fprintf(stderr,
		    "Wrong order in sep_mkw_tab[%zu]:\n\t%04x %s\n\t%04x %s\n",
			sep-sep_mkw_tab, sep[-1].mid, sep[-1].info, sep->mid, sep->info );
	fflush(stderr);
    }
    #endif

    const struct sep_t *sep_tab = support_mkw ? sep_mkw_tab : sep_std_tab;
    const struct sep_t *sep = brief_count < BRIEF_NO_COMMENT ? sep_tab : 0;

    bmg_item_t * bi  = bmg->item;
    bmg_item_t * bi_end = bi + bmg->item_used;
    for ( ; bi < bi_end; bi++ )
    {
	const u32 mid = bi->mid;
	if (  mid < MID__VIP_BEG
	   || mid >= MID__VIP_END
	   || !already_printed[mid-MID__VIP_BEG]
	   )
	{
	    if ( sep && sep->mid && mid >= sep->mid )
	    {
		sep++;
		while ( sep->mid && mid >= sep->mid )
		    sep++;
		if ( sep > sep_tab+1 && sep[-2].mid == mid && sep[-2].info )
		    fprintf(f,"\r\n#--- [%x] %s\r\n", mid, sep[-2].info );
		else if ( sep[-1].info )
		    fprintf(f,"\r\n#--- [%x:%x] %s\r\n",
			sep[-1].mid, sep->mid-1,  sep[-1].info );
		else
		    fputs("\r\n",f);
		intro = 0;
	    }

	    if ( brief_count < BRIEF_NO_COMMENT && intro )
	    {
		fputs(intro,f);
		intro = 0;
	    }
	    snprintf(mid_name,sizeof(mid_name),"%6x",bi->mid);
	    print_message(f,bmg,bi,mid_name);
	}
    }


    //--- raw sections

    if ( bmg->use_raw_sections && bmg->first_raw )
    {
	XDump_t xd;
	InitializeXDump(&xd);
	SetupXDump(&xd,XDUMPC_DUMP);
	xd.f		= f;
	xd.indent	= 1;
	xd.prefix	= "@X";
	xd.eol		= "\r\n";
	xd.format	= XDUMPF_INT_1;
	xd.print_format	= false;
	xd.print_summary= false;

	bmg_raw_section_t *raw;
	for ( raw = bmg->first_raw; raw; raw = raw->next )
	{
	    PRINT("HEXDUMP SECTION »%s« data: %u = 0x%x, total: %u = 0x%x\n",
			PrintID(raw->magic,4,0),
			GetFastBufLen(&raw->data), GetFastBufLen(&raw->data),
			raw->total_size, raw->total_size );

	    char name[20];
	    fprintf(f,"\r\n#\f\r\n#--------------------------------------"
		    "----------------------------------------\r\n\r\n"
		    "@SECTION \"%s\"\r\n\r\n",
		    PrintEscapedString( name, sizeof(name),
				raw->magic, sizeof(raw->magic),
				CHMD_ESC, '"',  0) );

	    const mem_t data = GetFastBufMem(&raw->data);
	    xd.assumed_size = data.len;
	    XDump(&xd,data.ptr,data.len,true);
	}
    }


    //--- terminate

    if ( brief_count < 2 )
	fputs("\r\n",f);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanBMGMID()			///////////////
///////////////////////////////////////////////////////////////////////////////

int ScanBMGMID
(
    // returns 'scan->status'

    bmg_scan_mid_t	*scan,		// result, never NULL
    bmg_t		*bmg,		// NULL or bmg to update parameters
    ccp			src,		// source pointer
    ccp			src_end		// NULL or end of source
)
{
    DASSERT(scan);

    //---- setup

    memset(scan,0,sizeof(*scan));

    ccp ptr = src;
    if (!src)
	return 0;
    if (!src_end)
	src_end = src + strlen(src);


    //--- skip blanks and controls

    while ( ptr < src_end && (uchar)*ptr <= ' ' )
	ptr++;
    if ( ptr == src_end )
	goto abort;


    //--- find keyword

    ccp key = ptr;
    while ( ptr < src_end && isalnum((int)*ptr) )
	ptr++;
    const uint keylen = ptr - key;

    if ( keylen == 3 )
    {
	if ( ( *key == 'T' || *key == 't' )
		&& key[1] >= '1' && key[1] <= '8'
		&& key[2] >= '1' && key[2] <= '4' )
	{
	    int idx = 4 * ( key[1] - '1' ) + ( key[2] - '1' );
	    DASSERT(GetTrackIndexBMG);
	    idx = GetTrackIndexBMG(idx,-1);
	    if ( idx >= 0 )
	    {
		scan->mid[0] = MID_TRACK1_BEG + idx;
		scan->mid[1] = MID_TRACK2_BEG + idx;
		scan->n_mid = 2;
		if (opt_bmg_support_lecode)
		{
		    scan->mid[2] = MID_LE_TRACK_BEG + idx;
		    scan->n_mid++;
		}
		else if (opt_bmg_support_ctcode)
		{
		    scan->mid[2] = MID_CT_TRACK_BEG + idx;
		    scan->n_mid++;
		}
		if ( bmg && bmg->use_mkw_messages == 1 )
		    bmg->use_mkw_messages = 2;
	    }
	}
	else if ( ( *key == 'U' || *key == 'u' )
		&& key[1] >= '1' && key[1] <= '2'
		&& key[2] >= '1' && key[2] <= '5' )
	{
	    int idx = 5 * ( key[1] - '1' ) + ( key[2] - '1' );
	    DASSERT(GetArenaIndexBMG);
	    idx = GetArenaIndexBMG(idx,-1);
	    if ( idx >= 0 )
	    {
		scan->mid[0] = MID_ARENA1_BEG + idx;
		scan->mid[1] = MID_ARENA2_BEG + idx;
		scan->n_mid = 2;
		if (opt_bmg_support_lecode)
		{
		    scan->mid[2] = MID_LE_ARENA_BEG + idx;
		    scan->n_mid++;
		}
		else if (opt_bmg_support_ctcode)
		{
		    scan->mid[2] = MID_CT_ARENA_BEG + idx;
		    scan->n_mid++;
		}
		if ( bmg && bmg->use_mkw_messages == 1 )
		    bmg->use_mkw_messages = 2;
	    }
	}
	else if ( *key == 'M' || *key == 'm' )
	{
	    char *end;
	    u32 num = strtoul(key+1,&end,10);
	    if ( num > 0 && num <= BMG_N_CHAT && end == ptr )
	    {
		scan->mid[0] = MID_CHAT_BEG + num - 1;
		scan->n_mid = 1;
		if ( bmg && bmg->use_mkw_messages == 1 )
		    bmg->use_mkw_messages = 2;
	    }
	}
    }

    if ( !scan->n_mid && keylen )
    {
	char *end;
	uint num = strtoul(key,&end,16);
	if ( end == ptr )
	{
	    scan->mid[0] = num;
	    scan->n_mid = 1;
	}
	else
	    scan->status = -1;
    }

    if ( scan->n_mid > 0 )
	while ( ptr < src_end && ( *ptr == ' ' || *ptr == '\t' ) )
	    ptr++;

 abort:
    if (!scan->status)
	scan->status = scan->n_mid;
    scan->scan_end = scan->status > 0 ? (char*)ptr : (char*)src;
    return scan->status;
}

///////////////////////////////////////////////////////////////////////////////

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
)
{
// [[2do]]
 #if defined(TEST) || defined(DEBUG) || 1

    bmg_scan_mid_t scan;
    ScanBMGMID(&scan,bmg,src,src_end);

    if (ret_mid1)
	*ret_mid1 = scan.n_mid > 0 ? scan.mid[0] : 0;
    if (ret_mid2)
	*ret_mid2 = scan.n_mid > 1 ? scan.mid[1] : 0;
    if (ret_mid3)
	*ret_mid3 = scan.n_mid > 2 ? scan.mid[2] : 0;
    if (scan_end)
	*scan_end = scan.scan_end;
    return scan.status;

 #else
    //---- setup

    u32 mid1 = 0, mid2 = 0, mid3 = 0;
    int stat = 0;
    ccp ptr = src;

    if (!src)
	goto abort;
    if (!src_end)
	src_end = src + strlen(src);


    //--- skip blanks and controls

    while ( ptr < src_end && (uchar)*ptr <= ' ' )
	ptr++;
    if ( ptr == src_end )
	goto abort;


    //--- find keyword

    ccp key = ptr;
    while ( ptr < src_end && isalnum((int)*ptr) )
	ptr++;
    const uint keylen = ptr - key;

    if ( keylen == 3 )
    {
	if ( ( *key == 'T' || *key == 't' )
		&& key[1] >= '1' && key[1] <= '8'
		&& key[2] >= '1' && key[2] <= '4' )
	{
	    int idx = 4 * ( key[1] - '1' )+ ( key[2] - '1' );
	    DASSERT(GetTrackIndexBMG);
	    idx = GetTrackIndexBMG(idx,-1);
	    if ( idx >= 0 )
	    {
		mid1 = MID_TRACK1_BEG + idx;
		mid2 = MID_TRACK2_BEG + idx;
		stat = 2;
		if (opt_bmg_support_lecode)
		{
		    mid3 = MID_LE_TRACK_BEG + idx;
		    stat++;
		}
		else if (opt_bmg_support_ctcode)
		{
		    mid3 = MID_CT_TRACK_BEG + idx;
		    stat++;
		}
		if ( bmg && bmg->use_mkw_messages == 1 )
		    bmg->use_mkw_messages = 2;
	    }
	}
	else if ( ( *key == 'U' || *key == 'u' )
		&& key[1] >= '1' && key[1] <= '2'
		&& key[2] >= '1' && key[2] <= '5' )
	{
	    int idx = 5 * ( key[1] - '1' ) + ( key[2] - '1' );
	    DASSERT(GetArenaIndexBMG);
	    idx = GetArenaIndexBMG(idx,-1);
	    if ( idx >= 0 )
	    {
		mid1 = MID_ARENA1_BEG + idx;
		mid2 = MID_ARENA2_BEG + idx;
		stat = 2;
		if (opt_bmg_support_lecode)
		{
		    mid3 = MID_LE_ARENA_BEG + idx;
		    stat++;
		}
		else if (opt_bmg_support_ctcode)
		{
		    mid3 = MID_CT_ARENA_BEG + idx;
		    stat++;
		}
		if ( bmg && bmg->use_mkw_messages == 1 )
		    bmg->use_mkw_messages = 2;
	    }
	}
	else if ( *key == 'M' || *key == 'm' )
	{
	    char *end;
	    u32 num = strtoul(key+1,&end,10);
	    if ( num > 0 && num <= BMG_N_CHAT && end == ptr )
	    {
		mid1 = MID_CHAT_BEG + num - 1;
		stat = 1;
		if ( bmg && bmg->use_mkw_messages == 1 )
		    bmg->use_mkw_messages = 2;
	    }
	}
    }

    if ( !stat && keylen )
    {
	char *end;
	uint num = strtoul(key,&end,16);
	if ( end == ptr )
	{
	    mid1 = num;
	    stat = 1;
	}
	else
	    stat = -1;
    }

    if ( stat > 0 )
	while ( ptr < src_end && ( *ptr == ' ' || *ptr == '\t' ) )
	    ptr++;

 abort:
    if (ret_mid1)
	*ret_mid1 = mid1;
    if (ret_mid2)
	*ret_mid2 = mid2;
    if (ret_mid3)
	*ret_mid3 = mid3;
    if (scan_end)
	*scan_end = stat > 0 ? (char*)ptr : (char*)src;
    return stat;
 #endif
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			patch bmg: helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp (*GetContainerNameBMG) ( const bmg_t *bmg ) = 0;

//-----------------------------------------------------------------------------

ccp GetNameBMG ( const bmg_t *bmg, uint *name_len )
{
    DASSERT(bmg);

    ccp name = 0;
    if (GetContainerNameBMG)
	name = GetContainerNameBMG(bmg);

    if ( !name && bmg->fname )
    {
	name = bmg->src_is_arch ? 0 : strstr(bmg->fname,".d/");
	if (name)
	{
	    while ( name > bmg->fname && name[-1] != '/' )
		name--;
	}
	else
	{
	    name = strrchr(bmg->fname,'/');
	    name = name ? name + 1 : bmg->fname;
	}
    }

    if (!name)
	name = "";
    else
	while ( *name == '.' )
	    name++;

    if (name_len)
    {
	ccp end = strchr(name,'.');
	*name_len = end ? end-name : strlen(name);
    }

    return name;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct format_message_t
{
    bmg_t	*dest;			// valid destination bmg
    uint	mod_count;		// incremented for each modification

    ccp		full_name;		// NULL or full name of source file
    uint	full_len;		// length of 'full_name'
    ccp		good_name;		// NULL or good name of source file
    uint	good_len;		// length of 'good_name'
}
format_message_t;

//-----------------------------------------------------------------------------

static bool format_message
(
    format_message_t	*fm,		// valid object
    bmg_item_t		*dptr,		// valid message
    const u16		*format,	// format string
    uint		format_len	// length of 'format'
)
{
    DASSERT(fm);
    DASSERT(fm->dest);
    DASSERT(dptr);
    DASSERT(format);


    //--- line support

    typedef struct line_t
    {
	const u16 *beg;
	const u16 *end;
    }
    line_t;

    const uint MAX_LINE = 100;
    line_t line[100];
    uint n_line = 0;


    //--- setup dest and source

    u16 buf[10000];
    u16 *dest = buf;
    u16 *dest_end = buf + sizeof(buf)/sizeof(*buf);
    u16* marker = buf;

    const u16 *source = format;
    const u16 *source_end = format + format_len;


    //--- main loop

    while ( dest < dest_end && source < source_end )
    {
	if ( be16(source) != '%' )
	{
	    *dest++ = *source++;
	    continue;
	}

	if ( source[0] == source[1] )
	{
	    source++;
	    *dest++ = *source++;
	    continue;
	}

	PRINT("%04x = %c\n",be16(source), be16(source) & 0xff );
	const u16 *start = source++;

	u32 p1, p2, stat;
	source = ScanRange16U32(source,&stat,&p1,&p2,0,~(u32)0);
	if ( stat == 1 )
	    p1 = 0;
	else if ( stat < 1 )
	{
	    p1 = 0;
	    p2 = M1(p2);
	}

	char buf8[50];
	uint  copy_len = 0;
	const u8  * copy_src8 = 0;
	const u16 * copy_src16 = 0;

	const u16 conv_char = be16(source++);
	switch (conv_char)
	{
	    case 's':
	    case 'S':
		copy_len   = dptr->len;
		copy_src16 = dptr->text;
		break;

	    case 'i':
		copy_len  = snprintf(buf8,sizeof(buf8),"%x",dptr->mid);
		copy_src8 = (u8*)buf8;
		break;

	    case 'I':
		copy_len  = snprintf(buf8,sizeof(buf8),"%04x",dptr->mid);
		copy_src8 = (u8*)buf8;
		break;

	    case 'n':
	    case 'N':
		if (!fm->full_name)
		{
		    fm->full_name = GetNameBMG(fm->dest,&fm->full_len);
		    if ( fm->full_len > 4 && !strncasecmp(fm->full_name,"Menu",4) )
		    {
			fm->good_name = fm->full_name + 4;
			fm->good_len  = fm->full_len - 4;
		    }
		    else
		    {
			fm->good_name = fm->full_name;
			fm->good_len  = fm->full_len;
		    }
		}

		if ( conv_char == 'N' )
		{
		    copy_len  = fm->good_len;
		    copy_src8 = (u8*)fm->good_name;
		}
		else
		{
		    copy_len  = fm->full_len;
		    copy_src8 = (u8*)fm->full_name;
		}
		break;

	    case 'm':
		marker = dest;
		break;

	    case 'l':
	    case 'L':
	    case 'M':
		if (!n_line)
		{
		    const u16 *src = dptr->text;
		    const u16 *end = src + dptr->len;
		    while ( n_line < MAX_LINE )
		    {
			line[n_line].beg = src;
			while ( src < end )
			{
			    if ( ntohs(*src) == '\n' )
				break;

			    src += GetWordLength16BMG(src);
			}
			line[n_line].end = src;
			n_line++;
			if ( src >= end )
			    break;
			src++;
		    }
		    PRINT("L: %u lines found\n",n_line);
		}

		if ( stat == 1 )
		    p1 = p2;
		if ( p2 > n_line-1 )
		    p2 = n_line-1;
		PRINT("L: select %u..%u/%u\n",p1,p2,n_line);

		if ( p1 <= p2 )
		{
		    copy_src16 = line[p1].beg;
		    copy_len   = line[p2].end - copy_src16;
		}

		if ( conv_char == 'M' )
		{
		    if ( !copy_len && marker )
			dest = marker;
		    //marker = 0;
		    copy_len = 0;
		}
		else
		{
		    if ( !copy_len && conv_char == 'L' && marker )
			dest = marker;
		    //if ( conv_char == 'L' )
		    //	marker = 0;
		}
		p1 = 0;
		p2 = M1(p2);
		break;

	    default:
		copy_len   = source - start;
		copy_src16 = start;
		break;
	}

	if (copy_len)
	{
	    // correct p1 and p2
	    if ( p1 > copy_len )
		 p1 = copy_len;
	    if ( p2 > copy_len )
		 p2 = copy_len;

	    u32 count = p2 - p1;
	    if (copy_src16)
	    {
		copy_src16 += p1;
		while ( count-- > 0 && dest < dest_end )
		    *dest++ = *copy_src16++;
	    }
	    else if (copy_src8)
	    {
		copy_src8 += p1;
		while ( count-- > 0 && dest < dest_end )
		    *dest++ = htons(*copy_src8++);
	    }
	}
    }

    const uint new_len = dest - buf;
    if ( new_len != dptr->len || memcmp(buf,dptr->text,new_len*sizeof(*dest)) )
    {
	AssignItemText16BMG(dptr,buf,dest-buf);
	fm->mod_count++;
	return true;
    }
    return false;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			patch bmg: functions		///////////////
///////////////////////////////////////////////////////////////////////////////

bool PatchPrintBMG
(
    bmg_t		* bmg_dest,		// pointer to destination bmg
    const bmg_t		* bmg_src		// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	source	-	mix	mix	//
	//--------------------------------------//

    DASSERT(bmg_dest);
    DASSERT(bmg_src);
    copy_param(bmg_dest,bmg_src);

    format_message_t fm;
    memset(&fm,0,sizeof(fm));
    fm.dest = bmg_dest;

    bmg_item_t * dptr = bmg_dest->item;
    bmg_item_t * dend = dptr + bmg_dest->item_used;

    const bmg_item_t * sptr = bmg_src->item;
    const bmg_item_t * send = sptr + bmg_src->item_used;

    for ( ; dptr < dend; dptr++ )
    {
	u32 mid = dptr->mid;
	while ( sptr < send && sptr->mid < mid )
	    sptr++;
	if ( sptr == send )
	    break;
	if ( sptr->mid == mid )
	    format_message(&fm,dptr,sptr->text,sptr->len);
    }
    return fm.mod_count > 0;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchFormatBMG
(
    bmg_t		* bmg_dest,		// pointer to destination bmg
    const bmg_t		* bmg_src,		// pointer to source bmg
    ccp			format			// format parameter
)
{
    DASSERT(bmg_dest);
    DASSERT(bmg_src);
    copy_param(bmg_dest,bmg_src);

    const uint form_size = 2000;
    u16 form_buf[form_size];
    uint form_len = ScanString16BMG(form_buf,form_size,format,-1,bmg_dest);

    format_message_t fm;
    memset(&fm,0,sizeof(fm));
    fm.dest = bmg_dest;

    bmg_item_t * dptr = bmg_dest->item;
    bmg_item_t * dend = dptr + bmg_dest->item_used;
    for ( ; dptr < dend; dptr++ )
	format_message(&fm,dptr,form_buf,form_len);

    return fm.mod_count > 0;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchRegexBMG
(
    bmg_t		* bmg,			// pointer to destination bmg
    ccp			regex			// format parameter
)
{
 #if ENABLE_BMG_REGEX
    DASSERT(bmg);
    DASSERT(regex);

    PRINT("REGEX=%s\n",regex);

    Regex_t re;
    enumError err = ScanRegex(&re,true,regex);
    if ( err || !re.valid )
    {
	ResetRegex(&re);
	return false;
    }

    char buf1[1000];
    struct { FastBuf_t b; char space[500]; } res;
    InitializeFastBuf(&res,sizeof(res));

    bool dirty = false;
    bmg_item_t * iptr = bmg->item;
    bmg_item_t * iend = iptr + bmg->item_used;

    for ( ; iptr < iend; iptr++ )
    {
	uint len1 = PrintString16BMG( buf1, sizeof(buf1), iptr->text, iptr->len,
			BMG_UTF8_MAX, 0, true );
// printf("-> [%d] %s\n",len1,buf1);

	const int stat = ReplaceRegex(&re,&res.b,buf1,len1);
	if ( stat >= 0 )
	{
// printf("{%d] %.*s\n",stat,stat,GetFastBufString(&res.b));
	    AssignItemScanTextBMG(iptr,GetFastBufString(&res.b),stat);
	    dirty = true;
	}
    }

    ResetFastBuf(&res.b);
    ResetRegex(&re);
    return dirty;

 #else
    return false;
 #endif
}

///////////////////////////////////////////////////////////////////////////////

bool PatchRmRegexBMG
(
    bmg_t		* bmg,			// pointer to destination bmg
    ccp			regex			// format parameter
)
{
 #if ENABLE_BMG_REGEX
    DASSERT(bmg);
    DASSERT(regex);

    PRINT("REGEX=%s\n",regex);

    Regex_t re;
    enumError err = ScanRegex(&re,true,regex);
    if ( err || !re.valid )
    {
	ResetRegex(&re);
	return false;
    }

    char buf1[1000];
    struct { FastBuf_t b; char space[500]; } res;
    InitializeFastBuf(&res,sizeof(res));

    bool dirty = false;
    bmg_item_t * iptr = bmg->item;
    bmg_item_t * iend = iptr + bmg->item_used;

    for ( ; iptr < iend; iptr++ )
    {
	uint len1 = PrintString16BMG( buf1, sizeof(buf1), iptr->text, iptr->len,
			BMG_UTF8_MAX, 0, true );
	//printf("-> [%d] %s\n",len1,buf1);

	int stat = ReplaceRegex(&re,&res.b,buf1,len1);
	if ( stat >= 0 )
	{
	    if (stat)
		AssignItemScanTextBMG(iptr,GetFastBufString(&res.b),stat);
	    else
	    {
		FreeItemBMG(iptr);
		iptr->text = 0;
		ResetAttribBMG(bmg,iptr);
	    }
	    dirty = true;
	}
    }

    ResetFastBuf(&res.b);
    ResetRegex(&re);
    return dirty;

 #else
    return false;
 #endif
}

///////////////////////////////////////////////////////////////////////////////

bool PatchReplaceBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	source	-	patch	both	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    bmg_item_t * dptr = dest->item;
    bmg_item_t * dend = dptr + dest->item_used;

    const bmg_item_t * sptr = src->item;
    const bmg_item_t * send = sptr + src->item_used;

    bool dirty = false;
    for ( ; dptr < dend; dptr++ )
    {
	if (!dptr->text)
	    continue;

	u32 mid = dptr->mid;
	while ( sptr < send && sptr->mid < mid )
	    sptr++;
	if ( sptr == send )
	    break;

	if ( sptr->mid == mid
		&& !IsItemEqualBMG(dptr,sptr)
		&& ( !sptr->cond || FindItemBMG(dest,sptr->cond) ) )
	{
	    FreeItemBMG(dptr);
	    DASSERT( dptr->mid == sptr->mid );

	    CopyAttribBMG(dest,dptr,sptr);
	    dptr->len	= sptr->len;
	    dptr->text	= sptr->text;
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchInsertBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	source	patch	source	both	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    const bmg_item_t * sptr = src->item;
    const bmg_item_t * send = sptr + src->item_used;

    bool dirty = false;
    for ( ; sptr < send; sptr++ )
    {
	if ( sptr->cond && !FindItemBMG(dest,sptr->cond) )
	    continue;

	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(dest,sptr->mid,0,0,&old_item);
	if (!old_item)
	{
	    FreeItemBMG(dptr);
	    DASSERT( dptr->mid == sptr->mid );

	    CopyAttribBMG(dest,dptr,sptr);
	    dptr->len	= sptr->len;
	    dptr->text	= sptr->text;
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchOverwriteBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src,			// pointer to source bmg
    bool		dup_strings		// true: allocate mem for copied strings
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	source	patch	patch	both	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    const bmg_item_t * sptr = src->item;
    const bmg_item_t * send = sptr + src->item_used;

    bool dirty = false;
    for ( ; sptr < send; sptr++ )
    {
	if ( sptr->cond && !FindItemBMG(dest,sptr->cond) )
	    continue;

	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(dest,sptr->mid,0,0,&old_item);
	if ( !old_item || !IsItemEqualBMG(dptr,sptr) )
	{
	    if (dup_strings)
		AssignItemText16BMG(dptr,sptr->text,sptr->len);
	    else
	    {
		FreeItemBMG(dptr);
		dptr->len   = sptr->len;
		dptr->text  = sptr->text;
	    }
	    DASSERT( dptr->mid == sptr->mid );

	    CopyAttribBMG(dest,dptr,sptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchDeleteBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	source	-	-	-	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    const bmg_item_t * sptr = src->item;
    const bmg_item_t * send = sptr + src->item_used;

    bool dirty = false;
    for ( ; sptr < send; sptr++ )
    {
	bmg_item_t * dptr = FindItemBMG(dest,sptr->mid);
	if (dptr)
	{
	    FreeItemBMG(dptr);
	    DASSERT( dptr->mid == sptr->mid );
	    dptr->text = 0;
	    ResetAttribBMG(dest,dptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchMaskBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	-	-	source	both	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    bmg_item_t * dptr = dest->item;
    bmg_item_t * dend = dptr + dest->item_used;

    bool dirty = false;
    for ( ; dptr < dend; dptr++ )
    {
	bmg_item_t * sptr = FindItemBMG(src,dptr->mid);
	if (!sptr)
	{
	    FreeItemBMG(dptr);
	    dptr->text = 0;
	    ResetAttribBMG(dest,dptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchEqualBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	-	-	-	both	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    bmg_item_t * dptr = dest->item;
    bmg_item_t * dend = dptr + dest->item_used;

    const bmg_item_t * sptr = src->item;
    const bmg_item_t * send = sptr + src->item_used;

    bool dirty = false;
    for ( ; dptr < dend; dptr++ )
    {
	u32 mid = dptr->mid;
	while ( sptr < send && sptr->mid < mid )
	    sptr++;
	if ( sptr == send )
	    break;
	if ( sptr == send || sptr->mid != mid || !IsItemEqualBMG(dptr,sptr) )
	{
	    FreeItemBMG(dptr);
	    dptr->text = 0;
	    ResetAttribBMG(dest,dptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchNotEqualBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	-	-	source	-	//
	//--------------------------------------//

    DASSERT(dest);
    DASSERT(src);
    copy_param(dest,src);

    bmg_item_t * dptr = dest->item;
    bmg_item_t * dend = dptr + dest->item_used;

    const bmg_item_t * sptr = src->item;
    const bmg_item_t * send = sptr + src->item_used;

    bool dirty = false;
    for ( ; dptr < dend; dptr++ )
    {
	u32 mid = dptr->mid;
	while ( sptr < send && sptr->mid < mid )
	    sptr++;
	if ( sptr == send )
	    break;
	if ( sptr != send && sptr->mid == mid && IsItemEqualBMG(dptr,sptr) )
	{
	    FreeItemBMG(dptr);
	    dptr->text = 0;
	    ResetAttribBMG(dest,dptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

static bool generic_helper
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src,			// pointer to source bmg
    uint		dest_id,		// index fo message to create
    uint		src_id,			// relevant source message
    uint		src_beg,		// source message for begining tests
    uint		src_end			// source message for end tests
)
{
    DASSERT(dest);
    DASSERT(src);

    //--- find source messages

    const bmg_item_t *s0 = FindItemBMG(src,src_id);
    if ( !s0 || !s0->text || !s0->len )
	return false;

    const bmg_item_t *s1 = FindItemBMG(src,src_beg);
    if ( !s1 || !s1->text || !s1->len )
	return false;

    const bmg_item_t *s2 = FindItemBMG(src,src_end);
    if ( !s2 || !s2->text || !s2->len )
	return false;


    //--- compare beginning of text

    const u16 *b0 = s0->text;
    const u16 *b1 = s1->text;
    const uint maxlen = s0->len < s1->len ? s0->len : s1->len;
    const u16 *b0end = b0 + maxlen;

    while ( b0 < b0end && *b0 == *b1 )
	b0++, b1++;
    if ( b0 == b0end )
	return false;

    while ( b0 > s0->text && isalnum((int)be16(b0-1)) )
	b0--, b1--;

    PRINT("GENERIC: |%c%c%c| |%c%c%c|\n",
	be16(b0+0) & 0xff, be16(b0+1) & 0xff, be16(b0+2) & 0xff,
	be16(b1+0) & 0xff, be16(b1+1) & 0xff, be16(b1+2) & 0xff );


    //--- compare end of text

    const u16 *e0 = s0->text + s0->len - 1;
    const u16 *e2 = s2->text + s2->len - 1;
    while ( e0 > b0 && *e0 == *e2 )
	e0--, e2--;
    e0++, e2++;

    const u16 *e0end = s0->text + s0->len;
    while ( b0 < e0end && isalnum((int)be16(b0-1)) )
	e0++, e2++;

    if ( b0 < e0 )
    {
	const uint len = e0 - b0;
	bmg_item_t *dptr = InsertItemBMG(dest,dest_id,0,0,0);
	DASSERT(dptr);
	if ( !dptr->text
	    || dptr->len != len
	    || memcmp(dptr->text,b0,len*sizeof(*b0)) )
	{
	    AssignItemText16BMG(dptr,b0,len);
	    return true;
	}
    }

    return false;
}

//-----------------------------------------------------------------------------

bool PatchGenericBMG
(
    bmg_t		* dest,			// pointer to destination bmg
    const bmg_t		* src			// pointer to source bmg
)
{
    DASSERT(dest);
    DASSERT(src);
    bool dirty = false;

    dirty |= generic_helper(dest,src, MID_G_SMALL,  0x1f44, 0x1f46, 0x1f46 );
    dirty |= generic_helper(dest,src, MID_G_MEDIUM, 0x1f45, 0x1f46, 0x1f46 );
    dirty |= generic_helper(dest,src, MID_G_LARGE,  0x1f46, 0x1f44, 0x1f44 );

    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchIdBMG
(
    bmg_t		* bmg_dest,		// pointer to destination bmg
    bool		prefix_all		// true: prefix all messages
)
{
	//--------------------------------------//
	//	only	only	source	source	//
	//	in	in	!=	==	//
	//	source	patch	patch	patch	//
	//--------------------------------------//
	//	prefix	n.a.	n.a.	n.a.	//
	//--------------------------------------//


    uint name_len;
    ccp name = GetNameBMG(bmg_dest,&name_len);
    noPRINT("\e[31;1m#### NAME=%s\e[0m\n",name);
    if ( name_len > 4 && !strncasecmp(name,"menu",4) )
	name += 4, name_len -= 4;

    u16 buf[10000];
    u16 *bufend = buf + sizeof(buf)/sizeof(*buf);

    bool dirty = false;
    bmg_item_t * dptr = bmg_dest->item;
    bmg_item_t * dend = dptr + bmg_dest->item_used;

    for ( ; dptr < dend; dptr++ )
    {
	if ( dptr->text && ( prefix_all || dptr->text != bmg_null_entry ) )
	{
	    u16 *dest = buf;
	    if (name_len)
		*dest++ = htons(*name);
	    char num[20];
	    snprintf(num,sizeof(num),"%x",dptr->mid);
	    ccp src = num;
	    while (*src)
		*dest++ = htons(*src++);

	    if ( dptr->text != bmg_null_entry )
	    {
		*dest++ = htons(':');

		uint count = bufend - dest;
		if ( count > dptr->len )
		    count = dptr->len;
		memcpy(dest,dptr->text,count*sizeof(*dest));
		dest += count;
	    }
	    AssignItemText16BMG(dptr,buf,dest-buf);
	    dirty = true;
	}
    }

    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchRmEscapesBMG
(
    bmg_t		* bmg,		// pointer to destination bmg
    bool		unicode,	// replace '\u{16bit}' by unicode chars
    bool		rm_escapes	// remove 1A escapes
)
{
    DASSERT(bmg);
    PRINT("patch_bmg_rm_escapes(unicode=%d,rm_esc=%d)\n",unicode,rm_escapes);

    bool dirty = false;
    bmg_item_t * iptr = bmg->item;
    bmg_item_t * iend = iptr + bmg->item_used;

    for ( ; iptr < iend; iptr++ )
    {
	// fast find first escape
	u16 *sp = iptr->text, *end = sp + iptr->len;
	if (!sp)
	    continue;
	while ( sp < end && ntohs(*sp) != 0x1a )
	    sp++;
	if ( sp == end )
	    continue;

	sp = ModifiyItem(iptr,sp);
	end = iptr->text + iptr->len;
	DASSERT( iptr->alloced_size > 0 );

	u16 *dp = sp;
	while ( sp < end )
	{
	    if ( ntohs(*sp) != 0x1a )
	    {
		*dp++ = *sp++;
		continue;
	    }

	    if ( unicode && ntohs(sp[1]) == 0x0801 && !ntohs(sp[2]) )
	    {
		const u16 code = ntohs(sp[3]);
		if ( code >= ' ' )
		{
		    *dp++ = sp[3];
		    sp += 4;
		    HexDump16(stdout,0,0x100,iptr->text,(dp-iptr->text)*2);
		    HexDump16(stdout,0,0x200,sp,(iptr->text+iptr->len-sp)*2);
		    continue;
		}
	    }

	    const uint esc_len = ((u8*)sp)[2] & 0xfe;
	    if (!rm_escapes)
	    {
		memmove(dp,sp,esc_len);
		dp += esc_len/2;
	    }
	    sp += esc_len/2;
	}
	iptr->len = dp - iptr->text;
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchRmCupsBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    DASSERT(bmg);
    bool dirty = false;
    uint mid;
    for ( mid = MID_RCUP_BEG; mid < MID_RCUP_END; mid++ )
    {
	bmg_item_t * ptr = FindItemBMG(bmg,mid);
	if (ptr)
	{
	    FreeItemBMG(ptr);
	    DASSERT( ptr->mid == mid );
	    ptr->text = 0;
	    ResetAttribBMG(bmg,ptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static uint bmg_copy_helper
(
    bmg_t		* bmg,		// pointer to bmg
    bool		force,		// true: force replace
    uint		mid_src_1,	// first source message id
    uint		mid_src_2,	// NULL or second source message id
    uint		mid_src_3,	// NULL or third source message id
    uint		mid_dest,	// destination message id
    uint		n_mid		// numbers of messages to procced
)
{
    DASSERT(bmg);

    uint i, count = 0;
    for ( i = 0; i < n_mid; i++ )
    {
	bmg_item_t * sptr = FindItemBMG(bmg,mid_src_1+i);
	if ( !sptr && mid_src_2 )
	{
	    sptr = FindItemBMG(bmg,mid_src_2+i);
	    if ( !sptr && mid_src_3 )
		sptr = FindItemBMG(bmg,mid_src_3+i);
	}

	if (sptr)
	{
	    bool old_item;
	    bmg_item_t * dptr = InsertItemBMG(bmg,mid_dest+i,0,0,&old_item);
	    if ( force || !old_item || !dptr->len )
	    {
		AssignItemText16BMG(dptr,sptr->text,sptr->len);
		CopyAttribBMG(bmg,dptr,sptr);
		count++;
	    }
	}
    }

    return count;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchCTCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    uint dirty_count
	= bmg_copy_helper( bmg, false, MID_TRACK1_BEG, MID_TRACK2_BEG, 0,
				MID_CT_TRACK_BEG, BMG_N_TRACK )
	+ bmg_copy_helper( bmg, false, MID_ARENA1_BEG, MID_ARENA2_BEG, 0,
				MID_CT_ARENA_BEG, BMG_N_ARENA )
	+ bmg_copy_helper( bmg, false, MID_RCUP_BEG, 0, 0,
				MID_CT_CUP_BEG,
				MID_RCUP_END - MID_RCUP_BEG )
	+ bmg_copy_helper( bmg, false, MID_BCUP_BEG, 0, 0,
				MID_CT_BCUP_BEG,
				MID_BCUP_END - MID_BCUP_BEG );
    return dirty_count > 0;
}

//-----------------------------------------------------------------------------

bool PatchLECopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    uint dirty_count
	= bmg_copy_helper( bmg, false, MID_ARENA1_BEG, MID_ARENA2_BEG, MID_CT_ARENA_BEG,
				MID_LE_ARENA_BEG, BMG_N_ARENA )
	+ bmg_copy_helper( bmg, false, MID_TRACK1_BEG, MID_TRACK2_BEG, MID_CT_TRACK_BEG,
				MID_LE_TRACK_BEG, BMG_N_TRACK )
	+ bmg_copy_helper( bmg, false, MID_RCUP_BEG, MID_CT_CUP_BEG, 0,
				MID_LE_CUP_BEG,
				MID_RCUP_END - MID_RCUP_BEG )
	+ bmg_copy_helper( bmg, false, MID_BCUP_BEG, MID_CT_BCUP_BEG, 0,
				MID_LE_BCUP_BEG,
				MID_BCUP_END - MID_BCUP_BEG );
    return dirty_count > 0;
}

//-----------------------------------------------------------------------------

bool PatchXCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    if (opt_bmg_support_lecode)
	return PatchLECopyBMG(bmg);
    if (opt_bmg_support_ctcode)
	return PatchCTCopyBMG(bmg);
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchCTForceCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    uint dirty_count
	= bmg_copy_helper( bmg, true, MID_TRACK1_BEG, MID_TRACK2_BEG, 0,
				MID_CT_TRACK_BEG, BMG_N_TRACK )
	+ bmg_copy_helper( bmg, true, MID_ARENA1_BEG, MID_ARENA2_BEG, 0,
				MID_CT_ARENA_BEG, BMG_N_ARENA )
	+ bmg_copy_helper( bmg, true, MID_RCUP_BEG, 0, 0,
				MID_CT_CUP_BEG,
				MID_RCUP_END - MID_RCUP_BEG )
	+ bmg_copy_helper( bmg, true, MID_BCUP_BEG, 0, 0,
				MID_CT_BCUP_BEG,
				MID_BCUP_END - MID_BCUP_BEG );
    return dirty_count > 0;
}

//-----------------------------------------------------------------------------

bool PatchLEForceCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    uint dirty_count
	= bmg_copy_helper( bmg, true, MID_TRACK1_BEG, MID_TRACK2_BEG, MID_CT_TRACK_BEG,
				MID_LE_TRACK_BEG, BMG_N_TRACK )
	+ bmg_copy_helper( bmg, true, MID_ARENA1_BEG, MID_ARENA2_BEG, MID_CT_ARENA_BEG,
				MID_LE_ARENA_BEG, BMG_N_ARENA )
	+ bmg_copy_helper( bmg, true, MID_RCUP_BEG, MID_CT_CUP_BEG, 0,
				MID_LE_CUP_BEG,
				MID_RCUP_END - MID_RCUP_BEG )
	+ bmg_copy_helper( bmg, true, MID_BCUP_BEG, MID_CT_BCUP_BEG, 0,
				MID_LE_BCUP_BEG,
				MID_BCUP_END - MID_BCUP_BEG );
    return dirty_count > 0;
}

//-----------------------------------------------------------------------------

bool PatchXForceCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    if (opt_bmg_support_lecode)
	return PatchLEForceCopyBMG(bmg);
    if (opt_bmg_support_ctcode)
	return PatchCTForceCopyBMG(bmg);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool PatchIdentificationBMG ( bmg_t *bmg, ct_bmg_t *ctb )
{
    DASSERT(bmg);
    DASSERT(ctb);

    bmg_item_t *bi = InsertItemBMG(bmg,MID_PARAM_IDENTIFY,0,0,0);
    DASSERT(bi);
    AssignItemTextBMG(bi,GetCtBMGIdentification(ctb,true),-1);
    return true;
}

//-----------------------------------------------------------------------------

bool PatchFillBMG
	( bmg_t *bmg, ct_mode_t ct_mode, uint rcup_limit, uint track_limit )
{
    ct_bmg_t ctb;
    ct_mode = SetupCtBMG(&ctb,ct_mode,ct_mode);
    if (!ctb.is_ct_code)
	return false;


    //--- check existence of at least one relevvant message

    bool found = false;
    bmg_item_t *dptr = bmg->item;
    bmg_item_t *dend = dptr + bmg->item_used;
    for ( ; dptr < dend; dptr++ )
    {
	const u32 m = dptr->mid;
	if (   m >= MID_TRACK1_BEG		&& m < MID_TRACK1_END
	    || m >= MID_TRACK2_BEG		&& m < MID_TRACK2_END
	    || m >= MID_ARENA1_BEG		&& m < MID_ARENA1_END
	    || m >= MID_ARENA2_BEG		&& m < MID_ARENA2_END
	    || m >= MID_RCUP_BEG		&& m < MID_RCUP_END
	    || m >= MID_BCUP_BEG		&& m < MID_BCUP_END
	    || m >= ctb.track_name1.beg		&& m < ctb.track_name1.end
	    || m >= ctb.track_name2.beg		&& m < ctb.track_name2.end
	    || m >= ctb.arena_name1.beg		&& m < ctb.arena_name1.end
	    || m >= ctb.arena_name2.beg		&& m < ctb.arena_name2.end
	    || m >= ctb.rcup_name.beg		&& m < ctb.rcup_name.end
	    || m >= ctb.bcup_name.beg		&& m < ctb.bcup_name.end
	    || m == ctb.random.beg
	   )
	{
	    if ( dptr->text && dptr->text != bmg_null_entry )
	    {
		found = true;
		break;
	    }
	}
    }

    if (!found)
	return false;


    //--- random

    char buf[100];
    bool dirty = false;

    if (ctb.random.beg)
    {
	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(bmg,ctb.random.beg,0,0,&old_item);
	if ( !old_item || !dptr->len )
	{
	    AssignItemTextBMG(dptr,"???",3);
	    ResetAttribBMG(bmg,dptr);
	    dirty = true;
	}
    }


    //--- fill arena names

    uint mid;
    for ( mid = ctb.arena_name1.beg; mid < ctb.arena_name1.end; mid++ )
    {
	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(bmg,mid,0,0,&old_item);
	if ( !old_item || !dptr->len )
	{
	    const uint len
		= snprintf(buf,sizeof(buf),"_A%u_",mid-ctb.arena_name1.beg);
	    AssignItemTextBMG(dptr,buf,len);
	    ResetAttribBMG(bmg,dptr);
	    dirty = true;
	}
    }


    //--- fill track names

    if (!track_limit)
	track_limit = opt_bmg_track_fill_limit;
    uint max_track;
    if (track_limit)
    {
	max_track = ctb.track_name1.beg + track_limit;
	if ( max_track > ctb.track_name1.end )
	    max_track = ctb.track_name1.end;
    }
    else
	max_track = ctb.track_name1.end;

    for ( mid = ctb.track_name1.beg; mid < max_track; mid++ )
    {
	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(bmg,mid,0,0,&old_item);
	if ( !old_item || !dptr->len )
	{
	    const uint len
		= snprintf(buf,sizeof(buf),"_T%03X_",mid-ctb.track_name1.beg);
	    AssignItemTextBMG(dptr,buf,len);
	    ResetAttribBMG(bmg,dptr);
	    dirty = true;
	}
    }


    //--- fill battle cup names

    for ( mid = ctb.bcup_name.beg; mid < ctb.bcup_name.end; mid++ )
    {
	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(bmg,mid,0,0,&old_item);
	if ( !old_item || !dptr->len )
	{
	    const uint len
		= snprintf(buf,sizeof(buf),"_B%u_",mid-ctb.bcup_name.beg);
	    AssignItemTextBMG(dptr,buf,len);
	    ResetAttribBMG(bmg,dptr);
	    dirty = true;
	}
    }


    //--- fill racing cup names

    if (!rcup_limit)
	rcup_limit = opt_bmg_rcup_fill_limit;
    uint max_cup;
    if (rcup_limit)
    {
	max_cup = ctb.rcup_name.beg + rcup_limit;
	if ( max_cup > ctb.rcup_name.end )
	    max_cup = ctb.rcup_name.end;
    }
    else
	max_cup = ctb.rcup_name.end;

    for ( mid = ctb.rcup_name.beg; mid < max_cup; mid++ )
    {
	bool old_item;
	bmg_item_t * dptr = InsertItemBMG(bmg,mid,0,0,&old_item);
	if ( !old_item || !dptr->len )
	{
	    const uint len
		= snprintf(buf,sizeof(buf),"_R%u_",mid-ctb.rcup_name.beg);
	    AssignItemTextBMG(dptr,buf,len);
	    ResetAttribBMG(bmg,dptr);
	    dirty = true;
	}
    }

    //--- identification

    if (dirty)
	PatchIdentificationBMG(bmg,&ctb);

    return dirty;
}

//-----------------------------------------------------------------------------

bool PatchXFillBMG ( bmg_t *bmg, uint rcup_limit, uint track_limit )
{
    if (opt_bmg_support_lecode)
	return PatchLEFillBMG(bmg,rcup_limit,track_limit);

    if (opt_bmg_support_ctcode)
	return PatchCTFillBMG(bmg,rcup_limit,track_limit);

    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchRmFilledBMG ( bmg_t *bmg )
{
    DASSERT(bmg);

    bool dirty = false;
    bmg_item_t * iptr = bmg->item;
    bmg_item_t * iend = iptr + bmg->item_used;

    for ( ; iptr < iend; iptr++ )
    {
	if (   iptr->mid >= 0x4000
	    && iptr->len > 3
	    && ntohs(iptr->text[0]) == '_'
	    && ntohs(iptr->text[iptr->len-1]) == '_' )
	{
	    u16 ch = ntohs(iptr->text[1]);
	    if ( ch == 'A' ||  ch == 'T' ||  ch == 'B' ||  ch == 'R' )
	    {
		FreeItemBMG(iptr);
		iptr->text = 0;
		ResetAttribBMG(bmg,iptr);
		dirty = true;
	    }
	}
    }

    return dirty;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PatchRemove*BMG()		///////////////
///////////////////////////////////////////////////////////////////////////////

bool PatchRemoveBMG ( bmg_t * bmg, int mid1, int mid2 )
{
    DASSERT(bmg);

    bmg_item_t * ptr = bmg->item;
    bmg_item_t * end = ptr + bmg->item_used;

    bool dirty = false;
    for ( ; ptr < end && ptr->mid < mid2; ptr++ )
    {
	if ( ptr->mid >= mid1 )
	{
	    FreeItemBMG(ptr);
	    ptr->text = 0;
	    ResetAttribBMG(bmg,ptr);
	    dirty = true;
	}
    }
    return dirty;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    PatchBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError PatchBMG
(
    bmg_t		*dest,			// pointer to destination bmg
    const bmg_t		*src,			// pointer to source bmg
    PatchModeBMG_t	patch_mode,		// patch mode
    ccp			string,			// NULL or string parameter
    bool		dup_strings	// true: alloc mem for copied strings (OVERWRITE)
)
{
    PRINT("PatchBMG(%p,%p,%u,%s,%d)\n",
		dest, src, patch_mode, string, dup_strings );
    DASSERT(dest);

    bool dirty = false;
    switch(patch_mode)
    {
     case BMG_PM_PRINT:		dirty = PatchPrintBMG(dest,src); break;
     case BMG_PM_FORMAT:	dirty = PatchFormatBMG(dest,src,string); break;
     case BMG_PM_REGEX:		dirty = PatchRegexBMG(dest,string); break;
     case BMG_PM_RM_REGEX:	dirty = PatchRmRegexBMG(dest,string); break;
     case BMG_PM_ID:		dirty = PatchIdBMG(dest,false); break;
     case BMG_PM_ID_ALL:	dirty = PatchIdBMG(dest,true); break;
     case BMG_PM_UNICODE:	dirty = PatchRmEscapesBMG(dest,true,false); break;
     case BMG_PM_RM_ESCAPES:	dirty = PatchRmEscapesBMG(dest,false,true); break;

     case BMG_PM_REPLACE:	dirty = PatchReplaceBMG(dest,src); break;
     case BMG_PM_INSERT:	dirty = PatchInsertBMG(dest,src); break;
     case BMG_PM_OVERWRITE:	dirty = PatchOverwriteBMG(dest,src,dup_strings); break;
     case BMG_PM_DELETE:	dirty = PatchDeleteBMG(dest,src); break;
     case BMG_PM_MASK:		dirty = PatchMaskBMG(dest,src); break;
     case BMG_PM_EQUAL:		dirty = PatchEqualBMG(dest,src); break;
     case BMG_PM_NOT_EQUAL:	dirty = PatchNotEqualBMG(dest,src); break;

     case BMG_PM_GENERIC:	dirty = PatchGenericBMG(dest,dest); break;
     case BMG_PM_RM_CUPS:	dirty = PatchRmCupsBMG(dest); break;

     case BMG_PM_CT_COPY:	dirty = PatchCTCopyBMG(dest); break;
     case BMG_PM_CT_FORCE_COPY:	dirty = PatchCTForceCopyBMG(dest); break;
     case BMG_PM_CT_FILL:	dirty = PatchCTFillBMG(dest,0,0); break;

     case BMG_PM_LE_COPY:	dirty = PatchLECopyBMG(dest); break;
     case BMG_PM_LE_FORCE_COPY:	dirty = PatchLEForceCopyBMG(dest); break;
     case BMG_PM_LE_FILL:	dirty = PatchLEFillBMG(dest,0,0); break;

     case BMG_PM_X_COPY:	dirty = PatchXCopyBMG(dest); break;
     case BMG_PM_X_FORCE_COPY:	dirty = PatchXForceCopyBMG(dest); break;
     case BMG_PM_X_FILL:	dirty = PatchXFillBMG(dest,0,0); break;

     case BMG_PM_RM_FILLED:	dirty = PatchRmFilledBMG(dest); break;

     default:			return ERROR0(ERR_NOT_IMPLEMENTED,0);
    }

    if (dirty)
	TouchFileAttrib(&dest->fatt);

    return dirty ? ERR_DIFFER : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CT/LE support			///////////////
///////////////////////////////////////////////////////////////////////////////

ct_mode_t NormalizeCtModeBMG ( ct_mode_t mode, ct_mode_t fallback )
{
    for(;;)
    {
	switch(mode)
	{
	    case CTM_NINTENDO:
	    case CTM_CTCODE:
	    case CTM_LECODE2:
		return mode;

	    case CTM_CTCODE_BASE:
		return CTM_CTCODE;

	    case CTM_LECODE1:
	    case CTM_LECODE_BASE:
		return CTM_LECODE_DEF;

	    case CTM_OPTIONS:
		if (opt_bmg_support_lecode)
		    return CTM_LECODE2;
		if (opt_bmg_support_ctcode)
		    return CTM_CTCODE;
		return CTM_NINTENDO;

	    case CTM_NO_INIT:
	    case CTM_AUTO:
		break;
	}

	mode = fallback;
	fallback = CTM_OPTIONS;
    }
}

///////////////////////////////////////////////////////////////////////////////

ct_mode_t SetupCtModeBMG ( ct_mode_t mode )
{
    mode = NormalizeCtModeBMG(mode,CTM_NINTENDO);
    switch (mode)
    {
     case CTM_NINTENDO:
	opt_bmg_support_mkw	= true;
	opt_bmg_support_ctcode	= false;
	opt_bmg_support_lecode	= false;
	break;

     case CTM_CTCODE:
	opt_bmg_support_mkw	= true;
	opt_bmg_support_ctcode	= true;
	opt_bmg_support_lecode	= false;
	break;

     case CTM_LECODE2:
	opt_bmg_support_mkw	= true;
	opt_bmg_support_ctcode	= false;
	opt_bmg_support_lecode	= true;
	break;

     default:
	opt_bmg_support_mkw	= false;
	opt_bmg_support_ctcode	= false;
	opt_bmg_support_lecode	= false;
	break;
    }
    return mode;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetCtModeNameBMG ( ct_mode_t mode, bool full )
{
    switch(mode)
    {
	case CTM_NINTENDO:	return "Nintendo";
	case CTM_CTCODE:	return "CT-CODE";
	case CTM_LECODE1:	return full ? "LE-CODE v1" : "LE-CODE";
	case CTM_LECODE2:	return full ? "LE-CODE v2" : "LE-CODE";
	default:		return "?";
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ct_mode_t SetupCtBMG ( ct_bmg_t *ctb, ct_mode_t mode, ct_mode_t fallback )
{
    mode = NormalizeCtModeBMG(mode,fallback);
    if (ctb)
    {
	ctb->ct_mode	= mode;
	ctb->identify	= MID_PARAM_IDENTIFY;
	ctb->param.beg	= MID_PARAM_BEG;
	ctb->param.end	= MID_PARAM_END;
	ctb->param.n	= ctb->param.end - ctb->param.beg;

	switch(mode)
	{
	    case CTM_CTCODE:
		ctb->version		= 0;
		ctb->is_ct_code		= 1;
		ctb->le_phase		= 0;

		ctb->rcup_name.beg	= MID_CT_RCUP_BEG;
		ctb->rcup_name.end	= MID_CT_RCUP_END;
		ctb->bcup_name.beg	= MID_CT_BCUP_BEG;
		ctb->bcup_name.end	= MID_CT_BCUP_END;

		ctb->track_name1.beg	= MID_CT_TRACK_BEG;
		ctb->track_name1.end	= MID_CT_TRACK_END;
		ctb->track_name2.beg	= 0;
		ctb->track_name2.end	= 0;

		ctb->arena_name1.beg	= MID_CT_ARENA_BEG;
		ctb->arena_name1.end	= MID_CT_ARENA_END;
		ctb->arena_name2.beg	= 0;
		ctb->arena_name2.end	= 0;

		ctb->cup_ref.beg	= MID_CT_CUP_REF_BEG;
		ctb->cup_ref.end	= MID_CT_CUP_REF_END;

		ctb->random.beg		= MID_CT_RANDOM;
		ctb->random.end		= MID_CT_RANDOM+1;
		break;

	    case CTM_LECODE1:
	    case CTM_LECODE2:
		ctb->version		= mode == CTM_LECODE2 ? 2 : 1;
		ctb->le_phase		= ctb->version;
		ctb->is_ct_code		= 2;

		ctb->rcup_name.beg	= MID_LE_RCUP_BEG;
		ctb->rcup_name.end	= MID_LE_RCUP_END;
		ctb->bcup_name.beg	= MID_LE_BCUP_BEG;
		ctb->bcup_name.end	= MID_LE_BCUP_END;

		ctb->track_name1.beg	= MID_LE_TRACK_BEG;
		ctb->track_name1.end	= MID_LE_TRACK_END;
		ctb->track_name2.beg	= 0;
		ctb->track_name2.end	= 0;

		ctb->arena_name1.beg	= MID_LE_ARENA_BEG;
		ctb->arena_name1.end	= MID_LE_ARENA_END;
		ctb->arena_name2.beg	= 0;
		ctb->arena_name2.end	= 0;

		ctb->cup_ref.beg	= MID_LE_CUP_REF_BEG;
		ctb->cup_ref.end	= MID_LE_CUP_REF_END;

		ctb->random.beg		= 0;
		ctb->random.end		= 0;
		break;

	    default:
		ctb->version		= 0;
		ctb->is_ct_code		= 0;
		ctb->le_phase		= 0;

		ctb->rcup_name.beg	= MID_RCUP_BEG;
		ctb->rcup_name.end	= MID_RCUP_END;
		ctb->bcup_name.beg	= MID_BCUP_BEG;
		ctb->bcup_name.end	= MID_BCUP_END;

		ctb->track_name1.beg	= MID_TRACK1_BEG;
		ctb->track_name1.end	= MID_TRACK1_END;
		ctb->track_name2.beg	= MID_TRACK2_BEG;
		ctb->track_name2.end	= MID_TRACK2_END;

		ctb->arena_name1.beg	= MID_ARENA1_BEG;
		ctb->arena_name1.end	= MID_ARENA1_END;
		ctb->arena_name2.beg	= MID_ARENA2_BEG;
		ctb->arena_name2.end	= MID_ARENA2_END;

		ctb->cup_ref.beg	= 0;
		ctb->cup_ref.end	= 0;

		ctb->random.beg		= MID_RANDOM;
		ctb->random.end		= MID_RANDOM+1;
		break;
	}

	u32 min = M1(u32), max = 0;
	bmg_range_t *ptr;
	for ( ptr = &ctb->rcup_name; ptr <= &ctb->random; ptr++ )
	{
	    ptr->n = ptr->end - ptr->beg;
	    if (ptr->n)
	    {
		if ( min > ptr->beg ) min = ptr->beg;
		if ( max < ptr->end ) max = ptr->end;
	    }
	}
	ctb->range.beg = min;
	ctb->range.end = max;
	ctb->range.n   = max - min;
    }
    return mode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#undef TAB
#define TAB(nam) ctb->nam.beg, ctb->nam.n

ccp GetCtBMGIdentification ( ct_bmg_t *ctb, bool full )
{
    DASSERT(ctb);

    ccp name = GetCtModeNameBMG(ctb->ct_mode,false);
    char buf[200];
    int len;
    if (full)
	len = snprintf(buf,sizeof(buf),
		"%x, %s, v=%x, ct=%x, le=%x, par=%x+%x,"
		" rcup=%x+%x, bcup=%x+%x, t1=%x+%x, t2=%x+%x,"
		" a1=%x+%x, a2=%x+%x, ref=%x+%x",
		ctb->ct_mode, name, ctb->version,
		ctb->is_ct_code==1, ctb->le_phase,
		TAB(param),
		TAB(rcup_name), TAB(bcup_name),
		TAB(track_name1), TAB(track_name2),
		TAB(arena_name1), TAB(arena_name2),
		TAB(cup_ref) );
    else
	len = snprintf(buf,sizeof(buf),"%x, %s v%d",ctb->ct_mode,name,ctb->version);
    return len > 0 ? CopyCircBuf(buf,len+1) : EmptyString;
}

#undef TAB

///////////////////////////////////////////////////////////////////////////////

static void DumpCtMidHelper1 ( FILE *f, int indent, u32 id, ccp info )
{
    DASSERT(f);
    if (id)
	fprintf(f,"%*s%-20s %04x\n", indent, "", info, id );
}

//-----------------------------------------------------------------------------

static void DumpCtMidHelper2 ( FILE *f, int indent, bmg_range_t *id, ccp info )
{
    DASSERT(f);
    DASSERT(id);
    if (id->beg)
	fprintf(f,"%*s%-20s %04x .. %04x, %5u = 0x%04x\n",
		indent, "", info, id->beg, id->end, id->n, id->n );
}

//-----------------------------------------------------------------------------

void DumpCtBMG ( FILE *f, int indent, ct_bmg_t *ctb )
{
    DASSERT(f);
    DASSERT(ctb);
    indent = NormalizeIndent(indent);

 #if defined(TEST) || defined(DEBUG)
    fprintf(f,"%*s%s\n",indent,"",GetCtBMGIdentification(ctb,true));
 #else
    fprintf(f,"%*s%s\n",indent,"",GetCtBMGIdentification(ctb,false));
 #endif

    indent += 2;
    DumpCtMidHelper1( f,indent, ctb->identify,		"Identification:" );
    DumpCtMidHelper2( f,indent, &ctb->param,		"Parameters:" );
    DumpCtMidHelper2( f,indent, &ctb->range,		"Data range:" );

    DumpCtMidHelper2( f,indent, &ctb->rcup_name,	"  Racing cup names:" );
    DumpCtMidHelper2( f,indent, &ctb->bcup_name,	"  Battle cup names:" );
    DumpCtMidHelper2( f,indent, &ctb->track_name1,	"  Track names 1:" );
    DumpCtMidHelper2( f,indent, &ctb->track_name2,	"  Track names 2:" );
    DumpCtMidHelper2( f,indent, &ctb->arena_name1,	"  Battle names 1:" );
    DumpCtMidHelper2( f,indent, &ctb->arena_name2,	"  Battle names 2:" );
    DumpCtMidHelper2( f,indent, &ctb->cup_ref,		"  Cup reference:" );
    DumpCtMidHelper1( f,indent, ctb->random.beg,	"  Random text:" );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sizeof_info_t: bmg		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[sizeof_info_bmg]]

const sizeof_info_t sizeof_info_bmg[] =
{
    SIZEOF_INFO_TITLE("BMG")
	SIZEOF_INFO_ENTRY(bmg_range_t)
	SIZEOF_INFO_ENTRY(bmg_section_t)
	SIZEOF_INFO_ENTRY(bmg_header_t)
	SIZEOF_INFO_ENTRY(bmg_inf_item_t)
	SIZEOF_INFO_ENTRY(bmg_inf_t)
	SIZEOF_INFO_ENTRY(bmg_dat_t)
	SIZEOF_INFO_ENTRY(bmg_mid_t)
	SIZEOF_INFO_ENTRY(bmg_flw_t)
	SIZEOF_INFO_ENTRY(bmg_fli_t)
	SIZEOF_INFO_ENTRY(bmg_sect_info_t)
	SIZEOF_INFO_ENTRY(bmg_sect_list_t)
	SIZEOF_INFO_ENTRY(bmg_raw_section_t)
	SIZEOF_INFO_ENTRY(bmg_item_t)
	SIZEOF_INFO_ENTRY(bmg_t)
	SIZEOF_INFO_ENTRY(bmg_create_t)
	SIZEOF_INFO_ENTRY(bmg_scan_mid_t)
	SIZEOF_INFO_ENTRY(ct_bmg_t)

    SIZEOF_INFO_TERM()
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

