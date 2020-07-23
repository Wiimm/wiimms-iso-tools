
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

#include "wiidisc.h"
#include <ctype.h>

//-----------------------------------------------------------------------------

#ifdef __CYGWIN__
    #define ISALNUM(a) isalnum((int)(a))
#else
    #define ISALNUM isalnum
#endif

//-----------------------------------------------------------------------------

#define WDPRINT noPRINT

//
///////////////////////////////////////////////////////////////////////////////
///////////////			common key & encryption		///////////////
///////////////////////////////////////////////////////////////////////////////

// The real common key must be set by the application.
// Only the first WII_KEY_SIZE bytes of 'common_key' are used.

static u8 common_key_templ[WII_KEY_SIZE] = "common key unset";
#ifdef SUPPORT_CKEY_DEVELOP
static u8 common_key[WD_CKEY__N][WII_KEY_SIZE] = {"","",""};
#else
static u8 common_key[WD_CKEY__N][WII_KEY_SIZE] = {"",""};
#endif

static const u8 iv0[WII_KEY_SIZE] = {0}; // always NULL

///////////////////////////////////////////////////////////////////////////////

static void wd_setup_common_key()
{
    static bool done = false;
    if (!done)
    {
	done = true;
	memset(common_key,0,sizeof(common_key));

	int i;
	for ( i = 0; i < WD_CKEY__N; i++ )
	    memcpy(common_key[i],common_key_templ,sizeof(common_key[0]));
    }
}

///////////////////////////////////////////////////////////////////////////////

const u8 * wd_get_common_key
(
    wd_ckey_index_t	ckey_index	// index of common key
)
{
    wd_setup_common_key();
    return (unsigned)ckey_index < WD_CKEY__N ? common_key[ckey_index] : 0;
}

///////////////////////////////////////////////////////////////////////////////

const u8 * wd_set_common_key
(
    wd_ckey_index_t	ckey_index,	// index of common key
    const void		* new_key	// the new key
)
{
    wd_setup_common_key();
    if ( (unsigned)ckey_index >= WD_CKEY__N )
	return 0;

    u8 * ckey = common_key[ckey_index];

    if (new_key)
    {
	memcpy(ckey,new_key,WII_KEY_SIZE);
 #ifdef TEST
	PRINT("new common key[%d]:\n",ckey_index);
	HEXDUMP16(8,0,ckey,WII_KEY_SIZE);
 #endif
    }
    return ckey;
}

///////////////////////////////////////////////////////////////////////////////

void wd_decrypt_title_key
(
    const wd_ticket_t	* tik,		// pointer to ticket
    u8			* title_key	// the result
)
{
    u8 iv[WII_KEY_SIZE];
    memset( iv, 0, sizeof(iv) );
    memcpy( iv, tik->title_id, 8 );

    const wd_ckey_index_t ckey_index = tik->common_key_index < WD_CKEY__N
				     ? tik->common_key_index : 0;
    aes_key_t akey;
    wd_aes_set_key( &akey, common_key[ckey_index] );
    wd_aes_decrypt( &akey, iv, &tik->title_key, title_key, WII_KEY_SIZE );

    TRACE("| IV:  "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,iv,WII_KEY_SIZE);
    TRACE("| IN:  "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,&tik->title_key,WII_KEY_SIZE);
    TRACE("| OUT: "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,title_key,WII_KEY_SIZE);
}

///////////////////////////////////////////////////////////////////////////////

void wd_encrypt_title_key
(
    wd_ticket_t		* tik,		// pointer to ticket (modified)
    const u8		* title_key	// valid pointer to wanted title key
)
{
    u8 iv[WII_KEY_SIZE];
    memset( iv, 0, sizeof(iv) );
    memcpy( iv, tik->title_id, 8 );

    const wd_ckey_index_t ckey_index = tik->common_key_index < WD_CKEY__N
				     ? tik->common_key_index : 0;
    aes_key_t akey;
    wd_aes_set_key( &akey, common_key[ckey_index] );
    wd_aes_encrypt( &akey, iv, title_key, &tik->title_key, WII_KEY_SIZE );

    TRACE("| IV:  "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,iv,WII_KEY_SIZE);
    TRACE("| IN:  "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,&tik->title_key,WII_KEY_SIZE);
    TRACE("| OUT: "); TRACE_HEXDUMP(0,0,1,WII_KEY_SIZE,title_key,WII_KEY_SIZE);
}

///////////////////////////////////////////////////////////////////////////////

void wd_decrypt_sectors
(
    wd_part_t		* part,		// NULL or pointer to partition
    const aes_key_t	* akey,		// aes key, if NULL use 'part'
    const void		* sect_src,	// pointer to first source sector
    void		* sect_dest,	// pointer to destination
    void		* hash_dest,	// if not NULL: store hash tables here
    u32			n_sectors	// number of wii sectors to decrypt
)
{
    if (!akey)
    {
	DASSERT(part);
	wd_disc_t * disc = part->disc;
	DASSERT(disc);
	if ( disc->akey_part != part )
	{
	    disc->akey_part = part;
	    wd_aes_set_key(&disc->akey,part->key);
	}
	akey = &disc->akey;
    }

    const u8 * src = sect_src;
    u8 * dest = sect_dest;
    u8 tempbuf[WII_SECTOR_SIZE];

    if (hash_dest)
    {
	u8 * hdest = hash_dest;
	while ( n_sectors-- > 0 )
	{
	    const u8 * src1 = src;
	    if ( src == dest )
	    {
		// inplace decryption not possible => use tempbuf
		memcpy(tempbuf,src,WII_SECTOR_SIZE);
		src1 = tempbuf;
	    }

	    // decrypt header first, because of possible overlay of 'src1' & 'dest'
	    wd_aes_decrypt( akey,
			    iv0,
			    src1,
			    hdest,
			    WII_SECTOR_HASH_SIZE);

	    // decrypt data
	    wd_aes_decrypt( akey,
			    src1 + WII_SECTOR_IV_OFF,
			    src1 + WII_SECTOR_HASH_SIZE,
			    dest,
			    WII_SECTOR_DATA_SIZE );


	    src   += WII_SECTOR_SIZE;
	    dest  += WII_SECTOR_DATA_SIZE;
	    hdest += WII_SECTOR_HASH_SIZE;
	}
    }
    else
    {
	while ( n_sectors-- > 0 )
	{
	    const u8 * src1 = src;
	    if ( src == dest )
	    {
		// inplace decryption not possible => use tempbuf
		memcpy(tempbuf,src,WII_SECTOR_SIZE);
		src1 = tempbuf;
	    }

	    // decrypt data
	    wd_aes_decrypt( akey,
			    src1 + WII_SECTOR_IV_OFF,
			    src1 + WII_SECTOR_HASH_SIZE,
			    dest + WII_SECTOR_HASH_SIZE,
			    WII_SECTOR_DATA_SIZE );

	    // decrypt header
	    wd_aes_decrypt( akey,
			    iv0,
			    src1,
			    dest,
			    WII_SECTOR_HASH_SIZE);

	    src  += WII_SECTOR_SIZE;
	    dest += WII_SECTOR_SIZE;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_encrypt_sectors
(
    wd_part_t		* part,		// NULL or pointer to partition
    const aes_key_t	* akey,		// aes key, if NULL use 'part'
    const void		* sect_src,	// pointer to first source sector
    const void		* hash_src,	// if not NULL: source of hash tables
    void		* sect_dest,	// pointer to first destination sector
    u32			n_sectors	// number of wii sectors to encrypt
)
{
    if (!akey)
    {
	DASSERT(part);
	wd_disc_t * disc = part->disc;
	DASSERT(disc);
	if ( disc->akey_part != part )
	{
	    disc->akey_part = part;
	    wd_aes_set_key(&disc->akey,part->key);
	}
	akey = &disc->akey;
    }

    const u8 * src = sect_src;
    u8 * dest = sect_dest;
    u8 tempbuf[WII_SECTOR_SIZE];

    if (hash_src)
    {
	// copy reverse to allow overlap of 'data_src' and 'sect_dest'
	const u8 * hsrc = (const u8*)hash_src + n_sectors * WII_SECTOR_HASH_SIZE;
	src  += n_sectors * WII_SECTOR_DATA_SIZE;
	dest += n_sectors * WII_SECTOR_SIZE;

	while ( n_sectors-- > 0 )
	{
	    src  -= WII_SECTOR_DATA_SIZE;
	    hsrc -= WII_SECTOR_HASH_SIZE;
	    dest -= WII_SECTOR_SIZE;

	    // encrypt header
	    wd_aes_encrypt( akey,
			    iv0,
			    hsrc,
			    tempbuf,
			    WII_SECTOR_HASH_SIZE );

	    // encrypt data
	    wd_aes_encrypt( akey,
			    tempbuf + WII_SECTOR_IV_OFF,
			    src,
			    tempbuf + WII_SECTOR_HASH_SIZE,
			    WII_SECTOR_DATA_SIZE );

	    memcpy(dest,tempbuf,WII_SECTOR_SIZE);
	}
    }
    else
    {
	while ( n_sectors-- > 0 )
	{
	    // encrypt header
	    wd_aes_encrypt( akey,
			    iv0,
			    src,
			    dest,
			    WII_SECTOR_HASH_SIZE );

	    // encrypt data
	    wd_aes_encrypt( akey,
			    dest + WII_SECTOR_IV_OFF,
			    src  + WII_SECTOR_HASH_SIZE,
			    dest + WII_SECTOR_HASH_SIZE,
			    WII_SECTOR_DATA_SIZE );

	    src  += WII_SECTOR_SIZE;
	    dest += WII_SECTOR_SIZE;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_split_sectors
(
    const void		* sect_src,	// pointer to first source sector
    void		* data_dest,	// pointer to data destination
    void		* hash_dest,	// pointer to hash destination
    u32			n_sectors	// number of wii sectors to decrypt
)
{
    DASSERT(sect_src);
    DASSERT(data_dest);
    DASSERT(hash_dest);
    DASSERT( sect_src != hash_dest );

    const u8 * src = sect_src;
    u8 * data = data_dest;
    u8 * hash = hash_dest;

    while ( n_sectors-- > 0 )
    {
	memcpy( hash, src, WII_SECTOR_HASH_SIZE );
	hash += WII_SECTOR_HASH_SIZE;
	src  += WII_SECTOR_HASH_SIZE;

	memmove( data, src, WII_SECTOR_DATA_SIZE );
	data += WII_SECTOR_DATA_SIZE;
	src  += WII_SECTOR_DATA_SIZE;
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_join_sectors
(
    const void		* data_src,	// pointer to data source
    const void		* hash_src,	// pointer to hash source
    void		* sect_dest,	// pointer to first destination sector
    u32			n_sectors	// number of wii sectors to encrypt
)
{
    DASSERT(data_src);
    DASSERT(hash_src);
    DASSERT(sect_dest);
    DASSERT( hash_src != sect_dest );

    // copy reverse to allow overlap of 'data_src' and 'sect_dest'
    const u8 * data = (const u8*)data_src + n_sectors * WII_SECTOR_DATA_SIZE;
    const u8 * hash = (const u8*)hash_src + n_sectors * WII_SECTOR_HASH_SIZE;
    u8       * dest = (u8*) sect_dest     + n_sectors * WII_SECTOR_SIZE;

    while ( n_sectors-- > 0 )
    {
	dest -= WII_SECTOR_DATA_SIZE;
	data -= WII_SECTOR_DATA_SIZE;
	memmove( dest, data, WII_SECTOR_DATA_SIZE );

	dest -= WII_SECTOR_HASH_SIZE;
	hash -= WII_SECTOR_HASH_SIZE;
	memcpy( dest, hash, WII_SECTOR_HASH_SIZE );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			names, ids, titles, ...		///////////////
///////////////////////////////////////////////////////////////////////////////

const char * wd_disc_type_name[]
	= { "unknown", "GameCube", "Wii", 0 };

//-----------------------------------------------------------------------------

const char * wd_get_disc_type_name
(
    wd_disc_type_t	disc_type,	// disc type
    ccp			not_found	// result if no name found
)
{
    return (unsigned)disc_type < WD_DT__N
	? wd_disc_type_name[disc_type]
	: not_found;
}

///////////////////////////////////////////////////////////////////////////////

const char * wd_part_name[]
	= { "DATA", "UPDATE", "CHANNEL", 0 };

//-----------------------------------------------------------------------------

const char * wd_get_part_name
(
    u32			ptype,		// partition type
    ccp			not_found	// result if no name found
)
{
    return ptype < sizeof(wd_part_name)/sizeof(*wd_part_name)-1
	? wd_part_name[ptype]
	: not_found;
}

///////////////////////////////////////////////////////////////////////////////

char * wd_print_part_name
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			ptype,		// partition type
    wd_pname_mode_t	mode		// print mode
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    ccp id4 = (ccp)&ptype;
    const bool is_id = ISALNUM(id4[0]) && ISALNUM(id4[1])
		    && ISALNUM(id4[2]) && ISALNUM(id4[3]);
    const ccp name = ptype < sizeof(wd_part_name)/sizeof(*wd_part_name)-1
		   ?  wd_part_name[ptype] : 0;

    switch (mode)
    {
      case WD_PNAME_NAME_NUM_9:
	if (name)
	    snprintf(buf,buf_size,"%7s=%x",name,ptype);
	else if (is_id)
	{
	    ptype = htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"   \"%.4s\"",id4);
	}
	else
	    snprintf(buf,buf_size,"%9x",ptype);
	break;

      case WD_PNAME_COLUMN_9:
	if (name)
	    snprintf(buf,buf_size,"%7s %x",name,ptype);
	else if (is_id)
	{
	    ptype = htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"   \"%.4s\"",id4);
	}
	else
	    snprintf(buf,buf_size,"%9x",ptype);
	break;

      case WD_PNAME_NAME:
	if (name)
	{
	    snprintf(buf,buf_size,"%s",name);
	    break;
	}
	// fall through

       case WD_PNAME_NUM:
	if (is_id)
	{
	    ptype = htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"%.4s",id4);
	}
	else
	    snprintf(buf,buf_size,"%x",ptype);
	break;

      case WD_PNAME_P_NAME:
	if (name)
	    snprintf(buf,buf_size,"%s",name);
	else if (is_id)
	{
	    ptype = htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"P-%.4s",id4);
	}
	else
	    snprintf(buf,buf_size,"P%x",ptype);
	break;

     //case WD_PNAME_NUM_INFO:
      default:
	if (name)
	    snprintf(buf,buf_size,"%x [%s]",ptype,name);
	else if (is_id)
	{
	    ptype = htonl(ptype); // we need big endian here
	    snprintf(buf,buf_size,"0x%x [\"%.4s\"]",ptype,id4);
	}
	else
	    snprintf(buf,buf_size,"0x%x [%u]",ptype,ptype);
	break;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
// returns a pointer to a printable ID, teminated with 0

char * wd_print_id
(
    const void		* id,		// ID to convert in printable format
    size_t		id_len,		// len of 'id'
    void		* buf		// Pointer to buffer, size >= id_len + 1
					// If NULL, a local circulary static buffer is used
)
{
    DASSERT(id);

    if (!buf)
	buf = GetCircBuf( id_len + 1);

    ccp src = id;
    char * dest = buf;
    while ( id_len-- > 0 )
    {
	const char ch = *src++;
	*dest++ = ch >= ' ' && ch < 0x7f ? ch : '.';
    }
    *dest = 0;

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
// returns a pointer to a printable ID with colors, teminated with 0

char * wd_print_id_col
(
    const void		* id,		// NULL | EMPTY | ID to convert
    size_t		id_len,		// len of 'id'
    const void		* ref_id,	// reference ID
    const ColorSet_t	*colset		// NULL or color set
)
{
    if ( !id || !*(u8*)id )
	return "  --";

    if ( !ref_id || !memcmp(id,ref_id,id_len) || !colset || !colset->colorize )
	return wd_print_id(id,id_len,0);

    char buf[100];
    char *dest = StringCopyS(buf,sizeof(buf)-id_len,colset->highlight);

    ccp src = id;
    while ( id_len-- > 0 )
    {
	const char ch = *src++;
	*dest++ = ch >= ' ' && ch < 0x7f ? ch : '.';
    }
    dest = StringCopyE(dest,buf+sizeof(buf),colset->reset);

    const uint len = dest - buf + 1;
    char *res = GetCircBuf(len);
    memcpy(res,buf,len);
    return res;
}

///////////////////////////////////////////////////////////////////////////////
// rename ID and title of a ISO header

int wd_rename
(
    void		* p_data,	// pointer to ISO data
    ccp			new_id,		// if !NULL: take the first 6 chars as ID
    ccp			new_title	// if !NULL: take the first 0x39 chars as title
)
{
    DASSERT(p_data);

    int done = 0;

    if ( new_id )
    {
	ccp end_id;
	char * dest = p_data;
	for ( end_id = new_id + 6; new_id < end_id && *new_id; new_id++, dest++ )
	    if ( *new_id != '.' && *new_id != *dest )
	    {
		*dest = *new_id;
		done |= 1;
	    }
    }

    if ( new_title && *new_title )
    {
	char buf[WII_TITLE_SIZE];
	memset(buf,0,WII_TITLE_SIZE);
	const size_t slen = strlen(new_title);
	memcpy(buf, new_title, slen < WII_TITLE_SIZE ? slen : WII_TITLE_SIZE-1 );

	u8 *dest = p_data;
	dest += WII_TITLE_OFF;
	if ( memcmp(dest,buf,WII_TITLE_SIZE) )
	{
	    memcpy(dest,buf,WII_TITLE_SIZE);
	    done |= 2;
	}
    }

    return done;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read functions			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError wd_read_raw
(
    wd_disc_t		* disc,		// valid disc pointer
    u32			disc_offset4,	// disc offset/4
    void		* dest_buf,	// destination buffer
    u32			read_size,	// number of bytes to read 
    wd_usage_t		usage_id	// not 0: mark usage usage_table with this value
)
{
    DASSERT(disc);
    DASSERT(disc->read_func);
    TRACE("#WD# wd_read_raw() %9llx %8x dest=%p\n",
			(u64)disc_offset4<<2, read_size, dest_buf );

    if (!read_size)
	return ERR_OK;

    if (dest_buf)
    {
	noTRACE("WD-READ: ++   %9llx %5x ++\n",(u64)disc_offset4<<2, read_size );

	u8  *dest   = dest_buf;
	u32 len	    = read_size;
	u32 offset4 = disc_offset4;

	// force reading only whole HD SECTORS
	char hd_buf[HD_SECTOR_SIZE];

	const int HD_SECTOR_SIZE4 = HD_SECTOR_SIZE >> 2;
	u32 skip4   = offset4 - offset4/HD_SECTOR_SIZE4 * HD_SECTOR_SIZE4;

	if (skip4)
	{
	    const u32 off1 = offset4 - skip4;
	    const int ret = disc->read_func(disc->read_data,off1,HD_SECTOR_SIZE,hd_buf);
	    if (ret)
	    {
		WD_ERROR(ERR_READ_FAILED,"Error while reading disc%s",disc->error_term);
		return ERR_READ_FAILED;
	    }

	    const u32 skip = skip4 << 2;
	    DASSERT( skip < HD_SECTOR_SIZE );
	    u32 copy_len = HD_SECTOR_SIZE - skip;
	    if ( copy_len > len )
		 copy_len = len;

	    noTRACE("WD-READ/PRE:  %9llx %5x -> %5zx %5x, skip=%x,%x\n",
			(u64)off1<<2, HD_SECTOR_SIZE, dest-(u8*)dest_buf, copy_len, skip,skip4 );

	    memcpy(dest,hd_buf+skip,copy_len);
	    dest    += copy_len;
	    len	    -= copy_len;
	    offset4 += copy_len >> 2;
	}

	const u32 copy_len = len / HD_SECTOR_SIZE * HD_SECTOR_SIZE;
	if ( copy_len > 0 )
	{
	    noTRACE("WD-READ/MAIN: %9llx %5x -> %5zx %5x\n",
			(u64)offset4<<2, copy_len, dest-(u8*)dest_buf, copy_len );
	    const int ret = disc->read_func(disc->read_data,offset4,copy_len,dest);
	    if (ret)
	    {
		WD_ERROR(ERR_READ_FAILED,"Error while reading disc%s",disc->error_term);
		return ERR_READ_FAILED;
	    }

	    dest    += copy_len;
	    len	    -= copy_len;
	    offset4 += copy_len >> 2;
	}

	if ( len > 0 )
	{
	    ASSERT( len < HD_SECTOR_SIZE );

	    noTRACE("WD-READ/POST: %9llx %5x -> %5zx %5x\n",
			(u64)offset4<<2, HD_SECTOR_SIZE, dest-(u8*)dest_buf, len );
	    const int ret = disc->read_func(disc->read_data,offset4,HD_SECTOR_SIZE,hd_buf);
	    if (ret)
	    {
		WD_ERROR(ERR_READ_FAILED,"Error while reading disc%s",disc->error_term);
		return ERR_READ_FAILED;
	    }

	    memcpy(dest,hd_buf,len);
	}
    }

    if ( usage_id )
    {
	u32 first_block	= disc_offset4 >> WII_SECTOR_SIZE_SHIFT-2;
	u32 end_block	= disc_offset4 + ( read_size + WII_SECTOR_SIZE - 1 >> 2 )
			>> WII_SECTOR_SIZE_SHIFT-2;
	if ( end_block > WII_MAX_SECTORS )
	     end_block = WII_MAX_SECTORS;
	noTRACE("mark %x+%x => %x..%x [%x]\n",
		disc_offset4, read_size, first_block, end_block, end_block-first_block );

	if ( first_block < end_block )
	{
	    memset( disc->usage_table + first_block,
			usage_id, end_block - first_block );
	    if ( disc->usage_max < end_block )
		 disc->usage_max = end_block;
	}
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_part_raw
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			offset4,	// offset/4 to partition start
    void		* dest_buf,	// destination buffer
    u32			read_size,	// number of bytes to read 
    bool		mark_block	// true: mark block in 'usage_table'
)
{
    DASSERT(part);
    return wd_read_raw( part->disc,
			part->part_off4 + offset4,
			dest_buf,
			read_size,
			mark_block ? part->usage_id : 0 );
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_part_block
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			block_num,	// block number of partition
    u8			* block,	// destination buf
    bool		mark_block	// true: mark block in 'usage_table'
)
{
    TRACE("#WD# #%08x          wd_read_part_block()\n",block_num);
    DASSERT(part);
    DASSERT(part->disc);

    wd_disc_t * disc = part->disc;

    enumError err = ERR_OK;
    if ( disc->block_part != part || disc->block_num != block_num )
    {

	err = wd_read_raw(	part->disc,
				part->data_off4 + block_num * WII_SECTOR_SIZE4,
				disc->temp_buf,
				WII_SECTOR_SIZE,
				mark_block ? part->usage_id | WD_USAGE_F_CRYPT : 0 );

	if (err)
	{
	    memset(disc->block_buf,0,sizeof(disc->block_buf));
	    disc->block_part = 0;
	}
	else
	{
	    disc->block_part = part;
	    disc->block_num  = block_num;

	    if (part->is_encrypted)
	    {
		if ( disc->akey_part != part )
		{
		    disc->akey_part = part;
		    wd_aes_set_key(&disc->akey,part->key);
		}

		// decrypt data
		wd_aes_decrypt(	&disc->akey,
				disc->temp_buf + WII_SECTOR_IV_OFF,
				disc->temp_buf + WII_SECTOR_HASH_SIZE,
				disc->block_buf,
				WII_SECTOR_DATA_SIZE );
	    }
	    else
		memcpy(	disc->block_buf,
			disc->temp_buf + WII_SECTOR_HASH_SIZE,
			WII_SECTOR_DATA_SIZE );
	}
    }

    memcpy( block, disc->block_buf, WII_SECTOR_DATA_SIZE );
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_part
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			data_offset4,	// partition data offset/4
    void		* dest_buf,	// destination buffer
    u32			read_size,	// number of bytes to read 
    bool		mark_block	// true: mark block in 'usage_table'
)
{
    TRACE("#WD# %8x %8x wd_read_part()\n",data_offset4,read_size);
    DASSERT(part);
    DASSERT(part->disc);
    DASSERT(dest_buf);

    wd_disc_t * disc = part->disc;

    if (part->is_gc)
    {
	noPRINT("%llx+%x + %8x\n", (u64)part->part_off4<<2, data_offset4, read_size );
	ASSERT(!(data_offset4&3));

	const u64 off = part->part_off4 + ( data_offset4 >> 2 );
	const u64 mark_end = ( off << 2 ) + read_size;
	if ( part->max_marked < mark_end )
	     part->max_marked = mark_end;

	return wd_read_raw ( disc, off, dest_buf, read_size,
				mark_block ? part->usage_id : 0 );
    }

    const u64 mark_end = ( (u64)( part->data_off4 + data_offset4 ) << 2 ) + read_size;
    if ( part->max_marked < mark_end )
	 part->max_marked = mark_end;

    u8 * temp = disc->temp_buf;
    u8 * dest = dest_buf;

    enumError err = ERR_OK;
    while ( read_size && err == ERR_OK )
    {
	u32 offset4_in_block = data_offset4 % WII_SECTOR_DATA_SIZE4;
	u32 len_in_block = WII_SECTOR_DATA_SIZE - ( offset4_in_block << 2 );
	if ( len_in_block > read_size )
	     len_in_block = read_size;

	err = wd_read_part_block( part,
				  data_offset4 / WII_SECTOR_DATA_SIZE4,
				  temp,
				  mark_block );
	memcpy( dest, temp + (offset4_in_block<<2), len_in_block );

	dest		+= len_in_block;
	data_offset4	+= len_in_block >> 2;
	read_size	-= len_in_block;
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

void wd_mark_part
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			data_offset4,	// partition data offset/4
    u32			data_size	// number of bytes to mark
)
{
    DASSERT(part);
    DASSERT(part->disc);

    u32 first_block, end_block;
    wd_usage_t marker;

    if (part->is_gc)
    {
	const u32 block_delta = part->data_off4 / WII_SECTOR_SIZE4;
	first_block = block_delta + data_offset4 / WII_SECTOR_SIZE;
	end_block = ( data_offset4 + data_size + WII_SECTOR_SIZE - 1 )
		  / WII_SECTOR_SIZE + block_delta;
	marker = part->usage_id;

	const u64 mark_end = data_offset4 + data_size;
	if ( part->max_marked < mark_end )
	     part->max_marked = mark_end;
    }
    else
    {
	const u32 block_delta = part->data_off4 / WII_SECTOR_SIZE4;
	first_block = data_offset4 / WII_SECTOR_DATA_SIZE4 + block_delta;
	end_block = (( data_size + WII_SECTOR_DATA_SIZE - 1 >> 2 )
		  + data_offset4 ) / WII_SECTOR_DATA_SIZE4
		  + block_delta;
	marker = part->usage_id | WD_USAGE_F_CRYPT;

	const u64 mark_end = ( (u64)data_offset4 << 2 ) + data_size;
	if ( part->max_marked < mark_end )
	     part->max_marked = mark_end;
    }

    if ( end_block > WII_MAX_SECTORS )
	end_block = WII_MAX_SECTORS;

    if ( first_block < end_block )
    {
	wd_disc_t * disc = part->disc;
	memset( disc->usage_table + first_block, marker, end_block - first_block );
	if ( part->end_sector < end_block )
	     part->end_sector = end_block;
	if ( disc->usage_max < end_block )
	     disc->usage_max = end_block;
    }
}

///////////////////////////////////////////////////////////////////////////////

u64 wd_calc_disc_offset
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u64			data_offset4	// partition offset
)
{
    DASSERT(part);

    if (part->is_gc) // GC: non shifted offsets
	return (u64)part->part_off4 + data_offset4;
	//return (u64)( part->part_off4 + data_offset4 ) << 2;

    const u32 offset_in_block = data_offset4 % WII_SECTOR_DATA_SIZE4;
    const u32 block = data_offset4 / WII_SECTOR_DATA_SIZE4;
    return (u64)( part->data_sector + block ) * WII_SECTOR_SIZE
		+ ( offset_in_block << 2 )
		+ WII_SECTOR_HASH_SIZE;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get sector status		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_sector_status_t wd_get_part_sector_status
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u32			block_num,	// block number of partition
    bool		silent		// true: don't print error messages
)
{
    DASSERT(part);
    DASSERT(part->disc);
    TRACE("wd_get_part_sector_status(%p,%d,%d)\n",part,block_num,silent);


    //----- setup

    // extra test '!part->is_loaded' to avoid circulary calling
    if ( !part->is_loaded && wd_load_part(part,false,false,silent) )
	return WD_SS_READ_ERROR;

    if ( block_num >= part->end_sector - part->data_sector )
	return WD_SS_INVALID_SECTOR;


    //----- load block

    wd_disc_t * disc = part->disc;
    u8 * rdbuf = part->disc->temp_buf;
    u8 * data  = rdbuf + WII_SECTOR_HASH_SIZE;
    const u32 abs_block_num = block_num + part->data_sector;
    enumError err = wd_read_raw(disc,
				abs_block_num * WII_SECTOR_SIZE4,
				rdbuf,
				WII_SECTOR_SIZE,
				0 );
    if (err)
	return WD_SS_READ_ERROR|WD_SS_F_PART_DATA;


    //----- check for 'SKELETON'

    wd_sector_status_t stat = WD_SS_F_PART_DATA;

    if ( !*rdbuf
	&& !memcmp(data-sizeof(skeleton_marker),skeleton_marker,sizeof(skeleton_marker))
	&& !memcmp(rdbuf,rdbuf+1,WII_SECTOR_HASH_SIZE-sizeof(skeleton_marker)-1) )
    {
	stat |= WD_SS_F_SKELETON;
    }


    //----- check cleared HASH

    if ( !memcmp(rdbuf,rdbuf+1,WII_SECTOR_HASH_SIZE-1) )
    {
	stat |= WD_SS_HASH_CLEARED;
	if (!*rdbuf)
	    stat |= WD_SS_HASH_ZEROED;
    }


    //----- check cleared DATA

    if ( !memcmp(data,data+1,WII_SECTOR_DATA_SIZE-1))
    {
	stat |= WD_SS_DATA_CLEARED;
	if (!*data)
	    stat |= WD_SS_DATA_ZEROED;
	if ( stat & WD_SS_HASH_CLEARED && *rdbuf == *data )
	{
	    stat |= WD_SS_SECTOR_CLEARED;
	    if (!disc->usage_table[abs_block_num])
		stat |= WD_SS_F_SCRUBBED;
	}
    }


    //----- check cleared encryption

    if ( !(stat & (WD_SS_HASH_CLEARED|WD_SS_F_SKELETON)) )
    {
	u8 hash[WII_HASH_SIZE];
	u8 dcbuf[WII_SECTOR_SIZE];

	int h0idx;
	for ( h0idx = 0; h0idx < WII_N_ELEMENTS_H0; h0idx++ )
	{
	    //--- check for decryption

	    SHA1( data + h0idx * WII_H0_DATA_SIZE, WII_H0_DATA_SIZE, hash );
	    if (!memcmp(rdbuf + h0idx * WII_HASH_SIZE , hash, WII_HASH_SIZE))
	    {
		noPRINT("- DEC #%02u!\n",h0idx);
		stat |= WD_SS_DECRYPTED;
		break;
	    }

	    //--- decrypt

	    if (!h0idx) // decrypt only once
		wd_decrypt_sectors(part,0,rdbuf,dcbuf,0,1);


	    //--- check for encryption

	    SHA1( dcbuf + WII_SECTOR_HASH_SIZE + h0idx * WII_H0_DATA_SIZE,
			WII_H0_DATA_SIZE, hash );
	    if (!memcmp(dcbuf + h0idx * WII_HASH_SIZE , hash, WII_HASH_SIZE))
	    {
		noPRINT("- ENC #%02u!\n",h0idx);
		stat |= WD_SS_ENCRYPTED;
		break;
	    }
	}
    }

    TRACELINE;
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

wd_sector_status_t wd_get_disc_sector_status
(
    wd_disc_t		* disc,		// valid disc pointer
    u32			block_num,	// block number of disc
    bool		silent		// true: don't print error messages
)
{
    DASSERT(disc);
    PRINT("wd_get_disc_sector_status(%p,%d,%d)\n",disc,block_num,silent);

    if ( block_num >= WII_MAX_SECTORS )
	return WD_SS_INVALID_SECTOR;

    //----- block in usage table?

    wd_usage_t usage = disc->usage_table[block_num];
    wd_part_t * part = 0;
    if (!usage)
    {
	//--- find partition

	const u64 block_off = block_num * (u64)WII_SECTOR_SIZE;

	int pi;
	for ( pi = disc->n_part-1; pi >= 0; pi-- )
	{
	    wd_part_t * test_part = disc->part + pi;
	    u64 off = (u64)test_part->part_off4 << 2;
	    noPRINT(" %llx: %llx .. %llx\n", block_off, off, off+test_part->part_size );
	    if ( block_off >= off
		&& wd_load_part(test_part,false,false,silent)
		&& block_off < off + test_part->part_size )
	    {
		part = test_part;
		break;
	    }
	}
    }
    else if ( usage & WD_USAGE_F_CRYPT )
    {
	const uint idx = ( usage & WD_USAGE__MASK ) - WD_USAGE_PART_0;
	if ( idx < disc->n_part )
	    part = disc->part + idx;
    }

    //----- if partiton found -> ...

    if (part)
	return wd_load_part(part,false,false,silent)
		? WD_SS_READ_ERROR
		: wd_get_part_sector_status(part,block_num-part->data_sector,silent);


    //----- not partiton data: load block

    u8 * rdbuf = disc->temp_buf;
    DASSERT(rdbuf);
    enumError err = wd_read_raw(disc,
				block_num * WII_SECTOR_SIZE4,
				rdbuf,
				WII_SECTOR_SIZE,
				0 );
    if (err)
	return WD_SS_READ_ERROR|WD_SS_F_PART_DATA;


    //----- check cleared DATA

    wd_sector_status_t stat = 0;

    if ( !memcmp(rdbuf,rdbuf+1,WII_SECTOR_SIZE-1) )
    {
	stat |= WD_SS_DATA_CLEARED | WD_SS_SECTOR_CLEARED;
	if (!*rdbuf)
	    stat |= WD_SS_DATA_ZEROED;
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

typedef struct info_t
{
    ccp		text;		// info to print if 'condition'
    u32		condition;	// condition
    u32		clear_flags;	// clear flags if 'condition'

} info_t;

//-----------------------------------------------------------------------------

static char * proceed_info_tab
(
    char	* buf,		// destination buffer
    ccp		buf_end,	// end of destination buffer
    info_t	* info,		// info table
    u32		flags,		// flags
    bool	sep		// use ", " as initial separator
)
{
    DASSERT(buf);
    DASSERT( buf < buf_end );
    DASSERT( info );

    char * dest = buf;
    if (flags)
    {
	for ( ; info->text; info++ )
	{
	    if ( flags & info->condition )
	    {
		flags &= ~info->clear_flags;
		const int len = strlen(info->text);
		if ( dest + len + 2 < buf_end )
		{
		    if (sep)
		    {
			*dest++ = ',';
			*dest++ = ' ';
		    }
		    else
			sep = true;
		    memcpy(dest,info->text,len);
		    dest += len;
		    DASSERT( dest <= buf_end );
		}
	    }
	}
    }
    DASSERT( dest < buf_end );
    *dest = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

ccp wd_log_sector_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_sector_status_t	sect_stat	// sector status
)
{
    static info_t info_tab[] =
    {
	{ "err:sect",	WD_SS_INVALID_SECTOR,	WD_SS_INVALID_SECTOR },
	{ "err:read",	WD_SS_READ_ERROR,	WD_SS_READ_ERROR },
	{ "err:?",	WD_SS_M_ERROR,		WD_SS_M_ERROR },

	{ "Ac",		WD_SS_SECTOR_CLEARED,	WD_SS_HASH_CLEARED|WD_SS_DATA_CLEARED },
	{ "Hz",		WD_SS_HASH_ZEROED,	WD_SS_HASH_CLEARED },
	{ "Dz",		WD_SS_DATA_ZEROED,	WD_SS_DATA_CLEARED },
	{ "Hc",		WD_SS_HASH_CLEARED,	0 },
	{ "Dc",		WD_SS_DATA_CLEARED,	0 },

	{ "enc",	WD_SS_ENCRYPTED,	0 },
	{ "dec",	WD_SS_DECRYPTED,	0 },

	{ "pdata",	WD_SS_F_PART_DATA,	0 },
	{ "scrub",	WD_SS_F_SCRUBBED,	0 },
	{ "skel",	WD_SS_F_SKELETON,	0 },

	{0,0,0}
    };

    if (!buf)
	buf = GetCircBuf( buf_size = 50 );
    proceed_info_tab( buf, buf+buf_size, info_tab, sect_stat, false );
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp wd_print_sector_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_sector_status_t	sect_stat,	// sector status
    cert_stat_t		sig_stat	// not NULL: add signature status
)
{
    static info_t info_tab_sect[] =
    {
	{ "encrypted",		WD_SS_ENCRYPTED,	0 },
	{ "decrypted",		WD_SS_DECRYPTED,	0 },
	{ "skeleton",		WD_SS_F_SKELETON,	WD_SS_F_SCRUBBED },
	{ "scrubbed",		WD_SS_F_SCRUBBED,	0 },

	{0,0,0}
    };

    static info_t info_tab_sig[] =
    {
	{ "not signed",		CERT_F_HASH_FAILED,	CERT_F_HASH_OK },
	{ "fake signed",	CERT_F_HASH_FAKED,	CERT_F_HASH_OK },
	{ "well signed",	CERT_F_HASH_OK,		0 },

	{0,0,0}
    };

    if (!buf)
	buf = GetCircBuf( buf_size = 80 );
    char * dest = proceed_info_tab( buf, buf+buf_size, info_tab_sect, sect_stat, false );
    proceed_info_tab( dest, buf+buf_size, info_tab_sig, sig_stat, dest > buf );
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

int wd_is_block_encrypted
(
    // returns -1 on read error

    wd_part_t		* part,		// valid pointer to a disc partition
    u32			block_num,	// block number of partition
    int			unknown_result,	// result if status is unknown
    bool		silent		// true: don't print error messages
)
{
    DASSERT(part);
    DASSERT(part->disc);

    const wd_sector_status_t ss = wd_get_part_sector_status(part,block_num,silent);
    if ( ss & WD_SS_F_PART_DATA )
    {
	part->sector_stat |= ss;
	part->disc->sector_stat |= part->sector_stat;
    }

    noPRINT("wd_is_block_encrypted(%d) -> %04x '%s'\n",
	block_num, ss, wd_log_sector_status(0,0,ss) );
    return ss & WD_SS_ENCRYPTED
		? 1
		: ss & (WD_SS_DECRYPTED|WD_SS_F_SKELETON)
			? 0
			: unknown_result;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_is_part_scrubbed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    bool		silent		// true: don't print error messages
)
{
    DASSERT(part);
    DASSERT(part->disc);

    if (!part->scrub_test_done)
    {
	part->scrub_test_done = true;

	const enumError err = wd_load_part(part,false,false,silent);
	if (err)
	    return false;

	int try_count = 5, sector;
	for ( sector = part->data_sector;
	      sector < part->end_sector
			&& try_count
			&& !(part->sector_stat&WD_SS_F_SCRUBBED);
	      sector++ )
	{
	    DASSERT( sector < WII_MAX_SECTORS );
	    if (!part->disc->usage_table[sector])
	    {
		try_count--;
		wd_sector_status_t ss = wd_get_part_sector_status(part,sector,silent);
		if ( ss & WD_SS_F_PART_DATA )
		    part->sector_stat |= ss;
	    }
	}
	part->disc->sector_stat |= part->sector_stat;
    }

    return ( part->sector_stat & WD_SS_F_SCRUBBED ) != 0;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_is_disc_scrubbed
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		silent		// true: don't print error messages
)
{
    DASSERT(disc);

    if (!disc->scrub_test_done)
    {
	disc->scrub_test_done = true;

	int pi;
	for ( pi = 0; pi < disc->n_part; pi++ )
	    wd_is_part_scrubbed(disc->part+pi,silent);
    }

    return ( disc->sector_stat & WD_SS_F_SCRUBBED ) != 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			open and close			///////////////
///////////////////////////////////////////////////////////////////////////////

static wd_part_t * wd_initialize_part
(
    wd_disc_t		* disc,		// a valid disc pointer
    u32			part_index	// index of partition
)
{
    DASSERT(disc);
    DASSERT( part_index <= disc->n_part ); // = because delayed increment
    wd_part_t * part = disc->part + part_index;

    cert_initialize(&part->cert_chain);

    part->index		= part_index;
    part->usage_id	= part_index + WD_USAGE_PART_0;
    part->is_enabled	= true;
    part->disc		= disc;

    return part;
}

///////////////////////////////////////////////////////////////////////////////

static wd_disc_t * wd_open_gc_disc
(
    wd_disc_t		* disc,		// a valid disc pointer
    enumError		* error_code	// store error code if not NULL
)
{
    DASSERT(disc);
    DASSERT( disc->disc_type == WD_DT_GAMECUBE );
    DASSERT( disc->disc_attrib & WD_DA_GAMECUBE );
    disc->region.region	= *(u32*)(disc->temp_buf+WII_BI2_OFF+WII_BI2_REGION_OFF);
    memcpy(&disc->ptab,disc->temp_buf,sizeof(disc->ptab));

    if ( disc->disc_attrib & WD_DA_GC_MULTIBOOT )
    {
	//----- setup multiboot gamecube disc

	const int dvd9 = disc->disc_attrib & WD_DA_GC_DVD9;

	u32 * ptab_beg = (u32*)( (char*)&disc->dhead + GC_MULTIBOOT_PTAB_OFF );
	u32 * ptab_end = ptab_beg + GC_MULTIBOOT_PTAB_SIZE/sizeof(u32);
	u32 * ptab;

	u32 n_part = 1; // assume primary partition
	for ( ptab = ptab_beg; ptab < ptab_end; ptab++ )
	    if (ntohl(*ptab))
		n_part++;

	WDPRINT("MULTIBOOT '%.6s', N=%d, dvd9=%d\n",&disc->dhead.disc_id,n_part,dvd9);

	wd_part_t * part = CALLOC(n_part,sizeof(*disc->part));
	disc->part = part;

	//----- check primary partition

	disc->n_part		= 1;
	wd_initialize_part(disc,0);
	part->part_type		= be32(&disc->dhead);
	part->is_gc		= true;
	if ( wd_load_part(part,false,false,true) == ERR_OK )
	{
	    WDPRINT("PRIMARY PARTITION FOUND\n");
	    disc->disc_attrib |= WD_DA_GC_START_PART;
	    part++;
	}
	else
	{
	    disc->n_part--;
	    memset(part,0,sizeof(*part));
	}

	//----- check secondary partitions

	for ( ptab = ptab_beg; ptab < ptab_end; ptab++ )
	{
	    u32 off = ntohl(*ptab);
	    if (off)
	    {
		if (!dvd9)
		    off >>= 2;

		WDPRINT("PART #%u: off=%llx\n",disc->n_part,(u64)off<<2);

		part = wd_initialize_part(disc,disc->n_part);
		part->part_type		= WD_PART_DATA; // will be changed after loading
		part->part_off4		= off;
		part->is_gc		= true;

		disc->n_part++;
	    }
	}
    }
    else
    {
	//----- setup the data partition of a standard gamecube disc

	disc->n_part = 1;
	wd_part_t * part = CALLOC(1,sizeof(*disc->part));
	disc->part = part;

	wd_initialize_part(disc,0);
	part->part_type		= WD_PART_DATA; // will be changed after loading
	part->is_gc		= true;
    }

    //----- terminate

 #ifdef DEBUG
    wd_print_disc(TRACE_FILE,10,disc,0);
 #endif

    wd_load_all_part(disc,false,false,false);
    if (error_code)
	*error_code = ERR_OK;
    return disc;
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_t * wd_open_disc
(
    wd_read_func_t	read_func,	// read function, always valid
    void		* read_data,	// data pointer for read function
    u64			iso_size,	// size of iso file, unknown if 0
    ccp			file_name,	// used for error messages if not NULL
    int			force,		// force level
    enumError		* error_code	// store error code if not NULL
)
{
    DASSERT(read_func);
    TRACE("wd_open_disc(%p,%p,%llx=%llu,%s,%d,%p)\n",
		read_func, read_data, iso_size, iso_size,
		file_name, force, error_code );

    //----- setup

    wd_disc_t * disc = MALLOC(sizeof(*disc));
    memset(disc,0,sizeof(*disc));
    disc->cache_sector = ~(u32)0;

    disc->read_func = read_func;
    disc->read_data = read_data;
    disc->iso_size  = iso_size;
    disc->force     = force;
    disc->open_count = 1;


    //----- calculate termination string for error messages

    if ( file_name && *file_name )
    {
	const size_t max_len = sizeof(disc->error_term) - 5;
	const size_t flen = strlen(file_name);
	if ( flen > max_len )
	    snprintf(disc->error_term,sizeof(disc->error_term),": ...%s\n",
			file_name + flen - max_len + 5 );
	else
	    snprintf(disc->error_term,sizeof(disc->error_term),": %s\n",file_name);
    }
    else
	snprintf(disc->error_term,sizeof(disc->error_term),".\n");


    //----- read disc header

    enumError err = wd_read_raw( disc, 0, disc->temp_buf,
					WII_BOOT_SIZE+WII_BI2_SIZE, WD_USAGE_DISC );
    memcpy(&disc->dhead,disc->temp_buf,sizeof(disc->dhead));

    disc->disc_type = get_header_disc_type(&disc->dhead,&disc->disc_attrib);
    switch(disc->disc_type)
    {
	case WD_DT_GAMECUBE:
	    return wd_open_gc_disc(disc,error_code);

	case WD_DT_WII:
	    // nothing to do
	    break;

	default:
	    WD_ERROR(ERR_WDISC_NOT_FOUND,"Disc magic not found%s",disc->error_term);
	    err = ERR_WDISC_NOT_FOUND;
	    break;
    }


    //----- read partition data

    if (!err)
	err = wd_read_raw( disc, WII_PTAB_REF_OFF>>2,
				&disc->ptab_info, sizeof(disc->ptab_info), WD_USAGE_DISC );

    if (!err)
    {
	u32 n_part = 0, ipt;
	for ( ipt = 0; ipt < WII_MAX_PTAB; ipt++ )
	    n_part += ntohl(disc->ptab_info[ipt].n_part);
	if ( n_part > WII_MAX_PARTITIONS )
	     n_part = WII_MAX_PARTITIONS;
	disc->n_part = n_part;

	disc->ptab_entry = CALLOC(n_part,sizeof(*disc->ptab_entry));
	disc->part = CALLOC(n_part,sizeof(*disc->part));

	disc->n_ptab = 0;
	for ( n_part = ipt = 0; ipt < WII_MAX_PTAB && !err; ipt++ )
	{
	    wd_ptab_info_t * pt = disc->ptab_info + ipt;
	    int n = ntohl(pt->n_part);
	    if ( n > disc->n_part - n_part ) // limit n
		 n = disc->n_part - n_part;

	    if (n)
	    {
		disc->n_ptab++;
		wd_ptab_entry_t * pe = disc->ptab_entry + n_part;
		err = wd_read_raw( disc, ntohl(pt->off4),
					pe, n*sizeof(*pe), WD_USAGE_DISC );
		if (!err)
		{
		    int pi;
		    for ( pi = 0; pi < n; pi++, pe++ )
		    {
			wd_part_t * part = wd_initialize_part(disc,n_part+pi);
			part->ptab_index	= ipt;
			part->ptab_part_index	= pi;
			part->part_type		= ntohl(pe->ptype);
			part->part_off4		= ntohl(pe->off4);
		    }
		}
		n_part += n;
	    }
	}
	DASSERT( err || n_part == disc->n_part );
    }


    //----- read region settings and magic2

    if (!err)
	err = wd_read_raw( disc, WII_REGION_OFF>>2, &disc->region,
				sizeof(disc->region), WD_USAGE_DISC );

    if (!err)
	err = wd_read_raw( disc, WII_MAGIC2_OFF>>2, &disc->magic2,
				sizeof(disc->magic2), WD_USAGE_DISC );


    //----- check unused head sectors

    if (!err)
    {
	int sector;
	for ( sector = 0; sector < WII_PART_OFF/WII_SECTOR_SIZE; sector++ )
	    if ( disc->usage_table[sector] == WD_USAGE_UNUSED )
	    {
		err = wd_read_raw( disc, sector * WII_SECTOR_SIZE4, disc->temp_buf,
				    WII_SECTOR_SIZE, WD_USAGE_UNUSED );
		if (err)
		    break;

		u32 * ptr = (u32*)disc->temp_buf;
		u32 * end = ptr + WII_SECTOR_SIZE4;
		while ( ptr < end )
		    if (*ptr++)
		    {
			WDPRINT("MARK SECTOR %u AS USED\n",sector);
			disc->usage_table[sector] = WD_USAGE_DISC;
			break;
		    }
	    }
    }

    //----- terminate

 #ifdef DEBUG
    wd_print_disc(TRACE_FILE,10,disc,0);
 #endif

    if (err)
    {
	WD_ERROR(ERR_WDISC_NOT_FOUND,"Can't open wii disc%s",disc->error_term);
	wd_close_disc(disc);
	disc = 0;
    }

    if (error_code)
	*error_code = err;

#if 0 && defined(TEST) && defined(DEBUG) // [[2do]]
    if (disc)
    {
	static int tab[] = { 0, 4, 7935, 7936, 7940, 7941, -1 };

	PRINT("+++++++\n");
	int i;
	for ( i = 0; tab[i] >= 0; i++ )
	{
	    wd_sector_status_t ss = wd_get_disc_sector_status(disc,tab[i],false);
	    printf("%5d: %04x |%s|\n",
		    tab[i], ss, wd_log_sector_status(0,0,ss) );
	}
	PRINT("-------\n");
    }
#endif

    return disc;
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_t * wd_dup_disc
(
    wd_disc_t		* disc		// NULL or a valid disc pointer
)
{
    TRACE("wd_dup_disc(%p) open_count=%d\n", disc, disc ? disc->open_count : -1 );
    if (disc)
	disc->open_count++;
    return disc;
}

///////////////////////////////////////////////////////////////////////////////

void wd_close_disc
(
    wd_disc_t		* disc		// NULL or valid disc pointer
)
{
    TRACE("wd_close_disc(%p) open_count=%d\n", disc, disc ? disc->open_count : -1 );

    if ( disc && --disc->open_count <= 0 )
    {
	wd_reset_memmap(&disc->patch);
	if (disc->part)
	{
	    int pi;
	    wd_part_t * part = disc->part;
	    for ( pi = 0; pi < disc->n_part; pi++, part++ )
	    {
		wd_reset_memmap(&part->patch);
		cert_reset(&part->cert_chain);
		FREE(part->tmd);
		FREE(part->cert);
		FREE(part->h3);
		FREE(part->setup_txt);
		FREE(part->setup_sh);
		FREE(part->setup_bat);
		FREE(part->fst);
	    } 
	    FREE(disc->part);
	} 
	FREE(disc->ptab_entry);
	FREE(disc->reloc);
	FREE(disc->group_cache);
	FREE(disc);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get status			///////////////
///////////////////////////////////////////////////////////////////////////////

bool wd_disc_has_ptab
(
    wd_disc_t		*disc		// valid disc pointer
)
{
    DASSERT(disc);
    return disc->disc_type != WD_DT_GAMECUBE;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_disc_has_region_setting
(
    wd_disc_t		*disc		// valid disc pointer
)
{
    DASSERT(disc);
    return disc->disc_type != WD_DT_GAMECUBE;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_part_has_ticket
(
    wd_part_t		*part		// valid disc partition pointer
)
{
    DASSERT(part);
    return !part->is_gc;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_part_has_tmd
(
    wd_part_t		*part		// valid disc partition pointer
)
{
    DASSERT(part);
    return !part->is_gc && part->tmd;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_part_has_cert
(
    wd_part_t		*part		// valid disc partition pointer
)
{
    DASSERT(part);
    return !part->is_gc && part->ph.cert_off4;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_part_has_h3
(
    wd_part_t		*part		// valid disc partition pointer
)
{
    DASSERT(part);
    return !part->is_gc && part->ph.h3_off4;
}

///////////////////////////////////////////////////////////////////////////////

uint wd_disc_is_encrypted
(
    // return
    //    0: no partition is encrypted
    //    N: some (=N) partitions are encrypted, but not all
    // 1000: all partitions are encrypted

    wd_disc_t		* disc		// disc to check, valid
)
{
    DASSERT(disc);

    int pi, n_encrypted = 0, n_total = 0;
    for ( pi = 0; pi < disc->n_part; pi++ )
    {
	wd_part_t *part = disc->part + pi;
	const enumError err = wd_load_part(part,false,false,true);
	if (err)
	    continue;
	n_total++;
	if ( part->is_encrypted)
	    n_encrypted++;
    }

    return !n_encrypted ? 0 : n_encrypted == n_total ? 1000 : n_encrypted;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get partition data		///////////////
///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_index
(
	wd_disc_t	*disc,		// valid disc pointer
	int		index,		// zero based partition index within wd_disc_t
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_index(%p,%d,%d)\n",disc,index,load_data);
    ASSERT(disc);

    if ( index < 0 || index >= disc->n_part )
	return 0;

    wd_part_t * part = disc->part + index;
    part->disc = disc;

    if ( load_data > 0 )
	wd_load_part( part, load_data > 1, load_data > 2, false );

    return part;
}

///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_usage
(
	wd_disc_t	* disc,		// valid disc pointer
	u8		usage_id,	// value of 'usage_table'
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_usage(%p,%02x,%d)\n",disc,usage_id,load_data);
    const int idx = (int)( usage_id & WD_USAGE__MASK ) - WD_USAGE_PART_0;
    return wd_get_part_by_index(disc,idx,load_data);
}

///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_ptab
(
	wd_disc_t	*disc,		// valid disc pointer
	int		ptab_index,	// zero based index of partition table
	int		ptab_part_index,// zero based index within owning partition table
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_ptab(%p,%d,%d,%d)\n",
		disc, ptab_index, ptab_part_index, load_data );
    ASSERT(disc);

    wd_part_t * part = disc->part;
    int i;
    for ( i = 0; i < disc->n_part; i++, part++ )
	if ( part->ptab_index == ptab_index
	  && part->ptab_part_index == ptab_part_index )
	{
	    return wd_get_part_by_index(disc,i,load_data);
	}
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

wd_part_t * wd_get_part_by_type
(
	wd_disc_t	*disc,		// valid disc pointer
	u32		part_type,	// find first partition with this type
	int		load_data	// >0: load partition data from disc
					// >1: load cert too
					// >2: load h3 too
)
{
    TRACE("wd_get_part_by_ptab(%p,%x,%d)\n",disc,part_type,load_data);
    ASSERT(disc);

    wd_part_t * part = disc->part;
    int i;
    for ( i = 0; i < disc->n_part; i++, part++ )
	if ( part->part_type == part_type )
	    return wd_get_part_by_index(disc,i,load_data);
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			load partition data		///////////////
///////////////////////////////////////////////////////////////////////////////

static enumError wd_check_part_offset
(
    wd_part_t	*part,		// valid disc partition pointer
    u64		off,		// offset to test
    u64		size,		// size to test
    ccp		name,		// name of object
    bool	silent		// true: don't print error messages
)
{
    DASSERT(part);
    DASSERT(part->disc);
    const u64 iso_size = part->disc->iso_size;
    if (iso_size)
    {
	noTRACE("TEST '%s': %llx + %llx <= %llx\n",name,off,size,iso_size);
	if ( off > iso_size )
	{
	    if (!silent)
		WD_ERROR(ERR_WPART_INVALID,
		    "Partition %s: Offset of %s (%#llx) behind end of file (%#llx)%s",
		    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
		    name, off, iso_size, part->disc->error_term );
	    return part->disc->force > 0 ? ERR_WARNING : ERR_WPART_INVALID;
	}

	off += size;
	if ( off > iso_size )
	{
	    if (!silent)
		WD_ERROR(ERR_WPART_INVALID,
		    "Partition %s: End of %s (%#llx) behind end of file (%#llx)%s",
		    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
		    name, off, iso_size, part->disc->error_term );
	    return part->disc->force > 0 ? ERR_WARNING : ERR_WPART_INVALID;
	}
    }

    return ERR_OK;
}

//-----------------------------------------------------------------------------

const int SYS_DIR_COUNT		= 3;
const int SYS_FILE_COUNT	= 12;

const int SYS_DIR_COUNT_GC	= 2;
const int SYS_FILE_COUNT_GC	= 6;

//-----------------------------------------------------------------------------

enumError wd_load_part
(
    wd_part_t		*part,		// valid disc partition pointer
    bool		load_cert,	// true: load cert data too
    bool		load_h3,	// true: load h3 data too
    bool		silent		// true: don't print error messages
)
{
    TRACE("wd_load_part(%p,%d,%d)\n",part,load_cert,load_h3);

    ASSERT(part);
    wd_disc_t * disc = part->disc;
    ASSERT(disc);

    const wd_part_header_t * ph = &part->ph;

    const bool load_now = !part->is_loaded;
    if (load_now)
    {
	TRACE(" - load now\n");

	// assume loaded, but invalid
	part->is_loaded = true;
	part->is_valid  = false;
	part->disc->invalid_part++;

	FREE(part->tmd);  part->tmd  = 0;
	FREE(part->cert); part->cert = 0;
	FREE(part->h3);   part->h3   = 0;
	FREE(part->fst);  part->fst  = 0;


	//----- scan partition header

	if (part->is_gc)
	    part->data_off4 = part->part_off4;
	else
	{
	    //----- load partition header

	    const u64 part_off = (u64)part->part_off4 << 2;
	    if (wd_check_part_offset(part,part_off,sizeof(part->ph),"TICKET",silent))
		return ERR_WPART_INVALID;

	    enumError err = wd_read_part_raw(part,0,&part->ph,sizeof(part->ph),true);
	    if (err)
	    {
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Can't read TICKET of partition '%s'%s",
				    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				    disc->error_term );
		return ERR_WPART_INVALID;
	    }

	    ntoh_part_header(&part->ph,&part->ph);
	    wd_decrypt_title_key(&ph->ticket,part->key);

	    part->data_off4	= part->part_off4 + ph->data_off4;
	    part->data_sector	= part->data_off4 / WII_SECTOR_SIZE4;
	    part->part_size	= (u64)( part->data_off4 + ph->data_size4 - part->part_off4 ) << 2;

	    TRACE("part=%llx,%llx, tmd=%llx,%x, cert=%llx,%x, h3=%llx, data=%llx,%llx\n",
		(u64)part->part_off4	<< 2,	part->part_size,
		(u64)ph->tmd_off4	<< 2,	ph->tmd_size,
		(u64)ph->cert_off4	<< 2,	ph->cert_size,
		(u64)ph->h3_off4	<< 2,
		(u64)ph->data_off4	<< 2,	(u64)ph->data_size4 << 2 );


	    //----- file size tests

	    if (wd_check_part_offset(part, part_off+((u64)ph->data_off4<<2),
			(u64)ph->data_size4<<2, "data", silent ) > ERR_WARNING )
		return ERR_WPART_INVALID;


	    //----- load tmd

	    if ( ph->tmd_size < sizeof(wd_tmd_t)
		|| ((u64)ph->tmd_off4<<2) + ph->tmd_size > ((u64)ph->data_off4<<2) )
	    {
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Invalid TMD size (0x%x) in partition '%s'%s",
			ph->tmd_size,
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
			disc->error_term );
		return ERR_WPART_INVALID;
	    }

	    wd_tmd_t * tmd = MALLOC(ph->tmd_size);
	    part->tmd = tmd;
	    err = wd_read_part_raw( part, ph->tmd_off4, tmd, ph->tmd_size, true );
	    if (err)
	    {
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Can't read TMD of partition '%s'%s",
				wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				disc->error_term );
		return ERR_WPART_INVALID;
	    }


	    //----- check encryption
	    //   -> call wd_is_block_encrypted() first because of 'sector_stat' setup

	    part->is_encrypted = wd_is_block_encrypted(part,0,1,false)
				&& !tmd_is_marked_not_encrypted(tmd);
	    TRACE("is_encrypted=%d\n",part->is_encrypted);

	} // !is_gc


	//----- load boot.bin 

	enumError err = wd_read_part(part,WII_BOOT_OFF>>2,
			disc->temp_buf,WII_BOOT_SIZE+WII_BI2_SIZE,true);
	if (err)
	{
	    if (!silent)
		WD_ERROR(ERR_WPART_INVALID,
			"Can't read BOOT.BIN of partition '%s'%s",
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
			disc->error_term );
	    return ERR_WPART_INVALID;
	}

	if (part->is_gc)
	{
	    const uint ssm = sizeof(skeleton_marker);
	    const uint off = WII_BI2_OFF + WII_BI2_SIZE - ssm;
	    if ( !memcmp( disc->temp_buf + off, skeleton_marker, ssm ))
	    {
		part->sector_stat |= WD_SS_F_SKELETON;
		PRINT("*** wd_load_part() SKELETON ***\n");
	    }
	}

	wd_boot_t * boot = &part->boot;
	ntoh_boot(boot,(wd_boot_t*)disc->temp_buf);
	part->region = be32(disc->temp_buf+WII_BI2_OFF+WII_BI2_REGION_OFF);
	if (part->is_gc)
	    part->part_type = be32(boot);

	if ( !boot->dol_off4 && !boot->fst_off4 )
	{
	    if (!silent)
		WD_ERROR(ERR_WPART_INVALID,
			"Invalid BOOT.BIN (no MAIN.DOL and no FST), partition '%s'%s",
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
			disc->error_term );
	    return ERR_WPART_INVALID;
	}


	//----- setup "setup.txt", "setup.sh", "setup.bat"

	// get data

	ccp disc_type	= wd_get_disc_type_name(disc->disc_type,"?");
	ccp image_type	= disc->image_type ? disc->image_type : "?";
	ccp image_ext	= disc->image_ext ? disc->image_ext : "";
	ccp part_id	= wd_print_id(&boot->dhead.disc_id,6,0);
	ccp part_title	= boot->dhead.disc_title;

	char norm_title[WII_TITLE_SIZE+1];
	{
	    ccp src = part_title;
	    ccp end = part_title + WII_TITLE_SIZE;
	    char *dest = norm_title;
	    while ( src < end && *src )
	    {
		uchar ch = *src++;
		*dest++ = ch < ' ' || ch == '"' ? ' ' : ch;
	    }
	    *dest = 0;
	}

	// "setup.txt"
	part->setup_txt_len
	    = snprintf((char*)disc->temp_buf,sizeof(disc->temp_buf),
			"# setup.txt : scanned by wit+wwt while composing a disc.\n"
			"# remove the '!' before names to activate parameters.\n"
			"\n"
			"disc-type = %s\n"
			"image-type = %s\n"
			"\n"
			"!part-id = %.6s\n"
			"!part-name = %.64s\n"
			"!part-offset = 0x%llx\n"
			"\n"
			,disc_type
			,image_type
			,part_id
			,part_title
			,(u64)part->part_off4 << 2
			);
	part->setup_txt = MEMDUP(disc->temp_buf,part->setup_txt_len);

	// "setup.sh"
	part->setup_sh_len
	    = snprintf((char*)disc->temp_buf,sizeof(disc->temp_buf),
			"DISC_TYPE=\"%s\"\n"
			"IMAGE_TYPE=\"%s\"\n"
			"IMAGE_EXT=\"%s\"\n"
			"PART_ID=\"%.6s\"\n"
			"PART_NAME=\"%s\"\n"
			,disc_type
			,image_type
			,image_ext
			,part_id
			,norm_title
			);
	part->setup_sh = MEMDUP(disc->temp_buf,part->setup_sh_len);

	// "setup.bat"
	part->setup_bat_len
	    = snprintf((char*)disc->temp_buf,sizeof(disc->temp_buf),
			"set DISC_TYPE=%s\r\n"
			"set IMAGE_TYPE=%s\r\n"
			"set IMAGE_EXT=%s\r\n"
			"set PART_ID=%.6s\r\n"
			"set PART_NAME=%s\r\n"
			,disc_type
			,image_type
			,image_ext
			,part_id
			,norm_title
			);
	part->setup_bat = MEMDUP(disc->temp_buf,part->setup_bat_len);


	//----- calculate size of main.dol

	u32 fst_n		= 0;
	u32 fst_dir_count	= part->is_gc ? SYS_DIR_COUNT_GC  : SYS_DIR_COUNT;
	u32 fst_file_count	= part->is_gc ? SYS_FILE_COUNT_GC : SYS_FILE_COUNT;
	u32 fst_max_off4	= part->data_off4;
	u32 fst_max_size	= WII_H3_SIZE;

	if (!boot->dol_off4)
	    fst_file_count--;
	else
	{
	    dol_header_t * dol = (dol_header_t*) disc->temp_buf;

	    err = wd_read_part(part,boot->dol_off4,dol,DOL_HEADER_SIZE,true);
	    if (err)
	    {
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Can't read MAIN.DOL in partition '%s'%s",
				    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				    disc->error_term );
		return ERR_WPART_INVALID;
	    }
	    ntoh_dol_header(dol,dol);

	    // iterate through all segments
	    int i;
	    u32 dol_size = 0;
	    for ( i = 0; i < DOL_N_SECTIONS; i++ )
	    {
		const u32 size = dol->sect_off[i] + dol->sect_size[i];
		if ( dol_size < size )
		     dol_size = size;
	    }
	    TRACE("DOL-SIZE: %x <= %x\n",dol_size,boot->fst_off4-boot->dol_off4<<2);
	    if ( dol_size > boot->fst_off4 - boot->dol_off4 << 2 )
	    {
		WDPRINT("!!! DOL-SIZE: %x <= %x & off=%x\n",
			dol_size, boot->fst_off4-boot->dol_off4<<2, boot->dol_off4 );
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Invalid MAIN.DOL in partition '%s'%s",
				    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				    disc->error_term );
		return ERR_WPART_INVALID;
	    }
	    part->dol_size = dol_size;
	    if ( fst_max_size < dol_size )
		 fst_max_size = dol_size;

	    wd_mark_part(part,boot->dol_off4,dol_size);
	}


	//----- calculate size of apploader.img

	{
	    u8 *apl_header = (u8*)disc->temp_buf;
	    const u32 apl_off4 = part->is_gc ? WII_APL_OFF : WII_APL_OFF >> 2;
	    err = wd_read_part(part,apl_off4,apl_header,0x20,false);
	    if (err)
	    {
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Can't read APPLOADER of partition '%s'%s",
				    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				    disc->error_term );
		return ERR_WPART_INVALID;
	    }
	    part->apl_size = 0x20 + be32(apl_header+0x14) + be32(apl_header+0x18);
	    if ( fst_max_size < part->apl_size )
		 fst_max_size = part->apl_size;

	    wd_mark_part(part,WII_APL_OFF>>2,part->apl_size);
	}


	//----- calculate offset and size of user.bin

 #if SUPPORT_USER_BIN > 0
	{
	    const u32 off = ALIGN32( WII_APL_OFF + part->apl_size, WIIUSER_DATA_ALIGN );
	    noPRINT(">>> user.bin offset: %x\n",off);

	    wiiuser_header_t *wuh = (wiiuser_header_t*)disc->temp_buf;
	    err = wd_read_part(part,off>>2,wuh,sizeof(*wuh),false);
	    if ( !err && !memcmp(wuh->magic,WIIUSER_MAGIC,sizeof(wuh->magic)) )
	    {
		noPRINT(">>> user.bin FOUND! vers=%u, size=%x\n",
			ntohl(wuh->version), ntohl(wuh->size) );
		part->ubin_off4 = ( off + sizeof(wiiuser_header_t) ) >> 2;
		part->ubin_size = ntohl(wuh->size);
		wd_mark_part(part,part->ubin_off4,part->ubin_size);
	    }
	}
 #endif


	//----- load and iterate fst

	u32 mgr_sect = part->end_sector;
	u32 fst_size = boot->fst_size4;

	if (fst_size)
	{
	    if (!part->is_gc)
		fst_size <<= 2;
	    TRACE("fst_size=%x\n",fst_size);

	    wd_fst_item_t * fst = MALLOC(fst_size);
	    part->fst = fst;
	    err = wd_read_part(part,boot->fst_off4,fst,fst_size,true);
	    if (err)
	    {
		if (!silent)
		    WD_ERROR(ERR_WPART_INVALID,"Can't read FST of partition '%s'%s",
				    wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO),
				    disc->error_term );
		return ERR_WPART_INVALID;
	    }
	    mgr_sect = part->end_sector;

	    // mark used blocks

	    fst_dir_count++; // directory './files/'
	    fst_n = ntohl(fst->size);
	    const wd_fst_item_t *fst_end = fst + fst_n;

	    bool in_wad_dir = false;

	    for ( fst++; fst < fst_end; fst++ )
		if (fst->is_dir)
		{
		    fst_dir_count++;
		    in_wad_dir = false;
		    if ( part->part_type == WD_PART_UPDATE )
		    {
			ccp fname = (ccp)fst_end + (ntohl(fst->name_off)&0xffffff);
			in_wad_dir = fname < (ccp)part->fst + fst_size
					&& !strcasecmp(fname,"_sys");
		    }
		}
		else
		{
		    fst_file_count++;
		    const u32 off4 = ntohl(fst->offset4);
		    if ( fst_max_off4 < off4 )
			 fst_max_off4 = off4;
		    const u32 size = ntohl(fst->size);
		    if ( fst_max_size < size )
			 fst_max_size = size;
		    wd_mark_part(part,off4,size);

		    if ( in_wad_dir )
		    {
			ccp fname = (ccp)fst_end + (ntohl(fst->name_off)&0xffffff);
			if ( fname < (ccp)part->fst + fst_size
			    && !strncasecmp(fname,"RVL-WiiSystemmenu-v",19)
			    && !strncasecmp(fname+strlen(fname)-4,".wad",4) )
			{
			    const u32 num = strtoul(fname+19,0,10);
			    if ( disc->system_menu < num )
			    {
				noPRINT("SYS-MENU: %u -> %u [%s]\n",
					disc->system_menu,num,fname);
				disc->system_menu = num;
			    }
			}
		    }
		}
	}

	part->fst_n		= fst_n;
	part->fst_max_off4	= fst_max_off4;
	part->fst_max_size	= fst_max_size;
	part->fst_dir_count	= fst_dir_count;
	part->fst_file_count	= fst_file_count;


	//----- gamecube settings

	if (part->is_gc)
	{
	    part->data_sector	= part->data_off4 / WII_SECTOR_SIZE4;
	    part->end_sector	= part->data_sector
				+ ( part->max_marked + WII_SECTOR_SIZE - 1 ) / WII_SECTOR_SIZE;
	    if ( part->end_sector > WII_MAX_SECTORS )
		 part->end_sector = WII_MAX_SECTORS;

	    part->part_size	= part->max_marked - ( (u64)part->data_off4 << 2 );
	    noPRINT(">> S=%x .. %x / %llx .. %llx / %llx / %llx\n",
			part->data_sector, part->end_sector,
			(u64)part->data_off4<<2, part->part_size,
			part->max_marked, disc->iso_size );

	    if (wd_check_part_offset( part, (u64)part->data_off4<<2,
				part->part_size, "PART", silent ) > ERR_WARNING )
		return ERR_WPART_INVALID;
	}
	else
	{
	    const u32 last_sect = part->data_sector + ph->data_size4 / WII_SECTOR_SIZE4;
	    noPRINT("last_sect=%x, end_sector=%x\n",last_sect,part->end_sector);
	    if ( part->end_sector < last_sect )
		 part->end_sector = last_sect;
	}

	if ( part->end_sector > WII_MAX_SECTORS )
	     part->end_sector = WII_MAX_SECTORS;

	mgr_sect = ( mgr_sect - part->data_sector + WII_GROUP_SECTORS - 1 )
		 / WII_GROUP_SECTORS * WII_GROUP_SECTORS + part->data_sector;
	if ( mgr_sect > part->end_sector )
	     mgr_sect = part->end_sector;
	part->end_mgr_sector = mgr_sect;

	WDPRINT("PART #%u, sectors: %llx .. %llx .. %llx .. %llx\n",
		part->index,
		(u64)WII_SECTOR_SIZE * part->data_sector,
		(u64)WII_SECTOR_SIZE * part->end_mgr_sector,
		(u64)WII_SECTOR_SIZE * part->end_sector,
		(u64)WII_SECTOR_SIZE * WII_MAX_SECTORS );


	//----- all done => mark as valid

	part->is_valid = true;
	wd_calc_fst_statistics(disc,false);


	//----- check overlays

	int p2;
	u32 end4a = part->part_off4 + (u32)(part->part_size>>2);
	for ( p2 = 0; p2 < disc->n_part; p2++ )
	{
	    wd_part_t * part2 = disc->part + p2;
	    if ( part == part2 )
		continue;

	    if (   part2->part_off4 >= part->part_off4
		&& part2->part_off4 < end4a )
	    {
		part->is_overlay = true;
		part2->is_overlay = true;
		continue;
	    }

	    if ( part2->is_loaded && part2->is_valid )
	    {
		u32 end4b = part2->part_off4 + (u32)(part2->part_size>>2);
		if (   part->part_off4 >= part2->part_off4
		    && part->part_off4 < end4b )
		{
		    part->is_overlay = true;
		    part2->is_overlay = true;
		    continue;
		}
	    }
	}

     #if 0 && defined(TEST) && defined(DEBUG) // [[2do]]
	{
	    int i;
	    for ( i = -1; i < 10; i += 1 )
	    {
		wd_sector_status_t ss = wd_get_part_sector_status(part,i,silent);
		printf("%2zu.%03d: %04x |%s|\n",
			part-disc->part, i, ss, wd_log_sector_status(0,0,ss) );
	    }
	}
     #endif

    }

    if (part->is_valid)
    {
	if ( ph->cert_off4 )
	{
	    if ( !part->cert && load_cert )
	    {
		part->cert = MALLOC(ph->cert_size);
		wd_read_part_raw( part, ph->cert_off4, part->cert,
					ph->cert_size, true );
	    }
	    else if (load_now)
		wd_read_part_raw( part, ph->cert_off4, 0, ph->cert_size, true );
	}

	if ( ph->h3_off4 )
	{
	    if ( !part->h3 && load_h3 )
	    {
		part->h3 = MALLOC(WII_H3_SIZE);
		wd_read_part_raw( part, ph->h3_off4, part->h3, WII_H3_SIZE, true );
	    }
	    else if (load_now)
		wd_read_part_raw( part, ph->h3_off4, 0, WII_H3_SIZE, true );
	}
    }

    TRACELINE;
    return part->is_valid ? ERR_OK : ERR_WPART_INVALID;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_load_all_part
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		load_cert,	// true: load cert data too
    bool		load_h3,	// true: load h3 data too
    bool		silent		// true: don't print error messages
)
{
    ASSERT(disc);

    enumError max_err = ERR_OK;
    int pi;
    for ( pi = 0; pi < disc->n_part; pi++ )
    {
	DASSERT(disc->part);
	const enumError err = wd_load_part(disc->part+pi,load_cert,load_h3,silent);
	if ( max_err < err )
	     max_err = err;
    }

    return max_err;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_calc_fst_statistics
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		sum_all		// false: summarize only enabled partitions
)
{
    DASSERT(disc);

    u32  invalid	= 0;
    u32  n		= 0;
    u32  max_off4	= 0;
    u32  max_size	= 0;
    u32  dir_count	= 0;
    u32  file_count	= 0;
    bool have_overlays	= false;

    disc->data_part = disc->update_part = disc->channel_part = disc->main_part = 0;
    wd_part_t *part, *part_end = disc->part + disc->n_part;
    for ( part = disc->part; part < part_end; part++ )
    {
	invalid += !part->is_valid;
	part->is_ok = part->is_loaded && part->is_valid && part->is_enabled;
	if ( sum_all || part->is_ok )
	{
	    n		+= part->fst_n;
	    dir_count	+= part->fst_dir_count;
	    file_count	+= part->fst_file_count;

	    if ( max_off4 < part->fst_max_off4 )
		 max_off4 = part->fst_max_off4;
	    if ( max_size < part->fst_max_size )
		 max_size = part->fst_max_size;

	    if ( part->is_overlay )
		have_overlays = true;
	}

	if (part->is_ok)
	{
	    switch (part->part_type)
	    {
		case WD_PART_DATA:
		    if (!disc->data_part)
			disc->data_part = part;
		    break;

		case WD_PART_UPDATE:
		    if (!disc->update_part)
			disc->update_part = part;
		    break;

		case WD_PART_CHANNEL:
		    if (!disc->channel_part)
			disc->channel_part = part;
		    break;
	    }
	    if (!disc->main_part)
		disc->main_part = part;
	}
    }

    if (disc->data_part)
	disc->main_part = disc->data_part;
    else if (disc->update_part)
	disc->main_part = disc->update_part;
    else if (disc->channel_part)
	disc->main_part = disc->channel_part;
    TRACE("D_PART=%2zd U_PART=%2zd C_PART=%2zd M_PART=%2zd\n",
	disc->data_part	   ? disc->data_part    - disc->part : -1,
	disc->update_part  ? disc->update_part  - disc->part : -1,
	disc->channel_part ? disc->channel_part - disc->part : -1,
	disc->main_part    ? disc->main_part    - disc->part : -1 );

    disc->invalid_part	 = invalid;
    disc->fst_n		 = n;
    disc->fst_max_off4	 = max_off4;
    disc->fst_max_size	 = max_size;
    disc->fst_dir_count	 = dir_count;
    disc->fst_file_count = file_count;
    disc->have_overlays	 = have_overlays;

    return invalid ? ERR_WDISC_INVALID : ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		interface: certificate support		///////////////
///////////////////////////////////////////////////////////////////////////////

const cert_chain_t * wd_load_cert_chain
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
)
{
    DASSERT(part);

    if (!part->cert)
	wd_load_part(part,true,false,silent);

    if ( part->cert && !part->cert_chain.used )
	cert_append_data( &part->cert_chain, part->cert, part->ph.cert_size, false );

    return &part->cert_chain;
}

///////////////////////////////////////////////////////////////////////////////

cert_stat_t wd_get_cert_ticket_stat
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
)
{
    DASSERT(part);
    DASSERT(part->disc);

    if (!part->cert_tik_stat)
    {
	const cert_chain_t * cc = wd_load_cert_chain(part,silent);
	part->cert_tik_stat = cert_check_ticket(cc,&part->ph.ticket,0);
	part->disc->cert_summary |= part->cert_tik_stat;
    }

    return part->cert_tik_stat;
}

///////////////////////////////////////////////////////////////////////////////

cert_stat_t wd_get_cert_tmd_stat
(
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
)
{
    DASSERT(part);
    DASSERT(part->disc);

    if (!part->cert_tmd_stat)
    {
	const cert_chain_t * cc = wd_load_cert_chain(part,silent);
	part->cert_tmd_stat = cert_check_tmd(cc,part->tmd,0);
	part->disc->cert_summary |= part->cert_tmd_stat;
    }

    return part->cert_tmd_stat;
}

///////////////////////////////////////////////////////////////////////////////

ccp wd_get_sig_status_short_text
(
    cert_stat_t		sig_stat
)
{
    return  sig_stat & CERT_F_HASH_FAILED
		? "no-sig"
		: sig_stat & CERT_F_HASH_FAKED
			? "faked"
			: sig_stat & CERT_F_HASH_OK
				? "signed"
				: 0;
}

///////////////////////////////////////////////////////////////////////////////

ccp wd_get_sig_status_text
(
    cert_stat_t		sig_stat
)
{
    return  sig_stat & CERT_F_HASH_FAILED
		? "not signed"
		: sig_stat & CERT_F_HASH_FAKED
			? "fake signed"
			: sig_stat & CERT_F_HASH_OK
				? "well signed"
				: 0;
}


///////////////////////////////////////////////////////////////////////////////

char * wd_print_sig_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent,		// true: don't print errors while loading data
    bool		add_enc_info	// true: append " Partition is *crypted."
)
{
    DASSERT(part);

    if (!buf)
	buf = GetCircBuf( buf_size = 80 );

    ccp enc_info = !add_enc_info
			? ""
			: part->is_encrypted
				? " Partition is encrypted"
				: " Partition is decrypted";
    ccp scrub_info = !add_enc_info
			? ""
			: wd_is_part_scrubbed(part,silent)
				? " and scrubbed."
				: ".";

//    const cert_stat_t mask = CERT_F_HASH_OK | CERT_F_HASH_FAKED | CERT_F_HASH_FAILED;
//    const cert_stat_t tik_stat = wd_get_cert_ticket_stat(part,silent) & mask;
//    const cert_stat_t tmd_stat = wd_get_cert_tmd_stat(part,silent) & mask;

    ccp tik_text = wd_get_sig_status_text(wd_get_cert_ticket_stat(part,silent));
    ccp tmd_text = wd_get_sig_status_text(wd_get_cert_tmd_stat(part,silent));

    if ( tik_text == tmd_text )
    {
	if (!tik_text)
	    snprintf(buf,buf_size,
		"Signing of TICKET & TMD is unknown.%s%s", enc_info, scrub_info );
	else
	    snprintf(buf,buf_size,
		"TICKET & TMD are %s.%s%s", tik_text, enc_info, scrub_info );
    }
    else
    {
	if (!tik_text)
	    snprintf(buf,buf_size,
		"Signing of TICKET is unknown. TMD is %s.%s%s",
			tmd_text, enc_info, scrub_info );
	else if (!tmd_text)
	    snprintf(buf,buf_size,
		"TICKET is %s. Signing of TMD is unknown.%s%s",
			tik_text, enc_info, scrub_info );
	else
	{
	    DASSERT( tik_text && tmd_text );
	    snprintf(buf,buf_size,
		"TICKET is %s. TMD is %s.%s%s",
			tik_text, tmd_text, enc_info, scrub_info );
	}
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * wd_print_part_status
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_part_t		* part,		// valid disc partition pointer
    bool		silent		// true: don't print errors while loading cert
)
{
    DASSERT(part);

    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    if (part->is_enabled)
    {
	ccp enc_info   = part->is_encrypted ? "enc" : "dec";
	ccp sign_info  = wd_get_sig_status_short_text
			( wd_get_cert_ticket_stat(part,silent)
			| wd_get_cert_tmd_stat(part,silent));
	ccp scrub_info = wd_is_part_scrubbed(part,silent) ? ",scrub" : "";
	if ( part->sector_stat & WD_SS_F_SKELETON )
	    scrub_info = ",skel";
	snprintf(buf,buf_size,"%s,%s%s",enc_info,sign_info,scrub_info);
    }
    else
	snprintf(buf,buf_size,"disabled");

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_calc_part_status
(
    wd_part_t		* part,		// valid pointer to a disc partition
    bool		silent		// true: don't print error messages
)
{
    DASSERT(part);
    DASSERT(part->disc);

    const enumError err = wd_load_part(part,true,false,silent);
    wd_get_cert_ticket_stat(part,silent);
    wd_get_cert_tmd_stat(part,silent);
    wd_is_part_scrubbed(part,silent);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_calc_disc_status
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		silent		// true: don't print error messages
)
{
    DASSERT(disc);

    int ip;
    enumError max_err = ERR_OK;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	const enumError err = wd_calc_part_status(disc->part+ip,silent);
	if ( max_err < err )
	     max_err = err;
    }

    disc->scrub_test_done = true;
    return max_err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			select partitions		///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_initialize_select
(
    wd_select_t		* select	// valid pointer to a partition selector
)
{
    DASSERT(select);
    memset(select,0,sizeof(*select));
}

///////////////////////////////////////////////////////////////////////////////

void wd_reset_select
(
    wd_select_t		* select	// valid pointer to a partition selector
)
{
    DASSERT(select);
    FREE(select->list);
    memset(select,0,sizeof(*select));
}

///////////////////////////////////////////////////////////////////////////////

wd_select_item_t * wd_append_select_item
(
    wd_select_t		* select,	// valid pointer to a partition selector
    wd_select_mode_t	mode,		// select mode of new item
    u32			table,		// partition table of new item
    u32			part		// partition type or index of new item
)
{
    DASSERT(select);
    if ( select->used == select->size )
    {
	select->size += 10;
	select->list = REALLOC(select->list, select->size*sizeof(*select->list));
    }

    DASSERT( select->used < select->size );
    wd_select_item_t * item = select->list + select->used++;
    item->mode	= mode;
    item->table	= table;
    item->part	= part;

    return item;
}

///////////////////////////////////////////////////////////////////////////////

void wd_copy_select
(
    wd_select_t		* dest,		// valid pointer to a partition selector
    const wd_select_t	* source	// NULL or pointer to a partition selector
)
{
    DASSERT(dest);
    wd_reset_select(dest);
    if (source)
    {
	memcpy(dest,source,sizeof(*dest));
	dest->size = source->used;
	if (dest->size)
	{
	    const int list_size = dest->size * sizeof(*dest->list);
	    dest->list = MALLOC(list_size);
	    memcpy(dest->list,source->list,list_size);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_move_select
(
    wd_select_t		* dest,		// valid pointer to a partition selector
    wd_select_t		* source	// NULL or pointer to a partition selector
)
{
    DASSERT(dest);
    if ( source != dest )
    {
	wd_reset_select(dest);
	if (source)
	{
	    memmove(dest,source,sizeof(*dest));
	    wd_initialize_select(source);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_select
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_select_t		* select	// valid pointer to a partition selector
)
{
    DASSERT(f);
    DASSERT(select);

    indent = wd_normalize_indent(indent);

    if ( select->whole_disc )
	fprintf(f,"%*sFLAG: Copy whole disc (=raw mode), ignore all others.\n", indent, "" );

    if ( select->whole_part )
	fprintf(f,"%*sFLAG: Copy whole partitions.\n", indent, "" );

    bool default_deny = false;
    const wd_select_item_t * item = select->list;
    const wd_select_item_t * end  = item + select->used;
    for ( ; item < end; item++ )
    {
	bool allow = !( item->mode & WD_SM_F_DENY );
	ccp verb = allow ? "ALLOW" : "DENY ";
	switch ( item->mode & WD_SM_M_MODE )
	{
	    case WD_SM_ALLOW_PTYPE:
		fprintf(f,"%*s%s partition type %s\n",
			indent, "", verb,
			wd_print_part_name(0,0,item->part,WD_PNAME_NUM_INFO) );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_PTAB:
		fprintf(f,"%*s%s partition table %u (ignored for GameCube partitions)\n",
			indent, "", verb, item->table );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_INDEX:
		fprintf(f,"%*s%s partition index #%u\n",
			indent, "", verb, item->part );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_LT_INDEX:
		fprintf(f,"%*s%s partition index < #%u\n",
			indent, "", verb, item->part );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_GT_INDEX:
		fprintf(f,"%*s%s partition index > #%u\n",
			indent, "", verb, item->part );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_PTAB_INDEX:
		fprintf(f,"%*s%s partition index #%u.%u (ignored for GameCube partitions)\n",
			indent, "", verb, item->table, item->part );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_ID:
		fprintf(f,"%*s%s ID partitions.\n",
			indent, "", verb );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_GC_BOOT:
		fprintf(f,"%*s%s GameCube boot partition (ignored for Wii partitions)\n",
			indent, "", verb );
		default_deny = allow;
		break;

	    case WD_SM_ALLOW_ALL:
		fprintf(f,"%*s%s all partitions.\n", indent, "", verb );
		default_deny = allow;
		break;
	}
    }
    fprintf(f,"%*s%s all partitions (default rule)\n",
		indent, "", default_deny ? "DENY " : "ALLOW" );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool wd_is_part_selected
(
    wd_part_t		* part,		// valid pointer to a disc partition
    const wd_select_t	* select	// NULL or pointer to a partition selector
)
{
    DASSERT(part);

    if ( !select || part->is_gc && !( part->disc->disc_attrib & WD_DA_GC_MULTIBOOT ) )
	return true;

    bool allow = false;
    const wd_select_item_t * item = select->list;
    const wd_select_item_t * end  = item + select->used;
    for ( ; item < end; item++ )
    {
	int match;
	switch ( item->mode & WD_SM_M_MODE )
	{
	    case WD_SM_ALLOW_PTYPE:
		match =  item->part == part->part_type;
		break;

	    case WD_SM_ALLOW_PTAB:
		match = part->is_gc ? -1 : item->table == part->ptab_index;
		break;

	    case WD_SM_ALLOW_INDEX:
		match = part->index == item->part;
		break;

	    case WD_SM_ALLOW_LT_INDEX:
		match = part->index < item->part;
		break;

	    case WD_SM_ALLOW_GT_INDEX:
		match = part->index > item->part;
		break;

	    case WD_SM_ALLOW_PTAB_INDEX:
		match = part->is_gc
			?  -1
			:  item->table == part->ptab_index
			&& item->part == part->ptab_part_index;
		break;

	    case WD_SM_ALLOW_ID:
		{
		    ccp d = (ccp)&part->part_type;
		    match = ISALNUM(d[0]) && ISALNUM(d[1]) && ISALNUM(d[2]) && ISALNUM(d[3]);
		}
		break;

	    case WD_SM_ALLOW_GC_BOOT:
		match	= part->is_gc
			? !part->part_off4 && part->disc->disc_attrib & WD_DA_GC_MULTIBOOT
			: -1;
		break;

	    case WD_SM_ALLOW_ALL:
		match = 1;
		break;

	    default:
		match = -1;
	}

	if ( match >= 0 )
	{
	    allow = !( item->mode & WD_SM_F_DENY );
	    if ( match > 0 )
		return allow;
	}
    }

    return !allow;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_select // return true if selection changed
(
    wd_disc_t		* disc,		// valid disc pointer
    const wd_select_t	* select	// NULL or pointer to a partition selector
)
{
    DASSERT(disc);

    disc->whole_disc = select && select->whole_disc;
    disc->whole_part = select && select->whole_part;

    bool selection_changed = false;
    bool any_part_disabled = false;

    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = disc->part + ip;
	if (!part->is_loaded)
	    selection_changed = true;
	const bool is_enabled = part->is_enabled;
	wd_load_part(part,false,false,false);
	part->is_enabled = wd_is_part_selected(part,select);
	if ( part->is_enabled != is_enabled )
	    selection_changed = true;
	part->is_ok = part->is_loaded && part->is_valid && part->is_enabled;
	if ( !part->is_ok )
	    any_part_disabled = true;
    }

    wd_calc_fst_statistics(disc,false);
    disc->patch_ptab_recommended = any_part_disabled && !disc->whole_disc;

    noPRINT("wd_select(%p,%p) recom=%d chg=%d\n",
		disc, select,
		disc->patch_ptab_recommended, selection_changed );
    return selection_changed;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 usage table			///////////////
///////////////////////////////////////////////////////////////////////////////

const char wd_usage_name_tab[256] =
{
	".*ABCDEFGHIJKLMNOPQRSTUVWXYZABCD"
	"EFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJ"
	"KLMNOPQRSTUVWXYZABCDEFGHIJKLMNOP"
	"QRSTUVWXYZABCDEFGHIJKLMNOPQRSTUV"

	"?!abcdefghijklmnopqrstuvwxyzabcd"
	"efghijklmnopqrstuvwxyzabcdefghij"
	"klmnopqrstuvwxyzabcdefghijklmnop"
	"qrstuvwxyzabcdefghijklmnopqrstuv"
};

const u8 wd_usage_class_tab[256] =
{
	0,1,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
	2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
	2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,
	2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,  2,2,2,2, 2,2,2,2, 2,2,2,2, 2,2,2,2,

	4,5,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,
	3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3,  3,3,3,3, 3,3,3,3, 3,3,3,3, 3,3,3,3
};

///////////////////////////////////////////////////////////////////////////////

u8 * wd_calc_usage_table
(
    wd_disc_t		* disc		// valid disc partition pointer
)
{
    DASSERT(disc);
    wd_load_all_part(disc,false,false,false);
    return disc->usage_table;
}

///////////////////////////////////////////////////////////////////////////////

u8 * wd_filter_usage_table
(
    wd_disc_t		* disc,		// valid disc pointer
    u8			* usage_table,	// NULL or result. If NULL -> MALLOC()
    const wd_select_t	* select	// NULL or a new selector
)
{
    TRACE("wd_filter_usage_table()\n");

    if (select)
	wd_select(disc,select);

    wd_calc_usage_table(disc);

    if (!usage_table)
	usage_table = MALLOC(WII_MAX_SECTORS);

    memcpy(usage_table,disc->usage_table,WII_MAX_SECTORS);

    u8 transform[0x100];
    memset( transform, disc->whole_disc ? WD_USAGE_DISC : WD_USAGE_UNUSED, sizeof(transform) );
    transform[WD_USAGE_DISC] = WD_USAGE_DISC;

    disc->patch_ptab_recommended = false;

    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = disc->part + ip;

	if ( part->is_valid && part->is_enabled )
	{
	    u8 val = part->usage_id & WD_USAGE__MASK;
	    transform[val] = val;
	    val |= WD_USAGE_F_CRYPT;
	    transform[val] = val;

	    if ( disc->whole_part && !part->is_overlay )
	    {
		const u32 first_block = part->data_off4 /  WII_SECTOR_SIZE4;
		u32 end_block = ( part->ph.data_size4 + WII_SECTOR_SIZE4 - 1 )
			      / WII_SECTOR_SIZE4 + first_block;
		if ( end_block > WII_MAX_SECTORS )
		     end_block = WII_MAX_SECTORS;

		noTRACE("mark %llx+%llx => %x..%x [%x]\n",
			    (u64)part->data_off4<<2,
			    (u64)part->ph.data_size4,
			    first_block, end_block, end_block-first_block );

		if ( first_block < end_block )
		    memset( usage_table + first_block,
			    part->usage_id | WD_USAGE_F_CRYPT,
			    end_block - first_block );
	    }
	}
	else
	    disc->patch_ptab_recommended = !disc->whole_disc;
    }

    u32 n_sect = disc->iso_size
		? ( disc->iso_size + WII_SECTOR_SIZE - 1 ) / WII_SECTOR_SIZE
		: WII_MAX_SECTORS;
    u8 *ptr, *end = usage_table + ( n_sect < WII_MAX_SECTORS ? n_sect : WII_MAX_SECTORS );
    for ( ptr = usage_table; ptr < end; ptr++ )
	*ptr = transform[*ptr];

    #if HAVE_PRINT0
	//wd_print_usage_tab(stdout,2,usage_table,disc->iso_size,false);
	//wd_print_usage_tab(stdout,2,usage_table,WII_MAX_DISC_SIZE,false);
    #endif

    return usage_table;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u32 wd_pack_disc_usage_table // returns the index if the 'last_used_sector + 1'
(
    u8			* dest_table,	// valid pointer to destination table
    wd_disc_t		* disc,		// valid pointer to a disc
    u32			block_size,	// if >1: count every 'block_size'
					//        continuous sectors as one block
    const wd_select_t	* select	// NULL or a new selector
)
{
    if (select)
	wd_select(disc,select);
    wd_calc_usage_table(disc);
    return wd_pack_usage_table(dest_table,disc->usage_table,block_size);
}

///////////////////////////////////////////////////////////////////////////////

u32 wd_pack_usage_table // returns the index if the 'last_used_sector + 1'
(
    u8			* dest_table,	// valid pointer to destination table
    const u8		* usage_table,	// valid pointer to usage table
    u32			block_size	// if >1: count every 'block_size'
					//        continuous sectors as one block
)
{
    DASSERT(dest_table);
    DASSERT(usage_table);

    const u8 * end_tab = usage_table + WII_MAX_SECTORS;
    u8 * dest = dest_table;

    if (  block_size <= 1 )   
    {
	// optimization for single block count

	for ( ; usage_table < end_tab; usage_table++ )
	    if ( *usage_table )
		*dest++ = *usage_table;
    }
    else
    {
	//----- block_size > 1

	if ( block_size < WII_MAX_SECTORS )
	{
	    //----- process all but last block

	    end_tab -= block_size;
	    for ( ; usage_table < end_tab; usage_table += block_size )
	    {
		int i;
		for ( i = 0; i < block_size; i++ )
		    if (usage_table[i])
		    {
			memcpy(dest,usage_table,block_size);
			dest += block_size;
			break;
		    }
	    }
	    end_tab += block_size;
	}

	//----- process last blocks

	const u8 * ptr = usage_table;
	while ( ptr < end_tab )
	    if ( *ptr++ )
	    {
		block_size = end_tab - usage_table;
		memcpy(dest,usage_table,block_size);
		dest += block_size;
		break;
	    }

	//---- find last used sector

	while ( dest > dest_table && !dest[-1] )
	    dest--;
    }

    memset(dest,0,dest_table+WII_MAX_SECTORS-dest);
    return dest - dest_table;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u64 wd_count_used_disc_size
(
    wd_disc_t		* disc,		// valid pointer to a disc
    int			block_size,	// if >1: count every 'block_size'
					//        continuous sectors as one block
					//        and return the block count
					// if <0: like >1, but give the result as multiple
					//        of WII_SECTOR_SIZE and reduce the count
					//        for non needed sectors at the end.
    const wd_select_t	* select	// NULL or a new selector
)
{
    return wd_count_used_disc_blocks(disc,block_size,select)
	* (u64)block_size
	* (u64)WII_SECTOR_SIZE;
}

///////////////////////////////////////////////////////////////////////////////

u32 wd_count_used_disc_blocks
(
    wd_disc_t		* disc,		// valid pointer to a disc
    int			block_size,	// if >1: count every 'block_size'
					//        continuous sectors as one block
					//        and return the block count
					// if <0: like >1, but give the result as multiple
					//        of WII_SECTOR_SIZE and reduce the count
					//        for non needed sectors at the end.
    const wd_select_t	* select	// NULL or a new selector
)
{
    u8 utab[WII_MAX_SECTORS];
    wd_filter_usage_table(disc,utab,select);
    return wd_count_used_blocks(utab,block_size);
}

///////////////////////////////////////////////////////////////////////////////

u32 wd_count_used_blocks
(
    const u8		* usage_table,	// valid pointer to usage table
    int			block_size	// if >1: count every 'block_size'
					//        continuous sectors as one block
					//        and return the block count
					// if <0: like >1, but give the result as multiple
					//        of WII_SECTOR_SIZE and reduce the count
					//        for non needed sectors at the end.
)
{
    DASSERT(usage_table);

    const u8 * end_tab = usage_table + WII_MAX_SECTORS;
    u32 count = 0;

    const bool return_wii_sectors = block_size < 0;
    if (return_wii_sectors)
	block_size = -block_size;
    TRACE("wd_count_used_blocks() => %d,%d\n",return_wii_sectors,block_size);

    if ( block_size > 1 )
    {
	//----- find last used sector

	end_tab--;
	while ( end_tab >= usage_table && !*end_tab )
	    end_tab--;
	end_tab++;

	//----- count all but last block

	if ( block_size < end_tab - usage_table )
	{
	    end_tab -= block_size;

	    for ( ; usage_table < end_tab; usage_table += block_size )
	    {
		int i;
		for ( i = 0; i < block_size; i++ )
		    if (usage_table[i])
		    {
			count++;
			break;
		    }
	    }

	    end_tab += block_size;
	}

	//----- count last block

	if (return_wii_sectors)
	    count = count * block_size + ( end_tab - usage_table );
	else if ( usage_table < end_tab )
	    count++;
    }
    else
    {
	// optimization for single block count

	while ( usage_table < end_tab )
	    if ( *usage_table++ )
		count++;
    }

    return count;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_is_block_used
(
    const u8		* usage_table,	// valid pointer to usage table
    u32			block_index,	// index of block
    u32			block_size	// if >1: number of sectors per block
)
{
    DASSERT(usage_table);

    if ( block_size <= 1 )
	return block_index < WII_MAX_SECTORS && usage_table[block_index];

    block_index *= block_size;
    u32 end = block_index + block_size;
    if ( end > WII_MAX_SECTORS )
	 end = WII_MAX_SECTORS;

    while ( block_index < end )
	if ( usage_table[block_index++] )
	    return true;

    return false;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  file iteration		///////////////
///////////////////////////////////////////////////////////////////////////////

static int wd_iterate_fst_helper
(
    wd_iterator_t	* it,		// valid pointer to iterator data
    const wd_fst_item_t	*fst_base,	// NULL or pointer to FST data
    wd_file_func_t	func,		// call back function
    wd_part_t		* sys_files,	// not NULL: process sys files too
    int			ignore_files,	// >0: ignore all real files
					// >1: ignore fst.bin + main.dol too
    wd_file_func_t	exec_func,	// NULL or call back function
					// that is called if func() returns 1
    bool		is_gc		// true if FST is GC formatted
)
{
    DASSERT(it);
    DASSERT(func);

    if (!fst_base)
	return 0;

    int stat = 0, mod = 0;
    it->fst_item  = 0;

    if (sys_files)
    {
	it->icm  = WD_ICM_DIRECTORY;
	it->off4 = 0;
	it->size = 5;
	it->data = 0;
	strcpy(it->fst_name,"sys/");
	stat = func(it);
	if ( stat == 1 && exec_func )
	{
	    mod = 1;
	    exec_func(it);
	}
	else if (stat)
	    return stat;

	it->icm  = WD_ICM_FILE;
	it->off4 = is_gc ? WII_BOOT_OFF : WII_BOOT_OFF >> 2;
	it->size = WII_BOOT_SIZE;
	DASSERT(!it->data);
	strcpy(it->fst_name,"sys/boot.bin");
	stat = func(it);
	if ( stat == 1 && exec_func )
	{
	    mod = 1;
	    exec_func(it);
	}
	else if (stat)
	    return stat;

	DASSERT( it->icm == WD_ICM_FILE );
	it->off4 = is_gc ? WII_BI2_OFF : WII_BI2_OFF >> 2;
	it->size = WII_BI2_SIZE;
	DASSERT(!it->data);
	strcpy(it->fst_name,"sys/bi2.bin");
	stat = func(it);
	if ( stat == 1 && exec_func )
	{
	    mod = 1;
	    exec_func(it);
	}
	else if (stat)
	    return stat;

	DASSERT( it->icm == WD_ICM_FILE );
	it->off4 = is_gc ? WII_APL_OFF : WII_APL_OFF >> 2;
	it->size = sys_files->apl_size;
	DASSERT(!it->data);
	strcpy(it->fst_name,"sys/apploader.img");
	stat = func(it);
	if ( stat == 1 && exec_func )
	{
	    mod = 1;
	    exec_func(it);
	}
	else if (stat)
	    return stat;

	if ( ignore_files < 2 )
	{
	    if (sys_files->boot.dol_off4)
	    {
		DASSERT( it->icm == WD_ICM_FILE );
		it->off4 = sys_files->boot.dol_off4;
		it->size = sys_files->dol_size;
		DASSERT(!it->data);
		strcpy(it->fst_name,"sys/main.dol");
		stat = func(it);
		if ( stat == 1 && exec_func )
		{
		    mod = 1;
		    exec_func(it);
		}
		else if (stat)
		    return stat;
	    }

	 #if SUPPORT_USER_BIN > 0
	    if ( sys_files->ubin_off4 && sys_files->ubin_size )
	    {
		DASSERT( it->icm == WD_ICM_FILE );
		DASSERT(!it->data);

		it->off4 = sys_files->ubin_off4;
		it->size = sys_files->ubin_size;
		strcpy(it->fst_name,"sys/user.bin");
		stat = func(it);
		if ( stat == 1 && exec_func )
		{
		    mod = 1;
		    exec_func(it);
		}
		else if (stat)
		    return stat;
	    }
	 #endif

// [[2do]] [[fst+]]
	    DASSERT( it->icm == WD_ICM_FILE );
	    it->off4 = sys_files->boot.fst_off4;
	    it->size = sys_files->boot.fst_size4;
	    if (!is_gc)
		it->size <<= 2;
	    DASSERT(!it->data);
	    strcpy(it->fst_name,"sys/fst.bin");
	    stat = func(it);
	    if ( stat == 1 && exec_func )
	    {
		mod = 1;
		exec_func(it);
	    }
	    else if (stat)
		return stat;
	}
    }

    if ( ignore_files < 1 )
    {

	//----- setup stack

	const int MAX_DEPTH = 25; // maximal supported directory depth
	typedef struct stack_t
	{
	    const wd_fst_item_t * dir_end;
	    char * path;
	} stack_t;
	stack_t stack_buf[MAX_DEPTH];
	stack_t *stack = stack_buf;
	stack_t *stack_max = stack_buf + MAX_DEPTH;


	//----- setup path

	const wd_fst_item_t *fst = fst_base;
	const int n_fst = ntohl(fst->size);
	char *path_end = it->path + sizeof(it->path) - MAX_DEPTH - 1;
	it->icm  = WD_ICM_DIRECTORY;
	it->off4 = 0;
	it->size = n_fst-1;
	DASSERT(!it->data);
	strcpy(it->fst_name,"files/");

	stat = func(it);
	if ( stat == 1 && exec_func )
	{
	    mod = 1;
	    exec_func(it);
	}
	else if (stat)
	    return stat;


	//----- main loop

	const wd_fst_item_t *fst_end = fst + n_fst;
	const wd_fst_item_t *dir_end = fst_end;
	char * path_ptr = it->fst_name + 6;

	stat = 0;
	for ( fst++; fst < fst_end && !stat; fst++ )
	{
	    while ( fst >= dir_end && stack > stack_buf )
	    {
		// leave a directory
		stack--;
		dir_end = stack->dir_end;
		path_ptr = stack->path;
	    }

	    ccp fname = (ccp)fst_end + (ntohl(fst->name_off)&0xffffff);
	    char * path_dest = path_ptr;
	    while ( path_dest < path_end && *fname )
	    {
		uchar ch = *fname++;
		if ( ch < 0x80 )
		    *path_dest++ = ch;
		else
		{
		    *path_dest++ = ch >> 6   | 0xc0;
		    *path_dest++ = ch & 0x3f | 0x80;
		}
	    }

	    it->fst_item = (wd_fst_item_t*)fst;
	    if (fst->is_dir)
	    {
		*path_dest++ = '/';
		*path_dest = 0;

		ASSERT(stack<stack_max);
		if ( stack < stack_max )
		{
		    stack->dir_end = dir_end;
		    stack->path = path_ptr;
		    stack++;
		    dir_end = fst_base + ntohl(fst->size);
		    path_ptr = path_dest;
		}

		it->icm  = WD_ICM_DIRECTORY;
		it->off4 = 0;
		it->size = dir_end-fst-1;
		stat = func(it);
	    }
	    else
	    {
		*path_dest = 0;
		it->icm  = WD_ICM_FILE;
		it->off4 = ntohl(fst->offset4);
		it->size = ntohl(fst->size);
		stat = func(it);
	    }

	    if ( stat == 1 && exec_func )
	    {
		mod = 1;
		stat = 0;
		exec_func(it);
	    }
	}
    }

    it->fst_item  = 0;
    return stat ? stat : mod;
}

///////////////////////////////////////////////////////////////////////////////

static int true_file_func ( wd_iterator_t *it )
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////

int wd_iterate_files
(
    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
    void		* param,	// user defined parameter
    int			ignore_files,	// >0: ignore all real files
					// >1: ignore fst.bin + main.dol too
    wd_ipm_t		prefix_mode	// prefix mode
)
{
    DASSERT(disc);
    DASSERT(func);

    if ( !disc->part || !disc->n_part )
	return 0;

    wd_load_all_part(disc,false,false,false);
    wd_part_t *part, *part_end = disc->part + disc->n_part;

    wd_iterator_t it;
    memset(&it,0,sizeof(it));
    it.param = param;
    it.disc  = disc;


    //----- prefix mode

    if ( prefix_mode <= WD_IPM_AUTO )
    {
	int count = 0;
	for ( part = disc->part; part < part_end; part++ )
	    if ( part->is_valid && part->is_enabled )
	    {
		wd_part_t *part2;
		for ( part2 = disc->part; part2 < part; part2++ )
		    if (   part2->is_valid
			&& part2->is_enabled
			&& part2->part_type == part->part_type )
		    {
			prefix_mode = WD_IPM_COMBI;
			goto exit_auto_text;
		    }

		if ( part->part_type == WD_PART_DATA )
		    count++;
		else
		    count += 2;
	    }
	prefix_mode = count == 1 ? WD_IPM_POINT : WD_IPM_PART_NAME;
    }
 exit_auto_text:
    it.prefix_mode = prefix_mode;


    //----- iterate partitions

    int stat = 0;
    for ( part = disc->part; part < part_end && !stat; part++ )
    {
	if ( !part->is_valid || !part->is_enabled )
	    continue;

	it.part = part;

	switch(prefix_mode)
	{
	    case WD_IPM_NONE:
		break;

	    case WD_IPM_SLASH:
		strcpy(it.prefix,"/");
		break;

	    case WD_IPM_POINT:
		strcpy(it.prefix,"./");
		break;

	    case WD_IPM_PART_ID:
		snprintf(it.prefix,sizeof(it.prefix),"P%x/", part->part_type );
		break;

	    case WD_IPM_PART_INDEX:
		snprintf(it.prefix,sizeof(it.prefix),"P%u.%u/",
			part->ptab_index, part->ptab_part_index );
		break;

	    case WD_IPM_COMBI:
		snprintf(it.prefix,sizeof(it.prefix),"P%u.%u-%s/",
			part->ptab_index, part->ptab_part_index,
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NAME));
		break;

	    //case WD_IPM_PART_NAME:
	    default:
		wd_print_part_name(it.prefix,sizeof(it.prefix)-1,part->part_type,WD_PNAME_P_NAME);
		strcat(it.prefix,"/");
		break;
	}

	it.prefix_len = strlen(it.prefix);
	strcpy(it.path,it.prefix);
	it.fst_name = it.path + it.prefix_len;

	it.icm	= WD_ICM_OPEN_PART;
	const u32 off4 = it.off4 = part->part_off4;
	stat = func(&it);
	if (stat)
	    break;


	//----- './' files

	const wd_part_header_t * ph = &part->ph;

	it.icm  = WD_ICM_DIRECTORY;
	it.off4 = 0;
	it.size = part->is_gc
			? SYS_DIR_COUNT_GC + SYS_FILE_COUNT_GC + part->fst_n - 1
			: SYS_DIR_COUNT    + SYS_FILE_COUNT    + part->fst_n - 1;
	it.data = 0;
	stat = func(&it);
	if (stat)
	    break;

	it.icm  = WD_ICM_DATA;
	it.off4 = 0;
	it.size = part->setup_txt_len;
	it.data = part->setup_txt;
	strcpy(it.fst_name,"setup.txt");
	stat = func(&it);
	if (stat)
	    break;

	it.size = part->setup_sh_len;
	it.data = part->setup_sh;
	strcpy(it.fst_name,"setup.sh");
	stat = func(&it);
	if (stat)
	    break;

	it.size = part->setup_bat_len;
	it.data = part->setup_bat;
	strcpy(it.fst_name,"setup.bat");
	stat = func(&it);
	if (stat)
	    break;

	if (!part->is_gc)
	{
	    DASSERT( it.icm == WD_ICM_DATA );
	    it.off4 = off4;
	    it.size = sizeof(part->ph.ticket);
	    it.data = &part->ph.ticket;
	    strcpy(it.fst_name,"ticket.bin");
	    stat = func(&it);
	    if (stat)
		break;

	    DASSERT( it.icm == WD_ICM_DATA );
	    it.off4 = off4 + ph->tmd_off4;
	    it.size = ph->tmd_size;
	    it.data = part->tmd;
	    strcpy(it.fst_name,"tmd.bin");
	    stat = func(&it);
	    if (stat)
		break;

	    it.icm  = WD_ICM_COPY;
	    it.off4 = off4 + ph->cert_off4;
	    it.size = ph->cert_size;
	    it.data = 0;
	    strcpy(it.fst_name,"cert.bin");
	    stat = func(&it);
	    if (stat)
		break;

	    DASSERT( it.icm == WD_ICM_COPY );
	    it.off4 = off4 + ph->h3_off4;
	    it.size = WII_H3_SIZE;
	    DASSERT ( !it.data );
	    strcpy(it.fst_name,"h3.bin");
	    stat = func(&it);
	    if (stat)
		break;
	}

	//----- 'disc/' files

	if (!part->is_gc)
	{
	    it.icm  = WD_ICM_DIRECTORY;
	    it.off4 = 0;
	    it.size = 2;
	    it.data = 0;
	    strcpy(it.fst_name,"disc/");
	    stat = func(&it);
	    if (stat)
		break;

	    it.icm  = WD_ICM_DATA;
	    it.off4 = 0;
	    it.size = sizeof(disc->dhead);
	    it.data = &disc->dhead;
	    strcpy(it.fst_name,"disc/header.bin");
	    stat = func(&it);
	    if (stat)
		break;

	    DASSERT( it.icm == WD_ICM_DATA );
	    it.off4 = WII_REGION_OFF >> 2;
	    it.size = sizeof(disc->region);
	    it.data = &disc->region;
	    strcpy(it.fst_name,"disc/region.bin");
	    stat = func(&it);
	    if (stat)
		break;
	}

	//----- SYS + FST files

	stat = wd_iterate_fst_helper( &it, part->fst, func,part,
					ignore_files, 0, part->is_gc );
	if (stat)
	    break;


	//----- WD_ICM_CLOSE_PART

	*it.fst_name = 0;
	it.icm  = WD_ICM_CLOSE_PART;
	it.off4 = off4;
	it.size = 0;
	it.data = 0;
	stat = func(&it);
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int wd_iterate_fst_files
(
    const wd_fst_item_t	*fst_base,	// valid pointer to FST data
    wd_file_func_t	func,		// call back function
    void		* param		// user defined parameter
)
{
    DASSERT(fst_base);
    DASSERT(func);

    wd_iterator_t it;
    memset(&it,0,sizeof(it));
    it.param = param;
    it.fst_name = it.path;

    return wd_iterate_fst_helper(&it,fst_base,func,0,0,0,0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int wd_remove_disc_files
(
    // Call wd_remove_part_files() for each enabled partition.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was removed
)
{
    DASSERT(disc);

    int stat = 0, mod = 0;
    wd_part_t *part, *part_end = disc->part + disc->n_part;
    for ( part = disc->part; part < part_end && !stat; part++ )
    {
	if (part->is_enabled)
	{
	    wd_load_part(part,false,false,false);
	    if (part->is_ok)
	    {
		stat = wd_remove_part_files(part,func,param,calc_usage_tab);
		if ( stat == 1 )
		{
		    stat = 0;
		    mod = 1;
		}
	    }
	}
    }
    return stat ? stat : mod;
}

///////////////////////////////////////////////////////////////////////////////

static int exec_mark_file ( wd_iterator_t *it )
{
    DASSERT(it);

    if ( it->fst_item )
	it->fst_item->is_dir |= 0x40;
    return 0;
};

///////////////////////////////////////////////////////////////////////////////

int wd_remove_part_files
(
    // Remove files and directories from internal FST copy.
    // Only empty directories are removed. If at least 1 files/dir
    // is removed the new FST.BIN is added to the patching map.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_part_t		* part,		// valid pointer to a partition
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was removed
)
{
    DASSERT(part);

    if (!part->fst)
	return 0;

    wd_iterator_t it;
    memset(&it,0,sizeof(it));
    it.disc	= part->disc;
    it.part	= part;
    it.param	= param;
    it.fst_name	= it.path;

    const int stat
	= wd_iterate_fst_helper(&it,part->fst,func,0,0,exec_mark_file,part->is_gc);

    if ( stat == 1 )
    {
	//----- setup

	wd_fst_item_t * fst = part->fst;
	fst->is_dir &= 1; // never remove first entry
	int n_fst = ntohl(fst->size);
	wd_fst_item_t *fst_end = fst + n_fst;

	HEXDUMP16(0,0,fst_end,16);

	//----- 1. loop: normalize directories

	for ( fst = fst_end-1; fst >= part->fst; fst-- )
	    if ( fst->is_dir & 1 )
	    {
		//--- transform size into number of dir elemes in host order
		int count = 0, i, n = ntohl(fst->size) - (fst - part->fst) - 1;
		for ( i = 1; i <= n; i++ )
		    if ( !(fst[i].is_dir & 0x40 ))
			count++;
		if (count)
		    fst->is_dir &= 1;
		WDPRINT("#%zu: N=%u/%u [%02x]\n",fst-part->fst,count,n,fst->is_dir);
		fst->size = count;
	    }

	//----- 2. loop: move data

	u32 name_delta = ( n_fst - part->fst->size - 1 ) * sizeof(*fst);
	noTRACE("NAME-DELTA=%x=%u\n",name_delta,name_delta);

	wd_fst_item_t * dest;
	for ( fst = dest = part->fst; fst < fst_end; fst++ )
	{
	    const u8 is_dir = fst->is_dir;
	    if ( is_dir & 0x40 )
		continue;

	    WDPRINT("COPY %zu -> %zu size=%zu\n",fst-part->fst,dest-part->fst,sizeof(*dest));
	    memcpy(dest,fst,sizeof(*dest));

	    WDPRINT("NAME-OFF: %x -> %x\n",
		ntohl(dest->name_off) & 0xffffff,
		( ntohl(dest->name_off) & 0xffffff ) + name_delta );

	    dest->name_off = htonl( ( ntohl(dest->name_off) & 0xffffff ) + name_delta );
	    if ( is_dir )
	    {
		dest->is_dir = 1;
		dest->size = htonl( dest->size + (dest-part->fst) + 1 );
	    }
	    dest++;
	}

	WDPRINT("N: %u -> %zu\n", part->fst_n, dest - part->fst);
	part->fst_n = dest - part->fst;
	DASSERT( part->fst->size = htonl(part->fst_n) );
	part->fst->name_off = htonl(0);
	part->fst->is_dir = 1;

	WDPRINT("fst_end=%p new_end=%p, n=%u->%u\n",
		fst_end, part->fst + part->fst_n, n_fst, part->fst_n );
	HEXDUMP16(0,0,fst_end,16);
	HEXDUMP16(0,0, (ccp)( part->fst + part->fst_n ) + name_delta,16);

	//----- insert patch    

	wd_insert_patch_fst(part);
	if (calc_usage_tab)
	     wd_select_part_files(part,0,0);
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int wd_zero_disc_files
(
    // Call wd_remove_part_files() for each enabled partition.
    // Returns 0 if nothing is zeroed, 1 if at least one file is zeroed
    // Other values are abort codes from func()

    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was zeroed
)
{
    DASSERT(disc);
    DASSERT(func);

    int stat = 0, mod = 0;
    wd_part_t *part, *part_end = disc->part + disc->n_part;
    for ( part = disc->part; part < part_end && !stat; part++ )
    {
	if (part->is_enabled)
	{
	    wd_load_part(part,false,false,false);
	    if (part->is_ok)
	    {
		stat = wd_zero_part_files(part,func,param,calc_usage_tab);
		if ( stat == 1 )
		{
		    stat = 0;
		    mod = 1;
		}
	    }
	}
    }
    return stat ? stat : mod;
}

///////////////////////////////////////////////////////////////////////////////

static int exec_zero_file ( wd_iterator_t *it )
{
    DASSERT(it);

    if ( it->icm == WD_ICM_FILE )
    {
	DASSERT_MSG(it->fst_item,"%s",it->path);
	const u32 zero = htonl(0);
	it->fst_item->offset4 = zero;
	it->fst_item->size    = zero;
    }

    return 0;
};

///////////////////////////////////////////////////////////////////////////////

int wd_zero_part_files
(
    // Zero files from internal FST copy (set offset and size to NULL).
    // If at least 1 file is zeroed the new FST.BIN is added to the patching map.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_part_t		* part,		// valid pointer to a partition
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param,	// user defined parameter
    bool		calc_usage_tab	// true: calc usage table again by using 
					// wd_select_part_files(func:=NULL)
					// if at least one file was zeroed
)
{
    DASSERT(part);

    if (!part->fst)
	return 0;

    wd_iterator_t it;
    memset(&it,0,sizeof(it));
    it.disc	= part->disc;
    it.part	= part;
    it.param	= param;
    it.fst_name	= it.path;

    const int stat
	= wd_iterate_fst_helper(&it,part->fst,func,0,0,exec_zero_file,part->is_gc);
    if ( stat == 1 )
    {
	wd_insert_patch_fst(part);
	if (calc_usage_tab)
	     wd_select_part_files(part,0,0);
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int wd_select_disc_files
(
    // Call wd_remove_part_files() for each enabled partition.
    // Returns 0 if nothing is ignored, 1 if at least one file is ignored
    // Other values are abort codes from func()

    wd_disc_t		* disc,		// valid pointer to a disc
    wd_file_func_t	func,		// call back function
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param		// user defined parameter
)
{
    DASSERT(disc);

    int stat = 0, mod = 0;
    wd_part_t *part, *part_end = disc->part + disc->n_part;
    for ( part = disc->part; part < part_end && !stat; part++ )
    {
	if (part->is_enabled)
	{
	    wd_load_part(part,false,false,false);
	    if (part->is_ok)
	    {
		stat = wd_select_part_files(part,func,param);
		if ( stat == 1 )
		{
		    stat = 0;
		    mod = 1;
		}
	    }
	}
    }

    return stat ? stat : mod;
}

///////////////////////////////////////////////////////////////////////////////

static int exec_select_file ( wd_iterator_t *it )
{
    DASSERT(it);
    if ( it->icm == WD_ICM_FILE )
	wd_mark_part(it->part,it->off4,it->size);
    return 0;
};

///////////////////////////////////////////////////////////////////////////////

int wd_select_part_files
(
    // Calculate the usage map for the partition by using the internal
    // and perhaps modified FST.BIN. If func() is not NULL ignore system
    // and real files (/sys/... and /files/...) for return value 1.
    // If at least 1 file is zeroed the new FST.BIN is added to the patching map.
    // Returns 0 if nothing is removed, 1 if at least one file is removed
    // Other values are abort codes from func()

    wd_part_t		* part,		// valid pointer to a partition
    wd_file_func_t	func,		// call back function
					// If NULL nor file is ignored and only
					// the usage map is re calculated.
					//   return 0 for don't touch file
					//   return 1 for zero file
					//   return other for abort
    void		* param		// user defined parameter
)
{
    DASSERT(part);
    DASSERT(part->disc);

    if (!part->fst)
	return 0;

    if (!func)
	func = true_file_func;

    //----- reset usage table

    const u8 usage_id = part->usage_id | WD_USAGE_F_CRYPT;
    u8 * utab = part->disc->usage_table;
    u32 sector;
    for ( sector = part->data_sector; sector < part->end_sector; sector++ )
	if ( utab[sector] == usage_id )
	    utab[sector] = WD_USAGE_UNUSED; 


    //----- mark needed system files

    wd_mark_part( part, WII_BOOT_OFF>>2, WII_BOOT_SIZE );	// boot.bin
    wd_mark_part( part, WII_BI2_OFF>>2, WII_BI2_SIZE );		// bi2.bin
    wd_mark_part( part, WII_APL_OFF>>2, part->apl_size );	// apploader.img
    wd_mark_part( part, part->boot.dol_off4, part->dol_size );	// main.dol

    u32 fst_size = part->boot.fst_size4;
    if (!part->is_gc)
	fst_size <<= 2;
    wd_mark_part( part, part->boot.fst_off4, fst_size );	// fst.bin


    //----- call iterator for files/...

    wd_iterator_t it;
    memset(&it,0,sizeof(it));
    it.disc	= part->disc;
    it.part	= part;
    it.param	= param;
    it.fst_name	= it.path;

    return wd_iterate_fst_helper(&it,part->fst,func,0,0,exec_select_file,part->is_gc);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    print files			///////////////
///////////////////////////////////////////////////////////////////////////////

const char wd_sep_200[201] = // 200 * '-' + NULL
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------";

///////////////////////////////////////////////////////////////////////////////

void wd_initialize_print_fst
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_pfst_t	mode,		// mode for setup
	FILE		* f,		// NULL or output file
	int		indent,		// indention of the output
	u32		max_off4,	// NULL or maximal offset4 value of all files
	u32		max_size	// NULL or maximal size value of all files
)
{
    DASSERT(pf);
    memset(pf,0,sizeof(*pf));

    pf->f		= f ? f : stdout;
    pf->indent		= wd_normalize_indent(indent);
    pf->mode		= mode;

    char buf[50];
    const int fw_offset =  max_off4 > 0
				? snprintf(buf,sizeof(buf),"%llx",(u64)max_off4<<2)
				: 9;

    if ( mode & WD_PFST_UNUSED )
	pf->fw_unused = fw_offset;

    if ( mode & WD_PFST_OFFSET )
	pf->fw_offset = fw_offset;

    if ( mode & WD_PFST_SIZE_HEX )
	pf->fw_size_hex = max_size > 0
			? snprintf(buf,sizeof(buf),"%x",max_size)
			: 7;

    if ( mode & WD_PFST_SIZE_DEC )
	pf->fw_size_dec = max_size > 0
			? snprintf(buf,sizeof(buf),"%u",max_size)
			: 8;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_fst_header
(
	wd_print_fst_t	* pf,		// valid pointer
	int		max_name_len	// max name len, needed for separator line
)
{
    ASSERT(pf);
    ASSERT(pf->f);
    TRACE("wd_print_fst_header()\n");

    pf->indent = wd_normalize_indent(pf->indent);
    if ( max_name_len < 11 )
	max_name_len = 11;

    //----- first header line

    fprintf(pf->f,"%*s",pf->indent,"");

    if ( pf->fw_unused )
    {
	if ( pf->fw_unused < 6 )
	     pf->fw_unused = 6;
	fprintf(pf->f,"%*s ",pf->fw_unused,"unused");
	max_name_len += 1 + pf->fw_unused;
    }

    if ( pf->fw_offset )
    {
	if ( pf->fw_offset < 6 )
	     pf->fw_offset = 6;
	fprintf(pf->f,"%*s  ",pf->fw_offset,"offset");
	max_name_len += 2 + pf->fw_offset;
    }

    if ( pf->fw_size_hex )
    {
	if ( pf->fw_size_hex < 4 )
	     pf->fw_size_hex = 4;
	fprintf(pf->f,"%*s ",pf->fw_size_hex,"size");
	max_name_len += 1 + pf->fw_size_hex;
    }

    if ( pf->fw_size_dec )
    {
	if ( pf->fw_size_dec < 4 )
	     pf->fw_size_dec = 4;
	fprintf(pf->f,"%*s ",pf->fw_size_dec,"size");
	max_name_len += 1 + pf->fw_size_dec;
    }

    //----- second header line

    fprintf(pf->f,"\n%*s",pf->indent,"");

    if ( pf->fw_unused )
	fprintf(pf->f,"%*s ",pf->fw_unused,"hex");

    if ( pf->fw_offset )
	fprintf(pf->f,"%*s  ",pf->fw_offset,"hex");

    if ( pf->fw_size_hex )
	fprintf(pf->f,"%*s ",pf->fw_size_hex,"hex");

    if ( pf->fw_size_dec )
	fprintf(pf->f,"%*s ",pf->fw_size_dec,"dec");

    ccp sep = "";
    if ( pf->fw_unused || pf->fw_offset || pf->fw_size_hex || pf->fw_size_dec )
    {
	sep = " ";
	max_name_len++;
    }

    fprintf(pf->f,
	    "%spath + file\n"
	    "%*s%.*s\n",
	    sep, pf->indent, "", max_name_len, wd_sep_200 );
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_fst_item
(
	wd_print_fst_t	* pf,		// valid pointer
	wd_part_t	* part,		// valid pointer to a disc partition
	wd_icm_t	icm,		// iterator call mode
	u32		offset4,	// offset/4 to read
	u32		size,		// size of object
	ccp		fname1,		// NULL or file name, part 1
	ccp		fname2		// NULL or file name, part 2
)
{
    DASSERT(pf);
    DASSERT(pf->f);

    char buf[200];
    ccp sep = pf->fw_unused || pf->fw_offset || pf->fw_size_hex || pf->fw_size_dec ? " " : "";

    switch (icm)
    {
	case WD_ICM_OPEN_PART:
	    if ( pf->mode & WD_PFST_PART && part && !part->is_gc )
	    {
		int indent = pf->indent;
		if (pf->fw_unused)
		    indent += 1 + pf->fw_unused;
		if (pf->fw_offset)
		    indent += 2 + pf->fw_offset;
		if (pf->fw_size_hex)
		    indent += 1 + pf->fw_size_hex;
		if (pf->fw_size_dec)
		    indent += 1 + pf->fw_size_dec;
		fprintf(pf->f,"%*s%s** Partition #%u.%u, %s **\n",
		    indent, "", sep,
		    part->ptab_index, part->ptab_part_index,
		    wd_print_part_name(buf,sizeof(buf),part->part_type,WD_PNAME_NUM_INFO));
	    }
	    break;

	case WD_ICM_DIRECTORY:
	    fprintf(pf->f,"%*s",pf->indent,"");
	    if (pf->fw_unused)
		fprintf(pf->f,"%*s ",pf->fw_unused,"-");
	    if (pf->fw_offset)
		fprintf(pf->f,"%*s  ",pf->fw_offset,"-");
	    if (pf->fw_size_dec)
	    {
		if (pf->fw_size_hex)
		    fprintf(pf->f,"%*s ",pf->fw_size_hex,"-");
		fprintf(pf->f,"N=%-*u",pf->fw_size_dec-1,size);
	    }
	    else if (pf->fw_size_hex)
		fprintf(pf->f,"N=%-*u",pf->fw_size_hex-1,size);
	    fprintf(pf->f,"%s%s%s\n", sep, fname1 ? fname1 : "", fname2 ? fname2 : "" );
	    break;

	case WD_ICM_FILE:
	case WD_ICM_COPY:
	case WD_ICM_DATA:
	    fprintf(pf->f,"%*s",pf->indent,"");

	    u64 offset = offset4;
	    if ( !part || !part->is_gc )
		offset <<= 2;

	    if (pf->fw_unused)
	    {
		if ( pf->last_icm == icm && offset > pf->last_end )
		    fprintf(pf->f,"%*llx ", pf->fw_unused, offset-pf->last_end );
		else
		    fprintf(pf->f,"%*s ",pf->fw_unused,"-");
	    }
	    if (pf->fw_offset)
		fprintf(pf->f,"%*llx%c ",
			pf->fw_offset, offset, icm == WD_ICM_FILE ? '+' : ' ' );
	    if (pf->fw_size_hex)
		fprintf(pf->f,"%*x ", pf->fw_size_hex, size);
	    if (pf->fw_size_dec)
		fprintf(pf->f,"%*u ", pf->fw_size_dec, size);
	    fprintf(pf->f,"%s%s%s\n", sep, fname1 ? fname1 : "", fname2 ? fname2 : "" );

	    pf->last_end = offset + size;
	    pf->last_icm = icm;
	    break;

	case WD_ICM_CLOSE_PART:
	    break;
    }
}    

///////////////////////////////////////////////////////////////////////////////

static int wd_print_fst_item_wrapper
(
	struct wd_iterator_t *it	// iterator struct with all infos
)
{
    DASSERT(it);
    wd_print_fst_t *d = it->param;
    DASSERT(d);
    bool print = !d->filter_func;
    if (!print)
    {
	it->param = d->filter_param;
	print = !d->filter_func(it);
	it->param = d;
    }
    if (print)
	wd_print_fst_item( d, it->part, it->icm, it->off4, it->size, it->path, 0 );
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_fst
(
	FILE		* f,		// valid output file
	int		indent,		// indention of the output
	wd_disc_t	* disc,		// valid pointer to a disc
	wd_ipm_t	prefix_mode,	// prefix mode
	wd_pfst_t	pfst_mode,	// print mode
	wd_file_func_t	filter_func,	// NULL or filter function
	void		* filter_param	// user defined parameter
)
{
    ASSERT(f);
    TRACE("wd_print_fst() pfst=%x, filter_func=%p\n",pfst_mode,filter_func);
    indent = wd_normalize_indent(indent);

    //----- setup pf and calc fw

    if ( pfst_mode & WD_PFST__OFF_SIZE )
    {
	wd_load_all_part(disc,false,false,false);
	wd_calc_fst_statistics(disc,false);
    }

    wd_print_fst_t pf;
    wd_initialize_print_fst(&pf,pfst_mode,f,indent,disc->fst_max_off4,disc->fst_max_size);
    pf.filter_func	= filter_func;
    pf.filter_param	= filter_param;

    //----- print out

    if ( pfst_mode & WD_PFST_HEADER )
	wd_print_fst_header(&pf,50);

    wd_iterate_files(disc,wd_print_fst_item_wrapper,&pf,0,prefix_mode);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			memmap helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_reset_memmap 
(
	wd_memmap_t  * mm		// NULL or patching data
)
{
    if (mm)
    {
	if (mm->item)
	{
	    int idx;
	    for ( idx = 0; idx < mm->used; idx++ )
	    {
		wd_memmap_item_t * item = mm->item + idx;
		if (item->data_alloced)
		    FREE(item->data);
	    }
	    FREE(mm->item);
	}
	memset(mm,0,sizeof(*mm));
    }
}

///////////////////////////////////////////////////////////////////////////////

static u32 wd_insert_patch_helper
(
    wd_memmap_t		* mm,		// NULL or patching data
    u64			offset,		// offset of object
    u64			size,		// size of object
    bool		* result_found	// true, if item found
)
{
    DASSERT(mm);
    DASSERT(result_found);

    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	const u32 idx = (beg+end)/2;
	wd_memmap_item_t * item = mm->item + idx;
	if ( offset < item->offset )
	    end = idx - 1 ;
	else if ( offset > item->offset )
	    beg = idx + 1;
	else if ( size < item->size )
	    end = idx - 1 ;
	else if ( size > item->size ) 
	    beg = idx + 1;
	else
	{
	    *result_found = true;
	    return idx;
	}
    }

    *result_found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_find_memmap
(
    wd_memmap_t		* mm,		// patching data
    u32			mode,		// memmap mode (e.g. wd_patch_mode_t)
    u64			offset,		// offset of object
    u64			size		// size of object
)
{
    DASSERT(mm);

    if ( mm->used == mm->size )
    {
	mm->size += 10;
	mm->item = REALLOC(mm->item,mm->size*sizeof(*mm->item));
    }

    bool found;
    u32 idx = wd_insert_patch_helper(mm,offset,size,&found);

    return found ? mm->item + idx : 0;
}

///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_insert_memmap
(
    wd_memmap_t		* mm,		// patching data
    u32			mode,		// memmap mode (e.g. wd_patch_mode_t)
    u64			offset,		// offset of object
    u64			size		// size of object
)
{
    DASSERT(mm);

    if ( mm->used == mm->size )
    {
	mm->size += 10;
	mm->item = REALLOC(mm->item,mm->size*sizeof(*mm->item));
    }

    bool found;
    u32 idx = wd_insert_patch_helper(mm,offset,size,&found);

    DASSERT( idx <= mm->used );
    wd_memmap_item_t * item = mm->item + idx;
    if (!found)
    {
	memmove(item+1,item,(mm->used-idx)*sizeof(*item));
	memset(item,0,sizeof(*item));
	mm->used++;
    }
    else if (item->data_alloced)
    {
	FREE(item->data);
	item->data = 0;
	item->data_alloced = 0;
    }

    item->mode	 = mode;
    item->offset = offset;
    item->size   = size;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_insert_memmap_alloc
(
    wd_memmap_t		* mm,		// patching data
    u32			mode,		// memmap mode (e.g. wd_patch_mode_t)
    u64			offset,		// offset of object
    u64			size		// size of object
)
{
    wd_memmap_item_t * item = wd_insert_memmap(mm,mode,offset,size);
    DASSERT(!item->data_alloced);
    item->data_alloced = true;
    item->data = MALLOC(size);
    memset(item->data,0,size);
    return item;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int wd_insert_memmap_disc_part 
(
    wd_memmap_t		* mm,		// patching data
    wd_disc_t		* disc,		// valid disc pointer

    wd_memmap_func_t	func,		// not NULL: Call func() for each inserted item
    void		* param,	// user defined parameter for 'func()'

    // creation modes:
    // value WD_PAT_IGNORE means: do not create such entires

    wd_patch_mode_t	wii_head_mode,	// value for the Wii partition header
    wd_patch_mode_t	wii_mgr_mode,	// value for the Wii partition mgr data
    wd_patch_mode_t	wii_data_mode,	// value for the Wii partition data
    wd_patch_mode_t	gc_mgr_mode,	// value for the GC partition mgr header
    wd_patch_mode_t	gc_data_mode	// value for the GC partition header
)
{
    DASSERT(disc);

    wd_load_all_part(disc,false,false,false);

    int ip, count = 0;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = disc->part + ip;
	if ( part->is_valid && part->is_enabled )
	    count += wd_insert_memmap_part
			( mm, part, func, param,
				wii_head_mode, wii_mgr_mode, wii_data_mode,
				gc_mgr_mode, gc_data_mode );
    }

    return count;
}

///////////////////////////////////////////////////////////////////////////////

int wd_insert_memmap_part 
(
    wd_memmap_t		* mm,		// patching data
    wd_part_t		* part,		// valid pointer to a disc partition

    wd_memmap_func_t	func,		// not NULL: Call func() for each inserted item
    void		* param,	// user defined parameter for 'func()'

    // creation modes:
    // value WD_PAT_IGNORE means: do not create such entires

    wd_patch_mode_t	wii_head_mode,	// value for the Wii partition header
    wd_patch_mode_t	wii_mgr_mode,	// value for the Wii partition mgr data
    wd_patch_mode_t	wii_data_mode,	// value for the Wii partition data
    wd_patch_mode_t	gc_mgr_mode,	// value for the GC partition mgr header
    wd_patch_mode_t	gc_data_mode	// value for the GC partition header
)
{
    DASSERT(mm);
    DASSERT(part);

    if ( !part->is_valid || !part->is_enabled )
	return 0;

    char intro[30];
    u32 count = mm->used;

    if (part->is_gc)
    {
	snprintf( intro, sizeof(intro), "GC P.%u, %s",
			part->index, wd_print_id(&part->boot,6,0) );

	wii_mgr_mode = gc_mgr_mode;
	wii_data_mode = gc_mgr_mode;
    }
    else
    {
	snprintf( intro, sizeof(intro), "P.%u.%u, %s",
			part->ptab_index, part->ptab_part_index,
			wd_print_part_name(0,0,part->part_type,WD_PNAME_NUM_INFO) );

	if ( wii_head_mode != WD_PAT_IGNORE )
	{
	    WDPRINT("INSERT WII PART-HEAD PATCH %u\n",wii_head_mode);

	    const u64 head_off = (u64)part->part_off4 << 2;
	    const u64 head_size = (u64)part->data_sector * WII_SECTOR_SIZE - head_off;

	    wd_memmap_item_t * item
		= wd_insert_memmap(mm,wii_head_mode,head_off,head_size);
	    DASSERT(item);
	    item->index = part->index;
	    snprintf(item->info,sizeof(item->info),"%s, partition head",intro);
	    if (func)
		func(param,mm,item);

	    u64 off = (u64)part->ph.tmd_off4 << 2;
	    if ( off && off + part->ph.tmd_size > head_size )
	    {
		item = wd_insert_memmap(mm,wii_head_mode,head_off+off,part->ph.tmd_size);
		DASSERT(item);
		item->index = part->index;
		snprintf(item->info,sizeof(item->info),"%s, tmd",intro);
		if (func)
		    func(param,mm,item);
	    }

	    off = (u64)part->ph.cert_off4 << 2;
	    if ( off && off + part->ph.cert_size > head_size )
	    {
		item = wd_insert_memmap(mm,wii_head_mode,head_off+off,part->ph.cert_size);
		DASSERT(item);
		item->index = part->index;
		snprintf(item->info,sizeof(item->info),"%s, cert",intro);
		if (func)
		    func(param,mm,item);
	    }

	    off = (u64)part->ph.h3_off4 << 2;
	    if ( off && off + WII_H3_SIZE > head_size )
	    {
		item = wd_insert_memmap(mm,wii_head_mode,head_off+off,WII_H3_SIZE);
		DASSERT(item);
		item->index = part->index;
		snprintf(item->info,sizeof(item->info),"%s, h3",intro);
		if (func)
		    func(param,mm,item);
	    }
	}
    }


    if ( wii_mgr_mode != WD_PAT_IGNORE && wii_mgr_mode == wii_data_mode )
    {
	WDPRINT("INSERT PART-DATA PATCH %u\n",wii_data_mode);

	wd_memmap_item_t * item = wd_insert_memmap
		( mm,
		  wii_mgr_mode,
		 (u64)part->data_sector * WII_SECTOR_SIZE,
		 (u64)( part->end_sector - part->data_sector ) * WII_SECTOR_SIZE
		);
	DASSERT(item);
	item->index = part->index;
	snprintf(item->info,sizeof(item->info),"%s, partition data",intro);
	if (func)
	    func(param,mm,item);
    }
    else
    {
	if ( wii_mgr_mode != WD_PAT_IGNORE )
	{
	    WDPRINT("INSERT WII PART-DATA #0 PATCH %u\n",wii_data_mode);

	    wd_memmap_item_t * item = wd_insert_memmap
			( mm,
			  wii_mgr_mode,
			 (u64)part->data_sector * WII_SECTOR_SIZE,
			 (u64)( part->end_mgr_sector - part->data_sector ) * WII_SECTOR_SIZE
			);
	    DASSERT(item);
	    item->index = part->index;
	    snprintf(item->info,sizeof(item->info),"%s, partition data #0",intro);
	    if (func)
		func(param,mm,item);
	}

	if ( wii_data_mode != WD_PAT_IGNORE && part->end_mgr_sector < part->end_sector)
	{
	    WDPRINT("INSERT WII PART-DATA #1 PATCH %u\n",wii_data_mode);

	    wd_memmap_item_t * item = wd_insert_memmap
			( mm,
			  wii_data_mode,
			 (u64)part->end_mgr_sector * WII_SECTOR_SIZE,
			 (u64)( part->end_sector - part->end_mgr_sector ) * WII_SECTOR_SIZE
			);
	    DASSERT(item);
	    item->index = part->index;
	    snprintf(item->info,sizeof(item->info),"%s, partition data #1",intro);
	    if (func)
		func(param,mm,item);
	}
    }

    return mm->used - count;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_memmap
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_memmap_t		* mm		// NULL or patching data
)
{
    ASSERT(mm);
    if ( !f || !mm->used )
	return;

    indent = wd_normalize_indent(indent);

    int fw = 7;
    wd_memmap_item_t *item, *last = mm->item + mm->used;
    for ( item = mm->item; item < last; item++ )
    {
	const int len = strlen(item->info);
	if ( fw < len )
	     fw = len;
    }
    fprintf(f,"\n%*s     offset .. offset end     size  comment\n"
		"%*s%.*s\n",
		indent,"", indent,"", fw + 37, wd_sep_200 );

    u64 prev_end = 0;
    for ( item = mm->item; item < last; item++ )
    {
	const u64 end = item->offset + item->size;
	fprintf(f,"%*s%c %9llx .. %9llx %9llx  %s\n", indent,"",
		item->offset < prev_end ? '!' : ' ',
		item->offset, end, item->size, item->info );
	if ( prev_end < end )
	     prev_end = end;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  patch helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_insert_patch_ticket
(
    wd_part_t		* part		// valid pointer to a disc partition
)  
{
    if (!wd_part_has_ticket(part))
	return 0;

    wd_memmap_item_t * item
	= wd_insert_memmap( &part->disc->patch, WD_PAT_PART_TICKET,
					(u64)part->part_off4<<2, WII_TICKET_SIZE );
    DASSERT(item);
    item->index = part->index;
    item->data = &part->ph.ticket;

    snprintf(item->info,sizeof(item->info),
			"ticket, id=%s, ckey=%u",
			wd_print_id(part->ph.ticket.title_id+4,4,0),
			part->ph.ticket.common_key_index );

    part->sign_ticket = true;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_insert_patch_tmd
(
    wd_part_t		* part		// valid pointer to a disc partition
)  
{
    if (!wd_part_has_tmd(part))
	return 0;

    wd_memmap_item_t * item
	= wd_insert_memmap( &part->disc->patch, WD_PAT_PART_TMD,
				(u64)( part->part_off4 + part->ph.tmd_off4 ) << 2,
				part->ph.tmd_size );
    DASSERT(item);
    item->index = part->index;
    item->data = part->tmd;

    u32 high = be32((ccp)&part->tmd->sys_version);
    u32 low  = be32(((ccp)&part->tmd->sys_version)+4);
    if ( high == 1 && low < 0x100 )
	snprintf(item->info,sizeof(item->info),
		"tmd, id=%s, ios=%u",
			wd_print_id(part->tmd->title_id+4,4,0), low );
    else
	snprintf(item->info,sizeof(item->info),
		"tmd, id=%s, sys=%x-%x",
			wd_print_id(part->tmd->title_id+4,4,0), high, low );

    part->sign_tmd = true;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

wd_memmap_item_t * wd_insert_patch_fst
(
    wd_part_t		* part		// valid pointer to a disc partition
)  
{
    wd_insert_patch_tmd(part);
    wd_memmap_item_t * item
	= wd_insert_memmap( &part->patch, WD_PAT_DATA,
				(u64)part->boot.fst_off4 << 2,
				(u64)part->boot.fst_size4 << 2 );
    DASSERT(item);
    item->index = part->index;
    item->data = part->fst;
    part->sign_tmd = true;

    snprintf(item->info,sizeof(item->info),
			"fst.bin, N=%u", ntohl(part->fst->size) );

    return item;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_disc_patch
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid pointer to a disc
    bool		print_title,	// true: print table titles
    bool		print_part	// true: print partitions too
)
{
    ASSERT(disc);
    if (!f)
	return;
    indent = wd_normalize_indent(indent);

    if (disc->patch.used)
    {
	if (print_title)
	{
	    fprintf(f,"\n%*sPatching table of the disc:\n",indent,"");
	    indent += 2;
	}
	wd_print_memmap(f,indent,&disc->patch);
    }

    if (print_part)
    {
	wd_part_t *part, *part_end = disc->part + disc->n_part;
	for ( part = disc->part; part < part_end; part++ )
	{
	    if (part->patch.used)
	    {
		if (print_title)
		{
		    char buf[50];
		    fprintf(f,"\n%*sPatching table of partition %s:\n",
			indent-2, "",
			wd_print_part_name(buf,sizeof(buf),part->part_type,WD_PNAME_NUM_INFO));
		}
		wd_print_memmap(f,indent,&part->patch);
	    }
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read and patch			///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_calc_group_hashes
(
    const u8		* group_data,	// group data space
    u8			* group_hash,	// group hash space
    u8			* h3,		// NULL or H3 element to change
    const u8		dirty[WII_GROUP_SECTORS]
					// NULL or 'dirty sector' flags
)
{
    int d;
    for ( d = 0; d < WII_GROUP_SECTORS; d++ )
    {
	if ( dirty && !dirty[d] )
	    continue;

	const u8 * data = group_data + d * WII_SECTOR_DATA_SIZE;
	u8 * h0 = group_hash + d * WII_SECTOR_HASH_SIZE;
	int d0;
	for ( d0 = 0; d0 < WII_N_ELEMENTS_H0; d0++ )
	{
	    SHA1(data,WII_H0_DATA_SIZE,h0);
	    data += WII_H0_DATA_SIZE;
	    h0   += WII_HASH_SIZE;
	}

	h0 = group_hash + d * WII_SECTOR_HASH_SIZE;
	u8 * h1 = group_hash
		+ (d/WII_N_ELEMENTS_H1) * WII_N_ELEMENTS_H1 * WII_SECTOR_HASH_SIZE
		+ (d%WII_N_ELEMENTS_H1) * WII_HASH_SIZE
		+ 0x280;
	SHA1(h0,WII_N_ELEMENTS_H0*WII_HASH_SIZE,h1);
    }

    u8 * h1 = group_hash + 0x280;
    u8 * h2 = group_hash + 0x340;
    int d1, d2;
    for ( d2 = 0; d2 < WII_N_ELEMENTS_H2; d2++ )
    {
	for ( d1 = 1; d1 < WII_N_ELEMENTS_H1; d1++ )
	    memcpy( h1 + d1 * WII_SECTOR_HASH_SIZE, h1, WII_N_ELEMENTS_H1*WII_HASH_SIZE );

	SHA1(h1,WII_N_ELEMENTS_H1*WII_HASH_SIZE,h2);
	h1 += WII_N_ELEMENTS_H1 * WII_SECTOR_HASH_SIZE;
	h2 += WII_HASH_SIZE;
    }

    h2 = group_hash + 0x340;
    for ( d1 = 1; d1 < WII_GROUP_SECTORS; d1++ )
	    memcpy( h2 + d1 * WII_SECTOR_HASH_SIZE, h2, WII_N_ELEMENTS_H2*WII_HASH_SIZE );

    if (h3)
	SHA1(h2,WII_N_ELEMENTS_H2*WII_HASH_SIZE,h3);
}

///////////////////////////////////////////////////////////////////////////////

static enumError wd_rap_part_sectors
(
    wd_part_t		* part,		// valid partition pointer
    u32			sector,		// index of first sector to read
    u32			end_sector	// index of last sector +1 to read
)
{
    DASSERT(part);
    DASSERT(part->disc);
    DASSERT( sector >= part->data_sector);
    DASSERT( end_sector <= part->end_sector );
    DASSERT( end_sector <= sector + WII_GROUP_SECTORS );

    //----- cache setup

    wd_disc_t * disc = part->disc;
    if (!disc->group_cache)
    {
	// space for cache and an extra sector (no inplace decryption possible)
	disc->group_cache = MALLOC(WII_GROUP_SIZE+WII_SECTOR_SIZE);
    }
    else if ( disc->group_cache_sector == sector )
	return ERR_OK;
    disc->group_cache_sector = sector;
    u8 * buf = disc->group_cache;


    //----- reloc check

    wd_reloc_t * reloc = disc->reloc;
    DASSERT(reloc);

    u32 or_rel = 0, sect = sector;
    while ( sect < end_sector )
	or_rel |= reloc[sect++];

    // delta must be same for all
    DASSERT( ( reloc[sector] & WD_RELOC_M_DELTA ) == ( or_rel & WD_RELOC_M_DELTA ) );
    DASSERT( ( reloc[sect-1] & WD_RELOC_M_DELTA ) == ( or_rel & WD_RELOC_M_DELTA ) );

    const wd_reloc_t decrypt = or_rel & (WD_RELOC_F_PATCH|WD_RELOC_F_DECRYPT);
    const wd_reloc_t patch   = or_rel & WD_RELOC_F_PATCH;
    const wd_reloc_t encrypt = or_rel & WD_RELOC_F_ENCRYPT;


    //----- load data

    const u64 size = ( end_sector - sector ) * (u64)WII_SECTOR_SIZE;
    u32 src = sector + ( reloc[sector] & WD_RELOC_M_DELTA );
    if ( src > WII_MAX_SECTORS )
	src -= WII_MAX_SECTORS;

    bool is_encrypted;
    if ( or_rel & WD_RELOC_F_COPY )
    {
	TRACE("READ(%x->%x..%x)\n",src,sector,end_sector);
	is_encrypted = part->is_encrypted;
	if ( is_encrypted && decrypt )
	    buf += WII_SECTOR_SIZE;
	const enumError err
	    = wd_read_raw( disc,
			   src * WII_SECTOR_SIZE4,
			   buf,
			   size,
			   0 );
	if (err)
	    return err;
    }
    else
    {
	TRACE("ZERO(%x->%x..%x)\n",src,sector,end_sector);
	is_encrypted = false;
	memset( buf, 0, size );
    }


    //----- decrypt

    bool is_splitted = false;
    DASSERT( sizeof(disc->temp_buf) >= WII_GROUP_SECTORS * WII_SECTOR_HASH_SIZE );
    u8 * hash_buf = patch ? disc->temp_buf : 0;

    if ( is_encrypted && decrypt )
    {
	WDPRINT("DECRYPT(%x->%x..%x) split=%d\n",src,sector,end_sector,hash_buf!=0);
	is_encrypted = false;

	wd_decrypt_sectors(part,0,buf,disc->group_cache,hash_buf,end_sector-sector);
	buf = disc->group_cache;
	is_splitted = hash_buf != 0;
    }


    //----- patching

    if (patch)
    {
	WDPRINT("PATCH-P(%x->%x..%x) split=%d\n",src,sector,end_sector,!is_splitted);
	DASSERT(!is_encrypted);
	DASSERT(hash_buf);

	if (!is_splitted)
	{
	    is_splitted = true;
	    wd_split_sectors(buf,buf,hash_buf,end_sector-sector);
	}

	//--- patch data

	u64 off1 = ( sector - part->data_sector ) * (u64)WII_SECTOR_DATA_SIZE;
	u64 off2 = ( end_sector - part->data_sector ) * (u64)WII_SECTOR_DATA_SIZE;
	const wd_memmap_item_t *item = part->patch.item;
	const wd_memmap_item_t *end_item = item + part->patch.used;
	noTRACE("> off=%llx..%llx, n-item=%u\n", off1, off2, part->patch.used );

	u8 dirty[WII_GROUP_SECTORS];
	memset(dirty,0,sizeof(dirty));

	for ( ; item < end_item && item->offset < off2; item++ )
	{
	  const u64 end = item->offset + item->size;
	  noTRACE("> off=%llx..%llx, item=%llx..%llx\n", off1, off2, item->offset, end );
	  noTRACE("> data=%p, fst=%p\n",item->data,part->fst);
	  if ( item->offset < off2 && end > off1 )
	  {
	    const u64 overlap1 = item->offset > off1 ? item->offset : off1;
	    const u64 overlap2 = end < off2 ? end : off2;
	    u32 dirty1 = (overlap1-off1) / WII_SECTOR_DATA_SIZE;
	    u32 dirty2 = (overlap2-off1 + WII_SECTOR_DATA_SIZE - 1) / WII_SECTOR_DATA_SIZE;
	    noTRACE(" -> %llx .. %llx, dirty=%u..%u\n",overlap1,overlap2,dirty1,dirty2);
	    memset(dirty+dirty1,1,dirty2-dirty1);

	    switch (item->mode)
	    {
	      case WD_PAT_ZERO:
		memset(	buf + (overlap1-off1), 0, overlap2-overlap1 );
		break;

	      case WD_PAT_DATA:
		memcpy(	buf + (overlap1-off1),
			(u8*)item->data + (overlap1-item->offset),
			overlap2-overlap1 );
		break;

	      default:
		ASSERT(0);
	    }
	  }
	}

	//--- calc hashes

	u8 * h3 = part->h3;
	if (h3)
	{
	    h3 += ( sector - part->data_sector ) / WII_GROUP_SECTORS * WII_HASH_SIZE;
	    part->h3_dirty = true;
	}
	wd_calc_group_hashes(buf,disc->temp_buf,h3,dirty);
    }


    //----- encrypt

    if ( !is_encrypted && encrypt )
    {
	WDPRINT("ENCRYPT(%x->%x..%x) compose=%d\n",src,sector,end_sector,hash_buf!=0);
	wd_encrypt_sectors(part,0,buf,hash_buf,buf,end_sector-sector);
    }
    else if (is_splitted)
    {
	WDPRINT("COMPOSE(%x->%x..%x)\n",src,sector,end_sector);
	wd_join_sectors(buf,hash_buf,buf,end_sector-sector);
    }

    DASSERT( buf == disc->group_cache );
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

static enumError wd_rap_sectors
(
    wd_disc_t		* disc,		// valid disc pointer
    u8			* buf,		// destination buffer
    u32			sector,		// index of first sector to read
    u32			n_sectors	// number of sectors to read
)
{
    TRACE("wd_rap_sectors(%p,%p,%x,%x)\n",disc,buf,sector,n_sectors);
    DASSERT(disc);
    DASSERT(buf);

    u32 end_sector = sector + n_sectors;
    if ( end_sector > WII_MAX_SECTORS )
    {
	// outside of manged area -> just fill with zeros
	u32 first_sector = sector;
	if ( first_sector < WII_MAX_SECTORS )
	     first_sector = WII_MAX_SECTORS;

	TRACE("ZERO(%x,+%x,%x)\n",first_sector,first_sector-sector,end_sector-first_sector);
	memset( buf + ( first_sector - sector ) * WII_SECTOR_SIZE, 0,
		( end_sector - first_sector ) * WII_SECTOR_SIZE );
	if ( sector >= WII_MAX_SECTORS )
	    return ERR_OK;

	end_sector = WII_MAX_SECTORS;
	n_sectors = end_sector - sector;
    }

    ASSERT( sector < end_sector );
    ASSERT( sector + n_sectors == end_sector );
    ASSERT( end_sector <= WII_MAX_SECTORS );

    wd_reloc_t * reloc = disc->reloc;
    DASSERT(reloc);

    while ( sector < end_sector )
    {
	wd_reloc_t rel = reloc[sector];
	if ( rel & WD_RELOC_M_CRYPT )
	{
	    u32 pidx = ( rel & WD_RELOC_M_PART ) >> WD_RELOC_S_PART;
	    if ( pidx < disc->n_part )
	    {
		// partition data is read in blocks of 64 sectors

		wd_part_t * part = disc->part + pidx;
		DASSERT(part->is_enabled);
		DASSERT(part->is_valid);
		u32 sect1 = ( sector - part->data_sector )
			  / WII_GROUP_SECTORS * WII_GROUP_SECTORS
			  + part->data_sector;
		u32 sect2 = sect1 + WII_GROUP_SECTORS;
		if ( sect2 > part->end_sector )
		     sect2 = part->end_sector;
		const enumError err = wd_rap_part_sectors(part,sect1,sect2);
		if (err)
		    return err;

		if ( sect2 > end_sector )
		     sect2 = end_sector;
		const u32 size = ( sect2 - sector ) * WII_SECTOR_SIZE;
		TRACE("COPY(sect=%x, delta=%x, cnt=%x, size=%x\n",
			sector, sector - sect1, sect2 - sector, size );
		memcpy( buf, disc->group_cache + (sector-sect1) * WII_SECTOR_SIZE, size );
		buf += size;
		sector = sect2;
		continue;
	    }
	}

	const u32 start = sector;
	rel &= WD_RELOC_M_DISCLOAD;
	u32 or_rel = 0;
	while ( sector < end_sector && (reloc[sector] & WD_RELOC_M_DISCLOAD) == rel )
	    or_rel |= reloc[sector++];

	const u32 count = sector - start;
	const u64 size  = count * (u64)WII_SECTOR_SIZE;
	DASSERT(count);

	u32 src = start + ( rel & WD_RELOC_M_DELTA );
	if ( src > WII_MAX_SECTORS )
	    src -= WII_MAX_SECTORS;

	if ( or_rel & WD_RELOC_F_COPY )
	{
	    TRACE("READ(%x->%x,%x)\n",src,start,count);
	    const enumError err
		= wd_read_raw(	disc,
				src * WII_SECTOR_SIZE4,
				buf,
				size,
				0 );
	    if (err)
		return err;
	}
	else
	{
	    TRACE("ZERO(%x->%x,%x)\n",src,start,sector-start);
	    memset( buf, 0, size );
	}

	if ( or_rel & WD_RELOC_F_PATCH )
	{
	    DASSERT( !( or_rel & WD_RELOC_M_CRYPT ) );
	    TRACE("PATCH-D(%x->%x,%x)\n",src,start,sector-start);
	    u64 off1 = start  * (u64)WII_SECTOR_SIZE;
	    u64 off2 = sector * (u64)WII_SECTOR_SIZE;
	    const wd_memmap_item_t *item = disc->patch.item;
	    const wd_memmap_item_t *end_item = item + disc->patch.used;

	    // skip unneeded items
	    while ( item < end_item && item->offset + item->size <= off1 )
		item++;

	    for ( ; item < end_item && item->offset < off2; item++ )
	    {
	      const u64 end = item->offset + item->size;
	      if ( item->offset < off2 && end > off1 )
	      {
		const u64 overlap1 = item->offset > off1 ? item->offset : off1;
		const u64 overlap2 = end < off2 ? end : off2;
		TRACE(" -> %llx .. %llx\n",overlap1,overlap2);
		DASSERT( item->index < disc->n_part );
		wd_part_t * part = disc->part + item->index;
		switch (item->mode)
		{
		  case WD_PAT_ZERO:
		    memset( buf + (overlap1-off1), 0, overlap2-overlap1 );
		    break;

		  case WD_PAT_DATA:
		    memcpy( buf + (overlap1-off1),
			    (u8*)item->data + (overlap1-item->offset),
			    overlap2-overlap1 );
		    break;

		  case WD_PAT_PART_TICKET:
		    DASSERT(part);
		    if (part->sign_ticket)
		    {
			PRINT("SIGN TICKET\n");
			wd_encrypt_title_key(&part->ph.ticket,part->key);
			ticket_fake_sign(&part->ph.ticket,sizeof(part->ph.ticket));
			part->sign_ticket = false;
		    }
		    memcpy( buf + (overlap1-off1),
			    (u8*)item->data + (overlap1-item->offset),
			    overlap2-overlap1 );
		    break;

		  case WD_PAT_PART_TMD:
		    DASSERT(part);
		    DASSERT(part->tmd);
		    if ( part->h3_dirty && part->h3 )
		    {
			PRINT("CALC H4\n");
			SHA1( part->h3, WII_H3_SIZE, part->tmd->content[0].hash );
			part->h3_dirty = false;
			part->sign_tmd = true;
		    }

		    if (part->sign_tmd)
		    {
			PRINT("SIGN TMD\n");
			tmd_fake_sign(part->tmd,part->ph.tmd_size);
			part->sign_tmd = false;
		    }
		    memcpy( buf + (overlap1-off1),
			    (u8*)item->data + (overlap1-item->offset),
			    overlap2-overlap1 );
		    break;

		  default:
		    ASSERT(0);
		}
	      }
	    }
	}
	buf += size;
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError wd_read_and_patch
(
    wd_disc_t		* disc,		// valid disc pointer
    u64			offset,		// offset to read
    void		* dest_buf,	// destination buffer
    u32			count		// number of bytes to read
)
{
    DASSERT(disc);
    DASSERT(dest_buf);

    //----- read first sector if unaligned

    u8  *dest	= dest_buf;
    u32 sector	= offset / WII_SECTOR_SIZE;
    u32 skip	= offset % WII_SECTOR_SIZE;
    noTRACE("sector=%x, skip=%x\n",sector,skip);

    if ( count > 0 && skip )
    {
	// read and cache whole sector
	noTRACE("READ %x, cache=%x\n",sector,disc->cache_sector);
	if ( disc->cache_sector != sector )
	{
	    const enumError err = wd_rap_sectors(disc,disc->cache,sector,1);
	    if (err)
	    {
		disc->cache_sector = ~(u32)0;
		return err;
	    }
	    disc->cache_sector = sector;
	}

	u32 copy_len = WII_SECTOR_SIZE - skip;
	if ( copy_len > count )
	     copy_len = count;

	memcpy(dest,disc->cache+skip,copy_len);
	dest   += copy_len;
	count  -= copy_len;
	sector += 1;
    }

    //----- read continous and well aligned sectors

    if ( count > WII_SECTOR_SIZE )
    {
	const u32 n_sectors = count / WII_SECTOR_SIZE;
	const enumError err = wd_rap_sectors(disc,dest,sector,n_sectors);
	if (err)
	    return err;

	const u32 read_size = n_sectors * WII_SECTOR_SIZE;
	dest   += read_size;
	count  -= read_size;
	sector += n_sectors;
    }

    //----- read last sector with cache

    if (count)
    {
	// read and cache whole sector
	noTRACE("READ %x, cache=%x\n",sector,disc->cache_sector);
	if ( disc->cache_sector != sector )
	{
	    const enumError err = wd_rap_sectors(disc,disc->cache,sector,1);
	    if (err)
	    {
		disc->cache_sector = ~(u32)0;
		return err;
	    }
	    disc->cache_sector = sector;
	}
	memcpy(dest,disc->cache,count);
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    patching			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 wd_get_ptab_sector
(
    wd_disc_t		* disc		// valid disc pointer
)
{
    DASSERT(disc);
    return disc->disc_type == WD_DT_GAMECUBE
		? 0
		: WII_PTAB_SECTOR;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_ptab // result = true if something changed
(
    wd_disc_t		* disc,		// valid disc pointer
    void		* sector_data,	// valid pointer to sector data
					//   GC:  GC_MULTIBOOT_PTAB_OFF + GC_MULTIBOOT_PTAB_SIZE
					//   Wii: WII_MAX_PTAB_SIZE
    bool		force_patch	// false: patch only if needed
)
{
    DASSERT(disc);
    DASSERT(sector_data);

    wd_load_all_part(disc,false,false,false);

    if (!force_patch)
    {
	// test if any partitions is invalid or disabled
	int ip;
	for ( ip = 0; ip < disc->n_part; ip++ )
	{
	    wd_part_t * part = disc->part + ip;
	    if ( !part->is_valid || !part->is_enabled )
	    {
		force_patch = true;
		break;
	    }
	}
	if (!force_patch)
	    return false;
    }

    if ( disc->disc_type == WD_DT_GAMECUBE )
    {
	memcpy( sector_data, &disc->dhead,
			GC_MULTIBOOT_PTAB_OFF + GC_MULTIBOOT_PTAB_SIZE );
	if ( !( disc->disc_attrib & WD_DA_GC_MULTIBOOT ) )
	    return true;

	// check dvd9
	u32 max_part_off4 = 0;
	int n_part = 0, ip;
	for ( ip = 0; ip < disc->n_part; ip++ )
	{
	    wd_part_t * part = disc->part + ip;
	    if ( part->is_valid && part->is_enabled && part->part_off4 )
	    {
		n_part++;
		if ( max_part_off4 < part->part_off4 )
		     max_part_off4 = part->part_off4;
	    }
	}

	const bool is_dvd9 = max_part_off4 >= 0x40000000;
	if ( n_part > GC_MULTIBOOT_PTAB_SIZE/sizeof(u32) )
	     n_part = GC_MULTIBOOT_PTAB_SIZE/sizeof(u32);
	WDPRINT("PATCH-PTAB: np=%u, max_part_off4=%x, is_dvd9=%d\n",
			n_part, max_part_off4, is_dvd9 );

	u32 * ptab = (u32*)( (char*)sector_data + GC_MULTIBOOT_PTAB_OFF );
	u32 * ptab_end = ptab + n_part;

	memcpy(sector_data, is_dvd9 ? "GCOPDVD9" : "GCOPDVD", 8 );
	memset(ptab,0,GC_MULTIBOOT_PTAB_SIZE);

	for ( ip = 0; ip < disc->n_part && ptab < ptab_end; ip++ )
	{
	    wd_part_t * part = disc->part + ip;
	    if ( part->is_valid && part->is_enabled && part->part_off4 )
	    {
		u32 off = part->part_off4;
		if (!is_dvd9)
		    off <<= 2;
		*ptab++ = htonl(off);
	    }
	}
    }
    else
    {
	memset(sector_data,0,WII_MAX_PTAB_SIZE);
	wd_ptab_t       * ptab  = sector_data;
	wd_ptab_info_t  * info  = ptab->info;
	wd_ptab_entry_t * entry = ptab->entry;
	u32 off4 = (ccp)entry - (ccp)info + WII_PTAB_REF_OFF >> 2;

	int it;
	for ( it = 0; it < WII_MAX_PTAB; it++, info++ )
	{
	    int n_part = 0, ip;
	    for ( ip = 0; ip < disc->n_part; ip++ )
	    {
		wd_part_t * part = disc->part + ip;
		if ( !part->is_valid || !part->is_enabled || part->ptab_index != it )
		    continue;

		n_part++;
		entry->off4  = htonl(part->part_off4);
		entry->ptype = htonl(part->part_type);
		entry++;
	    }

	    if (n_part)
	    {
		info->n_part = htonl(n_part);
		info->off4   = htonl(off4);
		off4 += n_part * sizeof(*entry) >> 2;
	    }
	}
    }

    return true;   
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_id
(
    void		* dest_id,	// destination, size=id_len, not 0 term
    const void		* source_id,	// NULL or source id, length=id_len
    const void		* new_id,	// NULL or new ID / 0 term / '.': don't change
    u32			id_len		// max length of id
)
{
    DASSERT(dest_id);

    if (!source_id)
	source_id = dest_id;
    else if ( dest_id != source_id )
	memcpy(dest_id,source_id,id_len);
    if (!new_id)
	return false;

    char * dest = dest_id;
    ccp src = new_id;

    int i;
    for ( i = 0; i < id_len && src[i]; i++ )
	if ( src[i] != '.' )
	    dest[i] = src[i];
    return memcmp(dest,source_id,id_len) != 0;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_disc_header // result = true if something changed
(
    wd_disc_t		* disc,		// valid pointer to a disc
    ccp			new_id,		// NULL or new ID / 0 term / '.': don't change
    ccp			new_name	// NULL or new disc name
)
{
    DASSERT(disc);
    bool stat = false;

    if ( new_id && !( disc->disc_attrib & WD_DA_GC_MULTIBOOT ) )
    {
	char id6[6];
	ccp src = &disc->dhead.disc_id;
	if (wd_patch_id(id6,src,new_id,6))
	{
	    stat = true;
	    wd_memmap_item_t * item
		= wd_insert_memmap_alloc( &disc->patch, WD_PAT_DATA, 0, 6 );
	    DASSERT(item);
	    snprintf(item->info,sizeof(item->info),
			"disc id: %s -> %s",
			wd_print_id(src,6,0), wd_print_id(id6,6,0) );
	    memcpy(item->data,id6,6);
	}
    }

    if (new_name)
    {
	char name[WII_TITLE_SIZE];
	strncpy(name,new_name,sizeof(name));
	name[sizeof(name)-1] = 0;

	if (memcmp(name,disc->dhead.disc_title,sizeof(name)))
	{
	    stat = true;
	    wd_memmap_item_t * item
		= wd_insert_memmap_alloc( &disc->patch, WD_PAT_DATA,
					WII_TITLE_OFF, WII_TITLE_SIZE );
	    DASSERT(item);
	    snprintf(item->info,sizeof(item->info),"disc name: %s",name);
	    memcpy(item->data,name,WII_TITLE_SIZE);
	}
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_region // result = true if something changed
(
    wd_disc_t		* disc,		// valid pointer to a disc
    u32			new_region	// new region id
)
{
    DASSERT(disc);

    if ( disc->disc_type == WD_DT_GAMECUBE )
    {
	const u32 be_region = htonl(new_region);
	if ( be_region == disc->region.region )
	    return false;

	wd_memmap_item_t * item
		= wd_insert_memmap_alloc( &disc->patch, WD_PAT_DATA,
					WII_BI2_OFF + WII_BI2_REGION_OFF,
					sizeof(u32) );
	snprintf(item->info,sizeof(item->info),"region: %x",new_region);
	*(u32*)item->data = be_region;
	return true;
    }

    if (!wd_disc_has_region_setting(disc))
	return false;

    wd_memmap_item_t * item
	= wd_insert_memmap_alloc( &disc->patch, WD_PAT_DATA,
					WII_REGION_OFF, WII_REGION_SIZE );
    snprintf(item->info,sizeof(item->info),"region: %x",new_region);
    wd_region_t * reg = item->data;
    reg->region = htonl(new_region);
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_common_key // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    wd_ckey_index_t	ckey_index	// new common key index
)
{
    DASSERT(part);

    if (!wd_part_has_ticket(part))
	return 0;

    bool stat = false;

    if ( (unsigned)ckey_index < WD_CKEY__N
	&& ckey_index != part->ph.ticket.common_key_index )
    {
	part->ph.ticket.common_key_index = ckey_index;
	wd_insert_patch_ticket(part);
	stat = true;
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_part_id // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    wd_modify_t		modify,		// objects to modify
    ccp			new_disc_id,	// NULL or new disc ID / '.': don't change
    ccp			new_boot_id,	// NULL or new boot ID / '.': don't change
    ccp			new_ticket_id,	// NULL or new ticket ID / '.': don't change
    ccp			new_tmd_id	// NULL or new tmd ID / '.': don't change
)
{
    DASSERT(part);
    DASSERT(part->disc);

    if (part->is_gc)
	modify = modify & (WD_MODIFY_DISC|WD_MODIFY_BOOT) ? WD_MODIFY_DISC : 0;

    bool stat = false;

    if ( new_disc_id && modify & WD_MODIFY_DISC )
	stat = wd_patch_disc_header(part->disc,new_disc_id,0);

    char id6[6];
    if ( new_ticket_id && modify & WD_MODIFY_TICKET && wd_part_has_ticket(part) )
    {
	u8 * src = part->ph.ticket.title_id+4;
	if (wd_patch_id(id6,src,new_ticket_id,4))
	{
	    memcpy(src,id6,4);
	    wd_insert_patch_ticket(part);
	    stat = true;
	}
    }

    if ( new_tmd_id && modify & WD_MODIFY_TMD && part->tmd && wd_part_has_tmd(part) )
    {
	u8 * src = part->tmd->title_id+4;
	if (wd_patch_id(id6,src,new_tmd_id,4))
	{
	    memcpy(src,id6,4);
	    wd_insert_patch_tmd(part);
	    stat = true;
	}
    }

    if ( new_boot_id && modify & WD_MODIFY_BOOT )
    {
	ccp src = &part->boot.dhead.disc_id;
	if (wd_patch_id(id6,src,new_boot_id,6))
	{
	    wd_memmap_item_t * item
		= wd_insert_memmap_alloc( &part->patch, WD_PAT_DATA, WII_BOOT_OFF, 6 );
	    DASSERT(item);
	    item->index = part->index;
	    snprintf(item->info,sizeof(item->info),
			"boot id: %s -> %s",
			wd_print_id(src,6,0), wd_print_id(id6,6,0) );
	    memcpy(item->data,id6,6);
	    stat = true;
	}
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_part_name // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    ccp			new_name,	// NULL or new disc name
    wd_modify_t		modify		// objects to modify
)
{
    DASSERT(part);

    if (part->is_gc)
	modify = modify & (WD_MODIFY_DISC|WD_MODIFY_BOOT) ? WD_MODIFY_DISC : 0;

    bool stat = false;

    if ( modify & WD_MODIFY_DISC )
	stat = wd_patch_disc_header(part->disc,0,new_name);

    if ( modify & WD_MODIFY_BOOT )
    {
	char name[WII_TITLE_SIZE];
	strncpy(name,new_name,sizeof(name));
	name[sizeof(name)-1] = 0;

	if (memcmp(name,part->boot.dhead.disc_title,sizeof(name)))
	{
	    wd_memmap_item_t * item
		= wd_insert_memmap_alloc( &part->patch, WD_PAT_DATA,
				WII_BOOT_OFF + WII_TITLE_OFF, WII_TITLE_SIZE );
	    DASSERT(item);
	    snprintf(item->info,sizeof(item->info),"boot name: %s",name);
	    memcpy(item->data,name,WII_TITLE_SIZE);
	    stat = true;
	}
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

bool wd_patch_part_system // result = true if something changed
(
    wd_part_t		* part,		// valid pointer to a disc partition
    u64			system		// new system id (IOS)
)
{
    DASSERT(part);

    bool stat = false;
    system = hton64(system);
    if ( wd_part_has_tmd(part) && part->tmd->sys_version != system )
    {
	part->tmd->sys_version = system;
	wd_insert_patch_tmd(part);
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int wd_patch_main_iterator
(
    struct wd_iterator_t	*it	// iterator struct with all infos
)
{
    if ( !it->part || it->icm != WD_ICM_FILE )
	return 0;

    wd_patch_main_t *pm = it->param;
    DASSERT(pm);

    wd_memmap_item_t ** item_ptr;
    if ( pm->patch_main && !pm->main
	&& !strcmp(it->path,"DATA/sys/main.dol") )
    {
	PRINT("-> PATCH MAIN [%u] %s\n",it->size,it->path);
	item_ptr = &pm->main;
    }
    else if ( pm->patch_staticr && !pm->staticr
	&& !strcasecmp(it->path,"DATA/files/rel/StaticR.rel") )
    {
	PRINT("-> PATCH STATIC [%u] %s\n",it->size,it->path);
	item_ptr = &pm->staticr;
    }
    else
	return 0;

    wd_memmap_item_t * item
	= wd_insert_memmap_alloc( &it->part->patch, WD_PAT_DATA,
					it->off4<<2, it->size );
    item->index = it->part->index;
    StringCopyS(item->info,sizeof(item->info),it->path+5);

    enumError err = wd_read_part(it->part,it->off4,item->data,it->size,false);
    if (err)
    {
	return ERROR0(ERR_ERROR,"abort\n");
    }

    *item_ptr = item;
    pm->part = it->part;
    it->part->sign_tmd = true;

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// [[domain]]

int wd_patch_main
(
    wd_patch_main_t	*pm,		// result only, will be initialized
    wd_disc_t		*disc,		// valid disc
    bool		patch_main,	// true: patch 'sys/main.dol'
    bool		patch_staticr	// true: patch 'files/rel/staticr.rel'
)
{
    DASSERT(pm);
    DASSERT(disc);

    memset(pm,0,sizeof(*pm));
    pm->disc		= disc;
    pm->patch_main	= patch_main;
    pm->patch_staticr	= patch_staticr;

    return wd_iterate_files( disc, wd_patch_main_iterator, pm,
			patch_staticr ? 0 : 1, WD_IPM_PART_NAME );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    relocation			///////////////
///////////////////////////////////////////////////////////////////////////////

static void wd_mark_disc_reloc
(
    wd_reloc_t		* reloc,	// valid pointer to relocation table
    u64			off,		// offset of data
    u64			size,		// size of data
    wd_reloc_t		flags		// flags to set
)
{
    u64 end = off + size;
    u32 idx = off / WII_SECTOR_SIZE;
    if ( off > idx * (u64)WII_SECTOR_SIZE )
    {
	DASSERT( idx < WII_MAX_SECTORS );
	reloc[idx] |= flags;
	off = ++idx * (u64)WII_SECTOR_SIZE;
    }

    while ( off < end )
    {
	DASSERT( idx < WII_MAX_SECTORS );
	off += WII_SECTOR_SIZE;
	if ( off <= end && flags & WD_RELOC_F_PATCH )
	    reloc[idx] &= ~WD_RELOC_F_COPY;
	reloc[idx++] |= flags;
    }
}

///////////////////////////////////////////////////////////////////////////////

static void wd_mark_part_reloc
(
    wd_part_t		* part,		// NULL (=disc) or pointer to partition
    wd_reloc_t		* reloc,	// valid pointer to relocation table
    u64			off,		// offset of data
    u64			size,		// size of data
    wd_reloc_t		flags		// flags to set
)
{
    WDPRINT("##### wd_mark_part_reloc() P=%u, off=%llx, size=%llx, fl=%x\n",
		part->index, off, size, flags );

    u64 end = off + size;
    u32 idx = off / WII_SECTOR_DATA_SIZE + part->data_off4 / WII_SECTOR_SIZE4;

    if ( off % WII_SECTOR_DATA_SIZE )
    {
	DASSERT( idx < WII_MAX_SECTORS );
	reloc[idx] |= flags;
	noPRINT("reloc[%x] = %x [off=%llx/%llx]\n",idx,reloc[idx],off,end);
	//off = ++idx * (u64)WII_SECTOR_DATA_SIZE;
	off = ( off / WII_SECTOR_DATA_SIZE + 1 ) * WII_SECTOR_DATA_SIZE;
    }
    noPRINT("[off=%llx/%llx]\n",off,end);

    while ( off < end )
    {
	DASSERT( idx < WII_MAX_SECTORS );
	off += WII_SECTOR_DATA_SIZE;
	if ( off <= end && flags & WD_RELOC_F_PATCH )
	    reloc[idx] &= ~WD_RELOC_F_COPY;
	reloc[idx++] |= flags;
	noPRINT("reloc[%x] = %x [off=%llx/%llx]\n",idx-1,reloc[idx-1],off,end);
    }
}

///////////////////////////////////////////////////////////////////////////////

wd_reloc_t * wd_calc_relocation
(
    wd_disc_t		* disc,		// valid disc pointer
    bool		encrypt,	// true: encrypt partition data
    wd_trim_mode_t	trim_mode,	// trim mode
    u32			trim_align,	// alignment value for trimming
    const wd_select_t	* select,	// NULL or a new selector
    bool		force		// true: force new calculation
)
{
    DASSERT(disc);
    TRACE("wd_calc_relocation(%p,%d,%x,%x,%p,%d)\n",
		disc, encrypt, trim_mode, trim_align, select, force );

    trim_mode  = wd_get_relococation_trim(trim_mode,&trim_align,disc->disc_type);
    if ( disc->trim_mode != trim_mode || disc->trim_align != trim_align )
    {
	PRINT("TRIM: %x/%x -> %x/%x\n",
		disc->trim_mode, disc->trim_align, trim_mode, trim_align );
	disc->trim_mode = trim_mode;
	disc->trim_align = trim_align;
	force = true;
	// [trim]  -> support for disc/part/file trimming -> below
    }

    if (select)
    {
	wd_select(disc,select);
	force = true;
    }

    const size_t reloc_size = WII_MAX_SECTORS * sizeof(*disc->reloc);
    if (!disc->reloc)
	disc->reloc = MALLOC(reloc_size);
    else if (!force)
	return disc->reloc;


    //----- check partition tables

    wd_part_t *part, *part_end = disc->part + disc->n_part;
    int enable_count = 0;
    for ( part = disc->part; part < part_end; part++ )
	if ( part->is_enabled && part->is_valid )
	    enable_count++;

    if ( enable_count < disc->n_part )
    {
	wd_memmap_item_t * item
	    = disc->disc_type == WD_DT_GAMECUBE
		    ? wd_insert_memmap(	&disc->patch,
					WD_PAT_DATA,
					0,
					GC_MULTIBOOT_PTAB_OFF + GC_MULTIBOOT_PTAB_SIZE )
		    : wd_insert_memmap(	&disc->patch,
					WD_PAT_DATA,
					WII_PTAB_REF_OFF,
					WII_MAX_PTAB_SIZE );
	DASSERT(item);
	snprintf(item->info,sizeof(item->info),
		"use %u of %u partitions", enable_count, disc->n_part );
	item->data = &disc->ptab;
	wd_patch_ptab(disc,&disc->ptab,true);

	if ( disc->disc_attrib & WD_DA_GC_MULTIBOOT )
	{
	    wd_part_t * part = disc->part;
	    DASSERT(part);
	    if ( ( !part->is_valid || !part->is_enabled ) && !part->data_off4 )
	    {
		WDPRINT("REMOVE GC BOOT PARTITION\n");
		item = wd_insert_memmap(	&disc->patch,
					WD_PAT_ZERO,
					sizeof(disc->dhead),
					WII_SECTOR_SIZE - sizeof(disc->dhead) );
		DASSERT(item);
		snprintf(item->info,sizeof(item->info),
				"remove boot partition (zero data)" );
	    }
	}
    }


    //---- setup tables

    u8 usage_table[WII_MAX_SECTORS];
    wd_filter_usage_table(disc,usage_table,0);

    wd_reloc_t * reloc = disc->reloc;
    memset(reloc,0,reloc_size);


    //----- base setup

    int idx;
    const wd_reloc_t f_part = encrypt ? WD_RELOC_F_ENCRYPT : WD_RELOC_F_DECRYPT;
    for ( idx = 0; idx < WII_MAX_SECTORS; idx++ )
    {
	const u32 uval = usage_table[idx];
	if ( uval >= WD_USAGE_PART_0 && !disc->whole_disc )
	{
	    reloc[idx]	= ( (uval&WD_USAGE__MASK) - WD_USAGE_PART_0 ) << WD_RELOC_S_PART
			| WD_RELOC_F_COPY;
	    if ( uval & WD_USAGE_F_CRYPT )
		reloc[idx] |= f_part;
	}
	else if ( uval )
	    reloc[idx] = WD_RELOC_F_COPY | WD_RELOC_M_PART;
    }

    if (disc->whole_disc)
	return reloc;


    //----- partition patching

    for ( part = disc->part; part < part_end; part++ )
    {
	if (part->patch.used)
	{
	    //--- tmd

	    wd_load_part(part,false,true,false);
	    wd_memmap_item_t * item = wd_insert_patch_tmd(part);
		wd_mark_disc_reloc(reloc,item->offset,item->size,WD_RELOC_F_LAST);

	    //--- h3

	    if (part->h3)
	    {
		item = wd_insert_memmap( &part->disc->patch, WD_PAT_DATA,
				(u64)( part->part_off4 + part->ph.h3_off4 ) << 2,
				WII_H3_SIZE );
		DASSERT(item);
		item->index = part->index;
		item->data = part->h3;
		snprintf(item->info,sizeof(item->info),"h3");
		wd_mark_disc_reloc(reloc,item->offset,item->size,
					WD_RELOC_F_PATCH|WD_RELOC_F_LAST);
	    }

	    //--- items

	    const wd_memmap_t *patch = &part->patch;
	    const wd_memmap_item_t *last = patch->item + patch->used;
	    for ( item = patch->item; item < last; item++ )
		wd_mark_part_reloc(part,reloc,item->offset,item->size,WD_RELOC_F_PATCH);


	    //--- calc WD_RELOC_F_HASH

	    u32 sect = part->data_sector;
	    while ( sect < part->end_sector )
	    {
		u32 start = sect;
		u32 end = start + WII_GROUP_SECTORS;
		if ( end > part->end_sector )
		     end = part->end_sector;
		for ( ; sect < end; sect++ )
		    if ( reloc[sect] & WD_RELOC_F_PATCH )
		    {
			while ( start < end )
			    reloc[start++] |= WD_RELOC_F_HASH;
			break;
		    }
		sect = end;
	    }
	}
    }


    //----- disc patching

    const wd_memmap_t * patch = &disc->patch;
    const wd_memmap_item_t *item, *last = patch->item + patch->used;
    for ( item = patch->item; item < last; item++ )
	wd_mark_disc_reloc(reloc,item->offset,item->size,WD_RELOC_F_PATCH);


    //----- terminate

    wd_patch_ptab(disc,&disc->ptab,true);
    return reloc;
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_relocation
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const wd_reloc_t	* reloc,	// valid pointer to relocation table
    bool		print_title	// true: print table titles
)
{
    DASSERT(f);
    DASSERT(reloc);

    indent = wd_normalize_indent(indent);
    if (print_title)
    {
	fprintf(f,"\n%*sRelocation table:\n",indent,"");
	indent += 2;
    }

    fprintf(f,
	"\n"
	"%*s   offset     dest blocks : n(b) :  source blocks : partition and flags\n"
	"%*s%.79s\n",
	indent, "", indent, "", wd_sep_200 );

    const wd_reloc_t *rel = reloc, *end = reloc + WII_MAX_SECTORS;
    while ( rel < end )
    {
	//--- skip unused blocks

	while ( rel < end && !*rel )
	    rel++;

	if ( rel == end )
	    break;

	//--- store values

	const wd_reloc_t * start = rel;
	const u32 val = *rel;
	while ( rel < end && val == *rel )
	    rel++;

	const int dest = start - reloc;
	int src = dest + ( val & WD_RELOC_M_DELTA );
	if ( src > WII_MAX_SECTORS )
	    src -= WII_MAX_SECTORS;
	const int count = rel - start;

	if ( count > 1 )
	    fprintf(f,"%*s%9llx  %5x .. %5x :%5x : %5x .. %5x :",
			indent, "",
			(u64)WII_SECTOR_SIZE * dest,
			dest, dest + count-1, count, src, src + count-1 );
	else
	    fprintf(f,"%*s%9llx  %14x :    1 : %14x :",
			indent, "",
			(u64)WII_SECTOR_SIZE * dest, dest, src );

	const wd_reloc_t pidx = val & WD_RELOC_M_PART;
	if ( pidx == WD_RELOC_M_PART )
	    fprintf(f,"  -");
	else
	    fprintf(f,"%3u", pidx >> WD_RELOC_S_PART );

	fprintf(f," %s %s %s %s %s\n",
		val & WD_RELOC_F_ENCRYPT ? "enc"
					   : val & WD_RELOC_F_DECRYPT ? "dec" : " - ",
		val & WD_RELOC_F_COPY  ? "copy"  : " -  ",
		val & WD_RELOC_F_PATCH ? "patch" : "  -  ",
		val & WD_RELOC_F_HASH  ? "hash"  : " -  ",
		val & WD_RELOC_F_LAST  ? "last"  : " -" );
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_disc_relocation
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid pointer to a disc
    bool		print_title	// true: print table titles
)
{
    DASSERT(disc);
    wd_print_relocation(f,indent,
	wd_calc_relocation(disc,true,disc->trim_mode,disc->trim_align,0,false),
	print_title );
}

///////////////////////////////////////////////////////////////////////////////

wd_trim_mode_t wd_get_relococation_trim
(
    wd_trim_mode_t	trim_mode,	// trim mode to check
    u32			* trim_align,	// NULL or trim alignment (modify)
    wd_disc_type_t	disc_type	// type of disc for align calculation
)
{
    wd_trim_mode_t res = trim_mode & (WD_TRIM_DISC|WD_TRIM_PART);
    if ( res & WD_TRIM_DISC )
	res |= trim_mode & WD_TRIM_F_END;

    if ( trim_align )
    {
	if (!res)
	    *trim_align = 0;
	else
	{
	    const u32 align0
		= *trim_align > WII_GROUP_SIZE
			? WII_GROUP_SIZE
			: *trim_align
				? *trim_align
				: disc_type == WD_DT_GAMECUBE
					? GC_GOOD_PART_ALIGN
					: WII_SECTOR_SIZE;
	    u32 align = WII_SECTOR_SIZE;
	    while ( align < align0 )
		align <<= 1;
	    *trim_align = align;
	}
    }
    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			file relocation			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_file_list_t * wd_initialize_file_list
(
    wd_file_list_t	* fl		// NULL or working file list
					// If NULL: allocate a file list
)
{
    if (!fl)
	fl = MALLOC(sizeof(*fl));

    memset(fl,0,sizeof(*fl));
    return fl;
}

//-----------------------------------------------------------------------------

void wd_reset_file_list
(
    wd_file_list_t	* fl,		// NULL or working file list to reset (free data)
    bool		free_fl		// true: call 'FREE(fl)'
)
{
    if (fl)
    {
	DASSERT( fl->used <= fl->size );
	wd_file_t * file = fl->file;
	wd_file_t * file_end  = file + fl->used;
	for ( ; file < file_end; file++ )
	{
	    FREE((char*)file->iso_path);
	    FREE((char*)file->file_path);
	}

	FREE(fl->file);
	if (free_fl)
	    FREE(fl);
	else
	    memset(fl,0,sizeof(*fl));
    }
}

//-----------------------------------------------------------------------------

wd_file_list_t * wd_create_file_list
(
    wd_file_list_t	* fl,		// NULL or working file list
					// If NULL: allocate a file list
    wd_fst_item_t	* fst,		// valid fst data structure
    bool		fst_is_gc	// true: 'fst' is a GameCube source
)
{
    if (fl)
	wd_reset_file_list(fl,false);
    else
	fl = wd_initialize_file_list(0);

    // [[2do]]

    return fl;
}

//-----------------------------------------------------------------------------

wd_file_t * wd_insert_file
(
    wd_file_list_t	* fl,		// working file list
    u32			src_off4,	// source offset
    u32			size,		// size of file
    ccp			file_path	// valid filename
)
{
    // [[2do]]
    return 0;
}

//-----------------------------------------------------------------------------

wd_file_t * wd_insert_directory
(
    wd_file_list_t	* fl,		// working file list
    ccp			dir_path	// valid directory name
)
{
    // [[2do]]
    return 0;
}

//-----------------------------------------------------------------------------

wd_fst_item_t * wd_create_file_list_fst
(
    wd_file_list_t	* fl,		// working file list
    u32			* n_fst,	// not NULL: store number of created fst elements
    bool		create_gc_fst,	// true: create a GameCube fst

    u32			align1,		// minimal alignment
    u32			align2,		// alignment for files with size >= align2
    u32			align3		// alignment for files with size >= align3
					//   All aligment values are rounded 
					//   up to the next power of 2.
					//   The values are normalized (increased) 
					//   to align1 <= align2 <= align3
)
{
    DASSERT(fl);

    // [[2do]]

    return 0;
}

//-----------------------------------------------------------------------------

bool wd_is_directory
(
    wd_file_t		* file		// pointer to a file
)
{
    DASSERT(file);
    return !file->dest_off4;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			dump data structure		///////////////
///////////////////////////////////////////////////////////////////////////////

void wd_print_disc
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid disc pointer
    int			dump_level	// dump level
					//   >0: print extended part info 
					//   >1: print usage table
)
{
    TRACE("wd_print_disc()\n");

    ASSERT(f);
    ASSERT(disc);

    if ( indent < 0 )
	indent = 0;
    else if ( indent > 50 )
	indent = 50;

    fprintf(f,"%*sRead func+data:     %p %p\n", indent,"",
		disc->read_func, disc->read_data );

    fprintf(f,"%*sFile size:          %llx/hex = %llu\n", indent,"",
		disc->iso_size, disc->iso_size );

    const u8 *m1 = (u8*)&disc->dhead.wii_magic;
    const u8 *m2 = (u8*)&disc->magic2;
    fprintf(f,"%*sMagics:             %02x-%02x-%02x-%02x %02x-%02x-%02x-%02x\n",
		indent,"",
		m1[0], m1[1], m1[2], m1[3], 
		m2[0], m2[1], m2[2], m2[3] );

    fprintf(f,"%*sID and title:       %.6s, %.64s\n", indent,"",
		&disc->dhead.disc_id,  disc->dhead.disc_title );

    fprintf(f,"%*sRegion setting:     %d / %s\n",
		indent,"", ntohl(disc->region.region),
		wd_print_age_rating(0,0,disc->region.age_rating) );

    int ipt;
    for ( ipt = 0; ipt < WII_MAX_PTAB; ipt++ )
    {
	wd_ptab_info_t * pt = disc->ptab_info + ipt;
	const int n = ntohl(pt->n_part);
	if (n)
	    fprintf(f,"%*sPartition table #%u: off=%llx, N(part)=%u\n", indent,"",
		ipt, (u64)ntohl(pt->off4)<<2, n );
    }

    const bool load_part = dump_level > 0;
    int ip;
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,load_part);

	if (load_part)
	    putchar('\n');

	char pname[30];
	fprintf(f,"%*sPartition #%-2u [%u.%02u], "
		"loaded=%d, valid=%d, enabled=%d, enc=%d, type=%s\n",
		indent,"",
		part->index, part->ptab_index , part->ptab_part_index,
		part->is_loaded, part->is_valid, part->is_enabled,
		part->is_encrypted,
		wd_print_part_name(pname,sizeof(pname),
					part->part_type, WD_PNAME_NUM_INFO ));

	if ( load_part && part->is_valid )
	{
	    u64 off = part->part_off4 << 2;
	    fprintf(f,"%*s  TICKET:      %9llx .. %9llx, %9zx\n", indent,"",
		off, off + sizeof(part->ph), sizeof(part->ph) );

	    if ( part->ph.tmd_off4 )
	    {
		off = ( part->part_off4 + part->ph.tmd_off4 ) << 2;
		fprintf(f,"%*s  TMD:         %9llx .. %9llx, %9x\n", indent,"",
		    off, off + part->ph.tmd_size, part->ph.tmd_size );
	    }

	    if ( part->ph.cert_off4 )
	    {
		off = ( part->part_off4 + part->ph.cert_off4 ) << 2;
		fprintf(f,"%*s  CERT:        %9llx .. %9llx, %9x,  loaded=%d\n", indent,"",
		    off, off + part->ph.cert_size, part->ph.cert_size,
		    part->cert != 0 );
	    }

	    if ( part->ph.h3_off4 )
	    {
		off = ( part->part_off4 + part->ph.h3_off4 ) << 2;
		fprintf(f,"%*s  H3:          %9llx .. %9llx, %9x,  loaded=%d\n", indent,"",
		    off, off + WII_H3_SIZE, WII_H3_SIZE,
		    part->h3 != 0 );
	    }

	    if ( part->ph.data_off4 )
	    {
		off = part->data_off4 << 2;
		u64 data_size = part->ph.data_size4 << 2;
		fprintf(f,"%*s  DATA:        %9llx .. %9llx, %9llx\n", indent,"",
		    off, off + data_size, data_size );
	    }
	}
    }

 #if 0 // defined(DEBUG) && defined(TEST)
    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,load_part);

	fprintf(f,"%*s  ID: |",indent,"");
	int pmode;
	for ( pmode = 0; pmode < WD_PNAME__N; pmode++ )
	{
	    char pname[30];
	    wd_print_part_name(pname,sizeof(pname),part->part_type,pmode);
	    fprintf(f,"%s|",pname);
	}
	fputc('\n',f);
    }
 #endif

    if ( dump_level > 1 )
    {
	fprintf(f,"\n%*sUsage map:\n",indent,"");
	wd_print_disc_usage_tab(f,indent+2,disc,false);
    }
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_disc_usage_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    wd_disc_t		* disc,		// valid disc pointer
    bool		print_all	// false: ignore const lines
)
{
    wd_print_usage_tab( f, indent, wd_calc_usage_table(disc), disc->iso_size, print_all );
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_usage_tab
(
    FILE		* f,		// valid output file
    int			indent,		// indention of the output
    const u8		* usage_tab,	// valid pointer, size = WII_MAX_SECTORS
    u64			iso_size,	// NULL or size of iso file
    bool		print_all	// false: ignore const lines
)
{
    wd_print_byte_tab( f, indent, usage_tab,
			( iso_size + WII_SECTOR_SIZE - 1 ) / WII_SECTOR_SIZE,
			WII_MAX_SECTORS, WII_SECTOR_SIZE,
			wd_usage_name_tab, print_all );
}

///////////////////////////////////////////////////////////////////////////////

static void wd_print_files
(
    wd_part_t		* part,		// pointer to partition
    wd_mem_func_t	func,		// valid function pointer
    void		* param,	// user defined parameter
    u64			off,		// minimal offset
    u64			end,		// maximal offset
    u32			fst_sect,	// sector index of fst.bin
    ccp			msg		// base message
)
{
    char buf[80];

    const u8 * utab = part->disc->usage_table;
    const u8 usage_id = part->usage_id;
    while ( off < end )
    {
	u32 sect = off / WII_SECTOR_SIZE;
	u32 start = sect;
	while ( sect < WII_MAX_SECTORS
		&& ( utab[sect] & WD_USAGE__MASK ) != usage_id )
	    sect++;
	if ( start < sect )
	{
	    u64 off2 = sect * (u64)WII_SECTOR_SIZE;
	    if ( off2 > end )
		off2 = end;
	    if ( off < off2 )
		off = off2;
	    start = sect;
	}

	while ( sect < WII_MAX_SECTORS
		&& ( utab[sect] & WD_USAGE__MASK ) == usage_id )
	    sect++;
	if ( start < sect )
	{
	    u64 off2 = sect * (u64)WII_SECTOR_SIZE;
	    if ( off2 > end )
		off2 = end;
	    if ( off < off2 )
	    {
		if ( fst_sect >= start && fst_sect < sect )
		{
		    snprintf(buf,sizeof(buf),"%s, N(fst)=%u", msg, part->fst_n );
		    func(param,off,off2-off,buf);
		}
		else
		    func(param,off,off2-off,msg);
		off = off2;
	    }
	}

	if ( sect >= WII_MAX_SECTORS )
	    break;
    }
}

///////////////////////////////////////////////////////////////////////////////

static void wd_print_gc_mem
(
    wd_disc_t		* disc,		// valid disc pointer
    wd_mem_func_t	func,		// valid function pointer
    void		* param		// user defined parameter
)
{
    ASSERT(disc);
    ASSERT(disc->part);
    DASSERT(func);
    TRACE("wd_print_gc_mem(%p,%p,%p)\n",disc,func,param);

    char msg[80];
    wd_part_t * part = disc->part, *part_end = part + disc->n_part;
    bool have_0_part = false; // partition at offset 0

    for ( ; part < part_end; part++ )
    {
	if (!part->part_off4)
	    have_0_part = true;

	u64 base_off = (u64)part->part_off4 << 2;
	WDPRINT("PART #%u, off=%9llx, size=%9llx, doff4=%8x, sect=%5x..%5x\n",
		part->index, base_off, part->part_size,
		part->data_off4, part->data_sector, part->end_sector );

	char * dest = msg;
	if ( disc->n_part || !have_0_part )
	    dest += snprintf( msg, sizeof(msg), "%sP.%u: ",
				part->index & 1 ? "" : " ", part->index );
	size_t msgsize = msg + sizeof(msg) - dest;

	//----- boot.bin

	wd_boot_t * boot = &part->boot;
	const u8 *m = (u8*)&boot->dhead.gc_magic;
	snprintf( dest, msgsize,
		    "boot.bin, magic=%02x-%02x-%02x-%02x, id=%s",
		    m[0], m[1], m[2], m[3], wd_print_id(&boot->dhead.disc_id,6,0) );
	func(param,base_off+WII_BOOT_OFF,WII_BOOT_SIZE,msg);

	//----- bi2.bin

	snprintf( dest, msgsize, "%s", "bi2.bin" );
	func(param,base_off+WII_BI2_OFF,WII_BI2_SIZE,msg);

	//----- apploader.img

	snprintf( dest, msgsize, "%s", "apploader.img" );
	func(param,base_off+WII_APL_OFF,part->apl_size,msg);

	//----- main.dol

	snprintf( dest, msgsize, "%s", "main.dol" );
	func(param,base_off+boot->dol_off4,part->dol_size,msg);

	//----- fst.bin

	snprintf( dest, msgsize, "%s", "fst.bin" );
	func(param,base_off+boot->fst_off4,boot->fst_size4,msg);

	//----- files

	wd_fst_item_t * fst = part->fst;
	if (fst)
	{
	    u32 min = ~(u32)0, max = 0;
	    const wd_fst_item_t *fst_end = fst + ntohl(fst->size);

	    for ( fst++; fst < fst_end; fst++ )
		if (!fst->is_dir)
		{
		    u32 off = ntohl(fst->offset4);
		    if ( min > off )
			 min = off;
		    off += ntohl(fst->size);
		    if ( max < off )
			 max = off;
		}

	    if ( min < max )
	    {
		min += base_off;
		max += base_off;
		snprintf( dest, msgsize, "%s", "files" );
		wd_print_files(part,func,param,min,max,min/WII_SECTOR_SIZE,msg);
	    }
	}
    }

    if (!have_0_part)
    {
	snprintf( msg, sizeof(msg),
		    "GameCube multiboot header, id=%.6s%s",
		    &disc->dhead.disc_id,
		    memcmp(&disc->dhead.disc_id+4,"DVD9",4) ? "" : ", dvd9" );
	func(param,0,0x100,msg);
    }

    if (disc->iso_size)
	func(param,disc->iso_size,0,"-- End of file --");
}

///////////////////////////////////////////////////////////////////////////////

void wd_print_mem
(
    wd_disc_t		* disc,		// valid disc pointer
    wd_mem_func_t	func,		// valid function pointer
    void		* param		// user defined parameter
)
{
    ASSERT(disc);
    DASSERT(func);

    if ( disc->disc_type == WD_DT_GAMECUBE )
    {
	wd_print_gc_mem(disc,func,param);
	return;
    }

    TRACE("wd_print_mem(%p,%p,%p)\n",disc,func,param);
    char msg[80];

    //----- header

    const u8 *m = (u8*)&disc->dhead.wii_magic;
    snprintf( msg, sizeof(msg),
		"Header, magic=%02x-%02x-%02x-%02x, id=%s",
		m[0], m[1], m[2], m[3], wd_print_id(&disc->dhead.disc_id,6,0) );
    func(param,0,sizeof(disc->dhead),msg);


    //----- partition tables

    snprintf( msg, sizeof(msg),
		"Partition address table, N=%u+%u+%u+%u",
		ntohl(disc->ptab_info[0].n_part),
		ntohl(disc->ptab_info[1].n_part),
		ntohl(disc->ptab_info[2].n_part),
		ntohl(disc->ptab_info[3].n_part) );
    func(param,WII_PTAB_REF_OFF,sizeof(disc->ptab_info),msg);

    int ip;
    for ( ip = 0; ip < WII_MAX_PTAB; ip++ )
    {
	const u32 np = ntohl(disc->ptab_info[ip].n_part);
	if (np)
	{
	    const u64 off = (u64)ntohl(disc->ptab_info[ip].off4) << 2;
	    snprintf( msg, sizeof(msg),
			"Partition table #%u with %u partition%s",
			ip, np, np == 1 ? "" : "s" );
	    func(param,off,np*sizeof(wd_ptab_entry_t),msg);
	}
    }


    //----- region settings

    snprintf( msg, sizeof(msg),
		"Region settings, region=%x",
		ntohl(disc->region.region) );
    func(param,WII_REGION_OFF,sizeof(disc->region),msg);


    //----- magic2

    m = (u8*)&disc->magic2;
    snprintf( msg, sizeof(msg),
		"Magic2: %02x-%02x-%02x-%02x",
		m[0], m[1], m[2], m[3] );
    func(param,WII_MAGIC2_OFF,sizeof(disc->magic2),msg);


    //----- iterate partitions

    for ( ip = 0; ip < disc->n_part; ip++ )
    {
	wd_part_t * part = wd_get_part_by_index(disc,ip,1);

	char pname_buf[20];
	char * pname = wd_print_part_name(pname_buf,sizeof(pname_buf),
						part->part_type, WD_PNAME_NAME );
	char * dest = msg + snprintf( msg, sizeof(msg),
					"P.%u.%u %s: ",
					part->ptab_index, part->ptab_part_index,
					pname );
	size_t msgsize = msg + sizeof(msg) - dest;

	if (!part->is_valid)
	{
	    snprintf( dest, msgsize, "** INVALID **" );
	    func(param,(u64)part->part_off4<<2,0,msg);
	}
	else if (part->is_enabled)
	{
	    const wd_part_header_t *ph = &part->ph;

	    //--- header + ticket

	    snprintf( dest, msgsize, "ticket, id=%s",
			wd_print_id(ph->ticket.title_id+4,4,0) );
	    func(param,(u64)part->part_off4<<2,sizeof(*ph),msg);

	    //--- tmd

	    if (ph->tmd_off4)
	    {
		const u64 sys_version = ntoh64(part->tmd->sys_version);
		const u32 hi = sys_version >> 32;
		const u32 lo = (u32)sys_version;
		if ( hi == 1 && lo < 0x100 )
		    snprintf( dest, msgsize, "tmd, ios=%u, id=%s",
				lo, wd_print_id(part->tmd->title_id+4,4,0) );
		else
		    snprintf( dest, msgsize, "tmd, sys=%08x-%08x, id=%s",
				hi, lo, wd_print_id(part->tmd->title_id+4,4,0) );
		func(param,(u64)(part->part_off4+ph->tmd_off4)<<2,ph->tmd_size,msg);
	    }

	    //--- cert

	    if (ph->cert_off4)
	    {
		snprintf( dest, msgsize, "cert" );
		func(param,(u64)(part->part_off4+ph->cert_off4)<<2,ph->cert_size,msg);
	    }

	    //--- h3

	    if (ph->h3_off4)
	    {
		snprintf( dest, msgsize, "h3" );
		func(param,(u64)(part->part_off4+ph->h3_off4)<<2,WII_H3_SIZE,msg);
	    }

	    //--- data

	    if (ph->data_off4)
	    {
		u64 off = (u64)part->data_off4<<2;
		u64 end = off + ((u64)ph->data_size4<<2);
		const u32 fst_sect = part->fst_n
			? off/WII_SECTOR_SIZE + part->boot.fst_off4/WII_SECTOR_DATA_SIZE4
			: 0;

		snprintf(dest,msgsize,"data+fst");
		wd_print_files(part,func,param,off,end,fst_sect,msg);
	    }
	}
    }

    //----- end of wii disc

    if (disc->iso_size)
	func(param,disc->iso_size,0,
		"-- End of file/disc --");
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////
