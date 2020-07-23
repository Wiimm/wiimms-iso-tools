/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2009-2010 by Dirk Clemens <wiimm@wiimm.de>              *
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

#include "libwbfs.h"
#include "wiidisc.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int main ( int argc, char ** argv )
{
    printf("hello world\n");

 #if 1 // test warnings
    wbfs_warning(0);
    wbfs_warning("%d Warning",1);
    wbfs_error("Error");
    OUT_OF_MEMORY;
    wbfs_fatal("should not reached");
 #endif

    return 0;
}

