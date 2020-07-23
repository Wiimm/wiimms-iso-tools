
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

#ifndef WIT_WINAPI_H
#define WIT_WINAPI_H 1

#include "dclib/dclib-types.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			wit definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

struct FileMapItem_t;
struct FileMap_t;

const struct FileMapItem_t * AppendFileMap
(
    // returns the modified or appended item

    struct FileMap_t	*fm,		// file map pointer
    u64			src_off,	// offset of source
    u64			dest_off,	// offset of dest
    u64			size		// size
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get file mapping		///////////////
///////////////////////////////////////////////////////////////////////////////

int GetWinFileMap
(
    struct FileMap_t	*fm,		// valid pointer to to map
    int			fd,		// file descriptor
    u64			split_off,	// base offset of split file
    u64			file_size	// file size
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_WINAPI_H
