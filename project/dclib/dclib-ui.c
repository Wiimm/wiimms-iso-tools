
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

#include <ctype.h>
#include <getopt.h>
#include <string.h>

#include "dclib-basics.h"
#include "dclib-ui.h"
#include "dclib-debug.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			register options		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError RegisterOptionByIndex
(
    const InfoUI_t	* iu,		// valid pointer
    int			opt_index,	// index of option (OPT_*)
    int			level,		// the level of registration
    bool		is_env		// true: register environment pre setting
)
{
    ASSERT(iu);
    DASSERT(iu->opt_info);
    DASSERT(iu->opt_used);
    DASSERT(iu->opt_index);

    if ( level <= 0 )
	return ERR_OK;

    if ( opt_index >= 0
	 && opt_index < iu->n_opt_total
	 && opt_index < UIOPT_INDEX_SIZE )
    {
	TRACE("OPT: RegisterOptionByIndex(,%02x,%d,%d) opt_index=%02x,%s\n",
		    opt_index, level, is_env, opt_index,
		    iu->opt_info[opt_index].long_name );
	u8 * obj = iu->opt_used + opt_index;
	u32 count = *obj;
	if (is_env)
	{
	    if ( count < 0x7f )
	    {
		count += level;
		*obj = count < 0x7f ? count : 0x7f;
	    }
	}
	else
	{
	    if ( count < 0x80 )
		count = 0x80;
	    count += level;
	    *obj = count < 0xff ? count : 0xff;
	}

	return ERR_OK;
    }

    PRINT("OPT: RegisterOptionbyIndex(,%02x/%02x/%02x,%d,%d) => WARNING\n",
		opt_index, iu->n_opt_total, UIOPT_INDEX_SIZE, level, is_env );
    return ERR_WARNING;
}

///////////////////////////////////////////////////////////////////////////////

enumError RegisterOptionByName
(
    const InfoUI_t	* iu,		// valid pointer
    int			opt_name,	// short name of GO_* value of option
    int			level,		// the level of registration
    bool		is_env		// true: register environment pre setting
)
{
    ASSERT(iu);
    DASSERT(iu->opt_index);

    if ( opt_name >= 0 && opt_name < UIOPT_INDEX_SIZE )
	return RegisterOptionByIndex(iu,iu->opt_index[opt_name],level,is_env);

    PRINT("OPT: RegisterOptionbyName(,%02x,%d,%d) => WARNING\n",
		opt_name, level, is_env );
    return ERR_WARNING;
}

///////////////////////////////////////////////////////////////////////////////

enumError VerifySpecificOptions ( const InfoUI_t * iu, const KeywordTab_t * cmd )
{
    ASSERT(iu);
    ASSERT(cmd);

    ASSERT( cmd->id > 0 && cmd->id < iu->n_cmd );
    const InfoCommand_t * ic = iu->cmd_info + cmd->id;
    ASSERT(ic->opt_allowed);
    u8 * allow  = ic->opt_allowed;
    u8 * active = iu->opt_used;

    TRACE("ALLOWED SPECIFIC OPTIONS:\n");
    TRACE_HEXDUMP(10,0,0,-20,allow,iu->n_opt_specific);
    TRACE("ACTIVE SPECIFIC OPTIONS:\n");
    TRACE_HEXDUMP(10,0,0,-20,active,iu->n_opt_specific);

    enumError err = ERR_OK;
    int idx;
    for ( idx = 0; idx < iu->n_opt_specific; idx++, allow++, active++ )
    {
	if (!*allow)
	{
	    if ( *active & 0x80 )
	    {
		const InfoOption_t * io = iu->opt_info + idx;
		if (io->short_name)
		    ERROR0(ERR_SEMANTIC,"Command '%s' doesn't allow the option --%s (-%c).\n",
				cmd->name1, io->long_name, io->short_name );
		else
		    ERROR0(ERR_SEMANTIC,"Command '%s' doesn't allow the option --%s.\n",
				cmd->name1, io->long_name );
		err = ERR_SEMANTIC;

	    }
	    else
		*active = 0; // clear environ settings
	}
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

int GetOptionCount ( const InfoUI_t * iu, int option )
{
    ASSERT(iu);
    ASSERT(iu->opt_info);
    ASSERT(iu->opt_used);

    if ( option > 0 && option <  iu->n_opt_total )
    {
	TRACE("GetOptionCount(,%02x) option=%s => %d\n",
		option, iu->opt_info[option].long_name,
		iu->opt_used[option] & UIOPT_USED_MASK );
	return iu->opt_used[option] & UIOPT_USED_MASK;
    }

    TRACE("GetOptionCount(,%02x) => INVALID [-1]\n",option);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

void DumpUsedOptions ( const InfoUI_t * iu, FILE * f, int indent )
{
    ASSERT(iu);
    ASSERT(iu->opt_info);
    ASSERT(iu->opt_used);

    TRACE("OptionUsed[]:\n");
    TRACE_HEXDUMP16(9,0,iu->opt_used,iu->n_opt_total+1);

    if (!f)
	return;

    indent = NormalizeIndent(indent);

    int i;
    for ( i = 0; i < iu->n_opt_total; i++ )
    {
	const u8 opt = iu->opt_used[i];
	if (opt)
	    fprintf(f,"%*s%s %s %2u* [%02x,%s]\n",
			indent, "",
			i <= iu->n_opt_specific ? "CMD" : "GLB",
			opt & 0x80 ? "PRM" : "ENV",
			opt & UIOPT_USED_MASK,
			i, iu->opt_info[i].long_name );
    }
};

///////////////////////////////////////////////////////////////////////////////

void WarnDepractedOptions ( const struct InfoUI_t * iu )
{
    DASSERT(iu);

    uint i, count = 0;
    char buf[PATH_MAX];
    char *dest = buf;

    for ( i = 0; i < iu->n_opt_total; i++ )
    {
	if ( iu->opt_used[i] )
	{
	    const InfoOption_t *io = iu->opt_info + i;
	    if ( io->deprecated )
	    {
		if (io->long_name)
		{
		    count++;
		    dest = snprintfE(dest,buf+sizeof(buf)," --%s",io->long_name);
		}
		else if (io->short_name)
		{
		    count++;
		    dest = snprintfE(dest,buf+sizeof(buf)," -%c",io->short_name);
		}
	    }
	}
    }

    if (count)
	ERROR0(ERR_WARNING,
		"Attention: Deprecated option%s:%s\n",
		count == 1 ? "" : "s", buf );
}

///////////////////////////////////////////////////////////////////////////////

enumError CheckEnvOptions ( ccp varname, check_opt_func func )
{
    TRACE("CheckEnvOptions(%s,%p)\n",varname,func);
    ccp env = getenv(varname);
    if ( !env || !*env )
	return ERR_OK;

    PRINT("env[%s] = %s\n",varname,env);

 #if 1 // [[new]] with ArgManager_t
    ArgManager_t am = {0};
    AppendArgManager(&am,ProgInfo.progname,0,false);
    ScanQuotedArgManager(&am,env,true);

  #ifdef DEBUG
    for ( int i = 0; i < am.argc; i++ )
	printf(" [%02u] |%s|\n",i,am.argv[i]);
  #endif
    enumError stat = func(am.argc,am.argv,true);

 #else // [[old]]

    const int envlen = strlen(env);
    char * buf = MALLOC(envlen+1);
    char * dest = buf;

    int argc = 1; // argv[0] = ProgInfo.progname
    ccp src = env;
    while (*src)
    {
	while ( *src > 0 && *src <= ' ' ) // skip blanks & control
	    src++;

	if (!*src)
	    break;

	argc++;
	while ( *(u8*)src > ' ' )
	    *dest++ = *src++;
	*dest++ = 0;
	DASSERT( dest <= buf+envlen+1 );
    }
    TRACE("argc = %d\n",argc);

    char ** argv = MALLOC((argc+1)*sizeof(*argv));
    argv[0] = (char*)ProgInfo.progname;
    argv[argc] = 0;
    dest = buf;
    int i;
    for ( i = 1; i < argc; i++ )
    {
	TRACE("argv[%d] = %s\n",i,dest);
	argv[i] = dest;
	while (*dest)
	    dest++;
	dest++;
	DASSERT( dest <= buf+envlen+1 );
    }

    enumError stat = func(argc,argv,true);

 #endif

    // don't free() because of possible pointers into argv[]

    if (stat)
	fprintf(stderr,
	    "Error while scanning the environment variable '%s'\n",varname);

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 text helpers			///////////////
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
)
{
    ASSERT(f||pbuf);

    ccp tie = is_makedoc ? " \\" : "";
    char buf[100], *buf_end = buf + 70, *dest = buf;

    bool set_apos = false;
    bool brace_active = false;

    if (!text)
	text = "";
    if ( text_len < 0 )
	text_len = strlen(text) ;
    ccp text_end = text + text_len;

    while ( text < text_end )
    {
	char * dest_break = 0;
	ccp text_break = 0;

	bool eol = false;
	while ( !eol && text < text_end && dest < buf_end )
	{
	    if ( *text >= 1 && *text <= 3 )
	    {
		// \1 : Text only for built in help
		// \2 : Text only for ui.def
		// \3 : Text for all (default)

		const bool skip = *text == ( is_makedoc ? 1 : 2 );
		//PRINT("CH=%d, SKIP=%d\n",*text,skip);
		text++;
		if (skip)
		    while ( text < text_end && (u8)*text > 3 )
			text++;
		continue;
	    }

	    switch (*text)
	    {
		case 0:
		    // ignore
		    break;

		case ' ':
		    if ( dest > buf )
		    {
			dest_break = dest;
			text_break = text;
		    }
		    *dest++ = *text++;
		    break;

		case '\n':
		    eol = true;
		    *dest++ = '\\';
		    *dest++ = 'n';
		    text++;
		    dest_break = 0;
		    text_break = 0;
		    break;

		case '\\':
		case '"':
		    *dest++ = '\\';
		    *dest++ = *text++;
		    break;

		case '{':
		    if (is_makedoc)
			*dest++ = *text++;
		    else if ( *++text == '{' )
			*dest++ = *text++;
		    else
		    {
			brace_active = true;
			ccp ptr = text;
			while ( ptr < text_end && *ptr > ' ' && *ptr != '}' )
			    ptr++;
			set_apos = *ptr != '}';
			if (set_apos)
			    *dest++ = '\'';
		    }
		    break;


		case '}':
		    if ( is_makedoc || !brace_active )
			*dest++ = *text++;
		    else if ( *++text == '}' )
			*dest++ = *text++;
		    else
		    {
			brace_active = false;
			if (set_apos)
			{
			    set_apos = false;
			    *dest++ = '\'';
			}
		    }
		    break;

		case '@':
		case '$':
		    if (!is_makedoc)
		    {
			if ( text[1] == *text )
			    *dest++ = *text++;
			text++;
			break;
		    }
		    // fall through

		default:
		    *dest++ = *text++;
	    }
	    DASSERT( dest < buf_end + 10 );
	}

	if ( dest >= buf_end && dest_break )
	{
	    dest = dest_break;
	    text = text_break;
	}

	if ( dest > buf && text < text_end )
	{
	    *dest = 0;
	    if (f)
		fprintf(f,"\t\"%s\"%s\n",buf,tie);
	    else
		pbuf = snprintfE(pbuf,pbuf_end,"\t\"%s\"%s\n",buf,tie);
	    dest = buf;
	}
    }

    *dest = 0;
    if (f)
	fprintf(f,"\t\"%s\"%s",buf,end);
    else
	snprintf(pbuf,pbuf_end-pbuf,"\t\"%s\"%s",buf,end);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   print options		///////////////
///////////////////////////////////////////////////////////////////////////////

static void print_single_option
(
    FILE		* f,		// valid output stream
    int			indent,		// normalized indent of output
    const InfoOption_t	* io,		// option to print
    uint		opt_fw,		// field width for options names
    uint		disp_fw		// total field width of displayed output
)
{
    DASSERT(f);
    DASSERT(io);

    if ( !io->short_name && !io->long_name )
    {
	fputc('\n',f);
	return;
    }

    ccp col_reset, col_param;
    if (IsFileColorized(f))
    {
	fputs(GetTextMode(1,TTM_COL_OPTION),f);
	col_param = io->param ? GetTextMode(1,TTM_COL_PARAM) : "";
	col_reset = TermTextModeReset;
    }
    else
	col_reset = col_param = "";

    ccp iop = io->param;
    ccp sep = !iop ? ""
	    : !io->optional_parm ? " "
	    : iop[0] == '=' || iop[0] == '[' && iop[0] == '=' ? ""
	    : "=";

    int len;
    if (!io->short_name)
	len = fprintf(f,"%*s     --%s%s%s%s ",
		indent, "",
		io->long_name,
		sep,
		col_param, iop ? iop : "" );
    else if (!io->long_name)
	len = fprintf(f,"%*s  -%c%s%s%s ",
		indent, "",
		io->short_name,
		iop ? " " : "",
		col_param, iop ? iop : "" );
    else
	len = fprintf(f,"%*s  -%c --%s%s%s%s ",
		indent, "",
		io->short_name,
		io->long_name,
		sep,
		col_param, iop ? iop : "" );

    len -= strlen(col_param);
    if ( len > opt_fw )
    {
	fprintf(f,"%s\n",col_reset);
	PutLines(f,opt_fw,disp_fw,0,0,io->help,0);
    }
    else
    {
	fputs(col_reset,f);
	PutLines(f,opt_fw,disp_fw,len,0,io->help,0);
    }
}

///////////////////////////////////////////////////////////////////////////////

static inline bool is_opt_selected
(
    const InfoUI_t	* iu,		// valid pointer
    const InfoOption_t	* io,		// active option
    bool		use_filter	// true: user filter
)
{
    DASSERT(io);
    return use_filter
	? ( iu->opt_used[io->id] & 0x80 ) != 0
	: !io->hidden;
}

///////////////////////////////////////////////////////////////////////////////

static void print_help_options
(
    const InfoUI_t	* iu,		// valid pointer
    FILE		* f,		// valid output stream
    bool		filter_opt,	// true: filter options by 'iu->opt_used'
    int			indent,		// indent of output
    uint		beg_index,	// index of first option to print
    uint		end_index,	// index of last option to print +1
    ccp			title		// NULL or title text
)
{
    DASSERT(iu);

    if ( end_index > iu->n_opt_total )
	 end_index = iu->n_opt_total;
    if ( beg_index >= end_index || !f )
	return;

    if (title)
    {
	const ColorMode_t colmode = GetFileColorized(f);
	fprintf(f,"%*s%s%s:%s\n\n",
		indent,"",
		GetTextMode(colmode,TTM_COL_HEADING), title,
		GetTextMode(colmode,TTM_RESET) );
    }

//    const int fw = GetTermWidth(80,40) - 1;
    const int fw = GetGoodTermWidth(0,false) - 1;

    const InfoOption_t *io_beg = iu->opt_info + beg_index;
    const InfoOption_t *io_end = iu->opt_info + end_index;
    const InfoOption_t *io;

    // calculate the base option field width
    int base_opt_fw = 0;
    for ( io = io_beg; io < io_end; io++ )
    {
	if ( io->long_name && is_opt_selected(iu,io,filter_opt) )
	{
	    int len = strlen(io->long_name);
	    if (io->param)
		len += 1 + strlen(io->param);
	    if ( base_opt_fw < len )
		 base_opt_fw = len;
	}
    }

    const int max_fw = 2 + (fw+5)/8;
    if ( base_opt_fw > max_fw )
	 base_opt_fw = max_fw;

    // calculate the option field width
    int opt_fw = 0;
    for ( io = io_beg; io < io_end; io++ )
    {
	if ( io->long_name && is_opt_selected(iu,io,filter_opt) )
	{
	    int len = strlen(io->long_name);
	    if (io->param)
		len += 1 + strlen(io->param);
	    if ( len <= base_opt_fw && opt_fw < len )
		opt_fw = len;
	}
    }
    opt_fw += indent + 9;

    for ( io = io_beg; io < io_end; io++ )
    {
	if ( is_opt_selected(iu,io,filter_opt) )
	{
	    if (io->separator)
		fputc('\n',f);
	    print_single_option(f,indent,io,opt_fw,fw);
	}
    }
    fputc('\n',f);
}

///////////////////////////////////////////////////////////////////////////////

void PrintHelpOptions
(
    const InfoUI_t	* iu,		// valid pointer
    FILE		* f,		// valid output stream
    int			indent		// indent of output
)
{
    DASSERT(iu);

    bool filter_opt = false;
    uint i;
    for ( i = 1; i < iu->n_opt_total; i++ )
    {
	const u8 used = iu->opt_used[i];
	if ( used & 0x80 && ( used != 0x81 || !iu->opt_info[i].ignore ) )
	{
	    filter_opt = true;
	    break;
	}
    }

    fputc('\n',f);
    if (filter_opt)
    {
	print_help_options( iu, f, true, indent,
			1, iu->n_opt_total,
			"Selected options with common description" );
    }
    else
    {
	print_help_options( iu, f, false, indent,
			iu->n_opt_specific+1, iu->n_opt_total,
			"Global options" );

	print_help_options( iu, f, false, indent,
			1, iu->n_opt_specific,
			"Command specific options with common description" );
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrintHelpCommands
(
    const InfoUI_t	* iu,		// valid pointer
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    ccp			help_cmd	// NULL or name of help command
)
{
    DASSERT(iu);

    if ( !iu->n_cmd )
	return;

    uint col_len;
    ccp col_head, col_cmd, col_reset;
    if ( IsFileColorized(f))
    {
	col_head  = GetTextMode(1,TTM_COL_HEADING);
	col_cmd   = GetTextMode(1,TTM_COL_CMD);
	col_reset = TermTextModeReset;
	col_len = strlen(col_cmd) + strlen(col_reset);
    }
    else
    {
	col_head = col_cmd = col_reset = "";
	col_len = 0;
    }

    fprintf(f,"\n%*s%sCommands:%s\n\n", indent,"", col_head, col_reset );
//    const int fw = GetTermWidth(80,40) - 1;
    const int fw = GetGoodTermWidth(0,false) - 1;

    int fw1 = 0, fw2 = 0;
    const InfoCommand_t * ic;

    for ( ic = iu->cmd_info; ic->name1; ic++ )
	if (!ic->hidden)
	{

	    const int len = strlen(ic->name1);
	    if ( fw1 < len )
		 fw1 = len;
	    if ( ic->name2 )
	    {
		const int len = strlen(ic->name2);
		if ( fw2 < len )
		     fw2 = len;
	    }
	}
    const int fw12 = fw1 + ( fw2 ? fw2 + 3 : 0 );

    for ( ic = iu->cmd_info+1; ic->name1; ic++ )
	if (!ic->hidden)
	{
	    if (ic->separator)
		fputc('\n',f);
	    int len;
	    if ( ic->name2 )
		len = fprintf(f,"%*s  %s%-*s%s | %s%-*s%s : ",
			indent, "",
			col_cmd, fw1, ic->name1, col_reset,
			col_cmd, fw2, ic->name2, col_reset )
		    - 2*col_len;
	    else
		len = fprintf(f,"%*s  %s%-*s%s : ",
			indent, "",
			col_cmd, fw12, ic->name1, col_reset )
		    - col_len;
	    PutLines(f,indent+len,fw,len,0,ic->help,0);
	}

    if (help_cmd)
	fprintf(f,
	    "\n%*sType '%s %s command' to get command specific help.\n\n",
	    indent, "", iu->tool_name, help_cmd );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   print help			///////////////
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
)
{
    ASSERT(iu);

    int cmd = 0;
    if ( iu->n_cmd && first_param && *first_param )
    {
	int cmd_stat;
	const KeywordTab_t * ct = ScanKeyword(&cmd_stat,first_param,iu->cmd_tab);
	if (ct)
	    cmd = ct->id;
	else
	{
	    char buf[100];
	    StringCat2S(buf,sizeof(buf),"+",first_param);
	    ct = ScanKeyword(&cmd_stat,buf,iu->cmd_tab);
	    if (ct)
		cmd = ct->id;
	    else
	    {
		char key_buf[12];
		char *dest = key_buf;
		char *end  = key_buf + sizeof(key_buf) - 1;
		while ( *first_param && dest < end )
		    *dest++ = toupper((int)*first_param++);
		*dest = 0;
		const int key_len = dest - key_buf;
		if ( key_len >= 3 )
		{
		    if (!memcmp(key_buf,"OPTIONS",key_len) )
		    {
			PrintHelpOptions(iu,f,indent);
			return;
		    }

		    if ( !memcmp(key_buf,"COMMANDS",key_len)
			|| !memcmp(key_buf,"CMD",key_len) )
		    {
			PrintHelpCommands(iu,f,indent,help_cmd);
			return;
		    }
		}
	    }
	}
    }

    PrintHelpCmd(iu,f,indent,cmd,help_cmd,info,base_uri);
}

///////////////////////////////////////////////////////////////////////////////

void PrintHelpCmd
(
    const InfoUI_t	* iu,		// valid pointer
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    int			cmd,		// index of command
    ccp			help_cmd,	// NULL or name of help command
    ccp			info,		// NULL or poiner to additional text
    ccp			base_uri	// NULL or base URI for a external help link
)
{
    ASSERT(iu);
    if (!f)
	return;
    indent = NormalizeIndent(indent);

    if ( cmd < 0 || cmd >= iu->n_cmd )
	cmd = 0;
    const InfoCommand_t * ic = iu->cmd_info + cmd;

//    const int fw = GetTermWidth(80,40) - 1;
    const int fw = GetGoodTermWidth(0,false) - 1;

    uint col_len;
    ccp col_head, col_reset;
    if (IsFileColorized(f))
    {
	col_head  = GetTextMode(1,TTM_COL_HEADING);
	col_reset = GetTextMode(1,TTM_RESET);
	col_len   = strlen(col_head) + strlen(col_reset);
    }
    else
    {
	col_head = col_reset = "";
	col_len  = 0;
    }

    char iobuf[10000];

    if (!cmd)
	snprintf(iobuf,sizeof(iobuf),"%s",iu->tool_name );
    else if (ic->name2)
	snprintf(iobuf,sizeof(iobuf),"%s %s|%s",iu->tool_name,ic->name1,ic->name2);
    else
	snprintf(iobuf,sizeof(iobuf),"%s %s",iu->tool_name,ic->name1);

    int len = fprintf(f,"\n%*s%s : ",indent,"",iobuf) - 1;
    PutLines(f,indent+len,fw,len,0, ic->xhelp ? ic->xhelp : ic->help, 0);

    len = fprintf(f,"\n%*s%sSyntax:%s ", indent, "", col_head, col_reset ) - 1 - col_len;
    PutLines(f,indent+len,fw,len,0,ic->syntax,0);
    fputc('\n',f);

    if (!cmd)
	PrintHelpCommands(iu,f,indent,help_cmd);

    if ( ic->n_opt )
    {
	fprintf(f,"%*s%s%sptions:%s\n\n",
		indent,"", col_head,
		!cmd && iu->n_cmd ? "Global o" : "O",
		col_reset );

	const InfoOption_t ** iop;

	// calculate the base option field width
	int base_opt_fw = 0;
	for ( iop = ic->opt; *iop; iop++ )
	{
	    const InfoOption_t * io = *iop;
	    if (io->long_name)
	    {
		int len = strlen(io->long_name);
		if (io->param)
		    len += 1 + strlen(io->param);
		if ( base_opt_fw < len )
		     base_opt_fw = len;
	    }
	}

	const int max_fw = 2 + (fw+5)/8;
	if ( base_opt_fw > max_fw )
	     base_opt_fw = max_fw;

	// calculate the option field width
	int opt_fw = 0;
	for ( iop = ic->opt; *iop; iop++ )
	{
	    const InfoOption_t * io = *iop;
	    if (io->long_name)
	    {
		int len = strlen(io->long_name);
		if (io->param)
		    len += 1 + strlen(io->param);
		if ( len <= base_opt_fw && opt_fw < len )
		    opt_fw = len;
	    }
	}
	opt_fw += indent + 9;

	for ( iop = ic->opt; *iop; iop++ )
	    print_single_option(f,indent,*iop,opt_fw,fw);
	fputc('\n',f);
    }

    if (info)
	fputs(info,f);

    if ( base_uri && *base_uri )
    {
	if (!cmd)
	    fprintf(f,"%*sMore help is available at %s%s\n\n",
		    indent, "", base_uri, iu->tool_name );
	else if (!ic->hidden)
	{
	    char *dest = iobuf;
	    ccp src;
	    for ( src = ic->name1; *src; src++ )
		*dest++ = *src == '_' ? '-' : tolower((int)*src);
	    *dest = 0;

	    fprintf(f,"%*sMore help is available at %scmd/%s/%s\n\n",
			indent, "", base_uri, iu->tool_name, iobuf );
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

