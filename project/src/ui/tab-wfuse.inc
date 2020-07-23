
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
/////////////			  TOOL wfuse			///////////////
///////////////////////////////////////////////////////////////////////////////

  { T_DEF_TOOL,	"wfuse", 0,
		"wfuse [option]... source mountdir\n"
		"wfuse --umount mountdir...",
		"Mount a Wii or GameCube image or a WBFS file or partition"
		" to a mount point using FUSE (Filesystem in USErspace)."
		" Use @'wfuse --umount mountdir'@ for unmounting." },

  //---------- list of all options ----------

  { T_OPT_S,	"VERSION",	"V|version",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"HELP",		"h|help",
		0, 0 /* copy of wit */ },

  { T_OPT_S,	"XHELP",	"xhelp",
		0, "Same as {--help}." },

  { T_OPT_S,	"HELP_FUSE",	"H|help-fuse|helpfuse",
		0,
		"Stop parsing the command line and print a FUSE help message." },

  { T_OPT_GP,	"WIDTH",	"width",
		0, 0 /* copy of wit */ },

  { T_OPT_GM,	"QUIET",	"q|quiet",
		0,
		"Be quiet and print only error messages." },

  { H_OPT_GM,	"VERBOSE",	"v|verbose",
		0,
		"Be verbose and print more progress information." },

  { H_OPT_GP,	"IO",		"io",
		0, 0 /* copy of wit */ },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_GP,	"PARAM",	"p|param",
		"param",
		"The parameter is forwarded to the FUSE command line scanner." },

  { T_OPT_GP,	"OPTION",	"o|option",
		"param",
		"This option is forwarded to FUSE command line scanner as @'-o param'@." },

  { T_OPT_G,	"ALLOW_OTHER",	"O|allow-other|allowother",
		0,
		"This option is a short cut for @'-o allow_other'@."
		" It enables re-exporting of the mounted file system"
		" for example by a samba server." },

  { T_OPT_G,	"CREATE",	"c|create",
		0,
		"If the mount point does not exist, create it and remove it on unmount." },

  { T_OPT_G,	"REMOUNT",	"r|remount",
		0,
		"If the mount point is already mounted, try silently to unmount it first." },

  { T_SEP_OPT,	0,0,0,0 }, //----- separator -----

  { T_OPT_G,	"UMOUNT",	"u|umount|unmount",
		0,
		"Enter 'unmount mode' and unmount each entered directory by calling"
		" @'fusermount -u mountdir'@ or alternatively @'umount mountdir'@." },

  { T_OPT_G,	"LAZY",		"l|lazy",
		0,
		"Lazy unmount:"
		" Detach the mounted file system from the file system hierarchy now,"
		" and cleanup all references to the file system"
		" as soon as it is not busy anymore." },

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
