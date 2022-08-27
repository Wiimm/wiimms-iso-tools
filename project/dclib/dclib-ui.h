
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

#ifndef DCLIB_UI_H
#define DCLIB_UI_H

#include "dclib-basics.h"
#include <getopt.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  definitions			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[OptionIndex_t]] // max size of OptionIndex[]

#ifndef UIOPT_INDEX_SIZE
  #define UIOPT_INDEX_SIZE 0x100
#endif

#undef UIOPT_INDEX_FW

#if UIOPT_INDEX_SIZE <= 0x100
  #define UIOPT_INDEX_FW 2
  typedef u8 OptionIndex_t;
#else
  #define UIOPT_INDEX_FW 3
  typedef u16 OptionIndex_t;
#endif

///////////////////////////////////////////////////////////////////////////////

enum // some const
{
    UIOPT_USED_MASK	=   0x7f,	// mask to calculate usage count
    UIOPT_LONG_BASE	=   0x80,	// first index for "only long options"
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  InfoOption_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[InfoOption_t]]

typedef struct InfoOption_t
{
    int			id;		// id of the option
    bool		optional_parm;	// true: option has optional parameters
    bool		hidden;		// true: option is hidden
    bool		deprecated;	// true: option is deprecated
    bool		ignore;		// true: ignore on 'HELP OPTIONS'
    bool		separator;	// true: print a separator above
    char		short_name;	// short option name
    ccp			long_name;	// the main long option name
    ccp			param;		// parameter name
    ccp			help;		// help info

} InfoOption_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  InfoCommand_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[InfoCommand_t]]

typedef struct InfoCommand_t
{
    int			id;		// id of the command
    bool		hidden;		// true: command is hidden
    bool		deprecated;	// true: command is deprecated
    bool		separator;	// true: print a separator above
    ccp			name1;		// main name
    ccp			name2;		// NULL or alternative name
    ccp			syntax;		// NULL or syntax info
    ccp			help;		// help text
    ccp			xhelp;		// NULL or a extended help text
    int			n_opt;		// number of options == elements of 'opt'
    const InfoOption_t	** opt;		// field with option info
    u8			* opt_allowed;	// field with OPT__N_SPECIFIC elements
					// 0: option permitted, 1: option allowed

} InfoCommand_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    InfoUI_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[InfoUI_t]]

typedef struct InfoUI_t
{
    ccp			tool_name;	// name of the tool

    //----- commands -----

    int			n_cmd;		// == CMD__N
    const KeywordTab_t	* cmd_tab;	// NULL or pointer to command table
    const InfoCommand_t	* cmd_info;	// pointer to 'CommandInfo[]'

    //----- options -----

    int			n_opt_specific;	// == OPT__N_SPECIFIC
    int			n_opt_total;	// == OPT__N_TOTAL
    const InfoOption_t	* opt_info;	// pointer to 'OptionInfo[]'
    u8			* opt_used;	// pointer to 'OptionUsed[]'
    const OptionIndex_t	* opt_index;	// pointer to 'OptionIndex[]'
    ccp			opt_short;	// pointer to 'OptionShort[]'
    const struct option	* opt_long;	// pointer to 'OptionLong[]'

} InfoUI_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Interface			///////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError RegisterOptionByIndex
(
    const InfoUI_t	* iu,		// valid pointer
    int			opt_index,	// index of option (OPT_*)
    int			level,		// the level of registration
    bool		is_env		// true: register environment pre setting
);

enumError RegisterOptionByName
(
    const InfoUI_t	* iu,		// valid pointer
    int			opt_name,	// short name of GO_* value of option
    int			level,		// the level of registration
    bool		is_env		// true: register environment pre setting
);

enumError VerifySpecificOptions ( const InfoUI_t * iu, const KeywordTab_t * cmd );
int GetOptionCount ( const InfoUI_t * iu, int option );
void DumpUsedOptions ( const InfoUI_t * iu, FILE * f, int indent );
void WarnDepractedOptions ( const InfoUI_t * iu );

typedef enumError (*check_opt_func) ( int argc, char ** argv, bool mode );
enumError CheckEnvOptions ( ccp varname, check_opt_func );

///////////////////////////////////////////////////////////////////////////////

void DumpText
(
    FILE		* f,		// output file, if NULL: 'pbuf' must be set
    char		* pbuf,		// output buffer, ignored if 'f' not NULL
    char		* pbuf_end,	// end of output buffer
    ccp			text,		// source text
    int			text_len,	// >=0: length of text, <0: use strlen(text)
    bool		is_makedoc,	// true: write makedoc format
    ccp			end		// write this text at the end of all
);

///////////////////////////////////////////////////////////////////////////////

void PrintHelp
(
    const InfoUI_t	* iu,		// valid pointer
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    ccp			help_cmd,	// NULL or name of help command
    ccp			info,		// NULL or additional text
    ccp			base_uri,	// NULL or base URI for a external help link
    ccp			first_param	// NULL or first param of argv[]
);

void PrintHelpCmd
(
    const InfoUI_t	* iu,		// valid pointer
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    int			cmd,		// index of command
    ccp			help_cmd,	// NULL or name of help command
    ccp			info,		// NULL or poiner to additional text
    ccp			base_uri	// NULL or base URI for a external help link
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_UI_H

