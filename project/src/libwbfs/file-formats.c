
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

#include "file-formats.h"
#include "crypt.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////				setup			///////////////
///////////////////////////////////////////////////////////////////////////////

int validate_file_format_sizes ( int trace_sizes )
{
    // 1. trace sizeof

 #ifdef TRACE_SIZEOF
    if (trace_sizes)
    {
	TRACE_SIZEOF(u8);
	TRACE_SIZEOF(u16);
	TRACE_SIZEOF(u32);
	TRACE_SIZEOF(u64);

	TRACE_SIZEOF(be16_t);
	TRACE_SIZEOF(be32_t);
	TRACE_SIZEOF(be64_t);

	TRACE_SIZEOF(dol_header_t);
	TRACE_SIZEOF(dol_record_t);
	TRACE_SIZEOF(wbfs_inode_info_t);
	TRACE_SIZEOF(wd_header_128_t);
	TRACE_SIZEOF(wd_header_t);
	TRACE_SIZEOF(wd_boot_t);
	TRACE_SIZEOF(wd_region_t);
	TRACE_SIZEOF(wd_ptab_info_t);
	TRACE_SIZEOF(wd_ptab_entry_t);
	TRACE_SIZEOF(wd_ptab_t);
	TRACE_SIZEOF(wd_ticket_t);
	TRACE_SIZEOF(wd_part_header_t);
	TRACE_SIZEOF(wd_tmd_content_t);
	TRACE_SIZEOF(wd_tmd_t);
	TRACE_SIZEOF(wd_part_control_t);
	TRACE_SIZEOF(wd_part_sector_t);
	TRACE_SIZEOF(wd_fst_item_t);

	TRACE_SIZEOF(wbfs_head_t);
	TRACE_SIZEOF(wbfs_disc_info_t);

	TRACE_SIZEOF(WIT_PATCH_MAGIC);
	TRACE_SIZEOF(wpat_magic);
	TRACE_SIZEOF(wpat_comment_t);
	TRACE_SIZEOF(wpat_data_t);
	TRACE_SIZEOF(wpat_filename_t);
	TRACE_SIZEOF(wpat_filenames_t);
	TRACE_SIZEOF(wpat_header_t);
	TRACE_SIZEOF(wpat_patch_file_t);
	TRACE_SIZEOF(wpat_size_t);
	TRACE_SIZEOF(wpat_toc_file_t);
	TRACE_SIZEOF(wpat_toc_header_t);
	TRACE_SIZEOF(wpat_type_t);

	TRACE_SIZEOF(wiiuser_header_t);

     #ifdef DEBUG
	wd_part_control_t pc, pc_saved;
	ASSERT(!clear_part_control(&pc,
			WII_TMD_GOOD_SIZE,
			0xa00, 0x1000000 ));
	memcpy(&pc_saved,&pc,sizeof(pc_saved));
	ASSERT(!setup_part_control(&pc));
	ASSERT(!memcmp(&pc_saved,&pc,sizeof(pc_saved)));
     #endif
    }
 #endif

    // 2. assertions

    wd_tmd_t * tmd = 0;
    wd_ticket_t * tik = 0;

    #undef  CHECK
    #define CHECK ASSERT
    #undef  OFFSET
    #define OFFSET(p,i) ((char*)&p->i-(char*)p)

    CHECK( 1 == sizeof(u8)  );
    CHECK( 2 == sizeof(u16) );
    CHECK( 4 == sizeof(u32) );
    CHECK( 8 == sizeof(u64) );

    CHECK( 2 == sizeof(be16_t) );
    CHECK( 4 == sizeof(be32_t) );
    CHECK( 8 == sizeof(be64_t) );

    CHECK( sizeof(wbfs_inode_info_t) + WBFS_INODE_INFO_OFF == 0x100 );
    CHECK( sizeof(wbfs_inode_info_t)	== WBFS_INODE_INFO_SIZE );

    CHECK( sizeof(dol_header_t)		== DOL_HEADER_SIZE );
    CHECK( sizeof(wd_header_128_t)	==  0x80 );
    CHECK( sizeof(wd_header_t)		== 0x100 );
    CHECK( sizeof(wd_region_t)		== WII_REGION_SIZE );
    CHECK( sizeof(wd_ptab_t)		== WII_MAX_PTAB_SIZE );
    CHECK( sizeof(wd_boot_t)		== WII_BOOT_SIZE );
    CHECK( sizeof(wd_part_sector_t)	== WII_SECTOR_SIZE );
    CHECK( sizeof(wd_fst_item_t)	== 12 ); // test because of union

    CHECK( OFFSET(tik,title_key)	== WII_TICKET_KEY_OFF );
    CHECK( OFFSET(tik,title_id)		== WII_TICKET_IV_OFF );
    CHECK( OFFSET(tik,issuer)		== WII_TICKET_SIG_OFF );
    CHECK( OFFSET(tik,fake_sign)	== WII_TICKET_BRUTE_FORCE_OFF );
    CHECK( sizeof(wd_ticket_t)		== WII_TICKET_SIZE );

    CHECK( OFFSET(tmd,issuer)		== WII_TMD_SIG_OFF );
    CHECK( OFFSET(tmd,fake_sign)	== WII_TMD_BRUTE_FORCE_OFF );
    CHECK( OFFSET(tmd,content[0].hash)	== 0x1f4 );
    CHECK( sizeof(wd_tmd_t)		== 0x1e4 );
    CHECK( sizeof(wd_tmd_content_t)	== 0x24 );
    CHECK( sizeof(wd_tmd_t) + sizeof(wd_tmd_content_t) == WII_TMD_GOOD_SIZE );

    CHECK( sizeof(wiiuser_header_t)	== WIIUSER_HEAD_SIZE );

    // >>> copy to below


    //----- 3. calculate return value

    #undef  CHECK
    #define CHECK(a) if (!(a)) return __LINE__;

    CHECK( 1 == sizeof(u8)  );
    CHECK( 2 == sizeof(u16) );
    CHECK( 4 == sizeof(u32) );
    CHECK( 8 == sizeof(u64) );

    CHECK( 2 == sizeof(be16_t) );
    CHECK( 4 == sizeof(be32_t) );
    CHECK( 8 == sizeof(be64_t) );

    CHECK( sizeof(wbfs_inode_info_t) + WBFS_INODE_INFO_OFF == 0x100 );
    CHECK( sizeof(wbfs_inode_info_t)	== WBFS_INODE_INFO_SIZE );

    CHECK( sizeof(dol_header_t)		== DOL_HEADER_SIZE );
    CHECK( sizeof(wd_header_128_t)	==  0x80 );
    CHECK( sizeof(wd_header_t)		== 0x100 );
    CHECK( sizeof(wd_region_t)		== WII_REGION_SIZE );
    CHECK( sizeof(wd_ptab_t)		== WII_MAX_PTAB_SIZE );
    CHECK( sizeof(wd_boot_t)		== WII_BOOT_SIZE );
    CHECK( sizeof(wd_part_sector_t)	== WII_SECTOR_SIZE );
    CHECK( sizeof(wd_fst_item_t)	== 12 ); // test because of union

    CHECK( OFFSET(tik,title_key)	== WII_TICKET_KEY_OFF );
    CHECK( OFFSET(tik,title_id)		== WII_TICKET_IV_OFF );
    CHECK( OFFSET(tik,issuer)		== WII_TICKET_SIG_OFF );
    CHECK( OFFSET(tik,fake_sign)	== WII_TICKET_BRUTE_FORCE_OFF );
    CHECK( sizeof(wd_ticket_t)		== WII_TICKET_SIZE );

    CHECK( OFFSET(tmd,issuer)		== WII_TMD_SIG_OFF );
    CHECK( OFFSET(tmd,fake_sign)	== WII_TMD_BRUTE_FORCE_OFF );
    CHECK( OFFSET(tmd,content[0].hash)	== 0x1f4 );
    CHECK( sizeof(wd_tmd_t)		== 0x1e4 );
    CHECK( sizeof(wd_tmd_content_t)	== 0x24 );
    CHECK( sizeof(wd_tmd_t) + sizeof(wd_tmd_content_t) == WII_TMD_GOOD_SIZE );

    CHECK( sizeof(wiiuser_header_t)	== WIIUSER_HEAD_SIZE );

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		endian conversions for structs		///////////////
///////////////////////////////////////////////////////////////////////////////

void ntoh_dol_header ( dol_header_t * dest, const dol_header_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    const u32 * src_ptr = src->sect_off;
    u32 * dest_ptr = dest->sect_off;
    u32 * dest_end = (u32*)&dest->padding;

    while ( dest_ptr < dest_end )
	*dest_ptr++ = ntohl(*src_ptr++);
}

//-----------------------------------------------------------------------------

void hton_dol_header ( dol_header_t * dest, const dol_header_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    const u32 * src_ptr = src->sect_off;
    u32 * dest_ptr = dest->sect_off;
    u32 * dest_end = (u32*)&dest->padding;

    while ( dest_ptr < dest_end )
	*dest_ptr++ = htonl(*src_ptr++);
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_boot ( wd_boot_t * dest, const wd_boot_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->dol_off4	= ntohl(src->dol_off4);
    dest->fst_off4	= ntohl(src->fst_off4);
    dest->fst_size4	= ntohl(src->fst_size4);
    dest->max_fst_size4	= ntohl(src->max_fst_size4);
}

//-----------------------------------------------------------------------------

void hton_boot ( wd_boot_t * dest, const wd_boot_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->dol_off4	= htonl(src->dol_off4);
    dest->fst_off4	= htonl(src->fst_off4);
    dest->fst_size4	= htonl(src->fst_size4);
    dest->max_fst_size4	= htonl(src->max_fst_size4);
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_part_header ( wd_part_header_t * dest, const wd_part_header_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->tmd_size	= ntohl(src->tmd_size);
    dest->tmd_off4	= ntohl(src->tmd_off4);
    dest->cert_size	= ntohl(src->cert_size);
    dest->cert_off4	= ntohl(src->cert_off4);
    dest->h3_off4	= ntohl(src->h3_off4);
    dest->data_off4	= ntohl(src->data_off4);
    dest->data_size4	= ntohl(src->data_size4);
}

//-----------------------------------------------------------------------------

void hton_part_header ( wd_part_header_t * dest, const wd_part_header_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->tmd_size	= htonl(src->tmd_size);
    dest->tmd_off4	= htonl(src->tmd_off4);
    dest->cert_size	= htonl(src->cert_size);
    dest->cert_off4	= htonl(src->cert_off4);
    dest->h3_off4	= htonl(src->h3_off4);
    dest->data_off4	= htonl(src->data_off4);
    dest->data_size4	= htonl(src->data_size4);
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_inode_info ( wbfs_inode_info_t * dest, const wbfs_inode_info_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->n_hd_sec	= ntohl(src->n_hd_sec);
    dest->info_version	= ntohl(src->info_version);

    dest->itime		= ntoh64(src->itime);
    dest->mtime		= ntoh64(src->mtime);
    dest->ctime		= ntoh64(src->ctime);
    dest->atime		= ntoh64(src->atime);
    dest->dtime		= ntoh64(src->dtime);
}

//-----------------------------------------------------------------------------

void hton_inode_info ( wbfs_inode_info_t * dest, const wbfs_inode_info_t * src )
{
    DASSERT(dest);

    if (!src)
	src = dest;
    else if ( dest != src )
	memcpy(dest,src,sizeof(*dest));

    dest->n_hd_sec	= htonl(src->n_hd_sec);
    dest->info_version	= htonl(src->info_version);

    dest->itime		= hton64(src->itime);
    dest->mtime		= hton64(src->mtime);
    dest->ctime		= hton64(src->ctime);
    dest->atime		= hton64(src->atime);
    dest->dtime		= hton64(src->dtime);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct dol_record_t		///////////////
///////////////////////////////////////////////////////////////////////////////

uint calc_dol_records
(
    dol_record_t	*rec,		// pointer to at least DOL_N_SECTIONS records
    bool		term_null,	// true: add a NULL record at end of list
    const dol_header_t	*dol_head	// source DOL header
)
{
    DASSERT(rec);
    DASSERT(dol_head);

    dol_header_t dh;
    ntoh_dol_header(&dh,dol_head);
    dol_record_t *dest = rec;
    u32 last_addr = 0xffffffff;

    for(;;)
    {
	u32 max_addr = 0;
	int i, found = -1;
	for ( i = 0; i < DOL_N_SECTIONS; i++ )
	{
	    if (   dh.sect_off[i]
		&& dh.sect_size[i]
		&& dh.sect_addr[i] < last_addr
		&& dh.sect_addr[i] > max_addr )
	    {
		found = i;
		max_addr = dh.sect_addr[i];
	    }
	}
	if ( found < 0 )
	    break;

	memset(dest,0,sizeof(*dest));
	dest->addr	= dh.sect_addr[found];
	dest->size	= dh.sect_size[found];
	dest->xsize	= dest->size;
	dest->delta	= dest->addr - dh.sect_off[found];

	if ( found < DOL_N_TEXT_SECTIONS )
	{
	    dest->name[0] = 'T';
	    dest->name[1] = found+'0';
	}
	else
	{
	    dest->name[0] = 'D';
	    dest->name[1] = found-DOL_N_TEXT_SECTIONS+'0';
	}

	if ( dest->name[1] > '9' )
	{
	    dest->name[2] = dest->name[1] - 10;
	    dest->name[1] = '1';
	}

	if ( dest > rec
	    && dest[-1].delta == dest->delta
	    && dest->addr + dest->size == dest[-1].addr )
	{
	    dest->xsize += dest[-1].xsize;
	}
	last_addr = dest->addr;
	dest++;
    }

    if (term_null)
	memset(dest,0,sizeof(*dest));
    return dest - rec;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const dol_record_t * search_dol_record
(
    dol_record_t	*rec,		// pointer to at least records
    uint		n_rec,		// number of records, set it to DOL_N_SECTIONS
					//   if record lsit is NULL termianted
    u32			addr,		// address to search
    u32			size		// size of object, if NULL, ignore it
)
{
    DASSERT(rec);

    while ( n_rec-- > 0 )
    {
	if (!rec->size)
	    break;
	    
	if ( addr >= rec->addr && addr+size <= rec->addr+rec->xsize )
	    return rec;
	rec++;
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_compression_t		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp wd_get_compression_name
(
    wd_compression_t	compr,		// compression mode
    ccp			invalid_result	// return value if 'compr' is invalid
)
{
    static ccp tab[] =
    {
	"NONE",
	"PURGE",
	"BZIP2",
	"LZMA",
	"LZMA2",
    };

    return (u32)compr < sizeof(tab)/sizeof(*tab) ? tab[compr] : invalid_result;
}

///////////////////////////////////////////////////////////////////////////////

ccp wd_print_compression
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    wd_compression_t	compr_method,	// compression method
    int			compr_level,	// compression level
    u32			chunk_size,	// compression chunk size, multiple of MiB
    int			mode		// 1=number, 2=name, 3=number and name
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    if ( compr_method < 0 || compr_method >= WD_COMPR__N )
    {
	compr_method = WD_COMPR__N;
	compr_level = 0;
    }
    else if ( compr_method < WD_COMPR__FIRST_REAL || compr_level < 0 )
	compr_level = 0;
    else if ( compr_level > 9 )
	compr_level = 9;

    char cbuf[10] = {0};
    if ( compr_method != WD_COMPR__N )
    {
	chunk_size /= WII_GROUP_SIZE; // reduce chunk_size to a factor
	if (chunk_size)
	    snprintf(cbuf,sizeof(cbuf),"@%u",chunk_size);
    }


    mode &= 3;
    if ( mode == 1 )
    {
	if ( compr_method == WD_COMPR__N )
	    snprintf(buf,buf_size,"-");
	else if (compr_level)
	    snprintf(buf,buf_size,"%u.%u%s",compr_method,compr_level,cbuf);
	else
	    snprintf(buf,buf_size,"%u%s",compr_method,cbuf);
    }
    else if ( mode == 2 )
    {
	ccp name = wd_get_compression_name(compr_method,"-");
	if ( compr_method == WD_COMPR__N )
	    snprintf(buf,buf_size,"-");
	else if (compr_level)
	    snprintf(buf,buf_size,"%s.%u%s",name,compr_level,cbuf);
	else
	    snprintf(buf,buf_size,"%s%s",name,cbuf);
    }
    else
    {
	ccp name = wd_get_compression_name(compr_method,"-");
	if ( compr_method == WD_COMPR__N )
	    snprintf(buf,buf_size,"- -");
	else if (compr_level)
	    snprintf(buf,buf_size,"%u.%u%s %s.%u%s",
			compr_method, compr_level, cbuf,
			name, compr_level, cbuf );
	else
	    snprintf(buf,buf_size,"%u%s %s%s",
			compr_method, cbuf, name, cbuf );
    }

    return buf;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_header_t		///////////////
///////////////////////////////////////////////////////////////////////////////

static void id_setup ( void * dest_id, const void * source_id, int id_size )
{
    DASSERT(dest_id);
    DASSERT(id_size>0);

    u8 * dest = dest_id;
    memset(dest,'0',id_size);
    
    int i;
    const u8 * src = source_id ? source_id : "WIT";
    for ( i = 0; i < id_size && *src; i++ )
	*dest++ = *src++;
}

///////////////////////////////////////////////////////////////////////////////

void header_128_setup
(
    wd_header_128_t	* dhead,	// valid pointer
    const void		* id6,		// NULL or pointer to ID
    ccp			disc_title,	// NULL or pointer to disc title (truncated)
    bool		is_gc		// true: GameCube setup
)
{
    memset(dhead,0,sizeof(*dhead));
    id_setup(&dhead->disc_id,id6,6);

    if (!disc_title)
	disc_title = "WIT: Wiimms ISO Tools, https://wit.wiimm.de/";
    strncpy(dhead->disc_title,disc_title,sizeof(dhead->disc_title)-1);

    if (is_gc)
	dhead->gc_magic = htonl(GC_MAGIC);
    else
	dhead->wii_magic = htonl(WII_MAGIC);
}

///////////////////////////////////////////////////////////////////////////////

void header_setup
(
    wd_header_t		* dhead,	// valid pointer
    const void		* id6,		// NULL or pointer to ID
    ccp			disc_title,	// NULL or pointer to disc title (truncated)
    bool		is_gc		// true: GameCube setup
)
{
    memset(dhead,0,sizeof(*dhead));
    header_128_setup((wd_header_128_t*)dhead,id6,disc_title,is_gc);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

wd_disc_type_t get_header_128_disc_type
(
    wd_header_128_t	* dhead,	// valid pointer
    wd_disc_attrib_t	* attrib	// not NULL: store disc attributes
)
{
    DASSERT(dhead);

    //----- check Wii disc

    if ( ntohl(dhead->wii_magic) == WII_MAGIC )
    {
	if (attrib)
	    *attrib = WD_DA_WII;
	return WD_DT_WII;
    }
    

    //----- check GameCube disc

    if ( ntohl(dhead->gc_magic) == GC_MAGIC )
    {
	if (attrib)
	{
	    wd_disc_attrib_t att = WD_DA_GAMECUBE;

	    ccp id6 = &dhead->disc_id;
	    if (   !memcmp(id6,"GCOPDV",6)
		|| !memcmp(id6,"COBRAM",6)
		|| !memcmp(id6,"GGCOSD",6)
		|| !memcmp(id6,"RGCOSD",6) )
	    {
		att |= WD_DA_GC_MULTIBOOT;
		if (!memcmp(id6+4,"DVD9",4))
		    att |= WD_DA_GC_DVD9;
	    }
	    *attrib = att;
	}
	return WD_DT_GAMECUBE;
    }


    //----- unknown disc

    if (attrib)
	*attrib = 0;
    return WD_DT_UNKNOWN; 
}

///////////////////////////////////////////////////////////////////////////////

wd_disc_type_t get_header_disc_type
(
    wd_header_t		* dhead,	// valid pointer
    wd_disc_attrib_t	* attrib	// not NULL: store disc attributes
)
{
    return get_header_128_disc_type((wd_header_128_t*)dhead,attrib);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum wd_age_rating_t		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp wd_print_age_rating
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u8			* age_rating	// valid buffer of 'WD_AGE__N' bytes
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 70 );

    DASSERT( WD_AGE__N == 10 );
    snprintf( buf, buf_size,
		"Jap=%u USA=%u ?=%u Eur=%u,%u,%u,%u,%u,%u Kor=%u",
		age_rating[WD_AGE_JAPAN],
		age_rating[WD_AGE_USA],
		age_rating[WD_AGE_UNKNOWN],
		age_rating[WD_AGE_EUROPE1],
		age_rating[WD_AGE_EUROPE2],
		age_rating[WD_AGE_EUROPE3],
		age_rating[WD_AGE_EUROPE4],
		age_rating[WD_AGE_EUROPE5],
		age_rating[WD_AGE_EUROPE6],
		age_rating[WD_AGE_KOREA] );

    return buf;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_ticket_t		///////////////
///////////////////////////////////////////////////////////////////////////////

const char not_encrypted_marker[] = "*** partition is not encrypted ***";

///////////////////////////////////////////////////////////////////////////////

void ticket_setup ( wd_ticket_t * tik, const void * id4 )
{
    DASSERT(tik);
    memset(tik,0,sizeof(*tik));
    tik->sig_type = htonl(0x10001);
    strncpy((char*)tik->issuer,"Root-CA00000001-XS00000003",sizeof(tik->issuer));

    static u8 base_id[] = { 0,1,0,0, 0,0,0,0 };
    memcpy(tik->title_id,base_id,8);
    id_setup(tik->title_id+4,id4,4);

    memset(tik->unknown7,0xff,sizeof(tik->unknown7));
    tik->unknown3 = 0xffff;

    RANDOM_FILL(tik->title_key,sizeof(tik->title_key));
    RANDOM_FILL(tik->ticket_id,sizeof(tik->ticket_id));
    tik->ticket_id[0] = 0;
    tik->ticket_id[1] = 1;
}

///////////////////////////////////////////////////////////////////////////////

void ticket_clear_encryption ( wd_ticket_t * tik, int mark_not_encrypted )
{
    ASSERT(tik);

    memset(tik->sig,0,sizeof(tik->sig));
    memset(tik->sig_padding,0,sizeof(tik->sig_padding));
    memset(tik->fake_sign,0,sizeof(tik->fake_sign));

    if (mark_not_encrypted)
    {
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tik->sig_padding));
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tik->fake_sign));
	strncpy( (char*)tik->sig_padding, not_encrypted_marker, sizeof(tik->sig_padding)-1 );
	strncpy( (char*)tik->fake_sign, not_encrypted_marker, sizeof(tik->fake_sign)-1 );
    }
}

///////////////////////////////////////////////////////////////////////////////

bool ticket_is_marked_not_encrypted ( const wd_ticket_t * tik )
{
    ASSERT(tik);
    DASSERT( sizeof(not_encrypted_marker) < sizeof(tik->sig_padding));
    DASSERT( sizeof(not_encrypted_marker) < sizeof(tik->fake_sign));

    return !strncmp( (char*)tik->sig_padding, not_encrypted_marker, sizeof(tik->sig_padding) )
	&& !strncmp( (char*)tik->fake_sign, not_encrypted_marker, sizeof(tik->fake_sign) );
}

///////////////////////////////////////////////////////////////////////////////

u32 ticket_fake_sign ( wd_ticket_t * tik, u32 tik_size )
{
    ASSERT(tik);
    ticket_clear_encryption(tik,0);

    if (!tik_size)  // auto calculation
	tik_size = sizeof(wd_ticket_t);

    // fake signing

 #ifdef DEBUG
    TRACE("FAKESIGN: start brute force\n");
    //TRACE_HEXDUMP16(0,0,tik,tik_size);
 #endif

    u32 val = 0;
    u8 hash[WII_HASH_SIZE];
    do
    {
	memcpy(tik->fake_sign,&val,sizeof(val));
	SHA1( ((u8*)tik)+WII_TICKET_SIG_OFF, tik_size-WII_TICKET_SIG_OFF, hash );
	if (!*hash)
	    break;
	//TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
	val++;

    } while (val);

    TRACE("FAKESIGN: success, count=%u\n", val+1);
    return *hash ? 0 : val+1;
}

///////////////////////////////////////////////////////////////////////////////

bool ticket_is_fake_signed ( const wd_ticket_t * tik, u32 tik_size )
{
    ASSERT(tik);
   
    if (!tik_size)  // auto calculation
	tik_size = sizeof(wd_ticket_t);

    int i;
    for ( i = 0; i < sizeof(tik->sig); i++ )
	if (tik->sig[i])
	    return 0;
	    
    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)tik)+WII_TICKET_SIG_OFF, tik_size-WII_TICKET_SIG_OFF, hash );
    return !*hash;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct wd_tmd_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void tmd_setup ( wd_tmd_t * tmd, u32 tmd_size, const void * id4 )
{
    DASSERT(tmd);
    DASSERT(tmd_size>=sizeof(wd_tmd_t));
    
    memset(tmd,0,tmd_size);
    if ( tmd_size >= sizeof(wd_tmd_t) + sizeof(wd_tmd_content_t) )
    {
	tmd->n_content       = htons(1);
	tmd->content[0].type = htons(3);
	tmd->content[0].size = hton64(0xff7c0000);
    }

    tmd->sig_type = htonl(0x10001);
    strncpy((char*)tmd->issuer,"Root-CA00000001-CP00000004",sizeof(tmd->issuer));

    tmd->sys_version	= hton64(0x100000023ull);
    tmd->title_type	= htonl(1);
    tmd->group_id	= htons(0x3031);

    static u8 base_id[] = { 0,1,0,0, 0,0,0,0 };
    memcpy(tmd->title_id,base_id,8);
    id_setup(tmd->title_id+4,id4,4);
}

///////////////////////////////////////////////////////////////////////////////

void tmd_clear_encryption ( wd_tmd_t * tmd, int mark_not_encrypted )
{
    ASSERT(tmd);

    memset(tmd->sig,0,sizeof(tmd->sig));
    memset(tmd->sig_padding,0,sizeof(tmd->sig_padding));
    memset(tmd->fake_sign,0,sizeof(tmd->fake_sign));

    if (mark_not_encrypted)
    {
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tmd->sig_padding));
	ASSERT( sizeof(not_encrypted_marker) < sizeof(tmd->fake_sign));
	strncpy( (char*)tmd->sig_padding, not_encrypted_marker, sizeof(tmd->sig_padding)-1 );
	strncpy( (char*)tmd->fake_sign, not_encrypted_marker, sizeof(tmd->fake_sign)-1 );
    }
}

///////////////////////////////////////////////////////////////////////////////

bool tmd_is_marked_not_encrypted ( const wd_tmd_t * tmd )
{
    DASSERT( !tmd || sizeof(not_encrypted_marker) < sizeof(tmd->sig_padding));
    DASSERT( !tmd || sizeof(not_encrypted_marker) < sizeof(tmd->fake_sign));

    return tmd
	&& !strncmp( (char*)tmd->sig_padding, not_encrypted_marker, sizeof(tmd->sig_padding) )
	&& !strncmp( (char*)tmd->fake_sign, not_encrypted_marker, sizeof(tmd->fake_sign) );
}

///////////////////////////////////////////////////////////////////////////////

u32 tmd_fake_sign ( wd_tmd_t * tmd, u32 tmd_size )
{
    ASSERT(tmd);
    tmd_clear_encryption(tmd,0);

    if (!tmd_size)  // auto calculation
	tmd_size = sizeof(wd_tmd_t) + tmd->n_content * sizeof(wd_tmd_content_t);

    // fake signing

 #ifdef DEBUG
    TRACE("FAKESIGN: start brute force\n");
    //TRACE_HEXDUMP16(0,0,tmd,tmd_size);
 #endif

    u32 val = 0;
    u8 hash[WII_HASH_SIZE];
    do
    {
	memcpy(tmd->fake_sign,&val,sizeof(val));
	SHA1( ((u8*)tmd)+WII_TMD_SIG_OFF, tmd_size-WII_TMD_SIG_OFF, hash );
	if (!*hash)
	    break;
	//TRACE_HEXDUMP(0,0,1,WII_HASH_SIZE,hash,WII_HASH_SIZE);
	val++;

    } while (val);

    TRACE("FAKESIGN: success, count=%u\n", val+1);
    return *hash ? 0 : val+1;
}

///////////////////////////////////////////////////////////////////////////////

bool tmd_is_fake_signed ( const wd_tmd_t * tmd, u32 tmd_size )
{
    if (!tmd)
	return false;
   
    if (!tmd_size)  // auto calculation
	tmd_size = sizeof(wd_tmd_t) + tmd->n_content * sizeof(wd_tmd_content_t);

    int i;
    for ( i = 0; i < sizeof(tmd->sig); i++ )
	if (tmd->sig[i])
	    return 0;
	    
    u8 hash[WII_HASH_SIZE];
    SHA1( ((u8*)tmd)+WII_TMD_SIG_OFF, tmd_size-WII_TMD_SIG_OFF, hash );
    return !*hash;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    struct wd_part_control_t		///////////////
///////////////////////////////////////////////////////////////////////////////

static int setup_part_control_helper ( wd_part_control_t * pc )
{
    // tmd, tmd_size, cert, cert_size and h3 must 0 or be valid

    ASSERT(pc);
    TRACE("setup_part_control_helper(%p)\n",pc);

    TRACE(" - PART: %p : %8x + %9zx\n", pc->part_bin, 0, sizeof(pc->part_bin) );

    // setup head

    wd_part_header_t * head = (wd_part_header_t*)pc->part_bin;
    pc->head		= head;
    pc->head_size	= sizeof(wd_part_header_t);
    TRACE(" - HEAD: %p : %8x + %9x\n", pc->head, 0, pc->head_size );

    u8* head_end = pc->part_bin + ALIGN32(sizeof(wd_part_header_t),4);
    u8* part_end = pc->part_bin + sizeof(pc->part_bin);

    // setup tmd

    u8* ptr		= pc->tmd
				? (u8*) pc->tmd
				: pc->part_bin + ALIGN32(sizeof(wd_part_header_t),32);
    pc->tmd		= (wd_tmd_t*)ptr;
    head->tmd_size	= htonl(pc->tmd_size);
    head->tmd_off4	= htonl(ptr-pc->part_bin>>2);
    TRACE(" - TMD:  %p : %8x + %9x\n", pc->tmd, ntohl(head->tmd_off4)<<2, pc->tmd_size );

    // setup tmd_content

    const u32 n_content = pc->tmd_size >= sizeof(wd_tmd_t)
			? ( pc->tmd_size - sizeof(wd_tmd_t) ) / sizeof(wd_tmd_content_t)
			: 0;
    if ( !pc->tmd->n_content || pc->tmd->n_content > n_content )
	pc->tmd->n_content = n_content;

    pc->tmd_content	= n_content ? pc->tmd->content : 0;
    
    TRACE(" - CON0: %p : %8zx + %9zx [N=%u/%u]\n",
		pc->tmd_content,
		(ntohl(head->tmd_off4)<<2) + sizeof(wd_tmd_t),
		sizeof(wd_tmd_content_t), pc->tmd->n_content, n_content );

    // setup cert

    ptr			= pc->cert
				? (u8*) pc->cert
				: (u8*) pc->tmd + ALIGN32(pc->tmd_size,32);
    pc->cert		= ptr;
    head->cert_size	= htonl(pc->cert_size);
    head->cert_off4	= htonl(ptr-pc->part_bin>>2);
    TRACE(" - CERT: %p : %8x + %9x\n", pc->cert, ntohl(head->cert_off4)<<2, pc->cert_size );

    // setup h3

    if (!pc->h3)
	pc->h3		= pc->part_bin + sizeof(pc->part_bin) - WII_H3_SIZE;
    pc->h3_size		= WII_H3_SIZE;
    head->h3_off4	= htonl(pc->h3-pc->part_bin>>2);
    TRACE(" - H3:   %p : %8x + %9x\n", pc->h3, ntohl(head->h3_off4)<<2, WII_H3_SIZE );

    // setup data

    if (!pc->data_off)
	pc->data_off	= sizeof(pc->part_bin);
    head->data_off4	= htonl(pc->data_off>>2);
    head->data_size4	= htonl(pc->data_size>>2);
    TRACE(" - DATA: %p : %8x + %9llx\n",
		pc->part_bin + sizeof(pc->part_bin),
		ntohl(head->data_off4)<<2, pc->data_size );

    // validation check

    pc->is_valid = 0;

    if ( pc->tmd_size < WII_TMD_GOOD_SIZE
	|| pc->cert_size < 0x10
	|| pc->data_off < sizeof(pc->part_bin)
	|| pc->data_size > WII_MAX_PART_SECTORS * (u64)WII_SECTOR_SIZE
    )
	return 1;

    // check that all data is within part_bin
    
    u8 * tmd_beg  = (u8*)pc->tmd;
    u8 * tmd_end  = tmd_beg + pc->tmd_size;
    u8 * cert_beg = (u8*)pc->cert;
    u8 * cert_end = cert_beg + pc->cert_size;
    u8 * h3_beg   = (u8*)pc->h3;
    u8 * h3_end   = h3_beg   + pc->h3_size;

    if (   tmd_beg  < head_end || tmd_end  > part_end
	|| cert_beg < head_end || cert_end > part_end
	|| h3_beg   < head_end || h3_end   > part_end
    )
	return 1;

    // check overlays

    if (   tmd_end  >= cert_beg && tmd_beg  < cert_end	// overlay tmd  <-> cert
	|| tmd_end  >= h3_beg   && tmd_beg  < h3_end	// overlay tmd  <-> h3
	|| cert_end >= h3_beg   && cert_beg < h3_end	// overlay cert <-> h3
    )
	return 1;

    // all seems ok

    pc->is_valid = 1;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int clear_part_control
	( wd_part_control_t * pc, u32 tmd_size, u32 cert_size, u64 data_size )
{
    ASSERT(pc);
    TRACE("clear_part_control(%p,%x,%x,%llx)\n",pc,tmd_size,cert_size,data_size);

    memset(pc,0,sizeof(*pc));

    pc->tmd_size	= tmd_size;
    pc->cert_size	= cert_size;
    pc->data_size	= ALIGN64(data_size,WII_SECTOR_SIZE);

    return setup_part_control_helper(pc);
}

///////////////////////////////////////////////////////////////////////////////

int setup_part_control ( wd_part_control_t * pc )
{
    ASSERT(pc);
    TRACE("setup_part_control(%p)\n",pc);

    // clear controling data
    memset( pc->part_bin+sizeof(pc->part_bin), 0, +sizeof(*pc)-sizeof(pc->part_bin) );
 
    wd_part_header_t * head = (wd_part_header_t*)pc->part_bin;

    pc->tmd		= (wd_tmd_t*)
			( pc->part_bin + ( ntohl(head->tmd_off4)   << 2 ));
    pc->tmd_size	=		   ntohl(head->tmd_size);
    pc->cert		= pc->part_bin + ( ntohl(head->cert_off4)  << 2 );
    pc->cert_size	=		   ntohl(head->cert_size);
    pc->h3		= pc->part_bin + ( ntohl(head->h3_off4)    << 2 );
    pc->data_off	=	      (u64)ntohl(head->data_off4)  << 2;
    pc->data_size	=	      (u64)ntohl(head->data_size4) << 2;

    return setup_part_control_helper(pc);
}

///////////////////////////////////////////////////////////////////////////////

u32 part_control_fake_sign ( wd_part_control_t * pc, int calc_h4 )
{
    ASSERT(pc);

    u32 stat = 0;
    if (pc->is_valid)
    {
	// caluclate SHA1 hash 'h4'
	if ( calc_h4 && pc->tmd_content )
	    SHA1( pc->h3, pc->h3_size, pc->tmd_content->hash );

	// fake signing
	const u32 stat1 = tmd_fake_sign(pc->tmd,pc->tmd_size);
	const u32 stat2 = ticket_fake_sign(&pc->head->ticket,0);
	stat = stat1 && stat2 ? stat + stat2 : 0;
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int part_control_is_fake_signed ( const wd_part_control_t * pc )
{
    ASSERT(pc);
    return pc->is_valid && pc->tmd && tmd_is_fake_signed(pc->tmd,pc->tmd_size);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wit patch files			///////////////
///////////////////////////////////////////////////////////////////////////////

const char wpat_magic[12] = WIT_PATCH_MAGIC;

///////////////////////////////////////////////////////////////////////////////

ccp wpat_get_type_name ( wpat_type_t type, ccp return_if_invalid )
{
    static ccp tab[] =
    {
	0,
	"TOC:HEADER",		// WPAT_HEADER
	"TOC:COMMENT",		// WPAT_COMMENT
	"TOC:DATA",		// WPAT_DATA
	"TOC:DELETE-FILE",	// WPAT_DELETE_FILE
	"TOC:CREATE-FILE",	// WPAT_CREATE_FILE
	"TOC:MOVE-FILE",	// WPAT_MOVE_FILE
	"TOC:COPY-FILE",	// WPAT_COPY_FILE
	"TOC:LINK-FILE",	// WPAT_LINK_FILE
	"TOC:PATCH-FILE",	// WPAT_PATCH_FILE
    };

    const unsigned id = type &= WPAT_M_ID;
    ccp res = id < sizeof(tab)/sizeof(*tab) ? tab[id] : 0;

    return !res
	    ? return_if_invalid
	    : type & WPAT_F_TOC
		? res
		: res + 4;
}

///////////////////////////////////////////////////////////////////////////////

u32 wpat_get_size ( wpat_size_t type_size )
{
    return ( ntohl(type_size.size4) & 0xffffff ) << 2;
}

///////////////////////////////////////////////////////////////////////////////

wpat_size_t wpat_calc_size ( wpat_type_t type, u32 size )
{
    wpat_size_t type_size;
    type_size.size4 = htonl( size + 3 >> 2 );
    type_size.type  = type;
    return type_size;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			consts & vars			///////////////
///////////////////////////////////////////////////////////////////////////////

const char skeleton_marker[10] = "[SKELETON]";

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

