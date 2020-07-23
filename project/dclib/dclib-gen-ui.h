
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
 *        Copyright (c) 2012-2018 by Dirk Clemens <wiimm@wiimm.de>         *
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

#ifndef DCLIB_GEN_UI_H
#define DCLIB_GEN_UI_H

#include "dclib-basics.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  text controls			///////////////
///////////////////////////////////////////////////////////////////////////////

//	\1 : the following text is only for the built in help
//	\2 : the following text is only for the web site
//	\3 : the following text is for both, built in help and web site
//	\4 : replace by '<' for html tags on web site

// Only for tools and commands:
//	\f : Text before: short help for lists
//	     Text behind: extended help for tool/command help

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  declarations			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum enumType
{
	//----- base types

	T_END		=    0x0001,  // end of list
	T_DEF_TOOL	=    0x0002,  // start of a new tool
	T_DEF_CMD	=    0x0004,  // define a command
	T_SEP_CMD	=    0x0008,  // define a command separator
	T_DEF_OPT	=    0x0010,  // define an option
	T_SEP_OPT	=    0x0020,  // define an option separator
	T_GRP_BEG	=    0x0040,  // start option definitions for a group
	T_CMD_BEG	=    0x0080,  // start option definitions for a command
	T_CMD_OPT	=    0x0100,  // allowed option for command
	T_COPY_CMD	=    0x0200,  // copy options of other command
	T_COPY_GRP	=    0x0400,  // copy options of group
	T_ALL_OPT	=    0x0800,  // allow all options

	//----- option flags

	F_OPT_COMMAND	=  0x010000,  // option is command specific
	F_OPT_GLOBAL	=  0x020000,  // option is global
	F_OPT_MULTIUSE	=  0x040000,  // multiple usage of option possible
	F_OPT_PARAM	=  0x080000,  // option needs a parameter
	F_OPT_OPTPARAM	=  0x100000,  // option accepts an optional parameter
	F_SEPARATOR	=  0x200000,  // separator element
	F_SUPERSEDE	=  0x400000,  // supersedes all other commands and options
	F_IGNORE	=  0x800000,  // ignore on command 'HELP OPTIONS'

	F_OPT_XPARAM	=  F_OPT_PARAM | F_OPT_OPTPARAM,


	//----- global flags

	F_HIDDEN	= 0x1000000,  // tool, command or option is hidden from help
	F_DEPRECATED	= 0x2000000,  // command or option is depreacted


	//----- option combinations

	T_OPT_C		= T_DEF_OPT | F_OPT_COMMAND,
	T_OPT_CM	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE,
	T_OPT_CP	= T_DEF_OPT | F_OPT_COMMAND                  | F_OPT_PARAM,
	T_OPT_CMP	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE | F_OPT_PARAM,
	T_OPT_CO	= T_DEF_OPT | F_OPT_COMMAND                  | F_OPT_OPTPARAM,
	T_OPT_CMO	= T_DEF_OPT | F_OPT_COMMAND | F_OPT_MULTIUSE | F_OPT_OPTPARAM,

	T_OPT_G		= T_DEF_OPT | F_OPT_GLOBAL,
	T_OPT_GM	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE,
	T_OPT_GP	= T_DEF_OPT | F_OPT_GLOBAL                   | F_OPT_PARAM,
	T_OPT_GMP	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE | F_OPT_PARAM,
	T_OPT_GO	= T_DEF_OPT | F_OPT_GLOBAL                   | F_OPT_OPTPARAM,
	T_OPT_GMO	= T_DEF_OPT | F_OPT_GLOBAL  | F_OPT_MULTIUSE | F_OPT_OPTPARAM,

	T_OPT_S		= T_DEF_OPT | F_OPT_GLOBAL | F_SUPERSEDE,


	T_COPT		= T_CMD_OPT,
	T_COPT_M	= T_CMD_OPT | F_OPT_MULTIUSE,


	//----- hidden options and commands (hide from help)

	H_DEF_TOOL	= F_HIDDEN | T_DEF_TOOL,
	H_DEF_CMD	= F_HIDDEN | T_DEF_CMD,

	H_OPT_C		= F_HIDDEN | T_OPT_C,
	H_OPT_CM	= F_HIDDEN | T_OPT_CM,
	H_OPT_CP	= F_HIDDEN | T_OPT_CP,
	H_OPT_CMP	= F_HIDDEN | T_OPT_CMP,
	H_OPT_CO	= F_HIDDEN | T_OPT_CO,
	H_OPT_CMO	= F_HIDDEN | T_OPT_CMO,

	H_OPT_G		= F_HIDDEN | T_OPT_G,
	H_OPT_GM	= F_HIDDEN | T_OPT_GM,
	H_OPT_GP	= F_HIDDEN | T_OPT_GP,
	H_OPT_GMP	= F_HIDDEN | T_OPT_GMP,
	H_OPT_GO	= F_HIDDEN | T_OPT_GO,
	H_OPT_GMO	= F_HIDDEN | T_OPT_GMO,

	H_COPT		= F_HIDDEN | T_COPT,
	H_COPT_M	= F_HIDDEN | T_COPT_M,

} enumType;

///////////////////////////////////////////////////////////////////////////////

typedef struct info_t
{
	enumType type;		// entry type
	ccp c_name;		// the C name
	ccp namelist;		// list of names
	ccp param;		// name of parameter
	ccp help;		// help text

	int index;		// calculated index

} info_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_GEN_UI_H
