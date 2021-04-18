
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
 *   Copyright (c) 2009-2021 by Dirk Clemens <wiimm@wiimm.de>              *
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

#include <ctype.h>

#include "dclib/dclib-debug.h"
#include "version.h"
#include "dclib/dclib-types.h"
#include "lib-std.h"
#include "lib-sf.h"

#include "ui-wdf.c"
#include "logo.inc"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    variables			///////////////
///////////////////////////////////////////////////////////////////////////////

#define NAME "wdf"
#define TITLE " v" VERSION " r" REVISION " " SYSTEM2 " - " AUTHOR " - " DATE

///////////////////////////////////////////////////////////////////////////////

typedef enum FileMode { FMODE_WDF, FMODE_WIA, FMODE_CISO, FMODE_WBI } FileMode;
ccp file_mode_name[] = { ".wdf", ".wia", ".ciso", ".wbi", 0 };
enumOFT file_mode_oft[] = { OFT__WDF_DEF, OFT_WIA, OFT_CISO, OFT_CISO, 0 };

enumCommands the_cmd	= CMD_PACK;
FileMode file_mode	= FMODE_WDF;
FileMode def_file_mode	= FMODE_WDF;

bool opt_stdout		= false;
ccp  opt_suffix		= 0;
bool opt_chunk		= false;
int  opt_minus1		= 0;
bool opt_preserve	= false;
bool opt_keep		= false;

ccp progname0		= 0;
FILE * logout		= 0;

char progname_buf[20];
char default_settings[80];

enumOFT create_oft = OFT_PLAIN;
ccp extract_verb = "extract";
ccp copy_verb	 = "copy   ";

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    help()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void print_title ( FILE * f )
{
    static bool done = false;
    if (!done)
    {
	done = true;
	if ( verbose >= 1 && f == stdout )
	    fprintf(f,"\n%s%s\n\n",ProgInfo.progname,TITLE);
	else
	    fprintf(f,"*****  %s%s  *****\n",ProgInfo.progname,TITLE);
	fflush(f);
    }
}

///////////////////////////////////////////////////////////////////////////////

static void help_exit ( bool xmode )
{
    fputs( TITLE "\n", stdout );

    if (xmode)
    {
	int cmd;
	for ( cmd = 0; cmd < CMD__N; cmd++ )
	    PrintHelpCmd(&InfoUI_wdf,stdout,0,cmd,0,0,URI_HOME);
    }
    else
	PrintHelpCmd(&InfoUI_wdf,stdout,0,0,"HELP",0,URI_HOME);

    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

static void version_exit()
{
    if ( brief_count > 1 )
	fputs( VERSION "\n", stdout );
    else if (brief_count)
	fputs( VERSION " r" REVISION " " SYSTEM2 "\n", stdout );
    else 
	printf("%s%s\n%.*s",
		ProgInfo.progname, TITLE,
		(int)strlen(default_settings)-1, default_settings );
    exit(ERR_OK);
}

///////////////////////////////////////////////////////////////////////////////

static const KeywordTab_t * current_command = 0;

void hint_exit ( enumError stat )
{
    if ( current_command )
	fprintf(stderr,
	    "-> Type '%s help %s' (pipe it to a pager like 'less') for more help.\n\n",
	    ProgInfo.progname, CommandInfo[current_command->id].name1 );
    else
	fprintf(stderr,
	    "-> Type '%s -h' or '%s help' (pipe it to a pager like 'less') for more help.\n\n",
	    ProgInfo.progname, ProgInfo.progname );
    exit(stat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError OpenOutput
    ( SuperFile_t * fo, SuperFile_t * fi, ccp fname, ccp testmode_msg )
{
    PRINT("OpenOutput(%p,%p,%s,%s)\n",fo,fi,fname,testmode_msg);
    DASSERT(fo);

    if ( verbose > 0 )
	print_title(logout);

    if (!fname)
	fname = opt_dest;

    if (testmode)
    {
	if (testmode_msg)
	    fprintf(logout,"%s: %s\n",ProgInfo.progname,testmode_msg);
	fname = 0;
    }

    if (!fname)
	fname = "-";

    fo->f.create_directory = opt_mkdir;
    DASSERT( !fi || IsOpenSF(fi) );
    if ( fi && fi->iod.oft == OFT_UNKNOWN )
    {
	fo->src = fi;
	SetupIOD(fi,OFT_PLAIN,OFT_PLAIN);
	fi->file_size = fi->f.st.st_size;
    }

    enumError err = CreateSF(fo,fname,create_oft,IOM_FORCE_STREAM,opt_overwrite);
    if ( !err && opt_split && GetFileMode(fo->f.st.st_mode) == FM_PLAIN )
	err = SetupSplitWFile(&fo->f,fo->iod.oft,opt_split_size);

    TRACE("OpenOutput() returns %d\n",err);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseInOut ( SuperFile_t * fi, SuperFile_t * fo,
			enumError prev_err, bool remove_source )
{
    TRACE("CloseInOut(%p,%p,%d,%d)\n",fi,fo,prev_err,remove_source);
    DASSERT(fi);

    if (fo)
    {
	const enumError err = ResetSF( fo, opt_preserve ? &fi->f.fatt : 0 );
	if ( prev_err < err )
	     prev_err = err;
    }

    if ( !prev_err && remove_source && !testmode )
	prev_err = RemoveSF(fi);
    else
    {
	const enumError err = ResetSF(fi,0);
	if ( prev_err < err )
	     prev_err = err;
    }

    TRACE("CloseInOut() returns %d\n",prev_err);
    return prev_err;
}

///////////////////////////////////////////////////////////////////////////////

static char * RemoveExt ( char *buf, size_t bufsize, ccp * ext_list, ccp fname )
{
    DASSERT( buf );
    DASSERT( bufsize > 10 );
    DASSERT( ext_list );
    DASSERT( fname );

    size_t flen = strlen(fname);
    while ( *ext_list )
    {
	ccp ext = *ext_list++;
	const size_t elen = strlen(ext);
	if ( flen > elen && !strcasecmp(fname+flen-elen,ext) )
	{
	    flen -= elen;
	    if ( flen > bufsize-1 )
		 flen = bufsize-1;
	    StringCopyS(buf,flen+1,fname);
	    return buf;
	}
    }
    return (char*)fname;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cat helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CatRaw ( SuperFile_t * fi, SuperFile_t * fo,
			ccp out_fname, bool ignore_raw, bool remove_source )
{
    TRACE("CatRaw()\n");
    DASSERT(fi);
    DASSERT( fo || out_fname );

    enumError err = ERR_OK;
    SuperFile_t fo_local;

    if ( !ignore_raw )
    {
	if (testmode)
	{
	    if (out_fname)
		fprintf(logout," - WOULD %s file %s -> %s\n",
			copy_verb, fi->f.fname, out_fname );
	    else
		fprintf(logout," - WOULD %s file %s\n",copy_verb,fi->f.fname);
	}
	else
	{
	    if (!fo)
	    {
		if ( verbose > 0 )
		    fprintf(logout," - %s file %s -> %s\n",
				copy_verb, fi->f.fname, out_fname );
		fo = &fo_local;
		InitializeSF(fo);
		err = OpenOutput(fo,fi,out_fname,0);
	    }

	    if (!err)
	    {
		SetupIOD(fi,OFT_PLAIN,OFT_PLAIN);
		err = AppendSparseF(&fi->f,fo,0,fi->f.fatt.size);
	    }
	}
    }

    TRACELINE;
    return CloseInOut( fi, fo == &fo_local ? fo : 0, err, remove_source );
}

///////////////////////////////////////////////////////////////////////////////

enumError CatCISO ( SuperFile_t * fi, CISO_Head_t * ch, SuperFile_t * fo,
			ccp out_fname, bool ignore_raw, bool remove_source )
{
    TRACE("CatCISO()\n");
    DASSERT(fi);
    DASSERT(ch);
    DASSERT( fo || out_fname );

    CISO_Info_t ci;
    if (InitializeCISO(&ci,ch))
	return CatRaw(fi,fo,out_fname,ignore_raw,remove_source);

    char out_fname_buf[PATH_MAX];
    if (out_fname)
    {
	ccp ext_list[] = { ".ciso", ".wbi", opt_suffix, 0 };
	out_fname = RemoveExt(out_fname_buf,sizeof(out_fname_buf),ext_list,out_fname);
    }

    if (testmode)
    {
	if (out_fname)
	    fprintf(logout," - WOULD %s CISO %s -> %s\n",
			extract_verb, fi->f.fname, out_fname );
	else
	    fprintf(logout," - WOULD %s CISO %s\n",extract_verb,fi->f.fname);
	ResetSF(fi,0);
	return ERR_OK;
    }

    enumError err = ERR_OK;
    SuperFile_t fo_local;
    if (!fo)
    {
	if ( verbose > 0 )
	    fprintf(logout," - %s CISO %s -> %s\n",extract_verb,fi->f.fname,out_fname);
	fo = &fo_local;
	InitializeSF(fo);
	err = OpenOutput(fo,fi,out_fname,0);
    }

    if (!err)
    {
	const u64 block_size = ci.block_size;
	ResetCISO(&ci);

	u32 zero_count = 0;
	off_t read_off = CISO_HEAD_SIZE;
	const u8 *bptr = ch->map, *blast = ch->map;
	const u8 *bend = bptr + CISO_MAP_SIZE;
	while ( bptr < bend )
	    if (*bptr++)
	    {
		if (zero_count)
		{
		    AppendZeroSF(fo,zero_count*block_size);
		    zero_count = 0;
		}

		AppendSparseF(&fi->f,fo,read_off,block_size);
		read_off += block_size; 
		blast = bptr;
	    }
	    else
		zero_count++;

	if ( fo == &fo_local )
	{
	    const off_t off = (off_t)block_size * ( blast - ch->map );
	    err = SetSizeSF(fo,off);
	}
    }

    TRACELINE;
    return CloseInOut( fi, fo == &fo_local ? fo : 0, err, remove_source );
}

///////////////////////////////////////////////////////////////////////////////

enumError CatWDF ( ccp fname, SuperFile_t * fo, ccp out_fname,
			bool ignore_raw, bool remove_source )
{
    TRACE("CatWDF()\n");
    DASSERT(fname);
    DASSERT( fo || out_fname );

    SuperFile_t fi;
    InitializeSF(&fi);
    enumError err = OpenWFile(&fi.f,fname,IOM_IS_IMAGE);
    if (err)
	return err;

    err = SetupReadWDF(&fi);
    if ( err == ERR_NO_WDF )
    {
	fi.f.disable_errors = true;
	CISO_Head_t ch;
	err = ReadAtF(&fi.f,0,&ch,sizeof(ch));
	fi.f.disable_errors = false;
	if (   !err
	    && !memcmp(&ch.magic,"CISO",4)
	    && !*(ccp)&ch.block_size
	    && *ch.map == 1 )
	{
	    return CatCISO(&fi,&ch,fo,out_fname,ignore_raw,remove_source);
	}

	// normal cat
	return CatRaw(&fi,fo,out_fname,ignore_raw,remove_source);
    }

    char out_fname_buf[PATH_MAX];
    if (out_fname)
    {
	ccp ext_list[] = { ".wdf", opt_suffix, 0 };
	out_fname = RemoveExt(out_fname_buf,sizeof(out_fname_buf),ext_list,out_fname);
    }

    if (testmode)
    {
	if (out_fname)
	    fprintf(logout," - WOULD %s WDF  %s -> %s\n",extract_verb,fname,out_fname);
	else
	    fprintf(logout," - WOULD %s WDF  %s\n",extract_verb,fname);
	ResetSF(&fi,0);
	return ERR_OK;
    }

    //--- wdf

    SuperFile_t fo_local;
    if ( !err && !fo )
    {
	if ( verbose > 0 )
	    fprintf(logout," - %s WDF  %s -> %s\n",extract_verb,fname,out_fname);
	fo = &fo_local;
	InitializeSF(fo);
	err = OpenOutput(fo,0,out_fname,0);
    }

    if (!err)
    {
	// [[wdf]] [[wdf2]] optimization like c++ example

	wdf_controller_t *wdf = fi.wdf;
	if (!wdf)
	    ERROR0(ERR_INTERNAL,0);

	int i;
	u64 last_off = 0;
	for ( i=0; i < wdf->chunk_used; i++ )
	{
	    if ( SIGINT_level > 1 )
	    {
		err = ERR_INTERRUPT;
		break;
	    }

	    wdf2_chunk_t *wc = wdf->chunk + i;
	    u64 zero_count = wc->file_pos - last_off;
	    if ( zero_count > 0 )
	    {
		TRACE("WDF >> ZERO #%02d @%9llx [%9llx].\n",i,last_off,zero_count);
		last_off += zero_count;
		err = AppendZeroSF(fo,zero_count);
		if (err)
		    return err;
	    }

	    if ( wc->data_size )
	    {
		TRACE("WDF >> COPY #%02d @%9llx [%9llx] read-off=%9llx.\n",
			i, last_off, wc->data_size, wc->data_off );
		last_off += wc->data_size;
		err = AppendF(&fi.f,fo,wc->data_off,wc->data_size);
		if (err)
		    return err;
	    }
	}

	if ( fo == &fo_local )
	    err = SetSizeSF(fo,last_off);
    }

    TRACELINE;
    return CloseInOut( &fi, fo == &fo_local ? fo : 0, err, remove_source );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd cat			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_cat ( bool ignore_raw )
{
    SuperFile_t out;
    InitializeSF(&out);
    enumError err = OpenOutput(&out,0,0,"CONCATENATE files:");

    ParamList_t * param;
    for ( param = first_param; !err && param; param = param->next )
	err = CatWDF(param->arg,&out,0,ignore_raw,false);

    SetSizeSF(&out,out.file_size);
    ResetSF(&out,0);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd unpack			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_unpack()
{
    if (opt_stdout)
	return cmd_cat(true);

    if ( verbose > 0 )
	print_title(logout);

    char dest_buf[PATH_MAX];

    ParamList_t * param;
    enumError err = ERR_OK;
    for ( param = first_param; !err && param; param = param->next )
    {
	ccp dest = param->arg;
	if (opt_dest)
	{
	    PathCatPP(dest_buf,sizeof(dest_buf),opt_dest,dest);
	    dest = dest_buf;
	}
	err = CatWDF(param->arg,0,dest,true,!opt_keep);
    }
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cmd pack			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_pack()
{
    if ( verbose > 0 )
	print_title(logout);

    create_oft = file_mode_oft[file_mode];
    extract_verb = copy_verb = "pack";

    char dest[PATH_MAX];
    ccp suffix = opt_suffix ? opt_suffix : file_mode_name[file_mode];

    ParamList_t * param;
    enumError err = ERR_OK;
    for ( param = first_param; !err && param; param = param->next )
    {
	PathCatPPE(dest,sizeof(dest),opt_dest,param->arg,suffix);
	err = CatWDF(param->arg,0,dest,false,!opt_keep);
    }
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   cmd cmp			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError cmd_cmp() // cmd_diff()
{
    if ( n_param < 1 )
	return ERROR0(ERR_SYNTAX,"Missing source file!");
    if ( n_param > 2 )
	return ERROR0(ERR_SYNTAX,"To much source files!");

    ccp fname1, fname2;
    if ( n_param == 1 )
    {
	fname1 = "-";
	fname2 = first_param->arg;
    }
    else
    {
	fname1 = first_param->arg;
	fname2 = first_param->next->arg;
    }

    SuperFile_t f1, f2;
    InitializeSF(&f1);
    InitializeSF(&f2);

    enumError err;
    err = OpenSF(&f1,fname1,true,false);
    if (err)
	return err;
    err = OpenSF(&f2,fname2,true,false);
    if (err)
	return err;

    Diff_t diff;
    SetupDiff(&diff,long_count);
    OpenDiffSource(&diff,&f1,&f2,true);
    err = DiffRawSF(&diff);

    enumError err2 = CloseDiff(&diff);
    ResetSF(&f1,0);
    ResetSF(&f2,0);
    return err > err2 ? err : err2;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   cmd dump			///////////////
///////////////////////////////////////////////////////////////////////////////

static u64 prev_val = 0;

static int print_range ( FILE *f, ccp text, u64 end, u64 len )
{
    const int err = end - prev_val != len;
    if (err)
	fprintf(f,"    %-18s: %10llx .. %10llx [%10llx!=%10llx] INVALID!\n", \
		text, prev_val, end-opt_minus1, len, end-prev_val );
    else
	fprintf(f,"    %-18s: %10llx .. %10llx [%10llx]\n", \
		text, prev_val, end-opt_minus1, len );

    prev_val = end;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ciso_dump ( FILE *f, CISO_Head_t * ch, WFile_t *df, ccp fname )
{
    ASSERT(ch);
    ASSERT(df);
    ASSERT(fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump CISO %s\n",fname);
	ResetWFile(df,false);
	return ERR_OK;
    }

    fprintf(f,"\nCISO dump of file %s\n\n",fname);

    //----- collect data

    // block size is stored in untypical little endian
    u8 * bs = (u8*)&ch->block_size;
    const u32 block_size = ((( bs[3] << 8 ) | bs[2] ) << 8 | bs[1] ) << 8 | bs[0];

    int used_bl = 0;
    const u8 *bptr, *bend = ch->map + CISO_MAP_SIZE, *bmax = ch->map;
    for( bptr = ch->map; bptr < bend; bptr++ )
	if (*bptr)
	{
	    bmax = bptr;
	    used_bl++;
	}
    const int max_bl		= bmax - ch->map + 1;
    const u64 max_off		= (u64)used_bl * block_size + CISO_HEAD_SIZE;
    const u64 min_file_size	= (u64)WII_SECTORS_SINGLE_LAYER * WII_SECTOR_SIZE;
    const u64 iso_file_usage	= (u64)max_bl * block_size;
    const u64 iso_file_size	= iso_file_usage > min_file_size
				? iso_file_usage : min_file_size;

    //----- print header

    u8 * m = (u8*)ch->magic;
    fprintf(f,"  %-18s:         \"%s\" = %02x-%02x-%02x-%02x\n",
		"Magic", wd_print_id(m,4,0), m[0], m[1], m[2], m[3] );

    fprintf(f,"  %-18s: %10x/hex =%11d\n","Used blocks",used_bl,used_bl);
    fprintf(f,"  %-18s: %10x/hex =%11d\n","Max block",max_bl-1,max_bl-1);
    fprintf(f,"  %-18s: %10x/hex =%11d\n","Block size",block_size,block_size);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","Real file usage",max_off,max_off);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","Real file size",(u64)df->st.st_size,(u64)df->st.st_size);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","ISO file usage",iso_file_usage,iso_file_usage);
    fprintf(f,"  %-18s: %10llx/hex =%11lld\n","ISO file size",iso_file_size,iso_file_size);
    putchar('\n');

    //----- print mapping table

    if (opt_chunk)
    {
	fprintf(f,
	    "  Mapping Table:\n\n"
	    "       blocks    :       CISO file offset :     virtual ISO offset :      size\n"
	    "   ----------------------------------------------------------------------------\n");

	u32 bcount = 0;
	u64 bs64 = block_size;
	for( bptr = ch->map; ; )
	{
	    // find first block used
	    while ( bptr < bend && !*bptr )
		bptr++;
	    const int b1 = bptr - ch->map;

	    // find last block used
	    while ( bptr < bend && *bptr )
		bptr++;
	    const int b2 = bptr - ch->map;

	    if ( b1 >= b2 )
		break;

	    const u64 cfa = bcount * bs64 + CISO_HEAD_SIZE;
	    const u64 via = b1 * bs64;
	    const u64 siz = (b2-b1) * bs64;

	    if ( b1 == b2-1 )
		fprintf(f,"%11x      : %9llx .. %9llx : %9llx .. %9llx : %9llx\n",
		    b1, cfa, cfa+siz-opt_minus1, via, via+siz-opt_minus1, siz );
	    else
		fprintf(f,"%8x .. %4x : %9llx .. %9llx : %9llx .. %9llx : %9llx\n",
		    b1, b2-1, cfa, cfa+siz-opt_minus1, via, via+siz-opt_minus1, siz );

	    bcount += b2 - b1;
	}
	putchar('\n');
    }

    ResetWFile(df,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wia_dump ( FILE *f, WFile_t *df, ccp fname )
{
    ASSERT(df);
    ASSERT(fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump WIA %s\n",fname);
	ResetWFile(df,false);
	return ERR_OK;
    }

    fprintf(f,"\nWIA dump of file %s\n\n",fname);

    SuperFile_t sf;
    InitializeSF(&sf);
    memcpy(&sf.f,df,sizeof(WFile_t));
    InitializeWFile(df);
    SetupReadWIA(&sf);

    wia_controller_t * wia = sf.wia;
    ASSERT(wia);

    //-------------------------

    fprintf(f,"\n  File header:\n");

    u8 * m = (u8*)wia->fhead.magic;
    fprintf(f,"    %-23s: \"%s\"  %02x %02x %02x %02x\n",
		"Magic",
		wd_print_id(m,4,0), m[0], m[1], m[2], m[3] );
    fprintf(f,"    %-23s: 0x%08x => %s\n",
		"Version",
		wia->fhead.version,
		PrintVersionWIA(0,0,wia->fhead.version) );
    fprintf(f,"    %-23s: 0x%08x => %s\n",
		"Version/compatible",
		wia->fhead.version_compatible,
		PrintVersionWIA(0,0,wia->fhead.version_compatible) );

    if ( wia->fhead.disc_size == sizeof(wia_disc_t) )
	fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		"Size of disc section",
		wia->fhead.disc_size, wia->fhead.disc_size,
		wd_print_size_1024(0,0,wia->fhead.disc_size,true) );
    else
	fprintf(f,"    %-23s: %10u = %9x [current: %zu = %zx/hex]\n",
		"Size of disc section",
		wia->fhead.disc_size, wia->fhead.disc_size,
		sizeof(wia_disc_t), sizeof(wia_disc_t) );

    if (wia->fhead.iso_file_size)
    {
	fprintf(f,"    %-23s: %10llu =%10llx/hex = %s\n",
		"ISO image size",
		wia->fhead.iso_file_size, wia->fhead.iso_file_size,
		wd_print_size_1024(0,0,wia->fhead.iso_file_size,true) );
	double percent = 100.0 * wia->fhead.wia_file_size / wia->fhead.iso_file_size;
	fprintf(f,"    %-23s: %10llu =%10llx/hex = %s  %4.*f%%\n",
		"Total file size",
		wia->fhead.wia_file_size, wia->fhead.wia_file_size,
		wd_print_size_1024(0,0,wia->fhead.wia_file_size,true),
		percent <= 9.9 ? 2 : 1, percent );
    }
    else
	fprintf(f,"    %-23s: %10llu =%10llx/hex = %s\n",
		"Total file size",
		wia->fhead.wia_file_size, wia->fhead.wia_file_size,
		wd_print_size_1024(0,0,wia->fhead.wia_file_size,true) );

    //-------------------------

    fprintf(f,"\n  Disc header:\n");

    const wia_disc_t * disc = &wia->disc;

    if (disc->disc_type)
	fprintf(f,"    %-23s: %s, %.64s\n",
		"ID & title",
		wd_print_id(disc->dhead,6,0),
		disc->dhead + WII_TITLE_OFF );
    fprintf(f,"    %-23s: %10u = %s\n",
		"Disc type",
		disc->disc_type, wd_get_disc_type_name(disc->disc_type,"?") );
    fprintf(f,"    %-23s: %10u = %s\n",
		"Compression method",
		disc->compression, wd_get_compression_name(disc->compression,"?") );
    fprintf(f,"    %-23s: %10u\n",
		"Compression level", disc->compr_level);
    fprintf(f,"    %-23s: %10u =%10x/hex = %s\n",
		"Chunk size",
		disc->chunk_size, disc->chunk_size,
		wd_print_size_1024(0,0,disc->chunk_size,true) );

    fprintf(f,"    %-23s: %10u\n",
		" Number of partitions",
		disc->n_part );
    fprintf(f,"    %-23s: %10llu = %9llx/hex = %s\n",
		" Offset of Part.header",
		disc->part_off, disc->part_off,
		wd_print_size_1024(0,0,disc->part_off,true) );
    fprintf(f,"    %-23s: %2d * %5u = %9u\n",
		" Size of Part.header",
		disc->n_part, disc->part_t_size,
		disc->n_part * disc->part_t_size );

    fprintf(f,"    %-23s: %10u\n",
		"Num. of raw data elem.", disc->n_raw_data );
    fprintf(f,"    %-23s: %10llu = %9llx/hex = %s\n",
		"Offset of raw data tab.",
		disc->raw_data_off, disc->raw_data_off,
		wd_print_size_1024(0,0,disc->raw_data_off,true) );
    fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		"Size of raw data tab.",
		disc->raw_data_size, disc->raw_data_size,
		wd_print_size_1024(0,0,disc->raw_data_size,true) );

    fprintf(f,"    %-23s: %10u\n",
		" Num. of group elem.", disc->n_groups );
    fprintf(f,"    %-23s: %10llu = %9llx/hex = %s\n",
		" Offset of group tab.",
		disc->group_off, disc->group_off,
		wd_print_size_1024(0,0,disc->group_off,true) );
    fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		" Size of group tab.",
		disc->group_size, disc->group_size,
		wd_print_size_1024(0,0,disc->group_size,true) );

    fprintf(f,"    %-23s: %10u = %9x/hex = %s\n",
		"Compressor data length",
		disc->compr_data_len, disc->compr_data_len,
		wd_print_size_1024(0,0,disc->compr_data_len,true) );
    if (disc->compr_data_len)
    {
	fprintf(f,"    %-23s:","Compressor Data");
	int i;
	for ( i = 0; i < disc->compr_data_len; i++ )
	    fprintf(f," %02x",disc->compr_data[i]);
	fputc('\n',f);
    }

    //-------------------------

    if ( long_count > 0 )
    {
	fprintf(f,"\n  ISO memory map:\n");
	wd_print_memmap(stdout,3,&wia->memmap);
	putchar('\n');
    }

    //-------------------------

    fputc('\n',f);
    CloseSF(&sf,0);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError gcz_dump ( FILE *f, GCZ_Head_t * gh1, WFile_t *df, ccp fname )
{
    ASSERT(gh1);
    ASSERT(df);
    ASSERT(fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump GCZ %s\n",fname);
	ResetWFile(df,false);
	return ERR_OK;
    }

    GCZ_Head_t gh;
    const bool valid = IsValidGCZ(gh1,sizeof(*gh1),df->st.st_size,&gh);

    fprintf(f,"\nGCZ dump of file %s\n\n",fname);
    if (!valid)
	fprintf(f,"  ** Invalid image! **\n\n");
    else if ( gh.block_size != GCZ_DEF_BLOCK_SIZE )
	fprintf(f,"  ** Not standard block size (%s)! **\n\n",
		wd_print_size_1024(0,0,gh.block_size,false) );

    fprintf(f,
	"  Magic:             %08x\n"
	"  GCZ type:        %10u\n"
	"  Compressed size: %10llu = %9llx/hex\n"
	"  Image size:      %10llu = %9llx/hex\n"
	"  Block size:      %10u = %9x/hex\n"
	"  Number of blocks:%10u = %9x/hex\n"
	,gh.magic
	,gh.sub_type
	,gh.compr_size ,gh.compr_size
	,gh.image_size ,gh.image_size
	,gh.block_size ,gh.block_size
	,gh.num_blocks ,gh.num_blocks
	);

    const uint list_size = 12 * gh.num_blocks;
    u64 *offset = MALLOC(list_size);
    //u32 *checksum = (u32*)(offset+gh.num_blocks);
    enumError stat = ReadAtF(df,sizeof(gh),offset,list_size);
    if (stat)
	return stat;

    u64 compr_size = 0;
    uint blk, n_compr = 0, n_zero = 0;
    for ( blk = 0; blk < gh.num_blocks; blk++ )
    {
	s64 blk_off  = le64(offset+blk);
	u32 blk_size = ( blk == gh.num_blocks - 1
				? gh.compr_size
				: le64(offset+blk+1) ) - blk_off;

	if ( blk_size == 39 && gh.block_size == GCZ_DEF_BLOCK_SIZE )
	{
	    n_zero++;
	    //printf("zero: %5x %u\n",blk,blk_size);
	}
	else if ( blk_off >= 0 )
	{
	    n_compr++;
	    compr_size += blk_size;
	    //printf("compr: %5x\n",blk);
	}
    }

    uint n_uncompr = gh.num_blocks - n_compr - n_zero;
    uint ave = n_compr ? compr_size /  n_compr : 0;
    fprintf(f,"     uncompressed: %10u = %9x/hex\n",n_uncompr,n_uncompr);
    if (n_zero)
	fprintf(f,"     zero filled:  %10u = %9x/hex\n",n_zero,n_zero);
    fprintf(f,
	"     compressed:   %10u = %9x/hex\n"
	"  Average compr.size:%8u = %9x/hex\n"
	"\n"
	,n_compr ,n_compr
	,ave, ave
	);

    ResetWFile(df,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static u64 PrintAlignWC ( FILE *f, ccp title, u64 num )
{
    u64 m;
    if (num)
    {
	m = 1;
	while (!(num&m))
	    m <<= 1;
    }
    else
	m = 0;

    fprintf(f,"    %-17s : %s\n",
		title,
		m ? wd_print_size_1024(0,0,m,true) : "   0" );
    return m;
}

///////////////////////////////////////////////////////////////////////////////

enumError wdf_dump ( FILE *f, ccp fname )
{
    ASSERT(fname);

    WFile_t df;
    InitializeWFile(&df);
    enumError err = OpenWFile(&df,fname,IOM_IS_IMAGE);
    if (err)
	return err;

    wdf_header_t wh;
    const int CISO_MAGIC_SIZE = 4;
    ASSERT( CISO_MAGIC_SIZE < sizeof(wh) );
    err = ReadAtF(&df,0,&wh,CISO_MAGIC_SIZE);
    if (err)
    {
	ResetWFile(&df,false);
	return err;
    }

    if (!memcmp(&wh,"CISO",CISO_MAGIC_SIZE))
    {
	CISO_Head_t ch;
	memcpy(&ch,&wh,CISO_MAGIC_SIZE);
	err = ReadAtF(&df,CISO_MAGIC_SIZE,(char*)&ch+CISO_MAGIC_SIZE,sizeof(ch)-CISO_MAGIC_SIZE);
	if (err)
	{
	    ResetWFile(&df,false);
	    return err;
	}
	return ciso_dump(f,&ch,&df,fname);
    }

    if ( le32(&wh) == GCZ_MAGIC_NUM )
    {
	GCZ_Head_t gh;
	const uint magic_size = 4;
	memcpy(&gh,&wh,magic_size);
	err = ReadAtF(&df,magic_size,(char*)&gh+magic_size,sizeof(gh)-magic_size);
	if (err)
	{
	    ResetWFile(&df,false);
	    return err;
	}
	return gcz_dump(f,&gh,&df,fname);
    }

    if (!memcmp(&wh,WIA_MAGIC,WIA_MAGIC_SIZE))
	return wia_dump(f,&df,fname);

    if (testmode)
    {
	fprintf(f," - WOULD dump WDF  %s\n",fname);
	ResetWFile(&df,false);
	return ERR_OK;
    }

    //---------------

    fprintf(f,"\nWDF dump of file %s\n",fname);

    err = ReadAtF(&df,CISO_MAGIC_SIZE,(char*)&wh+CISO_MAGIC_SIZE,
			sizeof(wh)-CISO_MAGIC_SIZE);
    if (err)
    {
	ResetWFile(&df,false);
	return err;
    }

    wdf_header_t wh_fixed;
    ConvertToHostWH(&wh,&wh);
    const bool fixed = FixHeaderWDF(&wh_fixed,&wh,true);
    AnalyzeWH(&df,&wh,false); // needed for splitting support!

    fprintf(f,"\n  File header:\n");

    u8 * m = (u8*)wh.magic;
    fprintf(f,"    %-18s: \"%s\"  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
		"Magic",
		wd_print_id(m,8,0),
		m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7] );

    if (memcmp(wh.magic,WDF_MAGIC,WDF_MAGIC_SIZE))
    {
	ResetWFile(&df,false);
	return ERROR0(ERR_WDF_INVALID,"Wrong magic: %s\n",fname);
    }

    #undef PRINT32
    #undef PRINT64
    #undef PRINT32F
    #define PRINT32(a,t) fprintf(f,"    %-18s: %10x/hex =%11d\n",t,wh.a,wh.a)
    #define PRINT64(a,t) fprintf(f,"    %-18s: %10llx/hex =%11lld\n",t,wh.a,wh.a)
    #define PRINT32F(a,t) if (wh_fixed.a!=wh.a) \
		fprintf(f,"    %-18s: %10x/hex =%11d\n",t,wh_fixed.a,wh_fixed.a)

    PRINT32(wdf_version,	"WDF version");
    PRINT32(wdf_compatible,	"WDF compatible");
    PRINT32(head_size,		"Head size");
    PRINT32(align_factor,	"Alignment factor");
    PRINT64(file_size,		"Image size");
    fprintf(f,"    %-18s: %10llx/hex =%11lld  %4.2f%%\n",
		"  WDF file size ",
		(u64)df.st.st_size, (u64)df.st.st_size,
		100.0 * df.st.st_size / wh.file_size );
    PRINT64(data_size,	"Data size");
    PRINT32(reserved,	"-reserved-");
    PRINT32(chunk_n,	"Number of chunks");
    PRINT64(chunk_off,	"Chunk data offset");

    const uint chunk_entry_size = GetChunkSizeWDF(wh.wdf_version);
    const uint chunk_table_size = chunk_entry_size * wh.chunk_n;

    fprintf(f,"    %-18s: %10x/hex =%11d\n",
		" Chunk entry size", chunk_entry_size, chunk_entry_size );
    fprintf(f,"    %-18s: %10x/hex =%11d\n",
		" Chunk table size", chunk_table_size, chunk_table_size );

    //--------------------------------------------------

    if (fixed)
    {
	fprintf(f,"\n  Auto fixed header:\n");

	PRINT32F(wdf_version,	"WDF version");
	PRINT32F(wdf_compatible,"WDF compatible");
	PRINT32F(head_size,	"Head size");
	PRINT32F(align_factor,	"Alignment factor");
	PRINT32F(reserved,	"-reserved-");

	memcpy(&wh,&wh_fixed,sizeof(wh));
    }

    //--------------------------------------------------

 #if 0
    fprintf(f,"\n  Memory map of file:%21s%18s\n","offset range","[ data size]");


    int ec = 0; // error count
    prev_val = 0;

    ec += print_range(f, "Header",	wh.head_size,			wh.head_size );
    ec += print_range(f, "Data",	wh.chunk_off,			wh.data_size );
    ec += print_range(f, "Chunk magic",	wh.chunk_off+WDF_MAGIC_SIZE,	WDF_MAGIC_SIZE );
    ec += print_range(f, "Chunk table",	df.st.st_size,			chunk_table_size );
    fputc('\n',f);

    if (ec)
    {
	ResetWFile(&df,false);
	return ERROR0(ERR_WDF_INVALID,"Invalid data: %s\n",fname);
    }
 #endif

    if ( wh.chunk_n > 1000000 )
    {
	ResetWFile(&df,false);
	return ERROR0(ERR_INTERNAL,"Too many chunk entries: %s\n",fname);
    }

    //--------------------------------------------------

    MemMap_t mm;
    InitializeMemMap(&mm);
    MemMapItem_t *mi = InsertMemMap(&mm,0,wh.head_size);
    snprintf(mi->info,sizeof(mi->info),"WDF header v%u",wh.wdf_version);

    char magic[WDF_MAGIC_SIZE];
    err = ReadAtF(&df,wh.chunk_off,magic,sizeof(magic));
    if (err)
    {
	ResetWFile(&df,false);
	return ERROR0(ERR_READ_FAILED,"ReadF error: %s\n",fname);
    }
    mi = InsertMemMap(&mm,wh.chunk_off,sizeof(magic));
    StringCopyS(mi->info,sizeof(mi->info),"magic of chunk list");

    if (memcmp(magic,WDF_MAGIC,WDF_MAGIC_SIZE))
    {
	ResetWFile(&df,false);
	return ERROR0(ERR_WDF_INVALID,"Wrong chunk table magic: %s\n",fname);
    }

    u8 *wc_data =  MALLOC(chunk_table_size);
    err = ReadF(&df,wc_data,chunk_table_size);
    if (err)
    {
	ResetWFile(&df,false);
	FREE(wc_data);
	return ERROR0(ERR_READ_FAILED,"Can't read chunk table: %s\n",fname);
    }
    mi = InsertMemMap(&mm,wh.chunk_off+sizeof(magic),chunk_table_size);
    snprintf(mi->info,sizeof(mi->info),"%u chunk elements",wh.chunk_n);
    u64 eof = mi->off + mi->size;

    wdf2_chunk_t *wc = (wdf2_chunk_t*)wc_data;
    ConvertToHostWC(wc,wc_data,wh.wdf_version,wh.chunk_n);

    u64 align_data_off = 0, align_data_size = 0, align_file_pos = 0;
    u64 max_data = wh.head_size, min_data = wh.head_size;
    int idx;
    wdf2_chunk_t *w;
    for ( idx = 0, w = wc; idx < wh.chunk_n; idx++, w++ )
    {
	align_data_off  |= w->data_off;
	align_data_size |= w->data_size;
	align_file_pos  |= w->file_pos;
	
	if ( !idx || min_data > w->data_off )
	     min_data = w->data_off;
	const u64 data_end = w->data_off + w->data_size;
	if ( max_data < data_end )
	     max_data = data_end;
    }
    mi = InsertMemMap(&mm,min_data,max_data-min_data);
    snprintf(mi->info,sizeof(mi->info),"%u chunk data blocks",wh.chunk_n);

    //--------------------------------------------------
    // split file?

    if ( eof < max_data )
	eof = max_data;

    if ( eof > df.st.st_size )
	SetupSplitWFile(&df,OFT_WDF2,eof);

    char ch;
    err = ReadAtF(&df,max_data-1,&ch,1);
    if (err)
    {
	ResetWFile(&df,false);
	FREE(wc_data);
	return ERROR0(ERR_READ_FAILED,"Can't read last data byte: %s\n",fname);
    }

    mi = InsertMemMap(&mm,df.st.st_size,0);
    if ( df.split_used > 1 )
	snprintf(mi->info,sizeof(mi->info),
	    "-- end of %u joined files --", df.split_used );
    else
	StringCopyS(mi->info,sizeof(mi->info),"-- end of file --");


    //--------------------------------------------------

    fprintf(f,"\n  Data alignments:\n");
    PrintAlignWC(f,"WDF header",	wh.align_factor);
    PrintAlignWC(f,"Calculated",	align_data_off | align_data_size | align_file_pos );
    PrintAlignWC(f," Data offset+size",	align_data_off | align_data_size );
    PrintAlignWC(f,"  Data offset",	align_data_off );
    PrintAlignWC(f,"  Data size",	align_data_size  );
    PrintAlignWC(f," File offset+size",	align_file_pos | align_data_size );
    PrintAlignWC(f,"  File offset",	align_file_pos );

    uint n_overlap = CalcOverlapMemMap(&mm);
    fprintf(f,"\n  Memory map of file (hex values)%s:\n\n",
		n_overlap ? " => INVALID!" : "" );
    PrintMemMap(&mm,f,4,0);
    fputc('\n',f);

    //--------------------------------------------------

    if (opt_chunk)
    {
	fprintf(f,
	    "  Chunk Table (hex values):\n\n"
	    "    idx        WDF file offset  data len     virtual ISO offset  hole size\n"
	    "   ------------------------------------------------------------------------\n");

	int idx;
	wdf2_chunk_t *w;
	for ( idx = 0, w = wc; idx < wh.chunk_n; idx++, w++ )
	{
	    fprintf(f,"%6d. %10llx..%10llx %9llx %10llx..%10llx  %9llx\n",
		    idx,
		    w->data_off, w->data_off + w->data_size-opt_minus1,
		    w->data_size,
		    w->file_pos, w->file_pos + w->data_size - opt_minus1,
		    idx+1 < wh.chunk_n
			    ? w[1].file_pos - w->file_pos - w->data_size : 0llu );
	}
	fputc('\n',f);
    }

    FREE(wc_data);
    ResetWFile(&df,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError cmd_dump()
{
    SuperFile_t sf;
    InitializeSF(&sf);
    enumError err = OpenOutput(&sf,0,0,"DUMP WDF and CISO data strutures:");

    ParamList_t * param;
    for ( param = first_param; !err && param; param = param->next )
	err = wdf_dump(sf.f.fp,param->arg);

    ResetSF(&sf,0);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CheckOptions()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckOptions ( int argc, char ** argv )
{
    int err = 0;
    for(;;)
    {
      const int opt_stat = getopt_long(argc,argv,OptionShort,OptionLong,0);
      if ( opt_stat == -1 )
	break;

      RegisterOptionByName(&InfoUI_wdf,opt_stat,1,false);

      switch ((enumGetOpt)opt_stat)
      {
	case GO__ERR:		err++; break;

	case GO_VERSION:	version_exit();
	case GO_HELP:		help_exit(false);
	case GO_XHELP:		help_exit(true);
	case GO_WIDTH:		err += ScanOptWidth(optarg); break;
	case GO_QUIET:		verbose = verbose > -1 ? -1 : verbose - 1; break;
	case GO_VERBOSE:	verbose = verbose <  0 ?  0 : verbose + 1; break;
	case GO_LOGGING:	logging++; break;
	case GO_COLOR:		err += ScanOptColorize(0,optarg,0); break;
	case GO_COLOR_256:	opt_colorize = COLMD_256_COLORS; break;
	case GO_NO_COLOR:	opt_colorize = -1; break;
	case GO_IO:		ScanIOMode(optarg); break;
	case GO_DSYNC:		err += ScanOptDSync(optarg); break;
	case GO_CHUNK:		opt_chunk = true; break;
	case GO_LONG:		opt_chunk = true; long_count++; break;
	case GO_MINUS1:		opt_minus1 = 1; break;

	//case GO_WDF:		file_mode = FMODE_WDF; break; // [[wdf2]]
	case GO_WDF:		file_mode = FMODE_WDF; SetModeWDF(0,optarg); break;
	case GO_WDF1:		file_mode = FMODE_WDF; SetModeWDF(1,optarg); break;
	case GO_WDF2:		file_mode = FMODE_WDF; SetModeWDF(2,optarg); break;
	case GO_ALIGN_WDF:	err += ScanOptAlignWDF(optarg,0); break;

	case GO_WIA:		file_mode = FMODE_WIA;
					err += ScanOptCompression(false,optarg);
					break;
	case GO_CISO:		file_mode = FMODE_CISO; break;
	case GO_WBI:		file_mode = FMODE_WBI; break;
	case GO_SUFFIX:		opt_suffix = optarg; break;

	case GO_DEST:		SetDest(optarg,false); break;
	case GO_DEST2:		SetDest(optarg,true); break;
	case GO_STDOUT:		opt_stdout = true; break;
	case GO_KEEP:		opt_keep = true; break;
	case GO_OVERWRITE:	opt_overwrite = true; break;
	case GO_PRESERVE:	opt_preserve = true; break;

	case GO_AUTO_SPLIT:	opt_auto_split = 2; opt_split = 0; break;
	case GO_NO_SPLIT:	opt_auto_split = opt_split = 0; break;
	case GO_SPLIT:		opt_split++; break;
	case GO_SPLIT_SIZE:	err += ScanOptSplitSize(optarg); break;
	case GO_PREALLOC:	err += ScanPreallocMode(optarg); break;
	case GO_CHUNK_MODE:	err += ScanChunkMode(optarg); break;
	case GO_CHUNK_SIZE:	err += ScanChunkSize(optarg); break;
	case GO_MAX_CHUNKS:	err += ScanMaxChunks(optarg); break;
	case GO_COMPRESSION:	err += ScanOptCompression(false,optarg); break;
	case GO_MEM:		err += ScanOptMem(optarg,true); break;

	case GO_TEST:		testmode++; break;

#if OPT_OLD_NEW
	case GO_OLD:		newmode = -1; break;
	case GO_NEW:		newmode = +1; break;
#endif

	case GO_LIMIT:
	    {
		u32 limit;
		if (ScanSizeOptU32(&limit,optarg,1,0,"limit",0,INT_MAX,0,0,true))
		    err++;
		else
		    opt_limit = limit;
	    }
	    break;

	case GO_FILE_LIMIT:
	    {
		u32 file_limit;
		if (ScanSizeOptU32(&file_limit,optarg,1,0,"fil-limit",0,INT_MAX,0,0,true))
		    err++;
		else
		    opt_file_limit = file_limit;
	    }
	    break;

	case GO_BLOCK_SIZE:
	    {
		u32 block_size;
		if (ScanSizeOptU32(&block_size,optarg,1,0,"block-size",0,INT_MAX,0,0,true))
		    err++;
		else
		    opt_block_size = block_size;
	    }
	    break;
      }
    }
    SetupColors();

 #ifdef DEBUG
    DumpUsedOptions(&InfoUI_wdf,TRACE_FILE,11);
 #endif

    return err ? ERR_SYNTAX : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  CheckCommand()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckCommand ( int argc, char ** argv )
{
    argc -= optind;
    argv += optind;

    TRACE("CheckCommand(%d,) optind=%d\n",argc+optind,optind);

    if ( argc > 0 && **argv == '+' )
    {
	int cmd_stat;
	ccp arg = *argv;
	const KeywordTab_t * cmd_ct = ScanKeyword(&cmd_stat,arg,CommandTab);

	if ( !cmd_ct && cmd_stat < 2 && toupper((int)arg[1]) == 'C' )
	{
	    char abuf[20];
	    abuf[0] = '+';
	    StringCopyS(abuf+1,sizeof(abuf)-1,arg[2] == '-' ? arg+3 : arg+2 );
	    cmd_ct = ScanKeyword(&cmd_stat,abuf,CommandTab);
	    if (cmd_ct)
	    {
		if (!opt_colorize)
		    opt_colorize = COLMD_ON;
		SetupColors();
	    }
	}

	if (!cmd_ct)
	{
	    PrintKeywordError(CommandTab,*argv,cmd_stat,0,0);
	    hint_exit(ERR_SYNTAX);
	}

	TRACE("COMMAND FOUND: #%lld = %s\n",cmd_ct->id,cmd_ct->name1);
	current_command = cmd_ct;

	the_cmd = cmd_ct->id;
	argc--;
	argv++;
    }
    else
    {
	const KeywordTab_t * cmd_ct;
	for ( cmd_ct = CommandTab; cmd_ct->name1; cmd_ct++ )
	    if ( cmd_ct->id == the_cmd )
	    {
		current_command = cmd_ct;
		break;
	    }
    }

    if (current_command)
    {
	enumError err = VerifySpecificOptions(&InfoUI_wdf,current_command);
	if (err)
	    hint_exit(err);
    }

    while ( argc-- > 0 )
	AtFileHelper(*argv++,false,true,AddParam);

    enumError err = ERR_OK;
    switch (the_cmd)
    {
	case CMD_VERSION:	version_exit();
	case CMD_HELP:		PrintHelp(&InfoUI_wdf,stdout,0,"+HELP",default_settings,
					URI_HOME, first_param ? first_param->arg : 0 );
				break;

	case CMD_PACK:		err = cmd_pack(); break;
	case CMD_UNPACK:	err = cmd_unpack(); break;
	case CMD_CAT:		err = cmd_cat(false); break;
	case CMD_CMP:		err = cmd_cmp(); break;
	case CMD_DUMP:		err = cmd_dump(); break;

	// no default case defined
	//	=> compiler checks the existence of all enum values

	case CMD__NONE:
	case CMD__N:
	    help_exit(false);
    }

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    main()			///////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    logout = stdout;
    SetupLib(argc,argv,NAME,PROG_WDF);
    opt_chunk_mode = CHUNK_MODE_32KIB;
    progname0 = ProgInfo.progname;

    //----- setup default options

    if (ProgInfo.progname)
    {
	ccp src = ProgInfo.progname;
	char *dest = iobuf, *end = iobuf + sizeof(iobuf) - 1;
	while ( *src && dest < end )
	    *dest++ = tolower((int)*src++);
	*dest = 0;
	TRACE("PROGNAME/LOWER: %s\n",iobuf);

	if ( strstr(iobuf,"dump") )
	    the_cmd = CMD_DUMP;
	else if ( strstr(iobuf,"cmp") || strstr(iobuf,"diff") )
	    the_cmd = CMD_CMP;
	else if ( strstr(iobuf,"cat") )
	    the_cmd = CMD_CAT;
	else if ( !memcmp(iobuf,"un",2) )
	    the_cmd = CMD_UNPACK;

	if (strstr(iobuf,"wia"))
	    file_mode = def_file_mode = FMODE_WIA;
	else if (strstr(iobuf,"ciso"))
	    file_mode = def_file_mode = FMODE_CISO;
	else if (strstr(iobuf,"wbi"))
	    file_mode = def_file_mode = FMODE_WBI;
    }

    ccp fmname = file_mode_name[def_file_mode] + 1;
    switch(the_cmd)
    {
	case CMD_DUMP:
	    snprintf(progname_buf,sizeof(progname_buf), "%s-dump", fmname );
	    break;

	case CMD_CMP:
	    snprintf(progname_buf,sizeof(progname_buf), "%s-cmp", fmname );
	    break;

	case CMD_CAT:
	    snprintf(progname_buf,sizeof(progname_buf), "%s-cat", fmname );
	    break;

	case CMD_UNPACK:
	    snprintf(progname_buf,sizeof(progname_buf), "un%s", fmname );
	    break;

	default:
	    snprintf(progname_buf,sizeof(progname_buf), "%s", fmname );
	    break;
    }
    ProgInfo.progname = progname_buf;

    snprintf( default_settings, sizeof(default_settings),
		"Default settings for '%s': %s --%s\n\n",
		progname0, CommandInfo[the_cmd].name1, fmname );

    //----- process arguments

    if ( argc < 2 )
    {
	printf("\n%s\n%s%s\nVisit %s%s for more info.\n\n",
		text_logo, ProgInfo.progname, TITLE, URI_HOME, NAME );
	hint_exit(ERR_OK);
    }

    if (CheckOptions(argc,argv))
	hint_exit(ERR_SYNTAX);

    if ( opt_stdout )
	logout = stderr;

    //----- cmd selector

    int err = CheckCommand(argc,argv);
    if (err)
	return err;


    //----- terminate

    CloseAll();

    if (SIGINT_level)
	ERROR0(ERR_INTERRUPT,"Program interrupted by user.");
    return err > ProgInfo.max_error ? err : ProgInfo.max_error;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

