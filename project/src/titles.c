
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
 *   Copyright (c) 2009-2017 by Dirk Clemens <wiimm@wiimm.de>              *
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#include "dclib/dclib-debug.h"
#include "lib-std.h"
#include "wbfs-interface.h"
#include "titles.h"
#include "dclib-utf8.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    variables                    ///////////////
///////////////////////////////////////////////////////////////////////////////

int title_mode = 1;

StringList_t * first_title_fname = 0;
StringList_t ** append_title_fname = &first_title_fname;

ID_DB_t title_db = {0,0,0};	// title database

static bool load_default_titles = true;
const int tdb_grow_size = 1000;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			titles interface		///////////////
///////////////////////////////////////////////////////////////////////////////

int AddTitleFile ( ccp arg, int unused )
{
    if ( arg && *arg )
    {
	if ( !arg[1] && arg[0] >= '0' && arg[0] <= '9' )
	{
	    TRACE("#T# set title mode: %d -> %d\n",title_mode,arg[0]-'0');
	    title_mode =  arg[0] - '0';
	}
	else if ( !arg[1] && arg[0] == '/' )
	{
	    TRACE("#T# disable default titles\n");
	    load_default_titles = false;
	    while (first_title_fname)
	    {
		StringList_t * sl = first_title_fname;
		first_title_fname = sl->next;
		FREE((char*)sl->str);
		FREE(sl);
	    }
	}
	else
	{
	    TRACE("#T# append title file: %s\n",arg);
	    StringList_t * sl = CALLOC(1,sizeof(StringList_t));
	    sl->str = STRDUP(arg);

	    *append_title_fname = sl;
	    append_title_fname = &sl->next;
	    ASSERT(!sl->next);
	}
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static int LoadTitleFile ( ccp fname, bool warn )
{
    ASSERT( fname && *fname );
    TRACE("#T# LoadTitleFile(%s)\n",fname);

    const int max_title_size = 200; // not a exact value
    char buf[PATH_MAX+max_title_size], title_buf[max_title_size+10];
    buf[0] = 0;

    FILE * f = 0;
    const bool use_stdin = fname[0] == '-' && !fname[1];
    if (use_stdin)
    {
	f = stdin;
	TRACE("#T#  - use stdin, f=%p\n",f);
    }
    else if (strchr(fname,'/'))
    {
     #ifdef __CYGWIN__
	NormalizeFilenameCygwin(buf,sizeof(buf),fname);
	f = fopen(buf,"r");
	TRACE("#T#  - f=%p: %s\n",f,buf);
	buf[0] = 0;
     #else
	f = fopen(fname,"r");
	TRACE("#T#  - f=%p: %s\n",f,fname);
     #endif
    }
    else
    {
	// no path found ==> use search_path[]
	ccp * sp;
	for ( sp = search_path; *sp && !f; sp++ )
	{
	    snprintf(buf,sizeof(buf),"%s%s",*sp,fname);
	    f = fopen(buf,"r");
	    TRACE("#T#  - f=%p: %s\n",f,buf);
	}
    }

    if (!f)
    {
	if ( warn || verbose > 3 )
	    ERROR0(ERR_WARNING,"Title file not found: %s\n",fname);
	return ERR_READ_FAILED;
    }

    if ( verbose > 3 )
	printf("SCAN TITLE FILE %s\n", *buf ? buf : fname );

    while (fgets(buf,sizeof(buf),f))
    {
	ccp ptr = buf;
	noTRACE("LINE: %s\n",ptr);

	// skip blanks
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	const int idtype = CountIDChars(ptr,false,false);
	char *id = (char*)ptr;
	ptr += idtype;

	// skip blanks and find '*'
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;
	const bool have_star = *ptr == '*';
	if (have_star)
	{
	    ptr++;
	    while ( *ptr > 0 && *ptr <= ' ' )
		ptr++;
	}

	if ( *ptr != '=' )
	    continue;
	ptr++;

	// title found, skip blanks
	id[idtype] = 0;
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	char *dest = title_buf;
	char *dend = dest + sizeof(title_buf) - 6; // enough for SPACE + UTF8 + NULL

	bool have_blank = false;
	while ( dest < dend && *ptr )
	{
	    ulong ch = ScanUTF8AnsiChar(&ptr);
	    if ( ch <= ' ' )
		have_blank = true;
	    else
	    {
		// real char found
		if (have_blank)
		{
		    have_blank = false;
		    *dest++ = ' ';
		}

		if ( ch >= 0x100 )
		{
		    const dcUnicodeTripel * trip = DecomposeUnicode(ch);
		    if (trip)
			ch = trip->code2;
		}

		if (use_utf8)
		    dest = PrintUTF8Char(dest,ch);
		else
		    *dest++ = ch < 0xff ? ch : '?';
	    }
	}
	*dest = 0;

	noPRINT("-> |%s| = |%s|\n",id,title_buf);
	if (!*title_buf)
	{
	    PRINT("RM %s\n",id);
	    RemoveID(&title_db,id,have_star);
	}
	else if ( idtype == 4 || idtype == 6 )
	    InsertID(&title_db,id,title_buf);
    }

    fclose(f);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

void InitializeTDB()
{
    static bool tdb_initialized = false;
    if (!tdb_initialized)
    {
	tdb_initialized = true;

	title_db.list	= 0;
	title_db.used	= 0;
	title_db.size	= 0;

	if (load_default_titles)
	{
	    LoadTitleFile("titles.txt",false);

	    if (lang_info)
	    {
		char lang[100];
		snprintf(lang,sizeof(lang),"titles-%s.txt",lang_info);
		LoadTitleFile(lang,false);
	    }

	    LoadTitleFile("titles.local.txt",false);
	}

	while (first_title_fname)
	{
	    StringList_t * sl = first_title_fname;
	    LoadTitleFile(sl->str,true);
	    first_title_fname = sl->next;
	    FREE((char*)sl->str);
	    FREE(sl);
	}

     #ifdef xxDEBUG
	TRACE("Title DB with %d titles:\n",title_db.used);
	DumpIDDB(&title_db,TRACE_FILE);
     #endif
    }
}

///////////////////////////////////////////////////////////////////////////////

ccp GetTitle ( ccp id6, ccp default_if_failed )
{
    if ( !title_mode || !id6 || !*id6 )
	return default_if_failed;

    InitializeTDB();
    TDBfind_t stat;
    int idx = FindID(&title_db,id6,&stat,0);
    TRACE("#T# GetTitle(%s) tm=%d  idx=%d/%d/%d  stat=%d -> %s %s\n",
		id6, title_mode, idx, title_db.used, title_db.size, stat,
		idx < title_db.used ? title_db.list[idx]->id : "",
		idx < title_db.used ? title_db.list[idx]->title : "" );
    ASSERT( stat == IDB_NOT_FOUND || idx < title_db.used );
    return stat == IDB_NOT_FOUND
		? default_if_failed
		: title_db.list[idx]->title;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan id helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static ccp ScanArgID
(
    char		buf[7],		// result buffer for ID6: 6 chars + NULL
					// On error 'buf7' is filled with NULL
    ccp			arg,		// argument to scan. Comma is a separator
    bool		trim_end	// true: remove trailing '.'
)
{
    if (!arg)
    {
	memset(buf,0,6);
	return 0;
    }
	
    while ( *arg > 0 && *arg <= ' ' )
	arg++;

    ccp start = arg;
    int err = 0, wildcards = 0;
    while ( *arg > ' ' && *arg != ',' && *arg != '=' )
    {
	int ch = *arg++;
	if ( ch == '+' || ch == '*' )
	    wildcards++;
	else if (!isalnum(ch) && !strchr("_.",ch))
	    err++;
    }
    const int arglen = arg - start;
    if ( err || wildcards > 1 || !arglen || arglen > 6 )
    {
	memset(buf,0,6);
	return start;
    }
    
    char * dest = buf;
    for ( ; start < arg; start++ )
    {
	if ( *start == '+' || *start == '*' )
	{
	    int count = 7 - arglen;
	    while ( count-- > 0 )
		*dest++ = '.';
	}
	else
	    *dest++ = toupper((int)*start);
	DASSERT( dest <= buf + 6 );
    }

    if (trim_end)
	while ( dest[-1] == '.' )
	    dest--;
    else
	while ( dest < buf+6 )
	    *dest++ = '.';
    *dest = 0;

    while ( *arg > 0 && *arg <= ' ' || *arg == ',' )
	arg++;
    return arg;
}

///////////////////////////////////////////////////////////////////////////////

static ccp ScanPatID // return NULL if ok or a pointer to the invalid text
(
    StringField_t	* sf_id6,	// valid pointer: add real ID6
    StringField_t	* sf_pat,	// valid pointer: add IDs with pattern '.'
    ccp			arg,		// argument to scan. Comma is a separator
    bool		trim_end,	// true: remove trailing '.'
    bool		allow_arg	// true: allow and store '=arg'
)
{
    DASSERT(sf_id6);
    DASSERT(sf_pat);

    char buf[7];
    while ( arg && *arg )
    {
 	arg = ScanArgID(buf,arg,trim_end);
 	TRACE(" -> |%s|\n",arg);
	if (!*buf)
	    return arg;

	ccp eq_arg = 0;
	if ( arg && *arg == '=' )
	{
	    if (!allow_arg)
		return arg;
	    eq_arg = ++arg;
	    arg = 0;
	}

	if ( sf_id6 != sf_pat && strchr(buf,'.') )
	    InsertStringID6(sf_pat,buf,SEL_UNUSED,eq_arg);
	else
	    InsertStringID6(sf_id6,buf,SEL_UNUSED,eq_arg);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static enumError AddId
(
    StringField_t	* sf_id6,
    StringField_t	* sf_pat,
    ccp			arg,
    int			select_mode
)
{
    DASSERT(sf_id6);
    DASSERT(sf_pat);


    if ( select_mode & SEL_F_FILE )
    {
	char id[7];
	ccp end = ScanArgID(id,arg,false);
	TRACE("->|%s|\n",end);
	if ( *id && ( !end || !*end ) )
	{
	    if (strchr(id,'.'))
	    {
		TRACE("ADD PAT/FILE: %s\n",id);
		InsertStringID6(sf_pat,id,SEL_UNUSED,0);
	    }
	    else
	    {
		TRACE("ADD ID6/FILE: %s\n",id);
		InsertStringID6(sf_id6,id,SEL_UNUSED,0);
	    }
	}
	else
	{
	    int idlen;
	    ScanID(id,&idlen,arg);
	    if ( idlen == 4 || idlen == 6 )
	    {
		TRACE("ADD ID/FILE: %s\n",id);
		InsertStringID6(sf_id6,id,SEL_UNUSED,0);
	    }
	}
    }
    else
    {
	TRACE("ADD PAT/PARAM: %s\n",arg);
	ccp res = ScanPatID(sf_id6,sf_pat,arg,false,
			(select_mode & SEL_F_PARAM) != 0 );
	if (res)
	    return ERROR0(ERR_SYNTAX,"Not a ID: %s\n",res);
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static IdItem_t * FindPatID
(
    StringField_t	* sf_id6,	// valid pointer: search real ID6
    StringField_t	* sf_pat,	// valid pointer: search IDs with pattern '.'
    ccp			id6,		// valid id6
    bool		mark_matching	// true: mark *all* matching records
)
{
    if (!id6)
	return 0;

    IdItem_t * found = 0;
    if (sf_id6)
    {
	found = (IdItem_t*)FindStringField(sf_id6,id6);
	if (found)
	{
	    if ( found->flag == 2 )
		return found;

	    found->flag = SEL_USED;
	    if (!mark_matching)
		return found;
	}
    }

    if (sf_pat)
    {
	IdItem_t **ptr = (IdItem_t**)sf_pat->field, **end;
	for ( end = ptr + sf_pat->used; ptr < end; ptr++ )
	{
	    ccp p1 = ptr[0]->id6;
	    ccp p2 = id6;
	    while ( *p1 && *p2 && ( *p1 == '.' || *p2 == '.' || *p1 == *p2 ))
		p1++, p2++;
	    if ( !*p1 && !*p2 )
	    {
		(*ptr)->flag = SEL_USED;
		if (!mark_matching)
		    return *ptr;
		if (!found)
		    found = *ptr;
	    }
	}
    }

    return found;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			select id interface		///////////////
///////////////////////////////////////////////////////////////////////////////

static bool include_db_enabled = false;

static StringField_t include_id6 = {0,0,0};	// include id6 (without wildcard '.')
static StringField_t include_pat = {0,0,0};	// include pattern (with wildcard '.')
static StringField_t exclude_id6 = {0,0,0};	// exclude id6 (without wildcard '.')
static StringField_t exclude_pat = {0,0,0};	// exclude pattern (with wildcard '.')

static StringField_t include_fname = {0,0,0};	// include filenames
static StringField_t exclude_fname = {0,0,0};	// exclude filenames

int disable_exclude_db = 0;			// disable exclude db at all if > 0
bool include_first = false;			// use include rules before exclude

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int AddIncludeID ( ccp arg, int select_mode )
{
    include_db_enabled = true;
    return AddId(&include_id6,&include_pat,arg,select_mode);
}

///////////////////////////////////////////////////////////////////////////////

int AddIncludePath ( ccp arg, int unused )
{
    char buf[PATH_MAX];
    if (realpath(arg,buf))
	arg = buf;

    InsertStringField(&include_fname,arg,false);

    include_db_enabled = true;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int AddExcludeID ( ccp arg, int select_mode )
{
    return AddId(&exclude_id6,&exclude_pat,arg,select_mode);
}

///////////////////////////////////////////////////////////////////////////////

int AddExcludePath ( ccp arg, int unused )
{
    noPRINT("AddExcludePath(%s,%d)\n",arg,unused);
    char buf[PATH_MAX];
    if (realpath(arg,buf))
	arg = buf;

    InsertStringField(&exclude_fname,arg,false);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static void CheckExcludePath
	( ccp path, StringField_t * sf, StringField_t * sf_id6, int max_dir_depth )
{
    TRACE("CheckExcludePath(%s,%p,%d)\n",path,sf,max_dir_depth);
    DASSERT(sf);
    DASSERT(sf_id6);

    WFile_t f;
    InitializeWFile(&f);
    if (OpenWFile(&f,path,IOM_NO_STREAM))
	return;

    AnalyzeFT(&f);
    ClearWFile(&f,false);

    if ( *f.id6_src )
    {
	TRACE(" - exclude id %s\n",f.id6_src);
	InsertStringID6(sf_id6,f.id6_src,SEL_UNUSED,0);
    }
    else if ( max_dir_depth > 0 && f.ftype == FT_ID_DIR )
    {
	char real_path[PATH_MAX];
	if (realpath(path,real_path))
	    path = real_path;
	if (InsertStringField(sf,path,false))
	{
	    TRACE(" - exclude dir %s\n",path);
	    DIR * dir = opendir(path);
	    if (dir)
	    {
		char buf[PATH_MAX], *bufend = buf+sizeof(buf);
		char * dest = StringCopyE(buf,bufend-1,path);
		if ( dest > buf && dest[-1] != '/' )
		    *dest++ = '/';

		max_dir_depth--;

		for(;;)
		{
		    struct dirent * dent = readdir(dir);
		    if (!dent)
			break;
		    ccp n = dent->d_name;
		    if ( n[0] != '.' )
		    {
			StringCopyE(dest,bufend,dent->d_name);
			CheckExcludePath(buf,sf,sf_id6,max_dir_depth);
		    }
		}
		closedir(dir);
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetupExcludeDB()
{
    TRACE("SetupExcludeDB()");

    if (include_fname.used)
    {
	TRACELINE;
	StringField_t sf;
	InitializeStringField(&sf);
	ccp * ptr = include_fname.field + include_fname.used;
	while ( ptr-- > include_fname.field )
	    CheckExcludePath(*ptr,&sf,&include_id6,opt_recurse_depth);
	ResetStringField(&sf);
	ResetStringField(&include_fname);
    }

    if (exclude_fname.used)
    {
	TRACELINE;
	StringField_t sf;
	InitializeStringField(&sf);
	ccp * ptr = exclude_fname.field + exclude_fname.used;
	while ( ptr-- > exclude_fname.field )
	    CheckExcludePath(*ptr,&sf,&exclude_id6,opt_recurse_depth);
	ResetStringField(&sf);
	ResetStringField(&exclude_fname);
    }
}

///////////////////////////////////////////////////////////////////////////////

void DefineExcludePath ( ccp path, int max_dir_depth )
{
    TRACE("DefineExcludePath(%s,%d)\n",path,max_dir_depth);

    if (exclude_fname.used)
	SetupExcludeDB();

    StringField_t sf;
    InitializeStringField(&sf);
    CheckExcludePath(path,&sf,&exclude_id6,max_dir_depth);
    ResetStringField(&sf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool IsExcludeActive()
{
    if ( exclude_fname.used || include_fname.used )
	SetupExcludeDB();

    return include_id6.used
	|| include_pat.used
	|| exclude_id6.used
	|| exclude_pat.used;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool IsExcluded ( ccp id6 )
{
    noTRACE("IsExcluded(%s) dis=%d ena=%d, n=%d+%d\n",
		id6, disable_exclude_db, include_db_enabled,
		exclude_fname.used, include_fname.used );

    if ( disable_exclude_db > 0 )
	return false;

    if ( exclude_fname.used || include_fname.used )
	SetupExcludeDB();

    if ( include_first
		&& include_db_enabled
		&& FindPatID(&include_id6,&include_pat,id6,false) )
	return false;

    if (FindPatID(&exclude_id6,&exclude_pat,id6,false))
	return true;

    return !include_first
	&& include_db_enabled
	&& !FindPatID(&include_id6,&include_pat,id6,false);
}

///////////////////////////////////////////////////////////////////////////////

void DumpExcludeDB()
{
    SetupExcludeDB();
    noPRINT("DumpExcludeDB() n=%d+%d\n",exclude_pat.used,exclude_pat.used);

    ccp *ptr = exclude_id6.field, *end;
    for ( end = ptr + exclude_id6.used; ptr < end; ptr++ )
	printf("%s\n",*ptr);

    ptr = exclude_pat.field;
    for ( end = ptr + exclude_pat.used; ptr < end; ptr++ )
	printf("%s\n",*ptr);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			param id6 interface		///////////////
///////////////////////////////////////////////////////////////////////////////

StringField_t	param_id6 = {0,0,0};	// param id6 (without wildcard '.')
StringField_t	param_pat = {0,0,0};	// param pattern (with wildcard '.')

///////////////////////////////////////////////////////////////////////////////

void ClearParamDB()
{
    ResetStringField(&param_id6);
    ResetStringField(&param_pat);
}

///////////////////////////////////////////////////////////////////////////////

int AddParamID ( ccp arg, int select_mode )
{
    return AddId(&param_id6,&param_pat,arg,select_mode);
}

///////////////////////////////////////////////////////////////////////////////

int CountParamID()
{
    return param_id6.used + param_pat.used;
}

///////////////////////////////////////////////////////////////////////////////

IdItem_t * FindParamID ( ccp id6 )
{
    return FindPatID(&param_id6,&param_pat,id6,true);
}

///////////////////////////////////////////////////////////////////////////////

int DumpParamDB ( enumSelectUsed mask, bool warn )
{
    int count_id6 = 0, count_pat = 0;

    IdItem_t **ptr = (IdItem_t**)param_id6.field, **end;
    for ( end = ptr + param_id6.used; ptr < end; ptr++ )
    {
	IdItem_t * item = *ptr;
	DASSERT(item);
	if ( item->flag & mask )
	{
	    count_id6++;
	    if (warn)
		ERROR0(ERR_WARNING,"Disc with ID6 [%s] not found.\n",item->id6);
	    else
		printf("%s%s%s\n", item->id6, *item->arg ? "=" : "", item->arg );
	}
    }

    ptr = (IdItem_t**)param_pat.field;
    for ( end = ptr + param_pat.used; ptr < end; ptr++ )
    {
	IdItem_t * item = *ptr;
	DASSERT(item);
	if ( item->flag & mask )
	{
	    count_pat++;
	    if (warn)
		ERROR0(ERR_WARNING,"Disc with pattern [%s] not found.\n",item->id6);
	    else
		printf("%s%s%s\n", item->id6, *item->arg ? "=" : "", item->arg );
	}
    }

    if ( warn && count_id6 + count_pat > 1 )
    {
	if ( count_id6 && count_pat )
	    ERROR0(ERR_WARNING,"==> %u disc ID%s and %u pattern not found!\n",
			count_id6, count_id6 == 1 ? "" : "s", count_pat );
	else if ( count_id6 )
	    ERROR0(ERR_WARNING,"==> %u disc ID%s not found!\n",
			count_id6, count_id6 == 1 ? "" : "s" );
	else if ( count_pat )
	    ERROR0(ERR_WARNING,"==> %u disc pattern not found!\n",
			count_pat );
	if ( count_id6 || count_pat )
	    putchar('\n');
    }

    return count_id6 + count_pat;
}

///////////////////////////////////////////////////////////////////////////////

int SetupParamDB
(
    // return the number of valid parameters or NULL on error

    ccp		default_param,		// not NULL: use this if no param is defined
    bool	warn,			// print a warning if no param is defined
    bool	allow_arg		// allow arguments '=arg'
)
{
    if ( default_param && !n_param )
	AddParam(default_param,false);

    enumSelectID id1 = SEL_ID;
    enumSelectID id2 = SEL_FILE;
    if ( allow_arg )
    {
	id1 |= SEL_F_PARAM;
	id2 |= SEL_F_PARAM;
    }

    ClearParamDB();
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AtFileHelper(param->arg,id1,id2,AddParamID);

    const int count = CountParamID();
    if ( !count && warn )
	ERROR0(ERR_MISSING_PARAM,"Missing parameters (list of IDs)!\n");

    return count;
}

///////////////////////////////////////////////////////////////////////////////

IdItem_t * CheckParamSlot
(
    // return NULL if no disc at slot found or disc not match or disabled
    // or a pointer to the ID6

    struct WBFS_t	* wbfs,		// valid and opened WBFS
    int			slot,		// valid slot index
    bool		open_disc,	// true: open the disc
    ccp			* ret_id6,	// not NULL: store pointer to ID6 of disc
					//   The ID6 is valid until the WBFS is closed
    ccp			* ret_title	// not NULL: store pointer to title of disc
					//   The title is searched in the title db first
					//   The title may be stored in a static buffer
)
{
    DASSERT(wbfs);
    DASSERT(wbfs->wbfs);
    DASSERT( slot >= 0 && slot < wbfs->wbfs->max_disc );

    CloseWDisc(wbfs);

    if (ret_title)
	*ret_title = 0;
    if (ret_id6)
	*ret_id6 = 0;

    ccp id6 = wbfs_load_id_list(wbfs->wbfs,false)[slot];
    if (!*id6)
	return 0;

    IdItem_t * item = FindParamID(id6);
    if ( !item || IsExcluded(id6) )
	return 0;

    if (open_disc)
	OpenWDiscID6(wbfs,id6);

    if (ret_title)
    {
	static char disc_title[WII_TITLE_SIZE+1];
	ccp title = GetTitle(id6,0);
	if (!title)
	{
	    if ( wbfs->disc || !OpenWDiscID6(wbfs,id6) )
	    {
		wd_header_t *dh = GetWDiscHeader(wbfs);
		if (dh)
		{
		    StringCopyS(disc_title,sizeof(disc_title),(ccp)dh->disc_title);
		    title = disc_title;
		}
		if (!open_disc)
		    CloseWDisc(wbfs);
	    }
	}
	*ret_title = title ? title : "?";
    }

    TRACE("CheckParamSlot(,%u,%d,%u) => disc=%p, %s, %s\n",
		slot, open_disc, ret_title!=0,
		wbfs->disc, id6, ret_title ? *ret_title : "-" );

    if (ret_id6)
	*ret_id6 = id6;
    return item;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		 low level title id interface		///////////////
///////////////////////////////////////////////////////////////////////////////

// disable extended tracing

#undef xTRACE

#if 0
    #define xTRACE TRACE
#else
    #define xTRACE noTRACE
#endif

///////////////////////////////////////////////////////////////////////////////

int FindID ( ID_DB_t *db, ccp id, TDBfind_t *p_stat, int * p_num )
{
    ASSERT(db);
    if ( !db || !db->used )
    {
	if (p_stat)
	    *p_stat = IDB_NOT_FOUND;
	if  (p_num)
	    *p_num = 0;
	return 0;
    }

    ID_t * elem = 0;
    size_t id_len  = strlen(id);
    if ( id_len > sizeof(elem->id)-1 )
	id_len = sizeof(elem->id)-1;

    int beg = 0, end = db->used-1;
    while ( beg <= end )
    {
	int idx = (beg+end)/2;
	elem = db->list[idx];
	const int cmp_stat = memcmp(id,elem->id,id_len);
	noPRINT(" - check: %d..%d..%d: %d = %s\n",beg,idx,end,cmp_stat,elem->id);
	if ( cmp_stat < 0 )
	    end = idx - 1;
	else if ( cmp_stat > 0 )
	    beg = idx + 1;
	else
	{
	    beg = idx;
	    while ( beg > 0 && !memcmp(id,db->list[beg-1]->id,id_len) )
		beg--;
	    break;
	}
    }
    ASSERT( beg >= 0 && beg <= db->used );
    elem = db->list[beg];

    TDBfind_t stat = IDB_NOT_FOUND;
    if ( beg < db->used )
    {
	if (!memcmp(id,elem->id,id_len))
	    stat = elem->id[id_len] ? IDB_EXTENSION_FOUND : IDB_ID_FOUND;
	else if (!memcmp(id,elem->id,strlen(elem->id)))
	    stat = IDB_ABBREV_FOUND;
	else if ( beg > 0 )
	{
	    elem = db->list[beg-1];
	    xTRACE("cmp-1[%s,%s] -> %d\n",id,elem->id,memcmp(id,elem->id,strlen(elem->id)));
	    if (!memcmp(id,elem->id,strlen(elem->id)))
	    {
		stat = IDB_ABBREV_FOUND;
		beg--;
	    }
	}
	xTRACE("cmp[%s,%s] %d %d -> %d\n", id, elem->id,
		memcmp(id,elem->id,id_len), memcmp(id,elem->id,strlen(elem->id)), stat );
    }
    xTRACE("#T# FindID(%p,%s,%p,%p) idx=%d/%d/%d, stat=%d, found=%s\n",
		db, id, p_stat, p_num, beg, db->used, db->size,
		stat, beg < db->used && elem ? elem->id : "-" );

    if (p_stat)
	*p_stat = stat;

    if (p_num)
    {
	int idx = beg;
	while ( idx < db->used && !memcmp(id,db->list[idx],id_len) )
	    idx++;
	xTRACE(" - num = %d\n",idx-beg);
	*p_num = idx - beg;    }

    return beg;
}

///////////////////////////////////////////////////////////////////////////////

int InsertID ( ID_DB_t * db, ccp id, ccp title )
{
    ASSERT(db);
    xTRACE("-----\n");

    // remove all previous definitions first
    int idx = RemoveID(db,id,true);

    if ( db->used == db->size )
    {
	db->size += tdb_grow_size;
	db->list = (ID_t**)REALLOC(db->list,db->size*sizeof(*db->list));
    }
    ASSERT( db->list );
    ASSERT( idx >= 0 && idx <= db->used );

    ID_t ** elem = db->list + idx;
    xTRACE(" - insert: %s|%s|%s\n",
		idx>0 ? elem[-1]->id : "-", id, idx<db->used ? elem[0]->id : "-");
    memmove( elem+1, elem, (db->used-idx)*sizeof(*elem) );
    db->used++;
    ASSERT( db->used <= db->size );

    int tlen = title ? strlen(title) : 0;
    ID_t * t = *elem = (ID_t*)MALLOC(sizeof(ID_t)+tlen);

    StringCopyS(t->id,sizeof(t->id),id);
    StringCopyS(t->title,tlen+1,title);

    xTRACE("#T# InsertID(%p,%s,) [%d], id=%s title=%s\n",
		db, id, idx, t->id, t->title );

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int RemoveID ( ID_DB_t * db, ccp id, bool remove_extended )
{
    ASSERT(db);

    TDBfind_t stat;
    int count;
    int idx = FindID(db,id,&stat,&count);

    switch(stat)
    {
	case IDB_NOT_FOUND:
	    noPRINT_IF(!memcmp(id,"RMCE9",5),
			"#T# RemoveID(%p,%s,%d) NOT-FOUND idx=%d, count=%d\n",
			db, id, remove_extended, idx, count );
	    break;

	case IDB_ABBREV_FOUND:
	    noPRINT_IF(!memcmp(id,"RMCE9",5),
			"#T# RemoveID(%p,%s,%d) ABBREV-FOUND idx=%d, count=%d\n",
			db, id, remove_extended, idx, count );
	    idx++;
	    break;

	case IDB_ID_FOUND:
	case IDB_EXTENSION_FOUND:
	    noPRINT("#T# RemoveID(%p,%s,%d) FOUND idx=%d, count=%d\n",
			db, id, remove_extended, idx, count );
	    ASSERT(count>0);
	    ASSERT(db->used>=count);

	    ID_t ** elem = db->list + idx;
	    if (!remove_extended)
	    {
		if (strcmp(elem[0]->id,id))
		    break;
		count = 1;
	    }

	    int c;
	    for ( c = 0; c < count; c++ )
	    {
		noPRINT(" - remove %s = %s\n",elem[c]->id,elem[c]->title);
		FREE(elem[c]);
	    }

	    db->used -= count;
	    elem = db->list + idx;
	    memmove( elem, elem+count, (db->used-idx)*sizeof(*elem) );
	    break;

	default:
	    ASSERT(0);
    }

    return idx;
}

///////////////////////////////////////////////////////////////////////////////

void DumpIDDB ( ID_DB_t * db, FILE * f )
{
    if ( !db || !db->list || !f )
	return;

    ID_t ** list = db->list, **end;
    for ( end = list + db->used; list < end; list++ )
    {
	ID_t * elem = *list;
	if (*elem->title)
	    fprintf(f,"%-6s = %s\n",elem->id,elem->title);
	else
	    fprintf(f,"%-6s\n",elem->id);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			system menu interface		///////////////
///////////////////////////////////////////////////////////////////////////////

static bool system_menu_loaded = false;
static StringField_t system_menu = {0};

///////////////////////////////////////////////////////////////////////////////

static void LoadSystemMenuTab()
{
    if (!system_menu_loaded)
    {
	system_menu_loaded = true;
	ResetStringField(&system_menu);

	char buf[PATH_MAX+100];
	FILE *f = 0;

	ccp * sp;
	for ( sp = search_path; *sp && !f; sp++ )
	{
	    snprintf(buf,sizeof(buf),"%s%s",*sp,"system-menu.txt");
	    f = fopen(buf,"r");
	}

	if (f)
	{
	    TRACE("SYS-MENU: f=%p: %s\n",f,buf);

	    while (fgets(buf,sizeof(buf),f))
	    {
		char * ptr;
		u32 vers = strtoul(buf,&ptr,10);
		if ( !vers || ptr == buf )
		    continue;

		while ( *ptr > 0 && *ptr <= ' ' )
		    ptr++;
		if ( *ptr != '=' )
		    continue;
		ptr++;
		while ( *ptr > 0 && *ptr <= ' ' )
		    ptr++;
		ccp text = ptr;
		while (*ptr)
		    ptr++;
		ptr--;
		while ( *ptr > 0 && *ptr <= ' ' )
		    ptr--;
		*++ptr = 0;
		
		const int len = snprintf(buf,sizeof(buf),"%u%c%s",vers,0,text) + 1;
		noPRINT("SYS-MENU: %6u = '%s' [%u]\n",vers,buf+strlen(buf)+1,len);

		char * dest = MALLOC(len);
		memcpy(dest,buf,len);
		InsertStringField(&system_menu,dest,true);
	    }

	    fclose(f);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

ccp GetSystemMenu
(
    u32		version,		// system version number
    ccp		return_if_not_found	// return value if not found
)
{
    LoadSystemMenuTab();

    char key[20];
    snprintf(key,sizeof(key),"%u",version);
    ccp res = FindStringField(&system_menu,key);
    return res ? res + strlen(res) + 1 : return_if_not_found;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

