
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

#include "dclib/dclib-debug.h"
#include "libwbfs.h"
#include "lib-sf.h"
#include "wbfs-interface.h"
#include "titles.h"
#include "cert.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                 SuperFile_t: setup              ///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    memset(sf,0,sizeof(*sf));
    InitializeWFile(&sf->f);
    InitializeMemMap(&sf->modified_list);
    ResetSF(sf,0);
}

///////////////////////////////////////////////////////////////////////////////
// close file & remove all dynamic data

void CleanSF ( SuperFile_t * sf )
{
    ASSERT(sf);

    if (sf->wdf)
    {
	TRACE("#S# close WDF %s id=%s=%s\n",
		sf->f.fname, sf->f.id6_src, sf->f.id6_dest );
	ResetWDF(sf->wdf);
	FREE(sf->wdf);
	sf->wdf = 0;
    }

    ResetCISO(&sf->ciso);

    if (sf->wbfs)
    {
	TRACE("#S# close WBFS %s id=%s=%s\n",
		sf->f.fname, sf->f.id6_src, sf->f.id6_dest );
	ResetWBFS(sf->wbfs);
	FREE(sf->wbfs);
	sf->wbfs = 0;
    }

    if (sf->wia)
    {
	TRACE("#S# close WIA %s id=%s=%s\n",
		sf->f.fname, sf->f.id6_src, sf->f.id6_dest );
	ResetWIA(sf->wia);
	FREE(sf->wia);
	sf->wia = 0;
    }

    if (sf->gcz)
    {
	TRACE("#S# close GCZ %s id=%s=%s\n",
		sf->f.fname, sf->f.id6_src, sf->f.id6_dest );
	ResetGCZ(sf->gcz);
	FREE(sf->gcz);
	sf->gcz = 0;
    }

    if (sf->fst)
    {
	TRACE("#S# close FST %s id=%s=%s\n",
		sf->f.fname, sf->f.id6_src, sf->f.id6_dest );
	ResetFST(sf->fst);
	FREE(sf->fst);
	sf->fst = 0;
    }

    ResetMemMap(&sf->modified_list);
}

///////////////////////////////////////////////////////////////////////////////

static enumError CloseHelperSF
(
    SuperFile_t		* sf,		// file to close
    FileAttrib_t	* set_time_ref,	// not NULL: set time
    bool		remove,		// true: remove file
    SuperFile_t		* remove_sf	// not NULL & 'sf' finished without error:
					// close & remove it before renaming 'sf'
)
{
    ASSERT(sf);
    enumError err = ERR_OK;

    if (sf->disc2)
    {
	wd_close_disc(sf->disc2);
	sf->disc2 = 0;
    }

    if (sf->disc1)
    {
	wd_close_disc(sf->disc1);
	sf->disc1 = 0;
    }

    if (remove)
	err = CloseWFile(&sf->f,true);
    else
    {
	if ( sf->f.is_writing && sf->min_file_size )
	{
	    err = SetMinSizeSF(sf,sf->min_file_size);
	    sf->min_file_size = 0;
	}

	if ( !err && sf->f.is_writing )
	{
	    if (sf->wdf)
		err = TermWriteWDF(sf);

	    if (sf->ciso.map)
		err = TermWriteCISO(sf);

	    if (sf->wbfs)
	    {
		err = CloseWDisc(sf->wbfs);
		if (!err)
		    err = sf->wbfs->is_growing
			    ? TruncateWBFS(sf->wbfs)
			    : SyncWBFS(sf->wbfs,false);
	    }

	    if (sf->wia)
		err = TermWriteWIA(sf);

	    if (sf->gcz)
		err = TermWriteGCZ(sf);
	}

	if ( err == ERR_OK && remove_sf )
	{
	    err = FlushSF(sf);
	    RemoveSF(remove_sf);
	}

	if ( err != ERR_OK )
	    CloseWFile(&sf->f,true);
	else
	{
	    err = CloseWFile(&sf->f,false);
	    if (set_time_ref)
		SetWFileTime(&sf->f,set_time_ref);
	}
    }

    if (sf->progress_summary)
    {
	sf->progress_summary = false;
	PrintSummarySF(sf);
    }

    CleanSF(sf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref )
{
    return CloseHelperSF(sf,set_time_ref,false,0);
}

///////////////////////////////////////////////////////////////////////////////

enumError Close2SF
(
    SuperFile_t		* sf,		// file to close
    SuperFile_t		* remove_sf,	// not NULL & 'sf' finished without error:
					// close & remove it before renaming 'sf'
    bool		preserve	// true: force preserve time
)
{
    DASSERT(sf);
    DASSERT(remove_sf);

    sf->src = 0;
    FileAttrib_t set_time_ref;
    memcpy(&set_time_ref,&remove_sf->f.fatt,sizeof(set_time_ref));
    return CloseHelperSF(sf,&set_time_ref,false,remove_sf);
}

///////////////////////////////////////////////////////////////////////////////

enumError ResetSF ( SuperFile_t * sf, FileAttrib_t * set_time_ref )
{
    DASSERT(sf);

    // free dynamic memory
    enumError err = CloseHelperSF(sf,set_time_ref,false,0);
    ResetWFile(&sf->f,false);
    sf->indent = NormalizeIndent(sf->indent);

    // reset data
    SetupIOD(sf,OFT_UNKNOWN,OFT_UNKNOWN);
    ResetCISO(&sf->ciso);
    sf->max_virt_off = 0;
    sf->allow_fst = opt_allow_fst >= OFFON_AUTO;

    // reset progress info
    sf->indent			= 5;
    sf->show_progress		= verbose > 1 || progress;
    sf->show_summary		= verbose > 0 || progress;
    sf->show_msec		= verbose > 2;
    sf->progress_trigger	= 1;
    sf->progress_trigger_init	= 1;
    sf->progress_start_time	= GetTimerMSec();
    sf->progress_last_view_sec	= 0;
    sf->progress_max_wd		= 0;

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError RemoveSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("RemoveSF(%p)\n",sf);

    if (sf->wbfs)
    {
	if ( sf->wbfs->used_discs > 1 )
	{
	    TRACE(" - remove wdisc %s,%s\n",sf->f.id6_src,sf->f.id6_dest);
	    RemoveWDisc(sf->wbfs,sf->f.id6_src,0,false);
	    return ResetSF(sf,0);
	}
    }

    TRACE(" - remove file %s\n",sf->f.fname);
    enumError err = CloseHelperSF(sf,0,true,0);
    ResetSF(sf,0);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

bool IsOpenSF ( const SuperFile_t * sf )
{
    return sf && ( sf->fst || IsOpenF(&sf->f) );
}

///////////////////////////////////////////////////////////////////////////////

bool IsWritableSF ( const SuperFile_t * sf )
{
    return sf && sf->f.is_writing && IsOpenF(&sf->f);
}

///////////////////////////////////////////////////////////////////////////////

SuperFile_t * AllocSF()
{
    SuperFile_t * sf = MALLOC(sizeof(*sf));
    InitializeSF(sf);
    return sf;
}

///////////////////////////////////////////////////////////////////////////////

SuperFile_t * FreeSF ( SuperFile_t * sf )
{
    if (sf)
    {
	CloseSF(sf,0);
	FREE(sf);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

enumOFT SetupIOD ( SuperFile_t * sf, enumOFT force, enumOFT def )
{
    ASSERT(sf);
    sf->iod.oft = CalcOFT(force,sf->f.fname,0,def);
    PRINT("SetupIOD(%p,%u,%u) OFT := %u\n",sf,force,def,sf->iod.oft);

    switch (sf->iod.oft)
    {
	case OFT_PLAIN:
	    sf->iod.read_func		= ReadISO;
	    sf->iod.data_block_func	= DataBlockStandard;	// standard function
	    sf->iod.file_map_func	= FileMapISO;
	    sf->iod.write_func		= WriteISO;
	    sf->iod.write_sparse_func	= WriteSparseISO;
	    sf->iod.write_zero_func	= WriteZeroISO;
	    sf->iod.flush_func		= FlushFile;		// standard function
	    break;

	case OFT_WDF1:
	case OFT_WDF2:
	    sf->iod.read_func		= ReadWDF;
	    sf->iod.data_block_func	= DataBlockWDF;
	    sf->iod.file_map_func	= FileMapWDF;
	    sf->iod.write_func		= WriteWDF;
	    sf->iod.write_sparse_func	= WriteSparseWDF;
	    sf->iod.write_zero_func	= WriteZeroWDF;
	    sf->iod.flush_func		= FlushFile;		// standard function
	    break;

	case OFT_WIA:
	    sf->iod.read_func		= ReadWIA;
	    sf->iod.data_block_func	= DataBlockWIA;
	    sf->iod.file_map_func	= 0;			// not supported
	    sf->iod.write_func		= WriteWIA;
	    sf->iod.write_sparse_func	= WriteSparseWIA;
	    sf->iod.write_zero_func	= WriteZeroWIA;
	    sf->iod.flush_func		= FlushWIA;
	    break;

	case OFT_GCZ:
	    sf->iod.read_func		= ReadGCZ;
	    sf->iod.data_block_func	= DataBlockGCZ;
	    sf->iod.file_map_func	= 0;			// not supported
	    sf->iod.write_func		= WriteGCZ;
	    sf->iod.write_sparse_func	= WriteGCZ;		// no special sparse handling
	    sf->iod.write_zero_func	= WriteZeroGCZ;
	    sf->iod.flush_func		= FlushGCZ;
	    break;

	case OFT_CISO:
	    sf->iod.read_func		= ReadCISO;
	    sf->iod.data_block_func	= DataBlockCISO;
	    sf->iod.file_map_func	= FileMapCISO;
	    sf->iod.write_func		= WriteCISO;
	    sf->iod.write_sparse_func	= WriteSparseCISO;
	    sf->iod.write_zero_func	= WriteZeroCISO;
	    sf->iod.flush_func		= FlushFile;		// standard function
	    break;

	case OFT_WBFS:
	    sf->iod.read_func		= ReadWBFS;
	    sf->iod.data_block_func	= DataBlockWBFS;
	    sf->iod.file_map_func	= FileMapWBFS;
	    sf->iod.write_func		= WriteWBFS;
	    sf->iod.write_sparse_func	= WriteWBFS;		// no sparse support
	    sf->iod.write_zero_func	= WriteZeroWBFS;
	    sf->iod.flush_func		= FlushFile;		// standard function
	    break;

	case OFT_FST:
	    sf->iod.read_func		= ReadFST;
	    sf->iod.data_block_func	= DataBlockStandard;	// standard function
	    sf->iod.file_map_func	= 0;			// not supported
	    sf->iod.write_func		= WriteSwitchSF;	// not supported
	    sf->iod.write_sparse_func	= WriteSparseSwitchSF;	// not supported
	    sf->iod.write_zero_func	= WriteZeroSwitchSF;	// not supported
	    sf->iod.flush_func		= 0;			// do nothing
	    break;

	default:
	    sf->iod.read_func		= ReadSwitchSF;
	    sf->iod.data_block_func	= DataBlockStandard;	// standard function
	    sf->iod.file_map_func	= 0;			// not supported
	    sf->iod.write_func		= WriteSwitchSF;
	    sf->iod.write_sparse_func	= WriteSparseSwitchSF;
	    sf->iod.write_zero_func	= WriteZeroSwitchSF;
	    sf->iod.flush_func		= 0;			// do nothing
	    break;
    }
    sf->std_read_func = sf->iod.read_func;

    return sf->iod.oft;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////	                       Setup SF                ////////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupReadSF(%p) fd=%d is-r=%d is-w=%d\n",
	sf, sf->f.fd, sf->f.is_reading, sf->f.is_writing );

    if ( !sf || !sf->f.is_reading )
	return ERROR0(ERR_INTERNAL,0);

    SetupIOD(sf,OFT_PLAIN,OFT_PLAIN);
    if ( sf->f.ftype == FT_UNKNOWN )
	AnalyzeFT(&sf->f);
    DASSERT( Count1Bits64(sf->f.ftype&FT__ID_MASK) <= 1  ); // [[2do]] [[ft-id]]

    if ( sf->allow_fst && sf->f.ftype & FT_ID_FST )
	return SetupReadFST(sf);

    if ( !IsOpenSF(sf) )
    {
	if (!sf->f.disable_errors)
	    ERROR0( ERR_CANT_OPEN, "Can't open file: %s\n", sf->f.fname );
	return ERR_CANT_OPEN;
    }

    if ( sf->f.ftype & FT_M_NKIT )
    {
	if ( !sf->f.disable_errors && !sf->f.disable_nkit_errors )
	    ERROR0( ERR_WRONG_FILE_TYPE, "NKIT not supported: %s\n", sf->f.fname );
	if (!sf->f.id6_dest[0])
	    memcpy(sf->f.id6_dest,sf->f.id6_src,sizeof(sf->f.id6_dest));
	return ERR_WRONG_FILE_TYPE;
    }

    if ( sf->f.ftype & FT_M_WDF )
	return SetupReadWDF(sf);

    if ( sf->f.ftype & FT_A_WIA )
	return SetupReadWIA(sf);

    if ( sf->f.ftype & FT_A_CISO )
	return SetupReadCISO(sf);

    if ( sf->f.ftype & (FT_A_GCZ|FT_A_NKIT_GCZ) )
	return SetupReadGCZ(sf);

    if ( sf->f.ftype & FT_ID_WBFS && ( sf->f.slot >= 0 || sf->f.id6_src[0] ) )
	return SetupReadWBFS(sf);

    sf->file_size = sf->f.st.st_size;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupReadISO ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupReadISO(%p)\n",sf);

    enumError err = SetupReadSF(sf);
    if ( err || sf->f.ftype & FT_A_ISO )
	return err;

    if (!sf->f.disable_errors)
	PrintErrorFT(&sf->f,FT_A_ISO);
    return ERR_WRONG_FILE_TYPE;
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupReadWBFS ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupReadWBFS(%p) id=%s,%s, slot=%d\n",
		sf, sf->f.id6_src, sf->f.id6_dest, sf->f.slot );

    if ( !sf || !sf->f.is_reading || sf->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    WBFS_t * wbfs = MALLOC(sizeof(*wbfs));
    InitializeWBFS(wbfs);
    enumError err = SetupWBFS(wbfs,sf,true,0,false);
    if (err)
	goto abort;

    err = sf->f.slot >= 0
		? OpenWDiscSlot(wbfs,sf->f.slot,false)
		: OpenWDiscID6(wbfs,sf->f.id6_src);
    if (err)
	goto abort;

    //--- calc file size & fragments

    ASSERT(wbfs->disc);
    ASSERT(wbfs->disc->header);
    ASSERT(wbfs->disc->header->wlba_table);

    wbfs_t *w = wbfs->wbfs;
    ASSERT(w);


    //--- calc disc size & fragments

    uint disc_blocks;
    sf->wbfs_fragments = wbfs_get_disc_fragments(wbfs->disc,&disc_blocks);
    sf->file_size = (off_t)disc_blocks * w->wbfs_sec_sz;
    CopyPatchWbfsId(sf->wbfs_id6,wbfs->disc->header->dhead);
    const off_t single_size = WII_SECTORS_SINGLE_LAYER * (off_t)WII_SECTOR_SIZE;
    PRINT("N-FRAG: %u, size = %llx,%llx\n",
	sf->wbfs_fragments, sf->file_size, (u64)single_size );
    if ( sf->file_size < single_size )
	sf->file_size = single_size;


    //---- store data

    TRACE("WBFS %s/%s,%s opened.\n",sf->f.fname,sf->f.id6_src,sf->f.id6_dest);
    sf->wbfs = wbfs;

    wd_header_t * dh = (wd_header_t*)wbfs->disc->header;
    snprintf(iobuf,sizeof(iobuf),"%s [%s]",
		GetTitle(sf->f.id6_dest, (ccp)dh->disc_title), sf->f.id6_dest );
    FreeString(sf->f.outname);
    sf->f.outname = STRDUP(iobuf);
    SetupIOD(sf,OFT_WBFS,OFT_WBFS);
    return ERR_OK;

 abort:
    ResetWBFS(wbfs);
    FREE(wbfs);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenSF
(
    SuperFile_t	* sf,
    ccp		fname,
    bool	allow_non_iso,
    bool	open_modify
)
{
    ASSERT(sf);
    CloseSF(sf,0);
    PRINT("#S# OpenSF(%p,%s,non-iso=%d,rw=%d)\n",sf,fname,allow_non_iso,open_modify);

    const bool disable_errors = sf->f.disable_errors;
    sf->f.disable_errors = true;
    if (open_modify)
	OpenWFileModify(&sf->f,fname,IOM_IS_IMAGE);
    else
	OpenWFile(&sf->f,fname,IOM_IS_IMAGE);
    sf->f.disable_errors = disable_errors;

    DefineCachedAreaISO(&sf->f,true);

    return allow_non_iso
		? SetupReadSF(sf)
		: SetupReadISO(sf);
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateSF
(
    SuperFile_t		* sf,		// file to setup
    ccp			fname,		// NULL or filename
    enumOFT		oft,		// output file mode
    enumIOMode		iomode,		// io mode
    int			overwrite	// overwrite mode
)
{
    ASSERT(sf);
    CloseSF(sf,0);
    TRACE("#S# CreateSF(%p,%s,%x,%x,%x)\n",sf,fname,oft,iomode,overwrite);

    const enumError err = CreateWFile(&sf->f,fname,iomode,overwrite);
    return err ? err : SetupWriteSF(sf,oft);
}

///////////////////////////////////////////////////////////////////////////////

enumError PreallocateSF
(
    SuperFile_t		* sf,		// file to operate
    u64			base_off,	// offset of pre block
    u64			base_size,	// size of pre block
					// base_off+base_size == address fpr block #0
    u32			block_size,	// size in wii sectors of 1 container block
    u32			min_hole_size	// the minimal allowed hole size in 32K sectors
)
{
    // [[2do]] is 'min_hole_size' obsolete?

    DASSERT(sf);
    if ( !sf->src || prealloc_mode == PREALLOC_OFF )
	return ERR_OK;

    wd_disc_t * disc = OpenDiscSF(sf->src,false,false);
    if (!disc)
	return ERR_OK;

    PRINT("PreallocateSF(%p,%llx,%llx,%x,%x)\n",
		sf, base_off, base_size, block_size, min_hole_size );

    u8 utab[WII_MAX_SECTORS];
    const u8 * uptr = utab;
    const u8 * uend = utab + wd_pack_disc_usage_table(utab,disc,block_size,0);
    const u64 off0 = base_off + base_size;

    while ( uptr < uend && !*uptr )
	uptr++;

    while ( uptr < uend )
    {
	const u32 beg = uptr - utab;
	u32 end;
	for(;;)
	{
	    while ( uptr < uend && *uptr )
		uptr++;
	    end =  uptr - utab;
	    while ( uptr < uend && !*uptr )
		uptr++;
	    if ( uptr == uend || uptr-utab-end >= min_hole_size )
		break;
	}

	if ( beg < end )
	{
	    u64 off  = off0 + beg * (u64)WII_SECTOR_SIZE;
	    u64 size = ( end - beg )  * (u64)WII_SECTOR_SIZE;
	    if ( base_size )
	    {
		if (beg)
		    PreallocateF(&sf->f,base_off,base_size);
		else
		{
		    off -= base_size;
		    size += base_size;
		}
		base_size = 0;
	    }
	    PRINT("PREALLOC BLOCK %05x..%05x -> %9llx..%9llx [%llx]\n",
			beg, end, off, off+size, size );

	    PreallocateF(&sf->f,off,size);
	}
    }
    return ERR_OK;

}

///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteSF
(
    SuperFile_t		* sf,		// file to setup
    enumOFT		oft		// force OFT mode of 'sf' 
)
{
    ASSERT(sf);
    TRACE("SetupWriteSF(%p,%x)\n",sf,oft);

    if ( !sf || sf->f.is_reading || !sf->f.is_writing )
	return ERROR0(ERR_INTERNAL,0);

    SetupIOD(sf,oft,OFT__DEFAULT);
    SetupAutoSplit(&sf->f,sf->iod.oft);
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:
	    PreallocateSF(sf,0,0,WII_MAX_SECTORS,1);
	    return ERR_OK;

	case OFT_WDF1:
	case OFT_WDF2:
	    return SetupWriteWDF(sf);

	case OFT_WIA:
	    return SetupWriteWIA(sf,0);

	case OFT_GCZ:
	    return SetupWriteGCZ(sf,0);

	case OFT_CISO:
	    return SetupWriteCISO(sf);

	case OFT_WBFS:
	    return SetupWriteWBFS(sf);

	default:
	    return ERROR0(ERR_INTERNAL,
			"Unknown output file format: %d\n",sf->iod.oft);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteWBFS ( SuperFile_t * sf )
{
    ASSERT(sf);
    TRACE("SetupWriteWBFS(%p)\n",sf);

    if ( !sf || sf->f.is_reading || !sf->f.is_writing || sf->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    WBFS_t * wbfs =  MALLOC(sizeof(*wbfs));
    InitializeWBFS(wbfs);

    const u64 size = ( WII_MAX_SECTORS *(u64)WII_SECTOR_SIZE / WBFS_MIN_SECTOR_SIZE + 2 )
			* WBFS_MIN_SECTOR_SIZE;
    PRINT("WBFS size = %llx = %llu\n",size,size);
    enumError err = CreateGrowingWBFS(wbfs,sf,size,sf->f.sector_size);
    sf->wbfs = wbfs;
    SetupIOD(sf,OFT_WBFS,OFT_WBFS);

    if (!err)
    {
#if 0 // [[id+]]
	ccp force_id6 = 0;
	id6_t patched_id6;
	if (sf->src)
	{
	    CopyPatchWbfsId(patched_id6,sf->src->f.id6_dest);
	    force_id6 = patched_id6;
	}

	wbfs_disc_t * disc = wbfs_create_disc(sf->wbfs->wbfs,0,force_id6);
#else
	wbfs_disc_t * disc = wbfs_create_disc(sf->wbfs->wbfs,0,0);
#endif
	if (disc)
	    wbfs->disc = disc;
	else
	    err = ERR_CANT_CREATE;
    }

    return err;
}

///////////////////////////////////////////////////////////////////////////////

static enumError ReadDiscWrapper
(
    SuperFile_t	* sf,
    off_t	off,
    void	* buf,
    size_t	count
)
{
    DASSERT(sf);
    DASSERT(sf->disc1);
    return wd_read_and_patch(sf->disc1,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

int IsFileSelected ( wd_iterator_t *it )
{
    DASSERT(it);
    DASSERT(it->param);

    FilePattern_t * pat = it->param;
    return MatchFilePattern(pat,it->fst_name,'/');
};

///////////////////////////////////////////////////////////////////////////////

void CloseDiscSF
(
    SuperFile_t		* sf		// valid file pointer
)
{
    DASSERT(sf);
    wd_close_disc(sf->disc2);
    wd_close_disc(sf->disc1);
    sf->disc1 = sf->disc2 = 0;
    sf->discs_loaded = false;
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_t * OpenDiscSF
(
    SuperFile_t		* sf,		// valid file pointer
    bool		load_part_data,	// true: load partition data
    bool		print_err	// true: print error message if open fails
)
{
    DASSERT(sf);
    if (sf->discs_loaded)
	return sf->disc2;

    DASSERT(!sf->disc1);
    DASSERT(!sf->disc2);

    if ( !print_err && sf->iod.oft != OFT_FST && S_ISDIR(sf->f.st.st_mode) )
	return 0;
    sf->discs_loaded = true;

    //****************************************************
    // The idea
    //	1.) Open disc1 and enable patching if necessary
    //  2.) Open disc2 and route reading through patched
    //      disc1 if patching is enabled; dup disc1 else
    //****************************************************

    //----- open unpatched disc

    wd_disc_t * disc = 0;

    const u64 file_size = sf->f.seek_allowed ? sf->file_size : 0;

    if ( IsOpenSF(sf) && sf->f.is_reading )
	disc = wd_open_disc(WrapperReadDirectSF,sf,file_size,sf->f.fname,opt_force,0);

    if (!disc)
    {
	if (print_err)
	    ERROR0(ERR_WDISC_NOT_FOUND,"Can't open Wii disc: %s\n",sf->f.fname);
	return 0;
    }
    disc->image_type = oft_info[sf->iod.oft].name;
    disc->image_ext  = oft_info[sf->iod.oft].ext1 + 1;
    if (!disc->image_ext)
	disc->image_ext = oft_info[OFT__DEFAULT].ext1 + 1;
    sf->disc1 = disc;


    if (load_part_data)
	wd_load_all_part(disc,false,false,false);

    if ( disc->disc_type == WD_DT_GAMECUBE )
	sf->f.read_behind_eof = 2;

    if ( opt_hook < 0 )
	return sf->disc2 = wd_dup_disc(disc);


    //----- select partitions

    wd_select(disc,&part_selector);
    wd_part_t *main_part = disc->main_part;


    //----- check for patching

    // activate reloc?
    int reloc = opt_hook > 0 || sf->disc1->patch_ptab_recommended; 

    const wd_modify_t modify = opt_modify & WD_MODIFY__AUTO
				? WD_MODIFY__ALL : opt_modify;

    enumEncoding enc = SetEncoding(encoding,0,0);
    const bool encrypt = !( enc & ENCODE_DECRYPT );

    if ( sf->iod.oft == OFT_FST )
    {	
	DASSERT(sf->fst);
	enc = sf->fst->encoding;
    }
    else if ( disable_patch_on_load <= 0 )
    {

	reloc |= patch_main_and_rel(disc,0);

	if (main_part)
	{

	    reloc |= wd_patch_part_id(main_part,modify,
				modify_disc_id, modify_boot_id,
				modify_ticket_id, modify_tmd_id );
	    if (modify_name)
		reloc |= wd_patch_part_name(main_part,modify_name,modify);
	    if (opt_ios_valid)
		reloc |= wd_patch_part_system(main_part,opt_ios);
	}
	else if (modify_disc_id)
	    reloc |= wd_patch_disc_header(disc,modify_disc_id,modify_name);

	if ( opt_region < REGION__AUTO )
	    reloc |= wd_patch_region(disc,opt_region);
	else if ( modify_id
		    && strlen(modify_id) > 3
		    && modify_id[3] != '.'
		    && modify_id[3] != disc->dhead.id6.region_code )
	{
	    const enumRegion region = GetRegionInfo(modify_id[3])->reg;
	    reloc |= wd_patch_region(disc,region);
	}

	if ( !reloc
	    && disc->disc_type != WD_DT_GAMECUBE
	    && enc & (ENCODE_M_CRYPT|ENCODE_F_ENCRYPT) )
	{
	    int ip;
	    for ( ip = 0; ip < disc->n_part; ip++ )
		if ( wd_get_part_by_index(disc,ip,0)->is_encrypted != encrypt )
		{
		    reloc = true;
		    break;
		}
	}

	if ( enc & ENCODE_SIGN )
	{
	    reloc = true;
	    int ip;
	    for ( ip = 0; ip < disc->n_part; ip++ )
	    {
		wd_part_t * part = wd_get_part_by_index(disc,ip,0);
		if ( part && part->is_valid && part->is_enabled )
		{
		    wd_insert_patch_ticket(part);
		    wd_insert_patch_tmd(part);
		}
	    }
	}
    }


    //----- common key

    if ( (unsigned)opt_common_key < WD_CKEY__N )
    {
	int ip;
	for ( ip = 0; ip < disc->n_part; ip++ )
	{
	    wd_part_t * part = wd_get_part_by_index(disc,ip,0);
	    if ( part && part->is_valid && part->is_enabled )
		reloc |= wd_patch_common_key(part,opt_common_key);
	}
    }


    //----- check file pattern

    bool files_dirty = false;

    FilePattern_t * pat = file_pattern + PAT_RM_FILES;
    if (SetupFilePattern(pat)
	&& pat->rules.used
	&& wd_remove_disc_files(disc,IsFileSelected,pat,false) )
    {
	files_dirty = true;
	reloc = 1;
    }

    pat = file_pattern + PAT_ZERO_FILES;
    if (SetupFilePattern(pat)
	&& pat->rules.used
	&& wd_zero_disc_files(disc,IsFileSelected,pat,false) )
    {
	files_dirty = true;
	reloc = 1;
    }

    if (files_dirty)
	wd_select_disc_files(disc,0,0);


    //----- reloc?

    const wd_trim_mode_t reloc_trim = wd_get_relococation_trim(opt_trim,0,0);

    if ( reloc || reloc_trim )
    {
	PRINT("SETUP RELOC, enc=%04x->%04x->%d, trim=%x\n",
		encoding, enc, !(enc&ENCODE_DECRYPT), reloc_trim );

	wd_calc_relocation(disc,encrypt,reloc_trim,opt_align_part,0,true);

	if ( logging > 0 )
	    wd_print_disc_patch(stdout,1,sf->disc1,true,logging>1);

	sf->iod.read_func = ReadDiscWrapper;
	sf->disc2 = wd_open_disc(WrapperReadSF,sf,file_size,sf->f.fname,opt_force,0);
	sf->disc2->image_type = oft_info[sf->iod.oft].name;
	sf->disc2->image_ext  = oft_info[sf->iod.oft].ext1 + 1;
	if (load_part_data)
	    wd_load_all_part(sf->disc2,false,false,false);
    }

    if (sf->disc2)
    {
	if (!sf->disc2->image_ext)
	    sf->disc2->image_ext = oft_info[OFT__DEFAULT].ext1 + 1;
	memcpy(sf->f.id6_dest,&sf->disc2->dhead,6);
    }
    else
	sf->disc2 = wd_dup_disc(sf->disc1);

    pat = file_pattern + PAT_IGNORE_FILES;
    SetupFilePattern(pat);
    if (pat->rules.used)
    {
	DefineNegatePattern(pat,true);
	wd_select_disc_files(sf->disc2,IsFileSelected,pat);
    }

    return sf->disc2;
}

///////////////////////////////////////////////////////////////////////////////

void SubstFileNameSF
(
    SuperFile_t	* fo,		// output file
    SuperFile_t	* fi,		// input file
    ccp		dest_arg	// arg to parse for escapes
)
{
    ASSERT(fo);
    ASSERT(fi);

    char fname[PATH_MAX*2];
    const int conv_count
	= SubstFileNameBuf(fname,sizeof(fname),fi,dest_arg,fo->f.fname,fo->iod.oft);
    if ( conv_count > 0 )
	fo->f.create_directory = true;
    SetWFileName(&fo->f,fname,true);
}

///////////////////////////////////////////////////////////////////////////////

int SubstFileNameBuf
(
    char	* fname_buf,	// output buf
    size_t	fname_size,	// size of output buf
    SuperFile_t	* fi,		// input file -> id6, fname
    ccp		fname,		// pure filename. if NULL: extract from 'fi'
    ccp		dest_arg,	// arg to parse for escapes
    enumOFT	oft		// output file type
)
{
    DASSERT(fi);

    ccp disc_name = 0;
    char buf[HD_SECTOR_SIZE];
    if ( fi->f.id6_dest[0] && !(fi->f.ftype & FT_M_NKIT) )
    {
	OpenDiscSF(fi,false,false);
	if (fi->disc2)
	    disc_name = (ccp)fi->disc2->dhead.disc_title;
	else
	{
	    const bool disable_errors = fi->f.disable_errors;
	    fi->f.disable_errors = true;
	    if (!ReadSF(fi,0,&buf,sizeof(buf)))
		disc_name = (ccp)((wd_header_t*)buf)->disc_title;
	    fi->f.disable_errors = disable_errors;
	}
    }

    return SubstFileName( fname_buf, fname_size,
			  fi->f.id6_dest, disc_name,
			  fi->f.path ? fi->f.path : fi->f.fname,
			  fname, dest_arg, oft );
}

///////////////////////////////////////////////////////////////////////////////

int SubstFileName
(
    char	* fname_buf,	// output buf
    size_t	fname_size,	// size of output buf
    ccp		id6,		// id6
    ccp		disc_name,	// name of disc
    ccp		src_file,	// full path to source file
    ccp		fname,		// pure filename, no path. If NULL: extract from 'src_file'
    ccp		dest_arg,	// arg to parse for escapes
    enumOFT	oft		// output file type
)
{
    DASSERT(fname_buf);
    DASSERT(fname_size>1);

    if ( !src_file || !*src_file )
	src_file = "./";

    if (!fname)
    {
	fname = src_file;
	ccp temp = strrchr(src_file,'/');
	if (temp)
	    fname = temp+1;
    }

    char pure_name[PATH_MAX];
    ccp ext = strrchr(fname,'.');
    if (ext)
    {
	int len = ext - fname + 1;
	if ( len > sizeof(pure_name) )
	     len = sizeof(pure_name);
	StringCopyS(pure_name,len,fname);
    }
    else
	StringCopyS(pure_name,sizeof(pure_name),fname);

    char src_path[PATH_MAX];
    StringCopyS(src_path,sizeof(src_path),src_file);
    char * temp = strrchr(src_path,'/');
    if (temp)
	*temp = 0;
    else
	*src_path = 0;

    if ( !disc_name || !*disc_name )
	disc_name = id6 && *id6 ? id6 : pure_name;
    ccp title = GetTitle(id6,disc_name);

    char x_buf[1000];
    snprintf(x_buf,sizeof(x_buf),"%s [%s]%s",
		title, id6, oft_info[oft].ext1 );

    char y_buf[1000];
    snprintf(y_buf,sizeof(y_buf),"%s [%s]",
		title, id6 );

    char plus_buf[20];
    ccp plus_name;
    if ( oft == OFT_WBFS )
    {
	snprintf(plus_buf,sizeof(plus_buf),"%s%s",
	    id6, oft_info[oft].ext1 );
	plus_name = plus_buf;
    }
    else
	plus_name = x_buf;

    SubstString_t subst_tab[] =
    {
	{ 'i', 'I', 0, id6 },
	{ 'n', 'N', 0, disc_name },
	{ 't', 'T', 0, title },
	{ 'e', 'E', 0, oft_info[oft].ext1+1 },
	{ 'p', 'P', 1, src_path },
	{ 'f', 'F', 1, fname },
	{ 'g', 'G', 1, pure_name },
	{ 'h', 'H', 1, ext },
	{ 'x', 'X', 0, x_buf },
	{ 'y', 'Y', 0, y_buf },
	{ '+', '+', 0, plus_name },
	{0,0,0,0}
    };

    int conv_count;
    SubstString(fname_buf,fname_size,subst_tab,dest_arg,&conv_count);
    return conv_count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sparse helper			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError PatchSF
(
    SuperFile_t		* sf,		// valid file pointer
    enumError		err_on_patch	// error message if patched
)
{
    DASSERT(sf);


    OpenDiscSF(sf,true,true);
    wd_disc_t * disc = sf->disc1;

    enumError err = ERR_OK;
    if ( disc && disc->reloc )
    {
	err = err_on_patch;

	PRINT("EDIT PHASE I\n");
	const wd_reloc_t * reloc = disc->reloc;
	u32 idx;
	for ( idx = 0; idx < WII_MAX_SECTORS && !err; idx++, reloc++ )
	    if ( *reloc & (WD_RELOC_F_PATCH|WD_RELOC_F_HASH)
		&& !( *reloc & WD_RELOC_F_LAST ) )
	    {
		TRACE(" - WRITE SECTOR %x, off %llx\n",idx,idx*(u64)WII_SECTOR_SIZE);
		err = CopyRawData(sf,sf,idx*(u64)WII_SECTOR_SIZE,WII_SECTOR_SIZE);
	    }

	PRINT("EDIT PHASE II\n");
	reloc = disc->reloc;
	for ( idx = 0; idx < WII_MAX_SECTORS && !err; idx++, reloc++ )
	    if ( *reloc & WD_RELOC_F_LAST )
	    {
		TRACE(" - WRITE SECTOR %x, off %llx\n",idx,idx*(u64)WII_SECTOR_SIZE);
		err = CopyRawData(sf,sf,idx*(u64)WII_SECTOR_SIZE,WII_SECTOR_SIZE);
	    }
    }

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sparse helper			///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError SparseHelperByte
	( SuperFile_t * sf, off_t off, const void * buf, size_t count,
	  WriteFunc write_func, size_t min_chunk_size )
{
    ASSERT(sf);
    ASSERT(write_func);
    ASSERT( min_chunk_size >= sizeof(WDF_Hole_t) );

    TRACE(TRACE_RDWR_FORMAT, "#SH# SparseHelperB()",
	GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ccp ptr = buf;
    ccp end = ptr + count;

    // skip start hole
    while ( ptr < end && !*ptr )
	ptr++;

    while ( ptr < end )
    {
	ccp data_beg = ptr;
	ccp data_end = data_beg;

	while ( ptr < end )
	{
	    // find end of data
	    while ( ptr < end && *ptr )
		ptr++;
	    data_end = ptr;

	    // skip holes
	    while ( ptr < end && !*ptr )
		ptr++;

	    // accept only holes >= min_chunk_size
	    if ( (ccp)ptr - (ccp)data_end >= min_chunk_size )
		break;
	}

	const size_t wlen = (ccp)data_end - (ccp)data_beg;
	if (wlen)
	{
	    const off_t woff = off + ( (ccp)data_beg - (ccp)buf );
	    const enumError err = write_func(sf,woff,data_beg,wlen);
	    if (err)
		return err;
	}
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SparseHelper
	( SuperFile_t * sf, off_t off, const void * buf, size_t count,
	  WriteFunc write_func, size_t min_chunk_size )
{
    ASSERT(sf);
    ASSERT(write_func);

    TRACE(TRACE_RDWR_FORMAT, "#SH# SparseHelper()",
	GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );
    TRACE(" -> write_func = %p, min_chunk_size = %zu\n",write_func, min_chunk_size );

    // adjust the file size
    const off_t off_end = off + count;
    if ( sf->file_size < off_end )
	 sf->file_size = off_end;


    //----- disable sparse check for already existing file areas

    if ( off < sf->max_virt_off )
    {
	const off_t max_overlap = sf->max_virt_off - off;
	const size_t overlap = count < max_overlap ? count : (size_t) max_overlap;
	const enumError err = write_func(sf,off,buf,overlap);
	count -= overlap;
	if ( err || !count )
	    return err;

	off += overlap;
	buf = (char*)buf + overlap;
    }


    //----- check minimal size

    if ( min_chunk_size < sizeof(WDF_Hole_t) )
	 min_chunk_size = sizeof(WDF_Hole_t);
    if ( count < 30 || count < min_chunk_size )
	return SparseHelperByte( sf, off, buf, count, write_func, min_chunk_size );


    //----- check if buf is well aligend

    DASSERT( Count1Bits32(sizeof(WDF_Hole_t)) == 1 );
    const size_t align_mask = sizeof(WDF_Hole_t) - 1;

    ccp start = buf;
#if !defined(__i386__) && !defined(__x86_64__)
    // avoid reading 'WDF_Hole_t' values from non aligned addresses 
    // not needed for i386 & x86_64 because they can do such readings
    const size_t start_align = (size_t)start & align_mask;
    if ( start_align )
    {
	const size_t wr_size =  sizeof(WDF_Hole_t) - start_align;
	const enumError err
	    = SparseHelperByte( sf, off, start, wr_size, write_func, min_chunk_size );
	if (err)
	    return err;
	start += wr_size;
	off   += wr_size;
	count -= wr_size;
    }
#endif


    //----- check aligned data

    WDF_Hole_t * ptr = (WDF_Hole_t*) start;
    WDF_Hole_t * end = (WDF_Hole_t*)( start + ( count & ~align_mask ) );

    // skip start hole
    while ( ptr < end && !*ptr )
	ptr++;

    while ( ptr < end )
    {
	WDF_Hole_t * data_beg = ptr;
	WDF_Hole_t * data_end = data_beg;

	while ( ptr < end )
	{
	    // find end of data
	    while ( ptr < end && *ptr )
		ptr++;
	    data_end = ptr;

	    // skip holes
	    while ( ptr < end && !*ptr )
		ptr++;

	    // accept only holes >= min_chunk_size
	    if ( (ccp)ptr - (ccp)data_end >= min_chunk_size )
		break;
	}

	const size_t wlen = (ccp)data_end - (ccp)data_beg;
	if (wlen)
	{
	    const off_t woff = off + ( (ccp)data_beg - start );
	    const enumError err = write_func(sf,woff,data_beg,wlen);
	    if (err)
		return err;
	}
    }


    //----- check remaining bytes and return

    const size_t remaining_len = count & align_mask;
    if ( remaining_len )
    {
	ASSERT( remaining_len < sizeof(WDF_Hole_t) );
	const off_t woff = off + ( (ccp)end - start );
	const enumError err
	    = SparseHelperByte ( sf, woff, end, remaining_len,
				write_func, min_chunk_size );
	if (err)
	    return err;
    }


    //----- all done

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		 standard read and write wrappers	///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadZero
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    // dummy read function: fill with zero
    memset(buf,0,count);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadSF
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.read_func);
    return sf->iod.read_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadDirectSF
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->std_read_func);
    return sf->std_read_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

off_t DataBlockSF
(
    SuperFile_t		* sf,		// valid file
    off_t		off,		// file offset
    size_t		align,		// if >1: round results to multiple of 'align'
    off_t		* block_size	// not null: return block size
)
{
    ASSERT(sf);
    ASSERT(sf->iod.data_block_func);
    off = sf->iod.data_block_func(sf,off,align,block_size);

    if ( align > 1 )
    {
	if (block_size)
	{
	    off_t end = (( off + *block_size - 1 ) / align + 1 ) * align;
	    off = ( off / align ) * align;
	    *block_size = end - off;
	}
	else
	    off = ( off / align ) * align;
    }
    return off;
}

///////////////////////////////////////////////////////////////////////////////

off_t UnionDataBlockSF
(
    SuperFile_t		* sf1,		// first file
    SuperFile_t		* sf2,		// second file
    off_t		off,		// file offset
    size_t		align,		// if >1: round results to multiple of 'align'
    off_t		* block_size	// not null: return block size
)
{
    ASSERT(sf1);
    ASSERT(sf2);

    off_t size1, size2;
    const off_t off1 = DataBlockSF(sf1,off,align,&size1);
    const off_t off2 = DataBlockSF(sf2,off,align,&size2);

    if ( off1 + size1 <= off2 )
    {
	// blocks do not overlay
	if (block_size)
	    *block_size = size1;
	return off1; 
    }

    if ( off2 + size2 <= off1 )
    {
	// blocks do not overlay
	if (block_size)
	    *block_size = size2;
	return off2; 
    }

    off = off1 < off2 ? off1 : off2;

    if (block_size)
    {
	const off_t end1 = off1 + size1;
	const off_t end2 = off2 + size2;
	*block_size = ( end1 > end2 ? end1 : end2 ) - off;
    }

    return off;
}

///////////////////////////////////////////////////////////////////////////////

uint GetFileMapSF
(
    SuperFile_t		* sf,		// valid super files
    FileMap_t		* fm,		// file map
    bool		init_fm		// true: initialize 'fm', false: reset 'fm'
)
{
    DASSERT(sf);
    DASSERT(fm);
    if (init_fm)
	InitializeFileMap(fm);
    else
	ResetFileMap(fm);

    if (sf->iod.file_map_func)
	sf->iod.file_map_func(sf,fm);

    return fm->used;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.write_func);
    return sf->iod.write_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.write_sparse_func);
    return sf->iod.write_sparse_func(sf,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroSF ( SuperFile_t * sf, off_t off, size_t count )
{
    ASSERT(sf);
    ASSERT(sf->iod.write_zero_func);
    return sf->iod.write_zero_func(sf,off,count);
}

///////////////////////////////////////////////////////////////////////////////

enumError FlushSF ( SuperFile_t * sf )
{
    ASSERT(sf);
    return sf->iod.flush_func ? sf->iod.flush_func(sf) : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadSwitchSF
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_WDF1:
	case OFT_WDF2:	return ReadWDF(sf,off,buf,count);
	case OFT_WIA:	return ReadWIA(sf,off,buf,count);
	case OFT_GCZ:	return ReadGCZ(sf,off,buf,count);
	case OFT_CISO:	return ReadCISO(sf,off,buf,count);
	case OFT_WBFS:	return ReadWBFS(sf,off,buf,count);
	case OFT_FST:	return ReadFST(sf,off,buf,count);
	default:	return ReadISO(sf,off,buf,count);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSwitchSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:	return WriteISO(sf,off,buf,count);
	case OFT_WDF1:
	case OFT_WDF2:	return WriteWDF(sf,off,buf,count);
	case OFT_WIA:	return WriteWIA(sf,off,buf,count);
	case OFT_GCZ:	return WriteGCZ(sf,off,buf,count);
	case OFT_CISO:	return WriteCISO(sf,off,buf,count);
	case OFT_WBFS:	return WriteWBFS(sf,off,buf,count);
	default:	return ERROR0(ERR_INTERNAL,0);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseSwitchSF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:	return WriteSparseISO(sf,off,buf,count);
	case OFT_WDF1:
	case OFT_WDF2:	return WriteSparseWDF(sf,off,buf,count);
	case OFT_WIA:	return WriteSparseWIA(sf,off,buf,count);
	case OFT_GCZ:	return WriteGCZ(sf,off,buf,count);
	case OFT_CISO:	return WriteSparseCISO(sf,off,buf,count);
	case OFT_WBFS:	return WriteWBFS(sf,off,buf,count); // no sparse support
	default:	return ERROR0(ERR_INTERNAL,0);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroSwitchSF ( SuperFile_t * sf, off_t off, size_t count )
{
    ASSERT(sf);
    DASSERT(0); // should never called
    switch(sf->iod.oft)
    {
	case OFT_PLAIN:	return WriteZeroISO(sf,off,count);
	case OFT_WDF1:
	case OFT_WDF2:	return WriteZeroWDF(sf,off,count);
	case OFT_WIA:	return WriteZeroWIA(sf,off,count);
	case OFT_GCZ:	return WriteZeroGCZ(sf,off,count);
	case OFT_CISO:	return WriteZeroCISO(sf,off,count);
	case OFT_WBFS:	return WriteZeroWBFS(sf,off,count);
	default:	return ERROR0(ERR_INTERNAL,0);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadISO
	( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    ASSERT(sf);
    const enumError err = ReadAtF(&sf->f,off,buf,count);

    off += count;
    if ( sf->f.read_behind_eof && off > sf->f.st.st_size && sf->f.st.st_size )
	off = sf->f.st.st_size;

    DASSERT_MSG( err || sf->f.cur_off == (off_t)-1 || sf->f.cur_off == off,
		"%llx : %llx\n",(u64)sf->f.cur_off,(u64)off);

    if ( sf->max_virt_off < off )
	 sf->max_virt_off = off;
    if ( sf->file_size < off )
	 sf->file_size = off;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteISO
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    ASSERT(sf);
    const enumError err = WriteAtF(&sf->f,off,buf,count);

    off += count;
    DASSERT_MSG( err || sf->f.cur_off == off,
		"%llx : %llx\n",(u64)sf->f.cur_off,(u64)off);
    if ( sf->max_virt_off < off )
	 sf->max_virt_off = off;
    if ( sf->file_size < off )
	 sf->file_size = off;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseISO
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    return SparseHelper(sf,off,buf,count,WriteISO,0);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroISO ( SuperFile_t * sf, off_t off, size_t size )
{
    // [[2do]] [zero] optimization

    while ( size > 0 )
    {
	const size_t size1 = size < sizeof(zerobuf) ? size : sizeof(zerobuf);
	const enumError err = WriteISO(sf,off,zerobuf,size1);
	if (err)
	    return err;
	off  += size1;
	size -= size1;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

off_t DataBlockStandard
    ( SuperFile_t * sf, off_t off, size_t hint_align, off_t * block_size )
{
    if ( block_size )
	*block_size = off < sf->file_size ? sf->file_size - off : GiB;
    return off;
}

///////////////////////////////////////////////////////////////////////////////

void FileMapISO ( SuperFile_t * sf, FileMap_t *fm )
{
    DASSERT(sf);
    DASSERT(fm);
    DASSERT(!fm->used);

    AppendFileMap(fm,0,0,sf->file_size);
}

///////////////////////////////////////////////////////////////////////////////

enumError FlushFile ( SuperFile_t * sf )
{
    DASSERT(sf);
    if ( sf->f.is_writing && sf->f.fp )
	fflush(sf->f.fp);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    set file size		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetSizeSF ( SuperFile_t * sf, off_t off )
{
    DASSERT(sf);
    TRACE("SetSizeSF(%p,%llx) wbfs=%p wdf=%p\n",sf,(u64)off,sf->wbfs,sf->wdf);

    sf->file_size = off;
    switch (sf->iod.oft)
    {
	case OFT_PLAIN:	return opt_truncate ? ERR_OK : SetSizeF(&sf->f,off);
	case OFT_WDF1:
	case OFT_WDF2:
	case OFT_WIA:
	case OFT_GCZ:
	case OFT_CISO:
	case OFT_WBFS:	return ERR_OK;
	default:	return ERROR0(ERR_INTERNAL,0);
    };
}

///////////////////////////////////////////////////////////////////////////////

enumError SetMinSizeSF ( SuperFile_t * sf, off_t off )
{
    DASSERT(sf);
    TRACE("SetMinSizeSF(%p,%llx) wbfs=%p wdf=%p fsize=%llx\n",
		sf, (u64)off, sf->wbfs, sf->wdf, (u64)sf->file_size );

    return sf->file_size < off ? SetSizeSF(sf,off) : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError MarkMinSizeSF ( SuperFile_t * sf, off_t off )
{
    DASSERT(sf);
    TRACE("MarkMinSizeSF(%p,%llx) wbfs=%p wdf=%p min_fsize=%llx\n",
		sf, (u64)off, sf->wbfs, sf->wdf, (u64)sf->min_file_size );

    if ( sf->f.seek_allowed )
	return SetMinSizeSF(sf,off);

    if ( sf->min_file_size < off )
	 sf->min_file_size = off;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetGoodMinSize ( bool is_gc )
{
    return opt_disc_size
	    ? opt_disc_size
	    : is_gc
		? GC_DISC_SIZE
		: (u64)WII_SECTORS_SINGLE_LAYER * WII_SECTOR_SIZE;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    ReadWBFS() & WriteWBFS()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadWBFS ( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    TRACE(TRACE_RDWR_FORMAT, "#B# ReadWBFS()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    DASSERT(sf->wbfs);
    DASSERT(sf->wbfs->disc);
    DASSERT(sf->wbfs->disc->header);
    u16 * wlba_tab = sf->wbfs->disc->header->wlba_table;
    DASSERT(wlba_tab);

    wbfs_t * w = sf->wbfs->wbfs;
    DASSERT(w);

    u32 bl = (u32)( off / w->wbfs_sec_sz );
    u32 bl_off = (u32)( off - (u64)bl * w->wbfs_sec_sz );

    while ( count > 0 )
    {
	u32 max_count = w->wbfs_sec_sz - bl_off;
	ASSERT( max_count > 0 );
	if  ( max_count > count )
	    max_count = count;

	const u32 wlba = bl < w->n_wbfs_sec_per_disc ? ntohs(wlba_tab[bl]) : 0;

	TRACE(">> BL=%d[%d], BL-OFF=%x, count=%x/%zx\n",
		bl, wlba, bl_off, max_count, count );

	if (wlba)
	{
	    enumError err = ReadAtF( &sf->f,
				     (off_t)w->wbfs_sec_sz * wlba + bl_off,
				     buf, max_count);
	    if (err)
		return err;
	}
	else
	    memset(buf,0,max_count);

	bl++;
	bl_off = 0;
	count -= max_count;
	buf = (char*)buf + max_count;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

off_t DataBlockWBFS
	( SuperFile_t * sf, off_t off, size_t hint_align, off_t * block_size )
{
    DASSERT(sf->wbfs);
    DASSERT(sf->wbfs->disc);
    DASSERT(sf->wbfs->disc->header);
    u16 * wlba_tab = sf->wbfs->disc->header->wlba_table;
    DASSERT(wlba_tab);

    wbfs_t * w = sf->wbfs->wbfs;
    DASSERT(w);

    u32 block = (u32)( off / w->wbfs_sec_sz );

    for(;;)
    {
	if ( block >= w->n_wbfs_sec_per_disc )
	    return DataBlockStandard(sf,off,hint_align,block_size);
	if ( ntohs(wlba_tab[block]) )
	    break;
	block++;
    }

    const off_t off1 = block * w->wbfs_sec_sz;
    if ( off < off1 )
	 off = off1;

    if (block_size)
    {
	while ( block < w->n_wbfs_sec_per_disc && ntohs(wlba_tab[block]) )
	    block++;
	*block_size = block * w->wbfs_sec_sz - off;
    }

    return off;
}

///////////////////////////////////////////////////////////////////////////////

void FileMapWBFS ( SuperFile_t * sf, FileMap_t *fm )
{
    DASSERT(sf);
    DASSERT(fm);
    DASSERT(!fm->used);

    ASSERT(sf->wbfs);
    ASSERT(sf->wbfs->disc);
    ASSERT(sf->wbfs->disc->header);
    u16 * wlba_tab = sf->wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t * w = sf->wbfs->wbfs;
    ASSERT(w);

    const u64 block_size = w->wbfs_sec_sz;

    uint block;
    for ( block = 0; block < w->n_wbfs_sec_per_disc; block++ )
    {
	const uint src_block = ntohs(wlba_tab[block]);
	if (src_block)
	    AppendFileMap( fm,
			block_size * block,
			block_size * src_block,
			block_size );
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteWBFS
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    TRACE(TRACE_RDWR_FORMAT, "#B# WriteWBFS()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    ASSERT(sf->wbfs);
    wbfs_disc_t * disc = sf->wbfs->disc;
    ASSERT(disc);
    ASSERT(disc->header);
    u16 * wlba_tab = sf->wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t * w = sf->wbfs->wbfs;
    ASSERT(w);

    u32 bl = (u32)( off / w->wbfs_sec_sz );
    u32 bl_off = (u32)( off - (u64)bl * w->wbfs_sec_sz );

    while ( count > 0 )
    {
	u32 max_count = w->wbfs_sec_sz - bl_off;
	ASSERT( max_count > 0 );
	if  ( max_count > count )
	    max_count = count;

	if ( bl >= w->n_wbfs_sec_per_disc )
	{
	    sf->f.last_error = ERR_WRITE_FAILED;
	    if ( sf->f.max_error < ERR_WRITE_FAILED )
		 sf->f.max_error = ERR_WRITE_FAILED;
	    if (!sf->f.disable_errors)
		ERROR0( ERR_WRITE_FAILED, 
			"Can't write behind WBFS limit [%c=%d]: %s\n",
			GetFT(&sf->f), GetFD(&sf->f), sf->f.fname );
	    return ERR_WRITE_FAILED;
	}

	u32 wlba = ntohs(wlba_tab[bl]);
	if (!wlba)
	{
	    wlba = wbfs_alloc_block(w,ntohs(wlba_tab[0]));
	    if ( wlba == WBFS_NO_BLOCK )
	    {
		sf->f.last_error = ERR_WRITE_FAILED;
		if ( sf->f.max_error < ERR_WRITE_FAILED )
		     sf->f.max_error = ERR_WRITE_FAILED;
		if (!sf->f.disable_errors)
		    ERROR0( ERR_WRITE_FAILED, 
			    "Can't allocate a free WBFS block [%c=%d]: %s\n",
			    GetFT(&sf->f), GetFD(&sf->f), sf->f.fname );
		return ERR_WRITE_FAILED;
	    }
	    noPRINT_IF( bl <= 10, "WBFS WRITE: wlba_tab[%x] = %x\n",bl,wlba);
	    wlba_tab[bl] = htons(wlba);
	    disc->is_dirty = 1;
	}

	if ( disc->is_creating && off < sizeof(disc->header->dhead) )
	{
	    size_t copy_count = 256 - (size_t)off;
	    if ( copy_count > count )
		 copy_count = count;
	    PRINT("WBFS WRITE HEADER: %llx + %zx\n",(u64)off,copy_count);
	    memcpy( disc->header->dhead + off, buf, copy_count );
	    disc->is_dirty = 1;
	}

	TRACE(">> BL=%d[%d], BL-OFF=%x, count=%x/%zx\n",
		bl, wlba, bl_off, max_count, count );

	enumError err = WriteAtF( &sf->f,
				 (off_t)w->wbfs_sec_sz * wlba + bl_off,
				 buf, max_count);
	if (err)
	    return err;

	bl++;
	bl_off = 0;
	count -= max_count;
	buf = (char*)buf + max_count;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroWBFS ( SuperFile_t * sf, off_t off, size_t size )
{
    // [[2do]] [zero] optimization

    while ( size > 0 )
    {
	const size_t size1 = size < sizeof(zerobuf) ? size : sizeof(zerobuf);
	const enumError err = WriteWBFS(sf,off,zerobuf,size1);
	if (err)
	    return err;
	off  += size1;
	size -= size1;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		SuperFile: read + write wrapper		///////////////
///////////////////////////////////////////////////////////////////////////////

int WrapperReadSF ( void * p_sf, u32 offset, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.read_func);

    return sf->iod.read_func( sf, (off_t)offset << 2, iobuf, count );
}

///////////////////////////////////////////////////////////////////////////////

int WrapperReadDirectSF ( void * p_sf, u32 offset, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.read_func);

    return sf->std_read_func( sf, (off_t)offset << 2, iobuf, count );
}

///////////////////////////////////////////////////////////////////////////////

int WrapperWriteSF ( void * p_sf, u32 lba, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.write_func);

    return sf->iod.write_func(
		sf,
		(off_t)lba * WII_SECTOR_SIZE,
		iobuf,
		count * WII_SECTOR_SIZE );
}

///////////////////////////////////////////////////////////////////////////////

int WrapperWriteSparseSF ( void * p_sf, u32 lba, u32 count, void * iobuf )
{
    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    SuperFile_t * sf = (SuperFile_t *)p_sf;
    DASSERT(sf);
    DASSERT(sf->iod.write_sparse_func);

    return sf->iod.write_sparse_func(
		sf,
		(off_t)lba * WII_SECTOR_SIZE,
		iobuf,
		count * WII_SECTOR_SIZE );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                SuperFile_t: progress            ///////////////
///////////////////////////////////////////////////////////////////////////////
// progress and statistics

void CopyProgressSF ( SuperFile_t * dest, SuperFile_t * src )
{
    DASSERT(dest);
    DASSERT(src);

    dest->indent			= src->indent;
    dest->show_progress			= src->show_progress;
    dest->show_summary			= src->show_summary;
    dest->show_msec			= src->show_msec;

    dest->progress_trigger		= src->progress_trigger;
    dest->progress_trigger_init		= src->progress_trigger_init;
    dest->progress_start_time		= src->progress_start_time;
    dest->progress_last_view_sec	= src->progress_last_view_sec;
    dest->progress_max_wd		= src->progress_max_wd;
    dest->progress_verb			= src->progress_verb;
}

///////////////////////////////////////////////////////////////////////////////

void PrintProgressSF ( u64 p_done, u64 p_total, void * param )
{
    SuperFile_t * sf = (SuperFile_t*) param;
    TRACE("PrintProgressSF(%llu,%llu), sf=%p, fd=%d, sh-prog=%d\n",
		p_done, p_total, sf,
		sf ? GetFD(&sf->f) : -2,
		sf ? sf->show_progress : -1 );

    if ( !sf || !sf->show_progress || !p_total )
	return;

    sf->progress_last_done  = p_done;
    sf->progress_last_total = p_total;

    if ( sf->progress_trigger <= 0 && p_done )
	return;

    p_total += sf->progress_add_total;
    if ( p_total > (u64)1 << 44 )
    {
	// avoid integer overflow
	p_done  >>= 20;
	p_total >>= 20;
    }

    const u32 now		= GetTimerMSec();
    const u32 elapsed		= now - sf->progress_start_time;
    const u32 view_sec		= elapsed / 1000;
    const u32 max_rate		= 1000000;
    const u32 rate		= max_rate * p_done / p_total;
    const u32 percent		= rate / (max_rate/100);
    const u64 total		= sf->f.bytes_read + sf->f.bytes_written;

    noTRACE(" - sec=%d->%d, rate=%d, total=%llu=%llu+%llu\n",
		sf->progress_last_view_sec, view_sec, rate,
		total, sf->f.bytes_read, sf->f.bytes_written );

    if ( sf->progress_last_view_sec != view_sec && rate > 0 && total > 0 )
    {
	sf->progress_trigger		 = sf->progress_trigger_init;
	sf->progress_last_view_sec	 = view_sec;

	// eta = elapsed / rate * max_rate - elapsed;
	const u32 eta = elapsed * (u64)max_rate / rate - elapsed;

	char buf1[50], buf2[50];
	ccp time1 = PrintTimerMSec(buf1,sizeof(buf1),elapsed,sf->show_msec?3:0);
	ccp time2 = PrintTimerMSec(buf2,sizeof(buf2),eta,0);

	noPRINT("PROGRESS: now=%7u perc=%3u ela=%6u eta=%6u [%s,%s]\n",
		now, percent, elapsed, eta, time1, time2 );

	if ( !sf->progress_verb || !*sf->progress_verb )
	    sf->progress_verb = "copied";

	if (print_sections)
	{
	    printf(
		"[progress]\n"
		"verb=%s\n"
		"percent=%d\n"
		"elapsed-msec=%d\n"
		"elapsed-text=%s\n"
		"eta-trigger=%d\n"
		"eta-msec=%d\n"
		"eta-text=%s\n"
		"mib-total=%llu\n"
		"mib-per-sec=%3.1f\n"
		"\n"
		,sf->progress_verb
		,percent
		,elapsed
		,time1
		,percent >= 10 || view_sec >= 10
		,eta
		,time2
		,(total+MiB/2)/MiB
		,(double)total * 1000 / MiB / elapsed
		);
	}
	else
	{
	    int wd;
	    if ( percent < 10 && view_sec < 10 )
		printf("%*s%3d%% %s in %s (%3.1f MiB/sec)  %n\r",
		    sf->indent,"", percent, sf->progress_verb, time1,
		    (double)total * 1000 / MiB / elapsed, &wd );
	    else
		printf("%*s%3d%% %s in %s (%3.1f MiB/sec) -> ETA %s   %n\r",
		    sf->indent,"", percent, sf->progress_verb, time1,
		    (double)total * 1000 / MiB / elapsed, time2, &wd );

	    if ( sf->progress_max_wd < wd )
		sf->progress_max_wd = wd;
	}
	fflush(stdout);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClearProgressLineSF ( SuperFile_t * sf )
{
    if ( sf && sf->show_progress && sf->progress_max_wd > 0 )
    {
	if (!print_sections)
	    printf("%*s\r",sf->progress_max_wd,"");
	sf->progress_max_wd = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrintSummarySF ( SuperFile_t * sf )
{
    if ( !sf || !sf->show_progress && !sf->show_summary )
	return;

    char buf[200];
    buf[0] = 0;

    FlushSF(sf);
    if (sf->show_summary)
    {
	const u32 elapsed = GetTimerMSec() - sf->progress_start_time;
	char timbuf[50];
	ccp tim = PrintTimerMSec(timbuf,sizeof(timbuf),elapsed,sf->show_msec?3:0);

	char ratebuf[50] = {0};
	u64 total = sf->f.bytes_read + sf->f.bytes_written;
	if (sf->source_size)
	{
	    total = sf->source_size;
	    if ( sf->f.max_off )
		snprintf(ratebuf,sizeof(ratebuf),
			", compressed to %4.2f%%",
			100.0 * (double)sf->f.max_off / sf->source_size );
	}

	if ( !sf->progress_verb || !*sf->progress_verb )
	    sf->progress_verb = "copied";

	if (print_sections)
	{
	    printf(
		"[progress:summary]\n"
		"verb=%s\n"
		"elapsed-msec=%d\n"
		"elapsed-text=%s\n"
		"mib-total=%llu\n"
		"mib-per-sec=%3.1f\n"
		,sf->progress_verb
		,elapsed
		,tim
		,(total+MiB/2)/MiB
		,(double)total * 1000 / MiB / elapsed
		);

	    if (*ratebuf)
		printf("compression-percent=%4.2f\n",
			100.0 * (double)sf->f.max_off / sf->source_size);

	    putchar('\n');
	}
	else if (total)
	{
	    snprintf(buf,sizeof(buf),"%*s%4llu MiB %s in %s, %4.1f MiB/sec%s",
		sf->indent,"", (total+MiB/2)/MiB, sf->progress_verb,
		tim, (double)total * 1000 / MiB / elapsed, ratebuf );
	}
	else
	    snprintf(buf,sizeof(buf),"%*sFinished in %s%s",
		sf->indent,"", tim, ratebuf );
    }

    if (!print_sections)
    {
	if (sf->show_progress)
	    printf("%-*s\n",sf->progress_max_wd,buf);
	else
	    printf("%s\n",buf);
    }
    fflush(stdout);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DefineProgressChunkSF
(
    SuperFile_t		* sf,		// valid file
    u64			data_size,	// the relevant data size
    u64			chunk_size	// size of chunk to write
)
{
    DASSERT(sf);
    TRACE("PROGCHUNK: %llu, %llu [setup]\n",data_size,chunk_size);
    sf->progress_data_size  = data_size;
    sf->progress_chunk_size = chunk_size;
}

///////////////////////////////////////////////////////////////////////////////

void PrintProgressChunkSF
(
    SuperFile_t		* sf,		// valid file
    u64			chunk_done	// size of progresses data
)
{
    if ( sf && sf->show_progress && sf->progress_chunk_size )
    {
	const u64 last_done	 = sf->progress_last_done;
	const int trigger	 = sf->progress_trigger;
	sf->progress_trigger	 = 1;
	sf->f.bytes_read	+= chunk_done;

	u64 done = last_done
		 + chunk_done * sf->progress_data_size / sf->progress_chunk_size
		 - sf->progress_data_size;
	u64 total = sf->progress_last_total + sf->progress_add_total;

	noTRACE("PROGCHUNK: %u,%u %u [+%u]\n",
		last_done, done, total, sf->progress_add_total );

	if ( done > total )
	     done = total;
	PrintProgressSF( done, sf->progress_last_total, sf );

	sf->f.bytes_read	-= chunk_done;
	sf->progress_last_done	 = last_done;
	sf->progress_trigger	 = trigger;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     AnalyzeFT()                 ///////////////
///////////////////////////////////////////////////////////////////////////////

enumFileType AnalyzeFT ( WFile_t * f )
{
    DASSERT(f);

    PRINT("AnalyzeFT(%p) fd=%d, split=%d, rd=%d, wr=%d\n",
	f, GetFD(f), IsSplittedF(f), f->is_reading, f->is_writing );
    f->id6_src[6] = f->id6_dest[6] = 0;

    if (!IsOpenF(f))
    {
	TRACELINE;
	f->ftype = FT_UNKNOWN;

	if ( !f->is_reading )
	    return f->ftype;

	ccp name = strrchr(f->fname,'/');
	if (!name)
	    return f->ftype;

	enum { IS_NONE, IS_ID6, IS_INDEX, IS_SLOT } mode = IS_NONE;
	char id6[7];

	ulong idx = 0;
	name++;
	if ( strlen(name) == 6 )
	{
	    TRACELINE;
	    // maybe an id6
	    int i;
	    for ( i = 0; i < 6; i++ )
		id6[i] = toupper((int)name[i]); // cygwin needs the '(int)'
	    id6[6] = 0;
	    if (CheckID6(id6,false,false))
		mode = IS_ID6;
	}

	if ( mode == IS_NONE )
	{
	    TRACELINE;
	    const bool is_slot = *name == '#';
	    ccp start = name + is_slot;
	    char * end;
	    idx = strtoul(start,&end,10);
	    if ( end > start && !*end )
		mode = is_slot ? IS_SLOT : IS_INDEX;
	    else
		return f->ftype;
	}

	TRACELINE;
	ASSERT( mode != IS_NONE );

	// try open ..
	char fname[PATH_MAX];
	int max = name - f->fname;
	if ( max > sizeof(fname) )
	    max = sizeof(fname);
	memcpy(fname,f->fname,max);
	fname[max-1] = 0;

	SuperFile_t sf;
	InitializeSF(&sf);
	sf.f.disable_errors = true;
	if (f->is_writing)
	{
	    if (OpenWFileModify(&sf.f,fname,IOM_IS_WBFS_PART))
		return f->ftype;
	}
	else
	{
	    if (OpenWFile(&sf.f,fname,IOM_IS_WBFS_PART))
		return f->ftype;
	}

	AnalyzeFT(&sf.f);

	bool ok = false;
	WDiscInfo_t wdisk;
	InitializeWDiscInfo(&wdisk);

	ASSERT( Count1Bits64(sf.f.ftype&FT__ID_MASK) <= 1  ); // [[2do]] [[ft-id]]

	if ( sf.f.ftype & FT_ID_WBFS )
	{
	    TRACE(" - f2=WBFS\n");
	    WBFS_t wbfs;
	    InitializeWBFS(&wbfs);
	    if (!SetupWBFS(&wbfs,&sf,false,0,false))
	    {
		switch(mode)
		{
		    case IS_ID6:
			ok = !FindWDiscInfo(&wbfs,&wdisk,id6);
			break;

		    case IS_INDEX:
			if (!GetWDiscInfo(&wbfs,&wdisk,idx))
			{
			    memcpy(id6,wdisk.id6,6);
			    ok = true;
			}
			break;

		    case IS_SLOT:
			if (!GetWDiscInfoBySlot(&wbfs,&wdisk,idx))
			{
			    memcpy(id6,wdisk.id6,6);
			    ok = true;
			}
			break;

		    default: // suppress warning "IS_NONE not handled in switch"
			break;
		}
	    }
	    TRACELINE;
	    ResetWBFS(&wbfs);
	}

	if (ok)
	{
	    TRACE(" - WBFS/%s found, slot=%d\n",id6,wdisk.slot);
	    sf.f.disable_errors = f->disable_errors;
	    ASSERT(!sf.f.path);
	    sf.f.path = sf.f.fname;
	    sf.f.fname = f->fname;
	    f->fname = 0;
	    sf.f.slot = wdisk.slot;
	    CopyFileAttribDiscInfo(&sf.f.fatt,&wdisk);

	    ResetWFile(f,false);
	    memcpy(f,&sf.f,sizeof(*f));
	    SetPatchFileID(f,id6,6);
	    memset(&sf.f,0,sizeof(sf.f));
	    TRACE(" - WBFS/fname = %s\n",f->fname);
	    TRACE(" - WBFS/path  = %s\n",f->path);
	    TRACE(" - WBFS/id6   = %s -> %s\n",f->id6_src,f->id6_dest);
	    switch(get_header_disc_type(&wdisk.dhead,0))
	    {
		case WD_DT_UNKNOWN:
		case WD_DT__N:
		    f->ftype |= FT_A_WDISC;
		    break;

		case WD_DT_GAMECUBE:
		    f->ftype |= FT_A_ISO|FT_A_GC_ISO|FT_A_WDISC;
		    break;

		case WD_DT_WII:
		    f->ftype |= FT_A_ISO|FT_A_WII_ISO|FT_A_WDISC;
		    break;
	    }
	}
	else
	    ResetSF(&sf,0);
	return f->ftype;
    }

    enumFileType ft = 0;

    if (S_ISREG(f->st.st_mode))
	ft |= FT_A_REGFILE;

    if (S_ISBLK(f->st.st_mode))
	ft |= FT_A_BLOCKDEV;

    if (S_ISCHR(f->st.st_mode))
	ft |= FT_A_CHARDEV;

    if (f->seek_allowed)
	ft |= FT_A_SEEKABLE;

    id6_t id6;
    if (S_ISDIR(f->st.st_mode))
    {
	f->ftype = IsFST(f->fname,id6);
	if ( f->ftype & FT_ID_FST )
	{
	    SetPatchFileID(f,id6,6);
	    PRINT("FST found, id=%s,%s: %s\n", f->id6_src, f->id6_dest, f->fname );
	}
	return f->ftype;
    }

    if (f->is_writing)
	ft |= FT_A_WRITING;

    if (!f->is_reading)
	return f->ftype;


    //----- now we must analyze the file contents

    // disable warnings
    const bool disable_errors = f->disable_errors;
    f->disable_errors = true;

    // read file header
    char buf1[FILE_PRELOAD_SIZE];
    char buf2[FILE_PRELOAD_SIZE];
    wd_disc_type_t disc_type;

    TRACELINE;
    memset(buf1,0,sizeof(buf1));
    const uint max_read = f->st.st_size < sizeof(buf1)
			? f->st.st_size : sizeof(buf1);
    enumError err = ReadAtF(f,0,&buf1,max_read);
    if (err)
    {
	TRACELINE;
	ft |= FT_ID_OTHER;
    }
    else if (IsWIA(buf1,sizeof(buf1),id6,&disc_type,0))
    {
	SetPatchFileID(f,id6,6);
	PRINT("WIA found, dt=%d, id=%s,%s: %s\n",
		disc_type, f->id6_src, f->id6_dest, f->fname );
	ft |= disc_type == WD_DT_GAMECUBE
		? FT_ID_GC_ISO  | FT_A_ISO | FT_A_GC_ISO  | FT_A_WIA
		: FT_ID_WII_ISO | FT_A_ISO | FT_A_WII_ISO | FT_A_WIA;
    }
    else
    {
     #ifdef DEBUG
	{
	  ccp ptr = buf1;
	  int i;
	  for ( i = 0; i < 4; i++, ptr+=8 )
	  {
	    TRACE("ANALYZE/HEAD: \"%c%c%c%c%c%c%c%c\"  "
			"%02x %02x %02x %02x  %02x %02x %02x %02x\n",
		ptr[0]>=' ' && ptr[0]<0x7f ? ptr[0] : '.',
		ptr[1]>=' ' && ptr[1]<0x7f ? ptr[1] : '.',
		ptr[2]>=' ' && ptr[2]<0x7f ? ptr[2] : '.',
		ptr[3]>=' ' && ptr[3]<0x7f ? ptr[3] : '.',
		ptr[4]>=' ' && ptr[4]<0x7f ? ptr[4] : '.',
		ptr[5]>=' ' && ptr[5]<0x7f ? ptr[5] : '.',
		ptr[6]>=' ' && ptr[6]<0x7f ? ptr[6] : '.',
		ptr[7]>=' ' && ptr[7]<0x7f ? ptr[7] : '.',
		(u8)ptr[0], (u8)ptr[1], (u8)ptr[2], (u8)ptr[3],
		(u8)ptr[4], (u8)ptr[5], (u8)ptr[6], (u8)ptr[7] );
	  }
	}
     #endif // DEBUG

	ccp data_ptr = buf1;

	wdf_header_t wh;
	ConvertToHostWH(&wh,(wdf_header_t*)buf1);

	err = AnalyzeWH(f,&wh,false);
	//if ( err != ERR_NO_WDF ) // not sure about the place
	//    ft |= wh.wdf_version > 1 ? FT_A_WDF2 : FT_A_WDF1;

	if (!err)
	{
	    TRACE(" - WDF v%u found -> load first chunk\n",wh.wdf_version);
	    ft |= wh.wdf_version > 1 ? FT_A_WDF2 : FT_A_WDF1;

	    data_ptr += wh.head_size;
	    if ( f->seek_allowed
		&& !ReadAtF(f,wh.chunk_off,&buf2,WDF_MAGIC_SIZE+sizeof(wdf1_chunk_t))
		&& !memcmp(buf2,WDF_MAGIC,WDF_MAGIC_SIZE) )
	    {
		TRACE(" - WDF chunk loaded\n");
		wdf2_chunk_t *wc = (wdf2_chunk_t*)(buf2+WDF_MAGIC_SIZE);
		ConvertToHostWC(wc,wc,wh.wdf_version,1);
		if ( wc->data_size >= 6 )
		{
		    // save param before clear buffer
		    const off_t off = wc->data_off;
		    const u32 ldlen = wc->data_size < sizeof(buf2)
				    ? wc->data_size : sizeof(buf2);
		    memset(buf2,0,sizeof(buf2));
		    if (!ReadAtF(f,off,&buf2,ldlen))
		    {
			TRACE(" - %d bytes of first chunk loaded\n",ldlen);
			data_ptr = buf2;
		    }
		}
	    }
	}

	if (!memcmp(data_ptr,"CISO",4))
	{
	    CISO_Head_t ch;
	    CISO_Info_t ci;
	    InitializeCISO(&ci,0);
	    if ( !ReadAtF(f,0,&ch,sizeof(ch)) && !SetupCISO(&ci,&ch) )
	    {
		TRACE(" - CISO found -> load part of first block\n");
		ft |= FT_A_CISO;
		if (!ReadAtF(f,sizeof(CISO_Head_t),&buf2,sizeof(buf2)))
		    data_ptr = buf2;
	    }
	    ResetCISO(&ci);
	}

	enumFileType gcz_type = AnalyzeGCZ(data_ptr,sizeof(buf1),f->st.st_size,0);
	if (gcz_type)
	{
	    GCZ_t gcz;
	    if ( !LoadHeadGCZ(&gcz,f,true) && !LoadDataGCZ(&gcz,f,0,buf2,sizeof(buf2)) )
	    {
		data_ptr = buf2;
		if ( gcz_type & FT_A_GCZ || be32(buf2+0x200) == 0x4E4B4954 ) // NKIT
		    ft |= gcz_type;
	    }
	    ResetGCZ(&gcz);
	}

	TRACE("ISO ID6+MAGIC:  \"%c%c%c%c%c%c\"    %02x %02x %02x %02x %02x %02x / %08x\n",
		data_ptr[0]>=' ' && data_ptr[0]<0x7f ? data_ptr[0] : '.',
		data_ptr[1]>=' ' && data_ptr[1]<0x7f ? data_ptr[1] : '.',
		data_ptr[2]>=' ' && data_ptr[2]<0x7f ? data_ptr[2] : '.',
		data_ptr[3]>=' ' && data_ptr[3]<0x7f ? data_ptr[3] : '.',
		data_ptr[4]>=' ' && data_ptr[4]<0x7f ? data_ptr[4] : '.',
		data_ptr[5]>=' ' && data_ptr[5]<0x7f ? data_ptr[5] : '.',
		(u8)data_ptr[0], (u8)data_ptr[1], (u8)data_ptr[2],
		(u8)data_ptr[3], (u8)data_ptr[4], (u8)data_ptr[5],
		*(u32*)(data_ptr + WII_MAGIC_OFF) );

	const enumFileType mt = AnalyzeMemFT(data_ptr,f->st.st_size);
	switch (mt)
	{
	    case FT_ID_WBFS:
		{
		    WBFS_t wbfs;
		    InitializeWBFS(&wbfs);
		    if (!OpenWBFS(&wbfs,f->fname,false,false,0))
		    {
			ft |= FT_ID_WBFS;
			if ( wbfs.used_discs == 1 )
			{
			    WDiscInfo_t dinfo;
			    InitializeWDiscInfo(&dinfo);
			    if (!GetWDiscInfo(&wbfs,&dinfo,0))
			    {
			      switch (dinfo.disc_type)
			      {
				case WD_DT_UNKNOWN:
				case WD_DT__N:
				  break;

				case WD_DT_GAMECUBE:
				  ft |= FT_A_ISO | FT_A_GC_ISO;
				  break;

				case WD_DT_WII:
				  ft |= FT_A_ISO | FT_A_WII_ISO;
				  break;
			      }
			      SetPatchFileID(f,dinfo.id6,6);
			    }
			    ResetWDiscInfo(&dinfo);
			}
		    }
		    ResetWBFS(&wbfs);
		}
		break;

	    case FT_ID_GC_ISO:
		ft |= FT_ID_GC_ISO | FT_A_ISO | FT_A_GC_ISO;
		SetPatchFileID(f,data_ptr,6);
		if ( f->st.st_size < ISO_SPLIT_DETECT_SIZE )
		    SetupSplitWFile(f,OFT_PLAIN,0);
		break;

	    case FT_ID_WII_ISO:
		if ( be32(buf1+0x200) == 0x4e4b4954 ) // NKIT
		{
		    ft |= FT_ID_WII_ISO | FT_A_ISO | FT_A_WII_ISO | FT_A_NKIT_ISO;
		    memcpy(f->id6_src,buf1,6);
		}
		else
		{
		    ft |= FT_ID_WII_ISO | FT_A_ISO | FT_A_WII_ISO;
		    if ( !(ft&FT_M_WDF) && !f->seek_allowed )
			DefineCachedAreaISO(f,false);

		    SetPatchFileID(f,data_ptr,6);
		    if ( f->st.st_size < ISO_SPLIT_DETECT_SIZE )
			SetupSplitWFile(f,OFT_PLAIN,0);
		}
		break;

	    case FT_ID_HEAD_BIN:
	    case FT_ID_BOOT_BIN:
		ft |= mt;
		SetPatchFileID(f,data_ptr,6);
		break;

	    default:
		ft |= mt;
	}
    }

    TRACE("ANALYZE FILETYPE: %llx [%s,%s]\n",(u64)ft,f->id6_src,f->id6_dest);

    // restore warnings
    f->disable_errors = disable_errors;

    // [[2do]] [[ft-id]]
    ASSERT_MSG( Count1Bits64(ft&FT__ID_MASK) <= 1,
		"ft = %llx, nbits = %u\n", (u64)ft, Count1Bits64(ft&FT__ID_MASK) );
    return f->ftype = ft;
}

///////////////////////////////////////////////////////////////////////////////

enumFileType AnalyzeMemFT ( const void * preload_buf, off_t file_size )
{
    // make some quick tests for different file formats

    ccp data = preload_buf;
    TRACE("AnalyzeMemFT(,%llx) id=%08x magic=%08x\n",
		(u64)file_size, be32(data), be32(data+WII_MAGIC_OFF) );


    //----- test WIT-PATCH

    if (!memcmp(preload_buf,wpat_magic,sizeof(wpat_magic)))
    {
	const wpat_header_t * wpat = preload_buf;
	if ( wpat->type_size.type == WPAT_HEADER )
	    return FT_ID_PATCH;
    }


    //----- test BOOT.BIN or ISO

    if (CheckID6(data,false,false))
    {
	if ( be32(data+WII_MAGIC_OFF) == WII_MAGIC )
	{
	    if ( file_size == WII_BOOT_SIZE )
		return FT_ID_BOOT_BIN;
	    if ( file_size == sizeof(wd_header_t) || file_size == WBFS_INODE_INFO_OFF )
		return FT_ID_HEAD_BIN;
	    if ( !file_size || file_size >= WII_PART_OFF )
		return FT_ID_WII_ISO;
	}
	else if ( be32(data+GC_MAGIC_OFF) == GC_MAGIC )
		return FT_ID_GC_ISO;
    }


    //----- test WBFS

    if (!memcmp(data,"WBFS",4))
	return FT_ID_WBFS;

    int i;
    const u32 check_size = HD_SECTOR_SIZE < file_size ? HD_SECTOR_SIZE : file_size;


    //----- test *.DOL

    const dol_header_t * dol = preload_buf;
    bool ok = true;
    for ( i = 0; ok && i < DOL_N_SECTIONS; i++ )
    {
	const u32 off  = ntohl(dol->sect_off[i]);
	const u32 size = ntohl(dol->sect_size[i]);
	const u32 addr = ntohl(dol->sect_addr[i]);
	PRINT(" %8x %8x %08x\n",off,size,addr);
	if ( off || size || addr )
	{
	    if (   off & 3
		|| size & 3
		|| addr & 3
		|| off < DOL_HEADER_SIZE
		|| file_size && off + size > file_size )
	    {
		ok = false;
		break;
	    }
	}
    }
    if (ok)
	return FT_ID_DOL;


    //----- test FST.BIN

    static u8 fst_magic[] = { 1,0,0,0, 0,0,0,0, 0 };
    if (!memcmp(data,fst_magic,sizeof(fst_magic)))
    {
	const u32 n_files = be32(data+8);
	const u32 max_name_off = (u32)file_size - n_files * sizeof(wd_fst_item_t);
	u32 max = be32(data+8);
	if ( max_name_off < file_size )
	{
	    if ( max > check_size/sizeof(wd_fst_item_t) )
		 max = check_size/sizeof(wd_fst_item_t);

	    const wd_fst_item_t * fst = preload_buf;
	    for ( i = 0; i < max; i++, fst++ )
	    {
		if ( (ntohl(fst->name_off)&0xffffff) >= max_name_off )
		    break;
		if ( fst->is_dir && ntohl(fst->size) > n_files )
		    break;
	    }
	    if ( i == max )
		return FT_ID_FST_BIN;
	}
    }


    //----- test cert + ticket + tmd

// [[FT_ID_SIG_BIN]]

    const u32 sig_type	= be32(preload_buf);
    const int sig_size	= cert_get_signature_size(sig_type);
    const u32 head_size	= ALIGN32( sig_size + sizeof(cert_head_t), WII_CERT_ALIGN );
    const bool is_root_sig = sig_size
			   && sig_size < file_size
			   && !memcmp(preload_buf+head_size,"Root",4);
    if (is_root_sig)
    {
	//----- test CERT.bin

	if ( !(file_size & WII_CERT_ALIGN-1) )
	{
	    const cert_data_t * data = cert_get_data(preload_buf);
	    if ( data
		&& (ccp)data + sizeof(data) < (ccp)preload_buf + file_size
		&& data->key_id[0] > ' ' && data->key_id[0] < 0x7f )
	    {
		const u8 * next = (u8*)cert_get_next_head(data);
		if ( next && next <= (u8*)preload_buf + file_size )
		    return FT_ID_CERT_BIN;
	    }
	}


	//----- test TICKET.BIN

	if ( sig_type == 0x10001 && file_size >= sizeof(wd_ticket_t) )
	{
	    if ( file_size == sizeof(wd_ticket_t)
		|| cert_get_signature_size(be32((ccp)preload_buf+sizeof(wd_ticket_t))) )
	    {
		return FT_ID_TIK_BIN;
	    }
	}


	//----- test TMD.BIN

	if ( sig_type == 0x10001 && file_size >= sizeof(wd_tmd_t) )
	{
	    const wd_tmd_t * tmd = preload_buf;
	    const int n = ntohs(tmd->n_content);
	    if ( n > 0 )
	    {
		const int tmd_size = sizeof(wd_tmd_t) + n * sizeof(wd_tmd_content_t);
		if ( file_size == tmd_size
		    || file_size > tmd_size
			    && cert_get_signature_size(be32((ccp)preload_buf+tmd_size)) )
		{
		    return FT_ID_TMD_BIN;
		}
	    }
	}
    }

    if	(  sig_size
	&& file_size >= 0x108
	&& !memcmp(preload_buf+0x80,"Root-",5)
	&& strlen(preload_buf+0xc4) > 6
	)
    {
	return FT_ID_SIG_BIN;
    }

    if (sig_size) // ISO usual data
    {
	//----- test TMD.BIN (ISO usual data)

	if ( file_size >= sizeof(wd_tmd_t) )
	{
	    const wd_tmd_t * tmd = preload_buf;
	    const int n = ntohs(tmd->n_content);
	    if ( file_size == sizeof(wd_tmd_t) + n * sizeof(wd_tmd_content_t)
		&& CheckID4(tmd->title_id+4,true,false) )
	    {
		return FT_ID_TMD_BIN;
	    }
	}


	//----- test TICKET.BIN (ISO usual data)

	if ( file_size == sizeof(wd_ticket_t) )
	{
	    const wd_ticket_t * tik = preload_buf;
	    if ( CheckID4(tik->title_id+4,true,false) )
		return FT_ID_TIK_BIN;
	}
    }


    //----- fall back to iso

    if ( CheckID6(data,false,false) && be32(data+WII_MAGIC_OFF) == WII_MAGIC )
	return FT_ID_WII_ISO;


    return FT_ID_OTHER;
}

///////////////////////////////////////////////////////////////////////////////

enumError XPrintErrorFT ( XPARM WFile_t * f, enumFileType err_mask )
{
    ASSERT(f);
    if ( f->ftype == FT_UNKNOWN )
	AnalyzeFT(f);

// [[2do]] [[ft-id]]
    enumError stat = ERR_OK;
    enumFileType  and_mask = err_mask &  f->ftype;
    enumFileType nand_mask = err_mask & ~f->ftype;
    TRACE("PRINT FILETYPE ERROR: ft=%llx mask=%lx -> %lx,%lx\n",
		(u64)f->ftype, err_mask, and_mask, nand_mask );

    if ( ( f->split_f ? f->split_f[0]->fd : f->fd ) == -1 )
	stat = PrintError( XERROR0, ERR_READ_FAILED,
		"File is not open: %s\n", f->fname );

    else if ( and_mask & FT_ID_DIR )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Is a directory: %s\n", f->fname );

    else if ( and_mask & FT_A_WDISC )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Single images of a WBFS not allowed: %s\n", f->fname );

    else if ( nand_mask & FT_A_SEEKABLE )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Wrong file type (not seekable): %s\n", f->fname );

    else if ( nand_mask & (FT_A_REGFILE|FT_A_BLOCKDEV|FT_A_CHARDEV) )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Wrong file type: %s\n", f->fname );

    else if ( nand_mask & FT_ID_WBFS )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"WBFS expected: %s\n", f->fname );

    else if ( nand_mask & FT_ID_GC_ISO || nand_mask & FT_A_GC_ISO )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"GameCube ISO image expected: %s\n", f->fname );

    else if ( nand_mask & FT_ID_WII_ISO || nand_mask & FT_A_WII_ISO )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"Wii ISO image expected: %s\n", f->fname );

    else if ( nand_mask & FT_A_ISO )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"GameCube or Wii ISO image expected: %s\n", f->fname );

    else if ( nand_mask & FT_M_WDF )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"WDF expected: %s\n", f->fname );

    else if ( nand_mask & FT_A_WIA )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"WIA expected: %s\n", f->fname );

    else if ( nand_mask & FT_A_CISO )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"CISO expected: %s\n", f->fname );

    else if ( nand_mask & FT_A_GCZ )
	stat = PrintError( XERROR0, ERR_WRONG_FILE_TYPE,
		"GCZ expected: %s\n", f->fname );
// [[nkit]]

    f->last_error = stat;
    if ( f->max_error < stat )
	 f->max_error = stat;

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetNameFT ( enumFileType ftype, int ignore )
{
    //////////////////////////
    // limit is 8 characters!
    //////////////////////////

    switch ( ftype & FT__ID_MASK )
    {
	case FT_UNKNOWN:
	    return ignore ? 0 : "NO-FILE";

	case FT_ID_DIR:
	    return ignore > 1 ? 0 : "DIR";

	case FT_ID_FST:
	    return ftype & FT_A_PART_DIR
			? ( ftype & FT_A_GC_ISO ? "FST/GC" : "FST/WII" )
			: ( ftype & FT_A_GC_ISO ? "FST/GC+" : "FST/WII+" );

	case FT_ID_WBFS:
	    return ftype & FT_A_WDISC
			? ( ftype & FT_A_GC_ISO ? "WBFS/GC" : "WBFS/WII" )
			: ignore > 1
				? 0
				: ftype & FT_M_WDF
					? "WDF+WBFS"
					: "WBFS";

	case FT_ID_GC_ISO:
	    return ftype & FT_A_WDF1     ? "WDF1/GC"
		 : ftype & FT_A_WDF2     ? "WDF2/GC"
		 : ftype & FT_A_WIA      ? "WIA/GC"
		 : ftype & FT_A_CISO     ? "CISO/GC"
		 : ftype & FT_A_GCZ      ? "GCZ/GC"
		 : ftype & FT_A_NKIT_ISO ? "NK-I/GC"
		 : ftype & FT_A_NKIT_GCZ ? "NK-G/GC"
		 : "ISO/GC";

	case FT_ID_WII_ISO:
	    return ftype & FT_A_WDF1     ? "WDF1/WII"
		 : ftype & FT_A_WDF2     ? "WDF2/WII"
		 : ftype & FT_A_WIA      ? "WIA/WII"
		 : ftype & FT_A_CISO     ? "CISO/WII"
		 : ftype & FT_A_GCZ      ? "GCZ/WII"
		 : ftype & FT_A_NKIT_ISO ? "NK-I/WII"
		 : ftype & FT_A_NKIT_GCZ ? "NK-G/WII"
		 : "ISO/WII";

	case FT_ID_DOL:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/DOL"  : "DOL";

	case FT_ID_SIG_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/SIG"  : "SIG.BIN";

	case FT_ID_CERT_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/CERT" : "CERT.BIN";

	case FT_ID_TIK_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/TIK"  : "TIK.BIN";

	case FT_ID_TMD_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/TMD"  : "TMD.BIN";

	case FT_ID_HEAD_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/HEAD" : "HEAD.BIN";
	    break;

	case FT_ID_BOOT_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/BOOT" : "BOOT.BIN";

	case FT_ID_FST_BIN:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/FST"  : "FST.BIN";

	case FT_ID_PATCH:
	    return ignore > 1 ? 0 : ftype & FT_M_WDF ? "WDF/WPAT" : "WITPATCH";

	default:
	    return ignore > 1 ? 0
		: ftype & FT_A_WDF1     ? "WDF1/*"
		: ftype & FT_A_WDF2     ? "WDF2/*"
		: ftype & FT_A_WIA      ? "WIA/*"
		: ftype & FT_A_CISO     ? "CISO/*"
		: ftype & FT_A_GCZ      ? "GCZ/*"
		: ftype & FT_A_NKIT_ISO ? "NK-I/*"
		: ftype & FT_A_NKIT_GCZ ? "NK-G/*"
		: "OTHER";
    }
}

///////////////////////////////////////////////////////////////////////////////

ccp GetContainerNameFT ( enumFileType ftype, ccp answer_if_no_container )
{
    const enumOFT oft = FileType2OFT(ftype);
    return oft > OFT_PLAIN ? oft_info[oft].name : answer_if_no_container;
}

///////////////////////////////////////////////////////////////////////////////

enumOFT FileType2OFT ( enumFileType ftype )
{
    if ( ftype & FT_A_WDF2 )
	return OFT_WDF2;

    if ( ftype & FT_A_WDF1 )
	return OFT_WDF1;

    if ( ftype & FT_A_WIA )
	return OFT_WIA;

    if ( ftype & (FT_A_GCZ|FT_A_NKIT_GCZ) )
	return OFT_GCZ;

    if ( ftype & FT_A_CISO )
	return OFT_CISO;

    if ( ftype & FT_A_NKIT_ISO )
	return OFT_WBFS;

    switch ( ftype & FT__ID_MASK )
    {
	case FT_ID_FST:
	    return OFT_FST;

	case FT_ID_WBFS:
	    return OFT_WBFS;

	case FT_ID_GC_ISO:
	case FT_ID_WII_ISO:
	    return OFT_PLAIN;
    }

    return OFT_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_type_t FileType2DiscType ( enumFileType ftype )
{
    switch ( ftype & FT__ID_MASK )
    {
	case FT_ID_GC_ISO:  return WD_DT_GAMECUBE;
	case FT_ID_WII_ISO: return WD_DT_WII;
    }

    return ftype & FT_A_GC_ISO
		? WD_DT_GAMECUBE
		: ftype & FT_A_WII_ISO
			? WD_DT_WII
			: WD_DT_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////

u32 CountUsedIsoBlocksSF ( SuperFile_t * sf, const wd_select_t * psel )
{
    ASSERT(sf);

    u32 count = 0;
    if ( psel && psel->whole_disc )
	count = sf->file_size * WII_SECTORS_PER_MIB / MiB;
    else
    {
	wd_disc_t *disc = OpenDiscSF(sf,true,true);
	if (disc)
	{
	    if ( psel != &part_selector )
		wd_select(disc,psel);
	    count = wd_count_used_disc_blocks(disc,1,0);
	}
    }
    return count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////	     high level copy and extract functions	///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CopyImage
(
    SuperFile_t		* fi,		// valid input file
    SuperFile_t		* fo,		// valid output file
    enumOFT		oft,		// oft, if 'OFT_UNKNOWN' it is detected automatically
    int			overwrite,	// overwrite mode
    bool		preserve,	// true: force preserve time
    bool		remove_source	// true: remove source on success
)
{
    DASSERT(fi);
    DASSERT(fo);
    fflush(0);

    if ( oft == OFT_UNKNOWN )
	oft = CalcOFT(output_file_type,opt_dest,fo->f.fname,fo->iod.oft);
    SetupIOD(fo,oft,oft);
    fo->src = fi;
    if (opt_mkdir)
	fo->f.create_directory = true;
    fo->raw_mode = part_selector.whole_disc || !fi->f.id6_dest[0];

    if (*fi->wbfs_id6)
	CopyPatchWbfsId(fo->wbfs_id6,fi->wbfs_id6);

    enumError err = CreateWFile( &fo->f, 0, oft_info[oft].iom, overwrite );
    if ( err || SIGINT_level > 1 )
	goto abort;

    if (opt_split)
	SetupSplitWFile(&fo->f,oft,opt_split_size);

    err = SetupWriteSF(fo,oft);
    if ( err || SIGINT_level > 1 )
	goto abort;

    err = CopySF(fi,fo);
    if ( err || SIGINT_level > 1 )
	goto abort;

    err = RewriteModifiedSF(fi,fo,0,0);
    if ( err || SIGINT_level > 1 )
	goto abort;

#if 0 && defined(TEST)  // [[2do]]

    fo->src = 0;
    if ( fi->disc1 == fi->disc2 )
	preserve = true;

    if (remove_source)
    {
	err = Close2SF(fo,fi,preserve);
	ResetSF(fo,0);
    }
    else
	err = ResetSF( fo, preserve ? &fi->f.fatt : 0 );
    return err;

#else

    noPRINT("COPY-STATUS: %3d : %s() @ %s #%u\n",err,__FUNCTION__,__FILE__,__LINE__);
    fo->src = 0;
    FileAttrib_t fatt;
    memcpy(&fatt,&fi->f.fatt,sizeof(fatt));
    if (remove_source)
	RemoveSF(fi);

    return ResetSF( fo, preserve || fi->disc1 == fi->disc2 ? &fatt : 0 );

#endif

 abort:
    noPRINT("COPY-STATUS: %3d : %s() @ %s #%u\n",err,__FUNCTION__,__FILE__,__LINE__);
    RemoveSF(fo);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyImageName
(
    SuperFile_t		* fi,		// valid input file
    ccp			path1,		// NULL or part 1 of path
    ccp			path2,		// NULL or part 2 of path
    enumOFT		oft,		// oft, if 'OFT_UNKNOWN' it is detected automatically
    int			overwrite,	// overwrite mode
    bool		preserve,	// true: force preserve time
    bool		remove_source	// true: remove source on success
)
{
    DASSERT(fi);

    SuperFile_t fo;
    InitializeSF(&fo);
    char path_buf[PATH_MAX];
    ccp path = PathCatPP(path_buf,sizeof(path_buf),path1,path2);
    fo.f.fname = STRDUP(path);
    enumError err = CopyImage(fi,&fo,oft,overwrite,preserve,remove_source);
    ResetSF(&fo,0);
    return err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError NormalizeExtractPath
(
    char		* dest_dir,	// result: pointer to path buffer
    size_t		dest_dir_size,	// size of 'dest_dir'
    ccp			source_dest,	// source for destination path
    int			overwrite	// overwrite mode
)
{
    DASSERT(dest_dir);
    DASSERT(dest_dir_size > 100 );
    DASSERT(source_dest);

    char * dest = StringCopyS(dest_dir,dest_dir_size-1,source_dest);
    if ( dest == dest_dir || dest[-1] != '/' )
    {
	*dest++ = '/';
	*dest   = 0;
    }

    if ( !overwrite && !opt_flat )
    {
	struct stat st;
	if (!stat(dest_dir,&st))
	    return ERROR0(ERR_ALREADY_EXISTS,"Destination already exists: %s",dest_dir);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ExtractImage
(
    SuperFile_t		* fi,		// valid input file
    ccp			dest_dir,	// destination directory terminated with '/'
    int			overwrite,	// overwrite mode
    bool		preserve	// true: copy time to extracted files
)
{
    DASSERT(fi);
    DASSERT(dest_dir);
    DASSERT( strlen(dest_dir) && dest_dir[strlen(dest_dir)-1] == '/' );

    wd_disc_t * disc = OpenDiscSF(fi,true,true);
    if (!disc)
	return ERR_WDISC_NOT_FOUND;

    const bool copy_image = disc->disc_type == WD_DT_GAMECUBE && opt_copy_gc;

    WiiFst_t fst;
    InitializeFST(&fst);
    CollectFST( &fst, disc, GetDefaultFilePattern(), false,
			copy_image ? 2 : 0, prefix_mode, false );
    // to detect links the files must be sorted by offset (is also fastest)
    SortFST( &fst, opt_links ? SORT_OFFSET : sort_mode, SORT_OFFSET );

    WiiFstInfo_t wfi;
    memset(&wfi,0,sizeof(wfi));
    wfi.sf		= fi;
    wfi.fst		= &fst;
    wfi.set_time	= preserve ? &fi->f.fatt : 0;
    wfi.overwrite	= overwrite;
    wfi.verbose		= long_count > 0 ? long_count : verbose > 0 ? 1 : 0;
    wfi.copy_image	= copy_image;
    wfi.link_image	= copy_image && !opt_no_link;

    enumError err = CreateFST(&wfi,dest_dir);
    ResetWiiParamField(&wfi.align_info);

    if ( !err && wfi.not_created_count )
    {	
	if ( wfi.not_created_count == 1 )
	    err = ERROR0(ERR_CANT_CREATE,
			"1 file or directory not created\n" );
	else
	    err = ERROR0(ERR_CANT_CREATE,
			"%d files and/or directories not created.\n",
			wfi.not_created_count );
    }
    ResetFST(&fst);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			copy functions			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError CopySF ( SuperFile_t * in, SuperFile_t * out )
{
    DASSERT(in);
    DASSERT(out);
    PRINT("---\n");
    PRINT("+++ CopySF(%d->%d) raw=%d+++\n",
		GetFD(&in->f), GetFD(&out->f), out->raw_mode );

    UpdateVersionWDF(in->wdf,out->wdf);

    if ( !out->raw_mode && !part_selector.whole_disc )
    {
	wd_disc_t * disc = OpenDiscSF(in,true,false);
	if (disc)
	{
	    MarkMinSizeSF( out, opt_disc_size ? opt_disc_size : in->file_size );
	    wd_filter_usage_table(disc,wdisc_usage_tab,0);

	    if ( out->iod.oft == OFT_WDF1 || out->iod.oft == OFT_WDF2 )
	    {
		// write an empty disc header => makes detection easier
		enumError err = WriteSF(out,0,zerobuf,WII_HEAD_INFO_SIZE);
		if (err)
		    return err;
	    }

	    int idx;
	    u64 pr_done = 0, pr_total = 0;
	    if ( out->show_progress )
	    {
		for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
		    if (wdisc_usage_tab[idx])
			pr_total++;
		pr_total *= WII_SECTOR_SIZE;
		PrintProgressSF(0,pr_total,out);
	    }

 #if defined(TEST) && 0 // [[2do]] scheint nutzlos zu sein (test mit linux64)
	    const int max_sect = sizeof(iobuf) / WII_SECTOR_SIZE;
	    idx = 0;
	    while ( idx < sizeof(wdisc_usage_tab) )
	    {
		const u8 cur_id = wdisc_usage_tab[idx];
		if (!cur_id)
		{
		    idx++;
		    continue;
		}

		if ( SIGINT_level > 1 )
		    return ERR_INTERRUPT;

		const int idx_end = idx + max_sect < sizeof(wdisc_usage_tab)
				  ? idx + max_sect : sizeof(wdisc_usage_tab);
		const int idx_begin = idx++;
		while ( idx < idx_end && cur_id == wdisc_usage_tab[idx] )
		    idx++;

		noPRINT("COPY: %5x .. %5x / %5x, n=%2x\n",idx_begin,idx,idx_end,idx-idx_begin);

		const off_t off = (off_t)WII_SECTOR_SIZE * idx_begin;
		const size_t size = (size_t)( idx - idx_begin ) * WII_SECTOR_SIZE;
		DASSERT( size <= sizeof(iobuf) );
		enumError err = ReadSF(in,off,iobuf,size);
		if (err)
		    return err;

		err = WriteSparseSF(out,off,iobuf,size);
		if (err)
		    return err;

		if ( out->show_progress )
		{
		    pr_done += size;
		    PrintProgressSF(pr_done,pr_total,out);
		}
	    }
 #else
	    for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
	    {
		const u8 cur_id = wdisc_usage_tab[idx];
		if (cur_id)
		{
		    if ( SIGINT_level > 1 )
			return ERR_INTERRUPT;

		    noPRINT("COPY: %5x\n",idx);

		    off_t off = (off_t)WII_SECTOR_SIZE * idx;
		    enumError err = ReadSF(in,off,iobuf,WII_SECTOR_SIZE);
		    if (err)
			return err;

		    err = WriteSparseSF(out,off,iobuf,WII_SECTOR_SIZE);
		    if (err)
			return err;

		    if ( out->show_progress )
		    {
			pr_done += WII_SECTOR_SIZE;
			PrintProgressSF(pr_done,pr_total,out);
		    }
		}
	    }
 #endif
	    if ( out->show_progress || out->show_summary )
		out->progress_summary = true; // delayed print after closing
	    return ERR_OK;
	}
    }

    switch (in->iod.oft)
    {
	case OFT_WDF1:
	case OFT_WDF2:	return CopyWDF(in,out);
	case OFT_WIA:	return CopyWIA(in,out);
	case OFT_WBFS:	return CopyWBFSDisc(in,out);
	default:	return CopyRaw(in,out);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyRaw ( SuperFile_t * in, SuperFile_t * out )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopyRaw(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    off_t copy_size = in->file_size;
    off_t off       = 0;

    MarkMinSizeSF(out, opt_disc_size ? opt_disc_size : copy_size );

    if ( out->show_progress )
	PrintProgressSF(0,in->file_size,out);

    while ( copy_size > 0 )
    {
	if ( SIGINT_level > 1 )
	    return ERR_INTERRUPT;

	u32 size = sizeof(iobuf) < copy_size ? sizeof(iobuf) : (u32)copy_size;
	enumError err = ReadSF(in,off,iobuf,size);
	if (err)
	    return err;

	err = WriteSparseSF(out,off,iobuf,size);
	if (err)
	    return err;

	copy_size -= size;
	off       += size;
	if ( out->show_progress )
	    PrintProgressSF(off,in->file_size,out);
    }

    if ( out->show_progress || out->show_summary )
	out->progress_summary = true;

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyRawData
(
    SuperFile_t	* in,
    SuperFile_t	* out,
    off_t	off,
    off_t	copy_size
)
{
    ASSERT(in);
    ASSERT(out);
    TRACE("+++ CopyRawData(%d,%d,%llx,%llx) +++\n",
		GetFD(&in->f), GetFD(&out->f), (u64)off, (u64)copy_size );

    while ( copy_size > 0 )
    {
	const u32 size = sizeof(iobuf) < copy_size ? sizeof(iobuf) : (u32)copy_size;
	enumError err = ReadSF(in,off,iobuf,size);
	if (err)
	    return err;

	err = WriteSF(out,off,iobuf,size);
	if (err)
	    return err;

	copy_size -= size;
	off       += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyRawData2
(
    SuperFile_t	* in,
    off_t	in_off,
    SuperFile_t	* out,
    off_t	out_off,
    off_t	copy_size
)
{
    ASSERT(in);
    ASSERT(out);
    TRACE("+++ CopyRawData2(%d,%llx,%d,%llx,%llx) +++\n",
		GetFD(&in->f), (u64)in_off,
		GetFD(&out->f), (u64)out_off, (u64)copy_size );

    while ( copy_size > 0 )
    {
	const u32 size = sizeof(iobuf) < copy_size ? (u32)sizeof(iobuf) : (u32)copy_size;
	enumError err = ReadSF(in,in_off,iobuf,size);
	if (err)
	    return err;

	err = WriteSF(out,out_off,iobuf,size);
	if (err)
	    return err;

	copy_size -= size;
	in_off    += size;
	out_off   += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyWDF ( SuperFile_t * in, SuperFile_t * out )
{
    DASSERT(in);
    DASSERT(out);
    PRINT("---\n");
    PRINT("+++ CopyWDF(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    wdf_controller_t *wdf = in->wdf;
    if (!wdf)
	return ERROR0(ERR_INTERNAL,0);

    UpdateVersionWDF(in->wdf,out->wdf);
    enumError err = MarkMinSizeSF(out, opt_disc_size ? opt_disc_size : in->file_size );
    if (err)
	return err;

    u64 pr_done = 0, pr_total = 0;
    if ( out->show_progress )
    {
	int i;
	for ( i = 0; i < wdf->chunk_used; i++ )
	    pr_total += wdf->chunk[i].data_size;
	PrintProgressSF(0,pr_total,out);
    }

    int i;
    for ( i = 0; i < wdf->chunk_used; i++ )
    {
	wdf2_chunk_t *wc = wdf->chunk + i;
	if ( wc->data_size )
	{
	    u64 dest_off = wc->file_pos;
	    u64 src_off  = wc->data_off;
	    u64 size64   = wc->data_size;

	    while ( size64 > 0 )
	    {
		if ( SIGINT_level > 1 )
		    return ERR_INTERRUPT;

		u32 size = sizeof(iobuf);
		if ( size > size64 )
		    size = (u32)size64;

		TRACE("cp #%02d %09llx .. %07x .. %09llx\n",i,src_off,size,dest_off);

		enumError err = ReadAtF(&in->f,src_off,iobuf,size);
		if (err)
		    return err;

		err = WriteSF(out,dest_off,iobuf,size);
		if (err)
		    return err;

		dest_off	+= size;
		src_off		+= size;
		size64		-= size;

		if ( out->show_progress )
		{
		    pr_done += size;
		    PrintProgressSF(pr_done,pr_total,out);
		}
	    }
	}
    }

    if ( out->show_progress || out->show_summary )
	out->progress_summary = true;

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyWIA ( SuperFile_t * in, SuperFile_t * out )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopyWIA(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    if (!in->wia)
	return ERROR0(ERR_INTERNAL,0);

    // fall back to CopyRaw()
    return CopyRaw(in,out);

 #if 0 // [[2do]] [wia]
    enumError err = MarkMinSizeSF(out, opt_disc_size ? opt_disc_size : in->file_size );
    if (err)
	return err;

    // [[2do]] [wia]
    return ERROR0(ERR_NOT_IMPLEMENTED,"WIA is not supported yet.\n");
 #endif
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyWBFSDisc ( SuperFile_t * in, SuperFile_t * out )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("---\n");
    TRACE("+++ CopyWBFSDisc(%d,%d) +++\n",GetFD(&in->f),GetFD(&out->f));

    if ( !in->wbfs )
	return ERROR0(ERR_INTERNAL,0);

    ASSERT(in->wbfs->disc);
    ASSERT(in->wbfs->disc->header);
    u16 * wlba_tab = in->wbfs->disc->header->wlba_table;
    ASSERT(wlba_tab);

    wbfs_t * w = in->wbfs->wbfs;
    ASSERT(w);

    char * copybuf;
    if ( w->wbfs_sec_sz <= sizeof(iobuf) )
	 copybuf = iobuf;
    else
	copybuf = MALLOC(w->wbfs_sec_sz);

    MarkMinSizeSF(out, opt_disc_size ? opt_disc_size : in->file_size );
    enumError err = ERR_OK;

    u64 pr_done = 0, pr_total = 0;
    if ( out->show_progress )
    {
	int bl;
	for ( bl = 0; bl < w->n_wbfs_sec_per_disc; bl++ )
	    if (ntohs(wlba_tab[bl]))
		pr_total++;
	pr_total *= WII_SECTOR_SIZE;
	PrintProgressSF(0,pr_total,out);
    }

    int bl;
    for ( bl = 0; bl < w->n_wbfs_sec_per_disc; bl++ )
    {
	const u32 wlba = ntohs(wlba_tab[bl]);
	if (wlba)
	{
	    err = ReadAtF( &in->f, (off_t)w->wbfs_sec_sz * wlba,
				copybuf, w->wbfs_sec_sz );
	    if (err)
		goto abort;

	    err = WriteSF( out, (off_t)w->wbfs_sec_sz * bl,
				copybuf, w->wbfs_sec_sz );
	    if (err)
		goto abort;

	    if ( out->show_progress )
	    {
		pr_done += WII_SECTOR_SIZE;
		PrintProgressSF(pr_done,pr_total,out);
	    }
	}
    }

    if ( out->show_progress || out->show_summary )
	out->progress_summary = true;

 abort:
    if ( copybuf != iobuf )
	FREE(copybuf);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendF
	( WFile_t * in, SuperFile_t * out, off_t in_off, size_t count )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("AppendF(%d,%d,%llx,%zx) +++\n",
		GetFD(in), GetFD(&out->f), (u64)in_off, count );

    while ( count > 0 )
    {
	const u32 size = sizeof(iobuf) < count ? sizeof(iobuf) : (u32)count;
	enumError err = ReadAtF(in,in_off,iobuf,size);
	TRACE_HEXDUMP16(3,in_off,iobuf,size<0x10?size:0x10);
	if (err)
	    return err;

	err = WriteSF(out,out->max_virt_off,iobuf,size);
	if (err)
	    return err;

	count  -= size;
	in_off += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendSparseF
	( WFile_t * in, SuperFile_t * out, off_t in_off, size_t count )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("AppendSparseF(%d,%d,%llx,%zx) +++\n",
		GetFD(in), GetFD(&out->f), (u64)in_off, count );

    while ( count > 0 )
    {
	const u32 size = sizeof(iobuf) < count ? sizeof(iobuf) : (u32)count;
	enumError err = ReadAtF(in,in_off,iobuf,size);
	TRACE_HEXDUMP16(3,in_off,iobuf,size<0x10?size:0x10);
	if (err)
	    return err;

	noPRINT(" - %9llx -> %9llx, size=%8x/%9zx\n",
		in_off, out->max_virt_off, size, count );
	//err = WriteSparseSF(out,out->max_virt_off,iobuf,size); // [wdf-cat] [[obsolete]]
	err = WriteSparseSF(out,out->file_size,iobuf,size);
	if (err)
	    return err;

	count  -= size;
	in_off += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendSF
	( SuperFile_t * in, SuperFile_t * out, off_t in_off, size_t count )
{
    ASSERT(in);
    ASSERT(out);
    TRACE("AppendSF(%d,%d,%llx,%zx) +++\n",
		GetFD(&in->f), GetFD(&out->f), (u64)in_off, count );

    while ( count > 0 )
    {
	const u32 size = sizeof(iobuf) < count ? sizeof(iobuf) : (u32)count;
	enumError err = ReadSF(in,in_off,iobuf,size);
	TRACE_HEXDUMP16(3,in_off,iobuf,size<0x10?size:0x10);
	if (err)
	    return err;

	err = WriteSF(out,out->max_virt_off,iobuf,size);
	if (err)
	    return err;

	count  -= size;
	in_off += size;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError AppendZeroSF ( SuperFile_t * out, off_t count )
{
    ASSERT(out);
    TRACE("AppendZeroSF(%d,%llx) +++\n",GetFD(&out->f),(u64)count);

    return WriteSF(out,out->max_virt_off+count,0,0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  diff helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupDiff
(
    // use globals: verbose, print_sections,
    //		    opt_limit, opt_file_limit, opt_block_size

    Diff_t		* diff,		// diff structure to setup
    int			long_count	// relevant long count
)
{
    DASSERT(diff);
    TRACE("DIFF: SetupDiff(%p,%d)\n",diff,long_count);

    memset(diff,0,sizeof(*diff));
    diff->logfile = stdout;
    diff->block_size = opt_block_size <= 0 || opt_block_size > sizeof(diff->data1)
				? opt_block_size
				: sizeof(diff->data1);
    diff->active_block_size = diff->block_size;

    diff->verbose = verbose;
    if ( diff->verbose < 0 )
    {
	diff->mismatch_limit = 1;
	if ( diff->verbose < -1 )
	    long_count = 0;
    }
    else
    {
	diff->mismatch_limit = opt_limit >= 0
				? opt_limit
				: long_count < 3
					? 1
					: long_count < 4
						? 10
						: 0;

	if ( long_count <= 0 )
	    long_count = diff->mismatch_limit > 1;
    }
    diff->file_differ_limit = opt_file_limit > 0 ? opt_file_limit : 0;
    diff->info_level = diff->verbose < 0 ? 0 : long_count;

    noPRINT("DIFF: v=%d, i=%d, l=%d,%d,%d, bs=%d,%d\n",
	diff->verbose, diff->info_level,
	diff->source_differ_limit, diff->file_differ_limit, diff->mismatch_limit,
	diff->active_block_size, diff->active_block_size );
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseDiff
(
    // close files and free dynamic data structures

    Diff_t		* diff		// NULL or diff structure to reset
)
{
    enumError err = ERR_OK;

    if (diff)
    {
	if (diff->patch)
	{
	    err = CloseWritePatch(diff->patch);
	    FREE(diff->patch);
	    diff->patch = 0;
	}
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

bool OpenDiffSource
(
    Diff_t		* diff,		// valid diff structure
    SuperFile_t		* f1,		// first file
    SuperFile_t		* f2,		// second file
    bool		diff_iso	// true: diff iso images
)
{
    const bool stat = CloseDiffSource(diff,true);
    TRACE("DIFF: OpenDiffSource(%p,%p,%p,%d)\n",diff,f1,f2,diff_iso);

    f1->progress_verb = f2->progress_verb = "compared";
    diff->f1 = f1;
    diff->f2 = f2;

    diff->source_differ	= false;
    diff->diff_iso	= diff_iso;
    diff->active_block_size = diff_iso && diff->block_size < 0
				? WII_SECTOR_SIZE
				: diff->block_size;

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool CloseDiffSource
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    bool		silent		// true: suppress printing status messages
)
{
    TRACE("DIFF: CloseDiffSource(%p,%d) -> %d,%d,%d\n",
		diff, silent,
		diff->source_differ_count,
		diff->file_differ_count,
		diff->mismatch_count );
    DASSERT(diff);

 #if HAVE_PRINT
    bool print_stat = false;
 #endif

    CloseDiffFile(diff,silent);
    if (diff->file_count)
    {
	diff->file_count = 0;
	diff->source_count++;
     #if HAVE_PRINT
	print_stat = true;
     #endif
    }

    if ( diff->file_differ_count || diff->file_differ )
    {
	diff->file_differ = false;
	diff->file_differ_count = 0;
	diff->source_differ_count++;
	diff->source_differ = true;

	if ( !silent
		&& diff->verbose >= -1
		//&& !diff->info_level
		&& diff->f1
		&& diff->f2 )
	{
	    // [[2do]] [diff] print only if differ || verbose >= 0
	    ClearProgressLineSF(diff->f2);
	    if (diff->diff_iso)
		fprintf(diff->logfile,
			"! ISOs differ: %s:%s : %s:%s\n",
			oft_info[diff->f1->iod.oft].name, diff->f1->f.fname,
			oft_info[diff->f2->iod.oft].name, diff->f2->f.fname );
	    else
		fprintf(diff->logfile,
			"! Files differ: %s : %s\n",
			diff->f1->f.fname, diff->f2->f.fname );
	}
    }
    else if ( !silent && diff->verbose > 0 && diff->f1 && diff->f2 )
    {
	if (diff->diff_iso)
	    fprintf(diff->logfile,
		    "* ISOs identical: %s:%s : %s:%s\n",
		    oft_info[diff->f1->iod.oft].name, diff->f1->f.fname,
		    oft_info[diff->f2->iod.oft].name, diff->f2->f.fname );
	else
	    fprintf(diff->logfile, 
		    "* Files identical: %s : %s\n",
		    diff->f1->f.fname, diff->f2->f.fname );
    }

    PRINT_IF(print_stat,
		"DIFF: CLOSE/SRC:  %u:%u/%u | %u:%u/%u, %u:%u\n",
		diff->source_count,
		diff->source_differ_count,
		diff->source_differ_limit,
		diff->file_count,
		diff->file_differ_count,
		diff->file_differ_limit,
		diff->total_file_count,
		diff->total_file_differ_count );

    if ( diff->f2 && ( diff->f2->show_progress || diff->f2->show_summary ))
	PrintSummarySF(diff->f2);

    diff->f1 = diff->f2 = 0;

    return !diff->source_differ_limit
	|| diff->source_differ_count < diff->source_differ_limit;
};

///////////////////////////////////////////////////////////////////////////////

bool OpenDiffFile
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    ccp			file_prefix,	// NULL or prefix of file_name
    ccp			file_name	// file name
)
{
    DASSERT(diff);
    DASSERT(file_name);

    const bool stat	= CloseDiffFile(diff,true);
    diff->file_differ	= false;
    diff->file_prefix	= file_prefix ? file_prefix : "";
    diff->file_name	= file_name   ? file_name   : "";
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool CloseDiffFile
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    bool		silent		// true: suppress printing status messages
)
{
    DASSERT(diff);
    DiffData(diff,0,0,0,0,1);

 #if HAVE_PRINT
    bool print_stat = false;
    MARK_USED(print_stat);
 #endif

    if (diff->chunk_count)
    {
	diff->chunk_count = 0;
	diff->file_count++;
     #if HAVE_PRINT
	print_stat = true;
     #endif
    }

    if ( diff->mismatch_count || diff->mismatch_marked )
    {
	diff->mismatch_count = 0;
	diff->mismatch_marked = false;
	diff->file_differ_count++;
	diff->total_file_differ_count++;
	diff->file_differ = true;

	if ( !silent && diff->verbose >= 0 && !diff->info_level && diff->file_name )
	{
	    ClearProgressLineSF(diff->f2);
	    if (print_sections)
		fprintf(diff->logfile,
			"[mismatch:file]\nfile=%s%s\n\n",
			diff->file_prefix, diff->file_name );
	    else
		fprintf(diff->logfile,
			"!   Files differ:     %s%s\n",
			diff->file_prefix, diff->file_name );
	}
    }

    noPRINT_IF(print_stat,
		"DIFF: CLOSE/FILE: %u:%u/%u | %u:%u/%u, %u:%u\n",
		diff->source_count,
		diff->source_differ_count,
		diff->source_differ_limit,
		diff->file_count,
		diff->file_differ_count,
		diff->file_differ_limit,
		diff->total_file_count,
		diff->total_file_differ_count );

    diff->file_prefix = 0;
    diff->file_name = 0;

    return !diff->file_differ_limit
	|| diff->file_differ_count < diff->file_differ_limit;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int get_mismatch_size
(
    Diff_t		* diff,		// valid diff structure
    off_t		data_off,	// offset of 'data*'
    size_t		data_size,	// size of 'data*', NULL for terminatiing
    s64			* blocknum,	// not NULL: store block number
    off_t		* next_offset,	// not NULL: store next offset
    bool		* incomplete	// not NULL: store incomplete status
)
{
    DASSERT(diff);
    bool incompl = data_size < sizeof(diff->data1);
    uint mlen	 = incompl ? data_size : sizeof(diff->data1);

    if ( diff->active_block_size < sizeof(diff->data1) )
    {
	// no blocks
	if (blocknum)
	    *blocknum = -1;
	if (next_offset)
	    *next_offset = data_off + mlen;
    }
    else
    {
	const off_t block = data_off / diff->active_block_size;
	const off_t next_off = ( block + 1 ) * diff->active_block_size;
	const off_t max_len = next_off - data_off;
	if ( mlen > max_len )
	{
	    mlen = max_len;
	    incompl = false;
	}

	if (blocknum)
	    *blocknum = block;
	if (next_offset)
	    *next_offset = incompl ? data_off + mlen : next_off;

	noPRINT("get_mismatch_size(off=%llx, siz=%zx) incompl=%d, next=%llx\n",
		(u64)data_off, data_size,
		incompl, (u64)(incompl ? data_off + mlen : next_off) );
    }

    if (incomplete)
	*incomplete = incompl;

    return mlen;
}

///////////////////////////////////////////////////////////////////////////////

bool DiffData
(
    // returns true on *non* abort

    Diff_t		* diff,		// valid diff structure
    off_t		data_off,	// offset of 'data*'
    size_t		data_size,	// size of 'data*', NULL for terminatiing
    ccp			data1,		// first data
    ccp			data2,		// second data
    int			mode		// 0:in file, 1:term, 2:complete
)
{
    DASSERT(diff);
    DASSERT( diff->f1 || !data_size && !diff->data_size );
    DASSERT( diff->f2 || !data_size && !diff->data_size );

    diff->next_off = data_off + data_size;
    if (data_size)
	diff->chunk_count++;


    //--- check cached data

    if (diff->data_size)
    {
	DASSERT( diff->data_size < sizeof(diff->data1) );

	uint total_len = diff->data_size;
	if ( diff->data_off + total_len == data_off )
	{
	    uint add_len = sizeof(diff->data1) - diff->data_size;
	    if ( add_len > data_size )
		 add_len = data_size;
	    memcpy( diff->data1 + diff->data_size, data1, add_len );
	    memcpy( diff->data2 + diff->data_size, data2, add_len );

	    bool incomplete;
	    int mis_len = get_mismatch_size ( diff, diff->data_off,
					total_len+add_len, 0, 0, &incomplete );

	    if ( !mode && incomplete )
	    {
		diff->data_size = total_len + add_len;
		return true;
	    }

	    total_len  = mis_len;
	    mis_len   -= diff->data_size;
	    data1     += mis_len;
	    data2     += mis_len;
	    data_off  += mis_len;
	    data_size -= mis_len;
	}

	diff->data_size = 0;
	diff->chunk_count--; // this is not a chunk!
	if (!DiffData(diff,diff->data_off,total_len,diff->data1,diff->data2,2))
	    return false;
    }

    noPRINT_IF(data_size, "DIFF: DiffData(%p,%llx+%zx,%p,%p,%d)\n",
		diff, (u64)data_off, data_size, data1, data2, mode );


    //--- fast diff data

    if ( !data_size || !memcmp(data1,data2,data_size) )
	return true;

    if (!diff->info_level)
    {
	diff->mismatch_count++;
	diff->total_mismatch_count++;
	return false;
    }


    //--- data differ -> fine test

    ccp src2 = data2;
    ccp src1 = data1;
    ccp end1 = data1 + data_size;
    while ( src1 < end1
	&& ( !diff->mismatch_limit || diff->mismatch_count < diff->mismatch_limit ))
    {
	while ( src1 < end1 && *src1 == *src2 )
	    src1++, src2++;
	if ( src1 == end1 )
	    break;

	s64 block;
	off_t next_off;
	bool incomplete;
	const off_t doff = data_off + (src1-data1);
	int mis_len = get_mismatch_size(diff,doff,end1-src1,
					&block,&next_off,&incomplete);

	if ( !mode && incomplete )
	{
	    const int dlen = end1 - src1;
	    if ( dlen < sizeof(diff->data1) )
	    {
		memcpy(diff->data1,src1,dlen);
		memcpy(diff->data2,src2,dlen);
		diff->data_size = dlen;
		diff->data_off = doff;
		break;
	    }
	}

	ClearProgressLineSF(diff->f2);
	diff->mismatch_count++;
	diff->total_mismatch_count++;

	if (print_sections)
	{
	    fprintf(diff->logfile,
		   "[mismatch:data]\n"
		   "file=%s%s\n"
		   "offset=0x%llx\n"
		   "block=%lld\n"
		   "block-size=%u\n"
		   ,diff->file_prefix ? diff->file_prefix : ""
		   ,diff->file_name ? diff->file_name : ""
		   ,(u64)doff
		   ,block
		   ,diff->active_block_size > 0 ? diff->active_block_size : 0
		   );
	}
	else
	{
	    fprintf(diff->logfile,"!   Differ at 0x%llx",(u64)doff);
	    if ( block >= 0 )
		fprintf(diff->logfile," [block %llu]",block);
	    if (diff->file_name)
		fprintf(diff->logfile,": %s%s\n",diff->file_prefix,diff->file_name);
	    else
		putchar('\n');
	}

	if ( diff->next_off < next_off )
	     diff->next_off = next_off;

	if ( diff->info_level > 1 || print_sections )
	{
	    mis_len--;
	    while ( mis_len >= 0 && src1[mis_len] == src2[mis_len] )
		mis_len--;
	    mis_len++;
	    fputs( print_sections ? "hexdump1=" : "!     file 1:",diff->logfile);
	    HexDump(diff->logfile,0,-1,0,sizeof(diff->data1),src1,mis_len);
	    fputs( print_sections ? "hexdump2=" : "!     file 2:",diff->logfile);
	    HexDump(diff->logfile,0,-1,0,sizeof(diff->data1),src2,mis_len);
	    if (print_sections)
		putchar('\n');
	}

	if ( mode > 1 )
	    break;

	const u32 delta = next_off - data_off;
	src1 = data1 + delta;
	src2 = data2 + delta;
    }

    return !diff->mismatch_limit || diff->mismatch_count < diff->mismatch_limit;
}

///////////////////////////////////////////////////////////////////////////////

static int DiffFile
(
    Diff_t		* diff		// valid diff structure
)
{
    diff->total_mismatch_count++;
    diff->file_differ_count++;
    diff->total_file_differ_count++;

    return verbose < 0
	? -1
	: !diff->file_differ_limit || diff->file_differ_count < diff->file_differ_limit;
}

///////////////////////////////////////////////////////////////////////////////

static int DiffOnlyPart
(
    Diff_t		* diff,		// valid diff structure
    int			iso_index,	// 1 | 2
    WiiFstPart_t	* part		// partition for reference
)
{
    DASSERT(diff);
    DASSERT(part);
    CloseDiffFile(diff,true);

    if ( verbose >= 0 )
    {
	ClearProgressLineSF(diff->f2);

	const wd_part_t * wp = part->part;
	DASSERT(wp);

	if (print_sections)
	    fprintf(diff->logfile,
		   "[only-in:partition]\n"
		   "image=%u\n"
		   "part-index=%u\n"
		   "part-type=0x%x\n"
		   "part-name=%s\n"
		   "\n"
		   ,iso_index
		   ,wp->index
		   ,wp->part_type
		   ,wd_get_part_name(wp->part_type,"")
		   );
	else
	    fprintf(diff->logfile,
			"!   Only in image #%u: Partition #%u, type=%x %s\n",
			iso_index, wp->index,
			wp->part_type, wd_get_part_name(wp->part_type,"") );
    }

    return DiffFile(diff);
}

///////////////////////////////////////////////////////////////////////////////

static int DiffOnlyFile
(
    Diff_t		* diff,		// valid diff structure
    int			iso_index,	// 1 | 2
    WiiFstPart_t	* part1,	// first partition for reference
    WiiFstPart_t	* part2,	// second partition for reference
    ccp			path1,		// first part of file name, never NULL
    ccp			path2		// second part of file name, never NULL
)
{
    DASSERT(diff);
    DASSERT(part1);
    DASSERT(part2);
    DASSERT(path1);
    DASSERT(path2);

    CloseDiffFile(diff,true);

    if ( verbose >= 0 )
    {
	ClearProgressLineSF(diff->f2);

	const wd_part_t * wp1 = part1->part;
	const wd_part_t * wp2 = part2->part;
	DASSERT(wp1);
	DASSERT(wp2);

	if (print_sections)
	    fprintf(diff->logfile,
		   "[only-in:file]\n"
		   "image=%u\n"
		   "part-1-index=%u\n"
		   "part-1-type=0x%x\n"
		   "part-1-name=%s\n"
		   "part-2-index=%u\n"
		   "part-2-type=0x%x\n"
		   "part-2-name=%s\n"
		   "file-name=%s%s\n"
		   "\n"
		   ,iso_index
		   ,wp1->index
		   ,wp1->part_type
		   ,wd_get_part_name(wp1->part_type,"")
		   ,wp2->index
		   ,wp2->part_type
		   ,wd_get_part_name(wp2->part_type,"")
		   ,path1,path2
		   );
	else
	    fprintf(diff->logfile,
			"!   Only in image #%u: %s%s\n", iso_index, path1, path2 );
    }

    return DiffFile(diff);
}

///////////////////////////////////////////////////////////////////////////////

static bool DiffCheckSize
(
    Diff_t		* diff,		// valid diff structure
    u64			size1,		// size of first file
    u64			size2		// size of second file
)
{
    DASSERT(diff);

    if ( size1 == size2 )
	return true;

    diff->mismatch_count++;
    diff->total_mismatch_count++;

    if ( diff->info_level < 0 || !diff->info_level && !diff->file_name )
	return false;

    ClearProgressLineSF(diff->f2);

    if (print_sections)
    {
	if ( diff->file_name )
	    fprintf(diff->logfile,
		"[mismatch:file-size]\nfile=%s%s\n",
		diff->file_prefix, diff->file_name );
	else
	    fprintf(diff->logfile,"[mismatch:size]\n");

	fprintf(diff->logfile,
		"size1=%llu\n"
		"size2=%llu\n"
		"\n"
		,size1
		,size2
		);
    }
    else
    {
	fputs("!   File size differ: ",diff->logfile);
	ccp postfix = "";
	if (diff->file_name)
	{
	    fputs(diff->file_prefix,diff->logfile);
	    fputs(diff->file_name,diff->logfile);
	    fputs(" [",diff->logfile);
	    postfix = "]";
	}
	fprintf(diff->logfile,
		"%llu%+lld=%llu%s\n", size1, size2-size1, size2, postfix );
    }

    return !diff->mismatch_limit || diff->mismatch_count < diff->mismatch_limit;
}

///////////////////////////////////////////////////////////////////////////////

bool DiffMarkMissmatch
(
    // returns true on *non* abort

    Diff_t		* diff		// valid diff structure
)
{
    DASSERT(diff);
    diff->mismatch_marked = true;
    if ( diff->info_level <= 0 )
	return false;
    ClearProgressLineSF(diff->f2);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

enumError GetDiffStatus
(
    Diff_t		* diff		// valid diff structure
)
{
    DASSERT(diff);
    return diff->source_differ || diff->file_differ ? ERR_DIFFER : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  DiffSF()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DiffSF
(
    Diff_t		* diff,		// valid diff structure
    bool		force_raw_mode	// true: force raw mode
)
{
    TRACE("DIFF: DiffSF(%p,%d)\n",diff,force_raw_mode);
    ASSERT(diff);

    SuperFile_t *f1 = diff->f1;
    SuperFile_t *f2 = diff->f2;
    DASSERT(f1);
    DASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);

    if ( force_raw_mode || part_selector.whole_disc )
	return DiffRawSF(diff);

    f1->progress_verb = f2->progress_verb = "compared";

    wd_disc_t * disc1 = OpenDiscSF(f1,true,true);
    wd_disc_t * disc2 = OpenDiscSF(f2,true,true);
    if ( !disc1 || !disc2 )
	return ERR_WDISC_NOT_FOUND;

    diff->diff_iso = true;
    diff->active_block_size = diff->block_size < 0 ? WII_SECTOR_SIZE : diff->block_size;

    wd_filter_usage_table(disc1,wdisc_usage_tab,0);
    wd_filter_usage_table(disc2,wdisc_usage_tab2,0);

    int idx;
    u64 pr_done = 0, pr_total = 0;

    const bool have_mod_list = f1->modified_list.used || f2->modified_list.used;

    for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
    {
	if (   wd_usage_class_tab[wdisc_usage_tab [idx]]
	    != wd_usage_class_tab[wdisc_usage_tab2[idx]] )
	{
	    if (!DiffMarkMissmatch(diff))
		goto abort;
	    wdisc_usage_tab[idx] = 1; // mark for future
	}

	if ( wdisc_usage_tab[idx] )
	{
	    pr_total++;

	    if (have_mod_list)
	    {
		// mark blocks for delayed comparing
		wdisc_usage_tab2[idx] = 0;
		off_t off = (off_t)WII_SECTOR_SIZE * idx;
		if (   FindMemMap(&f1->modified_list,off,WII_SECTOR_SIZE)
		    || FindMemMap(&f2->modified_list,off,WII_SECTOR_SIZE) )
		{
		    TRACE("DIFF BLOCK #%u (off=%llx) delayed.\n",idx,(u64)off);
		    wdisc_usage_tab[idx] = 0;
		    wdisc_usage_tab2[idx] = 1;
		}
	    }
	}
    }

    if ( f2->show_progress )
    {
	pr_total *= WII_SECTOR_SIZE;
	PrintProgressSF(0,pr_total,f2);
    }

    ASSERT( sizeof(iobuf) >= 2*WII_SECTOR_SIZE );
    char *iobuf1 = iobuf, *iobuf2 = iobuf + WII_SECTOR_SIZE;

    const int ptab_index1 = wd_get_ptab_sector(disc1);
    const int ptab_index2 = wd_get_ptab_sector(disc2);

    int run = 0;
    for(;;)
    {
	TRACE("DIFF: LOOP, run=%d, have_mod_list=%d\n",run,have_mod_list);
	int next_idx = 0;
	for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
	{
	    if ( SIGINT_level > 1 )
		return ERR_INTERRUPT;

	    if ( ( idx >= next_idx || have_mod_list ) && wdisc_usage_tab[idx] )
	    {
		off_t off = (off_t)WII_SECTOR_SIZE * idx;
		TRACE(" - DIFF BLOCK #%u (off=%llx).\n",idx,(u64)off);
		enumError err = ReadSF(f1,off,iobuf1,WII_SECTOR_SIZE);
		if (err)
		    return err;

		err = ReadSF(f2,off,iobuf2,WII_SECTOR_SIZE);
		if (err)
		    return err;

		if ( idx >= next_idx )
		{
		    if ( idx == ptab_index1 )
			wd_patch_ptab(disc1,iobuf1,true);

		    if ( idx == ptab_index2 )
			wd_patch_ptab(disc2,iobuf2,true);

		    if (!DiffData(diff,off,WII_SECTOR_SIZE,iobuf1,iobuf2,0))
			break;

		    next_idx = diff->next_off / WII_SECTOR_SIZE;
		}

		if ( f2->show_progress )
		{
		    pr_done += WII_SECTOR_SIZE;
		    PrintProgressSF(pr_done,pr_total,f2);
		}
	    }
	}
	if ( !have_mod_list || run++ )
	    break;

	memcpy(wdisc_usage_tab,wdisc_usage_tab2,sizeof(wdisc_usage_tab));
	UpdateSignatureFST(f1->fst); // NULL allowed
	UpdateSignatureFST(f2->fst); // NULL allowed
    }

 abort:
    CloseDiffSource(diff,print_sections>0);
    return GetDiffStatus(diff);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  DiffRawSF()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DiffRawSF
(
    Diff_t		* diff		// valid diff structure
)
{
    DASSERT(diff);

    SuperFile_t *f1 = diff->f1;
    SuperFile_t *f2 = diff->f2;
    DASSERT(f1);
    DASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);

    off_t max_off;
    const attribOFT attrib1 = oft_info[f1->iod.oft].attrib;
    const attribOFT attrib2 = oft_info[f2->iod.oft].attrib;
    if ( (attrib1|attrib2) & OFT_A_NOSIZE )
    {
	if ( attrib1 & OFT_A_NOSIZE )
	    f2->f.read_behind_eof = 2;
	if ( attrib2 & OFT_A_NOSIZE )
	    f1->f.read_behind_eof = 2;
	max_off = f1->file_size > f2->file_size
		? f1->file_size : f2->file_size;
    }
    else
    {
	if (!DiffCheckSize(diff,f1->file_size,f2->file_size))
	    goto abort;

	max_off = f1->file_size < f2->file_size
		? f1->file_size : f2->file_size;
    }

    const size_t io_size = sizeof(iobuf)/2;
    ASSERT( (io_size&511) == 0 );
    char *iobuf1 = iobuf, *iobuf2 = iobuf + io_size;

    if ( f2->show_progress )
	PrintProgressSF(0,max_off,f2);

    off_t off = 0;
    for(;;)
    {
	if (SIGINT_level>1)
	    return ERR_INTERRUPT;

	off = UnionDataBlockSF(f1,f2,off,HD_BLOCK_SIZE,0);
	if ( off >= max_off )
	    break;

	const off_t  max_size = max_off - off;
	const size_t cmp_size = io_size < max_size ? io_size : (size_t)max_size;

	enumError err;
	err = ReadSF(f1,off,iobuf1,cmp_size);
	if (err)
	    return err;
	err = ReadSF(f2,off,iobuf2,cmp_size);
	if (err)
	    return err;

	if (!DiffData(diff,off,cmp_size,iobuf1,iobuf2,0))
	    break;

	if ( f2->show_progress )
	    PrintProgressSF(off,max_off,f2);
	off = diff->next_off;
    }

abort:
    CloseDiffSource(diff,print_sections>0);
    return GetDiffStatus(diff);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  DiffFilesSF()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DiffFilesSF
(
    Diff_t		* diff,		// valid diff structure
    struct FilePattern_t * pat,		// NULL or file pattern
    wd_ipm_t		pmode		// prefix mode
)
{
    DASSERT(diff);

    SuperFile_t *f1 = diff->f1;
    SuperFile_t *f2 = diff->f2;
    DASSERT(f1);
    DASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);


    //----- setup fst

    wd_disc_t * disc1 = OpenDiscSF(f1,true,true);
    wd_disc_t * disc2 = OpenDiscSF(f2,true,true);
    if ( !disc1 || !disc2 )
	return ERR_WDISC_NOT_FOUND;

    WiiFst_t fst1, fst2;
    InitializeFST(&fst1);
    InitializeFST(&fst2);

    CollectFST(&fst1,disc1,GetDefaultFilePattern(),false,0,pmode,false);
    CollectFST(&fst2,disc2,GetDefaultFilePattern(),false,0,pmode,false);

    SortFST(&fst1,SORT_NAME,SORT_NAME);
    SortFST(&fst2,SORT_NAME,SORT_NAME);


    //----- define variables

    enumError err = ERR_OK;

    const int BUF_SIZE = sizeof(iobuf) / 2;
    char * buf1 = iobuf;
    char * buf2 = iobuf + BUF_SIZE;

    u64 done_count = 0, total_count = 0;
    f1->progress_verb = f2->progress_verb = "compared";


    //----- first find solo partions

    int ip1, ip2;
    for ( ip1 = 0; ip1 < fst1.part_used; ip1++ )
    {
	WiiFstPart_t * p1 = fst1.part + ip1;

	// find a partition with equal type in fst2

	WiiFstPart_t * p2 = 0;
	for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
	{
	    WiiFstPart_t * p = fst2.part + ip2;
	    if ( !p->done && p->part_type == p1->part_type )
	    {
		p2 = p;
		p2->done++;
		break;
	    }
	}

	if (!p2)
	{
	    if ( DiffOnlyPart(diff,1,p1) <= 0 )
		goto abort_source;
	    continue;
	}

	ASSERT(p1);
	ASSERT(p2);
	total_count += p1->file_used + p2->file_used;
    }

    for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
    {
	WiiFstPart_t * p2 = fst2.part + ip2;
	if (!p2->done)
	{
	    if ( DiffOnlyPart(diff,2,p2) <= 0 )
		goto abort_source;
	}
	// reset done flag
	p2->done = 0;
    }


    //----- compare files

    if ( f2->show_progress )
	PrintProgressSF(0,total_count,f2);

    for ( ip1 = 0; ip1 < fst1.part_used; ip1++ )
    {
	WiiFstPart_t * p1 = fst1.part + ip1;
	if (p1->done++)
	    continue;

	// find a partition with equal type in fst2

	WiiFstPart_t * p2 = 0;
	for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
	{
	    WiiFstPart_t * p = fst2.part + ip2;
	    if ( !p->done && p->part_type == p1->part_type )
	    {
		p2 = p;
		break;
	    }
	}
	if (!p2)
	    continue;
	p2->done++;

	//----- compare files

	ASSERT(p1);
	ASSERT(p2);

	const WiiFstFile_t *file1 = p1->file;
	const WiiFstFile_t *end1  = file1 + p1->file_used;
	const WiiFstFile_t *file2 = p2->file;
	const WiiFstFile_t *end2  = file2 + p2->file_used;

	while ( file1 < end1 && file2 < end2 )
	{
	    if (SIGINT_level>1)
		return ERR_INTERRUPT;

	    if ( f2->show_progress )
		PrintProgressSF(done_count,total_count,f2);

	    int stat = strcmp(file1->path,file2->path);
	    if ( stat < 0 )
	    {
		if ( DiffOnlyFile(diff,1,p1,p2,p1->path,file1->path) <= 0 )
		    goto abort_source;
		file1++;
		done_count++;
		continue;
	    }

	    if ( stat > 0 )
	    {	
		if ( DiffOnlyFile(diff,2,p1,p2,p2->path,file2->path) <= 0 )
		    goto abort_source;
		file2++;
		done_count++;
		continue;
	    }

	    if ( file1->icm != file2->icm )
	    {
		// this should never happen, because '.../file' or '.../dir/'

		const int diff_stat = DiffFile(diff);
		if ( diff_stat >= 0 )
		    fprintf(diff->logfile,
				"!   File types differ: %s%s\n", p1->path, file1->path );
		if ( diff_stat <= 0 )
		    goto abort_source;
	    }
	    else if ( file1->icm > WD_ICM_DIRECTORY )
	    {
		OpenDiffFile(diff,p1->path,file1->path);
		u32 size = file1->size;
		if ( file1->size != file2->size )
		{
		    if (!DiffCheckSize(diff,file1->size,file2->size))
		    {
			CloseDiffFile(diff,true);
			goto abort_file;
		    }
		    if ( size > file2->size )
			 size = file2->size;
		}

		u32 off  = 0;
		while ( size > 0 )
		{
		    const u32 read_size = size < BUF_SIZE ? size : BUF_SIZE;
		    if (   ReadFileFST(p1,file1,off,buf1,read_size)
			|| ReadFileFST(p2,file2,off,buf2,read_size) )
		    {
			err = ERR_READ_FAILED;
			goto abort_source;
		    }

		    if (!DiffData(diff,off,read_size,buf1,buf2,0))
			break;
		    size -= read_size;
		    off  += read_size;
		}
		CloseDiffFile(diff,false);
	    }
	 abort_file:
	    file1++;
	    file2++;
	    done_count += 2;
	}

	while ( file1 < end1 )
	{
	    if ( DiffOnlyFile(diff,1,p1,p2,p1->path,file1->path) <= 0 )
		goto abort_source;
	    file1++;
	}

	while ( file2 < end2 )
	{
	    if ( DiffOnlyFile(diff,2,p1,p2,p2->path,file2->path) <= 0 )
		goto abort_source;
	    file2++;
	}
    }

abort_source:
    CloseDiffSource(diff,print_sections>0);
    ResetFST(&fst1);
    ResetFST(&fst2);
    return err >= ERR_ERROR ? err : GetDiffStatus(diff);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  DiffPatchSF()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DiffPatchSF
(
    Diff_t		* diff,		// valid diff structure
    struct FilePattern_t * pat,		// NULL or file pattern
    wd_ipm_t		pmode		// prefix mode
)
{
    DASSERT(diff);

    //--- disable all limits

    diff->source_differ_limit	= 0;
    diff->file_differ_limit	= 0;
    diff->mismatch_limit	= 0;

    //--- setup patch data

    diff->patch = MALLOC(sizeof(*diff->patch));
    SetupWritePatch(diff->patch);
    enumError err1 = CreateWritePatch(diff->patch,opt_patch_file);
    if (err1)
	return err1;

    //--- diff

    err1 = DiffFilesSF(diff,pat,pmode);

    //--- close patch & return

    enumError err2 = CloseWritePatch(diff->patch);
    FREE(diff->patch);
    diff->patch = 0;

    return err1 > err2 ? err1 : err2;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			old diff functions		///////////////
///////////////////////////////////////////////////////////////////////////////

static void oldPrintDiff ( off_t off, ccp iobuf1, ccp iobuf2, size_t iosize )
{
    while ( iosize > 0 )
    {
	if ( *iobuf1 != *iobuf2 )
	{
	    const size_t bl = off/WII_SECTOR_SIZE;
	    printf("! differ at ISO sector %6zu=0x%05zx, off=0x%09llx\n",
				bl, bl, (u64)off );

	    if (verbose)
	    {
		printf("!"); HexDump(stdout,3,off,9,16,iobuf1,16);
		printf("!"); HexDump(stdout,3,off,9,16,iobuf2,16);
	    }

	    off_t old = off;
	    off = (bl+1) * (off_t)WII_SECTOR_SIZE;
	    const size_t skip = off - old;
	    if ( skip >= iosize )
		break;
	    iobuf1 += skip;
	    iobuf2 += skip;
	    iosize -= skip;
	}
	else
	{
	    off++;
	    iobuf1++;
	    iobuf2++;
	    iosize--;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError oldDiffSF
(
    SuperFile_t		* f1,		// first file
    SuperFile_t		* f2,		// second file
    int			long_count,	// relevant long count
    bool		force_raw_mode	// true: force raw mode
)
{
    ASSERT(f1);
    ASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);

    f1->progress_verb = f2->progress_verb = "compared";

    if ( force_raw_mode || part_selector.whole_disc )
	return oldDiffRawSF(f1,f2,long_count);

    wd_disc_t * disc1 = OpenDiscSF(f1,true,true);
    wd_disc_t * disc2 = OpenDiscSF(f2,true,true);
    if ( !disc1 || !disc2 )
	return ERR_WDISC_NOT_FOUND;

    wd_filter_usage_table(disc1,wdisc_usage_tab,0);
    wd_filter_usage_table(disc2,wdisc_usage_tab2,0);

    int idx;
    u64 pr_done = 0, pr_total = 0;
    bool differ = false;

    const bool have_mod_list = f1->modified_list.used || f2->modified_list.used;

    for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
    {
	if (   wd_usage_class_tab[wdisc_usage_tab [idx]]
	    != wd_usage_class_tab[wdisc_usage_tab2[idx]] )
	{
	    differ = true;
	    if (!long_count)
		goto abort;
	    wdisc_usage_tab[idx] = 1; // mark for future
	}

	if ( wdisc_usage_tab[idx] )
	{
	    pr_total++;

	    if (have_mod_list)
	    {
		// mark blocks for delayed comparing
		wdisc_usage_tab2[idx] = 0;
		off_t off = (off_t)WII_SECTOR_SIZE * idx;
		if (   FindMemMap(&f1->modified_list,off,WII_SECTOR_SIZE)
		    || FindMemMap(&f2->modified_list,off,WII_SECTOR_SIZE) )
		{
		    TRACE("DIFF BLOCK #%u (off=%llx) delayed.\n",idx,(u64)off);
		    wdisc_usage_tab[idx] = 0;
		    wdisc_usage_tab2[idx] = 1;
		}
	    }
	}
    }

    if ( f2->show_progress )
    {
	pr_total *= WII_SECTOR_SIZE;
	PrintProgressSF(0,pr_total,f2);
    }

    ASSERT( sizeof(iobuf) >= 2*WII_SECTOR_SIZE );
    char *iobuf1 = iobuf, *iobuf2 = iobuf + WII_SECTOR_SIZE;

    const int ptab_index1 = wd_get_ptab_sector(disc1);
    const int ptab_index2 = wd_get_ptab_sector(disc2);

    int run = 0;
    for(;;)
    {
	TRACE("** DIFF LOOP, run=%d, have_mod_list=%d\n",run,have_mod_list);
	for ( idx = 0; idx < sizeof(wdisc_usage_tab); idx++ )
	{
	    if ( SIGINT_level > 1 )
		return ERR_INTERRUPT;

	    if (wdisc_usage_tab[idx])
	    {
		off_t off = (off_t)WII_SECTOR_SIZE * idx;
		TRACE(" - DIFF BLOCK #%u (off=%llx).\n",idx,(u64)off);
		enumError err = ReadSF(f1,off,iobuf1,WII_SECTOR_SIZE);
		if (err)
		    return err;

		err = ReadSF(f2,off,iobuf2,WII_SECTOR_SIZE);
		if (err)
		    return err;

		if ( idx == ptab_index1 )
		    wd_patch_ptab(disc1,iobuf1,true);

		if ( idx == ptab_index2 )
		    wd_patch_ptab(disc2,iobuf2,true);

		if (memcmp(iobuf1,iobuf2,WII_SECTOR_SIZE))
		{
		    differ = true;
		    if (long_count)
			oldPrintDiff(off,iobuf1,iobuf2,WII_SECTOR_SIZE);
		    if ( long_count < 2 )
			break;
		}

		if ( f2->show_progress )
		{
		    pr_done += WII_SECTOR_SIZE;
		    PrintProgressSF(pr_done,pr_total,f2);
		}
	    }
	}
	if ( !have_mod_list || run++ )
	    break;

	memcpy(wdisc_usage_tab,wdisc_usage_tab2,sizeof(wdisc_usage_tab));
	UpdateSignatureFST(f1->fst); // NULL allowed
	UpdateSignatureFST(f2->fst); // NULL allowed
    }

    if ( f2->show_progress || f2->show_summary )
	PrintSummarySF(f2);

 abort:

    return differ ? ERR_DIFFER : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError oldDiffRawSF
(
	SuperFile_t * f1,
	SuperFile_t * f2,
	int long_count
)
{
    ASSERT(f1);
    ASSERT(f2);
    ASSERT(IsOpenSF(f1));
    ASSERT(IsOpenSF(f2));
    ASSERT(f1->f.is_reading);
    ASSERT(f2->f.is_reading);

    if ( f1->iod.oft == OFT_WBFS || f2->iod.oft == OFT_WBFS )
    {
	if ( f1->iod.oft == OFT_WBFS )
	    f2->f.read_behind_eof = 2;
	if ( f2->iod.oft == OFT_WBFS )
	    f1->f.read_behind_eof = 2;
    }
    else if ( f1->file_size != f2->file_size )
	return ERR_DIFFER;

    const size_t io_size = sizeof(iobuf)/2;
    ASSERT( (io_size&511) == 0 );
    char *iobuf1 = iobuf, *iobuf2 = iobuf + io_size;

    off_t off = 0;
    off_t total_size = f1->file_size;
    if ( total_size < f2->file_size )
	 total_size = f2->file_size;
    off_t size = total_size;

    if ( f2->show_progress )
	PrintProgressSF(0,total_size,f2);

    bool differ = false;
    while ( size > 0 )
    {
	if (SIGINT_level>1)
	    return ERR_INTERRUPT;

 #if 1
	off_t new_off = UnionDataBlockSF(f1,f2,off,HD_BLOCK_SIZE,0);
	if ( off < new_off )
	{
	    const off_t delta = new_off - off;
	    if ( size <= delta )
		break;
	    size -= delta;
	    off = new_off;
	}
 #endif

	const size_t cmp_size = io_size < size ? io_size : (size_t)size;

	enumError err;
	err = ReadSF(f1,off,iobuf1,cmp_size);
	if (err)
	    return err;
	err = ReadSF(f2,off,iobuf2,cmp_size);
	if (err)
	    return err;

	if (memcmp(iobuf1,iobuf2,cmp_size))
	{
	    differ = true;
	    if (long_count)
		oldPrintDiff(off,iobuf1,iobuf2,cmp_size);
	    if ( long_count < 2 )
		break;
	}

	off  += cmp_size;
	size -= cmp_size;

	if ( f2->show_progress )
	    PrintProgressSF(off,total_size,f2);
    }

    if ( f2->show_progress || f2->show_summary )
	PrintSummarySF(f2);

    return differ ? ERR_DIFFER : ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError oldDiffFilesSF
(
	SuperFile_t	* f1,
	SuperFile_t	* f2,
	int		long_count,
	FilePattern_t	*pat,
	wd_ipm_t	pmode
)
{
    ASSERT(f1);
    ASSERT(f2);
    DASSERT(IsOpenSF(f1));
    DASSERT(IsOpenSF(f2));
    DASSERT(f1->f.is_reading);
    DASSERT(f2->f.is_reading);
    ASSERT(pat);


    //----- setup fst

    wd_disc_t * disc1 = OpenDiscSF(f1,true,true);
    wd_disc_t * disc2 = OpenDiscSF(f2,true,true);
    if ( !disc1 || !disc2 )
	return ERR_WDISC_NOT_FOUND;

    WiiFst_t fst1, fst2;
    InitializeFST(&fst1);
    InitializeFST(&fst2);

    CollectFST(&fst1,disc1,GetDefaultFilePattern(),false,0,pmode,false);
    CollectFST(&fst2,disc2,GetDefaultFilePattern(),false,0,pmode,false);

    SortFST(&fst1,SORT_NAME,SORT_NAME);
    SortFST(&fst2,SORT_NAME,SORT_NAME);


    //----- define variables

    int differ = 0;
    enumError err = ERR_OK;

    const int BUF_SIZE = sizeof(iobuf) / 2;
    u8 * buf1 = (u8*)iobuf;
    u8 * buf2 = buf1 + BUF_SIZE;

    u64 done_count = 0, total_count = 0;
    f1->progress_verb = f2->progress_verb = "compared";


    //----- first find solo partions

    int ip1, ip2;
    for ( ip1 = 0; ip1 < fst1.part_used; ip1++ )
    {
	WiiFstPart_t * p1 = fst1.part + ip1;

	// find a partition with equal type in fst2

	WiiFstPart_t * p2 = 0;
	for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
	{
	    WiiFstPart_t * p = fst2.part + ip2;
	    if ( !p->done && p->part_type == p1->part_type )
	    {
		p2 = p;
		p2->done++;
		break;
	    }
	}

	if (!p2)
	{
	    p1->done++;
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf("Only in ISO #1:   Partition #%u, type=%x %s\n",
		ip1, p1->part_type, wd_get_part_name(p1->part_type,"") );
	    continue;
	}

	ASSERT(p1);
	ASSERT(p2);
	total_count += p1->file_used + p2->file_used;
    }

    for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
    {
	WiiFstPart_t * p2 = fst2.part + ip2;
	if (!p2->done)
	{
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf("Only in ISO #2:   Partition #%u, type=%x %s\n",
		ip2, p2->part_type, wd_get_part_name(p2->part_type,"") );
	}
	// reset done flag
	p2->done = 0;
    }


    //----- compare files

    if ( f2->show_progress )
	PrintProgressSF(0,total_count,f2);

    for ( ip1 = 0; ip1 < fst1.part_used; ip1++ )
    {
	WiiFstPart_t * p1 = fst1.part + ip1;
	if (p1->done++)
	    continue;

	// find a partition with equal type in fst2

	WiiFstPart_t * p2 = 0;
	for ( ip2 = 0; ip2 < fst2.part_used; ip2++ )
	{
	    WiiFstPart_t * p = fst2.part + ip2;
	    if ( !p->done && p->part_type == p1->part_type )
	    {
		p2 = p;
		break;
	    }
	}
	if (!p2)
	    continue;
	p2->done++;

	//----- compare files

	ASSERT(p1);
	ASSERT(p2);

	const WiiFstFile_t *file1 = p1->file;
	const WiiFstFile_t *end1  = file1 + p1->file_used;
	const WiiFstFile_t *file2 = p2->file;
	const WiiFstFile_t *end2  = file2 + p2->file_used;

	static char only_in[] = "Only in ISO #%u:   %s%s\n";

	while ( file1 < end1 && file2 < end2 )
	{
	    if (SIGINT_level>1)
		return ERR_INTERRUPT;

	    if ( f2->show_progress )
		PrintProgressSF(done_count,total_count,f2);

	    int stat = strcmp(file1->path,file2->path);
	    if ( stat < 0 )
	    {	
		differ++;
		if (verbose<0)
		    goto abort;
		printf(only_in,1,p1->path,file1->path);
		file1++;
		done_count++;
		continue;
	    }

	    if ( stat > 0 )
	    {	
		differ++;
		if (verbose<0)
		    goto abort;
		printf(only_in,2,p2->path,file2->path);
		file2++;
		done_count++;
		continue;
	    }

	    if ( file1->icm != file2->icm )
	    {
		// this should never happen
		differ++;
		if (verbose<0)
		    goto abort;
		printf("File types differ: %s%s\n", p1->path, file1->path );
	    }
	    else if ( file1->icm > WD_ICM_DIRECTORY )
	    {
		if ( file1->size != file2->size )
		{
		    differ++;
		    if (verbose<0)
			goto abort;
		    if ( file1->size < file2->size )
			printf("File size differ: %s%s -> %u+%u=%u\n", p1->path, file1->path,
				    file1->size, file2->size - file1->size, file2->size );
		    else
			printf("File size differ: %s%s -> %u-%u=%u\n", p1->path, file1->path,
				    file1->size, file1->size - file2->size, file2->size );
		}
		else
		{
		    u32 off4 = 0;
		    u32 size = file1->size;

		    while ( size > 0 )
		    {
			const u32 read_size = size < BUF_SIZE ? size : BUF_SIZE;
			if (   ReadFileFST4(p1,file1,off4,buf1,read_size)
			    || ReadFileFST4(p2,file2,off4,buf2,read_size) )
			{
			    err = ERR_READ_FAILED;
			    goto abort;
			}

			if (memcmp(buf1,buf2,read_size))
			{
			    differ++;
			    if (verbose<0)
				goto abort;
			    printf("Files differ:     %s%s\n", p1->path, file1->path );
			    break;
			}

			size -= read_size;
			off4 += read_size>>2;
		    }
		}
	    }

	    file1++;
	    file2++;
	    done_count += 2;
	}

	while ( file1 < end1 )
	{
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf(only_in,1,p1->path,file1->path);
	    file1++;
	}

	while ( file2 < end2 )
	{
	    differ++;
	    if (verbose<0)
		goto abort;
	    printf(only_in,2,p2->path,file2->path);
	    file2++;
	}
    }

abort: 

    if ( done_count && ( f2->show_progress || f2->show_summary ) )
	PrintSummarySF(f2);

    ResetFST(&fst1);
    ResetFST(&fst2);

    return err ? err : differ ? ERR_DIFFER : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			source iterator			///////////////
///////////////////////////////////////////////////////////////////////////////

static StringField_t dir_done_list;
static StringField_t file_done_list;
int opt_source_auto = 0;

//-----------------------------------------------------------------------------

void InitializeIterator ( Iterator_t * it, bool allow )
{
    DASSERT(it);
    memset(it,0,sizeof(*it));
    InitializeIdField(&it->source_list);
    it->act_wbfs_disc		= ACT_ALLOW;
    it->show_mode		= opt_show_mode;
    it->progress_enabled	= verbose > 1 || progress;
    it->scan_progress		= scan_progress > 0;

    it->opt_allow = allow;
    if (allow)
    {
	it->act_fst	= opt_allow_fst >= OFFON_AUTO
			? ACT_EXPAND : ACT_IGNORE;

	it->act_nkit	= opt_allow_nkit > OFFON_AUTO
			? ACT_ALLOW : ACT_WARN;
    }
}

//-----------------------------------------------------------------------------

void ResetIterator ( Iterator_t * it )
{
    DASSERT(it);
    ResetIdField(&it->source_list);
    InitializeIterator(it,it->opt_allow);
}

//-----------------------------------------------------------------------------

static void IteratorProgress
	( Iterator_t * it, bool last_message, SuperFile_t *sf )
{
    static bool active = false;
    bool flush = false;

    if ( sf && it->scan_progress )
    {
	if (print_sections)
	{
	    printf(
		"[progress:found]\n"
		"type=%s\n"
		"path=%s\n"
		"size=%llu\n"
		"\n"
		,oft_info[sf->iod.oft].name
		,sf->f.fname
		,(u64)sf->f.st.st_size
		);
	}
	else
	{
	    if (active)
	    {
		active = false;
		printf("%70s\r","");
	    }
	    printf("> %s found: %s:%s\n",
		it->progress_t_file, oft_info[sf->iod.oft].name, sf->f.fname );
	}
	flush = true;
    }

    //--------------------------------------------------

    if ( it->progress_enabled && ( it->num_of_scans || last_message ))
    {
	const u32 sec = GetTimerMSec() / 1000 + 1;
	if ( sec != it->progress_last_sec || last_message )
	{
	    it->progress_last_sec = sec;

	    if (!it->progress_t_file)
		it->progress_t_file = "supported file";
	    if (!it->progress_t_files)
		it->progress_t_files = "supported files";

	    if (print_sections)
	    {
		printf(
		    "[progress:scan%s]\n"
		    "n-scanned=%d\n"
		    "n-directories=%d\n"
		    "n-found=%d\n"
		    "found-name-1=%s\n"
		    "found-name-2=%s\n"
		    "\n"
		    ,last_message ? ":term" : ""
		    ,it->num_of_scans
		    ,it->num_of_dirs
		    ,it->num_of_files
		    ,it->progress_t_file
		    ,it->progress_t_files
		    );
	    }
	    else
	    {
		printf("%u object%s scanned", it->num_of_scans,
				it->num_of_scans == 1 ? "" : "s"  );

		ccp tie = ",", term = "";
		if ( it->num_of_dirs > 0 )
		{
		    printf(", %u director%s", it->num_of_dirs,
				    it->num_of_dirs == 1 ? "y" : "ies" );
		    tie = " and";
		    term = " found";
		}

		if ( it->num_of_files > 0 )
		{
		    printf("%s %u %s", tie, it->num_of_files,
				it->num_of_files == 1
					? it->progress_t_file
					: it->progress_t_files );
		    term = " found";
		}
		printf("%s%s", term, last_message ? ".   \n" : " ... \r" );
		active = !last_message;
	    }
	    flush = true;
	}
    }

    if (flush)
	fflush(stdout);
}

//-----------------------------------------------------------------------------

static enumError SourceIteratorHelper
	( Iterator_t * it, ccp path, bool collect_fnames )
{
    ASSERT(it);
    ASSERT( it->func || collect_fnames );
    TRACE("SourceIteratorHelper(%p,%s,%d) depth=%d/%d\n",
		it, path, collect_fnames, it->depth, it->max_depth );

    if ( collect_fnames && path && *path == '-' && !path[1] )
    {
	TRACE(" - ADD STDIN\n");
	InsertIdField(&it->source_list,0,0,0,path);
	return ERR_OK;
    }

    it->num_of_scans++;

 #ifdef __CYGWIN__
    char goodpath[PATH_MAX];
    NormalizeFilenameCygwin(goodpath,sizeof(goodpath),path);
    path = goodpath;
 #endif

    char buf[PATH_MAX+10];

    SuperFile_t sf;
    InitializeSF(&sf);
    enumError err = ERR_OK;

    //----- directory part

    if ( !stat(path,&sf.f.st) && S_ISDIR(sf.f.st.st_mode) )
    {
	if ( it->act_fst >= ACT_ALLOW )
	{
	    enumFileType ftype = IsFST(path,0);
	    if ( ftype & FT_ID_FST )
	    {
		sf.f.ftype = ftype;
		sf.allow_fst = it->act_fst >= ACT_EXPAND && !collect_fnames;
		goto check_file;
	    }
	}

	if (!it->expand_dir)
	{
	    it->num_of_files++;
	    sf.f.ftype = FT_ID_DIR;
	    goto check_file;
	}

	if ( it->depth >= it->max_depth )
	{
	    ResetSF(&sf,0);
	    return ERR_OK;
	}

	ccp real_path = realpath(path,buf);
	if (!real_path)
	    real_path = path;

	if (InsertStringField(&dir_done_list,real_path,false))
	{
	    it->num_of_dirs++;
	    if (it->progress_enabled)
		IteratorProgress(it,false,0);
	    DIR * dir = opendir(path);
	    if (dir)
	    {
		char * bufend = buf+sizeof(buf);
		char * dest = StringCopyE(buf,bufend-1,path);
		if ( dest > buf && dest[-1] != '/' )
		    *dest++ = '/';

		it->depth++;

		const enumAction act_non_exist	= it->act_non_exist;
		const enumAction act_non_iso	= it->act_non_iso;
		const enumAction act_gc		= it->act_gc;

		it->act_non_exist = it->act_non_iso = ACT_IGNORE;
		if ( it->act_gc == ACT_WARN )
		     it->act_gc = ACT_IGNORE;

		while ( !err && SIGINT_level < 2 && it->num_of_files < job_limit )
		{
		    struct dirent * dent = readdir(dir);
		    if (!dent)
			break;
		    ccp n = dent->d_name;
		    if ( n[0] != '.' )
		    {
			StringCopyE(dest,bufend,dent->d_name);
			err = SourceIteratorHelper(it,buf,collect_fnames);
		    }
		}
		closedir(dir);

		it->act_non_exist = act_non_exist;
		it->act_non_iso   = act_non_iso;
		it->act_gc	  = act_gc;
		it->depth--;
	    }
	}
	ResetSF(&sf,0);
	return err ? err : SIGINT_level ? ERR_INTERRUPT : ERR_OK;
    }

    //----- file part

 check_file:
    sf.f.disable_errors = it->act_non_exist != ACT_WARN;
    sf.f.disable_nkit_errors = it->act_nkit != ACT_WARN;
    err = OpenSF(&sf,path,it->act_non_iso||it->act_wbfs>=ACT_ALLOW,it->open_modify);

    if ( err != ERR_OK && err != ERR_CANT_OPEN )
    {
	if ( it->act_nkit != ACT_WARN && err == ERR_WRONG_FILE_TYPE && sf.f.ftype & FT_M_NKIT )
	{
	    err = ERR_OK;
	    if ( it->act_nkit == ACT_IGNORE )
		goto close;
	}
	else
	{
	 close:
	    ResetSF(&sf,0);
	    return ERR_OK;
	}
    }
    sf.f.disable_errors = sf.f.disable_nkit_errors = false;

    ccp real_path = realpath( sf.f.path ? sf.f.path : sf.f.fname, buf );
    if (!real_path)
	real_path = path;
    it->real_path = real_path;

    if ( !IsOpenSF(&sf)  )
    {
	if ( it->act_non_exist >= ACT_ALLOW )
	{
	    it->num_of_files++;
	    IteratorProgress(it,false,&sf);
	    if (collect_fnames)
	    {
		InsertIdField(&it->source_list,0,0,0,sf.f.fname);
		err = ERR_OK;
	    }
	    else
		err = it->func(&sf,it);

	    ResetSF(&sf,0);
	    return err;
	}
	if ( err != ERR_CANT_OPEN && it->act_non_exist == ACT_WARN )
	    ERROR0(ERR_CANT_OPEN, "Can't open file X: %s\n",path);
	goto abort;
    }
    else if ( it->act_open < ACT_ALLOW
		&& sf.f.st.st_dev == it->open_dev
		&& sf.f.st.st_ino == it->open_ino )
    {
	if ( !it->wbfs || !*sf.f.id6_dest || !ExistsWDisc(it->wbfs,sf.f.id6_dest) )
	{
	    if ( it->act_open == ACT_WARN )
		printf(" - Ignore input=output: %s\n",path);
	    ResetSF(&sf,0);
	    return ERR_OK;
	}
    }
    err = ERR_OK;

    if ( sf.f.ftype & FT_A_WDISC && sf.wbfs && sf.wbfs->disc )
    {
	char buf2[10];
	snprintf(buf2,sizeof(buf2),"/#%u",sf.wbfs->disc->slot);
	StringCat2S(buf,sizeof(buf),it->real_path,buf2);
	it->real_path = real_path = buf;

	if ( it->act_wbfs_disc < ACT_ALLOW )
	{
	    if ( it->act_wbfs_disc == ACT_WARN )
		PrintErrorFT(&sf.f,FT_A_WDISC);
	    goto abort;
	}
    }

// [[2do]] [[ft-id]]
    if ( it->act_wbfs >= ACT_EXPAND
	&& ( sf.f.ftype & (FT_ID_WBFS|FT_A_WDISC) ) == FT_ID_WBFS )
    {
	WBFS_t wbfs;
	InitializeWBFS(&wbfs);
	if (!SetupWBFS(&wbfs,&sf,false,0,false))
	{
	    const int max_disc = wbfs.wbfs->max_disc;
	    int fw, slot;
	    for ( slot = max_disc-1; slot >= 0
			&& !wbfs.wbfs->head->disc_table[slot]; slot-- )
		;

	    char fbuf[PATH_MAX+10];
	    snprintf(fbuf,sizeof(fbuf),"%u%n",slot,&fw);
	    char * dest = StringCopyS(fbuf,sizeof(fbuf)-10,path);
	    *dest++ = '/';

	    if ( collect_fnames )
	    {
		// fast scan of wbfs
		WDiscInfo_t wdisk;
		InitializeWDiscInfo(&wdisk);
		for ( slot = 0; slot < max_disc && it->num_of_files < job_limit; slot++ )
		{
		    if ( !GetWDiscInfoBySlot(&wbfs,&wdisk,slot) && !IsExcluded(wdisk.id6) )
		    {
			snprintf(dest,fbuf+sizeof(fbuf)-dest,"#%0*u",fw,slot);
			const time_t mtime
			    = wbfs_is_inode_info_valid(wbfs.wbfs,&wdisk.dhead.iinfo)
					? ntoh64(wdisk.dhead.iinfo.mtime) : 0;
			InsertIdField(&it->source_list,wdisk.id6,0,mtime,fbuf);
			it->num_of_files++;
		    }
		}
		if (it->progress_enabled)
		    IteratorProgress(it,false,0);

		ResetWDiscInfo(&wdisk);
		ResetWBFS(&wbfs);
		ResetSF(&sf,0);
		return ERR_OK;
	    }

	    // make a copy of the disc table because we want close the wbfs
	    u8 discbuf[512];
	    u8 * disc_table = max_disc <= sizeof(discbuf)
				? discbuf
				: MALLOC(sizeof(discbuf));
	    memcpy(disc_table,wbfs.wbfs->head->disc_table,max_disc);

	    ResetWBFS(&wbfs);
	    ResetSF(&sf,0);

	    for ( slot = 0; !err && slot < max_disc && it->num_of_files < job_limit; slot++ )
	    {
		if (disc_table[slot])
		{
		    snprintf(dest,fbuf+sizeof(fbuf)-dest,"#%0*u",fw,slot);
		    err = SourceIteratorHelper(it,fbuf,collect_fnames);
		}
	    }

	    if ( disc_table != discbuf )
		FREE(disc_table);
	}
	else
	    ResetSF(&sf,0);
	return err ? err : SIGINT_level ? ERR_INTERRUPT : ERR_OK;
    }

// [[2do]] [[ft-id]]
    if ( sf.f.ftype & FT__SPC_MASK )
    {
	const enumAction action = it->act_non_iso > it->act_known
				? it->act_non_iso : it->act_known;
	if ( action < ACT_ALLOW ) 
	{
	    if ( action == ACT_WARN )
		PrintErrorFT(&sf.f,FT_A_ISO);
	    goto abort;
	}
    }
    else if ( sf.f.ftype & FT_ID_GC_ISO )
    {
	if ( it->act_gc < ACT_ALLOW ) 
	{
	    if ( it->act_gc == ACT_WARN )
		PrintErrorFT(&sf.f,FT_A_WII_ISO);
	    goto abort;
	}
    }
    else if (!sf.f.id6_dest[0])
    {
	const enumAction action = sf.f.ftype & FT_ID_WBFS
				? it->act_wbfs : it->act_non_iso;
	if ( action < ACT_ALLOW ) 
	{
	    if ( action == ACT_WARN )
		PrintErrorFT(&sf.f,FT_A_ISO);
	    goto abort;
	}
    }

    if ( InsertStringField(&file_done_list,real_path,false)
	&& ( !sf.f.id6_src[0] || !IsExcluded(sf.f.id6_src) ))
    {
	it->num_of_files++;
	IteratorProgress(it,false,&sf);
	if (collect_fnames)
	{
	    if ( !IsTimeSpecNull(&sf.f.fatt.mtime) && it->newer && sf.f.ftype & FT_ID_FST )
		SetupReadFST(&sf);
	    InsertIdField(&it->source_list,sf.f.id6_src,0,sf.f.fatt.mtime.tv_sec,sf.f.fname);
	    err = ERR_OK;
	}
	else
	    err = it->func(&sf,it);
    }

 abort:
    ResetSF(&sf,0);
    return err ? err : SIGINT_level ? ERR_INTERRUPT : ERR_OK;
}

//-----------------------------------------------------------------------------

static enumError SourceIteratorStarter
	( Iterator_t * it, ccp path, bool collect_fnames )
{
    ASSERT(it);
    const u32 num_of_files = it->num_of_files;
    const u32 num_of_dirs  = it->num_of_dirs;

    const enumError err = SourceIteratorHelper(it,path,collect_fnames);

    if ( it->num_of_dirs > num_of_dirs && it->num_of_files == num_of_files )
	it->num_empty_dirs++;

    return err;
}

//-----------------------------------------------------------------------------

enumError SourceIterator
(
	Iterator_t * it,
	int warning_mode,
	bool current_dir_is_default,
	bool collect_fnames
)
{
    ASSERT(it);
    ASSERT( it->func || collect_fnames );
    TRACE("SourceIterator(%p) func=%p, act=%d,%d,%d\n",
		it, it->func,
		it->act_non_exist, it->act_non_iso, it->act_wbfs );

    if ( current_dir_is_default
		&& !source_list.used
		&& !recurse_list.used
		&& !opt_source_auto )
	AppendStringField(&source_list,".",false);

    if ( it->act_wbfs < it->act_non_iso )
	it->act_wbfs = it->act_non_iso;

    InitializeStringField(&dir_done_list);
    InitializeStringField(&file_done_list);

    ccp *ptr, *end;
    enumError err = ERR_OK;

    if ( opt_source_auto > 0 && !it->auto_processed && it->act_wbfs >= ACT_ALLOW )
    {
	it->auto_processed = true;
	FindWBFSPartitions();

	it->depth = 0;
	it->max_depth = 1;
	for ( ptr = wbfs_part_list.field, end = ptr + wbfs_part_list.used;
	      err == ERR_OK
			&& !SIGINT_level
			&& ptr < end
			&& it->num_of_files < job_limit;
	      ptr++ )
	{
	    if (collect_fnames)
		InsertIdField(&it->source_list,0,0,0,*ptr);
	    else
		err = SourceIteratorStarter(it,*ptr,collect_fnames);
	}
    }

    it->depth = 0;
    it->max_depth = opt_recurse_depth;
    it->expand_dir = true;
    for ( ptr = recurse_list.field, end = ptr + recurse_list.used;
	  err == ERR_OK
		&& !SIGINT_level
		&& ptr < end
		&& it->num_of_files < job_limit;
	  ptr++ )
    {
	err = SourceIteratorStarter(it,*ptr,collect_fnames);
    }

    it->depth = 0;
    it->max_depth = 1;
    it->expand_dir = !opt_no_expand;
    for ( ptr = source_list.field, end = ptr + source_list.used;
	  err == ERR_OK
		&& !SIGINT_level
		&& ptr < end
		&& it->num_of_files < job_limit;
	  ptr++ )
    {
	err = SourceIteratorStarter(it,*ptr,collect_fnames);
    }

    if (it->progress_enabled)
	IteratorProgress(it,true,0);

    ResetStringField(&dir_done_list);
    ResetStringField(&file_done_list);

    return warning_mode > 0
		? SourceIteratorWarning(it,err,warning_mode==1)
		: err;
}

//-----------------------------------------------------------------------------

enumError SourceIteratorCollected
(
    Iterator_t		* it,		// iterator info
    u8			mask,		// not NULL: execute only items
					// with: ( mask & itme->flag ) != 0
    int			warning_mode,	// warning mode if no source found
					// 0:off, 1:only return status, 2:print error
    bool		ignore_err	// false: abort on error > ERR_WARNING 
)
{
    ASSERT(it);
    ASSERT(it->func);
    TRACE("SourceIteratorCollected(%p) count=%d\n",it,it->source_list.used);

    ResetStringField(&file_done_list);

    it->depth		= 0;
    it->max_depth	= 1;
    it->expand_dir	= false;
    it->num_of_files	= 0;
    it->job_count	= 0;

    const bool progress_enabled = it->progress_enabled;
    it->progress_enabled = false;

    enumError max_err = 0;
    int idx;
    for ( idx = 0; idx < it->source_list.used && !SIGINT_level; idx++ )
    {
	it->source_index = idx;
	if ( mask && !( mask & it->source_list.field[idx]->flag ) )
	{
	    it->num_of_files++; // count this ignored file
	    continue;
	}

	it->job_count++;
	const enumError err
	    = SourceIteratorStarter(it,it->source_list.field[idx]->arg,false);
	if ( max_err < err )
	     max_err = err;
	if ( !ignore_err && err > ERR_WARNING )
	    break;
    }

    it->progress_enabled = progress_enabled;

    return warning_mode > 0
		? SourceIteratorWarning(it,max_err,warning_mode==1)
		: max_err;
}

//-----------------------------------------------------------------------------

enumError SourceIteratorWarning ( Iterator_t * it, enumError max_err, bool silent )
{
    if ( it->num_empty_dirs && max_err < ERR_NO_SOURCE_FOUND )
    {
	if (silent)
	    max_err = ERR_NO_SOURCE_FOUND;
	else if ( it->num_empty_dirs > 1 )
	    max_err = ERROR0(ERR_NO_SOURCE_FOUND,
			"%u directories scanned, but no valid source files found.\n",
			it->num_empty_dirs);
	else
	    max_err = ERROR0(ERR_NO_SOURCE_FOUND,
			"One directory scanned, but no valid source files found.\n");
    }

    if ( !it->num_of_files && max_err < ERR_NOTHING_TO_DO )
    {
	if (silent)
	    max_err = ERR_NOTHING_TO_DO;
	else
	    max_err = ERROR0(ERR_NOTHING_TO_DO,"No valid source file found.\n");
    }

    return max_err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

