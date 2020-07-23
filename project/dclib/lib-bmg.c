
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

#define _GNU_SOURCE 1
#include <errno.h>

#include "lib-bmg.h"
#include "dclib-utf8.h"

//#include "lib-xbmg.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    global bmg options			///////////////
///////////////////////////////////////////////////////////////////////////////

uint		opt_bmg_align		= 0x20;	// alignment of raw-bg sections
bool		opt_bmg_export		= 0;	// true: optimize for exports
bool		opt_bmg_no_attrib	= 0;	// true: suppress attributes
bool		opt_bmg_x_escapes	= 0;	// true: use x-scapes insetad of z-escapes
bool		opt_bmg_old_escapes	= 0;	// true: use only old escapes
int		opt_bmg_single_line	= 0;	// >0: single line, >1: suppress atrributes
bool		opt_bmg_support_mkw	= 0;	// true: support MKW specific extensions
bool		opt_bmg_support_ctcode	= 0;	// true: support MKW/CTCODE specific extensions
bool		opt_bmg_inline_attrib	= true;	// true: print attrinbutes inline
int		opt_bmg_colors		= 1;	// use c-escapss: 0:off, 1:auto, 2:on
ColNameLevelBMG	opt_bmg_color_name	= BMG_CNL_ALL;
uint		opt_bmg_max_recurse	= 10;	// max recurse depth
bool		opt_bmg_allow_print	= 0;	// true: allow '$' to print a log message

//-----------------------------------------------------------------------------

uint	opt_bmg_inf_size = 0;
bool	opt_bmg_force_attrib = false;
u8	bmg_force_attrib[BMG_ATTRIB_SIZE];
bool	opt_bmg_def_attrib = false;
u8	bmg_def_attrib[BMG_ATTRIB_SIZE];

//-----------------------------------------------------------------------------

const KeywordTab_t PatchKeysBMG[] =
{
	// 'opt'
	//  0: no param
	//  1: file param
	//  2: string param

	{ BMG_PM_PRINT,		"PRINT",	0,		1 },
	{ BMG_PM_FORMAT,	"FORMAT",	0,		2 },
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
	{ 0,0,0,0 }
};

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
    return code != 0x1a ? 1 : ( ((u8*)source)[2] & 0xfe ) / 2;
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
	    const uint len = *(u8*)src & 0xfe;
	    if ( len > 0 )
		src += len - 1;
	}
    }

    const uint len = src - source;
    return len < max_len ? len : max_len;
}

///////////////////////////////////////////////////////////////////////////////

uint PrintString16BMG
(
    char		* buf,		// destination buffer
    uint		buf_size,	// size of 'buf'
    const u16		* src,		// source
    int			src_len,	// length of source in u16 words, -1:NULL term
    u16			utf8_max,	// max code printed as UTF-8
    uint		quote,		// 0:no quotes, 1:escape ", 2: print in quotes
    int			use_color	// 0:no, 1:use, 2:use all
)
{
    DASSERT(buf);
    DASSERT( buf_size >= 100 );
    DASSERT(src);

    if ( buf_size < 30 )
    {
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
	    uint total_words = *(u8*)src >> 1;
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
    return dest - buf;
}

///////////////////////////////////////////////////////////////////////////////

uint ScanString16BMG
(
    u16			* buf,		// destination buffer
    uint		buf_size,	// size of 'buf' in u16 words
    ccp			src,		// UTF-8 source
    int			src_len		// length of source, -1: determine with strlen()
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

		case 'x':
		    {
			const bool have_brace = *src == '{';
			if (have_brace)
			    src++;
			for(;;)		// >>>>>  comma list support since v1.44  <<<<<
			{
			    char * end;
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
			const u16 code1 = strtoul(src,&end,16) & 0xfeff;
			src = end;
			uint total_words = code1 >> 9;
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
///////////////			struct bmg_item_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// special value for bmg_item_t::text
u16 bmg_null_entry[] = {0};

///////////////////////////////////////////////////////////////////////////////

void FreeItemBMG( bmg_item_t * bi )
{
    DASSERT(bi);
    if (bi->text)
    {
	if ( bi->text != bmg_null_entry && bi->alloced_size )
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
    if ( !text16 || text16 == bmg_null_entry )
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
	len = ScanString16BMG(buf,bufsize,utf8,len);
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

    memcpy(item->attrib,bmg->attrib,sizeof(item->attrib));
    if ( attrib && attrib_used )
    {
	item->attrib_used = bmg->attrib_used;
	memcpy(item->attrib,attrib,attrib_used);
    }
    return item;
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
///////////////			setup/reset bmg_t		///////////////
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

enumError ScanRawBMG ( bmg_t * bmg )
{
    DASSERT(bmg);
    TRACE("SCAN BMG/RAW: %s\n",bmg->fname);

    const bmg_header_t * bh = (bmg_header_t*)bmg->data;
    DASSERT(!memcmp(bh->magic,BMG_MAGIC,sizeof(bh->magic)));
    const u32 data_size = ntohl(bh->size);
    DASSERT( data_size <= bmg->data_size);
    bmg->is_text_src = false;

    bmg_inf_t * pinf = 0;
    bmg_dat_t * pdat = 0;
    bmg_mid_t * pmid = 0;

    const u8 * data_end = bmg->data + data_size;
    const bmg_section_t *sect = (bmg_section_t*)(bh+1);
    for(;;)
    {
	const u32 sect_size = ntohl(sect->size);
	noPRINT(" |%.4s| sect=%zx+%x=%zx/%x/%x\n",
		sect->magic,
		(u8*)sect - bmg->data, sect_size, (u8*)sect - bmg->data + sect_size,
		data_size, bmg->data_size );

	if ( (u8*)sect >= data_end || (u8*)sect + sect_size > data_end )
	{
	    const u8 * bmg_end = bmg->data + bmg->data_size;
	    if ( data_end < bmg_end && ( !pinf || !pdat || !pmid ))
	    {
		ERROR0(ERR_WARNING,
			"BMG internal size too small, use file size instead: %s\n",
			bmg->fname );
		data_end = bmg_end;
		continue;
	    }
	    break;
	}

	if      (!memcmp(sect->magic,BMG_INF_MAGIC,sizeof(sect->magic)))
	    pinf = (bmg_inf_t*)sect;
	else if (!memcmp(sect->magic,BMG_DAT_MAGIC,sizeof(sect->magic)))
	    pdat = (bmg_dat_t*)sect;
	else if (!memcmp(sect->magic,BMG_MID_MAGIC,sizeof(sect->magic)))
	    pmid = (bmg_mid_t*)sect;

	sect = (bmg_section_t*)( (u8*)sect + sect_size );
    }

    bmg->inf = pinf;
    bmg->dat = pdat;
    bmg->mid = pmid;

    if ( !pinf || !pdat || !pmid )
	return ERROR0(ERR_INVALID_DATA,"Corrupted BMG file: %s\n",bmg->fname);

    const uint max_offset = ( ntohl(pdat->size) - sizeof(*pdat) )
			  / sizeof(*pdat->text_pool);
    u8 *text_end = pdat->text_pool + max_offset;

    uint max_item = ( ntohl(pmid->size) - sizeof(*pmid) ) / sizeof(*pmid->mid);
    if ( max_item > ntohs(pmid->n_msg) )
	 max_item = ntohs(pmid->n_msg);

    u8 *raw_inf = (u8*)pinf->list;
    const uint src_inf_size = ntohs(pinf->inf_size);
    AssignInfSizeBMG(bmg,src_inf_size);


    //--- find default attributes

    if (!opt_bmg_def_attrib)
    {
	uint idx;
	for ( idx = 0; idx < bmg->attrib_used; idx += 4 )
	{
	    const u32 major = FindMajority32(raw_inf+idx+4,max_item,src_inf_size);
	    noPRINT("-> IDX: %2u -> %8x\n",idx,ntohl(major));
	    memcpy(bmg->attrib+idx,&major,4);
	}
	//HexDump16(stderr,8,0x10,bmg->attrib,bmg->attrib_used);
    }


    //--- decode raw files

    uint idx;
    for ( idx = 0; idx < max_item; idx++, raw_inf += src_inf_size )
    {
	u32 mid = ntohl(pmid->mid[idx]);
	bmg_inf_item_t *item = (bmg_inf_item_t*)raw_inf;

	const u32 offset = ntohl(item->offset);

 #ifdef TEST
	if ( offset >= max_offset )
	    return ERROR0(ERR_INVALID_DATA,
		"Invalid pointer at file offset 0x%zx (INF item #%u): %s\n",
			(u8*)&pinf->list[idx].offset - bmg->data, idx, bmg->fname);
 #else
	if ( offset >= max_offset )
	    return ERROR0(ERR_INVALID_DATA,"Corrupted BMG file: %s\n",bmg->fname);
 #endif

	bmg_item_t * bi = InsertItemBMG(bmg,mid,item->attrib,bmg->attrib_used,0);
	DASSERT(bi);

	if (!offset)
	{
	    AssignItemText16BMG(bi,bmg_null_entry,0);
	    continue;
	}

	u8 *ptr, *start = pdat->text_pool + offset;
	for ( ptr = start; ptr < text_end; ptr += 2 )
	{
	    if (!ptr[0])
	    {
		if (!ptr[1])
		    break;
		if ( ptr[1] == 0x1a )
		{
		    ptr += 2;
		    const uint skip = *ptr & 0x1e;
		    if ( skip > 4 )
			ptr += skip - 4;
		}
	    }
	}

	u16 * text = (u16*)( pdat->text_pool + offset );
	const int text_len = ( ptr - start )/2;
	AssignItemText16BMG( bi, text, text_len );
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			attribute helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

static inline void reset_attrib
(
    const bmg_t		* bmg,		// pointer to bmg
    bmg_item_t		* item		// dest item
)
{
    DASSERT(bmg);
    DASSERT(item);

    item->attrib_used = bmg->attrib_used;
    memcpy(item->attrib,bmg->attrib,sizeof(item->attrib));
}

///////////////////////////////////////////////////////////////////////////////

static void copy_attrib
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

    if ( opt_bmg_force_attrib )
    {
	dptr->attrib_used = dest->attrib_used;
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
	dest->attrib_used	= src->attrib_used;
	dest->use_color_names	= src->use_color_names;
	dest->use_mkw_messages	= src->use_mkw_messages;
	dest->param_defined	= true;
    }
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
		if (opt_bmg_support_ctcode)
		{
		    mid3 = MID_TRACK3_BEG + idx;
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
		if (opt_bmg_support_ctcode)
		{
		    mid3 = MID_ARENA2_BEG + idx;
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

    while ( ptr < end && ( *ptr == ' ' || *ptr == '\t' ) )
	ptr++;
    return (char*)ptr;
}

///////////////////////////////////////////////////////////////////////////////

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
    char xbuf[2*BMG_MSG_BUF_SIZE];
    enumError err_stat = ERR_OK;
    u32 cond = 0;

    for(;;)
    {
	while ( ptr < end && (uchar)*ptr <= ' ' )
	    ptr++;

	u32 mid1 = 0, mid2 = 0, mid3 = 0;
	noPRINT("PTR 1: %p '%s'\n",ptr,PrintID(ptr,10,0));
	const int mid_stat
	    = ScanMidBMG(bmg,&mid1,&mid2,&mid3,ptr,end,(char**)&ptr);
	noPRINT("    2: %p '%s' -> %d : %x %x %x\n",
		ptr,PrintID(ptr,10,0),mid_stat,mid1,mid2,mid3);
	if ( ptr >= end )
	    break;

	ccp start = ptr = GetNext(ptr,end);
	while ( ptr < end && *ptr != '\n' )
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
		start = GetText(xbuf,sizeof(xbuf),start+1,ptr,&len);
		if (len)
		{
		    PRINT("MATCH |%s|\n",xbuf);
		    if (!bmg_name)
		    {
			bmg_name = GetNameBMG(bmg,0);
			PRINT("NAME |%s|\n",bmg_name);
		    }

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
		start = GetText(xbuf,sizeof(xbuf),start+1,ptr,&len);
		if ( len && bmg->recurse_depth < opt_bmg_max_recurse )
		{
		    PRINT("BMG/%s: %s\n",close_current?"GOTO":"INCLUDE",xbuf);
		    bmg_t bmg2;
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
	    if ( *start != '=' || ++start >= ptr )
		continue;

	    noPRINT("PARAM: %s = |%.*s|\n",namebuf,(int)(ptr-start),start);
	    if (!strcmp(namebuf,"INF-SIZE"))
		AssignInfSizeBMG(bmg,str2ul(start,0,10));
	    else if (!strcmp(namebuf,"DEFAULT-ATTRIBS"))
	    {
		noPRINT("--- SCAN ATTRIB: %.*s\n",(int)(ptr-start),start);
		scan_attrib(bmg->attrib,bmg->attrib_used,start);
		//HEXDUMP16(0,0,bmg->attrib,bmg->attrib_used);
	    }
	    continue;
	}

	if ( mid_stat <= 0 ) // not a valid message line
	    continue;

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
	    AssignItemText16BMG(bi,bmg_null_entry,0);
	    if (mid2)
	    {
		bi = InsertItemBMG(bmg,mid2,attrib_buf,attrib_used,0);
		DASSERT(bi);
		AssignItemText16BMG(bi,bmg_null_entry,0);
	    }
	    if (mid3)
	    {
		bi = InsertItemBMG(bmg,mid3,attrib_buf,attrib_used,0);
		DASSERT(bi);
		AssignItemText16BMG(bi,bmg_null_entry,0);
	    }
	    continue;
	}

	if ( *start != '=' ) // not a valid line
	    continue;

	// assume that xbuf is large enough
	//   => make only simple and silent buffer overrun tests

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
	u16 * dest_buf = (u16*)xbuf;
	const uint len16
	    = ScanString16BMG( dest_buf, sizeof(xbuf)/sizeof(u16), in_beg, in_len );

	bmg_item_t * bi = InsertItemBMG(bmg,mid1,attrib_buf,attrib_used,0);
	DASSERT(bi);
	bi->cond = cond;
	AssignItemText16BMG(bi,dest_buf,len16);
	if (mid2)
	{
	    bi = InsertItemBMG(bmg,mid2,attrib_buf,attrib_used,0);
	    DASSERT(bi);
	    bi->cond = cond;
	    AssignItemText16BMG(bi,dest_buf,len16);
	}
	if (mid3)
	{
	    bi = InsertItemBMG(bmg,mid3,attrib_buf,attrib_used,0);
	    DASSERT(bi);
	    bi->cond = cond;
	    AssignItemText16BMG(bi,dest_buf,len16);
	}
    }

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
    if ( !memcmp(bh->magic,BMG_MAGIC,sizeof(bh->magic))
		&& ntohl(bh->size) <= bmg->data_size )
	return ScanRawBMG(bmg);

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
///////////////			CreateRawBMG()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CreateRawBMG
(
    bmg_t		* bmg		// pointer to valid BMG
)
{
    DASSERT(bmg);
    TRACE("CreateRawBMG()\n");


    //--- count messages

    bmg_item_t *bi, *bi_end = bmg->item + bmg->item_used;
    u32 n_msg = 0;
    u32 textpool_size = 1;

    for ( bi = bmg->item; bi < bi_end; bi++ )
    {
	if ( bi->text == bmg_null_entry )
	    n_msg++;
	else if ( bi->text )
	{
	    n_msg++;
	    textpool_size += bi->len + 1;
	}
    }
    textpool_size *= 2;


    //--- calculate sizes

    const u32 inf_size
	= ALIGN32( sizeof(bmg_inf_t) + n_msg * bmg->inf_size, opt_bmg_align );
    const u32 dat_size
	= ALIGN32( sizeof(bmg_dat_t) + textpool_size, opt_bmg_align );
    const u32 mid_size
	= ALIGN32( sizeof(bmg_mid_t) + n_msg * sizeof(u32), opt_bmg_align );

    const u32 total_size = sizeof(bmg_header_t)
			 + inf_size + dat_size + mid_size;

    PRINT("SIZE(%u msg): %zx + %x[/%x] + %x + %x = %x\n",
		n_msg, sizeof(bmg_header_t),
		inf_size,  bmg->inf_size, dat_size, mid_size, total_size );


    //--- alloc data

    FREE(bmg->raw_data);
    bmg->raw_data = CALLOC(total_size,1);
    bmg->raw_data_size = total_size;


    //--- setup pointers

    bmg_header_t* bh	= (bmg_header_t*)bmg->raw_data;
    bmg_inf_t	* pinf	= (bmg_inf_t*)(bh+1);
    bmg_dat_t	* pdat	= (bmg_dat_t*)( (u8*)pinf + inf_size );
    bmg_mid_t	* pmid	= (bmg_mid_t*)( (u8*)pdat + dat_size );


    //---- setup bmg header

    memcpy(bh->magic,BMG_MAGIC,sizeof(bh->magic));
    bh->size		= htonl(total_size);
    bh->n_sections	= htonl(3);
    bh->unknown1	= htonl(0x02000000);


    //---- setup inf header

    memcpy(pinf->magic,BMG_INF_MAGIC,sizeof(pinf->magic));
    pinf->size		= htonl(inf_size);
    pinf->n_msg		= htons(n_msg);
    pinf->inf_size	= htons(bmg->inf_size);


    //---- setup dat header

    memcpy(pdat->magic,BMG_DAT_MAGIC,sizeof(pdat->magic));
    pdat->size		= htonl(dat_size);


    //---- setup mid header

    memcpy(pmid->magic,BMG_MID_MAGIC,sizeof(pmid->magic));
    pmid->size		= htonl(mid_size);
    pmid->n_msg		= htons(n_msg);
    pmid->unknown1	= htons(0x1000);


    //---- main loop

    u32 offset = 2;
    u32 * mid_ptr = pmid->mid;
    bmg_inf_item_t *inf_list = pinf->list;

    for ( bi = bmg->item; bi < bi_end; bi++ )
    {
	if (!bi->text)
	    continue;

	*mid_ptr++ = htonl(bi->mid);
	memcpy(inf_list->attrib,bi->attrib,bmg->attrib_used);

	if ( bi->text != bmg_null_entry )
	{
	    inf_list->offset = htonl(offset);
	    memcpy( pdat->text_pool + offset, bi->text, bi->len * 2 );
	    offset += (bi->len+1) * 2;
	}

	inf_list = (bmg_inf_item_t*)( (u8*)inf_list + bmg->inf_size );
    }

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
	FILEERROR1(&F,ERR_WRITE_FAILED,"Write failed: %s\n",fname);
    return ResetFile(&F,set_time);
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
	|| def_attrib && !memcmp(attrib,def_attrib,BMG_ATTRIB_SIZE) )
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

    if (!memcmp(attrib,all0,BMG_ATTRIB_SIZE))
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
	( File_t *F, bmg_t *bmg, const bmg_item_t *bi, ccp mid_name )
{
    DASSERT(bmg);
    DASSERT(bi);

    if ( bmg->active_cond != bi->cond )
    {
	bmg->active_cond = bi->cond;
	fprintf(F->f,"@? %x\n",bmg->active_cond);
    }

    char buf[100+BMG_ATTRIB_BUF_SIZE];
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
	    fprintf(F->f,"%s\t~ 0x%08x\r\n",mid_name,be32(bi->attrib));
	}
	else
	{
	    // use new attrib vector
	    PrintAttribBMG( buf, sizeof(buf),
				bi->attrib, bi->attrib_used, bmg->attrib, true );
	    fprintf(F->f,"%s\t~ %s\r\n",mid_name,buf);
	}
    }

    if ( print_attrib && opt_bmg_inline_attrib )
    {
	char *dest = buf + snprintfS(buf,sizeof(buf),"%s ",mid_name);
	dest = PrintAttribBMG( dest, buf+sizeof(buf)-dest,
				bi->attrib, bi->attrib_used, bmg->attrib, false );
	*dest++ = ' ';
	*dest = 0;
	DASSERT( dest < buf + sizeof(buf) - 1 );
    }
    else
	snprintf(buf,sizeof(buf),"%s\t",mid_name);

    if ( bi->text == bmg_null_entry )
	fprintf(F->f,"%s/\r\n",buf);
    else if ( bi->text )
    {
	fputs(buf,F->f);
	const int use_colors = opt_bmg_colors != 1
				? opt_bmg_colors
				: bmg->use_color_names > 0;
	uint len = PrintString16BMG( xbuf, sizeof(xbuf), bi->text, bi->len,
					BMG_UTF8_MAX, 0, use_colors );
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

	    fprintf(F->f,"%s %.*s\r\n", sep, (int)(ptr-start), start );
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
    bool		force_numeric,	// true: force numric MIDs
    uint		brief_count	// >0: suppress syntax info
					// >1: suppress all comments
					// >2: suppress '#BMG' magic
)
{
    // use DOS/Windows line format -> unix can handle it ;)

    DASSERT(bmg);
    DASSERT(fname);
    PRINT("SaveTextBMG(%s,%d)\n",fname,set_time);
    bmg->active_cond = 0;

    enum { BRIEF_ALL, BRIEF_NO_SYNTAX, BRIEF_NO_COMMENT, BRIEF_NO_MAGIC };

    bool already_printed[BMG__VIP_N];
    memset(already_printed,0,sizeof(already_printed));
    char mid_name[10], abuf[BMG_ATTRIB_BUF_SIZE];


    static const char text_head[] =
    {
      "#BMG  <<<  The first 4 characters '#BMG' are the magic for a BMG text file.\r\n"
      "#     <<<  Don't remove them!\r\n"
      "#\r\n"
      "# Details about BMG text files are available in the Web:\r\n"
      "#  * Syntax and Semantics: https://szs.wiimm.de/doc/bmg/text\r\n"
      "#  * The BMG file format:  https://szs.wiimm.de/r/wiki/BMG\r\n"
      "#\r\n"
      "#------------------------------------------------------------------------------\r\n"
    };

    static const char text_param[] =
    {
      "# Parameters begin with '@'. Unknown parameters are ignored on scanning.\r\n"
      "\r\n"
      "# Size of each element of section 'INF0' (MKW: 8).\r\n"
      "# Set it first, because it defines defaults for other parameters.\r\n"
      "@INF-SIZE = 0x%02x\r\n"
      "\r\n"
      "# Default attribute values for this BMG (MKW: [1])\r\n"
      "@DEFAULT-ATTRIBS = %s\r\n"
      "\r\n"
      "# Use MKW specific color names: 0=off, 1=auto, 2=on\r\n"
      "@COLOR-NAMES = %u\r\n"
      "\r\n"
      "# Use MKW messages and track names: 0=off, 1=auto, 2=on\r\n"
      "@MKW-MESSAGES = %u\r\n"
      "\r\n"
      "#------------------------------------------------------------------------------\r\n"
    };


    //--- open file

    File_t F;
    enumError err = CreateFile(&F,true,fname,fmode);
    if ( err > ERR_WARNING || !F.f )
	return err;
    SetFileAttrib(&F.fatt,&bmg->fatt,0);

    if ( brief_count == BRIEF_ALL && !opt_bmg_export )
	fwrite(text_head,1,sizeof(text_head)-1,F.f);
    else if ( brief_count < BRIEF_NO_MAGIC )
	fprintf(F.f,"%s\r\n",BMG_TEXT_MAGIC);

    if ( bmg->attrib_used != 4
	|| be32(bmg->attrib) != BMG_INF_STD_ATTRIB
	|| !bmg->use_color_names
	|| !bmg->use_mkw_messages
    )
    {
	PrintAttribBMG(abuf,sizeof(abuf),bmg->attrib,bmg->attrib_used,0,true);
	if ( brief_count == BRIEF_ALL && !opt_bmg_export )
	    fprintf(F.f,text_param,
			bmg->inf_size, abuf,
			bmg->use_color_names, bmg->use_mkw_messages );
	else if ( brief_count < BRIEF_NO_MAGIC )
	    fprintf(F.f,"@INF-SIZE        = 0x%02x\n"
			"@DEFAULT-ATTRIBS = %s\n"
			"@COLOR-NAMES     = %u\n"
			"@MKW-MESSAGES    = %u\n",
			bmg->inf_size, abuf,
			bmg->use_color_names, bmg->use_mkw_messages );
    }

    ccp intro = "\r\n";
    static const char intro_other[] = "\r\n#--- other messages\r\n\r\n";

    if ( opt_bmg_support_mkw && !force_numeric && !opt_bmg_export )
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
			    fprintf(F.f,"\r\n#--- standard track names\r\n\r\n");
			}
			else
			    fputs("\r\n",F.f);
			last_cup = cup;
		    }
		    snprintf(mid_name,sizeof(mid_name), "  T%c%c", cup, idx % 4 + '1' );
		    print_message(&F,bmg,bi1,mid_name);

		    already_printed[ bi1->mid - MID__VIP_BEG ] = true;
		    already_printed[ bi2->mid - MID__VIP_BEG ] = true;

		    if (opt_bmg_support_ctcode)
		    {
			bi2 = FindItemBMG(bmg,MID_TRACK3_BEG+tidx);
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
			    fprintf(F.f,"\r\n#--- battle track names\r\n\r\n");
			}
			else
			    fputs("\r\n",F.f);
			last_cup = cup;
		    }
		    snprintf(mid_name,sizeof(mid_name), "  U%c%c", cup, idx % 5 + '1' );
		    print_message(&F,bmg,bi1,mid_name);

		    already_printed[ bi1->mid - MID__VIP_BEG ] = true;
		    already_printed[ bi2->mid - MID__VIP_BEG ] = true;

		    if (opt_bmg_support_ctcode)
		    {
			bi2 = FindItemBMG(bmg,MID_ARENA3_BEG+tidx);
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
		fprintf(F.f,"\r\n#--- online chat\r\n\r\n");
	    }

	    for ( ; bi1 < bi1end; bi1++ )
	    {
		DASSERT( bi1->mid >= MID_CHAT_BEG && bi1->mid <= MID_CHAT_END );
		snprintf(mid_name,sizeof(mid_name),"  M%02u",bi1->mid-MID_CHAT_BEG+1);
		print_message(&F,bmg,bi1,mid_name);
		already_printed[bi1->mid-MID__VIP_BEG] = true;
	    }
	}
    }


    //--- print messages not already printed

    struct sep_t { u32 mid; ccp info; };
    static const struct sep_t sep_tab[] =
    {
	{ MID_ENGINE_BEG,		"engines" },
	{ MID_ENGINE_END,		0 },
	{ MID_CHAT_BEG,			"online chat" },
	{ MID_CHAT_END,			0 },

	{ MID_RACING_CUP_BEG,		"racing cups" },
	{ MID_RACING_CUP_END,		0 },
	{ MID_TRACK1_BEG,		"track names, 1. copy" },
	{ MID_TRACK1_END,		0 },

	{ MID_BATTLE_CUP_BEG,		"battle cups" },
	{ MID_BATTLE_CUP_END,		0 },

	{ MID_TRACK2_BEG,		"track names, 2. copy" },
	{ MID_TRACK2_END,		0 },

	{ MID_ARENA1_BEG,		"arena names, 1. copy" },
	{ MID_ARENA1_END,		0 },
	{ MID_ARENA2_BEG,		"arena names, 2. copy" },
	{ MID_ARENA2_END,		0 },

	{ MID_PARAM_BEG,		"parameters for mkw-ana" },
	{ MID_PARAM_END,		0 },

	{ MID_CT_TRACK_BEG,		"CT-CODE: track names" },
	{ MID_CT_TRACK_END,		0 },
	{ MID_CT_CUP_BEG,		"CT-CODE: cup names" },
	{ MID_CT_CUP_END,		0 },
	{ MID_CT_TRACK_CUP_BEG,		"CT-CODE: cup index for each track" },
	{ MID_CT_TRACK_CUP_END,		0 },

	{0,0}
    };

    #ifdef TEST
    {
	fflush(stdout);
	const struct sep_t *sep;
	for ( sep = sep_tab+1; sep->mid; sep++ )
	    if ( sep[-1].mid > sep->mid )
		fprintf(stderr,
		    "Wrong order in sep_tab[%zu]:\n\t%04x %s\n\t%04x %s\n",
			sep-sep_tab, sep[-1].mid, sep[-1].info, sep->mid, sep->info );
	fflush(stderr);
    }
    #endif

    const struct sep_t *sep = sep_tab;
    const bool use_sep = opt_bmg_support_mkw && brief_count < BRIEF_NO_COMMENT;

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
	    if ( use_sep && sep->mid && mid >= sep->mid )
	    {
		sep++;
		while ( sep->mid && mid >= sep->mid )
		    sep++;
		if ( sep[-1].info )
		    fprintf(F.f,"\r\n#--- [%x:%x] %s\r\n",
			sep[-1].mid, sep->mid-1,  sep[-1].info );
		else
		    fputs("\r\n",F.f);
		intro = 0;
	    }

	    if ( brief_count < BRIEF_NO_COMMENT && intro )
	    {
		fputs(intro,F.f);
		intro = 0;
	    }
	    snprintf(mid_name,sizeof(mid_name),"%6x",bi->mid);
	    print_message(&F,bmg,bi,mid_name);
	}
    }

    if ( brief_count < 2 )
	fputs("\r\n",F.f);
    ResetFile(&F,set_time);
    return ERR_OK;
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

	noPRINT("%04x = %c\n",be16(source), be16(source) & 0xff );
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
    uint form_len = ScanString16BMG(form_buf,form_size,format,-1);

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

	    copy_attrib(dest,dptr,sptr);
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

	    copy_attrib(dest,dptr,sptr);
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

	    copy_attrib(dest,dptr,sptr);
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
	    reset_attrib(dest,dptr);
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
	    reset_attrib(dest,dptr);
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
	    reset_attrib(dest,dptr);
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
	    reset_attrib(dest,dptr);
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

//DEL	    HexDump16(stdout,0,0x300,sp,(iptr->text+iptr->len-sp)*2);
	    const uint esc_len = ((u8*)sp)[2] & 0xfe;
//DEL	    HexDump16(stdout,0,0x400,sp,esc_len);
	    if (!rm_escapes)
	    {
//DEL		HexDump16(stdout,0,0x500,iptr->text,(dp-iptr->text)*2);
		memmove(dp,sp,esc_len);
//DEL		HexDump16(stdout,0,0x600+(dp-iptr->text)*2,dp,esc_len);
		dp += esc_len/2;
	    }
	    sp += esc_len/2;
	}
	iptr->len = dp - iptr->text;
//DEL	HexDump16(stdout,0,0,iptr->text,iptr->len*2);
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
    for ( mid = MID_RACING_CUP_BEG; mid < MID_RACING_CUP_END; mid++ )
    {
	bmg_item_t * ptr = FindItemBMG(bmg,mid);
	if (ptr)
	{
	    FreeItemBMG(ptr);
	    DASSERT( ptr->mid == mid );
	    ptr->text = 0;
	    reset_attrib(bmg,ptr);
	    dirty = true;
	}
    }
    return dirty;
}

///////////////////////////////////////////////////////////////////////////////

static uint bmg_copy_helper
(
    bmg_t		* bmg,		// pointer to bmg
    bool		force,		// true: force replace
    uint		mid_src_1,	// first source message id
    uint		mid_src_2,	// NULL or second source message id
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
	    sptr = FindItemBMG(bmg,mid_src_2+i);
	if (sptr)
	{
	    bool old_item;
	    bmg_item_t * dptr = InsertItemBMG(bmg,mid_dest+i,0,0,&old_item);
	    if ( force || !old_item || !dptr->len )
	    {
		AssignItemText16BMG(dptr,sptr->text,sptr->len);
		copy_attrib(bmg,dptr,sptr);
		count++;
	    }
	}
    }

    return count;
}

//-----------------------------------------------------------------------------

bool PatchCtCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    uint dirty_count
	= bmg_copy_helper( bmg, false, MID_TRACK1_BEG, MID_TRACK2_BEG,
				MID_TRACK3_BEG, BMG_N_TRACK )
	+ bmg_copy_helper( bmg, false, MID_ARENA1_BEG, MID_ARENA2_BEG,
				MID_ARENA3_BEG, BMG_N_ARENA )
	+ bmg_copy_helper( bmg, false, MID_RACING_CUP_BEG, 0,
				MID_CT_CUP_BEG,
				MID_RACING_CUP_END - MID_RACING_CUP_BEG )
	+ bmg_copy_helper( bmg, false, MID_BATTLE_CUP_BEG, 0,
				MID_CT_BATTLE_CUP_BEG,
				MID_BATTLE_CUP_END - MID_BATTLE_CUP_BEG );
    return dirty_count > 0;
}

//-----------------------------------------------------------------------------

bool PatchCtForceCopyBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    uint dirty_count
	= bmg_copy_helper( bmg, true, MID_TRACK1_BEG, MID_TRACK2_BEG,
				MID_TRACK3_BEG, BMG_N_TRACK )
	+ bmg_copy_helper( bmg, true, MID_ARENA1_BEG, MID_ARENA2_BEG,
				MID_ARENA3_BEG, BMG_N_ARENA )
	+ bmg_copy_helper( bmg, true, MID_RACING_CUP_BEG, 0,
				MID_CT_CUP_BEG,
				MID_RACING_CUP_END - MID_RACING_CUP_BEG )
	+ bmg_copy_helper( bmg, true, MID_BATTLE_CUP_BEG, 0,
				MID_CT_BATTLE_CUP_BEG,
				MID_BATTLE_CUP_END - MID_BATTLE_CUP_BEG );
    return dirty_count > 0;
}

///////////////////////////////////////////////////////////////////////////////

bool PatchCtFillBMG
(
    bmg_t		* bmg		// pointer to destination bmg
)
{
    char buf[100];
    bool dirty = false, found = false;

    bmg_item_t * dptr = bmg->item;
    bmg_item_t * dend = dptr + bmg->item_used;
    for ( ; dptr < dend; dptr++ )
    {
	if (   dptr->mid >= MID_TRACK1_BEG	&& dptr->mid < MID_TRACK1_END
	    || dptr->mid >= MID_TRACK2_BEG	&& dptr->mid < MID_TRACK2_END
	    || dptr->mid >= MID_ARENA1_BEG	&& dptr->mid < MID_ARENA1_END
	    || dptr->mid >= MID_ARENA2_BEG	&& dptr->mid < MID_ARENA2_END
	    || dptr->mid >= MID_RACING_CUP_BEG	&& dptr->mid < MID_RACING_CUP_END
	    || dptr->mid >= MID_BATTLE_CUP_BEG	&& dptr->mid < MID_BATTLE_CUP_END
	    || dptr->mid >= MID_CT_TRACK_BEG	&& dptr->mid < MID_CT_TRACK_END
	    || dptr->mid >= MID_CT_CUP_BEG	&& dptr->mid < MID_CT_CUP_END
	   )
	{
	    if ( dptr->text && dptr->text != bmg_null_entry )
	    {
		found = true;
		break;
	    }
	}
    }

    if (found)
    {
	uint mid;
	for ( mid = MID_CT_TRACK_BEG; mid <= MID_CT_TRACK_END; mid++ )
	{
	    bool old_item;
	    bmg_item_t * dptr = InsertItemBMG(bmg,mid,0,0,&old_item);
	    if ( !old_item || !dptr->len )
	    {
		if ( mid < MID_CT_TRACK_END-1 )
		{
		    const uint len
			= snprintf(buf,sizeof(buf),"Track %02X",mid-MID_CT_TRACK_BEG);
		    AssignItemTextBMG(dptr,buf,len);
		}
		else
		    AssignItemTextBMG(dptr,"???", mid < MID_CT_TRACK_END ? 3 : 1 );

		reset_attrib(bmg,dptr);
		dirty = true;
	    }
	}

	for ( mid = MID_CT_CUP_BEG; mid < MID_CT_CUP_END; mid++ )
	{
	    bool old_item;
	    bmg_item_t * dptr = InsertItemBMG(bmg,mid,0,0,&old_item);
	    if ( !old_item || !dptr->len )
	    {
		const uint len = mid >= MID_CT_BATTLE_CUP_BEG && mid < MID_CT_BATTLE_CUP_END
			? snprintf(buf,sizeof(buf),"Battle %u",mid-MID_CT_BATTLE_CUP_BEG)
			: snprintf(buf,sizeof(buf),"Cup %02X",mid-MID_CT_CUP_BEG);
		AssignItemTextBMG(dptr,buf,len);
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
	    reset_attrib(bmg,ptr);
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
    ccp			format,			// NULL or format parameter
    bool		dup_strings	// true: alloc mem for copied strings (OVERWRITE)
)
{
    PRINT("PatchBMG(%p,%p,%u,%s,%d)\n",
		dest, src, patch_mode, format, dup_strings );
    DASSERT(dest);

    bool dirty = false;
    switch(patch_mode)
    {
     case BMG_PM_PRINT:		dirty = PatchPrintBMG(dest,src); break;
     case BMG_PM_FORMAT:	dirty = PatchFormatBMG(dest,src,format); break;
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
     case BMG_PM_CT_COPY:	dirty = PatchCtCopyBMG(dest); break;
     case BMG_PM_CT_FORCE_COPY:	dirty = PatchCtForceCopyBMG(dest); break;
     case BMG_PM_CT_FILL:	dirty = PatchCtFillBMG(dest); break;

     default:			return ERROR0(ERR_NOT_IMPLEMENTED,0);
    }

    if (dirty)
	TouchFileAttrib(&dest->fatt);

    return dirty ? ERR_DIFFER : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

