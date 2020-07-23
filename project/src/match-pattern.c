
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
 *   Copyright (c) 2009-2017 by Dirk Clemens <wiimm@wiimm.de>              *
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

#include <stdlib.h>
#include <string.h>

#include "dclib/dclib-debug.h"
#include "lib-std.h"
#include "match-pattern.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    variables                    ///////////////
///////////////////////////////////////////////////////////////////////////////

FilePattern_t file_pattern[PAT__N];

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    pattern db                   ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeFilePattern ( FilePattern_t * pat )
{
    DASSERT(pat);
    memset(pat,0,sizeof(*pat));
    InitializeStringField(&pat->rules);
    pat->match_all	= true;
}

///////////////////////////////////////////////////////////////////////////////

void ResetFilePattern ( FilePattern_t * pat )
{
    DASSERT(pat);
    ResetStringField(&pat->rules);
    InitializeFilePattern(pat);
}

///////////////////////////////////////////////////////////////////////////////

void InitializeAllFilePattern()
{
    memset(file_pattern,0,sizeof(file_pattern));

    FilePattern_t * pat = file_pattern;
    FilePattern_t * end = pat + PAT__N;
    for ( ; pat < end; pat++ )
	InitializeFilePattern(pat);
}

///////////////////////////////////////////////////////////////////////////////

struct macro_tab_t
{
    int len;
    ccp name;
    ccp expand;
};

static const struct macro_tab_t macro_tab[] =
{
    { 4, "base",	"+/*$" },
    { 6, "nobase",	"-/*$" },
    { 4, "disc",	"+/disc/" },
    { 6, "nodisc",	"-/disc/" },
    { 3, "sys",		"+/sys/" },
    { 5, "nosys",	"-/sys/" },
    { 5, "files",	"+/files/" },
    { 7, "nofiles",	"-/files/" },
    { 3, "wit",		"4+/h3.bin;3+/sys/user.bin;2+/sys/fst.bin;1+/sys/fst+.bin;+" },
    { 3, "wwt",		"4+/h3.bin;3+/sys/user.bin;2+/sys/fst.bin;1+/sys/fst+.bin;+" },
    { 7, "compose",	"+/cert.bin;4+/disc/;3+/*$;2+/sys/fst.bin;1+/sys/fst+.bin;+" },
    { 4, "neek",	"3+/setup.txt;2+/h3.bin;1+/disc/;+" },
    { 5, "sneek",	"3+/setup.txt;2+/h3.bin;1+/disc/;+" },

    {0,0,0}
};

///////////////////////////////////////////////////////////////////////////////

int AddFilePattern ( ccp arg, int pattern_index )
{
    TRACE("AddFilePattern(%s,%d)\n",arg,pattern_index);
    DASSERT( pattern_index >= 0 );
    DASSERT( pattern_index < PAT__N );
    
    if ( !arg || (u32)pattern_index >= PAT__N )
	return 0;

    FilePattern_t * pat = file_pattern + pattern_index;
   
    pat->is_active = true;

    while (*arg)
    {
	ccp start = arg;
	bool ok = false;
	if ( *arg >= '1' && *arg <= '9' )
	{
	    while ( *arg >= '0' && *arg <= '9' )
		arg++;
	    if ( *arg == '+' || *arg == '-' )
		ok = true;
	    else
		arg = start;
	}

	// hint: '=' is obsolete and compatible to ':'

	if ( !ok && *arg != '+' && *arg != '-' && *arg != ':' && *arg != '=' )
	    return ERROR0(ERR_SYNTAX,
		"File pattern rule must begin with '+', '-' or ':' => %.20s\n",arg);

	while ( *arg && *arg != ';' )
	    arg++;

	if ( *start == ':' || *start == '=' )
	{
	    const int len = arg - ++start;
	    const struct macro_tab_t *tab;
	    for ( tab = macro_tab; tab->len; tab++ )
		if ( tab->len == len && !memcmp(start,tab->name,len) )
		{
		    AddFilePattern(tab->expand,pattern_index);
		    break;
		}
	    if (!tab->len)
	    {
		if (!strcmp(start,"negate"))
		{
		    pat->macro_negate = true;
		    pat->active_negate = pat->macro_negate != pat->user_negate;
		}
		else
		    return ERROR0(ERR_SYNTAX,
			"Macro '%.*s' not found: :%.20s\n",len,start,start);
	    }
	}
	else
	{
	    const size_t len = arg - start;
	    char * pattern = MALLOC(len+1);
	    memcpy(pattern,start,len);
	    pattern[len] = 0;
	    TRACE(" - ADD |%s|\n",pattern);
	    AppendStringField(&pat->rules,pattern,true);
	    pat->is_dirty = true;
	}
	
	while ( *arg == ';' )
	    arg++;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int ScanRule ( ccp arg, enumPattern pattern_index )
{
    return AtFileHelper(arg,pattern_index,pattern_index,AddFilePattern) != 0;
}

///////////////////////////////////////////////////////////////////////////////

FilePattern_t * GetDFilePattern ( enumPattern pattern_index )
{
    DASSERT( (u32)pattern_index < PAT__N );
    FilePattern_t * pat = file_pattern + pattern_index;
    if (!pat->rules.used)
	pat = file_pattern + PAT_DEFAULT;
    return pat;
}

///////////////////////////////////////////////////////////////////////////////

FilePattern_t * GetDefaultFilePattern()
{
    FilePattern_t * pat = file_pattern + PAT_FILES;
    if (!pat->rules.used)
	pat = file_pattern + PAT_DEFAULT;
    return pat;
}

///////////////////////////////////////////////////////////////////////////////

void DefineNegatePattern ( FilePattern_t * pat, bool negate )
{
    DASSERT(pat);
    pat->user_negate = negate;
    pat->active_negate = pat->macro_negate != pat->user_negate;
}

///////////////////////////////////////////////////////////////////////////////

void MoveParamPattern ( FilePattern_t * dest_pat )
{
    DASSERT(dest_pat);
    FilePattern_t * src = file_pattern + PAT_PARAM;
    SetupFilePattern(src);
    memcpy( dest_pat, src, sizeof(*dest_pat) );
    InitializeFilePattern(src);
}

///////////////////////////////////////////////////////////////////////////////

bool SetupFilePattern ( FilePattern_t * pat )
{
    ASSERT(pat);
    if (pat->is_dirty)
    {
	pat->is_active	= true;
	pat->is_dirty	= false;
	pat->match_all	= false;
	pat->match_none	= false;

	if (!pat->rules.used)
	    pat->match_all = true;
	else
	{
	    ccp first = *pat->rules.field;
	    ASSERT(first);
	    if (   !strcmp(first,"+")
		|| !strcmp(first,"+*")
		|| !strcmp(first,"+**") )
	    {
		pat->match_all = true;
	    }
	    else if (   !strcmp(first,"-")
		     || !strcmp(first,"-*")
		     || !strcmp(first,"-**") )
	    {
		pat->match_none = true;
	    }
	}
     #ifdef DEBUG
	TRACE("FILE PATTERN: N=%u, all=%d, none=%d\n",
		pat->rules.used, pat->match_all, pat->match_none );
	
	ccp * ptr = pat->rules.field;
	ccp * end = ptr +  pat->rules.used;
	while ( ptr < end )
	    TRACE("  |%s|\n",*ptr++);
     #endif
    }

    pat->active_negate = pat->macro_negate != pat->user_negate;
    return pat->is_active && !pat->match_none;
}

///////////////////////////////////////////////////////////////////////////////

bool MatchFilePattern
(
    FilePattern_t	* pat,		// filter rules
    ccp			text,		// text to check
    char		path_sep	// path separator character, standard is '/'
)
{
    if (!pat)
	pat = GetDefaultFilePattern(); // use default pattern if not set
    DASSERT(pat);

    if (pat->is_dirty)
	SetupFilePattern(pat);
    if (pat->match_all)
	return !pat->active_negate;
    if (pat->match_none)
	return pat->active_negate;

    bool default_result = !pat->active_negate;
    int skip = 0;

    ccp * ptr = pat->rules.field;
    ccp * end = ptr + pat->rules.used;
    while ( ptr < end )
    {
	char * pattern = (char*)(*ptr++); // non const because of strtoul()
	DASSERT(pattern);
	switch (*pattern++)
	{
	    case '-':
		if ( skip-- <= 0 && MatchPattern(pattern,text,path_sep) )
		    return pat->active_negate;
		default_result = !pat->active_negate;
		break;

	    case '+':
		if ( skip-- <= 0 && MatchPattern(pattern,text,path_sep) )
		    return !pat->active_negate;
		default_result = pat->active_negate;
		break;

	    default:
		if ( skip-- <= 0 )
		{
		    pattern--;
		    ulong num = strtoul(pattern,&pattern,10);
		    switch (*pattern++)
		    {
			case '-':
			    if (!MatchPattern(pattern,text,path_sep))
				skip = num;
			    break;

			case '+':
			    if (MatchPattern(pattern,text,path_sep))
				skip = num;
			    break;
		    }
		}
		break;
	}
    }

    return default_result;
}

///////////////////////////////////////////////////////////////////////////////

int MatchFilePatternFST
(
	struct wd_iterator_t *it	// iterator struct with all infos
)
{
    DASSERT(it);
    // result>0: ignore this file
    return it->icm >= WD_ICM_DIRECTORY
	&& !MatchFilePattern(it->param,it->fst_name,'/');
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////
