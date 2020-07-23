
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
 *        Copyright (c) 2012-2018 by Dirk Clemens <wiimm@wiimm.de>         *
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

#ifndef DCLIB_BASICS_H
#define DCLIB_BASICS_H 1

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <arpa/inet.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef WIN_DCLIB
  #include <stdint.h>
#endif

#include "dclib-types.h"
#include "dclib-debug.h"
#include "dclib-color.h"
#include "dclib-numeric.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

#define SIZEOFMEMBER(type,a) sizeof(((type*)0)->a)

#define SIZEOFRANGE(type,a,b) \
	( offsetof(type,b) + sizeof(((type*)0)->b) - offsetof(type,a) )

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    bits			///////////////
///////////////////////////////////////////////////////////////////////////////

extern const uchar TableBitCount[0x100];
extern const char  TableLowest0Bit[0x100];
extern const char  TableLowest1Bit[0x100];
extern const char  TableHighest0Bit[0x100];
extern const char  TableHighest1Bit[0x100];

uint Count1Bits ( cvp addr, uint size );
static inline uint Count0Bits ( cvp addr, uint size )
	{ return CHAR_BIT * size - Count1Bits(addr,size); }

#define Count1BitsObj(obj) Count1Bits(&(obj),sizeof(obj))
#define Count0BitsObj(obj) Count1Bits(&(obj),sizeof(obj))

static inline uint Count1Bits8 ( u8 data )
{
    return TableBitCount[data];
}

static inline uint Count1Bits16 ( u16 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]];
}

static inline uint Count1Bits32 ( u32 data )
{
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]];
}

///////////////////////////////////////////////////////////////////////////////

int FindLowest0BitBE  ( cvp addr, uint size );
int FindLowest1BitBE  ( cvp addr, uint size );
int FindHighest0BitBE ( cvp addr, uint size );
int FindHighest1BitBE ( cvp addr, uint size );

int FindLowest0BitLE  ( cvp addr, uint size );
int FindLowest1BitLE  ( cvp addr, uint size );
int FindHighest0BitLE ( cvp addr, uint size );
int FindHighest1BitLE ( cvp addr, uint size );

#if __BYTE_ORDER == 1234
  #define FindLowest0Bit  FindLowest0BitLE
  #define FindLowest1Bit  FindLowest1BitLE
  #define FindHighest0Bit FindHighest0BitLE
  #define FindHighest1Bit FindHighest1BitLE
#else
  #define FindLowest0Bit  FindLowest0BitBE
  #define FindLowest1Bit  FindLowest1BitBE
  #define FindHighest0Bit FindHighest0BitBE
  #define FindHighest1Bit FindHighest1BitBE
#endif

u32 GetAlign32 ( u32 data );
u64 GetAlign64 ( u64 data );

///////////////////////////////////////////////////////////////////////////////

static inline void SetBit ( void *addr, uint bit_num )
	{ ((u8*)addr)[ bit_num >> 3 ] |= 1 << ( bit_num & 7 ); }

static inline void ClearBit ( void *addr, uint bit_num )
	{ ((u8*)addr)[ bit_num >> 3 ] &= ~(1 << ( bit_num & 7 )); }

static inline void InvertBit ( void *addr, uint bit_num )
	{ ((u8*)addr)[ bit_num >> 3 ] ^= 1 << ( bit_num & 7 ); }

static inline bool TestBit ( const void *addr, uint bit_num )
	{ return ( ((u8*)addr)[ bit_num >> 3 ] & 1 << ( bit_num & 7 ) ) != 0; }

// change a single bit and return the previous state
bool TestSetBit	   ( void *addr, uint bit_num );
bool TestClearBit  ( void *addr, uint bit_num );
bool TestInvertBit ( void *addr, uint bit_num );

// change bits inclusive 'beg' to exclusive 'end' if beg_bitnum<end_bitnum
void SetBits    ( void *addr, uint beg_bitnum, uint end_bitnum );
void ClearBits  ( void *addr, uint beg_bitnum, uint end_bitnum );
void InvertBits ( void *addr, uint beg_bitnum, uint end_bitnum );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

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
);

static inline int PrintBitfield
(
    // returns the number of printed characters, or -1 if buffer is to small

    char	*buf,		// buffer
    uint	buf_size,	// size of 'buf'
    cvp		bit_field,	// address of bit field
    uint	bit_field_size,	// size of 'bit_field' in bytes
    bool	use_ranges	// true: create ranges 'a:b'
)
{
    return PrintBitfieldEx ( buf, buf_size, bit_field,
				0, 8*bit_field_size-1, use_ranges, 1, 0 );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		low level endian conversions		///////////////
///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------
// swapping bytes = inverse byte order
//	void dswap16 ( void * data_ptr );
//	void dswap24 ( void * data_ptr );
//	void dswap32 ( void * data_ptr );
//	void dswap48 ( void * data_ptr );
//	void dswap64 ( void * data_ptr );
//----------------------------------------------------

static inline void dswap16 ( void * data_ptr )
{
    u8 *d = data_ptr;
    u8 temp = d[0];
    d[0] = d[1];
    d[1] = temp;
}

static inline void dswap24 ( void * data_ptr )
{
    u8 *d = data_ptr;
    const u8 temp = d[0];
    d[0] = d[2];
    d[2] = temp;
}

static inline void dswap32 ( void * data_ptr )
{
    u8 temp, *d = data_ptr;
    temp = d[0]; d[0] = d[3]; d[3] = temp;
    temp = d[1]; d[1] = d[2]; d[2] = temp;
}

static inline void dswap48 ( void * data_ptr )
{
    u8 temp, *d = data_ptr;
    temp = d[0]; d[0] = d[5]; d[5] = temp;
    temp = d[1]; d[1] = d[4]; d[4] = temp;
    temp = d[2]; d[2] = d[3]; d[3] = temp;
}

static inline void dswap64 ( void * data_ptr )
{
    u8 temp, *d = data_ptr;
    temp = d[0]; d[0] = d[7]; d[7] = temp;
    temp = d[1]; d[1] = d[6]; d[6] = temp;
    temp = d[2]; d[2] = d[5]; d[5] = temp;
    temp = d[3]; d[3] = d[4]; d[4] = temp;
}

///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------
// read data and swap bytes (inverse byte order)
//	u16	swap16 ( const void * data_ptr );
//	u32	swap24 ( const void * data_ptr );
//	u32	swap32 ( const void * data_ptr );
//	u64	swap48 ( const void * data_ptr );
//	u64	swap64 ( const void * data_ptr );
//	float	swapf4 ( const void * data_ptr );
//	double	swapf8 ( const void * data_ptr );
//----------------------------------------------------

static inline u16 swap16 ( const void * data_ptr )
{
    u8 r[2];
    const u8 *d = data_ptr;
    r[1] = *d++;
    r[0] = *d;
    return *(u16*)r;
}

static inline u32 swap24 ( const void * data_ptr )
{
    u8 r[4];
    const u8 *d = data_ptr;
    r[3] = 0;
    r[2] = *d++;
    r[1] = *d++;
    r[0] = *d;
    return *(u32*)r;
}

static inline u32 swap32 ( const void * data_ptr )
{
    u8 r[4];
    const u8 *d = data_ptr;
    r[3] = *d++;
    r[2] = *d++;
    r[1] = *d++;
    r[0] = *d;
    return *(u32*)r;
}

static inline u64 swap48 ( const void * data_ptr )
{
    u8 r[8];
    const u8 *d = data_ptr;
    r[7] = 0;
    r[6] = 0;
    r[5] = *d++;
    r[4] = *d++;
    r[3] = *d++;
    r[2] = *d++;
    r[1] = *d++;
    r[0] = *d;
    return *(u64*)r;
}

static inline u64 swap64 ( const void * data_ptr )
{
    u8 r[8];
    const u8 *d = data_ptr;
    r[7] = *d++;
    r[6] = *d++;
    r[5] = *d++;
    r[4] = *d++;
    r[3] = *d++;
    r[2] = *d++;
    r[1] = *d++;
    r[0] = *d;
    return *(u64*)r;
}

static inline float swapf4 ( const void * data_ptr )
{
    // assume: local system supports IEEE 754
    union { float f; u32 i; } u;
    u.i = swap32(data_ptr);
    return u.f;
}

static inline double swapf8 ( const void * data_ptr )
{
    // assume: local system supports IEEE 754
    union { double f; u64 i; } u;
    u.i = swap64(data_ptr);
    return u.f;
}

///////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------
// convert big endian data to a number in host format
//	u16	be16 ( const void * be_data_ptr );
//	u32	be24 ( const void * be_data_ptr );
//	u32	be32 ( const void * be_data_ptr );
//	u64	be40 ( const void * be_data_ptr );
//	u64	be48 ( const void * be_data_ptr );
//	u64	be56 ( const void * be_data_ptr );
//	u64	be64 ( const void * be_data_ptr );
//	float	bef4 ( const void * be_data_ptr );
//	double	bef8 ( const void * be_data_ptr );
//----------------------------------------------------

static inline u16 be16 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return d[0] << 8 | d[1];
}

static inline u32 be24 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return ( d[0] << 8 | d[1] ) << 8 | d[2];
}

static inline u32 be32 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (( d[0] << 8 | d[1] ) << 8 | d[2] ) << 8 | d[3];
}

static inline u64 be40 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (u64)be32(d) << 8 | ((u8*)d)[4];
}

static inline u64 be48 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (u64)be32(d) << 16 | be16(d+4);
}

static inline u64 be56 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (u64)be32(d) << 24 | be24(d+4);
}

static inline u64 be64 ( const void * be_data_ptr )
{
    const u8 * d = be_data_ptr;
    return (u64)be32(d) << 32 | be32(d+4);
}

static inline float bef4 ( const void * be_data_ptr )
{
    // assume: local system supports IEEE 754
    union { float f; u32 i; } u;
    u.i = be32(be_data_ptr);
    return u.f;
}

static inline double bef8 ( const void * be_data_ptr )
{
    // assume: local system supports IEEE 754
    union { double f; u64 i; } u;
    u.i = be64(be_data_ptr);
    return u.f;
}

///////////////////////////////////////////////////////////////////////////////

//-------------------------------------------------------
// convert little endian data to a number in host format
//	u16	le16 ( const void * le_data_ptr );
//	u32	le24 ( const void * le_data_ptr );
//	u32	le32 ( const void * le_data_ptr );
//	u64	le40 ( const void * le_data_ptr );
//	u64	le48 ( const void * le_data_ptr );
//	u64	le56 ( const void * le_data_ptr );
//	u64	le64 ( const void * le_data_ptr );
//	float	lef4 ( const void * le_data_ptr );
//	double	lef8 ( const void * le_data_ptr );
//-------------------------------------------------------

static inline u16 le16 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return d[1] << 8 | d[0];
}

static inline u32 le24 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return ( d[2] << 8 | d[1] ) << 8 | d[0];
}

static inline u32 le32 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return (( d[3] << 8 | d[2] ) << 8 | d[1] ) << 8 | d[0];
}

static inline u64 le40 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return (u64)le32(d+1) << 8 | ((u8*)d)[0];
}

static inline u64 le48 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return (u64)le16(d+4) << 32 | le32(d);
}

static inline u64 le56 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return (u64)le24(d+4) << 32 | le32(d);
}

static inline u64 le64 ( const void * le_data_ptr )
{
    const u8 * d = le_data_ptr;
    return (u64)le32(d+4) << 32 | le32(d);
}

static inline float lef4 ( const void * le_data_ptr )
{
    // assume: local system supports IEEE 754

    union { float f; u32 i; } u;
    u.i = le32(le_data_ptr);
    return u.f;
}

static inline double lef8 ( const void * le_data_ptr )
{
    // assume: local system supports IEEE 754

    union { double f; u64 i; } u;
    u.i = le64(le_data_ptr);
    return u.f;
}

///////////////////////////////////////////////////////////////////////////////

//-------------------------------------------
// convert u64 from/to network byte order
//	be64_t hton64 ( u64    data );
//	u64    ntoh64 ( be64_t data );
//-------------------------------------------

static inline be64_t hton64 ( u64 data )
{
    be64_t result;
    ((u32*)&result)[0] = htonl( (u32)(data >> 32) );
    ((u32*)&result)[1] = htonl( (u32)data );
    return result;
}

static inline u64 ntoh64 ( be64_t data )
{
    return (u64)ntohl(((u32*)&data)[0]) << 32 | ntohl(((u32*)&data)[1]);
}

///////////////////////////////////////////////////////////////////////////////

// convert a number in host format into big endian data
void write_be16 ( void * be_data_ptr, u16 data );
void write_be24 ( void * be_data_ptr, u32 data );
void write_be32 ( void * be_data_ptr, u32 data );
void write_be40 ( void * be_data_ptr, u64 data );
void write_be48 ( void * be_data_ptr, u64 data );
void write_be56 ( void * be_data_ptr, u64 data );
void write_be64 ( void * be_data_ptr, u64 data );
void write_bef4 ( void * be_data_ptr, float data );
void write_bef8 ( void * be_data_ptr, double data );

// convert a number in host format into little endian data
void write_le16 ( void * le_data_ptr, u16 data );
void write_le24 ( void * le_data_ptr, u32 data );
void write_le32 ( void * le_data_ptr, u32 data );
void write_le40 ( void * le_data_ptr, u64 data );
void write_le48 ( void * le_data_ptr, u64 data );
void write_le56 ( void * le_data_ptr, u64 data );
void write_le64 ( void * le_data_ptr, u64 data );
void write_lef4 ( void * le_data_ptr, float data );
void write_lef8 ( void * le_data_ptr, double data );

// convert lists
void be16n ( u16     * dest, const u16     * src, int n );
void be32n ( u32     * dest, const u32     * src, int n );
void bef4n ( float32 * dest, const float32 * src, int n );
void le16n ( u16     * dest, const u16     * src, int n );
void lef4n ( float32 * dest, const float32 * src, int n );

void write_be16n ( u16     * dest, const u16     * src, int n );
void write_be32n ( u32     * dest, const u32     * src, int n );
void write_bef4n ( float32 * dest, const float32 * src, int n );
void write_le16n ( u16     * dest, const u16     * src, int n );
void write_lef4n ( float32 * dest, const float32 * src, int n );

//-----------------------------------------------------------------------------
// [[endian_func_t]]

typedef struct endian_func_t
{
    //--- info section

    u8   bom[2];	// btye order mark: BE=0xfe,0xff, LE=0xff,0xfe
    bool is_be;		// true: is big endian
    bool is_le;		// true: is little endian
    dcEndian_t endian;	// DC_BIG_ENDIAN or DC_LITTLE_ENDIAN

    //--- read functions

    u16    (*rd16) ( const void * data_ptr );
    u32    (*rd24) ( const void * data_ptr );
    u32    (*rd32) ( const void * data_ptr );
    u64    (*rd40) ( const void * data_ptr );
    u64    (*rd48) ( const void * data_ptr );
    u64    (*rd56) ( const void * data_ptr );
    u64    (*rd64) ( const void * data_ptr );
    float  (*rdf4) ( const void * data_ptr );
    double (*rdf8) ( const void * data_ptr );

    //--- write functions

    void (*wr16) ( void * data_ptr, u16 data );
    void (*wr24) ( void * data_ptr, u32 data );
    void (*wr32) ( void * data_ptr, u32 data );
    void (*wr40) ( void * data_ptr, u64 data );
    void (*wr48) ( void * data_ptr, u64 data );
    void (*wr56) ( void * data_ptr, u64 data );
    void (*wr64) ( void * data_ptr, u64 data );
    void (*wrf4) ( void * data_ptr, float data );
    void (*wrf8) ( void * data_ptr, double data );

} endian_func_t;

extern const endian_func_t be_func;
extern const endian_func_t le_func;

const endian_func_t * GetEndianFunc ( const void * byte_order_mark );
uint GetTextBOMLen ( const void * data, uint data_size );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct mem_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[mem_t]]

typedef struct mem_t
{
    ccp	ptr;	// NULL or a pointer to the data
    int len;	// length in bytes of the data
}
mem_t;

extern mem_t EmptyMem;	    // mem_t(EmptyString,0)
extern mem_t NullMem;	    // mem_t(0,0)

//-----------------------------------------------------------------------------

static inline mem_t MemByChar ( char ch )
{
    mem_t mem = { &ch, 1 };
    return mem;
}

static inline mem_t MemByString ( ccp str )
{
    mem_t mem = { str, strlen(str) };
    return mem;
}

static inline mem_t MemByStringS ( ccp str, int len )
{
    mem_t mem = { str, len < 0 ? strlen(str) : len };
    return mem;
}

static inline mem_t MemByStringE ( ccp str, ccp end )
{
    mem_t mem = { str, end ? end-str : strlen(str) };
    return mem;
}

static inline mem_t MemByS ( ccp str, int len )
{
    mem_t mem = { str, len };
    return mem;
}

static inline mem_t MemByE ( ccp str, ccp end )
{
    mem_t mem = { str, end-str };
    return mem;
}

//-----------------------------------------------------------------------------

// Index helpers
// For the 'End' variants, index 0 is end_of_source

int CheckIndex1	   ( int max, int index );
int CheckIndex1End ( int max, int index );
int CheckIndex2    ( int max, int * p_begin, int * p_end );
int CheckIndex2End ( int max, int * p_begin, int * p_end );

//-----------------------------------------------------------------------------

// All functions are robust.
// A negative index is relative to the end of source.
// For RightMem() and ExtractEndMem(), index 0 means: end of source.
// count<0 means: copy left from 'begin' (only MidMem()) or skip bytes.

mem_t MidMem ( const mem_t src, int begin, int count );

static inline mem_t LeftMem ( const mem_t src, int count )
{
    mem_t res;
    res.ptr = src.ptr;
    res.len = CheckIndex1(src.len,count);
    return res;
}

static inline mem_t RightMem ( const mem_t src, int count )
{
    mem_t res;
    res.len = src.len - CheckIndex1End(src.len,-count);
    res.ptr = src.ptr + src.len - res.len;
    return res;
}

static inline mem_t ExtractMem ( const mem_t src, int begin, int end )
{
    mem_t res;
    res.len = CheckIndex2(src.len,&begin,&end);
    res.ptr = src.ptr + begin;
    return res;
}

static inline mem_t ExtractEndMem ( const mem_t src, int begin, int end )
{
    mem_t res;
    res.len = CheckIndex2End(src.len,&begin,&end);
    res.ptr = src.ptr + begin;
    return res;
}

mem_t BeforeMem ( const mem_t src, ccp ref );
mem_t BehindMem ( const mem_t src, ccp ref );

//-----------------------------------------------------------------------------

int  CmpMem		( const mem_t s1, const mem_t s2 );
int  StrCmpMem		( const mem_t mem, ccp str );
bool LeftStrCmpMemEQ	( const mem_t mem, ccp str );

// free a string if str is not NULL|EmptyString|MinusString
void FreeString ( ccp str );

static inline void FreeMem ( mem_t *mem )
{
    DASSERT(mem);
    FreeString(mem->ptr);
    mem->ptr = 0;
    mem->len = 0;
}

static inline void AssignDupMem ( mem_t *dest, mem_t src )
{
    DASSERT(dest);
    FreeString(dest->ptr);
    dest->ptr = MEMDUP(src.ptr,src.len);
    dest->len = src.len;
}

static inline mem_t DupMem ( const mem_t src )
{
    mem_t res = { MEMDUP(src.ptr,src.len), src.len };
    return res;
}

static inline char * DupStrMem ( const mem_t src )
{
    return MEMDUP(src.ptr,src.len);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct mem_list_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[mem_list_t]]

typedef struct mem_list_t
{
    mem_t	*list;		// List of mem_t objects.
				// Elements are always terminated with NULL.
				// Content is stored in 'buf' or points to EmptyString.

    uint	used;		// Number of active 'list' elements
    uint	size;		// Number of alloced 'list' elements

    char	*buf;		// Alloced temp buffer
    uint	buf_used;	// Usage of 'buf'
    uint	buf_size;	// Size of 'buf'
}
mem_list_t;

//-----------------------------------------------------------------------------

void InitializeMemList ( mem_list_t *ml );
void ResetMemList ( mem_list_t *ml );
void MoveMemList ( mem_list_t *dest, mem_list_t *src );

void GrowBufMemList ( mem_list_t *ml, uint need_size );
void PrepareMemList ( mem_list_t *ml, uint n_elem, uint buf_size );

//-----------------------------------------------------------------------------

void InsertMemListN
(
    mem_list_t		*ml,		// valid destination mem_list
    int			pos,		// insert position => CheckIndex1()
    const mem_t		*src,		// source list
    uint		src_len,	// number of elements in source list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
);

//-----------------------------------------------------------------------------

static inline void InsertMemList2
(
    mem_list_t		*ml,		// valid destination mem_list
    int			pos,		// insert position => CheckIndex1()
    const mem_list_t	*src,		// source mem_list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(src);
    InsertMemListN(ml,pos,src->list,src->used,src_ignore);
}

//-----------------------------------------------------------------------------

static inline void AppendMemListN
(
    mem_list_t		*ml,		// valid destination mem_list
    const mem_t		*src,		// source list
    uint		src_len,	// number of elements in source list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(ml);
    InsertMemListN(ml,ml->used,src,src_len,src_ignore);
}

//-----------------------------------------------------------------------------

static inline void AppendMemList2
(
    mem_list_t		*ml,		// valid destination mem_list
    const mem_list_t	*src,		// source mem_list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(ml);
    DASSERT(src);
    InsertMemListN(ml,ml->used,src->list,src->used,src_ignore);
}

//-----------------------------------------------------------------------------

static inline void AssignMemListN
(
    mem_list_t		*ml,		// valid destination mem_list
    const mem_t		*src,		// source list
    uint		src_len,	// number of elements in source list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(ml);
    ml->used = 0;
    InsertMemListN(ml,0,src,src_len,src_ignore);
}

//-----------------------------------------------------------------------------

static inline void AssignMemList2
(
    mem_list_t		*ml,		// valid destination mem_list
    const mem_list_t	*src,		// source mem_list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(ml);
    DASSERT(src);
    ml->used = 0;
    InsertMemListN(ml,0,src->list,src->used,src_ignore);
}

///////////////////////////////////////////////////////////////////////////////

void CatMemListN
(
    mem_list_t		*dest,		// valid destination mem_list
    const mem_list_t	**src_list,	// list with mem lists, element may be NULL
    uint		n_src_list,	// number of elements in 'src'
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
);

//-----------------------------------------------------------------------------

static inline void CatMemList2
(
    mem_list_t		*dest,		// valid destination mem_list
    const mem_list_t	*src1,		// source mem_list
    const mem_list_t	*src2,		// source mem_list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    const mem_list_t *ml[2] = { src1, src2 };
    CatMemListN(dest,ml,2,src_ignore);
}

//-----------------------------------------------------------------------------

static inline void CatMemList3
(
    mem_list_t		*dest,		// valid destination mem_list
    const mem_list_t	*src1,		// source mem_list
    const mem_list_t	*src2,		// source mem_list
    const mem_list_t	*src3,		// source mem_list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    const mem_list_t *ml[3] = { src1, src2, src3 };
    CatMemListN(dest,ml,3,src_ignore);
}

//-----------------------------------------------------------------------------

// All functions are robust and modify the current list.
// A negative index is relative to the end of source.
// For RightMemList() and *EndMemList(), index 0 means: end of source.
// count<0 means: copy left from 'begin' (only MidMemList()) or skip bytes.
// All functions return the number of elements aufer operation.

uint LeftMemList	( mem_list_t *ml, int count );
uint RightMemList	( mem_list_t *ml, int count );
uint MidMemList		( mem_list_t *ml, int begin, int count );
uint ExtractMemList	( mem_list_t *ml, int begin, int end );
uint ExtractEndMemList	( mem_list_t *ml, int begin, int end );
uint RemoveMemList	( mem_list_t *ml, int begin, int end );
uint RemoveEndMemList	( mem_list_t *ml, int begin, int end );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    MemPool_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MemPoolChunk_t]]

typedef struct MemPoolChunk_t
{
    struct MemPoolChunk_t *next;
    char data[];
}
MemPoolChunk_t;

//-----------------------------------------------------------------------------
// [[MemPool_t]]

typedef struct MemPool_t
{
    MemPoolChunk_t	*chunk;		// pointer to first (=current) chunk
    uint		space;		// space left in current chunk
    uint		chunk_size;	// wanted chunk size
}
MemPool_t;

//-----------------------------------------------------------------------------

static inline void InitializeMemPool ( MemPool_t *mp, uint chunk_size )
	{ DASSERT(mp); memset(mp,0,sizeof(*mp)); mp->chunk_size = chunk_size; }

void   ResetMemPool   ( MemPool_t *mp );
void * MallocMemPool  ( MemPool_t *mp, uint size );
void * CallocMemPool  ( MemPool_t *mp, uint size );
void * MallocMemPoolA ( MemPool_t *mp, uint size, uint align );
void * CallocMemPoolA ( MemPool_t *mp, uint size, uint align );
void * MemDupMemPool  ( MemPool_t *mp, cvp source, uint size );

static inline void * StrDupMemPool  ( MemPool_t *mp, ccp source )
  { DASSERT(mp); return source ? MemDupMemPool(mp,source,strlen(source)) : 0; }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    CircBuf_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[CircBuf_t]]

typedef struct CircBuf_t
{
    char *buf;		// NULL or alloced buffer
    uint pos;		// position of first valid byte
    uint used;		// used bytes in buffer
    uint size;		// size of 'buf'
}
CircBuf_t;

///////////////////////////////////////////////////////////////////////////////

void InitializeCircBuf ( CircBuf_t *cb, uint size );
void ResetCircBuf  ( CircBuf_t *cb );
uint WriteCircBuf  ( CircBuf_t *cb, cvp data, uint size );
uint PeakCircBuf   ( CircBuf_t *cb, char *buf, uint buf_size );
uint ReadCircBuf   ( CircBuf_t *cb, char *buf, uint buf_size );
uint DropCircBuf   ( CircBuf_t *cb, uint size );
void ClearCircBuf  ( CircBuf_t *cb );
uint PurgeCircBuf  ( CircBuf_t *cb );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    CopyMode_t			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum CopyMode_t
{
    CPM_COPY,	// make a copy of the data, alloced
    CPM_MOVE,	// move the alloced data
    CPM_LINK,	// link the data

} CopyMode_t;

///////////////////////////////////////////////////////////////////////////////

void * CopyData
(
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode,		// copy mode
    bool		*res_alloced	// not NULL:
					//   store true, if data must be freed
					//   otherwise store false
);

///////////////////////////////////////////////////////////////////////////////

void FreeData
(
    const void		*data,		// data to free
    CopyMode_t		mode		// copy mode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			split strings			///////////////
///////////////////////////////////////////////////////////////////////////////

uint SplitByCharMem
(
    // 'mem_list' will be padded with {EmptyString,0}

    mem_t		*mem_list,	// array of mem_t
    uint		n_mem,		// number of elements of 'mem_list'
    mem_t		source,		// source string
    char		sep		// separator character
);

//-----------------------------------------------------------------------------

uint SplitByTextMem
(
    // 'mem_list' will be padded with {EmptyString,0}

    mem_t		*mem_list,	// array of mem_t
    uint		n_mem,		// number of elements of 'mem_list'
    mem_t		source,		// source string
    mem_t		sep		// separator text
);

//-----------------------------------------------------------------------------

uint SplitByCharMemList
(
    mem_list_t		*ml,		// valid destination mem_list
    bool		init_ml,	// true: initialize 'ml' first
    mem_t		source,		// source string
    char		sep,		// separator character
    int			max_fields	// >0: max number of result fields
);

//-----------------------------------------------------------------------------

uint SplitByTextMemList
(
    mem_list_t		*ml,		// valid destination mem_list
    bool		init_ml,	// true: initialize 'ml' first
    mem_t		source,		// source string
    mem_t		sep,		// separator text
    int			max_fields	// >0: max number of result fields
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    alloc info			///////////////
///////////////////////////////////////////////////////////////////////////////

extern bool alloc_info_calced;
extern uint alloc_info_overhead;
extern uint alloc_info_block_size;
extern uint alloc_info_add1;
extern uint alloc_info_add2;
extern uint alloc_info_mask;

///////////////////////////////////////////////////////////////////////////////

uint SetupAllocInfo();

static inline uint GetGoodAllocSize ( uint need )
{
    return ( need + alloc_info_add1 & alloc_info_mask )
		+ alloc_info_add2;
}

static inline uint GetGoodAllocSize2 ( uint n_elem, uint itemsize )
{
    return (( n_elem*itemsize + alloc_info_add1 & alloc_info_mask )
		+ alloc_info_add2 ) / itemsize;
}

static inline uint GetGoodAllocSizeA ( uint need, uint align )
{
    need = ( need + align - 1) / align * align;
    return (( need + alloc_info_add1 & alloc_info_mask )
		+ alloc_info_add2 ) / align * align;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		some basic string functions		///////////////
///////////////////////////////////////////////////////////////////////////////

extern const char EmptyString[4];	// ""  + 4*NULL, Ignored by FreeString()
extern const char MinusString[4];	// "-" + 3*NULL, Ignored by FreeString()

extern const char LoDigits[0x41];	// 0-9 a-z A-Z .+ NULL
extern const char HiDigits[0x41];	// 0-9 A-Z a-z .+ NULL

extern const char Tabs20[21];		//  20 * TAB + NULL
extern const char LF20[21];		//  20 * LF  + NULL
extern const char Space200[201];	// 200 * ' ' + NULL
extern const char Minus300[301];	// 300 * '-' + NULL
extern const char Underline200[201];	// 200 * '_' + NULL
extern const char Hash200[201];		// 200 * '#' + NULL
extern const char Slash200[201];	// 200 * '/' + NULL

// UTF-8 => 3 bytes per char
extern const char ThinLine300_3[901];	// 300 * '─' (U+2500) + NULL

//-----------------------------------------------------------------------------

void SetupMultiProcessing();
void PrintSettingsDCLIB ( FILE *f, int indent );
void SetupProgname ( int argc, char ** argv,
			ccp tool_name, ccp tool_vers, ccp tool_title );

// return NULL or 'progpath', calc GetProgramPath() once if needed
ccp ProgramPath();

// return NULL or 'progdir', calc ProgramPath() once if needed
ccp ProgramDirectory();

#ifdef __CYGWIN__
    ccp ProgramPathNoExt();
#else
    static inline ccp ProgramPathNoExt() { return ProgramPath(); }
#endif

int GetProgramPath
(
    // returns strlen(buf) on success, -1 on failure

    char  *buf,		// buffer for the result; empty string on failure.
    uint  buf_size,	// size of buffer
    bool  check_proc,	// true: read files of /proc/... to find path
    ccp	  argv0		// not NULL: analyse this argument (argv[0])
);

//-----

// StringCopy(), StringCopyE(), StringCat*()
//	RESULT: end of copied string pointing to a NULL character.
//	'src*' may be a NULL pointer.

char * StringCopyS  ( char * buf, size_t bufsize, ccp src );
char * StringCopySM ( char * buf, size_t bufsize, ccp src, size_t max_copy );
char * StringCat2S  ( char * buf, size_t bufsize, ccp src1, ccp src2 );
char * StringCat3S  ( char * buf, size_t bufsize, ccp src1, ccp src2, ccp src3 );

char * StringCopyE  ( char * buf, ccp buf_end, ccp src );
char * StringCopyEM ( char * buf, ccp buf_end, ccp src, size_t max_copy );
char * StringCat2E  ( char * buf, ccp buf_end, ccp src1, ccp src2 );
char * StringCat3E  ( char * buf, ccp buf_end, ccp src1, ccp src2, ccp src3 );

static inline char * StringCopySMem ( char * buf, size_t bufsize, mem_t mem )
	{ return StringCopySM(buf,bufsize,mem.ptr,mem.len); }
static inline char * StringCopyEMem ( char * buf, ccp buf_end, mem_t mem )
	{ return StringCopyEM(buf,buf_end,mem.ptr,mem.len); }

char * StringLowerS  ( char * buf, size_t bufsize, ccp src );
char * StringLowerE  ( char * buf, ccp buf_end,    ccp src );
char * StringUpperS  ( char * buf, size_t bufsize, ccp src );
char * StringUpperE  ( char * buf, ccp buf_end,    ccp src );

// special handling for unsigned decimals
int StrNumCmp ( ccp a, ccp b );

// count the number of equal bytes and return a value of 0 to SIZE
uint CountEqual ( cvp m1, cvp m2, uint size );

// Concatenate path + path, return pointer to buf
// Return pointer to   buf  or  path1|path2|buf  or  alloced string
char *PathCatBufPP  ( char *buf, size_t bufsize, ccp path1, ccp path2 );
char *PathCatBufPPE ( char *buf, size_t bufsize, ccp path1, ccp path2, ccp ext );
ccp PathCatPP	    ( char *buf, size_t bufsize, ccp path1, ccp path2 );
//ccp PathCatPPE    ( char *buf, size_t bufsize, ccp path1, ccp path2, ccp ext );
ccp PathAllocPP	    ( ccp path1, ccp path2 );
ccp PathAllocPPE    ( ccp path1, ccp path2, ccp ext );

// inline wrapper
static inline ccp PathCatPPE ( char *buf, size_t bufsize, ccp path1, ccp path2, ccp ext )
	{ return PathCatBufPPE(buf,bufsize,path1,path2,ext); }

// Same as PathCatPP*(), but use 'base' as prefix for relative pathes.
// If 'base' is NULL (but not empty), use getcwd() instead.
// PathCatBufBP() and PathCatBP() are special: path can be part of buf
char *PathCatBufBP   ( char *buf, size_t bufsize, ccp base, ccp path );
char *PathCatBufBPP  ( char *buf, size_t bufsize, ccp base, ccp path1, ccp path2 );
char *PathCatBufBPPE ( char *buf, size_t bufsize, ccp base, ccp path1, ccp path2, ccp ext );
ccp PathCatBP        ( char *buf, size_t bufsize, ccp base, ccp path );
ccp PathCatBPP       ( char *buf, size_t bufsize, ccp base, ccp path1, ccp path2 );
ccp PathCatBPPE      ( char *buf, size_t bufsize, ccp base, ccp path1, ccp path2, ccp ext );
ccp PathAllocBP      ( ccp base, ccp path );
ccp PathAllocBPP     ( ccp base, ccp path1, ccp path2 );
ccp PathAllocBPPE    ( ccp base, ccp path1, ccp path2, ccp ext );

char * PathCombine     ( char *temp_buf, uint buf_size, ccp path, ccp base_path );
char * PathCombineFast ( char *dest_buf, uint buf_size, ccp path, ccp base_path );

// Remove last extension and add a new one.
//	If ext==NULL or emtpy: remove only
//	'ext' may start with '.'. If not, a additional '.' is included
//	If 'path' is NULL or path==buf: Inplace job using 'buf' as source.
char * NewFileExtE ( char * buf, ccp buf_end, ccp path, ccp ext );
char * NewFileExtS ( char * buf, size_t bufsize, ccp path, ccp ext );

// eliminate leading './' before comparing
int StrPathCmp ( ccp path1, ccp path2 );

int PathCmp
(
    ccp		path1,		// NULL or first path
    ccp		path2,		// NULL or second path
    uint	mode		// bit field:
				//  1: skip (multiple) leading './'
				//  2: compare case insensitive
				//  4: sort with respect for unsigned decimal numbers
);

int ReplaceToBuf
(
    // returns -1 on error (buf to small) or the length of the resulting string.
    // In the later case, a 0 byte is appended, but not counting for the length.

    char	*buf,		// pointer to buf
    uint	buf_size,	// size of 'buf'

    ccp		src1,		// NULL or source, maybe part of 'buf'
    int		src1_len,	// length of 'src1', if -1: determine by strlen()
    int		pos,		// robust replace position, <0: relative to end
    int		rm_len,		// robust num of chars to remove, <0: rm before 'pos'

    ccp		src2,		// NULL or source to insert, must not be part of 'buf'
    int		src2_len	// lengh of 'src2', if -1: determine by strlen()
);

int NormalizeIndent ( int indent );

//-----------------------------------------------------

// skips all character 1..SPACE; 'src' can be NULL
char * SkipControls ( ccp src );

// skips all character 0..SPACE; if end==NULL: Use SkipControls(); 'src' can be NULL
char * SkipControlsE ( ccp src, ccp end );

// same as above, but skip also 'ch1'; 'src' can be NULL
char * SkipControls1 ( ccp src, char ch1 );
char * SkipControlsE1 ( ccp src, ccp end, char ch1 );

// same as above, but skip also 'ch1' and 'ch2'; 'src' can be NULL
char * SkipControls2 ( ccp src, char ch1, char ch2 );
char * SkipControlsE2 ( ccp src, ccp end, char ch1, char ch2 );

//-----------------------------------------------------

// return the number of chars until first EOL ( \0 | \r | \n );
// pointer are NULL save
uint LineLen  ( ccp ptr );
uint LineLenE ( ccp ptr, ccp end );

//-----------------------------------------------------

uint TrimBlanks
(
    char	*str,		// pointer to string, modified
    ccp		end		// NULL or end of string
);

char * ScanName
(
    // return a pointer to the first unused character
    char	*buf,		// destination buffer, terminated with NULL
    uint	buf_size,	// size of 'buf', must be >0
    ccp		str,		// source string
    ccp		end,		// NULL or end of 'str'
    bool	allow_signs	// true: allow '-' and '+'
);

//-----------------------------------------------------

#define CIRC_BUF_MAX_ALLOC 1024  // request limit
#define CIRC_BUF_SIZE      4096	 // internal buffer size

char * GetCircBuf
(
    // never returns NULL, but always ALIGN(4)

    u32		buf_size	// wanted buffer size, add 1 for NULL-term
				// if buf_size > CIRC_BUF_MAX_ALLOC:
				//  ==> ERROR0(ERR_OUT_OF_MEMORY)
);

void ReleaseCircBuf
(
    ccp	    end_buf,		// pointer to end of previous alloced buffer
    uint    release_size	// number of bytes to give back from end
);

//-----------------------------------------------------
// Format of version number: AABBCCDD = A.BB | A.BB.CC
// If D != 0x00 && D != 0xff => append: 'beta' D
//-----------------------------------------------------

char * PrintVersion
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			version		// version number to print
);

char * PrintID
(
    const void		* id,		// ID to convert in printable format
    size_t		id_len,		// len of 'id'
    void		* buf		// Pointer to buffer, size >= id_len + 1
					// If NULL, a local circulary static buffer is used
);

//-----

#define PATTERN_WILDCARDS "*# ?[{"

bool MatchPatternFull
(
    ccp		pattern,	// pattern text
    ccp		text		// raw text
);

bool MatchPattern
(
    ccp		pattern,	// pattern text
    ccp		text,		// raw text
    char	path_sep	// path separator character, standard is '/'
);

char * MatchRuleLine
(
    // returns a pointer to the first non scanned char

    int		*status,	// not NULL: return match status here
				//   -2: no prefix found  (no ruile found)
				//   -1: empty line (no ruile found)
				//    0: rule found, but don't match
				//    1: rule found and match
    ccp		src,		// source line, scanned untilCONTROL
    char	rule_prefix,	// NULL or a rule-beginning-char'
    ccp		path,		// path to verify
    char	path_sep	// path separator character, standard is '/'
);

///////////////////////////////////////////////////////////////////////////////

static inline int GetHexDigit ( char ch )
{
    return ch >= '0' && ch <= '9' ? ch-'0'
	 : ch >= 'a' && ch <= 'f' ? ch-'a'+10
	 : ch >= 'A' && ch <= 'F' ? ch-'A'+10
	 : -1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    SplitArg_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[SplitArg_t]]

typedef struct SplitArg_t
{
    uint	argc;		// number of arguments in 'argv'
    char	**argv;		// array with 'argc' arguments + a NULL term, alloced
    uint	argv_size;	// number of alloced pointers for 'argv' without NULL term

    char	*temp;		// NULL or temporary buffer
    uint	temp_size;	// size of 'temp'
}
SplitArg_t;

static inline void InitializeSplitArg ( SplitArg_t *sa )
	{ DASSERT(sa); memset(sa,0,sizeof(*sa)); }

void ResetSplitArg ( SplitArg_t *sa );

int ScanSplitArg
(
    // returns arg->argc;

    SplitArg_t	    *arg,	// pointer to object
    bool	    init_arg,	// true: call InitializeSplitArg()
    ccp		    src,	// NULL or source string
    ccp		    src_end,	// end of 'src'; if NULL: calculated by strlen(src)
    char	    *work_buf	// pointer to a buffer for strings, may be 'src'
				// size must be: length of 'src' + 1
				// if NULL, a temporary buffer is alloced.
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		ContainerData_t, Container_t		///////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//				OVERVIEW
//-----------------------------------------------------------------------------
// Container_t * CreateContainer ( Container_t, protect, data,size, CopyMode_t )
// bool AssignContainer ( Container_t, protect, data,size, CopyMode_t )
// void ResetContainer ( Container_t )
// void DeleteContainer ( Container_t )
// void UnlinkContainerData ( Container_t )
// ContainerData_t * LinkContainerData ( Container_t )
// ContainerData_t * MoveContainerData ( Container_t )
// Container_t * CatchContainerData ( Container_t, int protect, ContainerData_t )
// Container_t * UseContainerData ( Container_t, int protect, Container_t )
// void FreeContainerData ( ContainerData_t )
// static inline bool ModificationAllowed ( const Container_t )
// void ModifyAllContainer ( Container_t )
// void ModifyContainer ( Container_t, data, size, CopyMode_t )
// int SetProtectContainer ( Container_t, int new_protect )
// static inline int AddProtectContainer ( Container_t, int add_protect )
// static inline bool IsValidContainer ( const Container_t * c )
// static inline bool InContainerP ( const Container_t, cvp ptr )
// static inline bool InContainerS ( const Container_t, cvp ptr, uint size )
// static inline bool InContainerE ( const Container_t, cvp ptr, cvp end )
// uint DumpInfoContainer ( f,colset,indent,prefix, Container_t, hexdump_len )
// uint DumpInfoContainerData ( f,col,indent,prefix, CData_t, hex_len, hex_indent )
//-----------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// [[ContainerData_t]]

typedef struct ContainerData_t
{
    u8			*data;		// data
    uint		size;		// size of 'data'
    int			ref_count;	// reference counter
    int			protect_count;	// >0: don't modify data
    bool		data_alloced;	// true: free data

} ContainerData_t;

//-----------------------------------------------------------------------------
// [[Container_t]]

typedef struct Container_t
{
    ContainerData_t	*cdata;		// NULL or pointer to ContainerData
    int			protect_level;	// >0: don't modify data

} Container_t;

///////////////////////////////////////////////////////////////////////////////

Container_t * CreateContainer
(
    // returns 'c' or the alloced container
    // 'c' is always initialized

    Container_t		*c,		// if NULL: alloc a container
    int			protect,	// initial value for protection
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
);

//-----------------------------------------------------------------------------

bool AssignContainer
(
    // return TRUE on new ContainerData_t

    Container_t		*c,		// valid container; if NULL: only FREE(data)
    int			protect,	// new protection level
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
);

//-----------------------------------------------------------------------------

void ResetContainer
(
    Container_t		*c		// container to reset => no data
);

//-----------------------------------------------------------------------------

void DeleteContainer
(
    Container_t		*c		// container to reset and to free => no data
);

//-----------------------------------------------------------------------------

void UnlinkContainerData
(
    Container_t		*c		// container to reset => no data
);

///////////////////////////////////////////////////////////////////////////////

ContainerData_t * LinkContainerData
(
    // increment 'ref_count' and return NULL or current ContainerData
    // => use CatchContainerData() to complete operation
    Container_t		*c		// NULL or valid container
);

//-----------------------------------------------------------------------------

ContainerData_t * MoveContainerData
(
    // return NULL or unlinked current ContainerData
    // => use CatchContainerData() to complete operation
    Container_t		*c		// NULL or valid container
);

//-----------------------------------------------------------------------------

Container_t * CatchContainerData
(
    // returns 'c' or the alloced container
    // 'c' is always initialized

    Container_t		*c,		// if NULL: alloc a container
    int			protect,	// initial value for protection
    ContainerData_t	*cdata		// if not NULL: catch this
);

//-----------------------------------------------------------------------------

static inline Container_t * UseContainerData
(
    // returns 'c' or the alloced container
    Container_t		*c,		// if NULL: alloc a container
    int			protect,	// initial value for protection
    Container_t		*src		// if not NULL: catch this
)
{
    return CatchContainerData(c,protect,LinkContainerData(src));
}

//-----------------------------------------------------------------------------

void FreeContainerData
(
    ContainerData_t	*cdata		// NULL or container-data to free
);

///////////////////////////////////////////////////////////////////////////////

static inline bool ModificationAllowed ( const Container_t *c )
	{ return c && c->cdata
		&& ( c->cdata->ref_count <= 1
			|| c->cdata->protect_count <= ( c->protect_level > 0 ) );
	}

//-----------------------------------------------------------------------------

bool ModifyAllContainer
(
    // prepare modification of container-data, create a copy if necessary
    // return true, if a new container-data is used

    Container_t		*c		// NULL or valid container
);

//-----------------------------------------------------------------------------

bool ModifyContainer
(
    // prepare modification of container-data, create an extract if necessary
    // return true, if a new container-data is used

    Container_t		*c,		// NULL or valid container
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
);

//-----------------------------------------------------------------------------

int SetProtectContainer
(
    // returns 'c' new protection level
    Container_t		*c,		// if NULL: alloc a container
    int			new_protect	// new protection value
);

//-----------------------------------------------------------------------------

static inline int AddProtectContainer
(
    // returns 'c' new protection level
    Container_t		*c,		// if NULL: alloc a container
    int			add_protect	// new protection value
)
{
    DASSERT(c);
    return SetProtectContainer(c,c->protect_level+add_protect);
}

///////////////////////////////////////////////////////////////////////////////

static inline bool IsValidContainer ( const Container_t * c )
	{ return c && c->cdata && c->cdata->data; }

//-----------------------------------------------------------------------------

static inline bool InContainerP ( const Container_t * c, cvp ptr )
	{ return c && c->cdata && ptr
		&& (u8*)ptr >= c->cdata->data
		&& (u8*)ptr <= c->cdata->data + c->cdata->size; }

//-----------------------------------------------------------------------------

static inline bool InContainerS ( const Container_t * c, cvp ptr, uint size )
	{ return c && c->cdata && ptr
		&& (u8*)ptr >= c->cdata->data
		&& (u8*)ptr + size <= c->cdata->data + c->cdata->size; }

//-----------------------------------------------------------------------------

static inline bool InContainerE ( const Container_t * c, cvp ptr, cvp end )
	{ return c && c->cdata
		&& ptr && (u8*)ptr >= c->cdata->data
		&& end && (u8*)end <= c->cdata->data + c->cdata->size; }

///////////////////////////////////////////////////////////////////////////////

struct ColorSet_t;
uint DumpInfoContainer
(
    // return the number of printed lines

    FILE		*f,		// valid output file
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent the output
    ccp			prefix,		// not NULL: use it as prefix behind indention
    const Container_t	*c,		// dump infos for this container, NULL allowed
    uint		hexdump_len	// max number of bytes used for a hexdump
);

//-----------------------------------------------------------------------------

uint DumpInfoContainerData
(
    // return the number of printed lines

    FILE		*f,		// valid output file
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent of output
    ccp			prefix,		// not NULL: use it as prefix behind indention
    const
      ContainerData_t	*cdata,		// dump infos for this container-data, NULL allowed
    uint		hexdump_len,	// max number of bytes used for a hexdump
    int			hexdump_indent	// indent of hexdump
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		print lines with autobreak		///////////////
///////////////////////////////////////////////////////////////////////////////

void PutLines
(
    FILE	* f,		// valid output stream
    int		indent,		// indent of output
    int		fw,		// field width of output
    int		first_line,	// length without prefix of already printed first line
    ccp		prefix,		// NULL or prefix for each line
    ccp		text,		// text to print
    ccp		eol		// End Of Line test. If NULL -> LF
);

void PrintArgLines
(
    FILE	* f,		// valid output stream
    int		indent,		// indent of output
    int		fw,		// field width of output
    int		first_line,	// length without prefix of already printed first line
    ccp		prefix,		// NULL or prefix for each line
    ccp		format,		// format string for vsnprintf()
    va_list	arg		// parameters for 'format'
);

void PrintLines
(
    FILE	* f,		// valid output stream
    int		indent,		// indent of output
    int		fw,		// field width of output
    int		first_line,	// length without prefix of already printed first line
    ccp		prefix,		// NULL or prefix for each line
    ccp		format,		// format string for vsnprintf()
    ...				// arguments for 'vsnprintf(format,...)'
)
__attribute__ ((__format__(__printf__,6,7)));

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// Rules for P*ColoredLines():
//
//  |+		: Define a TAB pos (not is defined at start)
//  |-		: Remove a TAB pos
//  |[pos,...]	: Comma separated list: Clear all tabs and define new TABs
//			POS       : Define TAB add POS
//			POS*DELTA : Define TAB add POS and then at every DELTA
//  \t		: Jump to TAB stop
//
//  |  or  |>	: Define an indention for continuation lines.
//		  Use the single char only, if a letter, digit, underline, SPACE,
//		  CTRL or '{' follows. All ather chars are reserved for extensions.
//  \r		: Force a new continuation line.
//  \n		: Terminate current line.
//  \r\n	: Same as '\n'.
//
//  {name}	: Change active color to NAME
//  {name|text}	: Use color NAME only for TEXT, then restore active color.
//  {}		: Reset and deactivate all colors.
//
//  ||		: Print Single '|'
//  {{		: Print Single '{'
//
///////////////////////////////////////////////////////////////////////////////

uint PutColoredLines
(
    // returns the number of written lines

    FILE		* f,		// valid output stream
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent of output
    int			fw,		// field width; indent+prefix+eol don't count
    ccp			prefix,		// NULL or prefix for each line
    ccp			eol,		// End Of Line test. If NULL -> LF
    ccp			text		// text to print
);

uint PrintArgColoredLines
(
    // returns the number of written lines

    FILE		* f,		// valid output stream
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent of output
    int			fw,		// field width; indent+prefix+eol don't count
    ccp			prefix,		// NULL or prefix for each line
    ccp			eol,		// End Of Line test. If NULL -> LF
    ccp			format,		// format string for vsnprintf()
    va_list		arg		// parameters for 'format'
);

uint PrintColoredLines
(
    // returns the number of written lines

    FILE		* f,		// valid output stream
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent of output
    int			fw,		// field width; indent+prefix+eol don't count
    ccp			prefix,		// NULL or prefix for each line
    ccp			eol,		// End Of Line test. If NULL -> LF
    ccp			format,		// format string for vsnprintf()
    ...					// arguments for 'vsnprintf(format,...)'
)
__attribute__ ((__format__(__printf__,7,8)));

//
///////////////////////////////////////////////////////////////////////////////
///////////////			encoding/decoding		///////////////
///////////////////////////////////////////////////////////////////////////////

extern const u32 TableCRC32[0x100];
u32 CalcCRC32 ( u32 crc, cvp buf, uint size );

///////////////////////////////////////////////////////////////////////////////
// [[CharMode_t]]

typedef enum CharMode_t // select encodig/decoding method
{
    CHMD_UTF8		= 0x01,  // UTF8 support enabled (compatible with legacy bool)
    CHMD_ESC		= 0x02,  // escape ESC by \e

    CHMD__ALL		= CHMD_UTF8 | CHMD_ESC,
}
__attribute__ ((packed)) CharMode_t;

///////////////////////////////////////////////////////////////////////////////
// [[EncodeMode_t]]

typedef enum EncodeMode_t // select encodig/decoding method
{
    ENCODE_OFF,		// no encoding (always 0)
    ENCODE_STRING,	// ScanEscapedString(ANSI), byte mode
    ENCODE_UTF8,	// ScanEscapedString(UTF8), force UTF8 on decoding
    ENCODE_BASE64,	// Base64
    ENCODE_BASE64URL,	// Base64.URL (=) / decoder detects Standard + URL + STAR
    ENCODE_BASE64STAR,	// Base64.URL (*) / decoder detects Standard + URL + STAR
    ENCODE_BASE64XML,	// Base64 with XML name tokens / decoder detects Standard + XML (name+id)
    ENCODE_JSON,	// JSON string encoding

    ENCODE__N		// number of encoding modes
}
EncodeMode_t;

///////////////////////////////////////////////////////////////////////////////
// [[DecodeType_t]]

enum DecodeType_t // for decoding tables
{
    DECODE_NULL		= -1,
    DECODE_CONTROL	= -2,
    DECODE_LINE		= -3,
    DECODE_SPACE	= -4,
    DECODE_SEPARATE	= -5,
    DECODE_FILLER	= -6,
    DECODE_OTHER	= -7
};

extern const char TableNumbers[256];

extern       char TableDecode64[256];		// Standard coding
extern       char TableDecode64url[256];	// Standard + URL + STAR coding
extern       char TableDecode64xml[256];	// Standard + XML (name tokens + identifiers)

extern const char TableEncode64[64+1];		// last char is filler
extern const char TableEncode64url[64+1];	// URL coding, used by Nintendo
extern const char TableEncode64star[64+1];	// Like URL, with '*' instead of '=' as filler
extern const char TableEncode64xml[64+1];	// XML name tokens

// for tests: encode the 48 bytes to get the full BASE64 alphabet
extern const u8 TableAlphabet64[48];

// Default tables for DecodeBase64() and EncodeBase64(), if no table is defined.
// They are initialized with TableDecode64[] and TableEncode64[].
extern ccp TableDecode64default, TableEncode64default;

///////////////////////////////////////////////////////////////////////////////

char * PrintEscapedString
(
    // returns 'buf'

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >= 10
    ccp		source,			// NULL string to print
    int		len,			// length of string. if -1, str is null terminated
    CharMode_t	char_mode,		// modes, bit field (CHMD_*)
    char	quote,			// NULL or quotation char, that must be quoted
    uint	*scanned_len		// not NULL: Store number of scanned 'str' bytes here
);

///////////////////////////////////////////////////////////////////////////////

uint ScanEscapedString
(
    // returns the number of valid bytes in 'buf' (NULL term not counted)

    char	*buf,			// valid destination buffer, maybe source
    uint	buf_size,		// size of 'buf'
    ccp		source,			// string to scan
    int		len,			// length of string. if -1, str is null terminated
    bool	utf8,			// true: source and output is UTF-8
    uint	*scanned_len		// not NULL: Store number of scanned 'source' bytes here
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u64 DecodeU64
(
    ccp		source,		// string to decode
    int		len,		// length of string. if -1, str is null terminated
    uint	base,		// number of possible digits (2..36):
    uint	*scanned_len	// not NULL: Store number of scanned 'str' bytes here
);

///////////////////////////////////////////////////////////////////////////////

ccp EncodeU64
(
    char	* buf,		// result buffer (size depends on base)
				// If NULL, a local circulary static buffer is used
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u64		num,		// number to encode
    uint	base		// number of possible digits (2..64):
);

///////////////////////////////////////////////////////////////////////////////
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
);

///////////////////////////////////////////////////////////////////////////////

uint EncodeBase64
(
    // returns the number of scanned bytes of 'source'

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >= 4
    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    const char	encode64[64+1],		// encoding table; if NULL: use TableEncode64default
    bool	use_filler,		// use filler for aligned output
    ccp		next_line,		// not NULL: use this string as new line sep
    uint	next_line_trigger	// >0: use 'next_line' every # input bytes
);

///////////////////////////////////////////////////////////////////////////////

static inline uint EncodeBase64Len ( uint src_len )
	{ return 4 * (src_len+2) / 3; }

static inline uint EncodeBase64FillLen ( uint src_len )
	{ return (src_len+2) / 3 * 4; }

static inline uint DecodeBase64Len ( uint src_len )
	{ return 3 * src_len / 4; }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static inline uint DecodeJSON
(
    // returns the number of valid bytes in 'buf'

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >= 3
    ccp		source,			// NULL or string to decode
    int		source_len,		// length of 'source'. If -1, str is NULL terminated
    uint	*scanned_len		// not NULL: Store number of scanned 'str' bytes here
)
{
    return ScanEscapedString(buf,buf_size,source,source_len,true,scanned_len);
}

///////////////////////////////////////////////////////////////////////////////

uint EncodeJSON
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >2 and 2 bytes longer than needed
    const void	*source,		// NULL or data to encode
    int		source_len		// length of 'source'; if <0: use strlen(source)
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint DecodeByMode
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char		*buf,		// valid destination buffer
    uint		buf_size,	// size of 'buf', >= 3
    ccp			source,		// string to decode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
);

///////////////////////////////////////////////////////////////////////////////

uint EncodeByMode
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char		*buf,		// valid destination buffer
    uint		buf_size,	// size of 'buf', >= 4
    ccp			source,		// string to encode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
);

///////////////////////////////////////////////////////////////////////////////

mem_t DecodeByModeMem
(
    // Returns the decoded 'source'. Result is NULL-terminated.
    // It points either to 'buf' or is alloced (on buf==NULL or to less space)
    // If alloced (mem.ptr!=buf) => call FreeMem(&mem)

    char		*buf,		// NULL or destination buffer
    uint		buf_size,	// size of 'buf'
    ccp			source,		// string to decode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
);

///////////////////////////////////////////////////////////////////////////////

mem_t EncodeByModeMem
(
    // Returns the encode 'source'. Result is NULL-terminated.
    // It points either to 'buf' or is alloced (on buf==NULL or to less space)
    // If alloced (mem.ptr!=buf) => call FreeMem(&mem)

    char		*buf,		// NULL or destination buffer
    uint		buf_size,	// size of 'buf'
    ccp			source,		// string to encode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan keywords			///////////////
///////////////////////////////////////////////////////////////////////////////

#define KEYWORD_NAME_MAX 100

typedef s64 (*KeywordCallbackFunc)
(
    void		*param,		// NULL or user defined parameter
    ccp			name,		// normalized name of option
    const KeywordTab_t	*key_tab,	// valid pointer to command table
    const KeywordTab_t	*key,		// valid pointer to found command
    char		prefix,		// 0 | '-' | '+' | '='
    s64			result		// current value of result
);

///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t *GetKewordById
(
    const KeywordTab_t	*key_tab,	// NULL or pointer to command table
    s64			id		// id to search
);

//-----------------------------------------------------------------------------

const KeywordTab_t *GetKewordByIdAndOpt
(
    const KeywordTab_t	*key_tab,	// NULL or pointer to command table
    s64			id,		// id to search
    s64			opt		// opt to search
);

//-----------------------------------------------------------------------------

static inline ccp GetKewordNameById
(
    const KeywordTab_t	*key_tab,	// NULL or pointer to command table
    s64			id,		// id to search
    ccp			res_not_found	// return this if not found
)
{
    const KeywordTab_t *key = GetKewordById(key_tab,id);
    return key ? key->name1 : res_not_found;
}

//-----------------------------------------------------------------------------

static inline ccp GetKewordNameByIdAndOpt
(
    const KeywordTab_t	*key_tab,	// NULL or pointer to command table
    s64			id,		// id to search
    s64			opt,		// opt to search
    ccp			res_not_found	// return this if not found
)
{
    const KeywordTab_t *key = GetKewordByIdAndOpt(key_tab,id,opt);
    return key ? key->name1 : res_not_found;
}

///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t * ScanKeyword
(
    int			*res_abbrev,	// NULL or pointer to result 'abbrev_count'
    ccp			arg,		// argument to scan
    const KeywordTab_t	*key_tab	// valid pointer to command table
);

//-----------------------------------------------------------------------------

s64 ScanKeywordListEx
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab,	// valid pointer to command table
    KeywordCallbackFunc	func,		// NULL or calculation function
    bool		allow_prefix,	// allow '-' | '+' | '=' as prefix
    u32			max_number,	// allow numbers < 'max_number' (0=disabled)
    s64			result,		// start value for result

    uint		err_mode,	// bit field:
					//	1: continue on error
    ccp			err_msg,	// not NULL: print a warning message:
					//   "<ERR_MSG>: Unknown keyword: <KEY>"
    enumError		err_code,	// use 'err_code' for the 'err_msg'
    uint		*err_count	// not NULL: store errors here
);

static inline s64 ScanKeywordList
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab,	// valid pointer to command table
    KeywordCallbackFunc	func,		// NULL or calculation function
    bool		allow_prefix,	// allow '-' | '+' | '=' as prefix
    u32			max_number,	// allow numbers < 'max_number' (0=disabled)
    s64			result,		// start value for result

    ccp			err_msg,	// not NULL: print a warning message:
					//   "<ERR_MSG>: Unknown keyword: <KEY>"
    enumError		err_code	// use 'err_code' for the 'err_msg'
)
{
    return ScanKeywordListEx( arg, key_tab, func, allow_prefix,
				max_number, result, 0, err_msg, err_code, 0 );
}

//-----------------------------------------------------------------------------

enumError ScanKeywordListFunc
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	*key_tab,	// valid pointer to command table
    KeywordCallbackFunc	func,		// calculation function
    void		*param,		// used define parameter for 'func'
    bool		allow_prefix	// allow '-' | '+' | '=' as prefix
);

//-----------------------------------------------------------------------------

s64 ScanKeywordListMask
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab	// valid pointer to command table
);

//-----------------------------------------------------------------------------

enumError PrintKeywordError
(
    const KeywordTab_t	* key_tab,	// NULL or pointer to command table
    ccp			key_arg,	// analyzed command
    int			key_stat,	// status of ScanKeyword()
    ccp			prefix,		// NULL or prefix for messages
    ccp			object		// NULL or object for error messages
					//	default= 'command'
);

uint CollectAmbiguousKeywords
(
    char		*buf,		// destination buffer, 200 bytes are good
    uint		buf_size,	// size of buffer
    const KeywordTab_t	* key_tab,	// NULL or pointer to command table
    ccp			key_arg		// analyzed command
);

//-----------------------------------------------------------------------------

char * PrintKeywordList
(
    // returns a pointer to the result buffer

    char		*buf,		// result buffer
					// If NULL, a local circulary static buffer
					// with max CIRC_BUF_MAX_ALLOC bytes is used
    uint		buf_size,	// size of 'buf', at least 10 bytes if buf is set
    uint		*ret_length,	// not NULL: store result length here

    const KeywordTab_t	*tab,		// related keyword table
    u64			mode,		// mode to print
    u64			default_mode,	// NULL or default mode
    u64			hide_mode	// bit field to hide parameters
);

//-----------------------------------------------------------------------------

int ScanKeywordOffOn
(
    // returns 0 for '0|OFF';  1 for '1|ON;  -1 for empty;  -2 on error

    ccp			arg,		// argument to scan
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
);

//-----------------------------------------------------------------------------

enumError Command_ARGTEST ( int argc, char ** argv );
enumError Command_TESTCOLORS ( int argc, char ** argv );

enumError Command_COLORS
(
    int		level,		// only used, if mode==NULL
				//  <  0: status message (ignore mode)
				//  >= 1: include names
				//  >= 2: include alt names
				//  >= 3: include color names incl bold
				//  >= 4: include background color names
    uint	mode,		// output mode => see PrintColorSetHelper()
    uint	format		// output format => see PrintColorSetEx()
);


///////////////////////////////////////////////////////////////////////////////
///////////////			OFF/AUTO/ON/FORCE		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum OffOn_t
{
    OFFON_ERROR	= -999,
    OFFON_OFF	=   -1,
    OFFON_AUTO	=    0,
    OFFON_ON	=    1,
    OFFON_FORCE	=    2,
}
OffOn_t;

extern const KeywordTab_t KeyTab_OFF_AUTO_ON[];

int ScanKeywordOffAutoOn
(
    // returns one of OFFON_* (OFFON_ON on empty argument)

    ccp			arg,		// argument to scan
    int			on_empty,	// return this value on empty
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
);

////
///////////////////////////////////////////////////////////////////////////////
///////////////			scan command lists		///////////////
///////////////////////////////////////////////////////////////////////////////
//
// Scan a semicolon separated command list with binary support of format:
//	COMMAND	:= NAME [BINLIST] CONTROL* ['='] CONTROL* [PARAM] CONTROL* ';'
//	BLANK	:= TAB(9) or SPACE(32)
//	CONTROL	:= NUL..SPACE (0..32)
//	NAME	:= 1..99 alphanumeric chars incl. minus and underline
//	BINLIST	:= BINARY [BINLIST]
//	BINARY	:= BLANK* '\1' be16:SIZE DATA
//	PARAM	:= all except ';'
//
// CONTROLS are possible before and behind the '=' and at the end of param.
// Only BLANK* are allowed between command NAME and BINARY block.
// BINARY data is stored as '->bin' and '->bin_len' and inludes SIZE and DATA.
// The BINARY.SIZE includes itself and is a 16-bit big-endian number.
// In '->bin[]', 'be16:SIZE' is excluded, but '->bin[].ptr-2' points to be16:SIZE.
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[CommandList_t]] docu -> see above

struct TCPStream_t;
#define COMMAND_LIST_N_BIN 5

typedef struct CommandList_t
{
    //--- input params

    ccp		command;	// command text
    uint	command_len;	// total length of 'command'
    char	command_sep;	// not NULL: alternative command separator
    u8		change_case;	// >0: convert command to lower(1) or upper(2) case
    bool	term_param;	// 1: write a NULL at end-of-param => modify 'command'
				// 2: write a ';' at end-of-param => modify 'command'
				//    in this case, ';' is counted to 'record_len'
    bool	is_terminated;	// true: last command is terminated
				//	 even without ';'

    //--- user parameters, pass-through

    struct TCPStream_t
		*user_ts;	// NULL or a stream for replies
    void	*user_ptr;	// any pointer
    int		user_int;	// any number or id

    //--- analysed data, only valid at callback

    char	cmd[100];	// scanned command name
    uint	cmd_len;	// length of scanned command name

    uint	n_bin;		// number of binary blocks
    mem_t	bin[COMMAND_LIST_N_BIN];
				// binary data detected, if bin[].ptr not null

    ccp		param;		// pointer to text parameter, not NULL terminated
    uint	param_len;	// length of text parameter
    const u8	*record;	// complete record from NAME to END_RECORD_CHAR
    uint	record_len;	// length of trimmed record
    uint	input_len;	// length of complete record at input time

    //--- statistics

    uint	scanned_len;	// number of used bytes of 'commands'
    uint	fail_count;	// number of failed commands
    uint	cmd_count;	// number of commands
}
CommandList_t;

// return -1 to abort scanning
typedef int (*CommandListFunc) ( CommandList_t *cli );

//-----------------------------------------------------------------------------

static inline void InitializeCommandList ( CommandList_t *cli )
	{ DASSERT(cli); memset(cli,0,sizeof(*cli)); }

static inline void ClearCommandList ( CommandList_t *cli )
	{ DASSERT(cli); }

static inline void ResetCommandList ( CommandList_t *cli )
	{ DASSERT(cli); memset(cli,0,sizeof(*cli)); }

// a test function for ScanCommandList()
int ExecCommandOfList ( CommandList_t *cli );

int ScanCommandList
(
    // scan a semicolon separated command list. See above for format
    // returns -1 on 'cli->fail_count'; otherwise 'cli->cmd_count'

    CommandList_t	*cli,	    // valid command list
    CommandListFunc	func	    // NULL or function for each command
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			numeric functions		///////////////
///////////////////////////////////////////////////////////////////////////////

extern int urandom_available; // <0:not, =0:not tested, >0:ok
extern bool use_urandom_for_myrandom;

uint ReadFromUrandom ( void *dest, uint size );

///////////////////////////////////////////////////////////////////////////////

u32 MyRandom ( u32 max );
u64 MySeed ( u64 base );
u64 MySeedByTime();

void MyRandomFill ( void * buf, size_t size );

///////////////////////////////////////////////////////////////////////////////

void CreateUUID ( uuid_buf_t dest );
uint CreateTextUUID ( char *buf, uint bufsize );
uint PrintUUID ( char *buf, uint bufsize, uuid_buf_t uuid );
char * ScanUUID ( uuid_buf_t uuid, ccp source );

///////////////////////////////////////////////////////////////////////////////

uint gcd   ( uint n1, uint n2 ); // greatest common divisor, german: ggt
u32  gcd32 ( u32  n1, u32  n2 );
u64  gcd64 ( u64  n1, u64  n2 );

uint lcm   ( uint n1, uint n2 ); // lowest common multiple, german: kgv
u32  lcm32 ( u32  n1, u32  n2 );
u64  lcm64 ( u64  n1, u64  n2 );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    scan number			///////////////
///////////////////////////////////////////////////////////////////////////////

enum DigitTable_t
{
	NUMBER_NULL	= -1,	// NULL
	NUMBER_CONTROL	= -2,	// < 0x20, but not SPACE or LINE
	NUMBER_LINE	= -3,	// LF, CR
	NUMBER_SPACE	= -4,	// SPACE, TAB, VT
	NUMBER_TIE	= -5,	// ':' '-' '.'
	NUMBER_SEPARATE	= -6,	// ',' ';'
	NUMBER_OTHER	= -7	// all other
};

extern const char DigitTable[256];

//-----------------------------------------------------------------------------

u64 ScanDigits
(
    // same as ScanNumber(), but other interface
    // returns the scanned number

    ccp	  *source,	// pointer to source string, modified
    ccp	  end_source,	// NULL or end of 'source'
    uint  intbase,	// integer base, 2..36
    int	  maxchar,	// max number of digits to read
    uint  *count	// not NULL: store number of scanned digits here
);

//-----------------------------------------------------------------------------

char * ScanNumber
(
    // same as ScanDigits(), but other interface
    // returns a pointer to the first not used character

    uint  *dest_num,	// store result here, never NULL
    ccp	  src,		// pointer to source string, modified
    ccp	  src_end,	// NULL or end of 'src'
    uint  intbase,	// integer base, 2..36
    int	  maxchar	// max number of digits to read
);

//-----------------------------------------------------------------------------

char * ScanEscape
(
    // returns a pointer to the first not used character

    uint  *dest_code,	// store result here, never NULL
    ccp	  src,		// pointer to source string (behind escape char)
    ccp	  src_end	// NULL or end of 'src'
);

//-----------------------------------------------------------------------------

uint ScanHexString
(
    // return the number of written bytes

    void  *buf,		// write scanned data here
    uint  buf_size,	// size of buf
    ccp	  *source,	// pointer to source string, modified
    ccp	  end_source,	// NULL or end of 'source'
    bool  allow_tie	// allow chars ' .:-' and TAB,VT as byte separator
);

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
);

static inline char * ScanU32 ( u32 *res_num, ccp source, uint default_base )
	{ return ScanS32((s32*)res_num,source,default_base); }

//-----------------------------------------------------------------------------

char * ScanS64
(
    // return 'source' on error

    s64		*res_num,		// not NULL: store result (only on success)
    ccp		source,			// NULL or source text
    uint	default_base		// base for numbers without '0x' prefix
					//  0: C like with octal support
					// 10: standard value for decimal numbers
					// 16: standard value for hex numbers
);

static inline char * ScanU64 ( u64 *res_num, ccp source, uint default_base )
	{ return ScanS64((s64*)res_num,source,default_base); }

//-----------------------------------------------------------------------------

#if __WORDSIZE >= 64

  static inline char * ScanINT ( int *res_num, ccp source, uint default_base )
	{ return ScanS32((s32*)res_num,source,default_base); }
  static inline char * ScanUINT ( uint *res_num, ccp source, uint default_base )
	{ return ScanS32((s32*)res_num,source,default_base); }

#else

  static inline char * ScanINT ( int *res_num, ccp source, uint default_base )
	{ return ScanS64((s64*)res_num,source,default_base); }
  static inline char * ScanUINT ( uint *res_num, ccp source, uint default_base )
	{ return ScanS64((s64*)res_num,source,default_base); }

#endif

//-----------------------------------------------------------------------------
// strto*() replacements: Use Scan*() with better base support, NULL allowed as source

long int str2l ( const char *nptr, char **endptr, int base );
long long int str2ll ( const char *nptr, char **endptr, int base );
unsigned long int str2ul ( const char *nptr, char **endptr, int base );
unsigned long long int str2ull ( const char *nptr, char **endptr, int base );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		     print numbers and size		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[sizeform_mode_t]]

typedef enum sizeform_mode_t
{
    DC_SFORM_ALIGN	= 0x01,	// aligned output
    DC_SFORM_NARROW	= 0x02,	// suppress space between number and unit
    DC_SFORM_UNIT1	= 0x04,	// limit unit to 1 character (space for bytes)
    DC_SFORM_INC	= 0x08,	// increment to next factor if no fraction is lost
    DC_SFORM_PLUS	= 0x10,	// on signed output: print PLUS sign for values >0
    DC_SFORM_DASH	= 0x20,	// on NULL: print only a dash (minus sign)

    DC_SFORM_TINY	= DC_SFORM_NARROW | DC_SFORM_UNIT1,
}
sizeform_mode_t;

///////////////////////////////////////////////////////////////////////////////

float  RoundF2bytes ( float f );
float  RoundF3bytes ( float f );
double RoundD6bytes ( double d );
double RoundD7bytes ( double d );

///////////////////////////////////////////////////////////////////////////////

char * PrintNumberU4 ( char *buf, size_t buf_size, u64 num, sizeform_mode_t mode );
char * PrintNumberU5 ( char *buf, size_t buf_size, u64 num, sizeform_mode_t mode );
char * PrintNumberU6 ( char *buf, size_t buf_size, u64 num, sizeform_mode_t mode );
char * PrintNumberU7 ( char *buf, size_t buf_size, u64 num, sizeform_mode_t mode );

char * PrintNumberS5 ( char *buf, size_t buf_size, s64 num, sizeform_mode_t mode );
char * PrintNumberS6 ( char *buf, size_t buf_size, s64 num, sizeform_mode_t mode );
char * PrintNumberS7 ( char *buf, size_t buf_size, s64 num, sizeform_mode_t mode );

char * PrintNumberD5 ( char *buf, size_t buf_size, double num, sizeform_mode_t mode );
char * PrintNumberD6 ( char *buf, size_t buf_size, double num, sizeform_mode_t mode );
char * PrintNumberD7 ( char *buf, size_t buf_size, double num, sizeform_mode_t mode );

// Parameters:
//	char	*buf,		// result buffer
//				// NULL: use a local circulary static buffer
//	size_t	buf_size,	// size of 'buf', ignored if buf==NULL
//	*	num,		// number to print
//	sizeform_mode_t mode	// any of DC_SFORM_ALIGN, DC_SFORM_DASH[not *D*()]

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[size_mode_t]]

typedef enum size_mode_t
{
    //----- modes => used as index for dc_size_tab_1000[] & dc_size_tab_1024[]

    DC_SIZE_DEFAULT	= 0,	// special default value, fall back to AUTO
    DC_SIZE_AUTO,		// select unit automatically

    DC_SIZE_BYTES,		// force output in bytes
    DC_SIZE_K,			// force output in KB or KiB (kilo,kibi)
    DC_SIZE_M,			// force output in MB or MiB (mega,mebi)
    DC_SIZE_G,			// force output in GB or GiB (giga,gibi)
    DC_SIZE_T,			// force output in TB or TiB (tera,tebi)
    DC_SIZE_P,			// force output in PB or PiB (peta,pebi)
    DC_SIZE_E,			// force output in EB or EiB (exa, exbi)
				// zetta/zebi & yotta/yobi not supported because >2^64

    DC_SIZE_N_MODES,		// number of modes

    //----- flags

    DC_SIZE_F_1000	= 0x010,  // force output in SI units (kB=1000, MB=1000000,...)
    DC_SIZE_F_1024	= 0x020,  // force output in IEC units (KiB=1024, MiB=1024*1024,...)
    DC_SIZE_F_AUTO_UNIT	= 0x040,  // suppress output of unit for non AUTO mode
    DC_SIZE_F_NO_UNIT	= 0x080,  // suppress allways output of unit

    //----- masks

    DC_SIZE_M_MODE	= 0x00f,  // mask for modes
    DC_SIZE_M_BASE	= 0x030,  // mask for base
    DC_SIZE_M_ALL	= 0x0ff,  // all relevant bits
}
size_mode_t;

//-----------------------------------------------------------------------------

extern ccp dc_size_tab_1000[DC_SIZE_N_MODES+1];
extern ccp dc_size_tab_1024[DC_SIZE_N_MODES+1];

//-----------------------------------------------------------------------------

ccp GetSizeUnit // get a unit for column headers
(
    size_mode_t	mode,		// print mode
    ccp		if_invalid	// output for invalid modes
);

//-----------------------------------------------------------------------------

int GetSizeFW // get a good value field width
(
    size_mode_t	mode,		// print mode
    int		min_fw		// minimum fw => return max(calc_fw,min_fw);
				// this value is also returned for invalid modes
);

//-----------------------------------------------------------------------------

char * PrintSize
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    sizeform_mode_t	sform_mode,	// output format, bit field
    size_mode_t		mode		// print mode
);

//-----------------------------------------------------------------------------

char * PrintSize1000
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    sizeform_mode_t	sform_mode	// output format, bit field
);

//-----------------------------------------------------------------------------

char * PrintSize1024
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			size,		// size to print
    sizeform_mode_t	sform_mode	// output format, bit field
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    scan size			///////////////
///////////////////////////////////////////////////////////////////////////////

extern u64 (*ScanSizeFactorHook)
(
    char	ch_factor,		// char to analyze
    int		force_base		// if 1000|1024: force multiple of this
);

u64 ScanSizeFactor
(
    char	ch_factor,		// char to analyze
    int		force_base		// if 1000|1024: force multiple of this
);

char * ScanSizeTerm
(
    double	*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    int		force_base		// if 1000|1024: force multiple of this
);

char * ScanSize
(
    double	*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base		// if 1000|1024: force multiple of this
);

char * ScanSizeU32
(
    u32		*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base		// if 1000|1024: force multiple of this
);

char * ScanSizeU64
(
    u64		*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base		// if 1000|1024: force multiple of this
);

//-----------------------------------------------------------------------------
// strto*() like wrappers

u32 GetSizeU32	( ccp src, char **end, int force_base );
u64 GetSizeU64	( ccp src, char **end, int force_base );
double GetSizeD	( ccp src, char **end, int force_base );

//-----------------------------------------------------------------------------

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
    double	max_value		// >0: max value for open ranges
);

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
    u32		max_value		// >0: max value for open ranges
);

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
    u64		max_value		// >0: max value for open ranges
);

//-----------------------------------------------------------------------------

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
    double	max_value		// >0: max value for open ranges
);

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
    u32		max_value		// >0: max value for open ranges
);

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
    u64		max_value		// >0: max value for open ranges
);

//-----------------------------------------------------------------------------

enumError ScanSizeOpt
(
    double	*num,			// not NULL: store result
    ccp		source,			// source text
    u64		default_factor,		// use this factor if number hasn't one
    u64		default_factor_add,	// use this factor for summands
    int		force_base,		// if 1000|1024: force multiple of this

    ccp		opt_name,		// NULL or name of option for error messages
    u64		min,			// >0: minimum allowed value
    u64		max,			// >0: maximum allowed value
    bool	print_err		// true: print error messages
);

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
);

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
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Scan Duration			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ScanDuration_t]]

typedef enum ScanDuration_t
{
    SDUMD_SKIP_BLANKS		= 0x01,  // skip leading blanks
    SDUMD_ALLOW_COLON		= 0x02,  // allow 'H:M' and 'H:M:S'
    SDUMD_ALLOW_DIV		= 0x04,  // allow fraction 'n/d' as number

    SDUMD_ALLOW_LIST		= 0x10,  // allow lists like '9h12m'
     SDUMD_ALLOW_SPACE_SEP	= 0x20,  //  lists: allow spaces as list separator
     SDUMD_ALLOW_PLUS_SEP	= 0x40,  //  lists: allow '+' as list separator
     SDUMD_ALLOW_MINUS_SEP	= 0x80,  //  lists: allow '-' as list separator

    SDUMD_M_ALL			= 0xf7,
}
ScanDuration_t;

//-----------------------------------------------------------------------------

char * ScanDuration
(
    double		*num,		// not NULL: store result
    ccp			source,		// source text
    double		default_factor,	// default factor if no SI unit found
    ScanDuration_t	mode
);

double GetDurationFactor ( char ch );

char * ScanSIFactor
(
    // returns end of scanned string

    double	*num,			// not NULL: store result
    ccp		source,			// source text
    double	default_factor		// return this if no factor found
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			time & timer			///////////////
///////////////////////////////////////////////////////////////////////////////

//--- conversion factors

#define SEC_PER_MIN	60
#define SEC_PER_HOUR	3600
#define SEC_PER_DAY	86400
#define SEC_PER_WEEK	604800

#define MSEC_PER_SEC	1000ull
#define MSEC_PER_MIN	(60*MSEC_PER_SEC)
#define MSEC_PER_HOUR	(3600*MSEC_PER_SEC)
#define MSEC_PER_DAY	(SEC_PER_DAY*MSEC_PER_SEC)
#define MSEC_PER_WEEK	(SEC_PER_WEEK*MSEC_PER_SEC)

#define USEC_PER_MSEC	1000ull
#define USEC_PER_SEC	1000000ull
#define USEC_PER_MIN	(60*USEC_PER_SEC)
#define USEC_PER_HOUR	(3600*USEC_PER_SEC)
#define USEC_PER_DAY	(SEC_PER_DAY*USEC_PER_SEC)
#define USEC_PER_WEEK	(SEC_PER_WEEK*USEC_PER_SEC)

//--- time types

typedef uint u_sec_t;	// unsigned type to store time as seconds
typedef int  s_sec_t;	//   signed type to store time as seconds
typedef u64  u_msec_t;	// unsigned type to store time as milliseconds
typedef s64  s_msec_t;	//   signed type to store time as milliseconds
typedef u64  u_usec_t;	// unsigned type to store time as microseconds
typedef s64  s_usec_t;	//   signed type to store time as microseconds

//--- only valid after call to SetupTimezone()

extern s64 timezone_adjust_sec;
extern s64 timezone_adjust_usec;

void SetupTimezone ( bool force );

u_sec_t  GetTimeSec  ( bool localtime );
u_msec_t GetTimeMSec ( bool localtime );
u_usec_t GetTimeUSec ( bool localtime );

u_msec_t GetTimerMSec();
u_usec_t GetTimerUSec();

static inline u64 double2msec ( double d ) { return d>0.0 ? (u64)trunc( 1e3*d+0.5 ) : 0; }
static inline u64 double2usec ( double d ) { return d>0.0 ? (u64)trunc( 1e6*d+0.5 ) : 0; }

static inline double msec2double ( u64 num ) { return num * 1e-3; }
static inline double usec2double ( u64 num ) { return num * 1e-6; }

//--- another epoch: Monday, 2001-01-01

#define EPOCH_2001_SEC  0x3a4fc880
#define EPOCH_2001_MSEC (EPOCH_2001_SEC*MSEC_PER_SEC)
#define EPOCH_2001_USEC (EPOCH_2001_SEC*USEC_PER_SEC)

//-----------------------------------------------------------------------------

ccp PrintTimeSec
(
    char		* buf,		// result buffer (>19 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec		// seconds (=time) to print
);

//-----------------------------------------------------------------------------

ccp PrintTimeMSec
(
    char		* buf,		// result buffer (>23 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// milliseconds to print
    uint		fraction	// number of digits (0-3) to print as fraction
);

//-----------------------------------------------------------------------------

ccp PrintTimeUSec
(
    char		* buf,		// result buffer (>26 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    uint		fraction	// number of digits (0-6) to print as fraction
);

//-----------------------------------------------------------------------------

ccp PrintTimerMSec
(
    char		* buf,		// result buffer (>12 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			msec,		// milliseconds to print
    uint		fraction	// number of digits (0-3) to print as fraction
);

ccp PrintTimerUSec
(
    char		* buf,		// result buffer (>16 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    uint		fraction	// number of digits (0-3) to print as fraction
);

//-----------------------------------------------------------------------------

ccp PrintTimer3 // helper function
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction, otherwise suppress ms/us output
    bool		aligned		// true: print aligned 3 character output
);

static inline ccp PrintTimerSec3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec,		// seconds to print
    bool		aligned		// true: print aligned 3 character output
)
{
    return PrintTimer3(buf,buf_size,sec,-1,aligned);
}

static inline ccp PrintTimerMSec3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// microseconds to print
    bool		aligned		// true: print aligned 3 character output
)
{
    return PrintTimer3(buf,buf_size,msec/1000,msec%1000*1000,aligned);
}

static inline ccp PrintTimerUSec3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    bool		aligned		// true: print aligned 3 character output
)
{
    return PrintTimer3(buf,buf_size,usec/1000000,usec%1000000,aligned);
}

//-----------------------------------------------------------------------------

ccp PrintTimer4 // helper function
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction, otherwise suppress ms/us output
    bool		aligned		// true: print aligned 4 character output
);

static inline ccp PrintTimerSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec,		// seconds to print
    bool		aligned		// true: print aligned 4 character output
)
{
    return PrintTimer4(buf,buf_size,sec,-1,aligned);
}

static inline ccp PrintTimerMSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// microseconds to print
    bool		aligned		// true: print aligned 4 character output
)
{
    return PrintTimer4(buf,buf_size,msec/1000,msec%1000*1000,aligned);
}

static inline ccp PrintTimerUSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    bool		aligned		// true: print aligned 4 character output
)
{
    return PrintTimer4(buf,buf_size,usec/1000000,usec%1000000,aligned);
}

//-----------------------------------------------------------------------------

ccp PrintTimer6 // helper function
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction, otherwise suppress ms/us output
    bool		aligned		// true: print aligned 6 character output
);

static inline ccp PrintTimerSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec,		// seconds to print
    bool		aligned		// true: print aligned 6 character output
)
{
    return PrintTimer6(buf,buf_size,sec,-1,aligned);
}

static inline ccp PrintTimerMSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// microseconds to print
    bool		aligned		// true: print aligned 6 character output
)
{
    return PrintTimer6(buf,buf_size,msec/1000,msec%1000*1000,aligned);
}

static inline ccp PrintTimerUSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    bool		aligned		// true: print aligned 6 character output
)
{
    return PrintTimer6(buf,buf_size,usec/1000000,usec%1000000,aligned);
}

//-----------------------------------------------------------------------------

ccp PrintTimerUSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// support of DC_SFORM_ALIGN, DC_SFORM_PLUS
);

static inline ccp PrintTimerMSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_msec_t		msec,		// milliseconds to print
    sizeform_mode_t	mode		// support of DC_SFORM_ALIGN, DC_SFORM_PLUS
)
{
    return PrintTimerUSec4s(buf,buf_size,1000*msec,mode);
}

static inline ccp PrintTimerSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    int			sec,		// seconds to print
    sizeform_mode_t	mode		// support of DC_SFORM_ALIGN, DC_SFORM_PLUS
)
{
    return PrintTimerUSec4s(buf,buf_size,1000000ll*sec,mode);
}

//-----------------------------------------------------------------------------

ccp PrintTimerUSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// support of DC_SFORM_ALIGN, DC_SFORM_PLUS
);

static inline ccp PrintTimerMSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_msec_t		msec,		// milliseconds to print
    sizeform_mode_t	mode		// support of DC_SFORM_ALIGN, DC_SFORM_PLUS
)
{
    return PrintTimerUSec7s(buf,buf_size,1000*msec,mode);
}

static inline ccp PrintTimerSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    int			sec,		// seconds to print
    sizeform_mode_t	mode		// support of DC_SFORM_ALIGN, DC_SFORM_PLUS
)
{
    return PrintTimerUSec7s(buf,buf_size,1000000ll*sec,mode);
}

//-----------------------------------------------------------------------------

typedef struct xtime_t
{
    time_t	time;
    suseconds_t	usec;
    u64		total_usec;
    u64	rel_usec;

} xtime_t;

u64 GetXTime ( xtime_t * xtime );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan date & time		///////////////
///////////////////////////////////////////////////////////////////////////////

// ScanInterval*() is old ==> try ScanDuration()

char * ScanInterval
(
    time_t	*res_time,	// not NULL: store seconds
    u32		*res_usec,	// not NULL: store micro seconds of second
    ccp		source		// source text
);

///////////////////////////////////////////////////////////////////////////////

char * ScanInterval64
(
    u64		*res_usec,	// not NULL: store total micro seconds
    ccp		source		// source text
);

///////////////////////////////////////////////////////////////////////////////

char * ScanDateTime
(
    time_t	*res_time,	// not NULL: store seconds
    u32		*res_usec,	// not NULL: store micro seconds of second
    ccp		source,		// source text
    bool	allow_delta	// true: allow +|- interval
);

///////////////////////////////////////////////////////////////////////////////

char * ScanDateTime64
(
    u64		*res_usec,	// not NULL: store total micro seconds
    ccp		source,		// source text
    bool	allow_delta	// true: allow +|- interval
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PrintIP4*()			///////////////
///////////////////////////////////////////////////////////////////////////////

#define FW_IPV4_A 12	// 255.16777215
#define FW_IPV4_B 13	// 255.255.65535
#define FW_IPV4_C 15	// 255.255.255.255

#define FW_IPV4_PORT 5  // 65535

#define FW_IPV4_A_PORT (FW_IPV4_A+1+FW_IPV4_PORT)
#define FW_IPV4_B_PORT (FW_IPV4_B+1+FW_IPV4_PORT)
#define FW_IPV4_C_PORT (FW_IPV4_C+1+FW_IPV4_PORT)

//-----------------------------------------------------------------------------

char * PrintIP4A
(
    // print in A-notation

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

char * PrintIP4B
(
    // print in A-notation

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

//-----------------------------------------------------------------------------
// print in C-notation (usual notation)

char * PrintIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

char * PrintLeftIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

char * PrintRightIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port,		// 0..0xffff: add ':port'
    uint	port_mode	// 0: 'ip:port', 1: 'ip :port', 2: 'ip : port'
);

char * PrintAlignedIP4
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

//-----------------------------------------------------------------------------
struct sockaddr_in;

static inline char * PrintIP4sa
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL

    const struct sockaddr_in *sa // socket address
)
{
    DASSERT(sa);
    return PrintIP4(buf,buf_size,
		ntohl(sa->sin_addr.s_addr), ntohs(sa->sin_port) );
}

static inline char * PrintRightIP4sa
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL

    struct sockaddr_in *sa	// socket address
)
{
    DASSERT(sa);
    return PrintRightIP4(buf,buf_size,
		ntohl(sa->sin_addr.s_addr), ntohs(sa->sin_port), 0 );
}

static inline char * PrintAlignedIP4sa
(
    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL

    struct sockaddr_in *sa	// socket address
)
{
    DASSERT(sa);
    return PrintAlignedIP4(buf,buf_size,
		ntohl(sa->sin_addr.s_addr), ntohs(sa->sin_port) );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			dynamic data support		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct DynData_t
{
    u8		*data;		// pointer to data, alloced
    uint	len;		// length of used data
    uint	size;		// size of allocated data

} DynData_t;

//-----------------------------------------------------------------------------

void ResetDD  ( DynData_t *dd );
void InsertDD ( DynData_t *dd, const void *data, uint data_size );

///////////////////////////////////////////////////////////////////////////////

typedef struct DynDataList_t
{
    DynData_t	*list;		// pointer to the data list
    uint	used;		// number of used data elements
    uint	size;		// number of allocated pointer in 'list'

} DynDataList_t;

//-----------------------------------------------------------------------------

void ResetDDL ( DynDataList_t *ddl );

DynData_t *InsertDDL
(
    DynDataList_t	*ddl,		// valid dynamic data list
    uint		index,		// element index to change
    const void		*data,		// NULL or data to assign
    uint		data_size	// size of data
);

DynData_t *GetDDL
(
    DynDataList_t	*ddl,		// valid dynamic data list
    uint		index,		// element index to get
    uint		create		// >0: create an empty element if not exist
					// >index: create at least # elements
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct StringField_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[StringField_t]]

typedef struct StringField_t
{
    ccp		* field;		// pointer to the string field
    uint	used;			// number of used titles in the title field
    uint	size;			// number of allocated pointer in 'field'
    int	(*func_cmp)( ccp s1, ccp s2 );	// compare function, default is strcmp()

} StringField_t;

//-----------------------------------------------------------------------------

static inline void InitializeStringField ( StringField_t * sf )
	{ DASSERT(sf); memset(sf,0,sizeof(*sf)); }

void ResetStringField ( StringField_t * sf );
void MoveStringField ( StringField_t * dest, StringField_t * src );

int FindStringFieldIndex ( const StringField_t * sf, ccp key, int not_found_value );
ccp FindStringField ( const StringField_t * sf, ccp key );

// return: true if item inserted/deleted
bool InsertStringField ( StringField_t * sf, ccp key,  bool move_key );
bool RemoveStringField ( StringField_t * sf, ccp key );
uint RemoveStringFieldByIndex ( StringField_t * sf, int index, int n );

// helper function
ccp * InsertStringFieldHelper ( StringField_t * sf, int idx );

// append at the end an do not sort
void AppendStringField ( StringField_t * sf, ccp key, bool move_key );

// re-sort field using sf->func_cmp()
void SortStringField ( StringField_t * sf );

// return the index of the (next) item
uint FindStringFieldHelper ( const StringField_t * sf, bool * found, ccp key );

// find first pattern that matches
ccp MatchStringField ( const StringField_t * sf, ccp key );

//-----------------------------------------------------------------------------
// file support

enumError LoadStringField
(
    StringField_t	* sf,		// string field
    bool		init_sf,	// true: initialize 'sf' first
    bool		sort,		// true: sort 'sf'
    ccp			filename,	// filename of source file
    bool		silent		// true: don't print open/read errors
);

enumError SaveStringField
(
    StringField_t	* sf,		// valid string field
    ccp			filename,	// filename of dest file
    bool		rm_if_empty	// true: rm dest file if 'sf' is empty
);

enumError WriteStringField
(
    FILE		* f,		// open destination file
    ccp			filename,	// NULL or filename (needed on write error)
    StringField_t	* sf,		// valid string field
    ccp			prefix,		// not NULL: insert prefix before each line
    ccp			eol		// end of line text (if NULL: use LF)
);

//-----------------------------------------------------------------------------
// save-state support

void SaveCurrentStateStringField
(
    FILE		*f,		// output file
    ccp			name_prefix,	// NULL or prefix of name, up to 40 chars
    uint		tab_pos,	// tab pos of '='
    const StringField_t	*sf,		// valid string field
    EncodeMode_t	emode		// encoding mode
);

struct RestoreState_t;
void RestoreStateStringField
(
    struct RestoreState_t *rs,		// info data, may be modified
    ccp			name_prefix,	// NULL or prefix of name, up to 40 chars
    StringField_t	*sf,		// string field
    bool		init_sf,	// true: initialize 'sf', otherwise reset
    EncodeMode_t	emode		// encoding mode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct ParamField_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ParamFieldItem_t]]

typedef struct ParamFieldItem_t
{
    ccp			key;		// string as main key
    uint		num;		// user defined numeric parameter
    void		*data;		// user defined pointer

} ParamFieldItem_t;

//-----------------------------------------------------------------------------
// [[ParamField_t]]

typedef struct ParamField_t
{
    ParamFieldItem_t	* field;	// pointer to the string field
    uint		used;		// number of used titles in the title field
    uint		size;		// number of allocated pointer in 'field'
    bool		free_data;	// true: unused data will be free'd
					//   initialized with 'false'
    int	(*func_cmp)( ccp s1, ccp s2 );	// compare function, default is strcmp()
} ParamField_t;

//-----------------------------------------------------------------------------

void InitializeParamField ( ParamField_t * pf );
void ResetParamField ( ParamField_t * pf );
void MoveParamField ( ParamField_t * dest, ParamField_t * src );

int FindParamFieldIndex ( const ParamField_t * pf, ccp key, int not_found_value );
ParamFieldItem_t * FindParamField ( const ParamField_t * pf, ccp key );
ParamFieldItem_t * FindParamFieldByNum ( const ParamField_t * pf, uint num );
ParamFieldItem_t * FindInsertParamField
		( ParamField_t * pf, ccp key, bool move_key, bool *old_found );
ParamFieldItem_t * CountParamField ( ParamField_t * pf, ccp key, bool move_key );

// return: true if item inserted/deleted
bool InsertParamField ( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data );
bool ReplaceParamField( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data );
bool RemoveParamField ( ParamField_t * pf, ccp key );

ParamFieldItem_t * InsertParamFieldEx
	( ParamField_t * pf, ccp key, bool move_key, uint num, bool *found );

// append at the end an do not sort
ParamFieldItem_t * AppendParamField
	( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data );

// return the index of the (next) item
uint FindParamFieldHelper ( const ParamField_t * pf, bool * found, ccp key );

// find first pattern that matches
ParamFieldItem_t * MatchParamField ( const ParamField_t * pf, ccp key );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Memory Map			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MemMapItem_t]]

typedef struct MemMapItem_t
{
    u64		off;		// offset
    u64		size;		// size
    u8		overlap;	// system info: item overlaps other items
    u8		index;		// user defined index
    char	info[62];	// user defined info text

} MemMapItem_t;

//-----------------------------------------------------------------------------
// [[MemMap_t]]

typedef struct MemMap_t
{
    MemMapItem_t ** field;	// pointer to the item field
    uint	used;		// number of used titles in the item field
    uint	size;		// number of allocated pointer in 'field'
    u64		begin;		// first address

} MemMap_t;

//-----------------------------------------------------------------------------
//	Memory maps allow duplicate entries.
//	The off+size pair is used as key.
//	The entries are sorted by off and size (small values first).
//-----------------------------------------------------------------------------

void InitializeMemMap ( MemMap_t * mm );
void ResetMemMap ( MemMap_t * mm );

void CopyMemMap
(
    MemMap_t		* mm1,		// merged mem map, not NULL, cleared
    const MemMap_t	* mm2,		// NULL or second mem map
    bool		use_tie		// TRUE: use InsertMemMapTie()
);

void MergeMemMap
(
    MemMap_t		* mm1,		// merged mem map, not NULL, not cleared
    const MemMap_t	* mm2,		// NULL or second mem map
    bool		use_tie		// TRUE: use InsertMemMapTie()
);

MemMapItem_t * FindMemMap
(
    // returns NULL or the pointer to the one! matched key (={off,size})

    MemMap_t		* mm,
    off_t		off,
    off_t		size
);

uint InsertMemMapIndex
(
    // returns the index of the new item

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
);

MemMapItem_t * InsertMemMap
(
    // returns a pointer to a new item (never NULL)

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
);

MemMapItem_t * InsertMemMapTie
(
    // returns a pointer to a new or existing item (never NULL)

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
);

void InsertMemMapWrapper
(
    void		* param,	// user defined parameter
    u64			offset,		// offset of object
    u64			size,		// size of object
    ccp			info		// info about object
);

// Remove all entires with key. Return number of delete entries
//uint RemoveMemMap ( MemMap_t * mm, off_t off, off_t size );

// Return the index of the found or next item
uint FindMemMapHelper ( MemMap_t * mm, off_t off, off_t size );

// Calculate overlaps. Return number of items with overlap.
uint CalcOverlapMemMap ( MemMap_t * mm );

u64 FindFreeSpaceMemMap
(
    // return address or NULL on not-found

    const MemMap_t	* mm,
    u64			addr_beg,	// first possible address
    u64			addr_end,	// last possible address
    u64			size,		// minimal size
    u64			align,		// aligning
    u64			*space		// not NULL: store available space here
);

// Print out memory map
void PrintMemMap ( MemMap_t * mm, FILE * f, int indent, ccp info_head );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    Memory Map: Scan addresses		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ScanAddr_t]]

typedef struct ScanAddr_t
{
    uint	stat;		// 0:invalid, 1:single address, 2:range
    uint	addr;		// address
    uint	size;		// size inf bytes

} ScanAddr_t;

//-----------------------------------------------------------------------------

char * ScanAddress
(
    // returns first not scanned char of 'arg'

    ScanAddr_t	* result,	// result, initialized by ScanAddress()
    ccp		arg,		// format: ADDR | ADDR:END | ADDR#SIZE
    uint	default_base	// base for numbers without '0x' prefix
				//  0: C like with octal support
				// 10: standard value for decimal numbers
				// 16: standard value for hex numbers
);

//-----------------------------------------------------------------------------

uint InsertAddressMemMap
(
    // returns the number of added addresses

    MemMap_t	* mm,		// mem map pointer
    bool	use_tie,	// TRUE: use InsertMemMapTie()
    ccp		arg,		// comma separated list of addresses
				// formats: ADDR | ADDR:END | ADDR#SIZE
    uint	default_base	// base for numbers without '0x' prefix
				//  0: C like with octal support
				// 10: standard value for decimal numbers
				// 16: standard value for hex numbers
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    DataBuf_t			///////////////
///////////////////////////////////////////////////////////////////////////////

struct DataBuf_t;

typedef enumError (*DataBufFill_t)
(
    // function to fill the buffer

    struct DataBuf_t*	db,	// valid pointer to related DataBuf_t to fill
    uint		need,	// number of bytes needed => wait for more data
    int			mode	// user mode, null on auto fill
);

//-----------------------------------------------------------------------------
// [[DataBuf_t]]

typedef struct DataBuf_t
{
    uint	size;		// size of buffer
    uint	used;		// number of used bytes
    u64		offset;		// current file offset, related to 'data'

    u8		*buf;		// pointer to buffer, cyclic usage
    u8		*buf_end;	// pointer to buffer end := buf + size

    u8		*data;		// pointer to data
    u8		*data_end;	// pointer to end of data

    //--- user data

    DataBufFill_t fill_func;	// function to fill buffer
    void	*user_data;	// NULL or pointer to user data for 'fill_func'
}
DataBuf_t;

///////////////////////////////////////////////////////////////////////////////
// management

void InitializeDataBuf
	( DataBuf_t *db, uint buf_size, DataBufFill_t func, void *user_data );
void ResetDataBuf	( DataBuf_t *db );
void ResizeDataBuf	( DataBuf_t *db, uint new_buf_size );
void ClearDataBuf	( DataBuf_t *db, uint new_buf_size_if_not_null );
void EmptyDataBuf	( DataBuf_t *db );
void PackDataBuf	( DataBuf_t *db );

///////////////////////////////////////////////////////////////////////////////
// debugging

bool IsValidDataBuf ( DataBuf_t *db );
bool DumpDataBuf ( FILE *f, uint indent, DataBuf_t *db, bool if_invalid );

// fill the buffer with a test pattern:
//	'-' for unused bytes
//	">0123456789<" in a longer or shorter form for used bytes
void FillPatternDataBuf ( DataBuf_t *db );

///////////////////////////////////////////////////////////////////////////////
// insert and remove data

static inline enumError FillDataBuf( DataBuf_t *db, uint need, int mode )
{
    return db && db->fill_func ? db->fill_func(db,need,mode) : ERR_WARNING;
}

uint InsertDataBuf
(
    // Insert at top of buffer.
    // Return the number of inserted bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    const void	*data,		// data to insert
    uint	data_size,	// size of 'data'
    bool	all_or_none	// true: insert complete data or nothing
);

uint AppendDataBuf
(
    // Append data at end of buffer
    // Return the number of inserted bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    const void	*data,		// data to insert
    uint	data_size,	// size of 'data'
    bool	all_or_none	// true: insert complete data or nothing
);

///////////////////////////////////////////////////////////////////////////////

void * GetPtrDataBuf
(
    // Return a pointer to the data and make sure,
    // that the data is not wrapped. Return NULL if failed.

    DataBuf_t	*db,		// valid pointer to data buffer
    uint	data_size,	// number of needed bytes
    uint	align,		// >1: Data alignment is also needed.
				//  Only 2,4,8,16,... are allowed, but not verified.
				//  Begin of buffer is always well aligned.
    bool	drop		// true: On success, drop data from buffer
				//       In real, the data is available and only
				//       the pointers and counters are updated.
);

uint CopyDataBuf
(
    // Copy data from buffer, but don't remove it
    // Return the number of copied bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    void	*dest,		// destination buffer wit at least 'max_size' space
    uint	min_read,	// return with 0 read bytes if less bytes available
				// if 'min_read' > 'size', NULL is returned
    uint	max_read	// try to read upto 'max_read' bytes
);

uint GetDataBuf
(
    // Copy data from buffer and remove it
    // Return the number of copied bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    void	*dest,		// destination buffer wit at least 'max_size' space
    uint	min_read,	// return with 0 read bytes if less bytes available
    uint	max_read	// try to read upto 'max_read' bytes
);

uint SkipDataBuf
(
    // Skip 'skip_size' from input stream. Read more data if needed.
    // Return the number of really skiped bytes
    // If <skip_size is returned, no more data is available.

    DataBuf_t	*db,		// valid pointer to data buffer
    uint	skip_size	// number of bytes to drop
);

uint DropDataBuf
(
    // Remove upto 'drop_size' bytes from buffer.
    // Return the number of dropped bytes.

    DataBuf_t	*db,		// valid pointer to data buffer
    uint	drop_size	// number of bytes to drop
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		ResizeElement_t, ResizeHelper_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct ResizeElement_t
{
    uint n_elem;
    uint factor;
}
ResizeElement_t;

//-----------------------------------------------------------------------------

typedef struct ResizeHelper_t
{
    //--- basic data

    uint	src_size;	// size of source
    uint	dest_size;	// size of dest


    //--- calculated data

    uint	sum_factor;	// sum of all factors
    uint	half_factor;	// half of 'sum_factor2'

    uint	idx_inc;	// increment of 'src_idx'
    uint	frac_inc;	// increment of 'src_frac'
    uint	frac_next;	// if 'frac>=frac_next': increment 'src_idx'


    //--- iteration data

    uint	dest_idx;	// dest index: (0..dest_size(
    uint	src_idx;	// src index:  (0..src_size(
    uint	src_frac;	// (0..frac_next(

    ResizeElement_t		// list terminates with n_elem=0
		elem[4];	//  ==> max 3 elements are used

}
ResizeHelper_t;

//-----------------------------------------------------------------------------

void InitializeResize
(
    ResizeHelper_t	*r,		// data structure
    uint		s_size,		// size of source
    uint		d_size		// size of dest
);

bool FirstResize ( ResizeHelper_t *r );
bool NextResize  ( ResizeHelper_t *r );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan configuration		///////////////
///////////////////////////////////////////////////////////////////////////////
// Definitons placed here because it is used scattered by dcLib

typedef enum RestoreStateLog_t
{
    RSL_WARNINGS	= 0x01,	// general warnings
    RSL_MISSED_NAMES	= 0x02,	// hints about missed names
    RSL_UNUSED_NAMES	= 0x04,	// hints about unused names

    RSL_ALL		= 0x07	// all flags
}
RestoreStateLog_t;

//-----------------------------------------------------------------------------
// [[RestoreState_t]]

typedef struct RestoreState_t
{
    ccp			sect;		// section name
    ccp			path;		// section sub path
    int			index;		// index or -1
    ParamField_t	param;		// name=value parameters
    cvp			user_param;	// pointer provided as parameter by user
    cvp			user_info;	// pointer to deliver insfos between stages

    RestoreStateLog_t	log_mode;	// log mode
    FILE		*log_file;	// log file

}
RestoreState_t;

//-----------------------------------------------------------------------------

typedef void (*RestoreStateFunc)
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

//-----------------------------------------------------------------------------

typedef struct RestoreStateTab_t
{
    ccp			name;		// name of section
    RestoreStateFunc	func;		// function to call
    cvp			user_table;	// any user provided pointer
}
RestoreStateTab_t;

///////////////////////////////////////////////////////////////////////////////

enumError RestoreState
(
    const RestoreStateTab_t
			*table,		// section table, terminated by name==0
    void		*data,		// file data, modified, terminated by NULL or LF
    uint		size,		// size of file data
    RestoreStateLog_t	log_mode,	// print warnings (bit field)
    FILE		*log_file,	// error file; if NULL use stderr
    cvp			user_param	// pointer provided by user
);

///////////////////////////////////////////////////////////////////////////////

// print into cyclic buffer
ccp PrintRestoreStateSection ( const RestoreState_t *rs );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * GetParamField
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name		// name of member
);

//-----------------------------------------------------------------------------

int GetParamFieldINT
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    int			not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

uint GetParamFieldUINT
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    uint		not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

u64 GetParamFieldU64
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    u64			not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

int GetParamFieldBUF
(
    // returns >=0: length of read data; -1:nothing done (not_found==NULL)

    char		*buf,		// buffer to store result
    uint		buf_size,	// size of buffer

    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    EncodeMode_t	decode,		// decoding mode, fall back to OFF
					// supported: STRING, UTF8, BASE64, BASE64X
    ccp			not_found	// not NULL: store this value if not found
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    GrowBuffer_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[GrowBuffer_t]]

typedef struct GrowBuffer_t
{
    // allways:  gb->ptr[gb->used] == 0

    u8		*buf;		// NULL or data buffer
    uint	size;		// size of 'buf'
    uint	grow_size;	// >0: auto grow buffer by multiple of this
    uint	max_size;	// >0: max size for auto grow

    u8		*ptr;		// pointer to first valid byte
    uint	used;		// number of valid bytes behind 'ptr'
    uint	max_used;	// max 'used' value

    uint	disabled;	// disable rading from TCP socket
}
GrowBuffer_t;

///////////////////////////////////////////////////////////////////////////////

void LogGrowBuffer
(
    FILE		*f,		// output file
    int			indent,		// indention
    const GrowBuffer_t	*gb,		// valid grow-buffer
    ccp			format,		// format string for vfprintf()
    ...					// arguments for 'vfprintf(format,...)'
)
__attribute__ ((__format__(__printf__,4,5)));

//-----------------------------------------------------------------------------

void InitializeGrowBuffer ( GrowBuffer_t *gb, uint max_buf_size );
void ResetGrowBuffer ( GrowBuffer_t *gb );
uint ClearGrowBuffer ( GrowBuffer_t *gb ); // returns available space
uint PurgeGrowBuffer ( GrowBuffer_t *gb ); // returns available space

//-----------------------------------------------------------------------------

uint GetSpaceGrowBuffer
(
    // returns the maximum possible space
    const GrowBuffer_t	*gb		// valid grow-buffer object
);

//-----------------------------------------------------------------------------

uint PrepareGrowBuffer
(
    // returns the number of available space, smaller than or equal 'size'

    GrowBuffer_t	*gb,	// valid grow-buffer object
    uint		size,	// needed size
    bool		force	// always alloc enough space and return 'size'
);

uint InsertGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    const void		*data,	// data to insert
    uint		size	// size of 'data'
);

static inline uint InsertCharGrowBuffer
(
    // returns the number of inserted bytes (0 on fail, 1 else)

    GrowBuffer_t	*gb,	// valid grow-buffer object
    char		ch	// any character including NULL
)
{
    return InsertGrowBuffer(gb,&ch,1);
}

static inline uint InsertMemGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    mem_t		mem	// any data
)
{
    return InsertGrowBuffer(gb,mem.ptr,mem.len);
}

uint InsertCapNGrowBuffer
(
    // insert 'cap_1' or 'cap_n' depending of 'n' and the existence of 'cap_*'
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    int			n,	// repeat number; if <1: insert nothing
    mem_t		cap_1,	// default for N=1
    mem_t		cap_n	// default for N>1
);

uint WriteCapN
(
    // print 'cap_1' or 'cap_n' depending of 'n' and the existence of 'cap_*'
    // returns the number of printed bytes

    FILE		*f,	// valid FILE
    int			n,	// repeat number; if <1: insert nothing
    mem_t		cap_1,	// default for N=1
    mem_t		cap_n	// default for N>1
);

uint InsertCatchedGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    SavedStdFiles_t	*ssf	// saved output and catched data
);

///////////////////////////////////////////////////////////////////////////////

uint PrintArgGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    ccp			format,	// format string for vfprintf()
    va_list		arg	// arguments for 'vsnprintf(format,...)'
);

uint PrintGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    ccp			format,	// format string for vfprintf()
    ...				// arguments for 'vsnprintf(format,...)'
)
__attribute__ ((__format__(__printf__,2,3)));

//-----------------------------------------------------------------------------

uint ConvertToCrLfGrowBuffer
(
    // returns the number of inserted CR bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    uint		begin	// index to begin with conversion
);

//-----------------------------------------------------------------------------

uint DropGrowBuffer
(
    // returns the number of dropped bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    uint		size	// number of bytes to drop
);

//-----------------------------------------------------------------------------
#ifndef __APPLE__

FILE * OpenFileGrowBuffer
(
    // returns a FILE opened by fmemopen() => call CloseFileGrowBuffer()

    GrowBuffer_t	*gb,	// valid grow-buffer object
    uint		size	// needed size
);

int CloseFileGrowBuffer
(
    // returns the number of wirtten bytes, or -1 on error
    GrowBuffer_t	*gb,	// valid grow-buffer object
    FILE		*f	// FILE returned by OpenFileGrowBuffer()
);

#endif // !__APPLE__
//-----------------------------------------------------------------------------

void SaveCurrentStateGrowBuffer
(
    FILE		*f,		// output file
    ccp			name_prefix,	// NULL or prefix of name
    uint		tab_pos,	// tab pos of '='
    const GrowBuffer_t	*gb		// valid grow-buffer
);

void RestoreStateGrowBuffer
(
    GrowBuffer_t	*gb,		// valid grow-buffer
    ccp			name_prefix,	// NULL or prefix of name
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
);

//-----------------------------------------------------------------------------

int WriteDirectGrowBuffer
(
    // write() data direct without blocking and without calling any notifier.
    // If output buf is not empty or send failed, append the data to 'gb'.
    // Returns the number of bytes added to the grow buffer or -1 on error.
    // The data is send+stored completely (returns 'size') or not at all
    // (returns '0'). -1 is returned if the socket is invalid.

    GrowBuffer_t	*gb,		// grow buffer to cache data
    int			fd,		// destination file
    bool		flush_output,	// true: try to flush 'gb' before
    cvp			data,		// data to send
    uint		size,		// size of 'data', If NULL && flush: flush only
    uint		*send_count	// not NULL: add num of sent bytes
);

//-----------------------------------------------------------------------------

int SendDirectGrowBuffer
(
    // send() data direct without blocking and without calling any notifier.
    // If output buf is not empty or send failed, append the data to 'gb'.
    // Returns the number of bytes added to the grow buffer or -1 on error.
    // The data is send+stored completely (returns 'size') or not at all
    // (returns '0'). -1 is returned if the socket is invalid.

    GrowBuffer_t	*gb,		// grow buffer to cache data
    int			sock,		// destination socket
    bool		flush_output,	// true: try to flush 'gb' before
    cvp			data,		// data to send
    uint		size,		// size of 'data', If NULL && flush: flush only
    uint		*send_count	// not NULL: add num of sent bytes
);

//-----------------------------------------------------------------------------

static inline int FlushGrowBuffer
(
    // Try to flush the bufer using SendDirectGrowBuffer() withour new data.

    GrowBuffer_t	*gb,		// grow buffer to cache data
    int			sock,		// destination socket
    uint		*send_count	// not NULL: add num of sent bytes
)
{
    return SendDirectGrowBuffer(gb,sock,true,0,0,send_count);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    misc			///////////////
///////////////////////////////////////////////////////////////////////////////

#define CONVERT_TO_STRING1(a) #a
#define CONVERT_TO_STRING(a) CONVERT_TO_STRING1(a)

extern int opt_new; // default 0

float double2float ( double d ); // reduce precision

uint CreateUniqueId ( int range );

///////////////////////////////////////////////////////////////////////////////

static uint snprintfS ( char *buf, size_t size, ccp format, ... )
	__attribute__ ((__format__(__printf__,3,4)));

static inline uint snprintfS ( char *buf, size_t size, ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    const int res = vsnprintf(buf,size,format,arg);
    va_end(arg);

    return res < 0 ? 0 : res < size ? res : size - 1;
}

static char * snprintfE ( char *buf, char *end, ccp format, ... )
	__attribute__ ((__format__(__printf__,3,4)));

static inline char * snprintfE ( char *buf, char *end, ccp format, ... )
{
    const int size = end - buf;
    DASSERT( size > 0 );

    va_list arg;
    va_start(arg,format);
    const int res = vsnprintf(buf,size,format,arg);
    va_end(arg);

    return buf + ( res < 0 ? 0 : res < size ? res : size - 1 );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_BASICS_H
