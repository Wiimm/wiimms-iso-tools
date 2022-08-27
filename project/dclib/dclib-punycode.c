
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

#include "dclib-basics.h"
#include "dclib-utf8.h"
#include "dclib-punycode.h"

#include <limits.h>

#include "dclib-punycode.inc"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Domain2UTF8()			///////////////
///////////////////////////////////////////////////////////////////////////////

uint Domain2UTF8
(
    // returns the number of scanned bytes of 'source' or 0 on error

    char	*buf,			// valid destination buffer
    int		buf_size,		// size of 'buf' >= 4
    const void	*source,		// NULL or UTF-8 domain to translate
    int		source_len		// length of 'source'; if <0: use strlen(source)
)
{
    DASSERT(buf)
    DASSERT(source||!source_len);

    if ( buf_size < 4 || !source )
	return 0;

    if ( source_len < 0 )
	source_len = strlen(source);

    ccp src = source, src_end = source + source_len;
    char *buf_ptr = buf, *buf_end = buf + buf_size - 2;

    enum { MAX_LEN = 1000 };
    char name[MAX_LEN+10];
    punycode_uint pname[MAX_LEN+2];
    MARK_USED(name,pname);

    while ( src < src_end )
    {
	while ( buf_ptr < buf_end && src < src_end && *src == '.' )
	    *buf_ptr++ = *src++;
	if ( buf_ptr >= buf_end )
	{
	    if (!src)
		goto error;
	    break;
	}

	if ( src >= src_end )
	    break;

	ccp point = strchr(src,'.');
	if (!point)
	    point = src_end;

	if ( src+4 > src_end || memcmp(src,"xn--",4 ))
	    buf_ptr = StringCopyEM(buf_ptr,buf_end,src,point-src);
	else
	{
	    src += 4;
	    punycode_uint olen = MAX_LEN;
	    const enum punycode_status stat
		= punycode_decode(point-src,src,&olen,pname,0);
	    char *dest = name;
	    if ( stat != punycode_success )
		dest = name + snprintf(name,sizeof(name),"!ERR=%d",stat);
	    else
	    {
		punycode_uint *src = pname, *src_end = pname + olen;
		char *dest_end = name + MAX_LEN;
		while ( src < src_end && dest < dest_end )
		    dest = PrintUTF8Char(dest,*src++);
		*dest = 0;
	    }

	    buf_ptr = StringCopyEM(buf_ptr,buf_end,name,dest-name);
	}
	src = point;
    }

    *buf_ptr = 0;
    return src - (ccp)source;

 error:
    *buf = 0;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Domain2ASCII()			///////////////
///////////////////////////////////////////////////////////////////////////////

uint Domain2ASCII
(
    // returns the number of scanned bytes of 'source' or 0 on error

    char	*buf,			// valid destination buffer
    int		buf_size,		// size of 'buf' >= 5
    const void	*source,		// NULL or ASCII domain to translate
    int		source_len		// length of 'source'; if <0: use strlen(source)
)
{
    DASSERT(buf)
    DASSERT(source||!source_len);

    if ( buf_size < 5 || !source )
	return 0;

    if ( source_len < 0 )
	source_len = strlen(source);

    ccp src = source, src_end = source + source_len;
    char *buf_ptr = buf, *buf_end = buf + buf_size - 5;

    enum { MAX_LEN = 1000 };
    char name[MAX_LEN+10];
    punycode_uint pname[MAX_LEN+2];

    while ( src < src_end )
    {
	while ( buf_ptr < buf_end && src < src_end && *src == '.' )
	    *buf_ptr++ = *src++;
	if ( buf_ptr >= buf_end )
	{
	    if (!src)
		goto error;
	    break;
	}

	if ( src >= src_end )
	    break;

	ccp point = strchr(src,'.');
	if (!point)
	    point = src_end;

	punycode_uint *dest = pname, *dest_end = pname + MAX_LEN;
	while ( dest < dest_end && src < point )
	{
	    u32 code = ScanUTF8AnsiCharE(&src,point);
	    *dest++ = code;
	}

	punycode_uint olen = MAX_LEN;
	const enum punycode_status stat =
	    punycode_encode(dest-pname,pname,0,&olen,name);
	if (stat)
	    goto error;

	if ( olen > 0 && name[olen-1] == '-' )
	    buf_ptr = StringCopyEM(buf_ptr,buf_end,name,olen-1);
	else
	{
	    name[olen] = 0;
	    buf_ptr = StringCat2E(buf_ptr,buf_end,"xn--",name);
	}
    }

    *buf_ptr = 0;
    return src - (ccp)source;

 error:
    *buf = 0;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
