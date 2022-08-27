
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

#ifndef DCLIB_BASICS_H
#define DCLIB_BASICS_H 1

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/un.h>
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
///////////////			 conditionals			///////////////
///////////////////////////////////////////////////////////////////////////////
// support function utimensat()

#undef SUPPORT_UTIMENSAT

#if defined(UTIME_OMIT) && !defined(__APPLE__)
  #define SUPPORT_UTIMENSAT 1
#else
  #define SUPPORT_UTIMENSAT 0
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

#define SIZEOFMEMBER(type,a) sizeof(((type*)0)->a)

#define SIZEOFRANGE(type,a,b) \
	( offsetof(type,b) + sizeof(((type*)0)->b) - offsetof(type,a) )

#define END_OF_VAR(v) ((void*)( (char*)(v) + sizeof(v) ))

#define PTR_DISTANCE(a,b) ( (char*)(a) - (char*)(b) )
#define PTR_DISTANCE_INT(a,b) ( (int)( (char*)(a) - (char*)(b) ))

#define GET_INT_SIGN(type) ({ typeof(type) a=-1; a>0 ? 1 : -1; })
#define GET_INT_TYPE(type) ((int)sizeof(type)*GET_INT_SIGN(type))

// free a string if str is !NULL | !EmptyString | !MinusString | !IsCircBuf()
void FreeString ( ccp str );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    HAVE_*			///////////////
///////////////////////////////////////////////////////////////////////////////

#undef HAVE_CLOCK_GETTIME
#if _POSIX_C_SOURCE >= 199309L
  #define HAVE_CLOCK_GETTIME 1
#else
  #define HAVE_CLOCK_GETTIME 0
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			external types			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct ColorSet_t ColorSet_t;
typedef struct RestoreState_t RestoreState_t;
typedef struct LogFile_t LogFile_t;

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
    // __builtin_popcount(unsigned int) is slower than table based summary

    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]];
}

static inline uint Count1Bits64 ( u64 data )
{
 #if __GNUC__ > 3
    return __builtin_popcountll(data);
 #else
    const u8 * d = (u8*)&data;
    return TableBitCount[d[0]]
	 + TableBitCount[d[1]]
	 + TableBitCount[d[2]]
	 + TableBitCount[d[3]]
	 + TableBitCount[d[4]]
	 + TableBitCount[d[5]]
	 + TableBitCount[d[6]]
	 + TableBitCount[d[7]];
 #endif
}

///////////////////////////////////////////////////////////////////////////////
// return -1 if bit not found

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
void le32n ( u32     * dest, const u32     * src, int n );
void lef4n ( float32 * dest, const float32 * src, int n );

void write_be16n ( u16     * dest, const u16     * src, int n );
void write_be32n ( u32     * dest, const u32     * src, int n );
void write_bef4n ( float32 * dest, const float32 * src, int n );
void write_le16n ( u16     * dest, const u16     * src, int n );
void write_le32n ( u32     * dest, const u32     * src, int n );
void write_lef4n ( float32 * dest, const float32 * src, int n );

//-----------------------------------------------------------------------------
// [[endian_func_t]]

typedef struct endian_func_t
{
    //--- info section

    u8   bom[2];	// byte order mark: BE=0xfe,0xff, LE=0xff,0xfe
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

    //--- convert functions

    u16 (*n2hs)  ( u16 data );
    u32 (*n2hl)  ( u32 data );
    u64 (*n2h64) ( u64 data );

    u16 (*h2ns)  ( u16 data );
    u32 (*h2nl)  ( u32 data );
    u64 (*h2n64) ( u64 data );

} endian_func_t;

extern const endian_func_t be_func;
extern const endian_func_t le_func;

const endian_func_t * GetEndianFunc ( const void * byte_order_mark );
uint GetTextBOMLen ( const void * data, uint data_size );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    CopyMode_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[CopyMode_t]]

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

char * CopyString
(
    ccp			string,		// string to copy/move/link
    CopyMode_t		mode,		// copy mode
    bool		*res_alloced	// not NULL:
					//   store true, if data must be freed
					//   otherwise store false
);

///////////////////////////////////////////////////////////////////////////////

static inline void FreeData
(
    const void		*data,		// data to free
    CopyMode_t		mode		// copy mode
)
{
    if ( mode != CPM_LINK )
	FreeString(data);
}

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

extern mem_t EmptyMem;	// mem_t(EmptyString,0)
extern mem_t NullMem;	// mem_t(0,0)

//-----------------------------------------------------------------------------
// [[mem_src_t]]

typedef struct mem_src_t
{
    const mem_t	*src;		// list of sources
    int		n_src;		// number of sources; if <0: first NULL ptr terminates list
    bool	allow_write;	// TRUE: allow to write n_src & src->len if <0
}
mem_src_t;

uint GetMemSrcN ( const mem_src_t *src ); // NULL allowed
uint GetMemSrcLen ( mem_t sep, const mem_src_t *src ); // NULL allowed

//-----------------------------------------------------------------------------

static inline mem_t MemByChar ( char ch )
{
    mem_t mem = { &ch, 1 };
    return mem;
}

// NULL allowed
static inline mem_t MemByString0 ( ccp str )
{
    if (!str)
	return NullMem;

    mem_t mem = { str, strlen(str) };
    return mem;
}

// NULL forbidden
static inline mem_t MemByString ( ccp str )
{
    mem_t mem = { str, strlen(str) };
    return mem;
}

// NULL forbidden if len<0
static inline mem_t MemByStringS ( ccp str, int len )
{
    mem_t mem = { str, len < 0 ? strlen(str) : len };
    return mem;
}

// NULL forbidden if end=0
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
int CheckIndexC    ( int max, int * p_begin, int count );
int CheckIndexCEnd ( int max, int * p_begin, int count );

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

// UTF8 support

mem_t MidMem8 ( const mem_t src, int begin, int count );
mem_t ExtractMem8 ( const mem_t src, int begin, int end );
mem_t ExtractEndMem8 ( const mem_t src, int begin, int end );

static mem_t LeftMem8 ( const mem_t src, int count )
	{ return ExtractMem8(src,0,count); }

static mem_t RightMem8 ( const mem_t src, int count )
	{ return ExtractEndMem8(src,-count,0); }

//-----------------------------------------------------------------------------

// Alloc dest buffer and terminate with 0.
// If m*.len < 0, then use strlen().

mem_t MemCat2A ( const mem_t m1, const mem_t m2 );
mem_t MemCat3A ( const mem_t m1, const mem_t m2, const mem_t m3 );

mem_t MemCatSep2A ( const mem_t sep, const mem_t m1, const mem_t m2 );
mem_t MemCatSep3A ( const mem_t sep, const mem_t m1, const mem_t m2, const mem_t m3 );

//-----------------------------------------------------------------------------

int  CmpMem		( const mem_t s1, const mem_t s2 );
int  StrCmpMem		( const mem_t mem, ccp str );
bool LeftStrCmpMemEQ	( const mem_t mem, ccp str );

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
///////////////			struct exmem_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[exmem_t]]

typedef struct exmem_t
{
    mem_t	data;		// pointer and size of data
    u32		attrib;		// user defined attribute
    bool	is_original;	// true: 'data' is a copy of source
    bool	is_circ_buf;	// true: 'data' is part of circ-buf
    bool	is_alloced;	// true: 'data' is alloced (not circ-buf)
    bool	is_key_alloced;	// true: external key is alloced
}
exmem_t;

extern exmem_t EmptyExMem;	// mem_t(EmptyString,0),false,false
extern exmem_t NullExMem;	// mem_t(0,0),false,false

//-----------------------------------------------------------------------------
// [[exmem_dest_t]]

typedef struct exmem_dest_t
{
    char	*buf;		// Dest buffer. If NULL then alloc buffer.
    uint	buf_size;	// Size of 'buf'. If too small then alloc buffer.
    bool	try_circ;	// TRUE: Use circ-buffer, if result is small enough.
}
exmem_dest_t;

// Get a buffer by settings. The returned buffer is always large enough
// to store len+1 bytes. If dest==NULL: Force MALLOC()
exmem_t GetExmemDestBuf ( const exmem_dest_t * dest, uint len );

//-----------------------------------------------------------------------------
// constructors

static inline exmem_t ExMemByChar ( char ch )
{
    exmem_t em = { .data={ &ch, 1 }, .is_original=true };
    return em;
}

static inline exmem_t ExMemByString0 ( ccp str )
{
    if (!str)
	return NullExMem;

    exmem_t em = { .data={ str, strlen(str) }, .is_original=true };
    return em;
}

static inline exmem_t ExMemByString ( ccp str )
{
    exmem_t em = { .data={ str, str ? strlen(str) : 0 }, .is_original=true };
    return em;
}

static inline exmem_t ExMemByStringS ( ccp str, int len )
{
    exmem_t em = { .data={ str, !str ? 0 : len < 0 ? strlen(str) : len }, .is_original=true };
    return em;
}

static inline exmem_t ExMemByStringE ( ccp str, ccp end )
{
    exmem_t em = { .data={ str, !str ? 0 : end ? end-str : strlen(str) }, .is_original=true };
    return em;
}

static inline exmem_t ExMemByS ( cvp str, int len )
{
    exmem_t em = { .data={ str, len }, .is_original=true };
    return em;
}

static inline exmem_t ExMemByE ( cvp str, cvp end )
{
    exmem_t em = { .data={ str, (u8*)end-(u8*)str }, .is_original=true };
    return em;
}

static inline exmem_t ExMemCalloc ( int size )
{
    char *data = CALLOC(size,1);
    exmem_t em = { .data={ data, size }, .is_original=true, .is_alloced=true };
    return em;
}

static inline exmem_t ExMemDup ( cvp src, int size )
{
    char *data = MEMDUP(src,size);
    exmem_t em = { .data={ data, size+1 }, .is_original=true, .is_alloced=true };
    return em;
}

//-----------------------------------------------------------------------------

exmem_t ExMemCat
(
    exmem_dest_t	*dest,	// kind of destination, if NULL then MALLOC()
    mem_t		sep,	// insert separators between sources with len>0
    const mem_src_t	*src	// sources. NULL allowed
);

exmem_t ExMemCat2
(
    exmem_dest_t	*dest,	// kind of destination, if NULL then MALLOC()
    mem_t		sep,	// insert separators between sources with len>0
    mem_t		src1,	// first source
    mem_t		src2	// second source
);

exmem_t ExMemCat3
(
    exmem_dest_t	*dest,	// kind of destination, if NULL then MALLOC()
    mem_t		sep,	// insert separators between sources with len>0
    mem_t		src1,	// first source
    mem_t		src2,	// second source
    mem_t		src3	// third source
);

//-----------------------------------------------------------------------------
// interface

// 'em' can be modified ignoring 'const'
void ResetExMem  ( const exmem_t *em );
void FreeExMem   ( const exmem_t *em );
void FreeExMemCM ( const exmem_t *em, CopyMode_t copy_mode ); // free on CPM_MOVE

exmem_t AllocExMemS
(
    cvp		src,		// NULL or source
    int		src_len,	// len of 'src'; if -1: use strlen(source)
    bool	try_circ,	// use circ-buffer, if result is small enough
    cvp		orig,		// NULL or data to compare
    int		orig_len	// len of 'orig'; if -1: use strlen(source)
);

static inline exmem_t AllocExMemM ( mem_t src, bool try_circ, mem_t orig )
	{ return AllocExMemS(src.ptr,src.len,try_circ,orig.ptr,orig.len); }

void AssignExMem  ( exmem_t *dest, const exmem_t *source, CopyMode_t copy_mode );
void AssignExMemS ( exmem_t *dest, cvp source, int slen,  CopyMode_t copy_mode );

static inline exmem_t CopyExMem ( const exmem_t *source, CopyMode_t copy_mode )
	{ exmem_t em = {{0}}; AssignExMem(&em,source,copy_mode); return em; }

static inline exmem_t CopyExMemS ( cvp source, int slen, CopyMode_t copy_mode )
	{ exmem_t em = {{0}}; AssignExMemS(&em,source,slen,copy_mode); return em; }

static inline exmem_t CopyExMemM ( const mem_t source, CopyMode_t copy_mode )
	{ exmem_t em = {{0}}; AssignExMemS(&em,source.ptr,source.len,copy_mode); return em; }

ccp PrintExMem ( const exmem_t * em ); // print to circ-buf

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct mem_list_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[mem_list_t]]

typedef struct mem_list_t
{
    mem_t	*list;		// List of mem_t objects.
				// Elements are always terminated with NULL.
				// Content is NULL or EmptyString or stored in 'buf'

    uint	used;		// Number of active 'list' elements
    uint	size;		// Number of alloced 'list' elements

    char	*buf;		// Alloced temp buffer
    uint	buf_used;	// Usage of 'buf'
    uint	buf_size;	// Size of 'buf'
}
mem_list_t;

//-----------------------------------------------------------------------------
// [[mem_list_mode_t]]

typedef enum mem_list_mode_t
{
    MEMLM_ALL,		// insert all
    MEMLM_IGN_NULL,	// ignore NULL
    MEMLM_IGN_EMPTY,	// ignore NULL and empty
    MEMLM_REPL_NULL,	// replace NULL by EmptyString
}
mem_list_mode_t;

//-----------------------------------------------------------------------------

static inline void InitializeMemList ( mem_list_t *ml )
	{ DASSERT(ml); memset(ml,0,sizeof(*ml)); }

void ResetMemList ( mem_list_t *ml );
void MoveMemList ( mem_list_t *dest, mem_list_t *src );

void NeedBufMemList ( mem_list_t *ml, uint need_size, uint extra_size );
void NeedElemMemList ( mem_list_t *ml, uint n_elem, uint need_size );

//-----------------------------------------------------------------------------

void InsertMemListN
(
    mem_list_t		*ml,		// valid destination mem_list
    int			pos,		// insert position => CheckIndex1()
    const mem_t		*src,		// source list
    uint		src_len,	// number of elements in source list
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
);

//-----------------------------------------------------------------------------

static inline void InsertMemList2
(
    mem_list_t		*ml,		// valid destination mem_list
    int			pos,		// insert position => CheckIndex1()
    const mem_list_t	*src,		// source mem_list
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
);

//-----------------------------------------------------------------------------

static inline void CatMemList2
(
    mem_list_t		*dest,		// valid destination mem_list
    const mem_list_t	*src1,		// source mem_list
    const mem_list_t	*src2,		// source mem_list
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
///////////////			PrintMode_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[PrintMode_t]]

typedef struct PrintMode_t
{
    FILE	*fout;		// NULL (stdout) or output file
    const ColorSet_t
		*cout;		// NULL (no colors) or color set for 'fout'

    FILE	*ferr;		// NULL or error file
    const ColorSet_t
		*cerr;		// NULL (no colors) or color set for 'ferr'

    FILE	*flog;		// if NULL: use 'ferr' or 'fout' as fallback
    const ColorSet_t
		*clog;		// NULL (no colors) or color set for 'flog'
				// if 'flog' is copied, use related 'col*'

    ccp		prefix;		// NULL (no prefix) or line prefix
    ccp		eol;		// NULL (LF) or end-of-line string
    int		indent;		// indention after prefix
    int		debug;		// debug level (ignored if <=0)

    int		mode;		// user defined print mode
    bool	compact;	// TRUE: print compact (e.g. no empty lines for readability)
}
PrintMode_t;

void SetupPrintMode ( PrintMode_t * pm );

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

uint SetupAllocInfo(void);

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
///////////////			basic string functions		///////////////
///////////////////////////////////////////////////////////////////////////////

extern const char EmptyQuote[];		// »""«		  Ignored by FreeString()
extern const char EmptyString[4];	// »«   + 4*NULL, Ignored by FreeString()
extern const char MinusString[4];	// »-«  + 3*NULL, Ignored by FreeString()

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
extern const char DoubleLine300_3[901];	// 300 * '═' (U+2550) + NULL

//-----------------------------------------------------------------------------

void SetupMultiProcessing(void);
void PrintSettingsDCLIB ( FILE *f, int indent );
void SetupProgname ( int argc, char ** argv,
			ccp tool_name, ccp tool_vers, ccp tool_title );

// return NULL or 'ProgInfo.progpath', calc GetProgramPath() once if needed
ccp ProgramPath(void);

// return NULL or 'ProgInfo.progdir', calc ProgramPath() once if needed
ccp ProgramDirectory(void);

// path0 can be a directory or a filename (->dir extracted)
void DefineLogDirectory ( ccp path0, bool force );

// get 'ProgInfo.logdir' without tailing '/' ("." as fall back)
ccp GetLogDirectory(void);

#ifdef __CYGWIN__
    ccp ProgramPathNoExt(void);
#else
    static inline ccp ProgramPathNoExt(void) { return ProgramPath(); }
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

// StringCopy*(), StringCat*()
//	RESULT: end of copied string pointing to a NULL character.
//	'src*' may be a NULL pointer.
// MAX_COPY define max number of bytes to copy.
//	It's only relevant if src is longer.
//	If MAX_COPY<0: ignore it.
// SRC_LEN is the length of src (or the number of bytes to copy)
//	If SRC_LEN<0: use strlen(src)

char * StringCopyS  ( char *buf, ssize_t buf_size, ccp src );
char * StringCopySM ( char *buf, ssize_t buf_size, ccp src, ssize_t max_copy );
char * StringCopySL ( char *buf, ssize_t buf_size, ccp src, ssize_t src_len );

static inline char * StringCopyE ( char * buf, ccp buf_end, ccp src )
	{ return StringCopyS(buf,buf_end-buf,src); }
static inline char * StringCopyEM ( char *buf, ccp buf_end, ccp src, ssize_t max_copy )
	{ return StringCopySM(buf,buf_end-buf,src,max_copy ); }
static inline char * StringCopyEL ( char *buf, ccp buf_end, ccp src, ssize_t src_len )
	{ return StringCopySL(buf,buf_end-buf,src,src_len ); }

// concat sources, *A(): use MALLOC()
char * StringCat2S ( char *buf, ssize_t buf_size, ccp src1, ccp src2 );
char * StringCat3S ( char *buf, ssize_t buf_size, ccp src1, ccp src2, ccp src3 );
char * StringCat2E ( char *buf, ccp buf_end, ccp src1, ccp src2 );
char * StringCat3E ( char *buf, ccp buf_end, ccp src1, ccp src2, ccp src3 );
char * StringCat2A ( ccp src1, ccp src2 );
char * StringCat3A ( ccp src1, ccp src2, ccp src3 );

// like above, but cat with separators
char * StringCatSep2S ( char *buf, ssize_t bufsize, ccp sep, ccp src1, ccp src2 );
char * StringCatSep3S ( char *buf, ssize_t bufsize, ccp sep, ccp src1, ccp src2, ccp src3 );
char * StringCatSep2E ( char *buf, ccp buf_end,     ccp sep, ccp src1, ccp src2 );
char * StringCatSep3E ( char *buf, ccp buf_end,     ccp sep, ccp src1, ccp src2, ccp src3 );
char * StringCatSep2A ( ccp sep, ccp src1, ccp src2 );
char * StringCatSep3A ( ccp sep, ccp src1, ccp src2, ccp src3 );

// center string
ccp StringCenterE ( char * buf, ccp buf_end, ccp src, int width );
static inline ccp StringCenterS ( char *buf, ssize_t bufsize, ccp src, int width )
	{ return StringCenterE(buf,buf+bufsize,src,width); }

static inline char * StringCopySMem ( char *buf, ssize_t bufsize, mem_t mem )
	{ return StringCopySM(buf,bufsize,mem.ptr,mem.len); }
static inline char * StringCopyEMem ( char *buf, ccp buf_end, mem_t mem )
	{ return StringCopyEM(buf,buf_end,mem.ptr,mem.len); }

char * StringLowerS ( char * buf, ssize_t bufsize, ccp src );
char * StringLowerE ( char * buf, ccp buf_end,     ccp src );
char * StringUpperS ( char * buf, ssize_t bufsize, ccp src );
char * StringUpperE ( char * buf, ccp buf_end,     ccp src );

char * MemLowerE    ( char * buf, ccp buf_end,     mem_t src );
char * MemLowerS    ( char * buf, ssize_t bufsize, mem_t src );
char * MemUpperE    ( char * buf, ccp buf_end,     mem_t src );
char * MemUpperS    ( char * buf, ssize_t bufsize, mem_t src );

// special handling for unsigned decimals
int StrNumCmp ( ccp a, ccp b );

// count the number of equal bytes and return a value of 0 to SIZE
uint CountEqual ( cvp m1, cvp m2, uint size );

// skip ESC & well known escape sequences (until noc-escape)
char * SkipEscapes  ( ccp str );
char * SkipEscapesE ( ccp str, ccp end );
char * SkipEscapesS ( ccp str, int size );
mem_t  SkipEscapesM ( mem_t mem );

// Concatenate path + path, return pointer to buf
// Return pointer to   buf  or  path1|path2|buf  or  alloced string
char *PathCatBufPP  ( char *buf, ssize_t bufsize, ccp path1, ccp path2 );
char *PathCatBufPPE ( char *buf, ssize_t bufsize, ccp path1, ccp path2, ccp ext );
ccp PathCatPP	    ( char *buf, ssize_t bufsize, ccp path1, ccp path2 );
//ccp PathCatPPE    ( char *buf, ssize_t bufsize, ccp path1, ccp path2, ccp ext );
ccp PathAllocPP	    ( ccp path1, ccp path2 );
ccp PathAllocPPE    ( ccp path1, ccp path2, ccp ext );

// inline wrapper
static inline ccp PathCatPPE ( char *buf, ssize_t bufsize, ccp path1, ccp path2, ccp ext )
	{ return PathCatBufPPE(buf,bufsize,path1,path2,ext); }

// like PathCatPP(), but ignores path2, if path1 is not empty and not a directory
ccp PathCatDirP ( char * buf, ssize_t bufsize, ccp path1, ccp path2 );

// Same as PathCatPP*(), but use 'base' as prefix for relative paths.
// If 'base' is NULL (but not empty), use getcwd() instead.
// PathCatBufBP() and PathCatBP() are special: path can be part of buf
char *PathCatBufBP   ( char *buf, ssize_t bufsize, ccp base, ccp path );
char *PathCatBufBPP  ( char *buf, ssize_t bufsize, ccp base, ccp path1, ccp path2 );
char *PathCatBufBPPE ( char *buf, ssize_t bufsize, ccp base, ccp path1, ccp path2, ccp ext );
ccp PathCatBP        ( char *buf, ssize_t bufsize, ccp base, ccp path );
ccp PathCatBPP       ( char *buf, ssize_t bufsize, ccp base, ccp path1, ccp path2 );
ccp PathCatBPPE      ( char *buf, ssize_t bufsize, ccp base, ccp path1, ccp path2, ccp ext );
ccp PathAllocBP      ( ccp base, ccp path );
ccp PathAllocBPP     ( ccp base, ccp path1, ccp path2 );
ccp PathAllocBPPE    ( ccp base, ccp path1, ccp path2, ccp ext );

char * PathCombine     ( char *temp_buf, uint buf_size, ccp path, ccp base_path );
char * PathCombineFast ( char *dest_buf, uint buf_size, ccp path, ccp base_path );

// Remove last extension and add a new one.
//	If ext==NULL or emtpy: remove only
//	'ext' may start with '.'. If not, a additional '.' is included
//	If 'path' is NULL or path==buf: Inplace job using 'buf' as source.
char * NewFileExtE ( char * buf, ccp buf_end,     ccp path, ccp ext );
char * NewFileExtS ( char * buf, ssize_t bufsize, ccp path, ccp ext );

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

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    circ buf			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[CircBuf]]

#define CIRC_BUF_MAX_ALLOC	0x0400  // request limit
#define CIRC_BUF_SIZE		0x4000  // internal buffer size

// returns true if 'ptr' points into circ buffer => ignored by FreeString()
bool IsCircBuf ( cvp ptr );

//-----------------------------------------------------------------------------

char * GetCircBuf
(
    // Never returns NULL, but always ALIGN(4)

    u32		buf_size	// wanted buffer size, caller must add 1 for NULL-term
				// if buf_size > CIRC_BUF_MAX_ALLOC:
				//  ==> ERROR0(ERR_OUT_OF_MEMORY)
);

static inline wchar_t * GetCircBufW
(
    // Never returns NULL, but always ALIGN(4)

    u32		buf_size	// wanted buffer size (wchar elements),
				// caller must add 1 for NULL-term
				// if final buf_size > CIRC_BUF_MAX_ALLOC:
				//  ==> ERROR0(ERR_OUT_OF_MEMORY)
)
{
    return (wchar_t*)GetCircBuf(buf_size*sizeof(wchar_t));
}

//-----------------------------------------------------------------------------

char * CopyCircBuf
(
    // Never returns NULL, but always ALIGN(4)

    cvp		data,		// source to copy
    u32		data_size	// see GetCircBuf()
);

char * CopyCircBuf0
(
    // Like CopyCircBuf(), but an additional char is alloced and set to NULL
    // Never returns NULL, but always ALIGN(4).

    cvp		data,		// source to copy
    u32		data_size	// see GetCircBuf()
);

char * PrintCircBuf
(
    // returns CopyCircBuf0() or EmptyString.

    ccp		format,		// format string for vsnprintf()
    ...				// arguments for 'format'
)
__attribute__ ((__format__(__printf__,1,2)));

char * AllocCircBuf
(
    // Never returns NULL. Call FreeString(RESULT) to free possible alloced memory.

    cvp		src,		// NULL or source
    int		src_len,	// len of 'src'; if -1: use strlen(source)
    bool	try_circ	// use circ-buffer, if result is small enough
);

char * AllocCircBuf0
(
    // Never returns NULL. Call FreeString(RESULT) to free possible alloced memory.
    // Like AllocCircBuf(), but an additional char is alloced and set to NULL

    cvp		src,		// NULL or source
    int		src_len,	// len of 'src'; if -1: use strlen(source)
    bool	try_circ	// use circ-buffer, if result is small enough
);

void ReleaseCircBuf
(
    ccp	    end_buf,		// pointer to end of previous alloced buffer
    uint    release_size	// number of bytes to give back from end
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get line by list		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetLineByListHelper ( const int *list, ccp beg, ccp mid, ccp end, ccp line );

static inline ccp GetTopLineByList ( const int *list )
{
    return GetLineByListHelper(list,"┌","┬","┐","─");
}

static inline ccp GetMidLineByList ( int *list )
{
    return GetLineByListHelper(list,"├","┼","┤","─");
}

static inline ccp GetBottomLineByList ( int *list )
{
    return GetLineByListHelper(list,"└","┴","┘","─");
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct IntMode_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[IntMode_t]]

typedef enum IntMode_t
{
	IMD_UNSET = 0,	// default, always 0

	//--- big endian modes

	IMD_BE0,	// default big endian
	IMD_BE1,
	IMD_BE2,
	IMD_BE3,
	IMD_BE4,
	IMD_BE5,
	IMD_BE6,
	IMD_BE7,
	IMD_BE8,

	//--- little endian modes

	IMD_LE0,	// default little endian
	IMD_LE1,
	IMD_LE2,
	IMD_LE3,
	IMD_LE4,
	IMD_LE5,
	IMD_LE6,
	IMD_LE7,
	IMD_LE8,

	//--- local endian modes

     #ifdef LITTLE_ENDIAN

	IMD_0 = IMD_LE0,
	IMD_1 = IMD_LE1,
	IMD_2 = IMD_LE2,
	IMD_3 = IMD_LE3,
	IMD_4 = IMD_LE4,
	IMD_5 = IMD_LE5,
	IMD_6 = IMD_LE6,
	IMD_7 = IMD_LE7,
	IMD_8 = IMD_LE8,

     #else

	IMD_0 = IMD_BE0,
	IMD_1 = IMD_BE1,
	IMD_2 = IMD_BE2,
	IMD_3 = IMD_BE3,
	IMD_4 = IMD_BE4,
	IMD_5 = IMD_BE5,
	IMD_6 = IMD_BE6,
	IMD_7 = IMD_BE7,
	IMD_8 = IMD_BE8,

     #endif
}
__attribute__ ((packed)) IntMode_t;

//-----------------------------------------------------------------------------

ccp GetIntModeName ( IntMode_t mode );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    lt/eq/gt			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[compare_operator_t]]

typedef enum compare_operator_t
{
    COP_LT	= 1,
    COP_EQ	= 2,
    COP_GT	= 4,

    COP_NE	= COP_LT | COP_GT,
    COP_LE	= COP_LT | COP_EQ,
    COP_GE	= COP_GT | COP_EQ,

    COP_FALSE	= 0,
    COP_TRUE	= COP_LT | COP_EQ | COP_GT,
}
__attribute__ ((packed)) compare_operator_t;

//-----------------------------------------------------------------------------

char * ScanCompareOp ( compare_operator_t *dest_op, ccp arg );
ccp GetCompareOpName ( compare_operator_t op );

static inline compare_operator_t NegateCompareOp ( compare_operator_t op )
	{ return op ^ COP_TRUE; }

bool CompareByOpStat ( int stat, compare_operator_t op );
bool CompareByOpINT  ( int a,    compare_operator_t op, int b );
bool CompareByOpUINT ( uint a,   compare_operator_t op, uint b );
bool CompareByOpI64  ( s64 a,    compare_operator_t op, s64 b );
bool CompareByOpU64  ( u64 a,    compare_operator_t op, u64 b );
bool CompareByOpFLT  ( float a,  compare_operator_t op, float b );
bool CompareByOpDBL  ( double a, compare_operator_t op, double b );

static inline bool CompareByOpS ( ccp a, compare_operator_t op, ccp b )
	{ return CompareByOpStat(strcmp(a,b),op); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			LOWER/AUTO/UPPER		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[LowerUpper_t]]

typedef enum LowerUpper_t
{
    LOUP_ERROR	= -99,
    LOUP_LOWER	=  -1,
    LOUP_AUTO	=   0,
    LOUP_UPPER	=   1,
}
LowerUpper_t;

extern const KeywordTab_t KeyTab_LOWER_AUTO_UPPER[];

int ScanKeywordLowerAutoUpper
(
    // returns one of LOUP_*

    ccp			arg,		// argument to scan
    int			on_empty,	// return this value on empty
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
);

ccp GetKeywordLowerAutoUpper ( LowerUpper_t value );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct FastBuf_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[FastBuf_t]]

typedef struct FastBuf_t
{
    char *buf;		// pointer to buffer
    char *ptr;		// pointer to first unused char (always space for 0-term)
    char *end;		// pointer to end of buffer := buf+size-1

    uint fast_buf_size;	// size of 'fast_buf+space'
    char fast_buf[4];	// first use buffer with minimal space
    char space[];	// additional space
}
FastBuf_t;

///////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------
//
// 4 examples for initialization:
//
//	FastBuf_t fb1;
//	InitializeFastBufAlloc(&fb1,1000);
//	printf("fb1: %s: %s\n", GetFastBufStatus(&fb1), fb1.buf );
//
//	FastBuf_t fb2;
//	InitializeFastBuf(&fb2,sizeof(fb2));
//	printf("fb2: %s: %s\n", GetFastBufStatus(&fb2), fb2.buf );
//
//	struct { FastBuf_t b; char space[2000]; } fb3;
//	InitializeFastBuf(&fb3,sizeof(fb3));
//	printf("fb3: %s: %s\n", GetFastBufStatus(&fb.b), fb3.b.buf );
//
//	char fb4x[2000];
//	FastBuf_t *fb4 = InitializeFastBuf(fb4x,sizeof(fb4x));
//	printf("fb4: %s: %s\n", GetFastBufStatus(fb4), fb4->buf );
//
//------------------------------------------------------------------

FastBuf_t * InitializeFastBuf ( cvp mem, uint size );
FastBuf_t * InitializeFastBufAlloc ( FastBuf_t *fb, uint size );

void ResetFastBuf	( FastBuf_t *fb );
static inline void ClearFastBuf ( FastBuf_t *fb )
	{ DASSERT(fb); fb->ptr = fb->buf; }

ccp GetFastBufStatus	( const FastBuf_t *fb );
int ReserveSpaceFastBuf ( FastBuf_t *fb, uint size );
char * GetSpaceFastBuf	( FastBuf_t *fb, uint size );

// if size<0: use strlen(source) instead
uint   InsertFastBuf	( FastBuf_t *fb, int index,   cvp source, int size );
char * AppendFastBuf	( FastBuf_t *fb,              cvp source, int size );
char * WriteFastBuf	( FastBuf_t *fb, uint offset, cvp source, int size );
uint   AssignFastBuf	( FastBuf_t *fb, cvp source,              int size );
uint   AlignFastBuf	( FastBuf_t *fb, uint align, u8 fill );
uint   DropFastBuf	( FastBuf_t *fb, int index,               int size );

void CopyFastBuf	( FastBuf_t *dest, const FastBuf_t *src );
void MoveFastBuf	( FastBuf_t *dest, FastBuf_t *src );

static inline void AppendMemFastBuf ( FastBuf_t *fb, const mem_t mem )
	{ AppendFastBuf(fb,mem.ptr,mem.len); }

static inline void AppendCharFastBuf ( FastBuf_t *fb, char ch )
{
    DASSERT(fb);
    GetSpaceFastBuf(fb,1)[0] = ch;
}

void AppendUTF8CharFastBuf ( FastBuf_t *fb, u32 code );
void AppendBE16FastBuf  ( FastBuf_t *fb, u16 num );
void AppendBE32FastBuf  ( FastBuf_t *fb, u32 num );
void AppendInt64FastBuf ( FastBuf_t *fb, u64 val, IntMode_t mode );

static inline void AppendU16FastBuf ( FastBuf_t *fb, u16 num )
	{ AppendFastBuf(fb,&num,sizeof(num)); }
static inline void AppendU32FastBuf ( FastBuf_t *fb, u32 num )
	{ AppendFastBuf(fb,&num,sizeof(num)); }
static inline void AppendU64FastBuf ( FastBuf_t *fb, u64 num )
	{ AppendFastBuf(fb,&num,sizeof(num)); }

static inline int GetFastBufLen ( const FastBuf_t *fb )
	{ DASSERT(fb); return fb->ptr - fb->buf; }

//-----------------------------------------------------------------------------
// The resulting strings of GetFastBufString() and GetFastBufMem0() are always
// terminated by an additional NULL byte.
// The resulting strings of GetFastBufMem() is not terminated by a NULL byte.
//-----------------------------------------------------------------------------

static inline char * GetFastBufString ( const FastBuf_t *fb )
{
    DASSERT(fb);
    *fb->ptr = 0;
    return fb->buf;
}

static inline mem_t GetFastBufMem0 ( const FastBuf_t *fb )
{
    DASSERT(fb);
    *fb->ptr = 0;
    mem_t mem = { fb->buf, fb->ptr - fb->buf };
    return mem;
}

static inline mem_t GetFastBufMem ( const FastBuf_t *fb )
{
    DASSERT(fb);
    mem_t mem = { fb->buf, fb->ptr - fb->buf };
    return mem;
}

//-----------------------------------------------------------------------------
// The allocated result is always terminated by an additional NULL byte.
// FastBuf_t itself is reset, similar to ResetFastBuf()

char * MoveFromFastBufString ( FastBuf_t *fb );
mem_t  MoveFromFastBufMem    ( FastBuf_t *fb );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 MatchPattern()			///////////////
///////////////////////////////////////////////////////////////////////////////

#define PATTERN_WILDCARDS "*#\t?[{"

bool HaveWildcards ( mem_t str );
char * FindFirstWildcard ( mem_t str );

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
				//   -2: no prefix found  (no rule found)
				//   -1: empty line (no rule found)
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
///////////////			PointerList_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[PointerList_t]]

typedef struct PointerList_t
{
    void **list;	// 0 terminated list, 1 longer than size
    uint used;		// num of used elements of 'list'
    uint size;		// num of alloced elements of 'list'
    int  grow;		// list grows by 1.5*size + grow (at least by 10)
}
PointerList_t;

//-----------------------------------------------------------------------------

static inline void InitializePointerMgr ( PointerList_t *pl, int grow )
	{ DASSERT(pl); memset(pl,0,sizeof(*pl)); pl->grow = grow; }

void ResetPointerMgr ( PointerList_t *pl );

// if n_elem<0: add until NULL pointer
void AddToPointerMgr ( PointerList_t *pl, const void *info );
void AddListToPointerMgr ( PointerList_t *pl, const void **list, int n_elem );

// reset==true: call ResetPointerMgr() after duplicating
void ** DupPointerMgr ( PointerList_t *pl, bool reset );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ArgManager_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ArgManager_t]]

typedef struct ArgManager_t
{
    LowerUpper_t force_case;	// change case?
    char	**argv;		// list of strings
    int		argc;		// number of used elements in 'argv'
    uint	size;		// >0: 'argv' is alloced to store # elements
				// last NULL element is not counted here
}
ArgManager_t;

//-----------------------------------------------------------------------------

void ResetArgManager  ( ArgManager_t *am );
void SetupArgManager  ( ArgManager_t *am, LowerUpper_t force_case,
					  int argc, char ** argv, bool clone );
void AttachArgManager ( ArgManager_t *am, int argc, char ** argv );
void CloneArgManager  ( ArgManager_t *am, int argc, char ** argv );

void AddSpaceArgManager ( ArgManager_t *am, int needed_space );

void CopyArgManager ( ArgManager_t *dest, const ArgManager_t *src );
void MoveArgManager ( ArgManager_t *dest,       ArgManager_t *src );

void PrepareEditArgManager ( ArgManager_t *am, int needed_space );

// return the pos behind last insert arguments
uint AppendArgManager    ( ArgManager_t *am,          ccp arg1, ccp arg2,     bool move_arg );
uint ReplaceArgManager   ( ArgManager_t *am, int pos, ccp arg1, ccp arg2,     bool move_arg );
uint InsertArgManager    ( ArgManager_t *am, int pos, ccp arg1, ccp arg2,     bool move_arg );
uint InsertListArgManager( ArgManager_t *am, int pos, int argc, char ** argv, bool move_arg );

// return the pos at removed argument
uint RemoveArgManager ( ArgManager_t *am, int pos, int count );

uint ScanSimpleArgManager ( ArgManager_t *am, ccp src );
uint ScanQuotedArgManager ( ArgManager_t *am, ccp src, bool is_utf8 );

// NULL if for both params accepted
bool CheckFilterArgManager ( const ArgManager_t *filter, ccp name );

void DumpArgManager ( FILE *f, int indent, const ArgManager_t *am, ccp title );

enumError ScanFileArgManager
(
    ArgManager_t	*am,		// valid arg-manager
    int			pos,		// insert position, relative to end if <0
    ccp			fname,		// filename to open
    int			silent,		// 0: print all error messages
					// 1: suppress file size warning
					// 2: suppress all error messages
    int			*n_added
);

//-----------------------------------------------------------------------------

typedef enum arg_expand_mode_t
{
    // AMXM = ArgManager eXpand Mode
    // P = parameters, S = short options, L = long options
    // 1 = single arg, 2 = double arg

    AMXM_P1	= 0x01,	    // expand "@file"
    AMXM_P2	= 0x02,	    // expand "@ file"
    AMXM_S1	= 0x04,	    // expand "-@file"
    AMXM_S2	= 0x08,	    // expand "-@ file"
    AMXM_L1	= 0x10,	    // expand "--@=file"  watch out '='
    AMXM_L2	= 0x20,	    // expand "--@ file"

    AMXM_PARAM	= AMXM_P1 | AMXM_P2,
    AMXM_SHORT	= AMXM_S1 | AMXM_S2,
    AMXM_LONG	= AMXM_L1 | AMXM_L2,

    AMXM_ALL	= AMXM_PARAM | AMXM_SHORT | AMXM_LONG,
}
arg_expand_mode_t;

enumError ExpandAtArgManager
(
    ArgManager_t	*am,		// valid arg-manager
    arg_expand_mode_t	expand_mode,	// objects to be replaced
    int			recursion,	// maximum recursion depth
    int			silent		// 0: print all error messages
					// 1: suppress file size warning
					// 2: suppress all error messages
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sizeof_info_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[sizeof_info_t]]

#define SIZEOF_INFO_TITLE(t)	{ -1, t },
#define SIZEOF_INFO_ENTRY(e)	{ sizeof(e), #e },
#define SIZEOF_INFO_SEP()	{ -2, 0 },
#define SIZEOF_INFO_TERM()	{ -9, 0 },

typedef struct sizeof_info_t
{
    int size;
    ccp name;
}
__attribute__ ((packed)) sizeof_info_t;

//-----------------------------------------------------------------------------

// basic lists
extern const sizeof_info_t sizeof_info_linux[];		// C and Linux types
extern const sizeof_info_t sizeof_info_dclib[];		// dcLib types

// default list of lists for GCMD_Sizeof(), terminated by 0
extern const sizeof_info_t *sizeof_info_default[];
extern PointerList_t SizeofInfoMgr;

static inline void AddDefaultToSizeofInfoMgr(void)
	{ AddListToPointerMgr(&SizeofInfoMgr,(cvpp)sizeof_info_default,-1); }

static inline void AddInfoToSizeofInfoMgr ( const sizeof_info_t *info )
	{ AddToPointerMgr(&SizeofInfoMgr,info); }

static inline void AddListToSizeofInfoMgr ( const sizeof_info_t **list )
	{ AddListToPointerMgr(&SizeofInfoMgr,(cvpp)list,-1); }

static inline const sizeof_info_t ** GetSizeofInfoMgrList(void)
	{ return SizeofInfoMgr.used ? (const sizeof_info_t**)SizeofInfoMgr.list : 0; }

///////////////////////////////////////////////////////////////////////////////
// [[sizeof_info_order_t]]

typedef enum sizeof_info_order_t
{
    SIZEOF_ORDER_NONE,		// don't sort output, but print categories
    SIZEOF_ORDER_NAME,		// Sort the output by name (case-insensitive)
				// and suppress output of categories.
    SIZEOF_ORDER_SIZE,		// Sort the output by size and suppress output of categories.
}
sizeof_info_order_t;

//-----------------------------------------------------------------------------

void ListSizeofInfo
(
    const PrintMode_t	*p_pm,		// NULL or print mode
    const sizeof_info_t	**si_list,	// list of list of entries
    ArgManager_t	*p_filter,	// NULL or filter arguments, LOUP_LOWER recommended
    sizeof_info_order_t	order		// kind of order
);

static inline void ListSizeofInfoMgr
(
    const PrintMode_t	*p_pm,		// NULL or print mode
    PointerList_t	*mgr,		// valid PointerList_t
    ArgManager_t	*p_filter,	// NULL or filter arguments, LOUP_LOWER recommended
    sizeof_info_order_t	order		// kind of order
)
{
     ListSizeofInfo(p_pm,(const sizeof_info_t**)mgr->list,p_filter,order);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		ContainerData_t, Container_t		///////////////
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
//				OVERVIEW
//-----------------------------------------------------------------------------
// Container_t * CreateContainer ( Container_t, protect, data,size, CopyMode_t )
// Container_t * InheritContainer ( Container_t cur, protect, Container_t parent, data,size )
// bool AssignContainer ( Container_t, protect, data,size, CopyMode_t )
// void ResetContainer ( Container_t )
// void DeleteContainer ( Container_t )
// void UnlinkContainerData ( Container_t )
// ContainerData_t * LinkContainerData ( Container_t )
// ContainerData_t * MoveContainerData ( Container_t )
// Container_t * CatchContainerData ( Container_t, int protect, ContainerData_t )
// Container_t * UseContainerData ( Container_t, int protect, Container_t )
//------------
// ContainerData_t DisjoinContainerData ( Container_t )
// void JoinContainerData ( Container_t, ContainerData_t )
// void FreeContainerData ( ContainerData_t )
//------------
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

    Container_t		*c,		// valid container, alloc one if NULL
    int			protect,	// initial value for protection
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
);

//-----------------------------------------------------------------------------

Container_t * InheritContainer
(
    // returns 'c' or the alloced container

    Container_t		*c,		// valid container, alloc one if NULL
    int			protect,	// initial value for protection
    const Container_t	*parent,	// NULL or valid parent container
    const void		*data,		// data to copy/move/link
    uint		size		// size of 'data'
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

    const Container_t	*c		// NULL or valid container
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

    Container_t		*c,		// valid container, alloc one if NULL
    int			protect,	// initial value for protection
    ContainerData_t	*cdata		// if not NULL: catch this
);

//-----------------------------------------------------------------------------

static inline Container_t * UseContainerData
(
    // returns 'c' or the alloced container

    Container_t		*c,		// valid container, alloc one if NULL
    int			protect,	// initial value for protection
    const Container_t	*src		// if not NULL: catch this
)
{
    return CatchContainerData(c,protect,LinkContainerData(src));
}

///////////////////////////////////////////////////////////////////////////////

ContainerData_t * DisjoinContainerData
(
    // Disjoin data container date without freeing it. Call either
    // JoinContainerData() or FreeContainerData() to finish operation.
    // Reference counters are not modified.
    // Return the data container or NULL

    Container_t		*c		// NULL or valid container
);

//-----------------------------------------------------------------------------

void JoinContainerData
(
    // Join a data container, that was diojoined by DisjoinContainerData()
    // Reference counters are not modified.

    Container_t		*c,		// if NULL: FreeContainerData()
    ContainerData_t	*cdata		// NULL or container-data to join
);

//-----------------------------------------------------------------------------

void FreeContainerData
(
    // Decrement the reference counter of an already disjoined data container
    // and free it if unused.

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
    Container_t		*c,		// valid container, alloc one if NULL
    int			new_protect	// new protection value
);

//-----------------------------------------------------------------------------

static inline int AddProtectContainer
(
    // returns 'c' new protection level
    Container_t		*c,		// valid container, alloc one if NULL
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
    ccp		eol		// End-Of-Line text. If NULL -> LF
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
    ccp			eol,		// End-Of-Line text. If NULL -> LF
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
    ccp			eol,		// End-Of-Line text. If NULL -> LF
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
    ccp			eol,		// End-Of-Line text. If NULL -> LF
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

extern const u16 TableCP1252_80[0x20];

///////////////////////////////////////////////////////////////////////////////
// [[CharMode_t]]

typedef enum CharMode_t // select encodig/decoding method
{
    CHMD_UTF8		= 0x01,  // UTF8 support enabled (compatible with legacy bool)
    CHMD_ESC		= 0x02,  // escape ESC by \e
    CHMD_PIPE		= 0x04,  // escape pipes '|' to '\!'
    CHMD_IF_REQUIRED	= 0x08,  // check for special chars and encode only if required
     CHMD__ALL		= 0x0f,

    CHMD__MODERN	= CHMD_UTF8 | CHMD_ESC,
    CHMD__PIPE8		= CHMD__MODERN | CHMD_PIPE | CHMD_IF_REQUIRED,
}
__attribute__ ((packed)) CharMode_t;

///////////////////////////////////////////////////////////////////////////////
// [[EncodeMode_t]]

typedef enum EncodeMode_t // select encodig/decoding method
{
    ENCODE_OFF,		// no encoding (always 0)
    ENCODE_STRING,	// ScanEscapedString(ANSI), byte mode
    ENCODE_UTF8,	// ScanEscapedString(UTF8), force UTF8 on decoding
    ENCODE_PIPE,	// analyse and use ENCODE_OFF|ENCODE_STRING, escape '|' too
    ENCODE_PIPE8,	// analyse and use ENCODE_OFF|ENCODE_UTF8, escape '|' too
    ENCODE_BASE64,	// Base64
    ENCODE_BASE64URL,	// Base64.URL (=) / decoder detects Standard + URL + STAR
    ENCODE_BASE64STAR,	// Base64.URL (*) / decoder detects Standard + URL + STAR
    ENCODE_BASE64XML,	// Base64 with XML name tokens / decoder detects Standard + XML (name+id)
    ENCODE_JSON,	// JSON string encoding
    ENCODE_HEX,		// Use hex-string without quotes

    ENCODE__N		// number of encoding modes
}
EncodeMode_t;

// [[doxygen]]
static inline bool NeedsQuotesByEncoding ( EncodeMode_t em )
	{ return em > ENCODE_OFF && em < ENCODE_BASE64 || em > ENCODE_BASE64XML; }

ccp GetEncodingName ( EncodeMode_t em );

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

uint GetEscapeLen
(
    // returns the extra size needed for escapes.
    // Add 'src_len' to get the escaped string size.
    // Add 'additionally 4 to get a good buffer size.

    ccp		source,		// NULL or string to print
    int		src_len,	// length of string. if -1, str is null terminated
    CharMode_t	char_mode,	// modes, bit field (CHMD_*)
    char	quote		// NULL or quotation char, that must be quoted
);

//-----------------------------------------------------------------------------

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
);

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
);

//-----------------------------------------------------------------------------
// escan string from tabels with pipe separator (UTF8 only)

static inline uint ScanEscapedStringPipe
(
    // returns the number of valid bytes in 'buf' (NULL term added but not counted)

    char	*buf,		// valid destination buffer, maybe source
    uint	buf_size,	// size of 'buf'
    ccp		source,		// string to scan
    uint	*scanned_len	// not NULL: Store number of scanned 'source' bytes here
)
{
    return ScanEscapedString(buf,buf_size,source,-1,true,
		source && *source == '"' ? '"' : '|', scanned_len );
}

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

//-----------------------------------------------------------------------------

mem_t DecodeBase64Circ
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    ccp		source,			// NULL or string to decode
    int		len,			// length of string. if -1, str is null terminated
    const char	decode64[256]		// decoding table; if NULL: use TableDecode64default
);

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
);

//-----------------------------------------------------------------------------

mem_t EncodeBase64Circ
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    const char	encode64[64+1]		// encoding table; if NULL: use TableEncode64default
);

//-----------------------------------------------------------------------------

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
);

///////////////////////////////////////////////////////////////////////////////

static inline uint GetEncodeBase64Len ( uint src_len )
	{ return 4 * (src_len+2) / 3; }

static inline uint GetEncodeBase64FillLen ( uint src_len )
	{ return (src_len+2) / 3 * 4; }

static inline uint GetDecodeBase64Len ( uint src_len )
	{ return 3 * src_len / 4; }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static inline uint DecodeJSON
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
    return ScanEscapedString(buf,buf_size,source,source_len,true,quote,scanned_len);
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
);

///////////////////////////////////////////////////////////////////////////////

uint EncodeJSON
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >2 and 2 bytes longer than needed
    const void	*source,		// NULL or data to encode
    int		source_len		// length of 'source'; if <0: use strlen(source)
);

//-----------------------------------------------------------------------------

mem_t EncodeJSONCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len		// length of 'source'; if <0: use strlen(source)
);

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
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint DecodeHex
(
    // returns the number of valid bytes in 'buf'

    char	*buf,		// valid destination buffer
    uint	buf_size,	// size of 'buf', >= 3
    ccp		source,		// NULL or string to decode
    int		source_len,	// length of 'source'. If -1, str is NULL terminated
    int		quote,		// -1:auto, 0:none, >0: quotation char
    uint	*scanned_len	// not NULL: Store number of scanned 'str' bytes here
);

//-----------------------------------------------------------------------------

mem_t DecodeHexCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    ccp		source,		// NULL or string to decode
    int		source_len,	// length of 'source'. If -1, str is NULL terminated
    int		quote		// -1:auto, 0:none, >0: quotation char
);

///////////////////////////////////////////////////////////////////////////////

uint EncodeHex
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of 'buf', >2 and 2 bytes longer than needed
    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    ccp		digits			// digits to use, eg. LoDigits[] (=fallback) or HiDigits[]
);

//-----------------------------------------------------------------------------

mem_t EncodeHexCirc
(
    // Returns a buffer alloced by GetCircBuf()
    // with valid pointer and null terminated.
    // If result is too large (>CIRC_BUF_MAX_ALLOC) then (0,0) is returned.

    const void	*source,		// NULL or data to encode
    int		source_len,		// length of 'source'; if <0: use strlen(source)
    ccp		digits			// digits to use, eg. LoDigits[] (=fallback) or HiDigits[]
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
    EncodeMode_t	emode,		// encoding mode
    uint		*scanned_len	// not NULL: Store number of scanned 'str' bytes here
);

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
);

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
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint EncodeByMode
(
    // returns the number of valid bytes in 'buf'. Result is NULL-terminated.

    char		*buf,		// valid destination buffer
    uint		buf_size,	// size of 'buf' >= 4
    ccp			source,		// string to encode
    int			slen,		// length of string. if -1, str is null terminated
    EncodeMode_t	emode		// encoding mode
);

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
);

///////////////////////////////////////////////////////////////////////////////

#define SJIS_TAB_MAPPING_BEG	0xeea0  // first code used for table mapping
#define SJIS_TAB_MAPPING_END	0xeecf  // last code used for table mapping
#define SJIS_INVALID_CODE	0xffff  // invalid code
#define SJIS_CACHE_SIZE		0xffff  // number of elements in cache

static inline bool IsValidShiftJIS ( uint code )
	{ return code < SJIS_TAB_MAPPING_BEG
	      || code > SJIS_TAB_MAPPING_END && code < SJIS_INVALID_CODE; }

int ScanShiftJISChar ( cucp * str );
int ScanShiftJISCharE ( cucp * str, cvp end );

void SetupGetShiftJISCache(void);
int GetShiftJISChar ( u32 code );

ccp GetShiftJISStatistics(void);

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
);

//-----------------------------------------------------------------------------

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
);

//-----------------------------------------------------------------------------

static inline char * EscapeStringM ( mem_t src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src.ptr,src.len,if_null,EmptyString,cm,0,false,0); }

static char * EscapeStringS ( ccp src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src,-1,if_null,EmptyString,cm,0,false,0); }

//-----------------------------------------------------------------------------

static inline char * EscapeStringCircM ( mem_t src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src.ptr,src.len,if_null,EmptyString,cm,0,true,0); }

static char * EscapeStringCircS ( ccp src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src,-1,if_null,EmptyString,cm,0,true,0); }

//-----------------------------------------------------------------------------

static inline char * QuoteStringM ( mem_t src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src.ptr,src.len,if_null,EmptyQuote,cm,true,false,0); }

static char * QuoteStringS ( ccp src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src,-1,if_null,EmptyQuote,cm,'"',false,0); }

static inline char * QuoteBashM ( mem_t src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src.ptr,src.len,if_null,EmptyQuote,cm,'$',false,0); }

static char * QuoteBashS ( ccp src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src,-1,if_null,EmptyQuote,cm,'$',false,0); }

//-----------------------------------------------------------------------------

static inline char * QuoteStringCircM ( mem_t src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src.ptr,src.len,if_null,EmptyQuote,cm,'"',true,0); }

static char * QuoteStringCircS ( ccp src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src,-1,if_null,EmptyQuote,cm,'"',true,0); }

static inline char * QuoteBashCircM ( mem_t src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src.ptr,src.len,if_null,EmptyQuote,cm,'$',true,0); }

static char * QuoteBashCircS ( ccp src, cvp if_null, CharMode_t cm )
	{ return EscapeString(src,-1,if_null,EmptyQuote,cm,'$',true,0); }


//-----------------------------------------------------------------------------
// Escape strings for tables with pipe separator (UTF8 only).
// Use FreeExMem(result) to free possible alloced result.

static inline exmem_t EscapeStringPipeM ( mem_t src )
	{ return EscapeStringEx(src.ptr,src.len,EmptyString,EmptyString,CHMD__PIPE8,'"',false); }

static exmem_t EscapeStringPipeS ( ccp src )
	{ return EscapeStringEx(src,-1,EmptyString,EmptyString,CHMD__PIPE8,'"',false); }

static inline exmem_t EscapeStringPipeCircM ( mem_t src )
	{ return EscapeStringEx(src.ptr,src.len,EmptyString,EmptyString,CHMD__PIPE8,'"',true); }

static exmem_t EscapeStringPipeCircS ( ccp src )
	{ return EscapeStringEx(src,-1,EmptyString,EmptyString,CHMD__PIPE8,'"',false); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct Escape_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[EscapeStat_t]]

typedef enum EscapeStat_t
{
    ESCST_NONE,	// nothing scanned (data incomplete)
    ESCST_CHAR,	// single char scanned (char behind '\e')
    ESCST_FE,	// FE scanned (single byte)
    ESCST_SS2,	// SS2 scanned ('N' + single byte), code := second char
    ESCST_SS3,	// SS3 scanned ('O' + single byte) , code := second char
    ESCST_CSI,	// 'Control Sequence Introduce' based string scanned, code := final char
    ESCST_OSC,	// 'Operating System Command' based string scanned

    ESCST__N
}
EscapeStat_t;

extern const char ecape_stat_name[ESCST__N+1][5];

///////////////////////////////////////////////////////////////////////////////
// [[Escape_t]]

typedef struct Escape_t
{
    EscapeStat_t status;	// one of EscapeStat_t
    int		scanned_len;	// number of scanned chars
    u32		code;		// depends on 'status', see enum EscapeStat_t
    mem_t	esc;		// if status==2: pointer+len to escape string;  else (0,0)
}
Escape_t;

//-----------------------------------------------------------------------------

// returns esc->status
EscapeStat_t CheckEscape ( Escape_t *esc, cvp source, cvp end, bool check_esc );

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

ccp GetKeywordError
(
    // return circ-buf

    const KeywordTab_t	* key_tab,	// NULL or pointer to command table
    ccp			key_arg,	// analyzed command
    int			key_stat,	// status of ScanKeyword()
    ccp			object		// NULL or object for error messages
					//	default= 'command'
);

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
    ColorSelect_t select,	// select color groups; if 0: use level
    uint	format		// output format => see PrintColorSet()
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			OFF/AUTO/ON/FORCE		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[OffOn_t]]

typedef enum OffOn_t
{
    OFFON_ERROR	= -99,
    OFFON_OFF	=  -1,
    OFFON_AUTO	=   0,
    OFFON_ON	=   1,
    OFFON_FORCE	=   2,
}
OffOn_t;

extern const KeywordTab_t KeyTab_OFF_AUTO_ON[];

int ScanKeywordOffAutoOnEx
(
    // returns one of OFFON_*

    const KeywordTab_t	*keytab,	// Keyword table. If NULL, then use KeyTab_OFF_AUTO_ON[]
    ccp			arg,		// argument to scan
    int			on_empty,	// return this value on empty
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
);

static inline int ScanKeywordOffAutoOn( ccp arg, int on_empty, uint max_num, ccp object )
	{ return ScanKeywordOffAutoOnEx(0,arg,on_empty,max_num,object); }

ccp GetKeywordOffAutoOn ( OffOn_t value );

//
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
// BINARY data starts with \1 and is stored as '->bin' and '->bin_len' and
// inludes SIZE and DATA. The BINARY.SIZE includes itself and is a 16-bit
// big-endian number. In '->bin[]', member 'be16:SIZE' is excluded,
// but '->bin[].ptr - 2' points to be16:SIZE.
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

    ccp		log_fname;	// not NULL: open log file
    uint	log_level;	// 0:off, 1:stat+err, 2:+cmd, 3:+bin+param, 4:+func

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

    uint	read_cmd_len;	// number of used bytes of 'command'
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
u64 MySeedByTime(void);

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
    DC_SFORM_CENTER	= 0x02,	// aligned and centered output
    DC_SFORM_NARROW	= 0x04,	// suppress space between number and unit
    DC_SFORM_UNIT1	= 0x08,	// limit unit to 1 character (space for bytes)
    DC_SFORM_INC	= 0x10,	// increment to next factor if no fraction is lost
    DC_SFORM_PLUS	= 0x20,	// on signed output: print PLUS sign for values >0
    DC_SFORM_DASH	= 0x40,	// on NULL: print only a dash (minus sign)

    DC_SFORM__MASK	= 0x7f,	// mask of above
    DC_SFORM__AUTO	= 0x80,	// hint for some functions to decide by themself

    DC_SFORM_TINY	= DC_SFORM_NARROW | DC_SFORM_UNIT1,
}
sizeform_mode_t;

///////////////////////////////////////////////////////////////////////////////

float  RoundF2bytes ( float f );
float  RoundF3bytes ( float f );
double RoundD6bytes ( double d );
double RoundD7bytes ( double d );

///////////////////////////////////////////////////////////////////////////////

// Parameters:
//	char	*buf,		// result buffer
//				// NULL: use a local circulary static buffer
//	size_t	buf_size,	// size of 'buf', ignored if buf==NULL
//	*	num,		// number to print
//	sizeform_mode_t mode	// any of DC_SFORM_ALIGN, DC_SFORM_DASH[not *D*()]

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
// [[range_opt_t]]

typedef enum range_opt_t
{
    RAOPT_COLON		= 0x01,  // allow ':' as from/to separator
    RAOPT_MINUS		= 0x02,  // allow '-' as from/to separator
    RAOPT_HASH		= 0x04,  // allow '#' as from/size separator
    RAOPT_COMMA		= 0x08,  // allow ',' as from/size separator
    RAOPT_NEG		= 0x10,  // accept negative numbers as index relative to max_value
    RAOPT_ASSUME_0	= 0x20,	 // allow ':-#,' as first char and assume 0 as first value
    RAOPT_F_FORCE	= 0x40,  // flag to avoid value 0 as default alias
    RAOPT__ALL		= 0x7f,  // all possible bits

    RAOPT__DEFAULT = RAOPT_COLON | RAOPT_HASH,
}
range_opt_t;

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
    double	max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
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
    u32		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
);

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
    u64		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
);

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
    double	max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
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
    u32		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
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
    u64		max_value,		// >0: max value for open ranges
    range_opt_t	opt			// options, if 0 then use RAOPT__DEFAULT
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
///////////////			time & timer			///////////////
///////////////////////////////////////////////////////////////////////////////

//--- conversion factors

#define SEC_PER_MIN	      60
#define SEC_PER_HOUR	    3600
#define SEC_PER_DAY	   86400
#define SEC_PER_WEEK	  604800
#define SEC_PER_MONTH	 2629746  // 365.2425 * SEC_PER_DAY / 12
#define SEC_PER_QUARTER	 7889238  // 365.2425 * SEC_PER_DAY /  4
#define SEC_PER_YEAR	31556952  // 365.2425 * SEC_PER_DAY

#define MSEC_PER_SEC	1000ll
#define MSEC_PER_MIN	 (MSEC_PER_SEC*SEC_PER_MIN)
#define MSEC_PER_HOUR	 (MSEC_PER_SEC*SEC_PER_HOUR)
#define MSEC_PER_DAY	 (MSEC_PER_SEC*SEC_PER_DAY)
#define MSEC_PER_WEEK	 (MSEC_PER_SEC*SEC_PER_WEEK)
#define MSEC_PER_MONTH	 (MSEC_PER_SEC*SEC_PER_MONTH)
#define MSEC_PER_QUARTER (MSEC_PER_SEC*SEC_PER_QUARTER)
#define MSEC_PER_YEAR	 (MSEC_PER_SEC*SEC_PER_YEAR)

#define USEC_PER_MSEC	   1000ll
#define USEC_PER_SEC	1000000ll
#define USEC_PER_MIN	 (USEC_PER_SEC*SEC_PER_MIN)
#define USEC_PER_HOUR	 (USEC_PER_SEC*SEC_PER_HOUR)
#define USEC_PER_DAY	 (USEC_PER_SEC*SEC_PER_DAY)
#define USEC_PER_WEEK	 (USEC_PER_SEC*SEC_PER_WEEK)
#define USEC_PER_MONTH	 (USEC_PER_SEC*SEC_PER_MONTH)
#define USEC_PER_QUARTER (USEC_PER_SEC*SEC_PER_QUARTER)
#define USEC_PER_YEAR	 (USEC_PER_SEC*SEC_PER_YEAR)

#define NSEC_PER_USEC	      1000ll
#define NSEC_PER_MSEC	   1000000ll
#define NSEC_PER_SEC	1000000000ll
#define NSEC_PER_MIN	 (NSEC_PER_SEC*SEC_PER_MIN)
#define NSEC_PER_HOUR	 (NSEC_PER_SEC*SEC_PER_HOUR)
#define NSEC_PER_DAY	 (NSEC_PER_SEC*SEC_PER_DAY)
#define NSEC_PER_WEEK	 (NSEC_PER_SEC*SEC_PER_WEEK)
#define NSEC_PER_MONTH	 (NSEC_PER_SEC*SEC_PER_MONTH)
#define NSEC_PER_QUARTER (NSEC_PER_SEC*SEC_PER_QUARTER)
#define NSEC_PER_YEAR	 (NSEC_PER_SEC*SEC_PER_YEAR)


//--- time types

typedef uint u_sec_t;	// unsigned type to store time as seconds
typedef int  s_sec_t;	//   signed type to store time as seconds
typedef u64  u_msec_t;	// unsigned type to store time as milliseconds
typedef s64  s_msec_t;	//   signed type to store time as milliseconds
typedef u64  u_usec_t;	// unsigned type to store time as microseconds
typedef s64  s_usec_t;	//   signed type to store time as microseconds
typedef u64  u_nsec_t;	// unsigned type to store time as nanoseconds
typedef s64  s_nsec_t;	//   signed type to store time as nanoseconds


//--- max values time types

#define S_SEC_MAX	S32_MAX
#define U_SEC_MAX	U32_MAX
#define S_MSEC_MAX	S64_MAX
#define U_MSEC_MAX	U64_MAX
#define S_USEC_MAX	S64_MAX
#define U_USEC_MAX	U64_MAX
#define S_NSEC_MAX	S64_MAX
#define U_NSEC_MAX	U64_MAX

//-----------------------------------------------------------------------------
// [[DayTime_t]]
typedef struct DayTime_t
{
    time_t	time;	// seconds since epoch, like time()
    int		day;	// days since epoch
    int		hour;	// hour of day
    int		min;	// minute of hour
    int		sec;	// second of minute
    int		usec;	// microsecond of second
    int		nsec;	// nanosecond of second
}
DayTime_t;

//--- only valid after call to SetupTimezone()

extern s64 timezone_adjust_sec;
extern s64 timezone_adjust_usec;
extern s64 timezone_adjust_nsec;
extern int timezone_adjust_isdst;

///////////////////////////////////////////////////////////////////////////////

extern ccp micro_second_char; // default = "u"

static inline void SetMicroSecondChar ( bool use_micro )
	{ micro_second_char = use_micro ? "µ" : "u"; }


///////////////////////////////////////////////////////////////////////////////

void SetupTimezone ( bool force );
int GetTimezoneAdjust ( time_t tim );

struct timeval	GetTimeOfDay ( bool localtime );
struct timespec	GetClockTime ( bool localtime );
DayTime_t	GetDayTime   ( bool localtime );
u_sec_t		GetTimeSec   ( bool localtime );
u_msec_t	GetTimeMSec  ( bool localtime );
u_usec_t	GetTimeUSec  ( bool localtime );
u_nsec_t	GetTimeNSec  ( bool localtime );

struct timespec	GetClockTimer(void);
u_sec_t  GetTimerSec(void);
u_msec_t GetTimerMSec(void);
u_usec_t GetTimerUSec(void);
u_nsec_t GetTimerNSec(void);

//-----------------------------------------------------------------------------

static inline u_sec_t Time2TimerSec ( u_sec_t tim, bool localtime )
	{ return tim - GetTimeSec(localtime) + GetTimerSec(); }

static inline u_msec_t Time2TimerMSec ( u_msec_t tim, bool localtime )
	{ return tim - GetTimeMSec(localtime) + GetTimerMSec(); }

static inline u_usec_t Time2TimerUSec ( u_usec_t tim, bool localtime )
	{ return tim - GetTimeUSec(localtime) + GetTimerUSec(); }

static inline u_nsec_t Time2TimerNSec ( u_nsec_t tim, bool localtime )
	{ return tim - GetTimeNSec(localtime) + GetTimerNSec(); }


static inline u_sec_t Timer2TimeSec ( u_sec_t tim, bool localtime )
	{ return tim - GetTimerSec() + GetTimeSec(localtime); }

static inline u_msec_t Timer2TimeMSec ( u_msec_t tim, bool localtime )
	{ return tim - GetTimerMSec() + GetTimeMSec(localtime); }

static inline u_usec_t Timer2TimeUSec ( u_usec_t tim, bool localtime )
	{ return tim - GetTimerUSec() + GetTimeUSec(localtime); }

static inline u_nsec_t Timer2TimeNSec ( u_nsec_t tim, bool localtime )
	{ return tim - GetTimerNSec() + GetTimeNSec(localtime); }

//-----------------------------------------------------------------------------

static inline u_sec_t  double2sec  ( double d ) { return d>0.0 ? (u_msec_t)trunc(     d+0.5 ) : 0; }
static inline u_msec_t double2msec ( double d ) { return d>0.0 ? (u_msec_t)trunc( 1e3*d+0.5 ) : 0; }
static inline u_usec_t double2usec ( double d ) { return d>0.0 ? (u_usec_t)trunc( 1e6*d+0.5 ) : 0; }
static inline u_nsec_t double2nsec ( double d ) { return d>0.0 ? (u_nsec_t)trunc( 1e9*d+0.5 ) : 0; }

static inline s_sec_t  double2ssec  ( double d ) { return (s_sec_t)  floor(     d+0.5 ); }
static inline s_msec_t double2smsec ( double d ) { return (s_msec_t) floor( 1e3*d+0.5 ); }
static inline s_usec_t double2susec ( double d ) { return (s_usec_t) floor( 1e6*d+0.5 ); }
static inline s_nsec_t double2snsec ( double d ) { return (s_nsec_t) floor( 1e9*d+0.5 ); }

static inline double sec2double  ( s_sec_t  num ) { return num; }
static inline double msec2double ( s_msec_t num ) { return num * 1e-3; }
static inline double usec2double ( s_usec_t num ) { return num * 1e-6; }
static inline double nsec2double ( s_nsec_t num ) { return num * 1e-9; }

//--- another epoch: Monday, 2001-01-01

#define EPOCH_2001_SEC  0x3a4fc880
#define EPOCH_2001_MSEC (EPOCH_2001_SEC*MSEC_PER_SEC)
#define EPOCH_2001_USEC (EPOCH_2001_SEC*USEC_PER_SEC)
#define EPOCH_2001_NSEC (EPOCH_2001_SEC*NSEC_PER_SEC)

///////////////////////////////////////////////////////////////////////////////
// [[CurrentTime_t]]

typedef struct CurrentTime_t
{
    u_sec_t	sec;
    u_msec_t	msec;
    u_usec_t	usec;
}
CurrentTime_t;

//-----------------------------------------------------------------------------

CurrentTime_t GetCurrentTime ( bool localtime );

extern CurrentTime_t current_time;
static inline void UpdateCurrentTime(void)
	{ current_time = GetCurrentTime(false); }

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// PrintRefTime*() return temporary buffer by GetCircBuf(), field width 10
// Formats:
//	±10h: "  %k:%M:%S"
//	±12d: " %e. %k:%M"
//	   *: "%F"

ccp PrintRefTime
(

    u_sec_t		tim,		// seconds since epoch; 0 is replaced by time()
    u_sec_t		*ref_tim	// NULL or ref_tim; if *ref_tim==0: write current time
					// it is used to select format
);

ccp PrintRefTimeUTC
(

    u_sec_t		tim,		// seconds since epoch; 0 is replaced by time()
    u_sec_t		*ref_tim	// NULL or ref_tim; if *ref_tim==0: write current time
					// it is used to select format
);

//-----------------------------------------------------------------------------

ccp PrintTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    u_sec_t		tim		// seconds since epoch; 0 is replaced by time()
);

ccp PrintTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    u_sec_t		tim		// seconds since epoch; 0 is replaced by time()
);

//-----------------------------------------------------------------------------

ccp PrintNSecByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'nsec'
    time_t		tim,		// seconds since epoch
    uint		nsec		// nanosecond of second
);

ccp PrintNSecByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'nsec'
    time_t		tim,		// seconds since epoch
    uint		nsec		// nanosecond of second
);

ccp PrintNTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_nsec_t		ntime
);

ccp PrintNTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_nsec_t		ntime
);

//-----------------------------------------------------------------------------

static inline ccp PrintUSecByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    time_t		tim,		// seconds since epoch
    uint		usec		// microsecond of second
)
{
    return PrintNSecByFormat(format,tim,usec*NSEC_PER_USEC);
}

static inline ccp PrintUSecByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    time_t		tim,		// seconds since epoch
    uint		usec		// microsecond of second
)
{
    return PrintNSecByFormatUTC(format,tim,usec*NSEC_PER_USEC);
}

ccp PrintUTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_usec_t		utime
);

ccp PrintUTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_usec_t		utime
);

//-----------------------------------------------------------------------------

static inline ccp PrintMSecByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    time_t		tim,		// seconds since epoch
    uint		msec		// millisecond of second
)
{
    return PrintNSecByFormat(format,tim,msec*NSEC_PER_MSEC);
}

static inline ccp PrintMSecByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    time_t		tim,		// seconds since epoch
    uint		msec		// millisecond of second
)
{
    return PrintNSecByFormatUTC(format,tim,msec*NSEC_PER_MSEC);
}

ccp PrintMTimeByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_msec_t		mtime
);

ccp PrintMTimeByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
					// 1-9 '@' in row replaced by digits of 'usec'
    u_msec_t		mtime
);

//-----------------------------------------------------------------------------

ccp PrintTimevalByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timeval *tv		// time to print, if NULL use gettimeofday()
);

ccp PrintTimevalByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timeval *tv		// time to print, if NULL use gettimeofday()
);

//-----------------------------------------------------------------------------

ccp PrintTimespecByFormat
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timespec *ts		// time to print, if NULL use GetClockTime(false)
);

ccp PrintTimespecByFormatUTC
(
    // returns temporary buffer by GetCircBuf();

    ccp			format,		// format string for strftime()
    const struct timespec *ts		// time to print, if NULL use GetClockTime(false)
);

///////////////////////////////////////////////////////////////////////////////

ccp PrintTimeSec
(
    char		* buf,		// result buffer (>19 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec		// seconds (=time) to print
);

ccp PrintTimeMSec
(
    char		* buf,		// result buffer (>23 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// milliseconds to print
    uint		fraction	// number of digits (0-3) to print as fraction
);

ccp PrintTimeUSec
(
    char		* buf,		// result buffer (>26 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    uint		fraction	// number of digits (0-6) to print as fraction
);

ccp PrintTimeNSec
(
    char		* buf,		// result buffer (>26 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_nsec_t		nsec,		// nanoseconds to print
    uint		fraction	// number of digits (0-9) to print as fraction
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
    uint		fraction	// number of digits (0-6) to print as fraction
);

ccp PrintTimerNSec
(
    char		* buf,		// result buffer (>16 bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_nsec_t		nsec,		// nanoseconds to print
    uint		fraction	// number of digits (0-9) to print as fraction
);

//-----------------------------------------------------------------------------

ccp PrintTimer3 // helper function
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction,
					// otherwise suppress ms/us output
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
);

static inline ccp PrintTimerSec3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec,		// seconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    return PrintTimer3(buf,buf_size,sec,-1,mode);
}

static inline ccp PrintTimerMSec3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    return PrintTimer3(buf,buf_size,msec/1000,msec%1000*1000,mode);
}

static inline ccp PrintTimerUSec3
(
    char		* buf,		// result buffer (>3 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    return PrintTimer3(buf,buf_size,usec/1000000,usec%1000000,mode);
}

//-----------------------------------------------------------------------------

ccp PrintTimer4 // helper function
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			usec,		// 0...999999: usec fraction,
					// otherwise suppress ms/us output
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
);

static inline ccp PrintTimerSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec,		// seconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    return PrintTimer4(buf,buf_size,sec,-1,mode);
}

static inline ccp PrintTimerMSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    return PrintTimer4(buf,buf_size,msec/MSEC_PER_SEC,msec%MSEC_PER_SEC*USEC_PER_SEC,mode);
}

static inline ccp PrintTimerUSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    return PrintTimer4(buf,buf_size,usec/USEC_PER_SEC,usec%USEC_PER_SEC,mode);
}

static inline ccp PrintTimerNSec4
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_nsec_t		nsec,		// nanoseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_DASH
)
{
    const u_usec_t usec = nsec / NSEC_PER_USEC;
    return PrintTimer4(buf,buf_size,usec/USEC_PER_SEC,usec%USEC_PER_SEC,mode);
}

//-----------------------------------------------------------------------------

ccp PrintTimer6N // helper function
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u64			sec,		// seconds to print
    int			nsec,		// 0...999999999: nsec fraction,
					//    otherwise suppress ms/us/ns output
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_CENTER | DC_SFORM_DASH
);

static inline ccp PrintTimerSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_sec_t		sec,		// seconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_CENTER | DC_SFORM_DASH
)
{
    return PrintTimer6N(buf,buf_size,sec,-1,mode);
}

static inline ccp PrintTimerMSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_msec_t		msec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_CENTER | DC_SFORM_DASH
)
{
    return PrintTimer6N( buf, buf_size,
		msec/MSEC_PER_SEC, msec%MSEC_PER_SEC*NSEC_PER_MSEC, mode );
}

static inline ccp PrintTimerUSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_CENTER | DC_SFORM_DASH
)
{
    return PrintTimer6N( buf, buf_size,
		usec/USEC_PER_SEC, usec%USEC_PER_SEC*NSEC_PER_USEC, mode );
}

static inline ccp PrintTimerNSec6
(
    char		* buf,		// result buffer (>6 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u_nsec_t		nsec,		// nanoseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_CENTER | DC_SFORM_DASH
)
{
    return PrintTimer6N( buf, buf_size,
		nsec/NSEC_PER_SEC, nsec%NSEC_PER_SEC, mode );
}

//-----------------------------------------------------------------------------

ccp PrintTimerUSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
);

static inline ccp PrintTimerNSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_nsec_t		nsec,		// nanoseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    return PrintTimerUSec4s(buf,buf_size,nsec/NSEC_PER_USEC,mode);
}

static inline ccp PrintTimerMSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_msec_t		msec,		// milliseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    return PrintTimerUSec4s(buf,buf_size,msec*USEC_PER_MSEC,mode);
}

static inline ccp PrintTimerSec4s
(
    char		* buf,		// result buffer (>4 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    int			sec,		// seconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    return PrintTimerUSec4s(buf,buf_size,1000000ll*sec,mode);
}

//-----------------------------------------------------------------------------

ccp PrintTimerNSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_nsec_t		nsec,		// nanoseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
);

ccp PrintTimerUSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_usec_t		usec,		// microseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
);

static inline ccp PrintTimerMSec7s
(
    char		* buf,		// result buffer (>7 bytes)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    s_msec_t		msec,		// milliseconds to print
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
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
    sizeform_mode_t	mode		// DC_SFORM_ALIGN | DC_SFORM_PLUS | DC_SFORM_DASH
)
{
    return PrintTimerUSec7s(buf,buf_size,1000000ll*sec,mode);
}

//-----------------------------------------------------------------------------

ccp PrintAge3Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
);

ccp PrintAge4Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
);

ccp PrintAge6Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
);

ccp PrintAge7Sec
(
    u_sec_t		now,		// NULL or current time by GetTimeSec(false)
    u_sec_t		time		// time to print
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
    SDUMD_ALLOW_CAPITALS	= 0x08,  // allow upper case letters

    SDUMD_ALLOW_LIST		= 0x10,  // allow lists like '9h12m'
     SDUMD_ALLOW_SPACE_SEP	= 0x20,  //  lists: allow spaces as list separator
     SDUMD_ALLOW_PLUS_SEP	= 0x40,  //  lists: allow '+' as list separator
     SDUMD_ALLOW_MINUS_SEP	= 0x80,  //  lists: allow '-' as list separator

    SDUMD_M_DEFAULT		= 0xf7,
    SDUMD_M_ALL			= 0xff,
}
ScanDuration_t;

//-----------------------------------------------------------------------------

char * ScanDuration
(
    // Returns a pointer to the the first not used character.
    // On error, it is 'source' and '*seconds' is set to 0.0.

    double		*seconds,	// not NULL: store result (number of seconds)
    ccp			source,		// source text
    double		default_factor,	// default factor if no SI unit found
    ScanDuration_t	mode		// flags
);

double GetDurationFactor ( char ch );

char * ScanSIFactor
(
    // returns end of scanned string

    double	*num,			// not NULL: store result
    ccp		source,			// source text
    double	default_factor		// return this if no factor found
);

//-----------------------------------------------------------------------------

// simple interfaces to ScanDuration(), SDUMD_M_DEFAULT is used as mode
double str2duration ( ccp src, char **endptr, double default_factor );


static inline u_sec_t str2sec ( ccp src, char **endptr )
	{ return double2sec(str2duration(src,endptr,1.0)); }

static inline u_msec_t str2msec ( ccp src, char **endptr )
	{ return double2msec(str2duration(src,endptr,1.0)); }

static inline u_usec_t str2usec ( ccp src, char **endptr )
	{ return double2usec(str2duration(src,endptr,1.0)); }

static inline u_nsec_t str2nsec ( ccp src, char **endptr )
	{ return double2nsec(str2duration(src,endptr,1.0)); }


static inline s_sec_t str2ssec ( ccp src, char **endptr )
	{ return double2ssec(str2duration(src,endptr,1.0)); }

static inline s_msec_t str2smsec ( ccp src, char **endptr )
	{ return double2smsec(str2duration(src,endptr,1.0)); }

static inline s_usec_t str2susec ( ccp src, char **endptr )
	{ return double2susec(str2duration(src,endptr,1.0)); }

static inline s_nsec_t str2snsec ( ccp src, char **endptr )
	{ return double2snsec(str2duration(src,endptr,1.0)); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Signals & Sleep			///////////////
///////////////////////////////////////////////////////////////////////////////

extern volatile int SIGINT_level;
extern volatile int SIGHUP_level;
extern volatile int SIGALRM_level;
extern volatile int SIGCHLD_level;
extern volatile int SIGPIPE_level;
extern volatile int SIGUSR1_level;
extern volatile int SIGUSR2_level;

// no timout for SIGPIPE
extern u_sec_t SIGINT_sec;
extern u_sec_t SIGHUP_sec;
extern u_sec_t SIGALRM_sec;
extern u_sec_t SIGCHLD_sec;
extern u_sec_t SIGUSR1_sec;
extern u_sec_t SIGUSR2_sec;

extern int  SIGINT_level_max;
extern bool ignore_SIGINT;
extern bool ignore_SIGHUP;

extern int  redirect_signal_pid;
extern LogFile_t log_signal_file;


extern volatile int SIGINT_level;
extern volatile int SIGHUP_level;
extern volatile int SIGALRM_level;
extern volatile int SIGPIPE_level;

extern int  SIGINT_level_max;
extern bool ignore_SIGINT;
extern bool ignore_SIGHUP;

void SetupSignalHandler ( int max_sigint_level, FILE *log_file );

//-----------------------------------------------------------------------------

int SleepNSec ( s_nsec_t nsec );

static inline int SleepUSec ( s_usec_t usec )
	{ return SleepNSec( usec * NSEC_PER_USEC ); }

static inline int SleepMSec ( s_msec_t msec )
	{ return SleepNSec( msec * NSEC_PER_MSEC ); }

static inline int SleepSec ( s_sec_t sec )
	{ return SleepNSec( sec * NSEC_PER_SEC ); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan date & time		///////////////
///////////////////////////////////////////////////////////////////////////////

// ScanInterval*() is old ==> try ScanDuration()

//-----------------------------------------------------------------------------
// new interface : all functions return first not scanned char

char * ScanIntervalTS
(
    struct timespec *res_ts,	// not NULL: store result here
    ccp		source		// source text
);

char * ScanIntervalUSec
(
    u_usec_t	*res_usec,	// not NULL: store total microseconds
    ccp		source		// source text
);


char * ScanIntervalNSec
(
    u_nsec_t	*res_nsec,	// not NULL: store total nanoseconds
    ccp		source		// source text
);

//-----------------------------------------------------------------------------
// legacy interface : all functions return first not scanned char

char * ScanInterval
(
    time_t	*res_time,	// not NULL: store seconds
    u32		*res_usec,	// not NULL: store microseconds of second
    ccp		source		// source text
);

char * ScanInterval64
(
    u64		*res_usec,	// not NULL: store total microseconds
    ccp		source		// source text
);

///////////////////////////////////////////////////////////////////////////////

char * ScanDateTime
(
    time_t	*res_time,	// not NULL: store seconds
    u32		*res_usec,	// not NULL: store microseconds of second
    ccp		source,		// source text
    bool	allow_delta	// true: allow +|- interval
);

///////////////////////////////////////////////////////////////////////////////

char * ScanDateTime64
(
    u64		*res_usec,	// not NULL: store total microseconds
    ccp		source,		// source text
    bool	allow_delta	// true: allow +|- interval
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Since 2001			///////////////
///////////////////////////////////////////////////////////////////////////////

// Values for day and week are exact.
// Values for month, quarter and year are based on 365.2425 days/year.

//-----------------------------------------------------------------------
// name		full/13		short/11	alt/10		tiny/8
//-----------------------------------------------------------------------
// hour		YYYY-MM-DD HH	YY-MM-DD HH	DD. HH:xx	MM-DD HH
// day		YYYY-MM-DD	<<		<<		YY-MM-DD
// week		YYYYwWW		<<		<<		<<
// month	YYYY-MM		<<		<<		<<
// quarter	YYYYqQ		<<		<<		<<
// year		YYYY		<<		<<		<<
//-----------------------------------------------------------------------

enum
{
    S2001_FW_FULL	= 13, ///< Standard-Text mit vollem Jahr (maximal 13 Zeichen).
    S2001_FW_SHORT	= 11, ///< Verkürzte Stundenangabe (maximal 11 Zeichen).
    S2001_FW_ALT	= 10, ///< Alternative Stundenangabe mit »:xx« (maximal 10 Zeichen).
    S2001_FW_TINY	=  8, ///< Verkürzte Stunden- und Wochenangabe (maximal 8 Zeichen).

    S2001_DUR_HOUR	=     3600, ///< Dauer einer Stunde in Sekunden.
    S2001_DUR_DAY	=    86400, ///< Dauer eines Tages in Sekunden.
    S2001_DUR_WEEK	=   604800, ///< Dauer einer Woche in Sekunden.
    S2001_DUR_MONTH	=  2629746, ///< Dauer eines Monats in Sekunden (365.2425/12 Tage).
    S2001_DUR_QUARTER	=  7889238, ///< Dauer eines Quartals in Sekunden (365.2425/4 Tage).
    S2001_DUR_YEAR	= 31556952, ///< Dauer eines Jahres in Sekunden (365.2425 Tage).
};

///////////////////////////////////////////////////////////////////////////
// hour support

static inline int time2hour ( s64 time )
	{ return time / 3600 - 271752; }

static inline double time2hourF ( s64 time )
	{ return time / 3600.0 - 271752; }

static inline s64 hour2time ( int hour )
	{ return ( hour + 271752 ) * 3600; }

static inline int hour2duration ( int hour )
	{ return S2001_DUR_HOUR; }

ccp hour2text
(
	int hour,	// hour to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use '-'
	ccp sep_dt	// NULL or separator of between date and time
			// if NULL: use ' '
);

///////////////////////////////////////////////////////////////////////////
// day support

static inline int time2day ( s64 time )
	{ return time / 86400 - 11323; }

static inline double time2dayF ( s64 time )
	{ return time / 86400.0 - 11323; }

static inline s64 day2time ( int day )
	{ return ( day + 11323 ) * 86400; }

static inline int day2duration ( int day )
	{ return S2001_DUR_DAY; }

ccp day2text
(
	int day,	// day to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use '-'
	ccp sep_dt	// ignored
);

///////////////////////////////////////////////////////////////////////////
// week support

static inline int time2week ( s64 time )
	{ return ( time - 978307200 ) / 604800; }

static inline double time2weekF ( s64 time )
	{ return ( time - 978307200 ) / 604800.0; }

static inline s64 week2time ( int week )
	{ return week * 604800 + 978307200; }

static inline int week2duration ( int week )
	{ return S2001_DUR_WEEK; }

ccp week2text
(
	int week,	// week to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use 'w'
	ccp sep_dt	// ignored
);

///////////////////////////////////////////////////////////////////////////
// month support

int time2month ( time_t tim );
time_t month2time ( int month );

static inline int month2duration ( int month )
	{ return month2time(month+1) - month2time(month); }

ccp month2text
(
	int month,	// month to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use '-'
	ccp sep_dt	// ignored
);

///////////////////////////////////////////////////////////////////////////
// quarter support

int time2quarter ( time_t tim );
time_t quarter2time ( int quarter );

static inline int quarter2duration ( int quarter )
	{ return quarter2time(quarter+1) - quarter2time(quarter); }

ccp quarter2text
(
	int quarter,	// quarter to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// NULL or separator within date
			// if NULL: use 'q'
	ccp sep_dt	// ignored
);

///////////////////////////////////////////////////////////////////////////
// year support

int time2year ( time_t tim );
time_t year2time ( int year );

static inline int year2duration ( int year )
	{ return year2time(year+1) - year2time(year); }

static inline ccp year2text
(
	int year,	// year to print
	int fw,		// wanted field width assuming separators are 1 char
	ccp sep_date,	// ignored
	ccp sep_dt	// ignored
)
{
    return PrintCircBuf("%04d",year);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			usage counter			///////////////
///////////////////////////////////////////////////////////////////////////////

#if IS_DEVELOP
  #define TEST_USAGE_COUNT 0
  #define USAGE_COUNT_ENABLED 0
#else
  #define TEST_USAGE_COUNT 0
  #define USAGE_COUNT_ENABLED 0
#endif

#if TEST_USAGE_COUNT
  #define USAGE_COUNT_ENTRIES	11	// number of entries
#else
  #define USAGE_COUNT_ENTRIES	30	// number of entries
#endif

#define USAGE_COUNT_FIRST_ADD	 8	// first entry where values wil be added
#define USAGE_COUNT_NEXT_MIN(v)	(15*(v)/8)

//-----------------------------------------------------------------------------
//  [[UsageParam_t]]

typedef struct UsageParam_t
{
    uint	used;			// number of used entries
    bool	enabled;		// TRUE: statistics are enabled
    bool	is_active;		// TRUE: counter is active (set by framework)

    //--- not saved/restored

    bool	dirty;			// TRUE: record needs an update @ 'update_nsec'
    u_nsec_t	update_nsec;		// next update, based on GetTimerNSec()
    u_nsec_t	force_update_nsec;	// force next update, based on GetTimerNSec()
}
UsageParam_t;

//-----------------------------------------------------------------------------
// [[UsageCountEntry_t]]

typedef struct UsageCountEntry_t
{
    u64		count;		// counter
    u_nsec_t	elapsed_nsec;	// elapsed time, by GetTimerNSec()
    u_sec_t	time_sec;	// time of record, by GetTimeSec(false)
    float	top1s;		// = 100.0*(utime+stime)/elapsed for 1s
    float	top5s;		// = 100.0*(utime+stime)/elapsed for 5s
}
__attribute__ ((packed)) UsageCountEntry_t;

//-----------------------------------------------------------------------------
//  [[UsageCount_t]]

typedef struct UsageCount_t
{
    UsageParam_t	par;				// base parameters
    UsageCountEntry_t	ref;				// reference with absolute values
 #if TEST_USAGE_COUNT
    UsageCountEntry_t	entry[USAGE_COUNT_ENTRIES+1];	// list of entries with delta values
 #else
    UsageCountEntry_t	entry[USAGE_COUNT_ENTRIES];	// list of entries with delta values
 #endif
}
UsageCount_t;

//-----------------------------------------------------------------------------

extern int usage_count_enabled;

void EnableUsageCount(void);
static inline void DisableUsageCount(void) { usage_count_enabled--; }

//-----------------------------------------------------------------------------

// n_rec<0: clear from end, n_rec>=0: clear from index
void ClearUsageCount ( UsageCount_t *uc, int count );

// NOW_NSEC: 0 or GetTimerNSec(), but not GetTime*()
void UpdateUsageCount ( UsageCount_t *uc, u_nsec_t now_nsec );
void UpdateUsageCountIncrement ( UsageCount_t *uc );
void UpdateUsageCountAdd ( UsageCount_t *uc, int add );
static inline void UsageCountIncrement ( UsageCount_t *uc )
	{ UpdateUsageCountAdd(uc,1); }

// stat about last 'nsec' nanoseconds
UsageCountEntry_t GetUsageCount ( const UsageCount_t *uc, s_nsec_t nsec );

void AddUsageCountEntry ( UsageCountEntry_t *dest, const UsageCountEntry_t *add );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wait counter			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[UsageDurationEntry_t]]

typedef struct UsageDurationEntry_t
{
    u_nsec_t	elapsed_nsec;	// elapsed time, by GetTimerNSec()
    u_nsec_t	total_nsec;	// total waiting time
    u_nsec_t	top_nsec;	// top waiting time
    u64		count;		// number of events
    u_sec_t	time_sec;	// time of record, by GetTimeSec(false)
    float	top_cnt_1s;	// top number of counts in 1 sec
    float	top_cnt_5s;	// top number of counts in 5 sec
}
__attribute__ ((packed)) UsageDurationEntry_t;

//-----------------------------------------------------------------------------
//  [[UsageDuration_t]]

typedef struct UsageDuration_t
{
    UsageParam_t	 par;				// base parameters
    UsageDurationEntry_t ref;				// reference with absolute values
 #if TEST_USAGE_COUNT
    UsageDurationEntry_t entry[USAGE_COUNT_ENTRIES+1];	// list of entries with delta values
 #else
    UsageDurationEntry_t entry[USAGE_COUNT_ENTRIES];	// list of entries with delta values
 #endif
}
UsageDuration_t;

//-----------------------------------------------------------------------------

// n_rec<0: clear from end, n_rec>=0: clear from index
void ClearUsageDuration ( UsageDuration_t *ud, int count );

// NOW_NSEC: 0 or GetTimerNSec(), but not GetTime*()
void UpdateUsageDuration ( UsageDuration_t *ud, u_nsec_t now_nsec );
void UpdateUsageDurationAdd ( UsageDuration_t *ud, int add, u_nsec_t wait_nsec, u_nsec_t top_nsec );
void UpdateUsageDurationIncrement ( UsageDuration_t *ud, u_nsec_t wait_nsec );
void UsageDurationIncrement ( UsageDuration_t *ud, u_nsec_t wait_nsec );

// stat about last 'nsec' nanoseconds
UsageDurationEntry_t GetUsageDuration ( const UsageDuration_t *ud, s_nsec_t nsec );

void AddUsageDurationEntry ( UsageDurationEntry_t *dest, const UsageDurationEntry_t *add );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			usage counter mgr		///////////////
///////////////////////////////////////////////////////////////////////////////
//  [[ListUsageFunc]]

typedef struct UsageCtrl_t UsageCtrl_t;

enum
{
    LUC_PM_STANDARD,	// standard view
    LUC_PM_RAW,		// raw values, not supported by ListUsageCount()
    LUC_PM_SUMMARY,	// summary
    LUC_PM__USER	// base value for user defined modes
};

typedef void (*ListUsageFunc)
(
    PrintMode_t		*pm,		// valid print mode
    const UsageCtrl_t	*uci,		// valid object to list
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
);

//-----------------------------------------------------------------------------

typedef void (*UpdateUsageFunc)
(
    const UsageCtrl_t	*uci,		// valid object to update
    u_nsec_t		now_usec	// NULL or current time by GetTimerNSec()
);

//-----------------------------------------------------------------------------
// [[UsageCtrl_t]]

typedef struct UsageCtrl_t
{
    UsageParam_t	*par;		// higher prioritiy than uc->par and ud->par
    UsageCount_t	*uc;		// not NULL for usage counters
    UsageDuration_t	*ud;		// not NULL for wait counters

    UpdateUsageFunc	update_func;	// not NULL: Call this function to update
    ListUsageFunc	list_func;	// not NULL: Call this function to list

    ccp			title;		// NULL or title
    ccp			key1;		// first key, upper case
    ccp			key2;		// NULL or second key, upper case

    ccp			srt_load;	// NULL or prefix vor SRT reading to allow prefix changes
    ccp			srt_prefix;	// NULL prefix or SRT reading + writing
					//   If NULL: don't save/restore
					//   'srt_load' is scanned before 'srt_prefix'

    float		*color_val;	// 6 values for mark,info,gint,warn,err,bad
					// if NULL then use system list
    u_nsec_t		*color_dur;	// 6 values for mark,info,gint,warn,err,bad
					// if NULL then use system list
}
UsageCtrl_t;

//-----------------------------------------------------------------------------

extern PointerList_t usage_count_mgr;

// NOW_NSEC: 0 or GetTimerNSec(), but not GetTime*()
u_nsec_t MaintainUsageByMgr ( u_nsec_t now_nsec );
void UpdateByUCI ( const UsageCtrl_t *uci, u_nsec_t now_nsec );
void UpdateUsageByMgr ( u_nsec_t now_nsec );
void AddToUsageMgr ( const UsageCtrl_t *uci );

void ListUsageCount
(
    PrintMode_t		*p_pm,		// NULL or print mode
    const UsageCtrl_t	*uci,		// NULL or object to list
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
);

void ListUsageDuration
(
    PrintMode_t		*p_pm,		// NULL or print mode
    const UsageCtrl_t	*uci,		// NULL or object to list
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
);

static inline UsageParam_t * GetUsageParam ( const UsageCtrl_t *uci )
	{ DASSERT(uci); return uci->par ? uci->par : uci->uc ? &uci->uc->par : uci->ud ? &uci->ud->par : 0; }

static inline char GetUsageTypeChar ( const UsageCtrl_t *uci )
	{ return !uci ? '-' : uci->uc ? 'C' : uci->ud ? 'D' : uci->par ? 'x' : '?'; }

void SaveCurrentStateByUsageCountMgr ( FILE *f, uint fw_name );
void RestoreStateByUsageCountMgr ( RestoreState_t *rs );

///////////////////////////////////////////////////////////////////////////////
// [[IntervalInfo_t]]

typedef struct IntervalInfo_t
{
    u_nsec_t	nsec;
    u_usec_t	usec;
    ccp		name;
}
IntervalInfo_t;

extern const IntervalInfo_t interval_info[];

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    cpu usage			///////////////
///////////////////////////////////////////////////////////////////////////////

#if TEST_USAGE_COUNT
  #define CPU_USAGE_ENTRIES	11	// number of entries
#else
  #define CPU_USAGE_ENTRIES	30	// number of entries
#endif

#if IS_DEVELOP
  #define CPU_USAGE_ENABLED	0	// initial value for cpu_usage.par.enabled
#else
  #define CPU_USAGE_ENABLED	0	// initial value for cpu_usage.par.enabled
#endif

#define CPU_USAGE_FIRST_ADD	 8	// first entry where values wil be added
#define CPU_USAGE_NEXT_MIN(v)	(15*(v)/8)

//-----------------------------------------------------------------------------
// [[CpuUsageEntry_t]]

typedef struct CpuUsageEntry_t
{
    u_usec_t	elapsed_usec;	// elapsed time, by GetTimerUSec()
    u_usec_t	utime_usec;	// user CPU time used
    u_usec_t	stime_usec;	// system CPU time used
    u_sec_t	time_sec;	// time of record, by GetTimeUSec(false)
    float	top1s;		// = 100.0*(utime+stime)/elapsed for 1s
    float	top5s;		// = 100.0*(utime+stime)/elapsed for 5s
    u32		n_wait;		// number of waits (poll() or select())
    float	topw1s;		// top number of waits in 1 sec
    float	topw5s;		// top number of waits in 5 sec
}
__attribute__ ((packed)) CpuUsageEntry_t;

//-----------------------------------------------------------------------------
//  [[CpuUsage_t]]

typedef struct CpuUsage_t
{
    UsageParam_t	par;				// base parameters
    CpuUsageEntry_t	ref;				// reference with absolute values
 #if TEST_USAGE_COUNT
    CpuUsageEntry_t	entry[CPU_USAGE_ENTRIES+1];	// list of entries with delta values
 #else
    CpuUsageEntry_t	entry[CPU_USAGE_ENTRIES];	// list of entries with delta values
 #endif
}
CpuUsage_t;

//-----------------------------------------------------------------------------

extern CpuUsage_t cpu_usage;
extern const UsageCtrl_t cpu_usage_ctrl;

void UpdateCpuUsage(void);
void UpdateCpuUsageByUCI ( const UsageCtrl_t *, u_nsec_t ); // both params ignored
void UpdateCpuUsageIncrement(void);

void ListCpuUsage
(
    PrintMode_t		*p_pm,		// NULL or print mode
    const UsageCtrl_t	*uci,		// ignored!
    int			print_mode,	// see LUC_PM_*
    s_nsec_t		limit_nsec	// >0: limit output until # is reached
);

// stat about last 'usec' microseconds
CpuUsageEntry_t GetCpuUsage ( s_usec_t usec );
uint GetCpuUsagePercent ( s_usec_t usec );

void AddCpuEntry ( CpuUsageEntry_t *dest, const CpuUsageEntry_t *add );

static inline uint GetCpuEntryPercent ( const CpuUsageEntry_t *entry )
{
    DASSERT(entry);
    return entry->elapsed_usec
	? 100 * ( entry->utime_usec + entry->stime_usec + entry->elapsed_usec/200 )
		/ entry->elapsed_usec
	: 0;
}

static inline double GetCpuEntryRatio ( const CpuUsageEntry_t *entry )
{
    DASSERT(entry);
    return entry->elapsed_usec
	? (double)(entry->utime_usec + entry->stime_usec ) / entry->elapsed_usec
	: 0.0;
}

//-----------------------------------------------------------------------------

// get percent, returns previuis valus if delay is <10ms
double get_rusage_percent (void);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sockaddr support		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[sockaddr_in46_t]]

typedef struct sockaddr_in46_t
{
    union
    {
	sa_family_t	family; // is first member of each sockaddr_* structure
	sockaddr_in4_t	in4;
	sockaddr_in6_t	in6;
    };
}
sockaddr_in46_t;

//-----------------------------------------------------------------------------
// [[sockaddr_dclib_t]]

typedef struct sockaddr_dclib_t
{
    union
    {
	sa_family_t	family; // is first member of each sockaddr_* structure
	sockaddr_in4_t	in4;
	sockaddr_in6_t	in6;
	sockaddr_un_t	un;
    };
}
sockaddr_dclib_t;

//-----------------------------------------------------------------------------
// [[sockaddr_info_t]]

typedef struct sockaddr_info_t
{
    sockaddr_t	*sa;	    // pointer to sockaddr or sockaddr_*
    uint	size;	    // size of '*sa'
}
sockaddr_info_t;

///////////////////////////////////////////////////////////////////////////////

// [[doxygen]]
static inline sockaddr_info_t CreateSockaddrInfo ( cvp sa, uint size )
{
    sockaddr_info_t i =
    {
	.sa   = (sockaddr_t*)sa,
	.size = size
    };
    return i;
}

//-----------------------------------------------------------------------------

// [[doxygen]]
static inline void SetupSockaddrInfo ( sockaddr_info_t *sai, cvp sa, uint size )
{
    DASSERT(sai);
    sai->sa   = (sockaddr_t*)sa;
    sai->size = size ? size : sizeof(sockaddr_in_t);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanIP4				///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ipv4_class_t]]

typedef enum ipv4_class_t
{
    CIP4_ERROR	= -1,	// error
    CIP4_INVALID = 0,	// not an IPv4
    CIP4_NUMERIC,	// 32 bit number only
    CIP4_ABBREV,	// abbreviated IPv4 (trailing point)
    CIP4_CLASS_A,	// IPv4 in class-A notation
    CIP4_CLASS_B,	// IPv4 in class-B notation
    CIP4_CLASS_C,	// IPv4 in class-C notation
}
ipv4_class_t;

//-----------------------------------------------------------------------------

ipv4_class_t ScanIP4
(
    ipv4_t	*ret_ip4,	// not NULL: return numeric IPv4 here
    ccp		addr,		// address to analyze
    int		addr_len	// length of addr; if <0: use strlen(addr)
);

//-----------------------------------------------------------------------------

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
);

//-----------------------------------------------------------------------------

static inline ipv4_class_t CheckIP4
(
    ccp		addr,		// address to analyze
    int		addr_len	// length of addr; if <0: use strlen(addr)
)
{
    return ScanIP4(0,addr,addr_len);
}

//-----------------------------------------------------------------------------

int dclib_aton ( ccp addr, struct in_addr *inp );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PrintIP4*()			///////////////
///////////////////////////////////////////////////////////////////////////////

#define FW_IPV4_N 10	// 4294967295
#define FW_IPV4_A 12	// 255.16777215
#define FW_IPV4_B 13	// 255.255.65535
#define FW_IPV4_C 15	// 255.255.255.255

#define FW_IPV4_PORT 5  // 65535

#define FW_IPV4_N_PORT (FW_IPV4_N+1+FW_IPV4_PORT)
#define FW_IPV4_A_PORT (FW_IPV4_A+1+FW_IPV4_PORT)
#define FW_IPV4_B_PORT (FW_IPV4_B+1+FW_IPV4_PORT)
#define FW_IPV4_C_PORT (FW_IPV4_C+1+FW_IPV4_PORT)

//-----------------------------------------------------------------------------

char * PrintIP4N
(
    // print as unsigned number (CIP4_NUMERIC)

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

char * PrintIP4A
(
    // print in A-notation (CIP4_CLASS_A)

    char	*buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	buf_size,	// size of 'buf', ignored if buf==NULL
    u32		ip4,		// IP4 to print
    s32		port		// 0..0xffff: add ':port'
);

char * PrintIP4B
(
    // print in B-notation (CIP4_CLASS_B)

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
// print by enum ipv4_class_t

char * PrintIP4ByMode
(
    char	 *buf,		// result buffer
				// NULL: use a local circulary static buffer
    size_t	 buf_size,	// size of 'buf', ignored if buf==NULL
    u32		 ip4,		// IP4 to print
    ipv4_class_t mode,		// CIP4_NUMERIC | CIP4_CLASS_A | CIP4_CLASS_B:
				// force numeric or class-A/B output; otherwise class-C output
    s32		 port		// 0..0xffff: add ':port'
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
    return sa
	? PrintIP4(buf,buf_size,
		ntohl(sa->sin_addr.s_addr), ntohs(sa->sin_port) )
	: "-";
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
// [[DynData_t]]

typedef struct DynData_t
{
    u8		*data;		// pointer to data, alloced
    uint	len;		// length of used data
    uint	size;		// size of allocated data
}
DynData_t;

//-----------------------------------------------------------------------------

void ResetDD  ( DynData_t *dd );
void InsertDD ( DynData_t *dd, const void *data, uint data_size );

///////////////////////////////////////////////////////////////////////////////
// [[DynDataList_t]]

typedef struct DynDataList_t
{
    DynData_t	*list;		// pointer to the data list
    uint	used;		// number of used data elements
    uint	size;		// number of allocated pointers in 'list'
}
DynDataList_t;

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
    uint	size;			// number of allocated pointers in 'field'
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

// append at the end and do not sort
void AppendStringField ( StringField_t * sf, ccp key, bool move_key );

// append at the end if not already exists, do not sort
void AppendUniqueStringField ( StringField_t * sf, ccp key, bool move_key );

// insert or append a path (path1+path2) with optional wildcards by SearchPaths()
uint InsertStringFieldExpand ( StringField_t * sf, ccp path1, ccp path2, bool allow_hidden );
uint AppendStringFieldExpand ( StringField_t * sf, ccp path1, ccp path2, bool allow_hidden );

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
    uint		size;		// number of allocated pointers in 'field'
    bool		free_data;	// true: unused data will be free'd
					//   initialized with 'false'
    int	(*func_cmp)( ccp s1, ccp s2 );	// compare function, default is strcmp()
} ParamField_t;

//-----------------------------------------------------------------------------

static inline void InitializeParamField ( ParamField_t * pf )
	{ DASSERT(pf); memset(pf,0,sizeof(*pf)); }

void ResetParamField ( ParamField_t * pf );
void MoveParamField ( ParamField_t * dest, ParamField_t * src );

int FindParamFieldIndex ( const ParamField_t * pf, ccp key, int not_found_value );
ParamFieldItem_t * FindParamField ( const ParamField_t * pf, ccp key );
ParamFieldItem_t * FindParamFieldByNum ( const ParamField_t * pf, uint num );
ParamFieldItem_t * FindInsertParamField
		( ParamField_t * pf, ccp key, bool move_key, uint num, bool *old_found );
ParamFieldItem_t * IncrementParamField ( ParamField_t * pf, ccp key, bool move_key );

// Insert: Insert only if not found, Replace: Insert or replace data
// return: true if item inserted/deleted
bool InsertParamField ( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data );
bool ReplaceParamField( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data );
bool RemoveParamField ( ParamField_t * pf, ccp key );

// append at the end and do not sort
ParamFieldItem_t * AppendParamField
	( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data );

// insert or append a path (path1+path2) with optional wildcards by SearchPaths()
uint InsertParamFieldExpand
	( ParamField_t * pf, ccp path1, ccp path2, bool allow_hidden, uint num, cvp data );
uint AppendParamFieldExpand
	( ParamField_t * pf, ccp path1, ccp path2, bool allow_hidden, uint num, cvp data );

// return the index of the (next) item
uint FindParamFieldHelper ( const ParamField_t * pf, bool * found, ccp key );

// find first pattern that matches
ParamFieldItem_t * MatchParamField ( const ParamField_t * pf, ccp key );

//-----------------------------------------------------------------------------
// get values and set marker by 'num |= mark'

ccp GetParamFieldStr	( ParamField_t *pf, uint mark, ccp key, ccp def );
int GetParamFieldInt	( ParamField_t *pf, uint mark, ccp key, int def );
int GetParamFieldIntMM	( ParamField_t *pf, uint mark, ccp key, int def, int min, int max );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct exmem_list_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[exmem_key_t]]

typedef struct exmem_key_t
{
    ccp			key;		// string as main key => data.is_key_alloced
    exmem_t		data;		// data
}
exmem_key_t;

//-----------------------------------------------------------------------------
// [[exmem_list_t]]

typedef struct exmem_list_t
{
    exmem_key_t		*list;		// data list
    uint		used;		// number of used titles in the title field
    uint		size;		// number of allocated pointers in 'field'
    bool		is_unsorted;	// true: list is not sorted (Append() was used)
    int	(*func_cmp)( ccp s1, ccp s2 );	// compare function, default is strcmp()
}
exmem_list_t;

//-----------------------------------------------------------------------------

static inline void InitializeEML ( exmem_list_t * eml )
	{ DASSERT(eml); memset(eml,0,sizeof(*eml)); }

void ResetEML ( exmem_list_t * eml );
void MoveEML ( exmem_list_t * dest, exmem_list_t * src );

// return the index of the (next) item
uint FindHelperEML ( const exmem_list_t * eml, ccp key, bool *found );

int FindIndexEML ( const exmem_list_t * eml, ccp key, int not_found_value );
exmem_key_t * FindEML ( const exmem_list_t * eml, ccp key );
exmem_key_t * FindAttribEML ( const exmem_list_t * eml, u32 attrib );
exmem_key_t * FindInsertEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key, bool *old_found );

// Insert if not found, ignore othwerwise. Return true if item inserted
bool InsertEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data );

// Insert or replace content. Return true if item inserted
bool ReplaceEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data );

// Replace content if found. Return true if item found
bool OverwriteEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data );

// Append at the end and do not sort
exmem_key_t * AppendEML  ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data );

// Return true if item found and removed
bool RemoveByIndexEML ( exmem_list_t * eml, int index );
bool RemoveByKeyEML ( exmem_list_t * eml, ccp key );


// find first pattern that matches
exmem_key_t * MatchEML ( const exmem_list_t * eml, ccp key );

void DumpEML ( FILE *f, int indent, const exmem_list_t * eml, ccp info );

//-----------------------------------------------------------------------------

exmem_t ResolveSymbolsEML ( const exmem_list_t * eml, ccp source );
uint ResolveAllSymbolsEML ( exmem_list_t * eml );

// add symbols 'home', 'etc', 'install' = 'prog' and 'cwd'
void AddStandardSymbolsEML ( exmem_list_t * eml, bool overwrite );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MultiColumn			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MultiColumn_t]]

typedef struct MultiColumn_t
{
    FILE	*f;		// NULL or output file
    const ColorSet_t
		*colset;	// NULL or color set for the frame
    ccp		prefix;		// NULL or line prefix
    int		indent;		// indention after prefix
    ccp		sep;		// NULL or separator for frame_mode 0 and 1
    ccp		eol;		// NULL or end-of-line string

    int		fw;		// max field width
    int		min_rows;	// minimal number of rows before MC
    int		max_cols;	// >0: maximal number columns

    int		frame_mode;	// 0:ASCII, 1:single line, 2:double line
    bool	print_header;	// TRUE: print column header
    bool	print_footer;	// TRUE: print footer line

    ccp		head_key;	// NULL or name of key column, default="Name"
    ccp		head_val;	// NULL or name of value column, default="Value"

    int		debug;		// >0: print basic debug info
				// >1: be verbose

    exmem_list_t list;		// list of parameters to print
}
MultiColumn_t;

//-----------------------------------------------------------------------------

void ResetMultiColumn ( MultiColumn_t *mc );

// return the number of printed lines including header and footer, or -1 on error
int PrintMultiColumn ( const MultiColumn_t *mc );

// Wrapper for short definition lines
//  opt == 0 align right value
//  opt >= 1 align left and try field width of # for value

exmem_key_t * PutLineMC ( MultiColumn_t *mc, uint opt, ccp key, ccp value, CopyMode_t cm_value );
exmem_key_t * PrintLineMC ( MultiColumn_t *mc, uint opt, ccp key, ccp format, ... )
	__attribute__ ((__format__(__printf__,4,5)));

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
    uint	size;		// number of allocated pointers in 'field'
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
// [[ResizeElement_t]]

typedef struct ResizeElement_t
{
    uint n_elem;
    uint factor;
}
ResizeElement_t;

//-----------------------------------------------------------------------------
// [[ResizeHelper_t]]

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
///////////////			restore configuration		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[RestoreStateLog_t]]

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

///////////////////////////////////////////////////////////////////////////////

static inline void InitializeRestoreState ( RestoreState_t *rs )
	{ DASSERT(rs); memset(rs,0,sizeof(*rs)); }

void ResetRestoreState ( RestoreState_t *rs );

///////////////////////////////////////////////////////////////////////////////
// [[RestoreStateFunc]]

typedef void (*RestoreStateFunc)
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

//-----------------------------------------------------------------------------
// [[RestoreStateTab_t]]

typedef struct RestoreStateTab_t
{
    ccp			name;		// name of section
    RestoreStateFunc	func;		// function to call
    cvp			user_table;	// any user provided pointer
}
RestoreStateTab_t;

///////////////////////////////////////////////////////////////////////////////

enumError ScanRestoreState
(
    RestoreState_t	*rs,		// valid control
    void		*data,		// file data, modified, terminated by NULL or LF
    uint		size,		// size of file data
    void		**end_data	// not NULL: store end of analysed here
);

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

u64 GetParamFieldS64
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    s64			not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

u64 GetParamFieldU64
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    u64			not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

float GetParamFieldFLOAT
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    float		not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

double GetParamFieldDBL
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    double		not_found	// return this value if not found
);

//-----------------------------------------------------------------------------

long double GetParamFieldLDBL
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    long double		not_found	// return this value if not found
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

//-----------------------------------------------------------------------------

mem_t GetParamFieldMEM
(
    // Returns the decoded 'source'. Result is NULL-terminated.
    // It points either to 'buf' or is alloced (on buf==NULL or too less space)
    // If alloced (mem.ptr!=buf) => call FreeMem(&mem)

    char		*buf,		// buffer to store result
    uint		buf_size,	// size of buffer

    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    EncodeMode_t	decode,		// decoding mode, fall back to OFF
					// supported: STRING, UTF8, BASE64, BASE64X
    mem_t		not_found	// not NULL: return this value
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		save + restore config by table		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[SaveRestoreType_t]]

typedef enum SaveRestoreType_t
{
    SRT__TERM,		// list terminator

    SRT_BOOL,		// type bool
    SRT_UINT,		// unsigned int of any size (%llu)
    SRT_HEX,		// unsigned int of any size as hex (%#llx)
    SRT_COUNT,		// unsigned int of any size (%llu), used as array counter
    SRT_INT,		// signed int of any size (%lld)
    SRT_FLOAT,		// float (%.8g) or double (%.16g) or long double (%.20g)
    SRT_XFLOAT,		// float or double or long double as hex-float (%a/%La)

    SRT_STRING_SIZE,	// string with max size, var is char[]
    SRT_STRING_ALLOC,	// alloced string, var is ccp or char*
    SRT_MEM,		// alloced string, var is mem_t

    SRT_DEF_ARRAY,	// define/end an array of structs

    SRT_MODE__BEGIN,	//--- begin of modes
    SRT_MODE_ASSIGN,	// load option: assign numeric values (default)
    SRT_MODE_ADD,	// load option: add numeric values
    SRT_MODE__END,	//--- end of modes

    SRT__IS_LIST,	//----- from here: print lists; also used as separator

    SRT_STRING_FIELD,	// var is StringField_t
    SRT_PARAM_FIELD,	// var is ParamField_t


    SRT_F_SIZE = 0x100,	// factor for implicit size-modes for numerical types
}
SaveRestoreType_t;

///////////////////////////////////////////////////////////////////////////////
// [[SaveRestoreTab_t]]

typedef struct SaveRestoreTab_t
{
    uint	offset;	// offset of variable
    uint	size;	// sizeof( var or string )
    ccp		name;	// name in configuration file

    union
    {
	struct
	{
	    s16		n_elem;	// >0: is array with N elements
				// <0: is array with -N elements, use last SRT_COUNT
	    u8		type;	// SaveRestoreType_t
	    u8		emode;	// EncodeMode_t
	};
	//struct SaveRestoreTab_t *ref;
    };
}
__attribute__ ((packed)) SaveRestoreTab_t;

//-----------------------------------------------------------------------------

// At the very beginning of the table, define 'SRT_NAME' by 2 lines:
// 	#undef  SRT_NAME
//	#define SRT_NAME name_of_the_struct

#define DEF_SRT_VAR(v,ne,n,t,e)		{offsetof(SRT_NAME,v),sizeof(((SRT_NAME*)0)->v),n, \
					.n_elem=ne,.type=t,.emode=e}

#define DEF_SRT_BOOL(v,n)		DEF_SRT_VAR(v,1,n,SRT_BOOL,0)
#define DEF_SRT_UINT(v,n)		DEF_SRT_VAR(v,1,n,SRT_UINT,0)
#define DEF_SRT_HEX(v,n)		DEF_SRT_VAR(v,1,n,SRT_HEX,0)
#define DEF_SRT_COUNT(v,n)		DEF_SRT_VAR(v,1,n,SRT_COUNT,0)
#define DEF_SRT_INT(v,n)		DEF_SRT_VAR(v,1,n,SRT_INT,0)
#define DEF_SRT_FLOAT(v,n)		DEF_SRT_VAR(v,1,n,SRT_FLOAT,0)
#define DEF_SRT_XFLOAT(v,n)		DEF_SRT_VAR(v,1,n,SRT_XFLOAT,0)

#define DEF_SRT_STR_SIZE(v,n,e)		DEF_SRT_VAR(v,1,n,SRT_STRING_SIZE,e)
#define DEF_SRT_STR_ALLOC(v,n,e)	DEF_SRT_VAR(v,1,n,SRT_STRING_ALLOC,e)
#define DEF_SRT_MEM(v,n,e)		DEF_SRT_VAR(v,1,n,SRT_MEM,e)

#define DEF_SRT_STRING_FIELD(v,n,e)	DEF_SRT_VAR(v,1,n,SRT_STRING_FIELD,e)
#define DEF_SRT_PARAM_FIELD(v,n,e)	DEF_SRT_VAR(v,1,n,SRT_PARAM_FIELD,e)

//--- n elements : if n<0, then n_elem=-n && use last SRT_COUNT

#define DEF_SRT_BOOL_N(v,ne,n)		DEF_SRT_VAR(v,ne,n,SRT_BOOL,0)
#define DEF_SRT_UINT_N(v,ne,n)		DEF_SRT_VAR(v,ne,n,SRT_UINT,0)
#define DEF_SRT_HEX_N(v,ne,n)		DEF_SRT_VAR(v,ne,n,SRT_HEX,0)
#define DEF_SRT_INT_N(v,ne,n)		DEF_SRT_VAR(v,ne,n,SRT_INT,0)
#define DEF_SRT_FLOAT_N(v,ne,n)		DEF_SRT_VAR(v,ne,n,SRT_FLOAT,0)
#define DEF_SRT_XFLOAT_N(v,ne,n)	DEF_SRT_VAR(v,ne,n,SRT_XFLOAT,0)

#define DEF_SRT_STR_SIZE_N(v,ne,n,e)	DEF_SRT_VAR(v,ne,n,SRT_STRING_SIZE,e)
#define DEF_SRT_STR_ALLOC_N(v,ne,n,e)	DEF_SRT_VAR(v,ne,n,SRT_STRING_ALLOC,e)
#define DEF_SRT_MEM_N(v,ne,n,e)		DEF_SRT_VAR(v,ne,n,SRT_MEM,e)

//--- auto array

#define DEF_SRT_ARRAY(v,n,t,e)		\
	{offsetof(SRT_NAME,v), sizeof(((SRT_NAME*)0)->v[0]), n, \
	 .n_elem=sizeof(((SRT_NAME*)0)->v)/sizeof(*((SRT_NAME*)0)->v), .type=t, .emode=e }

#define DEF_SRT_BOOL_A(v,n)		DEF_SRT_ARRAY(v,n,SRT_BOOL,0)
#define DEF_SRT_UINT_A(v,n)		DEF_SRT_ARRAY(v,n,SRT_UINT,0)
#define DEF_SRT_HEX_A(v,n)		DEF_SRT_ARRAY(v,n,SRT_HEX,0)
#define DEF_SRT_INT_A(v,n)		DEF_SRT_ARRAY(v,n,SRT_INT,0)
#define DEF_SRT_FLOAT_A(v,n)		DEF_SRT_ARRAY(v,n,SRT_FLOAT,0)
#define DEF_SRT_XFLOAT_A(v,n)		DEF_SRT_ARRAY(v,n,SRT_XFLOAT,0)

#define DEF_SRT_STR_SIZE_A(v,n,e)	DEF_SRT_ARRAY(v,n,SRT_STRING_SIZE,e)
#define DEF_SRT_STR_ALLOC_A(v,n,e)	DEF_SRT_ARRAY(v,n,SRT_STRING_ALLOC,e)
#define DEF_SRT_MEM_A(v,n,e)		DEF_SRT_ARRAY(v,n,SRT_MEM,e)

//--- auto array, use last SRT_COUNT for element-count

#define DEF_SRT_ARRAY_C(v,n,t,e)		\
	{offsetof(SRT_NAME,v), sizeof(((SRT_NAME*)0)->v[0]), n, \
	 .n_elem=-(s16)(sizeof(((SRT_NAME*)0)->v)/sizeof(*((SRT_NAME*)0)->v)), .type=t, .emode=e }

#define DEF_SRT_BOOL_AC(v,n)		DEF_SRT_ARRAY_C(v,n,SRT_BOOL,0)
#define DEF_SRT_UINT_AC(v,n)		DEF_SRT_ARRAY_C(v,n,SRT_UINT,0)
#define DEF_SRT_HEX_AC(v,n)		DEF_SRT_ARRAY_C(v,n,SRT_HEX,0)
#define DEF_SRT_INT_AC(v,n)		DEF_SRT_ARRAY_C(v,n,SRT_INT,0)
#define DEF_SRT_FLOAT_AC(v,n)		DEF_SRT_ARRAY_C(v,n,SRT_FLOAT,0)
#define DEF_SRT_XFLOAT_AC(v,n)		DEF_SRT_ARRAY_C(v,n,SRT_XFLOAT,0)

#define DEF_SRT_STR_SIZE_AC(v,n,e)	DEF_SRT_ARRAY_C(v,n,SRT_STRING_SIZE,e)
#define DEF_SRT_STR_ALLOC_AC(v,n,e)	DEF_SRT_ARRAY_C(v,n,SRT_STRING_ALLOC,e)
#define DEF_SRT_MEM_AC(v,n,e)		DEF_SRT_ARRAY_C(v,n,SRT_MEM,e)

//--- array of structs

#define DEF_SRT_ARRAY_FL(f,l) \
	{offsetof(SRT_NAME,f), sizeof(((SRT_NAME*)0)->f), 0, \
	 .n_elem=(offsetof(SRT_NAME,l)-offsetof(SRT_NAME,f))/sizeof(((SRT_NAME*)0)->f)+1, \
	 .type=SRT_DEF_ARRAY }

#define DEF_SRT_ARRAY_FLC(f,l) \
	{offsetof(SRT_NAME,f), sizeof(((SRT_NAME*)0)->f), 0, \
	 .n_elem=-(s16)((offsetof(SRT_NAME,l)-offsetof(SRT_NAME,f)) \
		/sizeof(((SRT_NAME*)0)->f)+1), .type=SRT_DEF_ARRAY }

#define DEF_SRT_ARRAY_N(v,ne)		DEF_SRT_VAR(v[0],ne,0,SRT_DEF_ARRAY,0)
#define DEF_SRT_ARRAY_A(v)		DEF_SRT_ARRAY(v,0,SRT_DEF_ARRAY,0)
#define DEF_SRT_ARRAY_AC(v)		DEF_SRT_ARRAY_C(v,0,SRT_DEF_ARRAY,0)
#define DEF_SRT_ARRAY_END()		{0,0,0,.type=SRT_DEF_ARRAY}

//--- global array of structs

#define DEF_SRT_ARRAY_GN(ne)		{0,sizeof(SRT_NAME),0,.n_elem=ne,.type=SRT_DEF_ARRAY}
#define DEF_SRT_ARRAY_GA(v)		{0,sizeof(*v),0,.n_elem=sizeof(v)/sizeof(*v),.type=SRT_DEF_ARRAY}

//--- modes

#define DEF_SRT_MODE_ASSIGN()		{0,0,0,.type=SRT_MODE_ASSIGN}
#define DEF_SRT_MODE_ADD()		{0,0,0,.type=SRT_MODE_ADD}

//--- special

#define DEF_SRT_SEPARATOR()		{0,0,0,.type=SRT__IS_LIST}
#define DEF_SRT_COMMENT(c)		{0,0,c,.type=SRT__IS_LIST}
#define DEF_SRT_TERM()			{0,0,0,.type=SRT__TERM}

///////////////////////////////////////////////////////////////////////////////

extern int srt_auto_dump;
extern FILE *srt_auto_dump_file;

void DumpStateTable
(
    FILE	*f,		// valid output file
    int		indent,
    const SaveRestoreTab_t
		*srt		// list of variables
);

void SaveCurrentStateByTable
(
    FILE	*f,		// valid output file
    cvp		data0,		// valid pointer to source struct
    const SaveRestoreTab_t
		*srt,		// list of variables
    ccp		prefix,		// NULL or prefix for names
    uint	fw_name		// field width of name, 0=AUTO
				// tabs are used for multiple of 8 and for AUTO
);

void RestoreStateByTable
(
    RestoreState_t	*rs,	// info data, can be modified (cleaned after call)
    void		*data0,	// valid pointer to destination struct
    const SaveRestoreTab_t
			*srt,	// list of variables
    ccp		prefix		// NULL or prefix for names
);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern const SaveRestoreTab_t SRT_UsageCount[];
extern const SaveRestoreTab_t SRT_UsageDuration[];
extern const SaveRestoreTab_t SRT_CpuUsage[];

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

uint CreateUniqueIdN ( int range );
static inline uint CreateUniqueId(void)	   { return CreateUniqueIdN(1); }
static inline uint CreateUniqueIdNBO(void) { return htonl(CreateUniqueIdN(1)); }

void Sha1Hex2Bin ( sha1_hash_t bin, ccp src, ccp end );
void Sha1Bin2Hex ( sha1_hex_t hex, cvp bin );

// return CircBuf()
ccp GetSha1Hex ( cvp bin );

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

///////////////////////////////////////////////////////////////////////////////

void * dc_memrchr ( cvp src, int ch, size_t size );
bool IsMemConst ( cvp mem, uint size, u8 val );

#ifdef __APPLE__
 #define memrchr dc_memrchr
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_BASICS_H
