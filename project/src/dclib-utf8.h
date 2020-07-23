
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

#ifndef __LIBDC_UTF8_H
#define __LIBDC_UTF8_H 1

#include "dclib/dclib-types.h"

///////////////////////////////////////////////////////////////////////////////
/////   this software is taken from dcLib2 and now publiced under GPL2.   /////
///////////////////////////////////////////////////////////////////////////////

typedef enum dcUnicodeConsts
{
    DC_UNICODE_MAX_UTF8_1	= 0x7f,
    DC_UNICODE_MAX_UTF8_2	= 0x7ff,
    DC_UNICODE_MAX_UTF8_3	= 0xffff,
    DC_UNICODE_MAX_UTF8_4	= 0x1fffff,

    DC_UNICODE_CODE_MASK	= 0x1fffff,

} dcUnicodeConsts;

///////////////////////////////////////////////////////////////////////////////

typedef enum dcUTF8Mode
{
	DC_UTF8_ILLEGAL		= 0x0000, // Illegale UTF8 Zeichen-Kombination

	DC_UTF8_1CHAR		= 0x0001, // Das Zeichen ist ein Einzelzeichen

	DC_UTF8_2CHAR		= 0x0002, // Beginn einer 2-Zeichen Sequenz
	DC_UTF8_CONT_22		= 0x0004, // ein Fortsetzungszeichen an Pos 2 einer 2-er Sequenz

	DC_UTF8_3CHAR		= 0x0008, // Beginn einer 3-Zeichen Sequenz
	DC_UTF8_CONT_23		= 0x0010, // ein Fortsetzungszeichen an Pos 2 einer 3-er Sequenz
	DC_UTF8_CONT_33		= 0x0020, // ein Fortsetzungszeichen an Pos 3 einer 3-er Sequenz

	DC_UTF8_4CHAR		= 0x0040, // Beginn einer 4-Zeichen Sequenz
	DC_UTF8_CONT_24		= 0x0080, // ein Fortsetzungszeichen an Pos 2 einer 4-er Sequenz
	DC_UTF8_CONT_34		= 0x0100, // ein Fortsetzungszeichen an Pos 3 einer 4-er Sequenz
	DC_UTF8_CONT_44		= 0x0200, // ein Fortsetzungszeichen an Pos 4 einer 4-er Sequenz

	DC_UTF8_CONT_ANY	= 0x0400, // ein Fortsetzungszeichen an beliebger Stelle

	DC_UTF8_1CHAR_POSSIBLE	= 0x0800, // als Einzelzeichen darstellbar
	DC_UTF8_2CHAR_POSSIBLE	= 0x1000, // als 2-er Sequenz darstellbar
	DC_UTF8_3CHAR_POSSIBLE	= 0x2000, // als 3-er Sequenz darstellbar
	DC_UTF8_4CHAR_POSSIBLE	= 0x4000, // als 4-er Sequenz darstellbar
} dcUTF8Mode;

///////////////////////////////////////////////////////////////////////////////

extern const unsigned short TableUTF8Mode[256];
dcUTF8Mode CheckUTF8Mode ( unsigned char ch );

int	GetUTF8CharLength	( ulong code );
char *	NextUTF8Char		( ccp str );
char *	NextUTF8CharE		( ccp str, ccp end );
char *	PrevUTF8Char		( ccp str );
char *  PrevUTF8CharB		( ccp str, ccp begin );
ulong	GetUTF8Char		( ccp str );
ulong	ScanUTF8Char		( ccp * str );
ulong	ScanUTF8CharE		( ccp * str, ccp end );
ulong	ScanUTF8CharInc		( ccp * str );
ulong	ScanUTF8CharIncE	( ccp * str, ccp end );
ulong	GetUTF8AnsiChar		( ccp str );
ulong	ScanUTF8AnsiChar	( ccp * str );
ulong	ScanUTF8AnsiCharE	( ccp * str, ccp end );
int	ScanUTF8Length		( ccp str, ccp end );
char *	PrintUTF8Char		( char * buf, ulong code );

///////////////////////////////////////////////////////////////////////////////

typedef struct dcUnicodeTripel
{
	ulong code1;
	ulong code2;
	ulong code3;
} dcUnicodeTripel;

extern const dcUnicodeTripel TableUnicodeDecomp[];
const dcUnicodeTripel * DecomposeUnicode ( ulong code );

///////////////////////////////////////////////////////////////////////////////

#endif //__LIBDC_UTF8_H

