
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

#ifndef DCLIB_DEBUG_H
#define DCLIB_DEBUG_H 1

#include "dclib-types.h"
#include <stdio.h>
#include <stdarg.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			error handling			///////////////
///////////////////////////////////////////////////////////////////////////////

extern ccp progpath;	// full path of program, use ProgramPath() to access
extern ccp progdir;	// directory part of progpath, use ProgramDirectory() to access
extern ccp progname;	// program name, based on path or directly set

extern ccp toolname;	// name of the tool, set by SetupProgname()
extern ccp toolversion;	// version of the tool, set by SetupProgname()
extern ccp tooltitle;	// title of the tool, set by SetupProgname()

extern bool multi_processing;
extern enumError last_error;
extern enumError max_error;
extern u32 error_count;

//-----------------------------------------------------------------------------

extern ccp (*GetErrorNameHook)( int stat, ccp ret_not_found );
extern ccp (*GetErrorTextHook)( int stat, ccp ret_not_found );
extern ccp GENERIC_ERROR_MESSAGE;

ccp GetErrorName
(
    int		stat,		// error status, err := abs(stat)
    ccp		ret_not_found	// not NULL: return this, if no message is found
				//     NULL: return a generic message
);

ccp GetErrorText
(
    int		stat,		// error status, err := abs(stat)
    ccp		ret_not_found	// not NULL: return this, if no message is found
				//     NULL: return a generic message
);

void ListErrorCodes
(
    FILE	* f,		// valid output stream
    int		indent,		// indent of output
    bool	all		// true: print all user reserved messages too.
);

enumError PrintErrorArg ( ccp func, ccp file, unsigned int line,
		int syserr, enumError err_code, ccp format, va_list arg );

enumError PrintError ( ccp func, ccp file, unsigned int line,
		int syserr, enumError err_code, ccp format, ... )
		__attribute__ ((__format__(__printf__,6,7)));

struct File_t;
enumError PrintErrorFile ( ccp func, ccp file, uint line, struct File_t *F,
		int syserr, enumError err_code, ccp format, ... )
		__attribute__ ((__format__(__printf__,7,8)));

enumError PrintErrorStat ( enumError err, int verbose, ccp cmdname );

#undef ERROR
#undef ERROR0
#undef ERROR1
#undef FILEERROR
#undef FILEERROR0
#undef FILEERROR1

#define ERROR(se,code,...) PrintError(__FUNCTION__,__FILE__,__LINE__,se,code,__VA_ARGS__)
#define ERROR0(code,...) PrintError(__FUNCTION__,__FILE__,__LINE__,0,code,__VA_ARGS__)
#define ERROR1(code,...) PrintError(__FUNCTION__,__FILE__,__LINE__,errno,code,__VA_ARGS__)

#define FILEERROR(f,se,code,...) \
	PrintErrorFile(__FUNCTION__,__FILE__,__LINE__,f,se,code,__VA_ARGS__)
#define FILEERROR0(f,code,...) \
	PrintErrorFile(__FUNCTION__,__FILE__,__LINE__,f,0,code,__VA_ARGS__)
#define FILEERROR1(f,code,...) \
	PrintErrorFile(__FUNCTION__,__FILE__,__LINE__,f,errno,code,__VA_ARGS__)

//-----------------------------------------------------------------------------

void mark_used ( ccp name, ... );
#define MARK_USED(...) mark_used(0,__VA_ARGS__)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    hexdump			///////////////
///////////////////////////////////////////////////////////////////////////////

// 0: don't use XDump
// 1: use 'enable_xdump_wrapper' to decide and init it with FALSE
// 2: use 'enable_xdump_wrapper' to decide and init it with TRUE
// 3: force usage of XDump

#ifndef ENABLE_HEXDUMP_WRAPPER
  #ifdef TEST
    #define ENABLE_HEXDUMP_WRAPPER 2
  #else
    #define ENABLE_HEXDUMP_WRAPPER 1
  #endif
#endif

///////////////////////////////////////////////////////////////////////////////

#if ENABLE_HEXDUMP_WRAPPER
 extern int enable_xdump_wrapper;
#endif

extern ccp  hexdump_prefix;
extern ccp  hexdump_eol;
extern bool hexdump_align;

// HexDump() and HexDump16() return the number of printed lines

uint HexDump ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * data, size_t count );

uint HexDump16 ( FILE * f, int indent, u64 addr,
		const void * data, size_t count );

void HexDiff ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * p_data1, size_t count1,
		const void * p_data2, size_t count2 );

void HexDiff16 ( FILE * f, int indent, u64 addr,
		 const void * data1, size_t count1,
		 const void * data2, size_t count2 );
//
///////////////////////////////////////////////////////////////////////////////
///////////////			DEBUG and TRACING		///////////////
///////////////////////////////////////////////////////////////////////////////
//
//  -------------------
//   DEBUG and TRACING
//  -------------------
//
//  If symbol 'DEBUG' or symbol _DEBUG' is defined, then and only then
//  DEBUG, DASSERT and TRACING are enabled.
//
//  There are to function like macros defined:
//
//     TRACE ( ccp format, ... );
//        Print to console only if TRACING is enabled.
//        Ignored when TRACING is disabled.
//        It works like the well known printf() function and include flushing.
//
//     TRACE_IF ( bool condition, ccp format, ... );
//        Like TRACE(), but print only if 'condition' is true.
//
//     TRACELINE
//        Print out current line and source.
//
//     TRACE_SIZEOF ( object_or_type );
//        Print out `sizeof(object_or_type)´
//
//     ASSERT(cond);
//	  If condition is false: print info and exit program.
//
//     DASSERT(cond);
//	  Like ASSERT; but only active on DEBUG.
//
//     BINGO
//        Like TRACELINE, but print to stderr instead custom file.
//
//
//  There are more macros with a preceding 'no' defined:
//
//      noTRACE ( ccp format, ... );
//      noTRACE_IF ( bool condition, ccp format, ... );
//      noTRACELINE ( bool condition, ccp format, ... );
//      noTRACE_SIZEOF ( object_or_type );
//      noASSERT(cond);
//
//      If you add a 'no' before a TRACE-Call it is disabled always.
//      This makes the usage and disabling of multi lines traces very easy.
//
//
//  The macros with a preceding 'x' are always active:
//	xTRACELINE
//	xBINGO
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#if defined(RELEASE)
    #undef TESTTRACE
    #undef DEBUG
    #undef _DEBUG
    #undef DEBUG_ASSERT
    #undef _DEBUG_ASSERT
    #undef WAIT_ENABLED
#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(IGNORE_DEBUG)
    #undef DEBUG
    #undef _DEBUG
#endif

///////////////////////////////////////////////////////////////////////////////

#undef TRACE
#undef TRACE_IF
#undef TRACELINE
#undef TRACE_SIZEOF

///////////////////////////////////////////////////////////////////////////////

#ifdef WIN_SZS_LIB
  #define __attribute__(x)
#endif

extern ccp TRACE_PREFIX; // never NULL
extern FILE *TRACE_FILE;
extern FILE *MEM_LOG_FILE;

void TRACE_FUNC ( ccp format, ... )
	__attribute__ ((__format__(__printf__,1,2)));
void TRACE_ARG_FUNC ( ccp format, va_list arg );

void PRINT_FUNC ( ccp format, ... )
	__attribute__ ((__format__(__printf__,1,2)));
void PRINT_ARG_FUNC ( ccp format, va_list arg );

void BINGO_FUNC ( ccp func, int line, ccp src );

void WAIT_FUNC ( ccp format, ... )
	__attribute__ ((__format__(__printf__,1,2)));
void WAIT_ARG_FUNC ( ccp format, va_list arg );

///////////////////////////////////////////////////////////////////////////////

#undef TEST0 // never defined

#ifdef TESTTRACE

    #undef DEBUG
    #define DEBUG 1

    #undef TEST
    #define TEST 1

    #undef WAIT_ENABLED
    #define WAIT_ENABLED 1

    #undef NEW_FEATURES
    #define NEW_FEATURES 1

    #define TRACE_FUNC printf
    #define PRINT_FUNC printf
    #define WAIT_FUNC  printf

    #define TRACE_ARG_FUNC vprintf
    #define PRINT_ARG_FUNC vprintf
    #define WAIT_ARG_FUNC  vprintf

#endif

///////////////////////////////////////////////////////////////////////////////

#if defined(DEBUG) || defined(_DEBUG)

    #define HAVE_TRACE 1

    #undef DEBUG
    #define DEBUG 1

    #undef DEBUG_ASSERT
    #define DEBUG_ASSERT 1

    #define TRACE(...) TRACE_FUNC(__VA_ARGS__)
    #define TRACE_IF(cond,...) if (cond) TRACE_FUNC(__VA_ARGS__)
    #define TRACELINE TRACE_FUNC("%s() line #%d @ %s\n",__FUNCTION__,__LINE__,__FILE__)
    #define TRACE_SIZEOF(t) TRACE_FUNC("%7zd ==%6zx/hex == sizeof(%s)\n",sizeof(t),sizeof(t),#t)

    #define HEXDUMP(i,a,af,rl,d,c) HexDump(stdout,i,a,af,rl,d,c);
    #define HEXDUMP16(i,a,d,c) HexDump16(stdout,i,a,d,c);
    #define TRACE_HEXDUMP(i,a,af,rl,d,c) HexDump(TRACE_FILE,i,a,af,rl,d,c);
    #define TRACE_HEXDUMP16(i,a,d,c) HexDump16(TRACE_FILE,i,a,d,c);

    #define HEXDIFF(i,a,af,rl,d1,c1,d2,c2) HexDiff(stdout,i,a,af,rl,d1,c1,d2,c2);
    #define HEXDIFF16(i,a,d1,c1,d2,c2) HexDiff16(stdout,i,a,d1,c1,d2,c2);
    #define TRACE_HEXDIFF(i,a,af,rl,d1,c1,d2,c2) HexDiff(TRACE_FILE,i,a,af,rl,d1,c1,d2,c2);
    #define TRACE_HEXDIFF16(i,a,d1,c1,d2,c2) HexDiff16(TRACE_FILE,i,a,d1,c1,d2,c2);

#else

    #define HAVE_TRACE 0

    #undef DEBUG

    #define TRACE(...)
    #define TRACE_IF(cond,...)
    #define TRACELINE
    #define TRACE_SIZEOF(t)
    #define HEXDUMP(i,a,af,rl,d,c)
    #define HEXDUMP16(a,i,d,c)
    #define TRACE_HEXDUMP(i,a,af,rl,d,c)
    #define TRACE_HEXDUMP16(i,a,d,c)
    #define HEXDIFF(i,a,af,rl,d1,c1,d2,c2)
    #define HEXDIFF16(a,i,d1,c1,d2,c2)
    #define TRACE_HEXDIFF(i,a,af,rl,d1,c1,d2,c2)
    #define TRACE_HEXDIFF16(i,a,d1,c1,d2,c2)

#endif

///////////////////////////////////////////////////////////////////////////////

#undef ASSERT
#undef ASSERT_MSG

#if defined(DEBUG_ASSERT) || defined(_DEBUG_ASSERT) || defined(TEST)

    #define HAVE_ASSERT 1

    #undef DEBUG_ASSERT
    #define DEBUG_ASSERT 1

    #define ASSERT(a) if (!(a)) ERROR0(ERR_FATAL,"ASSERTION FAILED !!!\n")
    #define ASSERT_MSG(a,...) if (!(a)) ERROR0(ERR_FATAL,__VA_ARGS__)

#else

    #define HAVE_ASSERT 0

    #undef DEBUG_ASSERT
    #define ASSERT(cond)
    #define ASSERT_MSG(a,...)

#endif

///////////////////////////////////////////////////////////////////////////////

#undef PRINT
#undef PRINT_IF
#undef BINGO
#undef xBINGO
#undef HAVE_PRINT
#undef PRINT_TIME

#undef HAVE_PRINT0	// always false
#define HAVE_PRINT0 0

#if defined(DEBUG) && defined(TEST)

    #define HAVE_PRINT 1

    #define PRINT(...) PRINT_FUNC(__VA_ARGS__)
    #define PRINT_IF(cond,...) if (cond) PRINT_FUNC(__VA_ARGS__)
    #define BINGO BINGO_FUNC(__FUNCTION__,__LINE__,__FILE__)

    void PRINT_TIME ( time_t time, ccp title );

#else

    #define HAVE_PRINT	HAVE_TRACE

    #define PRINT	TRACE
    #define PRINT_IF	TRACE_IF
    #define BINGO	TRACELINE

    #define PRINT_TIME(...)

#endif

#define xBINGO BINGO_FUNC(__FUNCTION__,__LINE__,__FILE__)
#define xTRACELINE TRACE_FUNC("%s() line #%d @ %s\n",__FUNCTION__,__LINE__,__FILE__)

#undef PRINTF
#define PRINTF PRINT

///////////////////////////////////////////////////////////////////////////////

#undef WAIT
#undef WAIT_IF
#undef HAVE_WAIT

#if defined(WAIT_ENABLED)

    #define HAVE_WAIT 1

    #define WAIT(...) WAIT_FUNC(__VA_ARGS__)
    #define WAIT_IF(cond,...) if (cond) WAIT_FUNC(__VA_ARGS__)

#else

    #define HAVE_WAIT 0

    #define WAIT(...)
    #define WAIT_IF(cond,...)

#endif

///////////////////////////////////////////////////////////////////////////////

#undef DASSERT
#undef DASSERT_MSG
#undef HAVE_DASSERT

#if defined(DEBUG) || defined(TEST)

    #define HAVE_DASSERT 1

    #define DASSERT ASSERT
    #define DASSERT_MSG ASSERT_MSG

#else

    #define HAVE_DASSERT 0

    #define DASSERT(cond)
    #define DASSERT_MSG(a,...)

#endif

///////////////////////////////////////////////////////////////////////////////

// prefix 'no' deactivates traces

#undef noTRACE
#undef noTRACE_IF
#undef noTRACELINE
#undef noTRACE_SIZEOF
#undef noPRINT
#undef noPRINT_IF
#undef noPRINT_TIME
#undef noWAIT
#undef noWAIT_IF
#undef noASSERT

#ifdef TESTTRACE

    #define noTRACE		TRACE
    #define noTRACE_IF		TRACE_IF
    #define noTRACELINE		TRACELINE
    #define noTRACE_SIZEOF	TRACE_SIZEOF
    #define noPRINT		PRINT
    #define noPRINT_IF		PRINT_IF
    #define noPRINT_TIME	PRINT_TIME
    #define noWAIT		WAIT
    #define noWAIT_IF		WAIT_IF
    #define noASSERT		ASSERT
    #define noASSERT_MSG	ASSERT_MSG

#else

    #define noTRACE(...)
    #define noTRACE_IF(cond,...)
    #define noTRACELINE
    #define noTRACE_SIZEOF(t)
    #define noPRINT(...)
    #define noPRINT_IF(...)
    #define noPRINT_TIME(...)
    #define noWAIT(...)
    #define noWAIT_IF(...)
    #define noASSERT(cond)
    #define noASSERT_MSG(cond,...)

#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			alloc/free system		///////////////
///////////////////////////////////////////////////////////////////////////////

// define TRACE_ALLOC_MODE
//
//  0: standard malloc without 'out of memory' detection (don't use it!)
//  1: standard malloc with 'out of memory' detection
//  2: standard malloc with 'out of memory' detection and source identification
//  3: like mode #1, but alloc debuging enabled too
//  4: like mode #2, but alloc tracing enabled too

// define ENABLE_MEM_CHECK (check specific memory areas)
//  0: disabled
//  1: enable a watch point for a memory location

#ifndef TRACE_ALLOC_MODE
 #ifdef MEMDEBUG
    #define TRACE_ALLOC_MODE 4
 #elif TEST
    #define TRACE_ALLOC_MODE 3
 #elif DEBUG
    #define TRACE_ALLOC_MODE 2
 #else
    #define TRACE_ALLOC_MODE 1
 #endif
#endif

#ifndef ENABLE_MEM_CHECK
    #define ENABLE_MEM_CHECK 0
#endif

///////////////////////////////////////////////////////////////////////////////

#if ENABLE_MEM_CHECK
    extern void MemCheckSetup ( const void * ptr, unsigned int size );
    extern void MemCheck ( ccp func, ccp file, unsigned int line );
    #define MEM_CHECK_SETUP(p,s) MemCheckSetup(p,s)
    #define MEM_CHECK MemCheck(__FUNCTION__,__FILE__,__LINE__)
#else
    #define MEM_CHECK_SETUP(p,s)
    #define MEM_CHECK
#endif

///////////////////////////////////////////////////////////////////////////////

void dclib_free ( void * ptr );

void * dclib_xcalloc  ( size_t nmemb, size_t size );
void * dclib_xmalloc  ( size_t size );
void * dclib_xrealloc ( void * ptr, size_t size );
char * dclib_xstrdup  ( ccp src );

#if TRACE_ALLOC_MODE > 1
    void * dclib_calloc  ( ccp,ccp,uint, size_t nmemb, size_t size );
    void * dclib_malloc  ( ccp,ccp,uint, size_t size );
    void * dclib_realloc ( ccp,ccp,uint, void * ptr, size_t size );
    char * dclib_strdup  ( ccp,ccp,uint, ccp src );
    char * dclib_strdup2 ( ccp,ccp,uint, ccp src1, ccp src2 );
    char * dclib_strdup3 ( ccp,ccp,uint, ccp src1, ccp src2, ccp src3 );
    void * dclib_memdup  ( ccp,ccp,uint, const void * src, size_t copylen );
#else
    void * dclib_calloc  ( size_t nmemb, size_t size );
    void * dclib_malloc  ( size_t size );
    void * dclib_realloc ( void * ptr, size_t size );
    char * dclib_strdup  ( ccp src );
    char * dclib_strdup2 ( ccp src1, ccp src2 );
    char * dclib_strdup3 ( ccp src1, ccp src2, ccp src3 );
    void * dclib_memdup  ( const void * src, size_t copylen );
#endif

uint   trace_test_alloc	( ccp,ccp,uint, const void * ptr, bool hexdump );
void   trace_free	( ccp,ccp,uint, void * ptr );
void * trace_calloc	( ccp,ccp,uint, size_t nmemb, size_t size );
void * trace_malloc	( ccp,ccp,uint, size_t size );
void * trace_realloc	( ccp,ccp,uint, void *ptr, size_t size );
char * trace_strdup	( ccp,ccp,uint, ccp src );
char * trace_strdup2	( ccp,ccp,uint, ccp src1, ccp src2 );
char * trace_strdup3	( ccp,ccp,uint, ccp src1, ccp src2, ccp src3 );
void * trace_memdup	( ccp,ccp,uint, const void * src, size_t copylen );

#if TRACE_ALLOC_MODE > 2
    void InitializeTraceAlloc();
    int  CheckTraceAlloc ( ccp func, ccp file, unsigned int line );
    void DumpTraceAlloc ( ccp func, ccp file, unsigned int line, FILE * f );
    struct mem_info_t *RegisterAlloc
	( ccp func, ccp file, uint line, cvp data, uint size, bool filler );
#endif

///////////////////////////////////////////////////////////////////////////////

#if TRACE_ALLOC_MODE > 2
    #define TEST_ALLOC(ptr,hexd) trace_test_alloc(__FUNCTION__,__FILE__,__LINE__,ptr,hexd)
    #define FREE(ptr) trace_free(__FUNCTION__,__FILE__,__LINE__,ptr)
    #define CALLOC(nmemb,size) trace_calloc(__FUNCTION__,__FILE__,__LINE__,nmemb,size)
    #define MALLOC(size) trace_malloc(__FUNCTION__,__FILE__,__LINE__,size)
    #define REALLOC(ptr,size) trace_realloc(__FUNCTION__,__FILE__,__LINE__,ptr,size)
    #define STRDUP(src) trace_strdup(__FUNCTION__,__FILE__,__LINE__,src)
    #define STRDUP2(src1,src2) trace_strdup2(__FUNCTION__,__FILE__,__LINE__,src1,src2)
    #define STRDUP3(src1,src2,src3) trace_strdup3(__FUNCTION__,__FILE__,__LINE__,src1,src2,src3)
    #define MEMDUP(src,size) trace_memdup(__FUNCTION__,__FILE__,__LINE__,src,size)
    #define INIT_TRACE_ALLOC  InitializeTraceAlloc()
    #define CHECK_TRACE_ALLOC CheckTraceAlloc(__FUNCTION__,__FILE__,__LINE__)
    #define DUMP_TRACE_ALLOC(f)  DumpTraceAlloc(__FUNCTION__,__FILE__,__LINE__,f)
#elif TRACE_ALLOC_MODE > 1
    #define TEST_ALLOC(ptr,hexd)
    #define FREE(ptr) dclib_free(ptr)
    #define CALLOC(nmemb,size) dclib_calloc(__FUNCTION__,__FILE__,__LINE__,nmemb,size)
    #define MALLOC(size) dclib_malloc(__FUNCTION__,__FILE__,__LINE__,size)
    #define REALLOC(ptr,size) dclib_realloc(__FUNCTION__,__FILE__,__LINE__,ptr,size)
    #define STRDUP(src) dclib_strdup(__FUNCTION__,__FILE__,__LINE__,src)
    #define STRDUP2(src1,src2) dclib_strdup2(__FUNCTION__,__FILE__,__LINE__,src1,src2)
    #define STRDUP3(src1,src2,src3) dclib_strdup3(__FUNCTION__,__FILE__,__LINE__,src1,src2,src3)
    #define MEMDUP(src,size) dclib_memdup(__FUNCTION__,__FILE__,__LINE__,src,size)
    #define INIT_TRACE_ALLOC
    #define CHECK_TRACE_ALLOC
    #define DUMP_TRACE_ALLOC(f)
#else
    #define TEST_ALLOC(ptr,hexd)
    #define FREE(ptr) dclib_free(ptr)
    #define CALLOC(nmemb,size) dclib_calloc(nmemb,size)
    #define MALLOC(size) dclib_malloc(size)
    #define REALLOC(ptr,size) dclib_realloc(ptr,size)
    #define STRDUP(src) dclib_strdup(src)
    #define STRDUP2(src1,src2) dclib_strdup2(src1,src2)
    #define STRDUP3(src1,src2,src3) dclib_strdup3(src1,src2,src3)
    #define MEMDUP(src,size) dclib_memdup(src,size)
    #define INIT_TRACE_ALLOC
    #define CHECK_TRACE_ALLOC
    #define DUMP_TRACE_ALLOC(f)
#endif

#define XFREE(ptr) dclib_free(ptr)

#ifndef DCLIB_DEBUG_C
    #define free	do_not_use_free
    #define calloc	do_not_use_calloc
    #define malloc	do_not_use_malloc
    #define realloc	do_not_use_realloc
    #undef  strdup
    #define strdup	do_not_use_strdup
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    END				///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_DEBUG_H
