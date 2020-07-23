
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
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "dclib/dclib-debug.h"
#include "dclib/dclib-file.h"

#include "iso-interface.h"
#include "wbfs-interface.h"
#include "titles.h"
#include "match-pattern.h"
#include "dirent.h"
#include "crypt.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 dump helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static void dump_data
	( FILE * f, int indent, off_t base, u32 off4, off_t size, ccp text )
{
    const u64 off = (off_t)off4 << 2;
    const u64 end = off + size;
    fprintf(f,"%*s"
	"    %-5s %9llx .. %9llx -> %9llx .. %9llx, size:%10llx/hex =%11llu\n",
	indent,"", text, off, end,
	(u64)base+off, (u64)base+end, (u64)size, (u64)size );
}

//-----------------------------------------------------------------------------

static void dump_size
(
    FILE		* f,		// output stream
    int			indent,		// indent
    ccp			title,		// header
    u64			size,		// size to print
    u64			percent_base1,	// >0: print percent value based on 'percent_base'
    u64			percent_base2,	// >0: print percent value based on 'percent_base'
    ccp			append_text	// NULL or text to append
)
{
    fprintf(f,"%*s%-17s%11llx/hex =%11llu",
	indent,"", title, size, size );

    if ( size >= 10240 )
	fprintf(f," = %s",wd_print_size_1024(0,0,size,true));

    if ( percent_base1 > 0 )
    {
	double percent = size * 100.0 / percent_base1;
	if ( percent >= 9.95 )
	    fprintf(f,", %4.1f%%",percent);
	else
	    fprintf(f,", %4.2f%%",percent);
    }

    if ( percent_base2 > 0 )
    {
	double percent = size * 100.0 / percent_base2;
	if ( percent >= 9.95 )
	    fprintf(f,", %4.1f%%",percent);
	else
	    fprintf(f,", %4.2f%%",percent);
    }

    if (append_text)
	fputs(append_text,f);

    fputc('\n',f);
}

//-----------------------------------------------------------------------------

static void dump_sys_version
(
    FILE		* f,		// output stream
    int			indent,		// indent
    u64			sys_version,	// system version to print
    int			title_fw	// field width of title
)
{
    const u32 hi = sys_version >> 32;
    const u32 lo = (u32)sys_version;
    if ( hi == 1 && lo < 0x100 )
	fprintf(f,"%*s%-*s%08x-%08x = IOS 0x%02x = IOS %u\n",
		    indent, "", title_fw, "System version: ", hi, lo, lo, lo );
    else
	fprintf(f,"%*s%-*s%08x-%08x\n",
		    indent, "", title_fw, "System version: ", hi, lo );
}

//-----------------------------------------------------------------------------

static void dump_id
(
    FILE		* f,		// output stream
    int			indent,		// indent
    ccp			disc_id6,	// NULL or disc id
    ccp			wbfs_id6,	// NULL or wbfs id
    wd_part_t		* part,		// not NULL: retrieve IDs from partition
    int			title_fw	// field width of title
)
{
    PRINT("dump_id() id=%s,%s, part=%p\n",disc_id6,wbfs_id6,part);

    char wbfs[20] = {0};
    if ( wbfs_id6 && *wbfs_id6 )
	snprintf(wbfs,sizeof(wbfs),", wbfs=%s",wd_print_id(wbfs_id6,6,0));

    if (part)
    {
	if ( disc_id6 && *disc_id6 )
	    fprintf(f,"%*s%-*sdisc=%s, ticket=%s, tmd=%s, boot=%s%s\n",
		indent,"", title_fw, "Disc & part IDs: ",
		wd_print_id(disc_id6,6,0),
		wd_print_id(part->ph.ticket.title_id+4,4,0),
		part->tmd ? wd_print_id(part->tmd->title_id+4,4,0) : "-",
		wd_print_id(&part->boot,6,0), wbfs );
	else
	    fprintf(f,"%*s%-*sticket=%s, tmd=%s, boot=%s%s\n",
		indent,"", title_fw, "Partition IDs: ",
		wd_print_id(part->ph.ticket.title_id+4,4,0),
		part->tmd ? wd_print_id(part->tmd->title_id+4,4,0) : "-",
		wd_print_id(&part->boot,6,0), wbfs );
    }
    else if ( disc_id6 && *disc_id6 )
	fprintf(f,"%*s%-*s%s%s\n",
		indent,"",  title_fw, "Disc ID: ",
		wd_print_id(disc_id6,6,0), wbfs );
}

//-----------------------------------------------------------------------------

static int dump_header
(
    FILE		* f,		// output stream
    int			indent,		// indent
    SuperFile_t		* sf,		// file to dump
    ccp			id6,		// NULL or id6
    ccp			real_path,	// NULL or pointer to real path
    wd_disc_t		* disc		// not NULL: retrieve info from disc
)
{
    ASSERT(f);
    ASSERT(sf);
    indent = NormalizeIndent(indent);

    fprintf(f,"\n%*sDump of file %s\n\n",indent,"",sf->f.fname);
    indent += 2;
    if ( real_path && *real_path && strcmp(sf->f.fname,real_path) )
	fprintf(f,"%*sReal path:         %s\n",indent,"",real_path);

    const u64 virt_size = sf->file_size;
    const u64 real_size = sf->f.st.st_size;
    if ( sf->iod.oft != OFT_UNKNOWN && sf->disc2 )
    {
	if ( sf->iod.oft == OFT_PLAIN )
	    dump_size(f,indent,"ISO file size:",real_size,0,0,0);
	else
	    dump_size(f,indent,"Virtual size:",virt_size,0,0,0);

	char buf[50];
	const u32 n_blocks = wd_count_used_disc_blocks(sf->disc2,1,0);
	snprintf(buf,sizeof(buf),", %u*%uK",n_blocks,WII_SECTOR_SIZE/KiB);
	const u64 scrubbed_size = n_blocks * (u64)WII_SECTOR_SIZE;
	dump_size(f,indent,"Scrubbed size:",scrubbed_size,virt_size,0,buf);

	if ( sf->iod.oft != OFT_PLAIN )
	{
	    snprintf(buf,sizeof(buf),"%s file size:",oft_info[sf->iod.oft].name);
	    dump_size(f,indent,buf,real_size,virt_size,scrubbed_size,0);
	}
	if (sf->wbfs_fragments)
	    fprintf(f,"%*sWBFS fragments:    1+%u"
			" = a ratio of %4.2f additional_fragments/GiB\n",
			indent,"", sf->wbfs_fragments-1,
			(sf->wbfs_fragments-1) * (double)(GiB) / scrubbed_size );
    }
    else
	dump_size(f,indent,"File size:",real_size,0,0,0);

    //----- file and disc type

    char ftype_info[30] = {0};
    if (sf->wia)
    {
	wia_disc_t * disc = &sf->wia->disc;
	if (disc)
	    snprintf( ftype_info, sizeof(ftype_info), " (v%s,%s)",
		    PrintVersionWIA(0,0,sf->wia->fhead.version),
		    wd_print_compression(0,0,disc->compression,
		    disc->compr_level,disc->chunk_size,2) );
    }

    if (disc)
    {
	fprintf(f,"%*sFile & disc type:  %s%s  &  %s%s%s%s\n",
		indent,"",
		GetNameFT(sf->f.ftype,0), ftype_info,
		wd_get_disc_type_name(disc->disc_type,"?"),
		disc->disc_attrib & WD_DA_GC_MULTIBOOT ? ", MultiBoot" : "",
		disc->disc_attrib & WD_DA_GC_START_PART ? "+" : "",
		disc->disc_attrib & WD_DA_GC_DVD9 ? ", DVD9" : "" );

	if ( !id6 && sf )
	    id6 = sf->f.id6_dest;
	dump_id(f,indent,id6,sf->wbfs_id6,disc->main_part,19);
    }
    else
	fprintf(f,"%*sFile type:         %s%s\n",
		indent,"", GetNameFT(sf->f.ftype,0), ftype_info );

    return indent;
}

//-----------------------------------------------------------------------------

static void dump_sig_type
(
    FILE		* f,		// output stream
    int			indent,		// indent
    u32			sig_type,	// signature or public key type
    cert_stat_t		sig_status,	// not NULL: cert status of signature
    bool		is_pubkey	// true: not sig but public key
)
{
    const int size = is_pubkey
			? cert_get_pubkey_size(sig_type)
			: cert_get_signature_size(sig_type);

    fprintf(f,"%*s%-17s%11x/hex =%11u  [%s, size=%u%s%s]\n",
		indent, "",
		is_pubkey ? "Public key type:" : "Signature type:",
		sig_type, sig_type,
		cert_get_signature_name(sig_type|0x10000,"?"),
		size,
		sig_status ? ", " : "",
		sig_status ? cert_get_status_name(sig_status,"?") : "" );

 #if 0
    if (sig_status)
	fprintf(f,"%*s%-19s%s\n",
		indent, "",
		is_pubkey ? "Public key status:" : "Signature status:",
		cert_get_status_message(sig_status,"?") );
 #endif
}

//-----------------------------------------------------------------------------

static void dump_hex
(
    FILE		* f,		// output file
    const void		* p_data,	// pointer to data
    size_t		dsize,		// size of data, must be <100
    size_t		asc_indent,	// if >0: print ":...:" with entered indent
    ccp			tail		// print this test and end of line (eg. LF)
)
{
    DASSERT(f);
    const u8 * data = p_data;
    DASSERT(p_data);
    char buf[100];
    ASSERT(dsize < sizeof(buf) );

    int count = 0, fw = 0;
    while ( dsize-- > 0 )
    {
	if ( count && !(count&3) )
	{
	    fputc(' ',f);
	    fw++;
	}
	buf[count++] = *data >= ' ' && *data < 0x7f ? *data : '.';
	fprintf(f,"%02x",*data++);
	fw += 2;
    }

    if ( asc_indent > 0 )
    {
	buf[count] = 0;
	fprintf(f,"%*s= '%s'%s",(int)(asc_indent-fw), "", buf, tail ? tail : "" );
    }
    else if (tail)
	fputs(tail,f);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_ISO()			///////////////
///////////////////////////////////////////////////////////////////////////////

static void dump_gc_part
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    wd_disc_t		* disc,		// disc pointer
    ShowMode		show_mode	// what should be printed
)
{
    DASSERT(f);
    DASSERT(disc);

    if ( !( disc->disc_attrib & WD_DA_GC_MULTIBOOT ) )
	return;

    //----- partitions

    if ( show_mode & SHOW_P_INFO && disc->n_part )
    {
	wd_part_t * part = disc->part;
	DASSERT(part);

	int i;
	u32 np = disc->n_part;
	for ( i = 0; i < np; i++, part++ )
	{
	    if ( !part->is_valid || !part->is_enabled )
		continue;

	    fprintf(f,"\n%*sGameCube partition #%d, offset 0x%llx, size %s:\n",
		indent,"", part->index,
		(u64)part->part_off4 << 2,
		wd_print_size_1024(0,0,part->part_size,false));

	    if (part->is_overlay)
		fprintf(f,"%*s  Partition overlays other partitions.\n", indent,"" );

	    fprintf(f,"%*s  Partition type:   %s\n",
			indent, "",
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO) );
	    fprintf(f,"%*s  boot.bin, title:  %.64s\n",
			indent, "", part->boot.dhead.disc_title);

	    ccp title = GetTitle(&part->boot.dhead.disc_id,0);
	    if (title)
		fprintf(f,"%*s  Title DB:         %s\n", indent, "", title );

	    fprintf(f,"%*s  Region:        %7u [%s]\n",indent,"",
			part->region, GetRegionName(part->region,"?") );

	    fprintf(f,"%*s  Directories:   %7u\n",indent,"",part->fst_dir_count);
	    fprintf(f,"%*s  Files:         %7u\n",indent,"",part->fst_file_count);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

static void dump_wii_part
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    wd_disc_t		* disc,		// disc pointer
    ShowMode		show_mode	// what should be printed
)
{
    DASSERT(f);
    DASSERT(disc);

    int i;
    u32 nt = disc->n_ptab;
    u32 np = disc->n_part;

    char pname[100];

    if ( show_mode & SHOW_P_TAB && wd_disc_has_ptab(disc) )
    {
	ccp sep = "  -------------------------------------------"
		  "---------------------------------------------";

	fprintf(f,"\n%*s%d partition table%s with %d partition%s%s:\n\n"
	    "%*s  index      type    offset ..   end off   size/hex =   size/dec =  MiB  status\n"
	    "%*s%s\n",
		indent,"", nt, nt == 1 ? "" : "s",
		np, np == 1 ? "" : "s",
		disc->patch_ptab_recommended ? " (patching recommended)" : "",
		indent,"", indent,"", sep );

	wd_ptab_info_t * pc = disc->ptab_info;
	for ( i = 0; i < WII_MAX_PTAB; i++, pc++ )
	{
	    const u32 np = ntohl(pc->n_part);
	    if (np)
	    {
		const u64 off  = (u64)ntohl(pc->off4) << 2;
		const u32 size = np * sizeof(wd_ptab_entry_t);
		fprintf(f,
		    "%*s%4d     part.tab%10llx ..%10llx %10x =%11u         %u partition%s\n",
		    indent,"", i, off, off+size, size, size,
		    np, np == 1 ? "" : "s" );
	    }
	}

	fprintf(f,"%*s%s\n",indent,"",sep);

	for ( i = 0; i < np; i++ )
	{
	    wd_part_t * part = disc->part + i;
	    wd_print_part_name(pname,sizeof(pname),part->part_type,WD_PNAME_COLUMN_9);

	    const u64 off  = (u64)part->part_off4 << 2;

	    if (part->is_valid)
	    {
		const u64 size = part->part_size;
		fprintf(f,"%*s%4d.%-2d %s %9llx ..%10llx %10llx =%11llu =%5llu  %s\n",
		    indent,"", part->ptab_index, part->ptab_part_index,
		    pname, off, off + size, size, size, (size+MiB/2)/MiB,
		    wd_print_part_status(0,0,part,true) );
	    }
	    else
		fprintf(f,"%*s%4d.%-2d %s %9llx         "
			  "** INVALID PARTITION **               invalid\n",
		    indent,"", part->ptab_index, part->ptab_part_index,
		    pname, off );
	}

	fprintf(f,"%*s%s\n",indent,"",sep);
    }


    //----- partitions

    if ( show_mode & SHOW__PART && disc->n_part && wd_disc_has_ptab(disc) )
    {
      for ( i = 0; i < np; i++ )
      {
	wd_part_t * part = disc->part + i;
	if ( !part->is_valid || !part->is_enabled )
	    continue;

	const wd_part_header_t * ph = &part->ph;

	wd_print_part_name(pname,sizeof(pname),part->part_type,WD_PNAME_NUM_INFO);
	fprintf(f,"\n%*sPartition table #%d, partition #%d, type %s:\n",
		indent,"", part->ptab_index, part->ptab_part_index, pname );

	if ( show_mode & SHOW_P_INFO )
	{
	    if (part->is_overlay)
		fprintf(f,"%*s  Partition overlays other partitions.\n", indent,"" );

	    if ( tmd_is_marked_not_encrypted(part->tmd) )
		fprintf(f,"%*s  Partition is marked as 'not encrypted'.\n", indent,"" );
	    else
		fprintf(f,"%*s  %s\n", indent,"", wd_print_sig_status(0,0,part,false,true) );

	    u8 * p8 = part->key;
	    fprintf(f,"%*s  Partition key:    %02x%02x%02x%02x %02x%02x%02x%02x"
					 " %02x%02x%02x%02x %02x%02x%02x%02x\n",
		indent,"",
		p8[0], p8[1], p8[2], p8[3], p8[4], p8[5], p8[6], p8[7],
		p8[8], p8[9], p8[10], p8[11], p8[12], p8[13], p8[14], p8[15] );

	    if ( !(show_mode & SHOW_F_PRIMARY) )
	    {
		dump_id(f,indent+2,0,0,part,18);
		fprintf(f,"%*s  boot.bin, title:  %.64s\n",
			indent, "", part->boot.dhead.disc_title);
		if ( !(show_mode & SHOW_TMD) && part->tmd )
		    dump_sys_version(f,indent+2,ntoh64(part->tmd->sys_version),18);
	    }

	    fprintf(f,"%*s  Directories:   %7u\n",indent,"",part->fst_dir_count);
	    fprintf(f,"%*s  Files:         %7u\n",indent,"",part->fst_file_count);
	}
	else if ( show_mode & SHOW_P_ID )
	    dump_id(f,indent+2,0,0,part,18);

	if ( show_mode & SHOW_P_MAP )
	{
	    const u64 off = (u64) part->part_off4 << 2;
	    dump_data(f,indent, off, 0,		    sizeof(*ph),	"TIK:" );
	    dump_data(f,indent, off, ph->tmd_off4,  ph->tmd_size,	"TMD:" );
	    dump_data(f,indent, off, ph->cert_off4, ph->cert_size,	"CERT:" );
	    dump_data(f,indent, off, ph->h3_off4,   WII_H3_SIZE,	"H3:" );
	    dump_data(f,indent, off, ph->data_off4, (u64)ph->data_size4<<2, "Data:" );
	}

	if ( show_mode & SHOW_CERT )
	    Dump_CERT(f,indent+2,wd_load_cert_chain(part,true),0);

	if ( show_mode & SHOW_TICKET )
	{
	    fprintf(f,"%*s  Ticket:\n",indent,"");
	    Dump_TIK_MEM(f,indent+4,&ph->ticket,wd_get_cert_ticket_stat(part,false));
	}

	if ( show_mode & SHOW_TMD && part->tmd )
	{
	    fprintf(f,"%*s  TMD:\n",indent,"");
	    Dump_TMD_MEM(f,indent+4,part->tmd,100,wd_get_cert_tmd_stat(part,false));
	}
      }
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_ISO
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode,	// what should be printed
    int			dump_level	// dump level: 0..3, ignored if show_mode is set
)
{
    //----- setup

    ASSERT(sf);
    if ( !f || !sf->f.id6_dest[0] )
	return ERR_OK;
    sf->f.read_behind_eof = 2;

    wd_disc_t * disc = OpenDiscSF(sf,true,true);
    if ( show_mode & SHOW_F_PRIMARY )
	disc = sf->disc1;
    if (!disc)
	return ERR_WDISC_NOT_FOUND;

    const bool is_gc = disc->disc_type == WD_DT_GAMECUBE;

    FilePattern_t * pat = GetDefaultFilePattern();
    indent = NormalizeIndent(indent) + 2;


    //----- options --show and --long

    if ( show_mode & SHOW__DEFAULT )
    {
	show_mode = SHOW__ALL;
	switch (dump_level)
	{
	    case 0:  
		show_mode = SHOW_INTRO | SHOW_P_TAB;
		break;

	    case 1:
		show_mode &= ~(SHOW_D_MAP|SHOW_USAGE|SHOW_TICKET|SHOW_TMD);
		// fall through

	    case 2:  
		show_mode &= ~SHOW_FILES;
		// fall through

	    case 3:
		show_mode &= ~(SHOW_PATCH|SHOW_RELOCATE);
		break;
	}

	show_mode |= SHOW_F_HEAD;
	if (pat->is_active)
	    show_mode |= SHOW_FILES;
    }

    if ( show_mode & SHOW_F_PRIMARY )
	show_mode &= ~(SHOW_TICKET|SHOW_TMD); // TICKET and TMD data is patched


    //----- print header

    if ( show_mode & SHOW_INTRO )
    {
	dump_header(f,indent-2,sf,&disc->dhead.disc_id,real_path,disc);

	fprintf(f,"%*sDisc name:         %.64s\n",indent,"",disc->dhead.disc_title);
	ccp title = GetTitle((ccp)&disc->dhead,0);
	if (title)
	    fprintf(f,"%*sDB title:          %s\n",indent,"",title);

	const RegionInfo_t * rinfo = GetRegionInfo(disc->dhead.region_code);
	fprintf(f,"%*sID Region:         %s [%s]\n",indent,"",rinfo->name,rinfo->name4);

	if (is_gc)
	{
	    const enumRegion reg = ntohl(disc->region.region);
	    fprintf(f,"%*sBI2 Region:        %x [%s]\n",
			indent,"", reg, GetRegionName(reg,"?") );
	}
	else    
	{
	    const enumRegion reg = ntohl(disc->region.region);
	    fprintf(f,"%*sRegion setting:    %x [%s] / %s\n",
			indent,"", reg, GetRegionName(reg,"?"),
			wd_print_age_rating(0,0,disc->region.age_rating) );

	    if (disc)
	    {
		if (disc->system_menu)
		    fprintf(f,"%*sSystem menu:       v%u = %s\n",
			indent,"", disc->system_menu,
			GetSystemMenu(disc->system_menu,"?") );

		if ( disc->main_part && disc->main_part->tmd )
		    dump_sys_version(f,indent,ntoh64(disc->main_part->tmd->sys_version),19);
	    }
	}

	wd_calc_disc_status(disc,true);
	fprintf(f,"%*sPartitions:       %7u  [%s]\n",
		indent,"", disc->n_part,
		wd_print_sector_status(0,0,disc->sector_stat,disc->cert_summary) );
	fprintf(f,"%*sDirectories:      %7u\n",indent,"",disc->fst_dir_count);
	fprintf(f,"%*sFiles:            %7u\n",indent,"",disc->fst_file_count);
    }
    else
    {
	fprintf(f,"\n%*sDump of file %s\n",indent-2,"",sf->f.fname);
	if ( show_mode & SHOW_D_ID )
	    dump_id(f,indent,&disc->dhead.disc_id,sf->wbfs_id6,disc->main_part,19);
    }


    //----- partition tables & partitions

    if ( disc->disc_type == WD_DT_GAMECUBE )
	dump_gc_part(f,indent,disc,show_mode);
    else
	dump_wii_part(f,indent,disc,show_mode);


    //----- FST (files)

    if ( show_mode & SHOW_FILES )
    {
	SetupFilePattern(pat);

	const u32 used_mib = ( wd_count_used_disc_blocks(disc,1,0)
				+ WII_SECTORS_PER_MIB/2 ) / WII_SECTORS_PER_MIB;

	fprintf(f,"\n\n%*s%u director%s with %u file%s, disk usage %u MiB:\n\n",
		indent,"",
		disc->fst_dir_count, disc->fst_dir_count==1 ? "y" : "ies",
		disc->fst_file_count, disc->fst_file_count==1 ? "" : "s",
		used_mib );

	wd_print_fst( f, indent+2, disc,
		prefix_mode == WD_IPM_DEFAULT ? WD_IPM_PART_NAME : prefix_mode,
		ConvertShow2PFST(show_mode,SHOW__ALL&~SHOW_UNUSED) | WD_PFST_PART,
		pat->match_all ? 0 : MatchFilePatternFST, pat );
    }


    //----- disc usage

    if ( show_mode & SHOW_USAGE )
    {
	fprintf(f,"\n\n%*sSector Usage Map:\n\n",indent,"");
	wd_filter_usage_table(disc,wdisc_usage_tab,0);
	wd_print_usage_tab(f,indent+2,wdisc_usage_tab,disc->iso_size,false);
    }


    //----- memory map

    if ( show_mode & SHOW_D_MAP )
    {
	MemMap_t mm;
	InitializeMemMap(&mm);
	InsertDiscMemMap(&mm,disc);
	fprintf(f,"\n\n%*sISO Memory Map:\n\n",indent,"");
	PrintMemMap(&mm,f,indent+2,0);
	ResetMemMap(&mm);
    }


    if ( sf->disc1 && sf->disc1 != sf->disc2 )
    {
	//----- patching tables

	if ( show_mode & SHOW_PATCH )
	{
	    putc('\n',f);
	    wd_print_disc_patch(f,indent,sf->disc1,true,true);
	}


	//----- relocating table

	if ( show_mode & SHOW_RELOCATE )
	{
	    putc('\n',f);
	    wd_print_disc_relocation(f,indent,sf->disc1,true);
	}
    }


    //----- terminate

    if (disc->invalid_part)
    {
	fprintf(f,"\n\nWARNING: Disc contains %u invalid partition%s!\n\n",
		disc->invalid_part, disc->invalid_part == 1 ? "" : "s" );
	return ERR_WPART_INVALID;
    }

    putc('\n',f);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_DOL()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_DOL
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    uint		dump_mode	// bit field:
					//  1: print header
					//  2: print file position table
					//  4: print virtual position table
					//  8: print virtual-to-file translation table
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    if ( dump_mode & 1 )
	indent = dump_header(f,indent,sf,0,real_path,0);
    else
	indent = NormalizeIndent(indent);

    dol_header_t dol;
    enumError err = ReadSF(sf,0,&dol,sizeof(dol));
    if (!err)
    {
	ntoh_dol_header(&dol,&dol);

	MemMap_t mm1, mm2, mm3;
	InitializeMemMap(&mm1);
	InitializeMemMap(&mm2);
	InitializeMemMap(&mm3);
	MemMapItem_t * mi = InsertMemMap(&mm1,0,sizeof(dol_header_t));
	StringCopyS(mi->info,sizeof(mi->info),"DOL header");
	mi = InsertMemMap(&mm1,sf->file_size,0);
	StringCopyS(mi->info,sizeof(mi->info),"--- end of file ---");

	int i;
	for ( i = 0; i < DOL_N_SECTIONS; i++ )
	{
	    const u32 size = dol.sect_size[i];
	    if (size)
	    {
		const u32 off  = dol.sect_off[i];
		const u32 addr = dol.sect_addr[i];

		char buf[sizeof(mi->info)], buf3[sizeof(mi->info)];
		if ( i < DOL_N_TEXT_SECTIONS )
		{
		    snprintf(buf,sizeof(buf),"text section T%u",i);
		    snprintf(buf3,sizeof(buf3),"%8x : T%u",addr-off,i);
		}
		else
		{
		    const int j = i - DOL_N_TEXT_SECTIONS;
		    snprintf(buf,sizeof(buf),"data section D%u",j);
		    snprintf(buf3,sizeof(buf3),"%8x : D%u",addr-off,j);
		}

		mi = InsertMemMap(&mm1,off,size);
		strcpy(mi->info,buf);
		mi = InsertMemMap(&mm2,addr,size);
		strcpy(mi->info,buf);
		mi = InsertMemMap(&mm3,addr,size);
		strcpy(mi->info,buf3);
	    }
	}

	if ( dump_mode & 2 )
	{
	    fprintf(f,"\n%*sMemory map of DOL file:\n\n",indent,"");
	    PrintMemMap(&mm1,f,indent+3,"section");
	}

	if ( dump_mode & 4 )
	{
	    mi = InsertMemMap(&mm2,dol.bss_addr,dol.bss_size);
	    snprintf(mi->info,sizeof(mi->info),"bss section");
	    mi = InsertMemMap(&mm2,dol.entry_addr,0);
	    snprintf(mi->info,sizeof(mi->info),"entry point");

	    fprintf(f,"\n%*sMemory map of DOL image:\n\n",indent,"");
	    mm2.begin = 0xffffffff;
	    PrintMemMap(&mm2,f,indent+3,"section");
	}

	if ( dump_mode & 8 )
	{
	    mm3.begin = 0xffffffff;
	    fprintf(f,"\n%*sDelta between file offset and virtual address:\n\n",
			indent,"" );
	    PrintMemMap(&mm3,f,indent+3,"   delta : section");
	}

	ResetMemMap(&mm1);
	ResetMemMap(&mm2);
	ResetMemMap(&mm3);
    }
    putc('\n',f);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		Dump_CERT_BIN() + Dump_CERT() 		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_CERT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    int			print_ext	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    indent = dump_header(f,indent,sf,0,real_path,0);

    u8 * buf = MALLOC(sf->file_size);

    enumError err = ReadSF(sf,0,buf,sf->file_size);
    if (!err)
    {
	putc('\n',f);
	Dump_CERT_MEM(f,indent,buf,sf->file_size,print_ext);
    }
    FREE(buf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_CERT_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const u8		* cert_data,	// valid pointer to cert data
    size_t		cert_size,	// size of 'cert_data'
    int			print_ext	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
)
{
    ASSERT(f);
    ASSERT(cert_data);
    indent = NormalizeIndent(indent);

    cert_chain_t cc;
    cert_initialize(&cc);
    cert_append_data(&cc,cert_data,cert_size,false);
    enumError err = Dump_CERT(f,indent,&cc,print_ext);
    cert_reset(&cc);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_CERT
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const cert_chain_t	* cc,		// valid pinter to cert chain
    int			print_ext	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
)
{
    DASSERT(f);
    DASSERT(cc);
    indent = NormalizeIndent(indent);

    if (!cc->used)
	fprintf(f,"%*sNo certificates found!%s\n",
		indent, "", print_ext ? "\n" : "" );

    int i;
    for ( i = 0; i < cc->used; i++ )
    {
	Dump_CERT_Item(f,indent,cc->cert+i,i,print_ext,cc);
	if (print_ext)
	    fputc('\n',f);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

void Dump_CERT_Item
(
    FILE		* f,		// valid output stream
    int			indent,		// normalized indent of output
    const cert_item_t	* item,		// valid item pointer
    int			cert_index,	// >=0: print title with certificate index
    int			print_ext,	// 0: off
					// 1: print extended version
					// 2:  + hexdump the keys
					// 3:  + base64 the keys
    const cert_chain_t	* cc		// not NULL: verify signature
)
{
    if ( cert_index >= 0)
    {
	fprintf(f,"%*sCertificate #%u, size=0x%x=%u\n",
		indent, "", cert_index,
		item->cert_size, item->cert_size );
	indent += 2;
    }

    fprintf(f,"%*sIssuer & Key ID:   %s\n", indent, "", item->name );

    if (item->head)
    {
	const cert_stat_t stat = cc ? cert_check_cert(cc,item,0) : 0;
	dump_sig_type(f,indent,ntohl(item->head->sig_type),stat,false);
    }
    dump_sig_type(f,indent,ntohl(item->data->key_type),0,true);

    if (print_ext)
    {
	const u32 pub_exp = be32(item->data->public_key+item->key_size);
	fprintf(f,"%*sPublic exponent: %11x/hex =%11u\n",indent,"",pub_exp,pub_exp);

	if (print_ext>1)
	{
	    fprintf(f,"%*sPublic key:\n",indent,"");
	    if ( print_ext > 2 )
		PrintEncode64(f,item->data->public_key,item->key_size,indent+5,70);
	    else
		HexDump(f,indent+4,0,4,-16,item->data->public_key,item->key_size);
	}
	else
	{
	    fprintf(f,"%*sPublic key:        ",indent,"");
	    dump_hex(f,item->data->public_key,16,0," ...\n");
	}

	if (item->head)
	{
	    if (print_ext>1)
	    {
		fprintf(f,"%*sSignature:\n",indent,"");
		if ( print_ext > 2 )
		    PrintEncode64(f,item->head->sig_data,item->sig_size,indent+5,70);
		else
		    HexDump(f,indent+4,0,4,-16,item->head->sig_data,item->sig_size);
	    }
	    else
	    {
		fprintf(f,"%*sSignature:         ",indent,"");
		dump_hex(f,item->head->sig_data,16,0," ...\n");
	    }
	}
    }

    if (item->head)
    {
	u8 hash[WII_HASH_SIZE];
	SHA1((u8*)item->data,item->data_size,hash);
	fprintf(f,"%*sRelevant SHA1:     ",indent,"");
	dump_hex(f,hash,20,0,"\n");
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		Dump_TIK_BIN() + Dump_TIK() 		///////////////
///////////////////////////////////////////////////////////////////////////////

static int try_load_cert
(
    cert_chain_t	* cc,		// valid pointer to uninitialized data
    ccp			filename	// filename of source
)
{
    cert_initialize(cc);

    char * pathend = strrchr(filename,'/');
    if (pathend)
    {
	const int len = pathend - filename;
	memcpy(iobuf,filename,len);
	pathend = iobuf + len;
    }
    else
	pathend = iobuf;
    strcpy(pathend,"/cert.bin");
    TRACE("CHECK %s\n",iobuf);
    return cert_append_file(cc,iobuf,false);
}

//-----------------------------------------------------------------------------

enumError Dump_TIK_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    char buf[10000];
    const size_t load_size = sf->file_size < sizeof(buf) ? sf->file_size : sizeof(buf);
    enumError err = ReadSF(sf,0,buf,load_size);
    wd_ticket_t * tik = (wd_ticket_t*)buf;
    if (!err)
	SetFileID(&sf->f,tik->title_id+4,4);
    indent = dump_header(f,indent,sf,0,real_path,0);

    if (!err)
    {
	if ( sizeof(wd_ticket_t) > load_size )
	    err = ERROR0(ERR_WARNING,"Not a valid TMD file: %s\n",sf->f.fname);
	else
	{
	    putc('\n',f);
	    cert_chain_t cc;

	    if ( sizeof(wd_ticket_t) < load_size )
	    {
		cert_initialize(&cc);
		cert_append_data(&cc,buf+sizeof(wd_ticket_t),
			load_size-sizeof(wd_ticket_t),false);
	    }
	    else
		try_load_cert(&cc,sf->f.fname);

	    Dump_TIK_MEM(f,indent,tik,cert_check_ticket(&cc,tik,0));
	    if ( sizeof(wd_ticket_t) < load_size )
	    {
		putc('\n',f);
		Dump_CERT(f,indent,&cc,0);
	    }
	    cert_reset(&cc);
	}
    } 

    putc('\n',f);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_TIK_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_ticket_t	* tik,		// valid pointer to ticket
    cert_stat_t		sig_status	// not NULL: cert status of signature
)
{
    ASSERT(f);
    ASSERT(tik);
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sIssuer:            %s\n", indent,"", tik->issuer );

    dump_sig_type(f,indent,ntohl(tik->sig_type),sig_status,false);
    if ( !tik->sig && !memcmp(tik->sig,tik->sig+1,sizeof(tik->sig)-1) )
	fprintf(f,"\n%*sSignature is cleared (all zero)\n", indent,"" );

    u32 val = ntohs(tik->n_dlc);
    fprintf(f,"%*sN(DLC):          %11x/hex =%11u\n", indent,"", val,val );
    val = tik->common_key_index;
    fprintf(f,"%*sCommon key index:%11u     = '%s'\n",
		indent,"", val, !val ? "standard" : val == 1 ? "korean" : "?" );
    val = ntohl(tik->time_limit);
    fprintf(f,"%*sTime limit:      %11x/hex =%11u [%sabled]\n",
		indent,"", val,val, ntohl(tik->enable_time_limit) ? "en" : "dis" );

    fprintf(f,"%*sTitle key:         ",indent,"");
    dump_hex(f,tik->title_key,sizeof(tik->title_key),0,"\n");

    u8 key[WII_KEY_SIZE];
    wd_decrypt_title_key(tik,key);
    fprintf(f,"%*s Decrypted key:    ",indent,"");
    dump_hex(f,key,sizeof(key),0,"\n");

    fprintf(f,"%*sTicket ID:         ",indent,"");
    dump_hex(f,tik->ticket_id,sizeof(tik->ticket_id),18,"\n");
    fprintf(f,"%*sConsole ID:        ",indent,"");
    dump_hex(f,tik->console_id,sizeof(tik->console_id),18,"\n");
    fprintf(f,"%*sTitle ID:          ",indent,"");
    dump_hex(f,tik->title_id,sizeof(tik->title_id),18,"\n");

    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)tik)+WII_TICKET_SIG_OFF, sizeof(*tik)-WII_TICKET_SIG_OFF, hash );
    fprintf(f,"%*sRelevant SHA1:     ",indent,"");
    dump_hex(f,hash,20,0,"\n");

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		Dump_TMD_BIN() + Dump_TMD() 		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_TMD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    char buf[10000];
    const size_t load_size = sf->file_size < sizeof(buf) ? sf->file_size : sizeof(buf);
    enumError err = ReadSF(sf,0,buf,load_size);

    wd_tmd_t * tmd = (wd_tmd_t*)buf;
    if (!err)
	SetFileID(&sf->f,tmd->title_id+4,4);
    indent = dump_header(f,indent,sf,0,real_path,0);

    if (!err)
    {
	const int n_content = ntohs(tmd->n_content);
	const int tmd_size = sizeof(wd_tmd_t) + n_content * sizeof(wd_tmd_content_t);
	if ( tmd_size > load_size )
	    err = ERROR0(ERR_WARNING,"Not a valid TMD file: %s\n",sf->f.fname);
	else
	{
	    putc('\n',f);
	    cert_chain_t cc;

	    if ( tmd_size < load_size )
	    {
		cert_initialize(&cc);
		cert_append_data(&cc,buf+tmd_size,load_size-tmd_size,false);
	    }
	    else
		try_load_cert(&cc,sf->f.fname);

	    Dump_TMD_MEM(f,indent,tmd,n_content,cert_check_tmd(&cc,tmd,0));
	    if ( tmd_size < load_size )
	    {
		putc('\n',f);
		Dump_CERT(f,indent,&cc,0);
	    }
	    cert_reset(&cc);
	}
    }

    putc('\n',f);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_TMD_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_tmd_t	* tmd,		// valid pointer to ticket
    int			n_content,	// number of loaded wd_tmd_content_t elements
    cert_stat_t		sig_status	// not NULL: cert status of signature
)
{
    ASSERT(f);
    ASSERT(tmd);
    ASSERT( n_content >= 0 );
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sIssuer:            %s\n", indent,"", tmd->issuer );

    dump_sig_type(f,indent,ntohl(tmd->sig_type),sig_status,false);
    if ( !tmd->sig && !memcmp(tmd->sig,tmd->sig+1,sizeof(tmd->sig)-1) )
	fprintf(f,"\n%*sSignature is cleared (all zero)\n", indent,"" );

    fprintf(f,"%*sVersion:         %11u\n", indent, "", tmd->version);
    fprintf(f,"%*sCA version:      %11u\n", indent, "", tmd->ca_crl_version);
    fprintf(f,"%*sSigner version:  %11u\n", indent, "", tmd->signer_crl_version);

    u32 high = be32((ccp)&tmd->sys_version);
    u32 low  = be32(((ccp)&tmd->sys_version)+4);
    if ( high == 1 && low < 0x100 )
	fprintf(f,"%*sSystem version:    %08x %08x = IOS 0x%02x = IOS %u\n",
		indent, "", high, low, low, low );
    else
	fprintf(f,"%*sSystem version:    %08x:%08x\n", indent, "", high, low );
    fprintf(f,"%*sTitle ID:          ",indent,"");
    dump_hex(f,tmd->title_id,sizeof(tmd->title_id),18,"\n");

    u32 val = ntohl(tmd->title_type);
    fprintf(f,"%*sTitle type:     %11x/hex =%11u\n", indent,"", val, val );
    val = ntohs(tmd->group_id);
    fprintf(f,"%*sGroup ID:       %11x/hex =%11u\n", indent,"", val, val );
    val = ntohl(tmd->access_rights);
    fprintf(f,"%*sAccess rights:  %11x/hex =%11u\n", indent,"", val, val );
    val = ntohs(tmd->title_version);
    fprintf(f,"%*sTitle version:  %11x/hex =%11u\n", indent,"", val, val );
    val = ntohs(tmd->boot_index);
    fprintf(f,"%*sBoot index:     %11x/hex =%11u\n", indent,"", val, val );

    val = ntohs(tmd->n_content);
    fprintf(f,"%*sN(content):     %11x/hex =%11u\n", indent,"", val, val );
    if ( n_content > val )
	n_content = val;

    int i;
    for ( i = 0; i < n_content; i++ )
    {
	const wd_tmd_content_t * c = tmd->content + i;
	val = ntohl(c->content_id);
	fprintf(f,"%*sContent #%u, ID:   %9x/hex =%11u\n", indent,"", i, val, val );
	val = ntohs(c->index);
	fprintf(f,"%*sContent #%u, index:%9x/hex =%11u\n", indent,"", i, val, val );
	val = ntohs(c->type);
	fprintf(f,"%*sContent #%u, type: %9x/hex =%11u\n", indent,"", i, val, val );
	u64 val64 = ntoh64(c->size);
	fprintf(f,"%*sContent #%u, size:%10llx/hex =%11llu\n", indent,"", i, val64, val64 );
	fprintf(f,"%*sContent #%u, hash:  ",indent,"",i);
	dump_hex(f,c->hash,sizeof(c->hash),0,"\n");
    }

    u8 hash[WII_HASH_SIZE];
    const int tmd_size = sizeof(wd_tmd_t) + n_content * sizeof(wd_tmd_content_t);
    SHA1( ((u8*)tmd)+WII_TMD_SIG_OFF, tmd_size-WII_TMD_SIG_OFF, hash );
    fprintf(f,"%*sRelevant SHA1:     ",indent,"");
    dump_hex(f,hash,20,0,"\n");

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_HEAD_BIN()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_HEAD_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;

    indent = dump_header(f,indent,sf,0,real_path,0);

    wd_header_t head;
    enumError err = ReadSF(sf,0,&head,WBFS_INODE_INFO_OFF);
    if (!err)
    {
	WDiscInfo_t wdi;
	InitializeWDiscInfo(&wdi);
	memcpy(&wdi.dhead,&head,sizeof(wdi.dhead));
	CalcWDiscInfo(&wdi,0);

	fprintf(f,"%*sDisc name:         %s\n",indent,"",wdi.dhead.disc_title);
	if (wdi.title)
	    fprintf(f,"%*sDB title:          %s\n",indent,"",wdi.title);

	const RegionInfo_t * rinfo = GetRegionInfo(wdi.id6[3]);
	fprintf(f,"%*sRegion:            %s [%s]\n",indent,"", rinfo->name, rinfo->name4);
	ResetWDiscInfo(&wdi);
    }
    putc('\n',f);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_BOOT_BIN()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_BOOT_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,0,real_path,0);

    wd_boot_t wb;
    enumError err = ReadSF(sf,0,&wb,sizeof(wb));
    if (!err)
    {
	WDiscInfo_t wdi;
	InitializeWDiscInfo(&wdi);
	memcpy(&wdi.dhead,&wb.dhead,sizeof(wdi.dhead));
	CalcWDiscInfo(&wdi,0);

	fprintf(f,"%*sDisc name:         %s\n",indent,"",wdi.dhead.disc_title);
	if (wdi.title)
	    fprintf(f,"%*sDB title:          %s\n",indent,"",wdi.title);

	const RegionInfo_t * rinfo = GetRegionInfo(wdi.id6[3]);
	fprintf(f,"%*sRegion:            %s [%s]\n",indent,"",rinfo->name,rinfo->name4);
	ResetWDiscInfo(&wdi);

	ntoh_boot(&wb,&wb);
	u64 val = (u64)wb.dol_off4 << 2;
	fprintf(f,"%*sDOL offset: %12x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.dol_off4, val, val );
	val = (u64)wb.fst_off4 << 2 ;
	fprintf(f,"%*sFST offset: %12x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.fst_off4, val, val );
	val = (u64)wb.fst_size4 << 2;
	fprintf(f,"%*sFST size:   %12x/hex * 4 =%8llx/hex =%9llu\n",
			indent,"", wb.fst_size4, val, val  );
    }
    putc('\n',f);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Dump_FST*()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_FST_BIN
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path,	// NULL or pointer to real path
    ShowMode		show_mode	// what should be printed
)
{
    ASSERT(sf);
    TRACE("Dump_FST_BIN(%p,%d,%p,,%x)\n", f, indent, sf, show_mode );

    if ( !f || sf->file_size > 20*MiB )
	return ERR_OK;
    indent = NormalizeIndent(indent);

    //----- options --show and --long

    if ( show_mode & SHOW__DEFAULT )
	show_mode = SHOW__ALL | SHOW_F_HEAD;

    if ( show_mode & SHOW_INTRO )
    {
	dump_header(f,indent,sf,0,real_path,0);
	indent += 2;
    }

    //----- setup fst

    wd_fst_item_t * ftab_data = MALLOC(sf->file_size);

    enumError err = ReadSF(sf,0,ftab_data,sf->file_size);
    if (!err)
    {
	err = Dump_FST_MEM(f,indent,ftab_data,sf->file_size,sf->f.fname,
				ConvertShow2PFST(show_mode,SHOW__ALL) );
	if (err)
	    ERROR0(err,"fst.bin is invalid: %s\n",sf->f.fname);
    }

    FREE(ftab_data);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError Dump_FST_MEM
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    const wd_fst_item_t	* ftab_data,	// the FST data
    size_t		ftab_size,	// size of FST data
    ccp			fname,		// NULL or source hint for error message
    wd_pfst_t		pfst		// print fst mode
)
{
    ASSERT(ftab_data);
    TRACE("Dump_FST_MEM(%p,%d,%p,%x,%s,%x)\n",
		f, indent, ftab_data, (u32)ftab_size, fname, pfst );

    indent = NormalizeIndent(indent);
    if (!fname)
	fname = "memory";

    const u32 n_files = ntohl(ftab_data->size);
    if ( (ccp)(ftab_data + n_files) > (ccp)ftab_data + ftab_size )
    {
	if (f)
	    ERROR0(ERR_INVALID_FILE,"fst.bin is invalid: %s\n",fname);
	return ERR_INVALID_FILE;
    }

    if (f)
    {
	WiiFst_t fst;
	InitializeFST(&fst);
	CollectFST_BIN(&fst,ftab_data,GetDefaultFilePattern(),false);
	sort_mode = SortFST(&fst,sort_mode,SORT_NAME);
	if ( (sort_mode&SORT__MODE_MASK) != SORT_OFFSET )
	    pfst &= ~WD_PFST_UNUSED;
	DumpFilesFST(f,indent,&fst,pfst,0);
	ResetFST(&fst);
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Dump_PATCH()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Dump_PATCH
(
    FILE		* f,		// valid output stream
    int			indent,		// indent of output
    SuperFile_t		* sf,		// file to dump
    ccp			real_path	// NULL or pointer to real path
)
{
    ASSERT(sf);
    if (!f)
	return ERR_OK;
    indent = dump_header(f,indent,sf,0,real_path,0);

    enumError err = OpenStreamWFile(&sf->f);
    if (err)
	return err;

    FILE * pf = sf->f.fp;
    DASSERT(pf);
    fseek(pf,0,SEEK_SET);

    ReadPatch_t pat;
    SetupReadPatch(&pat);
    err = OpenReadPatchF(&pat,pf,sf->f.fname);
    if (err)
	return err;

    fprintf(f,"%*sPatch version:     %s\n",
			indent,"", PrintVersion(0,0,pat.version) );
    fprintf(f,"%*sPatch compatible:  %s\n",
			indent,"", PrintVersion(0,0,pat.compatible) );
    fprintf(f,"%*sTool version:      %s\n",
			indent,"", PrintVersion(0,0,WIT_PATCH_VERSION) );
    fprintf(f,"%*sRead compatible:   %s\n",
			indent,"", PrintVersion(0,0,WIT_PATCH_READ_COMPATIBLE) );
    SupportedVersionReadPatch(&pat,false);

    int recnum;
    for ( recnum = 0; pat.cur_type; recnum++ )
    {
	// [[2do]] [[patch]]
	printf("%3d: %02x %u\n",recnum,pat.cur_type,pat.cur_size);
	enumError err = GetNextReadPatch(&pat);
	if (err)
	    return err;
    }

    pat.file = 0;
    CloseReadPatch(&pat);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			RenameISOHeader()		///////////////
///////////////////////////////////////////////////////////////////////////////

int RenameISOHeader ( void * data, ccp fname,
	ccp set_id6, ccp set_title, int verbose, int testmode )
{
    ASSERT(data);

    TRACE("RenameISOHeader(%p,,%.6s,%s,v=%d,tm=%d)\n",
		data, set_id6 ? set_id6 : "-",
		set_title ? set_title : "-", verbose, testmode );

    if ( !set_id6 || strlen(set_id6) < 6 )
	set_id6 = 0; // invalid id6

    if ( !set_title || !*set_title )
	set_title = 0; // invalid title

    if ( !set_id6 && !set_title )
	return 0; // nothing to do

    wd_header_t * whead = (wd_header_t*)data;
    char old_id6[7], new_id6[7];
    memset(old_id6,0,sizeof(old_id6));
    StringCopyS(old_id6,sizeof(old_id6),&whead->disc_id);
    memcpy(new_id6,old_id6,sizeof(new_id6));

    if ( testmode || verbose >= 0 )
    {
	if (!fname)
	    fname = "";
	printf(" - %sModify [%s] %s\n",
		testmode ? "WOULD " : "", old_id6, fname );
    }

    if (set_id6)
    {
	memcpy(new_id6,set_id6,6);
	set_id6 = new_id6;
	if ( testmode || verbose >= 0 )
	    printf("   - %sRename ID to `%s'\n", testmode ? "WOULD " : "", set_id6 );
    }

    if (set_title)
    {
	char old_name[0x40];
	StringCopyS(old_name,sizeof(old_name),(ccp)whead->disc_title);

	ccp old_title = GetTitle(old_id6,old_name);
	ccp new_title = GetTitle(new_id6,old_name);

	// and now the destination filename
	SubstString_t subst_tab[] =
	{
	    { 'j', 'J',	0, old_id6 },
	    { 'i', 'I',	0, new_id6 },

	    { 'n', 'N',	0, old_name },

	    { 'p', 'P',	0, old_title },
	    { 't', 'T',	0, new_title },

	    {0,0,0,0}
	};

	char title[PATH_MAX];
	SubstString(title,sizeof(title),subst_tab,set_title,0);
	set_title = title;

	if ( testmode || verbose >= 0 )
	    printf("   - %sSet title to `%s'\n",
			testmode ? "WOULD " : "", set_title );
    }

    return wd_rename(data,set_id6,set_title);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////

bool allow_fst		= false;  // FST diabled by default
bool ignore_setup	= false;  // ignore file 'setup.txt' while composing
bool opt_links		= false;  // find linked files and create hard links
bool opt_user_bin	= false;  // enable management of "sys/user.bin"

wd_select_t part_selector = {0};

u8 wdisc_usage_tab [WII_MAX_SECTORS];
u8 wdisc_usage_tab2[WII_MAX_SECTORS];

///////////////////////////////////////////////////////////////////////////////

struct scan_select_t
{
    wd_select_t		* select;
    ccp			err_text_extend;
    bool		err_printed;
};

//-----------------------------------------------------------------------------

static s64 PartSelectorFunc
(
    void		* param,	// NULL or user defined parameter
    ccp			name,		// normalized name of option
    const KeywordTab_t	* cmd_tab,	// valid pointer to command table
    const KeywordTab_t	* cmd,		// valid pointer to found command
    char		prefix,		// 0 | '-' | '+' | '='
    s64			result		// current value of result
)
{
    struct scan_select_t * scan_select = param;
    DASSERT(scan_select);
    wd_select_t * select = scan_select->select;
    DASSERT(select);

    if (prefix)
	name++;

    wd_select_mode_t mode = prefix == '-' ? WD_SM_F_DENY : 0;

    //----- scan partitions numbers

    if ( *name >= '0' && *name <= '9' )
    {
	char *end;
	u32 num = strtoul( name, &end, name[1] >= '0' && name[1] <= '9' ? 10 : 0 );
	if ( end > name && !*end )
	{
	    wd_append_select_item(select,WD_SM_ALLOW_PTYPE|mode,0,num);
	    return ERR_OK;
	}
    }


    //----- analyze keywords

    PRINT("name=%s, cmd=%p, prefix=%c\n", name, cmd, prefix );
    if (cmd)
    {
	mode |= cmd->id;
	switch (cmd->id)
	{
	    case WD_SM_ALLOW_PTYPE:
	    case WD_SM_ALLOW_PTAB:
	    case WD_SM_ALLOW_ID:
	    case WD_SM_ALLOW_GC_BOOT:
	    case WD_SM_ALLOW_ALL:
		wd_append_select_item(select,mode,cmd->opt,cmd->opt);
		break;

	    case WD_SM_N_MODE+1:
		select->whole_disc = prefix != '-';
		break;

	    case WD_SM_N_MODE+2:
		select->whole_part = prefix != '-';
		break;
	}

	return ERR_OK;
    }


    //----- analyze ID partition names

    if ( strlen(name) == 4
	&& isalnum((int)name[0])
	&& isalnum((int)name[1])
	&& isalnum((int)name[2])
	&& isalnum((int)name[3]) )
    {
	u32 ptype = toupper((int)name[0]) << 24
		  | toupper((int)name[1]) << 16
		  | toupper((int)name[2]) << 8
		  | toupper((int)name[3]);
	wd_append_select_item(select,WD_SM_ALLOW_PTYPE|mode,0,ptype);
	return ERR_OK;
    }


    //----- analyze #index | #<index | #>index | #index.index

    if ( *name == '#' )
    {
	ccp ptr = name + 1;
	int mode2 = 0;
	switch(*ptr)
	{
	    case '<': ptr++; mode2 = -1; break;
	    case '>': ptr++; mode2 = +1; break;
	    case '=': ptr++; break;
	}

	int add_num = 0;
	if ( mode2 && *ptr == '=' )
	{
	    ptr++;
	    add_num = -mode2;
	}

	char *end;
	u32 num1 = strtoul( ptr, &end, ptr[1] >= '0' && ptr[1] <= '9' ? 10 : 0 );
	if ( end > ptr )
	{
	    if (!*end)
	    {
		if ( !num1 && add_num < 0 )
		    wd_append_select_item(select,WD_SM_ALLOW_ALL|mode,0,num1+add_num);
		else
		{
		    mode |= mode2 < 0
			    ? WD_SM_ALLOW_LT_INDEX
			    :  mode2 > 0
				? WD_SM_ALLOW_GT_INDEX
				: WD_SM_ALLOW_INDEX;
		    wd_append_select_item(select,mode,0,num1+add_num);
		}
		return ERR_OK;
	    }

	    if ( *end == '.' && num1 < WII_MAX_PTAB && !mode2 )
	    {
		ptr = end + 1;
		if ( !*ptr)
		{
		    wd_append_select_item(select,WD_SM_ALLOW_PTAB|mode,num1,0);
		    return ERR_OK;
		}
		else
		{
		    u32 num2 = strtoul( ptr, &end, ptr[1] >= '0' && ptr[1] <= '9' ? 10 : 0 );
		    if ( end > ptr && !*end )
		    {
			wd_append_select_item(select,WD_SM_ALLOW_PTAB_INDEX|mode,num1,num2);
			return ERR_OK;
		    }
		}
	    }
	}
    }


    //----- terminate with error

    scan_select->err_printed = true;
    ERROR0(ERR_SYNTAX,"Illegal partition selector%s: %.20s\n",
		scan_select->err_text_extend, name );
    return ERR_SYNTAX;
}

//-----------------------------------------------------------------------------

enumError ScanPartSelector
(
    wd_select_t * select,	// valid partition selector
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
)
{
    static const KeywordTab_t tab[] =
    {
	{ WD_SM_ALLOW_PTYPE,	"DATA",		"D",	WD_PART_DATA },
	{ WD_SM_ALLOW_PTYPE,	"GAME",		"G",	WD_PART_DATA },
	{ WD_SM_ALLOW_PTYPE,	"UPDATE",	"U",	WD_PART_UPDATE },
	{ WD_SM_ALLOW_PTYPE,	"INSTALLER",	"I",	WD_PART_UPDATE },
	{ WD_SM_ALLOW_PTYPE,	"CHANNEL",	"C",	WD_PART_CHANNEL },

	{ WD_SM_ALLOW_PTAB,	"PTAB0",	"T0",	0 },
	{ WD_SM_ALLOW_PTAB,	"PTAB1",	"T1",	1 },
	{ WD_SM_ALLOW_PTAB,	"PTAB2",	"T2",	2 },
	{ WD_SM_ALLOW_PTAB,	"PTAB3",	"T3",	3 },

	{ WD_SM_ALLOW_ID,	"ID",		0,	0 },
	{ WD_SM_ALLOW_GC_BOOT,	"BOOT",		0,	0 },
	{ WD_SM_ALLOW_ALL,	"ALL",		0,	0 },

	{ WD_SM_N_MODE+1,	"1:1",		"RAW",	0 },
	{ WD_SM_N_MODE+2,	"WHOLE",	0,	0 },

	{ 0,0,0,0 }
    };

    DASSERT(select);
    wd_reset_select(select);

    struct scan_select_t scan_select;
    memset(&scan_select,0,sizeof(scan_select));
    scan_select.err_text_extend	= err_text_extend;
    scan_select.select		= select;

    const enumError err
	= ScanKeywordListFunc(arg,tab,PartSelectorFunc,&scan_select,true);
    if ( err && !scan_select.err_printed )
	ERROR0(err,"Illegal partition selector%s: %.20s\n",
			err_text_extend, arg );
    return err;
}

//-----------------------------------------------------------------------------

int ScanOptPartSelector ( ccp arg )
{
    return ScanPartSelector(&part_selector,optarg," (option --psel)") != ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

u32 ScanPartType ( ccp arg, ccp err_text_extend )
{
    static const KeywordTab_t tab[] =
    {
	{ WD_PART_DATA,		"DATA",		"D",		0 },
	{ WD_PART_DATA,		"GAME",		"G",		0 },
	{ WD_PART_UPDATE,	"UPDATE",	"U",		0 },
	{ WD_PART_UPDATE,	"INSTALLER",	"I",		0 },
	{ WD_PART_CHANNEL,	"CHANNEL",	"C",		0 },

	{ 0,0,0,0 }
    };

    const KeywordTab_t * cmd = ScanKeyword(0,arg,tab);
    if (cmd)
	return cmd->id;

    char * end;
    u32 val = strtoul(arg,&end,10);
    if ( end > arg && !*end )
	return val;

    if ( strlen(arg) == 4
	&& isalnum((int)arg[0])
	&& isalnum((int)arg[1])
	&& isalnum((int)arg[2])
	&& isalnum((int)arg[3]) )
    {
	return be32(arg);
    }

    ERROR0(ERR_SYNTAX,"Illegal partition type%s: %.20s\n",
			err_text_extend, arg );
    return -(u32)1;
}

//-----------------------------------------------------------------------------

enumError ScanPartTabAndType
(
    u32		* res_ptab,		// NULL or result: partition table
    bool	* res_ptab_valid,	// NULL or result: partition table is valid
    u32		* res_ptype,		// NULL or result: partition type
    bool	* res_ptype_valid,	// NULL or result: partition type is valid
    ccp		arg,			// argument to analyze
    ccp		err_text_extend		// text to extent error messages
)
{
    DASSERT(arg);

    u32 ptab = 0;
    ccp sep = strchr(arg,'.');
    bool ptab_valid = sep != 0;
    if (ptab_valid)
    {
	char * end;
	ptab = strtoul(arg,&end,10);
	if ( end != sep || ptab >= WII_MAX_PTAB )
	    return ERROR0(ERR_SYNTAX,
			"Not a valid partition table%s: %.20s\n",
			err_text_extend, arg );
	arg = sep + 1;

    }

    u32 ptype = 0;
    bool ptype_valid = !sep || *arg;
    if (ptype_valid)
    {
	ptype = ScanPartType(arg,err_text_extend);
	if ( ptype == -(u32)1 )
	    return ERR_SYNTAX;
    }

    if (res_ptab)
	*res_ptab = ptab;
    if (res_ptab_valid)
	*res_ptab_valid = ptab_valid;
    if (res_ptype)
	*res_ptype = ptype;
    if (res_ptype_valid)
	*res_ptype_valid = ptype_valid;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

wd_ipm_t prefix_mode = WD_IPM_DEFAULT;
int opt_flat = 0;

//-----------------------------------------------------------------------------

wd_ipm_t ScanPrefixMode ( ccp arg )
{
    static const KeywordTab_t tab[] =
    {
	{ WD_IPM_DEFAULT,	"DEFAULT",	"D",		0 },
	{ WD_IPM_AUTO,		"AUTO",		"A",		0 },

	{ WD_IPM_NONE,		"NONE",		"-",		0 },
	{ WD_IPM_SLASH,		"SLASH",	"/",		0 },
	{ WD_IPM_POINT,		"POINT",	".",		0 },
	{ WD_IPM_PART_ID,	"ID",		"I",		0 },
	{ WD_IPM_PART_NAME,	"NAME",		"N",		0 },
	{ WD_IPM_PART_INDEX,	"INDEX",	"IDX",		0 },
	{ WD_IPM_COMBI,		"COMBI",	"C",		0 },

	{ 0,0,0,0 }
    };

    const KeywordTab_t * cmd = ScanKeyword(0,arg,tab);
    if (cmd)
	return cmd->id;

    ERROR0(ERR_SYNTAX,"Illegal prefix mode (option --pmode): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

void SetupNeekMode()
{
    opt_copy_gc = true;
    wd_reset_select(&part_selector);
    wd_append_select_item(&part_selector,WD_SM_ALLOW_PTYPE,0,WD_PART_DATA);

    prefix_mode = WD_IPM_NONE;

    FilePattern_t * pat = file_pattern + PAT_DEFAULT;
    pat->rules.used = 0;
    AddFilePattern(":neek",PAT_DEFAULT);
    SetupFilePattern(pat);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Iso Mapping			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeIM ( IsoMapping_t * im )
{
    ASSERT(im);
    memset(im,0,sizeof(*im));
}

///////////////////////////////////////////////////////////////////////////////

void ResetIM ( IsoMapping_t * im )
{
    ASSERT(im);

    if (im->field)
    {
	IsoMappingItem_t *imi, *imi_end = im->field  + im->used;
	for ( imi = im->field; imi < imi_end; imi++ )
	    if (imi->data_alloced)
		FREE(imi->data);
	FREE(im->field);
    }
    memset(im,0,sizeof(*im));
}

///////////////////////////////////////////////////////////////////////////////

static uint InsertHelperIM ( IsoMapping_t * im, u32 offset, u32 size )
{
    ASSERT(im);

    int beg = 0;
    int end = im->used - 1;
    while ( beg <= end )
    {
	const uint idx = (beg+end)/2;
	IsoMappingItem_t * imi = im->field + idx;
	if ( offset < imi->offset )
	    end = idx - 1 ;
	else if ( offset > imi->offset )
	    beg = idx + 1;
	else if ( size < imi->size )
	    end = idx - 1 ;
	else
	    beg = idx + 1;
    }

    TRACE("InsertHelperIM(%x,%x) %d/%d/%d\n",
		offset, size, beg, im->used, im->size );
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

IsoMappingItem_t * InsertIM
	( IsoMapping_t * im, enumIsoMapType imt, u64 offset, u64 size )
{
    ASSERT(im);

    if ( im->used == im->size )
    {
	im->size += 10;
	im->field = REALLOC(im->field,im->size*sizeof(*im->field));
    }

    u32 idx = InsertHelperIM(im,offset,size);

    ASSERT( idx <= im->used );
    IsoMappingItem_t * dest = im->field + idx;
    memmove(dest+1,dest,(im->used-idx)*sizeof(IsoMappingItem_t));
    im->used++;

    memset(dest,0,sizeof(*dest));
    dest->imt	 = imt;
    dest->offset = offset;
    dest->size   = size;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

void PrintIM ( IsoMapping_t * im, FILE * f, int indent )
{
    ASSERT(im);
    if ( !f || !im->used )
	return;
    indent = NormalizeIndent(indent);

    int fw = 80-indent;
    if ( fw < 52 )
	 fw = 52;

    fprintf(f,"\n%*s     offset .. offset end     size  comment\n"
		"%*s%.*s\n",
		indent,"", indent,"", fw, wd_sep_200 );

    u64 prev_end = 0;
    IsoMappingItem_t *imi, *last = im->field + im->used - 1;
    for ( imi = im->field; imi <= last; imi++ )
    {
	const u64 end = imi->offset + imi->size;
	fprintf(f,"%*s%c %9llx .. %9llx %9llx  %s\n", indent,"",
		imi->offset < prev_end ? '!' : ' ',
		imi->offset, end, imi->size, imi->info );
	prev_end = end;
    }
    putc('\n',f);
}

///////////////////////////////////////////////////////////////////////////////

void PrintFstIM
	( WiiFst_t * fst, FILE * f, int indent, bool print_part, ccp title  )
{
    ASSERT(fst);
    if (!f)
	return;
    indent = NormalizeIndent(indent);

    if (title)
	fprintf(f,"\n%*s%s disc:\n",indent,"",title);
    PrintIM(&fst->im,f,indent+2);

    if (print_part)
    {
	WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
	for ( part = fst->part; part < part_end; part++ )
	{
	    if (title)
		fprintf(f,"%*s%s %s partition:\n",
			indent, "", title,
			wd_get_part_name(part->part_type,"?"));
	    PrintIM(&part->im,f,indent+2);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			file index list			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeFileIndex ( FileIndex_t * fidx )
{
    DASSERT(fidx);
    memset(fidx,0,sizeof(*fidx));
}

///////////////////////////////////////////////////////////////////////////////

void ResetFileIndex ( FileIndex_t * fidx )
{
    DASSERT(fidx);
    while ( fidx->data )
    {
	FileIndexData_t * ptr = fidx->data;
	fidx->data = ptr->next;
	FREE(ptr);
    }
    FREE(fidx->sort);
    memset(fidx,0,sizeof(*fidx));
}

///////////////////////////////////////////////////////////////////////////////

static uint FindFileIndexHelper
	( const FileIndex_t * fidx, bool * found, dev_t dev, ino_t ino )
{
    DASSERT(fidx);

    int beg = 0;
    int end = fidx->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	const FileIndexItem_t * item = fidx->sort[idx];
	if      ( ino < item->ino ) end = idx - 1;
	else if ( ino > item->ino ) beg = idx + 1;
	else if ( dev < item->dev ) end = idx - 1;
	else if ( dev > item->dev ) beg = idx + 1;
	else
	{
	    PRINT("FindFileIndexHelper(%llx,%llx) FOUND=%d/%d/%d\n",
			(u64)dev, (u64)ino, idx, fidx->used, fidx->size );
	    if (found)
		*found = true;
	    return idx;
	}
    }

    PRINT("FindFileIndexHelper(%llx,%llx) failed=%d/%d/%d\n",
		(u64)dev, (u64)ino, beg, fidx->used, fidx->size );

    if (found)
	*found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

FileIndexItem_t * FindFileIndex ( FileIndex_t * fidx, dev_t dev, ino_t ino )
{
    DASSERT(fidx);

    bool found;
    const int idx = FindFileIndexHelper(fidx,&found,dev,ino);
    return found ? fidx->sort[idx] : 0;
}

///////////////////////////////////////////////////////////////////////////////

FileIndexItem_t * InsertFileIndex
	( FileIndex_t * fidx, bool * found, dev_t dev, ino_t ino )
{
    DASSERT(fidx);

    bool local_found;
    if (!found)
	found = &local_found;
    const int idx = FindFileIndexHelper(fidx,found,dev,ino);
    if (*found)
	return fidx->sort[idx];

    if ( fidx->used == fidx->size )
    {
	fidx->size = 2 * fidx->size + 0x100;
	fidx->sort  = REALLOC(fidx->sort, fidx->size*sizeof(*fidx->sort));
    }

    if ( !fidx->data || !fidx->data->unused )
    {
	const int count = 0x1000;
	FileIndexData_t * data
	    = MALLOC( sizeof(FileIndexData_t) + count * sizeof(FileIndexItem_t) );
	data->unused = count;
	data->next   = fidx->data;
	fidx->data   = data;
    }

    DASSERT( idx <= fidx->used );
    FileIndexItem_t ** dest = fidx->sort + idx;
    memmove(dest+1,dest,(fidx->used-idx)*sizeof(*dest));

    FileIndexData_t * data = fidx->data;
    ASSERT( data && data->unused > 0 );
    FileIndexItem_t * item = data->data + --data->unused; 
    *dest = item;
    item->dev  = dev;
    item->ino  = ino;
    item->file = 0;
    fidx->used++;

    return item;
}

///////////////////////////////////////////////////////////////////////////////

void DumpFileIndex ( FILE *f, int indent, const FileIndex_t * fidx )
{
    DASSERT(f);
    DASSERT(fidx);
    indent = NormalizeIndent(indent);

    uint i;
    for ( i = 0; i < fidx->used; i++ )
    {
	const FileIndexItem_t * item = fidx->sort[i];
	fprintf(f,"%*s%8llx %8llx %p\n",
		indent, "", (u64)item->dev, (u64)item->ino, item->file );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      Wii FST                    ///////////////
///////////////////////////////////////////////////////////////////////////////

ccp SpecialRequiredFilesFST[] =
{
	"/sys/boot.bin",
	"/sys/bi2.bin",
	"/sys/apploader.img",
	"/sys/main.dol",
	0
};

///////////////////////////////////////////////////////////////////////////////

void InitializeFST ( WiiFst_t * fst )
{
    ASSERT(fst);
    memset(fst,0,sizeof(*fst));
    InitializeIM(&fst->im);
}

///////////////////////////////////////////////////////////////////////////////

void ResetFST ( WiiFst_t * fst )
{
    ASSERT(fst);
    ASSERT( !fst->part_size == !fst->part );
    ASSERT( fst->part_used >= 0 && fst->part_used <= fst->part_size );

    if (fst->part)
    {
	WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
	for ( part = fst->part; part < part_end; part++ )
	    ResetPartFST(part);
	FREE(fst->part);
    }
    ResetIM(&fst->im);
    FREE(fst->cache);

    wd_close_disc(fst->disc);

    memset(fst,0,sizeof(*fst));
    InitializeIM(&fst->im);
}

///////////////////////////////////////////////////////////////////////////////

void ResetPartFST ( WiiFstPart_t * part )
{
    ASSERT(part);
    ASSERT( !part->file_size == !part->file );
    ASSERT( part->file_used >= 0 && part->file_used <= part->file_size );

    if (part->file)
    {
	WiiFstFile_t *file, *file_end = part->file + part->file_used;
	for ( file = part->file; file < file_end; file++ )
	    FreeString(file->path);
	FREE(part->file);
    }

    FreeString(part->path);
    ResetStringField(&part->exclude_list);
    ResetStringField(&part->include_list);
    ResetIM(&part->im);
    FREE(part->ftab);
    FREE(part->pc);
    ResetFileIndex(&part->fidx);

    memset(part,0,sizeof(*part));
    InitializeIM(&part->im);
}

///////////////////////////////////////////////////////////////////////////////

WiiFstPart_t * AppendPartFST ( WiiFst_t * fst )
{
    ASSERT(fst);
    ASSERT( !fst->part_size == !fst->part );
    ASSERT( fst->part_used >= 0 && fst->part_used <= fst->part_size );

    if ( fst->part_used == fst->part_size )
    {
	fst->part_size += 10;
	fst->part = REALLOC(fst->part,fst->part_size*sizeof(*fst->part));
    }

    WiiFstPart_t * part = fst->part + fst->part_used++;
    memset(part,0,sizeof(*part));
    InitializeStringField(&part->exclude_list);
    InitializeStringField(&part->include_list);
    InitializeIM(&part->im);

    return part;
}

///////////////////////////////////////////////////////////////////////////////

WiiFstFile_t * AppendFileFST ( WiiFstPart_t * part )
{
    ASSERT(part);
    ASSERT( !part->file_size == !part->file );
    ASSERT( part->file_used >= 0 && part->file_used <= part->file_size );

    if ( part->file_used == part->file_size )
    {
	part->file_size += part->file_size/4 + 1000;
	part->file = REALLOC(part->file,part->file_size*sizeof(*part->file));
	TRACE("REALLOC(part->file): %u (size=%zu) -> %p\n",
		part->file_size,part->file_size*sizeof(*part->file),part->file);
    }

    part->sort_mode = SORT_NONE;

    WiiFstFile_t * file = part->file + part->file_used++;
    memset(file,0,sizeof(*file));
    return file;
}

///////////////////////////////////////////////////////////////////////////////

WiiFstFile_t * FindFileFST ( WiiFstPart_t * part, u32 offset4 )
{
    ASSERT(part);
    ASSERT(part->file_used);

    int beg = 0;
    int end = part->file_used - 1;
    while ( beg <= end )
    {
	const uint idx = (beg+end)/2;
	WiiFstFile_t * file = part->file + idx;
	if ( file->offset4 < offset4 )
	    beg = idx + 1;
	else
	    end = idx - 1 ;
    }

    if ( beg > 0 )
    {
	WiiFstFile_t * file = part->file + beg-1;
	if ( file->offset4 + (file->size+3>>2) > offset4 )
	    beg--;
    }

 #ifdef DEBUG
    {
	TRACE("FindFileFST(%x) %d/%d/%d, searched-off=%x\n",
		    offset4, beg, part->file_used, part->file_size, offset4<<2 );

	u64 off;
	WiiFstFile_t * f;

	if ( beg > 0 )
	{
	    f = part->file + beg - 1;
	    off = (u64)f->offset4 << 2;
	    TRACE(" - %5x: %9llx .. %9llx + %8x %s\n",
			beg-1, off, off+f->size, f->size, f->path );
	}
	f = part->file + beg;
	off = (u64)f->offset4 << 2;
	TRACE(" * %5x: %9llx .. %9llx + %8x %s\n",
			beg, off, off+f->size, f->size, f->path );
	if ( beg < part->file_used - 1 )
	{
	    f = part->file + beg + 1;
	    off = (u64)f->offset4 << 2;
	    TRACE(" - %5x: %9llx .. %9llx + %8x %s\n",
			beg+1, off, off+f->size, f->size, f->path );
	}
    }
 #endif

    ASSERT( beg >= 0 && beg < part->file_used );
    return part->file + beg;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CollectFST()			/////////////// 
///////////////////////////////////////////////////////////////////////////////

typedef struct CollectFST_t
{
	WiiFst_t	* fst;		// valid fst pointer
	wd_disc_t	* disc;		// valid disc pointer
	FilePattern_t	* pat;		// NULL or a valid filter
	bool		ignore_dir;	// true: ignore directories
	bool		store_prefix;	// true: store full path incl. prefix

} CollectFST_t;

//-----------------------------------------------------------------------------

static int CollectFST_helper
(
    struct wd_iterator_t *it	// iterator struct with all infos
)
{
    DASSERT(it);

    CollectFST_t * cf = it->param;
    WiiFst_t * fst = cf->fst;
    ASSERT(fst);
    WiiFstPart_t * part = fst->part_active;

    if ( it->icm >= WD_ICM_DIRECTORY )
    {
	ASSERT(part);
	if (!part)
	    return ERR_OK;

	if ( it->icm == WD_ICM_DIRECTORY )
	{
	    fst->dirs_served++;
	    if ( cf->ignore_dir || opt_flat )
		return 0;
	}
	else
	{
	    fst->files_served++;
	    part->total_file_size += it->size;
	}

	if ( cf->pat && !MatchFilePattern(cf->pat,it->fst_name,'/') )
	    return 0;

	noPRINT("%8x >> %8x + %8x  %s\n",it->off4,it->off4>>2,it->size,it->path);

	size_t slen = strlen(it->path);
	if ( fst->max_path_len < slen )
	     fst->max_path_len = slen;
	if ( fst->fst_max_off4 < it->off4 )
	     fst->fst_max_off4 = it->off4;
	if ( fst->fst_max_size < it->size )
	     fst->fst_max_size = it->size;

	WiiFstFile_t * wff = AppendFileFST(part);
	wff->icm	= it->icm;
	wff->offset4	= it->off4;
	wff->size	= it->size;
	wff->data	= (u8*)it->data;
	if (opt_flat)
	{
	    ccp src = strrchr(it->fst_name,'/');
	    src = src ? src+1 : it->fst_name;
	    if ( cf->store_prefix )
	    {
		char fname[sizeof(it->path)];
		StringCat2S(fname,sizeof(fname),it->prefix,src);
		wff->path = STRDUP(fname);
	    }
	    else
		wff->path = STRDUP(src);
	}
	else
	{
	    wff->path = STRDUP( cf->store_prefix ? it->path : it->fst_name );
	    if ( it->icm == WD_ICM_DIRECTORY && it->prefix_mode == WD_IPM_SLASH )
	    {
		const int plen = strlen(wff->path) - 1;
		if ( plen >= 0 && wff->path[plen] == '/' )
		    ((char*)wff->path)[plen] = 0;
	    }
	}
    }
    else switch(it->icm)
    {
	case WD_ICM_OPEN_PART:
	    part			= AppendPartFST(fst);
	    fst->part_active		= part;
	    part->part			= it->part;
	    part->path			= STRDUP(it->path);
	    part->part_type		= it->part->part_type;
	    part->part_off		= (u64) it->part->part_off4 << 2;

	    memcpy(part->key,it->part->key,sizeof(part->key));
	    wd_aes_set_key(&part->akey,part->key);
	    break;

	case WD_ICM_CLOSE_PART:
	    if (part)
	    {
		fst->total_file_count += part->file_used;
		fst->total_file_size  += part->total_file_size;
		fst->part_active = 0;
		TRACE("PART CLOSED: %u/%u files, %llu/%llu MiB\n",
			part->file_used, fst->total_file_count,
			part->total_file_size/MiB, fst->total_file_size/MiB );
	    }
	    break;

	default:
	    // nothing to do
	    break;
    }

    return 0;
}

//-----------------------------------------------------------------------------

int CollectFST
(
    WiiFst_t		* fst,		// valid fst pointer
    wd_disc_t		* disc,		// valid disc pointer
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir,	// true: ignore directories
    int			ignore_files,	// >0: ignore all real files
					// >1: ignore fst.bin + main.dol too
    wd_ipm_t		prefix_mode,	// prefix mode
    bool		store_prefix	// true: store full path incl. prefix
)
{
    ASSERT(fst);
    ASSERT(disc);

    // dup and store disc
    wd_dup_disc(disc);
    wd_close_disc(fst->disc);
    fst->disc = disc;

    if (pat)
	SetupFilePattern(pat);

    CollectFST_t cf;
    memset(&cf,0,sizeof(cf));
    cf.fst		= fst;
    cf.disc		= disc;
    cf.pat		= pat && pat->match_all ? 0 : pat;
    cf.ignore_dir	= ignore_dir;
    cf.store_prefix	= store_prefix;

    if ( opt_flat && ( prefix_mode == WD_IPM_AUTO || prefix_mode == WD_IPM_DEFAULT ))
	prefix_mode = WD_IPM_NONE;
    return wd_iterate_files(disc,CollectFST_helper,&cf,ignore_files,prefix_mode);
}

//-----------------------------------------------------------------------------

int CollectFST_BIN
(
    WiiFst_t		* fst,		// valid fst pointer
    const wd_fst_item_t * ftab_data,	// the FST data
    FilePattern_t	* pat,		// NULL or a valid filter
    bool		ignore_dir	// true: ignore directories
)
{
    CollectFST_t cf;
    memset(&cf,0,sizeof(cf));
    cf.fst		= fst;
    cf.pat		= pat && pat->match_all ? 0 : pat;
    cf.ignore_dir	= ignore_dir;

    fst->part_active = AppendPartFST(fst);
    return wd_iterate_fst_files(ftab_data,CollectFST_helper,&cf);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DumpFilesFST
(
    FILE		* f,		// NULL or output file
    int			indent,		// indention of the output
    WiiFst_t		* fst,		// valid FST pointer
    wd_pfst_t		pfst,		// print fst mode
    ccp			prefix		// NULL or path prefix
)
{
    DASSERT(fst);
    PRINT("DumpFilesFST() pfst=%x\n",pfst);
    if ( pfst & WD_PFST_HEADER )
	putchar('\n');

    if (!prefix)
	prefix = "";
    size_t prefix_len = strlen(prefix);

    wd_print_fst_t pf;
    wd_initialize_print_fst(&pf,pfst,f,indent,fst->fst_max_off4,fst->fst_max_size);
    if ( pfst & WD_PFST_HEADER )
	wd_print_fst_header(&pf,fst->max_path_len+prefix_len);

    WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
    for ( part = fst->part; part < part_end; part++ )
    {
	WiiFstFile_t * ptr = part->file;
	WiiFstFile_t * end = ptr + part->file_used;

	for ( ; ptr < end; ptr++ )
	    wd_print_fst_item( &pf, part->part,
			ptr->icm, ptr->offset4, ptr->size,
			prefix, ptr->path );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CreateFST()...			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CreateFST ( WiiFstInfo_t *wfi, ccp dest_path )
{
    TRACE("CreateFST(%p)\n",wfi);
    ASSERT(wfi);

    WiiFst_t * fst = wfi->fst;
    ASSERT(fst);
    ASSERT( !fst->part_size == !fst->part );
    ASSERT( fst->part_used >= 0 && fst->part_used <= fst->part_size );

    wfi->done_count  = 0;
    wfi->total_count = fst->total_file_count;
    wfi->done_size   = 0;
    wfi->total_size  = fst->total_file_size;

    if (wfi->verbose)
    {
	if (print_sections)
	{
	    printf(
		"[extract:info]\n"
		"n-partitions=%u\n"
		"n-files=%u\n"
		"total-size=%llu\n"
		"\n"
		,fst->part_used
		,fst->total_file_count
		,fst->total_file_size
		);
	}
	else
	{
	    char buf[20];
	    wfi->fw_done_count = snprintf(buf,sizeof(buf),"%u",wfi->total_count);

	    printf(" - will extract %u file%s of %u partition%s, %llu MiB total\n",
		fst->total_file_count, fst->total_file_count == 1 ? "" : "s",
		fst->part_used, fst->part_used  == 1 ? "" : "s",
		(fst->total_file_size + MiB/2) / MiB );
	}
    }


    //----- iterate partitions

    enumError err = ERR_OK;
    WiiFstPart_t *part_end = fst->part + fst->part_used;
    for ( wfi->part = fst->part; err == ERR_OK && wfi->part < part_end; wfi->part++ )
	err = CreatePartFST(wfi,dest_path);
    wfi->part = 0;

    if ( wfi->copy_image && wfi->sf && wfi->sf->disc1 )
    {
	DASSERT( wfi->sf->disc1->disc_type == WD_DT_GAMECUBE );
	PathCatPP(iobuf,sizeof(iobuf),dest_path,"game.iso");

	bool need_copy = true;
	if ( wfi->link_image
		&& wfi->sf->iod.oft == OFT_PLAIN	// ISO format
		&& wfi->sf->disc1 == wfi->sf->disc2 )	// not patched
	{
	    unlink(iobuf);
	    need_copy = link(wfi->sf->f.fname,iobuf) != 0;
	}

	if ( wfi->verbose > 0 )
	{
	    if (print_sections)
		printf(
		    "[extract:%s-image]\n"
		    "source-image=%s\n"
		    "dest-image=%s\n"
		    "\n"
		    ,need_copy ? "copy" : "link"
		    ,wfi->sf->f.fname
		    ,iobuf
		    );
	    else
		printf(" - %s image to %s\n", need_copy ? "copy" : "link", iobuf );
	}

	if (need_copy)
	{
	    enumError err = CopyImageName( wfi->sf, iobuf,0, OFT_PLAIN,
					wfi->overwrite, wfi->set_time != 0, false );
	    if (err)
		wfi->not_created_count++;
	}
    }

    if ( wfi->sf && ( wfi->sf->show_progress ||wfi->sf->show_summary ) )
	PrintSummarySF(wfi->sf);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreatePartFST ( WiiFstInfo_t *wfi, ccp dest_path )
{
    TRACE("CreatePartFST(%p)\n",wfi);
    ASSERT(wfi);

    ResetWiiParamField(&wfi->align_info);
    wfi->align_info.pft = PFT_ALIGN;
    WiiFstPart_t * part = wfi->part;
    ASSERT(part);


    //----- setup basic dest path

    char path[PATH_MAX];
    char * path_dest = SetupDirPath(path,sizeof(path),dest_path);
    char * path_end = path + sizeof(path) - 1;


    //----- setup include list

    StringCat2E(path_dest,path_end,part->path,FST_INCLUDE_FILE);
    LoadStringField(&part->include_list,false,true,path,true);


    //----- create all files but WD_ICM_COPY

    *path_dest = 0;

    enumError err = ERR_OK;
    WiiFstFile_t *file, *file_end = part->file + part->file_used;
    for ( file = part->file; err == ERR_OK && file < file_end; file++ )
	if ( file->icm != WD_ICM_COPY )
	    err = CreateFileFST(wfi,path,file);


    //----- Update the signatures of the source FST

    if (wfi->fst)
	UpdateSignatureFST(wfi->fst);


    //----- and now write WD_ICM_COPY files
    //      this is needed because changing hash data while processing other files

    for ( file = part->file; err == ERR_OK && file < file_end; file++ )
	if ( file->icm == WD_ICM_COPY )
	    err = CreateFileFST(wfi,path,file);


    //----- write include.list

    StringCat2E(path_dest,path_end,part->path,FST_INCLUDE_FILE);
    SaveStringField(&part->include_list,path,true);


    //----- write align.list

    StringCat2E(path_dest,path_end,part->path,FST_ALIGN_FILE);
    SaveWiiParamField(&wfi->align_info,path,true);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateFileFST ( WiiFstInfo_t *wfi, ccp dest_path, WiiFstFile_t * file )
{
    DASSERT(wfi);
    DASSERT(wfi->fst);
    DASSERT(dest_path);
    DASSERT(file);

    WiiFstPart_t * part = wfi->part;
    DASSERT(part);

    wfi->done_count++;


    //----- setup path

    char dest[PATH_MAX];
    char * dest_end = dest + sizeof(dest);
    char * dest_part = StringCat2E(dest,dest_end,dest_path,part->path);
    StringCopyE(dest_part,dest_end,file->path);
    TRACE("DEST: %.*s %s\n", (int)(dest_part-dest), dest, dest_part );


    //----- report files with trailing '.'

    char * ptr = dest_part;
    while (*ptr)
    {
	if ( *ptr++ == '/' && *ptr == '.' )
	{
	    while ( *ptr && *ptr != '/' )
		ptr++;
	    char ch = *ptr;
	    *ptr = 0;
	    TRACE("TRAILING POINT: %s\n",dest_part);
	    InsertStringField(&part->include_list,dest_part,false);
	    *ptr = ch;
	}
    }


    //----- directory handling

    if ( file->icm == WD_ICM_DIRECTORY )
    {
	if ( wfi->verbose > 1 )
	{
	    if (print_sections) // [sections]
		printf(
		    "[extract:create-dir]\n"
		    "done-count=%u\n"
		    "total-count=%u\n"
		    "iso-path=%s%s\n"
		    "dest-path=%s\n"
		    "\n"
		    ,wfi->done_count
		    ,wfi->total_count
		    ,part->path
		    ,file->path
		    ,dest
		    );
	    else
		printf(" - %*u/%u create directory %s\n",
			wfi->fw_done_count, wfi->done_count, wfi->total_count,
			dest);
	}
	if (CreatePath(dest,false))
	    wfi->not_created_count++;
	return ERR_OK;
    }

    if ( file->icm != WD_ICM_FILE && file->icm != WD_ICM_COPY && file->icm != WD_ICM_DATA )
	return 0;


    //----- alignment statstistics

    char source[PATH_MAX];
    if ( file->icm == WD_ICM_FILE
	&& !memcmp(file->path,"files/",6)
	&& ReduceToPathAndType(source,sizeof(source),file->path) )
    {
	noPRINT("ALIGN: %s\n",source);
	uint off = file->offset4;
	if (!part->part->is_gc)
	    off <<= 2;
	InsertWiiParamField(&wfi->align_info,source,off);
    }


    //----- detect links

    if ( opt_links && file->icm == WD_ICM_FILE )
    {
	WiiFstFile_t * last = wfi->last_file;
	if ( last && last->offset4 == file->offset4 && last->size == file->size )
	{
	    char * source_end = source + sizeof(source);
	    char * source_part = StringCat2E(source,source_end,dest_path,part->path);
	    StringCopyE(source_part,source_end,last->path);
	    PRINT("LINK %s ->%s\n",source,dest);
	    unlink(dest);
	    if (!link(source,dest))
	    {
		if ( wfi->verbose > 1 )
		{
		    if (print_sections) // [sections]
			printf(
			    "[extract:link-file]\n"
			    "done-count=%u\n"
			    "total-count=%u\n"
			    "iso-path=%s%s\n"
			    "dest-path=%s\n"
			    "link-source=%s\n"
			    "file-size=%u\n"
			    "\n"
			    ,wfi->done_count
			    ,wfi->total_count
			    ,part->path
			    ,file->path
			    ,dest
			    ,source
			    ,file->size
			    );
		    else
			printf(" - %*u/%u link %s -> %s\n",
			    wfi->fw_done_count, wfi->done_count, wfi->total_count,
			    source, dest );
		}
		return ERR_OK;
	    }
	}
	wfi->last_file = file;
    }

    //----- file handling

    if ( wfi->verbose > 1 )
    {
	if (print_sections) // [sections]
	    printf(
		"[extract:create-file]\n"
		"done-count=%u\n"
		"total-count=%u\n"
		"iso-path=%s%s\n"
		"dest-path=%s\n"
		"file-size=%u\n"
		"\n"
		,wfi->done_count
		,wfi->total_count
		,part->path
		,file->path
		,dest
		,file->size
		);
	else
	    printf(" - %*u/%u %s %8u bytes to %s\n",
		wfi->fw_done_count, wfi->done_count, wfi->total_count,
		file->icm == WD_ICM_DATA ? "write  " : "extract", file->size, dest );
    }

    WFile_t fo;
    InitializeWFile(&fo);
    fo.create_directory = true;
    fo.already_created_mode = ignore_count < 1;
    enumError err = CreateWFile( &fo, dest, IOM_NO_STREAM, wfi->overwrite );
    if (err)
    {
	ResetWFile(&fo,true);
	wfi->not_created_count++;
	return ERR_OK;
    }

    if ( file->size > sizeof(iobuf) )
	PreallocateF(&fo,0,file->size);

 #if 0 && defined(TEST) // test ReadFileFST4() [[obsolete]]
    if ( file->icm == WD_ICM_DATA ) 
	err = WriteF(&fo,file->data,file->size);
    else
    {
	u32 off4 = 0;
	u32 size = file->size;

	while ( size > 0 )
	{
	    const u32 read_size = size < sizeof(iobuf) ? size : sizeof(iobuf);
	    err = ReadFileFST4(part,file,off4,iobuf,read_size);
	    if (err)
		break;

	    err = WriteF(&fo,iobuf,read_size);
	    if (err)
		break;

	    size -= read_size;
	    off4 += read_size>>2;

	    //----- progress

	    wfi->done_size += read_size;
	    if ( wfi->sf && wfi->sf->show_progress )
		PrintProgressSF(wfi->done_size,wfi->total_size,wfi->sf);
	}
    }
 #else
    if ( file->icm == WD_ICM_DATA ) 
	err = WriteF(&fo,file->data,file->size);
    else
    {
	u32 size = file->size;
	u32 off4 = file->offset4;

	while ( size > 0 )
	{
	    const u32 read_size = size < sizeof(iobuf) ? size : sizeof(iobuf);
	    err = file->icm == WD_ICM_FILE
			? wd_read_part(part->part,off4,iobuf,read_size,false)
			: wd_read_raw(part->part->disc,off4,iobuf,read_size,0);
	    if (err)
		break;

	    err = WriteF(&fo,iobuf,read_size);
	    if (err)
		break;

	    size -= read_size;
	    if (part->part->is_gc)
		off4 += read_size;
	    else
		off4 += read_size >> 2;

	    //----- progress

	    wfi->done_size += read_size;
	    if ( wfi->sf && wfi->sf->show_progress )
		PrintProgressSF(wfi->done_size,wfi->total_size,wfi->sf);
	}
    }
 #endif

    if (wfi->set_time)
    {
	CloseWFile(&fo,0);
	SetWFileTime(&fo,wfi->set_time);
    }

    ResetWFile( &fo, err != ERR_OK );

    return err;    
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadFileFST4
(
    WiiFstPart_t	* part,	// valid fst partition pointer
    const WiiFstFile_t	*file,	// valid fst file pointer
    u32			off4,	// file offset/4
    void		* buf,	// destination buffer with at least 'size' bytes
    u32			size	// number of bytes to read
)
{
    DASSERT(part);
    DASSERT(part->part);
    DASSERT(part->part->disc);
    DASSERT(file);
    DASSERT(buf);

    noTRACE("ReadFileFST4(off=%x,sz=%x) icm=%x, off=%x, sz=%x\n",
		off4<<2, size, file->icm, file->offset4<<2, file->size );

    char mode = 0;
    switch(file->icm)
    {
	case WD_ICM_FILE:
	    if ( (off4<<2) + size > file->size )
	    {
		mode = 'F';
		break;
	    }
	    return wd_read_part( part->part, file->offset4 + off4, buf, size, false );

	case WD_ICM_COPY:
	    if ( (off4<<2) + size > file->size )
	    {
		mode = 'C';
		break;
	    }
	    return wd_read_raw( part->part->disc, file->offset4 + off4, buf, size, 0 );

	case WD_ICM_DATA:
	    if ( (off4<<2) + size > file->size )
	    {
		mode = 'D';
		break;
	    }
	    memcpy( buf, file->data + (off4<<2), size );
	    return ERR_OK;

	default:
	    if ( off4 || size )
	    {
		mode = '?';
		break;
	    }
	    return ERR_OK;
    }

    return ERROR0(ERR_READ_FAILED,
		"Reading from FST file failed [%c:%llx+%x>%x]: %s\n",
		mode, (u64)off4<<2, size, file->size, file->path );
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadFileFST
(
    WiiFstPart_t	* part,	// valid fst partition pointer
    const WiiFstFile_t	*file,	// valid fst file pointer
    u64			off,	// file offset
    void		* buf,	// destination buffer with at least 'size' bytes
    u32			size	// number of bytes to read
)
{
    const uint skip = (uint)off & 3;
    if (!skip)
	return ReadFileFST4(part,file,off>>2,buf,size);

    DASSERT(part);
    DASSERT(part->part);
    DASSERT(part->part->disc);
    DASSERT(file);
    DASSERT(buf);

    TRACE("ReadFileFST(off=%llx,sz=%x) icm=%x, off=%x, sz=%x\n",
		off, size, file->icm, file->offset4<<2, file->size );

    char temp[4];
    const enumError err = ReadFileFST4(part,file,off>>2,temp,sizeof(temp));
    if (err)
	return err;

    uint pre_read = 4 - skip;
    if ( pre_read > size )
	 pre_read = size;
    DASSERT( pre_read > 0 && pre_read < sizeof(temp) );

    memcpy(buf,temp+skip,pre_read);
    buf = (char*)buf + pre_read;
    return ReadFileFST4(part,file,(off+skip)>>2,buf,size-pre_read);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sort Wii FST			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef int (*compare_func) (const void *, const void *);

//-----------------------------------------------------------------------------

static int sort_fst_by_nintendo ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    const int stat = NintendoCMP(a->path,b->path);
    if (stat)
	return stat;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    return a->size < b->size ? -1 : a->size > b->size;
}

//-----------------------------------------------------------------------------

static int sort_fst_by_path ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    const int stat = PathCMP(a->path,b->path);
    if (stat)
	return stat;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    return a->size < b->size ? -1 : a->size > b->size;
}

//-----------------------------------------------------------------------------

static int sort_fst_by_offset ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    if ( a->icm == WD_ICM_DIRECTORY && b->icm != WD_ICM_DIRECTORY )
	return -1;

    if ( a->icm != WD_ICM_DIRECTORY && b->icm == WD_ICM_DIRECTORY )
	return 1;

    if ( a->icm == WD_ICM_FILE && b->icm != WD_ICM_FILE )
	return 1;

    if ( a->icm != WD_ICM_FILE && b->icm == WD_ICM_FILE )
	return -1;

    if ( a->offset4 != b->offset4 )
	return a->offset4 < b->offset4 ? -1 : 1;

    if ( a->size != b->size )
	return a->size < b->size ? -1 : 1;

    return strcmp(a->path,b->path);
}

//-----------------------------------------------------------------------------

static int sort_fst_by_size ( const void * va, const void * vb )
{
    const WiiFstFile_t * a = (const WiiFstFile_t *)va;
    const WiiFstFile_t * b = (const WiiFstFile_t *)vb;

    if ( a->icm == WD_ICM_DIRECTORY && b->icm != WD_ICM_DIRECTORY )
	return -1;

    if ( a->icm != WD_ICM_DIRECTORY && b->icm == WD_ICM_DIRECTORY )
	return 1;

    if ( a->size != b->size )
	return a->size < b->size ? -1 : 1;

    return sort_fst_by_offset(va,vb);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

SortMode SortFST ( WiiFst_t * fst, SortMode sort_mode, SortMode default_sort_mode )
{
    if (fst)
    {
	WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
	for ( part = fst->part; part < part_end; part++ )
	    sort_mode = SortPartFST(part,sort_mode,default_sort_mode);
    }
    return sort_mode;
}

///////////////////////////////////////////////////////////////////////////////

SortMode SortPartFST ( WiiFstPart_t * part, SortMode sort_mode, SortMode default_sort_mode )
{
    TRACE("SortPartFST(%p,%d,%d=\n",part,sort_mode,default_sort_mode);
    if (!part)
	return sort_mode;

    TRACE("SortFST(%p, s=%d,%d) prev=%d\n",
		part, sort_mode, default_sort_mode, part->sort_mode );

    if ( (uint)(sort_mode&SORT__MODE_MASK) >= SORT_DEFAULT )
    {
	sort_mode = (uint)default_sort_mode >= SORT_DEFAULT
			? part->sort_mode
			: default_sort_mode | sort_mode & SORT_REVERSE;
    }

    compare_func func = 0;
    SortMode smode = sort_mode & SORT__MODE_MASK;
    switch(smode)
    {
	case SORT_SIZE:		func = sort_fst_by_size; break;
	case SORT_OFFSET:	func = sort_fst_by_offset; break;

	case SORT_ID:
	case SORT_NAME:
	case SORT_TITLE:
	case SORT_NINTENDO:
	case SORT_FILE:
	case SORT_REGION:
	case SORT_WBFS:
	case SORT_NPART:
	case SORT_FRAG:
	case SORT_ITIME:
	case SORT_MTIME:
	case SORT_CTIME:
	case SORT_ATIME:
		smode = SORT_NINTENDO;
		func = sort_fst_by_nintendo;
		break;

	case SORT_PATH:
		smode = SORT_PATH;
		func = sort_fst_by_path;
		break;

	default:
		break;
    }
    smode |= sort_mode & SORT_REVERSE;

    if ( func && part->sort_mode != smode )
    {
	part->sort_mode = smode;
	if ( part->file_used > 1 )
	{
	    qsort(part->file,part->file_used,sizeof(*part->file),func);
	    if ( smode & SORT_REVERSE )
		ReversePartFST(part);
	}
    }

    return smode;
}

///////////////////////////////////////////////////////////////////////////////

void ReversePartFST ( WiiFstPart_t * part )
{
    if ( !part || part->file_used < 2 )
	return;

    ASSERT(part->file);

    WiiFstFile_t *a, *b, temp;
    for ( a = part->file, b = a + part->file_used-1; a < b; a++, b-- )
    {
	memcpy( &temp, a,     sizeof(WiiFstFile_t) );
	memcpy( a,     b,     sizeof(WiiFstFile_t) );
	memcpy( b,     &temp, sizeof(WiiFstFile_t) );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    IsFST()			///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
	PSUP_D_TYPE,
	PSUP_I_TYPE,
	PSUP_P_ID,
	PSUP_P_NAME,
	PSUP_P_OFFSET,

	PSUB__N
};

static SetupDef_t part_setup_def[] =
{
	{ "disc-type",		0 },
	{ "image-type",		0 },
	{ "part-id",		0 },
	{ "part-name",		0 },
	{ "part-offset",	0x10000 },
	{0,0}
};

///////////////////////////////////////////////////////////////////////////////

static enumError ScanSetupDef
(
	ccp path,		// filename of text file, part 1
				// part 2 := FST_SETUP_FILE
	bool silent		// true: suppress error message if file not found
)
{
    enumError err = ScanSetupFile(part_setup_def,path,FST_SETUP_FILE,silent);

    static const KeywordTab_t tab[] =
    {
	{ WD_DT_GAMECUBE,	"GAMECUBE",	"GC",	0 },
	{ WD_DT_WII,		"WII",		0,	0 },
	{ 0,0,0,0 }
    };

    SetupDef_t *dtype = part_setup_def + PSUP_D_TYPE;
    if (dtype->param)
    {
	const KeywordTab_t * cmd = ScanKeyword(0,dtype->param,tab);
	dtype->value = cmd ? cmd->id : WD_DT_UNKNOWN;
    }

    SetupDef_t *itype = part_setup_def + PSUP_I_TYPE;
    if (itype->param)
    {
	const KeywordTab_t * cmd = ScanKeyword(0,itype->param,ImageTypeTab);
	itype->value = cmd && !cmd->opt ? cmd->id : OFT_UNKNOWN;
    }

    PRINT("Scan %s: disc-type = %lld [%s] / image-type = %lld [%s]\n",
		FST_SETUP_FILE,
		dtype->value, wd_get_disc_type_name(dtype->value,"?"),
		itype->value, oft_info[itype->value].name );
    return err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumFileType IsFST ( ccp base_path, char * id6_result )
{
    enumFileType ftype = IsFSTPart(base_path,id6_result);
    if ( ftype != FT_ID_DIR )
	return ftype;

    //----- search for a data partition

    return SearchPartitionsFST(base_path,id6_result,0,0,0) & DATA_PART_FOUND
		? FT_ID_FST | FT_A_ISO | FT_A_WII_ISO
		: FT_ID_DIR;
}

///////////////////////////////////////////////////////////////////////////////

int SearchPartitionsFST
(
	ccp base_path,
	char * id6_result,
	char * data_part,
	char * update_part,
	char * channel_part
)
{
    if (id6_result)
	memset(id6_result,0,7);

    char data_buf[PARTITION_NAME_SIZE];
    char update_buf[PARTITION_NAME_SIZE];
    char channel_buf[PARTITION_NAME_SIZE];

    if (!data_part)
	data_part = data_buf;
    if (!update_part)
	update_part = update_buf;
    if (!channel_part)
	channel_part = channel_buf;
    *data_part = *update_part = *channel_part = 0;

    bool data_found = false, update_found = false, channel_found = false;
    char path[PATH_MAX];

    DIR * dir = opendir(base_path);
    if (dir)
    {
	while ( !data_found || !update_found || !channel_found )
	{
	    struct dirent * dent = readdir(dir);
	    if (!dent)
		break;

	 #ifdef _DIRENT_HAVE_D_TYPE
	    if ( dent->d_type != DT_DIR )
		continue;
	 #endif

	    if ( !data_found &&
		 (!strcasecmp("data",dent->d_name) || !strcasecmp("p0",dent->d_name)))
	    {
		StringCat3S(path,sizeof(path),base_path,"/",dent->d_name);
		if ( IsFSTPart(path,id6_result) & FT_A_PART_DIR )
		{
		    data_found = true;
		    StringCopyS(data_part,PARTITION_NAME_SIZE,dent->d_name);
		}
	    }

	    if ( !update_found &&
		 (!strcasecmp("update",dent->d_name) || !strcasecmp("p1",dent->d_name)))
	    {
		StringCat3S(path,sizeof(path),base_path,"/",dent->d_name);
		if ( IsFSTPart(path,0) & FT_A_PART_DIR )
		{
		    update_found = true;
		    StringCopyS(update_part,PARTITION_NAME_SIZE,dent->d_name);
		}
	    }

	    if ( !channel_found &&
		 (!strcasecmp("channel",dent->d_name) || !strcasecmp("p2",dent->d_name)))
	    {
		StringCat3S(path,sizeof(path),base_path,"/",dent->d_name);
		if ( IsFSTPart(path,0) & FT_A_PART_DIR )
		{
		    channel_found = true;
		    StringCopyS(channel_part,PARTITION_NAME_SIZE,dent->d_name);
		}
	    }
	}
	closedir(dir);
    }

    return ( data_found    ? DATA_PART_FOUND    : 0 )
	 | ( update_found  ? UPDATE_PART_FOUND  : 0 )
	 | ( channel_found ? CHANNEL_PART_FOUND : 0 );
}

///////////////////////////////////////////////////////////////////////////////

enumFileType IsFSTPart ( ccp base_path, char * id6_result )
{
    TRACE("IsFSTPart(%s,%p)\n",base_path,id6_result);

    if (id6_result)
	memset(id6_result,0,7);

    struct stat st;
    if ( !base_path || stat(base_path,&st) || !S_ISDIR(st.st_mode) )
	return FT_UNKNOWN;

    char path[PATH_MAX];

    //----- check if sys/ and files/ exists

    StringCat2S(path,sizeof(path),base_path,"/sys");
    TRACE(" - test dir  %s\n",path);
    if ( stat(path,&st) || !S_ISDIR(st.st_mode) )
	return FT_ID_DIR;

    StringCat2S(path,sizeof(path),base_path,"/files");
    TRACE(" - test dir  %s\n",path);
    if ( stat(path,&st) || !S_ISDIR(st.st_mode) )
	return FT_ID_DIR;


    //------ boot.bin

    StringCat2S(path,sizeof(path),base_path,"/sys/boot.bin");
    TRACE(" - read file %s\n",path);
    const int fd = open(path,O_RDONLY);
    if ( fd == -1 )
	return FT_ID_DIR;

    if ( fstat(fd,&st)
	|| !S_ISREG(st.st_mode)
	|| st.st_size != WII_BOOT_SIZE
	|| id6_result && read(fd,id6_result,6) != 6 )
    {
	close(fd);
	if (id6_result)
	    memset(id6_result,0,7);
	return FT_ID_DIR;
    }
    close(fd);


    //----- patch id

    if (id6_result)
    {
	if (!ignore_setup)
	{
	    ScanSetupDef(base_path,true);
	    ccp part_id = part_setup_def[PSUP_P_ID].param;
	    if (part_id)
		wd_patch_id(id6_result,id6_result,part_id,6);
	}
	PatchId(id6_result,modify_disc_id,0,6);
    }


    //----- more required files

    ccp * fname;
    for ( fname = SpecialRequiredFilesFST; *fname; fname++ )
    {
	StringCat2S(path,sizeof(path),base_path,*fname);
	TRACE(" - test file %s\n",path);
	if ( stat(path,&st) || !S_ISREG(st.st_mode) )
	    return FT_ID_DIR;
    }

    TRACE(" => FST found, id=%s\n",id6_result?id6_result:"");
    return FT_ID_FST | FT_A_PART_DIR | FT_A_ISO | FT_A_WII_ISO;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  scan partition		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct scan_data_t
{
	WiiFstPart_t * part;	// valid pointer to partition
	char path[PATH_MAX];	// the current path
	char * path_part;	// ptr to 'path': beginning of partition
	char * path_dir;	// ptr to 'path': current directory
	u32  name_pool_size;	// total size of name pool
	u32  depth;		// current depth
	u64  total_size;	// total size of all files

} scan_data_t;

//-----------------------------------------------------------------------------

static u32 scan_part ( scan_data_t * sd )
{
    ASSERT(sd);
    ASSERT(sd->part);

    char * path_end = sd->path + sizeof(sd->path) - 1;
    ASSERT(sd->path_part >= sd->path && sd->path_part < path_end);
    ASSERT(sd->path_dir  >= sd->path && sd->path_dir  < path_end);

    TRACE("scan_part(%.*s|%.*s|%s) depth=%d\n",
		(int)(sd->path_part - sd->path), sd->path,
		(int)(sd->path_dir - sd->path_part), sd->path_part,
		sd->path_dir, sd->depth );

    // save current path_dir
    char * path_dir = sd->path_dir;

    u32 count = 0;
    DIR * dir = opendir(sd->path);
    if (dir)
    {
	for (;;)
	{
	    struct dirent * dent = readdir(dir);
	    if (!dent)
		break;

	    ccp name = dent->d_name;
	    sd->path_dir = StringCopyE(path_dir,path_end,name);
	    if (FindStringField(&sd->part->exclude_list,sd->path_part))
		    continue;
	    if ( *name == '.' )
	    {
		if ( !name[1] || name[1] == '.' && !name[2] )
		    continue;
		if (!FindStringField(&sd->part->include_list,sd->path_part))
		    continue;
	    }

	    struct stat st;
	    if (stat(sd->path,&st))
		continue;

	    uint namesize;
	    if (use_utf8)
	    {
		namesize = 1;
		ccp nameptr = name;
		while (ScanUTF8AnsiChar(&nameptr))
		    namesize++;
		PRINT_IF( namesize-1 != strlen(name),
			    "NAME-LEN: %u,%zu: %s\n", namesize-1, strlen(name), name );
	    }
	    else
		namesize = strlen(name) + 1;

	    if (S_ISDIR(st.st_mode))
	    {
		count++;
		WiiFstFile_t * file = AppendFileFST(sd->part);
		ASSERT(file);

		sd->name_pool_size += namesize;
		file->icm = WD_ICM_DIRECTORY;
		*sd->path_dir++ = '/';
		*sd->path_dir = 0;
		file->path = STRDUP(sd->path_part);
		noTRACE("DIR:  %s\n",path_dir);
		file->offset4 = sd->depth++;

		// pointer 'file' becomes invalid if REALLOC() is called => store index
		const int idx = file - sd->part->file; 
		const u32 sub_count = scan_part(sd);
		sd->part->file[idx].size = sub_count;
		count += sub_count;
		sd->depth--;
	    }
	    else if (S_ISREG(st.st_mode))
	    {
		count++;
		WiiFstFile_t * file = AppendFileFST(sd->part);
		ASSERT(file);

		sd->name_pool_size += namesize;
		file->path = STRDUP(sd->path_part);
		noTRACE("FILE: %s\n",path_dir);
		file->icm  = WD_ICM_FILE;
		file->size = st.st_size;
		bool found = false;
		if (opt_links)
		{
		    file->data = (u8*)InsertFileIndex(&sd->part->fidx,
							&found,st.st_dev,st.st_ino);
		    PRINT("SET LINK: %p = %s\n",file->data,name);
		}
		if (!found)
		    sd->total_size += ALIGN32(file->size,4);
		MaxFileAttrib(&sd->part->max_fatt,0,&st);
	    }
	    // else ignore all other files
	}
	closedir(dir);
    }

    // restore path_dir
    sd->path_dir = path_dir;
    return count;
}

//-----------------------------------------------------------------------------

u32 ScanPartFST
	( WiiFstPart_t * part, ccp base_path, u32 cur_offset4, wd_boot_t * boot )
{
    ASSERT(part);
    ASSERT(base_path);

    //----- setup

    scan_data_t sd;
    memset(&sd,0,sizeof(sd));
    sd.part = part;
    sd.path_part = StringCat2S(sd.path,sizeof(sd.path),base_path,"/");

    StringCopyE(sd.path_part,sd.path+sizeof(sd.path),FST_EXCLUDE_FILE);
    LoadStringField(&part->exclude_list,false,true,sd.path,true);
    StringCopyE(sd.path_part,sd.path+sizeof(sd.path),FST_INCLUDE_FILE);
    LoadStringField(&part->include_list,false,true,sd.path,true);

    WiiParamField_t align_info;
    InitializeWiiParamField(&align_info,PFT_ALIGN);
    if (opt_align_files)
    {
	StringCopyE(sd.path_part,sd.path+sizeof(sd.path),FST_ALIGN_FILE);
	LoadWiiParamField(&align_info,0,sd.path,true);
	PRINT("LOAD-ALIGN, N=%u: %s\n",align_info.used,sd.path);
    }

    sd.path_dir = StringCopyE(sd.path_part,sd.path+sizeof(sd.path),"files/");
    WiiFstFile_t * file = AppendFileFST(sd.part);
    ASSERT(file);
    ASSERT( file == part->file );
    file->icm  = WD_ICM_DIRECTORY;
    file->path = STRDUP(sd.path_part);
    noTRACE("DIR:  %s\n",sd.path_dir);


    //----- scan files

    // pointer 'file' and 'part->file' becomes invalid if REALLOC() called
    // store the size first and then assign it (cross a sequence point)
    const u32 size = scan_part(&sd);
    ASSERT_MSG( size + 1 == part->file_used,
		"%d+1 != %d [%s]\n", size, part->file_used, part->file->path );
    part->file->size = size;

    // this sort is directory stable (first directory then contents)
    // because each directory path has a terminating '/'.
    // sorting is not neccessary but makes a listing nicer
    SortPartFST(part,SORT_NAME,SORT_NAME);


    //----- generate file table ('fst.bin' & 'fst+.bin')

    *sd.path_part = 0;
 #if SUPPORT_FST_PLUS > 1
    const u32 fst_plus_fsize = GetFileSize(sd.path,"sys/fst+.bin",0,0,false);
    const u32 fst_plus_size = fst_plus_fsize ? 4 + ALIGN32(fst_plus_fsize,4) : 0;
 #else
    const u32 fst_plus_size = 0;
 #endif

    // alloc buffer
    sd.name_pool_size = ALIGN32(sd.name_pool_size,4);
    part->ftab_size = sizeof(wd_fst_item_t) * part->file_used
			+ sd.name_pool_size + fst_plus_size;
    part->ftab = CALLOC(part->ftab_size,1);

 #if SUPPORT_FST_PLUS > 1
    if ( fst_plus_size > 0 )
    {
	u8 *dest = part->ftab + part->ftab_size - fst_plus_size;
	memcpy(dest,"fst+",4);
	LoadFile(sd.path, "sys/fst+.bin", 0, dest+4,
		fst_plus_fsize, 0, 0, false );
    }
 #endif

    wd_fst_item_t * ftab = (wd_fst_item_t*)part->ftab;
    char *namebase = (char*)(ftab + part->file_used);
    char *nameptr = namebase;
    ASSERT( (u8*)namebase + sd.name_pool_size + fst_plus_size
		== part->ftab + part->ftab_size );

    if (boot)
    {
	boot->fst_off4  = htonl(cur_offset4);
	boot->fst_size4 = boot->max_fst_size4 = htonl(part->ftab_size>>2);
 #if SUPPORT_FST_PLUS > 1
	if (fst_plus_size)
	    boot->max_fst_size4 = htonl(part->ftab_size+0x100>>2);
 #endif
    }

    IsoMappingItem_t * imi;
    imi = InsertIM(&part->im,IMT_DATA,(u64)cur_offset4<<2,part->ftab_size);
    imi->part = part;
    snprintf(imi->info,sizeof(imi->info),"fst.bin, N(fst)=%u",part->file_used);
    imi->data = part->ftab;
    cur_offset4 += part->ftab_size >> 2;


    //----- fill file table

    // setup first item
    ftab->is_dir = 1;
    ftab->size = htonl(part->file_used);
    ftab++;

    const int MAX_DIR_DEPTH = 50;
    u32 basedir[MAX_DIR_DEPTH+1];
    memset(basedir,0,sizeof(basedir));

    // calculate good start offset -> avoid first sector group
    if ( cur_offset4 < WII_GROUP_DATA_SIZE4 )
	 cur_offset4 = WII_GROUP_DATA_SIZE4;
    else
	cur_offset4 = ALIGN32(cur_offset4,64>>2);
    u32 offset4 = cur_offset4;

    // iterate files and remove directories

    u32 idx = 1, file_count = 0;
    WiiFstFile_t *file_dest = part->file;
    WiiFstFile_t *file_end  = part->file + part->file_used;
    for ( file = part->file+1; file < file_end; file++ )
    {
	ftab->name_off = htonl( nameptr - namebase );
	ccp name = file->path, ptr = name;
	while (*ptr)
	    if ( *ptr++ == '/' && *ptr )
		name = ptr;
	if (use_utf8)
	    while (*name)
		*nameptr++ = ScanUTF8AnsiChar(&name);
	else
	    while (*name)
		*nameptr++ = *name++;

	if ( file->icm == WD_ICM_DIRECTORY )
	{
	    nameptr[-1] = 0; // remove trailing '/'
	    ftab->is_dir = 1;
	    ftab->size = htonl( file->size + idx + 1 );
	    u32 depth = file->offset4;
	    if ( depth < MAX_DIR_DEPTH )
	    {
		ftab->offset4 = htonl(basedir[depth]);
		basedir[depth+1] = idx;
	    }
	    FreeString(file->path);
	}
	else
	{
	    DASSERT(file->icm == WD_ICM_FILE);
	    nameptr++; // remember CALLOC()
	    ftab->size = htonl(file->size);
	    ftab->offset4 = htonl(offset4);
	    file->offset4 = offset4;
	    bool ignore = false;
	    if (file->data)
	    {
		FileIndexItem_t * item = (FileIndexItem_t*)file->data;
		if (item->file)
		{
		    ignore = true;
		    ftab->offset4 = htonl(item->file->offset4);
		    PRINT("COPY: %p %p %8x %s %s\n",
				item, item->file, item->file->offset4<<2,
				file->path, item->file->path );
		}
		else
		{
		    item->file = file_dest;
		    PRINT("NEW:  %p %p %8x %s\n",
				item, item->file, offset4<<2, file->path );
		}
	    }

	    if (!ignore)
	    {
		char path[2000];
		if ( align_info.used && ReduceToPathAndType(path,sizeof(path),file->path) )
		{
		    const WiiParamFieldItem_t *par = FindWiiParamField(&align_info,path);
		    if ( par && par->num >= WII_SECTOR_SIZE )
		    {
			offset4 = ALIGN32(offset4,WII_SECTOR_SIZE4);
			ftab->offset4 = htonl(offset4);
			file->offset4 = offset4;
		    }
		}

		offset4 += file->size + 3 >> 2;
		memcpy(file_dest,file,sizeof(*file_dest));
		file_dest++;
	    }
	    file_count++;
	}

	noTRACE("-> %4x: %u %8x %8x %s\n",
		idx, ftab->is_dir, ntohl(ftab->offset4), ntohl(ftab->size),
		namebase + ( ntohl(ftab->name_off) & 0xffffff ));

	idx++;
	ftab++;
	ASSERT( (char*)ftab <= namebase );
	ASSERT( nameptr <= namebase + sd.name_pool_size );
    }
    part->file_used = file_dest - part->file;
    ASSERT( (char*)ftab == namebase );

 #if defined(TEST)
    if ( logging > 2 )
    {
	printf("File Index:\n");
	DumpFileIndex(stdout,3,&part->fidx);
    }
 #endif

    ResetFileIndex(&part->fidx);
    ResetWiiParamField(&align_info);

 #ifdef DEBUG
    {
	FILE * f = fopen("_fst.bin.tmp","wb");
	if (f)
	{
	    fwrite(part->ftab,1,part->ftab_size,f);
	    fclose(f);
	}
    }
 #endif

 #ifdef DEBUG
    if (Dump_FST_MEM(0,0,(wd_fst_item_t*)part->ftab,
				part->ftab_size,"scanned",WD_PFST__ALL))
    {
	Dump_FST_MEM(TRACE_FILE,0,(wd_fst_item_t*)part->ftab,
				part->ftab_size,"scanned",WD_PFST__ALL);
	return ERROR0(ERR_FATAL,"internal fst.bin is invalid -> see trace file\n");
    }
 #endif

    imi = InsertIM( &part->im, IMT_PART_FILES,
			(u64)cur_offset4<<2, (u64)(offset4-cur_offset4)<<2 );
    imi->part = part;
    snprintf(imi->info,sizeof(imi->info),"%u fst files",file_count);
    return offset4;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			generate partition		///////////////
///////////////////////////////////////////////////////////////////////////////

u64 GenPartFST
(
	SuperFile_t	* sf,		// valid file pointer
	WiiFstPart_t	* part,		// valid partition pointer
	ccp		path,		// path of partition directory
	u64		good_off,	// standard partition offset
	u64		min_off		// minimal possible partition offset
)
{
    ASSERT(sf);
    ASSERT(part);
    ASSERT(!part->file_used);


    //----- scan FST_SETUP_FILE

    ccp part_id = 0;
    ccp part_name = 0;
    part->image_type = OFT_UNKNOWN;
    if (!ignore_setup)
    {
	ScanSetupDef(path,true);
	part_id   = part_setup_def[PSUP_P_ID].param;
	part_name = part_setup_def[PSUP_P_NAME].param;
	part->image_type = part_setup_def[PSUP_I_TYPE].value;

	SetupDef_t * sdef = part_setup_def + PSUP_P_OFFSET;
	if (sdef->param)
	    good_off = sdef->value;
	PRINT("SETUP.TXT: id=%s name=%.20s off=%llx oft=%s\n",
			part_id, part_name, good_off, oft_info[part->image_type].name );
    }


    //----- setup

    if ( good_off < min_off )
	good_off = min_off;
    part->part_off = good_off;

    IsoMappingItem_t * imi;
    char pathbuf[PATH_MAX];

    const u32 good_align =  0x40;
    const u32 max_skip   = 0x100;

    WiiFst_t * fst = sf->fst;
    ASSERT(fst);


    //----- boot.bin + bi2.bin

    ASSERT( WII_BOOT_SIZE == WII_BI2_OFF );
    const u32 boot_size = WII_BOOT_SIZE + WII_BI2_SIZE;
    imi = InsertIM(&part->im,IMT_DATA,0,boot_size);
    imi->part = part;
    imi->data = MALLOC(boot_size);
    imi->data_alloced = true;
    u32 cur_offset4 = ALIGN32(boot_size>>2,good_align);

    LoadFile(path, "sys/boot.bin", 0, imi->data,
		WII_BOOT_SIZE, 0, &part->max_fatt, true );
    LoadFile(path, "sys/bi2.bin", 0, imi->data+WII_BOOT_SIZE,
		WII_BI2_SIZE, 0, &part->max_fatt, true );

    wd_boot_t * boot = imi->data;
    char * title = boot->dhead.disc_title;
    PatchDiscHeader(&boot->dhead,part_id,part_name);

    if ( part->part_type == WD_PART_DATA )
    {
	PatchId(imi->data,modify_boot_id,0,6);
	PatchName(title,WD_MODIFY_BOOT|WD_MODIFY__AUTO);
    }
    snprintf(imi->info,sizeof(imi->info),"boot.bin [%.6s] + bi2.bin",(ccp)imi->data);

    if ( !fst->dhead.disc_id && part->part_type == WD_PART_DATA )
    {
	char * dest = (char*)&fst->dhead;
	memset(dest,0,sizeof(fst->dhead));
	memcpy(dest,boot,6);
	memcpy(dest+WII_TITLE_OFF,(ccp)boot+WII_TITLE_OFF,WII_TITLE_SIZE);
	fst->dhead.wii_magic = htonl(WII_MAGIC);
	//HEXDUMP16(3,0,dest,6);
	//HEXDUMP16(3,WII_TITLE_OFF,dest+WII_TITLE_OFF,16);
    }


    //----- disc/header.bin

    if ( part->part_type == WD_PART_DATA )
    {
	LoadFile(path,"disc/header.bin",0,
			&fst->dhead, sizeof(fst->dhead), 2,
			&part->max_fatt, true);
	PatchDiscHeader(&fst->dhead,part_id,part_name);
	PatchId(&fst->dhead.disc_id,modify_disc_id,0,6);
	PatchName(fst->dhead.disc_title,WD_MODIFY_DISC|WD_MODIFY__AUTO);
    }


    //----- disc/region.bin

    if ( part->part_type == WD_PART_DATA )
    {
	enumRegion reg = opt_region;
	const RegionInfo_t * rinfo
	    = GetRegionInfo(fst->dhead.region_code);

	if ( reg == REGION__AUTO && rinfo->mandatory )
	    reg = rinfo->reg;
	else if ( reg == REGION__AUTO || reg == REGION__FILE )
	{
	    reg = LoadFile(path,"disc/region.bin",0,
				&fst->region, sizeof(fst->region), 2,
				&part->max_fatt, true )
		? REGION__AUTO : REGION__FILE;
	}

	if ( reg != REGION__FILE )
	{
	    memset( &fst->region, 0, sizeof(fst->region) );
	    fst->region.region
		= htonl(  reg == REGION__AUTO ? rinfo->reg : reg );
	}
    }


    //----- apploader.img

    ccp fpath = PathCatPP(pathbuf,sizeof(pathbuf),path,"sys/apploader.img");
    const u32 app_fsize4 = GetFileSize(fpath,0,0,&part->max_fatt,true) + 3 >> 2;
    imi = InsertIM(&part->im,IMT_FILE,WII_APL_OFF,(u64)app_fsize4<<2);
    imi->part = part;
    StringCopyS(imi->info,sizeof(imi->info),"apploader.img");
    imi->data = STRDUP(fpath);
    imi->data_alloced = true;

    cur_offset4 = ( WII_APL_OFF >> 2 ) + app_fsize4;


    //----- user.bin

 #if SUPPORT_USER_BIN > 1

    fpath = PathCatPP(pathbuf,sizeof(pathbuf),path,"sys/user.bin");
    const u32 ubin_fsize = GetFileSize(fpath,0,0,&part->max_fatt,true);
    if ( ubin_fsize > 0 )
    {
	cur_offset4 = ALIGN32(cur_offset4,WIIUSER_DATA_ALIGN>>2);

	wiiuser_header_t wuh;
	memset(&wuh,0,sizeof(wuh));
	memcpy(wuh.magic,WIIUSER_MAGIC,sizeof(wuh.magic));
	wuh.version = htonl(WIIUSER_VERSION);
	wuh.size = htonl(ubin_fsize);

	imi = InsertIM(&part->im,IMT_DATA,(u64)cur_offset4<<2,sizeof(wuh));
	imi->part = part;
	StringCopyS(imi->info,sizeof(imi->info),"user.head");
	imi->data = MEMDUP(&wuh,sizeof(wuh));
	imi->data_alloced = true;
	cur_offset4 += sizeof(wuh) + 3 >> 2;

	imi = InsertIM(&part->im,IMT_FILE,(u64)cur_offset4<<2,(u64)ubin_fsize);
	imi->part = part;
	StringCopyS(imi->info,sizeof(imi->info),"user.bin");
	imi->data = STRDUP(fpath);
	imi->data_alloced = true;
	cur_offset4 += ALIGN32(ubin_fsize,WIIUSER_DATA_ALIGN) >> 2;
    }

 #endif


    //----- main.dol

    // try to use the same offset for main.dol
    u32 good_off4 = ntohl(boot->dol_off4);
    if ( good_off4 >= cur_offset4 && good_off4 <= cur_offset4 + max_skip )
	 cur_offset4 = good_off4;
    boot->dol_off4 = htonl(cur_offset4);

    fpath = PathCatPP(pathbuf,sizeof(pathbuf),path,"sys/main.dol");
    const u32 dol_fsize4 = GetFileSize(fpath,0,0,&part->max_fatt,true) + 3 >> 2;
    imi = InsertIM(&part->im,IMT_FILE,(u64)cur_offset4<<2,(u64)dol_fsize4<<2);
    imi->part = part;
    StringCopyS(imi->info,sizeof(imi->info),"main.dol");
    imi->data = STRDUP(fpath);
    imi->data_alloced = true;

    cur_offset4	+= dol_fsize4;


    //----- fst.bin

    // try to use the same offset for fst
    good_off4 = ntohl(boot->fst_off4);
    if ( good_off4 >= cur_offset4 && good_off4 <= cur_offset4 + max_skip )
	 cur_offset4 = good_off4;

    cur_offset4 = ScanPartFST(part,path,cur_offset4,boot);
    TRACE("dol4=%x,%x  fst4=%x,%x,  cur_offset4=%x\n",
	ntohl(boot->dol_off4), dol_fsize4,
	ntohl(boot->fst_off4), ntohl(boot->fst_size4),
	cur_offset4 );


    //----- get file sizes and setup partition control

    const u32 blocks = ( cur_offset4 - 1 ) / WII_SECTOR_DATA_SIZE4 + 1;

    const s64 ticket_size = GetFileSize(path,"ticket.bin",-1,&part->max_fatt,true);
    bool load_ticket = ticket_size == WII_TICKET_SIZE;
    if ( !load_ticket && ticket_size >= 0 )
	ERROR0(ERR_WARNING,"Content of file 'ticket.bin' ignored!\n");

    const s64 tmd_size = GetFileSize(path,"tmd.bin",-1,&part->max_fatt,true);
    bool load_tmd = tmd_size == WII_TMD_GOOD_SIZE;
    if ( !load_tmd && tmd_size >= 0 )
	ERROR0(ERR_WARNING,"Content of file 'tmd.bin' ignored!\n");

    s64 cert_size = GetFileSize(path,"cert.bin",0,&part->max_fatt,true);
    const bool use_std_cert_chain = !cert_size;
    if (use_std_cert_chain)
	cert_size = sizeof(std_cert_chain);

    wd_part_control_t *pc = MALLOC(sizeof(wd_part_control_t));
    part->pc = pc;

    if (clear_part_control(pc, WII_TMD_GOOD_SIZE, cert_size,
					(u64)blocks*WII_SECTOR_SIZE) )
	return ERROR0(ERR_INVALID_FILE,"Content of file 'cert.bin' wrong!\n");


    //----- setup partition control content

    if (load_ticket)
	load_ticket = LoadFile(path,"ticket.bin", 0,
			    &pc->head->ticket, WII_TICKET_SIZE, 0,
			    &part->max_fatt, true )
		    == ERR_OK;
    if (!load_ticket)
	ticket_setup(&pc->head->ticket,&fst->dhead.disc_id);
    wd_patch_id(pc->head->ticket.title_id+4,0,part_id,4);

    if (load_tmd)
	load_tmd = LoadFile( path, "tmd.bin", 0,
				pc->tmd, pc->tmd_size, 0, &part->max_fatt, true )
		 == ERR_OK;
    if (!load_tmd)
	tmd_setup(pc->tmd,pc->tmd_size,&fst->dhead.disc_id);
    wd_patch_id(pc->tmd->title_id+4,0,part_id,4);

    if (use_std_cert_chain)
    {
	DASSERT( pc->cert_size == sizeof(std_cert_chain) );
	setup_cert_data();
	memcpy(pc->cert,std_cert_chain,pc->cert_size);
    }
    else
	LoadFile(path,"cert.bin", 0, pc->cert, pc->cert_size,
			0, &part->max_fatt, true );

    if ( part->part_type == WD_PART_DATA )
    {
	PatchId(pc->head->ticket.title_id+4,modify_ticket_id,0,4);
	PatchId(pc->tmd->title_id+4,modify_tmd_id,0,4);
	if (opt_ios_valid)
	    pc->tmd->sys_version = hton64(opt_ios);
    }


    //----- setup

    InsertMemMap(&sf->modified_list,good_off,sizeof(pc->part_bin));

    if ( fst->encoding & ENCODE_CALC_HASH )
    {
	tmd_clear_encryption(pc->tmd,0);

	// calc partition keys
	wd_decrypt_title_key(&pc->head->ticket,part->key);
	wd_aes_set_key(&part->akey,part->key);

	// fill h3 with data to avoid sparse writing
	int groups = (blocks-1)/WII_GROUP_SECTORS+1;
	if ( groups > WII_N_ELEMENTS_H3 )
	     groups = WII_N_ELEMENTS_H3;
	memset(pc->h3,1,groups*WII_HASH_SIZE);
    }
    else
    {
	ASSERT( !( fst->encoding & (ENCODE_ENCRYPT|ENCODE_SIGN) ));
	tmd_clear_encryption(pc->tmd,1);
    }


    //----- insert this partition in mem map of fst

    imi = InsertIM(&fst->im,IMT_DATA,good_off,sizeof(pc->part_bin));
    imi->data = pc->part_bin;
    snprintf(imi->info,sizeof(imi->info),"%s partition, header",
			wd_get_part_name(part->part_type,"?"));

    imi = InsertIM(&fst->im,IMT_PART, good_off+pc->data_off, pc->data_size );
    imi->part = part;
    snprintf(imi->info,sizeof(imi->info),"%s partition, data",
			wd_get_part_name(part->part_type,"?"));


    //----- terminate

    MaxFileAttrib(&sf->f.fatt,&part->max_fatt,0);
    ResetSetup(part_setup_def);
    return pc->data_size;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   SetupReadFST()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadFST ( SuperFile_t * sf )
{
    ASSERT(sf);
    ASSERT(!sf->fst);
    PRINT("SetupReadFST() -> %x %s\n",sf->f.ftype,sf->f.fname);

    SetupIOD(sf,OFT_FST,OFT_FST);
    WiiFst_t * fst = MALLOC(sizeof(*fst));
    sf->fst = fst;
    InitializeFST(fst);


    //----- setup fst->encoding mode

    enumEncoding enc = SetEncoding(encoding,0,0);

    // check ENCODE_F_ENCRYPT
    if ( enc & ENCODE_F_ENCRYPT )
	enc = SetEncoding(enc,0,ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH);

    // make some dependency checks
    if ( enc & ENCODE_SIGN )
	enc = SetEncoding(enc,ENCODE_ENCRYPT|ENCODE_CALC_HASH,0);
    if ( enc & ENCODE_M_CRYPT )
	enc = SetEncoding(enc,ENCODE_CALC_HASH,ENCODE_NO_SIGN);
    if ( enc & ENCODE_M_HASH )
	enc = SetEncoding(enc,0,ENCODE_NO_SIGN|ENCODE_DECRYPT);

    // force enrypting and signing if at least one is set
    if ( enc & (ENCODE_SIGN|ENCODE_ENCRYPT) )
	enc = SetEncoding(enc,ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH,0);

    fst->encoding = SetEncoding(enc,0,
		enc & ENCODE_F_FAST
			? ENCODE_NO_SIGN|ENCODE_DECRYPT|ENCODE_CLEAR_HASH
			: ENCODE_SIGN|ENCODE_ENCRYPT|ENCODE_CALC_HASH );
    PRINT("ENCODING: %04x -> %04x\n",encoding,fst->encoding);


    //----- setup partitions --> [[2do]] use part_selector

    u64 min_offset	= WII_PART_OFF;
    u64 update_off	= WII_PART_OFF;
    u64 update_size	= 0;
    u64 data_off	= WII_GOOD_DATA_PART_OFF;
    u64 data_size	= 0;
    u64 channel_off	= WII_GOOD_DATA_PART_OFF;
    u64 channel_size	= 0;
    WiiFstPart_t * data_part = 0;

    if ( sf->f.ftype & FT_A_PART_DIR )
    {
	min_offset = wd_align_part(min_offset,opt_align_part,false);
	data_part = AppendPartFST(fst);
	data_part->part_type = WD_PART_DATA;
	data_size = GenPartFST(sf,data_part,sf->f.fname,data_off,min_offset);
	data_off  = data_part->part_off;
	data_part->path = STRDUP(sf->f.fname);
	min_offset	= data_off
			+ WII_PARTITION_BIN_SIZE
			+ data_size;
	sf->oft_orig = data_part->image_type;
    }
    else
    {
	char data_path[PARTITION_NAME_SIZE];
	char update_path[PARTITION_NAME_SIZE];
	char channel_path[PARTITION_NAME_SIZE];

	int stat = SearchPartitionsFST(sf->f.fname,0,data_path,update_path,channel_path);
	if ( !stat & DATA_PART_FOUND )
	    return ERROR0(ERR_INVALID_FILE,"Not a valid FST: %s\n",sf->f.fname);

	char path_buf[PATH_MAX];
	if ( stat & UPDATE_PART_FOUND )
	{
	    min_offset = wd_align_part(min_offset,opt_align_part,false);
	    ccp path = PathCatPP(path_buf,sizeof(path_buf),sf->f.fname,update_path);
	    WiiFstPart_t * update_part = AppendPartFST(fst);
	    update_part->part_type = WD_PART_UPDATE;
	    update_size = GenPartFST(sf,update_part,path,update_off,min_offset);
	    update_off	= update_part->part_off;
	    min_offset	= update_off
			+ WII_PARTITION_BIN_SIZE
			+ update_size;
	    update_part->path = STRDUP(path);
	}

	min_offset = wd_align_part(min_offset,opt_align_part,false);
	ccp path = PathCatPP(path_buf,sizeof(path_buf),sf->f.fname,data_path);
	data_part = AppendPartFST(fst);
	data_part->part_type = WD_PART_DATA;
	data_size = GenPartFST(sf,data_part,path,data_off,min_offset);
	data_off  = data_part->part_off;
	min_offset	= data_off
			+ WII_PARTITION_BIN_SIZE
			+ data_size;
	data_part->path = STRDUP(path);
	sf->oft_orig = data_part->image_type;

	if ( stat & CHANNEL_PART_FOUND )
	{
	    min_offset = wd_align_part(min_offset,opt_align_part,false);
	    ccp path = PathCatPP(path_buf,sizeof(path_buf),sf->f.fname,channel_path);
	    WiiFstPart_t * channel_part = AppendPartFST(fst);
	    channel_part->part_type = WD_PART_CHANNEL;
	    channel_size = GenPartFST(sf,channel_part,path,channel_off,min_offset);
	    channel_off  = channel_part->part_off;
	    min_offset	= channel_off
			+ WII_PARTITION_BIN_SIZE
			+ channel_size;
	    channel_part->path = STRDUP(path);
	}
    }
    ASSERT(data_part);

    TRACE("UPD: %9llx .. %9llx : %9llx\n",update_off, update_off+update_size, update_size);
    TRACE("DAT: %9llx .. %9llx : %9llx\n",data_off, data_off+data_size, data_size);
    TRACE("CHN: %9llx .. %9llx : %9llx\n",channel_off, channel_off+channel_size, channel_size);


    //----- setup partition header

    const u32 n_part	    = 1 + (update_size>0) + (channel_size>0);
    const u64 cnt_tab_size  = WII_MAX_PTAB * sizeof(wd_ptab_info_t);
    const u64 part_tab_size = cnt_tab_size
			    + n_part * sizeof(wd_ptab_entry_t);

    IsoMappingItem_t * imi;
    imi = InsertIM(&fst->im,IMT_DATA,WII_PTAB_REF_OFF,part_tab_size);
    StringCopyS(imi->info,sizeof(imi->info),"partition tables");
    imi->data = CALLOC(1,part_tab_size);
    imi->data_alloced = true;

    wd_ptab_info_t * pcount = imi->data;
    pcount->n_part = htonl(n_part);
    pcount->off4   = htonl( WII_PTAB_REF_OFF + cnt_tab_size >> 2 );

    wd_ptab_entry_t * pentry = (void*)((ccp)imi->data+cnt_tab_size);
    if (update_size)
    {
	pentry->off4  = htonl( update_off >> 2 );
	pentry->ptype = htonl( WD_PART_UPDATE );
	pentry++;
    }
    pentry->off4  = htonl( data_off >> 2 );
    pentry->ptype = htonl( WD_PART_DATA );
    if (channel_size)
    {
	pentry++;
	pentry->off4  = htonl( channel_off >> 2 );
	pentry->ptype = htonl( WD_PART_CHANNEL );
    }


    //----- setup work+cache buffer of WII_GROUP_SIZE
    //		== WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2 * WII_SECTOR_SIZE

    ASSERT(!fst->cache);
    ASSERT(!fst->cache_part);
    fst->cache = MALLOC(WII_GROUP_SIZE);
    TRACE("CACHE= %x/hex = %u bytes\n",WII_GROUP_SIZE,WII_GROUP_SIZE);


    //----- setup mapping

    imi = InsertIM(&fst->im,IMT_DATA,0,sizeof(fst->dhead));
    imi->data = &fst->dhead;
    snprintf(imi->info,sizeof(imi->info),"disc header [%.6s]",(ccp)imi->data);

    imi = InsertIM(&fst->im,IMT_DATA,WII_REGION_OFF,sizeof(fst->region));
    snprintf(imi->info,sizeof(imi->info),"region settings, region=%u",
		ntohl(fst->region.region));
    imi->data = &fst->region;

    static u8 magic[] = { 0xc3, 0xf8, 0x1a, 0x8e };
    ASSERT( be32(magic) == WII_MAGIC2 );
    ASSERT( sizeof(magic) == WII_MAGIC2_LEN );
    imi = InsertIM(&fst->im,IMT_DATA,WII_MAGIC2_OFF,sizeof(magic));
    StringCopyS(imi->info,sizeof(imi->info),"magic c3-f8-1a-8e");
    imi->data = &magic;


    //----- terminate

 #ifdef DEBUG
    PrintFstIM(fst,TRACE_FILE,0,true,"Memory layout of virtual");
 #endif
    if ( logging > 0 )
	PrintFstIM(fst,stdout,0,logging>1,"Memory layout of virtual");

    const off_t single_size = WII_SECTORS_SINGLE_LAYER * (off_t)WII_SECTOR_SIZE;
    sf->file_size = min_offset > single_size ? min_offset : single_size;

    wd_disc_t * disc = OpenDiscSF(sf,false,true);
    if (!disc)
	return ERROR0(ERR_INTERNAL,"Composing error: %s\n",sf->f.fname);
    sf->f.fatt.size = wd_count_used_disc_blocks(disc,1,0) * (u64)WII_SECTOR_SIZE;

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			UpdateSignatureFST()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError UpdateSignatureFST ( WiiFst_t * fst )
{
    TRACE("UpdateSignatureFST(%p) ENCODING=%d\n",fst, fst ? fst->encoding : 0);
    if ( !fst || fst->encoding & ENCODE_CLEAR_HASH )
	return ERR_OK;

    // invalidate cache
    fst->cache_part = 0;

    // iterate partitions
    WiiFstPart_t *part, *part_end = fst->part + fst->part_used;
    for ( part = fst->part; part < part_end; part++ )
    {
	wd_part_control_t * pc = part->pc;
	TRACE(" - part #%zu: pc=%p\n",part-fst->part,pc);
	if (pc)
	{
	    if (pc->tmd_content)
		SHA1( pc->h3, pc->h3_size, pc->tmd_content->hash );
	    if ( fst->encoding & ENCODE_SIGN )
		part_control_fake_sign(pc,0);

	 #ifdef DEBUG
	    {
		u8 hash[WII_HASH_SIZE];
		SHA1( ((u8*)pc->tmd)+WII_TMD_SIG_OFF, pc->tmd_size-WII_TMD_SIG_OFF, hash );
		TRACE("FAKESIGN: ");
		TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
	    }
	 #endif
	}
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		      SF/FST read support		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadFST ( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    TRACE("---\n");
    TRACE(TRACE_RDWR_FORMAT, "#FST# ReadFST()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf);
    ASSERT(sf->fst);

    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    const IsoMappingItem_t * imi = sf->fst->im.field;
    const IsoMappingItem_t * imi_end = imi + sf->fst->im.used;
    char * dest = buf;
    sf->f.bytes_read += count;

    while ( count > 0 )
    {
	TRACE("imi-idx=%zu off=%llx data=%zx count=%zx\n",
		imi - sf->fst->im.field, (u64)off, dest-(ccp)buf, count );
	while ( imi < imi_end && off >= imi->offset + imi->size )
	    imi++;

	TRACE("imi-idx=%zu imt=%u, off=%llx size=%llx\n",
		imi - sf->fst->im.field, imi->imt, (u64)imi->offset, imi->size );

	if ( imi == imi_end || off + count <= imi->offset )
	{
	    TRACE(">FILL %zx=%zu\n",count,count);
	    memset(dest,0,count);
	    return ERR_OK;
	}

	ASSERT( off < imi->offset + imi->size && off + count > imi->offset );
	if ( off < imi->offset )
	{
	    TRACELINE;
	    size_t fill_count = count;
	    if ( fill_count > imi->offset - off )
		 fill_count = imi->offset - off;
	    TRACE(">FILL %zx=%zu\n",fill_count,fill_count);
	    memset(dest,0,fill_count);

	    off   += fill_count;
	    dest  += fill_count;
	    count -= fill_count;
	}
	ASSERT( off >= imi->offset );
	ASSERT( count );

	TRACELINE;
	const off_t  delta = off - imi->offset;
	const off_t  max_size = imi->size - delta;
	const size_t copy_count = count < max_size ? count : max_size;
	switch(imi->imt)
	{
	    case IMT_ID: // [[2do]] [[obsolete?]] is IMT_ID needed?
		DASSERT(0);
		TRACE(">ID %zx=%zu\n",copy_count,copy_count);
		PatchIdCond(dest,delta,copy_count,WD_MODIFY__ALWAYS);
		break;

	    case IMT_DATA:
		TRACE(">COPY %zx=%zu\n",copy_count,copy_count);
		memcpy(dest,(ccp)imi->data+delta,copy_count);
		break;

	    case IMT_PART:
		ASSERT(imi->part);
		{
		    const enumError err
			= ReadPartFST(sf,imi->part,delta,dest,copy_count);
		    if (err)
			return err;
		}
		break;

	    default:
		TRACELINE;
		return ERROR0(ERR_INTERNAL,0);
	}
	off   += copy_count;
	dest  += copy_count;
	count -= copy_count;
    }

    TRACE("READ done! [%u]\n",__LINE__);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadPartFST ( SuperFile_t * sf, WiiFstPart_t * part,
			off_t off, void * buf, size_t count )
{
    TRACE(TRACE_RDWR_FORMAT, "#PART# ReadPartFST()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf);
    ASSERT(sf->fst);
    ASSERT(sf->fst->cache);
    ASSERT(part);

    WiiFst_t * fst = sf->fst;
    char *dest = buf;
    TRACE("CACHE=%p, buf=%p, dest=%p\n",fst->cache,buf,dest);

    u32 group = off/WII_GROUP_SIZE;
    const off_t skip = off - group * (off_t)WII_GROUP_SIZE;
    if (skip)
    {
	TRACE("READ/skip=%llx off=%llx dest=+%zx\n",(u64)skip,(u64)off,dest-(ccp)buf);

	if ( fst->cache_part != part || fst->cache_group != group )
	{
	    const enumError err = ReadPartGroupFST(sf,part,group,fst->cache,1);
	    if (err)
		return err;
	    fst->cache_part  = part;
	    fst->cache_group = group;
	}

	ASSERT( skip < WII_GROUP_SIZE );
	u32 copy_len = WII_GROUP_SIZE - skip;
	if ( copy_len > count )
	     copy_len = count;

	TRACE("COPY/len=%x\n",copy_len);
	memcpy(dest,fst->cache+skip,copy_len);
	TRACELINE;
	dest  += copy_len;
	count -= copy_len;
	group++;
    }

    const u32 n_groups = count / WII_GROUP_SIZE;
    if ( n_groups > 0 )
    {
	TRACE("READ/n_groups=%x off=%llx dest=+%zx\n",n_groups,(u64)off,dest-(ccp)buf);

	// whole groups are not cached because the next read will read next group
	const enumError err = ReadPartGroupFST(sf,part,group,dest,n_groups);
	if (err)
	    return err;

	const size_t read_count = n_groups * (size_t)WII_GROUP_SIZE;
	dest  += read_count;
	count -= read_count;
	group += n_groups;
    }

    if ( count > 0 )
    {
	ASSERT( count < WII_GROUP_SIZE );
	TRACE("READ/count=%zx off=%llx dest=+%zx\n",count,(u64)off,dest-(ccp)buf);

	if ( fst->cache_part != part || fst->cache_group != group )
	{
	    const enumError err = ReadPartGroupFST(sf,part,group,fst->cache,1);
	    if (err)
		return err;
	    fst->cache_part  = part;
	    fst->cache_group = group;
	}
	TRACE("COPY/len=%zx\n",count);
	memcpy(dest,fst->cache,count);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadPartGroupFST ( SuperFile_t * sf, WiiFstPart_t * part,
				u32 group_no, void * buf, u32 n_groups )
{
    TRACELINE;
    TRACE(TRACE_RDWR_FORMAT, "#PART# read GROUP",
		GetFD(&sf->f), GetFP(&sf->f),
		(u64)group_no, (u64)group_no+n_groups, (size_t)n_groups, "" );

    ASSERT(sf);
    ASSERT(part);
    ASSERT(buf);

    if (!n_groups)
	return ERR_OK;

    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    //----- setup vars

    const u32 delta = n_groups * WII_SECTOR_HASH_SIZE  * WII_GROUP_SECTORS;
    const u32 dsize = n_groups * WII_GROUP_DATA_SIZE;
	  u64 off   = group_no * (u64)WII_GROUP_DATA_SIZE;
    const u64 max   = n_groups * (u64)WII_GROUP_DATA_SIZE + off;
    TRACE("delta=%x, dsize=%x, off=%llx..%llx\n",delta,dsize,off,max);

    char * dest = (char*)buf + delta, *src = dest;
    memset(dest,0,dsize);
    noPRINT("CACHE=%p, buf=%p, dest=%p, delta=%x\n",sf->fst->cache,buf,dest,delta);

    const IsoMappingItem_t * imi = part->im.field;
    const IsoMappingItem_t * imi_end = imi + part->im.used;

    //----- load data

    while ( off < max )
    {
	if (SIGINT_level>1)
	    return ERR_INTERRUPT;

	while ( imi < imi_end && off >= imi->offset + imi->size )
	    imi++;
	TRACE("imi-idx=%zu imt=%u, off=%llx size=%llx\n",
		imi - part->im.field, imi->imt, imi->offset, imi->size );
	TRACE("off=%llx..%llx  dest=%p+%zx\n",off,max,buf,dest-src);

	if ( imi == imi_end || max <= imi->offset )
	    break;

	ASSERT( off < imi->offset + imi->size && max > imi->offset );
	if ( off < imi->offset )
	{
	    TRACELINE;
	    size_t skip_count = max-off;
	    if ( skip_count > imi->offset - off )
		 skip_count = imi->offset - off;
	    TRACE("SKIP %zx\n",skip_count);
	    off   += skip_count;
	    dest  += skip_count;
	}
	ASSERT( off >= imi->offset );

	TRACELINE;

	const u32 skip_count = off - imi->offset;
	ASSERT( skip_count <= imi->size );
	u32 max_copy = max - off;
	if ( max_copy > imi->size - skip_count )
	     max_copy = imi->size - skip_count;

	switch(imi->imt)
	{
	    case IMT_ID: // [[2do]] [[obsolete?]] is IMT_ID needed?
		DASSERT(0);
		noTRACE("IMT_ID: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		PatchIdCond(dest,skip_count,max_copy,WD_MODIFY__ALWAYS);
		break;

	    case IMT_DATA:
		noTRACE("IMT_DATA: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		ASSERT(imi->data);
		memcpy(dest,imi->data+skip_count,max_copy);
		//HEXDUMP16(3,0,dest,max_copy<0x40?max_copy:0x40);
		break;

	    case IMT_FILE:
		noTRACE("IMT_FILE: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		ASSERT(imi->data);
		LoadFile(imi->data,0,skip_count,dest,max_copy,0,0,false);
		break;

	    case IMT_PART_FILES:
		noTRACE("IMT_PART_FILES: %x %x -> %zx (%s)\n",
			skip_count, max_copy, dest-src, imi->info );
		if (part->file_used)
		{
		    WiiFstFile_t * file = FindFileFST(part,off>>2);
		    WiiFstFile_t * file_end = part->file + part->file_used;
		    ASSERT(file);
		    u64  loff	= off;	// local copy
		    char *ldest	= dest;	// local copy
		    while ( file < file_end && loff < max )
		    {
			u64 foff = (u64)file->offset4 << 2;
			if ( loff < foff )
			{
			    noPRINT("SKIP %9llx [%s]\n",foff-loff,file->path);
			    ldest += foff - loff;
			    loff = foff;
			}

			if ( loff < foff + file->size )
			{
			    u32 skip = loff - foff;
			    u32 load_size = file->size - skip;
			    if ( load_size > max-loff )
				 load_size = max-loff;
			    noPRINT("LOAD %9llx %p [%s]\n",loff,ldest,file->path);
			    LoadFile(part->path,file->path,skip,ldest,load_size,0,0,false);
			    ldest += load_size;
			    loff  += load_size;
			}
			file++;
		    }
		}
		break;

	    default:
		TRACELINE;
		return ERROR0(ERR_INTERNAL,0);
	}
	//TRACE_HEXDUMP16(3,0,dest,max_copy<0x40?max_copy:0x40);
	off  += max_copy;
	dest += max_copy;
    }

    //----- move data into blocks

    dest = buf;
    for ( dest = buf; dest < src; )
    {
	memset(dest,0,WII_SECTOR_HASH_SIZE);
	dest += WII_SECTOR_HASH_SIZE;

	TRACE("GROUP %u+%u (%zx<-%zx)\n",
		group_no, (u32)(dest-(ccp)buf)/WII_SECTOR_SIZE,
		dest-(ccp)buf, src-(ccp)buf );

	memmove(dest,src,WII_SECTOR_DATA_SIZE);
	dest += WII_SECTOR_DATA_SIZE;
	src  += WII_SECTOR_DATA_SIZE;
    };
    ASSERT( dest == src );
    ASSERT( dest == (ccp)buf + n_groups * WII_GROUP_SIZE );


    //----- encrypt groups

    if ( sf->fst->encoding & ENCODE_CALC_HASH )
    {
	for ( dest = buf; dest < src; dest += WII_GROUP_SIZE )
	{
	    EncryptSectorGroup
	    (
		sf->fst->encoding & ENCODE_ENCRYPT ? &part->akey : 0,
		(wd_part_sector_t*)dest,
		WII_GROUP_SECTORS,
		group_no < WII_N_ELEMENTS_H3 ? part->pc->h3 + group_no * WII_HASH_SIZE : 0
	    );
	    group_no++;
	}
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			EncryptSectorGroup()		///////////////
///////////////////////////////////////////////////////////////////////////////

void EncryptSectorGroup
(
    const aes_key_t	* akey,
    wd_part_sector_t	* sect0,
    size_t		n_sectors,
    void		* h3
)
{
    // ASSUME: all header data is cleared (set to 0 in each byte)

    ASSERT(sect0);
    TRACE("EncryptSectorGroup(%p,%p,%zx,h3=%p)\n",akey,sect0,n_sectors,h3);

    if (!n_sectors)
	return;

    if ( n_sectors > WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2 )
	 n_sectors = WII_N_ELEMENTS_H1 * WII_N_ELEMENTS_H2;

    wd_part_sector_t * max_sect = sect0 + n_sectors;

    //----- calc SHA-1 for each H0 data for each sector

    int i, j;
    wd_part_sector_t *sect = sect0;
    for ( i = 0; i < n_sectors; i++, sect++ )
	for ( j = 0; j < WII_N_ELEMENTS_H0; j++ )
	    SHA1(sect->data[j],WII_H0_DATA_SIZE,sect->h0[j]);

    //----- calc SHA-1 for each H0 hash and copy to others

    for ( sect = sect0; sect < max_sect; )
    {
	wd_part_sector_t * store_sect = sect;
	for ( j = 0; j < WII_N_ELEMENTS_H1 && sect < max_sect; j++, sect++ )
	    SHA1(*sect->h0,sizeof(sect->h0),store_sect->h1[j]);

	wd_part_sector_t * dest_sect;
	for ( dest_sect = store_sect + 1; dest_sect < sect; dest_sect++ )
	    memcpy(dest_sect->h1,store_sect->h1,sizeof(dest_sect->h1));
    }

    //----- calc SHA-1 for each H1 hash and copy to others

    for ( j = 0, sect = sect0; sect < max_sect; j++, sect += WII_N_ELEMENTS_H1 )
	SHA1(*sect->h1,sizeof(sect->h1),sect0->h2[j]);

    for ( sect = sect0+1; sect < max_sect; sect++ )
	memcpy(sect->h2,sect0->h2,sizeof(sect->h2));

    //----- calc SHA-1 of last header and store it in h3

    if (h3)
	SHA1(*sect0->h2,sizeof(sect0->h2),h3);

    //----- encrypt header + data

    if (akey)
	wd_encrypt_sectors(0,akey,sect0,0,sect0,n_sectors);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Verify			///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef TEST // [[obsolete]] [[2do]]
  #define NEW_VERIFY_UI 1
#else
  #define NEW_VERIFY_UI 0
#endif

#ifdef TEST
  #define WATCH_BLOCK 0
#else
  #define WATCH_BLOCK 0
#endif

///////////////////////////////////////////////////////////////////////////////

void InitializeVerify ( Verify_t * ver, SuperFile_t * sf )
{
    TRACE("#VERIFY# InitializeVerify(%p,%p)\n",ver,sf);
    ASSERT(ver);
    memset(ver,0,sizeof(*ver));

    ver->sf		= sf;
    ver->verbose	= verbose;
    ver->max_err_msg	= 10;
}

///////////////////////////////////////////////////////////////////////////////

void ResetVerify ( Verify_t * ver )
{
    TRACE("#VERIFY# ResetVerify(%p)\n",ver);
    ASSERT(ver);
    InitializeVerify(ver,ver->sf);
}

///////////////////////////////////////////////////////////////////////////////

static enumError PrintVerifyMessage ( Verify_t * ver, ccp msg )
{
    DASSERT(ver);
    if ( ver->verbose >= -1 )
    {
	DASSERT(ver->sf);
	DASSERT(ver->part);
	DASSERT(msg);

	wd_part_t * part = ver->part;
	DASSERT(part);

	char pname_buf[20];
	wd_print_part_name(pname_buf,sizeof(pname_buf),part->part_type,WD_PNAME_NAME);

	char count_buf[100];
	if (!ver->disc_index)
	{
	    // no index supported
	    snprintf( count_buf, sizeof(count_buf),".%u%c",
			part->index,
			part->is_encrypted ? ' ' : 'd' );
	}
	else if ( ver->disc_total < 2 )
	{
	    // a single check
	    snprintf( count_buf, sizeof(count_buf),"%u.%u%c",
			ver->disc_index,
			part->index,
			part->is_encrypted ? ' ' : 'd' );
	}
	else
	{
	    // calculate fw
	    const int fw = snprintf(count_buf,sizeof(count_buf),"%u",ver->disc_total);
	    snprintf( count_buf, sizeof(count_buf),"%*u.%u/%u%c",
			fw,
			ver->disc_index,
			part->index,
			ver->disc_total,
			part->is_encrypted ? ' ' : 'd' );
	}

	printf("%*s%-7.7s %s %n%-7s %6.6s %s\n",
		ver->indent, "",
		msg, count_buf, &ver->info_indent,
		pname_buf, (ccp)&ver->sf->disc2->dhead,
		ver->fname ? ver->fname : ver->sf->f.fname );
	fflush(stdout);

	TRACE("%*s%-7.7s %s %n%-7s %6.6s %s\n",
		ver->indent, "",
		msg, count_buf, &ver->info_indent,
		pname_buf, (ccp)&ver->sf->disc2->dhead,
		ver->fname ? ver->fname : ver->sf->f.fname );
    }
    return ERR_DIFFER;
}

///////////////////////////////////////////////////////////////////////////////

static enumError VerifyHash
	( Verify_t * ver, ccp msg, u8 * data, size_t data_len, u8 * ref )
{
    DASSERT(ver);
    DASSERT(ver->part);

 #if WATCH_BLOCK && 0
    {
	const u32 delta_off = (ccp)data - iobuf;
	const u32 delta_blk = delta_off / WII_SECTOR_SIZE;
	const u64 offset    = ver->part->part_offset
			    + ver->part->pc->data_off
			    + ver->group * (u64)WII_GROUP_SIZE
			    + delta_off;
	const u32 block	    = offset / WII_SECTOR_SIZE;

	if ( block == WATCH_BLOCK )
	    printf("WB: %s: delta=%x->%x, cmp(%02x,%02x)\n",
			msg, delta_off, delta_blk, *hash, *ref );
    }
 #endif

    u8 hash[WII_HASH_SIZE];
    SHA1(data,data_len,hash);
    if (!memcmp(hash,ref,WII_HASH_SIZE))
	return ERR_OK;

    PrintVerifyMessage(ver,msg);
    if ( ver->verbose >= -1 && ver->long_count > 0 )
    {
	bool flush = false;

	const u32 delta_off	= (ccp)data - iobuf;
	const u32 delta_blk	= delta_off / WII_SECTOR_SIZE;
	const u64 offset	= ( (u64)ver->part->data_off4 << 2 )
				+ ver->group * (u64)WII_GROUP_SIZE
				+ delta_off;
	const int ref_delta	= ref - data; 

	if ( (ccp)ref >= iobuf && (ccp)ref <= iobuf + sizeof(iobuf) )
	{
	    const u32 block	= offset / WII_SECTOR_SIZE;
	    const u32 block_off	= offset - block * (u64)WII_SECTOR_SIZE;
	    const u32 sub_block	= block_off / WII_SECTOR_HASH_SIZE;

	    printf("%*sgroup=%x.%x, block=%x.%x, data-off=%llx=B+%x, hash-off=%llx=B+%x\n",
			ver->info_indent, "",
			ver->group, delta_blk, block, sub_block,
			offset, block_off, offset + ref_delta, block_off + ref_delta );
	    flush = true;
	}

	if ( ver->long_count > 1 )
	{
	    const size_t dsize = data_len < 16 ?  data_len : 16;
	    printf("%*sDATA",ver->info_indent,"");
	    HexDump(stdout,0,offset,10,dsize,data,dsize);

	    printf("%*sHASH",ver->info_indent,"");
	    HexDump(stdout,0,offset+ref_delta,10,-WII_HASH_SIZE,ref,WII_HASH_SIZE);

	    printf("%*sVERIFY",ver->info_indent,"");
	    HexDump(stdout,0,0,8,-WII_HASH_SIZE,hash,WII_HASH_SIZE);

	    flush = true;
	}

	if (flush)
	    fflush(stdout);
    }
    return ERR_DIFFER;
}

///////////////////////////////////////////////////////////////////////////////

enumError VerifyPartition ( Verify_t * ver )
{
    DASSERT(ver);
    DASSERT(ver->sf);
    DASSERT(ver->part);

    TRACE("#VERIFY# VerifyPartition(%p) sf=%p utab=%p part=%p\n",
		ver, ver->sf, ver->usage_tab, ver->part );
    TRACE(" - psel=%p, v=%d, lc=%d, maxerr=%d\n",
		ver->psel, ver->verbose, ver->long_count, ver->max_err_msg );

 #if WATCH_BLOCK
    printf("WB: WATCH BLOCK %x = %u\n",WATCH_BLOCK,WATCH_BLOCK);
 #endif

    DASSERT( sizeof(iobuf) >= 2*WII_GROUP_SIZE );
    if ( sizeof(iobuf) < WII_GROUP_SIZE )
	return ERROR0(ERR_INTERNAL,0);

    ver->indent = NormalizeIndent(ver->indent);

    if (  ver->verbose > 0
	|| ver->verbose >= 0
	    && ( !ver->part->is_encrypted
		|| ! ( CERT_F_HASH_OK
			& wd_get_cert_ticket_stat(ver->part,true)
			& wd_get_cert_tmd_stat(ver->part,true) )))
    {
	PrintVerifyMessage(ver,">scan");
	printf("%*s%-*s%s\n",
		ver->indent, "",
		ver->info_indent+8-ver->indent, ">info",
		wd_print_sig_status(0,0,ver->part,true,true) );
    }
    else if ( ver->verbose > 1 )
	PrintVerifyMessage(ver,">scan");


    //----- check partition header

    wd_part_t * part = ver->part;
    wd_load_part(part,false,true,false);

    if (!part->is_valid)
    {
	PrintVerifyMessage(ver,"!INVALID");
	return ERR_DIFFER;
    }

    if (tmd_is_marked_not_encrypted(part->tmd))
    {
	PrintVerifyMessage(ver,"!NO-HASH");
	return ERR_DIFFER;
    }


    //----- calculate end of blocks

    u32 block = part->data_off4 / WII_SECTOR_SIZE4;
    u32 block_end = block + part->ph.data_size4 / WII_SECTOR_SIZE4;
    if ( block_end > WII_MAX_SECTORS )
	 block_end = WII_MAX_SECTORS;

    TRACE(" - Partition %u [%s], valid=%d, blocks=%x..%x [%llx..%llx]\n",
		part->part_type,
		wd_get_part_name(part->part_type,"?"),
		part->is_valid,
		block, block_end,
		block * (u64)WII_SECTOR_SIZE, block_end * (u64)WII_SECTOR_SIZE );
    //TRACE_HEXDUMP16(0,block,ver->usage_tab,block_end-block);


    //----- more setup

    DASSERT(part->h3);
    const u8  usage_tab_marker = part->usage_id | WD_USAGE_F_CRYPT;
    const int max_differ_count = ver->verbose <= 0
				? 1
				: ver->max_err_msg > 0
					? ver->max_err_msg
					: INT_MAX;
    int differ_count = 0;

    aes_key_t akey;
    wd_aes_set_key(&akey,part->key);

    for ( ver->group = 0; block < block_end; ver->group++, block += WII_GROUP_SECTORS )
    {
	//----- preload data

     #if WATCH_BLOCK
	bool wb_trigger = false;
     #endif

	u32 blk = block;
	int index, found = -1, found_end = -1;
	for ( index = 0; blk < block_end && index < WII_GROUP_SECTORS; blk++, index++ )
	{
	 #if WATCH_BLOCK
	    if ( blk == WATCH_BLOCK )
	    {
		wb_trigger = true;
		printf("WB: ver->usage_tab[%x] = %x, usage_tab_marker=%x\n",
			blk, ver->usage_tab[blk], usage_tab_marker );
	    }
	 #endif
	    if ( ver->usage_tab[blk] == usage_tab_marker )
	    {
		found_end = index+1;
		if ( found < 0 )
		    found = index;
	    }
	}

     #if WATCH_BLOCK
	if (wb_trigger)
	    printf("WB: found=%d..%d\n",found,found_end);
     #endif

	if ( found < 0 )
	{
	    // nothing to do
	    continue;
	}

	const u64 read_off = (block+found) * (u64)WII_SECTOR_SIZE;
	wd_part_sector_t * read_sect = (wd_part_sector_t*)iobuf + found;
	if ( part->is_encrypted )
	    read_sect += WII_GROUP_SECTORS; // inplace decryption not possible
	const enumError err
	    = ReadSF( ver->sf, read_off, read_sect, (found_end-found)*WII_SECTOR_SIZE );
	if (err)
	    return err;

	//----- iterate through preloaded data

	blk = block;
	wd_part_sector_t * sect_h2 = 0;
	int i2; // iterate through H2 elements
	for ( i2 = 0; i2 < WII_N_ELEMENTS_H2 && blk < block_end; i2++ )
	{
	    wd_part_sector_t * sect_h1 = 0;
	    int i1; // iterate through H1 elements
	    for ( i1 = 0; i1 < WII_N_ELEMENTS_H1 && blk < block_end; i1++, blk++ )
	    {
		if (SIGINT_level>1)
		    return ERR_INTERRUPT;

		if ( ver->usage_tab[blk] != usage_tab_marker )
		    continue;

		//----- we have found a used blk -----

		wd_part_sector_t *sect	= (wd_part_sector_t*)iobuf
					+ i2 * WII_N_ELEMENTS_H1 + i1;

		if ( part->is_encrypted )
		    wd_decrypt_sectors(0,&akey,sect+WII_GROUP_SECTORS,sect,0,1);


		//----- check H0 -----

		int i0;
		for ( i0 = 0; i0 < WII_N_ELEMENTS_H0; i0++ )
		{
		    if (VerifyHash(ver,"!H0-ERR",sect->data[i0],WII_H0_DATA_SIZE,sect->h0[i0]))
		    {
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}

		//----- check H1 -----

		if (VerifyHash(ver,"!H1-ERR",*sect->h0,sizeof(sect->h0),sect->h1[i1]))
		{
		    if ( ++differ_count >= max_differ_count )
			goto abort;
		    continue;
		}

		//----- check first H1 -----

		if (!sect_h1)
		{
		    // first valid H1 sector
		    sect_h1 = sect;

		    //----- check H1 -----

		    if (VerifyHash(ver,"!H2-ERR",*sect->h1,sizeof(sect->h1),sect->h2[i2]))
		    {
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}
		else
		{
		    if (memcmp(sect->h1,sect_h1->h1,sizeof(sect->h1)))
		    {
			PrintVerifyMessage(ver,"!H1-DIFF");
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}

		//----- check first H2 -----

		if (!sect_h2)
		{
		    // first valid H2 sector
		    sect_h2 = sect;

		    //----- check H3 -----

		    u8 * h3 = part->h3 + ver->group * WII_HASH_SIZE;
		    if (VerifyHash(ver,"!H3-ERR",*sect->h2,sizeof(sect->h2),h3))
		    {
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}
		else
		{
		    if (memcmp(sect->h2,sect_h2->h2,sizeof(sect->h2)))
		    {
			PrintVerifyMessage(ver,"!H2-DIFF");
			if ( ++differ_count >= max_differ_count )
			    goto abort;
		    }
		}
	    }
	}
    }

    //----- check H4 -----

    if ( part->tmd && part->tmd->n_content > 0 )
    {
	u8 hash[WII_HASH_SIZE];
	SHA1( part->h3, WII_H3_SIZE, hash );
	if (memcmp(hash,part->tmd->content[0].hash,WII_HASH_SIZE))
	{
	    PrintVerifyMessage(ver,"!H4-ERR");
	    ++differ_count;
	}
    }

    //----- terminate partition ------

abort:

    if ( ver->verbose >= 0 )
    {
	if (!differ_count)
	    PrintVerifyMessage(ver,"+OK");
	else if ( ver->verbose > 0 && differ_count >= max_differ_count )
	    PrintVerifyMessage(ver,"!ABORT");
    }

    return differ_count ? ERR_DIFFER : ERR_OK;
};

///////////////////////////////////////////////////////////////////////////////

enumError VerifyDisc ( Verify_t * ver )
{
    DASSERT(ver);
    DASSERT(ver->sf);
    TRACE("#VERIFY# VerifyDisc(%p) sf=%p utab=%p part=%p\n",
		ver, ver->sf, ver->usage_tab, ver->part );
    TRACE(" - psel=%p, v=%d, lc=%d, maxerr=%d\n",
		ver->psel, ver->verbose, ver->long_count, ver->max_err_msg );


    //----- setup Verify_t data

    wd_disc_t * disc = OpenDiscSF(ver->sf,true,true);
    if (!disc)
	return ERR_WDISC_NOT_FOUND;

    u8 local_usage_tab[WII_MAX_SECTORS];
    if (!ver->usage_tab)
    {
	TRACE(" - use local_usage_tab\n");
	ver->usage_tab = local_usage_tab;
	wd_filter_usage_table(disc,local_usage_tab,ver->psel);
    }


    //----- iterate partitions

    enumError err = ERR_OK;
    int pi, differ_count = 0;

    if ( ver->verbose < 0 )
    {
	// find invalid partitions first for faster results, use disc1
	wd_disc_t * disc1 = ver->sf->disc1;
	for ( pi = 0; pi < disc1->n_part && !SIGINT_level; pi++ )
	{
	    ver->part = wd_get_part_by_index(disc1,pi,0);
	    if ( ver->part->is_enabled && !ver->part->is_valid )
	    {
		PrintVerifyMessage(ver,"!INVALID");
		err = ERR_DIFFER;
		break;
	    }
	}
    }

    if (!err)
    {
	for ( pi = 0; pi < disc->n_part && !SIGINT_level; pi++ )
	{
	    ver->part = wd_get_part_by_index(disc,pi,0);
	    if ( ver->part->is_enabled )
	    {
		err = VerifyPartition(ver);
		if ( err == ERR_DIFFER )
		{
		    differ_count++;
		    if ( ver->verbose < 0 )
			break;
		}
		else if (err)
		    break;
	    }
	}
    }

    //----- terminate

    if ( ver->usage_tab == local_usage_tab )
	ver->usage_tab = 0;

    return err ? err : differ_count ? ERR_DIFFER : ERR_OK;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  Skeletonize			///////////////
///////////////////////////////////////////////////////////////////////////////

static void skel_copy_phead
(
    wd_memmap_t		* mm,		// valid memory map
    void		* data,		// pointer to data
    u64			offset,		// disc offset
    u32			size,		// data size
    ccp			info,		// info
    u32			id4_off		// NULL or data offset of ID4
)
{
    DASSERT(mm);
    ASSERT(data);
    wd_memmap_item_t * mi = wd_insert_memmap(mm,0,offset,size);
    DASSERT(mi);
    mi->data = data;
    if (id4_off)
	snprintf(mi->info,sizeof(mi->info),"%s, id=%s",
		info, wd_print_id((ccp)data+id4_off,4,0) );
    else
	StringCopyS(mi->info,sizeof(mi->info),info);
}

///////////////////////////////////////////////////////////////////////////////

static enumError skel_load_part
(
    wd_memmap_t		* mm,		// valid memory map
    wd_part_t		* part,		// valid partition
    u64			data_offset4,	// partition data offset
    u32			data_size,	// data size
    ccp			info,		// info
    bool		have_id		// true: ID6 available
)
{
    DASSERT(mm);
    DASSERT(part);

    //--- calculate disc offset

    u8 * data = MALLOC(data_size);
    const enumError err = wd_read_part(part,data_offset4,data,data_size,false);
    if (err)
    {
	FREE(data);
	return err;
    }

    u64 disc_off_begin = wd_calc_disc_offset(part,data_offset4);
    const u32 calc_size = part->is_gc ? data_size : data_size + 3 >> 2;
    u64 disc_off_end   = wd_calc_disc_offset(part,data_offset4+calc_size);
    PRINT("P: %llx+%x -> %llx..%llx: %s\n",
		data_offset4, data_size,
		disc_off_begin, disc_off_end, info );

    int idx = 0;
    while ( disc_off_begin < disc_off_end )
    {
	u64 end = ( disc_off_begin / WII_SECTOR_SIZE + 1 ) * WII_SECTOR_SIZE;
	if ( part->is_gc || end > disc_off_end )
	     end = disc_off_end;

	const u32 chunk_size = end - disc_off_begin;
	wd_memmap_item_t * mi
		= wd_insert_memmap(mm,0,disc_off_begin,chunk_size);
	DASSERT(mi);
	mi->data = data;
	if (!idx)
	    mi->data_alloced = true;

	if ( idx++ || end < disc_off_end )
	    // id support is here not needed!
	    snprintf(mi->info,sizeof(mi->info),"%s #%u",info,idx);
	else
	    snprintf(mi->info,sizeof(mi->info),"%s%s%s",
			info, have_id ? ", id=" : "",
			have_id ? wd_print_id(data,6,0) : "" );

	data += chunk_size;
	disc_off_begin += chunk_size + WII_SECTOR_HASH_SIZE;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError SkeletonizeWii
(
    SuperFile_t		* fi,		// valid input file
    wd_disc_t		* disc,		// valid disc pointer
    wd_memmap_t		* mm		// valid memory map
)
{
    DASSERT(fi);
    DASSERT(disc);
    DASSERT( disc->disc_type == WD_DT_WII );
    DASSERT(mm);

    //--- collect data

    wd_memmap_item_t * mi = wd_insert_memmap_alloc(mm,0,0,WII_PART_OFF);
    DASSERT( mi && mi->data );
    snprintf(mi->info,sizeof(mi->info),"Disc header of %s",fi->f.id6_dest);
    enumError err = ReadSF(fi,0,mi->data,WII_PART_OFF);
    if (err)
	return err;

    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,2);
	if (!part)
	    continue;

	skel_copy_phead( mm, &part->ph,
			(u64)part->part_off4<<2,
			sizeof(part->ph), "ticket", WII_TICKET_ID4_OFF );

	skel_copy_phead( mm, part->tmd,
			(u64)(part->part_off4+part->ph.tmd_off4)<<2,
			part->ph.tmd_size, "tmd", WII_TMD_ID4_OFF );

	skel_copy_phead( mm, part->cert,
			(u64)(part->part_off4+part->ph.cert_off4)<<2,
			part->ph.cert_size, "cert", 0 );

	const size_t ssm = sizeof(skeleton_marker);
	mi = wd_insert_memmap_alloc(mm,0,
			wd_calc_disc_offset(part,0)-ssm,ssm);
	DASSERT(mi);
	memcpy(mi->data,skeleton_marker,ssm);
	snprintf(mi->info,sizeof(mi->info),"Marker '%s'",skeleton_marker);

	err = skel_load_part( mm, part,
			WII_BOOT_OFF>>2, WII_BOOT_SIZE,
			"boot.bin", false );
	if (err)
	    return err;

	err = skel_load_part( mm, part,
			WII_BI2_OFF>>2, WII_BI2_SIZE,
			"bi2.bin", false );
	if (err)
	    return err;

	err = skel_load_part( mm, part,
			part->boot.fst_off4,
			part->boot.fst_size4<<2,
			"fst.bin", false );
	if (err)
	    return err;

	err = skel_load_part( mm, part,
			part->boot.dol_off4,
			DOL_HEADER_SIZE,
			"main.dol/header", false );
	if (err)
	    return err;

	err = skel_load_part( mm, part,
			WII_APL_OFF >> 2,
			0x20,
			"apploader/header", false );
	if (err)
	    return err;

	hton_part_header(&part->ph,&part->ph); // inplace because values not longer needed
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError SkeletonizeGC
(
    SuperFile_t		* fi,		// valid input file
    wd_disc_t		* disc,		// valid disc pointer
    wd_memmap_t		* mm		// valid memory map
)
{
    DASSERT(fi);
    DASSERT(disc);
    DASSERT( disc->disc_type == WD_DT_GAMECUBE );
    DASSERT(mm);

//DEL	 #ifndef TEST
//DEL	    return ERROR0(ERR_WRONG_FILE_TYPE,
//DEL			"SKELETON: GameCube not supported: %s\n",fi->f.fname);
//DEL	 #endif

 #if 0
    if ( disc->disc_attrib & WD_DA_GC_MULTIBOOT )
	return ERROR0(ERR_WRONG_FILE_TYPE,
		"SKELETON: GameCube multiboot discs not supported: %s\n",fi->f.fname);
 #endif

    // GC: non shifted offsets

    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,2);
	if (!part)
	    continue;

	enumError err = skel_load_part( mm, part,
			WII_BOOT_OFF>>2, WII_BOOT_SIZE,
			"boot.bin", true );
	if (err)
	    return err;

	const size_t ssm = sizeof(skeleton_marker);
	err = skel_load_part( mm, part,
			WII_BI2_OFF, WII_BI2_SIZE - ssm,
			"bi2.bin", true );
	if (err)
	    return err;

	wd_memmap_item_t * mi
	    = wd_insert_memmap_alloc(mm,0, WII_BI2_OFF + WII_BI2_SIZE - ssm, ssm );
	DASSERT(mi);
	memcpy(mi->data,skeleton_marker,ssm);
	snprintf(mi->info,sizeof(mi->info),"Marker '%s'",skeleton_marker);

	err = skel_load_part( mm, part,
			part->boot.fst_off4,
			part->boot.fst_size4<<2,
			"fst.bin", false );
	if (err)
	    return err;

	err = skel_load_part( mm, part,
			part->boot.dol_off4,
			DOL_HEADER_SIZE,
			"main.dol/header", false );
	if (err)
	    return err;

	err = skel_load_part( mm, part,
			WII_APL_OFF,
			0x20,
			"apploader/header", false );
	if (err)
	    return err;

	hton_part_header(&part->ph,&part->ph); // inplace because values not longer needed
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError Skeletonize
(
    SuperFile_t		* fi,		// valid input file
    ccp			path,		// NULL or path for logging
    int			disc_index,	// 1 based index of disc
    int			disc_total	// total numbers of discs
)
{
    DASSERT(fi);
    DASSERT( disc_total > 0 );
    DASSERT( disc_index > 0 && disc_index <= disc_total );

    if ( testmode || verbose >= 0 )
	printf("SKELETONIZE %u/%u [%s] %s\n",
		disc_index, disc_total,
		fi->f.id6_dest, path ? path : fi->f.fname );

    wd_memmap_t mm;
    memset(&mm,0,sizeof(mm));

    enumError err = ERR_OK;
    wd_disc_t * disc = OpenDiscSF(fi,true,true);
    if (!disc)
    {
	err = ERR_WDISC_NOT_FOUND;
	goto abort;
    }

    switch (disc->disc_type)
    {
	case WD_DT_GAMECUBE:
	    err = SkeletonizeGC(fi,disc,&mm);
	    break;

	case WD_DT_WII:
	    err = SkeletonizeWii(fi,disc,&mm);
	    break;

	default:
	    err = ERROR0(ERR_WRONG_FILE_TYPE,
		"SKELETON: Disk type not supported: %s\n",fi->f.fname);
    }
    if (err)
	goto abort;


    //--- calc sha1 + fname

    bool local_mkdir = opt_mkdir;
    ccp local_dest = opt_dest;
    if ( !local_dest || !*local_dest )
    {
	local_mkdir = true;
	local_dest  = "./.skel";
    }

    char fname[PATH_MAX];
    const enumOFT oft = CalcOFT(output_file_type,0,0,OFT__WDF_DEF);

    {
	WIT_SHA_CTX ctx;
	if (!WIT_SHA1_Init(&ctx))
	{
	    ASSERT(0);
	    exit(0);
	}

	int i;
	for ( i = 0; i < mm.used; i++ )
	{
	    wd_memmap_item_t * mi = mm.item + i;
	    noPRINT("### %p %9llx %6llx\n",mi->data,mi->offset,mi->size);
	    WIT_SHA1_Update(&ctx,mi->data,mi->size);
	}
	u8 h[WII_HASH_SIZE];
	WIT_SHA1_Final(h,&ctx);

	snprintf(fname,sizeof(fname),
		"%s/%.6s-%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
		     "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%s",
		local_dest, fi->f.id6_dest,
		h[0], h[1], h[2], h[3], h[4], 
		h[5], h[6], h[7], h[8], h[9], 
		h[10], h[11], h[12], h[13], h[14], 
		h[15], h[16], h[17], h[18], h[19],
		output_file_type == OFT_UNKNOWN ? ".skel" : oft_info[oft].ext1 );

    }


    //--- create output filename

    SuperFile_t fo;
    InitializeSF(&fo);
    SetupIOD(&fo,oft,oft);
    fo.f.create_directory = local_mkdir;

    GenImageWFileName(&fo.f,fname,0,oft);
    SubstFileNameSF(&fo,fi,0);

    if ( testmode || verbose >= 0 )
	printf(" - %sCREATE %s:%s\n",
		testmode ? "WOULD " : "", oft_info[oft].name, fo.f.fname );


    //--- create output

    if (!testmode)
    {
	err = CreateWFile( &fo.f, 0, oft_info[oft].iom, true );
	if (!err)
	{
	    err = SetupWriteSF(&fo,oft);
	    MarkMinSizeSF(&fo,fi->file_size);
	    if (!err)
	    {
		int i;
		for ( i = 0; !err && i < mm.used; i++ )
		{
		    wd_memmap_item_t * mi = mm.item + i;
		    err = WriteSparseSF(&fo,mi->offset,mi->data,mi->size);
		}
	    }
	}

	if (err)
	    RemoveSF(&fo);
	else
	    err = ResetSF(&fo,&fi->f.fatt);
    }
    ResetSF(&fo,0);


    //--- terminate

 abort:
    if (logging)
    {
	printf("\n   SKELETON Memory Map:\n\n");
	wd_print_memmap(stdout,3,&mm);
	putchar('\n');
    }
    wd_reset_memmap(&mm);

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

