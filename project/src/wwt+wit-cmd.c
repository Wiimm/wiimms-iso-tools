
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

#include "lib-bzip2.h"
#include "lib-lzma.h"

///////////////////////////////////////////////////////////////////////////////
//   This file is included by wwt.c and wit.c and contains common commands.  //
///////////////////////////////////////////////////////////////////////////////

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cmd_error()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_error()
{
    if (!n_param)
    {
	if ( print_sections )
	{
	    int i;
	    for ( i=0; i<ERR__N; i++ )
		printf("\n[error-%02u]\ncode=%u\nname=%s\ntext=%s\n",
			i, i, GetErrorName(i,"?"), GetErrorText(i,"?") );
	}
	else
	{
	    const bool print_header = !OptionUsed[OPT_NO_HEADER];

	    if (print_header)
	    {
		print_title(stdout);
		printf(" List of error codes\n\n");
	    }
	    int i;

	    // calc max_wd
	    int max_wd = 0;
	    for ( i=0; i<ERR__N; i++ )
	    {
		const int len = strlen(GetErrorName(i,"?"));
		if ( max_wd < len )
		    max_wd = len;
	    }

	    // print table
	    for ( i=0; i<ERR__N; i++ )
		printf("%3d : %-*s : %s\n",
			i, max_wd,GetErrorName(i,"?"), GetErrorText(i,"?") );

	    if (print_header)
		printf("\n");
	}

	return ERR_OK;
    }

    int stat;
    long num = ERR__N;
    if ( n_param != 1 )
	stat = ERR_SYNTAX;
    else
    {
	char * end;
	num = strtoul(first_param->arg,&end,10);
	stat = *end ? ERR_SYNTAX : num < 0 || num >= ERR__N ? ERR_SEMANTIC : ERR_OK;
    }

    if (print_sections)
	printf("\n[error]\ncode=%lu\nname=%s\ntext=%s\n",
		num, GetErrorName(num,"?"), GetErrorText(num,"?"));
    else if (long_count)
	printf("%s\n",GetErrorText(num,"?"));
    else
	printf("%s\n",GetErrorName(num,"?"));
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cmd_gettitles()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_gettitles()
{
    if (n_param)
	return ERROR0(ERR_SYNTAX,"No parameters needed: %s%s\n",
		first_param->arg, n_param > 1 ? " ..." : "" );

 #ifdef __CYGWIN__

    ccp progs = getenv("ProgramFiles");
    if ( !progs || !*progs )
	progs = "C:/Program Files";
    char dirbuf[PATH_MAX], normbuf[PATH_MAX];
    snprintf(dirbuf,sizeof(dirbuf),"%s/%s",progs,WIN_INSTALL_PATH);
    NormalizeFilenameCygwin(normbuf,sizeof(normbuf),dirbuf);
    PRINT("-> %s\n",dirbuf);
    PRINT("-> %s\n",normbuf);
    
    if (chdir(normbuf))
	return ERROR0(ERR_CANT_OPEN,"Can't change directory: %s\n",normbuf);

    system("./load-titles.sh --cygwin");

 #else

    if (chdir(SHARE_PATH))
	return ERROR0(ERR_CANT_OPEN,"Can't change directory: %s\n",SHARE_PATH);

    system("./load-titles.sh");

 #endif

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cmd_compr()			///////////////
///////////////////////////////////////////////////////////////////////////////

void print_default_compr ( ccp mode )
{
    int level = 0;
    u32 csize = 0;
    wd_compression_t compr = ScanCompression(mode,true,&level,&csize);
    CalcDefaultSettingsWIA(&compr,&level,&csize);
    printf("mode-%s=%s\n",
		mode,
		wd_print_compression(0,0,compr,level,csize,2));
};

///////////////////////////////////////////////////////////////////////////////

enumError cmd_compr()
{
    const bool have_param = n_param > 0;
    if (!n_param)
    {
	int i;
	for ( i = 0; i < WD_COMPR__N; i++ )
	    AddParam(wd_get_compression_name(i,0),false);
	if (long_count)
	{
	    AddParam(" DEFAULT",false);
	    AddParam(" FAST",false);
	    AddParam(" GOOD",false);
	    AddParam(" BEST",false);
	    AddParam(" MEM",false);
	    
	    char buf[2] = {0,0};
	    for ( i = '0'; i <= '9'; i++ )
	    {
		buf[0] = i;
		AddParam(buf,true);
	    }
	}
    }

    int err_count = 0;
    if (print_sections)
    {
	if (!have_param)
	{
	    printf( "\n[compression-modes]\n"
		    "n-methods=%u\n", WD_COMPR__N );
	    print_default_compr("default");
	    print_default_compr("fast");
	    print_default_compr("good");
	    print_default_compr("best");
	    print_default_compr("mem");
	}

	int index = 0;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next, index++ )
	{
	    int level;
	    u32 csize;
	    wd_compression_t compr = ScanCompression(param->arg,true,&level,&csize);
	    printf( "\n[compression-mode-%u]\n"
		"input=%s\n"
		"num=%d\n"
		"name=%s\n",
		index, param->arg,
		compr, wd_get_compression_name(compr,"-") );
	    if ( compr == (wd_compression_t)-1 )
		err_count++;
	    else
	    {
		CalcDefaultSettingsWIA(&compr,&level,&csize);
		if ( level > 0 )
		    printf("level=%u\n",level);
		if ( csize > 0 )
		    printf("chunk-factor=%u\nchunk-size=%u\n",
				csize/WIA_BASE_CHUNK_SIZE, csize );
	     #ifdef NO_BZIP2
		if ( compr == WD_COMPR_BZIP2 )
		    fputs("not-supported=1\n",stdout);
	     #endif
	    }
	}
	putchar('\n');
    }
    else if (long_count)
    {
	const bool print_header = !OptionUsed[OPT_NO_HEADER];
	if (print_header)
	    printf( "\n"
		    " mode           memory usage\n"
		      " %s         reading   writing   input mode\n"
		    "---------------------------------------------\n",
		    OptionUsed[OPT_NUMERIC] ? "num " : "name" );

	const int mode = OptionUsed[OPT_NUMERIC] ? 1 : 2;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	{
	    int level;
	    u32 csize;
	    wd_compression_t compr = ScanCompression(param->arg,true,&level,&csize);
	    if ( verbose > 0 )
	    {
		wd_compression_t compr2 = compr;
		CalcDefaultSettingsWIA(&compr2,&level,&csize);
	    }

	    if ( compr == (wd_compression_t)-1 )
	    {
		err_count++;
		printf(" -                -         -     %s\n",param->arg);
	    }
	    else
	    {
		u32 read_size  = CalcMemoryUsageWIA(compr,level,csize,false);
		u32 write_size = CalcMemoryUsageWIA(compr,level,csize,true);
		printf(" %-11s %s  %s   %.30s\n",
			wd_print_compression(0,0,compr,level,csize,mode),
			wd_print_size_1024(0,0,read_size,true),
			wd_print_size_1024(0,0,write_size,true),
			param->arg );
	    }
	}

	if (print_header)
	    putchar('\n');
    }
    else
    {
	const int mode = OptionUsed[OPT_NUMERIC] ? 1 : 2;
	ParamList_t * param;
	for ( param = first_param; param; param = param->next )
	{
	    int level;
	    u32 csize;
	    wd_compression_t compr = ScanCompression(param->arg,true,&level,&csize);
	    if ( verbose > 0 )
	    {
		wd_compression_t compr2 = compr;
		CalcDefaultSettingsWIA(&compr2,&level,&csize);
	    }

	 #ifdef NO_BZIP2
	    if ( !have_param && compr == WD_COMPR_BZIP2 )
		continue; // ignore it
	 #endif
	    if ( compr == (wd_compression_t)-1 )
		err_count++;
	    printf("%s\n",wd_print_compression(0,0,compr,level,csize,mode));
	}
    }

    return err_count ? ERR_WARNING : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cmd_features()		///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    //--- image formats

    FEAT_ISO,
    FEAT_WDF,
    FEAT_WDF1,
    FEAT_WDF2,
    FEAT_CISO,
    FEAT_WBFS,
    FEAT_WIA,
    FEAT_GCZ,

    //--- misc

    FEAT_PRALLOC,
    
    //--- all

    FEAT__ALL
};

//-----------------------------------------------------------------------------

static const KeywordTab_t feature_tab[] =
{
    //--- image formats

    { FEAT_ISO,		"ISO",		0,		0 },
    { FEAT_WDF,		"WDF",		0,		0 },
    { FEAT_WDF1,	"WDF1",		"WDFV1",	0 },
    { FEAT_WDF2,	"WDF2",		"WDFV2",	0 },
    { FEAT_CISO,	"CISO",		"WBI",		0 },
    { FEAT_WBFS,	"WBFS",		0,		0 },
    { FEAT_WIA,		"WIA",		0,		0 },
    { FEAT_GCZ,		"GCZ",		0,		0 },

    //--- misc

    { FEAT_PRALLOC,	"PRALLOC",	0, 0 },

    //--- all

    { FEAT__ALL,	"ALL",		0, 1 },
    { 0,0,0,0 }
};

//-----------------------------------------------------------------------------

static bool check_feature ( uint feat )
{
    bool stat = false;
    ccp  msg = 0;

    switch (feat)
    {
	case FEAT_ISO:		stat = 1; msg = "Image format ISO"; break;
	case FEAT_WDF:		stat = 1; msg = "Image format WDF"; break;
	case FEAT_WDF1:		stat = 1; msg = "Image format WDF v1"; break;
	case FEAT_WDF2:		stat = 1; msg = "Image format WDF v2"; break;
	case FEAT_CISO:		stat = 1; msg = "Image format CISO"; break;
	case FEAT_WBFS:		stat = 1; msg = "Image format WBFS"; break;
	case FEAT_WIA:		stat = 1; msg = "Image format WIA"; break;

     #if HAVE_ZLIB
	case FEAT_GCZ:		stat = 1; msg = "Image format GCZ"; break;
     #else
	case FEAT_GCZ:		stat = 0; msg = "Image format GCZ"; break;
     #endif

     #if NO_PREALLOC
	case FEAT_PRALLOC:	stat = 0; msg = "File pre allocation"; break;
     #else
	case FEAT_PRALLOC:	stat = 1; msg = "File pre allocation"; break;
     #endif
    }

    if ( msg && verbose >= 0 && ( verbose > 0 || stat ))
    {
	const KeywordTab_t *cmd;
	for ( cmd = feature_tab; cmd->name1; cmd++ )
	if ( cmd->id == feat )
	{
	    if (stat)
		printf("+ %-7s : %s is supported.\n",cmd->name1,msg);
	    else
		printf("- %-7s : %s is not supported.\n",cmd->name1,msg);
	}
    }
    return stat;
}

//-----------------------------------------------------------------------------

static void check_all_features ( uint *n_supported, uint *n_total )
{
    DASSERT(n_supported);
    DASSERT(n_total);

    uint feat;
    for ( feat = 0; feat < FEAT__ALL; feat++ )
    {
	(*n_total)++;
	    if (check_feature(feat))
		(*n_supported)++;
    }
}

//-----------------------------------------------------------------------------

enumError cmd_features()
{
    uint n_supported = 0, n_total = 0;
    
    ParamList_t * param;
    for (  param = first_param; param; param = param->next )
    {
	ccp arg = param->arg;
	if ( !arg || !*arg )
	    continue;

	int cmd_stat;
	const KeywordTab_t * cmd = ScanKeyword(&cmd_stat,arg,feature_tab);
	if ( !cmd || cmd_stat )
	{
	    n_total++;
	    if ( verbose > 0 )
		printf("? Unknown feature: %s.\n",arg);
	    continue;
	}

	if ( cmd->id == FEAT__ALL )
	    check_all_features(&n_supported,&n_total);
	else
	{
	    n_total++;
	    if (check_feature(cmd->id))
		n_supported++;
	}
    }

    if (!n_total)
	check_all_features(&n_supported,&n_total);

    if ( n_supported == n_total )
    {
	if ( verbose >= -1 )
	{
	    if ( n_total == 1 )
		printf("> Exit status 0: The requested features is supported.\n");
	    else
		printf("> Exit status 0: All %u requested features are supported.\n",
			n_total);
	}
	return 0;
    }

    if ( !n_supported )
    {
	if ( verbose >= -1 )
	{
	    if ( n_total == 1 )
		printf("> Exit status 2: The requested features is not supported.\n");
	    else
		printf("> Exit status 2: All %u requested features are not supported.\n",
			n_total);
	}
	return 2;
    }

    if ( verbose >= -1 )
	printf("> Exit status 1: %u of %u requested features are supported.\n",
		    n_supported, n_total );
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cmd_exclude()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_exclude()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AtFileHelper(param->arg,0,1,AddExcludeID);

    DumpExcludeDB();    
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cmd_titles()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_titles()
{
    ParamList_t * param;
    for ( param = first_param; param; param = param->next )
	AtFileHelper(param->arg,0,0,AddTitleFile);

    InitializeTDB();
    DumpIDDB(&title_db,stdout);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			cmd_test_options()		///////////////
///////////////////////////////////////////////////////////////////////////////

static void print_val( ccp title, u64 val, ccp comment )
{
    ccp t1000 = wd_print_size_1000(0,0,val,true);
    ccp t1024 = wd_print_size_1024(0,0,val,true);

    if (strcmp(t1000,t1024))
	printf("  %-13s%16llx = %12lld = %s = %s%s\n",
			title, val, val, t1024, t1000, comment ? comment : "" );
    else
	printf("  %-13s%16llx = %12lld%s\n",
			title, val, val, comment ? comment : "" );
}

//-----------------------------------------------------------------------------

enumError cmd_test_options()
{
    printf("Options (hex=dec):\n");

    printf("  test:        %16d\n",testmode);
    printf("  verbose:     %16d\n",verbose);
    printf("  width:       %16d\n",opt_width);

 #if IS_WWT
    print_val( "size:",		opt_size, 0);
    print_val( "hd sec-size:",	opt_hss, 0);
    print_val( "wb sec-size:",	opt_wss, 0);
    print_val( "repair-mode:",	repair_mode, 0);
 #elif defined(TEST)
    u64 opt_size = 0;
 #endif

    snprintf(iobuf,sizeof(iobuf)," = %s",oft_info[output_file_type].name);
    print_val( "output-mode:",	output_file_type, iobuf );

    SetupOptionsWDF();
    if ( opt_wdf_version || opt_wdf_align || opt_wdf_min_holesize )
    {
	if ( use_wdf_version != opt_wdf_version )
	    snprintf(iobuf,sizeof(iobuf)," => use v%d",use_wdf_version);
	else
	    *iobuf = 0;
	print_val( "wdf-version:",	opt_wdf_version, iobuf );

	if ( use_wdf_align != opt_wdf_align )
	    snprintf(iobuf,sizeof(iobuf)," => use %x (%s)",
		use_wdf_align, wd_print_size_1024(0,0,use_wdf_align,false) );
	else
	    *iobuf = 0;
	print_val( "align-wdf:",	opt_wdf_align, iobuf );

	print_val( "minhole-wdf",	opt_wdf_min_holesize, 0 );
    }

    print_val( "chunk-mode:",	opt_chunk_mode,	0 );
    print_val( "chunk-size:",	opt_chunk_size,	force_chunk_size ? " FORCE!" : "" );
    print_val( "max-chunks:",	opt_max_chunks,	0 );

 #ifdef TEST
    {
	u64 filesize[] = { 100ull*MiB, 1ull*GiB, 10ull*GiB, opt_size, 0 };
	u64 *fs_ptr = filesize;
	for (;;)
	{
	    u32 n_blocks;
	    u32 block_size = CalcBlockSizeCISO(&n_blocks,*fs_ptr);
	    printf("   CISO block size %12llx = %12lld -> %5u * %8x/hex == %12llx/hex\n",
			*fs_ptr, *fs_ptr, n_blocks, block_size,
			(u64)block_size * n_blocks );
	    if (!*++fs_ptr)
		break;
	}
    }
 #endif

    printf("  auto-split:  %16x = %12d\n",opt_auto_split,opt_auto_split);
    printf("  split:       %16x = %12d\n",opt_split,opt_split);
    print_val( "split-size:",	opt_split_size, 0 );
    printf("  compression: %16x = %12d = %s (level=%d)\n",
		opt_compr_method, opt_compr_method,
		wd_get_compression_name(opt_compr_method,"?"), opt_compr_level );
    printf("   level:      %16x = %12d\n",opt_compr_level,opt_compr_level);
    print_val( " chunk-size:",	opt_compr_chunk_size, 0 );

    print_val( "mem:",		opt_mem, 0 );
    GetMemLimit();
    print_val( "mem limit:",	opt_mem, 0 );

    printf("  escape-char: %16x = %12d\n",escape_char,escape_char);
    printf("  print-time:  %16x = %12d\n",opt_print_time,opt_print_time);
    printf("  sort-mode:   %16x = %12d\n",sort_mode,sort_mode);
    printf("  show-mode:   %16x = %12d\n",opt_show_mode,opt_show_mode);
    printf("  unit:        %16x = %12d, unit=%s\n",
			opt_unit, opt_unit, wd_get_size_unit(opt_unit,"?") );
    printf("  limit:       %16x = %12d\n",opt_limit,opt_limit);
 #if IS_WIT
    printf("  file-limit:  %16x = %12d\n",opt_file_limit,opt_file_limit);
    printf("  block-size:  %16x = %12d\n",opt_block_size,opt_block_size);
 #endif
    printf("  gcz-block:   %16x = %12d\n",opt_gcz_block_size,opt_gcz_block_size);
    printf("  rdepth:      %16x = %12d\n",opt_recurse_depth,opt_recurse_depth);
    printf("  enc:         %16x = %12d\n",encoding,encoding);
    printf("  region:      %16x = %12d\n",opt_region,opt_region);
    printf("  prealloc:    %16x = %12d\n",prealloc_mode,prealloc_mode);

    if (opt_ios_valid)
    {
	const u32 hi = opt_ios >> 32;
	const u32 lo = (u32)opt_ios;
	if ( hi == 1 && lo < 0x100 )
	    printf("  ios:        %08x-%08x = IOS %u\n", hi, lo, lo );
	else
	    printf("  ios:        %08x-%08x\n", hi, lo );
    }

    printf("  modify:      %16x = %12d\n",opt_modify,opt_modify);
    if (modify_name)
	printf("  modify name: '%s'\n",modify_name);
    if (modify_id)
	printf("  modify id:        '%s'\n",modify_id);
    if (modify_disc_id)
	printf("  modify disc id:   '%s'\n",modify_disc_id);
    if (modify_boot_id)
	printf("  modify boot id:   '%s'\n",modify_boot_id);
    if (modify_ticket_id)
	printf("  modify ticket id: '%s'\n",modify_ticket_id);
    if (modify_tmd_id)
	printf("  modify tmd id:    '%s'\n",modify_tmd_id);
    if (modify_wbfs_id)
	printf("  modify wbfs id:   '%s'\n",modify_wbfs_id);

    if ( opt_http || opt_domain )
    {
	printf("  http:             %s\n", opt_http ? "enabled" : "disabled" );
	printf("  domain:           '%s'\n",opt_domain);
    }

 #if IS_WWT
    char buf_set_time[20];
    if (opt_set_time)
    {
	struct tm * tm = localtime(&opt_set_time);
	strftime(buf_set_time,sizeof(buf_set_time),"%F %T",tm);
    }
    else
	strcpy(buf_set_time,"NULL");
    printf("  set-time:    %16llx = %12lld = %s\n",
		(u64)opt_set_time, (u64)opt_set_time,buf_set_time );
 #endif

    printf("  trim:        %16x = %12u\n",opt_trim,opt_trim);
    print_val( "align #1:",	opt_align1, 0 );
    print_val( "align #2:",	opt_align2, 0 );
    print_val( "align #3:",	opt_align3, 0 );
    print_val( "align-part:",	opt_align_part, 0 );
    print_val( "disc-size:",	opt_disc_size, 0 );
    printf("  partition selector:\n");
    wd_print_select(stdout,6,&part_selector);

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PrintErrorStatWit()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError PrintErrorStatWit ( enumError err, ccp cmdname )
{
    if ( print_sections )
    {
	putchar('\n');
	if ( err )
	    printf("[error]\nprogram=%s\ncommand=%s\ncode=%u\nname=%s\ntext=%s\n\n",
			progname, cmdname, err,
			GetErrorName(err,"?"), GetErrorText(err,"?") );
    }

    if (   verbose > 0 && err >= ERR_WARNING
	|| verbose > 1 && err
	|| err == ERR_NOT_IMPLEMENTED )
    {
	fprintf(stderr,"%s: Command '%s' returns with status #%d [%s]\n",
			progname, cmdname, err, GetErrorName(err,"?") );
    }

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd_info()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void info_image_formats()
{
    if (print_sections)
    {
	printf(	"\n"
		"[IMAGE-FORMAT]\n"
		"n=%u\n",
		OFT__N - 1);

	ccp text = "list=";
	enumOFT oft;
	for ( oft = 1; oft < OFT__N; oft++ )
	{
	    printf("%s%s",text,oft_info[oft].name);
	    text = " ";
	}
	putchar('\n');

	for ( oft = 1; oft < OFT__N; oft++ )
	{
	    const OFT_info_t * info = oft_info + oft;
	    printf(	"\n"
			"[IMAGE-FORMAT:%s]\n"
			"name=%s\n"
			"info=%s\n"
			"option=%s\n"
			"extensions=%s %s\n"
			"attributes=%s%s%s%s%s%s%s%s%s\n"
			,info->name
			,info->name
			,info->info
			,info->option
			,info->ext1 ? info->ext1 : ""
			,info->ext2 ? info->ext2 : ""
			,info->attrib & OFT_A_READ	? "read "	: ""
			,info->attrib & OFT_A_CREATE	? "create "	: ""
			,info->attrib & OFT_A_MODIFY	? "modify "	: ""
			,info->attrib & OFT_A_EXTEND	? "extend "	: ""
			,info->attrib & OFT_A_FST	? "fst "	: ""
			,info->attrib & OFT_A_COMPR	? "compr "	: ""
			,info->attrib & OFT_A_NOSIZE    ? "nosize "	: ""
			,info->attrib & OFT_A_LOADER	? "loader "	: ""
			,info->attrib & OFT_A_DEST_EDIT	? "destedit "	: ""
			);
	}
	return;
    }

    //----- table output

    enumOFT oft;
    int info_fw = 0;
    for ( oft = 1; oft < OFT__N; oft++ )
    {
	const int len = strlen(oft_info[oft].info);
	if ( info_fw < len )
	     info_fw = len;
    }

    printf("\n"
	   "Image formats:\n\n"
	   "  Name  %-*s  Option  Extensions  Attributes\n"
	   " %.*s\n",
	   info_fw, "Description",
	   info_fw+64, wd_sep_200 );

    for ( oft = 1; oft < OFT__N; oft++ )
    {
	const OFT_info_t * i = oft_info + oft;
	printf("  %-4s  %-*s  %-7s %-5s %-5s %s %s %s %s %s\n",
		i->name, info_fw, i->info,
		i->option ? i->option : "  -",
		i->ext1 && *i->ext1 ? i->ext1 : " -",
		i->ext2 && *i->ext2 ? i->ext2 : " -",
		i->attrib & OFT_A_READ		? "read" : "-   ",
		i->attrib & OFT_A_CREATE	? "create"  : "-     ",
		i->attrib & OFT_A_EXTEND	? "extend"
		: i->attrib & OFT_A_MODIFY	? "modify" : "-     ",
		i->attrib & OFT_A_FST		? "fst   "
		: i->attrib & OFT_A_COMPR	? "compr "
		: i->attrib & OFT_A_NOSIZE	? "nosize" : "-     ",
		i->attrib & OFT_A_DEST_EDIT	? "destedit"
		: i->attrib & OFT_A_LOADER	? "loader" : "-" );
    }

    if (long_count)
	fputs("\n Attributes:\n"
	    "   read     : Image format can be read.\n"
	    "   create   : Image format can be created.\n"
	    "   modify   : Image format can be created and modified.\n"
	    "   extend   : Image format can be created, modified and extended.\n"
	    "   fst      : Image is an extracted file system.\n"
	    "   compr    : Image format supports compressed data (bzip2,7z,zip).\n"
	    "   nosize   : Info about the image size is not available.\n"
	    "   destedit : If using as source, the destination must be modifiable.\n"
	    "   loader   : Image format can be used by USB loaders.\n"
	    ,stdout);
}

///////////////////////////////////////////////////////////////////////////////

enum
{
    INFO_IMAGE_FORMAT	= 0x0001,

    INFO__ALL		= 0x0001
};

///////////////////////////////////////////////////////////////////////////////

enumError cmd_info()
{
    static const KeywordTab_t cmdtab[] =
    {
	{ INFO__ALL,		"ALL",			0,			0 },
	{ INFO_IMAGE_FORMAT,	"IMAGE-FORMATS",	"IMAGEFORMATS",		0 },
	{ INFO_IMAGE_FORMAT,	"FILE-FORMATS",		"FORMATS",		0 },

	{ 0,0,0,0 }
    };

    u32 keys = 0;
    ParamList_t * param;
    for (  param = first_param; param; param = param->next )
    {
	ccp arg = param->arg;
	if ( !arg || !*arg )
	    continue;

	const KeywordTab_t * cmd = ScanKeyword(0,arg,cmdtab);
	if (!cmd)
	    return ERROR0(ERR_SYNTAX,"Unknown keyword: %s\n",arg);

	keys |= cmd->id;
    }

    if (!keys)
	keys = INFO__ALL;

    if (print_sections)
    {
	printf("\n[INFO]\n");

	ccp text = "infos-avail=";
	const KeywordTab_t * cptr;
	for ( cptr = cmdtab + 1; cptr->name1; cptr++ )
	{
	    printf("%s%s",text,cptr->name1);
	    text = " ";
	}
	putchar('\n');

	text = "infos=";
	for ( cptr = cmdtab + 1; cptr->name1; cptr++ )
	{
	    if ( cptr->id & keys )
	    {
		printf("%s%s",text,cptr->name1);
		text = " ";
	    }
	}
	putchar('\n');
    }

    if ( keys & INFO_IMAGE_FORMAT )
	info_image_formats();

    putchar('\n');
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

