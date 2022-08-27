
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

#include <unistd.h>

#include "dclib-basics.h"
#include "dclib-debug.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global commands			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Command_ARGTEST ( int argc, char ** argv )
{
    printf("ARGUMENT TEST: %u arguments:\n",argc);

    int idx;
    for ( idx = 0; idx < argc; idx++ )
	printf("%4u.: |%s|\n",idx,argv[idx]);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError Command_COLORS
(
    int		level,		// only used, if select==0
				//  <  0: status message (ignore select)
				//  >= 1: include names
				//  >= 2: include alt names
				//  >= 3: include color names incl bold
				//  >= 4: include background color names
    ColorSelect_t select,	// select color groups; if 0: use level
    uint	format		// output format => see PrintColorSet()
)
{
    if (!colout)
	SetupStdMsg();

    if ( level < 0 )
    {
	ccp term = getenv("TERM");
	if (!term)
	    term = "?";
	printf("%s--color=%d [%s], colorize=%d [%s]\n"
		"term=%s\n"
		"stdout: tty=%d, mode=%d [%s], have-color=%d, n-colors=%u%s\n",
		colout->status,
		opt_colorize, GetColorModeName(opt_colorize,"?"),
		colorize_stdout, GetColorModeName(colorize_stdout,"?"),
		term, isatty(fileno(stdout)),
		colout->col_mode, GetColorModeName(colout->col_mode,"?"),
		colout->colorize, colout->n_colors, colout->reset );
	return ERR_OK;
    }

    if ( !select && level > 0 )
	switch (level)
	{
	    case 1:  select = COLSEL_NAME; break;
	    case 2:  select = COLSEL_NAME|COLSEL_F_ALT; break;
	    case 3:  select = COLSEL_M_NONAME; break;
	    default: select = COLSEL_M_ALL; break;
	}

    if (!format)
    {
	const ColorMode_t col_mode = colout ? colout->col_mode : COLMD_AUTO;
	PrintTextModes( stdout, 4, col_mode );
	PrintColorModes( stdout, 4, col_mode, GCM_ALT );
     #ifdef TEST
	PrintColorModes( stdout, 4, col_mode, GCM_ALT|GCM_SHORT );
     #endif
    }

    if ( select || format )
	PrintColorSet(stdout,4,colout,select,format);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

