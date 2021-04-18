
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

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dclib/dclib-types.h"
#include "lib-std.h"
#include "iso-interface.h"
#include "ui.h" // [[dclib]] wrapper
#include "dclib-ui.h"

#include "dclib-gen-ui.h"
#include "ui-head.inc"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			some helper macros		///////////////
///////////////////////////////////////////////////////////////////////////////

#define TEXT_WWT_OPT_REPAIR \
	"This option defines how to repair WBFS errors." \
	" The parameter is a comma separated list of the following keywords," \
	" case is ignored:" \
	" @NONE, FBT, INODES, STANDARD," \
	" RM-INVALID, RM-OVERLAP, RM-FREE, RM-EMPTY, RM-ALL, ALL@." \
	"\n " \
	" All keywords can be prefixed by '+' to enable that option," \
	" by a '-' to disable it or" \
	" by a '=' to enable that option and disable all others."

#define TEXT_OPT_CHUNK_MODE(def) \
	"Defines an operation mode for {--chunk-size} and {--max-chunks}." \
	" Allowed keywords are @'ANY'@ to allow any values," \
	" @'32K'@ to force chunk sizes with a multiple of 32 KiB," \
	" @'POW2'@ to force chunk sizes >=32K and with a power of 2" \
	" or @'ISO'@ for ISO images (more restrictive as @'POW2'@," \
	" best for USB loaders)." \
	" The case of the keyword is ignored." \
	" The default key is @'" def "'@." \
	"\n " \
	" @--chm@ is a shortcut for @--chunk-mode@."

#define TEXT_EXTRACT_LONG \
	"Print a summary line while extracting files." \
	" If set at least twice, print a status line for each extracted files."

#define TEXT_FILE_FILTER \
	" This option can be used multiple times to extend the rule list." \
	" Rules beginning with a '+' or a '-' are allow or deny rules rules." \
	" Rules beginning with a ':' are macros for predefined rule sets." \
	"\1\n " \
	" See https://wit.wiimm.de/info/file-filter.html" \
	" for more details about file filters."

///////////////////////////////////////////////////////////////////////////////

#define TEXT_DIFF_QUIET \
	"Be quiet and print only error messages and failure messages on mismatch." \
	" The comparison is aborted at the first mismatch for each source image." \
	" If set twice print nothing and report the diff result only as exit status" \
	" and the complete comparison is aborted at the first mismatch at all."

#define TEXT_DIFF_VERBOSE \
	"The default is to print only differ messages." \
	" If set success messages and summaries are printed too." \
	" If set at least twice, a progress counter is printed too."

#define TEXT_DIFF_FILE_LIMIT \
	"This option is only used if comparing discs on file level." \
	" If not set or set to null, then all files will be compared." \
	" If set to a value greater than comparison is aborted for" \
	" the current source image if the entered number of files differ." \
	" This option is ignored in quiet mode."

#define TEXT_DIFF_LIMIT \
	"If not set, the comparison of the current file is aborted" \
	" if a mismatch is found." \
	" If set, the comparison is aborted after @'limit'@ mismatches." \
	" To compare the whole file use the special value @0@." \
	" This option is ignored in quiet mode."

#define TEXT_DIFF_LONG \
	"If set, a status line with the offset is printed for each found mismatch." \
	" If set twice, an additional hex dump of the first bytes is printed." \
	" If set 3 or 4 times, the limit is set to 10 or unlimited" \
	" if option {--limit} is not already set." \
	" This option is ignored in quiet mode."

#define TEXT_DIFF_BLOCK_SIZE \
	"If a mismatch is found in raw or disc mode then the comparison" \
	" is continued with the next block. This option sets the block size." \
	" The default value is @32K@ (Wii sector size)." \
	" This option is ignored in quiet mode."

//
///////////////////////////////////////////////////////////////////////////////
///////////////			the info table			///////////////
///////////////////////////////////////////////////////////////////////////////

info_t info_tab[] =
{
    #include "tab-wit.inc"
    #include "tab-wwt.inc"
    #include "tab-wdf.inc"
    #include "tab-wfuse.inc"

    { T_END, 0,0,0,0 }
};

#include "dclib-gen-ui.inc"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    main()			///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    return ui_main(argc,argv);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

