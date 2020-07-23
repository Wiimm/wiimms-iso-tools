
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
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>

#include "dclib/dclib-debug.h"
#include "wbfs-interface.h"
#include "titles.h"

///////////////////////////////////////////////////////////////////////////////

#ifndef ENABLE_PARTITIONS_WORKAROUND
    #if defined(__CYGWIN__)
	// work around for a bug in /proc/partitions since cygwin 1.7.6
	#define ENABLE_PARTITIONS_WORKAROUND 1
    #else
	#define ENABLE_PARTITIONS_WORKAROUND 0
    #endif
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     partitions                  ///////////////
///////////////////////////////////////////////////////////////////////////////

PartitionInfo_t *  first_partition_info = 0;
PartitionInfo_t ** append_partition_info = &first_partition_info;

int pi_count = 0;
PartitionInfo_t * pi_list[MAX_WBFS+1];
WDiscList_t pi_wlist = {0,0,0,0};
u32 pi_free_mib = 0;

int opt_part	= 0;
int opt_auto	= 0;
int opt_all	= 0;

///////////////////////////////////////////////////////////////////////////////

PartitionInfo_t * CreatePartitionInfo ( ccp path, enumPartSource source )
{
    char real_path_buf[PATH_MAX];
    ccp real_path = real_path_buf;
    if (!realpath(path,real_path_buf))
    {
	TRACE("CAN'T DETERMINE REAL PATH: %s\n",path);
	real_path = path;
    }

    PartitionInfo_t * info = first_partition_info;
    while ( info && strcmp(info->real_path,real_path) )
	info = info->next;

    if (!info)
    {
	// new entry

	PartitionInfo_t * info = MALLOC(sizeof(PartitionInfo_t));
	memset(info,0,sizeof(PartitionInfo_t));
	info->path = STRDUP(path);
	info->real_path = STRDUP(real_path);
	info->part_mode = PM_UNKNOWN;
	info->source = source;
	*append_partition_info = info;
	append_partition_info = &info->next;
	TRACE("PARTITION inserted: %s\n",real_path);
    }
    else if ( source == PS_PARAM )
    {
	// overrides previous definition

	info->source = source;
	FREE((char*)info->path);
	info->path = STRDUP(path);
	TRACE("PARTITION redefined: %s\n",real_path);
    }
    return info;
}

///////////////////////////////////////////////////////////////////////////////

int AddPartition ( ccp arg, int unused )
{
    if (opt_part++)
	opt_all++;
    CreatePartitionInfo(arg,PS_PARAM);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int ScanDevForPartitions ( ccp dev_prefix )
{
    TRACE("ScanDevForPartitions(%s)\n",dev_prefix);
    size_t len_prefix = strlen(dev_prefix);

    static char prefix[] = "/dev/";
    const int bufsize = 100;
    char buf[bufsize+1+sizeof(prefix)];
    strcpy(buf,prefix);

    int count = 0;

    DIR * dir = opendir("/dev");
    if (dir)
    {
	for (;;)
	{
	    struct dirent * dent = readdir(dir);
	    if (!dent)
		break;
	 #ifdef _DIRENT_HAVE_D_TYPE
	    if ( dent->d_type == DT_BLK || dent->d_type == DT_CHR )
	    {
	 #endif
		if (!memcmp(dent->d_name,dev_prefix,len_prefix))
		{
		    StringCopyE(buf+sizeof(prefix)-1,buf+sizeof(buf),dent->d_name);
		    TRACE(" - part found: %s\n",buf);
		    CreatePartitionInfo(buf,PS_AUTO);
		    count++;
		}
	 #ifdef _DIRENT_HAVE_D_TYPE
	    }
	 #endif
	}
	closedir(dir);
    }
    
    return count;
}

///////////////////////////////////////////////////////////////////////////////
#if ENABLE_PARTITIONS_WORKAROUND

 static void iterate_dev ( ccp base )
 {
    TRACE("ITERATE %s\n",base);

    char dev[100];
    int i;
    for ( i = 1; i <= 15; i++ )
    {
	snprintf(dev,sizeof(dev),"%s%u",base,i);
	CreatePartitionInfo(dev,PS_AUTO_IGNORE);
    }
 }

#endif // ENABLE_PARTITIONS_WORKAROUND
///////////////////////////////////////////////////////////////////////////////

int ScanPartitions ( bool all )
{
    opt_auto++;
    opt_all += all;

    int count = 0;

    static char prefix[] = "/dev/";
    enum { bufsize = 100 };
    char buf[bufsize+1];
    char buf2[bufsize+1+sizeof(prefix)];
    strcpy(buf2,prefix);

 #if ENABLE_PARTITIONS_WORKAROUND
    PRINT("PARTITIONS WORKAROUND ENABLED\n");
    char prev_disc[sizeof(buf2)] = {0};
    int  prev_disc_len = 0;
 #endif

    FILE * f = fopen("/proc/partitions","r");
    if (f)
    {
	TRACE("SCAN /proc/partitions\n");

	// skip first line
	fgets(buf,bufsize,f);

	while (fgets(buf,bufsize,f))
	{
	    char * ptr = buf;
	    while (*ptr)
		ptr++;
	    if ( ptr > buf )
	    {
		ptr--;
		while ( ptr > buf && (u8)*ptr <= ' ' )
		    ptr--;
		ptr[1] = 0;
		while ( ptr > buf && isalnum((int)*ptr) )
		    ptr--;
		if (*++ptr)
		{
		    strcpy(buf2+sizeof(prefix)-1,ptr);

		 #if ENABLE_PARTITIONS_WORKAROUND

		    if ( prev_disc_len && memcmp(prev_disc,buf2,prev_disc_len) )
			iterate_dev(prev_disc);

		    prev_disc_len = strlen(buf2);
		    const int ch = buf2[prev_disc_len-1];
		    if ( ch >= '0' && ch <= '9' )
			prev_disc_len = 0;
		    else
			memcpy(prev_disc,buf2,sizeof(prev_disc));
		    TRACE("STORE: %d %s\n",prev_disc_len,prev_disc);
		 #endif

		    CreatePartitionInfo(buf2,PS_AUTO);
		}
	    }
	}
	fclose(f);

     #if ENABLE_PARTITIONS_WORKAROUND
	if (prev_disc_len)
	    iterate_dev(prev_disc);
     #endif
    }
    else
    {
	ScanDevForPartitions("sd");
	if (!ScanDevForPartitions("rdisk"))
	     ScanDevForPartitions("disk");
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AddEnvPartitions()
{
    TRACE("AddEnvPartitions() PART1=%d, AUTO=%d, all=%d, first=%p\n",
	opt_part, opt_auto, opt_all, first_partition_info );

    if ( !first_partition_info && !opt_part && !opt_auto )
    {
	TRACE("lookup environment var 'WWT_WBFS'\n");
	char * env = getenv("WWT_WBFS");
	if ( env && *env )
	{
	    char * ptr = env;
	    for(;;)
	    {
		env = ptr;
		while ( *ptr && *ptr != ';' )
		    ptr++;
		if ( ptr > env )
		{
		    char ch = *ptr;
		    *ptr = 0;
		    CreatePartitionInfo(env,PS_ENV);
		    opt_all++;
		    *ptr = ch;
		}
		if (!*ptr)
		    break;
		ptr++;
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int wbfs_count = 0; // number of wbfs partitions

enumError AnalyzePartitions ( FILE * outfile, bool non_found_is_ok, bool scan_wbfs )
{
    TRACE("AnalyzePartitions(,%d,%d) PART1=%d, AUTO=%d, all=%d, first=%p\n",
	non_found_is_ok, scan_wbfs,
	opt_part, opt_auto, opt_all, first_partition_info );

    AddEnvPartitions();

    // standalone --all enables --auto
    if ( opt_all && !opt_part && !opt_auto )
	ScanPartitions(false);

    wbfs_count = 0; // number of wbfs partitions

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    for ( info = first_partition_info; info; info = info->next )
    {
	TRACE("Analyze partition %s, mode=%d, source=%d\n",
		info->path, info->part_mode, info->source);
	TRACE(" - realpath: %s\n",info->real_path);

	if ( info->part_mode == PM_UNKNOWN )
	{
	    ccp read_error = 0;
	    WFile_t F;
	    InitializeWFile(&F);
	    F.disable_errors = info->source != PS_PARAM || !outfile;
	    enumError stat = OpenWFile(&F,info->real_path,IOM_IS_WBFS_PART);
	    if (stat)
	    {
		if ( info->source == PS_AUTO_IGNORE )
		    info->part_mode = PM_IGNORE;
		read_error = ""; // message already printed
		goto _done;
	    }

	    TRACE(" - st_mode=%x reg=%d dir=%d chr=%d blk=%d fifo=%d link=%d sock=%d\n",
		    F.st.st_mode,
		    S_ISREG(F.st.st_mode),
		    S_ISDIR(F.st.st_mode),
		    S_ISCHR(F.st.st_mode),
		    S_ISBLK(F.st.st_mode),
		    S_ISFIFO(F.st.st_mode),
		    S_ISLNK(F.st.st_mode),
		    S_ISSOCK(F.st.st_mode) );

	    info->filemode = GetFileMode(F.st.st_mode);
	    TRACE(" -> filemode= %x -> %d\n",F.st.st_mode,info->filemode);
	    if ( info->filemode == FM_OTHER )
	    {
		info->part_mode = PM_WRONG_TYPE;
		read_error = "Neither regular file nor char or block device: %s\n";
		goto _done;
	    }

	    TRACE("sizeof: st_size=%zd st_blksize=%zd st_blocks=%zd\n",
			sizeof(F.st.st_size),
			sizeof(F.st.st_blksize), sizeof(F.st.st_blocks) );
	    TRACE("st_blksize=%lld st_blocks=%lld\n",
			(u64)F.st.st_blksize, (u64)F.st.st_blocks );
	    info->file_size  = F.st.st_size;
	    info->hss = GetHSS(F.fd,HD_SECTOR_SIZE);
	    info->disk_usage = info->hss * (u64)F.st.st_blocks;
	    TRACE(" - hss:        %13d\n",info->hss);
	    TRACE(" - file-size:  %13lld = %5lld GiB\n",info->file_size,info->file_size/GiB);
	    TRACE(" - disk-usage: %13lld = %5lld GiB\n",info->disk_usage,info->disk_usage/GiB);

	    wbfs_head_t whead;
	    stat = F.st.st_size < sizeof(whead)
			? ERR_WARNING
			: ReadF(&F,&whead,sizeof(whead));
	    if (stat)
	    {
		read_error = "Can't read WBFS header: %s\n";
		goto _done;
	    }

	    if (memcmp(&whead.magic,"WBFS",sizeof(whead.magic)))
	    {
		info->part_mode = PM_NO_WBFS_MAGIC;
		read_error = "No WBFS magic found: %s\n";
		goto _done;
	    }
	    info->wbfs_hss  = 1 << whead.hd_sec_sz_s;
	    info->wbfs_wss  = 1 << whead.wbfs_sec_sz_s;
	    info->wbfs_size = (u64)whead.n_hd_sec * info->wbfs_hss;

	    info->part_mode = PM_WBFS_MAGIC_FOUND;
	    wbfs_count++;

	    if (!info->file_size)
	    {
		// second try: use lseek() (needed for block devices)
		info->file_size = lseek(F.fd,0,SEEK_END);
		if ( info->file_size == (off_t)-1 )
		    info->file_size = 0;
		TRACE(" - file-size:  %13lld = %5lld GiB\n",info->file_size,info->file_size/GiB);
	    }

	    if (scan_wbfs)
	    {
		OpenPartWBFS(&wbfs,info,false);
		ResetWBFS(&wbfs);
	    }

	_done:;
	    int syserr = errno;
	    ClearWFile(&F,false);

	    if ( read_error && info->part_mode != PM_IGNORE )
	    {
		TRACE(read_error," -> ",info->real_path);
		if ( info->part_mode < PM_CANT_READ )
		    info->part_mode = PM_CANT_READ;
		if ( *read_error && info->source == PS_PARAM && outfile )
		    ERROR(syserr,ERR_READ_FAILED,read_error,info->real_path);
	    }
	}
    }
    ASSERT( !wbfs.wbfs && !wbfs.sf ); // wbfs is closed!
    TRACE("*** %d WBFS partition(s) found\n",wbfs_count);

    if (!outfile)
	return wbfs_count ? ERR_OK : ERR_NO_WBFS_FOUND;

    enumError return_stat = ERR_OK;
    if ( !wbfs_count )
    {
	if ( !non_found_is_ok || verbose >= 1 )
	    ERROR0(ERR_NO_WBFS_FOUND,"no WBFS partitions found -> abort\n");
	if ( !non_found_is_ok && !return_stat )
	    return_stat = ERR_NO_WBFS_FOUND;
    }
    else if ( return_stat )
    {
	// [[2do]] ??? never reached
	if (!print_sections)
	    fprintf(outfile,"%d WBFS partition%s found\n",
			wbfs_count, wbfs_count == 1 ? "" : "s" );
	ERROR0(ERR_WARNING,"Abort because of read errors while scanning\n");
    }
    else if ( wbfs_count > 1 )
    {
	if ( !opt_all )
	    return_stat = ERROR0(ERR_TO_MUCH_WBFS_FOUND,
			"%d (more than 1) WBFS partitions found -> abort.\n",wbfs_count);
	else if ( verbose >= 1 && !print_sections )
	    fprintf(outfile,"%d WBFS partition%s found\n",
			wbfs_count, wbfs_count == 1 ? "" : "s" );
    }
    else if ( verbose > 0 && !print_sections )
    {
	fprintf(outfile,"One WBFS partition found.\n");
    }

    return return_stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ScanPartitionGames()
{
    int pi_disc_count = 0;
    pi_free_mib = 0;

    WBFS_t wbfs;
    InitializeWBFS(&wbfs);
    PartitionInfo_t * info;
    enumError stat;
    for ( stat = GetFirstWBFS(&wbfs,&info,false);
	  !stat;
	  stat = GetNextWBFS(&wbfs,&info,false) )
    {
	if ( !info->part_index || !info->wlist )
	{
	    if ( pi_count >= MAX_WBFS )
	    {
		ERROR0(ERR_TO_MUCH_WBFS_FOUND,"Too much (>%d) WBFS partitions\n",MAX_WBFS);
		break;
	    }

	    info->part_index = ++pi_count;
	    pi_list[pi_count] = info;
	    info->part_index = pi_count;
	    info->wlist = GenerateWDiscList(&wbfs,pi_count);
	}
	pi_disc_count += info->wlist->used;
	pi_free_mib += wbfs.free_mib;
    }

    if ( pi_wlist.used != pi_disc_count )
    {
	ResetWDiscList(&pi_wlist);
	pi_wlist.sort_mode = SORT_NONE;
	pi_wlist.used = pi_wlist.size = pi_disc_count;
	pi_wlist.first_disc = CALLOC(pi_disc_count,sizeof(WDiscListItem_t));

	WDiscListItem_t * dest = pi_wlist.first_disc;
	int i;
	for ( i = 1; i <= pi_count; i++ )
	{
	    WDiscList_t * wlist = pi_list[i]->wlist;
	    ASSERT(wlist);
	    ASSERT( dest-pi_wlist.first_disc + wlist->used <= pi_disc_count );
	    memcpy(dest,wlist->first_disc,wlist->used*sizeof(*dest));
	    dest += wlist->used;
	    pi_wlist.total_size_mib += wlist->total_size_mib;
	}
	ASSERT ( dest == pi_wlist.first_disc + pi_disc_count );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ScanParamID6
(
    StringField_t	* select_list,	// append all results to this list
    const ParamList_t	* param		// first param of a list to check
)
{
    DASSERT(select_list);

    char rule[8]; //, *rule_end = rule + 7;

    for ( ; param; param = param->next )
    {
	if (!param->arg)
	    continue;
	ccp arg = param->arg;
	for(;;)
	{
	    while ( *arg > 0 && *arg <= ' ' || *arg == ',' )
		arg++;
	    if (!*arg)
		break;

	    switch(*arg)
	    {
		case '+': *rule = '+'; arg++; break;
		case '/':
		case '-': *rule = '-'; arg++; break;
		default:  *rule = '+';
	    }

	    // [[2do]] ScanArgID() verwenden!

	    ccp start = arg;
	    int err = 0, wildcards = 0;
	    while ( *arg > ' ' && *arg != ',' )
	    {
		int ch = *arg++;
		if ( ch == '+' || ch == '*' )
		    wildcards++;
		else if (!isalnum(ch) && !strchr("_.",ch))
		    err++;
	    }
	    const int arglen = arg - start;
	    if ( err || wildcards > 1 || arglen > 6 )
		return ERROR0(ERR_SEMANTIC,
			"Illegal ID selector: %.*s\n", (int)(arg-start), start );

	    char * dest = rule+1;
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
		DASSERT( dest < rule + 8 );
	    }
	    while ( dest[-1] == '.' )
		dest--;
	    *dest = 0;
	    AppendStringField(select_list,rule,false);
	}
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

int AppendListID6 // returns number of inserted ids
(
    StringField_t	* id6_list,	// append all selected IDs in this list
    const StringField_t	* select_list,	// selector list
    WBFS_t		* wbfs		// open WBFS file
)
{
    DASSERT(id6_list);
    DASSERT(select_list);
    DASSERT(wbfs);

    const int count = id6_list->used;
    wbfs_t * w = wbfs->wbfs;
    if (w)
    {
	id6_t *id_list = wbfs_load_id_list(w,false);
	DASSERT(id_list);
	for ( ; **id_list; id_list++ )
	    if (MatchRulesetID(select_list,*id_list))
		InsertStringField(id6_list,*id_list,false);
    }

    return id6_list->used - count;
}

///////////////////////////////////////////////////////////////////////////////

int AppendWListID6 // returns number of inserted ids
(
    StringField_t	* id6_list,	// append all selected IDs in this list
    const StringField_t	* select_list,	// selector list
    WDiscList_t		* wlist,	// valid list
    bool		add_to_title_db	// true: add to title DB if unkown
)
{
    DASSERT(id6_list);
    DASSERT(wlist);

    const int count = id6_list->used;

    WDiscListItem_t * ptr = wlist->first_disc;
    WDiscListItem_t * end = ptr + wlist->used;
    for ( ; ptr < end; ptr++ )
	if ( !select_list || MatchRulesetID(select_list,ptr->id6) )
	{
	    InsertStringField(id6_list,ptr->id6,false);
	    if ( add_to_title_db && !GetTitle(ptr->id6,0) )
		InsertID(&title_db,ptr->id6,ptr->name64);
	}

    return id6_list->used - count;
}

///////////////////////////////////////////////////////////////////////////////

bool MatchRulesetID
(
    const StringField_t	* select_list,	// selector list
    ccp			id		// id to compare
)
{
    DASSERT(select_list);
    DASSERT(id);

    ccp * pattern = select_list->field;
    ccp * end_pattern = pattern + select_list->used;
    for ( ; pattern < end_pattern; pattern++ )
	if (MatchPatternID(*pattern+1,id))
	    return **pattern == '+';

    return !select_list->used || end_pattern[-1][0] == '-';
}

///////////////////////////////////////////////////////////////////////////////

bool MatchPatternID
(
    ccp			pattern,	// pattern, '.' is a wildcard
    ccp			id		// id to compare
)
{
    DASSERT(pattern);
    DASSERT(id);

    noTRACE("MATCH |%s|%s|\n",pattern,id);
    for(;;)
    {
	char pat = *pattern++;
	if (!pat)
	    return true;

	char ch = *id++;
	if ( !ch || pat != '.' && pat != ch )
	    return false;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckParamRename ( bool rename_id, bool allow_plus, bool allow_index )
{
    int syntax_count = 0, semantic_count = 0;
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
    {
	memset(param->selector,0,sizeof(param->selector));
	memset(param->id6,0,sizeof(param->id6));

	char * arg = (char*)param->arg;
	if (!arg)
	    continue;

	while ( *arg > 0 &&*arg <= ' ' )
	    arg++;

	long index = -1;
	if ( allow_plus && ( *arg == '+' || *arg == '*' ) )
	{
	    param->selector[0] = '+';
	    arg++;
	}
	else if ( CheckID(arg,true,false) == 6 )
	{
	    // ID6 found
	    int i;
	    for ( i = 0; i < 6; i++ )
		param->selector[i] = toupper((int)*arg++);
	}
	else if ( allow_index && *arg == '#' )
	{
	    // a slot index;
	    index = strtoul(arg+1,&arg,0);
	    snprintf(param->selector,sizeof(param->selector),"#%lu",index);
	}
	else if ( allow_index )
	{
	    char * start = arg;
	    index = strtoul(arg,&arg,0);
	    if ( arg == start )
	    {
		ERROR0(ERR_SEMANTIC,
			"ID6 or INDEX or #SLOT expected: %s\n", param->arg );
		syntax_count++;
		continue;
	    }
	    snprintf(param->selector,sizeof(param->selector),"$%lu",index);
	}
	else
	{
	    ERROR0(ERR_SEMANTIC,
		    "ID6 expected: %s\n", param->arg );
	    syntax_count++;
	    continue;
	}

	if ( index >= 0 && wbfs_count != 1 )
	{
	    ERROR0(ERR_SEMANTIC,
		"Slot or disc index is only allowed if exact 1 WBFS is selected: %s\n",
		param->arg );
	    semantic_count++;
	    continue;
	}

	if ( index > 99999 )
	{
	    ERROR0(ERR_SEMANTIC,
		"Slot or disc index too large: %s\n", param->arg );
	    semantic_count++;
	    continue;
	}

	while ( *arg > 0 &&*arg <= ' ' )
	    arg++;

	if ( *arg != '=' )
	{
	    ERROR0(ERR_SYNTAX,"Missing '=': %s -> %s\n", param->arg, arg );
	    syntax_count++;
	    continue;
	}

	arg++;
	bool scan_title = !rename_id;
	if (rename_id)
	{
	    while ( *arg > 0 &&*arg <= ' ' )
		arg++;

	    if ( *arg != ',' )
	    {
		const int idlen = CountIDChars(arg,true,true);
		if ( idlen < 1 || idlen > 6 )
		{
		    ERROR0(ERR_SYNTAX,"Missing ID: %s -> %s\n", param->arg, arg );
		    syntax_count++;
		    continue;
		}
		memset(param->id6,'.',6);
		int i;
		for ( i = 0; i < idlen; i++ )
		    param->id6[i] = toupper((int)*arg++);
		while ( *arg > 0 &&*arg <= ' ' )
		    arg++;
	    }

	    if ( *arg == ',' )
	    {
		arg++;
		scan_title = true;
	    }
	}

	if (scan_title)
	{
	    if (!*arg)
	    {
		ERROR0(ERR_SYNTAX,"Missing title: %s -> %s\n", param->arg, arg );
		syntax_count++;
		continue;
	    }
	    param->arg = arg;
	}
	else
	    param->arg = 0;
    }

    return syntax_count ? ERR_SYNTAX : semantic_count ? ERR_SEMANTIC : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                access WBFS partitions           ///////////////
///////////////////////////////////////////////////////////////////////////////

static WBFS_t wbfs_cache;
static bool wbfs_cache_valid = false;
bool wbfs_cache_enabled = true;	 // [[2do]] is 'wbfs_cache_enabled' [[obsolete]] ?

wbfs_balloc_mode_t opt_wbfs_alloc = WBFS_BA_AUTO;

//-----------------------------------------------------------------------------

enumError CloseWBFSCache()
{
    enumError err = ERR_OK;
    if (wbfs_cache_valid)
    {
	TRACE("WBFS: CLOSE CACHE: %s\n",wbfs_cache.sf->f.fname);
	wbfs_cache_valid = false;
	wbfs_cache.cache_candidate = false;
	err = ResetWBFS(&wbfs_cache);
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

void InitializeWBFS ( WBFS_t * w )
{
    ASSERT(w);
    memset(w,0,sizeof(*w));
    w->disc_slot = -1;
}

///////////////////////////////////////////////////////////////////////////////

enumError ResetWBFS ( WBFS_t * w )
{
    ASSERT(w);
    TRACE("ResetWBFS() fd=%d, alloced=%d\n", w->sf ? GetFD(&w->sf->f) : -2, w->sf_alloced );

    CloseWDisc(w);

    enumError err = ERR_OK;
    if ( wbfs_cache_enabled
	&& w->cache_candidate
	&& w->sf_alloced
	&& w->sf
	&& IsOpenSF(w->sf) )
    {
	CloseWBFSCache();
	TRACE("WBFS: SETUP CACHE: %s\n",w->sf->f.fname);
	DASSERT(!wbfs_cache_valid);
	memcpy(&wbfs_cache,w,sizeof(wbfs_cache));
	wbfs_cache_valid = true;
    }
    else
    {
	if (w->wbfs)
	    wbfs_close(w->wbfs);

	if (w->sf)
	{
	    w->sf->wbfs = 0;
	    if (w->sf_alloced)
	    {
		err = ResetSF(w->sf,0);
		FREE(w->sf);
	    }
	}
    }

    InitializeWBFS(w);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenParWBFS
	( WBFS_t * w, SuperFile_t * sf, bool print_err , wbfs_param_t * par )
{
    ASSERT(w);
    ASSERT(sf);
    ASSERT(par);
    TRACE("OpenParWBFS(%p,%p,%d,%p) fd=%d\n",
		w, sf, print_err, par, GetFD(&sf->f) );

    ResetWBFS(w);
    w->sf = sf;

    bool maybe_wbfs_file = false;
    wbfs_balloc_mode_t balloc_mode = opt_wbfs_alloc;
    if (S_ISREG(sf->f.st.st_mode))
    {
	maybe_wbfs_file = true;
	char format[2*PATH_MAX], fname[PATH_MAX];
	CalcSplitFilename(format,sizeof(format),sf->f.fname,OFT_WBFS);
	snprintf(fname,sizeof(fname),format,1);
	struct stat st;
	if (!stat(fname,&st))
	    SetupSplitWFile(&sf->f,OFT_WBFS,0);
	if ( balloc_mode == WBFS_BA_AUTO )
	    balloc_mode = WBFS_BA_FIRST; // better because of sparse effect
    }

    TRACELINE;
    if ( par->reset > 0 )
    {
	if (!par->hd_sector_size)
	    par->hd_sector_size = HD_SECTOR_SIZE;
	if (!par->num_hd_sector)
	    par->num_hd_sector = (u32)( sf->f.st.st_size / par->hd_sector_size );
    }
    else
    {
	TRACELINE;
	char buf[HD_SECTOR_SIZE];
	//char buf[HD_BLOCK_SIZE];
	enumError err = ReadAtF(&sf->f,0,&buf,sizeof(buf));
	if (err)
	    return err;

	TRACELINE;
	wbfs_head_t * whead	= (wbfs_head_t*)buf;
	par->hd_sector_size	= 1 << whead->hd_sec_sz_s;
	par->num_hd_sector	= 0; // not 'whead->n_hd_sec'
    }
    sf->f.sector_size = par->hd_sector_size;

    TRACELINE;
    ASSERT(!w->wbfs);
    TRACE("CALL wbfs_open_partition_param(ss=%u,ns=%u,reset=%d)\n",
		par->hd_sector_size, par->num_hd_sector, par->reset );

    par->read_hdsector	= WrapperReadSector;
    par->write_hdsector	= WrapperWriteSector;
    par->callback_data	= sf;
    par->part_lba	= 0;
    par->balloc_mode	= balloc_mode;

    w->wbfs = wbfs_open_partition_param(par);

    TRACELINE;
    if (!w->wbfs)
    {
	TRACE("!! can't open WBFS %s\n",sf->f.fname);
	if (print_err)
	{
	    if ( par->reset > 0 )
		ERROR0(ERR_WBFS,"Can't format WBFS partition: %s\n",sf->f.fname);
	    else
		ERROR0(ERR_WBFS_INVALID,"Invalid WBFS partition: %s\n",sf->f.fname);
	}
	return ERR_WBFS_INVALID;
    }

    TRACELINE;
    id6_t *id_list = wbfs_load_id_list(w->wbfs,1);
    CalcWBFSUsage(w);

    if ( maybe_wbfs_file && id_list[0][0] && wbfs_count_discs(w->wbfs) == 1 )
    {
	noPRINT("A WBFS FILE\n");
    	w->is_wbfs_file = true;
    }

 #ifdef DEBUG
    TRACE("WBFS %s\n\n",sf->f.fname);
    DumpWBFS(w,TRACE_FILE,15,SHOW_INTRO,0,0,0);
 #endif

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupWBFS ( WBFS_t * w, SuperFile_t * sf,
			bool print_err, int sector_size, bool recover )
{
    ASSERT(w);
    ASSERT(sf);
    TRACE("SetupWBFS(%p,%d,%d,%d) fd=%d\n",
		sf, print_err, sector_size, recover, GetFD(&sf->f) );


    wbfs_param_t par;
    memset(&par,0,sizeof(par));

    par.hd_sector_size	= sector_size;
    par.reset		= sector_size > 0;
    par.clear_inodes	= !recover;
    par.setup_iinfo	= !recover && GetFileMode(sf->f.st.st_mode) > FM_PLAIN;

    return OpenParWBFS(w,sf,print_err,&par);
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateGrowingWBFS ( WBFS_t * w, SuperFile_t * sf, off_t size, int sector_size )
{
    ASSERT(w);
    ASSERT(sf);
    ASSERT(size);
    ASSERT(sector_size);
    TRACE("CreateGrowingWBFS(%p,%p,%d)\n",w,sf,sector_size);

    if ( S_ISREG(sf->f.st.st_mode) && sf->src && prealloc_mode > PREALLOC_OFF )
    {
	const u32 bl_size = wbfs_calc_sect_size(size,sector_size);
	if ( prealloc_mode == PREALLOC_ALL )
	{
	    wd_disc_t * disc = OpenDiscSF(sf->src,false,false);
	    if (disc)
	    {
		const u32 sect_per_block = bl_size / WII_SECTOR_SIZE;
		const u32 n_blocks = wd_count_used_disc_blocks(disc,sect_per_block,0);
		noPRINT("NB = ( %u + 1 ) * 0x%x == %llx\n",
			n_blocks, bl_size, (n_blocks+1) * (u64)bl_size );
		PreallocateF( &sf->f, 0, (n_blocks+1) * (u64)bl_size );
	    }
	}
	else
	{
	    PreallocateF(&sf->f,0,sector_size
				+  ALIGN64( sizeof(wbfs_disc_info_t)
					+ 2*WII_MAX_DISC_SIZE/bl_size,sector_size) );
	    PreallocateSF(sf,bl_size,0,bl_size/WII_SECTOR_SIZE,1);
	}
    }

    wbfs_param_t par;
    memset(&par,0,sizeof(par));

    par.num_hd_sector	= (u32)( size / sector_size );
    par.hd_sector_size	= sector_size;
    par.reset		= 1;
    par.iinfo.mtime	= hton64(sf->f.fatt.mtime);

    sf->f.read_behind_eof = 2;

    const enumError err = OpenParWBFS(w,sf,true,&par);
    if ( !err && S_ISREG(sf->f.st.st_mode) )
    {
	TRACE("GROWING WBFS: %s\n",sf->f.fname);
	w->is_growing = true;
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

static enumError OpenWBFSHelper
(
	WBFS_t		* w,
	ccp		filename,
	bool		open_modify,	// true: open read+write
	bool		print_err, 
	wbfs_param_t	*par,
	int		sector_size,
	bool		recover
)
{
    ASSERT(w);
    PRINT("OpenWBFSHelper(%s,rw=%d,pe=%d,ss=%d,recover=%d)\n",
		filename, open_modify, print_err, sector_size, recover );

    if ( wbfs_cache_valid
	&& IsOpenSF(wbfs_cache.sf)
	&& ( !open_modify || IsWritableSF(wbfs_cache.sf) )
	&& !strcmp(wbfs_cache.sf->f.fname,filename) )
    {
	PRINT("WBFS: USE CACHE: %s\n",wbfs_cache.sf->f.fname);
	wbfs_cache_valid = false;
	ResetWBFS(w);
	memcpy(w,&wbfs_cache,sizeof(*w));
	return ERR_OK;
    }
    CloseWBFSCache();

    SuperFile_t * sf = MALLOC(sizeof(SuperFile_t));
    InitializeSF(sf);
    sf->f.disable_errors = !print_err;
    enumError err = open_modify
			? OpenWFileModify(&sf->f,filename,IOM_IS_WBFS_PART)
			: OpenWFile(&sf->f,filename,IOM_IS_WBFS_PART);
    if (err)
	goto abort;
    sf->f.disable_errors = false;
    SetupIOD(sf,OFT_PLAIN,OFT_PLAIN);

    err = par ? OpenParWBFS(w,sf,print_err,par)
	      : SetupWBFS(w,sf,print_err,sector_size,recover);
    if (err)
	goto abort;

    w->sf_alloced = true;
    w->cache_candidate = true;
    return ERR_OK;

 abort:
    ResetWBFS(w);
    ResetSF(sf,0);
    FREE(sf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenWBFS
(
	WBFS_t		*w,		// valid data structure
	ccp		filename,	// filename to open
	bool		open_modify,	// true: open read+write
	bool		print_err,	// true: pprint error messages
	wbfs_param_t	*par		// NULL or parameter record
)
{
    PRINT("OpenWBFS(rw=%d,pr-err=%d)\n",open_modify,print_err);
    return OpenWBFSHelper(w,filename,open_modify,print_err,par,0,false);
}

///////////////////////////////////////////////////////////////////////////////

enumError FormatWBFS
	( WBFS_t * w, ccp filename, bool print_err,
	  wbfs_param_t * par, int sector_size, bool recover )
{
    if ( sector_size < HD_SECTOR_SIZE )
	sector_size = HD_SECTOR_SIZE;
    return OpenWBFSHelper(w,filename,true,print_err,par,sector_size,recover);
}

///////////////////////////////////////////////////////////////////////////////

enumError RecoverWBFS ( WBFS_t * wbfs, ccp fname, bool testmode )
{
    ASSERT(wbfs);
    ASSERT(wbfs->wbfs);
    ASSERT(wbfs->sf);

    wbfs_t * w = wbfs->wbfs;
    ASSERT(w);
    ASSERT(w->head);
    ASSERT(w->head->disc_table);

    wbfs_load_freeblocks(w);
    ASSERT(w->freeblks);

    enumError err = ERR_OK;

    // load first wbfs sector
    u8 * first_sector = MALLOC(w->wbfs_sec_sz);
    DASSERT(wbfs->sf);
    err = ReadSF(wbfs->sf,0,first_sector,w->wbfs_sec_sz);
    if (!err)
    {
	bool inodes_dirty = false;

	int slot;
	for ( slot = 0; slot < w->max_disc; slot++ )
	{
	    if (!w->head->disc_table[slot])
	    {
		w->head->disc_table[slot] = WBFS_SLOT_VALID|WBFS_SLOT__USER;
		wd_header_t * head
		    = (wd_header_t*)( first_sector + w->hd_sec_sz + slot * w->disc_info_sz );
		if ( ntohl(head->wii_magic) == WII_MAGIC_DELETED )
		{
		    head->wii_magic = htonl(WII_MAGIC);
		    inodes_dirty = true;
		}
	    }
	    else
		w->head->disc_table[slot] &= ~WBFS_SLOT__USER;
	}

	if (inodes_dirty)
	    WriteSF(wbfs->sf,
		    w->hd_sec_sz,
		    first_sector + w->hd_sec_sz,
		    w->max_disc * w->disc_info_sz );

	memset(w->freeblks,0,w->freeblks_size4*4);
	SyncWBFS(wbfs,true);

	CheckWBFS_t ck;
	InitializeCheckWBFS(&ck);
	TRACELINE;
	if (CheckWBFS(&ck,wbfs,0,-1,0,0))
	{
	    err = ERR_DIFFER;
	    ASSERT(ck.disc);
	    bool dirty = false;
	    int n_recoverd = 0;

	    for ( slot = 0; slot < w->max_disc; slot++ )
		if ( w->head->disc_table[slot] & WBFS_SLOT__USER )
		{
		    CheckDisc_t * cd = ck.disc + slot;
		    if (   cd->no_blocks
			|| cd->bl_overlap
			|| cd->bl_invalid )
		    {
			w->head->disc_table[slot] = WBFS_SLOT_FREE;
			dirty = true;
		    }
		    else
			n_recoverd++;
		}

	    if (n_recoverd)
	    {
		if (testmode)
		    printf(" * WOULD recover %u disc%s:\n",
			n_recoverd, n_recoverd == 1 ? "" : "s" );
		else
		    printf(" * %u disc%s recoverd\n",
			n_recoverd, n_recoverd == 1 ? "" : "s" );

		for ( slot = 0; slot < w->max_disc; slot++ )
		    if ( w->head->disc_table[slot] & WBFS_SLOT__USER )
		    {
			w->head->disc_table[slot] &= ~WBFS_SLOT__USER;
			wd_header_t * inode
			    = (wd_header_t*)( first_sector + w->hd_sec_sz
						+ slot * w->disc_info_sz );
			ccp id6 = &inode->disc_id;
			printf("   - slot #%03u [%.6s] %s\n",
				slot, id6, GetTitle(id6,(ccp)inode->disc_title) );
		    }
	    }

	    if (!testmode)
	    {
		if (dirty)
		{
		    ResetCheckWBFS(&ck);
		    SyncWBFS(wbfs,true);
		    CheckWBFS(&ck,wbfs,0,-1,0,0);
		}

		TRACELINE;
		RepairWBFS(&ck,0,REPAIR_FBT|REPAIR_RM_INVALID|REPAIR_RM_EMPTY,-1,0,0);
		TRACELINE;
		ResetCheckWBFS(&ck);
		SyncWBFS(wbfs,true);
		if (CheckWBFS(&ck,wbfs,0,1,stdout,1))
		    printf(" *** Run REPAIR %s ***\n\n", fname ? fname : wbfs->sf->f.fname );
		else
		    putchar('\n');
	    }
	}
	ResetCheckWBFS(&ck);

	if (testmode)
	{
	    WriteSF(wbfs->sf,0,first_sector,w->wbfs_sec_sz);
	    err = ReloadWBFS(wbfs);
	}
    }

    FREE(first_sector);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError TruncateWBFS ( WBFS_t * w )
{
    ASSERT(w);
    TRACE("TruncateWBFS() fd=%d fp=%p\n",
		w->sf ? GetFD(&w->sf->f) : -2,
		w->sf ? GetFP(&w->sf->f) : 0 );

    enumError err = CloseWDisc(w);
    SyncWBFS(w,false);
    if ( w->wbfs && w->sf )
    {
	wbfs_trim(w->wbfs);
	const u64 cut = (u64)w->wbfs->n_hd_sec * w->wbfs->hd_sec_sz;
	TRACE(" - cut = %u * %u = %llu = %llx/hex\n",
		w->wbfs->n_hd_sec, w->wbfs->hd_sec_sz, cut, cut );
	SetSizeF(&w->sf->f,cut);
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CalcWBFSUsage ( WBFS_t * w )
{
    ASSERT(w);
    if (!w->wbfs)
    {
	w->used_discs	= 0;
	w->total_discs	= 0;
	w->free_discs	= 0;
	w->free_mib	= 0;
	w->total_mib	= 0;
	w->used_mib	= 0;
	return ERR_NO_WBFS_FOUND;
    }

    w->used_discs	= wbfs_count_discs(w->wbfs);
    w->total_discs	= w->wbfs->max_disc;
    w->free_discs	= w->total_discs - w->used_discs;

    u32 free_count	= wbfs_get_free_block_count(w->wbfs);
    w->free_blocks	= free_count;
    w->free_mib		= ( (u64)w->wbfs->wbfs_sec_sz * free_count ) / MiB; // round down!
    w->total_mib	= ( (u64)w->wbfs->wbfs_sec_sz * w->wbfs->n_wbfs_sec + MiB/2 ) / MiB;
    w->used_mib		= w->total_mib - w->free_mib;

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SyncWBFS ( WBFS_t * w, bool force_sync )
{
    ASSERT(w);
    wbfs_t * wbfs = w->wbfs;
    if (!wbfs)
	return ERR_OK;

    wbfs_disc_t * disc = w->disc;
    if ( disc && ( force_sync || disc->is_dirty ) )
	wbfs_sync_disc_header(disc);

    if ( force_sync || wbfs->is_dirty )
	wbfs_sync(wbfs);

    return CalcWBFSUsage(w);
}

///////////////////////////////////////////////////////////////////////////////

enumError ReloadWBFS ( WBFS_t * wbfs )
{
    ASSERT(wbfs);
    enumError err = ERR_OK;

    wbfs_t * w = wbfs->wbfs;
    if (w)
    {
	FREE(w->freeblks);
	w->freeblks = 0;
	FREE(w->id_list);
	w->id_list = 0;
	if ( w->head && wbfs->sf )
	{
	    err = ReadSF(wbfs->sf,0,w->head,w->hd_sec_sz);
	    CalcWBFSUsage(wbfs);
	}
    }

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenPartWBFS ( WBFS_t * w, PartitionInfo_t * info, bool open_modify )
{
    ASSERT(info);
    if ( !info || info->part_mode < PM_WBFS_MAGIC_FOUND )
    {
	ResetWBFS(w);
	return ERR_NO_WBFS_FOUND;
    }

    const enumError err	= OpenWBFSHelper( w, info->real_path, open_modify,
					info->source==PS_PARAM, 0, 0, 0 );
    if (err)
    {
	info->part_mode = PM_WBFS_INVALID;
	info->ignore = true;
	return err;
    }

    info->part_mode = PM_WBFS;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError GetWBFSHelper
	( WBFS_t * w, PartitionInfo_t ** p_info, bool open_modify )
{
    ResetWBFS(w);

    if (p_info)
    {
	PartitionInfo_t * info;
	for ( info = *p_info; info; info = info->next )
	    if ( !info->ignore && OpenPartWBFS(w,info,open_modify) == ERR_OK )
	    {
		*p_info = info;
		return ERR_OK;
	    }
	*p_info = 0;
    }
    return ERR_NO_WBFS_FOUND;
}

//-----------------------------------------------------------------------------

enumError GetFirstWBFS ( WBFS_t * w, PartitionInfo_t ** info, bool open_modify )
{
    if (info)
	*info = first_partition_info;
    return GetWBFSHelper(w,info,open_modify);
}

//-----------------------------------------------------------------------------

enumError GetNextWBFS ( WBFS_t * w, PartitionInfo_t ** info, bool open_modify )
{
    if ( info && *info )
	*info = (*info)->next;
    return GetWBFSHelper(w,info,open_modify);
}

//-----------------------------------------------------------------------------

static void print_wbfs_section 
(
    WBFS_t		* w,		// valid and opened WBFS
    int			count,		// wbfs counter, 1 based
					// if NULL: neither 'count' nor 'total' are printed
    int			total,		// total wbfs count to handle
					// if NULL: don't print info
    ccp			path,		// path of sourcefile
					// if NULL: use 'w->sf->f.fname' (real path)
    ccp			sub_mode,	// "open" | "close"
    ccp			term_string	// append this string
)
{
    DASSERT(w);
    DASSERT(w->wbfs);
    DASSERT(sub_mode);
    DASSERT(term_string);

    if (!count)
	printf("[wbfs:%s]\n",sub_mode);
    else if (!total)
	printf("[wbfs:%s]\nwbfs-count=%u\n",sub_mode,count);
    else
	printf("[wbfs:%s]\nwbfs-count=%u\nwbfs-total=%u\n",sub_mode,count,total);

    printf(
	"path=%s\n"
	"version=%u\n"
	"n-discs=%u\n"
	"max-discs=%u\n"
	"free-mib=%u\n"
	"used-mib=%u\n"
	"total-mib=%u\n"
	"%s"
	,path ? path : w->sf ? w->sf->f.fname : ""
	,w->wbfs->head->wbfs_version
	,wbfs_count_discs(w->wbfs)
	,w->wbfs->max_disc
	,w->free_mib
	,w->used_mib
	,w->total_mib
	,term_string
	);
}
 
//-----------------------------------------------------------------------------

void LogOpenedWBFS
(
    WBFS_t		* w,		// valid and opened WBFS
    int			count,		// wbfs counter, 1 based
					// if NULL: neither 'count' nor 'total' are printed
    int			total,		// total wbfs count to handle
					// if NULL: don't print info
    ccp			path		// path of sourcefile
					// if NULL: use 'w->sf->f.fname' (real path)
)
{
    DASSERT(w);
    DASSERT(w->wbfs);
    if ( !w || !w->wbfs )
	return;

    if (!path)
	path = w->sf ? w->sf->f.fname : "";
	
    if (print_sections)
	print_wbfs_section(w,count,total,path,"open","\n");
    else if (!count)
	printf("%sWBFSv%u opened: %s\n",
		verbose>0 ? "\n" : "",
		w->wbfs->head->wbfs_version, path );
    else if (!total)
	printf("%sWBFSv%u #%u opened: %s\n",
		verbose>0 ? "\n" : "",
		w->wbfs->head->wbfs_version, count, path );
    else
	printf("%sWBFSv%u #%u/%u opened: %s\n",
		verbose>0 ? "\n" : "",
		w->wbfs->head->wbfs_version, count, total, path );
}

//-----------------------------------------------------------------------------

void LogCloseWBFS
(
    WBFS_t		* w,		// valid and opened WBFS
    int			count,		// wbfs counter, 1 based
					// if NULL: neither 'count' nor 'total' are printed
    int			total,		// total wbfs count to handle
					// if NULL: don't print info
    ccp			path		// path of sourcefile
					// if NULL: use 'w->sf->f.fname' (real path)
)
{
    if (print_sections)
	print_wbfs_section(w,count,total,path,"close","");
}

//-----------------------------------------------------------------------------

uint CountWBFS()
{
    uint count = 0;
    PartitionInfo_t * info;
    for ( info = first_partition_info; info; info = info->next )
	if ( !info->ignore && info->part_mode >= PM_WBFS_MAGIC_FOUND )
	    count++;
    TRACE("CountWBFS(), N=%u\n",count);
    return count;
}

//-----------------------------------------------------------------------------

uint GetIdWBFS ( WBFS_t * wbfs, IdField_t * idf )
{
    DASSERT(wbfs);
    DASSERT(idf);
    uint count = 0;
    
    wbfs_t *w = wbfs->wbfs;
    if (w)
    {
	int slot;
	wd_header_t head;
	char slot_name[20];
	const int slot_fw = snprintf(slot_name,sizeof(slot_name),"#%u",w->max_disc-1);
	
	for ( slot = 0; slot < w->max_disc; slot++ )
	    if (!wbfs_get_disc_info_by_slot(w,slot,(u8*)&head,sizeof(head),0,0,0,0))
	    {
		const time_t mtime = wbfs_is_inode_info_valid(w,&head.iinfo)
					? ntoh64(head.iinfo.mtime) : 0;
		snprintf(slot_name,sizeof(slot_name),"#%0*u",slot_fw,slot);
		InsertIdField(idf,&head.disc_id,0,mtime,slot_name);
	    }
    }
    
    return count;
}

///////////////////////////////////////////////////////////////////////////////

StringField_t wbfs_part_list;

u32 FindWBFSPartitions()
{
    static bool scanned = false;
    if (!scanned)
    {
	scanned = true;
	noPRINT("SCAN FOR WBFS PARTITIONS\n");
	ScanPartitions(true);
	AnalyzePartitions(0,true,false);

	WBFS_t wbfs;
	InitializeWBFS(&wbfs);
	enumError err = ERR_OK;
	InitializeStringField(&wbfs_part_list);
	PartitionInfo_t * info;
	for ( err = GetFirstWBFS(&wbfs,&info,false);
	      !err && !SIGINT_level;
	      err = GetNextWBFS(&wbfs,&info,false) )
	{
	    noPRINT("ADD WBFS PARTITION: %s,%d\n",info->path,info->part_mode);
	    AppendStringField(&wbfs_part_list,info->path,false);
	}
	ResetWBFS(&wbfs);
    }

    return wbfs_part_list.used;
}

///////////////////////////////////////////////////////////////////////////////

enumError DumpWBFS
(
    WBFS_t	* wbfs,			// valid WBFS
    FILE	* f,			// valid output file
    int		indent,			// indention of output
    ShowMode	show_mode,		// what should be printed
    int		dump_level,		// dump level: 0..3, ignored if show_mode is set
    int		view_invalid_discs,	// !=0: view invalid discs too
    CheckWBFS_t	* ck			// not NULL: dump only discs with errors
)
{
    ASSERT(wbfs);
    char buf[100];

    if ( !f || !wbfs )
	return ERROR0(ERR_INTERNAL,0);

    wbfs_t * w = wbfs->wbfs;
    if (!w)
	return ERR_NO_WBFS_FOUND;


    //----- options --show and --long

    const int prt_sections = show_mode & SHOW_F_SECTIONS;
    if (prt_sections)
    {
	indent = 2;
	fprintf(f,"[check:info]\n\n");
    }

    if ( show_mode & SHOW__DEFAULT )
    {
	show_mode = SHOW__ALL;
	switch (dump_level)
	{
	    case 0:  
		show_mode &= ~SHOW_FILES;
		// fall through

	    case 1:
		show_mode &= ~SHOW_D_MAP;
		// fall through

	    case 2:  
		show_mode &= ~SHOW_W_MAP;
		// fall through

	    case 3:
		show_mode &= ~SHOW_USAGE;
		break;
	}
    }

    if ( show_mode & SHOW_INTRO )
	show_mode |= SHOW_FHEADER | SHOW_SLOT | SHOW_GEOMETRY;

    if ( view_invalid_discs )
	show_mode |= SHOW_FILES;


    //--- print WBFS header

    indent = NormalizeIndent(indent);
    wbfs_head_t * head = w->head;
    if ( head && show_mode&(SHOW_FHEADER|SHOW_SLOT) )
    {
	fprintf(f,"%*sWBFS-Header:\n", indent,"");
	if ( show_mode & SHOW_FHEADER )
	{
	    ccp magic = (ccp)&head->magic;
	    fprintf(f,"%*s  WBFS MAGIC: %10x %02x %02x %02x =         '%s'\n",
			    indent,"",
			    magic[0], magic[1], magic[2], magic[3],
			    wd_print_id(magic,4,0) );

	    fprintf(f,"%*s  WBFS VERSION:     %#13x =%15u\n", indent,"",
			    head->wbfs_version, head->wbfs_version );

	    fprintf(f,"%*s  hd sectors:       %#13x =%15u\n", indent,"",
			    (u32)htonl(head->n_hd_sec), (u32)htonl(head->n_hd_sec) );
	    u32 n = 1 << head->hd_sec_sz_s;
	    fprintf(f,"%*s  hd sector size:     %#11x =%15u =    2^%u\n", indent,"",
			    n, n, head->hd_sec_sz_s );
	    n = 1 << head->wbfs_sec_sz_s;
	    fprintf(f,"%*s  WBFS sector size:   %#11x =%15u =    2^%u\n\n", indent,"",
			    n, n, head->wbfs_sec_sz_s );
	}

	if ( show_mode & SHOW_SLOT )
	{
	    DASSERT(sizeof(buf)>=60);
	    fprintf(f,"%*s  Disc table (slot usage):\n", indent,"" );
	    u8 * dt = head->disc_table;
	    int count = w->max_disc, idx = 0;
	    while ( count > 0 )
	    {
		const int max = 50;
		int i, n = count < max ? count : max;
		char * dest = buf;
		for ( i = 0; i < n; i++ )
		{
		    if (!(i%10))
			*dest++ = ' ';
		    *dest++ = wbfs_slot_mode_info[ *dt++ & WBFS_SLOT__MASK ];
		}
		*dest = 0;
		fprintf( f, "%*s    %3d..%3d:%s\n", indent,"", idx, idx+n-1, buf );
		idx += n;
		count -= n;
	    }
	}
	fputc('\n',f);
    }
    else if (!head)
	fprintf(f,"%*s!! NO WBFS HEADER DEFINED !!\n\n", indent,"");


    //--- some calculations

    const u32 NSEC		= w->n_wbfs_sec;
    const u32 NSEC2		= w->n_wbfs_sec / 2;

    const u32 used_blocks	= NSEC - wbfs->free_blocks;
    const u32 used_perc		= NSEC ? ( 100 * used_blocks       + NSEC2 ) / NSEC : 0;
    const u32 free_perc		= NSEC ? ( 100 * wbfs->free_blocks + NSEC2 ) / NSEC : 0;

    const u64 wbfs_used		= (u64)w->wbfs_sec_sz * used_blocks;
    const u64 wbfs_free		= (u64)w->wbfs_sec_sz * wbfs->free_blocks;
    const u64 wbfs_total	= (u64)w->wbfs_sec_sz * NSEC;

    const u32 used_mib		= ( wbfs_used  + MiB/2 ) / MiB;
    const u32 free_mib		= ( wbfs_free  + MiB/2 ) / MiB;
    const u32 total_mib		= ( wbfs_total + MiB/2 ) / MiB;

    const u64 hd_total		= (u64)w->hd_sec_sz * w->n_hd_sec;


    //--- print WBFS geometry

    if ( show_mode & SHOW_GEOMETRY )
    {
	fprintf(f,"%*shd sector size:       %#11x =%15u =    2^%u\n", indent,"",
		w->hd_sec_sz, w->hd_sec_sz, w->hd_sec_sz_s );
	fprintf(f,"%*shd num of sectors:    %#11x =%15u\n", indent,"",
		w->n_hd_sec, w->n_hd_sec );
	fprintf(f,"%*shd total size:    %#15llx =%15llu =%8llu MiB\n\n", indent,"",
		hd_total, hd_total, ( hd_total + MiB/2 ) / MiB  );

	fprintf(f,"%*swii sector size:      %#11x =%15u =    2^%u\n", indent,"",
		w->wii_sec_sz, w->wii_sec_sz, w->wii_sec_sz_s );
	fprintf(f,"%*swii sectors/disc:     %#11x =%15u\n", indent,"",
		w->n_wii_sec_per_disc, w->n_wii_sec_per_disc  );
	fprintf(f,"%*swii num of sectors:   %#11x =%15u\n", indent,"",
		w->n_wii_sec, w->n_wii_sec );
	u64 wii_total =(u64)w->wii_sec_sz * w->n_wii_sec;
	fprintf(f,"%*swii total size:   %#15llx =%15llu =%8llu MiB\n\n", indent,"",
		wii_total, wii_total, ( wii_total + MiB/2 ) / MiB  );

	fprintf(f,"%*swbfs block size:      %#11x =%15u =    2^%u\n", indent,"",
		w->wbfs_sec_sz, w->wbfs_sec_sz, w->wbfs_sec_sz_s );
	fprintf(f,"%*swbfs blocks/disc:     %#11x =%15u\n", indent,"",
		w->n_wbfs_sec_per_disc, w->n_wbfs_sec_per_disc );
	fprintf(f,"%*swbfs free blocks:     %#11x =%15u =%8u MiB = %3u%%\n", indent,"",
		wbfs->free_blocks, wbfs->free_blocks, free_mib, free_perc );
	fprintf(f,"%*swbfs used blocks:     %#11x =%15u =%8u MiB = %3u%%\n", indent,"",
		used_blocks, used_blocks, used_mib, used_perc );
	fprintf(f,"%*swbfs total blocks:    %#11x =%15u =%8u MiB = 100%%\n\n", indent,"",
		w->n_wbfs_sec, w->n_wbfs_sec, total_mib );

	const u64 wbfs_max  = (u64)w->wbfs_sec_sz * WBFS_MAX_SECTORS;
	const u64 wbfs_trim = (u64)w->wbfs_sec_sz * (wbfs_find_last_used_block(w)+1);
	const u32 max_mib   = ( wbfs_max  + MiB/2 ) / MiB;
	const u32 trim_mib  = ( wbfs_trim + MiB/2 ) / MiB;
	fprintf(f,"%*swbfs min possible:%#15llx =%15llu =%8u MiB =%4llu%%\n", indent,"",
		wbfs_used, wbfs_used, used_mib, 100*wbfs_used/wbfs_total );
	fprintf(f,"%*swbfs trimmed size:%#15llx =%15llu =%8u MiB =%4llu%%\n", indent,"",
		wbfs_trim, wbfs_trim, trim_mib, 100*wbfs_trim/wbfs_total );
	fprintf(f,"%*swbfs total size:  %#15llx =%15llu =%8u MiB = 100%%\n", indent,"",
		wbfs_total, wbfs_total, total_mib  );
	fprintf(f,"%*swbfs max possible:%#15llx =%15llu =%8u MiB =%4llu%%\n\n", indent,"",
		wbfs_max, wbfs_max, max_mib, 100*wbfs_max/wbfs_total );

	fprintf(f,"%*spartition lba:        %#11x =%15u\n", indent,"",
		w->part_lba, w->part_lba );
	fprintf(f,"%*sfree blocks lba:      %#11x =%15u\n", indent,"",
		w->freeblks_lba, w->freeblks_lba );
	const u32 fb_lb_size = w->freeblks_lba_count * w->hd_sec_sz;
	fprintf(f,"%*sfree blocks lba size: %#11x =%15u =%8u block%s\n", indent,"",
		fb_lb_size, fb_lb_size,
		w->freeblks_lba_count, w->freeblks_lba_count == 1 ? "" : "s" );
	fprintf(f,"%*sfree blocks size:     %#11x =%15u\n", indent,"",
		w->freeblks_size4 * 4, w->freeblks_size4 * 4 );
	fprintf(f,"%*sfb last u32 mask:     %#11x =%15u\n", indent,"",
		w->freeblks_mask, w->freeblks_mask );
	fprintf(f,"%*sdisc info size:       %#11x =%15u\n\n", indent,"",
		w->disc_info_sz, w->disc_info_sz );

	fprintf(f,"%*sused slots (wii discs):  %8u =%14u%%\n", indent,"",
		wbfs->used_discs, 100 * wbfs->used_discs / w->max_disc );
	fprintf(f,"%*stotal slots (wii discs): %8u =%14u%%\n", indent,"",
		w->max_disc, 100 );
    }


    //--- print disc list

    MemMap_t mm;
    MemMapItem_t * mi;
    InitializeMemMap(&mm);

    if ( show_mode & (SHOW_W_MAP|SHOW_D_MAP|SHOW_FILES) )
    {
	ASSERT(w);
	const u32 sec_per_disc = w->n_wbfs_sec_per_disc;
	const u32 wii_sec_per_wbfs_sect
			= 1 << ( w->wbfs_sec_sz_s - w->wii_sec_sz_s );
	const off_t sec_size  = w->wbfs_sec_sz;
	const off_t sec_delta = w->part_lba * w->hd_sec_sz;

	u32 slot;
	WDiscInfo_t dinfo;
	for ( slot = 0; slot < w->max_disc; slot++ )
	{
	    noTRACE("---");
	    noTRACE(">> SLOT = %d/%d\n",slot,w->max_disc);
	    if ( ck && ck->disc && !ck->disc[slot].err_count )
		continue;

	    wbfs_disc_t * d = wbfs_open_disc_by_slot(w,slot,view_invalid_discs);
	    if (!d)
		continue;
	    wd_header_t ihead;
	    LoadIsoHeader(wbfs,&ihead,d);

	    if (GetWDiscInfoBySlot(wbfs,&dinfo,slot))
	    {
		if ( !view_invalid_discs
		    || ( view_invalid_discs < 2 && !d->is_valid && !d->is_deleted ) )
		{
		    if ( !view_invalid_discs && show_mode &SHOW_FILES )
			fprintf(f,"\n%*s!! NO INFO ABOUT DISC #%d AVAILABLE !!\n",indent,"",slot);
		    wbfs_close_disc(d);
		    continue;
		}

		memset(&dinfo,0,sizeof(dinfo));
		memcpy(&dinfo.dhead,d->header->dhead,sizeof(dinfo.dhead));
		CalcWDiscInfo(&dinfo,0);
	    }

	    if ( show_mode & (SHOW_FILES|SHOW_D_MAP) )
	    {
		fprintf(f,"\n%*sDump of %sWii disc at slot #%d of %d:\n",
			    indent,"", d->is_used ? "" : "*DELETED* ", slot, w->max_disc );
		if ( show_mode & SHOW_FILES )
		    DumpWDiscInfo( &dinfo, d->is_used ? &ihead : 0, f, indent+2 );
		if ( w->head->disc_table[slot] & WBFS_SLOT_INVALID )
		    fprintf(f,"%*s>>> DISC MARKED AS INVALID! <<<\n",indent,"");
		else
		{
		    if ( w->head->disc_table[slot] & WBFS_SLOT_F_SHARED )
			fprintf(f,"%*s>>> DISC IS/WAS SHARING BLOCKS WITH OTHER DISCS! <<<\n",indent,"");
		    if ( w->head->disc_table[slot] & WBFS_SLOT_F_FREED )
			fprintf(f,"%*s>>> DISC IS/WAS USING FREE BLOCKS! <<<\n",indent,"");
		}
	    }

	    if ( show_mode & (SHOW_D_MAP|SHOW_W_MAP) )
	    {
		int ind = indent + 3;
		if ( show_mode & SHOW_D_MAP )
		  fprintf(f,"\n%*sWii disc memory mapping:\n\n"
		    "%*s   mapping index :    wbfs blocks :      disc offset range :     size\n"
		    "%*s----------------------------------------------------------------------\n",
		    ind-1,"", ind,"", ind,"" );
		u16 * tab = d->header->wlba_table;

		u8 used[WBFS_MAX_SECTORS];
		memset(used,0,sizeof(used));
		ASSERT( sec_per_disc < sizeof(used) );

		int idx = 0;
		for (;;)
		{
		    while ( idx < sec_per_disc && !tab[idx] )
			idx++;
		    if ( idx == sec_per_disc )
			break;
		    const u32 start = idx;
		    int delta = ntohs(tab[idx])-idx;
		    while ( idx < sec_per_disc )
		    {
			u32 wlba = ntohs(tab[idx]);
			if (!wlba)
			    break;
			if ( wlba < sizeof(used) )
			    used[wlba] = 1;
			if ( wlba-idx != delta )
			    break;
			idx++;
		    }

		    if ( !( show_mode & SHOW_D_MAP ))
			continue;

		    off_t off  = start * wii_sec_per_wbfs_sect * (u64)WII_SECTOR_SIZE;
		    off_t size = (idx-start) * wii_sec_per_wbfs_sect * (u64)WII_SECTOR_SIZE;

		    if ( start == idx - 1 )
		    	fprintf(f,"%*s%11u      : %9u      :%10llx ..%10llx :%9llx\n",
			    ind,"",
			    start,
			    start + delta,
			    (u64)off,
			    (u64)( off + size ),
			    (u64)size );
		    else
		    	fprintf(f,"%*s%7u ..%6u : %5u .. %5u :%10llx ..%10llx :%9llx\n",
			    ind,"",
			    start,
			    idx - 1,
			    start + delta,
			    idx - 1 + delta,
			    (u64)off,
			    (u64)( off + size ),
			    (u64)size );
		}

		if ( show_mode & SHOW_W_MAP && d->is_used )
		{
		    mi = InsertMemMap(&mm, w->hd_sec_sz+slot*w->disc_info_sz,
				sizeof(d->header->dhead)
				+ sizeof(*d->header->wlba_table) * w->n_wbfs_sec_per_disc );
		    snprintf(mi->info,sizeof(mi->info),
				"Inode of slot #%03u [%s]",slot,dinfo.id6);

		    int idx = 0, segment = 0;
		    for ( ;; segment++ )
		    {
			while ( idx < NSEC && !used[idx] )
			    idx++;
			if ( idx == NSEC )
			    break;
			const u32 start = idx;
			while ( idx < NSEC && used[idx] )
			    idx++;

			const int n = idx - start;
			mi = InsertMemMap(&mm, start*sec_size+sec_delta, n*sec_size );
			if (segment)
			    snprintf(mi->info,sizeof(mi->info),
				"%4u block%s -> disc #%03u.%u [%s]",
				n, n == 1 ? " " : "s", slot, segment, dinfo.id6 );
			else
			    snprintf(mi->info,sizeof(mi->info),
				"%4u block%s -> disc #%03u   [%s]",
				n, n == 1 ? " " : "s", slot, dinfo.id6 );
		    }
		}
	    }
	    wbfs_close_disc(d);
	}
    }
    fputc('\n',f);

    if ( show_mode & SHOW_W_MAP )
    {
	mi = InsertMemMap(&mm,0,sizeof(wbfs_head_t));
	StringCopyS(mi->info,sizeof(mi->info),"WBFS header");

	mi = InsertMemMap(&mm,sizeof(wbfs_head_t),w->max_disc);
	StringCopyS(mi->info,sizeof(mi->info),"Disc table");

	int slot = 0;
	for (;;)
	{
	    int start = slot;
	    while ( slot < w->max_disc && !head->disc_table[slot] )
		slot++;
	    int count = slot - start;
	    if (count)
	    {
		mi = InsertMemMap(&mm, w->hd_sec_sz + start * w->disc_info_sz,
					count * w->disc_info_sz );
		if ( count > 1 )
		    snprintf(mi->info,sizeof(mi->info),
				"%d unused inodes, slots #%03u..#%03u",
				count, start, slot-1 );
		else
		    snprintf(mi->info,sizeof(mi->info),
				"Unused inode, slot #%03u",start);
	    }
	    if ( slot == w->max_disc )
		break;
	    slot++;
	}

	const off_t fbt_off  = ( w->part_lba + w->freeblks_lba ) * w->hd_sec_sz;
	mi = InsertMemMap(&mm,fbt_off,w->freeblks_size4*4);
	StringCopyS(mi->info,sizeof(mi->info),"Free blocks table");

	mi = InsertMemMap(&mm,w->wbfs_sec_sz,0);
	StringCopyS(mi->info,sizeof(mi->info),"-- end of management block #0 --");

	if ( wbfs_total < hd_total )
	{
	    mi = InsertMemMap(&mm,wbfs_total,0);
	    StringCopyS(mi->info,sizeof(mi->info),"-- end of WBFS data --");
	}

	mi = InsertMemMap(&mm,hd_total,0);
	StringCopyS(mi->info,sizeof(mi->info),"-- end of WBFS device/file --");

	fprintf(f,"\f\n%*sWBFS Memory Map:\n\n", indent,"" );
	PrintMemMap(&mm,f,indent+1,0);

	fputc('\n',f);
    }
    ResetMemMap(&mm);

    if ( show_mode & SHOW_USAGE )
    {
	fprintf(f,"\f\n%*sWBFS Memory Usage:\n\n", indent,"" );
	wbfs_print_block_usage(stdout,indent+1,w,false);
	fputc('\n',f);
    }

    if ( show_mode & SHOW_CHECK ) // && isatty(fileno(f))
    {
	CheckWBFS_t ck;
	InitializeCheckWBFS(&ck);
	if (CheckWBFS(&ck,wbfs,prt_sections,0,f,1))
	    fprintf(f,"   -> use command \"CHECK -vv\" for a verbose report!\n\n");
	ResetCheckWBFS(&ck);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

int ScanOptWbfsAlloc ( ccp arg )
{
    static const KeywordTab_t tab[] =
    {
	{ WBFS_BA_AUTO,		"AUTO",		"DEFAULT",	0 },
	{ WBFS_BA_FIRST,	"FIRST",	"FRAG",		0 },
	{ WBFS_BA_AVOID_FRAG,	"NO-FRAG",	"NOFRAG",	0 },
	{0,0,0,0}
    };
    
    const KeywordTab_t * cmd = ScanKeyword(0,arg,tab);
    if (cmd)
    {
	opt_wbfs_alloc = cmd->id;
	return 0;
    }

    ERROR0(ERR_SYNTAX,"Illegal WBFS block allocation mode (option --wbfs-allocv): '%s'\n",arg);
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                   Analyze WBFS                  ///////////////
///////////////////////////////////////////////////////////////////////////////

const size_t AW_BUF_SIZE = 16 * MiB;

///////////////////////////////////////////////////////////////////////////////

AWRecord_t * AW_get_record ( AWData_t * awd )
{
    ASSERT(awd);

    static AWRecord_t dummy = {0};

    AWRecord_t * r = awd && awd->n_record < AW_MAX_RECORD
			? awd->rec + awd->n_record++
			: &dummy;
    memset(r,0,sizeof(*r));
    return r;
}

///////////////////////////////////////////////////////////////////////////////

static void AW_header ( AWData_t * awd, WFile_t * f )
{
    TRACE("AW_header(%p,%p)\n",awd,f);
    ASSERT(awd);
    ASSERT(f);

    if ( !f || f->st.st_size < sizeof(wbfs_head_t) )
	return;

    wbfs_head_t wh;
    if ( ReadAtF(f,0,&wh,sizeof(wh)) == ERR_OK
	&& !memcmp((ccp)&wh.magic,"WBFS",4) )
    {
	AWRecord_t * r = AW_get_record(awd);
	r->status		= AW_HEADER;
	r->magic_found	= true;
	r->wbfs_version	= wh.wbfs_version;
	r->hd_sectors	= ntohl(wh.n_hd_sec);
	r->hd_sector_size	= 1 << wh.hd_sec_sz_s;
	r->wbfs_sector_size	= 1 << wh.wbfs_sec_sz_s;
	StringCopyS(r->title,sizeof(r->title),"HEADER:");
	StringCopyS(r->info,sizeof(r->info),"WBFS header scanning");
    }
}

///////////////////////////////////////////////////////////////////////////////

typedef struct aw_inode_t
{
	be32_t	n_hd_sec;
	u8	hd_sec_sz_s;
	u8	wbfs_sec_sz_s;
	u8	wbfs_version;
	u16	count;
	time_t	xtime;

} aw_inode_t;

//-----------------------------------------------------------------------------

static AWRecord_t * AW_insert_inode
	( AWData_t * awd, aw_inode_t * inode, ccp head )
{
    ASSERT(awd);
    ASSERT(inode);
    ASSERT(head);

    if (!inode->count)
	return 0;

    AWRecord_t * r = AW_get_record(awd);

    r->status		= AW_INODES;
    r->magic_found	= true;
    r->wbfs_version	= inode->wbfs_version;
    r->hd_sectors	= ntohl(inode->n_hd_sec);
    r->hd_sector_size	= 1 << inode->hd_sec_sz_s;
    r->wbfs_sector_size	= 1 << inode->wbfs_sec_sz_s;

    snprintf(r->title,sizeof(r->title),"INODE-%s:",head);
    
    if (inode->xtime)
    {
	struct tm * tm = localtime(&inode->xtime);
	char timbuf[40];
	strftime(timbuf,sizeof(timbuf),"%F %T",tm);
	snprintf(r->info,sizeof(r->info),"%s n=%u", timbuf, inode->count );
    }
    else
	snprintf(r->info,sizeof(r->info),"n=%u", inode->count );

    inode->count = 0;
    inode->xtime = 0;

    return r;
}

//-----------------------------------------------------------------------------

static void AW_inodes ( AWData_t * awd, WFile_t * f, ccp data )
{
    TRACE("AW_inodes(%p,%p,%p)\n",awd,f,data);
    ASSERT(awd);
    ASSERT(f);
    ASSERT(data);

    //----- collect data

    const int inode_size = 20;
    aw_inode_t inode_tab[inode_size];
    aw_inode_t *inode_end = inode_tab + inode_size, *iptr;
    memset(inode_tab,0,sizeof(inode_tab));

    TRACE_SIZEOF(aw_inode_t);
    TRACE_SIZEOF(inode_tab);

    int max_count = 0;
    time_t max_time = 0;

    ccp ptr = data + 512 + WBFS_INODE_INFO_OFF;
    ccp end = data + AW_BUF_SIZE;
    for ( ; ptr < end; ptr += 512 )
    {
	TRACELINE;
	wbfs_inode_info_t * iinfo = (wbfs_inode_info_t*)ptr;
	if (!memcmp((ccp)&iinfo->magic,"WBFS",4))
	{
	    const u32 hd_sec_size   = 1 << iinfo->hd_sec_sz_s;
	    const u32 wbfs_sec_size = 1 << iinfo->wbfs_sec_sz_s;
	    if (   hd_sec_size	>= HD_SECTOR_SIZE
		&& hd_sec_size	<  wbfs_sec_size
		&& ptr-data	<  wbfs_sec_size )
	    {
		TRACE("INODE FOUND: %8zx\n",ptr-data);
		for ( iptr = inode_tab; iptr < inode_end; iptr++ )
		{
		    if ( !iptr->count
			|| iptr->n_hd_sec		== iinfo->n_hd_sec
			    && iptr->hd_sec_sz_s	== iinfo->hd_sec_sz_s
			    && iptr->wbfs_sec_sz_s	== iinfo->wbfs_sec_sz_s )
		    {
			iptr->count++;
			if ( max_count < iptr->count )
			    max_count = iptr->count;

			iptr->n_hd_sec	= iinfo->n_hd_sec;
			iptr->hd_sec_sz_s	= iinfo->hd_sec_sz_s;
			iptr->wbfs_sec_sz_s	= iinfo->wbfs_sec_sz_s;
			if ( iptr->wbfs_version < iinfo->wbfs_version )
			     iptr->wbfs_version = iinfo->wbfs_version;

			time_t temp;
			temp = ntoh64(iinfo->itime);
			if ( iptr->xtime < temp )
			     iptr->xtime = temp;
			temp = ntoh64(iinfo->mtime);
			if ( iptr->xtime < temp )
			     iptr->xtime = temp;
			temp = ntoh64(iinfo->ctime);
			if ( iptr->xtime < temp )
			     iptr->xtime = temp;
			temp = ntoh64(iinfo->atime);
			if ( iptr->xtime < temp )
			     iptr->xtime = temp;
			temp = ntoh64(iinfo->dtime);
			if ( iptr->xtime < temp )
			     iptr->xtime = temp;
			if ( max_time < iptr->xtime )
			    max_time = iptr->xtime;
			break;
		    }
		}
	    }
	}
    }

    //----- find newest inode
    
    if (max_time)
	for ( iptr = inode_tab; iptr < inode_end; iptr++ )
	    if ( iptr->xtime == max_time )
		AW_insert_inode(awd,iptr,"TIM");

    //----- find most frequently inodes

    if (max_count)
    {
	max_count /= 4;
	for ( iptr = inode_tab; iptr < inode_end; iptr++ )
	    if ( iptr->count >= max_count )
		AW_insert_inode(awd,iptr,"CNT");
    }

    //----- add very first inode

    AW_insert_inode(awd,inode_tab,"1ST");
}

///////////////////////////////////////////////////////////////////////////////

static void AW_discs ( AWData_t * awd, WFile_t * f, ccp data )
{
    TRACE("AW_discs(%p,%p,%p)\n",awd,f,data);
    ASSERT(awd);
    ASSERT(f);
    ASSERT(data);

    const size_t SEC_SIZE  = HD_SECTOR_SIZE;
    const size_t SEC_MAX   = 501;
    const size_t SEC_LEVEL =   4;
    ASSERT( SEC_SIZE * SEC_MAX <= AW_BUF_SIZE );

    const size_t WSS_MIN_LEVEL = 20;
    const size_t WSS_MAX_LEVEL = 28;
    
    // helper vars
    int sec, level, total_count = 0;
    
    // hss vars
    int first_sec_found = 0, max_level = 0;
    u32 sec_count[SEC_LEVEL];
    memset(sec_count,0,sizeof(sec_count));

    // wss vars
    u32 wss = 0;
    int wss_count[WSS_MAX_LEVEL], max_wss_count = 0;
    memset(wss_count,0,sizeof(wss_count));
    char buf[SEC_SIZE];
    
    for ( sec = 1; sec <= SEC_MAX; sec++ )
    {
	wd_header_t *wd = (wd_header_t *)( data + sec * SEC_SIZE );
	u32 magic = ntohl(wd->wii_magic);
	if ( ( magic == WII_MAGIC || magic == WII_MAGIC_DELETED )
		&& CheckID6(wd,false,false) )
	{
	    noTRACE(" - DISC found @ sector #%u\n",sec);
	    for ( level = SEC_LEVEL-1; level >= 0; level-- )
	    {
		const int hss = 1 << level;
		if ( sec >= hss && !(sec-hss & hss-1) )
		{
		    total_count++;
		    TRACE(" - DISC found @ sector #%u -> level=%u\n",sec,level);
		    sec_count[level]++;
		    if (!first_sec_found)
		    {
			first_sec_found = sec;
			//first_iso_sect  = ntohs(*(u16*)(wd+1));
			max_level = level;
		    }
		    break;
		}
	    }

	    if (!wss)
	    {
		// try to calculate wss

		const size_t WSS_CMP_SIZE = 128;
		wd_header_t inode;
		memcpy( &inode, wd, WSS_CMP_SIZE );
		inode.wii_magic = htonl(WII_MAGIC);
		off_t iso_sect = ntohs(*(u16*)(wd+1));

		for ( level = WSS_MIN_LEVEL; level < WSS_MAX_LEVEL; level++ )
		{
		    const off_t off = iso_sect << level;
		    if ( f->st.st_size < off + SEC_SIZE || ReadAtF(f,off,buf,sizeof(buf)) )
			break;

		    if (!memcmp(buf,&inode,WSS_CMP_SIZE))
		    {
			wss_count[level]++;
			if ( max_wss_count < wss_count[level] )
			{
			    max_wss_count = wss_count[level];
			    if ( max_wss_count >= 3 )
			    {
			  	wss = 1 << level;
				break;
			    }
			}
		    }
		}
	    }
	}
    }

    if (!total_count)
	return; // no disc found

    if (!wss)
    {
	if (max_wss_count)
	    for ( level = WSS_MIN_LEVEL; level < WSS_MAX_LEVEL; level++ )
		if ( wss_count[level] == max_wss_count )
		{
		    wss = 1 << level;
		    break;
		}

	if (!wss)
	    return;
    }

    int hd_sec_level = 0;
    if ( max_level > 0 )
    {
	for ( level = 0; level < SEC_LEVEL-1; level++ )
	    if ( sec_count[level] && sec_count[level] >= sec_count[level+1] )
		break;
	hd_sec_level = level;
    }
    const u32 hss = SEC_SIZE << hd_sec_level;
    TRACE(" - NH=%llx, HSS=%x, WSS=%x\n",(u64)f->st.st_size/hss,hss,wss);

    AWRecord_t * r = AW_get_record(awd);
    r->status		= AW_DISCS;
    r->magic_found	= false;
    r->wbfs_version	= 0;
    r->hd_sectors	= f->st.st_size/hss;
    r->hd_sector_size	= hss;
    r->wbfs_sector_size	= wss;
    StringCopyS(r->title,sizeof(r->title),"DISCS:");
    snprintf(r->info,sizeof(r->info),"%u disc header found",total_count);
}

///////////////////////////////////////////////////////////////////////////////

static void AW_calc ( AWData_t * awd, WFile_t * f, u32 sec_size, bool old )
{
    TRACE("AW_calc(%p,%p,%u,%d)\n",awd,f,sec_size,old);
    ASSERT(awd);
    ASSERT(f);

    if ( !f || f->st.st_size < sizeof(wbfs_head_t) )
	return;
 
    const int version	= old ? 0 : WBFS_VERSION;
    const u64 num_sec	= f->st.st_size / sec_size;
    const u32 n_wii_sec	= old
			    ? ( num_sec / WII_SECTOR_SIZE ) * sec_size
			    : num_sec / ( WII_SECTOR_SIZE / sec_size );

    int sz_s;
    for ( sz_s = 6; sz_s < 12; sz_s++ )
	if ( n_wii_sec < (1<<16) * (1<<sz_s) )
	    break;
    const u32 wbfs_sec_size = ( 1 << sz_s ) * WII_SECTOR_SIZE;

    AWRecord_t * r;
    char title[sizeof(r->title)];
    ccp info;

    if (old)
    {
	// search AW_CALC with same geometry, if found => abort
	int i;
	for ( i = 0; i < awd->n_record; i++ )
	{
	    r = awd->rec + i;
	    if ( r->status == AW_CALC
		&& r->hd_sectors	== num_sec
		&& r->wbfs_sector_size	== wbfs_sec_size )
	    {
		return;
	    }
	}

	snprintf(title,sizeof(title),"OLD %4u:",sec_size);
	info = "old and buggy calculation";
    }
    else
    {
	snprintf(title,sizeof(title),"CALC %4u:",sec_size);
	info = "calculation of init function";
    }
    
    r = AW_get_record(awd);
    r->status		= old ? AW_OLD_CALC : AW_CALC;
    r->magic_found	= false;
    r->wbfs_version	= version;
    r->hd_sectors	= num_sec;
    r->hd_sector_size	= sec_size;
    r->wbfs_sector_size	= wbfs_sec_size;
    StringCopyS(r->title,sizeof(r->title),title);
    StringCopyS(r->info,sizeof(r->info),info);
}

///////////////////////////////////////////////////////////////////////////////

int AnalyzeWBFS ( AWData_t * awd, WFile_t * f )
{
    ASSERT(awd);
    ASSERT(f);
    ASSERT( AW_NONE == 0 );

    const bool disable_errors = f->disable_errors;
    f->disable_errors = true;

    memset(awd,0,sizeof(*awd));
    TRACE("AnalyzeWBFS(%p,%p)\n",awd,f);

    //----- scan wbfs header
    
    AW_header(awd,f);

    //----- scan disc infos

    if ( f->st.st_size >= AW_BUF_SIZE )
    {
	char * data = MALLOC(AW_BUF_SIZE);
	if ( ReadAtF(f,0,data,AW_BUF_SIZE) == ERR_OK )
	{
	    AW_inodes(awd,f,data);
	    AW_discs(awd,f,data);
	}
	FREE(data);
    }

    //----- calculate geometry

    int sec_size;
    for ( sec_size = 512; sec_size <= 4096; sec_size <<= 1 )
	AW_calc(awd,f,sec_size,false);
    for ( sec_size = 512; sec_size <= 4096; sec_size <<= 1 )
	AW_calc(awd,f,sec_size,true);


    //----- calculate dependencies and remove invalid data

    int i;
    AWRecord_t * dest = awd->rec;
    for ( i = 0; i < awd->n_record; i++ )
    {
	AWRecord_t * r = awd->rec + i;

	wbfs_t wbfs;
	memset(&wbfs,0,sizeof(wbfs));
	wbfs_calc_geometry( &wbfs,
			    r->hd_sectors,
			    r->hd_sector_size,
			    r->wbfs_sector_size );
	if ( wbfs.n_wbfs_sec > 1 )
	{
	    r->wbfs_sectors	= wbfs.n_wbfs_sec;
	    r->max_disc		= wbfs.max_disc;
	    r->disc_info_size	= wbfs.disc_info_sz;
	    memcpy(dest,r,sizeof(*dest));
	    dest++;
	}
    }
    awd->n_record = dest - awd->rec;

    //----- finish operation

 #ifdef DEBUG
    PrintAnalyzeWBFS(TRACE_FILE,0,awd,2);
 #endif

    f->disable_errors = disable_errors;
    return awd->n_record;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int PrintAnalyzeWBFS
(
    FILE		* out,		// valid output stream
    int			indent,		// indent of output
    AWData_t		* awd,		// valid pointer
    int			print_calc	// 0: suppress calculated values
					// 1: print calculated values if other values available
					// 2: print calculated values
)
{
    DASSERT(out);
    DASSERT(awd);
    
    if ( !out || !awd->n_record )
	return 0;
    if ( print_calc < 2 && ( awd->rec->status == AW_CALC || awd->rec->status == AW_OLD_CALC ))
	return 0;

    indent = NormalizeIndent(indent);

    char buf[30];
    static const char sep[] =
		"------------------------------------------"
		"------------------------------------------";

    fprintf(out,"%*s%s\n"
		"%*s                    HD SECTORS  WBFS SECTORS   DISCS       (all values in hex)\n"
		"%*s           WBFS     total  sec  total    sec  max inode\n"
		"%*sNAME    magic vrs     num size   num    size  num size  ADDITIONAL INFORMATION\n"
		"%*s%s\n",
		indent,"", sep, indent,"", indent,"", indent,"", indent,"", sep );

    int i;
    for ( i = 0; i < awd->n_record; i++ )
    {
	AWRecord_t * ar = awd->rec + i;
	const bool is_calc =  ar->status == AW_CALC || ar->status == AW_OLD_CALC;
	if ( !print_calc && is_calc )
	    continue;
	
	ccp info = ar->info;
	if ( i > 0 && ar->status == ar[-1].status && is_calc )
	{
	    snprintf(buf,sizeof(buf),
			" \" but sector-size=%u",ar->hd_sector_size);
	    info = buf;
	}

	fprintf(out,"%*s%-10s %s %2x %8x %4x %5x %7x %4x %4x  %s\n",
		indent,"", ar->title,
		ar->magic_found ? "ok" : " -",
		ar->wbfs_version,
		ar->hd_sectors, ar->hd_sector_size,
		ar->wbfs_sectors, ar->wbfs_sector_size,
		ar->max_disc, ar->disc_info_size,
		info );
    }

    fprintf(out,"%*s%s\n", indent,"",sep);
    return awd->n_record;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     Check WBFS                  ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeCheckWBFS ( CheckWBFS_t * ck )
{
    ASSERT(ck);
    memset(ck,0,sizeof(*ck));
}

///////////////////////////////////////////////////////////////////////////////

void ResetCheckWBFS ( CheckWBFS_t * ck )
{
    ASSERT(ck);
    FREE(ck->cur_fbt);
    FREE(ck->good_fbt);
    FREE(ck->ubl);
    FREE(ck->blc);
    FREE(ck->disc);
    InitializeCheckWBFS(ck);
}

///////////////////////////////////////////////////////////////////////////////

#if defined(TEST)

static void check_func
(
    wbfs_t		* p,		// valid WBFS descriptor
    wbfs_check_t	check_mode,	// modus
    int			slot,		// -1 or related slot index
    ccp			id6,		// NULL or pointer to disc ID6
    int			block,		// -1 or related block number
    uint		count,		// block usage count
    ccp			msg,		// clear text message
    uint		msg_len,	// strlen(msg)
    void		* param		// user defined parameter
)
{
    printf("%2u: %3d [%s] %d,%d %s\n",check_mode,slot,id6?id6:"-",block,count,msg);
}

#endif

///////////////////////////////////////////////////////////////////////////////

enumError CheckWBFS ( CheckWBFS_t * ck, WBFS_t * wbfs,
			int prt_sections, int verbose, FILE * f, int indent )
{
    ASSERT(ck);
    ASSERT(wbfs);

    enum { SHOW_SUM, SHOW_DETAILS, PRINT_DUMP, PRINT_EXT_DUMP, PRINT_FULL_DUMP };

    ResetCheckWBFS(ck);
    if ( !wbfs || !wbfs->wbfs || !wbfs->sf || !IsOpenSF(wbfs->sf) )
	return ERROR0(ERR_INTERNAL,0);

    ck->wbfs = wbfs;
    wbfs_t * w = wbfs->wbfs;

    if (!f)
	verbose = -1;
    indent = NormalizeIndent(indent);

    ck->fbt_off  = ( w->part_lba + w->freeblks_lba ) * w->hd_sec_sz;
    ck->fbt_size = w->freeblks_lba_count * w->hd_sec_sz;

 #if 0 && defined(TEST) // [[2do]]
    if (logging)
	wbfs_calc_used_blocks(w,true,true,check_func,ck);
 #endif

    //---------- calculate number of sectors

    const u32 N_SEC = w->n_wbfs_sec;

    u32 ALLOC_SEC = ck->fbt_size * 8;
    if ( ALLOC_SEC < N_SEC )
	ALLOC_SEC = N_SEC;

    u32 WBFS0_SEC = N_SEC;	// used sectors

    if ( w->head->wbfs_version == 0 )
    {
	// the old wbfs versions have calculation errors
	//  - because a rounding error 'n_wii_sec' is sometimes to short
	//	=> 'n_wbfs_sec' is to short
	//	=> free blocks table may be too small
	//  - block search uses only (n_wbfs_sec/32)*32 blocks

	// this is the old buggy calcualtion
	u32 n_wii_sec  = ( w->n_hd_sec / w->wii_sec_sz ) * w->hd_sec_sz;
	WBFS0_SEC = n_wii_sec >> ( w->wbfs_sec_sz_s - w->wii_sec_sz_s );

	WBFS0_SEC = ( WBFS0_SEC / 32 ) * 32 + 1;

	if ( WBFS0_SEC > N_SEC )
	     WBFS0_SEC = N_SEC;
    }
    TRACE("N-SEC=%u, WBFS0-SEC=%u, ALLOC-SEC=%u\n",N_SEC,WBFS0_SEC,ALLOC_SEC);

    //---------- alloctate data

    u32 * fbt  = MALLOC(ck->fbt_size);
    u8  * ubl  = CALLOC(ALLOC_SEC,1);
    u8  * blc  = CALLOC(ALLOC_SEC,1);
    CheckDisc_t * disc = CALLOC(w->max_disc,sizeof(*disc));

    ck->cur_fbt	= fbt;
    ck->ubl	= ubl;
    ck->blc	= blc;
    ck->disc	= disc;

    //---------- load free blocks table

    enumError err = ReadAtF(&wbfs->sf->f,ck->fbt_off,fbt,ck->fbt_size);
    if (err)
	return err;

    int i, j, bl = 1;
    for ( i = 0; i < w->freeblks_size4; i++ )
    {
	u32 v = wbfs_ntohl(fbt[i]);
	if ( v == ~(u32)0 )
	    bl += 32;
	else
	    for ( j = 0; j < 32; j++, bl++ )
		if ( !(v & 1<<j) )
		    ubl[bl] = 1;
    }

    //---------- scan discs

    if (prt_sections)
	fprintf(f,
		"n-slots=%u\n"
		"n-disc=%u\n"
		,w->max_disc
		,wbfs->used_discs
		);
    else if ( verbose >= SHOW_DETAILS )
	fprintf(f,"%*s* Scan %d discs in %d slots.\n",
			indent,"", wbfs->used_discs, w->max_disc );
    u32 slot;
    for ( slot = 0; slot < w->max_disc; slot++ )
    {
	wbfs_disc_t * d = wbfs_open_disc_by_slot(w,slot,0);
	if (!d)
	    continue;

	u16 * wlba_tab = d->header->wlba_table;
	ASSERT(wlba_tab);

	int bl;
	for ( bl = 0; bl < w->n_wbfs_sec_per_disc; bl++ )
	{
	    const u32 wlba = ntohs(wlba_tab[bl]);
	    if ( wlba > 0 && wlba < N_SEC && blc[wlba] < 255 )
		blc[wlba]++;
	}

	wbfs_close_disc(d);
    }

    //---------- check for disc errors

    if ( !prt_sections && verbose >= SHOW_DETAILS )
	fprintf(f,"%*s* Check for disc block errors.\n", indent,"" );

    u32 total_err_invalid   = 0;
    u32 total_err_no_blocks = 0;
    u32 invalid_disc_count  = 0;
    u32 no_iinfo_count      = 0;

    if (prt_sections)
	fprintf(f,
	    "column-header=           slot _id6__ index wbfs-block count\n");

    for ( slot = 0; slot < w->max_disc; slot++ )
    {
	wbfs_disc_t * d = wbfs_open_disc_by_slot(w,slot,0);
	if (!d)
	    continue;

	CheckDisc_t * g = disc + slot;
	memcpy(g->id6,d->header->dhead,6);

	wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(d,0);
	if (!wbfs_is_inode_info_valid(wbfs->wbfs,iinfo))
	{
	    g->no_iinfo_count = 1;
	    no_iinfo_count++;
	}

	if (!d->is_valid)
	{
	    wbfs_close_disc(d);
	    continue;
	}

	u16 * wlba_tab = d->header->wlba_table;
	ASSERT(wlba_tab);

	int bl;
	u32 invalid_game = 0, block_count = 0;
	for ( bl = 0; bl < w->n_wbfs_sec_per_disc; bl++ )
	{
	    const u32 wlba = ntohs(wlba_tab[bl]);
 #ifdef TEST0
	    if ( wlba >= N_SEC || wlba == 5 )
 #else
	    if ( wlba >= N_SEC )
 #endif
	    {
		invalid_game = 1;
		g->err_count++;
		g->bl_invalid++;
		total_err_invalid++;

		if (prt_sections)
		    fprintf(f,"err-invalid-block-ref=%7u %s %5u %7u\n"
			,slot, wd_print_id(g->id6,6,0), bl, wlba
			);
		else if ( verbose >= SHOW_DETAILS )
		    fprintf(f,"%*s  - #%d=%s/%u: invalid WBFS block #%u for block!\n",
				indent,"", slot, g->id6, bl, wlba );
	    }
	    else if ( wlba )
	    {
		block_count++;
		if ( !ubl[wlba] )
		{
		    invalid_game = 1;
		    g->err_count++;
		    g->bl_fbt++;
		    if (prt_sections)
			fprintf(f,"err-block-marked-free=%7u %s %5u %7u\n"
				,slot, wd_print_id(g->id6,6,0), bl, wlba
				);
		    else if ( verbose >= SHOW_DETAILS )
			fprintf(f,"%*s  - #%d=%s/%u: WBFS block #%u marked as free!\n",
				    indent,"", slot, g->id6, bl, wlba );
		}

		if ( blc[wlba] > 1 )
		{
		    invalid_game = 1;
		    g->err_count++;
		    g->bl_overlap++;
		    if (prt_sections)
			fprintf(f,"err-block-multi-usage=%7u %s %5u %7u %7u\n"
				,slot, wd_print_id(g->id6,6,0), bl, wlba, blc[wlba]
				);
		    else if ( verbose >= SHOW_DETAILS )
			fprintf(f,"%*s  - #%d=%s/%u: WBFS block #%u is used %u times!\n",
				    indent,"", slot, g->id6, bl, wlba, blc[wlba] );
		}
	    }
	}

#ifdef TEST0
	if (!block_count || slot == 2 )
#else    
	if (!block_count)
#endif
	{
	    invalid_game = 1;
	    g->no_blocks = 1;
	    g->err_count++;
	    total_err_no_blocks++;
	    if (prt_sections)
		fprintf(f,"err-disc-without-blocks=%5u %s\n"
			,slot, wd_print_id(g->id6,6,0)
			);
	    else if ( verbose >= SHOW_DETAILS )
		fprintf(f,"%*s  - #%d=%s: no valid WBFS block!\n", indent,"", slot, g->id6 );
	}

	if (invalid_game)
	{
	    invalid_disc_count += invalid_game;
	    ASSERT(w->head);

	    // we mark it, but don't set the dirty flag
	    //  ==> the marker is only written on an external sync()
	    w->head->disc_table[slot] |= WBFS_SLOT_INVALID;
	}

	wbfs_close_disc(d);
    }

    //---------- check free blocks table.

    if ( verbose >= SHOW_DETAILS && !prt_sections )
	fprintf(f,"%*s* Check free blocks table.\n", indent,"" );

    u32 total_err_overlap  = 0;
    for ( bl = 0; bl < N_SEC; bl++ )
	if ( blc[bl] > 1 )
	    total_err_overlap++;

    u32 total_err_fbt_used = 0;
    u32 total_err_fbt_free = 0;
    u32 total_err_fbt_free_wbfs0 = 0;

    for ( bl = 1; bl < N_SEC; )
    {
	if ( blc[bl] && !ubl[bl] )
	{
	    const int start_bl = bl;
	    while ( bl < N_SEC && blc[bl] && !ubl[bl] )
		bl++;
	    const int count = bl - start_bl;
	    total_err_fbt_used += count;
	    if ( verbose >= SHOW_DETAILS && !prt_sections )
	    {
		if ( count > 1 )
		    fprintf(f,"%*s  - %d used WBFS sectors #%u .. #%u marked as 'free'!\n",
				indent,"", count, start_bl, bl-1 );
		else
		    fprintf(f,"%*s  - Used WBFS sector #%u marked as 'free'!\n",
				indent,"", bl );
	    }
	}
	else if ( !blc[bl] && ubl[bl] )
	{
	    const int start_bl = bl;
	    while ( bl < N_SEC && !blc[bl] && ubl[bl] )
		bl++;
	    const int count = bl - start_bl;
	    total_err_fbt_free += count;

	    int wbfs0_count = 0;
	    if ( bl > WBFS0_SEC )
	    {
		const int max = bl - WBFS0_SEC;
		wbfs0_count = count < max ? count : max;
	    }

	    if ( verbose >= SHOW_DETAILS && !prt_sections )
	    {
		if ( count > 1 )
		    fprintf(f,"%*s  - %d free WBFS sectors #%u .. #%u marked as 'used'!\n",
				indent,"", count, start_bl, bl-1 );
		else
		    fprintf(f,"%*s  - Free WBFS sector #%u marked as 'used'!\n",
				indent,"", bl );
		if ( wbfs0_count && !total_err_fbt_free_wbfs0 )
		    fprintf(f,"%*sNote: Free sectors >= #%u are marked 'used'"
				" because a bug in libwbfs v0.\n",
				indent+6,"", WBFS0_SEC );
	    }

	    total_err_fbt_free_wbfs0 += wbfs0_count;
	}
	else
	    bl++;
    }


    //---------- summary

    ck->err_fbt_used		= total_err_fbt_used;
    ck->err_fbt_free		= total_err_fbt_free;
    ck->err_fbt_free_wbfs0	= total_err_fbt_free_wbfs0;
    ck->err_no_blocks		= total_err_no_blocks;
    ck->err_bl_overlap		= total_err_overlap;
    ck->err_bl_invalid		= total_err_invalid;
    ck->no_iinfo_count		= no_iinfo_count;
    ck->invalid_disc_count	= invalid_disc_count;

    ck->err_total = total_err_fbt_used
		  + total_err_fbt_free
		  + total_err_no_blocks
		  + total_err_overlap
		  + total_err_invalid;

    // with the new wbfs interface all errors are harmless
    ck->err = ck->err_total ? ERR_WARNING : ERR_OK;

    if ( ck->err_total && verbose >= PRINT_DUMP )
    {
	printf("\f\nWBFS DUMP:\n\n");
	ShowMode show_mode = SHOW__ALL & ~(SHOW_USAGE|SHOW_CHECK);
	if ( verbose < PRINT_FULL_DUMP )
	    show_mode &= ~SHOW_W_MAP;
	if (prt_sections)
	    show_mode |= SHOW_F_SECTIONS;
	DumpWBFS(wbfs,f,indent,show_mode,0,
		verbose >= PRINT_FULL_DUMP,
		verbose >= PRINT_EXT_DUMP  ? 0 : ck );
    }

    if ( ck->err_total && verbose >= SHOW_SUM || verbose >= SHOW_DETAILS )
    {
	if (!prt_sections)
	    printf("\f\n");
	PrintCheckedWBFS(ck,f,indent,prt_sections);
    }

    return ck->err;
}

///////////////////////////////////////////////////////////////////////////////

enumError AutoCheckWBFS	( WBFS_t * wbfs, bool ignore_check, int indent, int prt_sections )
{
    ASSERT(wbfs);
    ASSERT(wbfs->wbfs);

    CheckWBFS_t ck;
    InitializeCheckWBFS(&ck);
    enumError err = CheckWBFS(&ck,wbfs,0,-1,0,0);
    if (err)
    {
	PrintCheckedWBFS(&ck,stdout,indent,prt_sections);
	if ( !ignore_check && err > ERR_WARNING )
	    printf("!>> To avoid this automatic check use the option --no-check.\n"
		   "!>> To ignore the results of this check use option --force.\n"
		   "\n" );
    }
    ResetCheckWBFS(&ck);
    return ignore_check ? ERR_OK : err;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintCheckedWBFS ( CheckWBFS_t * ck, FILE * f, int indent, int prt_sections )
{
    ASSERT(ck);
    if ( !ck->wbfs || !ck->cur_fbt || !f )
	return ERR_OK;

    indent = NormalizeIndent(indent);

    if ( prt_sections )
    {
	fprintf(f,
		"\n"
		"[check:summary]\n"
		"err-blocks-marked-free=%u\n"
		"err-blocks-marked-used=%u\n"
		"err-blocks-multi-usage=%u\n"
		"err-blocks-invalid-reference=%u\n"
		"err-discs-without-blocks=%u\n"
		"total-errors=%u\n"
		"total-invalid-discs=%u\n"
		,ck->err_fbt_used
		,ck->err_fbt_free
		,ck->err_bl_overlap
		,ck->err_bl_invalid
		,ck->err_no_blocks
		,ck->err_total
		,ck->invalid_disc_count
		);
    }
    else if ( ck->err_total )
    {
	fprintf(f,"%*s* Summary of WBFS Check: %u error%s found:\n",
		indent,"", ck->err_total, ck->err_total == 1 ? "" : "s" );
	if (ck->err_fbt_used)
	    fprintf(f,"%*s%5u used WBFS sector%s marked as free!\n",
		indent,"", ck->err_fbt_used, ck->err_fbt_used == 1 ? "" : "s" );
	if (ck->err_fbt_free)
	{
	    fprintf(f,"%*s%5u free WBFS sector%s marked as used!\n",
		indent,"", ck->err_fbt_free, ck->err_fbt_free == 1 ? "" : "s" );
	    if (ck->err_fbt_free_wbfs0)
		fprintf(f,"%*sNote: %u error%s are based on a bug in libwbfs v0.\n",
		    indent+6,"", ck->err_fbt_free_wbfs0,
		    ck->err_fbt_free_wbfs0 == 1 ? "" : "s" );
	}
	if (ck->err_bl_overlap)
	    fprintf(f,"%*s%5u WBFS sector%s are used by 2 or more discs!\n",
		indent,"", ck->err_bl_overlap, ck->err_bl_overlap == 1 ? "" : "s" );
	if (ck->err_bl_invalid)
	    fprintf(f,"%*s%5u invalid WBFS block reference%s found!\n",
		indent,"", ck->err_bl_invalid, ck->err_bl_invalid == 1 ? "" : "s" );
	if (ck->err_no_blocks)
	    fprintf(f,"%*s%5u disc%s no valid WBFS blocks!\n",
		indent,"", ck->err_no_blocks, ck->err_no_blocks == 1 ? " has" : "s have" );
	if (ck->invalid_disc_count)
	    fprintf(f,"%*s  Total: %u disc%s invalid!\n",
		indent,"", ck->invalid_disc_count,
		ck->invalid_disc_count == 1 ? " is" : "s are" );
	fputc('\n',f);
    }
    else
	fprintf(f,"%*s* Summary of WBFS check: no errors found.\n", indent,"" );

    return ck->err_total ? ERR_WBFS_INVALID : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError RepairWBFS ( CheckWBFS_t * ck, int testmode,
	RepairMode rm, int verbose, FILE * f, int indent )
{
    TRACE("RepairWBFS(%p,%d,%x,%d,%p,%d)\n",
		ck, testmode, rm, verbose, f, indent );

    ASSERT(ck);
    ASSERT(ck->wbfs);
    ASSERT(ck->wbfs->sf);
    ASSERT(ck->cur_fbt);

    wbfs_t * w = ck->wbfs->wbfs;
    ASSERT(w);

    u32 repair_count = 0, sync = 0;
    if (!f)
	verbose = -1;
    else if ( testmode && verbose < 0 )
	verbose = 0;

    TRACELINE;
    if ( rm & REPAIR_RM_ALL )
    {
	int slot;
	for ( slot = 0; slot < w->max_disc; slot++ )
	{
	    CheckDisc_t * disc = ck->disc + slot;
	    // [dt] norm with WBFS_SLOT__MASK
	    if ( w->head->disc_table[slot]
		&& (   rm & REPAIR_RM_INVALID && disc->bl_invalid
		    || rm & REPAIR_RM_OVERLAP && disc->bl_overlap
		    || rm & REPAIR_RM_FREE    && disc->bl_fbt
		    || rm & REPAIR_RM_EMPTY   && disc->no_blocks
		   ))
	    {
		if ( verbose >= 0 )
		{
		    ccp title = GetTitle(disc->id6,0);
		    fprintf(f,"%*s* %sDrop disc at slot #%u, id=%s%s%s\n",
			indent,"", testmode ? "WOULD " : "", slot, disc->id6,
			title ? ", " : "", title ? title : "" );
		}

		if (!testmode)
		{
		    w->head->disc_table[slot] = WBFS_SLOT_FREE;
		    sync++;
		}
		repair_count++;
	    }
	}
    }

    TRACELINE;
    if ( rm & REPAIR_FBT )
    {
	if ( CalcFBT(ck) )
	{
	    TRACELINE;
	    if ( verbose >= 0 )
		fprintf(f,"%*s* %sStore fixed 'free blocks table' (%zd bytes).\n",
		    indent,"", testmode ? "WOULD " : "", ck->fbt_size );

	    if (!testmode)
	    {
		TRACELINE;
		enumError err = WriteAtF( &ck->wbfs->sf->f,
					    ck->fbt_off, ck->good_fbt, ck->fbt_size );
		if (err)
		    return err;
		memcpy(ck->cur_fbt,ck->good_fbt,ck->fbt_size);
		sync++;
	    }
	    repair_count++;
	}

	TRACELINE;
	if ( w->head->wbfs_version < WBFS_VERSION )
	{
	    if ( verbose >= 0 )
		fprintf(f,"%*s* %sSet WBFS version from %u to %u.\n",
		    indent,"", testmode ? "WOULD " : "",
		    w->head->wbfs_version,  WBFS_VERSION );
	    if (!testmode)
	    {
		w->head->wbfs_version = WBFS_VERSION;
		sync++;
	    }
	    repair_count++;
	}
    }

    TRACELINE;
    if ( rm & REPAIR_INODES )
    {
	TRACELINE;
	int slot;
	for ( slot = 0; slot < w->max_disc; slot++ )
	{
	    wbfs_disc_t * d = wbfs_open_disc_by_slot(w,slot,1);
	    if (d)
	    {
		wbfs_touch_disc(d,0,0,0,0);
		wbfs_close_disc(d);
	    }
	}
    }

    TRACELINE;
    if (repair_count)
	wbfs_calc_used_blocks(w,true,false,0,0);
    if (sync)
	wbfs_sync(w);

    TRACELINE;
    if ( verbose >= 0 && repair_count )
	fputc('\n',f);

    TRACELINE;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CheckRepairWBFS ( WBFS_t * wbfs, int testmode,
	RepairMode rm, int verbose, FILE * f, int indent )
{
    ASSERT(wbfs);

    CheckWBFS_t ck;
    InitializeCheckWBFS(&ck);
    enumError err = CheckWBFS(&ck,wbfs,0,-1,0,0);
    if ( err == ERR_WARNING || err == ERR_WBFS_INVALID )
	err = RepairWBFS(&ck,testmode,rm,verbose,f,indent);
    ResetCheckWBFS(&ck);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError RepairFBT ( WBFS_t * w, int testmode, FILE * f, int indent )
{
    return CheckRepairWBFS(w,testmode,REPAIR_FBT,0,f,indent);
}

///////////////////////////////////////////////////////////////////////////////

bool CalcFBT ( CheckWBFS_t * ck )
{
    ASSERT(ck);
    if ( !ck->wbfs || !ck->wbfs->wbfs || !ck->cur_fbt )
	return ERROR0(ERR_INTERNAL,0);

    if (!ck->good_fbt)
	ck->good_fbt = MALLOC(ck->fbt_size);
    memset(ck->good_fbt,0,ck->fbt_size);

    const u32 MAX_BL = ck->wbfs->wbfs->n_wbfs_sec - 2;
    u32 * fbt = ck->good_fbt;
    int bl;
    for ( bl = 0; bl <= MAX_BL; bl++ )
	if (!ck->blc[bl+1])
	    fbt[bl/32] |= 1 << (bl&31);

    for ( bl = 0; bl < ck->fbt_size/4; bl++ )
	fbt[bl] = htonl(fbt[bl]);

    return memcmp(ck->cur_fbt,ck->good_fbt,ck->fbt_size);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   WDiscInfo_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// is WDiscInfo_t obsolete? [wiidisc] [[obsolete]]

void InitializeWDiscInfo ( WDiscInfo_t * dinfo )
{
    memset(dinfo,0,sizeof(*dinfo));
    dinfo->slot = -1;
}

///////////////////////////////////////////////////////////////////////////////

enumError ResetWDiscInfo ( WDiscInfo_t * dinfo )
{
    // nothing to do

    InitializeWDiscInfo(dinfo);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError GetWDiscInfo ( WBFS_t * w, WDiscInfo_t * dinfo, int disc_index )
{
    ASSERT(w);
    if ( !w || !w->wbfs || !dinfo )
	return ERROR0(ERR_INTERNAL,0);

    w->disc_slot = -1;

    u32 slot, size4;
    const enumError err = wbfs_get_disc_info ( w->wbfs, disc_index,
			    (u8*)&dinfo->dhead, sizeof(dinfo->dhead), &slot,
			    &dinfo->disc_type, &dinfo->disc_attrib,
			    &size4, &dinfo->wbfs_fragments );

    if (err)
    {
	memset(dinfo,0,sizeof(*dinfo));
	return err;
    }

    CalcWDiscInfo(dinfo,0);
    dinfo->disc_index	= disc_index;
    dinfo->slot		= slot;
    dinfo->size		= (u64)size4 * 4;
    dinfo->used_blocks	= dinfo->size / WII_SECTOR_SIZE; // [[2do]] not exact

    return dinfo->disc_type == WD_DT_UNKNOWN ? ERR_WARNING : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError GetWDiscInfoBySlot ( WBFS_t * w, WDiscInfo_t * dinfo, u32 disc_slot )
{
    ASSERT(w);
    if ( !w || !w->wbfs || !dinfo )
	return ERROR0(ERR_INTERNAL,0);

    w->disc_slot = -1;

    memset(dinfo,0,sizeof(*dinfo));

    u32 size4;
    const enumError err = wbfs_get_disc_info_by_slot ( w->wbfs, disc_slot,
			    (u8*)&dinfo->dhead, sizeof(dinfo->dhead),
			    &dinfo->disc_type, &dinfo->disc_attrib,
			    &size4, &dinfo->wbfs_fragments );

    if (err)
    {
	TRACE("GetWDiscInfoBySlot() err=%d, magic=%x\n",err,dinfo->dhead.wii_magic);
	return err;
    }

    CalcWDiscInfo(dinfo,0);
    dinfo->disc_index	= disc_slot;
    dinfo->slot		= disc_slot;
    dinfo->size		= (u64)size4 * 4;
    dinfo->used_blocks	= dinfo->size / WII_SECTOR_SIZE; // [[2do]] not exact

    w->disc_slot = disc_slot;
    return dinfo->disc_type == WD_DT_UNKNOWN ? ERR_WARNING : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError FindWDiscInfo ( WBFS_t * w, WDiscInfo_t * dinfo, ccp id6 )
{
    // [[2do]] the wbfs subsystem can find ids!

    ASSERT(w);
    ASSERT(dinfo);
    ASSERT(id6);
    if ( !w || !w->wbfs || !dinfo || !id6 )
	return ERROR0(ERR_INTERNAL,0);

    w->disc_slot = -1;

    int i;
    for ( i = 0; i < w->used_discs; i++ )
    {
	if ( GetWDiscInfo(w,dinfo,i) == ERR_OK
	    && !memcmp(id6,&dinfo->dhead.disc_id,6) )
		return ERR_OK;
    }
    return ERR_WDISC_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////

enumError LoadIsoHeader	( WBFS_t * w, wd_header_t * iso_header, wbfs_disc_t * disc )
{
    ASSERT(w);
    ASSERT(iso_header);

    if ( !w || !w->sf || !w->wbfs || !iso_header )
	return ERROR0(ERR_INTERNAL,0);

    memset(iso_header,0,sizeof(*iso_header));
    if (!disc)
    {
	disc = w->disc;
	if (!disc)
	    return ERR_OK;
    }

    u16 wlba = ntohs(disc->header->wlba_table[0]);
    if (!wlba)
	return ERR_OK;

    char buf[HD_SECTOR_SIZE]; // read whole hd sector
    enumError err = ReadSF(w->sf, wlba * (off_t)w->wbfs->wbfs_sec_sz,
			buf, sizeof(buf) );
    if (!err)
	memcpy(iso_header,buf,sizeof(*iso_header));
    return err;
}

///////////////////////////////////////////////////////////////////////////////

void CalcWDiscInfo ( WDiscInfo_t * dinfo, SuperFile_t * sf )
{
    ASSERT(dinfo);

    if (sf)
    {
	const wd_disc_t * disc = OpenDiscSF(sf,false,false);
	if (disc)
	{
	    memcpy(&dinfo->dhead,&disc->dhead,sizeof(dinfo->dhead));
	    dinfo->magic2	= disc->magic2;
	    dinfo->n_part	= disc->n_part;
	    dinfo->used_blocks	= CountUsedIsoBlocksSF(sf,&part_selector);

	    static char mask[] = "DUC?";
	    strcpy(dinfo->part_info,"----");
	    int ip;
	    for ( ip = 0; ip < disc->n_part; ip++ )
	    {
		u32 pt = disc->part[ip].part_type;
		if ( pt > sizeof(mask)-2 )
		     pt = sizeof(mask)-2;
		dinfo->part_info[pt] = mask[pt];
	    }
	}
	else
	    ReadSF(sf,0,&dinfo->dhead,sizeof(dinfo->dhead));
    }

    memcpy(dinfo->id6,&dinfo->dhead.disc_id,6);
    dinfo->id6[6]	= 0;
    dinfo->used_blocks	= 0;
    dinfo->disc_index	= 0;
    dinfo->size		= 0;
    dinfo->title	= GetTitle(dinfo->id6,0);
    dinfo->disc_type	= get_header_disc_type(&dinfo->dhead,&dinfo->disc_attrib);
}

///////////////////////////////////////////////////////////////////////////////

void CopyWDiscInfo ( WDiscListItem_t * item, WDiscInfo_t * dinfo )
{
    ASSERT(item);
    ASSERT(dinfo);

    memset(item,0,sizeof(*item));

    memcpy(item->id6,&dinfo->dhead.disc_id,6);
    item->size_mib = (u32)(( dinfo->size + MiB/2 ) / MiB );
    memcpy(item->name64,dinfo->dhead.disc_title,64);
    memcpy(item->part_info,dinfo->part_info,sizeof(item->part_info));
    item->title		= dinfo->title;
    item->n_part	= dinfo->n_part;
    item->wbfs_slot	= dinfo->disc_index;
    item->wbfs_fragments= dinfo->wbfs_fragments;
    item->ftype		= FT_ID_WBFS | FT_A_WDISC;
    item->used_blocks	= dinfo->used_blocks;

    CopyFileAttribDiscInfo(&item->fatt,dinfo);
}

///////////////////////////////////////////////////////////////////////////////

static void DumpTimestamp ( FILE * f, int indent, ccp head, u64 xtime, ccp comment )
{
    time_t tim = ntoh64(xtime);
    if (tim)
    {
	struct tm * tm = localtime(&tim);
	char timbuf[40];
	strftime(timbuf,sizeof(timbuf),"%F %T",tm);
	fprintf(f,"%*s%-12s%s  (%s)\n", indent, "", head, timbuf, comment );
    }
}

//-----------------------------------------------------------------------------

enumError DumpWDiscInfo
	( WDiscInfo_t * di, wd_header_t * ih, FILE * f, int indent )
{
    if ( !di || !f )
	return ERROR0(ERR_INTERNAL,0);

    indent = NormalizeIndent(indent);

    const u32 magic = ntohl(di->dhead.wii_magic);
    fprintf(f,"%*smagic:      %08x, %.64s\n", indent, "", magic,
		magic == WII_MAGIC ? " (=WII-DISC)"
			: magic == WII_MAGIC_DELETED ? " (=DELETED)" : "" );

    fprintf(f,"%*swbfs name:  %6s, %.64s\n",
		indent, "", &di->dhead.disc_id, di->dhead.disc_title );
    if (ih)
	fprintf(f,"%*siso name:   %6s, %.64s\n",
		indent, "", &ih->disc_id, ih->disc_title );
    if ( ih && strcmp(&di->dhead.disc_id,&ih->disc_id) )
    {
	if (di->title)
	    fprintf(f,"%*swbfs title: %s\n", indent, "", (ccp)di->title );
	ccp title = GetTitle(&ih->disc_id,0);
	if (title)
	    fprintf(f,"%*siso title:  %s\n", indent, "", (ccp)di->title );
    }
    else if (di->title)
	fprintf(f,"%*stitle:      %s\n", indent, "", (ccp)di->title );

    const RegionInfo_t * rinfo = GetRegionInfo(di->dhead.region_code);
    fprintf(f,"%*sregion:     %s [%s]\n", indent, "", rinfo->name, rinfo->name4 );

    if (di->size)
	fprintf(f,"%*ssize:       %lld MiB\n", indent, "", ( di->size + MiB/2 ) / MiB );

    DumpTimestamp(f,indent,"i-time:",di->dhead.iinfo.itime,"insertion time");
    DumpTimestamp(f,indent,"m-time:",di->dhead.iinfo.mtime,"last modification time");
    DumpTimestamp(f,indent,"c-time:",di->dhead.iinfo.ctime,"last status change time");
    DumpTimestamp(f,indent,"a-time:",di->dhead.iinfo.atime,"last access time");
    DumpTimestamp(f,indent,"d-time:",di->dhead.iinfo.dtime,"deletion time");

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

WDiscList_t * GenerateWDiscList ( WBFS_t * w, int part_index )
{
    ASSERT(w);
    if ( !w || !w->wbfs )
    {
	ERROR0(ERR_INTERNAL,0);
	return 0;
    }

    WDiscList_t * wlist = MALLOC(sizeof(WDiscList_t));
    wlist->first_disc = CALLOC(w->used_discs,sizeof(WDiscListItem_t));
    wlist->total_size_mib = 0;

    WDiscInfo_t dinfo;
    WDiscListItem_t * item = wlist->first_disc;

    int slot;
    for ( slot = 0; slot < w->total_discs; slot++ )
    {
	if ( GetWDiscInfoBySlot(w,&dinfo,slot) == ERR_OK )
	{
	    memcpy(item->id6,&dinfo.dhead.disc_id,6);
	    if (!IsExcluded(item->id6))
	    {
		CopyWDiscInfo(item,&dinfo);
		item->part_index = part_index;
		wlist->total_size_mib += item->size_mib;
		item++;
	    }
	}
    }

    wlist->used = item - wlist->first_disc;
    return wlist;
}

///////////////////////////////////////////////////////////////////////////////

void InitializeWDiscList ( WDiscList_t * wlist )
{
    ASSERT(wlist);
    memset(wlist,0,sizeof(*wlist));
}

///////////////////////////////////////////////////////////////////////////////

void ResetWDiscList ( WDiscList_t * wlist )
{
    ASSERT(wlist);

    if ( wlist->first_disc )
    {
	WDiscListItem_t * ptr = wlist->first_disc;
	WDiscListItem_t * end = ptr + wlist->used;
	for ( ; ptr < end; ptr++ )
	    FREE((char*)ptr->fname);
	FREE(wlist->first_disc);
    }
    InitializeWDiscList(wlist);
}

///////////////////////////////////////////////////////////////////////////////

WDiscListItem_t * AppendWDiscList ( WDiscList_t * wlist, WDiscInfo_t * dinfo )
{
    ASSERT(wlist);
    ASSERT( wlist->used <= wlist->size );
    if ( wlist->used == wlist->size )
    {
	wlist->size += 200;
	wlist->first_disc = REALLOC(wlist->first_disc,
				wlist->size*sizeof(*wlist->first_disc));
    }
    wlist->sort_mode = SORT_NONE;
    WDiscListItem_t * item = wlist->first_disc + wlist->used++;
    CopyWDiscInfo(item,dinfo);
    return item;
}

///////////////////////////////////////////////////////////////////////////////

void FreeWDiscList ( WDiscList_t * wlist )
{
    ASSERT(wlist);
    ResetWDiscList(wlist);
    FREE(wlist);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                WDisc sorting                    ///////////////
///////////////////////////////////////////////////////////////////////////////

typedef int (*compare_func) (const void *, const void *);

//-----------------------------------------------------------------------------

static int sort_by_title ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    int stat = strcasecmp( a->title ? a->title : a->name64,
			   b->title ? b->title : b->name64 );
    if (!stat)
    {
	stat = strcasecmp(a->name64,b->name64);
	if (!stat)
	{
	    stat = strcmp(a->id6,b->id6);
	    if (!stat)
	    {
		stat = strcmp(	a->fname ? a->fname : "" ,
				b->fname ? b->fname : "" );
		if (!stat)
		    stat = a->part_index - b->part_index;
	    }
	}
    }
    return stat;
}

//-----------------------------------------------------------------------------

static int sort_by_id ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = strcmp(a->id6,b->id6);
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_name ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = strcasecmp(a->name64,b->name64);
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_path ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = PathCMP(a->name64,b->name64);
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_nintendo ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = NintendoCMP(a->name64,b->name64);
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_file ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = strcmp( a->fname ? a->fname : "" ,
			     b->fname ? b->fname : "" );
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_size ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const u32 ab = a->used_blocks ? a->used_blocks : a->size_mib * WII_SECTORS_PER_MIB;
    const u32 bb = b->used_blocks ? b->used_blocks : b->size_mib * WII_SECTORS_PER_MIB;
    
    return ab < bb ? -1 : ab > bb ? +1 : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_offset ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = (int)a->wbfs_slot - (int)b->wbfs_slot;
    return stat ? stat : sort_by_size(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_mtime ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    return a->fatt.mtime > b->fatt.mtime ?  1
	 : a->fatt.mtime < b->fatt.mtime ? -1
	 : a->fatt.itime > b->fatt.itime ?  1
	 : a->fatt.itime < b->fatt.itime ? -1
	 : a->fatt.ctime > b->fatt.ctime ?  1
	 : a->fatt.ctime < b->fatt.ctime ? -1
	 : a->fatt.atime > b->fatt.atime ?  1
	 : a->fatt.atime < b->fatt.atime ? -1
	 : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_itime ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    return a->fatt.itime > b->fatt.itime ?  1
	 : a->fatt.itime < b->fatt.itime ? -1
	 : a->fatt.ctime > b->fatt.ctime ?  1
	 : a->fatt.ctime < b->fatt.ctime ? -1
	 : sort_by_mtime(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_ctime ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    return a->fatt.ctime > b->fatt.ctime ?  1
	 : a->fatt.ctime < b->fatt.ctime ? -1
	 : sort_by_mtime(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_atime ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    return a->fatt.atime > b->fatt.atime ?  1
	 : a->fatt.atime < b->fatt.atime ? -1
	 : sort_by_mtime(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_region ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = a->id6[3] - b->id6[3];
    return stat ? stat : sort_by_id(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_wbfs ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = a->part_index - b->part_index;
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_npart ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = (int)a->n_part - (int)b->n_part;
    return stat ? stat : sort_by_title(va,vb);
}

//-----------------------------------------------------------------------------

static int sort_by_frag ( const void * va, const void * vb )
{
    const WDiscListItem_t * a = (const WDiscListItem_t *)va;
    const WDiscListItem_t * b = (const WDiscListItem_t *)vb;

    const int stat = (int)a->wbfs_fragments - (int)b->wbfs_fragments;
    return stat ? stat : sort_by_title(va,vb);
}

///////////////////////////////////////////////////////////////////////////////

void ReverseWDiscList ( WDiscList_t * wlist )
{
    if (! wlist || wlist->used < 2 )
	return;

    ASSERT(wlist->first_disc);

    WDiscListItem_t *a, *b, temp;
    for ( a = wlist->first_disc, b = a + wlist->used-1; a < b; a++, b-- )
    {
	memcpy( &temp, a,     sizeof(WDiscListItem_t) );
	memcpy( a,     b,     sizeof(WDiscListItem_t) );
	memcpy( b,     &temp, sizeof(WDiscListItem_t) );
    }
}

///////////////////////////////////////////////////////////////////////////////

void SortWDiscList ( WDiscList_t * wlist,
	SortMode sort_mode, SortMode default_sort_mode, int unique )
{
    if (!wlist)
	return;

    TRACE("SortWDiscList(%p, s=%d,%d, u=%d) prev=%d\n",
		wlist, sort_mode, default_sort_mode, unique, wlist->sort_mode );

    if ( (uint)(sort_mode&SORT__MODE_MASK) >= SORT_DEFAULT )
    {
	sort_mode = (uint)default_sort_mode >= SORT_DEFAULT
			? wlist->sort_mode
			: default_sort_mode | sort_mode & SORT_REVERSE;
    }

    if ( wlist->used < 2 )
    {
	// zero or one element is sorted!
	wlist->sort_mode = sort_mode;
	return;
    }

    compare_func func = 0;
    SortMode umode = SORT_ID;
    SortMode smode = sort_mode & SORT__MODE_MASK;
    if ( smode == SORT_TIME )
	smode = SelectSortMode(opt_print_time);
    switch(smode)
    {
	case SORT_ID:		func = sort_by_id;	break;
	case SORT_NAME:		func = sort_by_name;	umode = SORT_NAME; break;
	case SORT_TITLE:	func = sort_by_title;	umode = SORT_TITLE; break;
	case SORT_PATH:		func = sort_by_path;	umode = SORT_PATH; break;
	case SORT_NINTENDO:	func = sort_by_nintendo;umode = SORT_NINTENDO; break;
	case SORT_FILE:		func = sort_by_file;	break;
	case SORT_SIZE:		func = sort_by_size;	umode = SORT_SIZE; break;
	case SORT_OFFSET:	func = sort_by_offset;	break;
	case SORT_REGION:	func = sort_by_region;	umode = SORT_REGION; break;
	case SORT_WBFS:		func = sort_by_wbfs;	break;
	case SORT_NPART:	func = sort_by_npart;	break;
	case SORT_FRAG:		func = sort_by_frag;	break;
	case SORT_ITIME:	func = sort_by_itime;	break;
	case SORT_MTIME:	func = sort_by_mtime;	break;
	case SORT_CTIME:	func = sort_by_ctime;	break;
	case SORT_ATIME:	func = sort_by_atime;	break;
	default:		break;
    }

    if (unique)
    {
	SortWDiscList(wlist,umode,umode,false);

	WDiscListItem_t *src, *dest, *end, *prev = 0;
	src = dest = wlist->first_disc;
	end = src + wlist->used;
	wlist->total_size_mib = 0;
	for ( ; src < end; src++ )
	{
	    if ( !prev
		|| memcmp(src->id6,prev->id6,6)
		|| unique < 2 &&
		    (  memcmp(src->name64,prev->name64,64)
		    || src->size_mib != prev->size_mib ))
	    {
		if ( dest != src )
		    memcpy(dest,src,sizeof(*dest));
		wlist->total_size_mib += dest->size_mib;
		prev = dest++;
	    }
	    else
		FREE((char*)src->fname);
	}
	wlist->used = dest - wlist->first_disc;
    }

    if ( func && wlist->sort_mode != sort_mode )
    {
	wlist->sort_mode = sort_mode;
	qsort(wlist->first_disc,wlist->used,sizeof(*wlist->first_disc),func);
	if ( sort_mode & SORT_REVERSE )
	    ReverseWDiscList(wlist);
    }
}

///////////////////////////////////////////////////////////////////////////////

static void print_sect_time ( FILE *f, char name, time_t tim )
{
    ASSERT(f);
    if (tim)
    {
	struct tm * tm = localtime(&tim);
	char timbuf[40];
	strftime(timbuf,sizeof(timbuf),"%F %T",tm);
	fprintf(f,"%ctime=%llu %s\n", name, (u64)tim, timbuf );
    }
    else
	fprintf(f,"%ctime=\n",name);
}

//-----------------------------------------------------------------------------

void PrintSectWDiscListItem ( FILE * f, WDiscListItem_t * witem, ccp def_fname )
{
    ASSERT(f);
    ASSERT(witem);

    fprintf(f,"id=%s\n",witem->id6);
    fprintf(f,"name=%s\n",witem->name64);
    fprintf(f,"title=%s\n", witem->title ? witem->title : "" );
    fprintf(f,"region=%s\n",GetRegionInfo(witem->id6[3])->name4);
    fprintf(f,"size=%llu\n",(u64)witem->fatt.size
				? (u64)witem->fatt.size : (u64)witem->size_mib*MiB );
    //fprintf(f,"size_mib=%u\n",witem->size_mib);
    if (witem->used_blocks)
	fprintf(f,"used-blocks=%u\n",witem->used_blocks);

    print_sect_time(f,'i',witem->fatt.itime);
    print_sect_time(f,'m',witem->fatt.mtime);
    print_sect_time(f,'c',witem->fatt.ctime);
    print_sect_time(f,'a',witem->fatt.atime);
    //fprintf(f,"part_index=%u\n",witem->part_index);
    //fprintf(f,"n_part=%u\n",witem->n_part);

    fprintf(f,"filetype=%s\n",GetNameFT(witem->ftype,0));
    fprintf(f,"container=%s\n",GetContainerNameFT(witem->ftype,"-"));
    const wd_disc_type_t dt = FileType2DiscType(witem->ftype);
    fprintf(f,"disctype=%d %s\n",dt,wd_get_disc_type_name(dt,"?"));

    if ( witem->n_part > 0 || *witem->part_info )
    {
	fprintf(f,"n-partitions=%d\n",witem->n_part);
	fprintf(f,"partition-info=%s\n",witem->part_info);
    }

    fprintf(f,"wbfs_slot=%d\n",witem->wbfs_slot);
    if (witem->wbfs_fragments)
	fprintf(f,"wbfs-fragments=%u\n",witem->wbfs_fragments);

    ccp fname = witem->fname ? witem->fname : def_fname ? def_fname : "";
    fprintf(f,"filename=%s\n",fname);
    if ( *fname && witem->wbfs_slot >= 0 )
	fprintf(f,"source=%s/#%u\n",fname,witem->wbfs_slot);
    else
	fprintf(f,"source=%s\n",fname);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 access to WBFS dics             ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError OpenWDiscID6 ( WBFS_t * w, ccp id6 )
{
    ASSERT(w);
    CloseWDisc(w);

    if ( !w || !w->wbfs || !id6 )
	return ERROR0(ERR_INTERNAL,0);

    w->disc = wbfs_open_disc_by_id6(w->wbfs,(u8*)id6);
    w->disc_slot = w->disc ? w->disc->slot : -1;
    return w->disc ? ERR_OK : ERR_WDISC_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenWDiscIndex ( WBFS_t * wbfs, u32 index )
{
    ASSERT(wbfs);

    if ( !wbfs || !wbfs->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    wbfs_t *w = wbfs->wbfs;
    wbfs->disc_slot = -1;

    u32 slot;
    for ( slot = 0; slot < w->max_disc; slot++ )
	if ( w->head->disc_table[slot] && !index-- )
	    return OpenWDiscSlot(wbfs,slot,0);

    CloseWDisc(wbfs);
    return ERR_WDISC_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenWDiscSlot ( WBFS_t * w, u32 slot, bool force_open )
{
    ASSERT(w);
    CloseWDisc(w);

    if ( !w || !w->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    w->disc = wbfs_open_disc_by_slot(w->wbfs,slot,force_open);
    w->disc_slot = w->disc ? w->disc->slot : -1;
    return w->disc ? ERR_OK : ERR_WDISC_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenWDiscSF ( WBFS_t * w )
{
    DASSERT(w);
    CloseWDiscSF(w);
    if ( !w->sf || !w->disc )
	return ERROR0(ERR_INTERNAL,0);

    SuperFile_t * sf = w->sf;
    sf->wbfs = w;
    SetupIOD(sf,OFT_WBFS,OFT_WBFS);
    SetPatchFileID(&sf->f,w->disc->header,6); // [[2do]] SetFileID() ?
    w->disc_sf_opened = true;

    uint disc_blocks;
    sf->wbfs_fragments = wbfs_get_disc_fragments(w->disc,&disc_blocks);
    sf->file_size = (off_t)disc_blocks * w->wbfs->wbfs_sec_sz;

    SetFileAttrib( &sf->f.fatt, 0, &sf->f.st );
    if ( w->disc->header && w->disc->header->dhead )
    {
	const wbfs_inode_info_t * ii
	    = (wbfs_inode_info_t*) ( w->disc->header->dhead + WBFS_INODE_INFO_OFF );
	CopyFileAttribInode( &sf->f.fatt, ii, 0 );
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseWDiscSF ( WBFS_t * w )
{
    DASSERT(w);
    if ( w->disc_sf_opened )
    {
	w->disc_sf_opened = false;

	SuperFile_t * sf = w->sf;
	if ( sf && sf->iod.oft == OFT_WBFS )
	{
	    CloseDiscSF(sf);
	    ClearFileID(&sf->f);
	    SetupIOD(sf,OFT_PLAIN,OFT_PLAIN);
	    sf->wbfs = 0;
	}

	SetFileAttrib(&sf->f.fatt,0,&sf->f.st);
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseWDisc ( WBFS_t * w )
{
    DASSERT(w);

    CloseWDiscSF(w);
    SuperFile_t *sf = w->sf;

    if (w->disc)
    {
	if ( !sf || !IsOpenSF(sf) )
	    w->disc->is_dirty = false;

	if ( sf
		&& sf->f.is_writing
		&& w->disc->header
		&& w->is_wbfs_file
		&& CopyPatchWbfsId( (char*)w->disc->header->dhead,
				*sf->wbfs_id6
					? sf->wbfs_id6
					: (ccp)w->disc->header->dhead )
	   ) // [[id+]]
	{
	    w->disc->is_dirty = true;
	}

	wbfs_close_disc(w->disc);
	w->disc = 0;
    }

    if (sf)
	CloseDiscSF(sf);

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ExistsWDisc ( WBFS_t * w, ccp id6 )
{
    ASSERT(w);

    if ( !w || !w->wbfs || !id6 )
	return ERROR0(ERR_INTERNAL,0);

    char patched_id6[7];
    CopyPatchWbfsId(patched_id6,id6);

    return wbfs_find_slot(w->wbfs,(u8*)patched_id6) < 0
		? ERR_WDISC_NOT_FOUND
		: ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

wd_header_t * GetWDiscHeader ( WBFS_t * w )
{
    return w && w->disc && w->disc->header
		? (wd_header_t*)w->disc->header
		: 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      AddWDisc()                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError AddWDisc ( WBFS_t * w, SuperFile_t * sf, const wd_select_t * psel )
{
    if ( !w || !w->wbfs || !w->sf || !sf )
	return ERROR0(ERR_INTERNAL,0);

    TRACE("AddWDisc(w=%p,sf=%p) progress=%d,%d\n",
		w, sf, sf->show_progress, sf->show_summary );

    CloseWDisc(w);

    // this is needed for detailed error messages
    const enumError saved_max_error = max_error;
    max_error = 0;

    wbfs_param_t par;
    memset(&par,0,sizeof(par));
    par.read_src_wii_disc	= WrapperReadSF; // [[2do]] [[obsolete]]? (both: param and func)
    par.callback_data		= sf;
    par.spinner			= sf->show_progress ? PrintProgressSF : 0;
    par.psel			= psel;
    par.iinfo.mtime		= hton64(sf->f.fatt.mtime);
    par.iso_size		= sf->file_size;
    par.wd_disc			= OpenDiscSF(sf,false,true);

    PRINT("iso_size=%llu\n",par.iso_size);

    // try to copy mtime from WBFS source disc
    if ( sf->wbfs && sf->wbfs->disc )
    {
	const wbfs_inode_info_t * iinfo = wbfs_get_disc_inode_info(sf->wbfs->disc,0);
	if (ntoh64(iinfo->mtime))
	    par.iinfo.mtime = iinfo->mtime;
    }

    if (*sf->wbfs_id6)
	CopyPatchWbfsId( par.wbfs_id6, sf->wbfs_id6 );
    else if (par.wd_disc)
	CopyPatchWbfsId( par.wbfs_id6, &par.wd_disc->dhead.disc_id );

    const int wbfs_stat = wbfs_add_disc_param(w->wbfs,&par);

    // transfer results
    w->disc = par.open_disc;
    w->disc_slot = par.slot;

    enumError err = ERR_OK;
    if (wbfs_stat)
    {
	err = ERR_WBFS;
	if (!w->sf->f.disable_errors)
	    ERROR0(err,"Error while adding disc [%s]: %s\n",
		    sf->f.id6_dest, w->sf->f.fname );
    }
    else
    {
	ASSERT(w->sf);
	TRACE("AddWDisc/stat: w=%p, slot=%d, w->sf=%p, oft=%d\n",
		w, w->disc_slot, w->sf, w->sf->iod.oft );
        err = RewriteModifiedSF(sf,0,w,0);
    }

    // catch read/write errors
    err = max_error = max_error > err ? max_error : err;
    if ( max_error < saved_max_error )
	max_error = saved_max_error;

    PrintSummarySF(sf);

    // calculate the wbfs usage again
    CalcWBFSUsage(w);

    TRACE("AddWDisc() returns err=%d [%s]\n",err,GetErrorName(err,"?"));
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    RemoveWDisc()                ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError RemoveWDisc
(
    WBFS_t		* w,		// valid WBFS descriptor
    ccp			id6,		// id6 to remove. If NULL: remove 'slot'
    int			slot,		// slot index, only used if 'discid==NULL'
    int			free_slot_only	// true: do not free blocks
)
{
    TRACE("RemoveWDisc(%p,%s,%d,%d)\n",w,id6?id6:"-",slot,free_slot_only);
    if ( !w || !w->wbfs || !w->sf )
	return ERROR0(ERR_INTERNAL,0);

    // this is needed for detailed error messages
    const enumError saved_max_error = max_error;
    max_error = 0;

    // remove the disc
    enumError err = ERR_OK;
    if (wbfs_rm_disc(w->wbfs,(u8*)id6,slot,free_slot_only))
    {
	err = ERR_WDISC_NOT_FOUND;
	if (!w->sf->f.disable_errors)
	    ERROR0(err,"Can't remove disc non existing [%s]: %s\n",
		id6, w->sf->f.fname );
    }

 #if defined(TEST) && defined(DEBUG)
    if (logging)
	DumpWBFS(w,stdout,3,SHOW_USAGE,0,0,0);
 #endif

 #ifdef DEBUG
    DumpWBFS(w,TRACE_FILE,3,SHOW__DEFAULT,0,0,0);
 #endif

    // check if the disc is really removed
    if ( id6 && !ExistsWDisc(w,id6) )
    {
	err = ERR_REMOVE_FAILED;
	if (!w->sf->f.disable_errors)
	    ERROR0(err,"Can't remove disc [%s]: %s\n",
		id6, w->sf->f.fname );
    }

    // catch read/write errors
    err = max_error = max_error > err ? max_error : err;
    if ( max_error < saved_max_error )
	max_error = saved_max_error;

    // calculate the wbfs usage again
    CalcWBFSUsage(w);

    TRACE("RemoveWDisc(%s) returns err=%d [%s]\n",id6,err,GetErrorName(err,"?"));
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                    RenameWDisc()                ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError RenameWDisc
	( WBFS_t * wbfs, ccp set_id6, ccp set_title,
		bool change_wbfs_head, bool change_iso_head,
		int verbose, int testmode )
{
    ASSERT(wbfs);
    ASSERT(wbfs->wbfs);
    ASSERT(wbfs->disc);
    ASSERT(wbfs->disc->header);

    TRACE("RenameWDisc(%p,%.6s,%s,%d,%d,v=%d,tm=%d)\n",
		wbfs, set_id6 ? set_id6 : "-",
		set_title ? set_title : "-",
		change_wbfs_head, change_iso_head, verbose, testmode );

    if ( !wbfs || !wbfs->wbfs || !wbfs->sf )
	return ERROR0(ERR_INTERNAL,0);

    if ( !set_id6 || !*set_id6 || strlen(set_id6) > 6 )
	set_id6 = 0; // invalid id6

    if ( !set_title || !*set_title )
	set_title = 0; // invalid title

    if ( !set_id6 && !set_title )
	return ERR_OK; // nothing to do

    if ( !change_wbfs_head && !change_iso_head )
	change_wbfs_head = change_iso_head = true;

    wd_header_t * whead = (wd_header_t*)wbfs->disc->header;
    char w_id6[7], n_id6[7];
    memset(w_id6,0,sizeof(w_id6));
    StringCopyS(w_id6,sizeof(w_id6),&whead->disc_id);
    memcpy(n_id6,w_id6,sizeof(n_id6));

    if ( testmode || verbose >= 0 )
    {
	ccp mode = !change_wbfs_head
			? "(ISO header only)"
			: !change_iso_head
				? "(WBFS header only)"
				: "(WBFS+ISO header)";
	printf(" - %sModify %s [%s] %s\n",
		testmode ? "WOULD " : "", mode, w_id6, wbfs->sf->f.fname );
    }

    if (set_id6)
    {
	memcpy(n_id6,set_id6,6);
	set_id6 = n_id6;
	if ( testmode || verbose >= 0 )
	    printf("   - %sRename ID to `%s'\n", testmode ? "WOULD " : "", set_id6 );
    }

    if (set_title)
    {
	wd_header_t ihead;
	LoadIsoHeader(wbfs,&ihead,0);

	char w_name[0x40], i_id6[7], i_name[0x40];
	StringCopyS(i_id6,sizeof(i_id6),&ihead.disc_id);
	StringCopyS(w_name,sizeof(w_name),whead->disc_title);
	StringCopyS(i_name,sizeof(i_name),ihead.disc_title);

	ccp w_title = GetTitle(w_id6,w_name);
	ccp i_title = GetTitle(i_id6,i_name);
	ccp n_title = GetTitle(n_id6,w_name);

	TRACE("W-TITLE: %s, %s\n",w_id6,w_title);
	TRACE("I-TITLE: %s, %s\n",i_id6,i_title);
	TRACE("N-TITLE: %s, %s\n",n_id6,n_title);

	// and now the destination filename
	SubstString_t subst_tab[] =
	{
	    { 'j', 0,	0, w_id6 },
	    { 'J', 0,	0, i_id6 },
	    { 'i', 'I',	0, n_id6 },

	    { 'n', 0,	0, w_name },
	    { 'N', 0,	0, i_name },

	    { 'p', 0,	0, w_title },
	    { 'P', 0,	0, i_title },
	    { 't', 'T',	0, n_title },

	    {0,0,0,0}
	};

	char title[PATH_MAX];
	SubstString(title,sizeof(title),subst_tab,set_title,0);
	set_title = title;

	if ( testmode || verbose >= 0 )
	    printf("   - %sSet title to `%s'\n",
			testmode ? "WOULD " : "", set_title );
    }

    if (!testmode
	&& wbfs_rename_disc( wbfs->disc, set_id6, set_title,
				change_wbfs_head, change_iso_head ) )
		return ERROR0(ERR_WBFS,"Renaming of disc failed: %s\n",
				wbfs->sf->f.fname );
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       END                       ///////////////
///////////////////////////////////////////////////////////////////////////////

