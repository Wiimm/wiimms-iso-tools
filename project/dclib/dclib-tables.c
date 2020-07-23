
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

#define _GNU_SOURCE 1

//#include <string.h>
//#include <unistd.h>
//#include <time.h>
//#include <stdio.h>
//#include <errno.h>
//#include <sys/time.h>

#include "dclib-basics.h"
//#include "dclib-debug.h"
//#include "dclib-utf8.h"

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
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

