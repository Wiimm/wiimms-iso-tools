
/***************************************************************************
 *                                                                         *
 *                     _____     ____                                      *
 *                    |  __ \   / __ \   _     _ _____                     *
 *                    | |  \ \ / /  \_\ | |   | |  _  \                    *
 *                    | |   \ \| |      | |   | | |_| |                    *
 *                    | |   | || |      | |   | |  ___/                    *
 *                    | |   / /| |   __ | |   | |  _  \                    *
 *                    | |__/ / \ \__/ / | |___| | |_| |                    *
 *                    |_____/   \____/  |_____|_|_____/                    *
 *                                                                         *
 *                       Wiimms source code library                        *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *        Copyright (c) 2012-2022 by Dirk Clemens <wiimm@wiimm.de>         *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE 1

#include <string.h>
#include <unistd.h>
#include <stddef.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>

#include "dclib-basics.h"
#include "dclib-debug.h"
#include "dclib-utf8.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////				bits			///////////////
///////////////////////////////////////////////////////////////////////////////

uint Count1Bits ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    uint count = 0;
    const uchar *ptr = addr;
    while ( size-- > 0 )
	count += TableBitCount[*ptr++];
    return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FindLowest0BitLE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr;
    uint idx;
    for ( idx = 0; idx < size; idx++ )
    {
	const int val = TableLowest0Bit[*ptr++];
	if ( val >= 0 )
	    return 8*idx + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int FindLowest1BitLE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr;
    uint idx;
    for ( idx = 0; idx < size; idx++ )
    {
	const int val = TableLowest1Bit[*ptr++];
	if ( val >= 0 )
	    return 8*idx + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int FindHighest0BitLE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr + size;
    while ( size-- > 0 )
    {
	const int val = TableHighest0Bit[*--ptr];
	if ( val >= 0 )
	    return 8*size + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int FindHighest1BitLE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr + size;
    while ( size-- > 0 )
    {
	const int val = TableHighest1Bit[*--ptr];
	if ( val >= 0 )
	    return 8*size + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u32 GetAlign32 ( u32 data )
{
    const int index = FindLowest1Bit(&data,sizeof(data));
    return index < 0 ? 0 : 1 << index;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetAlign64 ( u64 data )
{
    const int index = FindLowest1Bit(&data,sizeof(data));
    return index < 0 ? 0 : 1llu << index;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int FindLowest0BitBE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr + size;
    uint idx;
    for ( idx = 0; idx < size; idx++ )
    {
	const int val = TableLowest0Bit[*--ptr];
	if ( val >= 0 )
	    return 8*idx + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int FindLowest1BitBE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr + size;
    uint idx;
    for ( idx = 0; idx < size; idx++ )
    {
	const int val = TableLowest1Bit[*--ptr];
	if ( val >= 0 )
	    return 8*idx + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int FindHighest0BitBE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr;
    while ( size-- > 0 )
    {
	const int val = TableHighest0Bit[*ptr++];
	if ( val >= 0 )
	    return 8*size + val;
    }
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int FindHighest1BitBE ( cvp addr, uint size )
{
    DASSERT( CHAR_BIT == 8 );

    const uchar *ptr = addr;
    while ( size-- > 0 )
    {
	const int val = TableHighest1Bit[*ptr++];
	if ( val >= 0 )
	    return 8*size + val;
    }
    return -1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			bit manipulation		///////////////
///////////////////////////////////////////////////////////////////////////////

bool TestSetBit ( void *addr, uint bit_num )
{
    const u8 mask = 1 << ( bit_num & 7 );
    u8 *ptr = ((u8*)addr) + ( bit_num >> 3 );
    const bool result = ( *ptr & mask ) != 0;
    *ptr |= mask;
    return result;
}

///////////////////////////////////////////////////////////////////////////////

bool TestClearBit  ( void *addr, uint bit_num )
{
    const u8 mask = 1 << ( bit_num & 7 );
    u8 *ptr = ((u8*)addr) + ( bit_num >> 3 );
    const bool result = ( *ptr & mask ) != 0;
    *ptr &= ~mask;
    return result;
}

///////////////////////////////////////////////////////////////////////////////

bool TestInvertBit ( void *addr, uint bit_num )
{
    const u8 mask = 1 << ( bit_num & 7 );
    u8 *ptr = ((u8*)addr) + ( bit_num >> 3 );
    const bool result = ( *ptr & mask ) != 0;
    *ptr ^= mask;
    return result;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetBits ( void *addr, uint beg_bitnum, uint end_bitnum )
{
    static uchar tab1[] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 };
    static uchar tab2[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

    if ( beg_bitnum < end_bitnum )
    {
	const uint lo8 =   beg_bitnum >> 3;
	const uint hi8 = --end_bitnum >> 3;

	if ( lo8 < hi8 )
	{

	    uchar * dest = (uchar*)addr + lo8;
	    *dest++ |= tab1[ beg_bitnum & 7 ];

	    uchar * end = (uchar*)addr + hi8;
	    *end |= tab2[ end_bitnum & 7 ];

	    while ( dest < end )
		*dest++ = 0xff;
	}
	else
	{
	    uchar * dest = (uchar*)addr + lo8;
	    *dest |= tab1[ beg_bitnum & 7 ] & tab2[ end_bitnum & 7 ];
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClearBits ( void *addr, uint beg_bitnum, uint end_bitnum )
{
    static uchar tab1[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f };
    static uchar tab2[] = { 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00 };

    if ( beg_bitnum < end_bitnum )
    {
	const uint lo8 =   beg_bitnum >> 3;
	const uint hi8 = --end_bitnum  >> 3;

	if ( lo8 < hi8 )
	{

	    uchar * dest = (uchar*)addr + lo8;
	    *dest++ &= tab1[ beg_bitnum & 7 ];

	    uchar * end = (uchar*)addr + hi8;
	    *end &= tab2[ end_bitnum & 7 ];

	    while ( dest < end )
		*dest++ = 0;
	}
	else
	{
	    uchar * dest = (uchar*)addr + lo8;
	    *dest &= tab1[ beg_bitnum & 7 ] | tab2[ end_bitnum & 7 ];
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InvertBits ( void *addr, uint beg_bitnum, uint end_bitnum )
{
    static uchar tab1[] = { 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80 };
    static uchar tab2[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };

    if ( beg_bitnum < end_bitnum )
    {
	const uint lo8 =   beg_bitnum >> 3;
	const uint hi8 = --end_bitnum >> 3;

	if ( lo8 < hi8 )
	{

	    uchar * dest = (uchar*)addr + lo8;
	    *dest++ ^= tab1[ beg_bitnum & 7 ];

	    uchar * end = (uchar*)addr + hi8;
	    *end ^= tab2[ end_bitnum & 7 ];

	    while ( dest < end )
		*dest++ ^= 0xff;
	}
	else
	{
	    uchar * dest = (uchar*)addr + lo8;
	    *dest ^= tab1[ beg_bitnum & 7 ] & tab2[ end_bitnum & 7 ];
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#undef GET_NEXT
#define GET_NEXT ch = *src++; \
		if ( ++min_idx > max_idx ) goto term; \
		if ( min_idx == max_idx ) ch &= max_mask

int PrintBitfieldEx
(
    // returns the number of printed characters, or -1 if buffer is to small

    char	*buf,		// buffer
    uint	buf_size,	// size of 'buf'
    cvp		bit_field,	// address of bit field
    uint	min_idx,	// minimal index (inclusive) to check
    uint	max_idx,	// maximal index (inclusive) to check
    bool	use_ranges,	// true: create ranges 'a:b'
    int		mult,		// each printed index is first multiplyed by 'mult'
    int		add		// ... and then added to 'add'.
)
{
    DASSERT(buf);
    DASSERT(buf_size);
    DASSERT(bit_field);

    char *dest = buf;
    char *end  = buf + buf_size;
    char *sep = "";

    const u8 *src = (u8*)bit_field + (min_idx >> 3);
    u8 ch = *src++; // always preload the current byte
    if ( min_idx & 7 )
	ch &= ~((1<<(min_idx&7))-1);
    min_idx >>= 3;
    const u8 max_mask = (2<<(max_idx&7))-1;
    max_idx >>= 3;
    noPRINT("SETUP[%zx] %02x, %u..%u, max_mask=%02x\n",
	src-(u8*)bit_field-1, ch, min_idx, max_idx, max_mask );

    while ( min_idx <= max_idx )
    {
	//--- first skip bytes with value 0

	if (!ch)
	    do { GET_NEXT; } while (!ch);


	//-- now find the first set bit

	DASSERT(ch);
	noPRINT("CHECK[%zx] %02x, %u..%u\n", src-(u8*)bit_field-1, ch, min_idx, max_idx);
	u8 mask = 1;
	uint bit_idx = 0;
	while ( !(ch & mask) )
	    mask <<= 1, bit_idx++;

	uint first = 8*min_idx + bit_idx;
	noPRINT("FOUND-BEG[%zx.%u=%u] %02x, %u..%u\n",
		src-(u8*)bit_field-1, bit_idx, first, ch, min_idx, max_idx );


	//-- now find the first bit not set

	while ( ch & mask )
	    mask <<= 1, bit_idx++;

	if ( bit_idx == 8 )
	{
	    do { GET_NEXT; } while (ch == 0xff);

	    mask = 1;
	    bit_idx = 0;
	    while ( ch & mask )
		mask <<= 1, bit_idx++;
	}

	uint last = 8*min_idx + bit_idx - 1;
	noPRINT("FOUND-END[%zx.%u=%u] %02x, %u..%u\n",
		src-(u8*)bit_field-1, bit_idx, last, ch, min_idx, max_idx );

	if ( first == last )
	    dest = snprintfE(dest,end,"%s%d",sep,first*mult+add);
	else if ( use_ranges )
	    dest = snprintfE(dest,end,"%s%d:%d",
			sep,
			first * mult + add,
			last * mult + add );
	else
	    while ( first <= last )
	    {
		dest = snprintfE(dest,end,"%s%d",sep,first++*mult+add);
		sep = ",";
	    }

	if ( dest >= end-1 )
	{
	    *dest = 0;
	    return -1;
	}

	sep = ",";
	ch &= ~((1<<bit_idx)-1);
    }

 term:
    *dest = 0;
    return dest - buf;
}


//
///////////////////////////////////////////////////////////////////////////////
///////////////		low level endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

void write_be16 ( void * be_data_ptr, u16 data )
{
    DASSERT(be_data_ptr);
    u8 * dest = be_data_ptr;
    *dest++ = data >> 8;
    *dest   = data;
}

void write_be24 ( void * be_data_ptr, u32 data )
{
    DASSERT(be_data_ptr);
    u8 * dest = be_data_ptr + 2;
    *dest-- = data; data >>= 8;
    *dest-- = data;
    *dest   = data >> 8;
}

void write_be32 ( void * be_data_ptr, u32 data )
{
    DASSERT(be_data_ptr);
    u8 * dest = be_data_ptr + 3;
    *dest-- = data; data >>= 8;
    *dest-- = data; data >>= 8;
    *dest-- = data;
    *dest   = data >> 8;
}

void write_be40 ( void * be_data_ptr, u64 data )
{
    DASSERT(be_data_ptr);
    *(u8*)be_data_ptr = data>>32;
    write_be32( (u8*)be_data_ptr+1, data );
}

void write_be48 ( void * be_data_ptr, u64 data )
{
    DASSERT(be_data_ptr);
    write_be16( be_data_ptr, data>>32 );
    write_be32( (u8*)be_data_ptr+2, data );
}

void write_be56 ( void * be_data_ptr, u64 data )
{
    DASSERT(be_data_ptr);
    write_be24( be_data_ptr, data>>32 );
    write_be32( (u8*)be_data_ptr+3, data );
}

void write_be64 ( void * be_data_ptr, u64 data )
{
    DASSERT(be_data_ptr);
    write_be32( be_data_ptr, data >> 32 );
    write_be32( (u8*)be_data_ptr+4, data );
}

void write_bef4 ( void * be_data_ptr, float data )
{
    // assume: local system supports IEEE 754

    write_be32(be_data_ptr,*(u32*)&data);
}

void write_bef8 ( void * be_data_ptr, double data )
{
    // assume: local system supports IEEE 754

    write_be64(be_data_ptr,*(u64*)&data);
}

///////////////////////////////////////////////////////////////////////////////

void write_le16 ( void * le_data_ptr, u16 data )
{
    DASSERT(le_data_ptr);
    u8 * dest = le_data_ptr;
    *dest++ = data;
    *dest   = data >> 8;
}

void write_le24 ( void * le_data_ptr, u32 data )
{
    DASSERT(le_data_ptr);
    u8 * dest = le_data_ptr;
    *dest++ = data; data >>= 8;
    *dest++ = data;
    *dest   = data >> 8;
}

void write_le32 ( void * le_data_ptr, u32 data )
{
    DASSERT(le_data_ptr);
    u8 * dest = le_data_ptr;
    *dest++ = data; data >>= 8;
    *dest++ = data; data >>= 8;
    *dest++ = data;
    *dest   = data >> 8;
}

void write_le40 ( void * le_data_ptr, u64 data )
{
    DASSERT(le_data_ptr);
    *(u8*)le_data_ptr = (u8)data;
    write_le32( (u8*)le_data_ptr+1, data >> 8 );
}

void write_le48 ( void * le_data_ptr, u64 data )
{
    DASSERT(le_data_ptr);
    write_le16( le_data_ptr, data );
    write_le32( (u8*)le_data_ptr+2, data >> 16 );
}

void write_le56 ( void * le_data_ptr, u64 data )
{
    DASSERT(le_data_ptr);
    write_le32( le_data_ptr, data );
    write_le24( (u8*)le_data_ptr+4, data >> 32 );
}

void write_le64 ( void * le_data_ptr, u64 data )
{
    DASSERT(le_data_ptr);
    write_le32( le_data_ptr, data );
    write_le32( (u8*)le_data_ptr+4, data >> 32 );
}

void write_lef4 ( void * le_data_ptr, float data )
{
    // assume: local system supports IEEE 754

    write_le32(le_data_ptr,*(u32*)&data);
}

void write_lef8 ( void * le_data_ptr, double data )
{
    // assume: local system supports IEEE 754

    write_le64(le_data_ptr,*(u64*)&data);
}

///////////////////////////////////////////////////////////////////////////////
// convert lists

void be16n ( u16 * dest, const u16 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	*dest++ = be16(src++);
}

void be32n ( u32 * dest, const u32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	*dest++ = be32(src++);
}

void bef4n ( float32 * dest, const float32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	*dest++ = bef4(src++);
}

void write_be16n ( u16 * dest, const u16 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	write_be16(dest++,*src++);
}

void write_be32n ( u32 * dest, const u32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	write_be32(dest++,*src++);
}

void write_bef4n ( float32 * dest, const float32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	write_bef4(dest++,*src++);
}

///////////////////////////////////////////////////////////////////////////////
// convert lists

void le16n ( u16 * dest, const u16 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	*dest++ = le16(src++);
}

void le32n ( u32 * dest, const u32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	*dest++ = le32(src++);
}

void lef4n ( float32 * dest, const float32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	*dest++ = lef4(src++);
}

void write_le16n ( u16 * dest, const u16 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	write_le16(dest++,*src++);
}

void write_le32n ( u32 * dest, const u32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	write_le32(dest++,*src++);
}

void write_lef4n ( float32 * dest, const float32 * src, int n )
{
    DASSERT( dest );
    DASSERT( n >= 0 );
    DASSERT( src || !n );

    while ( n-- > 0 )
	write_lef4(dest++,*src++);
}

///////////////////////////////////////////////////////////////////////////////

static inline u16 ident16 ( u16 data ) { return data; }
static inline u32 ident32 ( u32 data ) { return data; }
static inline u64 ident64 ( u64 data ) { return data; }

static inline u16 reverse16 ( u16 data ) { return swap16(&data); }
static inline u32 reverse32 ( u32 data ) { return swap32(&data); }
static inline u64 reverse64 ( u64 data ) { return swap64(&data); }

const endian_func_t be_func =
{
    {0xfe,0xff}, true, false, DC_BIG_ENDIAN,
    be16, be24, be32, be40, be48, be56, be64, bef4, bef8,
    write_be16, write_be24, write_be32,
    write_be40, write_be48, write_be56, write_be64,
    write_bef4, write_bef8,

 #if IS_BIG_ENDIAN
    ident16, ident32, ident64,
    ident16, ident32, ident64,
 #else
    reverse16, reverse32, reverse64,
    reverse16, reverse32, reverse64,
 #endif
};

const endian_func_t le_func =
{
    {0xff,0xfe}, false, true, DC_LITTLE_ENDIAN,
    le16, le24, le32, le40, le48, le56, le64, lef4, lef8,
    write_le16, write_le24, write_le32,
    write_le40, write_le48, write_le56, write_le64,
    write_lef4, write_lef8,

 #if IS_LITTLE_ENDIAN
    ident16, ident32, ident64,
    ident16, ident32, ident64,
 #else
    reverse16, reverse32, reverse64,
    reverse16, reverse32, reverse64,
 #endif
};

const endian_func_t * GetEndianFunc ( const void * byte_order_mark )
{
    DASSERT(byte_order_mark);
    const u8 * bom = byte_order_mark;
    return bom[0] == 0xfe && bom[1] == 0xff
		? &be_func
		: bom[0] == 0xff && bom[1] == 0xfe
			? &le_func
			: 0;
}

///////////////////////////////////////////////////////////////////////////////

uint GetTextBOMLen ( const void * data, uint data_size )
{
    static const u8 be_bom[] = { 0xef, 0xbb, 0xbf };
    static const u8 le_bom[] = { 0xef, 0xbf, 0xbe };

    if ( data_size >= 3 && ( !memcmp(data,be_bom,3) || !memcmp(data,le_bom,3) ))
	return 3;

    if ( data_size >= 2 && ( *(u16*)data == 0xfeff || *(u16*)data == 0xfffe ))
	return 2;

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			check index			///////////////
///////////////////////////////////////////////////////////////////////////////

int CheckIndex1 ( int max, int index )
{
    DASSERT( max >= 0 );

    if ( index >= 0 )
	return index <= max ? index : max;

    index += max;
    return index >= 0 ? index : 0;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIndex1End ( int max, int index )
{
    DASSERT( max >= 0 );
    if ( index > 0 )
	return index <= max ? index : max;

    index += max;
    return index >= 0 ? index : 0;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIndex2 ( int max, int * p_begin, int * p_end )
{
    DASSERT( max >= 0 );
    DASSERT(p_begin);
    DASSERT(p_end);

    int begin = *p_begin;
    if ( begin < 0 )
    {
	begin += max;
	if ( begin < 0 )
	    begin = 0;
    }
    else if ( begin > max )
	begin = max;

    int end = *p_end;
    if ( end < 0 )
    {
	end += max;
	// don't check for 'end<0' here, because 'end<begin' is checked later
    }
    else if ( end > max )
	end = max;

    if ( end < begin )
	end = begin;

    *p_begin = begin;
    *p_end   = end;
    return end - begin;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIndex2End ( int max, int * p_begin, int * p_end )
{
    DASSERT( max >= 0 );
    DASSERT(p_begin);
    DASSERT(p_end);

    int begin = *p_begin;
    if ( begin <= 0 )
    {
	begin += max;
	if ( begin < 0 )
	    begin = 0;
    }
    else if ( begin > max )
	begin = max;

    int end = *p_end;
    if ( end <= 0 )
    {
	end += max;
	// don't check for 'end<0' here, because 'end<begin' is checked later
    }
    else if ( end > max )
	end = max;

    if ( end < begin )
	end = begin;

    *p_begin = begin;
    *p_end   = end;
    return end - begin;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIndexC ( int max, int * p_begin, int count )
{
    DASSERT( max >= 0 );
    DASSERT(p_begin);

    int begin = *p_begin, end = begin + count;
    if ( begin < 0 )
    {
	begin += max;
	end = begin + count;
	if ( begin < 0 )
	    begin = 0;
    }
    else if ( begin > max )
	begin = max;

    if ( end < 0 )
	end = 0;
    else if ( end > max )
	end = max;

    if ( end < begin )
    {
	*p_begin = end;
	return begin-end;
    }

    *p_begin = begin;
    return end - begin;
}

///////////////////////////////////////////////////////////////////////////////

int CheckIndexCEnd ( int max, int * p_begin, int count )
{
    DASSERT( max >= 0 );
    DASSERT(p_begin);

    int begin = *p_begin, end = begin + count;
    if ( begin <= 0 )
    {
	begin += max;
	end = begin + count;
	if ( begin < 0 )
	    begin = 0;
    }
    else if ( begin > max )
	begin = max;

    if ( end < 0 )
	end = 0;
    else if ( end > max )
	end = max;

    if ( end < begin )
    {
	*p_begin = end;
	return begin-end;
    }

    *p_begin = begin;
    return end - begin;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    lt/eq/gt			///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanCompareOp ( compare_operator_t *dest_op, ccp arg )
{
    compare_operator_t op = COP_FALSE;
    ccp ptr = arg;
    if (ptr)
    {
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	if ( (uchar)ptr[0] == 0xe2 && (uchar)ptr[1] == 0x89 )
	{
	    switch ((uchar)ptr[2])
	    {
		case 0xa0: op = COP_NE; ptr += 3; break; // UTF8 ≠
		case 0xa4: op = COP_LE; ptr += 3; break; // UTF8 ≤
		case 0xa5: op = COP_GE; ptr += 3; break; // UTF8 ≥
	    }
	}
	else if ( *ptr == '!' )
	{
	    if ( *++ptr == '=' )
	    {
		op = COP_NE;
		ptr++;
	    }
	}
	else
	    switch (*ptr)
	    {
	     case '=':
		op = COP_EQ;
		if ( *++ptr == '=' )
		    ptr++;
		break;

	     case '<':
		op = COP_LT;
		switch (*++ptr)
		{
		 case '=':
		    op = COP_LE;
		    ptr++;
		    break;
		 case '>':
		    op = COP_NE;
		    ptr++;
		    break;
		}
		break;

	     case '>':
		op = COP_GT;
		if ( *++ptr == '=' )
		{
		    op = COP_GE;
		    ptr++;
		}
		break;
	    }
    }

    if (dest_op)
	*dest_op = op;
    return op == COP_FALSE ? (char*)arg : (char*)ptr;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetCompareOpName ( compare_operator_t op )
{
    switch (op)
    {
	case COP_EQ:	return "==";
	case COP_LT:	return "<";
	case COP_GT:	return ">";
	case COP_NE:	return "!=";
	case COP_LE:	return "<=";
	case COP_GE:	return ">=";
	case COP_TRUE:	return "*";
	default:	return "%";
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpStat ( int stat, compare_operator_t op )
{
    switch (op)
    {
	case COP_EQ:	return stat == 0;
	case COP_LT:	return stat <  0;
	case COP_GT:	return stat >  0;
	case COP_NE:	return stat != 0;
	case COP_LE:	return stat <= 0;
	case COP_GE:	return stat >= 0;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpINT ( int a, compare_operator_t op, int b )
{
    switch (op)
    {
	case COP_EQ:	return a == b;
	case COP_LT:	return a <  b;
	case COP_GT:	return a >  b;
	case COP_NE:	return a != b;
	case COP_LE:	return a <= b;
	case COP_GE:	return a >= b;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpUINT ( uint a, compare_operator_t op, uint b )
{
    switch (op)
    {
	case COP_EQ:	return a == b;
	case COP_LT:	return a <  b;
	case COP_GT:	return a >  b;
	case COP_NE:	return a != b;
	case COP_LE:	return a <= b;
	case COP_GE:	return a >= b;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpI64 ( s64 a, compare_operator_t op, s64 b )
{
    switch (op)
    {
	case COP_EQ:	return a == b;
	case COP_LT:	return a <  b;
	case COP_GT:	return a >  b;
	case COP_NE:	return a != b;
	case COP_LE:	return a <= b;
	case COP_GE:	return a >= b;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpU64 ( u64 a, compare_operator_t op, u64 b )
{
    switch (op)
    {
	case COP_EQ:	return a == b;
	case COP_LT:	return a <  b;
	case COP_GT:	return a >  b;
	case COP_NE:	return a != b;
	case COP_LE:	return a <= b;
	case COP_GE:	return a >= b;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpFLT ( float a, compare_operator_t op, float b )
{
    switch (op)
    {
	case COP_EQ:	return a == b;
	case COP_LT:	return a <  b;
	case COP_GT:	return a >  b;
	case COP_NE:	return a != b;
	case COP_LE:	return a <= b;
	case COP_GE:	return a >= b;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool CompareByOpDBL ( double a, compare_operator_t op, double b )
{
    switch (op)
    {
	case COP_EQ:	return a == b;
	case COP_LT:	return a <  b;
	case COP_GT:	return a >  b;
	case COP_NE:	return a != b;
	case COP_LE:	return a <= b;
	case COP_GE:	return a >= b;
	case COP_TRUE:	return true;
	default:	return false;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			encoding/decoding		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetEncodingName ( EncodeMode_t em )
{
    static const char tab[][7] =
    {
	"OFF",
	"ANSI",
	"UTF8",
	"BASE64",
	"URL64",
	"STAR64",
	"XML64",
	"JSON",
    };

    return (uint)em < sizeof(tab)/sizeof(*tab) ? tab[em] : PrintCircBuf("%d",em);
}

///////////////////////////////////////////////////////////////////////////////

uint GetEscapeLen
(
    // returns the extra size needed for escapes.
    // Add 'src_len' to get the escaped string size.
    // Add 'additionally 4 to get a good buffer size.

    ccp		source,		// NULL or string to print
    int		src_len,	// length of string. if -1, str is null terminated
    CharMode_t	char_mode,	// modes, bit field (CHMD_*)
    char	quote		// NULL or quotation char, that must be quoted
)
{
    const CharMode_t utf8	= char_mode & CHMD_UTF8;
    const int esc_size		= char_mode & CHMD_ESC  ? 1 : 3;
    const int pipe_size		= char_mode & CHMD_PIPE ? 1 : 0;

    if ( !source || !*source )
	return 0;
    ccp str = source;
    ccp end = src_len < 0 ? 0 : str + src_len;

    uint size = 0;
    while ( !end || str < end )
    {
	const u8 ch = (u8)*str++;
	switch (ch)
	{
	    case 0:
		if (!end)
		    return size;
		size += 3;
		break;

	    case '\\':
	    case '\a':
	    case '\b':
	    case '\f':
	    case '\n':
	    case '\r':
	    case '\t':
	    case '\v':
		size++;
		break;

	    case '\033':
		size += esc_size;
		break;

	    case '|':
		size += pipe_size;
		break;

	    default:
		if ( ch == quote )
		    size++;
		else if ( ch < ' ' || !utf8 && (ch&0x7f) < ' ' || (ch&0x7f) == 0x7f )
		    size += 3;
	}
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintEscapedString
(
    // returns 'buf'

    char	*buf,		// valid destination buffer
    uint	buf_size,	// size of 'buf', >= 10
    ccp		source,		// NULL or string to print
    int		len,		// length of string. if -1, str is null terminated
    CharMode_t	char_mode,	// modes, bit field (CHMD_*)
    char	quote,		// NULL or quotation char, that must be quoted
    uint	*dest_len	// not NULL: Store length of result here
)
{
    ///////////////////////////////////////////////////////
    /////  Update GetEscapeLen() on modifications!!   /////
    ///////////////////////////////////////////////////////

    DASSERT(buf);
    DASSERT(buf_size>=10);

    const CharMode_t utf8	= char_mode & CHMD_UTF8;
    const CharMode_t allow_e	= char_mode & CHMD_ESC;
    const CharMode_t esc_pipe	= char_mode & CHMD_PIPE;

    char *dest = buf;
    char *dest_end = dest + buf_size - 4;

    if (!source)
	source = "";
    ccp str = source;
    ccp end = len < 0 ? 0 : str + len;

    while ( dest < dest_end && ( !end || str < end ) )
    {
	const u8 ch = (u8)*str++;
	switch (ch)
	{
	    case 0:
		if (!end)
		{
		    str--;
		    goto eos;
		}
		*dest++ = '\\';
		*dest++ = 'x';
		*dest++ = '0';
		*dest++ = '0';
		break;

	    case '\\': *dest++ = '\\'; *dest++ = '\\'; break;
	    case '\a': *dest++ = '\\'; *dest++ = 'a'; break;
	    case '\b': *dest++ = '\\'; *dest++ = 'b'; break;
	    case '\f': *dest++ = '\\'; *dest++ = 'f'; break;
	    case '\n': *dest++ = '\\'; *dest++ = 'n'; break;
	    case '\r': *dest++ = '\\'; *dest++ = 'r'; break;
	    case '\t': *dest++ = '\\'; *dest++ = 't'; break;
	    case '\v': *dest++ = '\\'; *dest++ = 'v'; break;

	    case '\033':
		*dest++ = '\\';
		if (allow_e)
		    *dest++ = 'e';
		else
		{
		    *dest++ = 'x';
		    *dest++ = '1';
		    *dest++ = 'B';
		}
		break;

	    case '|':
		if (esc_pipe)
		{
		    *dest++ = '\\';
		    *dest++ = '!';
		}
		else
		    *dest++ = '|';
		break;

	    default:
		if ( ch == quote )
		{
		    *dest++ = '\\';
		    *dest++ = quote;
		}
		else if ( ch < ' ' || !utf8 && (ch&0x7f) < ' ' || (ch&0x7f) == 0x7f )
		{
		    *dest++ = '\\';
		    *dest++ = 'x';
		    *dest++ = HiDigits[ch>>4];
		    *dest++ = HiDigits[ch&15];
		}
		else
		    *dest++ = ch;
	}
    }
 eos:
    *dest = 0;
    if (dest_len)
	*dest_len = dest - buf;
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

uint ScanEscapedString
(
    // returns the number of valid bytes in 'buf' (NULL term added but not counted)

    char	*buf,		// valid destination buffer, maybe source
    uint	buf_size,	// size of 'buf'
    ccp		source,		// string to scan
    int		len,		// length of string. if -1, str is null terminated
    bool	utf8,		// true: source and output is UTF-8
    int		quote,		// -1:auto, 0:none, >0: quotation char
    uint	*scanned_len	// not NULL: Store number of scanned 'source' bytes here
)
{
    DASSERT(buf);
    DASSERT(buf_size>3);
    DASSERT(source);

    char *dest = buf;
    char *dest_end = dest + buf_size - ( utf8 ? 4 : 1 );

    ccp src = source;
    ccp src_end = src + ( len < 0 ? strlen(src) : len );

    bool have_quote = quote > 0;
    if ( quote == -1 && src < src_end && ( *src == '"' || *src == '\'' ))
    {
	quote = *src++;
	have_quote = true;
    }
    else if ( have_quote && *src == quote )
	src++;

    while ( dest < dest_end && src < src_end )
    {
	uint code;
	if ( *src == '\\' )
	    src = ScanEscape(&code,src+1,src_end);
	else
	{
	    if (utf8)
		code = ScanUTF8AnsiCharE(&src,src_end);
	    else
		code = *src++;

	    if ( code == quote && have_quote )
		break;
	}

	if (utf8)
	    dest = PrintUTF8Char(dest,code);
	else
	    *dest++ = code;
    }

    if (scanned_len)
	*scanned_len = src - source;

    *dest = 0;
    return dest - buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u32 CalcCRC32 ( u32 crc, cvp buf, uint size )
{
 #if HAVE_PRINT0
    static bool done = false;
    if ( !done && TRACE_FILE )
    {
	done = true;
	fprintf(TRACE_FILE,"\nstatic const u32 TableCRC32[0x100] =\n{\n");
	uint i;
	for ( i = 0; i < 0x100; i += 8 )
	    fprintf(TRACE_FILE," %#010x,%#010x,%#010x,%#010x, %#010x,%#010x,%#010x,%#010x,\n",
		TableCRC32[i+0],
		TableCRC32[i+1],
		TableCRC32[i+2],
		TableCRC32[i+3],
		TableCRC32[i+4],
		TableCRC32[i+5],
		TableCRC32[i+6],
		TableCRC32[i+7] );
	fprintf(TRACE_FILE,"};\n");
    }
 #endif

    crc = ~crc;
    const u8 *p = buf;
    while ( size-- )
	crc = TableCRC32[ (crc ^ *p++) & 0xff ] ^ crc >> 8;

    return ~crc;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u64 DecodeU64
(
    ccp		source,			// string to decode
    int		len,			// length of string. if -1, str is null terminated
    uint	base,			// number of possible digits (2..36):
    uint	*scanned_len		// not NULL: Store number of scanned 'str' bytes here
)
{
    DASSERT(source);
    DASSERT( base <= 36 );

    u64 num = 0;
    ccp src = source;
    ccp end = len < 0 ? 0 : src + len;

    while ( !end || src < end )
    {
	u8 digit = TableNumbers[(u8)*src];
	if ( digit >= base )
	    break;
	num = num * base + digit;
	src++;
    }

    if (scanned_len)
	*scanned_len = src - source;
    return num;
}

///////////////////////////////////////////////////////////////////////////////

ccp EncodeU64
(
    char	* buf,		// result buffer (size depends on base)
				// If NULL, a local circulary static buffer is used
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u64		num,		// number to encaode
    uint	base		// number of possible digits (2..64):
)
{
    DASSERT( base >= 2 );
    DASSERT( base <= 64 );
    if ( base < 2 )
	base = 2;
    else if ( base > 64 )
	base = 64;

    char digits[65], *dest = digits + sizeof(digits);
    *--dest = 0;

    while ( num )
    {
	*--dest = LoDigits[ num % base ];
	num /= base;
    }

    if (!buf)
	buf = GetCircBuf ( buf_size = digits + sizeof(digits)-1 - dest );

    StringCopyS(buf,buf_size,dest);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Default tables for DecodeBase64() and EncodeBase64(), if no table is defined.

ccp TableDecode64default = TableDecode64;
ccp TableEncode64default = TableEncode64;

///////////////////////////////////////////////////////////////////////////////

uint DecodeBase64
(
    // returns the number of valid bytes in 'buf'

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >= 3
    ccp		source,			// NULL or string to decode
    int		len,			// length of string. if -1, str is null terminated
    const char	decode64[256],		// decoding table; if NULL: use TableDecode64default
    bool	allow_white_spaces,	// true: skip white spaces
    uint	*scanned_len		// not NULL: Store number of scanned 'str' bytes here
)
{
    DASSERT(buf);
    DASSERT(buf_size>=3);
    u8 *dest = (u8*)buf;
    u8 *dest_end = dest + ( scanned_len ? buf_size / 3 * 3 : buf_size );

    if (!source)
	source = "";
    ccp src = source;
    ccp end = src + ( len < 0 ? strlen(src) : len );
    int ch1, ch2;

    if (!decode64)
	decode64 = TableDecode64default;

    while ( dest < dest_end && src < end )
    {
	ch1 = decode64[(u8)*src++];
	if (allow_white_spaces)
	    while ( ch1 == DECODE_SPACE && src < end )
		ch1 = decode64[(u8)*src++];
	if ( ch1 < 0 )
	{
	    src--;
	    break;
	}
	if ( src == end )
	    break;

	ch2 = decode64[(u8)*src++];
	if (allow_white_spaces)
	    while ( ch2 == DECODE_SPACE && src < end )
		ch2 = decode64[(u8)*src++];
	if ( ch2 < 0 )
	{
	    src--;
	    break;
	}

	*dest++ = ch1 << 2 | ch2 >> 4;
	if ( src == end || dest == dest_end )
	    break;

	ch1 = decode64[(u8)*src++];
	if (allow_white_spaces)
	    while ( ch1 == DECODE_SPACE && src < end )
		ch1 = decode64[(u8)*src++];
	if ( ch1 < 0 )
	{
	    src--;
	    break;
	}

	*dest++ = ch2 << 4 | ch1 >> 2;
	if ( src == end || dest == dest_end )
	    break;

	ch2 = decode64[(u8)*src++];
	if (allow_white_spaces)
	    while ( ch2 == DECODE_SPACE && src < end )
		ch2 = decode64[(u8)*src++];
	if ( ch2 < 0 )
	{
	    src--;
	    break;
	}
	*dest++ = ch1 << 6 | ch2;
    }

    if (scanned_len)
    {
	while ( src < end && decode64[(u8)*src] == DECODE_FILLER )
	    src++;
	*scanned_len = src - source;
    }
    return dest - (u8*)buf;
}

///////////////////////////////////////////////////////////////////////////////

mem_t DecodeBase64Circ
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    ccp		source,			// NULL or string to decode
    int		len,			// length of string. if -1, str is null terminated
    const char	decode64[256]		// decoding table; if NULL: use TableDecode64default
)
{
    char buf[CIRC_BUF_MAX_ALLOC+10];
    mem_t mem;
    mem.len = DecodeBase64(buf,sizeof(buf),source,len,decode64,false,0);
    if ( mem.len < CIRC_BUF_MAX_ALLOC )
	mem.ptr = CopyCircBuf0(buf,mem.len);
    else
    {
	mem.ptr = 0;
	mem.len = 0;
    }
    return mem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint EncodeBase64
(
    // returns the number of scanned bytes of 'source'

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf' >= 4
    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    const char	encode64[64+1],		// encoding table; if NULL: use TableEncode64default
    bool	use_filler,		// use filler for aligned output

    ccp		next_line,		// not NULL: use this string as new line sep
    uint	bytes_per_line		// >0: use 'next_line' every # input bytes
					// will be rounded down to multiple of 3
)
{
    DASSERT(buf);
    DASSERT(buf_size>3);
    DASSERT(source||!source_len);

    noPRINT("EncodeBase64(sz=%u,len=%d,fil=%d)\n",buf_size,source_len,use_filler);

    if (!next_line)
	bytes_per_line = ~0;
    else if (!bytes_per_line)
	bytes_per_line = 19;
    else
	bytes_per_line /= 3;

    char *dest = buf;
    char *dest_end = buf + buf_size - 4;

    if (!source)
	source = "";
    const u8 *src = source;
    const u8 *src_end = src + ( source_len < 0 ? strlen(source) : source_len );

    if (!encode64)
	encode64 = TableEncode64default;

    uint n_tupel = 0;

    while ( src < src_end && dest < dest_end )
    {
	if ( ++n_tupel > bytes_per_line && next_line )
	{
	    char *next = StringCopyE(dest,(char*)buf+buf_size,next_line);
	    if ( next >= dest_end )
		break;
	    dest = next;
	    n_tupel = 1;
	}

	//----- start: no pending bits

	u8 ch1 = *src++;
	*dest++ = encode64 [ ch1 >> 2 ];
	ch1 &= 0x03;
	DASSERT( (ch1 & ~0x03) == 0 );
	if ( src == src_end )
	{
	    *dest++ = encode64[ch1<<4];
	    if (use_filler)
	    {
		*dest++ = encode64[64];
		*dest++ = encode64[64];
	    }
	    break;
	}

	//----- 2 pending bits in 'ch1'

	u8 ch2 = *src++;
	*dest++ = encode64 [ ch1 << 4 | ch2 >> 4 ];
	ch2 &= 0x0f;
	DASSERT( (ch2 & ~0x0f) == 0 );
	if ( src == src_end )
	{
	    *dest++ = encode64[ch2<<2];
	    if (use_filler)
		*dest++ = encode64[64];
	    break;
	}

	//----- 4 pending bits in 'ch2'

	ch1 = *src++;
	*dest++ = encode64 [ ch2 << 2 | ch1 >> 6 ];

	//----- 6 pending bits in 'ch1'

	*dest++ = encode64[ch1&0x3f];
    }

    *dest = 0;

    return src - (const u8*)source;
}

///////////////////////////////////////////////////////////////////////////////

mem_t EncodeBase64Circ
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    const char	encode64[64+1]		// encoding table; if NULL: use TableEncode64default
)
{
    char buf[CIRC_BUF_MAX_ALLOC+10];
    EncodeBase64(buf,sizeof(buf),source,source_len,encode64,false,0,0);
    mem_t mem;
    mem.len = strlen(buf);
    if ( mem.len < CIRC_BUF_MAX_ALLOC )
	mem.ptr = CopyCircBuf(buf,mem.len+1);
    else
    {
	mem.ptr = 0;
	mem.len = 0;
    }
    return mem;
}

///////////////////////////////////////////////////////////////////////////////


uint EncodeBase64ml // ml: multi line
(
    // returns the number of scanned bytes of 'source'

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf' >= 4
    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    const char	encode64[64+1],		// encoding table; if NULL: use TableEncode64default
    bool	use_filler,		// use filler for aligned output

    int		indent,			// indention of output
    ccp		prefix,			// NULL or prefix before encoded data
    ccp		eol,			// line terminator, if NULL then use NL
    int		bytes_per_line		// create a new line every # input bytes
					// will be rounded down to multiple of 3
)
{
    indent = NormalizeIndent(indent);
    if (!eol)
	eol = "\n";

    char linesep[200];
    int pre_pos;
    snprintf(linesep,sizeof(linesep),"%s%n%*s%s",
		eol,
		&pre_pos,
		indent, "",
		prefix ? prefix : ""
		);

    char *end = buf + buf_size;
    char *dest = StringCopyE(buf,end,linesep+pre_pos);
    const uint scanned = EncodeBase64( dest, end-dest, source, source_len,
				encode64, use_filler, linesep, bytes_per_line );
    dest = buf + strlen(buf);
    StringCopyE(dest,end,eol);
    return scanned;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint EncodeJSON
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >2 and 2 bytes longer than needed
    const void	*source,		// NULL or data to encode
    int		source_len		// length of 'source'; if <0: use strlen(source)
)
{
    DASSERT(buf);
    DASSERT(buf_size>2);

    char *dest = buf;
    char *dest_end = dest + buf_size - 2;

    if (!source)
	source = "";
    ccp str = source;
    ccp end = str + ( source_len < 0 ? strlen(str) : source_len );

    while ( dest < dest_end && str < end )
    {
	const u8 ch = (u8)*str++;
	switch (ch)
	{
	    case '"':  *dest++ = '\\'; *dest++ = '"'; break;
	    case '\\': *dest++ = '\\'; *dest++ = '\\'; break;
	    case '\b': *dest++ = '\\'; *dest++ = 'b'; break;
	    case '\f': *dest++ = '\\'; *dest++ = 'f'; break;
	    case '\n': *dest++ = '\\'; *dest++ = 'n'; break;
	    case '\r': *dest++ = '\\'; *dest++ = 'r'; break;
	    case '\t': *dest++ = '\\'; *dest++ = 't'; break;

	    default:
		if ( ch >= ' ' )
		    *dest++ = ch;
		else if ( dest + 5 > dest_end )
		    goto eos;
		else
		{
		    *dest++ = '\\';
		    *dest++ = 'u';
		    *dest++ = '0';
		    *dest++ = '0';
		    *dest++ = HiDigits[ch>>4];
		    *dest++ = HiDigits[ch&15];
		}
		break;
	}
    }

 eos:
    *dest = 0;
    return dest - buf;
}

//-----------------------------------------------------------------------------

mem_t EncodeJSONCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len		// length of 'source'; if <0: use strlen(source)
)
{
    char buf[CIRC_BUF_MAX_ALLOC+10];
    mem_t mem;
    mem.len = EncodeJSON(buf,sizeof(buf),source,source_len);
    if ( mem.len < CIRC_BUF_MAX_ALLOC )
	mem.ptr = CopyCircBuf(buf,mem.len+1);
    else
    {
	mem.ptr = 0;
	mem.len = 0;
    }
    return mem;
}

//-----------------------------------------------------------------------------

mem_t QuoteJSONCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    int		null_if			// return 'null' without quotes ...
					//	<=0: never
					//	>=1: if source == NULL
					//	>=2: if source_len == 0
)
{
    if ( source_len < 0 )
	source_len = source ? strlen(source) : 0;

    mem_t mem;
    if ( null_if > 0 && !source || null_if > 1 && !source_len )
    {
	mem.ptr = "null";
	mem.len = 4;
    }
    else
    {
	char buf[CIRC_BUF_MAX_ALLOC+10];
	mem.len = EncodeJSON(buf+1,sizeof(buf)-2,source,source_len) + 2;
	if ( mem.len < CIRC_BUF_MAX_ALLOC - 2 )
	{
	    buf[0] = buf[mem.len-1] = '"';
	    buf[mem.len] = 0;
	    mem.ptr = CopyCircBuf(buf,mem.len+1);
	}
	else
	{
	    mem.ptr = 0;
	    mem.len = 0;
	}
    }
    return mem;
}

//-----------------------------------------------------------------------------

mem_t DecodeJSONCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    ccp		source,		// NULL or string to decode
    int		source_len,	// length of 'source'. If -1, str is NULL terminated
    int		quote		// -1:auto, 0:none, >0: quotation char
)
{
    char buf[CIRC_BUF_MAX_ALLOC+10];
    mem_t mem;
    mem.len = DecodeJSON(buf,sizeof(buf),source,source_len,quote,0);
    if ( mem.len < CIRC_BUF_MAX_ALLOC )
	mem.ptr = CopyCircBuf(buf,mem.len+1);
    else
    {
	mem.ptr = 0;
	mem.len = 0;
    }
    return mem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[hex]]

uint EncodeHex
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >2 and 2 bytes longer than needed
    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    ccp		digits			// digits to use, eg. LoDigits[] (=fallback) or HiDigits[]
)
{
    DASSERT(buf);
    DASSERT(buf_size>2);

    if (!digits)
	digits = LoDigits;

    char *dest = buf;
    char *dest_end = dest + buf_size - 1;

    if (!source)
	source = "";
    ccp str = source;
    ccp end = str + ( source_len < 0 ? strlen(str) : source_len );

    while ( dest < dest_end && str < end )
    {
	const u8 ch = (u8)*str++;
	*dest++ = digits[ch>>4];
	*dest++ = digits[ch&15];
    }
    *dest = 0;
    return dest - buf;
}

//-----------------------------------------------------------------------------
// [[hex]]

mem_t EncodeHexCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    ccp		digits			// digits to use, eg. LoDigits[] (=fallback) or HiDigits[]
)
{
    char buf[CIRC_BUF_MAX_ALLOC+10];
    mem_t mem;
    mem.len = EncodeHex(buf,sizeof(buf),source,source_len,digits);
    if ( mem.len < CIRC_BUF_MAX_ALLOC )
	mem.ptr = CopyCircBuf(buf,mem.len+1);
    else
    {
	mem.ptr = 0;
	mem.len = 0;
    }
    return mem;
}

///////////////////////////////////////////////////////////////////////////////
// [[hex]]

uint DecodeHex
(
    // returns the number of valid bytes in 'buf'

    char	*buf,		// valid destination buffer
    uint	buf_size,	// size of 'buf', >= 3
    ccp		source,		// NULL or string to decode
    int		source_len,	// length of 'source'. If -1, str is NULL terminated
    int		quote,		// -1:auto, 0:none, >0: quotation char
    uint	*scanned_len	// not NULL: Store number of scanned 'str' bytes here
)
{
    DASSERT(buf);
    DASSERT(buf_size>3);
    DASSERT(source);

    char *dest = buf;
    char *dest_end = dest + buf_size - 1;

    ccp src = source;
    ccp src_end = src + ( source_len < 0 ? strlen(src) : source_len );

    bool have_quote = quote > 0;
    if ( quote == -1 && src < src_end && ( *src == '"' || *src == '\'' ))
    {
	quote = *src++;
	have_quote = true;
    }

    while ( dest < dest_end && src < src_end )
    {
	u8 hi = TableNumbers[(u8)*src];
	if ( hi >= 16 )
	    break;
	hi <<= 4;

	u8 lo = ++src < src_end ? TableNumbers[(u8)*src] : 0;
	if ( lo >= 16 )
	{
	    *dest++ = hi;
	    break;
	}
	*dest++ = hi | lo;
	src++;
    }

    if (scanned_len)
    {
	if ( have_quote && src < src_end && *src == quote )
	    src++;
	*scanned_len = src - source;
    }

    *dest = 0;
    return dest - buf;
}

//-----------------------------------------------------------------------------
// [[hex]]

mem_t DecodeHexCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    ccp		source,		// NULL or string to decode
    int		source_len,	// length of 'source'. If -1, str is NULL terminated
    int		quote		// -1:auto, 0:none, >0: quotation char
)
{
    char buf[CIRC_BUF_MAX_ALLOC+10];
    mem_t mem;
    mem.len = DecodeHex(buf,sizeof(buf),source,source_len,quote,0);
    if ( mem.len < CIRC_BUF_MAX_ALLOC )
	mem.ptr = CopyCircBuf(buf,mem.len+1);
    else
    {
	mem.ptr = 0;
	mem.len = 0;
    }
    return mem;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint DecodeByMode
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char		*buf,		// valid destination buffer
    uint		buf_size,	// size of 'buf', >= 3
    ccp			source,		// string to decode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode,		// encoding mode
    uint		*scanned_len	// not NULL: Store number of scanned 'str' bytes here
)
{
    DASSERT(buf);
    DASSERT( buf_size >= 1 );
    DASSERT( source || !slen );
    DASSERT( (uint)emode < ENCODE__N );

    uint len = 0;
    if ( buf_size < 4 )
    {
	char temp[4];
	len = DecodeByMode(temp,sizeof(temp),source,slen,emode,scanned_len);
	memcpy(buf,temp,buf_size);
	goto term;
    }

    DASSERT( buf_size >= 3 );
    switch (emode)
    {
      case ENCODE_STRING:
      case ENCODE_PIPE:
	len = ScanEscapedString(buf,buf_size,source,slen,false,-1,scanned_len);
	break;

      case ENCODE_UTF8:
      case ENCODE_PIPE8:
	len = ScanEscapedString(buf,buf_size,source,slen,true,-1,scanned_len);
	break;

      case ENCODE_BASE64:
	len = DecodeBase64(buf,buf_size-1,source,slen,TableDecode64,true,scanned_len);
	break;

      case ENCODE_BASE64URL:
      case ENCODE_BASE64STAR:
	len = DecodeBase64(buf,buf_size-1,source,slen,TableDecode64url,true,scanned_len);
	break;

      case ENCODE_BASE64XML:
	len = DecodeBase64(buf,buf_size-1,source,slen,TableDecode64xml,true,scanned_len);
	break;

      case ENCODE_JSON:
	len = DecodeJSON(buf,buf_size,source,slen,-1,scanned_len);
	break;

      case ENCODE_HEX:
	len = DecodeHex(buf,buf_size,source,slen,-1,scanned_len);
	break;

      default:
	if ( slen < 0 )
	    len = StringCopyS(buf,buf_size,source) - buf;
	else
	    len = StringCopySM(buf,buf_size,source,slen) - buf;
	if (scanned_len)
	    *scanned_len = len;
	break;
    }

    //--- terminate with NULL

 term:;
    if ( len >= buf_size )
	len = buf_size - 1;
    buf[len] = 0;
    return len;
}

///////////////////////////////////////////////////////////////////////////////

mem_t DecodeByModeMem
(
    // Returns the decoded 'source'. Result is NULL-terminated.
    // It points either to 'buf' or is alloced (on buf==NULL or too less space)
    // If alloced (mem.ptr!=buf) => call FreeMem(&mem)

    char		*buf,		// NULL or destination buffer
    uint		buf_size,	// size of 'buf'
    ccp			source,		// string to decode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode,		// encoding mode
    uint		*scanned_len	// not NULL: Store number of scanned 'str' bytes here
)
{
    DASSERT( source || !slen );
    DASSERT( (uint)emode < ENCODE__N );

    if ( slen < 0 )
	slen = source ? strlen(source) : 0;

    uint need;
    switch (emode)
    {
      case ENCODE_STRING:
      case ENCODE_UTF8:
      case ENCODE_PIPE:
      case ENCODE_PIPE8:
	need = slen + 5;
	break;

      case ENCODE_BASE64:
      case ENCODE_BASE64URL:
      case ENCODE_BASE64STAR:
      case ENCODE_BASE64XML:
	need = ( 3*slen ) / 4 + 12;
	break;

      case ENCODE_HEX:
	need = (slen+3)/2;
	break;

      //case ENCODE_JSON: // [[json]]
      default:
	need = slen + 3; // at least 3 bytes
	break;
    }

    const bool alloced = !buf || buf_size < need;
    if (alloced)
    {
	buf = MALLOC(need);
	buf_size = need;
    }

    mem_t mem;
    mem.len = DecodeByMode(buf,buf_size,source,slen,emode,scanned_len);
    if ( alloced && mem.len+10 < buf_size )
	buf = REALLOC( buf, mem.len+1 );
    mem.ptr = buf;
    return mem;
}

///////////////////////////////////////////////////////////////////////////////

uint DecodeByModeMemList
(
    // returns the number of scanned strings

    mem_list_t		*res,		// result
    uint		res_mode,	// 0:append, 1:clear, 2:init
    ccp			source,		// string to decode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode,		// encoding mode
    uint		*scanned_len	// not NULL: Store number of scanned 'str' bytes here
)
{
    DASSERT(res);
    if ( res_mode > 1 )
	InitializeMemList(res);
    else if ( res_mode > 0 )
	ResetMemList(res);

    if (!source)
    {
	if (scanned_len)
	    *scanned_len = 0;
	return 0;
    }

    ccp src = source;
    if ( slen < 0 )
	slen = strlen(src);
    NeedBufMemList(res,slen,0);
    ccp end = src + slen;

    char buf[4000];

    uint count;
    for ( count = 1;; count++ )
    {
	if ( emode != ENCODE_OFF )
	    while ( src < end && (uchar)*src <= ' ' )
		src++;

	if ( src >= end )
	{
	    AppendMemListN(res,&EmptyMem,1,MEMLM_ALL);
	    break;
	}

	if ( *src == ',' )
	    AppendMemListN(res,&EmptyMem,1,MEMLM_ALL);
	else if ( *src == '%' )
	{
	    AppendMemListN(res,&NullMem,1,MEMLM_ALL);
	    src++;
	}
	else if ( emode == ENCODE_OFF )
	{
	    mem_t mem;
	    mem.ptr = src;
	    src = memchr(src,',',end-src);
	    if (!src)
		src = end;
	    mem.len = src - mem.ptr;
	    AppendMemListN(res,&mem,1,MEMLM_ALL);
	}
	else
	{
	    uint scanned;
	    mem_t mem = DecodeByModeMem(buf,sizeof(buf),src,end-src,emode,&scanned);
	    src += scanned;

	    AppendMemListN(res,&mem,1,MEMLM_ALL);
	    if ( mem.ptr != buf )
		FreeString(mem.ptr);
	}

	while ( src < end && (uchar)*src <= ' ' )
	    src++;
	if ( src >= end || *src != ',' )
	    break;
	src++;
    }

    //--- terminate

    if (scanned_len)
	*scanned_len = src - source;
    return count;
}

///////////////////////////////////////////////////////////////////////////////

uint EncodeByMode
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char		*buf,		// valid destination buffer
    uint		buf_size,	// size of 'buf' >= 4
    ccp			source,		// string to encode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
)
{
    DASSERT(buf);
    DASSERT( buf_size >= 3 );
    DASSERT( source || !slen );
    DASSERT( (uint)emode < ENCODE__N );

    noPRINT("EncodeByMode(,%u,%d,%d)\n",buf_size,slen,emode);

    uint len = 0;
    switch (emode)
    {
      case ENCODE_STRING:
	PrintEscapedString(buf,buf_size,source,slen,0,0,&len);
	break;

      case ENCODE_UTF8:
	PrintEscapedString(buf,buf_size,source,slen,CHMD__MODERN,0,&len);
	break;

      case ENCODE_PIPE:
	PrintEscapedString(buf,buf_size,source,slen,CHMD_IF_REQUIRED|CHMD_PIPE,0,&len);
	break;

      case ENCODE_PIPE8:
	PrintEscapedString(buf,buf_size,source,slen,CHMD__MODERN|CHMD_IF_REQUIRED|CHMD_PIPE,0,&len);
	break;

      case ENCODE_BASE64:
	len = EncodeBase64(buf,buf_size,source,slen,TableEncode64,true,0,0);
	len = GetEncodeBase64FillLen(len);
	break;

      case ENCODE_BASE64URL:
	len = EncodeBase64(buf,buf_size,source,slen,TableEncode64url,true,0,0);
	len = GetEncodeBase64FillLen(len);
	break;

      case ENCODE_BASE64STAR:
	len = EncodeBase64(buf,buf_size,source,slen,TableEncode64star,true,0,0);
	len = GetEncodeBase64FillLen(len);
	break;

      case ENCODE_BASE64XML:
	len = EncodeBase64(buf,buf_size,source,slen,TableEncode64xml,true,0,0);
	len = GetEncodeBase64FillLen(len);
	break;

      case ENCODE_JSON:
	len = EncodeJSON(buf,buf_size,source,slen);
	break;

      case ENCODE_HEX:
	len = EncodeHex(buf,buf_size,source,slen,0);
	break;

      default:
	if ( slen < 0 )
	    len = StringCopyS(buf,buf_size,source) - buf;
	else
	    len = StringCopySM(buf,buf_size,source,slen) - buf;
	break;
    }

    //--- terminate with NULL

    if ( len >= buf_size )
	len = buf_size - 1;
    buf[len] = 0;
    return len;
}

///////////////////////////////////////////////////////////////////////////////

mem_t EncodeByModeMem
(
    // Returns the encoded 'source'. Result is NULL-terminated.
    // It points either to 'buf' or is alloced (on buf==NULL or too less space)
    // If alloced (mem.ptr!=buf) => call FreeMem(&mem)

    char		*buf,		// NULL or destination buffer
    uint		buf_size,	// size of 'buf'
    ccp			source,		// string to encode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
)
{
    DASSERT( source || slen <= 0 );
    DASSERT( (uint)emode < ENCODE__N );

    if ( slen < 0 )
	slen = source ? strlen(source) : 0;

    uint need;
    switch (emode)
    {
      case ENCODE_STRING:
      case ENCODE_UTF8:
      case ENCODE_PIPE:
      case ENCODE_PIPE8:
	need = slen * 4 + 5;
	break;

      case ENCODE_BASE64:
      case ENCODE_BASE64URL:
      case ENCODE_BASE64STAR:
      case ENCODE_BASE64XML:
	need = ( 4*slen ) / 3 + 10;
	break;

      case ENCODE_HEX:
	need = 2*slen + 1;
	break;

      case ENCODE_JSON: // [[json]]
	{
	    need = slen + 1;
	    ccp ptr = source;
	    ccp end = ptr + slen;
	    while ( ptr < end )
		if ( (uchar)*ptr++ < ' ' )
		    need += 5;
	}
	break;

      default:
	need = slen + 1;
	break;
    }

    const bool alloced = !buf || buf_size < need;
    if (alloced)
    {
	buf = MALLOC(need);
	buf_size = need;
    }

    mem_t mem;
    mem.len = EncodeByMode(buf,buf_size,source,slen,emode);
    if ( alloced && mem.len+10 < buf_size )
	buf = REALLOC( buf, mem.len+1 );
    mem.ptr = buf;
    return mem;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		escape/quote strings, alloc space	///////////////
///////////////////////////////////////////////////////////////////////////////

exmem_t EscapeStringEx
(
    // Returns an exmem_t struct. If not quoted (CHMD_IF_REQUIRED) it returns 'src'
    // Use FreeExMem(&result) to free possible alloced result.

    cvp		src,			// NULL or source
    int		src_len,		// size of 'src'. If -1: Use strlen(src)
    cvp		return_if_null,		// return this, if 'src==NULL'
    cvp		return_if_empty,	// return this, if src is empty (have no chars)
    CharMode_t	char_mode,		// how to escape (CHMD_*)
    char	quote,			// quoting character: "  or  '  or  $ (for $'...')
    bool	try_circ		// use circ-buffer, if result is small enough
)
{
    if (!src)
	return ExMemByString((char*)return_if_null);

    if ( src_len < 0 )
	src_len = strlen(src);
    if (!src_len)
	return ExMemByString((char*)return_if_empty);

    char quote_char;
    uint quote_mode;
    if ( quote == '$' )
    {
	quote_mode = 3;
	quote_char = '\'';
    }
    else if (quote)
    {
	quote_mode = 2;
	quote_char = quote;
    }
    else
    {
	quote_mode = 0;
	quote_char = '"';
    }

    uint size = GetEscapeLen(src,src_len,char_mode,quote_char);
    if ( !size && char_mode & CHMD_IF_REQUIRED )
	return ExMemByS(src,src_len);

    size += quote_mode + src_len + 4;
    const bool use_circ = try_circ && size <= CIRC_BUF_MAX_ALLOC;
    char *buf = use_circ ? GetCircBuf(size) : MALLOC(size);

    uint len;
    if ( quote_mode == 3 )
    {
	PrintEscapedString(buf+2,size-3,src,src_len,char_mode,quote_char,&len);
	buf[0] = '$';
	buf[1] = buf[len+2] = quote_char;
	buf[len+3] = 0;

    }
    else if ( quote_mode == 2 )
    {
	PrintEscapedString(buf+1,size-2,src,src_len,char_mode,quote_char,&len);
	buf[0] = buf[len+1] = quote_char;
	buf[len+2] = 0;
    }
    else
	PrintEscapedString(buf,size,src,src_len,char_mode,quote_char,&len);

    exmem_t res = { .data={ buf, len+quote_mode },
			.is_circ_buf = use_circ, .is_alloced = !use_circ };
    return res;
};

///////////////////////////////////////////////////////////////////////////////

char * EscapeString
(
    // Returns a pointer.
    // Use FreeString(result) to free possible alloced result.
    // circ-buffer is ignored by FreeString().

    cvp		src,			// NULL or source
    int		src_len,		// size of 'src'. If -1: Use strlen(src)
    cvp		return_if_null,		// return this, if 'src==NULL'
    cvp		return_if_empty,	// return this, if src is empty (have no chars)
    CharMode_t	char_mode,		// how to escape
    char	quote,			// quoting character: "  or  '  or  $ (for $'...')
    bool	try_circ,		// use circ-buffer, if result is small enough
    uint	*dest_len		// not NULL: Store length of result here
)
{
    if (!src)
	return (char*)return_if_null;
    if ( src_len < 0 )
	src_len = strlen(src);
    if (!src_len)
	return (char*)return_if_empty;

 #if 1
    exmem_t res = EscapeStringEx(src,src_len,return_if_null,return_if_empty,char_mode,quote,try_circ);
    if (dest_len)
	*dest_len = res.data.len;
    return res.is_circ_buf || res.is_alloced
	? (char*)res.data.ptr
	: MEMDUP(res.data.ptr,res.data.len);
 #else // [[2do]] [[obsolete]]
    char quote_char;
    uint quote_mode;
    if ( quote == '$' )
    {
	quote_mode = 3;
	quote_char = '\'';
    }
    else if (quote)
    {
	quote_mode = 2;
	quote_char = quote;
    }
    else
    {
	quote_mode = 0;
	quote_char = '"';
    }

    uint size = GetEscapeLen(src,src_len,char_mode,quote_char);
    if ( !size && char_mode & CHMD_IF_REQUIRED )
    {
	char *buf = try_circ && size <= CIRC_BUF_MAX_ALLOC ? GetCircBuf(src_len+1) : MALLOC(src_len+1);
	memcpy(buf,src,src_len);
	buf[src_len] = 0;
	if (dest_len)
	    *dest_len = src_len;
	return buf;
    }

    size += quote_mode + src_len + 4;
    char *buf = try_circ && size <= CIRC_BUF_MAX_ALLOC ? GetCircBuf(size) : MALLOC(size);

    uint len;
    if ( quote_mode == 3 )
    {
	PrintEscapedString(buf+2,size-3,src,src_len,char_mode,quote_char,&len);
	buf[0] = '$';
	buf[1] = buf[len+2] = quote_char;
	buf[len+3] = 0;

    }
    else if ( quote_mode == 2 )
    {
	PrintEscapedString(buf+1,size-2,src,src_len,char_mode,quote_char,&len);
	buf[0] = buf[len+1] = quote_char;
	buf[len+2] = 0;
    }
    else
	PrintEscapedString(buf,size,src,src_len,char_mode,quote_char,&len);

    if (dest_len)
	*dest_len = len + quote_mode;
    return buf;
 #endif
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct Escape_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// https://en.wikipedia.org/wiki/ANSI_escape_code

const char ecape_stat_name[ESCST__N+1][5] =
{
	"-",	// ESCST_NONE
	"CHAR",	// ESCST_CHAR
	"FE",	// ESCST_FE
	"SS2",	// ESCST_SS2
	"SS3",	// ESCST_SS3
	"CSI",	// ESCST_CSI
	"OSC",	// ESCST_OSC
	"",	// ESCST__N
};

///////////////////////////////////////////////////////////////////////////////

EscapeStat_t CheckEscape ( Escape_t *esc, cvp source, cvp end, bool check_esc )
{
    DASSERT(esc);
    memset(esc,0,sizeof(*esc));
    if (!source)
	return 0;

    ccp src = source;
    if (!end)
	end = src + strlen(src);
    if ( src >= (ccp)end )
	return 0;

    ccp beg = src;
    if (check_esc)
    {
	if ( *src != '\e' )
	{
	    esc->status = ESCST_CHAR;
	    esc->code = *src++;
	    goto term;
	}

	src++;
	if ( src >= (ccp)end )
	    return 0;
    }
    esc->code = *src++;

    int term_wd = 0;
    switch (esc->code)
    {
     case 'N':			//--- SS2: Single Shift 2 ---
	if ( src < (ccp)end )
	{
	    esc->status = ESCST_SS2;
	    esc->code = *src++;
	}
	break;

     case 'O':			//--- SS3: Single Shift 3 ---
	if ( src < (ccp)end )
	{
	    esc->status = ESCST_SS3;
	    esc->code = *src++;
	}
	break;

     case '[':			//--- CSI ---
	while ( src < (ccp)end && *src >= 0x30 && *src <= 0x3f )
	    src++;
	while ( src < (ccp)end && *src >= 0x20 && *src <= 0x2f )
	    src++;
	if ( src < (ccp)end && *src >= 0x40 && *src <= 0x7e )
	{
	    esc->status = ESCST_CSI;
	    esc->code = *src++;
	}
	break;


     case ']':			//--- OSC ---
	// terminate by ST (ESC \ == 0x9c) or BEL (7)
	for ( ; src < (ccp)end; src++ )
	{
	    if ( *src == 7 || (u8)*src == 0x9c )
	    {
		src++;
		term_wd = 1;
		esc->status = ESCST_OSC;
		break;
	    }
	    if ( src+1 < (ccp)end && src[0] == '\e' && src[1] == '\\' )
	    {
		src += 2;
		term_wd = 2;
		esc->status = ESCST_OSC;
		break;
	    }
	}
	break;

     default:			//--- single char ---
	esc->status = ESCST_CHAR;
	break;
    }

 term:;
    if ( esc->status != ESCST_NONE )
    {
	esc->scanned_len = src - (ccp)source;
	if ( esc->status > ESCST_CHAR )
	{
	    esc->esc.ptr = beg;
	    esc->esc.len = src - beg - term_wd;
	}
    }

    return esc->status;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    time			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp micro_second_char = "u";

s64 timezone_adjust_sec   = -1;
s64 timezone_adjust_usec  = -1;
s64 timezone_adjust_nsec  = -1;
int timezone_adjust_isdst = -1;

///////////////////////////////////////////////////////////////////////////////

void SetupTimezone ( bool force )
{
    if ( force || timezone_adjust_sec == -1 )
    {
	timezone_adjust_sec  = GetTimezoneAdjust(GetTimeSec(false));
	timezone_adjust_usec = USEC_PER_SEC * timezone_adjust_sec;
	timezone_adjust_nsec = NSEC_PER_SEC * timezone_adjust_sec;
	TRACE("TZ: %s,%s, %ld, %d => %lld %lld %lld\n",
		tzname[0], tzname[1], timezone, daylight,
		timezone_adjust_sec, timezone_adjust_usec, timezone_adjust_nsec );
    }
}

///////////////////////////////////////////////////////////////////////////////

int GetTimezoneAdjust ( time_t tim )
{
    struct tm gmt, loc;
    gmtime_r(&tim,&gmt);
    localtime_r(&tim,&loc);
    int delta = ( gmt.tm_hour - loc.tm_hour ) * 3600
	      + ( gmt.tm_min  - loc.tm_min  ) *   60
	      + ( gmt.tm_sec  - loc.tm_sec  );
    if ( gmt.tm_yday != loc.tm_yday )
    {
	if ( gmt.tm_year < loc.tm_year
	    || gmt.tm_year == loc.tm_year && gmt.tm_yday < loc.tm_yday )
	{
	    delta -= 24*3600;
	}
	else
	{
	    delta += 24*3600;
	}
    }
    return delta;
}

///////////////////////////////////////////////////////////////////////////////

static u_sec_t AdjustLocalTime ( u_sec_t sec )
{
    static uint last_hour = 0;
    const uint hour = sec / 3600;
    if ( last_hour != hour )
    {
	last_hour = hour;
	SetupTimezone(true);
    }
    return sec - timezone_adjust_sec;
}

//-----------------------------------------------------------------------------

struct timeval GetTimeOfDay ( bool localtime )
{
    struct timeval tval;
    gettimeofday(&tval,NULL);
    if (localtime)
	tval.tv_sec = AdjustLocalTime(tval.tv_sec);
    return tval;
}

//-----------------------------------------------------------------------------

struct timespec GetClockTime ( bool localtime )
{
    struct timespec ts;

 #if HAVE_CLOCK_GETTIME
    if (!clock_gettime(CLOCK_REALTIME,&ts))
    {
	if (localtime)
	    ts.tv_sec = AdjustLocalTime(ts.tv_sec);
	return ts;
    }
 #endif

    // fall back
    struct timeval tval = GetTimeOfDay(localtime);
    ts.tv_sec  = tval.tv_sec;
    ts.tv_nsec = tval.tv_usec * NSEC_PER_USEC;
    return ts;
}

//-----------------------------------------------------------------------------

struct timespec GetClockTimer(void)
{
    struct timespec ts;

 #if HAVE_CLOCK_GETTIME
    if (!clock_gettime(CLOCK_MONOTONIC,&ts))
	return ts;
 #endif

    // fall back
    struct timeval tval = GetTimeOfDay(false);
    ts.tv_sec  = tval.tv_sec;
    ts.tv_nsec = tval.tv_usec * NSEC_PER_USEC;
    return ts;
}

///////////////////////////////////////////////////////////////////////////////

DayTime_t GetDayTime ( bool localtime )
{
    div_t d;
    DayTime_t dt;

 #if HAVE_CLOCK_GETTIME
    struct timespec tim	= GetClockTime(localtime);
    dt.usec	= tim.tv_nsec / NSEC_PER_USEC;
    dt.nsec	= tim.tv_nsec;
 #else
    struct timeval tim = GetTimeOfDay(localtime);
    dt.usec	= tim.tv_usec;
    dt.nsec	= tim.tv_usec * NSEC_PER_USEC;
 #endif

    dt.time	= tim.tv_sec;
    d		= div(tim.tv_sec,86400);
    dt.day	= d.quot;
    d		= div(d.rem,3600);
    dt.hour	= d.quot;
    d		= div(d.rem,60);
    dt.min	= d.quot;
    dt.sec	= d.rem;

    return dt;
}

///////////////////////////////////////////////////////////////////////////////

u_sec_t GetTimeSec ( bool localtime )
{
    struct timeval tval = GetTimeOfDay(localtime);
    return tval.tv_sec;
}

///////////////////////////////////////////////////////////////////////////////

u_msec_t GetTimeMSec ( bool localtime )
{
    struct timeval tval = GetTimeOfDay(localtime);
    return (u_msec_t)(tval.tv_sec) * MSEC_PER_SEC + tval.tv_usec/USEC_PER_MSEC;
}

///////////////////////////////////////////////////////////////////////////////

u_usec_t GetTimeUSec ( bool localtime )
{
    struct timeval tval = GetTimeOfDay(localtime);
    return (u_usec_t)(tval.tv_sec) * USEC_PER_SEC + tval.tv_usec;
}

///////////////////////////////////////////////////////////////////////////////

u_nsec_t GetTimeNSec ( bool localtime )
{
    struct timespec ts = GetClockTime(localtime);
    return (u_nsec_t)(ts.tv_sec) * NSEC_PER_SEC + ts.tv_nsec;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

CurrentTime_t current_time = {0};

CurrentTime_t GetCurrentTime ( bool localtime )
{
    struct timeval tval = GetTimeOfDay(localtime);

    CurrentTime_t ct;
    ct.sec  = tval.tv_sec;
    ct.usec = tval.tv_sec * USEC_PER_SEC + tval.tv_usec;
    ct.msec = ct.usec / 1000;
    return ct;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    timer			///////////////
///////////////////////////////////////////////////////////////////////////////

u_sec_t GetTimerSec()
{
    struct timespec ts = GetClockTimer();
    return ts.tv_sec;
}

///////////////////////////////////////////////////////////////////////////////

u_msec_t GetTimerMSec()
{
    struct timespec ts = GetClockTimer();
    return ts.tv_sec * MSEC_PER_SEC + ts.tv_nsec / NSEC_PER_MSEC;
}

///////////////////////////////////////////////////////////////////////////////

u_usec_t GetTimerUSec()
{
    struct timespec ts = GetClockTimer();
    return ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;
}

///////////////////////////////////////////////////////////////////////////////

u_nsec_t GetTimerNSec()
{
    struct timespec ts = GetClockTimer();
    return ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintRefTime
(

    u_sec_t		tim,		// seconds since epoch; 0 is replaced by time()
    u_sec_t		*ref_tim	// NULL or ref_tim; if *ref_tim==0: write current time
					// it is used to select format
)
{
    u_sec_t ref;
    if (!ref_tim)
	ref = GetTimeSec(false);
    else
    {
	if (!*ref_tim)
	    *ref_tim = GetTimeSec(false);
	ref = *ref_tim;
    }

    ccp format = tim >= ref - 10*SEC_PER_HOUR && tim <= ref + 10*SEC_PER_HOUR
	? "  %k:%M:%S"
	: tim >= ref - 12*SEC_PER_DAY && tim <= ref + 10*SEC_PER_DAY
	? " %e. %k:%M"
	: "%F";
    return PrintTimeByFormat(format,tim);
}

//-----------------------------------------------------------------------------

ccp PrintRefTimeUTC
(

    u_sec_t		tim,		// seconds since epoch; 0 is replaced by time()
    u_sec_t		*ref_tim	// NULL or ref_tim; if *ref_tim==0: write current time
					// it is used to select format
)
{
    u_sec_t ref;
    if (!ref_tim)
	ref = GetTimeSec(false);
    else
    {
	if (!*ref_tim)
	    *ref_tim = GetTimeSec(false);
	ref = *ref_tim;
    }

    ccp format = tim >= ref - 10*SEC_PER_HOUR && tim <= ref + 10*SEC_PER_HOUR
	? "  %k:%M:%S"
	: tim >= ref - 12*SEC_PER_DAY && tim <= ref + 10*SEC_PER_DAY
	? " %e. %k:%M"
	: "%F";
    return PrintTimeByFormatUTC(format,tim);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    u_sec_t		tim		// seconds since epoch; 0 is replaced by time()
)
{
    char buf[100];
    const time_t reftime = tim ? tim : time(0);
    struct tm *tm = localtime(&reftime);
    const uint len = strftime(buf,sizeof(buf),format,tm);
    return CopyCircBuf(buf,len+1);
}

//-----------------------------------------------------------------------------

ccp PrintTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    u_sec_t		tim		// seconds since epoch; 0 is replaced by time()
)
{
    char buf[100];
    const time_t reftime = tim ? tim : time(0);
    struct tm *tm = gmtime(&reftime);
    const uint len = strftime(buf,sizeof(buf),format,tm);
    return CopyCircBuf(buf,len+1);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintNSecByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'nsec'
    time_t		tim,		// seconds since epoch
    uint		nsec		// nanosecond of second
)
{
    char buf[100];
    struct tm *tm = localtime(&tim);
    const uint len = strftime(buf,sizeof(buf),format,tm);
    char *at = strchr(buf,'@');
    if (at)
    {
	char nbuf[10];
	snprintf(nbuf,sizeof(nbuf),"%09u",nsec);
	ccp src = nbuf;
	while ( *at == '@' && *src )
	    *at++ = *src++;
    }

    return CopyCircBuf(buf,len+1);
}

//-----------------------------------------------------------------------------

ccp PrintNSecByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'nsec'
    time_t		tim,		// seconds since epoch
    uint		nsec		// nanosecond of second
)
{
    char buf[100];
    struct tm *tm = gmtime(&tim);
    const uint len = strftime(buf,sizeof(buf),format,tm);
    char *at = strchr(buf,'@');
    if (at)
    {
	char nbuf[10];
	snprintf(nbuf,sizeof(nbuf),"%09u",nsec);
	ccp src = nbuf;
	while ( *at == '@' && *src )
	    *at++ = *src++;
    }

    return CopyCircBuf(buf,len+1);
}

//-----------------------------------------------------------------------------

ccp PrintNTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_nsec_t		ntime
)
{
    return PrintNSecByFormat(format,ntime/NSEC_PER_SEC,ntime%NSEC_PER_SEC);
}

//-----------------------------------------------------------------------------

ccp PrintNTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_nsec_t		ntime
)
{
    return PrintNSecByFormatUTC(format,ntime/NSEC_PER_SEC,ntime%NSEC_PER_SEC);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintUTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_usec_t		utime
)
{
    return PrintNSecByFormat(format,utime/USEC_PER_SEC,utime%USEC_PER_SEC*NSEC_PER_USEC);
}

//-----------------------------------------------------------------------------

ccp PrintUTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_usec_t		utime
)
{
    return PrintNSecByFormatUTC(format,utime/USEC_PER_SEC,utime%USEC_PER_SEC*NSEC_PER_USEC);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintMTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_msec_t		mtime
)
{
    return PrintNSecByFormat(format,mtime/MSEC_PER_SEC,mtime%MSEC_PER_SEC*NSEC_PER_MSEC);
}

//-----------------------------------------------------------------------------

ccp PrintMTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_msec_t		mtime
)
{
    return PrintNSecByFormatUTC(format,mtime/MSEC_PER_SEC,mtime%MSEC_PER_SEC*NSEC_PER_MSEC);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimevalByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timeval *tv		// time to print, if NULL use gettimeofday()
)
{
    struct timeval temp;
    if (!tv)
    {
	gettimeofday(&temp,0);
	tv = &temp;
    }

    return PrintUSecByFormat(format,tv->tv_sec,tv->tv_usec);
}

//-----------------------------------------------------------------------------

ccp PrintTimevalByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timeval *tv		// time to print, if NULL use gettimeofday()
)
{
    struct timeval temp;
    if (!tv)
    {
	gettimeofday(&temp,0);
	tv = &temp;
    }

    return PrintUSecByFormatUTC(format,tv->tv_sec,tv->tv_usec);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimespecByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timespec *ts		// time to print, if NULL use GetClockTime(false)
)
{
    struct timespec temp;
    if (!ts)
    {
	temp = GetClockTime(true);
	ts = &temp;
    }

    return PrintNSecByFormat(format,ts->tv_sec,ts->tv_nsec);
}

//-----------------------------------------------------------------------------

ccp PrintTimespecByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timespec *ts		// time to print, if NULL use GetClockTime(false)
)
{
    struct timespec temp;
    if (!ts)
    {
	temp = GetClockTime(false);
	ts = &temp;
    }

    return PrintNSecByFormatUTC(format,ts->tv_sec,ts->tv_nsec);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintTimeSec
(
    char		* buf,		// result buffer (>19 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    uint		sec		// seconds (=time) to print
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    time_t time = sec;
    struct tm *tm = localtime(&time);
    strftime(buf,buf_size,"%F %T",tm);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimeMSec
(
    char		* buf,		// result buffer (>23 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// milliseconds to print
    uint		fraction	// number of digits (0-3) to print as fraction
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 24 );

    time_t time = msec / MSEC_PER_SEC;
    struct tm *tm = localtime(&time);
    uint len = strftime(buf,buf_size,"%F %T",tm);

    if ( fraction && len + 4 < buf_size )
    {
	len += snprintf(buf+len, buf_size-len, ".%03llu", msec % MSEC_PER_SEC );
	const uint pos = len - 3 + ( fraction < 3 ? fraction : 3 );
	if ( pos < buf_size )
	    buf[pos] = 0;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimeUSec
(
    char		* buf,		// result buffer (>26 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    uint		fraction	// number of digits (0-6) to print as fraction
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 28 );

    time_t time = usec / USEC_PER_SEC;
    struct tm *tm = localtime(&time);
    uint len = strftime(buf,buf_size,"%F %T",tm);

    if ( fraction && len + 7 < buf_size )
    {
	len += snprintf(buf+len, buf_size-len, ".%06llu", usec % USEC_PER_SEC );
	const uint pos = len - 6 + ( fraction < 6 ? fraction : 6 );
	if ( pos < buf_size )
	    buf[pos] = 0;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimeNSec
(
    char		* buf,		// result buffer (>26 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_nsec_t		nsec,		// nanoseconds to print
    uint		fraction	// number of digits (0-9) to print as fraction
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 32 );

    time_t time = nsec / NSEC_PER_SEC;
    struct tm *tm = localtime(&time);
    uint len = strftime(buf,buf_size,"%F %T",tm);

    if ( fraction && len + 10 < buf_size )
    {
	len += snprintf(buf+len, buf_size-len, ".%09llu", nsec % NSEC_PER_SEC );
	const uint pos = len - 9 + ( fraction < 9 ? fraction : 9 );
	if ( pos < buf_size )
	    buf[pos] = 0;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintTimerMSec
(
    char		* buf,		// result buffer (>12 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			msec,		// milliseconds to print
    uint		fraction	// number of digits (0-3) to print as fraction
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 16 );

    if (fraction)
    {
	const uint len = snprintf(buf,buf_size,"%02d:%02d:%02d.%03d",
		msec/3600000, msec/60000%60, msec/1000%60, msec%1000 );
	if ( len > 3 && fraction < 3 )
	{
	    const uint pos = len - 3 + fraction;
	    if ( pos < buf_size )
		buf[pos] = 0;
	}
    }
    else
	snprintf(buf,buf_size,"%02d:%02d:%02d",
	    msec/3600000, msec/60000%60, msec/1000%60 );

    ccp ptr = buf;
    int colon_counter = 0;
    while ( *ptr == '0' || *ptr == ':' && !colon_counter++ )
	ptr++;
    return *ptr == ':' ? ptr-1 : ptr;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimerUSec
(
    char		* buf,		// result buffer (>16 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    uint		fraction	// number of digits (0-6) to print as fraction
)
{
    if ( !buf || buf_size < 4 )
	buf = GetCircBuf( buf_size = 20 );

    const bool minus = usec < 0;
    if (minus)
	usec = -usec;

    u32 sec = usec / USEC_PER_SEC;

    if (fraction)
    {
	const uint len = snprintf(buf+1,buf_size-1,"%02d:%02d:%02d.%06lld",
	    sec/3600, sec/60%60, sec%60, (u64)usec%USEC_PER_SEC );
	if ( len > 6 && fraction < 6 )
	{
	    const uint pos = len - 6 + fraction;
	    if ( pos < buf_size )
		buf[pos] = 0;
	}
    }
    else
	snprintf(buf+1,buf_size-1,"%02d:%02d:%02d",
	    sec/3600, sec/60%60, sec%60 );

    char *ptr = buf+1;
    while ( *ptr == '0' || *ptr == ':' )
	ptr++;
    if ( *ptr == '.' )
	ptr--;
    if (minus)
	*--ptr = '-';
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimerNSec
(
    char		* buf,		// result buffer (>16 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_nsec_t		nsec,		// nanoseconds to print
    uint		fraction	// number of digits (0-9) to print as fraction
)
{
    if ( !buf || buf_size < 4 )
	buf = GetCircBuf( buf_size = 24 );

    const bool minus = nsec < 0;
    if (minus)
	nsec = -nsec;

    u32 sec = nsec / NSEC_PER_SEC;

    if (fraction)
    {
	const uint len = snprintf(buf+1,buf_size-1,"%02d:%02d:%02d.%09lld",
	    sec/3600, sec/60%60, sec%60, (u64)nsec%NSEC_PER_SEC );
	if ( len > 9 && fraction < 9 )
	{
	    const uint pos = len - 9 + fraction;
	    if ( pos < buf_size )
		buf[pos] = 0;
	}
    }
    else
	snprintf(buf+1,buf_size-1,"%02d:%02d:%02d",
	    sec/3600, sec/60%60, sec%60 );

    char *ptr = buf+1;
    while ( *ptr == '0' || *ptr == ':' )
	ptr++;
    if ( *ptr == '.' )
	ptr--;
    if (minus)
	*--ptr = '-';
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// [[nsec]] -> PrintTimer3U(), PrintTimer3N()

ccp PrintTimer3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction,
					// otherwise suppress ms/us output
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 4 );
    const bool aligned = mode & DC_SFORM_ALIGN;

    if ( !sec && !usec && mode & DC_SFORM_DASH )
	StringCopyS(buf,buf_size,"  -");
    else if ( !sec && usec >= 0 && usec <= 999999 )
    {
	if (!usec)
	    StringCopyS(buf,buf_size,aligned?"  0":"0");
	else if ( usec < 100 )
	    snprintf(buf,buf_size,aligned?"%2uu":"%uu",usec);
	else if ( usec < 1000 )
	    snprintf(buf,buf_size,".%ui",usec/100);
	else if ( usec < 100000 )
	    snprintf(buf,buf_size,aligned?"%2ui":"%ui",usec/1000);
	else
	    snprintf(buf,buf_size,".%us",usec/100000);
    }
    else if ( sec < 100 )
	snprintf(buf,buf_size,aligned?"%2llus":"%llus",sec);
    else if ( sec < 6000 )
	snprintf(buf,buf_size,aligned?"%2llum":"%llum",sec/60);
    else if ( sec < 360000 )
	snprintf(buf,buf_size,aligned?"%2lluh":"%lluh",sec/3600);
    else
    {
	const u64 days = sec / (24*3600);
	if ( days < 100 )
	    snprintf(buf,buf_size,aligned?"%2llud":"%llud",days);
	else if ( days < 700 )
	    snprintf(buf,buf_size,aligned?"%2lluw":"%lluw",days/7);
	else
	{
	    const u32 years = days / 365;
	    if ( years < 100 )
		snprintf(buf,buf_size,aligned?"%2uy":"%uy",years);
	    else
		buf = "***";
	}
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

// [[nsec]] -> PrintTimer4U(), PrintTimer4N()

ccp PrintTimer4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction,
					// otherwise suppress ms/us output
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 5 );
    const bool aligned = mode & DC_SFORM_ALIGN;

    if ( !sec && !usec && mode & DC_SFORM_DASH )
	StringCopyS(buf,buf_size,"   -");
    else if ( sec < 10 && usec >= 0 && usec <= 999999 )
    {
	if (sec)
	    snprintf(buf,buf_size,"%llu.%us",sec,usec/100000);
	else if (!usec)
	    StringCopyS(buf,buf_size,aligned?"   0":"0");
	else if ( usec < 1000 )
	    snprintf(buf,buf_size,aligned?"%3uu":"%uu",usec);
	else if ( usec < 10000 )
	    snprintf(buf,buf_size,"%u.%ui",usec/1000,usec/100%10);
	else
	    snprintf(buf,buf_size,aligned?"%3ui":"%ui",usec/1000);
    }
    else if ( sec < 600 )
	snprintf(buf,buf_size,aligned?"%3llus":"%llus",sec);
    else if ( sec < 36000 )
	snprintf(buf,buf_size,aligned?"%3llum":"%llum",sec/60);
    else if ( sec < 168*3600 )
	snprintf(buf,buf_size,aligned?"%3lluh":"%lluh",sec/3600);
    else
    {
	const u64 days = sec / (24*3600);
	if ( days < 365 )
	    snprintf(buf,buf_size,aligned?"%3llud":"%llud",days);
	else if ( days < 10*365 )
	    snprintf(buf,buf_size,aligned?"%3lluw":"%lluw",days/7);
	else
	{
	    const u32 years = days / 365;
	    if ( years < 1000 )
		snprintf(buf,buf_size,aligned?"%3uy":"%uy",years);
	    else
		buf = "****";
	}
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimer6N
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			nsec,		// 0...999999999: nsec fraction,
					//    otherwise suppress ms/us/ns output
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_CENTER | DC_SFORM_DASH
)
{
    if ( mode & DC_SFORM_CENTER )
    {
	char buf2[20];
	PrintTimer6N(buf2,sizeof(buf2),sec,nsec,mode&~(DC_SFORM_CENTER|DC_SFORM_ALIGN));
	return StringCenterS(buf,buf_size,buf2,6);
    }

    char temp[20];
    if (!buf)
    {
	buf = temp;
	buf_size = sizeof(temp);
    }

    const bool nsec_valid = nsec >= 0 && nsec <= 999999999;

    if ( mode & DC_SFORM_ALIGN )
    {
	if ( !sec && !nsec && mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size,"     -");
	else if ( sec < 10 && nsec_valid )
	{
	    u_nsec_t sum = sec *NSEC_PER_SEC + nsec;
	    if (!sum)
		StringCopyS(buf,buf_size,"     0");
	    else if ( sum < 10000 )
		snprintf(buf,buf_size,"%4lluns",sum);
	    else if ( sum < 10000000 )
		snprintf(buf,buf_size,"%4lluus",sum/1000);
	    else
		snprintf(buf,buf_size,"%4llums",sum/1000000);
	}
	else if ( sec < 100 )
	{
	    if (nsec_valid )
		snprintf(buf,buf_size,"%2llu.%02us",sec,nsec/10000000);
	    else
		snprintf(buf,buf_size,"%5llus",sec);
	}
	else if ( sec < 100*60 )
	    snprintf(buf,buf_size,"%2llum%02llus",sec/60,sec%60);
	else if ( sec < 100*3600 )
	    snprintf(buf,buf_size,"%2lluh%02llum",sec/3600,sec/60%60);
	else
	{
	    const u64 days = sec / (24*3600);
	    if ( days < 100 )
		snprintf(buf,buf_size,"%2llud%02lluh",days,sec/3600%24);
	    else if ( days < 7000 )
		snprintf(buf,buf_size,"%3lluw%llud",days/7,days%7);
	    else
	    {
		const u32 years = days / 365;
		if ( years < 100 )
		    snprintf(buf,buf_size,"%2uy%02lluw",years,days%365/7);
		else if ( years < 100000 )
		    snprintf(buf,buf_size,"%5uy",years);
		else
		    buf = "******";
	    }
	}
    }
    else
    {
	if ( !sec && !nsec && mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size,"-");
	else if ( sec < 10 && nsec_valid )
	{
	    u_nsec_t sum = sec *NSEC_PER_SEC + nsec;
	    if (!sum)
		StringCopyS(buf,buf_size,"0");
	    else if ( sum < 10000 )
		snprintf(buf,buf_size,"%lluns",sum);
	    else if ( sum < 10000000 )
		snprintf(buf,buf_size,"%lluus",sum/1000);
	    else
		snprintf(buf,buf_size,"%llums",sum/1000000);
	}
	else if ( sec < 100 )
	{
	    if ( nsec_valid )
	    {
		const uint val2 = nsec/10000000;
		if (val2)
		    snprintf(buf,buf_size,"%llu.%02us",sec,val2);
		else
		    snprintf(buf,buf_size,"%llus",sec);
	    }
	    else
		snprintf(buf,buf_size,"%llus",sec);
	}
	else if ( sec < 100*60 )
	{
	    const uint val2 = sec%60;
	    if (val2)
		snprintf(buf,buf_size,"%llum%02us",sec/60,val2);
	    else
		snprintf(buf,buf_size,"%llum",sec/60);
	}
	else if ( sec < 100*3600 )
	{
	    const uint val2 = sec/60%60;
	    if (val2)
		snprintf(buf,buf_size,"%lluh%02um",sec/3600,val2);
	    else
		snprintf(buf,buf_size,"%lluh",sec/3600);
	}
	else
	{
	    const u64 days = sec / (24*3600);
	    if ( days < 100 )
	    {
		const uint val2 = sec/3600%24;
		if (val2)
		    snprintf(buf,buf_size,"%llud%02uh",days,val2);
		else
		    snprintf(buf,buf_size,"%llud",days);
	    }
	    else if ( days < 7000 )
	    {
		const uint val2 = days%7;
		if (val2)
		    snprintf(buf,buf_size,"%lluw%ud",days/7,val2);
		else
		    snprintf(buf,buf_size,"%lluw",days/7);
	    }
	    else
	    {
		const u32 years = days / 365;
		if ( years < 100 )
		{
		    const uint val2 = days%365/7;
		    if (val2)
			snprintf(buf,buf_size,"%uy%02uw",years,val2);
		    else
			snprintf(buf,buf_size,"%uy",years);
		}
		else if ( years < 100000 )
		    snprintf(buf,buf_size,"%uy",years);
		else
		    buf = "******";
	    }
	}
    }

    return buf == temp ? CopyCircBuf0(buf,strlen(buf)) : buf;
}

///////////////////////////////////////////////////////////////////////////////

// [[nsec]] -> PrintTimer4Us(), PrintTimer4Ns()

ccp PrintTimerUSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    if ( !buf || buf_size < 2 )
	buf = GetCircBuf( buf_size = 5 );

    if (!usec)
    {
	if ( mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, mode & DC_SFORM_ALIGN ? "   -" : "-" );
	else
	    StringCopyS(buf,buf_size, mode & DC_SFORM_ALIGN ? "   0" : "0" );
    }
    else
    {
	char sign;
	if ( usec < 0 )
	{
	    usec = -usec;
	    sign = '-';
	}
	else
	    sign = mode & DC_SFORM_PLUS ? '+' : ' ';

	PrintTimerUSec3(buf+1,buf_size-1,usec,mode&~DC_SFORM_PLUS);
	*buf = ' ';

	char *ptr = buf+1;
	while ( *ptr == ' ' )
	    ptr++;
	ptr[-1] = sign;
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimerUSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    if ( !buf || buf_size < 2 )
	buf = GetCircBuf( buf_size = 8 );

    if (!usec)
    {
	if ( mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, mode & DC_SFORM_ALIGN ? "      -" : "-" );
	else
	    StringCopyS(buf,buf_size, mode & DC_SFORM_ALIGN ? "      0" : "0" );
    }
    else
    {
	char sign;
	if ( usec < 0 )
	{
	    usec = -usec;
	    sign = '-';
	}
	else
	    sign = mode & DC_SFORM_PLUS ? '+' : ' ';

	PrintTimerUSec6(buf+1,buf_size-1,usec, mode & DC_SFORM_ALIGN );
	*buf = ' ';

	char *ptr = buf+1;
	while ( *ptr == ' ' )
	    ptr++;
	ptr[-1] = sign;
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimerNSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_nsec_t		nsec,		// nansoseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    if ( !buf || buf_size < 2 )
	buf = GetCircBuf( buf_size = 8 );

    if (!nsec)
    {
	if ( mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, mode & DC_SFORM_ALIGN ? "      -" : "-" );
	else
	    StringCopyS(buf,buf_size, mode & DC_SFORM_ALIGN ? "      0" : "0" );
    }
    else
    {
	char sign;
	if ( nsec < 0 )
	{
	    nsec = -nsec;
	    sign = '-';
	}
	else
	    sign = mode & DC_SFORM_PLUS ? '+' : ' ';

	PrintTimerNSec6(buf+1,buf_size-1, nsec, mode & DC_SFORM_ALIGN );
	*buf = ' ';

	char *ptr = buf+1;
	while ( *ptr == ' ' )
	    ptr++;
	ptr[-1] = sign;
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintAge3Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
)
{
    if (!time)
	return "  -";

    if (!now)
	now = GetTimeSec(false);

    now -= time;
    return now < 0 ? "  +" : PrintTimer3(0,0,now,-1,DC_SFORM_ALIGN);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintAge4Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
)
{
    if (!time)
	return "   -";

    if (!now)
	now = GetTimeSec(false);

    now -= time;
    return now < 0
	? PrintTimerSec4s(0,0,now,DC_SFORM_ALIGN)
	: PrintTimer4(0,0,now,-1,DC_SFORM_ALIGN);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintAge6Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
)
{
    if (!time)
	return "     -";

    if (!now)
	now = GetTimeSec(false);

    now -= time;
    return now < 0 ? "     +" : PrintTimerSec6(0,0,now,DC_SFORM_ALIGN);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintAge7Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
)
{
    if (!time)
	return "      -";

    if (!now)
	now = GetTimeSec(false);

    return PrintTimerSec7s(0,0,now-time,DC_SFORM_ALIGN);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan date & time		///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanIntervalTS
(
    struct timespec *res_ts,	// not NULL: store result here
    ccp		source		// source text
)
{
    uint days = 0, sec = 0, nsec = 0;
    bool day_possible = true;
    uint colon_count = 0;
    char *src = source ? (char*)source : "";

    while ( *src > 0 && *src <= ' ' )
	src++;

    for(;;)
    {
	if ( *src < '0' || *src >= '9' )
	    break;
	ulong num = strtoul(src,&src,10);
	if ( *src == 'd' && day_possible )
	{
	    src++;
	    days = num;
	    day_possible = false;
	}
	else if ( *src == ':' && colon_count < 2 )
	{
	    src++;
	    colon_count++;
	    sec = ( sec + num ) * 60;
	}
	else
	{
	    sec += num;
	    if ( *src == '.' )
	    {
		src++;
		uint i;
		for ( i = 0; i < 9; i++ )
		{
		    nsec *= 10;
		    if ( *src >= '0' && *src <= '9' )
			nsec += *src++ - '0';
		}
		while ( *src >= '0' && *src <= '9' )
		    src++;
	    }
	    else if ( colon_count == 1 )
		sec *= 60;
	}
    }

    if (res_ts)
    {
	res_ts->tv_sec  = days * SEC_PER_DAY + sec;
	res_ts->tv_nsec = nsec;
    }

    return src;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanIntervalUSec
(
    u_usec_t	*res_usec,	// not NULL: store total microseconds
    ccp		source		// source text
)
{
    struct timespec ts;
    char *res = ScanIntervalTS(&ts,source);

    if (res_usec)
	*res_usec = ts.tv_sec * USEC_PER_SEC + ts.tv_nsec / NSEC_PER_USEC;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanIntervalNSec
(
    u_nsec_t	*res_nsec,	// not NULL: store total nanoseconds
    ccp		source		// source text
)
{
    struct timespec ts;
    char *res = ScanIntervalTS(&ts,source);

    if (res_nsec)
	*res_nsec = ts.tv_sec * NSEC_PER_SEC + ts.tv_nsec;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanInterval
(
    time_t	*res_time,	// not NULL: store seconds
    u32		*res_usec,	// not NULL: store microseconds of second
    ccp		source		// source text
)
{
    struct timespec ts;
    char *res = ScanIntervalTS(&ts,source);

    if (res_time)
	*res_time = ts.tv_sec;
    if (res_usec)
	*res_usec = ts.tv_nsec / NSEC_PER_USEC;

    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanInterval64
(
    u64		*res_usec,	// not NULL: store total microseconds
    ccp		source		// source text
)
{
    u_usec_t usec;
    char *res = ScanIntervalUSec(&usec,source);
    if (res_usec)
	*res_usec = usec;
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// [[nsec]]

char * ScanDateTime
(
    time_t	*res_time,	// not NULL: store seconds here
    u32		*res_usec,	// not NULL: store useconds of second here
    ccp		source,		// source text
    bool	allow_delta	// true: allow +|- interval
)
{
    //--- scan date

    time_t total_time = 0;
    int total_usec = 0;
    ccp src = source ? source : "";
    while ( *src > 0 && *src <= ' ' )
	src++;

    struct tm tm;
    memset(&tm,0,sizeof(tm));
    src = strptime(src,"%Y-%m-%d",&tm);

    noPRINT(" %4u-%02u-%02u %2u:%02u:%02u |%s|\n",
	tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
	tm.tm_hour, tm.tm_min, tm.tm_sec, src );

    if (src)
	total_time = mktime(&tm);
    else
	src = source ? source : "";
    noPRINT_TIME(total_time,"DATE");


    //--- scan time

    time_t time;
    u32 usec;
    src = ScanInterval(&time,&usec,src);
    total_time += time;
    total_usec += usec;
    noPRINT_TIME(total_time,"TIME");


    //--- scan timezone

    ccp src2 = src;
    while ( *src2 > 0 && *src2 <= ' ' )
	src2++;
    if ( ( src2[0] == '-' || src2[0] == '+' )
	&& src2[1] >= '0' && src2[1] <= '9'
	&& src2[2] >= '0' && src2[2] <= '9'
	&& src2[3] >= '0' && src2[3] <= '9'
	&& src2[4] >= '0' && src2[4] <= '9'
	&& ( src2[5] < '0' || src2[5] > '9' ))
    {
	int delta = 36000 * src2[1]
		  +  3600 * src2[2]
		  +   600 * src2[3]
		  +    60 * src2[4]
		  - 40260 * '0';
	if ( src2[0] == '-' )
	    delta = -delta;

	noPRINT("GOT TIMEZONE: %.5s => %d\n",src2,delta);
	total_time -= delta + timezone;
	src = src2 + 5;
    }
    else
    {
	PRINT("timezone=%ld\n",timezone);
	struct tm *tm = localtime(&total_time);
	if (tm->tm_isdst)
	    total_time -= 3600;
    }
    noPRINT_TIME(total_time,"ZONE");


    //--- scan delta

    if (allow_delta)
    {
	ccp src2 = src;
	while ( *src2 > 0 && *src2 <= ' ' )
	    src2++;
	bool minus = *src2 == '-';
	if ( minus || *src2 == '+' )
	{
	    src = ScanInterval(&time,&usec,src2+1);
	    if (minus)
	    {
		total_time -= time;
		total_usec -= usec;
	    }
	    else
	    {
		total_time += time;
		total_usec += usec;
	    }
	}
    }

    if ( total_usec < 0 )
    {
	total_time -= ( 999999 - total_usec ) / 1000000;
	total_usec  = 1000000 - (-total_usec) % 1000000;
    }
    else if ( total_usec >= 1000000 )
    {
	total_time += total_usec / 1000000;
	total_usec %= 1000000;
    }

    if (res_time)
	*res_time = total_time;
    if (res_usec)
	*res_usec = total_usec;
    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

// [[nsec]]

char * ScanDateTime64
(
    u64		*res_usec,	// not NULL: store total microseconds
    ccp		source,		// source text
    bool	allow_delta	// true: allow +|- interval
)
{
    time_t time;
    u32 usec;
    char *src = ScanDateTime(&time,&usec,source,allow_delta);
    if (res_usec)
	*res_usec = time * 1000000ull + usec;
    return src;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Since 2001			///////////////
///////////////////////////////////////////////////////////////////////////////
// hour support

ccp hour2text
(
	int hour,	// hour to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use '-'
	ccp sep_dt	// NULL or separator of between date and time
			// if NULL: use ' '
)
{
    time_t tim = hour2time(hour);
    struct tm tm;
    gmtime_r(&tim,&tm);

    if (!sep_date)
	sep_date = "-";

    if (!sep_dt)
	sep_dt = " ";

    return fw >= 13
	? PrintCircBuf("%04d%s%02u%s%02u%s%02u",
		tm.tm_year + 1900, sep_date,
		tm.tm_mon  +    1, sep_date,
		tm.tm_mday, sep_dt,
		tm.tm_hour
		)
	: fw >= 11
	? PrintCircBuf("%02d%s%02u%s%02u%s%02u",
		tm.tm_year % 100, sep_date,
		tm.tm_mon  +   1, sep_date,
		tm.tm_mday, sep_dt,
		tm.tm_hour
		)
	: fw >= 10
	? PrintCircBuf("%2u.%s%02u:xx",
		tm.tm_mday, sep_dt,
		tm.tm_hour
		)
	: PrintCircBuf("%02u%s%02u%s%02u",
		tm.tm_mon  +   1, sep_date,
		tm.tm_mday, sep_dt,
		tm.tm_hour
		);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// day support

ccp day2text
(
	int day,	// day to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use '-'
	ccp sep_dt	// ignored
)
{
    time_t tim = day2time(day);
    struct tm tm;
    gmtime_r(&tim,&tm);

    if (!sep_date)
	sep_date = "-";

    return fw >= 10
	? PrintCircBuf("%04d%s%02u%s%02u",
		tm.tm_year + 1900, sep_date,
		tm.tm_mon  +    1, sep_date,
		tm.tm_mday )
	: PrintCircBuf("%02d%s%02u%s%02u",
		tm.tm_year % 100, sep_date,
		tm.tm_mon  +   1, sep_date,
		tm.tm_mday );
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// week support

ccp week2text
(
	int week,	// week to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use 'w'
	ccp sep_dt	// ignored
)
{
    time_t tim = week2time(week) + 4*86400;
    struct tm tm;
    gmtime_r(&tim,&tm);

    return PrintCircBuf("%04d%s%02u",
		tm.tm_year + 1900,
		sep_date ? sep_date : "w",
		tm.tm_yday / 7 + 1 );
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// month support

int time2month ( time_t tim )
{
    struct tm tm;
    gmtime_r(&tim,&tm);
    return ( tm.tm_mon + 12 * tm.tm_year ) - 101*12;
}

///////////////////////////////////////////////////////////////////////////

time_t month2time ( int month )
{
    SetupTimezone(false);
    struct tm tm = {0};

    tm.tm_isdst	= -1;
    tm.tm_year	= month / 12 + 101;
    tm.tm_mon	= month % 12;
    tm.tm_mday	= 1;

    int delta = 43200-timezone_adjust_sec;
    if ( delta < 0 )
    {
	delta += 86400;
	tm.tm_mday++;
    }

    tm.tm_hour	= delta / 3600;
    tm.tm_min	= delta / 60 % 60;
    return mktime(&tm) / 86400 * 86400;
}

///////////////////////////////////////////////////////////////////////////

ccp month2text
(
	int month,	// month to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use '-'
	ccp sep_dt	// ignored
)
{
    month += 2001*12;
    return PrintCircBuf("%4d%s%02u",
		month/12,
		sep_date ? sep_date : "-",
		month%12 + 1 );
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// quarter support

int time2quarter ( time_t tim )
{
    struct tm tm;
    gmtime_r(&tim,&tm);
    return ( tm.tm_mon/3 + 4 * tm.tm_year ) - 101*4;
}

///////////////////////////////////////////////////////////////////////////

time_t quarter2time ( int quarter )
{
    SetupTimezone(false);
    struct tm tm = {0};

    tm.tm_isdst	= -1;
    tm.tm_year	= quarter / 4 + 101;
    tm.tm_mon	= quarter % 4 * 3;
    tm.tm_mday	= 1;

    int delta = 43200-timezone_adjust_sec;
    if ( delta < 0 )
    {
	delta += 86400;
	tm.tm_mday++;
    }

    tm.tm_hour	= delta / 3600;
    tm.tm_min	= delta / 60 % 60;
    return mktime(&tm) / 86400 * 86400;
}

///////////////////////////////////////////////////////////////////////////

ccp quarter2text
(
	int quarter,	// quarter to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use 'q'
	ccp sep_dt	// ignored
)
{
    quarter += 2001*4;
    return PrintCircBuf("%4d%s%u",
		quarter/4,
		sep_date ? sep_date : "q",
		quarter%4 + 1 );
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// year support

int time2year ( time_t tim )
{
    struct tm tm;
    gmtime_r(&tim,&tm);
    return tm.tm_year + 1900;
}

///////////////////////////////////////////////////////////////////////////

time_t year2time ( int year )
{
    SetupTimezone(false);
    struct tm tm = {0};

    tm.tm_isdst	= -1;
    tm.tm_year	= year - 1900;
    tm.tm_mday	= 1;

    int delta = 43200-timezone_adjust_sec;
    if ( delta < 0 )
    {
	delta += 86400;
	tm.tm_mday++;
    }

    tm.tm_hour	= delta / 3600;
    tm.tm_min	= delta / 60 % 60;
    return mktime(&tm) / 86400 * 86400;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			usage counter			///////////////
///////////////////////////////////////////////////////////////////////////////

int usage_count_enabled = 0;

void EnableUsageCount()
{
    usage_count_enabled++;
    UpdateUsageByMgr(0);
}

///////////////////////////////////////////////////////////////////////////////

const SaveRestoreTab_t SRT_UsageCount[] =
{
    #undef SRT_NAME
    #define SRT_NAME UsageCount_t

    DEF_SRT_UINT(	par.enabled,		"enabled" ),
    DEF_SRT_UINT(	par.used,		"used" ),
    DEF_SRT_ARRAY_A(entry),
      DEF_SRT_UINT(	entry[0].count,		"count" ),
      DEF_SRT_UINT(	entry[0].elapsed_nsec,	"elapsed" ),
      DEF_SRT_UINT(	entry[0].time_sec,	"time" ),
      DEF_SRT_FLOAT(	entry[0].top1s,		"top1s" ),
      DEF_SRT_FLOAT(	entry[0].top5s,		"top5s" ),
    DEF_SRT_ARRAY_END(),

    DEF_SRT_SEPARATOR(),
    DEF_SRT_TERM()
};

///////////////////////////////////////////////////////////////////////////////

void ClearUsageCount ( UsageCount_t *uc, int count )
{
    if (!uc)
	return;

    uint index = CheckIndex1(uc->par.used,count);
    if ( index >= uc->par.used - 1 )
	return;

    uc->par.used = index;
    if (!index)
    {
	const bool enabled = uc->par.enabled;
	memset( uc, 0, sizeof(*uc) );
	uc->par.enabled = enabled;
    }
    else
    {
	UsageCountEntry_t *e = uc->entry + index;
	memset( e, 0, PTR_DISTANCE((u8*)uc->entry+sizeof(uc->entry),e) );
    }
}

///////////////////////////////////////////////////////////////////////////////
// [[UpdateUsageCount]]

void UpdateUsageCount ( UsageCount_t *uc, u_nsec_t timer_nsec )
{
    if (!uc)
	return;

    if ( !uc->par.enabled || usage_count_enabled <= 0 )
    {
	uc->ref.elapsed_nsec = 0;
	uc->ref.count = 0;
	uc->par.dirty = false;
	return;
    }

    if ( !uc->par.is_active )
    {
	if ( !uc->par.used && !uc->ref.count )
	    return;
	uc->par.is_active = true;
    }

    UsageCountEntry_t cur = {0};
    cur.elapsed_nsec = timer_nsec ? timer_nsec : GetTimerNSec();

    uc->par.update_nsec = uc->ref.elapsed_nsec + NSEC_PER_SEC;
    if ( cur.elapsed_nsec >= uc->par.update_nsec )
    {
	// needed for the case that USAGE_COUNT_ENTRIES was reduced after restart!
	if ( uc->par.used > USAGE_COUNT_ENTRIES )
	     uc->par.used = USAGE_COUNT_ENTRIES;

	cur.time_sec = GetTimeSec(false);

	if (uc->ref.elapsed_nsec)
	{
	    //--- find index and move data

	    int index;
	    if ( uc->par.used < USAGE_COUNT_FIRST_ADD )
		index = uc->par.used + 1;
	    else
	    {
		u_nsec_t min = USAGE_COUNT_NEXT_MIN(NSEC_PER_SEC);
		for ( index = USAGE_COUNT_FIRST_ADD;
		      index < uc->par.used;
		      index++, min = USAGE_COUNT_NEXT_MIN(min)
		    )
		{
		    UsageCountEntry_t *e = uc->entry + index;
		    if ( e->elapsed_nsec < min )
		    {
			AddUsageCountEntry(e,e-1);
			index--;
			break;
		    }
		}
		if ( ++index > USAGE_COUNT_ENTRIES )
		    index = USAGE_COUNT_ENTRIES;
	    }

	    if ( index > 1 )
		memmove( uc->entry+1, uc->entry, (index-1) * sizeof(*uc->entry) );

	    if ( uc->par.used < index )
		 uc->par.used = index;


	    //--- update first entry

	    UsageCountEntry_t *e = uc->entry;
	    *e = uc->ref;
	    e->elapsed_nsec = cur.elapsed_nsec - e->elapsed_nsec;


	    //--- update top value (1s)

	    if (e->elapsed_nsec)
		e->top1s = (double)( NSEC_PER_SEC * e->count ) / e->elapsed_nsec;


	    //--- update top value (5s)

	    u_nsec_t elapsed5 = 0, count5 = 0;
	    UsageCountEntry_t *e5 = uc->entry;
	    for ( int i = 0; i < uc->par.used && elapsed5 < 5*NSEC_PER_SEC; i++, e5++ )
	    {
		elapsed5 += e5->elapsed_nsec;
		count5   += e5->count;
	    }

	    if (elapsed5)
	    {
		if ( count5 == e->count )
		    elapsed5 = 5*NSEC_PER_SEC;
		e->top5s = (double)( NSEC_PER_SEC * count5 ) / elapsed5;
	    }
	}

	uc->ref = cur;
	uc->par.update_nsec = uc->ref.elapsed_nsec + NSEC_PER_SEC;
    }

    uc->par.force_update_nsec = uc->ref.elapsed_nsec + NSEC_PER_MIN;
    uc->par.dirty = uc->ref.count > 0;
}

///////////////////////////////////////////////////////////////////////////////

void UpdateUsageCountIncrement ( UsageCount_t *uc )
{
    DASSERT(uc);
    uc->par.is_active = true;
    UpdateUsageCount(uc,0);
    if ( usage_count_enabled > 0 )
    {
	uc->ref.count++;
	uc->par.dirty = true;
    }
}

///////////////////////////////////////////////////////////////////////////////

void UpdateUsageCountAdd ( UsageCount_t *uc, int add )
{
    DASSERT(uc);
    if ( add > 0 )
    {
	uc->par.is_active = true;
	UpdateUsageCount(uc,0);
	if ( usage_count_enabled > 0 )
	{
	    uc->ref.count += add;
	    uc->par.dirty = true;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void AddUsageCountEntry ( UsageCountEntry_t *dest, const UsageCountEntry_t *add )
{
    DASSERT(dest);
    DASSERT(add);
    dest->count		+= add->count;
    dest->elapsed_nsec	+= add->elapsed_nsec;

    if ( !dest->time_sec || dest->time_sec > add->time_sec )
	dest->time_sec = add->time_sec;

    if ( dest->top1s < add->top1s )
	 dest->top1s = add->top1s;
    if ( dest->top5s < add->top5s )
	 dest->top5s = add->top5s;
}

///////////////////////////////////////////////////////////////////////////////

UsageCountEntry_t GetUsageCount ( const UsageCount_t *uc, s_nsec_t nsec )
{
    DASSERT(uc);
    UsageCountEntry_t res = {0};
    const UsageCountEntry_t *e = uc->entry;
    for ( int i = 0; i < uc->par.used && nsec > 0; i++, e++ )
    {
	nsec -= e->elapsed_nsec;
	AddUsageCountEntry(&res,e);
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

PointerList_t usage_count_mgr = { .used = 0, .grow = 10 };

///////////////////////////////////////////////////////////////////////////////

u_nsec_t MaintainUsageByMgr ( u_nsec_t now_nsec )
{
    // returns delta time to next maintenance based on GetTimerNSec()

    if (!now_nsec)
	now_nsec = GetTimerNSec();
    u_nsec_t next_nsec = now_nsec + NSEC_PER_MIN;

    for ( int i = 0; i < usage_count_mgr.used; i++ )
    {
	const UsageCtrl_t *uci = usage_count_mgr.list[i];
	if (!uci)
	    continue;

	const UsageParam_t *par = GetUsageParam(uci);
	if (par)
	{
	    if ( par->dirty && par->update_nsec && par->update_nsec <= now_nsec
		|| par->force_update_nsec && par->force_update_nsec <= now_nsec )
	    {
		UpdateByUCI(uci,now_nsec);
	    }
	    if ( par->dirty && par->update_nsec && next_nsec > par->update_nsec )
		next_nsec = par->update_nsec;
	}
    }

    return next_nsec;
}

///////////////////////////////////////////////////////////////////////////////

void UpdateByUCI ( const UsageCtrl_t *uci, u_nsec_t now_nsec )
{
    if (uci)
    {
	if (uci->update_func)
	    uci->update_func(uci,now_nsec);
	else if (uci->uc)
	    UpdateUsageCount(uci->uc,now_nsec);
	else if (uci->ud)
	    UpdateUsageDuration(uci->ud,now_nsec);
    }
}

///////////////////////////////////////////////////////////////////////////////

void UpdateUsageByMgr ( u_nsec_t timer_nsec )
{
    if (!timer_nsec)
	timer_nsec = GetTimerNSec();
    for ( int i = 0; i < usage_count_mgr.used; i++ )
    {
	const UsageCtrl_t *uci = usage_count_mgr.list[i];
	if (uci)
	{
	    if (uci->uc)
		UpdateUsageCount(uci->uc,timer_nsec);
	    else if (uci->ud)
		UpdateUsageDuration(uci->ud,timer_nsec);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

#if 0
 static int sort_usage_count ( const UsageCtrl_t *a, const UsageCtrl_t *b )
 {
    DASSERT( a && b );
    return strcmp(a->key1,b->key1);
 }
#endif

//-----------------------------------------------------------------------------

void AddToUsageMgr ( const UsageCtrl_t *uci )
{
    if ( !uci || !uci->key1 )
	return;

    // no duplicates!
    for ( int i = 0; i < usage_count_mgr.used; i++ )
    {
	const UsageCtrl_t *uci2 = usage_count_mgr.list[i];
	if ( uci2 && ( uci2->uc && uci2->uc == uci->uc || uci2->ud && uci2->ud == uci->ud ))
	    return;
    }

    AddToPointerMgr(&usage_count_mgr,uci);

 #if 0
    qsort( usage_count_mgr.list, usage_count_mgr.used,
		sizeof(*usage_count_mgr.list), (qsort_func)sort_usage_count );
 #endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateByUsageCountMgr ( FILE *f, uint fw_name )
{
    DASSERT(f);
    const u_nsec_t timer_nsec = GetTimerNSec();
    void **ucip = usage_count_mgr.list;
    for ( int i = 0; i < usage_count_mgr.used; i++, ucip++ )
    {
	UsageCtrl_t *uci = *ucip;
	if (uci)
	{
	    UpdateUsageCount(uci->uc,timer_nsec);
	    if (uci->srt_prefix)
	    {
		if (uci->uc)
		    SaveCurrentStateByTable(f,uci->uc,SRT_UsageCount,uci->srt_prefix,fw_name);
		else if (uci->ud)
		    SaveCurrentStateByTable(f,uci->ud,SRT_UsageDuration,uci->srt_prefix,fw_name);
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void RestoreStateByUsageCountMgr ( RestoreState_t *rs )
{
    DASSERT(rs);
    void **ucip = usage_count_mgr.list;
    for ( int i = 0; i < usage_count_mgr.used; i++, ucip++ )
    {
	UsageCtrl_t *uci = *ucip;
	if (uci)
	{
	    if (uci->uc)
	    {
		UsageCount_t *uc = uci->uc;
		if (uci->srt_load)
		    RestoreStateByTable(rs,uc,SRT_UsageCount,uci->srt_load);
		if (uci->srt_prefix)
		    RestoreStateByTable(rs,uc,SRT_UsageCount,uci->srt_prefix);

		// needed for the case that USAGE_COUNT_ENTRIES was reduced after restart!
		if ( uc->par.used > USAGE_COUNT_ENTRIES )
		     uc->par.used = USAGE_COUNT_ENTRIES;
	    }
	    else if (uci->ud)
	    {
		UsageDuration_t *ud = uci->ud;
		if (uci->srt_load)
		    RestoreStateByTable(rs,ud,SRT_UsageDuration,uci->srt_load);
		if (uci->srt_prefix)
		    RestoreStateByTable(rs,ud,SRT_UsageDuration,uci->srt_prefix);

		// needed for the case that USAGE_COUNT_ENTRIES was reduced after restart!
		if ( ud->par.used > USAGE_COUNT_ENTRIES )
		     ud->par.used = USAGE_COUNT_ENTRIES;
	    }
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wait counter			///////////////
///////////////////////////////////////////////////////////////////////////////

const SaveRestoreTab_t SRT_UsageDuration[] =
{
    #undef SRT_NAME
    #define SRT_NAME UsageDuration_t

    DEF_SRT_UINT(	par.enabled,		"enabled" ),
    DEF_SRT_UINT(	par.used,		"used" ),
    DEF_SRT_ARRAY_A(entry),
      DEF_SRT_UINT(	entry[0].elapsed_nsec,	"elapsed" ),
      DEF_SRT_UINT(	entry[0].total_nsec,	"total" ),
      DEF_SRT_UINT(	entry[0].top_nsec,	"top" ),
      DEF_SRT_UINT(	entry[0].count,		"count" ),
      DEF_SRT_UINT(	entry[0].time_sec,	"time" ),
      DEF_SRT_FLOAT(	entry[0].top_cnt_1s,	"top-c1" ),
      DEF_SRT_FLOAT(	entry[0].top_cnt_5s,	"top-c5" ),
    DEF_SRT_ARRAY_END(),

    DEF_SRT_SEPARATOR(),
    DEF_SRT_TERM()
};

///////////////////////////////////////////////////////////////////////////////

// n_rec<0: clear from end, n_rec>=0: clear from index
void ClearUsageDuration ( UsageDuration_t *ud, int count )
{
    if (!ud)
	return;

    uint index = CheckIndex1(ud->par.used,count);
    if ( index >= ud->par.used - 1 )
	return;

    ud->par.used = index;
    if (!index)
    {
	const bool enabled = ud->par.enabled;
	memset( ud, 0, sizeof(*ud) );
	ud->par.enabled = enabled;
    }
    else
    {
	UsageDurationEntry_t *e = ud->entry + index;
	memset( e, 0, PTR_DISTANCE((u8*)ud->entry+sizeof(ud->entry),e) );
    }
}

///////////////////////////////////////////////////////////////////////////////
// [[UpdateUsageDuration]]

// TIMER_NSEC: 0 or GetTimerNSec(), but not GetTime*()
void UpdateUsageDuration ( UsageDuration_t *ud, u_nsec_t timer_nsec )
{
    if (!ud)
	return;

    if ( !ud->par.enabled || usage_count_enabled <= 0 )
    {
	ud->ref.elapsed_nsec = 0;
	ud->ref.count = 0;
	ud->par.dirty = false;
	return;
    }

    if ( !ud->par.is_active )
    {
	if ( !ud->par.used && !ud->ref.count )
	    return;
	ud->par.is_active = true;
    }

    UsageDurationEntry_t cur = {0};
    cur.elapsed_nsec = timer_nsec ? timer_nsec : GetTimerNSec();

    ud->par.update_nsec = ud->ref.elapsed_nsec + NSEC_PER_SEC;
    if ( cur.elapsed_nsec >= ud->par.update_nsec )
    {
	// needed for the case that USAGE_COUNT_ENTRIES was reduced after restart!
	if ( ud->par.used > USAGE_COUNT_ENTRIES )
	     ud->par.used = USAGE_COUNT_ENTRIES;

	cur.time_sec = GetTimeSec(false);

	if (ud->ref.elapsed_nsec)
	{
	    //--- find index and move data

	    int index;
	    if ( ud->par.used < USAGE_COUNT_FIRST_ADD )
		index = ud->par.used + 1;
	    else
	    {
		u_nsec_t min = USAGE_COUNT_NEXT_MIN(NSEC_PER_SEC);
		for ( index = USAGE_COUNT_FIRST_ADD;
		      index < ud->par.used;
		      index++, min = USAGE_COUNT_NEXT_MIN(min)
		    )
		{
		    UsageDurationEntry_t *e = ud->entry + index;
		    if ( e->elapsed_nsec < min )
		    {
			AddUsageDurationEntry(e,e-1);
			index--;
			break;
		    }
		}
		if ( ++index > USAGE_COUNT_ENTRIES )
		    index = USAGE_COUNT_ENTRIES;
	    }

	    if ( index > 1 )
		memmove( ud->entry+1, ud->entry, (index-1) * sizeof(*ud->entry) );

	    if ( ud->par.used < index )
		 ud->par.used = index;


	    //--- update first entry

	    UsageDurationEntry_t *e = ud->entry;
	    *e = ud->ref;
	    e->elapsed_nsec = cur.elapsed_nsec - e->elapsed_nsec;


	    //--- update top value (1s)

	    if (e->elapsed_nsec)
		e->top_cnt_1s  = (double)( NSEC_PER_SEC * e->count ) / e->elapsed_nsec;


	    //--- update top value (5s)

	    u_nsec_t elapsed5 = 0, count5 = 0;
	    UsageDurationEntry_t *e5 = ud->entry;
	    for ( int i = 0; i < ud->par.used && elapsed5 < 5*NSEC_PER_SEC; i++, e5++ )
	    {
		elapsed5 += e5->elapsed_nsec;
		count5   += e5->count;
	    }

	    if (elapsed5)
	    {
		if ( count5 == e->count )
		    elapsed5 = 5*NSEC_PER_SEC;
		e->top_cnt_5s = (double)( NSEC_PER_SEC * count5 ) / elapsed5;
	    }
	}

	ud->ref = cur;
	ud->par.update_nsec = ud->ref.elapsed_nsec + NSEC_PER_SEC;
    }

    ud->par.force_update_nsec = ud->ref.elapsed_nsec + NSEC_PER_MIN;
    ud->par.dirty = ud->ref.count > 0;
}

///////////////////////////////////////////////////////////////////////////////

void UpdateUsageDurationAdd
	( UsageDuration_t *ud, int add, u_nsec_t wait_nsec, u_nsec_t top_nsec )
{
    DASSERT(ud);
    ud->par.is_active = true;
    UpdateUsageDuration(ud,0);

    if ( add > 0 && usage_count_enabled > 0 )
    {
	ud->par.dirty = true;
	ud->ref.count += add;
	ud->ref.total_nsec += wait_nsec;
	if ( ud->ref.top_nsec < top_nsec )
	     ud->ref.top_nsec = top_nsec;
    }
}

///////////////////////////////////////////////////////////////////////////////

void UpdateUsageDurationIncrement ( UsageDuration_t *ud, u_nsec_t wait_nsec )
{
    DASSERT(ud);
    ud->par.is_active = true;
    UpdateUsageDuration(ud,0);
    if ( usage_count_enabled > 0 )
    {
	ud->par.dirty = true;
	ud->ref.count++;
	ud->ref.total_nsec += wait_nsec;
	if ( ud->ref.top_nsec < wait_nsec )
	     ud->ref.top_nsec = wait_nsec;
    }
}

///////////////////////////////////////////////////////////////////////////////

void UsageDurationIncrement ( UsageDuration_t *ud, u_nsec_t wait_nsec )
{
    DASSERT(ud);
    ud->par.is_active = true;
    if ( usage_count_enabled > 0 )
    {
	ud->par.dirty = true;
	ud->ref.count++;
	ud->ref.total_nsec += wait_nsec;
	if ( ud->ref.top_nsec < wait_nsec )
	     ud->ref.top_nsec = wait_nsec;
    }
}

///////////////////////////////////////////////////////////////////////////////

UsageDurationEntry_t GetUsageDuration ( const UsageDuration_t *ud, s_nsec_t nsec )
{
    DASSERT(ud);
    UsageDurationEntry_t res = {0};
    const UsageDurationEntry_t *e = ud->entry;
    for ( int i = 0; i < ud->par.used && nsec > 0; i++, e++ )
    {
	nsec -= e->elapsed_nsec;
	AddUsageDurationEntry(&res,e);
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void AddUsageDurationEntry ( UsageDurationEntry_t *dest, const UsageDurationEntry_t *add )
{
    DASSERT(dest);
    DASSERT(add);
    dest->count		+= add->count;
    dest->elapsed_nsec	+= add->elapsed_nsec;
    dest->total_nsec	+= add->total_nsec;

    if ( !dest->time_sec || dest->time_sec > add->time_sec )
	dest->time_sec = add->time_sec;

    if ( dest->top_nsec < add->top_nsec )
	 dest->top_nsec = add->top_nsec;

    if ( dest->top_cnt_1s < add->top_cnt_1s )
	 dest->top_cnt_1s = add->top_cnt_1s;
    if ( dest->top_cnt_5s < add->top_cnt_5s )
	 dest->top_cnt_5s = add->top_cnt_5s;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			list usage count/wait		///////////////
///////////////////////////////////////////////////////////////////////////////

static ccp print_usage_count_val ( double val, const float *color_val )
{
    if ( val <= 0 )
	return "       -";

    static const float color_val0[] = { 100, 200, 300, 500, 1000, 2000 };
    if (!color_val)
	color_val = color_val0;

    if ( val > color_val[0] || val >= 9999.9994 )
    {
	ccp col =   val < color_val[2]
		? ( val < color_val[1] ? colout->mark : colout->info )
		:   val < color_val[4]
		? ( val < color_val[3] ? colout->hint : colout->warn )
		: ( val < color_val[5] ? colout->err  : colout->bad );

	return val < 9999.9994
		? PrintCircBuf("%s%8.3f%s",col,val,colout->reset)
		: val < 999999.94
		? PrintCircBuf("%s%8.1f%s",col,val,colout->reset)
		: PrintCircBuf("%s%8.0f%s",col,val,colout->reset);
    }

    char *res = PrintCircBuf("%8.3f",val);
    if ( val < 1.0 && res[3] == '0' )
	res[3] = ' ';
    return res;
}

//-----------------------------------------------------------------------------

static ccp print_usage_count
	( u32 n_wait, u_nsec_t elapsed_nsec, const float *color_val )
{
    return elapsed_nsec
	? print_usage_count_val( (double)( NSEC_PER_SEC * n_wait ) / elapsed_nsec, color_val )
	: "       ×";
}

//-----------------------------------------------------------------------------

static ccp print_usage_dur_val ( u_nsec_t dur, const u_nsec_t *color_dur )
{
    if (!dur)
	return "     -";

    ccp res = PrintTimerNSec6(0,0,dur,DC_SFORM_ALIGN);

    static const u_nsec_t color_dur0[] =
    {
	 10 *	NSEC_PER_MSEC,	// mark
	 50 *	NSEC_PER_MSEC,	// info
	200 *	NSEC_PER_MSEC,	// hint
		NSEC_PER_SEC,	// warn
	  8 *	NSEC_PER_SEC,	// err
		NSEC_PER_MIN,	// bad
    };
    if (!color_dur)
	color_dur = color_dur0;

    if ( dur > color_dur[0] )
    {
	ccp col =   dur < color_dur[2]
		? ( dur < color_dur[1] ? colout->mark : colout->info )
		:   dur < color_dur[4]
		? ( dur < color_dur[3] ? colout->hint : colout->warn )
		: ( dur < color_dur[5] ? colout->err  : colout->bad );

	return PrintCircBuf("%s%s%s",col,res,colout->reset);
    }

    return res;
}

//-----------------------------------------------------------------------------

static ccp print_usage_dur
	( u_nsec_t total, u64 count, const u_nsec_t *color_dur )
{
    return count
	? print_usage_dur_val( total/count, color_dur )
	: "     -";
}

//
///////////////////////////////////////////////////////////////////////////////

void ListUsageCount
(
    PrintMode_t		*p_pm,		// NULL or print mode
    const UsageCtrl_t	*uci,		// NULL or object to list
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
)
{
    if (!uci)
	return;

    if (uci->list_func)
    {
	uci->list_func(p_pm,uci,print_mode,limit_nsec);
	return;
    }

    if (!uci->uc)
    {
	if (uci->ud)
	    ListUsageDuration(p_pm,uci,print_mode,limit_nsec);
	return;
    }

    //--- setup

    PrintMode_t pm = {0};
    if (p_pm)
	pm = *p_pm;
    SetupPrintMode(&pm);

    if ( limit_nsec <= 0 )
	limit_nsec = S_NSEC_MAX;

    UsageCount_t *uc = uci->uc;
    fprintf(pm.fout,"%s%s%*s %s%s (%u record%s%s%s)%s%s",
		pm.compact ? "" : pm.eol, pm.prefix, pm.indent, "",
		pm.cout->caption, uci->title,
		uc->par.used, uc->par.used == 1 ? "" : "s",
		usage_count_enabled > 0 ? "" : ", global disabled",
		uc->par.enabled ? "" : ", disabled",
		pm.cout->reset, pm.eol );

    if (!uc->par.used)
	return;


    //--- print table

    UsageCountEntry_t sum = {0};
    u_sec_t reftime = 0;
    u_nsec_t min = USAGE_COUNT_NEXT_MIN(NSEC_PER_SEC);

    if ( print_mode == LUC_PM_SUMMARY )
    {
     fprintf(pm.fout,
	"%s%*s%s╔════════╦════════╤═════════╤══════════╦═══════════╤═══════════╗%s"
	"%s%*s%s║ range  ║ period │ counter │  count/s ║ top~1s /s │ top~5s /s ║%s"
	"%s%*s%s╠════════╬════════╪═════════╪══════════╬═══════════╪═══════════╣%s%s"
	,pm.prefix ,pm.indent,"" ,colout->heading ,pm.eol
	,pm.prefix ,pm.indent,"" ,colout->heading ,pm.eol
	,pm.prefix ,pm.indent,"" ,colout->heading ,colout->reset ,pm.eol
	);

     const IntervalInfo_t *interval = interval_info;
     const UsageCountEntry_t *e = uc->entry;
     for ( int i = 0; i < uc->par.used; i++, e++ )
     {
	AddUsageCountEntry(&sum,e);
	bool is_last = false;
	const bool is_limit = sum.elapsed_nsec >= limit_nsec;
	if ( sum.elapsed_nsec < interval->nsec )
	{
	    is_last = i+1 == uc->par.used;
	    if ( !is_last && !is_limit )
		continue;
	}

	ccp name;
	if ( is_last || sum.elapsed_nsec < interval->nsec )
	    name = "*";
	else
	{
	    while ( sum.elapsed_nsec > interval->nsec )
		interval++;
	    name = interval[-1].name;
	}

	fprintf(pm.fout,
		"%s%*s%s║%s %-6.6s %s║%s %s %s│%s %s %s│%s %s %s║%s"
		,pm.prefix ,pm.indent,""
		,colout->heading, colout->reset
		,name
		,colout->heading, colout->reset
		,PrintTimerNSec6(0,0,sum.elapsed_nsec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,PrintNumberU7(0,0,sum.count,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_usage_count(sum.count,sum.elapsed_nsec,uci->color_val)
		,colout->heading, colout->reset
		);

	fprintf(pm.fout,
		" %s  %s│%s  %s %s║%s"
		,print_usage_count_val(sum.top1s,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_count_val(sum.top5s,uci->color_val)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	    fprintf(pm.flog," %2u%s",i,pm.eol);
	else
	    fputs(pm.eol,pm.fout);

	if ( is_limit || !interval->name )
	    break;
     }

     fprintf(pm.fout,
	"%s ╚════════╩════════╧═════════╧══════════╩═══════════╧═══════════╝%s%s"
	,colout->heading ,colout->reset ,pm.eol );
    }
    else
    {
     fprintf(pm.fout,
	"%s%*s%s╔═══════════╦═════════════════════════════╦══════════╦══════════╦═════════════════════════════╗%s"
	"%s%*s%s║           ║       Single Record         ║  top 1s  ║  top 5s  ║       Summed Values         ║%s"
	"%s%*s%s║ real time ║ period │ counter │  count/s ║  count/s ║  count/s ║ period │ counter │  count/s ║%s"
	"%s%*s%s╠═══════════╬════════╪═════════╪══════════╬══════════╬══════════╬════════╪═════════╪══════════╣%s%s"
	,pm.prefix ,pm.indent,"" ,colout->heading ,pm.eol
	,pm.prefix ,pm.indent,"" ,colout->heading ,pm.eol
	,pm.prefix ,pm.indent,"" ,colout->heading ,pm.eol
	,pm.prefix ,pm.indent,"" ,colout->heading ,colout->reset ,pm.eol
	);

     const IntervalInfo_t *interval = interval_info+1;
     const UsageCountEntry_t *e = uc->entry;
 #if TEST_USAGE_COUNT
     for ( int i = 0; i <= uc->par.used && sum.elapsed_nsec < limit_nsec; i++, e++ )
 #else
     for ( int i = 0; i < uc->par.used && sum.elapsed_nsec < limit_nsec; i++, e++ )
 #endif
     {
	AddUsageCountEntry(&sum,e);
	if ( i > 0 && ( sum.elapsed_nsec >= interval->nsec || i == USAGE_COUNT_FIRST_ADD ))
	{
	    fprintf(pm.fout,
		"%s ╟───────────╫────────┼─────────┼──────────╫"
		"──────────╫──────────╫"
		"────────┼─────────┼──────────╢%s%s",
		colout->heading, colout->reset, pm.eol );
	    while ( sum.elapsed_nsec >= interval->nsec )
		interval++;
	}

	fprintf(pm.fout,
		"%s%*s%s║%s%s %s║%s %s %s│%s %s %s│%s %s %s║%s"
		,pm.prefix ,pm.indent,""
		,colout->heading, colout->reset
		,PrintRefTime(e->time_sec,&reftime)
		,colout->heading, colout->reset
		,PrintTimerNSec6(0,0,e->elapsed_nsec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,PrintNumberU7(0,0,e->count,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_usage_count(e->count,e->elapsed_nsec,uci->color_val)
		,colout->heading, colout->reset
		);

	fprintf(pm.fout,
		" %s %s║%s %s %s║%s"
		,print_usage_count_val(e->top1s,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_count_val(e->top5s,uci->color_val)
		,colout->heading, colout->reset
		);

	fprintf(pm.fout,
		" %s %s│%s %s %s│%s %s %s║%s"
		,PrintTimerNSec6(0,0,sum.elapsed_nsec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,PrintNumberU7(0,0,sum.count,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_usage_count(sum.count,sum.elapsed_nsec,uci->color_val)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	{
	    if ( pm.debug >= 2 && i >= USAGE_COUNT_FIRST_ADD )
	    {
		fprintf(pm.flog," %2u  %s %s%4llu%%%s %s%5llu%%%s%s",
			i,
			PrintTimerNSec6(0,0,min,DC_SFORM_ALIGN|DC_SFORM_DASH),
			e->elapsed_nsec < min ? ""
				: e->elapsed_nsec < min*2 ? colout->hint : colout->warn,
			100 * e->elapsed_nsec / min, colout->reset,
			sum.elapsed_nsec < min ? colout->info : "",
			100 * sum.elapsed_nsec / min, colout->reset, pm.eol );
		min = USAGE_COUNT_NEXT_MIN(min);
	    }
	    else
		fprintf(pm.flog," %2u%s",i,pm.eol);
	}
	else
	    fputs(pm.eol,pm.fout);
     }

     fprintf(pm.fout,
	"%s ╚═══════════╩════════╧═════════╧══════════╩"
	"══════════╩══════════╩"
	"════════╧═════════╧══════════╝%s%s"
	,colout->heading ,colout->reset ,pm.eol );
    }
}

//
///////////////////////////////////////////////////////////////////////////////

void ListUsageDuration
(
    PrintMode_t		*p_pm,		// NULL or print mode
    const UsageCtrl_t	*uci,		// NULL or object to list
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
)
{
    if (!uci)
	return;

    if (uci->list_func)
    {
	uci->list_func(p_pm,uci,print_mode,limit_nsec);
	return;
    }

    if (!uci->ud)
    {
	if (uci->uc)
	    ListUsageCount(p_pm,uci,print_mode,limit_nsec);
	return;
    }

    //--- setup

    PrintMode_t pm = {0};
    if (p_pm)
	pm = *p_pm;
    SetupPrintMode(&pm);

    if ( limit_nsec <= 0 )
	limit_nsec = S_NSEC_MAX;

    UsageDuration_t *ud = uci->ud;
    fprintf(pm.fout,"%s%s%*s %s%s (%u record%s%s%s)%s%s",
		pm.compact ? "" : pm.eol, pm.prefix, pm.indent, "",
		pm.cout->caption, uci->title,
		ud->par.used, ud->par.used == 1 ? "" : "s",
		usage_count_enabled > 0 ? "" : ", global disabled",
		ud->par.enabled ? "" : ", disabled",
		pm.cout->reset, pm.eol );

    if (!ud->par.used)
	return;


    //--- print table

    UsageDurationEntry_t sum = {0};
    u_sec_t reftime = 0;
    u_nsec_t min = USAGE_COUNT_NEXT_MIN(NSEC_PER_SEC);

    if ( print_mode == LUC_PM_SUMMARY )
    {
     printf(
	"%s ╔════════╦═════════════════════════════════════╦═════════╤═════════╤════════╗%s"
	"%s ║        ║            Summed Record            ║ top 1s  │ top 5s  │  top   ║%s"
	"%s ║ range  ║ period │ counter │ count/s │ Ø time ║ count/s │ count/s │  time  ║%s"
	"%s ╠════════╬════════╪═════════╪═════════╪════════╬═════════╪═════════╪════════╣%s%s"
	,colout->heading, pm.eol
	,colout->heading, pm.eol
	,colout->heading, pm.eol
	,colout->heading ,colout->reset, pm.eol
	);

     const IntervalInfo_t *interval = interval_info;
     const UsageDurationEntry_t *e = ud->entry;
     for ( int i = 0; i < ud->par.used; i++, e++ )
     {
	AddUsageDurationEntry(&sum,e);
	bool is_last = false;
	const bool is_limit = sum.elapsed_nsec >= limit_nsec;
	if ( sum.elapsed_nsec < interval->nsec )
	{
	    is_last = i+1 == ud->par.used;
	    if ( !is_last && !is_limit )
		continue;
	}

	ccp name;
	if ( is_last || sum.elapsed_nsec < interval->nsec )
	    name = "*";
	else
	{
	    while ( sum.elapsed_nsec > interval->nsec )
		interval++;
	    name = interval[-1].name;
	}

	if ( i > 0 && ( sum.elapsed_nsec >= interval->nsec || i == USAGE_COUNT_FIRST_ADD ))
	{
	    fprintf(pm.fout,
		"%s ╟───────────╫────────┼─────────┼─────────┼────────╫"
		"─────────┼────────╫─────────┼────────╫"
		"────────┼─────────┼─────────┼────────╢%s%s",
		colout->heading, colout->reset, pm.eol );
	    while ( sum.elapsed_nsec >= interval->nsec )
		interval++;
	}

	fprintf(pm.fout,
		"%s%*s%s║%s %-6.6s %s║%s %s %s│%s %s %s│%s%s %s│%s %s %s║%s"
		,pm.prefix ,pm.indent,""
		,colout->heading, colout->reset
		,name
		,colout->heading, colout->reset
		,PrintTimerNSec6(0,0,sum.elapsed_nsec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,PrintNumberU7(0,0,sum.count,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_usage_count(sum.count,sum.elapsed_nsec,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_dur(sum.total_nsec,sum.count,uci->color_dur)
		,colout->heading, colout->reset
		);

	fprintf(pm.fout,
		"%s %s│%s%s %s│%s %s %s║%s"
		,print_usage_count_val(sum.top_cnt_1s,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_count_val(sum.top_cnt_5s,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_dur_val(sum.top_nsec,uci->color_dur)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	    fprintf(pm.flog," %2u%s",i,pm.eol);
	else
	    fputs(pm.eol,pm.fout);

	if ( is_limit || !interval->name )
	    break;
     }

     fprintf(pm.fout,
	"%s ╚════════╩════════╧═════════╧═════════╧════════╩"
	"═════════╧═════════╧════════╝%s%s"
	,colout->heading ,colout->reset ,pm.eol );
    }
    else
    {
     printf(
	"%s ╔═══════════╦═════════════════════════════════════╦═════════╤═════════╤════════╦═════════════════════════════════════╗%s"
	"%s ║           ║            Single Record            ║ top 1s  │ top 5s  │  top   ║            Summed Values            ║%s"
	"%s ║ real time ║ period │ counter │ count/s │ Ø time ║ count/s │ count/s │  time  ║ period │ counter │ count/s │ Ø time ║%s"
	"%s ╠═══════════╬════════╪═════════╪═════════╪════════╬═════════╪═════════╪════════╬════════╪═════════╪═════════╪════════╣%s%s"
	,colout->heading, pm.eol
	,colout->heading, pm.eol
	,colout->heading, pm.eol
	,colout->heading ,colout->reset, pm.eol
	);

     const IntervalInfo_t *interval = interval_info+1;
     const UsageDurationEntry_t *e = ud->entry;
 #if TEST_USAGE_COUNT
     for ( int i = 0; i <= ud->par.used && sum.elapsed_nsec < limit_nsec; i++, e++ )
 #else
     for ( int i = 0; i < ud->par.used && sum.elapsed_nsec < limit_nsec; i++, e++ )
 #endif
     {
	AddUsageDurationEntry(&sum,e);
	if ( i > 0 && ( sum.elapsed_nsec >= interval->nsec || i == USAGE_COUNT_FIRST_ADD ))
	{
	    fprintf(pm.fout,
		"%s ╟───────────╫────────┼─────────┼─────────┼────────╫"
		"─────────┼─────────┼────────╫"
		"────────┼─────────┼─────────┼────────╢%s%s",
		colout->heading, colout->reset, pm.eol );
	    while ( sum.elapsed_nsec >= interval->nsec )
		interval++;
	}

	fprintf(pm.fout,
		"%s%*s%s║%s%s %s║%s %s %s│%s %s %s│%s%s %s│%s %s %s║%s"
		,pm.prefix ,pm.indent,""
		,colout->heading, colout->reset
		,PrintRefTime(e->time_sec,&reftime)
		,colout->heading, colout->reset
		,PrintTimerNSec6(0,0,e->elapsed_nsec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,PrintNumberU7(0,0,e->count,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_usage_count(e->count,e->elapsed_nsec,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_dur(e->total_nsec,e->count,uci->color_dur)
		,colout->heading, colout->reset
		);

	fprintf(pm.fout,
		"%s %s│%s%s %s│%s %s %s║%s"
		,print_usage_count_val(e->top_cnt_1s,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_count_val(e->top_cnt_5s,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_dur_val(e->top_nsec,uci->color_dur)
		,colout->heading, colout->reset
		);

	fprintf(pm.fout,
		" %s %s│%s %s %s│%s%s %s│%s %s %s║%s"
		,PrintTimerNSec6(0,0,sum.elapsed_nsec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,PrintNumberU7(0,0,sum.count,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_usage_count(sum.count,sum.elapsed_nsec,uci->color_val)
		,colout->heading, colout->reset
		,print_usage_dur(sum.total_nsec,sum.count,uci->color_dur)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	{
	    if ( pm.debug >= 2 && i >= USAGE_COUNT_FIRST_ADD )
	    {
		fprintf(pm.flog," %2u  %s %s%4llu%%%s %s%5llu%%%s%s",
			i,
			PrintTimerNSec6(0,0,min,DC_SFORM_ALIGN|DC_SFORM_DASH),
			e->elapsed_nsec < min ? ""
				: e->elapsed_nsec < min*2 ? colout->hint : colout->warn,
			100 * e->elapsed_nsec / min, colout->reset,
			sum.elapsed_nsec < min ? colout->info : "",
			100 * sum.elapsed_nsec / min, colout->reset, pm.eol );
		min = USAGE_COUNT_NEXT_MIN(min);
	    }
	    else
		fprintf(pm.flog," %2u%s",i,pm.eol);
	}
	else
	    fputs(pm.eol,pm.fout);
     }

     fprintf(pm.fout,
	"%s ╚═══════════╩════════╧═════════╧═════════╧════════╩"
	"═════════╧═════════╧════════╩"
	"════════╧═════════╧═════════╧════════╝%s%s"
	,colout->heading ,colout->reset ,pm.eol );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cpu usage			///////////////
///////////////////////////////////////////////////////////////////////////////

CpuUsage_t cpu_usage = { .par.used=0, .par.enabled = CPU_USAGE_ENABLED };

const UsageCtrl_t cpu_usage_ctrl =
{
	.par		= &cpu_usage.par,
	.update_func	= UpdateCpuUsageByUCI,
	.list_func	= ListCpuUsage,
	.title		= "CPU usage (extern)",
	.key1		= "CPU",
};

///////////////////////////////////////////////////////////////////////////////

static ccp print_cpu_percent ( double val )
{
    if (!val)
	return "     -";

  //static const double collist[] = { 40, 60, 80, 95 };
    static const double collist[] = { 25, 50, 75, 95 };

    if ( val >= collist[0] )
    {
	ccp col = val < collist[2]
		? ( val < collist[1] ? colout->info : colout->hint )
		: ( val < collist[3] ? colout->warn : colout->err );
	return PrintCircBuf("%s%6.2f%s", col, val, colout->reset );
    }

    char *res = PrintCircBuf("%6.2f",val);
    if ( val < 1.0 && res[2] == '0' )
	res[2] = ' ';
    return res;
}

//-----------------------------------------------------------------------------

static ccp print_cpu_seconds ( u_usec_t val )
{
    return !val
	? "       -"
	: val < USEC_PER_SEC
	? PrintCircBuf("%8llu",val)
	: val < 10*USEC_PER_SEC
	? PrintCircBuf("%llu.%06llu",val/USEC_PER_SEC,val%USEC_PER_SEC)
	: PrintCircBuf("%4llu.%03llu",val/USEC_PER_SEC,val%USEC_PER_SEC/1000);
}

//-----------------------------------------------------------------------------

static ccp print_cpu_wait_val ( double val )
{
    static const double collist[] = { 100, 150, 200, 250, 300, 350 };
    if ( val > collist[0] )
    {
	if ( val >= 999.0 )
	    return PrintCircBuf(" %s%6.0f%s",colout->fatal,val,colout->reset);

	ccp col = val < collist[2]
		? ( val < collist[1] ? colout->mark : colout->info )
		: val < collist[4]
		? ( val < collist[3] ? colout->hint : colout->warn )
		: ( val < collist[5] ? colout->err  : colout->bad );

	return PrintCircBuf("%s%7.2f%s",col,val,colout->reset);
    }

    if ( val <= 0 )
	return "      -";

    char * res = PrintCircBuf("%7.2f",val);
    if ( val < 1.0 && res[3] == '0' )
	res[3] = ' ';
    return res;
}

//-----------------------------------------------------------------------------

static ccp print_cpu_wait ( u32 n_wait, u_usec_t elapsed_usec )
{
    return elapsed_usec
	? print_cpu_wait_val( (double)( USEC_PER_SEC * n_wait ) / elapsed_usec )
	: "      ×";
}

//-----------------------------------------------------------------------------

static void print_cpu_debug
	( int idx, u_usec_t *min, u_usec_t elapsed1, u_usec_t elapsed2 )
{
    DASSERT(min);

    if ( idx >= CPU_USAGE_FIRST_ADD && *min )
    {
	printf(" %2u  %s %s%4llu%%%s %s%4llu%%%s\n",
		idx,
		PrintTimerUSec6(0,0,*min,DC_SFORM_ALIGN|DC_SFORM_DASH),
		elapsed1 < *min ? "" : elapsed1 < *min*2 ? colout->hint : colout->warn,
		100 * elapsed1 / *min, colout->reset,
		elapsed2 < *min ? colout->info : "", 100 * elapsed2 / *min, colout->reset );
	*min = CPU_USAGE_NEXT_MIN(*min);
    }
    else
	printf(" %2u\n",idx);
}

///////////////////////////////////////////////////////////////////////////////

void ListCpuUsage
(
    PrintMode_t		*p_pm,		// NULL or print mode
    const UsageCtrl_t	*uci,		// ignored!
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
)
{
    //--- setup

    PrintMode_t pm = {0};
    if (p_pm)
	pm = *p_pm;
    SetupPrintMode(&pm);

    const u_usec_t limit_usec = limit_nsec > 0 ? limit_nsec/NSEC_PER_USEC : S_USEC_MAX;


    //--- print header

    const ccp empty_line = pm.compact ? "" : "\n";
    printf("%s%s  CPU Usage (%u record%s%s)%s\n",
		empty_line, colout->caption,
		cpu_usage.par.used, cpu_usage.par.used == 1 ? "" : "s",
		cpu_usage.par.enabled ? "" : ", disabled", colout->reset );

    if (!cpu_usage.par.used)
	return;


    //--- print table

    CpuUsageEntry_t sum = {0};
    u_sec_t reftime = 0;
    u_usec_t min = CPU_USAGE_NEXT_MIN(USEC_PER_SEC);

    if ( print_mode == LUC_PM_SUMMARY )
    {
     printf(
	"%s ╔════════╦═════════════════════════════════════════╦════════════════╦════════════════╗\n"
	"%s ║        ║              Summed Values              ║    top 1s      ║    top 5s      ║\n"
	"%s ║ range  ║ period │ user%% │system%%│total%% │ wait/s ║total%% │ wait/s ║total%% │ wait/s ║\n"
	"%s ╠════════╬════════╪═══════╪═══════╪═══════╪════════╬═══════╪════════╬═══════╪════════╣%s\n"
	,colout->heading
	,colout->heading
	,colout->heading
	,colout->heading ,colout->reset
	);

     const IntervalInfo_t *interval = interval_info;
     const CpuUsageEntry_t *e = cpu_usage.entry;
     for ( int i = 0; i < cpu_usage.par.used; i++, e++ )
     {
	AddCpuEntry(&sum,e);
	bool is_last = false;
	const bool is_limit = sum.elapsed_usec >= limit_usec;
	if ( sum.elapsed_usec < interval->usec )
	{
	    is_last = i+1 == cpu_usage.par.used;
	    if ( !is_last && !is_limit )
		continue;
	}

	ccp name;
	if ( is_last || sum.elapsed_usec < interval->usec )
	    name = "*";
	else
	{
	    while ( sum.elapsed_usec > interval->usec )
		interval++;
	    name = interval[-1].name;
	}

	const double elapsed = 100.0 / sum.elapsed_usec;
	printf(" %s║%s %-6.6s %s║%s %s %s│%s%s %s│%s%s %s│%s%s %s│%s%s %s║%s"
		,colout->heading, colout->reset
		,name
		,colout->heading, colout->reset
		,PrintTimerUSec6(0,0,sum.elapsed_usec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_cpu_percent(sum.utime_usec * elapsed)
		,colout->heading, colout->reset
		,print_cpu_percent(sum.stime_usec * elapsed)
		,colout->heading, colout->reset
		,print_cpu_percent(( sum.utime_usec + sum.stime_usec) * elapsed)
		,colout->heading, colout->reset
		,print_cpu_wait(sum.n_wait,sum.elapsed_usec)
		,colout->heading, colout->reset
		);

	printf("%s %s│%s%s %s║%s%s %s│%s%s %s║%s"
		,print_cpu_percent(sum.top1s)
		,colout->heading, colout->reset
		,print_cpu_wait_val(sum.topw1s)
		,colout->heading, colout->reset
		,print_cpu_percent(sum.top5s)
		,colout->heading, colout->reset
		,print_cpu_wait_val(sum.topw5s)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	    fprintf(pm.flog," %2u%s",i,pm.eol);
	else
	    fputs(pm.eol,pm.fout);

	if ( is_limit || !interval->name )
	    break;
     }
     printf(
	"%s ╚════════╩════════╧═══════╧═══════╧═══════╧════════╩"
	"═══════╧════════╩═══════╧════════╝%s%s%s",
	colout->heading ,colout->reset, pm.eol, empty_line );
    }
    else if ( print_mode == LUC_PM_RAW )
    {
     printf(
	"%s ╔═══════════╦══════════════════════════════════════════════════╦══════════════════════════════════════════════════╗\n"
	"%s ║           ║                Single Record (µs)                ║                Summed Values (µs)                ║\n"
	"%s ║ real time ║ period │   user   │  system  │   total  │ wait/s ║ period │   user   │  system  │   total  │ wait/s ║\n"
	"%s ╠═══════════╬════════╪══════════╪══════════╪══════════╪════════╬════════╪══════════╪══════════╪══════════╪════════╣%s\n"
	,colout->heading
	,colout->heading
	,colout->heading
	,colout->heading ,colout->reset
	);

     const IntervalInfo_t *interval = interval_info+1;
     for ( int i = 0; i < cpu_usage.par.used && sum.elapsed_usec < limit_usec; i++ )
     {
	const CpuUsageEntry_t *e = cpu_usage.entry+i;
	AddCpuEntry(&sum,e);
	if ( i > 0 && ( sum.elapsed_usec >= interval->usec || i == CPU_USAGE_FIRST_ADD ))
	{
	    printf(
		"%s ╟───────────╫────────┼──────────┼──────────┼──────────┼────────╫"
		"────────┼──────────┼──────────┼──────────┼────────╢%s\n",
		colout->heading, colout->reset );
	    while ( sum.elapsed_usec >= interval->usec )
		interval++;
	}

	printf(" %s║%s%s %s║%s %s %s│%s %s %s│%s %s %s│%s %s %s│%s%s %s║%s"
		,colout->heading, colout->reset
		,PrintRefTime(e->time_sec,&reftime)
		,colout->heading, colout->reset
		,PrintTimerUSec6(0,0,e->elapsed_usec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_cpu_seconds(e->utime_usec)
		,colout->heading, colout->reset
		,print_cpu_seconds(e->stime_usec)
		,colout->heading, colout->reset
		,print_cpu_seconds( e->utime_usec + e->stime_usec )
		,colout->heading, colout->reset
		,print_cpu_wait(e->n_wait,e->elapsed_usec)
		,colout->heading, colout->reset
		);

	printf(" %s %s│%s %s %s│%s %s %s│%s %s %s│%s%s %s║%s"
		,PrintTimerUSec6(0,0,sum.elapsed_usec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_cpu_seconds(sum.utime_usec)
		,colout->heading, colout->reset
		,print_cpu_seconds(sum.stime_usec)
		,colout->heading, colout->reset
		,print_cpu_seconds( sum.utime_usec + sum.stime_usec )
		,colout->heading, colout->reset
		,print_cpu_wait(sum.n_wait,sum.elapsed_usec)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	    print_cpu_debug(i,&min,e->elapsed_usec,sum.elapsed_usec);
	else
	    putchar('\n');
     }

     printf(
	"%s ╚═══════════╩════════╧══════════╧══════════╧══════════╧════════╩"
	"════════╧══════════╧══════════╧══════════╧════════╝%s\n%s",
	colout->heading, colout->reset, empty_line );
    }
    else
    {
     printf(
	"%s ╔═══════════╦═════════════════════════════════════════╦════════════════╦════════════════╦═════════════════════════════════════════╗\n"
	"%s ║           ║              Single Record              ║    top 1s      ║    top 5s      ║              Summed Values              ║\n"
	"%s ║ real time ║ period │ user%% │system%%│total%% │ wait/s ║total%% │ wait/s ║total%% │ wait/s ║ period │ user%% │system%%│total%% │ wait/s ║\n"
	"%s ╠═══════════╬════════╪═══════╪═══════╪═══════╪════════╬═══════╪════════╬═══════╪════════╬════════╪═══════╪═══════╪═══════╪════════╣%s\n"
	,colout->heading
	,colout->heading
	,colout->heading
	,colout->heading ,colout->reset
	);

     const IntervalInfo_t *interval = interval_info+1;
 #if TEST_USAGE_COUNT
     for ( int i = 0; i <= cpu_usage.par.used && sum.elapsed_usec < limit_usec; i++ )
 #else
     for ( int i = 0; i < cpu_usage.par.used && sum.elapsed_usec < limit_usec; i++ )
 #endif
     {
	const CpuUsageEntry_t *e = cpu_usage.entry+i;
	AddCpuEntry(&sum,e);
	if ( i > 0 && ( sum.elapsed_usec >= interval->usec || i == CPU_USAGE_FIRST_ADD ))
	{
	    printf(
		"%s ╟───────────╫────────┼───────┼───────┼───────┼────────╫"
		"───────┼────────╫───────┼────────╫"
		"────────┼───────┼───────┼───────┼────────╢%s\n",
		colout->heading, colout->reset );
	    while ( sum.elapsed_usec >= interval->usec )
		interval++;
	}

	double elapsed = 100.0 / e->elapsed_usec;
	printf(" %s║%s%s %s║%s %s %s│%s%s %s│%s%s %s│%s%s %s│%s%s %s║%s"
		,colout->heading, colout->reset
		,PrintRefTime(e->time_sec,&reftime)
		,colout->heading, colout->reset
		,PrintTimerUSec6(0,0,e->elapsed_usec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_cpu_percent(e->utime_usec * elapsed)
		,colout->heading, colout->reset
		,print_cpu_percent(e->stime_usec * elapsed)
		,colout->heading, colout->reset
		,print_cpu_percent(( e->utime_usec + e->stime_usec) * elapsed)
		,colout->heading, colout->reset
		,print_cpu_wait(e->n_wait,e->elapsed_usec)
		,colout->heading, colout->reset
		);

	printf("%s %s│%s%s %s║%s%s %s│%s%s %s║%s"
		,print_cpu_percent(e->top1s)
		,colout->heading, colout->reset
		,print_cpu_wait_val(e->topw1s)
		,colout->heading, colout->reset
		,print_cpu_percent(e->top5s)
		,colout->heading, colout->reset
		,print_cpu_wait_val(e->topw5s)
		,colout->heading, colout->reset
		);

	elapsed = 100.0 / sum.elapsed_usec;
	printf(" %s %s│%s%s %s│%s%s %s│%s%s %s│%s%s %s║%s"
		,PrintTimerUSec6(0,0,sum.elapsed_usec,DC_SFORM_ALIGN|DC_SFORM_DASH)
		,colout->heading, colout->reset
		,print_cpu_percent(sum.utime_usec * elapsed)
		,colout->heading, colout->reset
		,print_cpu_percent(sum.stime_usec * elapsed)
		,colout->heading, colout->reset
		,print_cpu_percent(( sum.utime_usec + sum.stime_usec) * elapsed)
		,colout->heading, colout->reset
		,print_cpu_wait(sum.n_wait,sum.elapsed_usec)
		,colout->heading, colout->reset
		);

	if (pm.debug)
	    print_cpu_debug(i,&min,e->elapsed_usec,sum.elapsed_usec);
	else
	    putchar('\n');
     }

     printf(
	"%s ╚═══════════╩════════╧═══════╧═══════╧═══════╧════════╩"
	"═══════╧════════╩═══════╧════════╩"
	"════════╧═══════╧═══════╧═══════╧════════╝%s%s",
	colout->heading, colout->reset, empty_line );
    }
}

///////////////////////////////////////////////////////////////////////////////

const SaveRestoreTab_t SRT_CpuUsage[] =
{
    #undef SRT_NAME
    #define SRT_NAME CpuUsage_t

    DEF_SRT_COMMENT("cpu_usage"),
    DEF_SRT_SEPARATOR(),

    DEF_SRT_INT(	par.enabled,		"cpu-enabled" ),
    DEF_SRT_UINT(	par.used,		"cpu-used" ),
    DEF_SRT_ARRAY_A(entry),
      DEF_SRT_UINT(	entry[0].elapsed_usec,	"cpu-elapsed" ),
      DEF_SRT_UINT(	entry[0].utime_usec,	"cpu-utime" ),
      DEF_SRT_UINT(	entry[0].stime_usec,	"cpu-stime" ),
      DEF_SRT_UINT(	entry[0].time_sec,	"cpu-rtime" ),
      DEF_SRT_FLOAT(	entry[0].top1s,		"cpu-top1s" ),
      DEF_SRT_FLOAT(	entry[0].top5s,		"cpu-top5s" ),
      DEF_SRT_UINT(	entry[0].n_wait,	"cpu-nwait" ),
      DEF_SRT_FLOAT(	entry[0].topw1s,	"cpu-topw1s" ),
      DEF_SRT_FLOAT(	entry[0].topw5s,	"cpu-topw5s" ),
    DEF_SRT_ARRAY_END(),

    DEF_SRT_SEPARATOR(),
    DEF_SRT_TERM()
};

//-----------------------------------------------------------------------------
// [[UpdateCpuUsage]]

void UpdateCpuUsageByUCI ( const UsageCtrl_t *uci, u_nsec_t now_nsec )
{
    UpdateCpuUsage();
}

//-----------------------------------------------------------------------------

void UpdateCpuUsage()
{
    if (!cpu_usage.par.enabled)
    {
	cpu_usage.ref.elapsed_usec = 0;
	cpu_usage.ref.n_wait = 0;
	return;
    }
    cpu_usage.par.is_active = true;

    CpuUsageEntry_t cur = {0};
    cur.elapsed_usec = GetTimerUSec();

    if ( cur.elapsed_usec >= cpu_usage.ref.elapsed_usec + USEC_PER_SEC )
    {
	// needed for the case that CPU_USAGE_ENTRIES was reduced after restart!
	if ( cpu_usage.par.used > CPU_USAGE_ENTRIES )
	     cpu_usage.par.used = CPU_USAGE_ENTRIES;

	struct rusage ru;
	if (!getrusage(RUSAGE_SELF,&ru))
	{
	    cur.time_sec   = GetTimeSec(false);
	    cur.utime_usec = ru.ru_utime.tv_sec * USEC_PER_SEC + ru.ru_utime.tv_usec;
	    cur.stime_usec = ru.ru_stime.tv_sec * USEC_PER_SEC + ru.ru_stime.tv_usec;

	    if (cpu_usage.ref.elapsed_usec)
	    {
		//--- find index and move data

		int index;
		if ( cpu_usage.par.used < CPU_USAGE_FIRST_ADD )
		    index = cpu_usage.par.used + 1;
		else
		{
		    u_usec_t min = CPU_USAGE_NEXT_MIN(USEC_PER_SEC);
		    for ( index = CPU_USAGE_FIRST_ADD;
			  index < cpu_usage.par.used;
			  index++, min = CPU_USAGE_NEXT_MIN(min)
			)
		    {
			CpuUsageEntry_t *e = cpu_usage.entry + index;
			if ( e->elapsed_usec < min )
			{
			    AddCpuEntry(e,e-1);
			    index--;
			    break;
			}
		    }
		    if ( ++index > CPU_USAGE_ENTRIES )
			index = CPU_USAGE_ENTRIES;
		}

		if ( index > 1 )
		    memmove( cpu_usage.entry+1, cpu_usage.entry,
				(index-1) * sizeof(*cpu_usage.entry) );

		if ( cpu_usage.par.used < index )
		     cpu_usage.par.used = index;


		//--- update first entry

		CpuUsageEntry_t *e = cpu_usage.entry;
		*e		= cpu_usage.ref;
		e->elapsed_usec	= cur.elapsed_usec - cpu_usage.ref.elapsed_usec;
		e->utime_usec	= cur.utime_usec   - cpu_usage.ref.utime_usec;
		e->stime_usec	= cur.stime_usec   - cpu_usage.ref.stime_usec;


		//--- update top values (1s)

		if (e->elapsed_usec)
		{
		    e->top1s  = 100.0 * ( e->utime_usec + e->stime_usec ) / e->elapsed_usec;
		    e->topw1s = (double)( USEC_PER_SEC * e->n_wait ) / e->elapsed_usec;
		}


		//--- update top values (5s)

		u_usec_t elapsed5 = 0, total5 = 0, wait5 = 0;
		CpuUsageEntry_t *e5 = cpu_usage.entry;
		for ( int i = 0; i < cpu_usage.par.used && elapsed5 < 5*USEC_PER_SEC; i++, e5++ )
		{
		    elapsed5 += e5->elapsed_usec;
		    total5   += e5->utime_usec + e5->stime_usec;
		    wait5    += e5->n_wait;
		}

		if (elapsed5)
		{
		    if ( wait5 == e->n_wait && total5 == e->utime_usec + e->stime_usec )
			elapsed5 = 5*NSEC_PER_SEC;
		    e->top5s  = 100.0 * total5 / elapsed5;
		    e->topw5s = (double)( USEC_PER_SEC * wait5 ) / elapsed5;
		}
	    }

	    cpu_usage.ref = cur;
	}
    }
}

//-----------------------------------------------------------------------------

void UpdateCpuUsageIncrement()
{
    UpdateCpuUsage();
    cpu_usage.ref.n_wait++;
}

//-----------------------------------------------------------------------------

void AddCpuEntry ( CpuUsageEntry_t *dest, const CpuUsageEntry_t *add )
{
    DASSERT(dest);
    DASSERT(add);
    dest->elapsed_usec	+= add->elapsed_usec;
    dest->utime_usec	+= add->utime_usec;
    dest->stime_usec	+= add->stime_usec;
    dest->n_wait	+= add->n_wait;

    if ( !dest->time_sec || dest->time_sec > add->time_sec )
	dest->time_sec = add->time_sec;

    if ( dest->top1s < add->top1s )
	 dest->top1s = add->top1s;
    if ( dest->top5s < add->top5s )
	 dest->top5s = add->top5s;

    if ( dest->topw1s < add->topw1s )
	 dest->topw1s = add->topw1s;
    if ( dest->topw5s < add->topw5s )
	 dest->topw5s = add->topw5s;
}

//-----------------------------------------------------------------------------

CpuUsageEntry_t GetCpuUsage ( s_usec_t usec )
{
    CpuUsageEntry_t res = {0};
    for ( int i = 0; i < cpu_usage.par.used && usec > 0; i++ )
    {
	const CpuUsageEntry_t *e = cpu_usage.entry + i;
	usec -= e->elapsed_usec;
	AddCpuEntry(&res,e);
    }
    return res;
}

//-----------------------------------------------------------------------------

uint GetCpuUsagePercent ( s_usec_t usec )
{
    CpuUsageEntry_t e = GetCpuUsage(usec);
    return GetCpuEntryPercent(&e);
}

///////////////////////////////////////////////////////////////////////////////

double get_rusage_percent (void)
{
    static u_nsec_t prev_time    =   0;
    static u_usec_t prev_rusage  =   0;
    static double   prev_percent = 0.0;

    const u_nsec_t cur_time = GetTimerNSec();
    if ( cur_time < prev_time + 10*NSEC_PER_MSEC )
	return prev_percent;

    u_usec_t cur_rusage = prev_rusage;
    struct rusage ru;
    if (!getrusage(RUSAGE_SELF,&ru))
    {
	cur_rusage = ru.ru_utime.tv_sec * USEC_PER_SEC + ru.ru_utime.tv_usec
		   + ru.ru_stime.tv_sec * USEC_PER_SEC + ru.ru_stime.tv_usec;
    }

    double percent = 0.0;
    if ( prev_time && cur_time > prev_time )
	percent = (double)( 100 * NSEC_PER_USEC * ( cur_rusage - prev_rusage ))
		/ ( cur_time - prev_time );

    prev_time = cur_time;
    prev_rusage = cur_rusage;

    return prev_percent = percent;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			urandom support			///////////////
///////////////////////////////////////////////////////////////////////////////

int urandom_available = 0; // <0:not, =0:not tested, >0:ok
bool use_urandom_for_myrandom = true;

uint ReadFromUrandom ( void *dest, uint size )
{
    static int fd = -1;

    if ( fd == -1 )
    {
	if ( urandom_available < 0 )
	    return 0;
     #ifdef O_CLOEXEC
	fd = open("/dev/urandom",O_CLOEXEC|O_NONBLOCK);
     #else
	fd = open("/dev/urandom",O_NONBLOCK);
     #endif
	if ( fd == -1 )
	{
	    urandom_available = -1;
	    return 0;
	}
     #ifndef O_CLOEXEC
	fcntl(fd,F_SETFD,FD_CLOEXEC);
     #endif
	urandom_available = 1;
    }

    u8 *buf = dest;
    while ( size > 0 )
    {
	ssize_t stat = read(fd,buf,size);
	if ( stat < 0 )
	{
	    urandom_available = -1;
	    close(fd);
	    return 0;
	}
	buf  += stat;
	size -= stat;
    }

    return buf - (u8*)dest;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			random numbers			///////////////
///////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------
// Algorithm: The Art of Computer Programming, part II, section 3.6
// by Donald E. Knuth
//------------------------------------------------------------------

const u32 RANDOM32_C_ADD = 2 * 197731421; // 2 Primzahlen
const u32 RANDOM32_COUNT_BASE = 4294967; // Primzahl == ~UINT_MAX32/1000;

static int random32_a_index = -1;	// Index in die a-Tabelle
static u32 random32_count = 1;		// Abwärts-Zähler bis zum Wechsel von a,c
static u32 random32_a,
	   random32_c,
	   random32_X;			// Die letzten Werte

static u32 random32_a_tab[] = // Init-Tabelle
{
    0xbb40e62d, 0x3dc8f2f5, 0xdc024635, 0x7a5b6c8d,
    0x583feb95, 0x91e06dbd, 0xa7ec03f5, 0
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u32 MyRandom ( u32 max )
{
    if (use_urandom_for_myrandom)
    {
	u32 res32;
	if (ReadFromUrandom(&res32,sizeof(res32)))
	{
	    if (!max)
		return res32;

	    u64 res64 = (u64)max * res32;
	    return res64 >> 32;
	}
	use_urandom_for_myrandom = false;
    }

    if (!--random32_count)
    {
	// Neue Berechnung von random32_a und random32_c fällig

	if ( random32_a_index < 0 )
	{
	    // allererste Initialisierung auf Zeitbasis

	    MySeedByTime();
	}
	else
	{
	    random32_c += RANDOM32_C_ADD;
	    random32_a = random32_a_tab[++random32_a_index];
	    if (!random32_a)
	    {
		random32_a_index = 0;
		random32_a = random32_a_tab[0];
	    }

	    random32_count = RANDOM32_COUNT_BASE;
	}
    }

    // Jetzt erfolgt die eigentliche Berechnung

    random32_X = random32_a * random32_X + random32_c;

    if (!max)
	return random32_X;

    u64 res = (u64)max * random32_X;
    return res >> 32;
}

///////////////////////////////////////////////////////////////////////////////

u64 MySeed ( u64 base )
{
    uint a_tab_len = 0;
    while (random32_a_tab[a_tab_len])
	a_tab_len++;
    const u32 base32 = base / a_tab_len;

    random32_a_index	= (int)( base % a_tab_len );
    random32_a		= random32_a_tab[random32_a_index];
    random32_c		= ( base32 & 15 ) * RANDOM32_C_ADD + 1;
    random32_X		= base32 ^ ( base >> 32 );
    random32_count	= RANDOM32_COUNT_BASE;

    return base;
}

///////////////////////////////////////////////////////////////////////////////

u64 MySeedByTime()
{
 #if WIN_SZS_LIB

    u64 time;
    QueryPerformanceCounter((LARGE_INTEGER*)&time);
    return MySeed(time);

 #else

    struct timeval tval;
    gettimeofday(&tval,NULL);
    return MySeed( tval.tv_sec ^ tval.tv_usec );

 #endif
}

///////////////////////////////////////////////////////////////////////////////

void MyRandomFill ( void * buf, size_t size )
{
    size_t xsize = size / sizeof(u32);
    if (xsize)
    {
	size -= xsize * sizeof(u32);
	u32 * ptr = buf;
	while ( xsize-- > 0 )
	    *ptr++ = MyRandom(0);
	buf = ptr;
    }

    u8 * ptr = buf;
    while ( size-- > 0 )
	*ptr++ = MyRandom(0);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			UUID				///////////////
///////////////////////////////////////////////////////////////////////////////

void CreateUUID ( uuid_buf_t dest )
{
    DASSERT(dest);
    const uint stat = ReadFromUrandom(dest,sizeof(uuid_buf_t));
    if (!stat)
	MyRandomFill(dest,sizeof(uuid_buf_t));

    dest[6] = dest[6] & 0x0f | 0x40;
    dest[8] = dest[8] & 0x3f | 0x80;
}

///////////////////////////////////////////////////////////////////////////////

uint CreateTextUUID ( char *buf, uint bufsize )
{
    uuid_buf_t uuid;
    CreateUUID(uuid);
    return PrintUUID(buf,bufsize,uuid);
}

///////////////////////////////////////////////////////////////////////////////

uint PrintUUID ( char *buf, uint bufsize, uuid_buf_t uuid )
{
    return snprintf(buf,bufsize,
	"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
	uuid[0],uuid[1],uuid[2],uuid[3],
	uuid[4],uuid[5], uuid[6],uuid[7],  uuid[8],uuid[9],
	uuid[10],uuid[11],uuid[12],uuid[13],uuid[14],uuid[15] );
}

///////////////////////////////////////////////////////////////////////////////

char * ScanUUID ( uuid_buf_t uuid, ccp source )
{
    memset(uuid,0,sizeof(uuid_buf_t));

    uint i;
    ccp src = source;
    for ( i = 0; i < sizeof(uuid_buf_t); i++ )
    {
	if ( *src == '-' )
	    src++;

	uint num;
	ccp end = ScanNumber(&num,src,0,16,2);
	if ( end != src+2 )
	    return (char*)source;
	uuid[i] = num;
	src = end;
    }
    return (char*)src;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			gcd(), lcm()			///////////////
///////////////////////////////////////////////////////////////////////////////

uint gcd ( uint n1, uint n2 ) // greatest common divisor, german: ggt
{
    if ( !n1 || !n2 )
	return 0;

    if ( n1 > n2 )
    {
	uint temp = n1;
	n1 = n2;
	n2 = temp;
    }

    while ( n1 != 1 )
    {
	DASSERT ( n1 > 1 );
	DASSERT ( n2 >= n1 );

	uint temp = n2 % n1;
	if ( !temp )
	    break;
	n2 = n1;
	n1 = temp;
    }
    return n1;
}

///////////////////////////////////////////////////////////////////////////////

u32 gcd32 ( u32 n1, u32 n2 ) // greatest common divisor, german: ggt
{
    if ( !n1 || !n2 )
	return 0;

    if ( n1 > n2 )
    {
	u32 temp = n1;
	n1 = n2;
	n2 = temp;
    }

    while ( n1 != 1 )
    {
	DASSERT ( n1 > 1 );
	DASSERT ( n2 > n1 );

	u32 temp = n2 % n1;
	if ( !temp )
	    break;
	n2 = n1;
	n1 = temp;
    }
    return n1;
}

///////////////////////////////////////////////////////////////////////////////

u64 gcd64 ( u64 n1, u64 n2 ) // greatest common divisor, german: ggt
{
    if ( !n1 || !n2 )
	return 0;

    if ( n1 > n2 )
    {
	u64 temp = n1;
	n1 = n2;
	n2 = temp;
    }

    while ( n1 != 1 )
    {
	DASSERT ( n1 > 1 );
	DASSERT ( n2 > n1 );

	u64 temp = n2 % n1;
	if ( !temp )
	    break;
	n2 = n1;
	n1 = temp;
    }
    return n1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint lcm ( uint n1, uint n2 ) // lowest common multiple, german: kgv
{
    uint g = gcd(n1,n2);
    if ( g > 0 )
    {
	n1 /= g;
	g = n1 * n2;
	if ( g < n1 || g < n2 ) // overflow?
	    g = 0;
    }
    return g;
}

///////////////////////////////////////////////////////////////////////////////

u32 lcm32 ( u32 n1, u32 n2 ) // lowest common multiple, german: kgv
{
    u32 g = gcd32(n1,n2);
    if ( g > 0 )
    {
	n1 /= g;
	g = n1 * n2;
	if ( g < n1 || g < n2 ) // overflow?
	    g = 0;
    }
    return g;
}

///////////////////////////////////////////////////////////////////////////////

u64 lcm64 ( u64 n1, u64 n2 ) // lowest common multiple, german: kgv
{
    u64 g = gcd64(n1,n2);
    if ( g > 0 )
    {
	n1 /= g;
	g = n1 * n2;
	if ( g < n1 || g < n2 ) // overflow?
	    g = 0;
    }
    return g;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    scan number			///////////////
///////////////////////////////////////////////////////////////////////////////

#define NN NUMBER_NULL
#define NC NUMBER_CONTROL
#define NL NUMBER_LINE
#define NS NUMBER_SPACE
#define NT NUMBER_TIE
#define NP NUMBER_SEPARATE
#define NX NUMBER_OTHER

const char DigitTable[] =
{
    NN, NC, NC, NC,  NC, NC, NC, NC,  NC, NS, NL, NC,  NS, NL, NC, NC,
    NC, NC, NC, NC,  NC, NC, NC, NC,  NC, NC, NC, NC,  NC, NC, NC, NC,
    NS, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NP, NT, NT, NX,	//  !"#$%&' ()*+,-./
     0,  1,  2,  3,   4,  5,  6,  7,   8,  9, NT, NP,  NX, NX, NX, NX,	// 01234567 89:;<=>?

    NX, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,	// @ABCDEFG HIJKLMNO
    25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, NX,  NX, NX, NX, NX,	// PQRSTUVW XYZ[\]^_
    NX, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,	// `abcdefg hijklmno
    25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, NX,  NX, NX, NX, NX,	// pqrstuvw xyz{|}~

    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,

    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
    NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,  NX, NX, NX, NX,
};

#undef NN
#undef NC
#undef NL
#undef NS
#undef NT
#undef NP
#undef NX

///////////////////////////////////////////////////////////////////////////////

u64 ScanDigits
(
    // same as ScanNumber(), but other interface
    // returns the scanned number

    ccp	  *source,	// pointer to source string, modified
    ccp	  end_source,	// NULL or end of 'source'
    uint  intbase,	// integer base, 2..36
    int	  maxchar,	// max number of digits to read
    uint  *count	// not NULL: store number of scanned digits here
)
{
    DASSERT(source);
    DASSERT( intbase >= 2 && intbase <= 36 );

    ccp src = *source;
    u64 num = 0;
    while ( maxchar-- && ( !end_source || src < end_source ) )
    {
	const u8 digit = (u8)DigitTable[(u8)*src];
	if ( digit >= intbase )
	    break;
	num = num * intbase + digit;
	src++;
    }
    if (count)
	*count = src - *source;
    *source = src;
    return num;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanNumber
(
    // same as ScanDigits(), but other interface
    // returns a pointer to the first not used character

    uint  *dest_num,	// store result here, never NULL
    ccp	  src,		// pointer to source string, modified
    ccp	  src_end,	// NULL or end of 'src'
    uint  intbase,	// integer base, 2..36
    int	  maxchar	// max number of digits to read
)
{
    DASSERT(dest_num);
    uint num = 0;
    if (src)
    {
	while ( maxchar-- && ( !src_end || src < src_end ) )
	{
	    const uint n = (uint)DigitTable[*(uchar*)src];
	    if ( n >= (uint)intbase )
		break;
	    num = num * intbase + n;
	    src++;
	}
    }
    *dest_num = num;
    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanEscape
(
    // returns a pointer to the first not used character

    uint  *dest_code,	// store result here, never NULL
    ccp	  src,		// pointer to source string (behind escape char)
    ccp	  src_end	// NULL or end of 'src'
)
{
    DASSERT(dest_code);
    DASSERT(src);
    if ( !src || ( src_end ? src >= src_end : !*src ) )
    {
	*dest_code = 0;
	return (char*)src;
    }

    uint code = 0;
    switch (*src++)
    {
	case '\\': code = '\\'; break;
	case 'a':  code = '\a'; break;
	case 'b':  code = '\b'; break;
	case 'E':
	case 'e':  code = '\e'; break;
	case 'f':  code = '\f'; break;
	case 'n':  code = '\n'; break;
	case 'r':  code = '\r'; break;
	case 't':  code = '\t'; break;
	case 'v':  code = '\v'; break;
	case '!':  code = '|'; break;

	case 'x':
	    src = ScanNumber(&code,src,src_end,16,2);
	    break;

	case 'u':
	    src = ScanNumber(&code,src,src_end,16,4);
	    break;

	case 'U':
	    src = ScanNumber(&code,src,src_end,16,8);
	    break;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	    src = ScanNumber(&code,src-1,src_end,8,3);
	    break;

	default:
	    code = src[-1];
	    break;
    }

    *dest_code = code;
    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

uint ScanHexString
(
    // return the number of written bytes

    void  *buf,		// write scanned data here
    uint  buf_size,	// size of buf
    ccp	  *source,	// pointer to source string, modified
    ccp	  end_source,	// NULL or end of 'source'
    bool  allow_tie	// allow chars ' .:-' and TAB,VT as byte separator
)
{
    DASSERT(buf);
    DASSERT(source);

    u8 *dest = buf;
    uint count = 0;
    while ( count < buf_size )
    {
	ccp src = *source;
	if (allow_tie)
	{
	    while ( !end_source || src < end_source )
	    {
		const char res = DigitTable[(u8)*src];
		if ( res != NUMBER_SPACE && res != NUMBER_TIE )
		    break;
		src++;
	    }
	    *source = src;
	}

	while ( !end_source || src < end_source )
	{
	    const char res = DigitTable[(u8)*src];
	    if ( res < 0 || res >= 16 )
		break;
	    src++;
	}

	uint n_digits = src - *source;
	if (!n_digits)
	    break;

	while ( n_digits > 0 )
	{
	    uint scanned;
	    uint num = ScanDigits( source, end_source, 16,
					n_digits & 1 ? 1 : 2, &scanned );
	    if (!scanned)
		return count;

	    n_digits -= scanned;
	    *dest++ = num;
	    count++;
	}
    }

    return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanS32
(
    // return 'source' on error

    s32		*res_num,		// not NULL: store result (only on success)
    ccp		source,			// NULL or source text
    uint	default_base		// base for numbers without '0x' prefix
					//  0: C like with octal support
					// 10: standard value for decimal numbers
					// 16: standard value for hex numbers
)
{
    if (!source)
	return 0;

    ccp src = source;
    while ( *src > 0 && *src <= ' ' )
	src++;

    const bool minus = *src == '-';
    if ( minus || *src == '+' )
    {
	src++;
	while ( *src > 0 && *src <= ' ' )
	    src++;
    }
 #if 0 // 0b and 0d can be hex numbers => so this doeasn't work
    uint base = default_base;
    if ( src[0] == '0' )
    {
	switch(tolower((int)src[1]))
	{
	    case 'x': base = 16; break;
	    case 'd': base = 10; src+=2; break;
	    case 'o': base =  8; src+=2; break;
	    case 'b': base =  2; src+=2; break;
	}
    }
 #else
    const uint base = src[0] == '0' && ( src[1] == 'x' || src[1] == 'X' )
			? 16 : default_base;
 #endif

    char *end;
    const s32 num = strtoul( src, &end, base );
    if ( (ccp)end > src )
    {
	if (res_num)
	    *res_num = minus ? -num : num;
	return end;
    }

    return (char*)source;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanS64
(
    // return 'source' on error

    s64		*res_num,		// not NULL: store result (only on success)
    ccp		source,			// NULL or source text
    uint	default_base		// base for numbers without '0x' prefix
					//  0: C like with octal support
					// 10: standard value for decimal numbers
					// 16: standard value for hex numbers
)
{
    if (!source)
	return 0;

    ccp src = source;
    while ( *src > 0 && *src <= ' ' )
	src++;

    const bool minus = *src == '-';
    if ( minus || *src == '+' )
    {
	src++;
	while ( *src > 0 && *src <= ' ' )
	    src++;
    }
    const uint base = src[0] == '0' && ( src[1] == 'x' || src[1] == 'X' )
			? 16 : default_base;
    char *end;
    const s64 num = strtoull( src, &end, base );
    if ( (ccp)end > src )
    {
	if (res_num)
	    *res_num = minus ? -num : num;
	return end;
    }

    return (char*)source;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// strto*() replacements, base ion Scan*() with better base support

long int str2l ( const char *nptr, char **endptr, int base )
{
    s32 res = 0;
    char *end = ScanS32(&res,nptr,base);
    if (endptr)
	*endptr = end;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

long long int str2ll ( const char *nptr, char **endptr, int base )
{
    s64 res = 0;
    char *end = ScanS64(&res,nptr,base);
    if (endptr)
	*endptr = end;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

unsigned long int str2ul ( const char *nptr, char **endptr, int base )
{
    u32 res = 0;
    char *end = ScanU32(&res,nptr,base);
    if (endptr)
	*endptr = end;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

unsigned long long int str2ull ( const char *nptr, char **endptr, int base )
{
    u64 res = 0;
    char *end = ScanU64(&res,nptr,base);
    if (endptr)
	*endptr = end;
    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		round float/double (zero LSB)		///////////////
///////////////////////////////////////////////////////////////////////////////

float RoundF2bytes ( float f )
{
    union { float f; u32 u; } x;
    x.f = f;
    if ( x.u & 0x8000 )
    {
	x.u &= 0xffff0000;
	int exp;
	const float frac = frexpf(x.f,&exp);
	x.f = ldexpf(frac+0.00585938,exp);
    }

    x.u &= 0xffff0000;
    return x.f;
}

//-----------------------------------------------------------------------------

float RoundF3bytes ( float f )
{
    union { float f; u32 u; } x;
    x.f = f;
    if ( x.u & 0x80 )
    {
	x.u &= 0xffffff00;
	int exp;
	const float frac = frexpf(x.f,&exp);
	x.f = ldexpf(frac+0.00002289,exp);
    }

    x.u &= 0xffffff00;
    return x.f;
}

//-----------------------------------------------------------------------------

double RoundD6bytes ( double d )
{
    union { double d; u64 u; } x;
    x.d = d;
    if ( x.u & 0x8000 )
    {
	x.u &= 0xffffffffffff0000ull;
	int exp;
	const double frac = frexp(x.d,&exp);
	x.d = ldexp(frac+0.000000000010913936,exp);
    }

    x.u &= 0xffffffffffff0000ull;
    return x.d;
}

//-----------------------------------------------------------------------------

double RoundD7bytes ( double d )
{
    union { double d; u64 u; } x;
    x.d = d;
    if ( x.u & 0x80 )
    {
	x.u &= 0xffffffffffffff00ull;
	int exp;
	const double frac = frexp(x.d,&exp);
	x.d = ldexp(frac+0.000000000000042633,exp);
    }

    x.u &= 0xffffffffffffff00ull;
    return x.d;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			print number			///////////////
///////////////////////////////////////////////////////////////////////////////

char * PrintNumberU4
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 5 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num < 1000000000 ) // 1e9
    {
	u32 n32 = num;
	if ( !n32 && mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, aligned ? "   -" : "-" );
	else if ( n32 < 10000 )
	    snprintf(buf,buf_size, aligned ? "%4u" : "%u", n32 );
	else if ( n32 < 1000000 )
	    snprintf(buf,buf_size, aligned ? "%3uk" : "%uk", n32/1000 );
	else if ( n32 < 10000000 )
	{
	    n32 /= 100000;
	    snprintf(buf,buf_size,"%u.%uM", n32/10, n32%10 );
	}
	else
	    snprintf(buf,buf_size, aligned ? "%3uM" : "%uM", n32/1000000 );
    }
    else
    {
	u32 n32;
	ccp factor = "GTPE";
	if ( num < 1000000000000000ull ) // 1e15
	    n32 = num / 100000000; // 1e8
	else
	{
	    factor += 2;
	    n32 = num / 100000000000000ull; // 1e14
	}

	while (*factor)
	{
	    if ( n32 < 100 )
	    {
		snprintf(buf,buf_size,"%u.%u%c", n32/10, n32%10, *factor );
		break;
	    }
	    else if ( n32 < 10000 )
	    {
		snprintf(buf,buf_size, aligned ? "%3u%c" : "%u%c", n32/10, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberU5
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 6 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num < 1000000000 ) // 1e9
    {
	u32 n32 = num;
	if ( !n32 && mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, aligned ? "    -" : "-" );
	else if ( n32 < 100000 )
	    snprintf(buf,buf_size, aligned ? "%5u" : "%u", n32 );
	else if ( n32 < 10000000 )
	    snprintf(buf,buf_size, aligned ? "%4uk" : "%uk", n32/1000 );
	else
	    snprintf(buf,buf_size, aligned ? "%4uM" : "%uM", n32/1000000 );
    }
    else
    {
	u32 n32;
	ccp factor = "MGTPE";
	if ( num < 1000000000000000ull ) // 1e15
	    n32 = num / 1000000; // 1e6
	else
	{
	    factor += 2;
	    n32 = num / 1000000000000ull; // 1e12
	}

	while (*factor)
	{
	    if ( n32 < 10000 )
	    {
		snprintf(buf,buf_size, aligned ? "%4u%c" : "%u%c", n32, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberU6
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 7 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num < 1000000000 ) // 1e9
    {
	u32 n32 = num;
	if ( !n32 && mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, aligned ? "     -" : "-" );
	else if ( n32 < 1000000 )
	    snprintf(buf,buf_size, aligned ? "%6u" : "%u", n32 );
	else if ( n32 < 100000000 )
	    snprintf(buf,buf_size, aligned ? "%5uk" : "%uk", n32/1000 );
	else
	    snprintf(buf,buf_size, aligned ? "%5uM" : "%uM", n32/1000000 );
    }
    else
    {
	u32 n32;
	ccp factor = "MGTPE";
	if ( num < 1000000000000000ull ) // 1e15
	    n32 = num / 1000000; // 1e6
	else
	{
	    factor += 2;
	    n32 = num / 1000000000000ull; // 1e12
	}

	while (*factor)
	{
	    if ( n32 < 100000 )
	    {
		snprintf(buf,buf_size, aligned ? "%5u%c" : "%u%c", n32, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberU7
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 8 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num < 1000000000 ) // 1e9
    {
	u32 n32 = num;
	if ( !n32 && mode & DC_SFORM_DASH )
	    StringCopyS(buf,buf_size, aligned ? "      -" : "-" );
	else if ( n32 < 10000000 )
	    snprintf(buf,buf_size, aligned ? "%7u" : "%u", n32 );
	else
	    snprintf(buf,buf_size, aligned ? "%6uk" : "%uk", n32/1000 );
    }
    else
    {
	u32 n32;
	ccp factor = "MGTPE";
	if ( num < 1000000000000000ull ) // 1e15
	    n32 = num / 1000000; // 1e6
	else
	{
	    factor += 2;
	    n32 = num / 1000000000000ull; // 1e12
	}

	while (*factor)
	{
	    if ( n32 < 1000000 )
	    {
		snprintf(buf,buf_size, aligned ? "%6u%c" : "%u%c", n32, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PrintNumberS5
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if ( num >= 0 )
	return PrintNumberU5(buf,buf_size,num,mode);

    if (!buf)
	buf = GetCircBuf( buf_size = 6 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num > -1000000000 ) // -1e9
    {
	s32 n32 = num;
	if ( n32 > -10000 )
	    snprintf(buf,buf_size, aligned ? "%5d" : "%d", n32 );
	else if ( n32 > -1000000 )
	    snprintf(buf,buf_size, aligned ? "%4dk" : "%dk", n32/1000 );
	else if ( n32 > -10000000 )
	{
	    n32 /= -100000;
	    snprintf(buf,buf_size,"-%u.%uM", n32/10, n32%10 );
	}
	else
	    snprintf(buf,buf_size, aligned ? "%4dM" : "%dM", n32/1000000 );
    }
    else
    {
	s32 n32;
	ccp factor = "GTPE";
	if ( num > -1000000000000000ull ) // -1e15
	    n32 = num / 100000000; // -1e8
	else
	{
	    factor += 2;
	    n32 = num / 100000000000000ull; // -1e14
	}

	while (*factor)
	{
	    if ( n32 > -100 )
	    {
		snprintf(buf,buf_size,"-%u.%u%c", -n32/10, -n32%10, *factor );
		break;
	    }
	    else if ( n32 > -10000 )
	    {
		snprintf(buf,buf_size, aligned ? "%4d%c" : "%d%c", n32/10, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberS6
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if ( num >= 0 )
	return PrintNumberU6(buf,buf_size,num,mode);

    if (!buf)
	buf = GetCircBuf( buf_size = 7 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num > -1000000000 ) // -1e9
    {
	s32 n32 = num;
	if ( n32 > -100000 )
	    snprintf(buf,buf_size, aligned ? "%6d" : "%d", n32 );
	else if ( n32 > -10000000 )
	    snprintf(buf,buf_size, aligned ? "%5dk" : "%dk", n32/1000 );
	else
	    snprintf(buf,buf_size, aligned ? "%5dM" : "%dM", n32/1000000 );
    }
    else
    {
	s32 n32;
	ccp factor = "MGTPE";
	if ( num > -1000000000000000ull ) // -1e15
	    n32 = num / 1000000; // -1e6
	else
	{
	    factor += 2;
	    n32 = num / 1000000000000ull; // -1e12
	}

	while (*factor)
	{
	    if ( n32 > -10000 )
	    {
		snprintf(buf,buf_size, aligned ? "%5d%c" : "%d%c", n32, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberS7
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s64			num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN, DC_SFORM_DASH
)
{
    if ( num >= 0 )
	return PrintNumberU7(buf,buf_size,num,mode);

    if (!buf)
	buf = GetCircBuf( buf_size = 8 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    if ( num > -100000000 ) // -1e8
    {
	s32 n32 = num;
	if ( n32 > -1000000 )
	    snprintf(buf,buf_size, aligned ? "%7d" : "%d", n32 );
	else
	    snprintf(buf,buf_size, aligned ? "%6dk" : "%dk", n32/1000 );
    }
    else
    {
	s32 n32;
	ccp factor = "MGTPE";
	if ( num > -1000000000000000ull ) // -1e15
	    n32 = num / 1000000; // -1e6
	else
	{
	    factor += 2;
	    n32 = num / 1000000000000ull; // -1e12
	}

	while (*factor)
	{
	    if ( n32 > -100000 )
	    {
		snprintf(buf,buf_size, aligned ? "%6d%c" : "%d%c", n32, *factor );
		break;
	    }
	    n32 /= 1000;
	    factor++;
	}
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PrintNumberD5
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    double		num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 16 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    double abs = fabs(num);
    if ( abs >= 1e2 )
    {
	if ( num <= 0 )
	{
	    if ( abs <= S64_MAX )
		return PrintNumberS5(buf,buf_size,double2int64(num),mode);
	    return "-****";
	}
	else
	{
	    if ( abs <= U64_MAX )
		return PrintNumberU5(buf,buf_size,double2int64(num),mode);
	}
	snprintf(buf,buf_size,"%5.0e",num);
	return buf;
    }

    if ( num < 0.0 )
    {
	if ( abs < 1e-2 )
	    return aligned ? "   -0" : "-0";

	if ( abs < 10.0 )
	    snprintf(buf,buf_size,"%5.2f",num);
	else
	    snprintf(buf,buf_size,"%5.1f",num);
    }
    else
    {
	if ( num < 1e-3 )
	    return aligned ? "    0" : "0";

	if ( num < 10.0 )
	    snprintf(buf,buf_size,"%5.3f",num);
	else
	    snprintf(buf,buf_size,"%5.2f",num);
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberD6
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    double		num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 7 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    double abs = fabs(num);
    if ( abs >= 1e3 )
    {
	if ( num <= 0 )
	{
	    if ( abs <= S64_MAX )
		return PrintNumberS6(buf,buf_size,double2int64(num),mode);
	}
	else
	{
	    if ( abs <= U64_MAX )
		return PrintNumberU6(buf,buf_size,double2int64(num),mode);
	}
	snprintf(buf,buf_size,"%6.0e",num);
	return buf;
    }

    if ( num < 0.0 )
    {
	if ( abs < 1e-3 )
	    return aligned ? "    -0" : "-0";

	if ( abs < 10.0 )
	    snprintf(buf,buf_size,"%6.3f",num);
	else if ( abs < 100.0 )
	    snprintf(buf,buf_size,"%6.2f",num);
	else
	    snprintf(buf,buf_size,"%6.1f",num);
    }
    else
    {
	if ( num < 1e-4 )
	    return aligned ? "     0" : "0";

	if ( num < 10.0 )
	    snprintf(buf,buf_size,"%6.4f",num);
	else if ( num < 100.0 )
	    snprintf(buf,buf_size,"%6.3f",num);
	else
	    snprintf(buf,buf_size,"%6.2f",num);
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberD7
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    double		num,		// number to print
    sizeform_mode_t	mode		// any of DC_SFORM_ALIGN
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 18 );
    const sizeform_mode_t aligned = mode & DC_SFORM_ALIGN;

    double abs = fabs(num);
    if ( abs >= 1e4 )
    {
	if ( num <= 0 )
	{
	    if ( abs <= S64_MAX )
		return PrintNumberS7(buf,buf_size,double2int64(num),mode);
	    snprintf(buf,buf_size,"%7.0e",num);
	    return buf;
	}
	else
	{
	    if ( abs <= U64_MAX )
		return PrintNumberU7(buf,buf_size,double2int64(num),mode);
	}
	snprintf(buf,buf_size,"%7.1e",num);
	return buf;
    }

    if ( num < 0.0 )
    {
	if ( abs < 1e-4 )
	    return aligned ? "     -0" : "-0";

	if ( abs < 10.0 )
	    snprintf(buf,buf_size,"%7.4f",num);
	else if ( abs < 100.0 )
	    snprintf(buf,buf_size,"%7.3f",num);
	else if ( abs < 1000.0 )
	    snprintf(buf,buf_size,"%7.2f",num);
	else
	    snprintf(buf,buf_size,"%7.1f",num);
    }
    else
    {
	if ( num < 1e-5 )
	    return aligned ? "      0" : "0";

	if ( num < 10.0 )
	    snprintf(buf,buf_size,"%7.5f",num);
	else if ( num < 100.0 )
	    snprintf(buf,buf_size,"%7.4f",num);
	else if ( num < 1000.0 )
	    snprintf(buf,buf_size,"%7.3f",num);
	else
	    snprintf(buf,buf_size,"%7.2f",num);
    }

    return buf;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			print size			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp dc_size_tab_1000[DC_SIZE_N_MODES+1] =
{
    0,		// DC_SIZE_DEFAULT
    0,		// DC_SIZE_AUTO

    "B",	// DC_SIZE_BYTES
    "kB",	// DC_SIZE_K
    "MB",	// DC_SIZE_M
    "GB",	// DC_SIZE_G
    "TB",	// DC_SIZE_T
    "PB",	// DC_SIZE_P
    "EB",	// DC_SIZE_E

    0		// all others
};

//-----------------------------------------------------------------------------

ccp dc_size_tab_1024[DC_SIZE_N_MODES+1] =
{
    0,		// DC_SIZE_DEFAULT
    0,		// DC_SIZE_AUTO

    "B",	// DC_SIZE_BYTES
    "KiB",	// DC_SIZE_K
    "MiB",	// DC_SIZE_M
    "GiB",	// DC_SIZE_G
    "TiB",	// DC_SIZE_T
    "PiB",	// DC_SIZE_P
    "EiB",	// DC_SIZE_E

    0		// all others
};

///////////////////////////////////////////////////////////////////////////////

ccp GetSizeUnit // get a unit for column headers
(
    size_mode_t	mode,		// print mode
    ccp		if_invalid	// output for invalid modes
)
{
    const bool force_1000 = ( mode & DC_SIZE_M_BASE ) == DC_SIZE_F_1000;

    switch ( mode & DC_SIZE_M_MODE )
    {
	//---- SI and IEC units

	case DC_SIZE_DEFAULT:
	case DC_SIZE_AUTO:	return "size";
	case DC_SIZE_BYTES:	return "bytes";

	case DC_SIZE_K:		return force_1000 ? "kB" : "KiB";
	case DC_SIZE_M:		return force_1000 ? "MB" : "MiB";
	case DC_SIZE_G:		return force_1000 ? "GB" : "GiB";
	case DC_SIZE_T:		return force_1000 ? "TB" : "TiB";
	case DC_SIZE_P:		return force_1000 ? "PB" : "PiB";
	case DC_SIZE_E:		return force_1000 ? "EB" : "EiB";
    }

    return if_invalid;
}

///////////////////////////////////////////////////////////////////////////////

int GetSizeFW // get a good value field width
(
    size_mode_t	mode,		// print mode
    int		min_fw		// minimum fw => return max(calc_fw,min_fw);
				// this value is also returned for invalid modes
)
{
    int fw = mode & (DC_SIZE_F_AUTO_UNIT|DC_SIZE_F_NO_UNIT) ? 0 : 4;

    switch ( mode & DC_SIZE_M_MODE )
    {
	case DC_SIZE_DEFAULT:
	case DC_SIZE_AUTO:
	    if ( !(mode & DC_SIZE_F_NO_UNIT) )
		fw = 4;
	    fw += 4;
	    break;

	case DC_SIZE_BYTES:	fw += 10; break;
	case DC_SIZE_K:		fw +=  7; break;
	case DC_SIZE_M:		fw +=  4; break;
	case DC_SIZE_G:		fw +=  3; break;
	case DC_SIZE_T:		fw +=  3; break;
	case DC_SIZE_P:		fw +=  3; break;
	case DC_SIZE_E:		fw +=  3; break;

	default:		fw = min_fw;
    }

    return fw > min_fw ? fw : min_fw;
}

///////////////////////////////////////////////////////////////////////////////
// [[PrintSize]]

char * PrintSize
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    sizeform_mode_t	sform_mode,	// output format, bit field
    size_mode_t		mode		// print mode
)
{
    DASSERT( DC_SIZE_N_MODES <= DC_SIZE_M_MODE + 1 );


    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    const bool force_1000 = ( mode & DC_SIZE_M_BASE ) == DC_SIZE_F_1000;

    // ??? [[2do]] follow 'sform_mode'
    const sizeform_mode_t aligned = sform_mode & DC_SFORM_ALIGN;

    switch ( mode & DC_SIZE_M_MODE )
    {
	//---- SI and IEC units

	case DC_SIZE_BYTES:
	    snprintf(buf,buf_size, aligned ? "%6llu B" : "%llu B", size );
	    break;

	case DC_SIZE_K:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu kB" : "%llu kB",
			( size + KB_SI/2 ) / KB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu KiB" : "%llu KiB",
			( size + KiB/2 ) / KiB );
	    break;

	case DC_SIZE_M:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu MB" : "%llu MB",
			( size + MB_SI /2 ) / MB_SI  );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu MiB" : "%llu MiB",
			( size + MiB /2 ) / MiB  );
	    break;

	case DC_SIZE_G:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu GB" : "%llu GB",
			( size + GB_SI/2 ) / GB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu GiB" : "%llu GiB",
			( size + GiB/2 ) / GiB );
	    break;

	case DC_SIZE_T:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu TB" : "%llu TB",
			( size + TB_SI/2 ) / TB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu TiB" : "%llu TiB",
			( size + TiB/2 ) / TiB );
	    break;

	case DC_SIZE_P:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu PB" : "%llu PB",
			( size + PB_SI/2 ) / PB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu PiB" : "%llu PiB",
			( size + PiB/2 ) / PiB );
	    break;

	case DC_SIZE_E:
	    if (force_1000)
		snprintf(buf,buf_size, aligned ? "%5llu EB" : "%llu EB",
			( size + EB_SI/2 ) / EB_SI );
	    else
		snprintf(buf,buf_size, aligned ? "%4llu EiB" : "%llu EiB",
			( size + EiB/2 ) / EiB );
	    break;


	//----- default == auto

	default:
	    buf = force_1000
			? PrintSize1000(buf,buf_size,size,sform_mode)
			: PrintSize1024(buf,buf_size,size,sform_mode);
	    if ( !(mode&DC_SIZE_F_NO_UNIT) )
		return buf;
    }

    if ( mode & (DC_SIZE_F_AUTO_UNIT|DC_SIZE_F_NO_UNIT) )
    {
	char * ptr = buf;
	while ( *ptr == ' ' )
	    ptr++;
	while ( *ptr && *ptr != ' ' )
	    ptr++;
	*ptr  = 0;
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
// [[PrintSize1000]]

char * PrintSize1000
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    sizeform_mode_t	sform_mode	// output format, bit field
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    u64 num;
    size_mode_t unit;

    u64 mb = (size+MB_SI/2)/MB_SI; // maybe an overflow => extra if
    if ( mb < 10000 && size < EB_SI )
    {
	u64 kb = (size+KB_SI/2)/KB_SI;
	if ( kb < 10 )
	{
	    num  = size;
	    unit = DC_SIZE_BYTES;
	}
	else if ( kb < 10000 )
	{
	    num  = kb;
	    unit = DC_SIZE_K;
	}
	else
	{
	    num  = mb;
	    unit = DC_SIZE_M;
	}
    }
    else
    {
	mb = size / MB_SI; // recalc because of possible overflow
	u64 tb = (mb+MB_SI/2)/MB_SI;
	if ( tb < 10000 )
	{
	    if ( tb < 10 )
	    {
		num  = (mb+KB_SI/2)/KB_SI;
		unit = DC_SIZE_G;
	    }
	    else
	    {
		num  = tb;
		unit = DC_SIZE_T;
	    }
	}
	else
	{
	    u64 pb = (mb+GB_SI/2)/GB_SI;
	    if ( pb < 10000 )
	    {
		num  = pb;
		unit = DC_SIZE_P;
	    }
	    else
	    {
		num  = (mb+TB_SI/2)/TB_SI;
		unit = DC_SIZE_E;
	    }
	}
    }

    if	(  sform_mode & DC_SFORM_INC
	&& num
	&& !( num % 1000 )
	&& dc_size_tab_1000[unit+1]
	)
    {
	unit++;
	num /= 1000;
    }

    char unit_buf[8];
    char *dest = unit_buf;
    if ( !(sform_mode & DC_SFORM_NARROW) )
	*dest++ = ' ';

    if ( sform_mode & DC_SFORM_UNIT1 )
    {
	if ( unit != DC_SIZE_BYTES )
	    *dest++ = dc_size_tab_1000[unit][0];
	else if ( sform_mode & DC_SFORM_ALIGN )
	    *dest++ = ' ';
	else
	    dest = unit_buf; // not any char
	*dest = 0;
    }
    else if ( sform_mode & DC_SFORM_ALIGN )
	snprintf(dest,sizeof(unit_buf)-1,"%-3s",dc_size_tab_1000[unit]);
    else
	StringCopyS(dest,sizeof(unit_buf)-1,dc_size_tab_1000[unit]);

    if ( !num && unit == DC_SIZE_BYTES && sform_mode & DC_SFORM_DASH )
    {
	if ( sform_mode & DC_SFORM_ALIGN )
	    snprintf(buf,buf_size,"   -%s",unit_buf);
	else
	    StringCopyS(buf,buf_size,"-");
    }
    if ( sform_mode & DC_SFORM_ALIGN )
	snprintf(buf,buf_size,"%4llu%s",num,unit_buf);
    else
	snprintf(buf,buf_size,"%llu%s",num,unit_buf);

    return buf;
};

///////////////////////////////////////////////////////////////////////////////
// [[PrintSize1024]]

char * PrintSize1024
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    sizeform_mode_t	sform_mode	// output format, bit field
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    u64 num;
    size_mode_t unit;

    u64 mib = (size+MiB/2)/MiB; // maybe an overflow => extra if
    if ( mib < 10000 && size < EiB )
    {
	u64 kib = (size+KiB/2)/KiB;
	if ( kib < 10 )
	{
	    num  = size;
	    unit = DC_SIZE_BYTES;
	}
	else if ( kib < 10000 )
	{
	    num  = kib;
	    unit = DC_SIZE_K;
	}
	else
	{
	    num  = mib;
	    unit = DC_SIZE_M;
	}
    }
    else
    {
	mib = size / MiB; // recalc because of possible overflow
	u64 tib = (mib+MiB/2)/MiB;
	if ( tib < 10000 )
	{
	    if ( tib < 10 )
	    {
		num  = (mib+KiB/2)/KiB;
		unit = DC_SIZE_G;
	    }
	    else
	    {
		num  = tib;
		unit = DC_SIZE_T;
	    }
	}
	else
	{
	    u64 pib = (mib+GiB/2)/GiB;
	    if ( pib < 10000 )
	    {
		num  = pib;
		unit = DC_SIZE_P;
	    }
	    else
	    {
		num  = (mib+TiB/2)/TiB;
		unit = DC_SIZE_E;
	    }
	}
    }

    if	(  sform_mode & DC_SFORM_INC
	&& num
	&& !( num & 0x3ff )
	&& dc_size_tab_1024[unit+1]
	)
    {
	unit++;
	num /= 0x400;
    }

    char unit_buf[8];
    char *dest = unit_buf;
    if ( !(sform_mode & DC_SFORM_NARROW) )
	*dest++ = ' ';

    if ( sform_mode & DC_SFORM_UNIT1 )
    {
	if ( unit != DC_SIZE_BYTES )
	    *dest++ = dc_size_tab_1000[unit][0];
	else if ( sform_mode & DC_SFORM_ALIGN )
	    *dest++ = ' ';
	else
	    dest = unit_buf; // not any char
	*dest = 0;
    }
    else if ( sform_mode & DC_SFORM_ALIGN )
	snprintf(dest,sizeof(unit_buf)-1,"%-3s",dc_size_tab_1024[unit]);
    else
	StringCopyS(dest,sizeof(unit_buf)-1,dc_size_tab_1024[unit]);

    if ( !num && unit == DC_SIZE_BYTES && sform_mode & DC_SFORM_DASH )
    {
	if ( sform_mode & DC_SFORM_ALIGN )
	    snprintf(buf,buf_size,"   -%s",unit_buf);
	else
	    StringCopyS(buf,buf_size,"-");
    }
    else if ( sform_mode & DC_SFORM_ALIGN )
	snprintf(buf,buf_size,"%4llu%s",num,unit_buf);
    else
	snprintf(buf,buf_size,"%llu%s",num,unit_buf);

    return buf;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			   scan size			///////////////
///////////////////////////////////////////////////////////////////////////////

u64 (*ScanSizeFactorHook)
(
    char	ch_factor,		// char to analyze
    int		force_base		// if 1000|1024: force multiple of this
) = 0;

//-----------------------------------------------------------------------------

u64 ScanSizeFactor
(
    char	ch_factor,		// char to analyze
    int		force_base		// if 1000|1024: force multiple of this
)
{
    if (ScanSizeFactorHook)
    {
	u64 res = ScanSizeFactorHook(ch_factor,force_base);
	if (res)
	    return res;
    }

    if ( force_base == 1000 )
    {
	switch (ch_factor)
	{
	    case 'b': case 'c': return     1;
	    case 'k': case 'K': return KB_SI;
	    case 'm': case 'M': return MB_SI;
	    case 'g': case 'G': return GB_SI;
	    case 't': case 'T': return TB_SI;
	    case 'p': case 'P': return PB_SI;
	    case 'e': case 'E': return EB_SI;
	}
    }
    else if ( force_base == 1024 )
    {
	switch (ch_factor)
	{
	    case 'b': case 'c': return   1;
	    case 'k': case 'K': return KiB;
	    case 'm': case 'M': return MiB;
	    case 'g': case 'G': return GiB;
	    case 't': case 'T': return TiB;
	    case 'p': case 'P': return PiB;
	    case 'e': case 'E': return EiB;
	}
    }
    else
    {
	switch (ch_factor)
	{
	    case 'b':
	    case 'c': return 1;

	    case 'k': return KB_SI;
	    case 'm': return MB_SI;
	    case 'g': return GB_SI;
	    case 't': return TB_SI;
	    case 'p': return PB_SI;
	    case 'e': return EB_SI;

	    case 'K': return KiB;
	    case 'M': return MiB;
	    case 'G': return GiB;
	    case 'T': return TiB;
	    case 'P': return PiB;
	    case 'E': return EiB;
	}
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeTerm
(
    double	*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    int		force_base		// if 1000|1024: force multiple of this
)
{
    ASSERT(source);

    char * end;
    double d = strtod(source,&end);
    if ( end > source )
    {
	// something was read

	if ( *end == '/' )
	{
	    const double div = strtod(end+1,&end);
	    if ( div > 0.0 )
		d /= div;
	}

	u64 factor = ScanSizeFactor(*end,force_base);
	if (factor)
	    end++;
	else
	    factor = default_factor;

	if (factor)
	    d *= factor;
	else
	    end = (char*)source;
    }

    if (num)
	*num = d;

    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSize
(
    double	*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base		// if 1000|1024: force multiple of this
)
{
    DASSERT(source);
    TRACE("ScanSize(df=%llx,%llx, base=%u)\n",
			default_factor, default_factor_add, force_base );

    double sum = 0.0;
    bool add = true;
    char * end = 0;
    for(;;)
    {
	double term;
	end = ScanSizeTerm(&term,source,default_factor,force_base);
	if ( end == source )
	    break;

	if (add)
	    sum += term;
	else
	    sum -= term;

	while ( *end > 0 && *end <= ' ' )
	    end++;

	if ( *end == '+' )
	    add = true;
	else if ( *end == '-' )
	    add = false;
	else
	    break;

	source = end+1;
	while ( *source > 0 && *source <= ' ' )
	    source++;

	if ( !*source && default_factor_add )
	{
	    if (add)
		sum += default_factor_add;
	    else
		sum -= default_factor_add;
	    end = (char*)source;
	    break;
	}

	default_factor = default_factor_add;
    }

    if (num)
	*num = sum;

    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeU32
(
    u32		*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base		// if 1000|1024: force multiple of this
)
{
    double d;
    char * end = ScanSize(&d,source,default_factor,default_factor_add,force_base);
    if ( d < 0 || d > ~(u32)0 )
	end = (char*)source;
    else if (num)
	*num = (u32)d;

    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeU64
(
    u64		*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base		// if 1000|1024: force multiple of this
)
{
    double d;
    char * end = ScanSize(&d,source,default_factor,default_factor_add,force_base);
    if ( d < 0 || d > ~(u64)0 )
	end = (char*)source;
    else if (num)
	*num = (u64)d;

    return end;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRange
(
    int		*stat,			// if not NULL: store result
					//	0:none, 1:single, 2:range
    double	*num1,			// not NULL: store 'from' result
    double	*num2,			// not NULL: store 'to' result

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    double	max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    if (!opt)
	opt = RAOPT__DEFAULT;

    int count = 0;
    double d1 = 0, d2;

    bool have_d1 = false;
    if ( opt & RAOPT_ASSUME_0 )
    {
	have_d1	=  *source == ':' && opt & RAOPT_COLON
		|| *source == '#' && opt & RAOPT_HASH
		|| *source == ',' && opt & RAOPT_COMMA
		|| *source == '-' && (opt & (RAOPT_COMMA|RAOPT_NEG)) == RAOPT_COMMA;
    }

    char *end = have_d1 ? (char*)source
		: ScanSize(&d1,source,default_factor,default_factor_add,force_base);
    if ( !have_d1 && end == source )
    {
	d1 = d2 = 0.0;
	count = 0;
    }
    else
    {
	count++;
	d2 = d1;

	const bool have_range	=  *end == ':' && opt & RAOPT_COLON
				|| *end == '-' && opt & RAOPT_MINUS;
	const bool have_size	=  *end == '#' && opt & RAOPT_HASH
				|| *end == ',' && opt & RAOPT_COMMA;
	PRINT0("have_d1=%d, have_range=%d, have_size=%d, %.0g:%.0g |%s|\n",have_d1,have_range,have_size,d1,d2,end);
	if ( have_range || have_size )
	{
	    if ( d1 < 0 && max_value > 0 && opt & RAOPT_NEG )
	    {
		d1 += max_value;
		if ( d1 < 0 )
		    d1 = 0;
	    }

	    source = end;
	    end = ScanSize(&d2,source+1,default_factor,default_factor_add,force_base);
	    if ( have_range && d2 < 0 && max_value > 0 && opt & RAOPT_NEG )
	    {
		d2 += max_value;
		if ( d2 < 0 )
		    d2 = 0;
	    }

	    if ( end == source+1 )
	    {
		if ( have_range && max_value > 0.0 )
		{
		    count++;
		    d2 = max_value;
		    if ( *end == '*' )
			end++;
		}
		else
		{
		    end--;
		    d2 = d1;
		}
	    }
	    else
	    {
		count++;
		if ( have_size )
		    d2 += d1-1;
	    }
	    if ( d2 < d1 )
	    {
		count = 0;
		d2 = d1;
	    }
	}
    }

    if (stat)
	*stat = count;
    if (num1)
	*num1 = d1;
    if (num2)
	*num2 = d2;
    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeU32
(
    int		*stat,			// if not NULL: store result
					//	0:none, 1:single, 2:range
    u32		*num1,			// not NULL: store 'from' result
    u32		*num2,			// not NULL: store 'to' result

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    u32		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    double d1, d2;
    char * end = ScanSizeRange(stat,&d1,&d2,source,
			default_factor,default_factor_add,force_base,
			max_value>0 ? max_value : U32_MAX, opt );

    if ( d1 < 0 || d1 > U32_MAX || d2 < 0 || d2 > U32_MAX )
    {
	end = (char*)source;
	if (stat)
	    *stat = 0;
    }
    else
    {
	if (num1)
	    *num1 = (u32)d1;
	if (num2)
	    *num2 = (u32)d2;
    }

    return end;

}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeS32
(
    int		*stat,			// if not NULL: store result
					//	0:none, 1:single, 2:range
    s32		*num1,			// not NULL: store 'from' result
    s32		*num2,			// not NULL: store 'to' result

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    s32		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    double d1, d2;
    char * end = ScanSizeRange(stat,&d1,&d2,source,
			default_factor,default_factor_add,force_base,
			max_value>0 ? max_value : S32_MAX, opt );

    if ( d1 < S32_MIN || d1 > S32_MAX || d2 < S32_MIN || d2 > U32_MAX )
    {
	end = (char*)source;
	if (stat)
	    *stat = 0;
    }
    else
    {
	if (num1)
	    *num1 = (s32)d1;
	if (num2)
	    *num2 = (s32)d2;
    }

    return end;

}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeU64
(
    int		*stat,			// if not NULL: store result
					//	0:none, 1:single, 2:range
    u64		*num1,			// not NULL: store 'from' result
    u64		*num2,			// not NULL: store 'to' result

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    u64		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    double d1, d2;
    char * end = ScanSizeRange(stat,&d1,&d2,source,
			default_factor,default_factor_add,force_base,
			max_value>0 ? max_value : U64_MAX, opt );

    if ( d1 < 0 || d1 > U64_MAX || d2 < 0 || d2 > U64_MAX )
    {
	end = (char*)source;
	if (stat)
	    *stat = 0;
    }
    else
    {
	if (num1)
	    *num1 = (u64)d1;
	if (num2)
	    *num2 = (u64)d2;
    }

    return end;

}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeS64
(
    int		*stat,			// if not NULL: store result
					//	0:none, 1:single, 2:range
    s64		*num1,			// not NULL: store 'from' result
    s64		*num2,			// not NULL: store 'to' result

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    s64		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    double d1, d2;
    char * end = ScanSizeRange(stat,&d1,&d2,source,
			default_factor,default_factor_add,force_base,
			max_value>0 ? max_value : S64_MAX, opt );

    if ( d1 < S64_MIN || d1 > S64_MAX || d2 < S64_MIN || d2 > U64_MAX )
    {
	end = (char*)source;
	if (stat)
	    *stat = 0;
    }
    else
    {
	if (num1)
	    *num1 = (s64)d1;
	if (num2)
	    *num2 = (s64)d2;
    }

    return end;

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeList
(
    uint	*n_range,		// not NULL: store number of scanned ranges
    double	*num,			// array with '2*max_range' elements
					//	unused elements are filled with 0.0
    uint	max_range,		// max number of allowed ranges

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    double	max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    DASSERT(num);

    uint i = 0;
    char *ptr = (char*)source;

    while ( i < max_range )
    {
	while ( *ptr > 0 && *ptr <= ' ' || *ptr == ',' )
	    ptr++;

	int stat;
	ptr = ScanSizeRange( &stat, num, num+1, ptr,
			default_factor, default_factor_add,
			force_base, max_value, opt );
	if (!stat)
	    break;
	num += 2;
	i++;

	char *ptr2 = ptr;
	while ( *ptr2 > 0 && *ptr2 <= ' ' )
	    ptr2++;
	if ( *ptr2 != ',' )
	    break;

	ptr = ptr2 + 1;
    }

    if (n_range)
	*n_range = i;

    while ( i < max_range )
    {
	*num++ = 0.0;
	*num++ = 0.0;
	i++;
    }

    return ptr;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeListU32
(
    uint	*n_range,		// not NULL: store number of scanned ranges
    u32		*num,			// array with '2*max_range' elements
					//	unused elements are filled with 0
    uint	max_range,		// max number of allowed ranges

    ccp		source,			// source text
    u32		default_factor,		// use this factor if number hasn't one
    u32		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    u32		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    DASSERT(num);

    const uint max = 100;
    double d_buf[2*max];
    double *d = max_range < max ? d_buf : MALLOC(max_range*2*sizeof(double));

    char *end = ScanSizeRangeList( n_range, d, max_range,
			source, default_factor, default_factor_add,
			force_base, max_value, opt );

    uint i;
    for ( i = 0; i < 2*max_range; i++ )
	num[i] = d[i] < 0.0 ? 0.0 : d[i] < U32_MAX ? d[i] : U32_MAX;

    if ( d != d_buf )
	FREE(d);
    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanSizeRangeListU64
(
    uint	*n_range,		// not NULL: store number of scanned ranges
    u64		*num,			// array with '2*max_range' elements
					//	unused elements are filled with 0
    uint	max_range,		// max number of allowed ranges

    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this
    u64		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
)
{
    DASSERT(num);

    const uint max = 100;
    double d_buf[2*max];
    double *d = max_range < max ? d_buf : MALLOC(max_range*2*sizeof(double));

    char *end = ScanSizeRangeList( n_range, d, max_range,
			source, default_factor, default_factor_add,
			force_base, max_value, opt );

    uint i;
    for ( i = 0; i < 2*max_range; i++ )
	num[i] = d[i] < 0.0 ? 0.0 : d[i] < U64_MAX ? d[i] : U64_MAX;

    if ( d != d_buf )
	FREE(d);
    return end;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ScanSizeOpt
(
    double	*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this

    ccp		opt_name,		// NULL or name of option for error messages
    u64		min,			// minimum allowed value
    u64		max,			// maximum allowed value
    bool	print_err		// true: print error messages
)
{
    double d;
    char * end = ScanSize(&d,source,default_factor,default_factor_add,force_base);

 #ifdef DEBUG
    {
	u64 size = d;
	TRACE("--%s %8.6g ~ %llu ~ %llu GiB ~ %llu GB\n",
		opt_name, d, size, (size+GiB/2)/GiB, (size+500000000)/1000000000 );
    }
 #endif

    enumError err = ERR_OK;

    if ( source == end || *end )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SYNTAX,
			"Invalid number for option --%s: %s\n",
			opt_name, source );
    }
    else if ( min > 0 && d < min )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SEMANTIC,
			"Value of --%s too small (must not <%llu): %s\n",
			opt_name, min, source );
    }
    else if ( max > 0 && d > max )
    {
	err = ERR_SYNTAX;
	if (print_err)
	    ERROR0(ERR_SEMANTIC,
			"Value of --%s too large (must not >%llu): %s\n",
			opt_name, max, source );
    }

    if ( num && !err )
	*num = d;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanSizeOptU64
(
    u64		*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    int		force_base,		// if 1000|1024: force multiple of this

    ccp		opt_name,		// NULL or name of option for error messages
    u64		min,			// >0: minimum allowed value
    u64		max,			// >0: maximum allowed value
    u32		multiple,		// >0: result must be multiple
    u32		pow2,			// >0: result must power of '1<<pow2'
    bool	print_err		// true: print error messages
)
{
    if (!max)
	max = ~(u64)0;

    if ( pow2 && !force_base )
    {
	// try base 1024 first without error messages
	u64 val;
	if (!ScanSizeOptU64( &val, source, default_factor, 1024,
				opt_name, min,max, multiple, pow2, false ))
	{
	    if (num)
		*num = val;
	    return ERR_OK;
	}
    }

    double d = 0.0;
    enumError err = ScanSizeOpt(&d,source,default_factor,
				multiple ? multiple : 1,
				force_base,opt_name,min,max,print_err);

    u64 val;
    if ( d < 0.0 )
    {
	val = 0;
	err = ERR_SEMANTIC;
	if (print_err)
	    ERROR0(ERR_SEMANTIC, "--%s: negative values not allowed: %s\n",
			opt_name, source );
    }
    else
	val = d;

    if ( err == ERR_OK && pow2 > 0 )
    {
	int shift_count = 0;
	u64 shift_val = val;
	if (val)
	{
	    while (!(shift_val&1))
	    {
		shift_count++;
		shift_val >>= 1;
	    }
	}

	if ( shift_val != 1 || shift_count/pow2*pow2 != shift_count )
	{
	    err = ERR_SEMANTIC;
	    if (print_err)
		ERROR0(ERR_SYNTAX,
			"--%s: value must be a power of %d but not %llu\n",
			opt_name, 1<<pow2, val );
	}
    }

    if ( err == ERR_OK && multiple > 1 )
    {
	u64 xval = val / multiple * multiple;
	if ( xval != val )
	{
	    if ( min > 0 && xval < min )
		xval += multiple;

	    if (print_err)
		ERROR0(ERR_WARNING,
		    "--%s: value must be a multiple of %u -> use %llu instead of %llu.\n",
		    opt_name, multiple, xval, val );
	    val = xval;
	}
    }

    if ( num && !err )
	*num = val;
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanSizeOptU32
(
    u32		*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    int		force_base,		// if 1000|1024: force multiple of this

    ccp		opt_name,		// NULL or name of option for error messages
    u64		min,			// >0: minimum allowed value
    u64		max,			// >0: maximum allowed value
    u32		multiple,		// >0: result must be multiple
    u32		pow2,			// >0: result must power of '1<<pow2'
    bool	print_err		// true: print error messages
)
{
    if ( !max || max > ~(u32)0 )
	max = ~(u32)0;

    u64 val = 0;
    enumError err = ScanSizeOptU64( &val, source, default_factor, force_base,
				opt_name, min, max, multiple, pow2, print_err );

    if ( num && !err )
	*num = (u32)val;
    return err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// strto*() like wrappers

u32 GetSizeU32	( ccp src, char **end, int force_base )
{
    u32 num = 0;
    char *ptr = ScanSizeU32(&num,src,1,1,force_base);
    if (end)
	*end = ptr;
    return num;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetSizeU64	( ccp src, char **end, int force_base )
{
    u64 num = 0;
    char *ptr = ScanSizeU64(&num,src,1,1,force_base);
    if (end)
	*end = ptr;
    return num;
}

///////////////////////////////////////////////////////////////////////////////

double GetSizeD	( ccp src, char **end, int force_base )
{
    double num = 0.0;
    char *ptr = ScanSize(&num,src,1,1,force_base);
    if (end)
	*end = ptr;
    return num;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Scan Duration			///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanSIFactor
(
    // returns end of scanned string

    double	*num,			// not NULL: store result
    ccp		source,			// source text
    double	default_factor		// return this if no factor found
)
{
    DASSERT(source);
    double i = 0.0;
    switch (*(uchar*)source++)
    {
	case 'y': default_factor = 1e-24; break;
	case 'z': default_factor = 1e-21; break;
	case 'a': default_factor = 1e-18; break;
	case 'f': default_factor = 1e-15; break;
	case 'p': default_factor = 1e-12; break;
	case 'n': default_factor = 1e-9; break;
	case 0xb5: // µ as ASCII
	case 'u': default_factor = 1e-6; break;
	case 'm': default_factor = 1e-3; break;
	case 'c': default_factor = 1e-2; break;
	case 'd': default_factor = 1e-1; break;

	case 'h': default_factor = 1e2; break;
	case 'k':
	case 'K': default_factor = 1e3;  i =                      1024.0; break;
	case 'M': default_factor = 1e6;  i =                   1048576.0; break;
	case 'G': default_factor = 1e9;  i =                1073741824.0; break;
	case 'T': default_factor = 1e12; i =             1099511627776.0; break;
	case 'P': default_factor = 1e15; i =          1125899906842624.0; break;
	case 'E': default_factor = 1e18; i =       1152921504606846976.0; break;
	case 'Z': default_factor = 1e21; i =    1180591620717411303424.0; break;
	case 'Y': default_factor = 1e24; i = 1208925819614629174706176.0; break;

	case 0xc2:
	    if ( (uchar)*source == 0xb5 )
	    {
		// µ as UTF-8
		source++;
		default_factor = 1e-6;
		break;
	    }
	    else
		source--;
	    break;

	default:
	   source--;
    }

    if ( *source == 'i' && i )
    {
	source++;
	default_factor = i;
    }

    if (num)
	*num = default_factor;
    return (char*)source;
}

///////////////////////////////////////////////////////////////////////////////

double GetDurationFactor ( char ch )
{
    switch (ch)
    {
	case 's': return 1.0; break;
	case 'm': return SEC_PER_MIN; break;
	case 'h': return SEC_PER_HOUR; break;
	case 't':
	case 'd': return SEC_PER_DAY; break;
	case 'w': return SEC_PER_WEEK; break;
	case 'j':
	case 'y': return SEC_PER_YEAR; break;
	default:  return 0.0;
    }
}

///////////////////////////////////////////////////////////////////////////////

char * ScanDuration
(
    // Returns a pointer to the the first not used character.
    // On error, it is 'source' and '*seconds' is set to 0.0.

    double		*seconds,	// not NULL: store result (number of seconds)
    ccp			source,		// source text
    double		default_factor,	// default factor if no SI unit found
    ScanDuration_t	mode
)
{
    if (!source)
	return 0;
    ccp src, done;
    done = src = source;

    if ( mode & SDUMD_SKIP_BLANKS )
	while ( *src == ' ' || *src == '\t' )
	    src++;

    if ( mode & SDUMD_ALLOW_LIST )
    {
	bool valid = false;
	mode &= ~(SDUMD_SKIP_BLANKS|SDUMD_ALLOW_LIST);
	double sum = 0.0;
	bool sub = false;
	for(;;)
	{
	    double val;
	    ccp end = ScanDuration(&val,src,default_factor,mode);
	    if ( src == end )
		break;
	    if (sub)
	    {
		sub = false;
		sum -= val;
	    }
	    else
		sum += val;
	    done = src = end;
	    valid = true;

	    if ( mode & SDUMD_ALLOW_SPACE_SEP )
		while ( *src == ' ' || *src == '\t' )
		    src++;
	    if ( *src == '+' && mode & SDUMD_ALLOW_PLUS_SEP )
		src++;
	    else if ( *src == '-' && mode & SDUMD_ALLOW_MINUS_SEP )
	    {
		src++;
		sub = true;
	    }
	    mode |= SDUMD_SKIP_BLANKS;
	}
	if (seconds)
	    *seconds = sum;
	return (char*)( valid ? done : source );
    }


    //--- single scan

    char *end;
    double val = strtod(src,&end);
    if ( end > src )
    {
	if ( *end == ':' && mode & SDUMD_ALLOW_COLON )
	{
	    // 'H:M' or 'H:M:S'
	    src = end + 1;
	    val *= 3600;
	    double min = strtod(src,&end);
	    if ( end > src )
	    {
		val += 60*min;
		src = end;
		if ( *src == ':' )
		{
		    // 'H:M:S'
		    double sec = strtod(++src,&end);
		    if ( end > src )
		    {
			src = end;
			val += sec;
		    }
		}
	    }
	}
	else
	{
	    if ( *end == '/' && mode & SDUMD_ALLOW_DIV )
	    {
		const double div = strtod(end+1,&end);
		if ( div > 0.0 )
		    val /= div;
	    }
	    src = end;

	    double si_factor;
	    end = ScanSIFactor(&si_factor,src,1.0);
	    char factor = *end;
	    if ( mode & SDUMD_ALLOW_CAPITALS )
		factor = tolower((int)factor);
	    double unit = GetDurationFactor(factor);

	    if (unit)
	    {
		val *= unit * si_factor;
		src = end+1;
	    }
	    else
	    {
		char factor = *src;
		if ( mode & SDUMD_ALLOW_CAPITALS )
		    factor = tolower((int)factor);
		unit = GetDurationFactor(factor);
		if (unit)
		{
		    val *= unit;
		    src++;
		}
		else
		{
		    val *= (end>src) ? si_factor : default_factor;
		    src = end;
		}
	    }
	}
	if (seconds)
	    *seconds = val;
	return (char*)src;
    }

    if (seconds)
	*seconds = 0.0;
    return (char*)source;
}

///////////////////////////////////////////////////////////////////////////////

double str2duration ( ccp src, char **endptr, double default_factor )
{
    double dur = 0.0;
    char *end = ScanDuration(&dur,src,default_factor,SDUMD_M_DEFAULT);
    if (endptr)
	*endptr = end;
    return dur;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanIP4				///////////////
///////////////////////////////////////////////////////////////////////////////

ipv4_class_t ScanIP4
(
    ipv4_t	*ret_ip4,	// not NULL: return numeric IPv4 here
    ccp		addr,		// address to analyze
    int		addr_len	// length of addr; if <0: use strlen(addr)
)
{
    //--- param check

    if (!addr)
    {
     error:
	if (ret_ip4)
	    *ret_ip4 = 0;
	return CIP4_ERROR;
    }

    if ( addr_len < 0 )
	addr_len = strlen(addr);

    ccp addr_end = addr + addr_len;


    //--- skip leading blanks

    while ( addr < addr_end && isblank((int)*addr) )
	addr++;


    //--- scan input

    int count = 0;
    bool trailing_pt = false;
    uint num[4] = {0,0,0,0};
    while ( addr < addr_end )
    {
	if ( *addr < '0' || *addr > '9' )
	    break;

	addr = ScanNumber ( num+count++, addr, addr_end, *addr == '0' ? 8 : 10, 3 );
	if ( count == 4 || *addr != '.' )
	{
	    trailing_pt = false;
	    break;
	}
	addr++;
	trailing_pt = true;
    }

    //printf("N=%u .=%u [%u,%u,%u,%u] addr=%s\n",count,trailing_pt,num[0],num[1],num[2],num[3],addr);

    if ( !count || addr < addr_end )
	goto error;


    //--- calculate IPv4 based on class-X

    ipv4_class_t res = CIP4_INVALID;
    u32 ipv4 = 0;

    switch ( trailing_pt ? 4 : count )
    {
     case 1:
	ipv4 = num[0];
	res = CIP4_NUMERIC;
	break;

     case 2:
	if ( num[0] <= 0xff && num[1] <= 0xffffff )
	{
	    ipv4 = num[0] << 24 | num[1];
	    res = CIP4_CLASS_A;
	}
	break;

     case 3:
	if ( num[0] <= 0xff && num[1] <= 0xff && num[2] <= 0xffff )
	{
	    ipv4 = num[0] << 24 | num[1] << 16 | num[2];
	    res = CIP4_CLASS_B;
	}
	break;

     case 4:
	if ( num[0] <= 0xff && num[1] <= 0xff && num[2] <= 0xff && num[3] <= 0xff )
	{
	    ipv4 = num[0] << 24 | num[1] << 16 | num[2] << 8 | num[3];
	    res = CIP4_CLASS_C;
	}
	break;
    }


    //--- return status

    if (ret_ip4)
	*ret_ip4 = ipv4;

    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * NormalizeIP4
(
    // Return a pointer to dest if valid IPv4,
    // Otherwise return 0 and buf is not modifieid.

    char	*buf,		// result buffer, can be 'addr'
				// if NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    bool	c_notation,	// TRUE: force lass-C notation (a.b.c.d)

    ccp		addr,		// address to analyze
    int		addr_len	// length of addr; if <0: use strlen(addr)
)
{
    ipv4_t ip4;
    ipv4_class_t stat = ScanIP4(&ip4,addr,-1);
    if ( stat <= CIP4_INVALID )
	return 0;

    return PrintIP4ByMode( buf, buf_size, ip4, c_notation ? CIP4_CLASS_C : stat, -1 );
}

///////////////////////////////////////////////////////////////////////////////

int dclib_aton ( ccp addr, struct in_addr *inp )
{
    ipv4_t ip4;
    ipv4_class_t stat = ScanIP4(&ip4,addr,-1);
    if (inp)
	inp->s_addr = htonl(ip4);
    return stat > CIP4_INVALID;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PrintIP4*()			///////////////
///////////////////////////////////////////////////////////////////////////////

char * PrintIP4N
(
    // print as unsigned number (CIP4_NUMERIC)

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = FW_IPV4_A_PORT+1 );

    if ( port >= 0 && port <= 0xffff )
	snprintf(buf,buf_size,"%u:%u", ip4, port );
    else
	snprintf(buf,buf_size,"%u",ip4);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintIP4A
(
    // print in A-notation (CIP4_CLASS_A)

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = FW_IPV4_A_PORT+1 );

    u8 addr[4];
    write_be32(addr,ip4);

    if ( port >= 0 && port <= 0xffff )
	snprintf(buf,buf_size,"%u.%u:%u",
		addr[0], ip4 & 0xffffff, port );
    else
	snprintf(buf,buf_size,"%u.%u",
		addr[0], ip4 & 0xffffff );
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintIP4B
(
    // print in B-notation (CIP4_CLASS_B)

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = FW_IPV4_B_PORT+1 );

    u8 addr[4];
    write_be32(addr,ip4);

    if ( port >= 0 && port <= 0xffff )
	snprintf(buf,buf_size,"%u.%u.%u:%u",
		addr[0], addr[1], ip4 & 0xffff, port );
    else
	snprintf(buf,buf_size,"%u.%u.%u",
		addr[0], addr[1], ip4 & 0xffff );
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = FW_IPV4_C_PORT+1 );

    u8 addr[4];
    write_be32(addr,ip4);

    if ( port >= 0 && port <= 0xffff )
	snprintf(buf,buf_size,"%u.%u.%u.%u:%u",
		addr[0], addr[1], addr[2], addr[3], port );
    else
	snprintf(buf,buf_size,"%u.%u.%u.%u",
		addr[0], addr[1], addr[2], addr[3] );
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintLeftIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = FW_IPV4_C_PORT+1 );

    u8 addr[4];
    write_be32(addr,ip4);
    uint len, fw;

    if ( port >= 0 && port <= 0xffff )
    {
	fw = 21;
	len = snprintf(buf,buf_size,"%u.%u.%u.%u:%u",
			addr[0], addr[1], addr[2], addr[3], port );
    }
    else
    {
	fw = 15;
	len = snprintf(buf,buf_size,"%u.%u.%u.%u",
			addr[0], addr[1], addr[2], addr[3] );
    }

    char *dest = buf + len;
    char *end  = buf + ( fw < buf_size ? fw : buf_size );
    while ( dest < end )
	*dest++ = ' ';
    *dest = 0;

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintRightIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port,		// 0..0xffff: add ':port'
    uint	port_mode	// 0: 'ip:port', 1: 'ip :port', 2: 'ip : port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 24 );

    u8 addr[4];
    write_be32(addr,ip4);

    char tempbuf[16];
    snprintf(tempbuf,sizeof(tempbuf),"%u.%u.%u.%u",
			addr[0], addr[1], addr[2], addr[3] );

    if ( port >= 0 && port <= 0xffff )
    {
	if (!port_mode)
	    snprintf(buf,buf_size,"%15s:%-5u",tempbuf,port);
	else if ( port_mode == 1 )
	    snprintf(buf,buf_size,"%15s :%-5u",tempbuf,port);
	else
	    snprintf(buf,buf_size,"%15s : %-5u",tempbuf,port);
    }
    else
	snprintf(buf,buf_size,"%15s",tempbuf);

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintAlignedIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = FW_IPV4_C_PORT+1 );

    u8 addr[4];
    write_be32(addr,ip4);

    if ( port >= 0 && port <= 0xffff )
	snprintf(buf,buf_size,"%3u.%3u.%3u.%3u:%5u",
		addr[0], addr[1], addr[2], addr[3], port );
    else
	snprintf(buf,buf_size,"%3u.%3u.%3u.%3u",
		addr[0], addr[1], addr[2], addr[3] );
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintIP4ByMode
(
    char	 *buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	 buf_size,	// size of 'buf', ignored if buf==NULL
    u32		 ip4,		// IP4 to print
    ipv4_class_t mode,		// CIP4_NUMERIC | CIP4_CLASS_A | CIP4_CLASS_B:
				// force numeric or class-A/B output; otherwise class-C output
    s32		 port		// 0..0xffff: add ':port'
)
{
    switch (mode)
    {
	case CIP4_NUMERIC: return PrintIP4N(buf,buf_size,ip4,port);
	case CIP4_CLASS_A: return PrintIP4A(buf,buf_size,ip4,port);
	case CIP4_CLASS_B: return PrintIP4B(buf,buf_size,ip4,port);
	default:	   return PrintIP4 (buf,buf_size,ip4,port);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				CRC16			///////////////
///////////////////////////////////////////////////////////////////////////////

void CreateCRC16Table ( u16 table[0x100], u16 polynom )
{
    int i, j;
    for ( i = 0; i < 256; i++ )
    {
	u16 val = i << 8;
	for ( j = 0; j < 8; j++ )
	{
	    if ( val & 0x8000 )
		val = val << 1 ^ polynom;
	    else
		val <<= 1;
	}
	table[i] = val;
    }
}

//-----------------------------------------------------------------------------

const u16 * GetCRC16Table ( u16 polynom )
{
    static u16 *table = 0;
    static u16 active_polynom = 0;

    if ( !table || active_polynom != polynom )
    {
	if (!table)
	    table = MALLOC(sizeof(u16)*0x100);
	CreateCRC16Table(table,polynom);
	active_polynom = polynom;
    }
    return table;
}

//-----------------------------------------------------------------------------

u16 CalcCRC16 ( cvp data, uint data_size, u16 polynom, u16 preset )
{
    DASSERT( data || !data_size );

    register u16 crc = preset;
    if ( data_size > 0 )
    {
	const u16 *table = GetCRC16Table(polynom);
	const u8 *ptr = data;
	while ( data_size-- > 0 )
	    crc = table[ ( *ptr++ ^ crc >> 8 ) & 0xff ] ^ crc << 8;
    }
    return crc;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    misc			///////////////
///////////////////////////////////////////////////////////////////////////////

float double2float ( double d )
{
    // reduce precision
    return d;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u64 MulDivU64 ( u64 factor1, u64 factor2, u64 divisor )
{
 #if HAVE_INT128

    return (u128)factor1 * (u128)factor2 / divisor;

 #else

    if ( !factor1 || !factor2 )
	return 0;

    const double product = (double)factor1 * (double)factor2;
    return product < 0xfffffffffffffff0
	? factor1 * factor2 / divisor
	: (u64) trunc( product / divisor );

 #endif
}

//-----------------------------------------------------------------------------

s64 MulDivS64 ( s64 factor1, s64 factor2, s64 divisor )
{
    bool neg = false;
    if ( factor1 < 0 ) { factor1 = -factor1; neg = !neg; }
    if ( factor2 < 0 ) { factor2 = -factor2; neg = !neg; }
    if ( divisor < 0 ) { divisor = -divisor; neg = !neg; }
    s64 r = MulDivU64(factor1,factor2,divisor);
    return neg ? -r : r;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintNiceID4 ( uint id )
{
    if (!id)
	return "----";

    char *buf = GetCircBuf(5);
    DASSERT(buf);

 #if 0 // the second solution is much faster!

    buf[4]  = 0;
    div_t d = div( (int)( id % (26*26*100) ), 10 );
    buf[3]  = '0' + d.rem;
    d       = div(d.quot,10);
    buf[2]  = '0' + d.rem;
    d       = div(d.quot,26);
    buf[1]  = 'A' + d.rem;
    buf[0]  = 'A' + d.quot;

 #else

    buf[0] = 'A' + id / 2600 % 26;
    buf[1] = 'A' + id /  100 % 26;
    buf[2] = '0' + id /   10 % 10;
    buf[3] = '0' + id        % 10;
    buf[4] = 0;

 #endif

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanNiceID4 ( int * res, ccp arg )
{
    DASSERT(res);
    *res = -1;

    if (!arg)
	return 0;

    char ch = tolower((int)arg[0]);
    if ( ch >= 'a' && ch <= 'z' )
    {
	int num = ch - 'a';
	ch = tolower((int)arg[1]);
	if ( ch >= 'a' && ch <= 'z' )
	{
	    num = 26 * num + ch - 'a';
	    ch = arg[2];
	    if ( ch >= '0' && ch <= '9' )
	    {
		num = 10 * num + ch - '0';
		ch = arg[3];
		if ( ch >= '0' && ch <= '9' )
		{
		    *res = 10 * num + ch - '0';
		    return (char*)arg+4;
		}
	    }
	}
    }
    return (char*)arg;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintNiceID5 ( uint id )
{
    if (!id)
	return "-----";

    char *buf = GetCircBuf(6);
    DASSERT(buf);

    buf[0] = 'A' + id / 26000 % 26;
    buf[1] = 'A' + id /  1000 % 26;
    buf[2] = '0' + id /   100 % 10;
    buf[3] = '0' + id /    10 % 10;
    buf[4] = '0' + id         % 10;
    buf[5] = 0;

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanNiceID5 ( int * res, ccp arg )
{
    DASSERT(res);
    *res = -1;

    if (!arg)
	return 0;

    char ch = tolower((int)arg[0]);
    if ( ch >= 'a' && ch <= 'z' )
    {
	int num = ch - 'a';
	ch = tolower((int)arg[1]);
	if ( ch >= 'a' && ch <= 'z' )
	{
	    num = 26 * num + ch - 'a';
	    ch = arg[2];
	    if ( ch >= '0' && ch <= '9' )
	    {
		num = 10 * num + ch - '0';
		ch = arg[3];
		if ( ch >= '0' && ch <= '9' )
		{
		    num = 10 * num + ch - '0';
		    ch = arg[4];
		    if ( ch >= '0' && ch <= '9' )
		    {
			*res = 10 * num + ch - '0';
			return (char*)arg+5;
		    }
		}
	    }
	}
    }
    return (char*)arg;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintNiceID6 ( uint id )
{
    if (!id)
	return "------";

    char *buf = GetCircBuf(7);
    DASSERT(buf);

    buf[0] = 'A' + id / 676000 % 26;
    buf[1] = 'A' + id /  26000 % 26;
    buf[2] = 'A' + id /   1000 % 26;
    buf[3] = '0' + id /    100 % 10;
    buf[4] = '0' + id /     10 % 10;
    buf[5] = '0' + id          % 10;
    buf[6] = 0;

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanNiceID6 ( int * res, ccp arg )
{
    DASSERT(res);
    *res = -1;

    if (!arg)
	return 0;

    char ch = tolower((int)arg[0]);
    if ( ch >= 'a' && ch <= 'z' )
    {
	int num = ch - 'a';
	ch = tolower((int)arg[1]);
	if ( ch >= 'a' && ch <= 'z' )
	{
	    num = 26 * num + ch - 'a';
	    ch = tolower((int)arg[2]);
	    if ( ch >= 'a' && ch <= 'z' )
	    {
		num = 26 * num + ch - 'a';
		ch = arg[3];
		if ( ch >= '0' && ch <= '9' )
		{
		    num = 10 * num + ch - '0';
		    ch = arg[4];
		    if ( ch >= '0' && ch <= '9' )
		    {
			num = 10 * num + ch - '0';
			ch = arg[5];
			if ( ch >= '0' && ch <= '9' )
			{
			    *res = 10 * num + ch - '0';
			    return (char*)arg+6;
			}
		    }
		}
	    }
	}
    }
    return (char*)arg;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

