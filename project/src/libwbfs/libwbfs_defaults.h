
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
 *   Copyright (c) 2009 Kwiirk                                             *
 *   Copyright (c) 2009-2021 by Dirk Clemens <wiimm@wiimm.de>              *
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
  
#ifndef LIBWBFS_DEFAULTS_H
#define LIBWBFS_DEFAULTS_H

//
///////////////////////////////////////////////////////////////////////////////
///////////////			define error messages		///////////////
///////////////////////////////////////////////////////////////////////////////

#ifndef WD_ERROR
    #define WD_ERROR(...) wd_print_error(__FUNCTION__,__FILE__,__LINE__,__VA_ARGS__)
#endif

#ifndef OUT_OF_MEMORY
    #define OUT_OF_MEMORY WD_ERROR(ERR_OUT_OF_MEMORY,"Out of memory")
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			define trace macros		///////////////
///////////////////////////////////////////////////////////////////////////////
// This are macros used by WIT for debugging and tracing the code
// If not already defined => define them as empty macros

#ifndef PRINT
    #define PRINT(...)
#endif

#ifndef PRINT_IF
    #define PRINT_IF(cond,...)
#endif

#ifndef BINGO
    #define BINGO
#endif

#ifndef TRACE
    #define TRACE(...)
#endif

#ifndef TRACE_IF
    #define TRACE_IF(cond,...)
#endif

#ifndef TRACELINE
    #define TRACELINE
#endif

#ifndef TRACE_SIZEOF
    #define TRACE_SIZEOF(t)
#endif

#ifndef HEXDUMP
    #define HEXDUMP(i,a,af,rl,d,c)
#endif

#ifndef HEXDUMP16
    #define HEXDUMP16(a,i,d,c)
#endif

#ifndef TRACE_HEXDUMP
    #define TRACE_HEXDUMP(i,a,af,rl,d,c)
#endif

#ifndef TRACE_HEXDUMP16
    #define TRACE_HEXDUMP16(i,a,d,c)
#endif

#ifndef ASSERT
    #define ASSERT(cond)
#endif

#ifndef ASSERT_MSG
    #define ASSERT_MSG(a,...)
#endif

#ifndef DASSERT
    #define DASSERT(cond)
#endif

#ifndef DASSERT_MSG
    #define DASSERT_MSG(a,...)
#endif

#ifndef noTRACE
    #define noTRACE(...)
#endif

#ifndef noTRACE_IF
    #define noTRACE_IF(cond,...)
#endif

#ifndef noTRACELINE
    #define noTRACELINE
#endif

#ifndef noTRACE_SIZEOF
    #define noTRACE_SIZEOF(t)
#endif

#ifndef noASSERT
    #define noASSERT(cond)
#endif

#ifndef noASSERT_MSG
    #define noASSERT_MSG(cond,...)
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////				E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // LIBWBFS_DEFAULTS_H
