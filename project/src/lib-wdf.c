
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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "dclib/dclib-debug.h"
#include "libwbfs.h"
#include "lib-sf.h"
#include "wbfs-interface.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////                         data                    ///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
    #define WC_GROW_SIZE  10
#else
    #define WC_GROW_SIZE 500
#endif

///////////////////////////////////////////////////////////////////////////////
// [[NEW_WDF_ALGORITHM]]

#if NEW_FEATURES
  #define NEW_WDF_ALGORITHM 1
#else
  #define NEW_WDF_ALGORITHM 0
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////                      Setup data                 ///////////////
///////////////////////////////////////////////////////////////////////////////
// initialize WH

void InitializeWH ( wdf_header_t * wh )
{
    DASSERT(wh);

    memset(wh,0,sizeof(*wh));
    memcpy(wh->magic,WDF_MAGIC,sizeof(wh->magic));

    SetupOptionsWDF();
    wh->wdf_version  = use_wdf_version;
    wh->align_factor = use_wdf_align;
    FixHeaderWDF(wh,0,true);
}

///////////////////////////////////////////////////////////////////////////////

void InitializeWDF ( wdf_controller_t * wdf )
{
    DASSERT(wdf);
    memset(wdf,0,sizeof(*wdf));
    InitializeWH(&wdf->head);

    wdf->align = use_wdf_align;

    wdf->min_hole_size	= GetChunkSizeWDF(wdf->head.wdf_version)
			+ sizeof(WDF_Hole_t);
    if ( wdf->min_hole_size < opt_wdf_min_holesize )
	 wdf->min_hole_size = opt_wdf_min_holesize;
    if ( wdf->min_hole_size < wdf->align )
	 wdf->min_hole_size = wdf->align;

    wdf->max_data_off = GetHeaderSizeWDF(wdf->head.wdf_version);
    if ( wdf->max_data_off < wdf->align )
	 wdf->max_data_off = wdf->align;
    wdf->min_data_off = wdf->max_data_off;

    PRINT("InitializeWDF(), align=0x%x, min-hole=0x%x, data-off=0x%llx\n",
		wdf->align, wdf->min_hole_size, wdf->max_data_off );
}

///////////////////////////////////////////////////////////////////////////////

void ResetWDF ( wdf_controller_t * wdf )
{
    DASSERT(wdf);
    FREE(wdf->chunk);
    memset(wdf,0,sizeof(*wdf));
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////        convert data to network byte order       ///////////////
///////////////////////////////////////////////////////////////////////////////

#undef CONV32
#undef CONV64
#define CONV32(var) dest->var = htonl(src->var)
#define CONV64(var) dest->var = hton64(src->var)

///////////////////////////////////////////////////////////////////////////////

void ConvertToNetworkWH ( wdf_header_t * dest, const wdf_header_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    // initialize this again before exporting
    memcpy(dest->magic,WDF_MAGIC,sizeof(dest->magic));

    CONV32(wdf_version);
    CONV32(head_size);
    CONV32(align_factor);
    CONV32(wdf_compatible);

    CONV64(file_size);
    CONV64(data_size);

    CONV32(reserved);
    CONV32(chunk_n);
    CONV64(chunk_off);
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToNetworkWC1 ( wdf1_chunk_t * dest, const wdf1_chunk_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    CONV32(ignored_split_file_index);
    CONV64(file_pos);
    CONV64(data_off);
    CONV64(data_size);
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToNetworkWC2 ( wdf2_chunk_t * dest, const wdf2_chunk_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    CONV64(file_pos);
    CONV64(data_off);
    CONV64(data_size);
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToNetworkWC1n
(
    wdf1_chunk_t	*dest,		// dest data, must not overlay 'src'
    const wdf2_chunk_t	*src,		// source data, network byte order
    uint		n_elem		// number of elements to convert
)
{
    DASSERT(dest);
    DASSERT(src);

    for ( ; n_elem-- > 0; src++, dest++ )
    {
	dest->ignored_split_file_index = 0;
	CONV64(file_pos);
	CONV64(data_off);
	CONV64(data_size);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToNetworkWC2n
(
    wdf2_chunk_t	*dest,		// dest data, may overlay 'src'
    const wdf2_chunk_t	*src,		// source data, network byte order
    uint		n_elem		// number of elements to convert
)
{
    DASSERT(dest);
    DASSERT(src);

    for ( ; n_elem-- > 0; src++, dest++ )
    {
	CONV64(file_pos);
	CONV64(data_off);
	CONV64(data_size);
    }
}

///////////////////////////////////////////////////////////////////////////////

// clear defines
#undef CONV32
#undef CONV64

//
///////////////////////////////////////////////////////////////////////////////
///////////////         convert data to host byte order         ///////////////
///////////////////////////////////////////////////////////////////////////////

#undef CONV32
#undef CONV64
#define CONV32(var) dest->var = ntohl(src->var)
#define CONV64(var) dest->var = ntoh64(src->var)

///////////////////////////////////////////////////////////////////////////////

void ConvertToHostWH ( wdf_header_t * dest, const wdf_header_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    memcpy(dest->magic,src->magic,sizeof(dest->magic));

    CONV32(wdf_version);
    CONV32(head_size);
    CONV32(align_factor);
    CONV32(wdf_compatible);

    CONV64(file_size);
    CONV64(data_size);

    CONV32(reserved);
    CONV32(chunk_n);
    CONV64(chunk_off);
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToHostWC1 ( wdf1_chunk_t * dest, const wdf1_chunk_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    CONV32(ignored_split_file_index);
    CONV64(file_pos);
    CONV64(data_off);
    CONV64(data_size);
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToHostWC2 ( wdf2_chunk_t * dest, const wdf2_chunk_t * src )
{
    ASSERT(dest);
    ASSERT(src);

    CONV64(file_pos);
    CONV64(data_off);
    CONV64(data_size);
}

///////////////////////////////////////////////////////////////////////////////

void ConvertToHostWC
(
    wdf2_chunk_t	*dest,		// dest data, may overlay 'src_data'
    const void		*src_data,	// source data, network byte order
    uint		wdf_version,	// related wdf version
    uint		n_elem		// number of elements to convert
)
{
    DASSERT(dest);
    DASSERT(src_data);

    if ( wdf_version == 1 )
    {
	const wdf1_chunk_t *src;
	for ( src = src_data; n_elem-- > 0; src++, dest++ )
	{
	    CONV64(file_pos);
	    CONV64(data_off);
	    CONV64(data_size);
	}
    }
    else if ( ntoh64(0x123456789abcdef0ull) != 0x123456789abcdef0ull )
    {
	const wdf2_chunk_t *src;
	for ( src = src_data; n_elem-- > 0; src++, dest++ )
	{
	    CONV64(file_pos);
	    CONV64(data_off);
	    CONV64(data_size);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

// clear defines
#undef CONV32
#undef CONV64

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     WDF: helpers                ///////////////
///////////////////////////////////////////////////////////////////////////////

bool FixHeaderWDF ( wdf_header_t * wh, const wdf_header_t *wh_src, bool setup )
{
    DASSERT(wh);
    if ( wh_src && wh_src != wh )
	memcpy(wh,wh_src,sizeof(*wh));

    if ( !setup && wh->wdf_version != 1 )
	return false;

    wdf_header_t save;
    memcpy(&save,wh,sizeof(save));

    wh->reserved = 0;
    const u32 head_size = GetHeaderSizeWDF(wh->wdf_version);

    switch (wh->wdf_version)
    {
	case 1:
	    wh->wdf_compatible	= 1;
	    if ( wh->head_size != head_size )
	    {
		// old style header!
		wh->head_size    = head_size;
		wh->align_factor = 0;
	    }
	    break;

	case 2:
	    wh->wdf_compatible  = 2;
	    wh->head_size       = head_size;
	    break;

	default:
	    ERROR0(ERR_INTERNAL,0);
	    break;
    }

    if (wh->align_factor)
    {
	u32 align = 1;
	while (!(wh->align_factor&align))
	    align <<= 1;
	wh->align_factor = align;
    }

    return memcmp(&save,wh,sizeof(save)) != 0;
}

///////////////////////////////////////////////////////////////////////////////

bool UpdateVersionWDF
(
    const wdf_controller_t * wdf_in,	// NULL or input file
    wdf_controller_t	   * wdf_out	// NULL or output file
)
{
    noPRINT("opt=%d, wdf=%d,%d, vers=%d,%d\n",
	opt_wdf_version, wdf_in!=0, wdf_out!=0,
	wdf_in  ? wdf_in->head.wdf_version : -1,
	wdf_out ? wdf_out->head.wdf_version : -1 );

    if ( !opt_wdf_version
	&& wdf_in
	&& wdf_out
	&& wdf_in->head.wdf_version >  wdf_out->head.wdf_version
	&& wdf_in->head.wdf_version <= WDF_MAX_VERSION
    )
    {
	PRINT("CHANGE WDF v%u -> v%u\n",
		wdf_out->head.wdf_version, wdf_in->head.wdf_version );
	wdf_out->head.wdf_version = wdf_in->head.wdf_version;
	FixHeaderWDF(&wdf_out->head,0,true);
	return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

enumOFT ProposeOFT_WDF ( enumOFT src_oft )
{
    if ( !opt_wdf_version && oft_info[src_oft].attrib & OFT_A_WDF )
	return src_oft;

    SetupOptionsWDF();
    return use_wdf_version > 1 ? OFT_WDF2 : OFT_WDF1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// chunk management

static wdf2_chunk_t * FindChunkWDF
(
    wdf_controller_t	*wdf,	// valid controller object
    u64			offset	// wanted offset
)
{
    DASSERT(wdf);

    if ( !wdf || !wdf->chunk_used )
	return 0;

    wdf2_chunk_t *wc;
    if ( wdf->chunk_used < 8 )
	wc = wdf->chunk;
    else
    {
	// use a binary search
	const int n1 = (int)wdf->chunk_used - 1;
	int beg = 0, end = n1;
	while ( beg < end )
	{
	    int idx = (beg+end)/2;
	    wc = wdf->chunk + idx;
	    if ( offset < wc->file_pos )
		end = idx - 1;
	    else if ( idx < n1 && offset >= wc[1].file_pos )
		beg = idx + 1;
	    else
		beg = end = idx;
	}
	wc = wdf->chunk + beg;
    }

    wdf2_chunk_t *end = wdf->chunk + wdf->chunk_used;
    while ( offset >= wc->file_pos + wc->data_size )
	if ( ++wc >= end )
	    return 0;

    noPRINT("CHUNK FOUND, off %llx: %u/%u/%u: %llx %llx %llx\n",
		offset, (int)(wc-wdf->chunk), wdf->chunk_used, wdf->chunk_size,
		wc->file_pos, wc->data_off, wc->data_size );
    return wc;
}

///////////////////////////////////////////////////////////////////////////////

static wdf2_chunk_t * NewChunkWDF
(
    wdf_controller_t	*wdf,		// valid controller object
    int			index,		// >=0: index, where to insert the new chunk
					// <0:  unknown,
    u64			offset,		// wanted offset to insert
    u64			size,		// wanted size of new chunk
    u64			*res_delta	// not NULL: store the delta between 'offset'
					//           and the new chunk offset here
)
{
    DASSERT(wdf);
    DASSERT( index <= wdf->chunk_used );
    DASSERT( wdf->chunk_used <= wdf->chunk_size );


    //--- calc chunk offset and size

    const u64 src_end = offset + size;
    if ( wdf->image_size < src_end )
	 wdf->image_size = src_end;

    u64 delta, ch_file_pos, ch_data_size;
    if ( wdf->align > 1 )
    {
	delta = offset & wdf->align - 1;
	ch_file_pos  = offset - delta;
	ch_data_size = ALIGN64( size+delta, wdf->align );
    }
    else
    {
	delta = 0;
	ch_file_pos  = offset;
	ch_data_size = size;
    }

    const u64 ch_file_end = ch_file_pos + ch_data_size;
    if ( wdf->max_virt_off < ch_file_end )
	 wdf->max_virt_off = ch_file_end;


    //--- can we extend the last chunk?

    if (wdf->chunk_used)
    {
	wdf2_chunk_t *wc = wdf->chunk + wdf->chunk_used - 1;
	noPRINT(" %llx = %llx / %llx < %llx\n",
		wc->data_off + wc->data_size, wdf->max_data_off,
		ch_file_pos, wc->file_pos + wc->data_size + wdf->min_hole_size );

	if ( wc->data_off + wc->data_size == wdf->max_data_off
	    && ch_file_pos >= wc->file_pos
	    && ch_file_pos <  wc->file_pos + wc->data_size + wdf->min_hole_size )
	{
	    wc->data_size = ch_file_end - wc->file_pos;
	    wdf->max_data_off = wc->data_off + wc->data_size;
	    if (res_delta)
		*res_delta = offset - wc->file_pos;

	    noPRINT("#W# GROW CHUNK [idx=%zd/%d/%d] %llx+%llx -> %llx, d=%llx\n",
		wc-wdf->chunk, wdf->chunk_used, wdf->chunk_size,
		wc->data_off, wc->data_size, wc->file_pos, delta );

	    return wc;
	}
    }


    //--- get a correct index

    if ( index < 0 )
    {
	wdf2_chunk_t * wc = FindChunkWDF(wdf,offset);
	index = wc ? wc - wdf->chunk : wdf->chunk_used;

	DASSERT( index >= 0 );
	DASSERT( index <= wdf->chunk_used );
    }


    //--- must we extend the chunk list?

    if ( wdf->chunk_used == wdf->chunk_size )
    {
	// grow the field

	noPRINT("#W# NewChunkWDF() GROW LIST %d ->%d [used=%d]\n",
		wdf->chunk_size, wdf->chunk_size+WC_GROW_SIZE, wdf->chunk_used );

	wdf->chunk_size += WC_GROW_SIZE;
	wdf->chunk = REALLOC(wdf->chunk,wdf->chunk_size*sizeof(*wdf->chunk));
    }
    DASSERT( wdf->chunk_used < wdf->chunk_size );


    //--- insert new chunk

    wdf2_chunk_t *wc = wdf->chunk + index;
    if ( index < wdf->chunk_used )
	memmove( wc+1, wc, (wdf->chunk_used-index)*sizeof(*wc) );
    wdf->chunk_used++;


    //--- assign chunk values

    wc->file_pos  = ch_file_pos;
    wc->data_size = ch_data_size;
    wc->data_off  = wdf->max_data_off;
    wdf->max_data_off += wc->data_size;
    if (res_delta)
	*res_delta = delta;

    noPRINT("#W#  NEW CHUNK [idx=%zd/%d/%d] %llx+%llx -> %llx, d=%llx\n",
		wc-wdf->chunk, wdf->chunk_used, wdf->chunk_size,
		wc->data_off, wc->data_size, wc->file_pos, delta );

    return wc;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                       read WDF                  ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadWDF ( SuperFile_t * sf )
{
    PRINT("#W# SetupReadWDF(%p) wdf=%p wbfs=%p, fp=%p, fd=%d\n",
		sf, sf->wdf, sf->wbfs, sf->f.fp, sf->f.fd );
    DASSERT(sf);
    if ( sf->wdf || sf->wbfs )
	return ERR_OK;

    CleanSF(sf);
    wdf_controller_t *wdf = MALLOC(sizeof(*wdf));
    sf->wdf = wdf;
    InitializeWDF(wdf); // reset data

    if ( sf->f.seek_allowed && sf->f.st.st_size < sizeof(wdf_header_t) )
	return ERR_NO_WDF;

    wdf_header_t wh;
    enumError stat = ReadAtF(&sf->f,0,&wh,sizeof(wh));
    if (stat)
	return stat;
    TRACE("#W#  - header read\n");


    //----- fix old style header (WDF v1)

    ConvertToHostWH(&wh,&wh);
    FixHeaderWDF(&wh,0,false);


    //----- test header

    stat = AnalyzeWH(&sf->f,&wh,true);
    if (stat)
	return stat;


    //----- test chunk table magic

    char magic[WDF_MAGIC_SIZE];
    stat = ReadAtF(&sf->f,wh.chunk_off,magic,sizeof(magic));
    if (stat)
	return stat;

    if (memcmp(magic,WDF_MAGIC,WDF_MAGIC_SIZE))
	return ERROR0(ERR_WDF_INVALID,"Invalid WDF magic: %s\n",sf->f.fname);

    const uint chunk_tab_size = wh.chunk_n * GetChunkSizeWDF(wh.wdf_version);
    wdf2_chunk_t *wc = MALLOC(chunk_tab_size);


    //--- load and check chunc table

    stat = ReadAtF(&sf->f,wh.chunk_off+WDF_MAGIC_SIZE,wc,chunk_tab_size);
    if (stat)
	return stat;
    wdf->filesize_on_open = sf->f.cur_off;
    TRACE("#W#  - chunk table read\n");

    wdf->chunk = wc;
    wdf->chunk_used = wh.chunk_n;
    wdf->chunk_size = chunk_tab_size / sizeof(*wc);
    ConvertToHostWC(wc,wc,wh.wdf_version,wh.chunk_n);

    int idx;
    u32 align_mask = wdf->head.align_factor | wdf->align;
    u64 max_data_off = wdf->head.head_size;
    u64 min_data_off = wdf->min_data_off;
    u64 max_virt_off = 0;
    for ( idx = 0; idx < wh.chunk_n; idx++, wc++ )
    {
	align_mask |= wc->file_pos | wc->data_off | wc->data_size;
	if ( idx && wc->file_pos < wc[-1].file_pos + wc[-1].data_size )
	    return ERROR0(ERR_WDF_INVALID,"WDF chunk table corrupted: %s\n",sf->f.fname);

	if ( !idx || min_data_off > wc->data_off )
	     min_data_off = wc->data_off;

	u64 end_off = wc->data_off + wc->data_size;
	if ( max_data_off < end_off )
	     max_data_off = end_off;

	end_off = wc->file_pos + wc->data_size;
	if ( max_virt_off < end_off )
	     max_virt_off = end_off;
    }

    const u64 chunk_end = wdf->head.chunk_off + WDF_MAGIC_SIZE + chunk_tab_size;
    if ( chunk_end > min_data_off && wh.chunk_off < max_data_off )
	return ERROR0(ERR_WDF_INVALID,
			"Chunk table overlaps with data: %s\n",sf->f.fname);

    wdf->image_size   = wdf->head.file_size;
    wdf->max_virt_off = max_virt_off;
    wdf->max_data_off = wdf->align
		? ALIGN64(max_data_off,wdf->align) : max_data_off;

    if ( wdf->head.align_factor )
    {
	u32 align = 1;
	while (!(align_mask&align))
	    align <<= 1;
	wdf->align = wdf->head.align_factor = align;
    }

    PRINT("#W#  - chunk loop exits [n=%u/%u], max_off=%llx, align=%x\n",
		wdf->chunk_used, wdf->chunk_size,
		wdf->max_data_off, wdf->align );


    //--- last assignments

    memcpy(&wdf->head,&wh,sizeof(wdf->head));
    sf->file_size	= wdf->head.file_size;
    sf->f.max_off	= wdf->head.chunk_off;
    sf->max_virt_off	= wdf->max_virt_off;

    const enumOFT oft_wdf = wdf->head.wdf_version > 1 ? OFT_WDF2 : OFT_WDF1;
    SetupIOD(sf,oft_wdf,oft_wdf);

    TRACE("#W# WDF FOUND!\n");
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadWDF ( SuperFile_t * sf, off_t off, void * buf, size_t count )
{
    DASSERT(sf);
    wdf_controller_t *wdf = sf->wdf;
    if (!wdf)
	ERROR0(ERR_INTERNAL,0);
    DASSERT(wdf->chunk);
    DASSERT(wdf->chunk_used);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# ReadWDF()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    if ( off + count > wdf->head.file_size )
    {
	if (!sf->f.read_behind_eof)
	{
	    if ( !sf->f.disable_errors )
		ERROR0( ERR_READ_FAILED, "Read behind eof [%c,%llx+%zx]: %s\n",
		    sf->f.fp ? 'S' : sf->f.fd != -1 ? 'F' : '-',
		    (u64)off, count, sf->f.fname );
	    return ERR_READ_FAILED;
	}

	const off_t max_read = wdf->head.file_size > off
					? wdf->head.file_size - off
					: 0;
	ASSERT( count > max_read );

	if ( sf->f.read_behind_eof == 1 )
	{
	    sf->f.read_behind_eof = 2;
	    if ( !sf->f.disable_errors )
		ERROR0( ERR_WARNING, "Read behind eof -> zero filled [%c,%llx+%zx]: %s\n",
		    sf->f.fp ? 'S' : sf->f.fd != -1 ? 'F' : '-',
		    (u64)off, count, sf->f.fname );
	}

	size_t fill_count = count - (size_t)max_read;
	count = (size_t)max_read;
	memset((char*)buf+count,0,fill_count);

	if (!count)
	    return ERR_OK;
    }

    // find chunk header
    wdf2_chunk_t * wc = wdf->chunk;
    const int used_m1 = wdf->chunk_used - 1;
    int beg = 0, end = used_m1;
    ASSERT( beg <= end );
    while ( beg < end )
    {
	int idx = (beg+end)/2;
	wc = wdf->chunk + idx;
	if ( off < wc->file_pos )
	    end = idx-1;
	else if ( idx < used_m1 && off >= wc[1].file_pos )
	    beg = idx + 1;
	else
	    beg = end = idx;
    }
    wc = wdf->chunk + beg;

    noTRACE("#W#  - FOUND #%03d: off=%09llx ds=%llx, off=%09llx\n",
	    beg, wc->file_pos, wc->data_size, (u64)off );
    ASSERT( off >= wc->file_pos );
    ASSERT( beg == used_m1 || off < wc[1].file_pos );

    char * dest = buf;
    while ( count > 0 )
    {
	noTRACE("#W# %d/%d count=%zd off=%llx,%llx \n",
		beg, wdf->chunk_used, count, (u64)off, (u64)wc->file_pos );

	if ( off < wc->file_pos )
	{
	    const u64 max_size = wc->file_pos - off;
	    const u32 fill_size = max_size < count ? (u32)max_size : count;
	    TRACE("#W# >FILL %p +%8zx = %p .. %x\n",
		    buf, dest-(ccp)buf, dest, fill_size );
	    memset(dest,0,fill_size);
	    count -= fill_size;
	    off  += fill_size;
	    dest += fill_size;
	    if (!count)
		break;
	}

	if ( off >= wc->file_pos && off < wc->file_pos + wc->data_size )
	{
	    // we want a part of this
	    const u64 delta     = off - wc->file_pos;
	    const u64 max_size  = wc->data_size - delta;
	    const u32 read_size = max_size < count ? (u32)max_size : count;
	    TRACE("#W# >READ %p +%8zx = %p .. %x <- %10llx\n",
		    buf, dest-(ccp)buf, dest, read_size, wc->data_off+delta );
	    int stat = ReadAtF(&sf->f,wc->data_off+delta,dest,read_size);
	    if (stat)
		return stat;
	    count -= read_size;
	    off  += read_size;
	    dest += read_size;
	    if (!count)
		break;
	}

	wc++;
	if ( ++beg >= wdf->chunk_used )
	{
	    TRACE("ERR_WDF_INVALID\n");
	    return ERR_WDF_INVALID;
	}
    }

    TRACE("#W#  - done, dest = %p\n",dest);
    return ERR_OK;
}


///////////////////////////////////////////////////////////////////////////////

off_t DataBlockWDF
	( SuperFile_t * sf, off_t off, size_t hint_align, off_t * block_size )
{
    DASSERT(sf);
    wdf_controller_t *wdf = sf->wdf;
    if (!wdf)
	ERROR0(ERR_INTERNAL,0);

    if ( off >= wdf->head.file_size )
	return DataBlockStandard(sf,off,hint_align,block_size);

    //--- find chunk header
    // [[align]] optimize using FindChunkWDF()

    wdf2_chunk_t * wc = wdf->chunk;
    const int used_m1 = wdf->chunk_used - 1;
    int beg = 0, end = used_m1;
    ASSERT( beg <= end );
    while ( beg < end )
    {
	int idx = (beg+end)/2;
	wc = wdf->chunk + idx;
	if ( off < wc->file_pos )
	    end = idx-1;
	else if ( idx < used_m1 && off >= wc[1].file_pos )
	    beg = idx + 1;
	else
	    beg = end = idx;
    }
    wc = wdf->chunk + beg;

    while ( off >= wc->file_pos + wc->data_size )
    {
	if ( ++beg >= wdf->chunk_used )
	    return DataBlockStandard(sf,off,hint_align,block_size);
	wc++;
    }
    noPRINT("WC: %llx %llx %llx\n",wc->file_pos,wc->data_off,wc->data_size);

    if ( off < wc->file_pos )
	 off = wc->file_pos;


    //--- calc block_size, ignore holes < 4k

    if ( block_size )
    {
	if ( hint_align < HD_BLOCK_SIZE )
	    hint_align = HD_BLOCK_SIZE;

	for (;;)
	{
	    if ( ++beg >= wdf->chunk_used
		    || wc[1].file_pos - (wc->file_pos+wc->data_size) >= hint_align )
		break;
	    wc++;
	}
	*block_size = wc->data_size - ( off - wc->file_pos );
    }


    //--- term

    return off;
}

///////////////////////////////////////////////////////////////////////////////

void FileMapWDF ( SuperFile_t * sf, FileMap_t *fm )
{
    DASSERT(fm);
    DASSERT(!fm->used);

    DASSERT(sf);
    wdf_controller_t *wdf = sf->wdf;
    if (!wdf)
	ERROR0(ERR_INTERNAL,0);
    const wdf2_chunk_t *wc = wdf->chunk;
    const wdf2_chunk_t *end = wc + wdf->chunk_used;
    for ( ; wc < end; wc++ )
	AppendFileMap(fm,wc->file_pos,wc->data_off,wc->data_size);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     write WDF                   ///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteWDF ( SuperFile_t * sf )
{
    PRINT("#W# SetupWriteWDF(%p)\n",sf);
    DASSERT(sf);

    wdf_controller_t *wdf = MALLOC(sizeof(*wdf));
    sf->wdf = wdf;
    InitializeWDF(wdf); // reset data

    if (sf->src)
    {
	u64 size = 0;
	if (sf->raw_mode)
	    size = sf->f.st.st_size;
	else
	{
	    wd_disc_t * disc = OpenDiscSF(sf->src,false,true);
	    if (disc)
		size = wd_count_used_disc_size(disc,1,0);
	}

	if (size)
	{
	    const enumError err = PreallocateF(&sf->f,0,size+wdf->head.head_size);
	    if (err)
	    {
		noPRINT("#W# SetupWriteWDF() returns %d\n",err);
		return err;
	    }
	}
    }

    const enumOFT oft_wdf = wdf->head.wdf_version > 1 ? OFT_WDF2 : OFT_WDF1;
    SetupIOD(sf,oft_wdf,oft_wdf);

    sf->max_virt_off = 0;
    wdf->head.magic[0] = '-'; // write a 'not complete' indicator
    const enumError err = WriteAtF(&sf->f,0,&wdf->head,wdf->head.head_size);

    PRINT("#W# SetupWriteWDF() returns %d\n",err);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError TermWriteWDF ( SuperFile_t * sf )
{
    DASSERT(sf);
    wdf_controller_t *wdf = sf->wdf;
    if (!wdf)
	ERROR0(ERR_INTERNAL,0);
    DASSERT(wdf->chunk);
    PRINT("#W# TermWriteWDF(%p) v%u [%u,%u]\n",
		sf, wdf->head.wdf_version, opt_wdf_version, use_wdf_version );

    if ( opt_wdf_version && opt_wdf_version == use_wdf_version )
    {
	wdf->head.wdf_version = opt_wdf_version;
	FixHeaderWDF(&wdf->head,0,true);
    }

    if ( sf->file_size < wdf->image_size )
    {
	// correction for double layer discs [[2do]]
	sf->file_size = wdf->image_size;
    }
    else if ( sf->file_size > wdf->image_size )
    {
	// insert a last empty chunk
	NewChunkWDF(wdf,wdf->chunk_used,sf->file_size,0,0);
    }

    wdf->head.chunk_n	= wdf->chunk_used;
    wdf->head.file_size	= wdf->image_size;
    wdf->head.data_size	= wdf->max_data_off - wdf->min_data_off;


    //--- calc chunk offset

    u32 chunk_size;
    u64 eof = wdf->max_data_off;
    bool force_set_size = false;

    if ( wdf->head.wdf_version < 2 )
    {
	wdf->head.chunk_off = eof;
	chunk_size = wdf->head.chunk_n * sizeof(wdf1_chunk_t);
	eof += WDF_MAGIC_SIZE + chunk_size;
    }
    else
    {
	chunk_size = wdf->head.chunk_n * sizeof(wdf2_chunk_t);
	u32 chunk_size2 = chunk_size + WDF_MAGIC_SIZE;
	if ( chunk_size2 > wdf->min_data_off - sizeof(wdf_header_t) )
	{
	    wdf->head.chunk_off = eof;
	    eof += chunk_size2;
	}
	else
	{
	    wdf->head.chunk_off = sizeof(wdf_header_t);
	    force_set_size = true;
	}
    }


    //--- write magic & chunk list

    // write the magic behind the data (use header)
    int stat = WriteAtF( &sf->f, wdf->head.chunk_off, WDF_MAGIC, WDF_MAGIC_SIZE );

    // write the chunk table
    if (!stat)
    {
	if ( wdf->head.wdf_version < 2 )
	{
	    wdf1_chunk_t *wc = MALLOC(chunk_size);
	    ConvertToNetworkWC1n(wc,wdf->chunk,wdf->head.chunk_n);
	    stat = WriteF( &sf->f, wc, chunk_size );
	    FREE(wc);
	}
	else
	{
	    ConvertToNetworkWC2n(wdf->chunk,wdf->chunk,wdf->head.chunk_n);
	    stat = WriteF( &sf->f, wdf->chunk, chunk_size );
	}

	if (!stat)
	{
	    // write the header
	    wdf_header_t wh;
	    ConvertToNetworkWH(&wh,&wdf->head);
	    stat = WriteAtF(&sf->f,0,&wh,wdf->head.head_size);
	    if ( !stat && ( eof < wdf->filesize_on_open || force_set_size ) )
		SetSizeF(&sf->f,eof);
	}
    }

    TRACE("#W# TermWriteWDF() returns %d\n",stat);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteWDF ( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    DASSERT(sf);
    wdf_controller_t *wdf = sf->wdf;
    if (!wdf)
	ERROR0(ERR_INTERNAL,0);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# WriteWDF()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx, fs = %llx\n",
		(u64)sf->f.file_off, (u64)sf->f.max_off, (u64)sf->file_size );

    if (!count)
	return ERR_OK;


    //--- append?

    if ( off >= wdf->max_virt_off )
    {
	// SPECIAL CASE:
	//    the current virtual file will be extended
	//    -> no need to find a related chunk

	u64 delta;
	wdf2_chunk_t *wc = NewChunkWDF(wdf,wdf->chunk_used,off,count,&delta);
	DASSERT(wc);
	return WriteAtF(&sf->f,wc->data_off+delta,buf,count);
    }


    //--- search chunk element

    wdf2_chunk_t * wc = FindChunkWDF(wdf,off);
    if (!wc)
	ERROR0(ERR_INTERNAL,0);

    TRACE("#W#  - FOUND #%03d: off=%09llx ds=%llx, off=%09llx\n",
	    (int)(wc-wdf->chunk), wc->file_pos, wc->data_size, (u64)off );
    //DASSERT_MSG( off >= wc->file_pos, "%llx %llx\n", (u64)off, wc->file_pos );


    //--- main loop

    ccp src = buf;
    while ( count > 0 )
    {
	TRACE("#W# %d/%d count=%zd off=%llx,%llx \n",
		(int)(wc-wdf->chunk), wdf->chunk_used, count, (u64)off, wc->file_pos );

	//--- test, if offset is in space between to chunks

	if ( off < wc->file_pos )
	{
	    const u64 max_size = wc->file_pos - off;
	    u32 wr_size = max_size < count ? (u32)max_size : count;

	    //--- skip all leading zeros

	    while ( wr_size >= 4 && !*(u32*)src )
	    {
		off     += 4;
		src     += 4;
		wr_size -= 4;
		count   -= 4;
	    }

	    if ( wr_size)
	    {
		TRACE("#W# >CREATE#%02d    %p +%8zx = %10llx .. +%4x\n",
			(int)(wc-wdf->chunk), buf, src-(ccp)buf, (u64)off, wr_size );

		//--- create a new chunk

		u64 delta;
		wc = NewChunkWDF(wdf,wc-wdf->chunk,off,wr_size,&delta);
		const enumError stat = WriteAtF(&sf->f,wc->data_off+delta,src,wr_size);
		if (stat)
		    return stat;
		wc++;

		count -= wr_size;
		off   += wr_size;
		src   += wr_size;
	    }
	    if (!count)
		break;
	}


	//--- modify existing chunk

	if ( off >= wc->file_pos && off < wc->file_pos + wc->data_size )
	{
	    // we want a part of this
	    const u64 delta	= off - wc->file_pos;
	    const u64 max_size	= wc->data_size - delta;
	    const u32 wr_size	= max_size < count ? (u32)max_size : count;

	    TRACE("#W# >OVERWRITE#%02d %p +%8zx = %10llx .. +%4x, delta=%lld\n",
			(int)(wc-wdf->chunk), buf, src-(ccp)buf, (u64)off, wr_size, delta );

	    const enumError stat = WriteAtF(&sf->f,wc->data_off+delta,src,wr_size);
	    if (stat)
		return stat;

	    count -= wr_size;
	    off  += wr_size;
	    src += wr_size;
	    if (!count)
		break;
	}

	if ( ++wc >= wdf->chunk + wdf->chunk_used )
	    return WriteWDF(sf,off,src,count); // fall back to 'append'
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseWDF
	( SuperFile_t * sf, off_t off, const void * buf, size_t count )
{
    DASSERT(sf->wdf);
    return SparseHelper(sf,off,buf,count,WriteWDF,sf->wdf->min_hole_size);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroWDF ( SuperFile_t * sf, off_t off, size_t count )
{
    DASSERT(sf);
    wdf_controller_t *wdf = sf->wdf;
    if (!wdf)
	ERROR0(ERR_INTERNAL,0);
    DASSERT(wdf->chunk);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# WriteZeroWDF()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx,%llx\n",
		(u64)sf->f.file_off, (u64)sf->f.max_off, (u64)sf->max_virt_off);

    if (!count)
	return ERR_OK;

    // adjust the file size
    const off_t data_end = off + count;
    if ( sf->file_size < data_end )
	sf->file_size = data_end;

    ASSERT( wdf->chunk_used > 0 );
    const int used_m1 = wdf->chunk_used - 1;

    if ( off >= sf->max_virt_off )
	return ERR_OK;

    // [[align]] optimize using FindChunkWDF()
    // search chunk header with a binary search
    wdf2_chunk_t * wc = wdf->chunk;
    int beg = 0, end = used_m1;
    ASSERT( beg <= end );
    while ( beg < end )
    {
	int idx = (beg+end)/2;
	wc = wdf->chunk + idx;
	if ( off < wc->file_pos )
	    end = idx-1;
	else if ( idx < used_m1 && off >= wc[1].file_pos )
	    beg = idx + 1;
	else
	    beg = end = idx;
    }
    wc = wdf->chunk + beg;

    TRACE("#W#  - FOUND #%03d: off=%09llx ds=%llx, off=%09llx\n",
	    beg, wc->file_pos, wc->data_size, (u64)off );
    ASSERT( off >= wc->file_pos );
    ASSERT( beg == used_m1 || off < wc[1].file_pos );

    wdf2_chunk_t * last_wc = wdf->chunk + wdf->chunk_used;
    for ( ; wc < last_wc || wc->file_pos < data_end; wc++ )
    {
	off_t end = wc->file_pos + wc->data_size;
	TRACE("loop: wc=%llx,%llx,%llx off=%llx, end=%llx\n",
	    wc->file_pos, wc->data_off, wc->data_size, (u64)off, (u64)end );
	if ( off >= end )
	    continue;

	if ( off < wc->file_pos )
	    off = wc->file_pos;
	if ( end > data_end )
	    end = data_end;
	if ( off < end )
	{
	    const enumError err
		= WriteZeroAtF( &sf->f, wc->data_off+(off-wc->file_pos), end-off );
	    if (err)
		return err;
	}
    }
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    options			///////////////
///////////////////////////////////////////////////////////////////////////////

uint opt_wdf_version		= 0;
uint opt_wdf_align		= 0;
uint opt_wdf_min_holesize	= 0;

uint use_wdf_version		= WDF_DEF_VERSION;
uint use_wdf_align		= WDF_DEF_ALIGN;

///////////////////////////////////////////////////////////////////////////////

void SetupOptionsWDF()
{
    if (opt_wdf_align)
    {
	use_wdf_align = 1;
	while (!(opt_wdf_align&use_wdf_align))
	    use_wdf_align <<= 1;
    }
    else
	use_wdf_align = 0;

 #if WDF_DEF_VERSION == WDF_MAX_VERSION // [[wdf2]]
    use_wdf_version = opt_wdf_version && opt_wdf_version <= WDF_MAX_VERSION
			? opt_wdf_version : WDF_MAX_VERSION;
 #else
    use_wdf_version = opt_wdf_version ? opt_wdf_version
		    : use_wdf_align >= 0x80 ? 2 : WDF_DEF_VERSION;
    if ( use_wdf_version > WDF_MAX_VERSION )
	 use_wdf_version = WDF_MAX_VERSION;
 #endif

    opt_wdf_min_holesize = opt_wdf_min_holesize + 3 & ~3;
}

///////////////////////////////////////////////////////////////////////////////

int SetModeWDF ( uint vers, ccp align )
{
    if (vers)
    {
	opt_wdf_version = vers;
	output_file_type = vers > 1 ? OFT_WDF2 : OFT_WDF1;
    }
    else if ( !(oft_info[output_file_type].attrib & OFT_A_WDF) )
	output_file_type = OFT__WDF_DEF;

    char opt_buf[10], *opt_name = "wdf";
    if (vers)
    {
	snprintf(opt_buf,sizeof(opt_buf),"wdf%u",vers);
	opt_name = opt_buf;
    }

    const int stat = ScanOptAlignWDF(align,opt_name);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int ScanOptAlignWDF ( ccp arg, ccp opt_name )
{
    if (!arg)
	return 0;

    if (!opt_name)
	opt_name = "align-wdf";

    ccp minhole = strchr(arg,',');
    char buf[1000];
    if ( minhole && minhole - arg < sizeof(buf) )
    {
	StringCopyS(buf,sizeof(buf),arg);
	buf[ minhole - arg ] = 0;
	arg = buf;
	minhole++;
    }
    else
	minhole = 0;

    u32 align = 0;
    enumError stat = !*arg
	? 0
	: ScanSizeOptU32(
		&align,			// u32 * num
		arg,			// ccp source
		1,			// default_factor1
		0,			// int force_base
		opt_name,		// ccp opt_name
		0,			// u64 min
		WDF_MAX_ALIGN,		// u64 max
		0,			// u32 multiple
		1,			// u32 pow2
		true			// bool print_err
		) != ERR_OK;

    if (!stat)
    {
	opt_wdf_align = align;
	opt_wdf_min_holesize = 0;

	if ( minhole && *minhole )
	{
	    u32 value;
	    stat = ScanSizeOptU32(
		&value,			// u32 * num
		minhole,		// ccp source
		1,			// default_factor1
		0,			// int force_base
		opt_name,		// ccp opt_name
		0,			// u64 min
		WDF_MAX_ALIGN,		// u64 max
		0,			// u32 multiple
		0,			// u32 pow2
		true			// bool print_err
		) != ERR_OK;

	    if (!stat)
		opt_wdf_min_holesize = value + 3 & ~3;
	}
    }
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

