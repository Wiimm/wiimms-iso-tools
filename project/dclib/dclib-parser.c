
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

#include "dclib-basics.h"
#include "dclib-parser.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    ScanText_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupScanText ( ScanText_t *st, cvp data, uint data_size )
{
    DASSERT(st);
    memset(st,0,sizeof(*st));
    st->data		= data;
    st->data_size	= data_size;
    st->ignore_comments	= true;
    
    RewindScanText(st);
}

///////////////////////////////////////////////////////////////////////////////

void ResetScanText ( ScanText_t *st )
{
    if (st)
    {
	if (st->fname_alloced)
	    FreeString(st->fname);
	memset(st,0,sizeof(*st));
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetFilenameScanText( ScanText_t *st, ccp fname, CopyMode_t cm )
{
    DASSERT(st);
    if (st->fname_alloced)
	FreeString(st->fname);
    st->fname = CopyString(fname,cm,&st->fname_alloced);
}

///////////////////////////////////////////////////////////////////////////////

void RewindScanText ( ScanText_t *st )
{
    DASSERT(st);

    st->ptr		= st->data;
    st->eot		= st->data + st->data_size;
    st->line		= st->data;
    st->eol		= st->data;

    st->is_eot		= !st->ptr || st->ptr >= st->eot;
    st->is_section	= false;
    st->is_term		= st->is_eot;
}

///////////////////////////////////////////////////////////////////////////////

bool NextLineScanText ( ScanText_t *st )
{
    DASSERT(st);
    st->is_section = false;

    ccp ptr = st->ptr;
    ccp eot = st->eot;
    if ( !ptr || ptr >= eot )
    {
	st->line = st->eol = ptr;
	st->is_eot = st->is_term = true;
	return false;
    }

    while ( ptr < eot )
    {
	// skip leading spaced and controls
	while ( ptr < eot && (uchar)*ptr <= ' ' )
	    ptr++;

	ccp line = ptr;
	while ( ptr < eot && *ptr && *ptr != '\r' && *ptr != '\n' )
	    ptr++;
	ccp eol = ptr;
	while ( eol > line && eol[-1] == ' ' || eol[-1] == '\t' )
	    eol--;

	if ( line == eol || *line == '#' && st->ignore_comments )
	    continue;
	if ( st->detect_sections > 0
	   && *line == '['
	   && eol[-1] == ']'
	   && !memchr(line+1,'[',eol-line-2)
	   && !memchr(line+1,']',eol-line-2)
	   )
	{
	    st->is_section = true;
	}

	st->line = line;
	st->eol  = eol;
	break;
    }

    st->ptr = ptr;
    st->is_eot = ptr >= eot;
    st->is_term = st->is_eot || st->is_section;
    return !st->is_term;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

