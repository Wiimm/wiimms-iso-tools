
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

#ifdef HAVE_ZLIB
  #include <zlib.h>
#endif

#include "dclib/dclib-debug.h"
#include "lib-gcz.h"
#include "lib-sf.h"
#include "patch.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    options			///////////////
///////////////////////////////////////////////////////////////////////////////

bool opt_gcz_zip	= false;
u32  opt_gcz_block_size	= GCZ_DEF_BLOCK_SIZE;

///////////////////////////////////////////////////////////////////////////////

int ScanOptGCZBlock ( ccp arg )
{
    if (!arg)
	return 0;

    u32 block_size;
    enumError stat = ScanSizeOptU32(
		&block_size,		// u32 * num
		arg,			// ccp source
		1,			// default_factor1
		0,			// int force_base
		"gcz-block",		// ccp opt_name
		256,			// u64 min
		16*MiB,			// u64 max
		0,			// u32 multiple
		0,			// u32 pow2
		true			// bool print_err
		) != ERR_OK;

    if (!stat)
	opt_gcz_block_size = block_size;
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

#define MOD_ADLER 65521

static u32 CalcAdler32 ( const u8 *data, uint len )
{
    // source adapted from Dolphin project

    DASSERT(data);
    u32 a = 1, b = 0;

    while (len)
    {
	uint tlen = len < 5550 ? len : 5550;
	len -= tlen;

	do
	{
	    a += *data++;
	    b += a;
	}
	while (--tlen);

	a = (a & 0xffff) + (a >> 16) * (65536 - MOD_ADLER);
	b = (b & 0xffff) + (b >> 16) * (65536 - MOD_ADLER);
    }

    // It can be shown that a <= 0x1013a here, so a single subtract will do.
    if ( a >= MOD_ADLER )
	a -= MOD_ADLER;

    // It can be shown that b can reach 0xfff87 here.
    b = (b & 0xffff) + (b >> 16) * (65536 - MOD_ADLER);

    if ( b >= MOD_ADLER )
	b -= MOD_ADLER;

    return b << 16 | a;
}

///////////////////////////////////////////////////////////////////////////////

static enumError CheckAdler32
	( WFile_t *f, uint block, u32 adler32, const void *data, uint data_size )
{
    DASSERT(f);
    DASSERT(data);

    u32 calced = CalcAdler32(data,data_size);
    if ( calced == adler32 )
	return ERR_OK;

    if (!f->disable_errors)
	ERROR0(ERR_GCZ_INVALID,
		"Wrong checksum for block block #%u (need=%08x,have=%08x]: %s\n",
		block, adler32, calced, f->fname );
    return ERR_GCZ_INVALID;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Interface			///////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef HAVE_ZLIB

 static enumError ZLIB_MISSING ( WFile_t * f )
 {
    DASSERT(f);
    if (!f->disable_errors)
	ERROR0(ERR_NOT_IMPLEMENTED,
		"GCZ support is not implemented,"
		" because of missing `zlib´ support: %s\n",
		f->fname );
    return ERR_NOT_IMPLEMENTED;
 } 

#endif

///////////////////////////////////////////////////////////////////////////////

bool IsValidGCZ
(
    const void		*data,		// valid pointer to data
    uint		data_size,	// size of data to analyze
    u64			file_size,	// NULL or known file size
    GCZ_Head_t		*head		// not NULL: store header (local endian) here
)
{
    DASSERT(data);

    if ( !data || data_size < sizeof(GCZ_Head_t) )
    {
	if (head)
	    memset(head,0,sizeof(*head));
	return false;
    }

    GCZ_Head_t local_head;
    if (!head)
	head = &local_head;
    const GCZ_Head_t *src_head = data;
    head->magic		= le32(&src_head->magic);
    head->sub_type	= le32(&src_head->sub_type);
    head->compr_size	= le64(&src_head->compr_size);
    head->image_size	= le64(&src_head->image_size);
    head->block_size	= le32(&src_head->block_size);
    head->num_blocks	= le32(&src_head->num_blocks);

    if	(  head->magic != GCZ_MAGIC_NUM
	|| head->sub_type != GCZ_TYPE
	|| !head->block_size
	)
    {
	return false;
    }

    const uint max_blocks = WII_MAX_DISC_SIZE / head->block_size;

 #if HAVE_PRINT
    PRINT("----- GCZ:\n");

    u64 off = 0, size = sizeof(GCZ_Head_t);
    PRINT("%10llx %9llx  head\n",off,size);
    off += size; size = head->num_blocks * 8;
    PRINT("%10llx %9llx  %u offsets [%u max]\n",off,size,head->num_blocks,max_blocks);
    off += size; size = head->num_blocks * 4;
    PRINT("%10llx %9llx  %u checksums\n",off,size,head->num_blocks);
    off += size; size = head->compr_size;
    PRINT("%10llx %9llx  compressed data\n",off,size);
    off += size;
    PRINT("%10llx            END\n",off);

    PRINT("%10llx            file size\n",file_size);
    PRINT("%10llx            image size\n",head->image_size);
    PRINT(  "%10x            block size\n",head->block_size);
    PRINT("-----\n");
 #endif

    if ( head->num_blocks > max_blocks )
	return false;

    if (file_size)
    {
	const u64 total	= sizeof(GCZ_Head_t)
			+ 12 * head->num_blocks
			+ head->num_blocks;
	if ( total > file_size )
	    return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ResetGCZ( GCZ_t *gcz )
{
    if (gcz)
    {
	FREE(gcz->offset);
	FREE(gcz->data);
	FREE(gcz->zero_data);
	memset(gcz,0,sizeof(*gcz));
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    GCZ: WFile_t level reading		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError LoadHeadGCZ
(
    GCZ_t		*gcz,		// pointer to data, will be initalized
    WFile_t		*f		// file to read
)
{
    DASSERT(gcz);
    DASSERT(f);
    memset(gcz,0,sizeof(*gcz));

 #ifndef HAVE_ZLIB
    return ZLIB_MISSING(f);
 #else

    enumError stat = ReadAtF(f,0,&gcz->head,sizeof(gcz->head));
    if (stat)
	return stat;

    if (!IsValidGCZ(&gcz->head,sizeof(gcz->head),f->st.st_size,&gcz->head))
	return ERR_NO_GCZ;

    const uint list_size = 12 * gcz->head.num_blocks;
    gcz->data_offset = sizeof(GCZ_Head_t) + list_size;
    gcz->offset = MALLOC(list_size);
    gcz->checksum = (u32*)(gcz->offset+gcz->head.num_blocks);
    stat = ReadAtF(f,sizeof(gcz->head),gcz->offset,list_size);
    if (stat)
	return stat;

    gcz->block = M1(gcz->block);
    gcz->data  = MALLOC(2*gcz->head.block_size);
    gcz->cdata = gcz->data + gcz->head.block_size;

    return ERR_OK;
 #endif
}

///////////////////////////////////////////////////////////////////////////////

enumError LoadDataGCZ
(
    GCZ_t		*gcz,		// pointer to data, will be initalized
    WFile_t		*f,		// source file
    off_t		off,		// file offset
    void		*buf,		// destination buffer
    size_t		count		// number of bytes to read
)
{
    DASSERT(gcz);
    DASSERT(f);
    DASSERT(buf);

 #ifndef HAVE_ZLIB
    return ZLIB_MISSING(f);
 #else

    if ( !f || !gcz )
	return ERROR0(ERR_INTERNAL,0);

    u8 *dest = buf;
    while (count)
    {
	u32 block = off/gcz->head.block_size;
	u32 copy_delta = off - block * (u64)gcz->head.block_size;
	u32 copy_size = gcz->head.block_size - copy_delta;
	if ( copy_size > count )
	    copy_size = count;

	noPRINT("off=%llx -> %d,%d\n",(u64)off,block,gcz->block);
	if ( block >= gcz->head.num_blocks )
	{
	    noPRINT("GCZ/ZERO: b=%x, copy=%x +%x\n",block,copy_delta,copy_size);
	    memset(dest,0,copy_size);
	}
	else if ( block != gcz->block )
	{
	    gcz->block = M1(gcz->block);
	    s64 read_off  = le64(gcz->offset+block);
	    u32 read_size = ( block == gcz->head.num_blocks - 1
				? gcz->head.compr_size
				: le64(gcz->offset+block+1) ) - read_off;

	    if ( read_size > gcz->head.block_size )
	    {
		if (!f->disable_errors)
		    ERROR0(ERR_GCZ_INVALID,
			    "Invalid block size for block #%u/%u (size 0x%x): %s\n",
			    block, gcz->head.num_blocks, read_size, f->fname );
		return ERR_GCZ_INVALID;
	    }

	    if ( read_off < 0 )
	    {
		//--- uncompressed

		read_off &= 0x7fffffffffffffffllu;
		read_off += gcz->data_offset;
		noPRINT("GCZ/RAW: b=%x, read=%llx + %x, copy=%x +%x\n",
			block, read_off, read_size, copy_delta, copy_size );
		enumError err = ReadAtF(f,read_off,gcz->data,read_size);
		if (!err)
		    err = CheckAdler32( f, block, le32(gcz->checksum+block),
					gcz->data, read_size );
		if (err)
		    return err;
	    }
	    else
	    {
		//--- compressed

		read_off += gcz->data_offset;
		noPRINT("GCZ/UNZIP: b=%x, read=%llx + %x, copy=%x +%x\n",
			block, read_off, read_size, copy_delta, copy_size );
		enumError err = ReadAtF(f,read_off,gcz->cdata,read_size);
		if (!err)
		    err = CheckAdler32( f, block, le32(gcz->checksum+block),
					gcz->cdata, read_size );
		if (err)
		    return err;

		z_stream zs;
		memset(&zs,0,sizeof(zs));
		zs.next_in   = gcz->cdata;
		zs.avail_in  = read_size;
		zs.next_out  = gcz->data;
		zs.avail_out = gcz->head.block_size;
		noPRINT("Z: in=%p+%x, out=%p+%x\n",
			zs.next_in, zs.avail_in, zs.next_out, zs.avail_out );
		int stat = inflateInit(&zs);
		if ( stat < 0 )
		{
		 inflate_err:
		    if (!f->disable_errors)
			ERROR0(ERR_GCZ_INVALID,
				"Error while uncompressing block #%u (zlib-err=%d): %s\n",
				block, stat, f->fname );
		    return ERR_GCZ_INVALID;
		}
		stat = inflate(&zs,Z_FULL_FLUSH);
		if ( stat != Z_STREAM_END )
		{
		    inflateEnd(&zs);
		    goto inflate_err;
		}

		read_size = gcz->head.block_size - zs.avail_out;
		stat = inflateEnd(&zs);
		if ( stat < 0 )
		    goto inflate_err;
		PRINT("block %u, read=%x, cksum %08x %08x\n",
			block, read_size, (u32)zs.adler, le32(gcz->checksum+block) );
	    }
	    if ( read_size < gcz->head.block_size )
		memset( gcz->data + read_size, 0, gcz->head.block_size - read_size );
	    gcz->block = block;
	}

	noPRINT("GCZ/COPY: b=%x, copy=%x +%x/%zx\n",block,copy_delta,copy_size,count);
	memcpy(dest,gcz->data+copy_delta,copy_size);
	dest  += copy_size;
	off   += copy_size;
	count -= copy_size;
    }

    return ERR_OK;

 #endif
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			GCZ reading support		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadGCZ
(
    struct SuperFile_t	* sf		// file to setup
)
{
    DASSERT(sf);
    PRINT("#G# SetupReadGCZ(%p) gcz=%p, wbfs=%p, fp=%p, fd=%d\n",
		sf, sf->gcz, sf->wbfs, sf->f.fp, sf->f.fd );

    if ( sf->gcz || sf->wbfs )
	return ERR_OK;

    ASSERT(sf);
    CleanSF(sf);

    if ( sf->f.seek_allowed && sf->f.st.st_size < sizeof(GCZ_t) )
	return ERR_NO_GCZ;

    sf->gcz = MALLOC(sizeof(GCZ_t));
    enumError err = LoadHeadGCZ(sf->gcz,&sf->f);
    if (err)
	return ERROR0(ERR_GCZ_INVALID,"Invalid GCZ file: %s\n",sf->f.fname);

    sf->file_size = sf->max_virt_off = sf->gcz->head.image_size;
    SetupIOD(sf,OFT_GCZ,OFT_GCZ);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ReadGCZ
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// file offset
    void		* buf,		// destination buffer
    size_t		count		// number of bytes to read
)
{
    DASSERT(sf);
    DASSERT(sf->gcz);
    TRACE("#G# -----\n");
    PRINT(TRACE_RDWR_FORMAT, "#G# ReadGCZ()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    if ( !sf || !sf->gcz )
	return ERROR0(ERR_INTERNAL,0);

    return LoadDataGCZ(sf->gcz,&sf->f,off,buf,count);
}

///////////////////////////////////////////////////////////////////////////////

off_t DataBlockGCZ
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// virtual file offset
    size_t		hint_align,	// if >1: hint for a aligment factor
    off_t		* block_size	// not null: return block size
)
{
    DASSERT(sf);
    GCZ_t *gcz = sf->gcz;
    DASSERT(gcz);

    const u64 gcz_block_size = gcz->head.block_size;
    const u32 block = off / gcz_block_size;
    if ( block >= gcz->head.num_blocks )
	return DataBlockStandard(sf,off,hint_align,block_size);

    const off_t off1 = block * gcz_block_size;
    if ( off < off1 )
	 off = off1;

    if (block_size)
	*block_size = ( gcz->head.num_blocks * gcz_block_size ) - off;

    return off;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			GCZ writing support		///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef HAVE_ZLIB

 static enumError write_block
 (
    struct SuperFile_t	* sf,		// file
    bool		is_zero		// true: all data is zero
 )
 {
    DASSERT(sf);
    GCZ_t *gcz = sf->gcz;
    DASSERT(gcz);

    DASSERT(!IS_M1(gcz->block));
    DASSERT( gcz->block <= gcz->head.num_blocks );

    if ( gcz->block >= gcz->head.num_blocks )
    {
	if (!sf->f.disable_errors)
	    ERROR0(ERR_WRITE_FAILED,
			"GCZ files can't grow (block %u/%u): %s\n",
			gcz->block, gcz->head.num_blocks, sf->f.fname );
	return ERR_WRITE_FAILED;
    }

    u8 *data;
    uint size;
    u64 flag;
    u32 checksum;

    if ( is_zero && gcz->zero_data )
    {
	data = gcz->zero_data;
	size = gcz->zero_size;
	flag = 0;
	checksum = gcz->zero_checksum;
    }
    else
    {
	data = gcz->data;
	size = gcz->head.block_size;
	flag = 0x8000000000000000ull;

	//--- try compression

	bool try_zip = true;
	if (gcz->fast)
	{
	    DASSERT(gcz->disc);
	    uint wii_sector = gcz->block * (u64)gcz->head.block_size / WII_SECTOR_SIZE;
	    if ( wii_sector < WII_MAX_SECTORS
		&& gcz->disc->usage_table[wii_sector] & WD_USAGE_F_CRYPT )
	    {
		//PRINT("SKIP ZIP: %u -> %u\n",gcz->block,wii_sector);
		try_zip = false;
	    }
	}

	if (try_zip)
	{
	    z_stream zs;
	    memset(&zs,0,sizeof(zs));
	    zs.next_in   = data;
	    zs.avail_in  = size;
	    zs.next_out  = gcz->cdata;
	    zs.avail_out = gcz->head.block_size;
	    zs.zalloc    = Z_NULL;
	    zs.zfree     = Z_NULL;
	    zs.opaque    = Z_NULL;

	    if ( deflateInit(&zs,9) == Z_OK
		&& deflate(&zs,Z_FINISH) == Z_STREAM_END
		&& zs.avail_out > gcz->head.block_size/64 )
	    {
		data = gcz->cdata;
		size = gcz->head.block_size - zs.avail_out;
		flag = 0;
	    }
	    deflateEnd(&zs);
	}

	checksum = CalcAdler32(data,size);
    }


    //--- write data

    noPRINT("WRITE BLOCK #%d [raw=%d,z=%d,%d,size=%x]\n",
		gcz->block,flag!=0,data==gcz->zero_data,is_zero,size);
    write_le32( gcz->checksum + gcz->block, checksum );
    write_le64( gcz->offset + gcz->block, gcz->head.compr_size|flag );

    const off_t off = gcz->data_offset + gcz->head.compr_size;
    enumError err = WriteAtF(&sf->f,off,data,size);
    if (err)
	return err;
    gcz->head.compr_size += size;

    if ( is_zero && !gcz->zero_data )
    {
	gcz->zero_data = MEMDUP(data,size);
	gcz->zero_size = size;
	gcz->zero_checksum = checksum;
    }
    return ERR_OK;
 }
#endif

///////////////////////////////////////////////////////////////////////////////

static enumError flush_block
(
    struct SuperFile_t	* sf,		// file
    u32			next_block	// number of next block
)
{
 #ifndef HAVE_ZLIB
    return ZLIB_MISSING(&sf->f);
 #else

    DASSERT(sf);
    GCZ_t *gcz = sf->gcz;
    DASSERT(gcz);

    if ( gcz->block == next_block )
	return ERR_OK; // block is still active

    if (!IS_M1(gcz->block))
    {
	if ( next_block < gcz->block )
	{
	    if (!sf->f.disable_errors)
		ERROR0(ERR_WRITE_FAILED,
			"GCZ file must be created sequential: %s\n",
			sf->f.fname );
	    return ERR_WRITE_FAILED;
	}

	enumError err = write_block(sf,false);
	if (err)
	    return err;
    }
    memset(gcz->data,0,gcz->head.block_size);

    if (IS_M1(next_block))
	gcz->block = next_block;
    else
    {
	while ( ++gcz->block < next_block )
	{
	    enumError err = write_block(sf,true);
	    if (err)
		return err;
	}
    }
    DASSERT( gcz->block == next_block );
    return ERR_OK;

 #endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteGCZ
(
    struct SuperFile_t	* sf,		// file to setup
    u64			src_file_size	// NULL or source file size
)
{
 #ifndef HAVE_ZLIB
    return ZLIB_MISSING(&sf->f);
 #else

    DASSERT(sf);
    PRINT("#G# SetupWriteGCZ(%p) gcz=%p wbfs=%p, fp=%p, fd=%d\n",
		sf, sf->gcz, sf->wbfs, sf->f.fp, sf->f.fd );

    if (sf->gcz)
	return ERROR0(ERR_INTERNAL,0);

    //--- retrieve image size

    u64 image_size = 0;
    if (sf->src)
	image_size = sf->src->file_size;

    if (!image_size)
    {
	image_size = sf->source_size;
	if (!image_size)
	{
	    image_size = sf->file_size;
	    if (!image_size)
		return ERROR0(ERR_INTERNAL,"%p %llu %llu %llu",
			sf->src, sf->src ? sf->src->file_size : 0ull,
			sf->source_size, sf->file_size );
	}
    }


    //--- create GCZ_t

    GCZ_t *gcz = CALLOC(1,sizeof(*gcz));
    sf->gcz = gcz;
    gcz->head.block_size = opt_gcz_block_size;
    gcz->head.num_blocks = ( image_size + gcz->head.block_size - 1 )
			 / gcz->head.block_size;
    gcz->head.image_size = gcz->head.num_blocks * (u64)gcz->head.block_size;

    const uint list_size = 12 * gcz->head.num_blocks;
    gcz->data_offset = sizeof(GCZ_Head_t) + list_size;
    gcz->offset = CALLOC(1,list_size);
    gcz->checksum = (u32*)(gcz->offset+gcz->head.num_blocks);

    gcz->block = M1(gcz->block);
    gcz->data  = MALLOC(2*gcz->head.block_size);
    gcz->cdata = gcz->data + gcz->head.block_size;


    //--- source disc support

    if (sf->src)
    {
	gcz->disc = OpenDiscSF(sf->src,false,true);
	if (  gcz->disc
	   && gcz->disc->disc_attrib & WD_DA_WII
	   && !opt_gcz_zip
	   && gcz->head.block_size <= WII_SECTOR_SIZE
	   )
	{
	    enumEncoding enc = SetEncoding(encoding,0,0);
	    PRINT("enc=%x, DECRYPT=%x, ENCRYPT=%x, is_enc=%u\n",
		enc,ENCODE_DECRYPT,ENCODE_ENCRYPT,wd_disc_is_encrypted(gcz->disc));
	    if ( !( enc & ENCODE_DECRYPT )
		&& ( enc & ENCODE_ENCRYPT || wd_disc_is_encrypted(gcz->disc) == 1000 ))
	    {
		gcz->fast = true;

	     #if HAVE_PRINT
		wd_print_usage_tab(stdout,2,gcz->disc->usage_table,gcz->disc->iso_size,false);
	     #endif
	    }
	}
    }

    PRINT("GCZ/SETUP-WRITE: blocks: %u * 0x%x\n",
		gcz->head.num_blocks, gcz->head.block_size );
    return ERR_OK;

 #endif
}

///////////////////////////////////////////////////////////////////////////////

enumError TermWriteGCZ
(
    struct SuperFile_t	* sf		// file to terminate
)
{
 #ifndef HAVE_ZLIB
    return ZLIB_MISSING(&sf->f);
 #else

    DASSERT(sf);
    GCZ_t *gcz = sf->gcz;
    DASSERT(gcz);

    PRINT("TermWriteGCZ() block %u/%u\n",gcz->block,gcz->head.num_blocks);
    enumError err = flush_block(sf,gcz->head.num_blocks);
    if (err)
	return err;
    gcz->block = M1(gcz->block);

    GCZ_Head_t hd;
    write_le32( &hd.magic,	GCZ_MAGIC_NUM );
    write_le32( &hd.sub_type,	GCZ_TYPE );
    write_le64( &hd.compr_size,	gcz->head.compr_size );
    write_le64( &hd.image_size,	gcz->head.image_size );
    write_le32( &hd.block_size,	gcz->head.block_size );
    write_le32( &hd.num_blocks,	gcz->head.num_blocks );
    err = WriteAtF(&sf->f,0,&hd,sizeof(hd));
    return err
	? err
	: WriteAtF(&sf->f,sizeof(hd),gcz->offset,gcz->head.num_blocks*12);

 #endif
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteGCZ
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
)
{
 #ifndef HAVE_ZLIB
    return ZLIB_MISSING(&sf->f);
 #else

    DASSERT(sf);
    GCZ_t *gcz = sf->gcz;
    DASSERT(gcz);

    TRACE("#G# -----\n");
    noPRINT(TRACE_RDWR_FORMAT, "#G# WriteGCZ()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count, "" );

    const u8 *src = buf;
    while (count)
    {
	u32 block = off / gcz->head.block_size;
	enumError err = flush_block(sf,block);
	if (err)
	    return err;

	u32 copy_delta = off - block * (u64)gcz->head.block_size;
	u32 copy_size = gcz->head.block_size - copy_delta;
	if ( copy_size > count )
	    copy_size = count;
	noPRINT("GCZ/WRITE: b=%x, copy=%x +%x\n",block,copy_delta,copy_size);

	memcpy( gcz->data + copy_delta, src, copy_size );
	src   += copy_size;
	off   += copy_size;
	count -= copy_size;
    }
    return ERR_OK;

 #endif
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroGCZ
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    size_t		count		// number of bytes to write
)
{
    while ( count > 0 )
    {
	const size_t size = count < sizeof(zerobuf) ? count : sizeof(zerobuf);
	const enumError err = WriteGCZ(sf,off,zerobuf,size);
	if (err)
	    return err;
	off   += size;
	count -= size;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError FlushGCZ
(
    struct SuperFile_t	* sf		// destination file
)
{
    DASSERT(sf);
    DASSERT(sf->gcz);

    return flush_block(sf,M1(u32));
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////                          END                    ///////////////
///////////////////////////////////////////////////////////////////////////////

