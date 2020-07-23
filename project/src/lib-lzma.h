
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

#ifndef WIT_LIB_LZMA_H
#define WIT_LIB_LZMA_H 1

#define _GNU_SOURCE 1

#include "lib-std.h"
#include "lib-sf.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct EncLZMA_t
{
    void *		handle;		// lzma handle
    u8			enc_props[8];	// encoded properties
    size_t		enc_props_len;	// used length of 'enc_props'
    int			compr_level;	// active compression level
    ccp			error_object;	// object name for error messages

} EncLZMA_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			LZMA helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetMessageLZMA
(
    int			err,		// error code
    ccp			unkown_error	// result for unkown error codes
);

//-----------------------------------------------------------------------------

void InitializeLZMA
(
    EncLZMA_t		* lzma		// valid object to initialize
);

//-----------------------------------------------------------------------------

int CalcCompressionLevelLZMA
(
    int			compr_level	// valid are 1..9 / 0: use default value
);

//-----------------------------------------------------------------------------

u32 CalcMemoryUsageLZMA
(
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		is_writing	// false: reading mode, true: writing mode
);

//-----------------------------------------------------------------------------

u32 CalcMemoryUsageLZMA2
(
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		is_writing	// false: reading mode, true: writing mode
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		   LZMA encoding (compression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA_Open
(
    EncLZMA_t		* lzma,		// object, will be initialized
    ccp			error_object,	// object name for error messages
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_endmark	// true: write end marker at end of stream
);

//-----------------------------------------------------------------------------

enumError EncLZMA_WriteDataToFile
(
    EncLZMA_t		* lzma,		// valid pointer, opened with EncLZMA_Open()
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    bool		write_props,	// true: write encoding properties
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
);

//-----------------------------------------------------------------------------

enumError EncLZMA_WriteList2File
(
    EncLZMA_t		* lzma,		// valid pointer, opened with EncLZMA_Open()
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    bool		write_props,	// true: write encoding properties
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
);

//-----------------------------------------------------------------------------

enumError EncLZMA_Close
(
    EncLZMA_t		* lzma		// valid pointer
);

//-----------------------------------------------------------------------------

enumError EncLZMA_Data2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
);

//-----------------------------------------------------------------------------

enumError EncLZMA_List2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		 LZMA decoding (decompression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DecLZMA_File2Buf // open + read + close lzma stream
(
    SuperFile_t		* file,		// source file and progress support
					//  -> read from 'file->f' at current offset
    size_t		read_count,	// not NULL: max bytes to read from file
    void		* buf,		// destination buffer
    size_t		buf_size,	// size of destination buffer
    u32			* bytes_written,// not NULL: store bytes written to buf
    u8			* enc_props	// Encoding properties
					// If NULL: read it from file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		   LZMA2 encoding (compression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError EncLZMA2_Open
(
    EncLZMA_t		* lzma,		// object, will be initialized
    ccp			error_object,	// object name for error messages
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_endmark	// true: write end marker at end of stream
);

//-----------------------------------------------------------------------------

enumError EncLZMA2_WriteDataToFile
(
    EncLZMA_t		* lzma,		// valid pointer, opened with EncLZMA2_Open()
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    bool		write_props,	// true: write encoding properties
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
);

//-----------------------------------------------------------------------------

enumError EncLZMA2_WriteList2File
(
    EncLZMA_t		* lzma,		// valid pointer, opened with EncLZMA2_Open()
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    bool		write_props,	// true: write encoding properties
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
);

//-----------------------------------------------------------------------------

enumError EncLZMA2_Close
(
    EncLZMA_t		* lzma		// valid pointer
);

//-----------------------------------------------------------------------------

enumError EncLZMA2_Data2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    const void		* data,		// data to write
    size_t		data_size,	// size of data to write
    u32			* bytes_written	// not NULL: store written bytes
);

//-----------------------------------------------------------------------------

enumError EncLZMA2_List2File // open + write + close lzma stream
(
    EncLZMA_t		* lzma,		// if NULL: use internal structure
    SuperFile_t		* file,		// destination file and progress support
					//  -> write to 'file->f' at current offset
    int			compr_level,	// valid are 1..9 / 0: use default value
    bool		write_props,	// true: write encoding properties
    bool		write_endmark,	// true: write end marker at end of stream
    DataList_t		* data_list,	// NULL or data list (modified)
    u32			* bytes_written	// not NULL: store written bytes
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		 LZMA2 decoding (decompression)		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError DecLZMA2_File2Buf // open + read + close lzma stream
(
    SuperFile_t		* file,		// source file and progress support
					//  -> read from 'file->f' at current offset
    size_t		read_count,	// not NULL: max bytes to read from file
    void		* buf,		// destination buffer
    size_t		buf_size,	// size of destination buffer
    u32			* bytes_written,// not NULL: store bytes written to buf
    u8			* enc_props	// Encoding properties
					// If NULL: read it from file
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_LIB_LZMA_H 1
