
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

#define _GNU_SOURCE 1

#include "winapi.h"
#include "dclib/dclib-debug.h"
#include "io.h"
#include <w32api/windows.h>
//#include <w32api/winioctl.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get file mapping		///////////////
///////////////////////////////////////////////////////////////////////////////
// FSCTL_GET_RETRIEVAL_POINTERS
//	http://msdn.microsoft.com/en-us/library/aa364572%28v=vs.85%29.aspx
///////////////////////////////////////////////////////////////////////////////

int GetWinFileMap
(
    struct FileMap_t	*fm,		// valid pointer to to map
    int			fd,		// file descriptor
    u64			split_off,	// base offset of split file
    u64			file_size	// file size
)
{
    long handle = get_osfhandle(fd);
    PRINT("FD: %d -> %ld\n",fd,handle);

    STARTING_VCN_INPUT_BUFFER start;
    memset(&start,0,sizeof(start));

    u8 rawbuf[100000];
    RETRIEVAL_POINTERS_BUFFER *buf = (RETRIEVAL_POINTERS_BUFFER*)rawbuf;

    DWORD written;
    int stat = DeviceIoControl( (HANDLE)handle,
				FSCTL_GET_RETRIEVAL_POINTERS,
				&start, sizeof(start),
				&rawbuf, sizeof(rawbuf),
				&written, 0 );
    int last_err = GetLastError();
    PRINT("stat=%d,%d [moredata=%ld], wr=%ld, size=%zu\n",
	stat,last_err,ERROR_MORE_DATA, written,sizeof(rawbuf));

    if ( !stat && last_err == ERROR_MORE_DATA )
    {
	uint bufsize = 1000000;
	for(;;)
	{
	    buf = MALLOC(bufsize);
	    stat = DeviceIoControl( (HANDLE)handle,
					FSCTL_GET_RETRIEVAL_POINTERS,
					&start, sizeof(start),
					buf, bufsize,
					&written, 0 );
	    last_err = GetLastError();
	    PRINT("stat=%d,%d [moredata=%ld], wr=%ld, size=%zu\n",
		stat,last_err,ERROR_MORE_DATA, written, bufsize);

	    if ( stat || last_err != ERROR_MORE_DATA )
		break;
	    FREE(buf);
	    bufsize *= 2;
	}
    }

    int err = 1;
    if ( stat && buf->ExtentCount )
    {
	err = 0;
	uint blocksize = file_size / buf->Extents[buf->ExtentCount-1].NextVcn.QuadPart;
	PRINT("file size=%lld, last_cluster = %lld, cluster_size = %d\n",
		file_size, buf->Extents[buf->ExtentCount-1].NextVcn.QuadPart,
		blocksize );

	uint i;
	u64 block = 0;
	for ( i = 0; i < buf->ExtentCount; i++ )
	{
	    noPRINT("%15lld %11lld\n",
		    buf->Extents[i].NextVcn.QuadPart,
		    buf->Extents[i].Lcn.QuadPart );

	    const u64 next_block = buf->Extents[i].NextVcn.QuadPart;
	    AppendFileMap( fm, block * blocksize + split_off,
				buf->Extents[i].Lcn.QuadPart * blocksize,
				( next_block - block ) *  blocksize );
	    block = next_block;
	}
    }

    if ( (u8*)buf != rawbuf )
	FREE(buf);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

