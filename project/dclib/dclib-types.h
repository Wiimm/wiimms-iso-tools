
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

#ifndef DCLIB_TYPES_H
#define DCLIB_TYPES_H 1

#define _GNU_SOURCE 1

#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifndef WIN_DCLIB
  #include <stdint.h>
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			endian				///////////////
///////////////////////////////////////////////////////////////////////////////
// [[dcEndian_t]]

typedef enum dcEndian_t
{
    //--- always the same

    DC_DEFAULT_ENDIAN	= 0,	    // always 0
    DC_SECOND_ENDIAN	= 1,	    // always >0


    //--- define local & reverse endian, if possible

 #if __BYTE_ORDER == __LITTLE_ENDIAN || __BYTE_ORDER == __BIG_ENDIAN

    DC_LOCAL_ENDIAN	= DC_DEFAULT_ENDIAN,
    DC_REVERSE_ENDIAN	= DC_SECOND_ENDIAN,

 #endif


    //--- define aliases for little & big endian

 #if __BYTE_ORDER == __BIG_ENDIAN

    DC_BIG_ENDIAN	= DC_DEFAULT_ENDIAN,
    DC_LITTLE_ENDIAN	= DC_SECOND_ENDIAN,

 #else

    DC_LITTLE_ENDIAN	= DC_DEFAULT_ENDIAN,
    DC_BIG_ENDIAN	= DC_SECOND_ENDIAN,

 #endif
}
__attribute__ ((packed)) dcEndian_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  constants			///////////////
///////////////////////////////////////////////////////////////////////////////

#define KB_SI 1000
#define MB_SI (1000*1000)
#define GB_SI (1000*1000*1000)
#define TB_SI (1000ull*1000*1000*1000)
#define PB_SI (1000ull*1000*1000*1000*1000)
#define EB_SI (1000ull*1000*1000*1000*1000*1000)

#define KiB 1024
#define MiB (1024*1024)
#define GiB (1024*1024*1024)
#define TiB (1024ull*1024*1024*1024)
#define PiB (1024ull*1024*1024*1024*1024)
#define EiB (1024ull*1024*1024*1024*1024*1024)

// specal '-1' support
#define M1(a) ( (typeof(a))~0 )
#define IS_M1(a) ( (a) == (typeof(a))~0 )

#define ALIGN32(d,a) ((d)+((a)-1)&~(u32)((a)-1))
#define ALIGN64(d,a) ((d)+((a)-1)&~(u64)((a)-1))
#define ALIGNOFF32(d,a) ((d)&~(u32)((a)-1))
#define ALIGNOFF64(d,a) ((d)&~(u64)((a)-1))

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  error codes			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[enumError]]

typedef enum enumError
{
	//-------------------------------------------------------------------
	ERR_OK,			// 100% success; below = warnings
	//-------------------------------------------------------------------

	ERU_DIFFER,
	ERR_DIFFER,
	ERU_NOTHING_TO_DO,
	ERR_NOTHING_TO_DO,
	ERU_SOURCE_FOUND,
	ERR_SOURCE_FOUND,
	ERU_NO_SOURCE_FOUND,
	ERR_NO_SOURCE_FOUND,
	ERU_JOB_IGNORED,
	ERR_JOB_IGNORED,
	ERU_SUBJOB_WARNING,
	ERR_SUBJOB_WARNING,
	ERU_NOT_EXISTS,
	ERR_NOT_EXISTS,

	 ERU_WARN_00,
	 ERU_WARN_01,
	 ERU_WARN_02,
	 ERU_WARN_03,
	 ERU_WARN_04,
	 ERU_WARN_05,
	 ERU_WARN_06,
	 ERU_WARN_07,
	 ERU_WARN_08,
	 ERU_WARN_09,
	 ERU_WARN_10,
	 ERU_WARN_XX,

	//-------------------------------------------------------------------
	ERU_WARNING,
	ERR_WARNING,	// separator: below = real errors and not warnings
	//-------------------------------------------------------------------

	ERU_WRONG_FILE_TYPE,
	ERR_WRONG_FILE_TYPE,
	ERU_INVALID_FILE,
	ERR_INVALID_FILE,
	ERU_INVALID_VERSION,
	ERR_INVALID_VERSION,
	ERU_INVALID_DATA,
	ERR_INVALID_DATA,

	 ERU_ERROR1_00,
	 ERU_ERROR1_01,
	 ERU_ERROR1_02,
	 ERU_ERROR1_03,
	 ERU_ERROR1_04,
	 ERU_ERROR1_05,
	 ERU_ERROR1_06,
	 ERU_ERROR1_07,
	 ERU_ERROR1_08,
	 ERU_ERROR1_09,
	 ERU_ERROR1_10,
	 ERU_ERROR1_11,
	 ERU_ERROR1_12,
	 ERU_ERROR1_13,
	 ERU_ERROR1_14,
	 ERU_ERROR1_15,
	 ERU_ERROR1_16,
	 ERU_ERROR1_17,
	 ERU_ERROR1_18,
	 ERU_ERROR1_19,
	 ERU_ERROR1_20,
	 ERU_ERROR1_XX,

	ERU_ENCODING,
	ERR_ENCODING,
	ERU_DECODING,
	ERR_DECODING,
	ERU_ALREADY_EXISTS,
	ERR_ALREADY_EXISTS,
	ERU_SUBJOB_FAILED,
	ERR_SUBJOB_FAILED,

	ERR_CANT_REMOVE,
	ERU_CANT_REMOVE,
	ERU_CANT_RENAME,
	ERR_CANT_RENAME,
	ERU_CANT_CLOSE,
	ERR_CANT_CLOSE,
	ERU_CANT_CONNECT,
	ERR_CANT_CONNECT,
	ERU_CANT_OPEN,
	ERR_CANT_OPEN,
	ERU_CANT_APPEND,
	ERR_CANT_APPEND,
	ERU_CANT_CREATE,
	ERR_CANT_CREATE,
	ERU_CANT_CREATE_DIR,
	ERR_CANT_CREATE_DIR,

	ERU_READ_FAILED,
	ERR_READ_FAILED,
	ERU_REMOVE_FAILED,
	ERR_REMOVE_FAILED,
	ERU_WRITE_FAILED,
	ERR_WRITE_FAILED,
	ERU_DATABASE,
	ERR_DATABASE,

	 ERU_ERROR2_00,
	 ERU_ERROR2_01,
	 ERU_ERROR2_02,
	 ERU_ERROR2_03,
	 ERU_ERROR2_04,
	 ERU_ERROR2_05,
	 ERU_ERROR2_06,
	 ERU_ERROR2_07,
	 ERU_ERROR2_08,
	 ERU_ERROR2_09,
	 ERU_ERROR2_10,
	 ERU_ERROR2_XX,

	ERU_MISSING_PARAM,
	ERR_MISSING_PARAM,
	ERU_SEMANTIC,
	ERR_SEMANTIC,
	ERU_SYNTAX,
	ERR_SYNTAX,

	ERU_INTERRUPT,
	ERR_INTERRUPT,

	//-------------------------------------------------------------------
	ERU_ERROR,
	ERR_ERROR,	// separator: below = hard/fatal errors => exit
	//-------------------------------------------------------------------

	ERU_NOT_IMPLEMENTED,
	ERR_NOT_IMPLEMENTED,
	ERU_INTERNAL,
	ERR_INTERNAL,

	 ERU_FATAL_00,
	 ERU_FATAL_01,
	 ERU_FATAL_02,
	 ERU_FATAL_03,
	 ERU_FATAL_04,
	 ERU_FATAL_XX,

	ERU_OUT_OF_MEMORY,
	ERR_OUT_OF_MEMORY,

	//-------------------------------------------------------------------
	ERU_FATAL,
	ERR_FATAL,
	//-------------------------------------------------------------------

	ERR__N

} enumError;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 base types			///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef WIN_DCLIB

    typedef unsigned char	u8;
    typedef unsigned short	u16;
    typedef unsigned int	u32;
    typedef unsigned long long 	u64;

    typedef signed char		s8;
    typedef signed short	s16;
    typedef signed int		s32;
    typedef signed long long 	s64;

#else

    typedef uint8_t		u8;
    typedef uint16_t		u16;
    typedef uint32_t		u32;
    //typedef uint64_t		u64;	// is 'long unsigned' in 64 bit linux
    typedef unsigned long long	u64;

    typedef int8_t		s8;
    typedef int16_t		s16;
    typedef int32_t		s32;
    //typedef int64_t		s64;	// is 'long int' in 64 bit linux
    typedef long long int	s64;

 #ifndef __cplusplus
    typedef enum bool { false, true } __attribute__ ((packed)) bool;
 #endif

#endif

typedef u16		be16_t;
typedef u32		be32_t;
typedef u64		be64_t;

typedef float float32; // float as 32 bit binary data, endian is file dependent

///////////////////////////////////////////////////////////////////////////////

#define U8_MIN     (0)
#define U8_MAX  (0xff)
#define S8_MIN (-0x80)
#define S8_MAX  (0x7f)

#define U16_MIN       (0)
#define U16_MAX  (0xffff)
#define S16_MIN (-0x8000)
#define S16_MAX  (0x7fff)

#define U32_MIN           (0u)
#define U32_MAX  (0xffffffffu)
#define S32_MIN (-0x80000000)
#define S32_MAX  (0x7fffffff)

#define U64_MIN                   (0ull)
#define U64_MAX  (0xffffffffffffffffull)
#define S64_MIN (-0x8000000000000000ll)
#define S64_MAX  (0x7fffffffffffffffll)

///////////////////////////////////////////////////////////////////////////////

#ifndef DCLIB_BASIC_TYPES
  #define DCLIB_BASIC_TYPES 1
  typedef const void *  cvp;
  typedef const char *  ccp;
  typedef unsigned char uchar;
  typedef unsigned int  uint;
  typedef unsigned long ulong;
#endif

// [[sha1_hash_t]] [[sha1_id_t]]
typedef u8 sha1_hash_t[20];
typedef u8 sha1_id_t[9];	// 8*chars + NULL

// [[uuid_buf_t]] [[uuid_text_t]]
typedef u8 uuid_buf_t[16];
typedef char uuid_text_t[37];	// "12345678-1234-1234-1234-123456789012" + NULL

typedef int (*qsort_func)( const void *, const void * );
typedef int (*qsort_r_func)( const void *, const void *, void * );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			some structs			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[KeywordTab_t]]

typedef struct KeywordTab_t
{
    s64			id;		// id
    ccp			name1;		// first name
    ccp			name2;		// NULL or second name
    s64			opt;		// option

} KeywordTab_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////		MSVC extensions (not CYGWIN)		///////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef WIN_DCLIB

  typedef u32		mode_t;

#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_TYPES_H

