
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

#ifndef DCLIB_PARSER_H
#define DCLIB_PARSER_H 1

#include "dclib-types.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    ScanText_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ScanText_t]]

typedef struct ScanText_t
{
    //-- base data

    ccp		data;			// begin of text
    uint	data_size;		// total size of text

    ccp		fname;			// NULL or filename for messages
    bool	fname_alloced;		// true: call FreeString(fname)

    //-- options

    bool	ignore_comments;	// true: ignore lines with leading '#'
    int		detect_sections;	// >0: detect lines of format '[...]'

    //-- current statee

    ccp		ptr;			// current text position
    ccp		eot;			// end of text
    ccp		line;			// current line
    ccp		eol;			// end of current line

    bool	is_eot;			// end of text reached
    bool	is_section;		// new section reached
    bool	is_term;		// := is_eot || is_section
}
ScanText_t;

///////////////////////////////////////////////////////////////////////////////

void SetupScanText	( ScanText_t *st, cvp data, uint data_size );
void ResetScanText	( ScanText_t *st );
void SetFilenameScanText( ScanText_t *st, ccp fname, CopyMode_t cm );

void RewindScanText	( ScanText_t *st );
bool NextLineScanText	( ScanText_t *st );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_PARSER_H

