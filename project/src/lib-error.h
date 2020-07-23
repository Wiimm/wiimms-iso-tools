
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

#ifndef WIT_ERROR_H
#define WIT_ERROR_H 1

//
///////////////////////////////////////////////////////////////////////////////
///////////////                  error messages                 ///////////////
///////////////////////////////////////////////////////////////////////////////

//--- error group 1

#define ERR_NO_WDF		ERU_ERROR1_00
#define ERR_WDF_VERSION		ERU_ERROR1_01
#define ERR_WDF_SPLIT		ERU_ERROR1_02
#define ERR_WDF_INVALID		ERU_ERROR1_03

#define ERR_NO_CISO		ERU_ERROR1_04
#define ERR_CISO_INVALID	ERU_ERROR1_05

#define ERR_NO_GCZ		ERU_ERROR1_06
#define ERR_GCZ_INVALID		ERU_ERROR1_07

#define ERR_WPART_INVALID	ERU_ERROR1_08
#define ERR_WDISC_INVALID	ERU_ERROR1_09
#define ERR_WDISC_NOT_FOUND	ERU_ERROR1_10

#define ERR_NO_WBFS_FOUND	ERU_ERROR1_11
#define ERR_TO_MUCH_WBFS_FOUND	ERU_ERROR1_12
#define ERR_WBFS_INVALID	ERU_ERROR1_13

#define ERR_NO_WIA		ERU_ERROR1_14
#define ERR_WIA_INVALID		ERU_ERROR1_15
#define ERR_BZIP2		ERU_ERROR1_16
#define ERR_LZMA		ERU_ERROR1_17

//--- error group 2

#define ERR_WBFS		ERU_ERROR2_00

//-----------------------------------------------------------------------------

ccp LibGetErrorName ( int stat, ccp ret_not_found );
ccp LibGetErrorText ( int stat, ccp ret_not_found );

//
///////////////////////////////////////////////////////////////////////////////
///////////////                     END                         ///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_ERROR_H 1
