
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
/////////////			    TOOL wit			///////////////
///////////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wit", 0,
		"wit [option]... command [option|parameter|@file]...",
		"Wiimms ISO Tool :"
		" It can list, analyze, verify, convert, split, join,"
		" patch, mix, extract, compose, rename and compare"
		" Wii and GameCube images."
		" It also can create and dump different other Wii file formats." },

  //
  //---------- list of all wit commands ----------

  { T_DEF_CMD,	"VERSION",	"VERSION",
		    "wit VERSION [ignored]...",
		"Print program name and version and exit." },

  { T_DEF_CMD,	"HELP",		"HELP|H|?",
		    "wit HELP [command] [ignored]...",
		"Print help and exit."
		" If the first non option is a valid command name,"
		" then a help for the given command is printed." },

  { T_DEF_CMD,	"INFO",		"INFO",
		    "wit INFO [keyword]...",
		"Print some internal information about the keywords."
		" If the keyword @ALL@ is set or no keyword is entered,"
		" then all informations are printed."
		" Possible keywords are: @IMAGE-FORMAT@ and @ALL@." },

  #define CMD_INFO_LONG \
		"Print additional infos, if available." \
		" For @IMAGE-FORMAT@: Print a list, that explains the attributes."

  { T_DEF_CMD,	"TEST",		"TEST",
		    "wit TEST [ignored]...",
		"Test options: All options are allowed, some are printed." },

  { T_DEF_CMD,	"ERROR",	"ERROR|ERR",
		    "wit ERROR [error_code]",
		"Translate an exit code to a message name."
		" If no exit code is entered, print a table with all error messages." },

  { T_DEF_CMD,	"COMPR",	"COMPR",
		    "wit COMPR [mode]...",
		"Scan compression modes and print the normalized names."
		" See option {--compression} for syntax details."
		" If no mode is given than print a table"
		" with all available compression modes and alternative mode names." },

  { T_DEF_CMD,	"FEATURES",	"FEATURES",
		    "wit FEATURES [feature]...",
		"Check, if the requested features are available."
		" All keywords are possible."
		" If no keyword is used, all supported features are printed."
		" The exit status is 0 for 'all features supported',"
		" 1 for 'some features supported' and 2 for 'no feature supported'." },

  { T_DEF_CMD,	"ANAID",	"ANAID",
		    "wit ANAID [id]...",
		"Analyze the entered IDs and print one line for each ID"
		" with 3 columns: HEX_ID, ASCII_ID, Game title." },

  { T_DEF_CMD,	"EXCLUDE",	"EXCLUDE",
		    "wit EXCLUDE [additional_excludes]...",
		"Dump the internal exclude database to standard output (stdout)." },

  { T_DEF_CMD,	"TITLES",	"TITLES",
		    "wit TITLES [additional_title_file]...",
		"Dump the internal title database to standard output (stdout)." },

  { T_DEF_CMD,	"GETTITLES",	"GETTITLES",
		    "wit GETTITLES",
		"Call the script 'load-titles.sh' in the share folder"
		" to update the title database." },

  { T_DEF_CMD,	"CERT",		"CERT",
		    "wit CERT [additional_cert_file]...",
		"Collect certificates"
		" and eliminate multiple entires of the same certificate."
		" Dump all collected certificates to standard output (stdout)"
		" and/or write the certificate to a new binary cert file."
		" The optional parameters are handled like parameters of option {--cert}." },

  { T_DEF_CMD,	"CREATE",	"CREATE",
		    "wit CREATE TICKET outfile [--id id] [title_id] [decrypted_key]\n"
		    "wit CREATE TMD outfile [--id id] [--ios ios] [hash_val]",
		"Create a system file (TICKET or TMD)." },

  { T_DEF_CMD,	"DOLPATCH",	"DOLPATCH",
		    "wit DOLPATCH dolfile [sub_command]...",
		"Patch a dol file."
		" A sub-command is either (upper case are keywords)"
		" 'XML=filname' or 'NEW=type,addr,size'"
		" or 'NEW=type,AUTO ' or 'LOAD=addr,filename' or 'addr=patch[#cond]',"
		" and 'type' is either 'TEXT' or 'DATA'."
		"\1 Read https://wit.wiimm.de/cmd/wit/dolpatch for more details." },

  { H_DEF_CMD,	"CODE",		"_CODE",
		    "wit _CODE [files]",
		"All bytes of all input files are xored with 0xDC and copied to stdout."
		" If no source file is given, stdin is read." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"FILELIST",	"FILELIST|FLIST",
		    "wit FILELIST [source]...",
		"List all source files in a table." },

  { T_DEF_CMD,	"FILETYPE",	"FILETYPE|FTYPE",
		    "wit FILETYPE [source]...",
		"Print a status line for each source file." },

  { T_DEF_CMD,	"ISOSIZE",	"ISOSIZE|SIZE",
		"wit ISOSIZE [source]...",
		"Print a status line with size infos for each source file." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"DUMP",		"DUMP|D",
		    "wit DUMP [source]...",
		"Dump the data structure and content of Wii and GameCube ISO files,"
		" @cert.bin@, @ticket.bin@, @tmd.bin@,"
		" @header.bin@, @boot.bin@, @fst.bin@ and of DOL-files."
		" The file type is detected automatically by analyzing the content." },

  { T_DEF_CMD,	"ID6",		"ID6|ID",
		    "wit ID6 [source]...",
		"Print ID6 of all found ISO files as simple list." },

  { T_DEF_CMD,	"ID8",		"ID8",
		    "wit ID8 [source]...",
		"Print ID6 + 2 next bytes as disc and version number"
		" of all found ISO files as simple list." },

  { T_DEF_CMD,	"FRAGMENTS",	"FRAGMENTS",
		    "wit FRAGMENTS [source]...",
		"Print the image fragments and, if possible,"
		" their file system mapping and fragments."
		" If option --brief is set, only the fragment counts are printed."
		" The Mac version can only detect"
		" the file system mapping for WBFS partitions." },

  { T_DEF_CMD,	"LIST",		"LIST|LS",
		"wit LIST [source]...",
		"List all found ISO files." },

  { T_DEF_CMD,	"LIST_L",	"LIST-L|LL|LISTL",
		    "wit LIST-L [source]...",
		"List all found ISO files with size and region."
		" 'LIST-L' is a shortcut for {LIST --long}." },

  { T_DEF_CMD,	"LIST_LL",	"LIST-LL|LLL|LISTLL",
		    "wit LIST-LL [source]...",
		"List all found ISO files with date, size and region."
		" 'LIST-LL' is a shortcut for {LIST --long --long}." },

  { T_DEF_CMD,	"LIST_LLL",	"LIST-LLL|LLLL|LISTLLL",
		    "wit LIST-LLL [source]...",
		"List all found ISO files with date, size and region"
		" and add a second status line with more info."
		" 'LIST-LLL' is a shortcut for {LIST --long --long --long}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

	// [[2do]] old name 'ILIST' is obsolete since 2010-08-15

  { T_DEF_CMD,	"FILES",	"FILES|F|ILIST|IL",  
		    "wit FILES [source]...",
		"List all files of all discs." },

  { T_DEF_CMD,	"FILES_L",	"FILES-L|FL|FILESL|ILIST-L|ILL|ILISTL",
		    "wit FILES-L [source]...",
		"List all files of all discs."
		" 'FILES-L' is a shortcut for {FILES --long}." },

  { T_DEF_CMD,	"FILES_LL",	"FILES-LL|FLL|FILESLL|ILIST-LL|ILLL|ILISTLL",
		    "wit FILES-LL [source]...",
		"List all files of all discs."
		" 'FILES-LL' is a shortcut for {FILES --long --long}." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"DIFF",		"DIFF|CMP",
		    "wit DIFF source dest\n"
		    "wit DIFF [[--source|--recurse] source]... [-d|-D] dest",
		"DIFF compares ISO images in scrubbed or raw mode or on file level."
		" Images, WBFS partitions and directories are accepted as source."
		" DIFF works like {COPY} but comparing source and destination." },

  { T_DEF_CMD,	"FDIFF",	"FDIFF|FCMP",
		    "wit FDIFF source dest\n"
		    "wit FDIFF [[--source|--recurse] source]... [-d|-D] dest",
		"FDIFF compares ISO images on file level."
		" Images, WBFS partitions and directories are accepted as source."
		" 'FDIFF' is a shortcut for {DIFF --files +}." },

  { T_DEF_CMD,	"EXTRACT",	"EXTRACT|X",
		    "wit EXTRACT source dest\n"
		    "wit EXTRACT [[--source|--recurse] source]... [-d|-D] dest",
		"Extract all files of each source to new directory structures."
		" Images, WBFS partitions and directories are accepted as source."  },

  { T_DEF_CMD,	"COPY",		"COPY|CP",
		    "wit COPY source dest\n"
		    "wit COPY [[--source|--recurse] source]... [-d|-D] dest",
		"Copy, scrub, convert, join, split, compose, extract,"
		" patch, encrypt and decrypt Wii and GameCube disc images."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"CONVERT",	"CONVERT|CV|SCRUB|SB",
		    "wit CONVERT source\n"
		    "wit CONVERT [[--source|--recurse] source]...",
		"Convert, scrub, join, split, compose, extract,"
		" patch, encrypt and decrypt Wii and GameCube disc images"
		" and replace the source with the result."
		" Images, WBFS partitions and directories are accepted as source."
		" The former command name was @SCRUB@."
		"\n "
		" {wit CONVERT} is like {wit COPY} but removes the source"
		" and replace it with the new file if copying is successful."
		" It have been implemented as replacement of the @SCRUB@ command"
		" of other tools. {wit CONVERT} does more than only scrubbing"
		" and therefor it was renamed from @'SCRUB'@ to @'CONVERT'@,"
		" but the old command name is still allowed." },

  { T_DEF_CMD,	"EDIT",		"EDIT|ED",
		    "wit EDIT source\n"
		    "wit EDIT [[--source|--recurse] source]...",
		"Edit an existing Wii and GameCube ISO image"
		" and patch some values."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"IMGFILES",	"IMGFILES|IF",
		    "wit IMGFILES source\n"
		    "wit IMGFILES [[--source|--recurse] source]...",
		"Print a list of all image files including their associated split files."
		" Each file is printed on a separate line for further batch processing." },

  { T_DEF_CMD,	"REMOVE",	"REMOVE|RM",
		    "wit REMOVE source\n"
		    "wit REMOVE [[--source|--recurse] source]...",
		"Remove images including their associated split files." },

  { T_DEF_CMD,	"MOVE",		"MOVE|MV",
		    "wit MOVE source dest\n"
		    "wit MOVE [[--source|--recurse] source]... [-d|-D] dest",
		"Move and rename Wii and GameCube ISO images."
		" Images, WBFS partitions and directories are accepted as source." },

  { T_DEF_CMD,	"RENAME",	"RENAME|REN",
		    "wit RENAME id6=[new][,title]...",
		"Rename the ID6 of discs. Disc title can also be set." },

  { T_DEF_CMD,	"SETTITLE",	"SETTITLE|ST",
		    "wit SETTITLE id6=title...",
		"Set the disc title of discs." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"VERIFY",	"VERIFY|V",
		    "wit VERIFY [source]...",
		"Verify ISO images (calculate and compare SHA1 checksums)"
		" to find bad dumps." },

  { T_DEF_CMD,	"SKELETON",	"SKELETON|SKEL",
		    "wit SKELETON [source]...",
		"Create very small skeletons of ISO images."
		" A skeleton contains only disc and partition headers for further"
		" analysis and is not playable because all files are zeroed."
		"\1 Read https://wit.wiimm.de/cmd/wit/skel for more details." },

  { T_DEF_CMD,	"MIX",		"MIX",
		    "wit MIX SOURCE... --dest|--DEST outfile\n"
		    "  where SOURCE    = infile [QUALIFIER]...\n"
		    "  where QUALIFIER = 'select' part_type\n"
		    "                  | 'as' [part_table '.'] [part_type]\n"
		    "                  | 'ignore' ruleset\n"
		    "                  | 'header'\n"
		    "                  | 'region'\1\n"
		    "Read https://wit.wiimm.de/cmd/wit/mix for more details.",
		"Mix the partitions from different sources into one new"
		" Wii or GameCube disc." },

  //
  //---------- list of all wit options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0,
		"Stop parsing the command line, print a version info and exit." },

  { T_OPT_S,	"HELP",		"h|help",
		0,
		"Stop parsing the command line, print a help message and exit." },

  { T_OPT_S,	"XHELP",	"xhelp",
		0,
		"Stop parsing the command line and print a help message"
		" with all commands included. Exit after printing." },

  { T_OPT_GP,	"WIDTH",	"width",
		"width",
		"Define the width (number of columns) for help and some other messages"
		" and disable the automatic detection of the terminal width." },

  { T_OPT_GM,	"QUIET",	"q|quiet",
		0,
		"Be quiet and print only error messages." },

  { T_OPT_GM,	"VERBOSE",	"v|verbose",
		0,
		"Be verbose and print more progress information."
		" Multiple usage is possible:"
		" Progress counter is enabled if set at least two times."
		" Extended logging is enabled if set at least four times."
		" The impact of the other verbose levels are command dependent." },

  { T_OPT_G,	"PROGRESS",	"P|progress",
		0,
		"Print progress counter."
		" If --verbose is set at least twice, printing is enabled too."
		" If progress is enabled, the default of --dsync is changed."  },

  { T_OPT_G,	"SCAN_PROGRESS","scan-progress|scanprogress",
		0,
		"Print a message for each found image"
		" while scanning the file system." },

  { T_OPT_GM,	"LOGGING",	"L|logging",
		0,
		"This debug option enables the logging of internal memory maps."
		" If set twice second level memory maps are printed too." },

  { T_OPT_GP,	"ESC",		"E|esc",
		"char",
		"Define an alternative escape character for destination files."
		" The default is '%'."
		" For Windows (CYGWIN) it is a good choice to set @'-E$$'@"
		" to avoid conflicts with command shell variables." },

  { T_OPT_GO|F_IGNORE,
		"COLOR",	"color",
		"[=modus]",
		"Define the modus for colored text output."
		" Allowed keywords are:"
		" @OFF@ or @NO-COLORS@ to disable colors,"
		" @AUTO@ (default) for automatic detection,"
		" @ON@ for automatic detection but never @OFF@,"
		" @8-COLORS@ and @256-COLORS@ for 8 and 256 color support."
		" Without parameter, @ON@ is used."
		"\n "
		" @AUTO@ will enable colorized output only for terminals."
		" @AUTO@ and @ON@ use environment variable @TERM@"
		" to find the correct color modus."
		"\n "
		" If a command is prefixed by @'C-'@,"
		" then @--color=ON@ is used implicitly as default." },

  { T_OPT_G|F_IGNORE,
		"COLOR_256",	"256-colors|256colors",
		0,
		"Short cut for {--color=256-colors}:"
		" Force colorized text with 256 color support." },

  { T_OPT_G|F_IGNORE,
		"NO_COLOR",	"no-colors|nocolors",
		0,
		"Short cut for {--color=off}: Deactivate colorized text."
		" This is the default, if an output file is not a terminal." },

  { T_OPT_GP,	"IO",		"io",
		"flags",
		"Setup the IO mode for experiments."
		" The standard file IO is based on open() function."
		" The value '1' defines that WBFS IO is based on fopen() function."
		" The value '2' defines the same for ISO files"
		" and value '4' for WIA files."
		" You can combine the values by adding them." },

  { T_OPT_G,	"FORCE",	"f|force",
		0, "Force operation." },

  { T_OPT_GO,	"DSYNC",	"dsync",
		"[=mode]",
		"This option enables the usage of flag O_DSYNC"
		" when opening a partition at a hard drive for writing."
		" With activated flag, writing an image is some percent slower,"
		" but the progress counters are exact again."
		"\n "
		" Parameter MODE is optional."
		" If set, it is one of OFF (disable), ON (enable) or AUTO (default)."
		" With AUTO, DSYNC is enabled if the progress counters are active."
		"\n "
		" This option has only impact, if compiler and operation system"
		" support the flag O_DSYNC. Linux does." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GMP,	"TITLES",	"T|titles",
		"file",
		"Read file for disc titles."
		" @-T/@ disables automatic search for title files." },

  { T_OPT_G,	"UTF_8",	"utf-8|utf8",
		0,
		"Enables UTF-8 support for filenames (default)." },

  { T_OPT_G,	"NO_UTF_8",	"no-utf-8|no-utf8|noutf8",
		0, 
		"Disables UTF-8 support for filenames." },

  { T_OPT_GP,	"LANG",		"lang",
		"lang", 
		"Define the language for titles." },

  { T_OPT_GMP,	"CERT",		"cert",
		"file",
		"Scan a file for certificates"
		" and add them to the internal certificate database."
		" Valid sources are CERT, TICKET, TMD and ISO files."
		" All partitions of ISO images are scanned for certificates."
		" Files without certificates are ignored without notification." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GM,	"TEST",		"t|test",
		0,
		"Run in test mode, modify nothing."
		"\n"
		">>> USE THIS OPTION IF UNSURE! <<<" },

 #if OPT_OLD_NEW
  { H_OPT_GM,	"OLD",		"OLD",
		0,
		"Use old implementation if available." },

  { H_OPT_GM,	"NEW",		"NEW",
		0,
		"Use new implementation if available." },
 #endif

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"SOURCE",	"s|source",
		"path",
		"Use the entered file or directory as source."
		"\n "
		" Directories are expanded to all containing files"
		" but hidden files (file names begins with a point) are ignored."
		" If a command needs only images then non image files"
		" of the directory are ignored without notification."
		" The option {--no-expand} suppress the directory expansion." },

  { T_OPT_C,	"NO_EXPAND",	"no-expand|noexpand",
		0,
		"Do not expand directories to the containing files or images."
		" This option does not change the behavior of {--recurse}." },

  { T_OPT_CMP,	"RECURSE",	"r|recurse",
		"path",
		" If @path@ is not a directory, then it is used as a simple"
		" source file like {--source}."
		"\n "
		" Directories are scanned for source files recursively."
		" The option {--rdepth} limits the search depth."
		" Hidden files and hidden sub directories (file names begins"
		" with a point) and files with non supported file types (non ISO"
		" files for most commands) are ignored without notification." },

  { T_OPT_CP,	"RDEPTH",	"rdepth",
		"depth",
		"Set the maximum recurse depth for option {--recurse}."
		" The default search depth is 10." },

  { T_OPT_C,	"AUTO",		"a|auto",
		0,
		"Search WBFS partitions using '/proc/partitions'"
		" or searching hard disks in '/dev/' and use all readable as source."
		" This works like {wwt --auto --all}." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CMP,	"EXCLUDE",	"x|exclude",
		"id",
		"A comma separated list with ID4 and ID6 values is expected."
		" @'.'@ is a wildcard for exact 1 character and @'+'@"
		" is a wildcard for any number characters."
		" If the parameter begins with a '@@' the given file is read"
		" and each line is scanned for one ID."
		" Images with the given ID are excluded from operation."
		" Each use of this option expands the exclude list."
		" See {--include-first} for precedence issues." },

  { T_OPT_CMP,	"EXCLUDE_PATH",	"X|exclude-path|excludepath",
		"file_or_dir",
		"Scan the ID of the source and add it to the exclude list."
		" If the source is a directory then scan all images of the directory."
		" Images with the given ID are excluded from operation."
		" Each use of this option expands the exclude list."
		" See {--include-first} for precedence issues." },

  { T_OPT_CMP,	"INCLUDE",	"n|include",
		"id",
		"A comma separated list with ID values is expected."
		" @'.'@ is a wildcard for exact 1 character and @'+'@"
		" is a wildcard for any number characters."
		" If the parameter begins with a '@@' the given file is read"
		" and each line is scanned for one ID."
		" Only images with the given ID are included into the operation."
		" Each use of this option expands the include list."
		" See {--include-first} for precedence issues." },

  { T_OPT_CMP,	"INCLUDE_PATH",	"N|include-path|includepath",
		"file_or_dir",
		"Scan the ID of the source and add it to the include list."
		" If the source is a directory then scan all images of the directory."
		" Only images with the given ID are included into the operation."
		" Each use of this option expands the include list."
		" See {--include-first} for precedence issues." },

  { T_OPT_C,	"INCLUDE_FIRST","include-first|includefirst",
		0,
		"The options {--include}, {--include-path}, {--exclude} and"
		" {--exclude-path} decide which discs are included into the"
		" operation. If neither include nor exclude options are used,"
		" than all disc are included into the operation."
		" If only include options are used, than only the specified"
		" discs are operated. If only exclude options are used,"
		" than all all discs but not the excluded are operated."
		"\n "
		" If include and exclude options are used together and @--include-first@"
		" is not set, than all discs are operated that are specified by"
		" any include option and not by any exclude option."
		" If @--include-first@ is set, than all discs are ignored that"
		" are specified by any exclude option and not by any include option." },

  { T_OPT_C,	"ONE_JOB",	"1|one-job|onejob",
		0,
		"Execute only the first job and exit."
		" This is a shortcut for {--job-limit 1}." },

  { T_OPT_CP,	"JOB_LIMIT",	"job-limit|joblimit",
		"num",
		"Execute only the first @'num'@ jobs and exit."
		" If done without errors the exit status is OK (zero)." },

  { T_OPT_CP,	"FAKE_SIGN",	"fake-sign|fakesign",
		"ruleset",
		"Add a certificate selection rule."
		" All certificates that matches the rule set will be fake signed."
		"\1\n "
		" See https://wit.wiimm.de/info/file-filter.html"
		" for more details about filters." },

  { T_OPT_CM,	"IGNORE",	"i|ignore",
		0,
		"Ignore non existing files/discs without warning."
		" If set twice then all non Wii and GameCube ISO images are ignored too." },

  { T_OPT_C,	"IGNORE_FST",	"ignore-fst|ignorefst",
		0, 
		"Disable composing and ignore FST directories as input." },

  { T_OPT_C,	"IGNORE_SETUP",	"ignore-setup|ignoresetup",
		0, 
		"While composing ignore the file @'setup.txt'@,"
		" which defines some partition parameters." },

  { T_OPT_C,	"LINKS",	"links",
		0, 
		"Detect soft and hard linked source files while composing"
		" or extracting discs and try to create hard links instead"
		" of independent copies on the destination file system." },

  { H_OPT_C,	"USER_BIN",	"user-bin|userbin",
		0, 
		"Try to detect file '@sys/user.bin@'."
		" If available, use it like other system files." },

  { T_OPT_C,	"NULL",		"0|null",
		0,
		"Terminate each output line with a NUL (ASCII 0) character"
		" instead with a line feed (LF, ASCII 10)." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"PSEL",		"psel",
		"list",
		"This option set the scrubbing mode and defines,"
		" which disc partitions are handled."
		" It expects a comma separated list of keywords, numbers and names;"
		" all together called parameter. All parameters are case insensitive"
		" and non ambiguous abbreviations of keywords are allowed."
		"\n "
		" Each parameter becomes a rule and each rule is appended to a rule list."
		" Rules prefixed by a minus sign are DENY rules."
		" Rules prefixed by a plus sign or without a prefix are ALLOW rules."
		" Each partition is compared with each rule until a rule matches"
		" the partition. If a match it found, the partition is enabled"
		" for a ALLOW rule or disabled for a DENY rule."
		"\n "
		" The allowed keywords are: @DATA@, @UPDATE@, @CHANNEL@,"
		" @PTAB0@ .. @PTAB3@, @ID@, @ALL@, @WHOLE@ and @RAW@."
		" The following input formats are accepted too:"
		" @ptype@, @#index@, @#<index@, @#<=index@, @#>index@, @#>=index@"
		" and @#tab_index.part_index@."
		"\1\n "
		" See https://wit.wiimm.de/opt/psel for more details." },

  { T_OPT_C,	"RAW",		"raw",
		0, "Abbreviation of {--psel RAW}." },

  { T_OPT_CP,	"PMODE",	"pmode",
		"p-mode",
		"This options set the prefix mode for listed or extracted files."
		" One of the following values is allowed:"
		" @AUTO, NONE, POINT, ID, NAME, INDEX, COMBI@."
		" The default value is @'AUTO'@."
		"\1 See https://wit.wiimm.de/opt/pmode for more details." },

  { T_OPT_C,	"FLAT",		"flat",
		0,
		"While extracting a disc image strip all path names of the source file"
		" and store all files in the same directory."
		" This option sets the default for {--pmode} to @NONE@." },

  { T_OPT_C,	"COPY_GC",	"copy-gc|copygc",
		0,
		"If extracting a GameCube disc image, don't extract the real files"
		" to @'/files/...'@. Instead create a copy of the source image"
		" and store it as @'game.iso'@."
		" If the source image is already in this format,"
		" try to create a hard link and copy only if it fails." },

  { T_OPT_C,	"NO_LINK",	"no-link|nolink",
		0,
		"Don't try to create a hard link if copying the image"
		" forced by the option --copy-gc." },

  { T_OPT_C,	"NEEK",		"neek|sneek",
		0,
		"Abbreviation of {--psel data --pmode none --files :neek --copy-gc}."
		" The old name --sneek is accepted too." },

  { H_OPT_G,	"HOOK",		"hook",
		0,
		"Force relocation hook while reading iso images." },

  { T_OPT_CP,	"ENC",		"enc",
		"encoding",
		"Define the encoding mode."
		" The mode is one of NONE, HASHONLY, DECRYPT, ENCRYPT, SIGN or AUTO."
		" The case of the keywords is ignored."
		" The default mode is 'AUTO'." },

  { T_OPT_CP,	"MODIFY",	"modify",
		"list",
		" This $patching$ option defines the impact of the options {--name}"
		" and {--id}. It expects a comma separated list"
		" of the following keywords (case ignored) as parameter:"
		" @NONE, DISC, BOOT, TICKET, TMD, WBFS, TT, ALL@ and @AUTO@ (default)."
		"\n "
		" All keywords can be prefixed by @'+'@ to enable that option,"
		" by a @'-'@ to disable it or"
		" by a @'='@ to enable that option and disable all others." },

  { T_OPT_CP,	"NAME",		"name",
		"name",
		"This $patching$ option changes the name (disc title) of the disc"
		" to the given parameter. Up to 63 characters are expected."
		" The disc header and boot.bin are objects to modify."
		" The option {--modify} selects the objects." },

  { T_OPT_CP,	"ID",		"id",
		"id",
		"This $patching$ option changes the ID of the disc"
		" to the given parameter. 1 to 6 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The plus sign '+' is a wildcard for multiple '.'"
		" to fill the complete entered ID to 6 characters."
		" The disc header, boot.bin, ticket.bin and tmd.bin are"
		" objects to modify. The option {--modify} selects the objects."
		"\1\n"
		"See https://wit.wiimm.de/opt/id for more details." },

  { T_OPT_CP,	"DISC_ID",	"disc-id|discid",
		"id",
		"This $patching$ option changes the ID of the disc header"
		" to the given parameter. 1 to 6 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The plus sign '+' is a wildcard for multiple '.'"
		" to fill the complete entered ID to 6 characters."
		" Option --disc-id overrides the definition of option {--id}."
		"\1\n"
		"See https://wit.wiimm.de/opt/disc-id for more details." },

  { T_OPT_CP,	"BOOT_ID",	"boot-id|bootid",
		"id",
		"This $patching$ option changes the ID of boot.bin"
		" to the given parameter. 1 to 6 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The plus sign '+' is a wildcard for multiple '.'"
		" to fill the complete entered ID to 6 characters."
		" Option --boot-id overrides the definition of option {--id}."
		"\1\n"
		"See https://wit.wiimm.de/opt/boot-id for more details." },

  { T_OPT_CP,	"TICKET_ID",	"ticket-id|ticketid",
		"id",
		"This $patching$ option changes the ID of ticket.bin"
		" to the given parameter. 1 to 4 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The plus sign '+' is a wildcard for multiple '.'"
		" to fill the complete entered ID to 4 characters."
		" Option --ticket-id overrides the definition of option {--id}."
		"\1\n"
		"See https://wit.wiimm.de/opt/ticket-id for more details." },

  { T_OPT_CP,	"TMD_ID",	"tmd-id|tmdid",
		"id",
		"This $patching$ option changes the ID of tmd.bin"
		" to the given parameter. 1 to 4 characters are expected."
		" Only defined characters not equal '.' are modified."
		" The plus sign '+' is a wildcard for multiple '.'"
		" to fill the complete entered ID to 4 characters."
		" Option --tmd-id overrides the definition of option {--id}."
		"\1\n"
		"See https://wit.wiimm.de/opt/tmd-id for more details." },

  { T_OPT_CP,	"TT_ID",	"tt-id|ttid",
		"id",
		"This is a short cut for {--ticket id --tmd id}."
		" If TICKET and TMD differ, the game will freeze after loading."
		" So it makes only sense to change TICKET and TMD IDs together." },

  { T_OPT_CP,	"WBFS_ID",	"wbfs-id|wbfsid",
		"id",
		"This $patching$ option changes the ID of the WBFS header"
		" to the given parameter if adding a file to a WBFS"
		" or if creating a WBFS file. 1 to 6 characters are expected."
		" The already patched disc ID of the source is used as base"
		" and only defined characters not equal '.' are modified."
		" The plus sign '+' is a wildcard for multiple '.'"
		" to fill the complete entered ID to 6 characters."
		" Option --wbfs-id overrides the definition of option {--id}."
		"\1\n"
		"See https://wit.wiimm.de/opt/wbfs-id for more details." },

  { T_OPT_CP,	"REGION",	"region",
		"region",
		"This $patching$ option defines the region of the disc."
		" The region is one of @JAPAN, USA, EUROPE, KOREA, FILE@"
		" or @AUTO@ (default). The case of the keywords is ignored."
		" Unsigned numbers are also accepted." },

  { T_OPT_CP,	"COMMON_KEY",	"common-key",
		"index",
		"This $patching$ option defines the common key index as part"
		" of the TICKET. Keywords @0@, @STANDARD@, @1@ and @KOREAN@"
		" are accepted." },

  { T_OPT_CP,	"IOS",		"ios",
		"ios",
		"This $patching$ option defines the system version (IOS to load)"
		" within TMD. The format is @'HIGH:LOW'@ or @'HIGH-LOW'@ or @'LOW'@."
		" If only @LOW@ is set than @HIGH@ is assumed as 1 (standard IOS)." },

  { T_OPT_C,	"HTTP",		"http",
		0,
		"This $patching$ option replaces 'https' request to 'http'"
		" in the files 'main.dol' and 'rel/StaticR.rel', if the files exist."
		" It also replaces the sub-domain 'naswii' to 'nas'." },

  { T_OPT_CP,	"DOMAIN",	"domain",
		"domain",
		"This $patching$ option replaces the domain 'nintendowifi.net'"
		" by the new domain. The length of the new domain must not be"
		" larger than the old length (16 characters)."
		"\n "
		" If the new domain length is not larger than 11,"
		" then 'gamespy.com' of 'sake.gamespy.com' is replaced too. "
		" This is a special support for 'Super Smash Bros. Brawl'." },

  { T_OPT_C,	"WIIMMFI",	"wiimmfi",
		0,
		"Patch the images for $Wiimmfi$, the new custom server."
		" It is a short cut for {--http --domain wiimmfi.de}." },

  { H_OPT_C,	"TWIIMMFI",	"twiimmfi",
		0,
		"Patch the images for the test custom server."
		" It is a short cut for {--http --domain test.wiimmfi.de}." },

  { T_OPT_CP,	"RM_FILES",	"rm-files|rm-file|rmfiles|rmfile",
		"ruleset",
		"This patching option defines filter rules to remove real files"
		" and directories from the FST of the DATA partition."
		" $Fake signing$ of the TMD is necessary."
		" The processing order of file options is:"
		" {--rm-files --zero-files --ignore-files}."
		"\1\n "
		" See https://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

  { T_OPT_CP,	"ZERO_FILES",	"zero-files|zero-file|zerofiles|zerofile",
		"ruleset",
		"This patching option defines filter rules to zero (set size to zero)"
		" real files of the FST of the DATA partition."
		" $Fake signing$ of the TMD is necessary."
		" The processing order of file options is:"
		" {--rm-files --zero-files --ignore-files}."
		"\1\n "
		" See https://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

  { T_OPT_C,	"OVERLAY",	"overlay",
		0,
		"Most partitions have holes (unused areas) in the data section."
		" If combining multiple partitions into one disc it is possible"
		" to overlay the partitions so that the data of one partition"
		" resides in the hole of other partitions."
		" This option enables this feature."
		" It also limits the number of input partitions to 12,"
		" because the calculation is rated as O(2\1^N\2\4sup>N\4/sup>\3)."
		" 12 partitions can be combined in 479 millions permutations and"
		" all are tested with a back tracking algorithm to find the best one." },

  { H_OPT_CP,	"REPL_FILE",	"repl-file|repl-files|replfile|replfiles",
		"filedef",
		"This relocation option ???"
		" The processing order of file options is:"
		" {--rm-files --zero-files --repl-file --add-file --ignore-files}." },

  { H_OPT_CP,	"ADD_FILE",	"add-file|add-files|addfile|addfiles",
		"filedef",
		"This relocation option ???"
		" The processing order of file options is:"
		" {--rm-files --zero-files --repl-file --add-file --ignore-files}." },

  { T_OPT_CP,	"IGNORE_FILES",	"ignore-files|ignore-file|ignorefiles|ignorefile",
		"ruleset",
		"This option defines filter rules to ignore"
		" real files of the FST of the DATA partition."
		" $Fake signing$ is not necessary, but the partition becomes invalid,"
		" because the content of some files is not copied."
		" If such file is accessed the Wii will halt immediately,"
		" because the verification of the checksum calculation fails."
		" The processing order of file options is:"
		" {--rm-files --zero-files --ignore-files}."
		"\1\n "
		" See https://wit.wiimm.de/info/file-filter.html"
		" for more details about file filters." },

  { H_OPT_CP,	"TRIM",		"trim",
		"keylist",
		"This relocation option ???" },

  { H_OPT_CP,	"ALIGN",	"align",
		"size1[,size2][,size3]",
		"???" },

  { T_OPT_CP,	"ALIGN_PART",	"align-part|alignpart",
		"size",
		"If creating or moving partitions the beginning of each partition"
		" is set to an offset that is a multiple of the align size."
		" Size must be a power of 2 and at least 32 KiB (=default)." },

  { T_OPT_C,	"ALIGN_FILES",	"align-files|alignfiles",
		0,
		"If creating a partition the file 'align-files.txt' is read."
		" Files listed with values >=0x8000 (Wii sector size)"
		" are automatically aligned to 0x8000." },

  { T_OPT_CP,	"DEST",		"d|dest",
		"path",
		"Define a destination path (directory or file)."
		" The destination path is scanned for escape sequences"
		" (see option {--esc}) to allow generic paths." },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		"path",
		"Like {--dest}, but create the directory path automatically." },

  { T_OPT_C,	"AUTO_SPLIT",	"auto-split|autosplit",		// [[split]]
		0,
		"Enable auto split modus:"
		" Split only if necessary and determine the split size automatically."
		"\n "
		" THIS OPTION IS EXPERIMENTAL."
		" In future versions it becomes the default." },

  { T_OPT_C,	"NO_SPLIT",	"no-split|nosplit",		// [[split]]
		0,
		"Disable output file splitting."
		" This is the default, but in future versions,"
		" the new option {--auto-split} becomes the default." },

  { T_OPT_C,	"SPLIT",	"z|split",
		0,
		"Enable output file splitting. The default split size is 4 GB." },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		"sz",
		"Enable output file splitting and define a split size."
		" The parameter 'sz' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or"
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" The default unit is 'G' (GiB)." },

  { T_OPT_CP,	"DISC_SIZE",	"disc-size|discsize",
		"size",
		"Define a minimal (virtual) ISO disc size." },

  { T_OPT_CO,	"PREALLOC",	"prealloc",
		"[=mode]",
		"This option enables or disables the disc space preallocation."
		" If enabled the tools try to allocate disc space for the new files"
		" before writing the data. This reduces the fragmentation but also"
		" disables the sparse effect for preallocated areas."
		"\n "
		" The optional parameter decides the preallocation mode:"
		" @OFF@ (or @0@), @SMART@ (or @1@), @ALL@ (or @2@)."
		" If no parameter is set, @ALL@ is used."
		"\n "
		" Mode @'OFF'@ disables the preallocation."
		" Mode @'SMART'@ looks into the source disc to find out the writing areas."
		" @SMART@ is only available for $ISO$, $CISO$ and $WBFS$ file types."
		" For other file types @ALL@ is used instead."
		" Mode @'ALL'@ (the default) preallocate the whole destination file."
		" Because of the large holes in plain ISO images,"
		" the @SMART@ mode is used for ISOs instead." },

  { T_OPT_C,	"TRUNC",	"trunc",
		0,
		"Truncate $PLAIN ISO$ and $WBFS$ images after creating or copying"
		" to the minimal needed size with respect to the block size." },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		"mode", TEXT_OPT_CHUNK_MODE("ISO") },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chs",
		"sz",
		"Define the minimal chunk size if creating a CISO"
		" or WIA file (for WIA details see option --compression})."
		" The default is to calculate the chunk size from the input file size"
		" and find a good value by using a minimal value of 1 MiB"
		" for {--chunk-mode ISO} and @32 KiB@ for modes @32K@ and @POW2@."
		" For the modes @ISO@ and @POW2@ the value is rounded"
		" up to the next power of 2."
		" This calculation also depends from option {--max-chunks}."
		"\n "
		" The parameter 'sz' is a floating point number followed"
		" by an optional unit factor (one of 'cb' [=1] or"
		" 'kmgtpe' [base=1000] or 'KMGTPE' [base=1024])."
		" The default unit is 'M' (MiB)."
		" If the number is prefixed with a @'='@ then options"
		" {--chunk-mode} and {--max-chunks} are ignored"
		" and the given value is used without any rounding or changing."
		"\n "
		" If the input file size is not known (e.g. reading from pipe),"
		" its size is assumed as @12 GiB@."
		"\n "
		" @--chs@ is a shortcut for @--chunk-size@." },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		"n",
		"Define the maximal number of chunks if creating a CISO file."
		" The default value is 8192 for {--chunk-mode ISO}"
		" and 32760 (maximal value) for all other modes."
		" If this value is set than the automatic calculation"
		" of {--chunk-size} will be modified too."
		"\n "
		" @--mch@ is a shortcut for @--max-chunks@." },

  { T_OPT_CP,	"BLOCK_SIZE",	"block-size|blocksize",
		"size",
		TEXT_DIFF_BLOCK_SIZE },

  { T_OPT_CP,	"COMPRESSION",	"compression|compr",
		"mode",
		"Select one compression method, level and chunk size for new WIA files."
		" The syntax for mode is: @[method] [.level] [@@factor]@"
		"\n "
		" @'method'@ is the name of the method."
		" Possible compressions method are @NONE@, @PURGE@, @BZIP2@,"
		" @LZMA@ and @LZMA2@."
		" There are additional keywords: @DEFAULT@ (=@LZMA.5@@20@),"
		" @FAST@ (=@BZIP2.3@@10@), @GOOD@ (=@LZMA.5@@20@) @BEST@ (=@LZMA.7@@50@),"
		" and @MEM@ (use best mode in respect to memory limit set by {--mem})."
		" Additionally the single digit modes @0@ (=@NONE@),"
		" @1@ (=fast @LZMA@) .. @9@ (=@BEST@)are defined."
		" These additional keywords may change their meanings"
		" if a new compression method is implemented."
		"\n "
		" @'.level'@ is a point followed by one digit."
		" It defines the compression level."
		" The special value @.0@ means: Use default compression level (=@.5@)."
		"\n "
		" @'@@factor'@ is a factor for the chunk size. The base size is 2 MiB."
		" The value@ @@0@ is replaced by the default factor@ @@20@ (40 MiB)."
		" If the factor is not set but option {--chunk-size} is set,"
		" the factor will be calculated by using a rounded value of that option."
		"\n "
		" All three parts are optional."
		" All default values may be changed in the future."
		" @--compr@ is a shortcut for @--compression@"
		" and {--wia=mode} a shortcut for {--wia --compression mode}."
		" The command {wit COMPR} prints an overview about all compression modes." },

  { T_OPT_CP,	"MEM",		"mem",
		"size",
		"This option defines a memory usage limit for compressing files"
		" (in MiB if no other unit is entered)."
		" When compressing a file with method @MEM@ (see {--compression})"
		" the the compression method, level and chunk size"
		" are selected with respect to this limit."
		"\n "
		" If this option is not set or the value is 0,"
		" then the environment @$WIT_MEM$@ is tried to read instead."
		" If this fails, the tool tries to find out the total memory"
		" by reading @/proc/meminfo@."
		" The limit is set to 80% of the total memory minus 50 MiB." },

  { T_OPT_C,	"PRESERVE",	"p|preserve",
		0,
		"Preserve file times (atime+mtime) while copying an image."
		" This option is enabled by default if an unmodified disc image is copied." },

  { T_OPT_C,	"UPDATE",	"u|update",
		0,
		"Copy only files that do not exist."
		" Already existing files are ignored without warning." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0,
		"Overwrite already existing files without warning." },

  { T_OPT_C,	"DIFF",		"diff",
		0,
		"Diff source and destination after copying." },

  { T_OPT_C,	"REMOVE",	"R|remove",
		0,
		"Remove source files/discs if operation is successful."
		" If the source is an extracted file systems (FST) it isn't removed." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

		// [[wdf2]] use WDFv2 as default.
  { T_OPT_CO,	"WDF",		"W|wdf",
		"[=param]",
		"Set the image output file type to WDF (Wii Disc Format)."
		" The output format is either WDFv1 or WDFv2."
		" It depends of the input file format and of the aligning."
		" {--wdf=param} is a short cut for {--wdf --align-wdf=param}." },

  { T_OPT_CO,	"WDF1",		"wdf1",
		"[=param]",
		"Set image output file type to WDF and force version 1."
		" {--wdf1=param} is a short cut for {--wdf1 --align-wdf=param}." },

  { T_OPT_CO,	"WDF2",		"wdf2",
		"[=param]",
		"Set image output file type to WDF and force version 2."
		" {--wdf2=param} is a short cut for {--wdf2 --align-wdf=param}." },

		// [[2do]] 2014-01: wdf-align is obsolete at 2014-07
  { T_OPT_GP,	"ALIGN_WDF",	"align-wdf|alignwdf|wdf-align|wdfalign",
		"[align][,minhole]",
		"Parameter @align@ defines the aligning factor for new WDF images."
		" It must be a power of 2 and smaller or equal than "
		WDF_MAX_ALIGN_TEXT "."
		" The default WDF alignment is 1 for WDF v1 and "
		WDF_DEF_ALIGN_TEXT " for WDF v2."
		" Usual values are 1, 512, 4K and 32K."
		"\n "
		" The optional parameter @minhole@ defines the minimal hole size,"
		" before a new chunk is created."
		" If NULL, an internal value is used to minimize the total file size."
		" @minhole@ can't be smaller than @align@." },

  { T_OPT_C,	"ISO",		"I|iso",
		0,
		"Set image output file type to PLAIN ISO." },

  { T_OPT_C,	"CISO",		"C|ciso",
		0,
		"Set image output file type to CISO (Compact ISO, same as WBI)." },

  { T_OPT_C,	"WBFS",		"B|wbfs",
		0,
		"Set image output file type to WBFS (Wii Backup File System, default)." },

  { T_OPT_CO,	"WIA",		"wia",
		"[=compr]",
		"Set image output file type to WIA (Wii ISO Archive)."
		" The optional parameter is a compression mode and"
		" {--wia=mode} is a shortcut for {--wia --compression mode}." },

  { T_OPT_C,	"GCZ",		"G|gcz",
		0,
		"Set image output file type to GCZ (Dolphins GameCube Zip)." },

  { T_OPT_G,	"GCZ_ZIP",	"gcz-zip|gczzip|gczip",
		0,
		"If creating a GCZ image, a blockwise z-compression is tried."
		" If the compressed data is larger than 98.5%,"
		" the uncompressed data is stored."
		" Encrypted blocks are stored directly as uncompressed data,"
		" because the 98.5% test fails all the time."
		"\n "
		" Option --gcz-zip disables this optimization for encrypted data"
		" and makes the creation process slower." },

  { T_OPT_GP,	"GCZ_BLOCK",	"gcz-block|gczblock",
		"size",
		"The value defines the block size, if creating a GCZ image."
		" The default is 16K (also Dolphins default)."
		" Smaller values enlarge the managment data and"
		" reduce the compression ratio."
		" Use the option with caution!" },

  { T_OPT_C,	"FST",		"fst",
		0,
		"Set image output mode to 'file system' (extracted ISO)." },

  { T_OPT_CMP,	"FILES",	"F|files",
		"ruleset",
		"Append file select rules."
		TEXT_FILE_FILTER },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_C,	"ITIME",	"itime",
		0,
		"Select 'itime' (insertion time) for printing."
		" @--itime@ is an abbreviation of {--time i}." },

  { T_OPT_C,	"MTIME",	"mtime",
		0,
		"Select 'mtime' (last modification time) for printing."
		" @--mtime@ is an abbreviation of {--time m}." },

  { T_OPT_C,	"CTIME",	"ctime",
		0,
		"Select 'ctime' (last status change time) for printing."
		" @--ctime@ is an abbreviation of {--time c}." },

  { T_OPT_C,	"ATIME",	"atime",
		0,
		"Select 'atime' (last access time) for printing."
		" @--atime@ is an abbreviation of {--time a}." },

  { T_OPT_CMP,	"TIME",		"time",
		"list",
		"Set time printing and sorting mode."
		" The parameter is a comma separated list of the following keywords,"
		" case is ignored:"
		" RESET, OFF, ON, SINGLE, MULTI, NONE, ALL,"
		" I, M, C, A, DATE, TIME, SEC,"
		" IDATE, MDATE, CDATE, ADATE,"
		" ITIME, MTIME, CTIME, ATIME,"
		" ISEC, MSEC, CSEC, ASEC." },

  { T_OPT_CM,	"LONG",		"l|long",
		0,
		"Print in long format. Multiple usage possible." },

  { T_OPT_C,	"BRIEF",	"b|brief",
		0,
		"Print only a summary." },

  { T_OPT_C,	"NUMERIC",	"numeric",
		0,
		"Force numeric output instead of printing names." },

  { T_OPT_C,	"TECHNICAL",	"technical|tech",
		0,
		"Force a technical output instead of user friendly text."
		" --tech is a short cut for --technical."},

  { T_OPT_C,	"REALPATH",	"real-path|realpath",
		0, "Print real path instead of entered path." },

  { T_OPT_CP,	"SHOW",		"+|show",
		"list",
		"This option allows fine control over the things that are to be printed."
		" The parameter is a comma separated list of the"
		" following keywords, case is ignored:"
		" @NONE, INTRO, HEADER, SLOTS, GEOMETRY,"
		" D-ID, P-ID, P-TAB, P-INFO, P-MAP, D-MAP, W-MAP, TICKET, TMD, USAGE,"
		" PATCH, RELOCATE, FILES, UNUSED, OFFSET, SIZE, PATH@ and @ALL@."
		" There are some combined keys:"
		" @ID := D-ID,P-ID@,"
		" @PART := P-INFO,P-ID,P-MAP,TICKET,TMD@,"
		" @DISC := FILES,D-ID,D-MAP@,"
		" @MAP := P-MAP,D-MAP,W-MAP@."
		"\n "
		" All keywords can be prefixed by '+' to enable that option,"
		" by a '-' to disable it or"
		" by a '=' to enable that option and disable all others."
		"\n "
		" The additional keywords @DEC@ and @HEX@ can be used to set"
		" a prefered number format."
		" @-HEADER@ suppresses the output of header lines."
		"\n "
		" The commands recognize only some of these keywords"
		" and ignore the others."
		" If @--show@ is set, option {--long} is ignored"
		" for selecting output elements." },

  { T_OPT_CP,	"UNIT",		"unit",
		"list",
		"This option set the output unit for sizes."
		" The parameter is a comma separated list of the"
		" following keywords, case is ignored:"
		" @1000=10, 1024=2, BYTES, K, M, G, T, P, E,"
		" KB, MB, GB, TB, PB, EB, KIB, MIB, GIB, TIB, PIB, EIB,"
		" HDS, WDS, GAMECUBE=GC, WII, AUTO@ and @DEFAULT@."
		"\n "
		" The values @1000@ and @1024@ (=default base) set the base factor"
		" and @BYTES, K, M, G, T, P, E@ the SI factor."
		" @MB@ is a shortcut for @1000,M@ and @MIB@ for @1024,M@;"
		" this is also valid for the other SI factors."
		" @AUTO@ selects a value dependent SI factor."
		"\n "
		" @HDS@ and @WDS@ forces the output"
		" as multiple of the HD or Wii disc sector size (512 or 32768 bytes)."
		" @GAMECUBE@ and @WII@ forces the output of a floating point value"
		" as multiple of the single layer ISO images size of the given type."
		"\n "
		" @DEFAULT@ allows the command to select a adequate size unit." },

  { T_OPT_C,	"UNIQUE",	"U|unique",
		0, "Eliminate multiple entries with same ID6." },

  { T_OPT_C,	"NO_HEADER",	"H|no-header|noheader",
		0, "Suppress printing of header and footer." },

  { T_OPT_C,	"OLD_STYLE",	"old-style|oldstyle",
		0,
		"Print in old style."
		" This is important for tools and GUIs that are scanning the output." },

  { T_OPT_C,	"SECTIONS",	"sections",
		0,
		"Print in machine readable sections and parameter lines."
		"\1 Read https://wit.wiimm.de/opt/sections for more details." },

  { T_OPT_CP,	"SORT",		"S|sort",
		"list",
		"Define the sort mode for listings."
		" The parameter is a comma separated list of the following keywords:"
		" @NONE, NAME, TITLE, PATH, NINTENDO, FILE, SIZE, OFFSET, REGION, WBFS, NPART,"
		" FRAGMENTS, ITIME, MTIME, CTIME, ATIME, TIME = DATE, DEFAULT,"
		" ASCENDING, DESCENDING = REVERSE@." },

  { T_OPT_CP,	"LIMIT",	"limit",
		"num",
		"Limit the output to NUM messages." },

  { T_OPT_CP,	"FILE_LIMIT",	"file-limit|filelimit",
		"size",
		TEXT_DIFF_FILE_LIMIT },

  { H_OPT_CP,	"PATCH_FILE",	"patch-file|patchfile",
		"file",
		"Define a patch file." },

  //
  //---------- wit GROUP TITLES ----------

  { T_GRP_BEG,	"TITLES",	0,0,0 },

  { T_COPT_M,	"TITLES",	0,0,0 },
  { T_COPT,	"UTF_8",	0,0,0 },
  { T_COPT,	"NO_UTF_8",	0,0,0 },
  { T_COPT,	"LANG",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP FST_OPTIONS ----------

  { T_GRP_BEG,	"FST_OPTIONS",	0,0,0 },

  { T_COPT,	"IGNORE_FST",	0,0,0 },
  { T_COPT,	"IGNORE_SETUP",	0,0,0 },
  { T_COPT,	"LINKS",	0,0,0 },
  { H_COPT,	"USER_BIN",	0,0,0 },

  //---------- wit GROUP FST_SELECT ----------

  { T_GRP_BEG,	"FST_SELECT",	0,0,0 },

  { T_COPT,	"PMODE",	0,0,0 },
  { T_COPT,	"FLAT",		0,0,0 },
  { T_COPT_M,	"FILES",	0,0,0 },
  { T_COPT,	"COPY_GC",	0,0,0 },
  { T_COPT,	"NO_LINK",	0,0,0 },
  { T_COPT,	"NEEK",		0,0,0 },

  //---------- wit GROUP SOURCE ----------

  { T_GRP_BEG,	"SOURCE",	0,0,0 },

  { T_COPT_M,	"SOURCE",	0,0,0 },
  { T_COPT,	"NO_EXPAND",	0,0,0 },
  { T_COPT_M,	"RECURSE",	0,0,0 },
  { T_COPT,	"RDEPTH",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP EXCLUDE ----------

  { T_GRP_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },
  { T_COPT_M,	"INCLUDE",	0,0,0 },
  { T_COPT_M,	"INCLUDE_PATH",	0,0,0 },
  { T_COPT,	"INCLUDE_FIRST",0,0,0 },
  { T_COPT_M,	"ONE_JOB",	0,0,0 },
  { T_COPT_M,	"JOB_LIMIT",	0,0,0 },

  //---------- wit GROUP XSOURCE ----------

  { T_GRP_BEG,	"XSOURCE",	0,0,0 },

  { T_COPY_GRP,	"SOURCE",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP XXSOURCE ----------

  { T_GRP_BEG,	"XXSOURCE",	0,0,0 },

  { T_COPY_GRP,	"SOURCE",	0,0,0 },
  { T_COPY_GRP,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"IGNORE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  //---------- wit GROUP OUTMODE_EDIT ----------

  { T_GRP_BEG,	"OUTMODE_EDIT",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"WDF1",		0,0,0 },
  { T_COPT,	"WDF2",		0,0,0 },
  { T_COPT,	"ALIGN_WDF",	0,0,0 },
  { T_COPT,	"ISO",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBFS",		0,0,0 },

  //---------- wit GROUP OUTMODE ----------

  { T_GRP_BEG,	"OUTMODE",	0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT,	"WIA",		0,0,0 },
  { T_COPT,	"GCZ",		0,0,0 },
  { T_COPT,	"GCZ_ZIP",	0,0,0 },
  { T_COPT,	"GCZ_BLOCK",	0,0,0 },

  //---------- wit GROUP OUTMODE_FST ----------

  { T_GRP_BEG,	"OUTMODE_FST",	0,0,0 },

  { T_COPY_GRP,	"OUTMODE",	0,0,0 },
  { T_COPT,	"FST",		0,0,0 },

  //---------- wit GROUP XTIME ----------

  { T_GRP_BEG,	"XTIME",	0,0,0 },

  { T_COPT,	"ITIME",	0,0,0 },
  { T_COPT,	"MTIME",	0,0,0 },
  { T_COPT,	"CTIME",	0,0,0 },
  { T_COPT,	"ATIME",	0,0,0 },

  //---------- wit GROUP TIME ----------

  { T_GRP_BEG,	"TIME",		0,0,0 },

  { T_COPY_GRP,	"XTIME",	0,0,0 },
  { T_COPT_M,	"TIME",		0,0,0 },


  //---------- wit GROUP PARTITIONS ----------

  { T_GRP_BEG,	"PARTITIONS",	0,0,0 },

  { T_COPT,	"PSEL",		0,0,0 },
  { T_COPT,	"RAW",		0,0,0 },

  //---------- wit GROUP PATCH_ID ----------

  { T_GRP_BEG,	"PATCH_ID",	0,0,0 },

  { T_COPT,	"MODIFY",	0,0,0 },
  { T_COPT,	"ID",		0,0,0 },
  { T_COPT,	"DISC_ID",	0,0,0 },
  { T_COPT,	"BOOT_ID",	0,0,0 },
  { T_COPT,	"TICKET_ID",	0,0,0 },
  { T_COPT,	"TMD_ID",	0,0,0 },
  { T_COPT,	"TT_ID",	0,0,0 },
  { T_COPT,	"WBFS_ID",	0,0,0 },

  //---------- wit GROUP PATCH ----------

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

  //---------- wit GROUP RELOCATE ----------

  { T_GRP_BEG,	"RELOCATE",	0,0,0 },

  { H_COPT,	"REPL_FILE",	0,0,0 },
  { H_COPT,	"ADD_FILE",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },
  { H_COPT,	"TRIM",		0,0,0 },
  { H_COPT,	"ALIGN",	0,0,0 },
  { T_COPT,	"ALIGN_PART",	0,0,0 },
  { T_COPT,	"ALIGN_FILES",	0,0,0 },

  //---------- wit GROUP SPLIT_CHUNK ----------

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
  { T_COPT,	"COMPRESSION",	0,0,0 },
  { T_COPT,	"MEM",		0,0,0 },

  //
  //---------- COMMAND wit VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"BRIEF",	0,0,
	"Print only the version number, revision and system."
	" If set twice, print only the version number." },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print in long format. Ignored if option {--sections} is set." },

  //---------- COMMAND wit HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_COPT,	"WIDTH",	0,0,0 },
  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wit INFO ----------

  { T_CMD_BEG,	"INFO",		0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"LONG",		0,0, CMD_INFO_LONG },

  //---------- COMMAND wit TEST ----------

  { T_CMD_BEG,	"TEST",		0,0,0 },

  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wit ERROR ----------

  { T_CMD_BEG,	"ERROR",	0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print a message text instead of a message name." },

  //---------- COMMAND wit COMPR ----------

  { T_CMD_BEG,	"COMPR",	0,0,0 },

  { T_COPT,	"MEM",		0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print a table with the normalized mode name,"
	" compression level, chunk size factor and memory usage." },
  { T_COPT,	"VERBOSE",	0,0,
	"Print always compression level and chunk size factor."
	" Standard is to suppress these values if not explicitly set." },
  { T_COPT,	"NUMERIC",	0,0,0 },

  //---------- COMMAND wit FEATURES ----------

  { T_CMD_BEG,	"FEATURES",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,
	"If set, suppress messages about the single features."
	" If set twice, suppress also the summary message." },
  { T_COPT,	"VERBOSE",	0,0,
	"Normally, a status line for each supported feature is printed."
	" If set, print an info line for each requested feature." },

  //---------- COMMAND wit ANAID ----------

  { T_CMD_BEG,	"ANAID",	0,0,0 },

  { T_COPT,	"NO_HEADER",	0,0,0 },

  //---------- COMMAND wit EXCLUDE ----------

  { T_CMD_BEG,	"EXCLUDE",	0,0,0 },

  { T_COPT_M,	"EXCLUDE",	0,0,0 },
  { T_COPT_M,	"EXCLUDE_PATH",	0,0,0 },

  //---------- COMMAND wit TITLES ----------

  { T_CMD_BEG,	"TITLES",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },

  //---------- COMMAND wit GETTITLES ----------

  { T_CMD_BEG,	"GETTITLES",	0,0,0 },

  //---------- COMMAND wit CERT ----------

  { T_CMD_BEG,	"CERT",		0,0,0 },

  { T_COPT_M,	"CERT",		0,0,0 },
  { T_COPT_M,	"FILES",	0,0,
	"Filter the certificates by rules."
	" Therefor the certificate name is build in the form 'issuer.keyid'."
	"\1\n "
	" See https://wit.wiimm.de/info/file-filter.html"
	" for more details about filters." },
  { T_COPT_M,	"FAKE_SIGN",	0,0,0 },
  { T_COPT,	"DEST",		0,0,
	"Define a destination file."
	" All selected certificates are written to this new created file." },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Print the content of all certificates to standard output."
	" This is the default if neither --dest nor --DEST are set." },
  { T_COPT,	"LONG",		0,0,
	"If printing, print the complete key as hex dump, not only the first bytes."
	" If set twice, print BASE64 encoded."},

  //---------- COMMAND wit FILELIST ----------

  { T_CMD_BEG,	"FILELIST",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"LONG",		0,0,
	"Print the real path instead of given path." },

  //---------- COMMAND wit FILETYPE ----------

  { T_CMD_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set at least once or twice additional columns with ID6 (1x)"
	" or the region (2x) are enabled."
	" If set three or more times the real path instead of given path"
	" is printed." },

  //---------- COMMAND wit ISOSIZE ----------

  { T_CMD_BEG,	"ISOSIZE",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set the size is printed in MiB too."
	" If set twice two columns with WBFS calculations are added."
	" If set three times the real path of the source is printed." },
  { T_COPT_M,	"UNIT",		0,0,0 },

  //---------- COMMAND wit CREATE ----------

  { T_CMD_BEG,	"CREATE",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },
  { T_COPT,	"DEST",		0,0,
	"Define a destination path (directory/file)."
	" This path is concatenated with the @outfile@." },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ID",		0,0,
	"Define an ID for the TICKET or TMD." },
  { T_COPT,	"IOS",		0,0,
	"Define an IOS/SYS-VERSION for the TMD." },

  //---------- COMMAND wit DOLPATCH ----------

  { T_CMD_BEG,	"DOLPATCH",	0,0,0 },
  { T_COPT_M,	"TEST",		0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set, a address to file offset translation table is printed." },
  { T_COPT_M,	"VERBOSE",	0,0,
	"Normally, only applied patches and warnings are printed."
	" If verbose is set, also logs about already patched and"
	" ignored patches because of a wrong condition are printed." },
  { T_COPT_M,	"QUIET",	0,0,
	"Suppress logging and print only warnings."
	" If set twice, warnings are also suppressed and only reported"
	" as status message." },
  { T_COPT,	"SOURCE",	0,0,
	"Define a search path for source files (e.g. 'valuefile=' attribute)."
	" If the source is a directory, it is used for every searched file."
	" Otherwise it is a file path and only used for files with the"
	" same file name (case ignored)." },
  { T_COPT,	"DEST",		0,0,
	"Define a destination path (directory or file)."
	" If not set, the source file is replaced by the patched version." },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  //---------- COMMAND wit _CODE ----------

  { T_CMD_BEG,	"CODE",	0,0,0 },

  //---------- COMMAND wit DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"Images: If set at least once a memory map for each partition is printed."
	" If set twice or more a memory map for whole ISO image is printed."
	"\n "
	"DOL files: If set, a third table with delta values between"
	" the virtual address and the file offset is printed." },
  { T_COPT,	"SHOW",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },
  { T_COPT,	"DISC_SIZE",	0,0,0 },

  //---------- COMMAND wit ID6 ----------

  { T_CMD_BEG,	"ID6",		0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set, a table with 5 IDs (DISC, BOOT, TICKET, TMD and WBFS) is printed."
	" BOOT, TICKET and TMD IDs are taken from the main partition."
	" If set twice, all IDs of all partitions are printed." },

  { T_SEP_OPT,	0,0,0,0 },
  { T_COPY_GRP,	"PATCH_ID",	0,0,0 },

  //---------- COMMAND wit ID8 ----------

  { T_CMD_BEG,	"ID8",		0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set, a table with 5 IDs (DISC, BOOT, TICKET, TMD and WBFS) is printed."
	" BOOT, TICKET and TMD IDs are taken from the main partition."
	" If set twice, all IDs of all partitions are printed." },

  { T_SEP_OPT,	0,0,0,0 },
  { T_COPY_GRP,	"PATCH_ID",	0,0,0 },

  //---------- COMMAND wit FRAGMENTS ----------

  { T_CMD_BEG,	"FRAGMENTS",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,
	"Print up to 3 tables:"
	" @virtual->file@, @file->filesystem@ and @virtual->filesystem@." },
  { T_COPT_M,	"LONG",	0,0,
	"Print aligning infos too."
	" If set twice and --brief is not set, print them as hex values." },
  { T_COPT,	"BRIEF",	0,0,
	"Ignore --verbose and print only a summary with the fragment counts." },

  //---------- COMMAND wit LIST ----------

  { T_CMD_BEG,	"LIST",		0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPY_GRP,	"FST_OPTIONS",	0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"UNIQUE",	0,0,0 },
  { T_COPT,	"SORT",		0,0,0 },

  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"If set, the size in MiB and the region is printed too."
	" If set twice, a date columns is added."
	" If set three times, a second line with number or partitions,"
	" file type and real path is added." },
  { T_COPT,	"REALPATH",	0,0,0 },
  { T_COPT_M,	"UNIT",		0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SCAN_PROGRESS",0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TIME",		0,0,0 },

  //---------- COMMAND wit LIST-L ----------

  { T_CMD_BEG,	"LIST_L",	0,0,0 },
  { T_COPY_CMD,	"LIST",		0,0,0 },

  //---------- COMMAND wit LIST-LL ----------

  { T_CMD_BEG,	"LIST_LL",	0,0,0 },
  { T_COPY_CMD,	"LIST_L",	0,0,0 },

  //---------- COMMAND wit LIST-LLL ----------

  { T_CMD_BEG,	"LIST_LLL",	0,0,0 },
  { T_COPY_CMD,	"LIST_LL",	0,0,0 },

  //---------- COMMAND wit FILES ----------

  { T_CMD_BEG,	"FILES",	0,0,0 },

  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,0 },
  { T_COPT,	"NO_HEADER",	0,0,0 },
  { T_COPT,	"SHOW",		0,0,0 },
  { T_COPT,	"SORT",		0,0,
	"Define the sort mode for the file listing."
	" The parameter is a comma separated list of the following keywords:"
	" NONE, NAME, SIZE, OFFSET, ASCENDING, DESCENDING = REVERSE." },

  //---------- COMMAND wit FILES-L ----------

  { T_CMD_BEG,	"FILES_L",	0,0,0 },
  { T_COPY_CMD,	"FILES",	0,0,0 },

  //---------- COMMAND wit FILES-LL ----------

  { T_CMD_BEG,	"FILES_LL",	0,0,0 },
  { T_COPY_CMD,	"FILES",	0,0,0 },

  //---------- COMMAND wit DIFF ----------

  { T_CMD_BEG,	"DIFF",		0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_COPT_M,	"FILES",	0,0,
		"Enter file mode (compare file by file) and append file select rules."
		TEXT_FILE_FILTER },

  { H_COPT,	"PATCH_FILE",	0,0,
		"Enter file mode (compare file by file) and create a patch file."
		" The options {--limit}, {--file-limit} and {--raw} are ignored." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0, TEXT_DIFF_QUIET },
  { T_COPT_M,	"VERBOSE",	0,0, TEXT_DIFF_VERBOSE },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SCAN_PROGRESS",0,0,0 },
  { T_COPT_M,	"FILE_LIMIT",	0,0,0 },
  { T_COPT_M,	"LIMIT",	0,0, TEXT_DIFF_LIMIT },
  { T_COPT_M,	"LONG",		0,0, TEXT_DIFF_LONG },
  { T_COPT,	"BLOCK_SIZE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPY_GRP,	"OUTMODE_FST",	0,0,0 },

  //---------- COMMAND wit FDIFF ----------

  { T_CMD_BEG,	"FDIFF",	0,0,0 },
  { T_COPY_CMD,	"DIFF",		0,0,0 },

  //---------- COMMAND wit EXTRACT ----------

  { T_CMD_BEG,	"EXTRACT",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"FST_SELECT",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"SORT",		0,0,
	"Define the extracting order."
	" The parameter is a comma separated list of the following keywords:"
	" NONE, NAME, SIZE, OFFSET, ASCENDING, DESCENDING = REVERSE." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0, TEXT_EXTRACT_LONG },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SCAN_PROGRESS",0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },


  //---------- COMMAND wit COPY ----------

  { T_CMD_BEG,	"COPY",		0,0,0 },

  { T_COPY_CMD,	"EXTRACT",	0,0,0 },

  { T_COPT,	"UPDATE",	0,0,0 },
  { T_COPT,	"DIFF",		0,0,0 },
  { T_COPT,	"REMOVE",	0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_FST",	0,0,0 },


  //---------- COMMAND wit CONVERT ----------

  { T_CMD_BEG,	"CONVERT",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SCAN_PROGRESS",0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPY_GRP,	"RELOCATE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },
  { T_COPY_GRP,	"OUTMODE",	0,0,0 },


  //---------- COMMAND wit EDIT ----------

  { T_CMD_BEG,	"EDIT",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT_M,	"IGNORE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPY_GRP,	"PATCH",	0,0,0 },
  { T_COPT,	"WDF1",		0,0,
	"If the image format is WDF, then force WDF version 1."
	" The optional aligning factor is ignored." },
  { T_COPT,	"WDF2",		0,0,
	"If the image format is WDF, then force WDF version 2."
	" The optional aligning factor is ignored." },

  //---------- COMMAND wit IMGFILES ----------

  { T_CMD_BEG,	"IMGFILES",	0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },
  { T_COPT,	"NULL",		0,0,0 },

  //---------- COMMAND wit REMOVE ----------

  { T_CMD_BEG,	"REMOVE",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  //---------- COMMAND wit MOVE ----------

  { T_CMD_BEG,	"MOVE",		0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },
  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"SECTIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  //---------- COMMAND wit RENAME ----------

  { T_CMD_BEG,	"RENAME",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XSOURCE",	0,0,0 },
  { T_COPT,	"IGNORE",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ISO",		0,0,
	"Modify ID and title of the ISO image."
	" If neither @--iso@ nor {--wbfs} is set, then both are assumed as active." },
  { T_COPT,	"WBFS",		0,0,
	"Modify ID and title of the inode in the WBFS management area."
	" Option --wbfs make only sense for images within WBFS."
	" If neither {--iso} nor @--wbfs@ is set, then both are assumed as active." },

  //---------- COMMAND wit SETTITLE ----------

  { T_CMD_BEG,	"SETTITLE",	0,0,0 },
  { T_COPY_CMD,	"RENAME",	0,0,0 },

  //---------- COMMAND wit VERIFY ----------

  { T_CMD_BEG,	"VERIFY",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },
  { T_COPT,	"IGNORE_FILES",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT_M,	"QUIET",	0,0,
	"Be quiet and report only errors. If set twice"
	" then wit will print nothing and only the exit status is set." },
  { T_COPT_M,	"VERBOSE",	0,0,0 },
  { T_COPT,	"PROGRESS",	0,0,0 },
  { T_COPT,	"SCAN_PROGRESS",0,0,0 },
  { T_COPT,	"LIMIT",	0,0,
	"Maximal printed errors of each partition."
	" A zero means unlimited. The default is 10." },
  { T_COPT_M,	"LOGGING",	0,0,0 },
  { T_COPT_M,	"LONG",		0,0,
	"On error print an additional line to localize the exact"
	" position where the error is found."
	" If set twice a hex dump of the hash values is printed too." },
  { T_COPT_M,	"TECHNICAL",	0,0,0 },

  //---------- COMMAND wit SKELETON ----------

  { T_CMD_BEG,	"SKELETON",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },
  { T_COPT_M,	"QUIET",	0,0,0 },
  { T_COPT_M,	"LOGGING",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"TITLES",	0,0,0 },
  { T_COPT,	"AUTO",		0,0,0 },
  { T_COPY_GRP,	"XXSOURCE",	0,0,0 },
  { T_COPY_GRP,	"PARTITIONS",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },
  { T_COPT_M,	"DEST",		0,0,
	"Define a destination directory for the skeleton files."
	" The default is {--DEST ./.skel/}." },
  { T_COPT_M,	"DEST2",	0,0,0 },

  //---------- COMMAND wit MIX ----------

  { T_CMD_BEG,	"MIX",	0,0,0 },

  { T_COPT_M,	"TEST",		0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"ESC",		0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },
  { T_COPY_GRP,	"SPLIT_CHUNK",	0,0,0 },
  { T_COPT,	"ALIGN_PART",	0,0,
	"The beginning of each partition is set to an offset that is"
	" a multiple of the align size."
	" Size must be a power of 2 and at least 32 KiB (=default)."
	" If option {--overlay} is set only the first partition is aligned." },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPY_GRP,	"OUTMODE_EDIT",	0,0,0 },

  { T_SEP_OPT,	0,0,0,0 },

  { T_COPT,	"ID",		0,0,
	"Define an ID for the disc header."
	" The default is 'WIT000'." },
  { T_COPT,	"NAME",		0,0,
	"Define a disc title for the disc header."
	" The default is a generic title based on all sources." },
  { T_COPT,	"REGION",	0,0,0 },
  { T_COPT,	"OVERLAY",	0,0,0 },

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
