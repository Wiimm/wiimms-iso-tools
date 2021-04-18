
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

#ifndef WIT_MATCH_PATTERN_H
#define WIT_MATCH_PATTERN_H 1

#include "dclib/dclib-types.h"
#include "wiidisc.h"

///////////////////////////////////////////////////////////////////////////////

typedef struct FilePattern_t
{
	StringField_t rules;	// rules db

	bool is_active;		// true if at least one pattern set
	bool is_dirty;		// true if setup is needed
	bool match_all;		// true if all files allowed
	bool match_none;	// true if no files allowed

	bool macro_negate;	// true if macro ':negate' was called
	bool user_negate;	// user defined negation
	bool active_negate;	// := macro_negate != user_negate

} FilePattern_t;

///////////////////////////////////////////////////////////////////////////////

typedef enum enumPattern
{
	PAT_FILES,		// ruleset of option --files
	PAT_RM_FILES,		// ruleset of option --rm-files
	PAT_ZERO_FILES,		// ruleset of option --zero-files
	PAT_IGNORE_FILES,	// ruleset of option --ignore-files
	PAT_FAKE_SIGN,		// ruleset of option --fake-sign
	PAT_DEFAULT,		// default ruleset if PAT_FILES is empty
	PAT_PARAM,		// file pattern for parameters

	PAT__N,			// number of patterns

} enumPattern;

extern FilePattern_t file_pattern[PAT__N];

///////////////////////////////////////////////////////////////////////////////
// pattern db

void InitializeFilePattern ( FilePattern_t * pat );
void ResetFilePattern ( FilePattern_t * pat );
void InitializeAllFilePattern();
int  AddFilePattern ( ccp arg, int pattern_index );
int ScanRule ( ccp arg, enumPattern pattern_index );
FilePattern_t * GetDefaultFilePattern();
void DefineNegatePattern ( FilePattern_t * pat, bool negate );
void MoveParamPattern ( FilePattern_t * dest_pat );
bool SetupFilePattern ( FilePattern_t * pat );

bool MatchFilePattern
(
    FilePattern_t	* pat,		// filter rules
    ccp			text,		// text to check
    char		path_sep	// path separator character, standard is '/'
);

int MatchFilePatternFST
(
	struct wd_iterator_t *it	// iterator struct with all infos
);

///////////////////////////////////////////////////////////////////////////////

#endif // WIT_MATCH_PATTERN_H

