
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
 *        Copyright (c) 2012-2021 by Dirk Clemens <wiimm@wiimm.de>         *
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

#include <unistd.h>

#include "dclib-basics.h"
#include "dclib-debug.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			arg_manager_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetArgManager ( arg_manager_t *am )
{
    static char *null_list[] = {0};

    if (am)
    {
	if (am->size)
	{
	    uint i;
	    for ( i = 0; i < am->argc; i++ )
		FreeString(am->argv[i]);
	    if ( am->argv != null_list )
		FREE(am->argv);
	}
	memset(am,0,sizeof(*am));

	am->argv = null_list;
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetupArgManager ( arg_manager_t *am, int argc, char ** argv, bool clone )
{
    DASSERT(am);
    memset(am,0,sizeof(*am));
    if (clone)
	CloneArgManager(am,argc,argv);
    else
	AttachArgManager(am,argc,argv);
}

///////////////////////////////////////////////////////////////////////////////

void AttachArgManager ( arg_manager_t *am, int argc, char ** argv )
{
    DASSERT(am);
    ResetArgManager(am);
    am->argc = argc;
    am->argv = argv;
}

///////////////////////////////////////////////////////////////////////////////

void CloneArgManager ( arg_manager_t *am, int argc, char ** argv )
{
    DASSERT(am);
    AttachArgManager(am,argc,argv);
    PrepareEditArgManager(am,0);
}

///////////////////////////////////////////////////////////////////////////////

void CopyArgManager ( arg_manager_t *dest, const arg_manager_t *src )
{
    DASSERT(dest);
    if (!src)
	ResetArgManager(dest);
    else if( dest != src )
	AttachArgManager(dest,src->argc,src->argv);
}

///////////////////////////////////////////////////////////////////////////////

void MoveArgManager ( arg_manager_t *dest, arg_manager_t *src )
{
    DASSERT(dest);
    ResetArgManager(dest);
    if (src)
    {
	dest->argv = src->argv;
	dest->argc = src->argc;
	dest->size = src->size;
	memset(src,0,sizeof(*src));
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrepareEditArgManager ( arg_manager_t *am, int needed_space )
{
    DASSERT(am);

    uint new_size = am->argc;
    if ( needed_space > 0 )
	new_size += needed_space;
    new_size = GetGoodAllocSize2(new_size+(new_size>>4)+10,sizeof(*am->argv));
    PRINT("PrepareEditArgManager(,%d) n=%d, size: %d -> %d\n",
		needed_space, am->argc, am->size, new_size );

    if (am->size)
	am->argv = REALLOC(am->argv,sizeof(*am->argv)*new_size);
    else
    {
	char **old = am->argv;
	am->argv = MALLOC(sizeof(*am->argv)*new_size);

	uint i;
	for ( i = 0; i < am->argc; i++ )
	    am->argv[i] = old[i] ? STRDUP(old[i]) : 0;
    }
    am->size = new_size - 1;
    DASSERT( am->argc <= am->size );
    am->argv[am->argc] = 0;
}

///////////////////////////////////////////////////////////////////////////////

uint AppendArgManager ( arg_manager_t *am, ccp arg1, ccp arg2, bool move_arg )
{
    DASSERT(am);
    PRINT("AppendArgManager(,%s,%s,%d)\n",arg1,arg2,move_arg);
    return InsertArgManager(am,am->argc,arg1,arg2,move_arg);
}

///////////////////////////////////////////////////////////////////////////////

uint InsertArgManager
	( arg_manager_t *am, int pos, ccp arg1, ccp arg2, bool move_arg )
{
    PRINT("InsertArgManager(,%d,%s,%s,%d)\n",pos,arg1,arg2,move_arg);

    pos = CheckIndex1(am->argc,pos);
    const int n = (arg1!=0) + (arg2!=0);
    if (n)
    {
	PrepareEditArgManager(am,n);
	DASSERT( pos>0 && pos <= am->argc );
	char **dest = am->argv + pos;
	if ( pos < am->argc )
	    memmove( dest+n, dest, (u8*)(am->argv+am->argc) - (u8*)dest );
	if (arg1)
	    *dest++ = move_arg ? (char*)arg1 : STRDUP(arg1);
	if (arg2)
	    *dest++ = move_arg ? (char*)arg2 : STRDUP(arg2);
	am->argc += n;
	am->argv[am->argc] = 0;
    }
    return pos+n;
}

///////////////////////////////////////////////////////////////////////////////

uint ReplaceArgManager
	( arg_manager_t *am, int pos, ccp arg1, ccp arg2, bool move_arg )
{
    PRINT("ReplaceArgManager(,%d,%s,%s,%d)\n",pos,arg1,arg2,move_arg);

    pos = CheckIndex1(am->argc,pos);
    const int n = (arg1!=0) + (arg2!=0);
    if (n)
    {
	PrepareEditArgManager(am,n);
	DASSERT( pos>0 && pos <= am->argc );
	char **dest = am->argv + pos;
	if (arg1)
	{
	    if ( pos++ < am->argc )
		FreeString(*dest);
	    *dest++ = move_arg ? (char*)arg1 : STRDUP(arg1);
	}
	if (arg2)
	{
	    if ( pos++ < am->argc )
		FreeString(*dest);
	    *dest++ = move_arg ? (char*)arg2 : STRDUP(arg2);
	}
	if ( pos > am->argc )
	{
	    am->argc = pos;
	    am->argv[am->argc] = 0;
	}
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////

uint RemoveArgManager ( arg_manager_t *am, int pos, int count )
{
    DASSERT(am);

 #if HAVE_PRINT
    const int oldpos = pos;
 #endif
    const int n = CheckIndexC(am->argc,&pos,count);
    PRINT("RemoveArgManager(,%d,%d) : remove n=%d: %d..%d\n",
		oldpos, count, n, pos, pos+n );

    DASSERT( pos >= 0 && pos <= am->argc );
    DASSERT( n >= 0 && pos+n <= am->argc );

    if ( n > 0 )
    {
	PrepareEditArgManager(am,0);
	char **dest = am->argv + pos;

	uint i;
	for ( i = 0; i <n; i++ )
	    FreeString(dest[i]);
	am->argc -= n;
	memmove( dest, dest+n, (u8*)(am->argv+am->argc) - (u8*)dest );
	am->argv[am->argc] = 0;
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////

void DumpArgManager ( FILE *f, int indent, const arg_manager_t *am, ccp title )
{
    if ( !f || !am )
	return;

    indent = NormalizeIndent(indent);
    if (title)
	fprintf(f,"%*s" "ArgManager[%s]: N=%d",
		indent,"", title, am->argc );
    else
	fprintf(f,"%*s" "ArgManager: N=%d", indent,"", am->argc );

    if (am->size)
	fprintf(f,"/%u\n",am->size);
    else
	fputc('\n',f);

    if (am->argc)
    {
	char buf[10];
	int fw = snprintf(buf,sizeof(buf),"%u",am->argc-1)+2;

	uint i;
	for ( i = 0; i < am->argc; i++ )
	    fprintf(f,"%*s%*u: |%s|\n", indent,"", fw,i, am->argv[i] );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global commands			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Command_ARGTEST ( int argc, char ** argv )
{
    printf("ARGUMENT TEST: %u arguments:\n",argc);

    int idx;
    for ( idx = 0; idx < argc; idx++ )
	printf("%4u.: |%s|\n",idx,argv[idx]);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError Command_COLORS
(
    int		level,		// only used, if mode==NULL
				//  <  0: status message (ignore mode)
				//  >= 1: include names
				//  >= 2: include alt names
				//  >= 3: include color names incl bold
				//  >= 4: include background color names
    uint	mode,		// output mode => see PrintColorSetHelper()
    uint	format		// output format => see PrintColorSetEx()
)
{
    if (!colout)
	SetupStdMsg();

    if ( level < 0 )
    {
	ccp term = getenv("TERM");
	if (!term)
	    term = "?";
	printf("%s--color=%d [%s], colorize=%d [%s]\n"
		"term=%s\n"
		"stdout: tty=%d, mode=%d [%s], have-color=%d, n-colors=%u%s\n",
		colout->status,
		opt_colorize, GetColorModeName(opt_colorize,"?"),
		colorize_stdout, GetColorModeName(colorize_stdout,"?"),
		term, isatty(fileno(stdout)),
		colout->col_mode, GetColorModeName(colout->col_mode,"?"),
		colout->colorize, colout->n_colors, colout->reset );
	return ERR_OK;
    }

    if ( !mode && level > 0 )
	switch (level)
	{
	    case 1:  mode = 0x08; break;
	    case 2:  mode = 0x18; break;
	    case 3:  mode = 0x1b; break;
	    default: mode = 0x1f; break;
	}

    if (!format)
    {
	const ColorMode_t col_mode = colout ? colout->col_mode : COLMD_AUTO;
	PrintTextModes( stdout, 4, col_mode );
	PrintColorModes( stdout, 4, col_mode, GCM_ALT );
     #ifdef TEST
	PrintColorModes( stdout, 4, col_mode, GCM_ALT|GCM_SHORT );
     #endif
    }
    if ( mode || format )
	PrintColorSetEx(stdout,4,colout,mode,format);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

