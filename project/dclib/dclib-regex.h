
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

#ifndef DCLIB_REGEX_H
#define DCLIB_REGEX_H 1

#if DCLIB_USE_PCRE
  #include <pcre.h>
  #define DC_REGEX_TYPE pcre
#else
  #ifndef DCLIB_USE_REGEX
    #define DCLIB_USE_REGEX 1
  #endif
  #if DCLIB_USE_REGEX
    #include <regex.h>
    #define DC_REGEX_TYPE regex_t
  #else
    #error Nn Regex Support!
  #endif
#endif

#include "dclib-types.h"
#include "dclib-debug.h"
#include "dclib-basics.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct Regex_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[RegexReplace_t]]

typedef struct RegexReplace_t
{
    mem_t	str;	// first:  string to replace (maybe 0 bytes)
    int		ref;	// second: back reference to copy (if >=0)
}
RegexReplace_t;

///////////////////////////////////////////////////////////////////////////////
// [[RegexElem_t]]

typedef struct RegexElem_t
{
    bool		valid;		// true: successfull compiled
    bool		global;		// true: replace all ('g')
    bool		icase;		// true: ignore case ('i')

 #if DCLIB_USE_PCRE
    DC_REGEX_TYPE	*regex;		// not NULL: compiled regular expression
 #else
    DC_REGEX_TYPE	regex;		// compiled regular expression
 #endif

    int			opt;		// compile options
    ccp			pattern;	// pattern string, alloced
    mem_t		replace;	// replace string, alloced

    RegexReplace_t	*repl;		// replace data, use 'replace' as reference
    uint		repl_used;	// number of used elements in 'repl'
    uint		repl_size;	// number of available elements in 'repl'
}
RegexElem_t;

///////////////////////////////////////////////////////////////////////////////
// [[RegexMain_t]]

typedef struct Regex_t
{
    bool		valid;		// true: succesfull initialized

    RegexElem_t		*re_list;	// list; either 're_pool' or alloced
    RegexElem_t		re_pool[3];	// pool for fast access
    uint		re_used;	// number of used elements in 're_list'
    uint		re_size;	// number of available elements in 're_list'
}
Regex_t;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeRegex ( Regex_t *re );
void ResetRegex ( Regex_t *re );

enumError ScanRegex ( Regex_t *re, bool init_re, ccp regex );

int ReplaceRegex
(
    Regex_t	*re,		// valid Regex_t
    FastBuf_t	*res,		// return buffer, cleared
    ccp		src,
    int		src_len		// -1: use strlen()
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_REGEX_H

