
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
/////////////			   TOOL wdf			///////////////
///////////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wdf", 0,
		"wdf [options]... [+command] [options]... files...",
		"wdf is a support tool for WDF, WIA, CISO and GCZ images."
		" It converts (packs and unpacks), compares and dumps WDF and CISO images."
		" Additionally it dumps WIA and GCT image and unpacks WIA images."
		" The default command depends on the program file name"
		" (see command descriptions). Usual names are"
		" @wdf@, @unwdf@, @wdf-cat@, @wdf-cmp@ and @wdf-dump@"
		" (with or without minus signs)."
		"\n "
		" {wdf +CAT} replaces the old tool @wdf-cat@"
		" and {wdf +DUMP} the old tool @wdf-dump@." },

  //
  //---------- list of all wdf commands ----------

  { T_DEF_CMD,	"VERSION",	"+VERSION|+V",
		    "wdf +VERSION [ignored]...",
		"Print program name, version and the defaults and exit." },

  { T_DEF_CMD,	"HELP",		"+HELP|+H",
		    "wdf +HELP [+command] [ignored]...",
		"Print help and exit."
		" If the first non option is a valid command name,"
		" then a help for the given command is printed." },

  { T_SEP_CMD,	0,0,0,0 }, //----- separator -----

  { T_DEF_CMD,	"PACK",		"+PACK|+P",
		    "wdf +PACK [option]... files...",
		"Pack sources into WDF or CISO images."
		" This is the general default." },

  { T_DEF_CMD,	"UNPACK",	"+UNPACK|+U",
		    "wdf +UNPACK [option]... files...",
		"Unpack WDF, WIA and CISO images."
		"\n "
		" This is the default command, when the program name"
		" starts with the two letters @'un'@ in any case." },

  { T_DEF_CMD,	"CAT",		"+CAT|+C",
		    "wdf +CAT [option]... files...",
		"Concatenate files and print on the standard output."
		" WDF, WIA and CISO files are extracted before printing,"
		" all other files are copied byte by byte."
		"\n "
		" This is the default command, when the program name"
		" contains the sub string @'cat'@ in any case."
		" {wdf +CAT} replaces the old tool @wdf-cat@." },

  { T_DEF_CMD,	"CMP",		"+DIFF|+CMP",
		    "wdf +DIFF [option]... files...",
		"Compare files and unpack WDF, WIA and CISO while comparing."
		"\n "
		" The standard is to compare two source files."
		" If {--dest} or {--DEST} is set, than all source files"
		" are compared against files in the destination path with equal names."
		" If the second source file is missed,"
		" then standard input (stdin) is used instead."
		"\n "
		" This is the default command, when the program name"
		" contains the sub string @'diff'@ or @'cmp'@ in any case." },

  { T_DEF_CMD,	"DUMP",		"+DUMP|+D",
		    "wdf +DUMP [option]... files...",
		"Dump the data structure of WDF, WIA, CISO and GCZ images"
		" and ignore other files."
		"\n "
		" This is the default command, when the program"
		" contains the sub string @'dump'@ in any case."
		" {wdf +DUMP} replaces the old tool @wdf-dump@." },

  //
  //---------- list of all wdf options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, 0 /* copy of wit */ },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_G,	"QUIET",	"q|quiet",
		0, 0 /* copy of wit */ },

  { T_OPT_G,	"VERBOSE",	"v|verbose",
		0, "Be verbose -> print program name." },

  { T_OPT_GM,	"LOGGING",	"L|logging",
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

  { T_OPT_C,	"CHUNK",	"chunk",
		0,
		"Print table with chunk header too." },

  { T_OPT_C,	"LONG",		"l|long",
		0,
		"Print (un)pack statistics, 1 line for each source."
		" In dump mode: Print table with chunk header too (same as {--chunk})." },

  { T_OPT_C,	"MINUS1",	"1|minus-1|minus1",
		0,
		"If set the end address is the last address of a range."
		" The standard is to print the first address"
		" that is not part of the address of a range." },

  { T_OPT_CP,	"LIMIT",	"limit",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"FILE_LIMIT",	"file-limit|filelimit",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"BLOCK_SIZE",	"block-size|blocksize",
		0, 0 /* copy of wit */ },


  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CO,	"WDF",		"W|wdf",
		"[=param]",
		"Force WDF output mode if packing"
		" and set the default suffix to @'.wdf'@."
		" This is the general default."
		" {--wdf=param} is a short cut for {--wdf --align-wdf=param}." },

  { T_OPT_CO,	"WDF1",		"wdf1",
		"[=param]",
		"Force WDF v1 output mode, if packing."
		" Set the default suffix to @'.wdf'@."
		" {--wdf1=param} is a short cut for {--wdf1 --align-wdf=param}." },

  { T_OPT_CO,	"WDF2",		"wdf2",
		"[=param]",
		"Force WDF v2 output mode, if packing."
		" Set the default suffix to @'.wdf'@."
		" {--wdf2=param} is a short cut for {--wdf2 --align-wdf=param}." },

		// [[2do]] 2014-01: wdf-align is obsolete at 2014-07
  { T_OPT_GP,	"ALIGN_WDF",	"align-wdf|alignwdf|wdf-align|wdfalign",
		0, 0 /* copy of wit */ },
		
  { T_OPT_CO,	"WIA",		"wia",
		"[=compr]",
		"Force WIA output mode if packing"
		" and set the default suffix to @'.wia'@."
		" The optional parameter is a compression mode and"
		" {--wia=mode} is a shortcut for {--wia --compression mode}."
		"\n "
		" WIA output is the default, when the program name contains"
		" the sub string @'wia'@ in any case." },

  { T_OPT_C,	"CISO",		"C|ciso",
		0,
		"Force CISO output mode if packing"
		" and set the default suffix to @'.ciso'@."
		"\n "
		"  This is the default, when the program name contains"
		" the sub string @'ciso'@ in any case." },

  { T_OPT_C,	"WBI",		"wbi",
		0,
		"Force CISO output mode if packing"
		" and set the default suffix to @'.wbi'@."
		"\n "
		"  This is the default, when the program name contains"
		" the sub string @'wbi'@ but not @'ciso'@ in any case." },

  { T_OPT_CP,	"SUFFIX",	"s|suffix",
		".suf",
		"Use suffix @'.suf'@ instead of @'.wdf'@, @'.wia'@,"
		" or @'.ciso'@ for packed files." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_CP,	"DEST",		"d|dest",
		"path",
		"Define a destination path (directory/file)." },

  { T_OPT_CP,	"DEST2",	"D|DEST",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"STDOUT",	"c|stdout",
		0,
		"Write to standard output (stdout)"
		" and keep (don't delete) input files."
		"\n "
		" This is the default, when the program"
		" is reading from standard input (stdin)." },

  { T_OPT_C,	"KEEP",		"k|keep",
		0,
		"Keep (don't delete) input files during (un-)packing." },

  { T_OPT_C,	"OVERWRITE",	"o|overwrite",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"PRESERVE",	"p|preserve",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"AUTO_SPLIT",	"auto-split|autosplit",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"NO_SPLIT",	"no-split|nosplit",
		0, 0 /* copy of wit */ },

  { T_OPT_C,	"SPLIT",	"z|split",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"SPLIT_SIZE",	"Z|split-size|splitsize",
		0, 0 /* copy of wit */ },

  { T_OPT_CO,	"PREALLOC",	"prealloc",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"CHUNK_MODE",	"chunk-mode|chunkmode|chm",
		"mode", TEXT_OPT_CHUNK_MODE("32K") },

  { T_OPT_CP,	"CHUNK_SIZE",	"chunk-size|chunksize|chs",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MAX_CHUNKS",	"max-chunks|maxchunks|mch",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"COMPRESSION",	"compression|compr",
		0, 0 /* copy of wit */ },

  { T_OPT_CP,	"MEM",		"mem",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_G,	"TEST",		"t|test",
		0, 0 /* copy of wit */ },

#if OPT_OLD_NEW
  { H_OPT_GM,	"OLD",		"OLD",
		0, 0 /* copy of wit */ },

  { H_OPT_GM,	"NEW",		"NEW",
		0, 0 /* copy of wit */ },
#endif

  //
  //---------- wdf GROUP BASE ----------

  { T_GRP_BEG,	"BASE",	0,0,0 },

  { T_COPT,	"QUIET",	0,0,0 },
  { T_COPT,	"VERBOSE",	0,0,0 },

  //---------- wdf GROUP DEST ----------

  { T_GRP_BEG,	"DEST",	0,0,0 },

  { T_COPT,	"DEST",		0,0,0 },
  { T_COPT,	"DEST2",	0,0,0 },
  { T_COPT,	"OVERWRITE",	0,0,0 },

  //---------- wdf GROUP DEST_PLUS ----------

  { T_GRP_BEG,	"DEST_PLUS",	0,0,0 },

  { T_COPY_GRP,	"DEST",		0,0,0 },
  { T_COPT,	"AUTO_SPLIT",	0,0,0 },
  { T_COPT,	"NO_SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT",	0,0,0 },
  { T_COPT,	"SPLIT_SIZE",	0,0,0 },
  { T_COPT,	"PREALLOC",	0,0,0 },
  { T_COPT,	"CHUNK_MODE",	0,0,0 },
  { T_COPT,	"CHUNK_SIZE",	0,0,0 },
  { T_COPT,	"MAX_CHUNKS",	0,0,0 },
  { T_COPT,	"COMPRESSION",	0,0,0 },
  { T_COPT,	"MEM",		0,0,0 },

  //---------- wdf GROUP FILETYPE ----------

  { T_GRP_BEG,	"FILETYPE",	0,0,0 },

  { T_COPT,	"WDF",		0,0,0 },
  { T_COPT,	"WDF1",		0,0,0 },
  { T_COPT,	"WDF2",		0,0,0 },
  { T_COPT,	"ALIGN_WDF",	0,0,0 },
  { T_COPT,	"WIA",		0,0,0 },
  { T_COPT,	"CISO",		0,0,0 },
  { T_COPT,	"WBI",		0,0,0 },
  { T_COPT,	"SUFFIX",	0,0,0 },

  //
  //---------- COMMAND wdf VERSION ----------

  { T_CMD_BEG,	"VERSION",	0,0,0 },

  { T_COPT,	"LONG",		0,0, "Print in long format." },

  //---------- COMMAND wdf HELP ----------

  { T_CMD_BEG,	"HELP",		0,0,0 },

  { T_COPT,	"WIDTH",	0,0,0 },
  { T_ALL_OPT,	0,		0,0,0 },

  //---------- COMMAND wdf UNPACK ----------

  { T_CMD_BEG,	"UNPACK",	0,0,0 },

  { T_COPY_GRP,	"DEST_PLUS",	0,0,0 },
  { T_COPT,	"STDOUT",	0,0,0 },
  { T_COPT,	"KEEP",		0,0,0 },
  { T_COPT,	"PRESERVE",	0,0,0 },

  //---------- COMMAND wdf PACK ----------

  { T_CMD_BEG,	"PACK",		0,0,0 },

  { T_COPY_CMD,	"UNPACK",	0,0,0 },
  { T_SEP_OPT,	0,0,0,0 },
  { T_COPY_GRP,	"FILETYPE",	0,0,0 },

  //---------- COMMAND wdf CAT ----------

  { T_CMD_BEG,	"CAT",		0,0,0 },

  { T_COPY_GRP,	"DEST",		0,0,0 },

  //---------- COMMAND wdf CMP ----------

  { T_CMD_BEG,	"CMP",		0,0,0 },

  { T_COPT,	"QUIET",	0,0, TEXT_DIFF_QUIET },
  { T_COPT_M,	"VERBOSE",	0,0, TEXT_DIFF_VERBOSE },
  { T_COPT_M,	"FILE_LIMIT",	0,0,0 },
  { T_COPT_M,	"LIMIT",	0,0, TEXT_DIFF_LIMIT },
  { T_COPT_M,	"LONG",		0,0, TEXT_DIFF_LONG },
  { T_COPT,	"BLOCK_SIZE",	0,0,0 },

  //---------- COMMAND wdf DUMP ----------

  { T_CMD_BEG,	"DUMP",		0,0,0 },

  { T_COPY_GRP,	"DEST",		0,0,0 },
  { T_COPT,	"CHUNK",	0,0,0 },
  { T_COPT,	"LONG",		0,0, "Same as {--chunk}" },
  { T_COPT,	"MINUS1",	0,0,0 },

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
