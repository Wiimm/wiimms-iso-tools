
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

#include <time.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include "dclib-basics.h"
#include "dclib-utf8.h"
#include "dclib-file.h"
#include "dclib-option.h"
#include "dclib-network.h"
#include "dclib-regex.h"
#include "dclib-xdump.h"
#include "dclib-ui.h"

#ifdef DCLIB_MYSQL
  #include "dclib-mysql.h"
#endif

#if DCLIB_TERMINAL
  #include "dclib-terminal.h"
#endif

#if DCLIB_THREAD
  #include "dclib-thread.h"
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    table element abbreviations		///////////////
///////////////////////////////////////////////////////////////////////////////

#define DN DECODE_NULL
#define DC DECODE_CONTROL
#define DL DECODE_LINE
#define DS DECODE_SPACE
#define DP DECODE_SEPARATE
#define DF DECODE_FILLER
#define DX DECODE_OTHER

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    bits			///////////////
///////////////////////////////////////////////////////////////////////////////

const uchar TableBitCount[0x100] =
{
	0,1,1,2, 1,2,2,3, 1,2,2,3, 2,3,3,4,
	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,

	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,

	1,2,2,3, 2,3,3,4, 2,3,3,4, 3,4,4,5,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,

	2,3,3,4, 3,4,4,5, 3,4,4,5, 4,5,5,6,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,
	3,4,4,5, 4,5,5,6, 4,5,5,6, 5,6,6,7,
	4,5,5,6, 5,6,6,7, 5,6,6,7, 6,7,7,8
};

///////////////////////////////////////////////////////////////////////////////

const char TableLowest0Bit[0x100] =
{
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,5,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,6,

	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,5,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,7,

	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,5,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,6,

	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,5,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,4,
	0,1,0,2, 0,1,0,3, 0,1,0,2, 0,1,0,-1
};

///////////////////////////////////////////////////////////////////////////////

const char TableLowest1Bit[0x100] =
{
	-1,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 5,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,

	 6,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 5,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,

	 7,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 5,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,

	 6,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 5,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0,
	 4,0,1,0, 2,0,1,0, 3,0,1,0, 2,0,1,0
};

///////////////////////////////////////////////////////////////////////////////

const char TableHighest0Bit[0x100] =
{
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,

	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,

	6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
	6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
	6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
	6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,

	5,5,5,5, 5,5,5,5, 5,5,5,5, 5,5,5,5,
	5,5,5,5, 5,5,5,5, 5,5,5,5, 5,5,5,5,
	4,4,4,4, 4,4,4,4, 4,4,4,4, 4,4,4,4,
	3,3,3,3, 3,3,3,3, 2,2,2,2, 1,1,0,-1
};

///////////////////////////////////////////////////////////////////////////////

const char TableHighest1Bit[0x100] =
{
	-1,0,1,1, 2,2,2,2, 3,3,3,3, 3,3,3,3,
	 4,4,4,4, 4,4,4,4, 4,4,4,4, 4,4,4,4,
	 5,5,5,5, 5,5,5,5, 5,5,5,5, 5,5,5,5,
	 5,5,5,5, 5,5,5,5, 5,5,5,5, 5,5,5,5,

	 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
	 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
	 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,
	 6,6,6,6, 6,6,6,6, 6,6,6,6, 6,6,6,6,

	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,

	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7,
	 7,7,7,7, 7,7,7,7, 7,7,7,7, 7,7,7,7
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scanning numbers		///////////////
///////////////////////////////////////////////////////////////////////////////

const char TableNumbers[256] =
{
	DN, DC, DC, DC,  DC, DC, DC, DC,  DC, DS, DL, DC,  DS, DL, DC, DC,
	DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC,
	DS, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DP, DX, DX, DX,
	 0,  1,  2,  3,   4,  5,  6,  7,   8,  9, DX, DP,  DX, DX, DX, DX,

	DX, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,
	25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, DX,  DX, DX, DX, DX,
	DX, 10, 11, 12,  13, 14, 15, 16,  17, 18, 19, 20,  21, 22, 23, 24,
	25, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, DX,  DX, DX, DX, DX,

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CRC32 support			///////////////
///////////////////////////////////////////////////////////////////////////////

const u32 TableCRC32[0x100] =
{
 0x00000000,0x77073096,0xee0e612c,0x990951ba, 0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
 0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988, 0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
 0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de, 0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
 0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec, 0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
 0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172, 0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
 0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940, 0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
 0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116, 0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
 0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924, 0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
 0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a, 0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
 0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818, 0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
 0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e, 0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
 0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c, 0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
 0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2, 0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
 0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0, 0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
 0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086, 0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
 0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4, 0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
 0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a, 0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
 0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8, 0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
 0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe, 0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
 0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc, 0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
 0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252, 0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
 0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60, 0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
 0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236, 0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
 0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04, 0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
 0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a, 0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
 0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38, 0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
 0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e, 0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
 0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c, 0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
 0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2, 0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
 0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0, 0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
 0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6, 0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
 0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94, 0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d,
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			helpers for codepages		///////////////
///////////////////////////////////////////////////////////////////////////////

const u16 TableCP1252_80[0x20] =
{
    0x20ac,     0,0x201a,0x0192, 0x201e,0x2026,0x2020,0x2021, // 80..87
    0x02c6,0x2030,0x0160,0x2039, 0x0152,     0,0x017d,     0, // 88..8f
	 0,0x2018,0x2019,0x201c, 0x201d,0x2022,0x2013,0x2014, // 90..97
    0x02dc,0x2122,0x0161,0x203a, 0x0153,     0,0x017e,0x0178, // 98..9f
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			BASE64 support			///////////////
///////////////////////////////////////////////////////////////////////////////
// https://en.wikipedia.org/wiki/Base64

char TableDecode64[256] =
{
	DN, DC, DC, DC,  DC, DC, DC, DC,  DC, DS, DS, DC,  DS, DS, DC, DC, // 0x
	DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC, // 1x
	DS, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, 62,  DP, DX, DX, 63, // 2x
	52, 53, 54, 55,  56, 57, 58, 59,  60, 61, DX, DP,  DX, DF, DX, DX, // 3x

	DX,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14, // 4x
	15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25, DX,  DX, DX, DX, DX, // 5x
	DX, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40, // 6x
	41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51, DX,  DX, DX, DX, DX, // 7x

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // 8x
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // 9x
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Ax
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Bx

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Cx
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Dx
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Ex
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX  // Fx
};

///////////////////////////////////////////////////////////////////////////////

char TableDecode64url[256] =
{
	DN, DC, DC, DC,  DC, DC, DC, DC,  DC, DS, DS, DC,  DS, DS, DC, DC, // 0x
	DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC, // 1x
	DS, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DF, 62,  DP, 62, DX, 63, // 2x
	52, 53, 54, 55,  56, 57, 58, 59,  60, 61, DX, DP,  DX, DF, DX, DX, // 3x

	DX,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14, // 4x
	15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25, DX,  DX, DX, DX, 63, // 5x
	DX, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40, // 6x
	41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51, DX,  DX, DX, DX, DX, // 7x

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // 8x
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // 9x
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Ax
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Bx

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Cx
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Dx
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Ex
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX  // Fx
};

///////////////////////////////////////////////////////////////////////////////

char TableDecode64xml[256] =
{
	DN, DC, DC, DC,  DC, DC, DC, DC,  DC, DS, DS, DC,  DS, DS, DC, DC, // 0x
	DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC,  DC, DC, DC, DC, // 1x
	DS, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DF, 62,  DP, 63, 62, 63, // 2x
	52, 53, 54, 55,  56, 57, 58, 59,  60, 61, 63, DP,  DX, DF, DX, DX, // 3x

	DX,  0,  1,  2,   3,  4,  5,  6,   7,  8,  9, 10,  11, 12, 13, 14, // 4x
	15, 16, 17, 18,  19, 20, 21, 22,  23, 24, 25, DX,  DX, DX, DX, 62, // 5x
	DX, 26, 27, 28,  29, 30, 31, 32,  33, 34, 35, 36,  37, 38, 39, 40, // 6x
	41, 42, 43, 44,  45, 46, 47, 48,  49, 50, 51, DX,  DX, DX, DX, DX, // 7x

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // 8x
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // 9x
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Ax
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Bx

	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Cx
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Dx
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX, // Ex
	DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX,  DX, DX, DX, DX  // Fx
};

///////////////////////////////////////////////////////////////////////////////

const char TableEncode64[64+1] =
{
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9',
	'+','/',
	'=' // fill char
};

///////////////////////////////////////////////////////////////////////////////

const char TableEncode64url[64+1] =
{
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9',
	'-','_',
	'=' // fill char
};

///////////////////////////////////////////////////////////////////////////////

const char TableEncode64star[64+1] =
{
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9',
	'-','_',
	'*' // fill char
};

///////////////////////////////////////////////////////////////////////////////

const char TableEncode64xml[64+1] =
{
	'A','B','C','D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
	'a','b','c','d','e','f','g','h','i','j','k','l','m',
	'n','o','p','q','r','s','t','u','v','w','x','y','z',
	'0','1','2','3','4','5','6','7','8','9',
	'.','-',
	'*' // fill char
};

///////////////////////////////////////////////////////////////////////////////

// for tests: encode the 48 bytes to get the full BASE64 alphabet

const u8 TableAlphabet64[48] =
{
  0x00,0x10,0x83,0x10, 0x51,0x87,0x20,0x92, 0x8b,0x30,0xd3,0x8f, 0x41,0x14,0x93,0x51,
  0x55,0x97,0x61,0x96, 0x9b,0x71,0xd7,0x9f, 0x82,0x18,0xa3,0x92, 0x59,0xa7,0xa2,0x9a,
  0xab,0xb2,0xdb,0xaf, 0xc3,0x1c,0xb3,0xd3, 0x5d,0xb7,0xe3,0x9e, 0xbb,0xf3,0xdf,0xbf,
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			sizeof_info_t			///////////////
///////////////////////////////////////////////////////////////////////////////

const sizeof_info_t sizeof_info_linux[] =
{
    SIZEOF_INFO_TITLE("Basic C types")
	SIZEOF_INFO_ENTRY(char)
	SIZEOF_INFO_ENTRY(wchar_t)
	SIZEOF_INFO_ENTRY(short)
	SIZEOF_INFO_ENTRY(int)
	SIZEOF_INFO_ENTRY(long)
	SIZEOF_INFO_ENTRY(long long)
 #ifdef __SIZEOF_INT128__
	SIZEOF_INFO_ENTRY(__int128_t)
 #endif
	SIZEOF_INFO_ENTRY(float)
	SIZEOF_INFO_ENTRY(double)
	SIZEOF_INFO_ENTRY(long double)
	SIZEOF_INFO_ENTRY(void*)

    SIZEOF_INFO_TITLE("Linux C types")
	SIZEOF_INFO_ENTRY(size_t)
	SIZEOF_INFO_ENTRY(time_t)
	SIZEOF_INFO_ENTRY(struct timeval)
	SIZEOF_INFO_ENTRY(struct timespec)
	SIZEOF_INFO_ENTRY(struct tm)
	SIZEOF_INFO_ENTRY(struct timezone)
	SIZEOF_INFO_ENTRY(struct utimbuf)
	SIZEOF_INFO_ENTRY(struct stat)
	SIZEOF_INFO_ENTRY(fd_set)

    SIZEOF_INFO_TERM()
};

///////////////////////////////////////////////////////////////////////////////

const sizeof_info_t sizeof_info_dclib[] =
{
    SIZEOF_INFO_TITLE("dcLib basic types")
	SIZEOF_INFO_ENTRY(bool)
	SIZEOF_INFO_ENTRY(u8)
	SIZEOF_INFO_ENTRY(u16)
	SIZEOF_INFO_ENTRY(u32)
	SIZEOF_INFO_ENTRY(u64)
 #if HAVE_INT128
	SIZEOF_INFO_ENTRY(u128)
 #endif
	SIZEOF_INFO_ENTRY(intx_t)
	SIZEOF_INFO_ENTRY(mem_t)
	SIZEOF_INFO_ENTRY(exmem_t)

    SIZEOF_INFO_TITLE("dcLib time & timer")
	SIZEOF_INFO_ENTRY(u_sec_t)
	SIZEOF_INFO_ENTRY(u_msec_t)
	SIZEOF_INFO_ENTRY(u_usec_t)
	SIZEOF_INFO_ENTRY(u_nsec_t)
	SIZEOF_INFO_ENTRY(DayTime_t)
	SIZEOF_INFO_ENTRY(CurrentTime_t)

    SIZEOF_INFO_TITLE("dcLib numeric & strings")
	SIZEOF_INFO_ENTRY(float3)
	SIZEOF_INFO_ENTRY(float3List_t)
	SIZEOF_INFO_ENTRY(double3)
	SIZEOF_INFO_ENTRY(double3List_t)
	SIZEOF_INFO_ENTRY(float34)
	SIZEOF_INFO_ENTRY(double34)
	SIZEOF_INFO_ENTRY(MatrixD_t)
	SIZEOF_INFO_ENTRY(RegexReplace_t)
	SIZEOF_INFO_ENTRY(RegexElem_t)
	SIZEOF_INFO_ENTRY(Regex_t)
	SIZEOF_INFO_ENTRY(dcUnicodeTripel)
	SIZEOF_INFO_ENTRY(Escape_t)

    SIZEOF_INFO_TITLE("dcLib buffers")
	SIZEOF_INFO_ENTRY(exmem_dest_t)
	SIZEOF_INFO_ENTRY(sha1_hash_t)
	SIZEOF_INFO_ENTRY(sha1_hex_t)
	SIZEOF_INFO_ENTRY(sha1_id_t)
	SIZEOF_INFO_ENTRY(uuid_text_t)
	SIZEOF_INFO_ENTRY(CircBuf_t)
	SIZEOF_INFO_ENTRY(FastBuf_t)
	SIZEOF_INFO_ENTRY(ContainerData_t)
	SIZEOF_INFO_ENTRY(Container_t)
	SIZEOF_INFO_ENTRY(DataBuf_t)
	SIZEOF_INFO_ENTRY(GrowBuffer_t)

    SIZEOF_INFO_TITLE("dcLib lists")
	SIZEOF_INFO_ENTRY(mem_src_t)
	SIZEOF_INFO_ENTRY(mem_list_t)
	SIZEOF_INFO_ENTRY(exmem_key_t)
	SIZEOF_INFO_ENTRY(exmem_list_t)
	SIZEOF_INFO_ENTRY(sizeof_info_t)
	SIZEOF_INFO_ENTRY(PointerList_t)
	SIZEOF_INFO_ENTRY(KeywordTab_t)
	SIZEOF_INFO_ENTRY(StringField_t)
	SIZEOF_INFO_ENTRY(ParamFieldItem_t)
	SIZEOF_INFO_ENTRY(ParamField_t)
	SIZEOF_INFO_ENTRY(SplitArg_t)
	SIZEOF_INFO_ENTRY(ArgManager_t)
	SIZEOF_INFO_ENTRY(CommandList_t)
	SIZEOF_INFO_ENTRY(MemMapItem_t)
	SIZEOF_INFO_ENTRY(MemMap_t)
	SIZEOF_INFO_ENTRY(DynData_t)
	SIZEOF_INFO_ENTRY(DynDataList_t)
	SIZEOF_INFO_ENTRY(MemPoolChunk_t)
	SIZEOF_INFO_ENTRY(MemPool_t)
	SIZEOF_INFO_ENTRY(GenericOptParm_t)
	SIZEOF_INFO_ENTRY(GenericOpt_t)
	SIZEOF_INFO_ENTRY(GParam_t)
	SIZEOF_INFO_ENTRY(GOptions_t)
	SIZEOF_INFO_ENTRY(InfoOption_t)
	SIZEOF_INFO_ENTRY(InfoCommand_t)
	SIZEOF_INFO_ENTRY(InfoUI_t)

    SIZEOF_INFO_TITLE("dcLib file")
	SIZEOF_INFO_ENTRY(LogFile_t)
	SIZEOF_INFO_ENTRY(FileAttrib_t)
	SIZEOF_INFO_ENTRY(File_t)
	SIZEOF_INFO_ENTRY(MemFile_t)
	SIZEOF_INFO_ENTRY(TraceLog_t)
 #ifndef __APPLE__
	SIZEOF_INFO_ENTRY(LineBuffer_t)
 #endif
	SIZEOF_INFO_ENTRY(search_file_t)
	SIZEOF_INFO_ENTRY(search_file_list_t)
	SIZEOF_INFO_ENTRY(search_paths_stat_t)
	SIZEOF_INFO_ENTRY(CatchOutput_t)
	SIZEOF_INFO_ENTRY(SectionInfo_t)
	SIZEOF_INFO_ENTRY(stat_file_count_t)
	SIZEOF_INFO_ENTRY(PrintScript_t)
	SIZEOF_INFO_ENTRY(RestoreState_t)
	SIZEOF_INFO_ENTRY(RestoreStateTab_t)
	SIZEOF_INFO_ENTRY(SaveRestoreType_t)
	SIZEOF_INFO_ENTRY(SaveRestoreTab_t)
 #ifdef DCLIB_MYSQL
	SIZEOF_INFO_ENTRY(MySqlStatus_t)
	SIZEOF_INFO_ENTRY(MySqlResult_t)
	SIZEOF_INFO_ENTRY(MySql_t)
	SIZEOF_INFO_ENTRY(MySqlServerStats_t)
 #endif

    SIZEOF_INFO_TITLE("dcLib network")
	SIZEOF_INFO_ENTRY(ipv4_t)
	SIZEOF_INFO_ENTRY(ipv4x_t)
	SIZEOF_INFO_ENTRY(ipv6_t)
	SIZEOF_INFO_ENTRY(sockaddr_t)
	SIZEOF_INFO_ENTRY(sockaddr_in4_t)
	SIZEOF_INFO_ENTRY(sockaddr_in6_t)
	SIZEOF_INFO_ENTRY(sockaddr_in46_t)
	SIZEOF_INFO_ENTRY(sockaddr_un_t)
	SIZEOF_INFO_ENTRY(sockaddr_dclib_t)
	SIZEOF_INFO_ENTRY(sockaddr_info_t)
	SIZEOF_INFO_ENTRY(socket_info_t)
	SIZEOF_INFO_ENTRY(FDList_t)
	SIZEOF_INFO_ENTRY(SplitIP_t)
	SIZEOF_INFO_ENTRY(NamesIP_t)
	SIZEOF_INFO_ENTRY(BinIP_t)
	SIZEOF_INFO_ENTRY(BinIPItem_t)
	SIZEOF_INFO_ENTRY(BinIPList_t)
	SIZEOF_INFO_ENTRY(BinIPIterate_t)
	SIZEOF_INFO_ENTRY(ManageIP_t)
	SIZEOF_INFO_ENTRY(ResolveIP_t)
	SIZEOF_INFO_ENTRY(NetworkHost_t)
	SIZEOF_INFO_ENTRY(AllowIP4Item_t)
	SIZEOF_INFO_ENTRY(AllowIP4_t)
	SIZEOF_INFO_ENTRY(ether_head_t)
	SIZEOF_INFO_ENTRY(ether_head_vlan_t)
	SIZEOF_INFO_ENTRY(arp_head_t)
	SIZEOF_INFO_ENTRY(ip4_head_t)
	SIZEOF_INFO_ENTRY(udp_head_t)
	SIZEOF_INFO_ENTRY(udp_packet_t)
	SIZEOF_INFO_ENTRY(tcp_head_t)
	SIZEOF_INFO_ENTRY(TransferStats_t)
	SIZEOF_INFO_ENTRY(Socket_t)
	SIZEOF_INFO_ENTRY(TCPStream_t)
	SIZEOF_INFO_ENTRY(TCPHandler_t)
	SIZEOF_INFO_ENTRY(CommandTCPInfo_t)
 #if defined(SYSTEM_LINUX) || defined(__CYGWIN__)
	SIZEOF_INFO_ENTRY(RouteIP4_t)
 #endif

 #if DCLIB_TERMINAL
    SIZEOF_INFO_TITLE("dcLib terminal & colors")
 #else
    SIZEOF_INFO_TITLE("dcLib colors")
 #endif
	SIZEOF_INFO_ENTRY(term_size_t)
	SIZEOF_INFO_ENTRY(good_term_width_t)
	SIZEOF_INFO_ENTRY(TermColorId_t)
	SIZEOF_INFO_ENTRY(ColorSet_t)
	SIZEOF_INFO_ENTRY(ColorView_t)
	SIZEOF_INFO_ENTRY(SavedStdFiles_t)
 #if DCLIB_TERMINAL
	SIZEOF_INFO_ENTRY(History_t)
	SIZEOF_INFO_ENTRY(TelnetRepeat_t)
	SIZEOF_INFO_ENTRY(TelnetParam_t)
	SIZEOF_INFO_ENTRY(TelnetTCPInfo_t)
	SIZEOF_INFO_ENTRY(TelnetClient_t)
	SIZEOF_INFO_ENTRY(TerminalKey_t)
	SIZEOF_INFO_ENTRY(TerminalInfo_t)
	SIZEOF_INFO_ENTRY(TerminalNameList_t)
	SIZEOF_INFO_ENTRY(Keyboard_t)
 #endif

 #if DCLIB_THREAD
    SIZEOF_INFO_TITLE("dcLib threads")
	SIZEOF_INFO_ENTRY(WatchdogThread_t)
 #endif

    SIZEOF_INFO_TITLE("dcLib statistics")
	SIZEOF_INFO_ENTRY(UsageParam_t)
	SIZEOF_INFO_ENTRY(UsageCountEntry_t)
	SIZEOF_INFO_ENTRY(UsageCount_t)
	SIZEOF_INFO_ENTRY(UsageDurationEntry_t)
	SIZEOF_INFO_ENTRY(UsageDuration_t)
	SIZEOF_INFO_ENTRY(CpuUsageEntry_t)
	SIZEOF_INFO_ENTRY(CpuUsage_t)
	SIZEOF_INFO_ENTRY(UsageCtrl_t)
	SIZEOF_INFO_ENTRY(usage_count_mgr)
	SIZEOF_INFO_ENTRY(CpuStatus_t)
	SIZEOF_INFO_ENTRY(MemoryStatus_t)

    SIZEOF_INFO_TITLE("dcLib misc")
	SIZEOF_INFO_ENTRY(ProgInfo_t)
	SIZEOF_INFO_ENTRY(IntervalInfo_t)
	SIZEOF_INFO_ENTRY(MultiColumn_t)
	SIZEOF_INFO_ENTRY(ScanAddr_t)
	SIZEOF_INFO_ENTRY(ResizeElement_t)
	SIZEOF_INFO_ENTRY(ResizeHelper_t)
	SIZEOF_INFO_ENTRY(XDump_t)

    SIZEOF_INFO_TERM()
};

///////////////////////////////////////////////////////////////////////////////

const sizeof_info_t *sizeof_info_default[] =
{
	sizeof_info_linux,
	sizeof_info_dclib,
	0
};

PointerList_t SizeofInfoMgr = {0};

///////////////////////////////////////////////////////////////////////////////

static int compare_sizeof_size
	( const sizeof_info_t *a, const sizeof_info_t *b )
{
    DASSERT( a && b );
    const int stat = a->size - b->size;
    return stat ? stat : strcasecmp(a->name,b->name);
}

//-----------------------------------------------------------------------------

static int compare_sizeof_name
	( const sizeof_info_t *a, const sizeof_info_t *b )
{
    DASSERT( a && b );
    return strcasecmp(a->name,b->name);
}

///////////////////////////////////////////////////////////////////////////////

void ListSizeofInfo
(
    const PrintMode_t	*p_pm,		// NULL or print mode
    const sizeof_info_t	**si_list,	// list of list of entries
    ArgManager_t	*p_filter,	// NULL or filter arguments, LOUP_LOWER recommended
    sizeof_info_order_t	order		// kind of order
)
{
    if ( !si_list || !*si_list )
	return;


    //--- setup

    PrintMode_t pm = {0};
    if (p_pm)
	pm = *p_pm;
    SetupPrintMode(&pm);

    if (!si_list)
    {
	si_list = GetSizeofInfoMgrList();
	if (!si_list)
	    si_list = sizeof_info_default;
    }

    int n_tabs = 0, n_records = 0, n_elem = 0, n_cat = 0, str_size = 0;;
    for ( const sizeof_info_t **si_ptr = si_list; *si_ptr; si_ptr++ )
    {
	n_tabs++;
	for ( const sizeof_info_t *si = *si_ptr; si->size != -9; si++ )
	{
	    n_records++;
	    if ( si->size >= 0 )
		n_elem++;
	    else if ( si->size == -1 )
		n_cat++;
	    if (si->name)
		str_size += strlen(si->name) + 1;
	}
    }

    if ( pm.debug > 0 )
	fprintf(pm.flog,"\n%s> %u source list%s with %u record%s, %u categorie%s"
		" and %u element%s, %zu bytes total%s%s",
		pm.clog->info,
		n_tabs, n_tabs == 1 ? "" : "s",
		n_records, n_records == 1 ? "" : "s",
		n_cat, n_cat == 1 ? "" : "s",
		n_elem, n_elem == 1 ? "" : "s",
		(n_records+n_tabs) * sizeof(sizeof_info_t) + str_size,
		pm.clog->reset, pm.eol );

    ArgManager_t my_filter = {0}, *filter = &my_filter;

    if (p_filter)
    {
	if ( p_filter->force_case == LOUP_LOWER )
	    filter = p_filter;
	else
	{
	    for ( int i = 0; i < p_filter->argc; i++ )
		AppendArgManager(&my_filter,p_filter->argv[i],0,false);

	    for ( int i = 0; i < my_filter.argc; i++ )
	    {
		char *arg = my_filter.argv[i];
		StringLowerS(arg,strlen(arg)+1,arg);
	    }
	}
    }

    sizeof_info_t *list = 0, *list_end = 0;
    if (order)
	list_end = list = MALLOC( n_elem * sizeof(*list) );


    //--- get field widths

    int max_size = 0;
    for ( const sizeof_info_t **si_ptr = si_list; *si_ptr; si_ptr++ )
    {
	for ( const sizeof_info_t *si = *si_ptr; si->size != -9; si++ )
	{
	    if ( max_size < si->size && CheckFilterArgManager(filter,si->name) )
		max_size = si->size;
	}
    }

    char buf[100];
    const int fw_hex = snprintf(buf,sizeof(buf),"%#x",max_size);
    const int fw_dec = snprintf(buf,sizeof(buf),"%u",max_size);


    //--- print or collect

    ccp head = 0;
    int n_head = 0, n_sizeof = 0;

    for ( const sizeof_info_t **si_ptr = si_list; *si_ptr; si_ptr++ )
    {
	for ( const sizeof_info_t *si = *si_ptr; si->size != -9; si++ )
	{
	    if ( si->size >= 0 )
	    {
		if (CheckFilterArgManager(filter,si->name))
		{
		    n_sizeof++;
		    if (order)
			*list_end++ = *si;
		    else
		    {
			if (head)
			{
			    n_head++;
			    fprintf(pm.fout,"\n%s%s%s\n",pm.cout->caption,head,pm.cout->reset);
			    head = 0;
			}
			fprintf(pm.fout," %#*x = %*u : %s\n",
				fw_hex, si->size, fw_dec, si->size, si->name );
		    }
		}
	    }
	    else if ( si->size == -1 )
		head = si->name;
	    else if ( si->size == -2 )
		putchar('\n');
	}
    }


    //--- print ordered list

    if (order)
    {
	putchar('\n');
	if ( n_sizeof > 1 )
	{
	    if ( order == SIZEOF_ORDER_NAME )
		qsort(list,n_sizeof,sizeof(*list),(qsort_func)compare_sizeof_name);
	    else
		qsort(list,n_sizeof,sizeof(*list),(qsort_func)compare_sizeof_size);
	}

	ccp prev = "";
	for ( const sizeof_info_t *ptr = list; ptr < list_end; ptr++ )
	{
	    if (!strcmp(prev,ptr->name))
		fprintf(pm.fout,"%s %#*x = %*u : %s%s\n",
			    pm.cout->warn, fw_hex, ptr->size,
			    fw_dec, ptr->size, ptr->name, pm.cout->reset );
	    else
	    {
		fprintf(pm.fout," %#*x = %*u : %s\n",
			fw_hex, ptr->size, fw_dec, ptr->size, ptr->name );
		prev = ptr->name;
	    }
	}
    }


    //--- terminate

    ResetArgManager(&my_filter);

    if ( pm.debug > 0 )
    {
	putchar('\n');
	if (!n_sizeof)
	    fprintf(pm.flog,"%s> No elements printed.%s\n",pm.cout->info,pm.cout->reset);
	else
	{
	    fputs(pm.cout->info,stdout);
	    fputs("> ",stdout);
	    if (n_head)
		fprintf(pm.flog,"%u head line%s and ", n_head, n_head == 1 ? "" : "s" );

	    if ( n_sizeof < n_elem )
		fprintf(pm.flog,"%u of %u elements printed.%s\n",
			n_sizeof, n_elem, pm.cout->reset );
	    else
		fprintf(pm.flog,"%u element%s printed.%s\n",
			n_sizeof, n_sizeof == 1 ? "" : "s",
			pm.cout->reset );
	}
    }

    putchar('\n');
    FREE(list);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			misc				///////////////
///////////////////////////////////////////////////////////////////////////////

const IntervalInfo_t interval_info[] =
{
    { NSEC_PER_SEC,	USEC_PER_SEC,	"Second"	},
    { NSEC_PER_MIN,	USEC_PER_MIN,	"Minute"	},
    { NSEC_PER_HOUR,	USEC_PER_HOUR,	"Hour"		},
    { NSEC_PER_DAY,	USEC_PER_DAY,	"Day"		},
    { NSEC_PER_WEEK,	USEC_PER_WEEK,	"Week"		},
    { NSEC_PER_MONTH,	USEC_PER_MONTH,	"Month"		},
    { NSEC_PER_YEAR,	USEC_PER_YEAR,	"Year"		},
    { M1(u_nsec_t),	M1(u_usec_t),	"*"		},
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

