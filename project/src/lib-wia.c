
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

#include "dclib/dclib-debug.h"
#include "iso-interface.h"
#include "lib-bzip2.h"
#include "lib-lzma.h"

///////////////////////////////////////////////////////////////////////////////


	//////////////////////////////////
	//  [[2do]]:
	//	- WIA_MM_GROWING
	//	- raw data not 4-aligned
	//////////////////////////////////

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    consts			///////////////
///////////////////////////////////////////////////////////////////////////////

#define WATCH_GROUP -1		// -1: disabled
#define WATCH_SUB_GROUP 0

///////////////////////////////////////////////////////////////////////////////

#if 0
    #undef  PRINT
    #define PRINT noPRINT
    #undef  PRINT_IF
    #define PRINT_IF noPRINT
#endif

///////////////////////////////////////////////////////////////////////////////

typedef enum mm_mode_t
{
    WIA_MM_IGNORE = WD_PAT_IGNORE,

    WIA_MM_HEADER_DATA,		// header data, part of wia_disc_t
    WIA_MM_CACHED_DATA,		// cached data, write at close
    WIA_MM_RAW_GDATA,		// raw data, managed with 'gdata'
    WIA_MM_PART_GDATA_0,	// partition data #0, managed with 'gdata'
    WIA_MM_PART_GDATA_1,	// partition data #1, managed with 'gdata'
    WIA_MM_EOF,			// end of file marker
    WIA_MM_GROWING,		// growing space

} mm_mode_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    static data			///////////////
///////////////////////////////////////////////////////////////////////////////

static bool empty_decrypted_sector_initialized = false;
static wd_part_sector_t empty_decrypted_sector;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    manage WIA			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetWIA
(
    wia_controller_t	* wia		// NULL or valid pointer
)
{
    if (wia)
    {
	wd_close_disc(wia->wdisc);
	FREE(wia->part);
	FREE(wia->raw_data);
	FREE(wia->group);
	FREE(wia->gdata);
	wd_reset_memmap(&wia->memmap);

	memset(wia,0,sizeof(*wia));
    }
}

///////////////////////////////////////////////////////////////////////////////

static u32 AllocBufferWIA
(
    wia_controller_t	* wia,		// valid pointer
    u32			chunk_size,	// wanted chunk size
    bool		is_writing,	// true: cut chunk size
    bool		calc_only	// true: don't allocate memory
)
{
    DASSERT(wia);

    u32 chunk_groups = chunk_size <= 1
		? WIA_DEF_CHUNK_FACTOR
		: ( chunk_size + WIA_BASE_CHUNK_SIZE/2 ) / WIA_BASE_CHUNK_SIZE;
		
    if (!chunk_groups)
	chunk_groups = 1;
    else if ( is_writing && chunk_groups > WIA_MAX_CHUNK_FACTOR )
	chunk_groups = WIA_MAX_CHUNK_FACTOR;

    chunk_size = chunk_groups * WIA_BASE_CHUNK_SIZE;

    wia->chunk_size	= chunk_size;
    wia->chunk_groups	= chunk_groups;
    wia->chunk_sectors	= chunk_groups * WII_GROUP_SECTORS;

    u32 needed_tempbuf_size
		= wia->chunk_groups
		* ( WII_GROUP_SIZE + WII_N_HASH_GROUP * sizeof(wia_exception_t) );
    wia->memory_usage = needed_tempbuf_size + chunk_size;

    if (calc_only)
    {
	wia->gdata_size = chunk_size;
	FREE(wia->gdata);
	wia->gdata = 0;
    }
    else
    {
	PRINT("CHUNK_SIZE=%s, GDATA_SIZE=%s, IOBUF_SIZE=%s/%s, G+S=%u,%u\n",
	    wd_print_size_1024(0,0,wia->chunk_size,0),
	    wd_print_size_1024(0,0,wia->gdata_size,0),
	    wd_print_size_1024(0,0,needed_tempbuf_size,0),
	    wd_print_size_1024(0,0,tempbuf_size,0),
	    wia->chunk_groups, wia->chunk_sectors );

	AllocTempBuffer(needed_tempbuf_size);
	if ( !wia->gdata || wia->gdata_size != chunk_size )
	{
	    wia->gdata_size = chunk_size;
	    FREE(wia->gdata);
	    wia->gdata = MALLOC(wia->gdata_size);
	}
    }

    DASSERT( calc_only || wia->gdata );
    DASSERT( calc_only || tempbuf );
    DASSERT( wia->chunk_sectors == wia->chunk_groups * WII_GROUP_SECTORS );

    return wia->chunk_size;
};

///////////////////////////////////////////////////////////////////////////////

u32 CalcMemoryUsageWIA
(
    wd_compression_t	compression,	// compression method
    int			compr_level,	// valid are 1..9 / 0: use default value
    u32			chunk_size,	// wanted chunk size
    bool		is_writing	// false: reading mode, true: writing mode
)
{
    wia_controller_t wia;
    memset(&wia,0,sizeof(wia));
    u32 size = AllocBufferWIA(&wia,chunk_size,is_writing,true);
    ResetWIA(&wia);

    switch(compression)
    {
	case WD_COMPR__N:
	case WD_COMPR_NONE:
	case WD_COMPR_PURGE:
	    break;

	case WD_COMPR_BZIP2:
	 #ifndef NO_BZIP2
	    size += CalcMemoryUsageBZIP2(compr_level,is_writing);
	 #endif
	    break;

	case WD_COMPR_LZMA:
	    size += CalcMemoryUsageLZMA(compr_level,is_writing);
	    break;

	case WD_COMPR_LZMA2:
	    size += CalcMemoryUsageLZMA2(compr_level,is_writing);
	    break;
    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////

int CalcDefaultSettingsWIA
(
    wd_compression_t	* compression,	// NULL or compression method
    int			* compr_level,	// NULL or compression level
    u32			* chunk_size	// NULL or wanted chunk size
)
{
    //----- get param

    wd_compression_t
	compr = compression ? *compression : WD_COMPR__DEFAULT;
    int level = compr_level ? *compr_level : 0;
    u32 csize = chunk_size  ? *chunk_size  : 0;


    //----- normalize compression method

    if ( (unsigned)compr >= WD_COMPR__N )
	compr = WD_COMPR__DEFAULT;


    //----- normalize compression level

    u32 clevel = WIA_DEF_CHUNK_FACTOR;
    switch(compr)
    {
	case WD_COMPR__N:
	case WD_COMPR_NONE:
	case WD_COMPR_PURGE:
	    level = 0;
	    //clevel = WIA_DEF_CHUNK_FACTOR; // == default setting
	    break;

	case WD_COMPR_BZIP2:
	 #ifdef NO_BZIP2
	    level = 0;
	 #else
	    level = CalcCompressionLevelBZIP2(level);
	 #endif
	    //clevel = WIA_DEF_CHUNK_FACTOR; // == default setting
	    break;

	case WD_COMPR_LZMA:
	case WD_COMPR_LZMA2:
	    level = CalcCompressionLevelLZMA(level);
	    //clevel = WIA_DEF_CHUNK_FACTOR; // == default setting
	    break;
    }


    //----- normalize compression chunksize

    if ( csize > 1 )
    {
	clevel = csize / WIA_BASE_CHUNK_SIZE;
	if (!clevel)
	    clevel = 1;
	else if ( clevel > WIA_MAX_CHUNK_FACTOR )
	    clevel = WIA_MAX_CHUNK_FACTOR;
    }
    csize = clevel * WIA_BASE_CHUNK_SIZE;


    //----- store results

    int stat = 0;
    if ( compression && *compression != compr )
    {
	*compression = compr;
	stat |= 1;
    }
    if ( compr_level && *compr_level != level )
    {
	*compr_level = level;
	stat |= 2;
    }
    if ( chunk_size && *chunk_size != csize )
    {
	*chunk_size = csize;
	stat |= 4;
    }
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool IsWIA
(
    const void		* data,		// data to check
    size_t		data_size,	// size of data
    void		* id6_result,	// not NULL: store ID6 (6 bytes without null term)
    wd_disc_type_t	* disc_type,	// not NULL: store disc type
    wd_compression_t	* compression	// not NULL: store compression
)
{
    bool is_wia = false;
    const wia_file_head_t * fhead = data;
    if ( data_size >= sizeof(wia_file_head_t) )
    {
	if (!memcmp(fhead->magic,WIA_MAGIC,sizeof(fhead->magic)))
	{
	    sha1_hash_t hash;
	    SHA1( (u8*)fhead, sizeof(*fhead)-sizeof(fhead->file_head_hash), hash );
	    is_wia = !memcmp(hash,fhead->file_head_hash,sizeof(hash));
	    //HEXDUMP(0,0,0,-WII_HASH_SIZE,hash,WII_HASH_SIZE);
	    //HEXDUMP(0,0,0,-WII_HASH_SIZE,fhead->file_head_hash,WII_HASH_SIZE);
	}
    }

    const wia_disc_t * disc = (const wia_disc_t*)(fhead+1);

    if (id6_result)
    {
	memset(id6_result,0,6);
	if ( is_wia && data_size >= (ccp)&disc->dhead - (ccp)data + 6 )
	    memcpy(id6_result,&disc->dhead,6);
    }

    if (disc_type)
    {
	*disc_type = is_wia && data_size >= (ccp)&disc->disc_type
						- (ccp)data + sizeof(disc->disc_type)
		? ntohl(disc->disc_type)
		: 0;
    }

    if (compression)
    {
	*compression = is_wia && data_size >= (ccp)&disc->compression
						- (ccp)data + sizeof(disc->compression)
		? ntohl(disc->compression)
		: 0;
    }

    return is_wia;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static u32 calc_except_size
(
    const void		* except,	// pointer to wia_except_list_t
    u32			n_groups	// number of groups
)
{
    DASSERT(except);
    DASSERT( be16(except) == ntohs(((wia_except_list_t*)except)->n_exceptions) );

    wia_except_list_t * elist = (wia_except_list_t*)except;
    while ( n_groups-- > 0 )
	elist = (wia_except_list_t*)( elist->exception + ntohs(elist->n_exceptions) );

    return (ccp)elist - (ccp)except;
}

///////////////////////////////////////////////////////////////////////////////

static u64 calc_group_size
(
    wia_controller_t	* wia,		// valid pointer
    u32			first_group,	// index of first group
    u32			n_groups	// number of groups
)
{
    DASSERT(wia);
    DASSERT( first_group <= wia->disc.n_groups );
    DASSERT( first_group + n_groups <= wia->disc.n_groups );

    u64 size = 0;
    wia_group_t * grp = wia->group + first_group;
    while ( n_groups-- > 0 )
    {
	size += ntohl(grp->data_size);
	grp++;
    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////

static enumError expand_segments
(
    SuperFile_t		* sf,		// source file
    const wia_segment_t	* seg,		// source segment pointer
    void		* seg_end,	// end of segment space
    void		* dest_ptr,	// pointer to destination
    u32			dest_size	// size of destination
)
{
    DASSERT( seg );
    DASSERT( seg_end );
    DASSERT( dest_ptr );
    DASSERT( dest_size >= 8 );
    DASSERT( !(dest_size&3) );

    u8 * dest = dest_ptr;
    memset(dest,0,dest_size);

    while ( seg != seg_end )
    {
	const u32 offset = ntohl(seg->offset);
	const u32 size   = ntohl(seg->size);
	if (!size)
	    break;

	noPRINT("SEG: %p: %x+%x\n",seg,offset,size);

	if ( (u8*)seg >= (u8*)seg_end || offset + size > dest_size )
	{
	    PRINT("seg=%p..%p, off=%x, size=%x, end=%x/%x\n",
		seg, seg_end, offset, size, offset + size, dest_size );
	    return ERROR0(ERR_WIA_INVALID,
		"Invalid WIA data segment: %s\n",sf->f.fname);
	}

	memcpy( dest + offset, seg->data, size );
	seg = (wia_segment_t*)( seg->data + size );
    }
    
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError read_data
(
    SuperFile_t		* sf,		// source file
    u64			file_offset,	// file offset
    u32			file_data_size,	// expected file data size
    bool		have_except,	// true: data contains exception list and
					// the exception list is stored in 'tempbuf'
    void		* inbuf,	// valid pointer to data
    u32			inbuf_size	// size of data to read
)
{   
    DASSERT( sf );
    DASSERT( sf->wia );
    DASSERT( inbuf );
    DASSERT( inbuf_size );

    wia_controller_t * wia = sf->wia;
    DASSERT(wia);

    if ( file_data_size > 2 * tempbuf_size )
	return ERROR0(ERR_WIA_INVALID,
	    "WIA chunk size too large: %s\n",sf->f.fname);

    bool align_except = false;
    u32 data_bytes_read = 0;

    u8  * dest    = have_except ? tempbuf : inbuf;
    u32 dest_size = have_except ? tempbuf_size : inbuf_size;

    switch ((wd_compression_t)wia->disc.compression)
    {
      //----------------------------------------------------------------------

      case WD_COMPR_NONE:
      {
	noPRINT(">> READ NONE: %9llx, %6x => %6x, except=%d, dest=%p, iobuf=%p\n",
		file_offset, file_data_size, inbuf_size, have_except, dest, tempbuf );

	if ( file_data_size > dest_size )
	    return ERROR0(ERR_WIA_INVALID,
		"WIA chunk size too large: %s\n",sf->f.fname);

	enumError err = ReadAtF( &sf->f, file_offset, dest, file_data_size );
	if (err)
	    return err;
	data_bytes_read = file_data_size;
	align_except = true;
      }
      break;

      //----------------------------------------------------------------------

      case WD_COMPR_PURGE:
      {
	enumError err = ReadAtF( &sf->f, file_offset, tempbuf, file_data_size );
	if (err)
	    return err;

	if ( file_data_size <= WII_HASH_SIZE )
	    return ERROR0(ERR_WIA_INVALID,
		"Invalid WIA data size: %s\n",sf->f.fname);
		
	file_data_size -= WII_HASH_SIZE;
	sha1_hash_t hash;
	SHA1(tempbuf,file_data_size,hash);
	if (memcmp(hash,tempbuf+file_data_size,WII_HASH_SIZE))
	{
	    HEXDUMP16(0,0,inbuf,16);
	    HEXDUMP(0,0,0,-WII_HASH_SIZE,tempbuf+file_data_size,WII_HASH_SIZE);
	    HEXDUMP(0,0,0,-WII_HASH_SIZE,hash,WII_HASH_SIZE);
	    return ERROR0(ERR_WIA_INVALID,
		"SHA1 check for WIA data segment failed: %s\n",sf->f.fname);
	}
    
	const u32 except_size
	    = have_except ? calc_except_size(tempbuf,wia->chunk_groups) + 3 & ~(u32)3 : 0;
	wia_segment_t * seg = (wia_segment_t*)( tempbuf + except_size );
	err = expand_segments( sf, seg, tempbuf+file_data_size,
				inbuf, inbuf_size );
	if (err)
	    return err;
	data_bytes_read = inbuf_size; // extraction is ok
	have_except = false; // no more exception handling needed
      }
      break;

      //----------------------------------------------------------------------

      case WD_COMPR_BZIP2:

 #ifdef NO_BZIP2
	return ERROR0(ERR_NOT_IMPLEMENTED,
			"No WIA/BZIP2 support for this release! Sorry!\n");
 #else
      { 
	ASSERT(sf->f.fp);
	enumError err = SeekF(&sf->f,file_offset);
	if (err)
	    return err;

	BZIP2_t bz;
	err = DecBZIP2_Open(&bz,&sf->f);
	if (err)
	    return err;

	err = DecBZIP2_Read(&bz,dest,dest_size,&data_bytes_read);
	if (err)
	    return err;

	err = DecBZIP2_Close(&bz);
	if (err)
	    return err;
      }
      break;

 #endif // !NO_BZIP2

      //----------------------------------------------------------------------

      case WD_COMPR_LZMA:
      {
	noPRINT("SEEK TO %llx=%llu\n",file_offset,file_offset);
	enumError err = SeekF(&sf->f,file_offset);
	if (err)
	    return err;

	err = DecLZMA_File2Buf( sf, file_data_size, dest, dest_size,
				&data_bytes_read, wia->disc.compr_data );
	if (err)
	    return err;
      }
      break;

      //----------------------------------------------------------------------

      case WD_COMPR_LZMA2:
      {
	enumError err = SeekF(&sf->f,file_offset);
	if (err)
	    return err;

	err = DecLZMA2_File2Buf( sf, file_data_size, dest, dest_size,
				&data_bytes_read, wia->disc.compr_data );
	if (err)
	    return err;
      }
      break;

      //----------------------------------------------------------------------

      // no default case defined
      //	=> compiler checks the existence of all enum values

      case WD_COMPR__N:
	ASSERT(0);
    }
    
    if (have_except)
    {
	u32 except_size = calc_except_size(dest,wia->chunk_groups);
	if (align_except)
	    except_size = except_size + 3 & ~(u32)3;
	noPRINT("%u exceptions, size=%u\n",be16(dest),except_size);

	data_bytes_read -= except_size;
	DASSERT( dest != inbuf );
	DASSERT( dest == tempbuf );
	memcpy( inbuf, dest + except_size, inbuf_size );
	//HEXDUMP16(0,1,inbuf,16);
    }

    if ( data_bytes_read != inbuf_size )
	    return ERROR0(ERR_WIA_INVALID,
		"WIA chunk size miss match [%x,%x]: %s\n",
			data_bytes_read, inbuf_size, sf->f.fname );

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError read_gdata
(
    SuperFile_t		* sf,		// source file
    u32			group,		// group index
    u32			size,		// group size
    bool		have_except	// true: data contains exception list and
					// the exception list is stored in tempbuf
)
{
    DASSERT(sf);
    DASSERT(sf->wia);
    wia_controller_t * wia = sf->wia;

    if ( group >= wia->group_used || size > wia->gdata_size )
	return ERROR0(ERR_WIA_INVALID,
			"Access to invalid group: %s\n",sf->f.fname);


    wia->gdata_group = group;
    wia_group_t * grp = wia->group + group;
    u32 gsize = ntohl(grp->data_size);
    if (gsize)
    {
	memset(wia->gdata+size,0,wia->gdata_size-size);
	return read_data( sf, (u64)ntohl(grp->data_off4)<<2,
			gsize, have_except, wia->gdata, size );
    }
    
    memset(wia->gdata,0,wia->gdata_size);
    if (have_except)
	memset(tempbuf,0,sizeof(wia_except_list_t));
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError read_part_gdata
(
    SuperFile_t		* sf,		// source file
    u32			part_index,	// partition index
    u32			group,		// group index
    u32			size		// group size
)
{
    DASSERT(sf);
    DASSERT(sf->wia);

    noPRINT("SIZE = %x -> %x\n", size, size / WII_SECTOR_SIZE * WII_SECTOR_DATA_SIZE );
    enumError err = read_gdata( sf, group,
				size / WII_SECTOR_SIZE * WII_SECTOR_DATA_SIZE, true );
    if (err)
	return err;
    
    wia_controller_t * wia = sf->wia;
    DASSERT( part_index < wia->disc.n_part );
    if ( wia->gdata_part != part_index )
    {
	wia->gdata_part = part_index;
	wd_aes_set_key(&wia->akey,wia->part[part_index].part_key);
    }


    //----- process hash and exceptions

    u8 * hashtab0 = tempbuf + tempbuf_size - WII_GROUP_HASH_SIZE * wia->chunk_groups;
    u8 * hashtab = hashtab0;

    int g;
    wia_except_list_t * except_list = (wia_except_list_t*)tempbuf;
    u8 * gdata = wia->gdata;
    for ( g = 0;
	  g < wia->chunk_groups;
	  g++, gdata += WII_GROUP_DATA_SIZE, hashtab += WII_GROUP_HASH_SIZE )
    {
	DASSERT( hashtab + WII_GROUP_HASH_SIZE <= tempbuf + tempbuf_size );
	memset(hashtab,0,WII_GROUP_HASH_SIZE);
	wd_calc_group_hashes(gdata,hashtab,0,0);

	u32 n_except = ntohs(except_list->n_exceptions);
	wia_exception_t * except = except_list->exception;
	if (n_except)
	{
	 #if WATCH_GROUP >= 0 && defined(TEST)
	    if ( wia->gdata_group == WATCH_GROUP && g == WATCH_SUB_GROUP )
	    {
		FILE * f = fopen("pool/read.except.dump","wb");
		if (f)
		{
		    const size_t sz = sizeof(wia_exception_t);
		    HexDump(f,0,0,9,sz,except_list,sizeof(except_list));
		    HexDump(f,0,0,9,sz,except_list->exception,n_except*sz);
		    fclose(f);
		}
	    }
	 #endif

	    noPRINT("%u exceptions for group %u\n",n_except,group);
	    for ( ; n_except > 0; n_except--, except++  )
	    {
		noPRINT_IF(wia->gdata_group == WATCH_GROUP && g == WATCH_SUB_GROUP,
			    "EXCEPT: %4x: %02x %02x %02x %02x\n",
			    ntohs(except->offset), except->hash[0],
			    except->hash[1], except->hash[2], except->hash[3] );
		DASSERT( ntohs(except->offset) + WII_HASH_SIZE <= WII_GROUP_HASH_SIZE );
		memcpy( hashtab + ntohs(except->offset), except->hash, WII_HASH_SIZE );
	    }
	}
	except_list = (wia_except_list_t*)except;
	DASSERT( (u8*)except_list < hashtab0 );
    }
    DASSERT( hashtab == tempbuf + tempbuf_size );

 #if WATCH_GROUP >= 0 && defined(TEST)
    if ( wia->gdata_group == WATCH_GROUP )
    {
	PRINT("##### WATCH GROUP #%u #####\n",WATCH_GROUP);
	FILE * f = fopen("pool/read.calc.dump","wb");
	if (f)
	{
	    HexDump(f,0,0,9,16,hashtab,WII_GROUP_HASH_SIZE*wia->chunk_groups);
	    fclose(f);
	}
    }
 #endif


    //----- encrpyt and join data

 #if WATCH_GROUP >= 0 && defined(TEST)
    if ( wia->gdata_group == WATCH_GROUP )
    {
	FILE * f = fopen("pool/read.split.dump","wb");
	if (f)
	{
	    HexDump(f,0,0,9,16,wia->gdata,WII_GROUP_DATA_SIZE*wia->chunk_groups);
	    fclose(f);
	}

	f = fopen("pool/read.hash.dump","wb");
	if (f)
	{
	    HexDump(f,0,0,9,16,hashtab0,WII_GROUP_HASH_SIZE*wia->chunk_groups);
	    fclose(f);
	}
    }
 #endif

    if ( wia->encrypt )
	wd_encrypt_sectors(0,&wia->akey,wia->gdata,
				hashtab0,wia->gdata,wia->chunk_sectors);
    else
	wd_join_sectors(wia->gdata,hashtab0,wia->gdata,wia->chunk_sectors);


 #if WATCH_GROUP >= 0 && defined(TEST)
    if ( wia->gdata_group == WATCH_GROUP )
    {
	FILE * f = fopen("pool/read.all.dump","wb");
	if (f)
	{
	    HexDump(f,0,0,9,16,wia->gdata,wia->chunk_size);
	    fclose(f);
	}
    }
 #endif

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    ReadWIA()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError ReadWIA 
(
    struct SuperFile_t	* sf,		// source file
    off_t		off,		// file offset
    void		* p_buf,	// destination buffer
    size_t		count		// number of bytes to read
)
{
    ASSERT(sf);
    ASSERT(sf->wia);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# ReadWIA()",
		GetFD(&sf->f), GetFP(&sf->f),
		(u64)off, (u64)off+count, count, "" );

    char * buf = p_buf;
    memset(buf,0,count);

    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    wia_controller_t * wia = sf->wia;
    if (!wia->is_valid)
	return ERR_WIA_INVALID;

    const u64 off2 = off + count;
    const wd_memmap_item_t * item = wia->memmap.item;
    const wd_memmap_item_t * item_end = item + wia->memmap.used;

    for ( ; item < item_end && item->offset < off2; item++ )
    {
      const u64 end = item->offset + item->size;
      noTRACE("> off=%llx..%llx, item=%llx..%llx\n", (u64)off, off2, item->offset, end );
      if ( item->offset < off2 && end > off )
      {
	u64 overlap1 = item->offset > off ? item->offset : off;
	const u64 overlap2 = end < off2 ? end : off2;
	noTRACE(" -> %llx .. %llx\n",overlap1,overlap2);

	switch (item->mode)
	{
	 case WIA_MM_HEADER_DATA:
	    DASSERT(item->data);
	    noPRINT("> COPY DATA: %9llx .. %9llx\n",overlap1,overlap2);
	    memcpy( buf + (overlap1-off),
		    (u8*)item->data + (overlap1-item->offset),
		    overlap2 - overlap1 );
	    break;


	 case WIA_MM_RAW_GDATA:
	    {
		DASSERT( item->index >= 0 && item->index < wia->raw_data_used );
		wia_raw_data_t * rdata = wia->raw_data + item->index;
		while ( overlap1 < overlap2 )
		{
		    // align content on WII_SECTOR_SIZE!

		    const int base_sector = item->offset / WII_SECTOR_SIZE;
		    const int sector      = overlap1 / WII_SECTOR_SIZE - base_sector;
		    const int base_group  = sector / wia->chunk_sectors;
		    const int group       = base_group + ntohl(rdata->group_index);

		    u64 base_off = base_sector * (u64)WII_SECTOR_SIZE
				 + base_group  * (u64)wia->chunk_size;
		    u64 end_off  = base_off + wia->chunk_size;
		    if ( end_off > end )
			 end_off = end;

		    noPRINT("\tWIA_MM_RAW_GDATA: s=%d,%d, g=%d,%d/%d, off=%llx..%llx/%llx\n",
			    base_sector, sector,
			    base_group, group, wia->group_used,
			    base_off, end_off, end );
		    DASSERT( base_group < ntohl(rdata->n_groups) );
		    DASSERT( group >= 0 && group < wia->group_used );

		    if ( group != wia->gdata_group )
		    {
			noPRINT("----- SETUP RAW%4u GROUP %4u/%4u>%4u, off=%9llx, size=%6llx\n",
				item->index, base_group, ntohl(rdata->n_groups), group,
				base_off, end_off - base_off );
			enumError err = read_gdata( sf, group, end_off - base_off, false );
			DASSERT( group == wia->gdata_group );
			if (err)
			    return err;
		    }

		    if ( end_off > overlap2 )
			 end_off = overlap2;

		    noPRINT("> READ RAW DATA:"
			    " %llx .. %llx -> %llx + %llx, base = %9llx + %6x\n",
				overlap1, end_off,
				overlap1 - base_off, end_off - overlap1,
				base_off, wia->gdata_used );
		    memcpy( buf + (overlap1-off),
			    wia->gdata + ( overlap1 - base_off ),
			    end_off - overlap1 );
		    overlap1 = end_off;
		}
	    }
	    break;


	 case WIA_MM_PART_GDATA_0:
	 case WIA_MM_PART_GDATA_1:
	    {
		DASSERT( item->index >= 0 && item->index < wia->disc.n_part );
		wia_part_t * part = wia->part + item->index;
		wia_part_data_t * pd = part->pd + ( item->mode - WIA_MM_PART_GDATA_0 );

		while ( overlap1 < overlap2 )
		{
		    int group = ( overlap1 - item->offset ) / wia->chunk_size;
		    DASSERT( group < pd->n_groups );
		    u64 base_off = item->offset + group * (u64)wia->chunk_size;
		    u64 end_off  = base_off + wia->chunk_size;
		    if ( end_off > end )
			 end_off = end;

		    group += pd->group_index;
		    DASSERT( group >= 0 && group < wia->group_used );

		    if ( group != wia->gdata_group || item->index != wia->gdata_part )
		    {
			noPRINT("----- SETUP PART%3u GROUP %4u/%4u>%4u, off=%9llx, size=%6llx\n",
				item->index,
				group - pd->group_index, pd->n_groups, group,
				base_off, end_off-base_off );
			enumError err
			    = read_part_gdata( sf, item->index, group, end_off-base_off );
			if (err)
			    return err;
		    }

		    if ( end_off > overlap2 )
			 end_off = overlap2;

		    noPRINT("> READ PART DATA:"
			    " %llx .. %llx -> %llx + %llx, base = %9llx + %6x\n",
				overlap1, end_off,
				overlap1 - base_off, end_off - overlap1,
				base_off, wia->gdata_used );
		    memcpy( buf + (overlap1-off),
			    wia->gdata + ( overlap1 - base_off ),
			    end_off - overlap1 );
		    overlap1 = end_off;
		}
	    }
	    break;


	  default:
	    return ERROR0(ERR_INTERNAL,0);
	}
      }
    }
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			DataBlockWIA()			///////////////
///////////////////////////////////////////////////////////////////////////////

off_t DataBlockWIA
	( SuperFile_t * sf, off_t off, size_t hint_align, off_t * block_size )
{
    // [[2do]] [datablock] : analyze chunks

    ASSERT(sf);
    ASSERT(sf->wia);

    wia_controller_t * wia = sf->wia;
    if (!wia->is_valid)
	return DataBlockStandard(sf,off,hint_align,block_size);

    const wd_memmap_item_t * item = wia->memmap.item;
    const wd_memmap_item_t * item_end = item + wia->memmap.used;

    for ( ; item < item_end && off >= item->offset + item->size; item++ )
	PRINT("%llx %llx+%llx\n",(u64)off,item->offset,item->size);

    if ( off < item->offset )
	 off = item->offset;

    if (block_size)
    {
	if ( hint_align < HD_BLOCK_SIZE )
	    hint_align = HD_BLOCK_SIZE;

	item_end--;
	while ( item < item_end )
	{
	    // [[2do]] this loops ends always with item==item_end
	    if ( item[1].offset - (item->offset + item->size) >= hint_align )
		break;
	    item++;
	}
	*block_size = item->offset + item->size - off;
    }

    return off;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup read WIA			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SetupReadWIA 
(
    struct SuperFile_t	* sf		// file to setup
)
{
    PRINT("#W# SetupReadWIA(%p) file=%d/%p, wia=%p wbfs=%p v=%s/%s\n",
		sf, GetFD(&sf->f), GetFP(&sf->f),
		sf->wia, sf->wbfs,
		PrintVersionWIA(0,0,WIA_VERSION_READ_COMPATIBLE),
		PrintVersionWIA(0,0,WIA_VERSION) );
    ASSERT(sf);

    if (sf->wia)
	return ERROR0(ERR_INTERNAL,0);


    //----- setup controller

    wia_controller_t * wia = CALLOC(1,sizeof(*wia));
    sf->wia = wia;
    wia->gdata_group = wia->gdata_part = -1;  // reset gdata
    wia->encrypt = encoding & ENCODE_ENCRYPT || !( encoding & ENCODE_DECRYPT );

    AllocBufferWIA(wia,WIA_BASE_CHUNK_SIZE,false,false);


    //----- read and check file header
    
    wia_file_head_t *fhead = &wia->fhead;
    enumError err = ReadAtF(&sf->f,0,fhead,sizeof(*fhead));
    if (err)
	return err;

    const bool is_wia = IsWIA(fhead,sizeof(*fhead),0,0,0);
    wia_ntoh_file_head(fhead,fhead);
    if ( !is_wia || fhead->disc_size > MiB )
	return ERROR0(ERR_WIA_INVALID,"Invalid file header: %s\n",sf->f.fname);

    if ( WIA_VERSION < fhead->version_compatible
	|| fhead->version < WIA_VERSION_READ_COMPATIBLE )
    {
	if ( WIA_VERSION_READ_COMPATIBLE < WIA_VERSION )
	    return ERROR0(ERR_WIA_INVALID,
		"WIA version %s is not supported (compatible %s .. %s): %s\n",
		PrintVersionWIA(0,0,fhead->version),
		PrintVersionWIA(0,0,WIA_VERSION_READ_COMPATIBLE),
		PrintVersionWIA(0,0,WIA_VERSION),
		sf->f.fname );
	else
	    return ERROR0(ERR_WIA_INVALID,
		"WIA version %s is not supported (%s expected): %s\n",
		PrintVersionWIA(0,0,fhead->version),
		PrintVersionWIA(0,0,WIA_VERSION),
		sf->f.fname );
    }


    //----- check file size

    if ( sf->f.st.st_size < fhead->wia_file_size )
	SetupSplitWFile(&sf->f,OFT_WIA,0);

    if ( sf->f.st.st_size != fhead->wia_file_size )
	return ERROR0(ERR_WIA_INVALID,
		"Wrong file size %llu (%llu expected): %s\n",
		(u64)sf->f.st.st_size, fhead->wia_file_size, sf->f.fname );


    //----- read and check disc info

    DASSERT( fhead->disc_size   <= tempbuf_size );
    DASSERT( sizeof(wia_disc_t) <= tempbuf_size );

    memset(tempbuf,0,sizeof(wia_disc_t));
    ReadAtF(&sf->f,sizeof(*fhead),tempbuf,fhead->disc_size);
    if (err)
	return err;

    sha1_hash_t hash;
    SHA1(tempbuf,fhead->disc_size,hash);
    if (memcmp(hash,fhead->disc_hash,sizeof(hash)))
	return ERROR0(ERR_WIA_INVALID,
	    "Hash error for disc area: %s\n",sf->f.fname);

    wia_disc_t *disc = &wia->disc;
    wia_ntoh_disc(disc,(wia_disc_t*)tempbuf);

    AllocBufferWIA(wia,disc->chunk_size,false,false);
    if ( wia->chunk_size != disc->chunk_size )
	return ERROR0(ERR_WIA_INVALID,
	    "Only multiple of %s, but not %s, are supported as a chunk size: %s\n",
		wd_print_size_1024(0,0,wia->chunk_size,false),
		wd_print_size_1024(0,0,disc->chunk_size,false), sf->f.fname );


    //----- read and check partition header

    const u32 load_part_size = disc->part_t_size * disc->n_part;
    if ( load_part_size > tempbuf_size )
	return ERROR0(ERR_WIA_INVALID,
	    "Total partition header size too large: %s\n",sf->f.fname);

    ReadAtF(&sf->f,disc->part_off,tempbuf,load_part_size);
    if (err)
	return err;

    SHA1(tempbuf,load_part_size,hash);
    if (memcmp(hash,disc->part_hash,sizeof(hash)))
	return ERROR0(ERR_WIA_INVALID,
	    "Hash error for partition header: %s\n",sf->f.fname);

    wia->part = CALLOC(disc->n_part,sizeof(wia_part_t));

    int ip;
    const u8 * src = tempbuf;
    int shortage = sizeof(wia_part_t) - disc->part_t_size;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wia_part_t *part = wia->part + ip;
	wia_ntoh_part(part,(wia_part_t*)src);
	if ( shortage > 0 )
	    memset( (u8*)part + disc->part_t_size, 0, shortage );
	src += disc->part_t_size;
    }


    //----- check compression method

    switch ((wd_compression_t)disc->compression)
    {
	case WD_COMPR__N:
	case WD_COMPR_NONE:
	case WD_COMPR_PURGE:
	    // nothing to do
	    break;

	case WD_COMPR_BZIP2:
	 #ifdef NO_BZIP2
	    return ERROR0(ERR_NOT_IMPLEMENTED,
			"No bzip2 support for this release! Sorry!\n");
	 #else
	    wia->memory_usage += CalcMemoryUsageBZIP2(disc->compr_level,false);
	    PRINT("DISABLE CACHE & OPEN STREAM\n");
	    ClearCache(&sf->f);
	    OpenStreamWFile(&sf->f);
	 #endif
	    break;

	case WD_COMPR_LZMA:
	    wia->memory_usage += CalcMemoryUsageLZMA(disc->compr_level,false);
	    break;

	case WD_COMPR_LZMA2:
	    wia->memory_usage += CalcMemoryUsageLZMA2(disc->compr_level,false);
	    break;

	default:
	    return ERROR0(ERR_NOT_IMPLEMENTED,
			"No support for compression method #%u (%x/hex, %s)\n",
			disc->compression, disc->compression,
			wd_get_compression_name(disc->compression,"unknown") );
    }


    //----- read and check raw data table

    PRINT("** RAW DATA TABLE: n=%u, off=%llx, size=%x\n",
		disc->n_raw_data, disc->raw_data_off, disc->raw_data_size );

    if (disc->n_raw_data)
    {
	wia->raw_data_used = disc->n_raw_data;
	const u32 raw_data_len = wia->raw_data_used * sizeof(wia_raw_data_t);
	wia->raw_data = MALLOC(raw_data_len);

	err = read_data( sf, disc->raw_data_off, disc->raw_data_size,
			 0, wia->raw_data, raw_data_len );
	if (err)
	    return err;

	wia->memory_usage += wia->raw_data_size * sizeof(*wia->raw_data);
    }


    //----- read and check group table

    PRINT("** GROUP TABLE: n=%u, off=%llx, size=%x\n",
		disc->n_groups, disc->group_off, disc->group_size );

    if (disc->n_groups)
    {
	wia->group_used = disc->n_groups;
	const u32 group_len = wia->group_used * sizeof(wia_group_t);
	wia->group = MALLOC(group_len);

	err = read_data( sf, disc->group_off, disc->group_size,
			 0, wia->group, group_len );
	if (err)
	    return err;

	wia->memory_usage += wia->group_size * sizeof(*wia->group);
    }


    //----- setup memory map

    wd_memmap_item_t * it;

    it = wd_insert_memmap(&wia->memmap,WIA_MM_HEADER_DATA,0,sizeof(disc->dhead));
    DASSERT(it);
    it->data = disc->dhead;
    snprintf(it->info,sizeof(it->info),"Disc header");

    it = wd_insert_memmap(&wia->memmap,WIA_MM_EOF,wia->fhead.iso_file_size,0);
    DASSERT(it);
    snprintf(it->info,sizeof(it->info),"--- end of file ---");
    
    const int g_fw = sprintf(iobuf,"%u",disc->n_groups);

    //----- setup memory map: partitions

    wia_part_t * part = wia->part;
    for ( ip = 0; ip < disc->n_part; ip++, part++ )
    {
	int id;
	for ( id = 0; id < sizeof(part->pd)/sizeof(part->pd[0]); id++ )
	{
	    wia_part_data_t * pd = part->pd + id;
	    const u64 size = pd->n_sectors * (u64)WII_SECTOR_SIZE;
	    if (size)
	    {
		it = wd_insert_memmap(&wia->memmap,WIA_MM_PART_GDATA_0+id,
				pd->first_sector * (u64)WII_SECTOR_SIZE, size );
		DASSERT(it);
		it->index = ip;
		const u64 compr_size = calc_group_size(wia,pd->group_index,pd->n_groups);
		snprintf(it->info,sizeof(it->info),
				"Part%3u,%5u chunk%s @%0*u, %s -> %s",
				it->index,
				pd->n_groups, pd->n_groups == 1 ? " " : "s",
				g_fw, pd->group_index,
				wd_print_size_1024(0,0,size,true),
				wd_print_size_1024(0,0,compr_size,true) );
	    }
	}
    }


    //----- setup memory map: raw data

    for ( ip = 0; ip < wia->raw_data_used; ip++ )
    {
	wia_raw_data_t * rd = wia->raw_data + ip;
	const u64 size = ntoh64(rd->raw_data_size);
	it = wd_insert_memmap(&wia->memmap,WIA_MM_RAW_GDATA,
				ntoh64(rd->raw_data_off), size );
	DASSERT(it);
	it->index = ip;
	const u32 group_index = ntohl(rd->group_index);
	const u32 n_groups = ntohl(rd->n_groups);
	const u64 compr_size = calc_group_size(wia,group_index,n_groups);
	snprintf(it->info,sizeof(it->info),
			"RAW%4u,%5u chunk%s @%0*u, %s -> %s",
			it->index,
			n_groups, n_groups == 1 ? " " : "s",
			g_fw, group_index,
			wd_print_size_1024(0,0,size,true),
			wd_print_size_1024(0,0,compr_size,true) );
    }

    
    //----- logging

    if ( verbose > 2 )
    {
	printf("  Compression mode: %s (method %s, level %u, chunk size %u MiB, mem ~%u MiB)\n",
		wd_print_compression(0,0,disc->compression,
				disc->compr_level,disc->chunk_size,2),
		wd_get_compression_name(disc->compression,"?"),
		disc->compr_level, disc->chunk_size / MiB,
		( wia->memory_usage + MiB/2 ) / MiB );
	fflush(0);
    }

    if ( logging > 0 )
    {
	printf("\nWIA memory map:\n\n");
	wd_print_memmap(stdout,3,&wia->memmap);
	putchar('\n');
    }


    //----- finish setup

    sf->file_size = fhead->iso_file_size;
    wia->is_valid = true;
    SetupIOD(sf,OFT_WIA,OFT_WIA);

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  write helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static wia_segment_t * calc_segments
(
    wia_segment_t	* seg,		// destination segment pointer
    void		* seg_end,	// end of segment space
    const void		* src_ptr,	// pointer to source
    u32			src_size	// size of source
)
{
    DASSERT( seg );
    DASSERT( seg_end );
    DASSERT( src_ptr );
    DASSERT( src_size >= 8 );
    DASSERT( !(src_size&3) );

    const u32 * src = (u32*)src_ptr;
    const u32 * src_end  = src + src_size/4;
    while ( src_end > src && !src_end[-1] )
	src_end--;
    const u32 * src_end2 = src + 2 >= src_end ? src : src_end - 2;
    noPRINT("SRC: %p, end=%zx,%zx\n", src, src_end2-src, src_end-src );

    while ( src < src_end )
    {
	// skip null data
	while ( src < src_end && !*src )
	    src++;
	if ( src == src_end )
	    break;
	const u32 * src_beg = src;

	// find next hole
	while ( src < src_end2 )
	{
	    if (src[2])
		src += 3;
	    else if (src[1])
		src += 2;
	    else if (src[0])
		src++;
	    else
		break;
	}
	if ( src >= src_end2 )
	    src = src_end;

	const u32 offset	= (u8*)src_beg - (u8*)src_ptr;
	seg->offset		= htonl(offset);
	const u32 size		= (u8*)src - (u8*)src_beg;
	seg->size		= htonl(size);
	noPRINT("SEG: %p: %x+%x\n",seg,offset,size);

	DASSERT(!(size&3));
	DASSERT( seg->data + size < (u8*)seg_end );
	memcpy(seg->data,src_beg,size);
	seg = (wia_segment_t*)( seg->data + size );
    }

    if ( (u8*)seg > (u8*)seg_end )
	ERROR0(ERR_INTERNAL,"buffer overflow");

    return seg;
}

///////////////////////////////////////////////////////////////////////////////

static enumError write_data
(
    struct SuperFile_t	* sf,		// destination file
    wia_except_list_t	* except,	// NULL or exception list
    const void		* data_ptr,	// NULL or u32-aligned pointer to data
    u32			data_size,	// size of data, u32-aligned
    int			group,		// >=0: write group data
    u32			* write_count	// not NULL: store written data count
)
{
    DASSERT( sf );
    DASSERT(data_ptr);
    DASSERT(!(data_size&3));

    wia_controller_t * wia = sf->wia;
    DASSERT(wia);

    u32 except_size = except ? calc_except_size(except,wia->chunk_groups) : 0;
    TRACE_IF( except_size > wia->chunk_groups * sizeof(wia_except_list_t),
		"%zd exceptions in group %d, size=%u=0x%x\n",
		( except_size - wia->chunk_groups * sizeof(wia_except_list_t) )
			/ sizeof(wia_exception_t),
		group, except_size, except_size );

    DefineProgressChunkSF(sf,data_size,data_size+except_size);

    u32 written = 0;
    switch((wd_compression_t)wia->disc.compression)
    {
      //----------------------------------------------------------------------

      case WD_COMPR_NONE:
      {
	if (except_size)
	{
	    except_size = except_size + 3 & ~(u32)3; // u32 alignment
	    enumError err = WriteAtF( &sf->f, wia->write_data_off, except, except_size );
	    if (err)
		return err;
	    written += except_size;
	}

	if (data_size)
	{
	    enumError err = WriteAtF( &sf->f, wia->write_data_off + except_size,
					data_ptr, data_size );
	    if (err)
		return err;
	    written += data_size;
	}

	noPRINT(">> WRITE NONE: %9llx, %6x+%6x => %6x, grp %d\n",
		    wia->write_data_off, except_size, data_size, written, group );
      }
      break;

      //----------------------------------------------------------------------

      case WD_COMPR_PURGE:
      {
	if (except_size)
	{
	    except_size = except_size + 3 & ~(u32)3; // u32 alignment
	    if ( (u8*)except != tempbuf )
		memmove(tempbuf,except,except_size);
	}

	wia_segment_t * seg1 = (wia_segment_t*)(tempbuf+except_size);
	wia_segment_t * seg2
	    = calc_segments( seg1, tempbuf + tempbuf_size,
				data_ptr, data_size );

	if ( except_size || seg2 > seg1+1 )
	{
	    written = except_size + ( (ccp)seg2 - (ccp)seg1 );
	    SHA1(tempbuf,written,tempbuf+written);
	    written += WII_HASH_SIZE;

	    enumError err = WriteAtF( &sf->f, wia->write_data_off, tempbuf, written );
	    if (err)
		return err;
	}

	noPRINT(">> WRITE PURGE: %9llx, %6x+%6x => %6x, grp %d\n",
		    wia->write_data_off, except_size, data_size, written, group );
      }
      break;

      //----------------------------------------------------------------------

      case WD_COMPR_BZIP2:
 #ifdef NO_BZIP2
	return ERROR0(ERR_NOT_IMPLEMENTED,
			"No WIA/BZIP2 support for this release! Sorry!\n");
 #else
      {
	ASSERT(sf->f.fp);
	enumError err = SeekF(&sf->f,wia->write_data_off);
	if (err)
	    return err;

	BZIP2_t bz;
	err = EncBZIP2_Open(&bz,&sf->f,opt_compr_level);
	if (err)
	    return err;

	err = EncBZIP2_Write(&bz,except,except_size);
	if (err)
	    return err;

	err = EncBZIP2_Write(&bz,data_ptr,data_size);
	if (err)
	    return err;

	err = EncBZIP2_Close(&bz,&written);
	if (err)
	    return err;

	noPRINT(">> WRITE BZIP2: %9llx, %6x+%6x => %6x, grp %d\n",
		    wia->write_data_off, except_size, data_size, written, group );

	sf->f.max_off = wia->write_data_off + written;
      }
      break;
 #endif // !NO_BZIP2

      //----------------------------------------------------------------------

      case WD_COMPR_LZMA:
      case WD_COMPR_LZMA2:
      {
	enumError err = SeekF(&sf->f,wia->write_data_off);
	if (err)
	    return err;

	DataArea_t area[3], *ap = area;
	if (except_size)
	{
	    ap->data = (u8*)except;
	    ap->size = except_size;
	    ap++;
	}
	if (data_size)
	{
	    ap->data = data_ptr;
	    ap->size = data_size;
	    ap++;
	}
	ap->data = 0;

	DataList_t list;
	SetupDataList(&list,area);

	err = wia->disc.compression == WD_COMPR_LZMA
		? EncLZMA_List2File (0,sf,opt_compr_level,false,true,&list,&written)
		: EncLZMA2_List2File(0,sf,opt_compr_level,false,true,&list,&written);
	if (err)
	    return err;

	noPRINT(">> WRITE LZMA: %9llx, %6x+%6x => %6x, grp %d\n",
		    wia->write_data_off, except_size, data_size, written, group );
      }
      break;

      //----------------------------------------------------------------------

      // no default case defined
      //	=> compiler checks the existence of all enum values

      case WD_COMPR__N:
	ASSERT(0);
    }


    if ( group >= 0 && group < wia->group_used )
    {
	wia_group_t * grp = wia->group + group;
	grp->data_off4 = htonl( written ? wia->write_data_off >> 2 : 0 );
	grp->data_size = htonl( written );
    }

    wia->write_data_off += written + 3 & ~3;
    if ( sf->f.bytes_written > wia->disc.chunk_size )
	sf->progress_trigger++;

    if (write_count)
	*write_count = written;

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static enumError write_part_data
(
    struct SuperFile_t	* sf		// destination file
)
{
    DASSERT(sf);
    DASSERT(sf->wia);

    wia_controller_t * wia = sf->wia;
    DASSERT( wia->gdata_group >= 0 && wia->gdata_group < wia->group_used );
    DASSERT( wia->gdata_part  >= 0 && wia->gdata_part  < wia->disc.n_part );


    //----- decrypt and split data

    wd_disc_t * wdisc = wia->wdisc;
    ASSERT(wdisc);
    ASSERT( wia->gdata_part < wdisc->n_part );
    wd_part_t * wpart = wdisc->part + wia->gdata_part;

 #if WATCH_GROUP >= 0 && defined(TEST)
    if ( wia->gdata_group == WATCH_GROUP )
    {
	PRINT("##### WATCH GROUP #%u #####\n",WATCH_GROUP);
	FILE * f = fopen("pool/write.all.dump","wb");
	if (f)
	{
	    HexDump(f,0,0,9,16,wia->gdata,wia->chunk_size);
	    fclose(f);
	}
    }
 #endif

    u8 * hashtab0 = tempbuf + tempbuf_size - WII_GROUP_HASH_SIZE * wia->chunk_groups;
    if ( wpart->is_encrypted )
	wd_decrypt_sectors(0,&wia->akey,
			wia->gdata,wia->gdata,hashtab0,wia->chunk_sectors);
    else
	wd_split_sectors(wia->gdata,wia->gdata,hashtab0,wia->chunk_sectors);

 #if WATCH_GROUP >= 0 && defined(TEST)
    if ( wia->gdata_group == WATCH_GROUP )
    {
	FILE * f = fopen("pool/write.split.dump","wb");
	if (f)
	{
	    HexDump(f,0,0,9,16,wia->gdata,WII_GROUP_DATA_SIZE*wia->chunk_groups);
	    fclose(f);
	}

	f = fopen("pool/write.hash.dump","wb");
	if (f)
	{
	    int g;
	    for ( g = 0; g < wia->chunk_groups; g++ )
	    {
		HexDump(f,0,0,9,16,hashtab0+WII_GROUP_HASH_SIZE*g,WII_GROUP_HASH_SIZE);
		fprintf(f,"---\f\n");
	    }
	    fclose(f);
	}
    }
 #endif


    //----- setup exceptions

    u8 * gdata = wia->gdata;
    u8 * hashtab1 = hashtab0;
    wia_except_list_t * except_list = (wia_except_list_t*)tempbuf;

    int g;
    for ( g = 0;
	  g < wia->chunk_groups;
	  g++, gdata += WII_GROUP_DATA_SIZE, hashtab1 += WII_GROUP_HASH_SIZE )
    {
	u8 hashtab2[WII_GROUP_HASH_SIZE];
	wd_calc_group_hashes(gdata,hashtab2,0,0);

     #if WATCH_GROUP >= 0 && defined(TEST)
	if ( wia->gdata_group == WATCH_GROUP && g == WATCH_SUB_GROUP )
	{
	    FILE * f = fopen("pool/write.calc.dump","wb");
	    if (f)
	    {
		HexDump(f,0,0,9,16,hashtab2,WII_GROUP_HASH_SIZE);
		fclose(f);
	    }
	}
     #endif

	int is;
	wia_exception_t * except = except_list->exception;
	noPRINT("** exc=%p,%p, gdata=%p, hash=%p\n",
		except_list, except, gdata, hashtab1 );
	for ( is = 0; is < WII_GROUP_SECTORS; is++ )
	{
	    wd_part_sector_t * sector1
		= (wd_part_sector_t*)( hashtab1 + is * WII_SECTOR_HASH_SIZE );
	    wd_part_sector_t * sector2
		= (wd_part_sector_t*)( hashtab2 + is * WII_SECTOR_HASH_SIZE );

	    int ih;
	    u8 * h1 = sector1->h0[0];
	    u8 * h2 = sector2->h0[0];
	    for ( ih = 0; ih < WII_N_ELEMENTS_H0; ih++ )
	    {
		if (memcmp(h1,h2,WII_HASH_SIZE))
		{
		    TRACE("%5u.%02u.H0.%02u -> %04zx,%04zx\n",
				wia->gdata_group, is, ih,
				h1 - hashtab1,  h2 - hashtab2 );
		    except->offset = htons(h1-hashtab1);
		    memcpy(except->hash,h1,sizeof(except->hash));
		    except++;
		}
		h1 += WII_HASH_SIZE;
		h2 += WII_HASH_SIZE;
	    }

	    h1 = sector1->h1[0];
	    h2 = sector2->h1[0];
	    for ( ih = 0; ih < WII_N_ELEMENTS_H1; ih++ )
	    {
		if (memcmp(h1,h2,WII_HASH_SIZE))
		{
		    TRACE("%5u.%02u.H1.%u  -> %04zx,%04zx\n",
				wia->gdata_group, is, ih,
				h1 - hashtab1,  h2 - hashtab2 );
		    except->offset = htons(h1-hashtab1);
		    memcpy(except->hash,h1,sizeof(except->hash));
		    except++;
		}
		h1 += WII_HASH_SIZE;
		h2 += WII_HASH_SIZE;
	    }

	    h1 = sector1->h2[0];
	    h2 = sector2->h2[0];
	    for ( ih = 0; ih < WII_N_ELEMENTS_H2; ih++ )
	    {
		if (memcmp(h1,h2,WII_HASH_SIZE))
		{
		    TRACE("%5u.%02u.H2.%u  -> %04zx,%04zx\n",
				wia->gdata_group, is, ih,
				h1 - hashtab1,  h2 - hashtab2 );
		    except->offset = htons(h1-hashtab1);
		    memcpy(except->hash,h1,sizeof(except->hash));
		    except++;
		}
		h1 += WII_HASH_SIZE;
		h2 += WII_HASH_SIZE;
	    }
	}
	except_list->n_exceptions = htons( except - except_list->exception );
	memset(except,0,sizeof(*except)); // no garbage data
	noPRINT_IF( except > except_list->exception,
			" + %zu excpetions in group %u.%u\n",
			except - except_list->exception,
			wia->gdata_group, g );

     #if WATCH_GROUP >= 0 && defined(TEST)
	if ( wia->gdata_group == WATCH_GROUP && g == WATCH_SUB_GROUP
		&& except > except_list->exception )
	{
	    FILE * f = fopen("pool/write.except.dump","wb");
	    if (f)
	    {
		const size_t sz = sizeof(wia_exception_t);
		HexDump(f,0,0,9,sz,except_list,sizeof(except_list));
		HexDump(f,0,0,9,sz,except_list->exception,
				(except - except_list->exception) * sz );
		fclose(f);
	    }
	}
     #endif

	except_list = (wia_except_list_t*)except;
	DASSERT( (u8*)except_list < hashtab0 );
    }
    noPRINT("## exc=%p,%p, gdata=%p, hash=%p\n",
		except_list, except_list, gdata, hashtab1 );


    //----- write data

    return write_data( sf, (wia_except_list_t*)tempbuf, wia->gdata,
			wia->gdata_used / WII_SECTOR_SIZE * WII_SECTOR_DATA_SIZE,
			wia->gdata_group, 0 );
}

///////////////////////////////////////////////////////////////////////////////

static enumError write_cached_gdata
(
    struct SuperFile_t	* sf,		// destination file
    int			new_part	// >=0: index of new partition
)
{
    DASSERT(sf);
    DASSERT(sf->wia);
    wia_controller_t * wia = sf->wia;

    enumError err = ERR_OK;
    if ( wia->gdata_group >= 0 && wia->gdata_group < wia->group_used )
    {
	if ( wia->gdata_part < 0 || wia->gdata_part >= wia->disc.n_part )
	{
	    err = write_data(sf, 0, wia->gdata, wia->gdata_used, wia->gdata_group, 0 );
	}
	else
	{
	    err = write_part_data(sf);
	}
    }

    // setup empty gdata

    wia->gdata_group = -1;

    if ( new_part < 0 )
    {
	memset(wia->gdata,0,wia->gdata_size);
	wia->gdata_part  = -1;
    }
    else
    {
	DASSERT( new_part < wia->disc.n_part );
	if ( wia->gdata_part != new_part )
	{
	    if (!empty_decrypted_sector_initialized)
	    {
		PRINT("SETUP empty_decrypted_sector\n");
		empty_decrypted_sector_initialized = true;

		wd_part_sector_t * empty = &empty_decrypted_sector;
		memset(empty,0,sizeof(empty_decrypted_sector));

		int i;
		SHA1(empty->data[0],WII_H0_DATA_SIZE,empty->h0[0]);
		for ( i = 1; i < WII_N_ELEMENTS_H0; i++ )
		    memcpy(empty->h0[i],empty->h0[0],WII_HASH_SIZE);

		SHA1(empty->h0[0],sizeof(empty->h0),empty->h1[0]);
		for ( i = 1; i < WII_N_ELEMENTS_H1; i++ )
		    memcpy(empty->h1[i],empty->h1[0],WII_HASH_SIZE);

		SHA1(empty->h1[0],sizeof(empty->h1),empty->h2[0]);
		for ( i = 1; i < WII_N_ELEMENTS_H2; i++ )
		    memcpy(empty->h2[i],empty->h2[0],WII_HASH_SIZE);
	    }

	    wia->gdata_part = new_part;
	    wd_aes_set_key(&wia->akey,wia->part[new_part].part_key);

	    if ( wia->wdisc && !wia->wdisc->part[new_part].is_encrypted )
		memcpy(&wia->empty_sector,&empty_decrypted_sector,sizeof(wia->empty_sector));
	    else
		wd_encrypt_sectors(0,&wia->akey,
			&empty_decrypted_sector,0,&wia->empty_sector,1);
	}

	int i;
	u8 * dest = wia->gdata;
	for ( i = 0; i < wia->chunk_sectors; i++, dest += WII_SECTOR_SIZE )
	    memcpy( dest, &wia->empty_sector, WII_SECTOR_SIZE );
    }

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    WriteWIA()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError WriteWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
)
{
    ASSERT(sf);
    ASSERT(sf->wia);
    ASSERT(sf->wia->is_writing);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# WriteWIA()",
		GetFD(&sf->f), GetFP(&sf->f),
		(u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx\n",(u64)sf->f.file_off,(u64)sf->f.max_off);

    if (SIGINT_level>1)
	return ERR_INTERRUPT;

    wia_controller_t * wia = sf->wia;
    const u64 off2 = off + count;
    const wd_memmap_item_t * item = wia->memmap.item;
    const wd_memmap_item_t * item_end = item + wia->memmap.used;

    for ( ; item < item_end && item->offset < off2; item++ )
    {
      const u64 end = item->offset + item->size;
      noPRINT("> off=%llx..%llx, item=%llx..%llx\n", (u64)off, off2, item->offset, end );
      if ( item->offset < off2 && end > off )
      {
	u64 overlap1 = item->offset > off ? item->offset : off;
	const u64 overlap2 = end < off2 ? end : off2;
	noPRINT("   -> %llx..%llx\n",overlap1,overlap2);

	switch (item->mode)
	{
	 case WIA_MM_HEADER_DATA:
	 case WIA_MM_CACHED_DATA:
	    DASSERT(item->data);
	    noPRINT("> COPY DATA: %9llx .. %9llx\n",overlap1,overlap2);
	    memcpy(	(u8*)item->data + (overlap1-item->offset),
		    (ccp)buf + (overlap1-off),
		    overlap2 - overlap1 );
	    break;

	 case WIA_MM_GROWING:
	    return ERROR0(ERR_INTERNAL,0);
	    break; // [[2do]]

	    // ... fall through ...

	 case WIA_MM_RAW_GDATA:
	    {
		DASSERT( item->index >= 0 && item->index < wia->raw_data_used );
		wia_raw_data_t * rdata = wia->raw_data + item->index;
		while ( overlap1 < overlap2 )
		{
		    // align content on WII_SECTOR_SIZE!

		    const int base_sector = item->offset / WII_SECTOR_SIZE;
		    const int sector      = overlap1 / WII_SECTOR_SIZE - base_sector;
		    const int base_group  = sector / wia->chunk_sectors;
		    const int group       = base_group + ntohl(rdata->group_index);

		    u64 base_off = base_sector * (u64)WII_SECTOR_SIZE
				 + base_group  * (u64)wia->chunk_size;
		    u64 end_off  = base_off + wia->chunk_size;
		    if ( end_off > end )
			 end_off = end;

		    noPRINT("\tWIA_MM_RAW_GDATA: s=%d,%d, g=%d,%d/%d, off=%llx..%llx/%llx\n",
			    base_sector, sector,
			    base_group, group, wia->group_used,
			    base_off, end_off, end );
		    DASSERT( base_group < ntohl(rdata->n_groups) );
		    DASSERT( group >= 0 && group < wia->group_used );

		    if ( group != wia->gdata_group )
		    {
			enumError err = write_cached_gdata(sf,-1);
			wia->gdata_group = group;
			wia->gdata_used  = end_off - base_off;
			noPRINT("----- SETUP RAW%4u GROUP %4u/%4u>%4u, off=%9llx, size=%6x\n",
				item->index, base_group, ntohl(rdata->n_groups), group,
				base_off, wia->gdata_used );
			if (err)
			    return err;
		    }

		    if ( end_off > overlap2 )
			 end_off = overlap2;

		    noPRINT("> COLLECT RAW DATA:"
			    " %llx .. %llx -> %llx + %llx, base = %9llx + %6x\n",
				overlap1, end_off,
				overlap1 - base_off, end_off - overlap1,
				base_off, wia->gdata_used );
		    memcpy( wia->gdata + ( overlap1 - base_off ),
			    (ccp)buf + (overlap1-off),
			    end_off - overlap1 );
		    overlap1 = end_off;
		}
	    }
	    break;

	 case WIA_MM_PART_GDATA_0:
	 case WIA_MM_PART_GDATA_1:
	    {
		DASSERT( item->index >= 0 && item->index < wia->disc.n_part );
		wia_part_t * part = wia->part + item->index;
		wia_part_data_t * pd = part->pd + ( item->mode - WIA_MM_PART_GDATA_0 );

		while ( overlap1 < overlap2 )
		{
		    const int base_group = ( overlap1 - item->offset ) / wia->chunk_size;
		    u64 base_off = item->offset + base_group * (u64)wia->chunk_size;
		    u64 end_off  = base_off + wia->chunk_size;
		    if ( end_off > end )
			 end_off = end;

		    const int group = base_group + pd->group_index;

		    if ( group != wia->gdata_group || item->index != wia->gdata_part )
		    {
			noPRINT("----- SETUP PART%3u GROUP %4u/%4u>%4u, off=%9llx, size=%6x\n",
				item->index,
				group - pd->group_index, pd->n_groups, group,
				base_off, wia->gdata_used );
			DASSERT( group >= 0 && group < wia->group_used );
			DASSERT( base_group >= 0 && base_group < pd->n_groups );

			enumError err = write_cached_gdata(sf,item->index);
			if (err)
			    return err;
			DASSERT( wia->gdata_part == item->index );

			wia->gdata_group = group;
			wia->gdata_used  = end_off - base_off;
		    }

		    if ( end_off > overlap2 )
			 end_off = overlap2;

		    noPRINT("> COLLECT PART DATA:"
			    " %llx .. %llx -> %llx + %llx, base = %9llx + %6x\n",
				overlap1, end_off,
				overlap1 - base_off, end_off - overlap1,
				base_off, wia->gdata_used );
		    memcpy( wia->gdata + ( overlap1 - base_off ),
			    (ccp)buf + (overlap1-off),
			    end_off - overlap1 );
		    overlap1 = end_off;
		}
	    }
	    break;

	  default:
	    if (!item->size)
		break;
	    PRINT("mode=%x, off=%llx, size=%llx\n",item->mode,item->offset,item->size);
	    return ERROR0(ERR_INTERNAL,0);
	}
      }
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteSparseWIA
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    const void		* buf,		// source buffer
    size_t		count		// number of bytes to write
)
{
    return off + count < WII_PART_OFF
		? WriteWIA(sf,off,buf,count)
		: SparseHelper(sf,off,buf,count,WriteWIA,WIA_MIN_HOLE_SIZE);
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteZeroWIA 
(
    struct SuperFile_t	* sf,		// destination file
    off_t		off,		// file offset
    size_t		count		// number of bytes to write
)
{
    ASSERT(sf);
    ASSERT(sf->wia);

    TRACE("#W# -----\n");
    TRACE(TRACE_RDWR_FORMAT, "#W# WriteZeroWIA()",
		GetFD(&sf->f), GetFP(&sf->f), (u64)off, (u64)off+count, count,
		off < sf->max_virt_off ? " <" : "" );
    TRACE(" - off = %llx,%llx,%llx\n",
		(u64)sf->f.file_off, (u64)sf->f.max_off,
		(u64)sf->max_virt_off);

    // [wia]
    return ERROR0(ERR_NOT_IMPLEMENTED,"WIA is not supported yet.\n");
}

///////////////////////////////////////////////////////////////////////////////

enumError FlushWIA
(
    struct SuperFile_t	* sf		// destination file
)
{
    ASSERT(sf);
    ASSERT(sf->wia);

    enumError err = ERR_OK;
    wia_controller_t * wia = sf->wia;
    if (wia->is_writing)
    {
	err = write_cached_gdata(sf,-1);
	if (!err)
	    err = FlushFile(sf);
    }

    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			setup write WIA			///////////////
///////////////////////////////////////////////////////////////////////////////

static wia_raw_data_t * need_raw_data
(
    wia_controller_t	* wia,		// valid pointer
    u32			grow_size,	// if field is full grow # elements
    u64			data_offset,	// offset of data
    u64			data_size,	// size of data
    u32			* return_n_sect	// not NULL && data_size>0: return n_sect
)
{
    DASSERT(wia);
    DASSERT( grow_size > 0 );
    if ( wia->raw_data_used == wia->raw_data_size )
    {
	wia->raw_data_size += grow_size;
	noPRINT("ALLOC %u RAW_DATA\n",wia->raw_data_size);
	wia->raw_data
	    = REALLOC( wia->raw_data, wia->raw_data_size * sizeof(*wia->raw_data) );
    }

    wia_raw_data_t * rdata = wia->raw_data + wia->raw_data_used++;
    memset(rdata,0,sizeof(*rdata));

    if (data_size)
    {
	const u32 sect1  = data_offset / WII_SECTOR_SIZE;
	const u32 sect2	 = ( data_offset + data_size + WII_SECTOR_SIZE - 1 ) / WII_SECTOR_SIZE;
	const u32 n_sect = sect2 - sect1;
	const u32 n_grp  = ( n_sect + wia->chunk_sectors - 1 ) / wia->chunk_sectors;

	rdata->raw_data_off	= hton64(data_offset);
	rdata->raw_data_size	= hton64(data_size);
	rdata->group_index	= htonl(wia->group_used);
	rdata->n_groups		= htonl(n_grp);
	wia->group_used		+= n_grp;

	if (return_n_sect)
	    *return_n_sect = n_sect;
    }

    return rdata;
}

///////////////////////////////////////////////////////////////////////////////

static enumError FinishSetupWriteWIA
(
    struct SuperFile_t	* sf		// file to setup
)
{
    DASSERT(sf);
    DASSERT(sf->wia);
    
    wia_controller_t * wia = sf->wia;
    wia_disc_t * disc = &wia->disc;


    //----- setup raw data area

    wd_memmap_item_t * it;

    it = wd_insert_memmap(&wia->memmap,WIA_MM_HEADER_DATA,0,sizeof(disc->dhead));
    DASSERT(it);
    it->data = disc->dhead;
    snprintf(it->info,sizeof(it->info),"Disc header");

    it = wia->memmap.item + wia->memmap.used - 1;
    u64 fsize = it->offset + it->size;
    if ( fsize < wia->fhead.iso_file_size )
	 fsize = wia->fhead.iso_file_size;
    it = wd_insert_memmap(&wia->memmap,WIA_MM_EOF,fsize,0);
    DASSERT(it);
    snprintf(it->info,sizeof(it->info),"--- end of file ---");

    if ( fsize < WIA_MAX_SUPPORTED_ISO_SIZE )
    {
	it = wd_insert_memmap(&wia->memmap,WIA_MM_GROWING,fsize,
				WIA_MAX_SUPPORTED_ISO_SIZE-fsize);
	DASSERT(it);
	snprintf(it->info,sizeof(it->info),"additional space");
    }

    u64 last_off = 0;
    for(;;)
    {
	bool dirty = false;
	const wd_memmap_item_t * item = wia->memmap.item;
	const wd_memmap_item_t * item_end = item + wia->memmap.used;
	for ( ; item < item_end; item++ )
	{
	    if ( last_off < item->offset )
	    {
		u64 size = item->offset - last_off;
		if ( last_off < WII_PART_OFF )
		{
		    // small chunk to allow fast access to disc header
		    const u32 max_size = WII_PART_OFF - last_off;
		    if ( size > max_size )
			 size = max_size;
		}
		u32 n_sect = 0;
		wia_raw_data_t * rdata
		    = need_raw_data( wia, 0x20, last_off, size, &n_sect );

		it = wd_insert_memmap(&wia->memmap,WIA_MM_RAW_GDATA,last_off,size);
		DASSERT(it);
		it->index = wia->raw_data_used - 1;
		const u32 n_groups = ntohl(rdata->n_groups);
		snprintf(it->info,sizeof(it->info),
			    "RAW #%u, %u sector%s, %u chunk%s, %s",
			    it->index,
			    n_sect, n_sect == 1 ? "" : "s",
			    n_groups, n_groups == 1 ? "" : "s",
			    wd_print_size_1024(0,0,size,false) );

		dirty = true;
		break;
	    }
	    last_off = item->offset + item->size;
	}
	if (!dirty)
	    break;
    }
    wia->growing = need_raw_data(wia,1,0,0,0);
    wia->memory_usage += wia->raw_data_size * sizeof(*wia->raw_data);
    sf->progress_add_total += wia->raw_data_size * sizeof(*wia->raw_data);


    //----- setup group area

    if (wia->group_used)
    {
	wia->group_size = wia->group_used;
	PRINT("ALLOC %u GROUPS\n",wia->group_size);
	wia->group = CALLOC(wia->group_size,sizeof(*wia->group));
	wia->memory_usage += wia->group_size * sizeof(*wia->group);
	sf->progress_add_total += wia->group_size * sizeof(*wia->group);
    }


    //----- logging

    if ( verbose > 1 )
	printf("  Compression mode: %s (method %s, level %u, chunk size %u MiB, mem ~%u MiB)\n",
		wd_print_compression(0,0,disc->compression,
				disc->compr_level,disc->chunk_size,2),
		wd_get_compression_name(disc->compression,"?"),
		disc->compr_level, disc->chunk_size / MiB,
		( wia->memory_usage + MiB/2 ) / MiB );

    if ( logging > 0 )
    {
	printf("\nWIA memory map:\n\n");
	wd_print_memmap(stdout,3,&wia->memmap);
	putchar('\n');
    }


    //----- finish setup

    wia->write_data_off	= sizeof(wia_file_head_t)
			+ sizeof(wia_disc_t)
			+ sizeof(wia_part_t) * disc->n_part;
    sf->progress_add_total += wia->write_data_off + sizeof(wia_part_t) * disc->n_part;

    wia->is_valid = true;
    SetupIOD(sf,OFT_WIA,OFT_WIA);

    //----- preallocate disc space

    if ( disc->compression >= WD_COMPR__FIRST_REAL )
    {
	if ( sf->src && !sf->raw_mode )
	{
	    wd_disc_t * disc = OpenDiscSF(sf->src,false,true);
	    if (disc)
		fsize = wd_count_used_disc_size(disc,1,0);
	}
	PreallocateF(&sf->f,0,fsize);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static void setup_dynamic_mem
(
    void			* param,	// user defined parameter
    struct wd_memmap_t		* mm,		// valid pointer to patch object
    struct wd_memmap_item_t	* item		// valid pointer to inserted item
)
{
    if ( item->mode == WIA_MM_CACHED_DATA )
    {
	PRINT("ALLOC PART HEADER #%u, size=%llx\n",item->index,item->size);
	item->data = CALLOC(1,item->size);
	item->data_alloced = true;

	wia_controller_t * wia = param;
	DASSERT(wia);
	item->index = wia->raw_data_used;

     #if HAVE_ASSERT
	wia_raw_data_t * rdata = need_raw_data(wia,0x20,item->offset,item->size,0);
	ASSERT(rdata);
	ASSERT( ntohl(rdata->n_groups) == 1 ); // [[2do]] really needed?
     #endif
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError SetupWriteWIA
(
    struct SuperFile_t	* sf,		// file to setup
    u64			src_file_size	// NULL or source file size
)
{
    ASSERT(sf);
    PRINT("#W# SetupWriteWIA(%p,%llx) src=%p, file=%d/%p, oft=%x, wia=%p, v=%s/%s\n",
		sf, src_file_size,
		sf->src, GetFD(&sf->f), GetFP(&sf->f),
		sf->iod.oft, sf->wia,
		PrintVersionWIA(0,0,WIA_VERSION_COMPATIBLE),
		PrintVersionWIA(0,0,WIA_VERSION) );

    if (sf->wia)
	return ERROR0(ERR_INTERNAL,0);

    sf->progress_trigger = sf->progress_trigger_init = 0;


    //----- setup controller

    wia_controller_t * wia = CALLOC(1,sizeof(*wia));
    sf->wia = wia;
    wia->is_writing = true;
    wia->gdata_group = wia->gdata_part = -1;  // reset gdata

    AllocBufferWIA(wia, opt_compr_chunk_size
			? opt_compr_chunk_size : opt_chunk_size, true, false );


    //----- setup file header

    wia_file_head_t *fhead = &wia->fhead;
    memcpy(fhead->magic,WIA_MAGIC,sizeof(fhead->magic));
    fhead->magic[3]++; // magic is invalid now
    fhead->version		= WIA_VERSION;
    fhead->version_compatible	= WIA_VERSION_COMPATIBLE;
    fhead->iso_file_size	= src_file_size ? src_file_size
					: sf->src ? sf->src->file_size : 0;

    //----- setup disc info && compression

    wia_disc_t *disc = &wia->disc;
    disc->disc_type	= WD_DT_UNKNOWN;
    disc->compression	= opt_compr_method;
    disc->chunk_size	= wia->chunk_size;

    switch(opt_compr_method)
    {
	case WD_COMPR__N:
	case WD_COMPR_NONE:
	case WD_COMPR_PURGE:
	    // nothing to do
	    break;

	case WD_COMPR_BZIP2:
	 #ifdef NO_BZIP2
	    return ERROR0(ERR_NOT_IMPLEMENTED,
			"No bzip2 support for this release! Sorry!\n");
	 #else
	    disc->compr_level = CalcCompressionLevelBZIP2(opt_compr_level);
	    wia->memory_usage += CalcMemoryUsageBZIP2(opt_compr_level,true);
	 #endif
	    PRINT("OPEN STREAM\n");
	    OpenStreamWFile(&sf->f);
	    break;

	case WD_COMPR_LZMA:
	    {
		EncLZMA_t lzma;
		enumError err = EncLZMA_Open(&lzma,sf->f.fname,opt_compr_level,false);
		if (err)
		    return err;
		disc->compr_level = lzma.compr_level;
		wia->memory_usage += CalcMemoryUsageLZMA(lzma.compr_level,true);
		const size_t len = lzma.enc_props_len < sizeof(disc->compr_data)
				 ? lzma.enc_props_len : sizeof(disc->compr_data);
		disc->compr_data_len = len;
		memcpy(disc->compr_data,lzma.enc_props,len);
		EncLZMA_Close(&lzma);
	    }
	    break;

	case WD_COMPR_LZMA2:
	    {
		EncLZMA_t lzma;
		enumError err = EncLZMA2_Open(&lzma,sf->f.fname,opt_compr_level,false);
		if (err)
		    return err;
		disc->compr_level = lzma.compr_level;
		const size_t len = lzma.enc_props_len < sizeof(disc->compr_data)
				 ? lzma.enc_props_len : sizeof(disc->compr_data);
		disc->compr_data_len = len;
		wia->memory_usage += CalcMemoryUsageLZMA2(lzma.compr_level,true);
		memcpy(disc->compr_data,lzma.enc_props,len);
		EncLZMA2_Close(&lzma);
	    }
	    break;
    }


    //----- check source disc type

    if (!sf->src)
	return FinishSetupWriteWIA(sf);

    wd_disc_t * wdisc = OpenDiscSF(sf->src,true,false);
    if (!wdisc)
	return FinishSetupWriteWIA(sf);
    wia->wdisc = wd_dup_disc(wdisc);
    sf->source_size = wd_count_used_disc_blocks(wdisc,1,0) * (u64)WII_SECTOR_SIZE;

    disc->disc_type = wdisc->disc_type;
    if ( disc->disc_type == WD_DT_GAMECUBE )
	return FinishSetupWriteWIA(sf);

    if (wdisc->have_overlays)
    {
	if ( verbose >= -1 )
	    ERROR0(ERR_WARNING,
		"Wii disc contains overlayed partitions!\n"
		"=> Create WIA in non effective raw mode: %s\n",
		sf->f.fname );
	return FinishSetupWriteWIA(sf);
    }


    int ip;
    for ( ip = 0; ip < wdisc->n_part; ip++ )
    {
	wd_part_t * wpart = wdisc->part + ip;
	wd_load_part(wpart,true,true,false);
	noPRINT("SETUP PARTITION #%u: %s\n",
		ip, wd_print_part_name(0,0,wpart->part_type,WD_PNAME_NUM_INFO) );
	if (!wpart->is_valid)
	{
	    if ( verbose >= -1 )
		ERROR0(ERR_WARNING,
			"Wii disc contains invalid partitions!\n"
			"=> Create WIA in non effective raw mode: %s\n",
			sf->f.fname );
	    return FinishSetupWriteWIA(sf);
	}
    }


    //----- setup wii partitions

    disc->n_part = wdisc->n_part;
    memcpy(&disc->dhead,&wdisc->dhead,sizeof(disc->dhead));

    wia_part_t * part = CALLOC(disc->n_part,sizeof(wia_part_t));
    wia->part = part;

    for ( ip = 0; ip < wdisc->n_part; ip++, part++ )
    {
	wd_part_t * wpart	= wdisc->part + ip;
	memcpy(part->part_key,wpart->key,sizeof(part->part_key));

	wia_part_data_t * pd	= part->pd;
	pd->first_sector	= wpart->data_sector;
	if ( pd->first_sector < WII_PART_OFF / WII_SECTOR_SIZE )
	     pd->first_sector = WII_PART_OFF / WII_SECTOR_SIZE;
	pd->n_sectors		= wpart->end_mgr_sector - pd->first_sector;
	pd->n_groups		= ( pd->n_sectors + wia->chunk_sectors - 1 )
				/ wia->chunk_sectors;
	pd->group_index		= wia->group_used;
	wia->group_used		+= pd->n_groups;


	pd++;
	pd->first_sector	= wpart->end_mgr_sector;
	pd->n_sectors		= wpart->end_sector - pd->first_sector;
	pd->n_groups		= ( pd->n_sectors + wia->chunk_sectors - 1 )
				/ wia->chunk_sectors;
	pd->group_index		= wia->group_used;
	wia->group_used		+= pd->n_groups;

	noTRACE("PT %u, sect = %x,%x,%x\n",
		ip, pd->first_sector, pd->n_sectors, pd->n_groups );
    }

    wd_insert_memmap_disc_part(&wia->memmap,wdisc,setup_dynamic_mem,wia,
			WIA_MM_CACHED_DATA,
			WIA_MM_PART_GDATA_0, WIA_MM_PART_GDATA_1,
			WIA_MM_IGNORE, WIA_MM_IGNORE );


    //----- finish setup

    return FinishSetupWriteWIA(sf);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			term write WIA			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError TermWriteWIA 
(
    struct SuperFile_t	* sf		// file to terminate
)
{
    ASSERT(sf);
    ASSERT(sf->wia);
    PRINT("#W# TermWriteWIA(%p)\n",sf);

    sf->progress_trigger = sf->progress_trigger_init = 1;

    wia_controller_t * wia = sf->wia;
    wia_disc_t *disc = &wia->disc;


    //----- write chached gdata

    enumError err = write_cached_gdata(sf,-1);
    if (err)
	return err;


    //----- write cached data

    wd_memmap_item_t *it, *it_end = wia->memmap.item + wia->memmap.used;
    for ( it = wia->memmap.item; it < it_end; it++ )
	if ( it->mode == WIA_MM_CACHED_DATA )
	{
	    DASSERT( it->index < wia->raw_data_used );
	    wia_raw_data_t * rdata = wia->raw_data + it->index;
	    ASSERT( ntohl(rdata->n_groups) == 1 );
	    DASSERT( ntohl(rdata->group_index) < wia->group_used );
	    err = write_data(sf, 0, it->data,it->size, ntohl(rdata->group_index), 0 );
	    if (err)
		return err;
	}
    

    //----- write raw data table

    DASSERT( wia->raw_data_used > 0 );
    if (!wia->growing->n_groups)
	wia->raw_data_used--;

    if (wia->raw_data_used)
    {
	DASSERT(wia->raw_data);
	disc->n_raw_data	= wia->raw_data_used;
	disc->raw_data_off	= wia->write_data_off;
	const u32 raw_data_len	= wia->raw_data_used * sizeof(wia_raw_data_t);
	err = write_data( sf, 0, wia->raw_data, raw_data_len, -1, &disc->raw_data_size );
	PRINT("** RAW DATA TABLE: n=%d, off=%llx, size=%x\n",
			disc->n_raw_data, disc->raw_data_off, disc->raw_data_size );
	if (err)
	    return err;
    }


    //----- write group table

    if (wia->group_used)
    {
	DASSERT(wia->group);
	disc->n_groups	= wia->group_used;
	disc->group_off	= wia->write_data_off;
	const u32 group_len	= wia->group_used * sizeof(wia_group_t);
	err = write_data( sf, 0, wia->group, group_len, -1, &disc->group_size );
	PRINT("** GROUP TABLE: n=%d, off=%llx, size=%x\n",
			disc->n_groups, disc->group_off, disc->group_size );
	if (err)
	    return err;
    }


    //----- calc part header

    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
	wia_hton_part(wia->part+ip,0);

    const u32 total_part_size = sizeof(wia_part_t) * disc->n_part;
    const u64 part_off = sizeof(wia_file_head_t) + sizeof(wia_disc_t);


    //----- calc disc info

    disc->part_t_size		= sizeof(wia_part_t);
    disc->part_off		= part_off;
    SHA1((u8*)wia->part,total_part_size,disc->part_hash);

    wia_hton_disc(disc,disc);


    //----- calc file header

    wia_file_head_t *fhead	= &wia->fhead;
    memcpy(fhead->magic,WIA_MAGIC,sizeof(fhead->magic));
    fhead->version		= WIA_VERSION;
    fhead->version_compatible	= WIA_VERSION_COMPATIBLE;
    fhead->disc_size		= sizeof(wia_disc_t);
    fhead->wia_file_size	= sf->f.max_off;

    SHA1((u8*)disc,sizeof(*disc),fhead->disc_hash);
    wia_hton_file_head(fhead,fhead);

    SHA1((u8*)fhead, sizeof(*fhead)-sizeof(fhead->file_head_hash),
		fhead->file_head_hash );


    //----- write data

    err = WriteAtF(&sf->f,0,fhead,sizeof(*fhead));
    if (err)
	return err;

    WriteAtF(&sf->f,sizeof(*fhead),disc,sizeof(*disc));
    if (err)
	return err;

    WriteAtF(&sf->f,part_off,wia->part,total_part_size);
    if (err)
	return err;


    // convert back to local endian

    wia_ntoh_file_head(fhead,fhead);
    wia_ntoh_disc(disc,disc);
    for ( ip = 0; ip < disc->n_part; ip++ )
	wia_ntoh_part(wia->part+ip,0);

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_file_head ( wia_file_head_t * dest, const wia_file_head_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->version		= ntohl (src->version);
    dest->version_compatible	= ntohl (src->version_compatible);

    dest->disc_size		= ntohl (src->disc_size);

    dest->iso_file_size		= ntoh64(src->iso_file_size);
    dest->wia_file_size		= ntoh64(src->wia_file_size);
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_file_head ( wia_file_head_t * dest, const wia_file_head_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->version		= htonl (src->version);
    dest->version_compatible	= htonl (src->version_compatible);

    dest->disc_size		= htonl (src->disc_size);

    dest->iso_file_size		= hton64(src->iso_file_size);
    dest->wia_file_size		= hton64(src->wia_file_size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_disc ( wia_disc_t * dest, const wia_disc_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->disc_type		= ntohl (src->disc_type);
    dest->compression		= ntohl (src->compression);
    dest->compr_level		= ntohl (src->compr_level);
    dest->chunk_size		= ntohl (src->chunk_size);

    dest->n_part		= ntohl (src->n_part);
    dest->part_t_size		= ntohl (src->part_t_size);
    dest->part_off		= ntoh64(src->part_off);

    dest->n_raw_data		= ntohl (src->n_raw_data);
    dest->raw_data_off		= ntoh64(src->raw_data_off);
    dest->raw_data_size		= ntohl (src->raw_data_size);

    dest->n_groups		= ntohl (src->n_groups);
    dest->group_off		= ntoh64(src->group_off);
    dest->group_size		= ntohl (src->group_size);
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_disc ( wia_disc_t * dest, const wia_disc_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->disc_type		= htonl (src->disc_type);
    dest->compression		= htonl (src->compression);
    dest->compr_level		= htonl (src->compr_level);
    dest->chunk_size		= htonl (src->chunk_size);

    dest->n_part		= htonl (src->n_part);
    dest->part_t_size		= htonl (src->part_t_size);
    dest->part_off		= hton64(src->part_off);

    dest->n_raw_data		= htonl (src->n_raw_data);
    dest->raw_data_off		= hton64(src->raw_data_off);
    dest->raw_data_size		= htonl (src->raw_data_size);

    dest->n_groups		= htonl (src->n_groups);
    dest->group_off		= hton64(src->group_off);
    dest->group_size		= htonl (src->group_size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_part ( wia_part_t * dest, const wia_part_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    int id;
    const wia_part_data_t * src_pd = src->pd;
    wia_part_data_t * dest_pd = dest->pd;

    for ( id = 0; id < sizeof(src->pd)/sizeof(src->pd[0]); id++, src_pd++, dest_pd++ )
    {
	dest_pd->first_sector	= ntohl (src_pd->first_sector);
	dest_pd->n_sectors	= ntohl (src_pd->n_sectors);

	dest_pd->group_index	= ntohl (src_pd->group_index);
	dest_pd->n_groups	= ntohl (src_pd->n_groups);
    }
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_part ( wia_part_t * dest, const wia_part_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    int id;
    const wia_part_data_t * src_pd = src->pd;
    wia_part_data_t * dest_pd = dest->pd;

    for ( id = 0; id < sizeof(src->pd)/sizeof(src->pd[0]); id++, src_pd++, dest_pd++ )
    {
	dest_pd->first_sector	= htonl (src_pd->first_sector);
	dest_pd->n_sectors	= htonl (src_pd->n_sectors);

	dest_pd->group_index	= htonl (src_pd->group_index);
	dest_pd->n_groups	= htonl (src_pd->n_groups);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void wia_ntoh_raw_data ( wia_raw_data_t * dest, const wia_raw_data_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->raw_data_off		= ntoh64(src->raw_data_off);
    dest->raw_data_size		= ntoh64(src->raw_data_size);

    dest->group_index		= ntohl (src->group_index);
    dest->n_groups		= ntohl (src->n_groups);
}

///////////////////////////////////////////////////////////////////////////////

void wia_hton_raw_data ( wia_raw_data_t * dest, const wia_raw_data_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->raw_data_off		= hton64(src->raw_data_off);
    dest->raw_data_size		= hton64(src->raw_data_size);

    dest->group_index		= htonl (src->group_index);
    dest->n_groups		= htonl (src->n_groups);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

