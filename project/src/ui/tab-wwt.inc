
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
 *   Copyright (c) 2009-2012 by Dirk Clemens <wiimm@wiimm.de>              *
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

//
///////////////////////////////////////////////////////////////////////////////
/////////////			    TOOL wwt			///////////////
///////////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wwt", 0,
		"wwt [option]... command [option|parameter|@file]...",
		"Wiimms WBFS Tool (WBFS manager) :"
		" It can create, check, repair, verify and clone WBFS files"
		" and partitions. It can list, add, extract, remove, rename"
		" and recover ISO images as part of a WBFS." },

  //
  //---------- list of all wwt commands ----------

  { T_DEF_CMD,	"VERSION",	"VERSION",
		    "wwt VERSION [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"HELP",		"HELP|H|?",
		    "wwt HELP [command] [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"INFO",		"INFO",
		    "wit INFO [keyword]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"TEST",		"TEST",
		    "wwt TEST [ignored]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"ERROR",	"ERROR|ERR",
		    "wwt ERROR [error_code]",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"COMPR",	"COMPR",
		    "wwt COMPR [error_code]",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"FEATURES",	"FEATURES",
		    "wwt FEATURES [error_code]",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"EXCLUDE",	"EXCLUDE",
		    "wwt EXCLUDE [additional_excludes]...",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"TITLES",	"TITLES",
		    "wwt TITLES [additional_title_file]",
		0 /* copy of wit */ },

  { T_DEF_CMD,	"GETTITLES",	"GETTITLES",
		    "wwt GETTITLES",
		0 /* copy of wit */ },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FIND",		"FIND|F",
		    "wwt FIND [wbfs_partition]...",
		"Find WBFS partitions and optionally print some geometric values." },

  { T_DEF_CMD,	"SPACE",	"SPACE|DF",
		    "wwt SPACE [wbfs_partition]...",
		"Print disk space of WBFS partitions." },

  { T_DEF_CMD,	"ANALYZE",	"ANALYZE|ANA|ANALYSE",
		    "wwt ANALYZE [wbfs_partition]...",
		"Analyze files and partitions for WBFS usage."
		" Try to find old WBFS structures and make calculations for new WBFS." },

  { T_DEF_CMD,	"DUMP",		"DUMP|D",
		    "wwt DUMP [wbfs_partition]...",
		"Dump the data structure of WBFS partitions." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"ID6",		"ID6|ID",
		    "wwt ID6 [id]...",
		"List all ID6 of all discs of WBFS partitions."
		" If the ID list is set use it as selector."},

  { T_DEF_CMD,	"LIST",		"LIST|LS",
		    "wwt LIST [wbfs_partition]...",
		"List all discs of WBFS partitions." },

  { T_DEF_CMD,	"LIST_L",	"LIST-L|LL|LISTL",
		    "wwt LIST-L [wbfs_partition]...",
		"List all discs of WBFS partitions with size and region."
		" 'LIST-L' is a shortcut for {LIST --long}." },

  { T_DEF_CMD,	"LIST_LL",	"LIST-LL|LLL|LISTLL",
		    "wwt LIST-LL [wbfs_partition]...",
		"List all discs of WBFS partitions with date, size and region."
		" 'LIST-LL' is a shortcut for {LIST --long --long}." },

  { T_DEF_CMD,	"LIST_LLL",	"LIST-LLL|LLLL|LISTLLL",
		    "wwt LIST-LLL [wbfs_partition]...",
		"List all discs of WBFS partitions with date, time, size and region."
		" 'LIST-LLL' is a shortcut for {LIST --long --long --long}." },

  { T_DEF_CMD,	"LIST_A",	"LIST-A|LA|LISTA",
		    "wwt LIST-A [wbfs_partition]...",
		"List all discs of all WBFS partitions."
		" 'LIST-A' is a shortcut for {LIST --long --long --auto}." },

  { T_DEF_CMD,	"LIST_M",	"LIST-M|LM|LISTM",
		    "wwt LIST-M [wbfs_partition]...",
		"List all discs of WBFS partitions in mixed view."
		" 'LIST-M' is a shortcut for {LIST --long --long --mixed}." },

  { T_DEF_CMD,	"LIST_U",	"LIST-U|LU|LISTU",
		    "wwt LIST-U [wbfs_partition]...",
		"List all discs of WBFS partitions in mixed view."
		" 'LIST-U' is a shortcut for {LIST --long --long --unique}." },

  { T_DEF_CMD,	"LIST_F",	"LIST-F|LF|LISTF",
		    "wwt LIST-F [wbfs_partition]...",
		"List all discs of WBFS partitions and include fragmentation info."
		" 'LIST-F' is a shortcut for {LIST --fragments}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FORMAT",	"FORMAT|INIT",
		    "wwt FORMAT file|blockdev...",
		"Initialize (=format) WBFS partitions and files."
		" Combine with {--recover} to recover discs." },

  { T_DEF_CMD,	"RECOVER",	"RECOVER",
		    "wwt RECOVER [wbfs_partition]..",
		"Recover deleted discs of WBFS partitions." },

  { T_DEF_CMD,	"CHECK",	"CHECK|FSCK",
		    "wwt CHECK [wbfs_partition]..",
		"Check WBFS partitions and print error listing."
		" To repair WBFS partitions use the option {--repair modelist}."  },

  { T_DEF_CMD,	"REPAIR",	"REPAIR",
		    "wwt REPAIR [wbfs_partition]..",
		"Check WBFS partitions and repair errors."
		" 'REPAIR' is a shortcut for {CHECK --repair standard}." },

  { T_DEF_CMD,	"EDIT",		"EDIT",
		    "wwt EDIT [JOB]...\n"
		    " \n"
		    "JOB      := REMOVE | ACTIVATE | INVALID | FREE | USE | ID6\n"
		    "REMOVE   := RM=a,b-c,...\n"
		    "ACTIVATE := ACT=a,b-c,...\n"
		    "INVALID  := INV=a,b-c,...\n"
		    "FREE     := FREE=a,b-c,...\n"
		    "USE      := USE=a,b-c,...\n"
		    "ID6      := ID6=a:b,c-d:e,...",
		"Low level edit of slot and block assignments."
		" Dangerous! Read the documentation!" },

  { T_DEF_CMD,	"PHANTOM",	"PHANTOM",
		    "wwt PHANTOM [RULE]...\n"
		    " \n"
		    "RULE     := [ NUM 'x' ] SIZE ['m'|'g']\n"
		    "NUM,SIZE := UINT | UINT '-' UINT",
		"Add @NUM@ phantom discs with entered @SIZE@ (default unit 'g')"
		" with a generic ID6 ('PHT###' where '###' is the smallest"
		" not already used integer)."
		" If @NUM@ and/or @SIZE@ are ranges use random values from this range."
		"\n "
		" Phantom discs have no content and only a header is written."
		" This makes adding discs very fast and this is good for testing."
		" The creation process stops if the WBFS becomes full."
		" In this case, the last added image may be shorter as planned." },

  { T_DEF_CMD,	"TRUNCATE",	"TRUNCATE|TR",
		    "wwt TRUNCATE [wbfs_partition]..",
		"Truncate WBFS partitions to the really used size." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"ADD",		"ADD|A",
		    "wwt ADD [[--source|--recurse] source]...",
		"Add Wii and GameCube ISO discs to WBFS partitions."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"UPDATE",	"UPDATE|U",
		    "wwt UPDATE [[--source|--recurse] source]...",
		"Add missing Wii and GameCube ISO discs to WBFS partitions."
		" Images, WBFS partitions and directories are accepted as source."
		" 'UPDATE' is a shortcut for {ADD --update}." },

  { T_DEF_CMD,	"NEW",		"NEW|N",
		    "wwt NEW [[--source|--recurse] source]...",
		"Add missing and newer Wii and GameCube ISO discs to WBFS partitions."
		" Images, WBFS partitions and directories are accepted as source."
		" 'NEW' is a shortcut for {ADD --update --newer}." },

  { T_DEF_CMD,	"SYNC",		"SYNC",
		    "wwt SYNC [[--source|--recurse] source]...",
		"Modify primary WBFS (REMOVE and ADD)"
		" until it contains exactly the same discs as all sources together."
		" Images, WBFS partitions and directories are accepted as source."
		" 'SYNC' is a shortcut for {ADD --sync}." },

  { T_DEF_CMD,	"DUP",		"DUP",
		    "wwt DUP [source_wbfs]... [-d|-D] dest_file",
		"Duplicate source WBFS files or partitions and create new WBFS files."
		" If multiple sources entered, the destination must be a directory."
		" All used WBFS blocks are copied 1:1 to the destination file"
		" and unused blocks are skipped and stored as sparse blocks." },

  { T_DEF_CMD,	"EXTRACT",	"EXTRACT|X",
		    "wwt EXTRACT id6[=dest]...",
		"Extract discs from WBFS partitions and store them"
		" as Wii or GameCube images." },

  { T_DEF_CMD,	"SCRUB",	"SCRUB",
		    "wwt SCRUB id6...",
		"Scrub discs of WBFS partitions and rewrite (repair) the WBFS block map."
		" Use '+' as ID6 to scrub all images of the WBFS partitions."
		" Use option {--psel=list} to remove image partitions." },

  { T_DEF_CMD,	"REMOVE",	"REMOVE|RM",
		    "wwt REMOVE id6...",
		"Remove discs from WBFS partitions." },

  { T_DEF_CMD,	"RENAME",	"RENAME|REN",
		    "wwt RENAME SELECTOR=[new_id6][,new_title]...\n"
		    "SELECTOR := ID6 | '#'SLOT | INDEX",
		"Rename the ID6 and/or the disc title of WBFS discs." },

  { T_DEF_CMD,	"SETTITLE",	"SETTITLE|ST",
		    "wwt SETTITLE id6=title...",
		"Set the disc title of WBFS discs." },

  { T_DEF_CMD,	"TOUCH",	"TOUCH",
		    "wwt TOUCH id6...",
		"Set time stamps of WBFS discs." },

  { T_DEF_CMD,	"VERIFY",	"VERIFY|V",
		    "wwt VERIFY [id6]...",
		"Verify all discs of WBFS (calculate and compare SHA1 checksums)"
		" to find bad dumps." },

  { T_DEF_CMD,	"SKELETON",	"SKELETON|SKEL",
		    "wwt SKELETON [id6]...",
		"Create very small skeletons of ISO images."
		" A skeleton contains only disc and partition headers for further"
		" analysis and is not playable because all files are zeroed."
		"\1 Read https://wit.wiimm.de/cmd/wwt/skel for more details." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FILETYPE",	"FILETYPE|FTYPE",
		    "wwt FILETYPE filename...",
		"Print a status line for each source file." },

  //
  //---------- list of all wwt options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_OPT_GM,	"QUIET",	"q|quiet",
		0, 0 /* copy of wit */ },

  { T_OPT_GM,	"VERBOSE",	"v|verbose",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"PROGRESS",	"P|progress",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"SCAN_PROGRESS","scan-progress|scanprogress",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"LOGGING",	"L|logging",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"ESC",		"E|esc",
		0, 0 /* copy of wit */ },

  { T_OPT_GO|F_IGNORE,
		"COLOR",	"color",
		0, 0 /* copy of wit */ },

  { T_OPT_G|F_IGNORE,
		"COLOR_256",	"256-colors|256colors",
		0, 0 /* copy of wit */ },

  { T_OPT_G|F_IGNORE,
		"NO_COLOR",	"no-colors|nocolors",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"IO",		"io",
		0, 0 /* copy of wit */ },

  { T_OPT_GO,	"DSYNC",	"dsync",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GMP,	"TITLES",	"T|titles",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"UTF_8",	"utf-8|utf8",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"NO_UTF_8",	"no-utf-8|no-utf8|noutf8",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"LANG",		"lang",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GM,	"TEST",		"t|test",
		0, 0 /* copy of wit */ },

#if OPT_OLD_NEW
  { H_OPT_GM,	"OLD",		"OLD",
		0, 0 /* copy of wit */ },

  { H_OPT_GM,	"NEW",		"NEW",
		0, 0 /* copy of wit */ },
#endif

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"AUTO",		"a|auto",
		0,
		"Search for WBFS partitions using '/proc/partitions'"
		" or searching hard disks in '/dev/'." },

  { T_OPT_C,	"ALL",		"A|all",
		0,
		"Use all WBFS partitions found." },

  { T_OPT_CMP,	"PART",		"p|part",
		"part",
		"Define a primary WBFS file or partition. Multiple usage possible." },

  { T_OPT_CMP,	"SOURCE",	"source",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"NO_EXPAND",	"no-expand|noexpand",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"RECURSE",	"r|recurse",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"RDEPTH",	"rdepth",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"PSEL",		"psel",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"RAW",		"raw",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"WBFS_ALLOC",	"wbfs-alloc|wbfsalloc",
		"mode",
		"Select one WBFS block allocation modus:"
		"\n "
		" @FIRST@: Use always the first free WBFS block."
		" @NO-FRAG@: Find large blocks to minimize fragmentation."
		" @AUTO@ (=default): Use modus @NO-FRAG@ for partitions >=20GiB"
		" and modus @FIRST@ for partitions <20GiB and for files." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"EXCLUDE",	"x|exclude",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"EXCLUDE_PATH",	"X|exclude-path|excludepath",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"INCLUDE",	"n|include",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"INCLUDE_PATH","N|include-path|includepath",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"INCLUDE_FIRST","include-first|includefirst",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ONE_JOB",	"1|one-job|onejob",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"JOB_LIMIT",	"job-limit|joblimit",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE",	"i|ignore",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE_FST",	"ignore-fst|ignorefst",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"IGNORE_SETUP",	"ignore-setup|ignoresetup",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"LINKS",	"links",
		0, 0 /* copy of wit */ },

  { H_OPT_C,	"USER_BIN",	"user-bin|userbin",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"PMODE",	"pmode",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"FLAT",		"flat",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"COPY_GC",	"copy-gc|copygc",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"NO_LINK",	"no-link|nolink",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"NEEK",		"neek|sneek",
		0, 0 /* copy of wit */ },

  { H_OPT_G,	"HOOK",		"hook",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ENC",		"enc",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MODIFY",	"modify",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"NAME",		"name",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ID",		"id",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DISC_ID",	"disc-id|discid",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"BOOT_ID",	"boot-id|bootid",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"TICKET_ID",	"ticket-id|ticketid",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"TMD_ID",	"tmd-id|tmdid",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"TT_ID",	"tt-id|ttid",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"WBFS_ID",	"wbfs-id|wbfsid",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"REGION",	"region",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"COMMON_KEY",	"common-key",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"IOS",		"ios",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"HTTP",		"http",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DOMAIN",	"domain",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"WIIMMFI",	"wiimmfi",
		0, 0 /* copy of wit */ },

  { H_OPT_C,	"TWIIMMFI",	"twiimmfi",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"RM_FILES",	"rm-files|rm-file|rmfiles|rmfile",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ZERO_FILES",	"zero-files|zero-file|zerofiles|zerofile",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"REPL_FILE",	"repl-file|repl-files|replfile|replfiles",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"ADD_FILE",	"add-file|add-files|addfile|addfiles",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"IGNORE_FILES",	"ignore-files|ignore-file|ignorefiles|ignorefile",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"TRIM",		"trim",
		0, 0 /* copy of wit */ },

  { H_OPT_CP,	"ALIGN",	"align",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"ALIGN_PART",	"align-part|alignpart",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ALIGN_FILES",	"align-files|alignfiles",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DEST",		"d|dest",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"AUTO_SPLIT",	"auto-split|autosplit",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"NO_SPLIT",	"no-split|nosplit",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SPLIT",	"z|split",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"DISC_SIZE",	"disc-size|discsize",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"PREALLOC",	"prealloc",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"TRUNC",	"trunc",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chs",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"COMPRESSION",	"compression|compr",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MEM",		"mem",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SIZE",		"s|size",
		"size",
		"Define then total size of a WBFS file."
		" @'size'@ is a floating number optionally followed"
		" by one of the single letter factors 'kKmMgGtT'."
		" This value is only used while creating a new WBFS file." },

  { T_OPT_CP,	"HSS",		"hss|sector-size|sectorsize",
		"size",
		"Define HD sector size."
		" The parameter 'size' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or"
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" Only power of 2 values larger or equal 512 are accepted."
		" The default value is 512."  },

  { T_OPT_CP,	"WSS",		"wss",
		"size",
		"Define WBFS sector size."
		" The parameter 'size' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or"
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" Only power of 2 values larger or equal 1024 are accepted."
		" If not set the WBFS sector size is calculated automatically." },

  { T_OPT_C,	"RECOVER",	"recover",
		0,
		"Format a WBFS in recover mode:"
		" Write the WBFS sector, but don't reset the disc info area."
		" Then look into each disc slot to find valid discs and restore them." },

  { T_OPT_G,	"FORCE",	"f|force",
		0, "Force operation." },

  { T_OPT_C,	"NO_CHECK",	"no-check|nocheck",
		0, "Disable automatic check of WBFS before modificastions." },

  { T_OPT_CP,	"REPAIR",	"repair",
		"mode",
		TEXT_WWT_OPT_REPAIR },

  { T_OPT_C,	"NO_FREE",	"no-free|nofree",
		0,
		"The discs are only dropped (slot is marked free),"
		" but the correspondent blocks are not freed."
		" You should run CHECK or REPAIR to repair the WBFS"
		" after using this option." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"UPDATE",	"u|update",
		0, "Copy only non existing discs." },

  { T_OPT_C,	"SYNC",		"y|sync",
		0,
		"Synchronize the destination with all sources:"
		" Remove and copy discs until the destination WBFS"
		" contains exactly the same discs as all sources together."
		" Remove and add only discs in respect to the"
		" include and exclude lists."},

  { T_OPT_C,	"SYNC_ALL",	"sync-all|syncall",
		0,
		"Synchronize the destination with all sources like {--sync}."
		" But use the include and exclude lists only to create the"
		" sync list and remove all disc on the destination WBFS"
		" that are not part of the sync list." },

  { T_OPT_C,	"NEWER",	"e|newer|new",
		0,
		"If source and destination have valid mtimes:"
		" Copy only if source is newer." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"REMOVE",	"R|remove",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CO,	"WDF",		"W|wdf",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"WDF1",		"wdf1",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"WDF2",		"wdf2",
		0, 0 /* copy of wit */ },

		// [[2do]] 2014-01: wdf-align is obsolete at 2014-07
  { T_OPT_GP,	"ALIGN_WDF",	"align-wdf|alignwdf|wdf-align|wdfalign",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"WIA",		"wia",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"GCZ",		"gcz",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"GCZ_ZIP",	"gcz-zip|gczzip|gczip",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"GCZ_BLOCK",	"gcz-block|gczblock",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ISO",		"I|iso",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"CISO",		"C|ciso",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"WBFS",		"B|wbfs",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"FST",		"fst",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"FILES",	"files",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"ITIME",	"itime",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"MTIME",	"mtime",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"CTIME",	"ctime",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"ATIME",	"atime",
		0, 0 /* copy of wit */ },

  { T_OPT_CMP,	"TIME",		"time",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SET_TIME",	"set-time|settime",
		"time", "Use given time instead of current time." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CM,	"LONG",		"l|long",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SHOW",		"+|show",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"FRAGMENTS",	"fragments|frag",
		0,
		"Print fragmentation info instead of region info."
		" --frag is a short cut." },

  { T_OPT_C,	"NUMERIC",	"numeric",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"TECHNICAL",	"technical|tech",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"INODE",	"inode",
		0,
		"Print information for all inodes (invalid discs too)." },

  { T_OPT_C,	"MIXED",	"M|mixed",
		0, "Print disc infos of all WBFS in one combined table." },

  { T_OPT_C,	"UNIQUE",	"U|unique",
		0, "Eliminate multiple entries with same values." },

  { T_OPT_C,	"NO_HEADER",	"H|no-header|noheader",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"OLD_STYLE",	"old-style|oldstyle",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SECTIONS",	"sections",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SORT",		"S|sort",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"LIMIT",	"limit",
		0, 0 /* copy of wit */ },


  //
  //---------- wwt GROUP TITLES ----------

  { T_GRP_BEG,	"TITLES",	0,0,0 },

  { T_COPT_M,	"TITLES",	0,0,0 },
  { T_COPT,	"UTF_8",	0,0,0 },
  { T_COPT,	"NO_UTF_8",	0,0,0 },
  { T_COPT,	"LANG",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP READ_WBFS ----------

  { T_GRP_BEG,	"READ_WBFS",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPT,	"ALL",		0,0,0 },
  { T_COPT_M,	"PART",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP MOD_WBFS ----------

  { T_GRP_BEG,	"MOD_WBFS",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPT,	"ALL",		0,0,0 },
  { T_COPT_M,	"PART",		0,0,0 },
  { H_COPT,	"FORCE",	0,0,
	"This option is not longer needed because of the new WBFS interface." },
  { T_COPT,	"NO_CHECK",	0,0,
	"Disable automatic check of WBFS before modifications." },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP FST_OPTIONS ----------

  { T_GRP_BEG,	"FST_OPTIONS",	0,0,0 },

  { T_COPT,	"IGNORE_FST",	0,0,0 },
  { T_COPT,	"IGNORE_SETUP",	0,0,0 },
  { T_COPT,	"LINKS",	0,0,0 },
  { H_COPT,	"USER_BIN",	0,0,0 },

  //---------- wwt GROUP EXCLUDE ----------

  { T_GRP_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT,	"INCLUDE_FIRST",0,0,0 },
  { T_COPT_M,	"ONE_JOB",	0,0,0 },
  { T_COPT_M,	"JOB_LIMIT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP IGN_EXCLUDE ----------

  { T_GRP_BEG,	"IGN_EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT,	"INCLUDE_FIRST",0,0,0 },
  { T_COPT_M,	"ONE_JOB",	0,0,0 },
  { T_COPT_M,	"JOB_LIMIT",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wwt GROUP VERBOSE ----------

  { T_GRP_BEG,	"VERBOSE",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Show a runtime summary for each job."
	" If set twice enable progress information."
	" If set three times the progress information is more detailed." },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SCAN_PROGRESS",0,0,0 },

  //---------- wwt GROUP XTIME ----------

  { T_GRP_BEG,	"XTIME",	0,0,0 },

  { T_COPT,	"ITIME",	0,0,0 },
  { T_COPT,	"MTIME",	0,0,0 },
  { T_COPT,	"CTIME",	0,0,0 },
  { T_COPT,	"ATIME",	0,0,0 },

  //---------- wwt GROUP TIME ----------

  { T_GRP_BEG,	"TIME",		0,0,0 },

  { T_COPY_GRP,	"XTIME",	0,0,0 },
  { T_COPT_M,	"TIME",		0,0,0 },

  //---------- wwt GROUP OUTMODE_EDIT ----------

  { T_GRP_BEG,	"OUTMODE_EDIT",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { H_COPT,	"WDF1",		0,0,0 },
  { H_COPT,	"WDF2",		0,0,0 },
  { H_COPT,	"ALIGN_WDF",	0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },

  //---------- wwt GROUP OUTMODE ----------

  { T_GRP_BEG,	"OUTMODE",	0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT,	"WIA",		0,0,0 },
  { T_COPT,	"GCZ",		0,0,0 },
  { T_COPT,	"GCZ_ZIP",	0,0,0 },
  { T_COPT,	"GCZ_BLOCK",	0,0,0 },
  { T_COPT,	"FST",		0,0,0 },

  //---------- wwt GROUP PARTITIONS ----------

  { T_GRP_BEG,	"PARTITIONS",	0,0,0 },

  { T_COPT,	"PSEL",		0,0,0 },
  { T_COPT,	"RAW",		0,0,0 },

  //---------- wwt GROUP FST_SELECT ----------

  { T_GRP_BEG,	"FST_SELECT",	0,0,0 },

  { T_COPT,	"PMODE",	0,0,0 },
  { T_COPT,	"FLAT",		0,0,0 },
  { T_COPT_M,	"FILES",	0,0,0 },
  { T_COPT,	"COPY_GC",	0,0,0 },
  { T_COPT,	"NO_LINK",	0,0,0 },
  { T_COPT,	"NEEK",		0,0,0 },

  //---------- wwt GROUP PATCH ----------

  { T_GRP_BEG,	"PATCH",	0,0,0 },

  { H_COPT,	"HOOK",		0,0,0 },
  { T_COPT,	"ENC",		0,0,0 },
  { T_COPT,	"MODIFY",	0,0,0 },
  { T_COPT,	"NAME",		0,0,0 },
  { T_COPT,	"ID",		0,0,0 },
  { T_COPT,	"DISC_ID",	0,0,0 },
  { T_COPT,	"BOOT_ID",	0,0,0 },
  { T_COPT,	"TICKET_ID",	0,0,0 },
  { T_COPT,	"TMD_ID",	0,0,0 },
  { T_COPT,	"TT_ID",	0,0,0 },
  { T_COPT,	"WBFS_ID",	0,0,0 },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"COMMON_KEY",	0,0,0 },
  { T_COPT,	"IOS",		0,0,0 },
  { T_COPT,	"HTTP",		0,0,0 },
  { T_COPT,	"DOMAIN",	0,0,0 },
  { T_COPT,	"WIIMMFI",	0,0,0 },
  { H_COPT,	"TWIIMMFI",	0,0,0 },
  { T_COPT,	"RM_FILES",	0,0,0 },
  { T_COPT,	"ZERO_FILES",	0,0,0 },

  //---------- wwt GROUP RELOCATE ----------

  { T_GRP_BEG,	"RELOCATE",	0,0,0 },

  { H_COPT,	"REPL_FILE",	0,0,0 },
  { H_COPT,	"ADD_FILE",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },
  { H_COPT,	"TRIM",		0,0,0 },
  { H_COPT,	"ALIGN",	0,0,0 },
  { T_COPT,	"ALIGN_PART",	0,0,0 },
  { T_COPT,	"ALIGN_FILES",	0,0,0 },

  //---------- wwt GROUP SPLIT_CHUNK ----------

  { T_GRP_BEG,	"SPLIT_CHUNK",	0,0,0 },

  { T_COPT,	"AUTO_SPLIT",	0,0,0 },
  { T_COPT,	"NO_SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"DISC_SIZE",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },
  { H_COPT,	"COMPRESSION",	0,0,0 },
  { H_COPT,	"MEM",		0,0,0 },


  //
  //---------- COMMAND wwt VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,0 },

  //---------- COMMAND wwt HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_COPT,	"WIDTH",	0,0,0 },
  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wwt INFO ----------

  { T_CMD_BEG,	"INFO",		0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0, CMD_INFO_LONG },

  //---------- COMMAND wwt TEST ----------

  { T_CMD_BEG,	"TEST",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wwt ERROR ----------

  { T_CMD_BEG,	"ERROR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print a message text instead of a message name." },

  //---------- COMMAND wwt COMPR ----------

  { T_CMD_BEG,	"COMPR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print a table with the normalized mode name,"
	" compression level, chunk size factor and memory usage." },
  { T_COPT,	"VERBOSE",	0,0,
	"Print always compression level and chunk size factor."
	" Standard is to suppress these values if not explicitly set." },
  { T_COPT,	"NUMERIC",	0,0,0 },

  //---------- COMMAND wwt FEATURES ----------

  { T_CMD_BEG,	"FEATURES",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,
	"If set, suppress messages about the single features."
	" If set twice, suppress also the summary message." },
  { T_COPT,	"VERBOSE",	0,0,
	"Normally, a status line for each supported feature is printed."
	" If set, print an info line for each requested feature." },

  //---------- COMMAND wwt EXCLUDE ----------

  { T_CMD_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  //---------- COMMAND wwt TITLES ----------

  { T_CMD_BEG,	"TITLES",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  //---------- COMMAND wwt GETTITLES ----------

  { T_CMD_BEG,	"GETTITLES",	0,0,0 },

  //---------- COMMAND wwt FIND ----------

  { T_CMD_BEG,	"FIND",		0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,
	"Be absolute quiet and report the find status as exit code only." },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"Without @--long@ only partition names of WBFS partitions are printed."
	" If option @--long@ is set then additional infos are printed for"
	" each partition, WBFS or not (this includes output via --sections)."
	" If option @--long@ is set at least twice"
	" the real path and the size in bytes are printed." },
  { T_COPT,	"OLD_STYLE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  //---------- COMMAND wwt SPACE ----------

  { T_CMD_BEG,	"SPACE",	0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	" If option @--long@ is set the real path is printed." },

  //---------- COMMAND wwt ANALYZE ----------

  { T_CMD_BEG,	"ANALYZE",	0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"If option @--long@ is set then calculated values"
	" are printed too if other values are available."
	" If option @--long@ is set twice calculated values are always printed." },

  //---------- COMMAND wwt DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"INODE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set then print a status for each valid disc within WBFS"
	" ({--show=FILES})."
	" If set twice print a memory map for each disc is printed"
	" too ({--show=D-MAP})."
	" If set three times print an additional memory map for the"
	" whole WBFS ({--show=W-MAP})."
	" If set four times print an usage table ({--show=USAGE})." },
  { T_COPT,	"SHOW",		0,0,0 },

  //---------- COMMAND wwt ID6 ----------

  { T_CMD_BEG,	"ID6",		0,0,0 },

  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  //---------- COMMAND wwt LIST ----------

  { T_CMD_BEG,	"LIST",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_COPT,	"MIXED",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0,0 },
  { T_COPT,	"SORT",		0,0,0 },
  { T_COPY_GRP,	"TIME",		0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set, the size in MiB and the region is printed too."
	" If set twice, a date column is added."
	" If set three times, a date and a time column is added." },
  { T_COPT,	"FRAGMENTS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  //---------- COMMAND wwt LIST-L ----------

  { T_CMD_BEG,	"LIST_L",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-LL ----------

  { T_CMD_BEG,	"LIST_LL",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-LLL ----------

  { T_CMD_BEG,	"LIST_LLL",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-A ----------

  { T_CMD_BEG,	"LIST_A",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-M ----------

  { T_CMD_BEG,	"LIST_M",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-U ----------

  { T_CMD_BEG,	"LIST_U",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt LIST-F ----------

  { T_CMD_BEG,	"LIST_F",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wwt FORMAT ----------

  { T_CMD_BEG,	"FORMAT",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  { T_COPT,	"VERBOSE",	0,0,
	"Be verbose and explain the actions." },
  { T_COPT,	"SIZE",		0,0,0 },
  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"HSS",		0,0,0 },
  { T_COPT,	"WSS",		0,0,0 },
  { T_COPT,	"RECOVER",	0,0,0 },
  { T_COPT,	"INODE",	0,0,
	"Force creating inode infos with predefined timestamps."
	" The timestamps reduce effect of sparse files."
	" This option is set for devices automatically." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,
	"Force test mode and ignore {--force}." },
  { T_COPT,	"FORCE",	0,0,
	"This option is needed for leaving test mode and for real formatting!" },

  //---------- COMMAND wwt RECOVER ----------

  { T_CMD_BEG,	"RECOVER",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },
  { T_COPT,	"TEST",		0,0,0 },

  //---------- GROUP wwt CHECK ----------

  { T_GRP_BEG,	"CHECK",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Be verbose."
	" If set once then more details are printed."
	" If set twice a info dump of all corrupted discs is included."
	" If set three times a info dump of all discs is included if a error is found."
	" If set four times a full memory map is included." },

  { T_COPT_M,	"LONG",		0,0,
	"Option @--long@ does the same as option {--verbose}."
	" If set at least once it overwrites the {--verbose} level." },

  //---------- COMMAND wwt CHECK ----------

  { T_CMD_BEG,	"CHECK",	0,0,0 },

  { T_COPY_GRP,	"CHECK",	0,0,0 },
  { T_COPT,	"REPAIR",	0,0,
	TEXT_WWT_OPT_REPAIR " The default is 'NONE'." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt REPAIR ----------

  { T_CMD_BEG,	"REPAIR",	0,0,0 },

  { T_COPY_GRP,	"CHECK",	0,0,0 },
  { T_COPT,	"REPAIR",	0,0,
	TEXT_WWT_OPT_REPAIR " The default is 'STANDARD' (FBT,INODES)." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt EDIT ----------

  { T_CMD_BEG,	"EDIT",		0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPT,	"PART",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,
	"Force test mode and ignore {--force}." },
  { T_COPT,	"FORCE",	0,0,
	"This option is needed for leaving test mode and for formatting!" },

  //---------- COMMAND wwt PHANTOM ----------

  { T_CMD_BEG,	"PHANTOM",	0,0,0 },

  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPT,	"WBFS_ALLOC",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Print a status line for each added disc." },
  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt TRUNCATE ----------

  { T_CMD_BEG,	"TRUNCATE",	0,0,0 },

  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"TEST",		0,0,0 },

  //---------- COMMAND wwt SYNC ----------

  { T_CMD_BEG,	"SYNC",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPT,	"WBFS_ALLOC",	0,0,0 },

  { T_COPT_M,	"SOURCE",	0,0,0 },
  { T_COPT,	"NO_EXPAND",	0,0,0 },
  { T_COPT_M,	"RECURSE",	0,0,0 },
  { T_COPT,	"RDEPTH",	0,0,0 },
  { T_COPY_GRP,	"IGN_EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,
	"Truncate WBFS until operation finished." },
  { T_COPT,	"NEWER",	0,0,0 },
  { T_COPT,	"SYNC_ALL",	0,0,0 },

  //---------- COMMAND wwt UPDATE ----------

  { T_CMD_BEG,	"UPDATE",	0,0,0 },

  { T_COPY_CMD,	"SYNC",		0,0,0 },
  { T_COPT,	"SYNC",		0,0,0 },

  //---------- COMMAND wwt NEWER ----------

  { T_CMD_BEG,	"NEW",		0,0,0 },

  { T_COPY_CMD,	"UPDATE",	0,0,0 },

  //---------- COMMAND wwt ADD ----------

  { T_CMD_BEG,	"ADD",		0,0,0 },

  { T_COPY_CMD,	"UPDATE",	0,0,0 },
  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,
	"Overwrite already existing discs." },

  //---------- COMMAND wwt DUP ----------

  { T_CMD_BEG,	"DUP",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"READ_WBFS",	0,0,0 },

  { T_COPT,	"NO_CHECK",	0,0,
	"Disable automatic check of WBFS before reading." },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"DEST",		0,0,
	"Define the path to the destination file."
	" If not set, the last parameter is used as destination." },
  { T_COPT_M,	"DEST2",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },


  //---------- COMMAND wwt EXTRACT ----------

  { T_CMD_BEG,	"EXTRACT",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0, TEXT_EXTRACT_LONG },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },
  { T_COPT,	"LINKS",	0,0,0 },
  { H_COPT,	"USER_BIN",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,
	"Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },
  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },
  { T_COPT,	"TRUNC",	0,0,0 },


  //---------- COMMAND wwt SCRUB ----------

  { T_CMD_BEG,	"SCRUB",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0, TEXT_EXTRACT_LONG },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },


  //---------- COMMAND wwt REMOVE ----------

  { T_CMD_BEG,	"REMOVE",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,
	"Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },
  { T_COPT,	"NO_FREE",	0,0,0 },

  //---------- COMMAND wwt RENAME ----------

  { T_CMD_BEG,	"RENAME",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },
  { T_COPT,	"ISO",		0,0,
	"Modify ID and title of the ISO image."
	" If neither @--iso@ nor {--wbfs} is set, then both are assumed as active." },
  { T_COPT,	"WBFS",		0,0,
	"Modify ID and title of the inode in the WBFS management area."
	" If neither {--iso} nor @--wbfs@ is set, then both are assumed as active." },

  //---------- COMMAND wwt SETTITLE ----------

  { T_CMD_BEG,	"SETTITLE",	0,0,0 },
  { T_COPY_CMD,	"RENAME",	0,0,0 },

  //---------- COMMAND wwt TOUCH ----------

  { T_CMD_BEG,	"TOUCH",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"UNIQUE",	0,0,
	"Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0,
	"Ignore non existing discs without any warning." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ITIME",	0,0, "Touch the 'itime' (insertion time)." },
  { T_COPT,	"MTIME",	0,0, "Touch the 'mtime' (last modification time)." },
  { T_COPT,	"CTIME",	0,0, "Touch the 'ctime' (last status change time)." },
  { T_COPT,	"ATIME",	0,0, "Touch the 'atime' (last access time)." },
  { T_COPT,	"SET_TIME",	0,0,0 },

  //---------- COMMAND wwt VERIFY ----------

  { T_CMD_BEG,	"VERIFY",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPY_GRP,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"LIMIT",	0,0,
	"Maximal printed errors of each partition."
	" A zero means unlimited. The default is 10." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0, "Eliminate multiple ID6 from the source list." },
  { T_COPT,	"IGNORE",	0,0, "Ignore non existing discs without any warning." },
  { T_COPT,	"REMOVE",	0,0, "Remove bad discs from WBFS." },
  { T_COPT,	"NO_FREE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"On error print an additional line to localize the exact"
	" position where the error is found."
	" If set twice a hex dump of the hash values is printed too." },
  { T_COPT_M,	"TECHNICAL",	0,0,0 },


  //---------- COMMAND wwt SKELETON ----------

  { T_CMD_BEG,	"SKELETON",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"MOD_WBFS",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0, "Ignore non existing discs without any warning." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT_M,	"DEST",		0,0,0 },
  { T_COPT_M,	"DEST2",	0,0,0 },


  //---------- COMMAND wwt FILETYPE ----------

  { T_CMD_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT_M,	"IGNORE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set then ID6 and split file count are printed too."
	" If set twice the region is printed too." },

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
