
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

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dclib-basics.h"
#include "dclib-ui.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct control_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct control_t
{
    FILE		* cf;		// output c file
    FILE		* hf;		// output h file
    FILE		* df;		// output def file

    const info_t	* info;		// pointer to first info_t
    const info_t	* end;		// pointer to end of info_t

    int			n_cmd;		// number of commands
    int			n_grp;		// number of helper groups
    int			n_opt_specific;	// number of cmd specific options (=OPT__N_SPECIFIC)
    int			n_opt;		// number of options (=OPT__N_TOTAL)
    int			n_cmd_opt;	// number of options of current command
    bool		need_sep;	// separator needed
    ccp			cmd_name;	// name of current command
    ccp			opt_prefix;	// prefix for option variables, never NULL

    u8			*opt_allow_grp;	// grp allowed options: field[n_grp][n_opt_specific]
    u8			*opt_allow_cmd;	// cmd allowed options: field[n_cmd][n_opt_specific]

    StringField_t	gopt;		// global options
    StringField_t	copt;		// command specfic options
    StringField_t	opt_done;	// option handled

} control_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static char tabs[] = "\t\t\t\t";

static char sep1[] =
	"////////////////////////////////////////"
	"///////////////////////////////////////";

static char sep2[] =
	"########################################"
	"#######################################";

//-----------------------------------------------------------------------------

static void print_section ( FILE *f, ccp sep, ccp name )
{
    const size_t namelen = strlen(name);
    const int fill = ( 49 - namelen ) /2;
    const int fw   = 49 - fill;
    fprintf(f,"\n%.2s\f\n%s\n%.15s%*s%*s%.15s\n%s\n\n",
		sep, sep, sep, fill, "", -fw, name, sep, sep );
}

//-----------------------------------------------------------------------------

static void print_info_opt
(
    control_t		* ctrl,
    const info_t	* info,
    const info_t	* default_info,
    const info_t	* first_info,	// NULL or first info of a list of infos
    ccp			cmd_name
)
{
    ASSERT(ctrl);
    ASSERT(info);

    FILE * cf = ctrl->cf;
    ASSERT(cf);

    if (cmd_name)
	fprintf(cf,"static const InfoOption_t option_%s_%s_%s =\n",
		ctrl->opt_prefix, cmd_name, info->c_name );

    fprintf(cf,"    {\tOPT_%s, %s, %s, %s, %s, %s, ",
		info->c_name,
		info->type & F_OPT_OPTPARAM	? "true" : "false",
		info->type & F_HIDDEN		? "true" : "false",
		info->type & F_DEPRECATED	? "true" : "false",
		info->type & F_IGNORE		? "true" : "false",
		first_info && info>first_info && info[-1].type&T_SEP_OPT ? "true" : "false" );
    ccp start = ( default_info ? default_info : info ) -> namelist;
    if ( *start && start[1] == '|' )
    {
	fprintf(cf,"'%c', ",*start);
	start += 2;
    }
    else
	fprintf(cf,"0, ");

    ccp end = start;
    while ( *end && *end != '|' )
	end++;
 #ifdef TEST0
    fprintf(cf,"\"%.*s\", // hid=%x, dep=%x, sep=%x\n",
		(int)(end-start), start,
		info->type & F_HIDDEN,
		info->type & F_DEPRECATED,
		info > ctrl->info ? info[-1].type & T_SEP_OPT : 0 );
 #else
    fprintf(cf,"\"%.*s\",\n", (int)(end-start), start );
 #endif

    ccp param = info->param;
    ccp help  = info->help;
    if (default_info)
    {
	if (!help)
	    help  = default_info->help;
	if (!param)
	    param  = default_info->param;
    }

    if ( info->type & F_OPT_XPARAM || default_info && default_info->type & F_OPT_XPARAM )
	fprintf(cf,"\t\"%s\",\n", param && *param ? param : "param" );
    else
	fprintf(cf,"\t0,\n");

    DumpText(cf,0,0,help,-1,0,"");
    fprintf(cf,"\n    }%c\n\n",cmd_name ? ';' : ',' );
}

//-----------------------------------------------------------------------------

static void print_opt
(
    control_t		* ctrl,
    const info_t	* info,
    const info_t	* first_info	// NULL or first info of a list of infos
)
{
    ASSERT(ctrl);
    ASSERT(info);

    ctrl->n_opt++;
    print_info_opt(ctrl,info,0,first_info,0);
    fprintf( ctrl->hf, "\tOPT_%s,\n", info->c_name );
};

//-----------------------------------------------------------------------------

static void print_opt_link ( control_t * ctrl, const info_t * info )
{
    ASSERT(ctrl);
    ASSERT(ctrl->cf);
    ASSERT(ctrl->opt_prefix);
    ASSERT(info);

    if (!InsertStringField(&ctrl->opt_done,info->c_name,false))
	return;

    if ( ctrl->need_sep )
    {
	ctrl->need_sep = false;
	if ( ctrl->n_cmd_opt )
	    fprintf(ctrl->cf,"\n\tOptionInfo + OPT_NONE, // separator\n\n" );
    }

    if ( !info->help || info->type & T_DEF_OPT )
	fprintf(ctrl->cf,"\tOptionInfo + OPT_%s,\n", info->c_name );
    else
	fprintf(ctrl->cf,"\t&option_%s_%s,\n", ctrl->opt_prefix, info->c_name );

    if (ctrl->cmd_name)
    {
	fprintf(ctrl->df,"#:def_cmd_opt( \"%s\", \"%s\", \\\n",
		ctrl->cmd_name, info->c_name );
	DumpText(ctrl->df,0,0,info->param,-1,1,", \\\n");
	DumpText(ctrl->df,0,0,info->help,-1,1," )\n\n");
    }

    ctrl->n_cmd_opt++;
}

//-----------------------------------------------------------------------------

static const info_t * print_links_iterator
	( control_t * ctrl, const info_t * info, ccp prefix );

//-----------------------------------------------------------------------------

static void print_cmd_links ( control_t * ctrl, const info_t * ref )
{
    ASSERT(ctrl);
    ASSERT(ref);

    const info_t * info;
    for ( info = ctrl->info; info < ctrl->end; info++ )
	if ( info->type & T_CMD_BEG && !strcmp(info->c_name,ref->c_name) )
	{
	    print_links_iterator(ctrl,info,"cmd");
	    break;
	}
}

//-----------------------------------------------------------------------------

static void print_grp_links ( control_t * ctrl, const info_t * ref )
{
    ASSERT(ctrl);
    ASSERT(ref);

    const info_t * info;
    for ( info = ctrl->info; info < ctrl->end; info++ )
	if ( info->type & T_GRP_BEG && !strcmp(info->c_name,ref->c_name) )
	{
	    print_links_iterator(ctrl,info,"grp");
	    break;
	}
}

//-----------------------------------------------------------------------------

static const info_t * print_links_iterator
	( control_t * ctrl, const info_t * info, ccp prefix )
{
    ASSERT(ctrl);
    ASSERT(info);

    ccp saved_prefix = ctrl->opt_prefix;
    char buf[100];
    if (prefix)
    {
	snprintf(buf,sizeof(buf),"%s_%s",prefix,info->c_name);
	ctrl->opt_prefix = buf;
    };

    for ( info++; info < ctrl->end && !(info->type & (T_CMD_BEG|T_GRP_BEG) ); info++ )
    {
	if ( info->type & F_HIDDEN )
	{
	    // ignored -> do nothing
	}
	else if ( info->type & T_SEP_OPT )
	    ctrl->need_sep = true;
	else if ( info->type & (F_OPT_GLOBAL|T_CMD_OPT) )
	    print_opt_link(ctrl,info);
	else if ( info->type & T_COPY_CMD )
	    print_cmd_links(ctrl,info);
	else if ( info->type & T_COPY_GRP )
	    print_grp_links(ctrl,info);
    }

    ctrl->opt_prefix = saved_prefix;
    return info;
}

//-----------------------------------------------------------------------------

static void dump_help ( const info_t * info_cmd )
{
    char * temp_param	= iobuf;
    char * temp_help	= iobuf + sizeof(iobuf)/8;
    char * temp_xhelp	= iobuf + sizeof(iobuf)/4;

    DumpText(0,temp_param,iobuf+sizeof(iobuf),info_cmd->param,-1,0,"");
    ccp ff = info_cmd->help ? strchr(info_cmd->help,'\f') : 0;
    if (ff)
    {
	DumpText(0,temp_help,iobuf+sizeof(iobuf),info_cmd->help,ff-info_cmd->help,0,"");
	DumpText(0,temp_xhelp,iobuf+sizeof(iobuf),ff+1,-1,0,"");
    }
    else
    {
	DumpText(0,temp_help,iobuf+sizeof(iobuf),info_cmd->help,-1,0,"");
	StringCopyE(temp_xhelp,iobuf+sizeof(iobuf),"\t0");
    }
}

//-----------------------------------------------------------------------------

static void print_links ( control_t * ctrl )
{
    ASSERT(ctrl);
    FILE * cf = ctrl->cf;
    ASSERT(cf);

    print_section(cf,sep1,"InfoOption tabs");

    char * temp_param	= iobuf;
    char * temp_help	= iobuf + sizeof(iobuf)/8;
    char * temp_xhelp	= iobuf + sizeof(iobuf)/4;
    char * sum_beg	= iobuf + sizeof(iobuf)/2;
    char * sum_end	= iobuf + sizeof(iobuf);
    char * sum		= sum_beg;

    const info_t * info_cmd = ctrl->info;
    ASSERT ( info_cmd->type & T_DEF_TOOL );
    fprintf(cf,"static const InfoOption_t * option_tab_tool[] =\n{\n");

    ctrl->n_cmd_opt	= 0;
    ctrl->need_sep	= false;

    print_links_iterator(ctrl,info_cmd,"def");
    fprintf(cf,"\n\t0\n};\n\n");

    dump_help(info_cmd);
    sum = snprintfE(sum,sum_end,
			"    {\t0,\n"		// id
			"\tfalse,\n"		// hidden
			"\tfalse,\n"		// deprecated
			"\tfalse,\n"		// separator
			"\t\"%s\",\n"		// name1
			"\t0,\n"		// name2
			"%s,\n"			// param
			"%s,\n"			// help
			"%s,\n"			// xhelp
			"\t%u,\n"		// n_opt
			"\toption_tab_tool,\n"	// opt
			"\t0\n"			// opt_allowed
			"    },\n\n"
			,info_cmd->c_name
			,temp_param
			,temp_help
			,temp_xhelp
			,ctrl->n_cmd_opt
			);

    bool separator = false;
    for ( info_cmd++; info_cmd < ctrl->end; info_cmd++ )
    {
	if ( info_cmd->type & T_SEP_CMD )
	    separator = true;
	//if ( !( info_cmd->type & T_DEF_CMD ) || info_cmd->type & F_HIDDEN )
	if ( !( info_cmd->type & T_DEF_CMD ) )
	    continue;

	fprintf(cf,"static const InfoOption_t * option_tab_cmd_%s[] =\n{\n",
		info_cmd->c_name);

	ctrl->n_cmd_opt	= 0;
	ctrl->need_sep	= false;
	ctrl->cmd_name	= info_cmd->c_name;
	ResetStringField(&ctrl->opt_done);

	const info_t * info;
	for ( info = info_cmd;  info < ctrl->end; info++ )
	    if ( info->type & T_CMD_BEG && !strcmp(info->c_name,info_cmd->c_name) )
	    {
		print_links_iterator(ctrl,info,"cmd");
		break;
	    }
	fprintf(cf,"\n\t0\n};\n\n");

	ccp name1 = info_cmd->namelist, ptr = name1;
	while ( *ptr && *ptr != '|' )
	    ptr++;
	const int len1 = ptr - name1;
	char name2[100] = "0";
	if ( *ptr == '|' )
	{
	    ccp n2 = ++ptr;
	    while ( *ptr && *ptr != '|' )
		ptr++;
	    if ( ptr > n2 )
		snprintf(name2,sizeof(name2),"\"%.*s\"",(int)(ptr-n2),n2);
	}

	dump_help(info_cmd);
	sum = snprintfE(sum,sum_end,
			"    {\tCMD_%s,\n"		// id
			"\t%s,\n"			// hidden
			"\t%s,\n"			// deprecated
			"\t%s,\n"			// separator
			"\t\"%.*s\",\n"			// name1
			"\t%s,\n"			// name2
			"%s,\n"				// param
			"%s,\n"				// help
			"%s,\n"				// xhelp
			"\t%u,\n"			// n_opt
			"\toption_tab_cmd_%s,\n"	// opt
			"\toption_allowed_cmd_%s\n"	// opt_allowed
			"    },\n\n"
			,info_cmd->c_name
			,info_cmd->type & F_HIDDEN	 ? "true" : "false"
			,info_cmd->type & F_DEPRECATED	 ? "true" : "false"
			,separator ? "true" : "false"
			,len1 ,name1
			,name2
			,temp_param
			,temp_help
			,temp_xhelp
			,ctrl->n_cmd_opt
			,info_cmd->c_name
			,info_cmd->c_name
			);
	separator = false;
    }

    print_section(cf,sep1,"InfoCommand");
    fprintf(cf,"static const InfoCommand_t CommandInfo[CMD__N+1] =\n{\n");
    fputs(sum_beg,cf);
    fprintf(cf,"    {0,0,0,0,0,0,0,0,0,0,0,0}\n};\n");
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Generate()			///////////////
///////////////////////////////////////////////////////////////////////////////

static char var_buf[10000];

//-----------------------------------------------------------------------------

static void CheckUsedName ( StringField_t *list, ccp name, uint len, ccp title )
{
    //printf("CHECK: |%.*s|\n",len,name);
    ccp key = MEMDUP(name,len);
    if (!InsertStringField(list,key,len))
	fprintf(stderr,"!UI-WARNING: %s already defined: %.*s\n", title, len, name );
}

//-----------------------------------------------------------------------------

static enumError Generate ( control_t * ctrl )
{
    ASSERT(ctrl);
    ASSERT(ctrl->info);

    FILE * cf = ctrl->cf;
    FILE * hf = ctrl->hf;
    ASSERT(cf);
    ASSERT(hf);

    FREE(ctrl->opt_allow_grp);
    FREE(ctrl->opt_allow_cmd);
    ctrl->opt_allow_grp = ctrl->opt_allow_cmd = 0;

    const info_t *info;
    StringField_t used_name;
    InitializeStringField(&used_name);


    //----- ui header

    fprintf(cf,text_ui_head);
    fprintf(hf,text_ui_head);


    //----- setup guard

    char guard[100];
    snprintf(guard,sizeof(guard),"SZS_UI_%s_h",ctrl->info->c_name);
    char * ptr;
    for ( ptr = guard; *ptr; ptr++ )
	*ptr = *ptr == '-' ? '_' : toupper((int)*ptr);
    fprintf(hf,"\n#ifndef %s\n#define %s\n",guard,guard);


    //----- header

    ASSERT( ctrl->info->type & T_DEF_TOOL );
    ccp tool_name = ctrl->info->c_name;

    fprintf(cf,"#include <getopt.h>\n");
    fprintf(cf,"#include \"ui-%s.h\"\n",tool_name);

    fprintf(hf,"#include \"dclib-basics.h\"\n");
    fprintf(hf,"#include \"dclib-ui.h\"\n");


    //----- print enum enumOptions & OptionInfo[]

    print_section(cf,sep1,"OptionInfo[]");
    print_section(hf,sep1,"enum enumOptions");

    char * var_ptr = var_buf;
    char * var_end = var_buf + sizeof(var_buf);
    var_ptr = snprintfE(var_ptr,var_end,
		"//extern const InfoOption_t OptionInfo[OPT__N_TOTAL+1];\n");

    fprintf(cf,
	    "static const InfoOption_t OptionInfo[OPT__N_TOTAL+1] =\n"
	    "{\n"
	    "    {0,0,0,0,0,0,0,0,0,0}, // OPT_NONE,\n"
	    "\n"
	    );

    fprintf(hf,
	    "typedef enum enumOptions\n"
	    "{\n"
	    "\tOPT_NONE,\n"
	    "\n"
	    );

    ctrl->n_opt = 1;
    ctrl->n_opt_specific = 0;
    if ( ctrl->n_cmd )
    {
	fprintf(cf,"    //----- command specific options -----\n\n");
	fprintf(hf,"\t//----- command specific options -----\n\n");

	const info_t *first_info = 0;
	for ( info = ctrl->info; info < ctrl->end; info++ )
	    if ( info->type & F_OPT_COMMAND )
	    {
		if (!first_info)
		    first_info = info;
		print_opt(ctrl,info,first_info);
	    }

	ctrl->n_opt_specific = ctrl->n_opt;

	fprintf(cf,
		"    {0,0,0,0,0,0,0,0,0,0}, // OPT__N_SPECIFIC == %d\n\n"
		"    //----- global options -----\n\n",
		ctrl->n_opt_specific );

	fprintf(hf,
		"\n\tOPT__N_SPECIFIC, // == %d \n\n"
		"\t//----- global options -----\n\n",
		ctrl->n_opt_specific );
    }

    const info_t *first_info = 0;
    for ( info = ctrl->info; info < ctrl->end; info++ )
	if ( info->type & F_OPT_GLOBAL )
	{
	    if (!first_info)
		first_info = info;
	    print_opt(ctrl,info,first_info);
	}

    fprintf(cf,
	    "    {0,0,0,0,0,0,0,0,0,0} // OPT__N_TOTAL == %d\n\n"
	    "};\n"
	    ,ctrl->n_opt );

    fprintf(hf,
	    "\n\tOPT__N_TOTAL // == %d\n\n"
	    "} enumOptions;\n"
	    ,ctrl->n_opt );

    if (ctrl->n_opt_specific)
    {
	noTRACE("opt_allowed = ( %2u + %2u ) * %2u\n",
		ctrl->n_grp, ctrl->n_cmd, ctrl->n_opt_specific );
	if (ctrl->n_grp)
	{
	    ctrl->opt_allow_grp = CALLOC(ctrl->n_grp,ctrl->n_opt_specific);
	}
	ctrl->opt_allow_cmd = CALLOC(ctrl->n_cmd,ctrl->n_opt_specific);
    }


    //----- print alternate option infos

    bool done = false;
    const info_t * last_cmd = ctrl->info;
    ctrl->opt_prefix = "def";
    for ( info = ctrl->info; info < ctrl->end; info++ )
    {
	if ( info->type & T_CMD_BEG )
	{
	    ctrl->opt_prefix = "cmd";
	    last_cmd = info;
	}
	else if ( info->type & T_GRP_BEG )
	{
	    ctrl->opt_prefix = "grp";
	    last_cmd = info;
	}
	else if ( info->type & T_CMD_OPT && info->help && *info->help )
	{
	    if (!done)
	    {
		print_section(cf,sep1,"alternate option infos");
		done = true;
	    }

	    const info_t * info0;
	    for ( info0 = ctrl->info; info0 < info; info0++ )
		if ( info0->type & T_DEF_OPT && !strcmp(info->c_name,info0->c_name) )
		{
		    print_info_opt(ctrl,info,info0,0,last_cmd->c_name);
		    break;
		}
	}
    }

    //----- print enum enumOptionsBit

    if ( ctrl->n_cmd )
    {
	print_section(hf,sep1,"enum enumOptionsBit");

	fprintf(hf,
		"//\t*****  only for verification  *****\n"
		"\n"
		"//typedef enum enumOptionsBit\n"
		"//{\n"
		"//\t//----- command specific options -----\n"
		"//\n"
		);

	for ( info = ctrl->info; info < ctrl->end; info++ )
	    if ( info->type & F_OPT_COMMAND )
		fprintf(hf,"//\tOB_%s%.*s= 1llu << OPT_%s,\n",
			info->c_name,
			( 28 - (int)strlen(info->c_name) ) / 8, tabs,
			info->c_name );

	fprintf(hf,"//\n//\t//----- group & command options -----\n");

	for ( info = ctrl->info; info < ctrl->end; )
	{
	    ccp cmd_name;
	    u8 * opt_allow = 0;
	    if ( info->type & T_CMD_BEG )
	    {
		cmd_name = info->c_name;
		fprintf(hf,"//\n//\tOB_CMD_%s%.*s=",
			info->c_name,
			( 24 - (int)strlen(info->c_name) ) / 8, tabs );
		if (ctrl->opt_allow_cmd)
		{
		    //PRINT("SELECT ALLOW CMD %u/%s\n",info->index,info->c_name);
		    opt_allow = ctrl->opt_allow_cmd + info->index * ctrl->n_opt_specific;
		}
	    }
	    else if ( info->type & T_GRP_BEG )
	    {
		cmd_name = info->c_name;
		fprintf(hf,"//\n//\tOB_GRP_%s%.*s=",
			info->c_name,
			( 24 - (int)strlen(info->c_name) ) / 8, tabs );
		if (ctrl->opt_allow_grp)
		{
		    //PRINT("SELECT ALLOW GRP %u/%s\n",info->index,info->c_name);
		    opt_allow = ctrl->opt_allow_grp + info->index * ctrl->n_opt_specific;
		}
	    }
	    else
	    {
		info++;
		continue;
	    }

	    info++;
	    char * dest = iobuf;
	    while ( info < ctrl->end )
	    {
		if ( info->type & T_ALL_OPT )
		{
		    dest += sprintf(dest,"\n//\t\t\t\t| ~(u64)0");
		    if (opt_allow)
		    {
			//PRINT("ALLOW ALL\n");
			memset(opt_allow,1,ctrl->n_opt_specific);
		    }
		}
		else if ( info->type & T_COPY_CMD )
		{
		    dest += sprintf(dest,"\n//\t\t\t\t| OB_CMD_%s",info->c_name);
		    if (opt_allow)
		    {
			//PRINT("OR CMD %u/%s\n",info->index,info->c_name);
			DASSERT(ctrl->opt_allow_cmd);
			u8 * src = ctrl->opt_allow_cmd + info->index * ctrl->n_opt_specific;
			u8 * dest = opt_allow;
			int count = ctrl->n_opt_specific;
			while ( count-- > 0 )
			    *dest++ |= *src++;
		    }
		}
		else if ( info->type & T_COPY_GRP )
		{
		    dest += sprintf(dest,"\n//\t\t\t\t| OB_GRP_%s",info->c_name);
		    if ( opt_allow && ctrl->opt_allow_grp )
		    {
			//PRINT("OR GRP %u/%s\n",info->index,info->c_name);
			u8 * src = ctrl->opt_allow_grp + info->index * ctrl->n_opt_specific;
			u8 * dest = opt_allow;
			int count = ctrl->n_opt_specific;
			while ( count-- > 0 )
			    *dest++ |= *src++;
		    }
		}
		else if ( info->type & T_CMD_OPT )
		{
		    if (FindStringField(&ctrl->copt,info->c_name))
		    {
			dest += sprintf(dest,"\n//\t\t\t\t| OB_%s",info->c_name);
			if ( opt_allow && info->index )
			{
			    //PRINT("ALLOW OPT %u/%s\n",info->index,info->c_name);
			    opt_allow[info->index] = 1;
			}
		    }
		    else if (!FindStringField(&ctrl->gopt,info->c_name))
			ERROR0(ERR_SEMANTIC,"Option not defined: %s %s --%s",
				tool_name, cmd_name, info->c_name );
		}
		else if ( info->type & (T_CMD_BEG|T_GRP_BEG) )
		    break;
		ASSERT( dest < iobuf + sizeof(iobuf) );
		info++;
	    }
	    if ( dest == iobuf )
		fprintf(hf," 0,\n");
	    else
		fprintf(hf,"%s,\n",iobuf+8);
	}

	fprintf(hf,"//\n//} enumOptionsBit;\n");
    }


    //----- print enum enumCommands & CommandTab[]

    print_section(hf,sep1,"enum enumCommands");
    fprintf(hf,
	    "typedef enum enumCommands\n"
	    "{\n"
	    "\tCMD__NONE,"
	    );

    if ( ctrl->n_cmd )
    {
	print_section(cf,sep1,"CommandTab[]");
	fputs("\n\n",hf);

	var_ptr = snprintfE(var_ptr,var_end,
		"//extern const KeywordTab_t CommandTab[];\n");

	fprintf(cf,
		"static const KeywordTab_t CommandTab[] =\n"
		"{\n"
		);

	for ( info = ctrl->info; info < ctrl->end; info++ )
	    if ( info->type & T_DEF_CMD )
	    {
		fprintf( hf, "\tCMD_%s,\n",info->c_name);
		ccp ptr = info->namelist;
		while (*ptr)
		{
		    ccp n1 = ptr;
		    while ( *ptr && *ptr != '|' )
			ptr++;
		    const int l1 = ptr - n1;
		    CheckUsedName(&used_name,n1,l1,"Command");
		    while ( *ptr == '|' )
			ptr++;
		    if (*ptr)
		    {
			ccp n2 = ptr;
			while ( *ptr && *ptr != '|' )
			    ptr++;
			const int l2 = ptr - n2;
			CheckUsedName(&used_name,n2,l2,"Command");

			fprintf(cf,
				"    { CMD_%s,%.*s\"%.*s\",%.*s\"%.*s\",%.*s0 },\n",
				info->c_name, (20-(int)strlen(info->c_name))/8, tabs,
				l1, n1, (20-l1)/8, tabs,
				l2, n2, (20-l2)/8, tabs );

			while ( *ptr == '|' )
			    ptr++;
		    }
		    else
			fprintf(cf, "    { CMD_%s,%.*s\"%.*s\",%.*s0,\t\t0 },\n",
				info->c_name, (20-(int)strlen(info->c_name))/8, tabs,
				l1, n1, (20-l1)/8, tabs );
		}
	    }
	    else if ( info->type == T_SEP_CMD )
		fprintf(hf,"\n");

	fprintf(cf,
		"\n    { CMD__N,0,0,0 }\n"
		"};\n"
		);

	ResetStringField(&used_name);
    }

    fprintf(hf,
	    "\n\tCMD__N // == %u\n\n"
	    "} enumCommands;\n"
	    , ctrl->n_cmd );


    //----- print options

    print_section(cf,sep1,"OptionShort & OptionLong");

    char * dest = iobuf;
    for ( info = ctrl->info; info < ctrl->end; info++ )
	if ( info->type & T_DEF_OPT && info->namelist[1] == '|' )
	{
	    CheckUsedName(&used_name,info->namelist,1,"Short option");
	    *dest++ = info->namelist[0];
	    if ( ( info->type & (F_OPT_OPTPARAM|F_OPT_PARAM) ) == F_OPT_PARAM )
		*dest++ = ':';
	}
    *dest = 0;
    fprintf(cf,"static const char OptionShort[] = \"%s\";\n\n",iobuf);
    var_ptr = snprintfE(var_ptr,var_end,
		"//extern const char OptionShort[];\n");

    ccp opt_buf[UIOPT_INDEX_SIZE];
    memset(opt_buf,0,sizeof(opt_buf));
    int getopt_idx = UIOPT_LONG_BASE;
    ResetStringField(&used_name);

    fprintf(cf,"static const struct option OptionLong[] =\n{\n");
    var_ptr = snprintfE(var_ptr,var_end,
		"//extern const struct option OptionLong[];\n");

    for ( info = ctrl->info; info < ctrl->end; info++ )
	if ( info->type & T_DEF_OPT )
	{
	    ccp ptr = info->namelist;
	    const int pmode = (info->type & F_OPT_OPTPARAM)
				? 2
				: (info->type & F_OPT_PARAM)
					? 1
					: 0;

	    if ( info->namelist[1] == '|' )
	    {
		snprintf(iobuf,sizeof(iobuf),"%d, 0, '%c'",
			pmode, info->namelist[0] );
		ptr += 2;
		opt_buf[(u8)(info->namelist[0])] = info->c_name;
	    }
	    else
	    {
		snprintf(iobuf,sizeof(iobuf),"%d, 0, GO_%s",
			pmode, info->c_name );
		ASSERT_MSG( getopt_idx < UIOPT_INDEX_SIZE,
				"getopt_idx >= UIOPT_INDEX_SIZE [ 0x%x >= 0x%x ]\n",
				getopt_idx, UIOPT_INDEX_SIZE );
		if ( getopt_idx < UIOPT_INDEX_SIZE )
		    opt_buf[getopt_idx++] = info->c_name;
	    }

	    int indent = 0;
	    while (*ptr)
	    {
		ccp start = ptr;
		while ( *ptr && *ptr != '|' )
		    ptr++;
		const int len = ptr - start;
		if (len)
		{
		    CheckUsedName(&used_name,start,len,"Long option");
		    fprintf(cf,"\t%s{ \"%.*s\",%.*s%s },\n",
			indent ? " " : "", len, start, (26-len-indent)/8, tabs, iobuf );
		    indent = 1;
		}
		if (*ptr)
		    ptr++;
	    }
	}
    fprintf(cf,"\n\t{0,0,0,0}\n};\n");


    //----- print enumGetOpt

    print_section(hf,sep1,"enumGetOpt");
    fprintf(hf,"typedef enum enumGetOpt\n{");

    // add '?' temporary;
    ASSERT(!opt_buf['?']);
    opt_buf['?'] = "_ERR";

    static const int septab[] = { 0, '0', '9'+1, '?', '?'+1,
				  'A', 'Z'+1, 'a', 'z'+1,
				  UIOPT_LONG_BASE, UIOPT_INDEX_SIZE };
    const int * sepptr = septab;
    int i;
    for ( i = 0; i < UIOPT_INDEX_SIZE; i++ )
	if ( opt_buf[i] )
	{
	    if ( i >= *sepptr )
	    {
		fputc('\n',hf);
		while ( i >= *sepptr )
		    sepptr++;
	    }
	    if ( i < UIOPT_LONG_BASE )
		fprintf(hf,"\tGO_%s%.*s= '%c',\n",
			opt_buf[i], (28-(int)strlen(opt_buf[i]))/8, tabs, i );
	    else if ( i == UIOPT_LONG_BASE )
		fprintf(hf,"\tGO_%s%.*s= 0x%02x,\n",
			opt_buf[i], (28-(int)strlen(opt_buf[i]))/8, tabs, i );
	    else
		fprintf(hf,"\tGO_%s,\n",opt_buf[i]);
	}

    fprintf(hf,"\n} enumGetOpt;\n");
    opt_buf['?'] = 0;


    //----- print option index

    print_section(cf,sep1,"OptionUsed & OptionIndex");

    fprintf(cf,"static u8 OptionUsed[OPT__N_TOTAL+1] = {0};\n\n");
    var_ptr = snprintfE(var_ptr,var_end,
		"//extern u8 OptionUsed[OPT__N_TOTAL+1];\n");

    fprintf(cf,"static const OptionIndex_t OptionIndex[UIOPT_INDEX_SIZE] = \n{\n");
    var_ptr = snprintfE(var_ptr,var_end,
		"//extern const OptionIndex_t OptionIndex[UIOPT_INDEX_SIZE];\n"
		"//UIOPT_INDEX_SIZE := 0x%x = %u\n",
		UIOPT_INDEX_SIZE, UIOPT_INDEX_SIZE );

    for ( i = 0; i < UIOPT_INDEX_SIZE; )
    {
	int start = i;
	while ( i < UIOPT_INDEX_SIZE && !opt_buf[i] )
	    i++;
	int len = i - start;
	while ( len > 0 )
	{
	    const int now_len = len < 16 ? len : 16 - start % 16;
	    fprintf(cf,"\t/* 0x%0*x   */\t %.*s\n",
			UIOPT_INDEX_FW,
			start,
			2*now_len + now_len/4,
			"0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0," );
	    start += now_len;
	    len -= now_len;
	}

	while ( i < UIOPT_INDEX_SIZE && opt_buf[i] )
	{
	    fprintf(cf,"\t/* 0x%0*x %c */\tOPT_%s,\n",
		UIOPT_INDEX_FW, i, i > ' ' && i < 0x7f ? i : ' ', opt_buf[i]);
	    i++;
	}
    }
    fprintf(cf,"};\n");


    //----- option allowed

    if (ctrl->opt_allow_cmd)
    {
	print_section(cf,sep1,"opt_allowed_cmd_*");
	for ( info = ctrl->info; info < ctrl->end; info++ )
	{
	    if ( !( info->type & T_DEF_CMD ) )
		continue;

	    fprintf(cf,"static u8 option_allowed_cmd_%s[%u] = // cmd #%u\n{",
		info->c_name, ctrl->n_opt_specific, info->index );

	    int i;
	    u8 * ptr = ctrl->opt_allow_cmd + info->index * ctrl->n_opt_specific;
	    for ( i = 0; i < ctrl->n_opt_specific; i++ )
		fprintf(cf, "%s%u%s",
			    !(i%30) ? "\n    " : !(i%10) ? "  " : !(i%5) ? " " : "",
			    ptr[i],
			    i < ctrl->n_opt_specific-1 ? "," : "" );

	    fprintf(cf,"\n};\n\n");
	}
    }

    //----- InfoCommand

    print_links(ctrl);
    var_ptr = snprintfE(var_ptr,var_end,
		"//extern const InfoCommand_t CommandInfo[CMD__N+1];\n");

    //----- InfoUI

    char xname[100];
    {
	StringCopyS(xname,sizeof(xname),ctrl->info->c_name);
	char *ptr;
	for ( ptr = xname; *ptr; ptr++ )
	    if (!isalnum((int)*ptr))
		*ptr = '_';
    }

    print_section(cf,sep1,"InfoUI");

    var_ptr = snprintfE(var_ptr,var_end,
		"extern const InfoUI_t InfoUI_%s;\n",
		xname );

    fprintf(cf,
	    "const InfoUI_t InfoUI_%s =\n"
	    "{\n"
	    "\t\"%s\",\n"	// tool_name
	    "\t%s\n"		// n_cmd
	    "\t%s\n"		// cmd_tab
	    "\tCommandInfo,\n"	// cmd_info
	    "\t%s\n"		// n_opt_specific
	    "\tOPT__N_TOTAL,\n"	// n_opt_total
	    "\tOptionInfo,\n"	// opt_info
	    "\tOptionUsed,\n"	// opt_used
	    "\tOptionIndex,\n"	// opt_index
	    "\tOptionShort,\n"	// opt_short
	    "\tOptionLong\n"	// opt_long
	    "};\n",
	    xname,
	    ctrl->info->c_name,
	    ctrl->n_cmd ? "CMD__N," : "0, // n_cmd",
	    ctrl->n_cmd ? "CommandTab," : "0, // cmd_tab",
	    ctrl->n_cmd ? "OPT__N_SPECIFIC," : "0, // n_opt_specific" );


    //----- external vars

    print_section(hf,sep1,"external vars");
    fputs(var_buf,hf);


    //----- terminate

    print_section(cf,sep1,"END");
    print_section(hf,sep1,"END");
    fprintf(hf,"#endif // %s\n\n",guard);
    ResetStringField(&used_name);

    return ERR_OK;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  dummy functions		///////////////
///////////////////////////////////////////////////////////////////////////////

static void print_title ( FILE * f ) {}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  AddTables()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void AddTables ( FILE * df )
{
    DASSERT(df);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    main()			///////////////
///////////////////////////////////////////////////////////////////////////////

int ui_main ( int argc, char ** argv )
{
    static ccp def_null_name = "/dev/null";
    FILE * nf = fopen(def_null_name,"wb");
    if (!nf)
    {
	fprintf(stderr,"!!! Can't create file: %s\n",def_null_name);
	return ERR_CANT_CREATE;
    }

    static ccp def_fname = "src/ui/ui.def";
    FILE * df = fopen(def_fname,"wb");
    if (!df)
    {
	fprintf(stderr,"!!! Can't create file: %s\n",def_fname);
	return ERR_CANT_CREATE;
    }

    char fname[200];
    info_t * info = info_tab;

    while ( info->type != T_END )
    {
	if ( ! ( info->type & T_DEF_TOOL ) )
	{
	    fprintf(stderr,"!!! Missing T_DEF_TOOL entry.\n");
	    return ERR_SYNTAX;
	}

	control_t ctrl;
	memset(&ctrl,0,sizeof(ctrl));
	ctrl.df = nf;
	ctrl.opt_prefix = "";
	InitializeStringField(&ctrl.gopt);
	InitializeStringField(&ctrl.copt);
	InitializeStringField(&ctrl.opt_done);

	if ( !( info->type & F_HIDDEN ) )
	{
	    ctrl.df = df;
	    snprintf(iobuf,sizeof(iobuf),"Tool '%s'",info->c_name);
	    print_section(df,sep2,iobuf);
	    fprintf(df,"#:def_tool( \"%s\", \\\n",info->c_name);
	    DumpText(df,0,0,info->param,-1,1,", \\\n");
	    DumpText(df,0,0,info->help,-1,1," )\n\n");
	}

	snprintf(fname,sizeof(fname),"src/ui/ui-%s.c",info->c_name);
	ctrl.cf = fopen(fname,"wb");
	if (!ctrl.cf)
	{
	    fprintf(stderr,"!!! Can't create file: %s\n",fname);
	    return ERR_CANT_CREATE;
	}

	snprintf(fname,sizeof(fname),"src/ui/ui-%s.h",info->c_name);
	ctrl.hf = fopen(fname,"wb");
	if (!ctrl.hf)
	{
	    fprintf(stderr,"!!! Can't create file: %s\n",fname);
	    return ERR_CANT_CREATE;
	}

	ctrl.info = info++;
	while ( ! ( info->type & (T_END|T_DEF_TOOL)) )
	{
	    if ( info->type & T_DEF_OPT )
	    {
		if ( info->type & F_OPT_GLOBAL )
		{
		    InsertStringField(&ctrl.gopt,info->c_name,false);
		    ++ctrl.n_opt;
		    //opt_index = 0 for global options
		}
		else
		{
		    InsertStringField(&ctrl.copt,info->c_name,false);
		    info->index = ++ctrl.n_opt_specific;
		}

		if ( !info->help )
		{
		    // copy 'param' and 'help' info from previous tool
		    const info_t * search;
		    for ( search = info; search >= info_tab; search-- )
			if ( search->type & T_DEF_OPT
			    && search->help
			    && !strcmp(search->c_name,info->c_name) )
			{
			    info->help  = search->help;
			    if (!info->param)
				info->param = search->param;
			    break;
			}
		}

		if ( !( info->type & F_HIDDEN ) )
		{
		    fprintf(ctrl.df,"#:def_opt( \"%s\", \"%s\", \"%s%s%s%s%s\", \\\n",
			info->c_name, info->namelist,
			info->type & F_OPT_COMMAND  ? "C" : "",
			info->type & F_OPT_GLOBAL   ? "G" : "",
			info->type & F_OPT_MULTIUSE ? "M" : "",
			info->type & F_OPT_PARAM    ? "P" : "",
			info->type & F_OPT_OPTPARAM ? "O" : "" );
		    DumpText(ctrl.df,0,0,info->param,-1,1,", \\\n");
		    DumpText(ctrl.df,0,0,info->help,-1,1," )\n\n");
		}
	    }
	    else if ( info->type & T_DEF_CMD )
	    {
		info->index = ++ctrl.n_cmd;

		if ( !info->help )
		{
		    // copy 'param' and 'help' info from previous tool
		    const info_t * search = info-1;
		    for ( search = info; search >= info_tab; search-- )
			if ( search->type & T_DEF_CMD
			    && search->help
			    && !strcmp(search->c_name,info->c_name) )
			{
			    info->help  = search->help;
			    if (!info->param)
				info->param = search->param;
			    break;
			}
		}

		if ( !( info->type & F_HIDDEN ) )
		{
		    fprintf(ctrl.df,"#:def_cmd( \"%s\", \"%s\", \\\n",
			info->c_name, info->namelist );
		    DumpText(ctrl.df,0,0,info->param,-1,1,", \\\n");

		    ccp ff = info->help ? strchr(info->help,'\f') : 0;
		    if (ff)
		    {
			DumpText(ctrl.df,0,0,info->help,ff-info->help,1,", \\\n");
			DumpText(ctrl.df,0,0,ff+1,-1,1," )\n\n");
		    }
		    else
			DumpText(ctrl.df,0,0,info->help,-1,1," )\n\n");
		}
	    }
	    else if ( info->type & T_GRP_BEG )
		info->index = ctrl.n_grp++; // NULL based
	    else if ( info->type & (T_CMD_OPT|T_CMD_BEG|T_COPY_GRP|T_COPY_CMD) )
	    {
		const int type	= info->type & T_CMD_OPT  ? T_DEF_OPT
				: info->type & T_COPY_GRP ? T_GRP_BEG
				: T_DEF_CMD;

		const info_t * search;
		for ( search = ctrl.info; search < info; search++ )
		    if ( search->type & type
			&& !strcmp(search->c_name,info->c_name) )
		    {
			//PRINT("COPY INDEX #%x: %s\n",info->type,info->c_name);
			info->index = search->index;
			break;
		    }
	    }

	    info++;
	}
	ctrl.end = info;

	if (ctrl.n_cmd)
	    ctrl.n_cmd++; // one more for CMD_NONE;

	if (ctrl.n_opt_specific)
	    ctrl.n_opt += ++ctrl.n_opt_specific;

	TRACE("N: cmd=%u, grp=%u, opt=%d/%d\n",
		ctrl.n_cmd, ctrl.n_grp, ctrl.n_opt_specific, ctrl.n_opt );
	const enumError err = Generate(&ctrl);

	fclose(ctrl.cf);
	fclose(ctrl.hf);
	ResetStringField(&ctrl.gopt);
	ResetStringField(&ctrl.copt);
	ResetStringField(&ctrl.opt_done);

	if (err)
	    return err;
    }

    AddTables(df);
    print_section(df,sep2,"END");
    fclose(df);
    fclose(nf);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
