
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

#ifndef WIN_SZS_LIB
  // needed for GetTermWidthFD()
  #include <sys/ioctl.h>
#endif

#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/time.h>

#include "dclib-basics.h"
#include "dclib-debug.h"
#include "dclib-file.h"
#include "dclib-utf8.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////		some basic string functions		///////////////
///////////////////////////////////////////////////////////////////////////////

const char EmptyString[4] = "\0\0\0";	// Ignored by FreeString()
const char MinusString[4] = "-\0\0";	// Ignored by FreeString()

const char LoDigits[0x41] =
	"0123456789"
	"abcdefghijklmnopqrstuvwxyz"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	".+";

const char HiDigits[0x41] =
	"0123456789"
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	".+";

const char Tabs20[21] = // 20 * TABS + NULL
	"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

const char LF20[21] = // 20 * LF + NULL
	"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n";

const char Space200[201] = // 200 * ' ' + NULL
	"                                                  "
	"                                                  "
	"                                                  "
	"                                                  ";

const char Minus300[301] = // 300 * '-' + NULL
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------"
	"--------------------------------------------------";

const char Underline200[201] = // 200 * '_' + NULL
	"__________________________________________________"
	"__________________________________________________"
	"__________________________________________________"
	"__________________________________________________";

const char Hash200[201] = // 200 * '#' + NULL
	"##################################################"
	"##################################################"
	"##################################################"
	"##################################################";

const char Slash200[201] = // 200 * '/' + NULL
	"//////////////////////////////////////////////////"
	"//////////////////////////////////////////////////"
	"//////////////////////////////////////////////////"
	"//////////////////////////////////////////////////";

// UTF-8 => 3 bytes per char
const char ThinLine300_3[901] = // 300 * '─' (U+2500) + NULL
	"──────────────────────────────────────────────────"
	"──────────────────────────────────────────────────"
	"──────────────────────────────────────────────────"
	"──────────────────────────────────────────────────"
	"──────────────────────────────────────────────────"
	"──────────────────────────────────────────────────";

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void PrintSettingsDCLIB ( FILE *f, int indent )
{
    DASSERT(f);
    indent = NormalizeIndent(indent);
    MARK_USED(indent); // for the case, that all other conditions fail!

 #ifdef DCLIB_NETWORK
    fprintf(f,"%*s" "DCLIB_NETWORK\n",indent,"");
 #endif

 #ifdef DCLIB_TERMINAL
    fprintf(f,"%*s" "DCLIB_TERMINAL\n",indent,"");
 #endif

 #ifdef DCLIB_MYSQL
    fprintf(f,"%*s" "DCLIB_MYSQL\n",indent,"");
 #endif

 #ifdef DCLIB_HAVE_USABLE_SIZE
    fprintf(f,"%*s" "DCLIB_HAVE_USABLE_SIZE\n",indent,"");
 #endif
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetupMultiProcessing()
{
    multi_processing = true;
}

///////////////////////////////////////////////////////////////////////////////

void SetupProgname ( int argc, char ** argv, ccp tname, ccp tvers, ccp ttitle )
{
    if (tname)
	toolname = tname;
    if (tvers)
	toolversion = tvers;
    if (ttitle)
	tooltitle = ttitle;

    if (!progpath)
    {
	char path[PATH_MAX];
	int res = GetProgramPath( path, sizeof(path), true, argc > 0 ? argv[0] : 0 );
	if ( res > 0 )
	    progpath = STRDUP(path);
    }

    if ( argc > 0 && *argv && **argv )
    {
	ccp slash = strrchr(*argv,'/');
	progname = slash ? slash + 1 : *argv;
    }
    else
	progname = toolname && *toolname ? toolname : "?";
}

///////////////////////////////////////////////////////////////////////////////

ccp ProgramPath()
{
    static bool done = false;
    if ( !progpath && !done )
    {
	done = true;

	char path[PATH_MAX];
	int res = GetProgramPath( path, sizeof(path), true, 0 );
	if ( res > 0 )
	    progpath = STRDUP(path);
    }
    return progpath;
}

///////////////////////////////////////////////////////////////////////////////

ccp ProgramDirectory()
{
    if (!progdir)
    {
	ccp path = ProgramPath();
	if (path)
	{
	    ccp end = strrchr(path,'/');
	    const uint len = end ? end - path : strlen(path);
	    progdir = MEMDUP(path,len);
	}
    }
    return progdir;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef __CYGWIN__

ccp ProgramPathNoExt()
{
    static ccp res = 0;
    if (!res)
    {
	res = ProgramPath();
	if (res)
	{
	    ccp slash = strrchr(res,'/');
	    ccp ext = strrchr( slash ? slash + 1 : res, '.' );
	    if (ext)
		res = MEMDUP(res,ext-res);
	}
    }
    return res;
}

#endif // __CYGWIN__

///////////////////////////////////////////////////////////////////////////////

int GetProgramPath
(
    // returns strlen(buf) on success, -1 on failure

    char  *buf,		// buffer for the result; empty string on failure.
    uint  buf_size,	// size of buffer
    bool  check_proc,	// true: read files of /proc/... to find path
    ccp	  argv0		// not NULL: analyse this argument (argv[0])
)
{
    char path[PATH_MAX], path2[PATH_MAX];


    //--- read files of /proc/...

    if (check_proc)
    {
	static ccp tab[] =
	{
	    "/proc/self/exe",
	    "/proc/curproc/file",
	    "/proc/self/path/a.out",
	    0
	};

	ccp *ptr;
	for ( ptr = tab; *ptr; ptr++ )
	    if ( realpath(*ptr,path) && *path )
		return StringCopyS(buf,buf_size,path) - buf;
    }


    //--- analyze argv0

    if (argv0)
    {
	if ( *argv0 == '/' )
	{
	    // absolute path
	    if ( realpath(argv0,path) && *path )
		return StringCopyS(buf,buf_size,path) - buf;
	}
	else if (strchr(argv0,'/'))
	{
	    // relative path
	    if ( getcwd(path2,sizeof(path2)) > 0 )
	    {
		ccp p2 = PathCatPP(path2,sizeof(path2),path,argv0);
		if ( realpath(p2,path) && *path )
		    return StringCopyS(buf,buf_size,path) - buf;
	    }
	}
	else
	{
	    // simple name => search in path environment
	    uint arglen = strlen(argv0);
	    if ( arglen < sizeof(path2) - 2 )
	    {
		ccp dest_end = path2 + sizeof(path2) - 1 - arglen;
		ccp env = getenv("PATH");
		while ( env && *env )
		{
		    while ( *env == ':' )
			env++;
		    char *dest = path2;
		    while ( *env && *env != ':' && dest < dest_end )
			*dest++ = *env++;
		    *dest++ = '/';
		    memcpy(dest,argv0,arglen+1);
		    if ( realpath(path2,path) && *path )
			return StringCopyS(buf,buf_size,path) - buf;
		}
	    }
	}
    }


    //--- failed

    *buf = 0;
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

void FreeString ( ccp str )
{
    noTRACE("FreeString(%p) EmptyString=%p MinusString=%p\n",
	    str, EmptyString, MinusString );
    if ( str != EmptyString && str != MinusString )
	FREE((char*)str);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * StringCopyE ( char * buf, ccp buf_end, ccp src )
{
    // RESULT: end of copied string pointing to NULL
    // 'src' may be a NULL pointer.

    if ( buf >= buf_end )
	return (char*)buf_end - 1;

    DASSERT(buf);
    DASSERT(buf<buf_end);
    buf_end--;

    if (src)
	while( buf < buf_end && *src )
	    *buf++ = *src++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCopyS ( char * buf, size_t buf_size, ccp src )
{
    return StringCopyE(buf,buf+buf_size,src);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCopyEM ( char * buf, ccp buf_end, ccp src, size_t max_copy )
{
    // RESULT: end of copied string pointing to NULL
    // 'src' may be a NULL pointer.

    ASSERT(buf);
    ASSERT(buf<buf_end);
    buf_end--;
    if ( max_copy >= 0 && buf + max_copy < buf_end )
	buf_end = buf + max_copy;

    if (src)
	while( buf < buf_end && *src )
	    *buf++ = *src++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCopySM ( char * buf, size_t buf_size, ccp src, size_t max_copy )
{
    return StringCopyEM(buf,buf+buf_size,src,max_copy);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat2E ( char * buf, ccp buf_end, ccp src1, ccp src2 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    ASSERT(buf);
    ASSERT(buf<buf_end);
    buf_end--;

    if (src1)
	while( buf < buf_end && *src1 )
	    *buf++ = *src1++;

    if (src2)
	while( buf < buf_end && *src2 )
	    *buf++ = *src2++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCat2S ( char * buf, size_t buf_size, ccp src1, ccp src2 )
{
    return StringCat2E(buf,buf+buf_size,src1,src2);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat3E ( char * buf, ccp buf_end, ccp src1, ccp src2, ccp src3 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    ASSERT(buf);
    ASSERT(buf<buf_end);
    buf_end--;

    if (src1)
	while( buf < buf_end && *src1 )
	    *buf++ = *src1++;

    if (src2)
	while( buf < buf_end && *src2 )
	    *buf++ = *src2++;

    if (src3)
	while( buf < buf_end && *src3 )
	    *buf++ = *src3++;

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCat3S ( char * buf, size_t buf_size, ccp src1, ccp src2, ccp src3 )
{
    return StringCat3E(buf,buf+buf_size,src1,src2,src3);
}

///////////////////////////////////////////////////////////////////////////////

char * StringLowerE ( char * buf, ccp buf_end, ccp src )
{
    // RESULT: end of copied string pointing to NULL
    // 'src' may be a NULL pointer.

    if ( buf >= buf_end )
	return (char*)buf_end - 1;

    DASSERT(buf);
    DASSERT(buf<buf_end);
    buf_end--;

    if (src)
	while( buf < buf_end && *src )
	    *buf++ = tolower((int)*src++);

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringLowerS ( char * buf, size_t buf_size, ccp src )
{
    return StringLowerE(buf,buf+buf_size,src);
}

///////////////////////////////////////////////////////////////////////////////

char * StringUpperE ( char * buf, ccp buf_end, ccp src )
{
    // RESULT: end of copied string pointing to NULL
    // 'src' may be a NULL pointer.

    if ( buf >= buf_end )
	return (char*)buf_end - 1;

    DASSERT(buf);
    DASSERT(buf<buf_end);
    buf_end--;

    if (src)
	while( buf < buf_end && *src )
	    *buf++ = toupper((int)*src++);

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringUpperS ( char * buf, size_t buf_size, ccp src )
{
    return StringUpperE(buf,buf+buf_size,src);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				PathCat*()		///////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufPP ( char * buf, size_t bufsize, ccp path1, ccp path2 )
{
    // concatenate path + path; returns buf

    DASSERT(buf);
    DASSERT(bufsize);

    if ( !path1 || !*path1 )
    {
	if (path2)
	    StringCopyS(buf,bufsize,path2);
	else
	    *buf = 0;
	return buf;
    }

    char * ptr = StringCopyS(buf,bufsize-1,path1);
    DASSERT( ptr > buf );
    if ( ptr[-1] != '/' )
	*ptr++ = '/';
    if (path2)
    {
	while ( *path2 == '/' )
	    path2++;
	StringCopyE(ptr,buf+bufsize,path2);
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatPP ( char * buf, size_t bufsize, ccp path1, ccp path2 )
{
    // concatenate path + path; returns path1 | path2 | buf

    if ( !path1 || !*path1 )
	return path2 ? path2 : "";

    if ( !path2 || !*path2 )
	return path1;

    char * ptr = StringCopyS(buf,bufsize-1,path1);
    DASSERT( ptr > buf );
    if ( ptr[-1] != '/' )
	*ptr++ = '/';
    while ( *path2 == '/' )
	path2++;
    StringCopyE(ptr,buf+bufsize,path2);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PathAllocPP ( ccp path1, ccp path2 )
{
    char buf[PATH_MAX];
    return STRDUP(PathCatPP(buf,sizeof(buf),path1,path2));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufPPE ( char * buf, size_t bufsize, ccp path1, ccp path2, ccp ext )
{
    // concatenate path + path; returns (by definition) path1 | path2 | buf

    DASSERT(buf);
    DASSERT(bufsize);

    char *ptr = path1 ? StringCopyS(buf,bufsize-1,path1) : buf;
    if ( ptr > buf && ptr[-1] != '/' )
	*ptr++ = '/';
    *ptr = 0;

    if (path2)
    {
	while ( *path2 == '/' )
	    path2++;
	ptr = StringCopyE(ptr,buf+bufsize,path2);
    }

    if (ext)
	StringCopyE(ptr,buf+bufsize,ext);

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

// PathCatPPE() is inline

///////////////////////////////////////////////////////////////////////////////

ccp PathAllocPPE ( ccp path1, ccp path2, ccp ext )
{
    char buf[PATH_MAX];
    return STRDUP(PathCatPPE(buf,sizeof(buf),path1,path2,ext));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufBP ( char *buf, size_t bufsize, ccp base, ccp path )
{
    // PathCatBP() is special: path can be part of buf

    if (!path)
	path = "";
    if ( *path == '/' || base && !*base )
    {
	// already absolute | no base to add
	StringCopyS(buf,bufsize,path);
	return buf;
    }


    //--- now copy base or cwd

    while ( path[0] == '.' && path[1] == '/' )
	path += 2;

    char buf2[PATH_MAX];
    StringCopyS(buf2,sizeof(buf2),path);

    char *dest;
    if (base)
	dest = StringCopyS(buf,bufsize,base);
    else
    {
	char *cwd = getcwd(buf,bufsize);
	if (cwd)
	    dest = buf + strlen(buf);
	else
	{
	    // on error: return fallback
	    dest = StringCopyS(buf,bufsize,"./");
	}
    }
    StringCat2E(dest,buf+bufsize,"/",buf2);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatBP ( char *buf, size_t bufsize, ccp base, ccp path )
{
    // PathCatBP() is special: path can be part of buf

    if ( path && *path == '/' || base && !*base )
	return path; // already absolute | no base to add

    return PathCatBufBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathAllocBP ( ccp base, ccp path )
{
    char buf[PATH_MAX];
    return STRDUP(PathCatBP(buf,sizeof(buf),base,path));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufBPP ( char * buf, size_t bufsize, ccp base, ccp path1, ccp path2 )
{
    ccp path = PathCatPP(buf,bufsize,path1,path2);
    return PathCatBufBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatBPP ( char * buf, size_t bufsize, ccp base, ccp path1, ccp path2 )
{
    ccp path = PathCatPP(buf,bufsize,path1,path2);
    return PathCatBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathAllocBPP ( ccp base, ccp path1, ccp path2 )
{
    char buf[PATH_MAX];
    return STRDUP(PathCatBPP(buf,sizeof(buf),base,path1,path2));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufBPPE ( char * buf, size_t bufsize, ccp base, ccp path1, ccp path2, ccp ext )
{
    ccp path = PathCatPPE(buf,bufsize,path1,path2,ext);
    return PathCatBufBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatBPPE ( char * buf, size_t bufsize, ccp base, ccp path1, ccp path2, ccp ext )
{
    ccp path = PathCatPPE(buf,bufsize,path1,path2,ext);
    return PathCatBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathAllocBPPE ( ccp base, ccp path1, ccp path2, ccp ext )
{
    char buf[PATH_MAX];
    return STRDUP(PathCatBPPE(buf,sizeof(buf),base,path1,path2,ext));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCombine ( char *buf, uint buf_size, ccp path, ccp base_path )
{
    DASSERT(buf);
    DASSERT( buf_size > 10 );

    if (!path)
	path = "";

    char *dest = buf;
    if ( base_path && *base_path && *path != '/' )
    {
	StringCopyS(buf,buf_size,base_path);
	char *p = strrchr(buf,'/');
	if (p)
	    dest = p+1;
    }

    StringCopyE(dest,buf+buf_size,path);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PathCombineFast ( char *buf, uint buf_size, ccp path, ccp base_path )
{
    DASSERT(buf);
    DASSERT( buf_size > 10 );

    if (!path)
	path = "";

    if ( !base_path || !*base_path || *path == '/' )
	return (char*)path;

    StringCopyS(buf,buf_size,base_path);
    char *p = strrchr(buf,'/');
    if (!p)
	return (char*)path;

    StringCopyE(p+1,buf+buf_size,path);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * NewFileExtE ( char * buf, ccp buf_end, ccp path, ccp ext )
{
    DASSERT( buf);
    DASSERT( buf_end);
    DASSERT( buf_end > buf );

    char * end = path && path != buf
			? StringCopyE(buf,buf_end-1,path)
			: buf + strlen(buf);
    DASSERT( end < buf_end );

    char * ptr = end;
    while ( ptr > buf && *--ptr != '/' )
	if ( *ptr == '.' )
	{
	    end = ptr;
	    break;
	}

    if ( ext && *ext )
    {
	if ( *ext != '.' )
	    *end++ = '.';
	end = StringCopyE(end,buf_end,ext);
    }

    *end = 0;
    return end;
}

///////////////////////////////////////////////////////////////////////////////

char * NewFileExtS ( char * buf, size_t bufsize, ccp path, ccp ext )
{
    return NewFileExtE(buf,buf+bufsize,path,ext);
}

///////////////////////////////////////////////////////////////////////////////

int StrPathCmp ( ccp path1, ccp path2 )
{
    if ( path1[0] == '.' && path1[1] == '/' )
	path1 += 2;
    if ( path2[0] == '.' && path2[1] == '/' )
	path2 += 2;
    return strcmp(path1,path2);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int PathCmp
(
    ccp		path1,		// NULL or first path
    ccp		path2,		// NULL or second path
    uint	mode		// bit field:
				//  1: skip (multiple) leading './'
				//  2: compare case insensitive
				//  4: sort with respect for unsigned decimal numbers
)
{
    if (!path2)
	return path1 != 0;
    if (!path1)
	return -1;

    noPRINT("PathCmp( | %s | %s | %u )\n",path1,path2,mode);

    //--- skip './'

    if ( mode & 1 )
    {
	while ( path1[0] == '.' && path1[1] == '/' )
	    path1 += 2;
	while ( path2[0] == '.' && path2[1] == '/' )
	    path2 += 2;
    }


    //--- eliminate equal characters

    while ( *path1 == *path2 )
    {
	if (!*path1)
	    return 0;
	path1++;
	path2++;
    }


    //--- start the case+path test

    ccp p1 = path1;
    ccp p2 = path2;

    const uint ignore_case = mode & 2;

    while ( *p1 || *p2 )
    {
	int ch1, ch2;
	if (ignore_case)
	{
	    ch1 = (uchar)tolower((int)*p1++);
	    ch2 = (uchar)tolower((int)*p2++);
	}
	else
	{
	    ch1 = (uchar)*p1++;
	    ch2 = (uchar)*p2++;
	}
	int stat = ch1 - ch2;
	noPRINT("%02x,%02x,%c  %02x,%02x,%c -> %2d\n",
		p1[-1], ch1, ch1, p2[-1], ch2, ch2, stat );

	if (stat)
	{
	    if ( mode & 4 )
	    {
		p1--;
		while ( *p1 == '0' )
		    p1++;
		ccp q1 = p1;
		while ( *q1 >= '0' && *q1 <= '9' )
		    q1++;
		const int len1 = q1 - p1;

		if (len1)
		{
		    p2--;
		    while ( *p2 == '0' )
			p2++;
		    ccp q2 = p2;
		    while ( *q2 >= '0' && *q2 <= '9' )
			q2++;
		    const int len2 = q2 - p2;

		    if (len2)
		    {
			int stat2 = len1 - len2;
			if (!stat2)
			    stat2 = memcmp(p1,p2,len1);
			if (stat2)
			    return stat2;
		    }
		}
	    }
	    return ch1 == '/' ? -1 : ch2 == '/' ? 1 : stat;
	}

	if ( ch1 == '/' )
	    break;
    }

    return strcmp(path1,path2);
}

///////////////////////////////////////////////////////////////////////////////

int StrNumCmp ( ccp a, ccp b )
{
    if (!b)
	return a != 0;
    if (!a)
	return -1;

    for(;;)
    {
	const int va = *a++ & 0xff;
	const int vb = *b++ & 0xff;
	const int delta = va - vb;

	if (delta)
	{
	    if ( va < '0' || va > '9' || vb < '0' || vb > '9' )
		return delta; // no number

	    //--- both are numbers

	    a--;
	    while ( *a == '0' )
		a++;
	    ccp start_a = a;
	    while ( *a >= '0' && *a <= '9' )
		a++;

	    b--;
	    while ( *b == '0' )
		b++;
	    ccp start_b = b;
	    while ( *b >= '0' && *b <= '9' )
		b++;

	    // compare num length
	    int num_delta = ( a - start_a ) - ( b - start_b );
	    if (num_delta)
		return num_delta;

	    // compare numeric values
	    while ( start_a < a )
	    {
		num_delta = *start_a++ - *start_b++;
		if (num_delta)
		    return num_delta;
	    }

	    // number identical, but not string
	    return delta;
	}
	else if (!va)
	    return 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

uint CountEqual ( cvp m1, cvp m2, uint size )
{
    const u8 *p1 = m1;
    const u8 *p2 = m2;
    while ( size-- )
	if ( *p1++ != *p2++ )
	    return  p1 - (u8*)m1 - 1;
    return p1 - (u8*)m1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ReplaceToBuf
(
    // returns -1 on error (buf to small) or the length of the resulting string.
    // In the later case, a 0 byte is appended, but not counting for the length.

    char	*buf,		// pointer to buf
    uint	buf_size,	// size of 'buf'

    ccp		src1,		// NULL or source, maybe part of 'buf'
    int		src1_len,	// length of 'src1', if -1: determine by strlen()
    int		pos,		// robust replace position, <0: relative to end
    int		rm_len,		// robust num of chars to remove, <0: rm before 'pos'

    ccp		src2,		// NULL or source to insert, must not be part of 'buf'
    int		src2_len	// lengh of 'src2', if -1: determine by strlen()
)
{
    DASSERT(buf);

    //--- normalize source #2

    if (!src2)
	src2_len = 0;
    else if ( src2_len < 0 )
	src2_len = strlen(src2);


    //--- if source #1 available

    if (src1)
    {
	//--- check src1 length

	if ( src1_len < 0 )
	    src1_len = strlen(src1);

	if (!src1_len)
	    goto no_src1;


	//--- normalize 'pos'

	if ( pos < 0 )
	{
	    pos += src1_len;
	    if ( pos < 0 )
	    {
		if ( rm_len > 0 )
		{
		    rm_len += pos;
		    if ( rm_len < 0 )
			rm_len = 0;
		}
		pos = 0;
	    }
	}
	else if ( pos > src1_len )
	    pos = src1_len;


	//--- normalize 'rm_len'

	if ( rm_len < 0 )
	{
	    rm_len = -rm_len;
	    if ( rm_len < pos )
		pos -= rm_len;
	    else
	    {
		rm_len = pos;
		pos = 0;
	    }
	}
	else
	{
	    const int max = src1_len - pos;
	    if ( rm_len > max )
		rm_len = max;
	}

	const int res_len = src1_len + src2_len - rm_len;
	if ( res_len >= buf_size )
	    return -1;


	//--- copy src1

	if ( src1 > buf )
	{
	    memmove( buf, src1, pos );
	    const int pos2 = pos + rm_len;
	    memmove( buf+pos+src2_len, src1+pos2, src1_len-pos2 );
	}
	else
	{
	    const int pos2 = pos + rm_len;
	    memmove( buf+pos+src2_len, src1+pos2, src1_len-pos2 );
	    memmove( buf, src1, pos );
	}


	//--- copy src2 & terminate

	memcpy( buf+pos, src2, src2_len );
	buf[res_len] = 0;
	return res_len;
    }

  no_src1:
    if ( src2_len < buf_size )
    {
	memcpy(buf,src2,src2_len);
	buf[src2_len] = 0;
	return src2_len;
    }

    return -1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int NormalizeIndent ( int indent )
{
    return indent < 0 ? 0 : indent < 50 ? indent : 50;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * SkipControls ( ccp src )
{
    // skips all character 1..SPACE
    // 'src' can be NULL

    if (src)
	while ( *src > 0 && *src <= ' ' )
	    src++;

    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

char * SkipControlsE ( ccp src, ccp end )
{
    // skips all character 0..SPACE
    // if end==NULL: Use SkipControls()
    // 'src' can be NULL

    if (!end)
	return SkipControls(src);

    if (src)
	while ( src < end && (uchar)*src <= ' ' )
	    src++;

    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

char * SkipControls1 ( ccp src, char ch1 )
{
    // skips all character 1..SPACE and 'ch1'
    // 'src' can be NULL

    if (src)
	while ( *src > 0 && *src <= ' ' || *src == ch1 )
	    src++;

    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

char * SkipControlsE1 ( ccp src, ccp end, char ch1 )
{
    // skips all character 0..SPACE and 'ch1'
    // if end==NULL: Use SkipControls1()
    // 'src' can be NULL

    if (!end)
	return SkipControls1(src,ch1);

    if (src)
	while ( src < end && ( (uchar)*src <= ' ' || *src == ch1 ))
	    src++;

    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

char * SkipControls2 ( ccp src, char ch1, char ch2 )
{
    // skips all character 1..SPACE and 'ch1' and 'ch2'
    // 'src' can be NULL

    if (src)
	while ( *src > 0 && *src <= ' ' || *src == ch1 || *src == ch2 )
	    src++;

    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////

char * SkipControlsE2 ( ccp src, ccp end, char ch1, char ch2 )
{
    // skips all character 0..SPACE and 'ch1' and 'ch2'
    // if end==NULL: Use SkipControls2()
    // 'src' can be NULL

    if (!end)
	return SkipControls2(src,ch1,ch2);

    if (src)
	while ( src < end && ( (uchar)*src <= ' ' || *src == ch1  || *src == ch2 ))
	    src++;

    return (char*)src;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// return the number of chars until first EOL ( \0 | \r | \n );
// pointer are NULL save

uint LineLen ( ccp ptr )
{
    if (!ptr)
	return 0;

    ccp p = ptr;
    while ( *p && *p != '\n' && *p != '\r' )
	p++;
    return p - ptr;
}

///////////////////////////////////////////////////////////////////////////////

uint LineLenE ( ccp ptr, ccp end )
{
    if (!ptr)
	return 0;
    if (!end)
	return LineLen(ptr);

    ccp p = ptr;
    while ( p < end && *p && *p != '\n' && *p != '\r' )
	p++;
    return p - ptr;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint TrimBlanks
(
    char	*str,		// pointer to string, modified
    ccp		end		// NULL or end of string
)
{
    char *src  = str;
    char *dest = str;

    bool space = false;
    while ( end ? src < end : *src )
    {
	DASSERT( dest <= src );
	uchar ch = *src++;
	if ( ch > ' ' )
	{
	    if (space)
	    {
		space = false;
		if ( dest > str )
		    *dest++ = ' ';
	    }
	    *dest++ = ch;
	}
	else if ( ch == ' ' )
	    space = true;
    }

    if ( !end || dest < end )
	*dest = 0;

    return dest - str;
}

///////////////////////////////////////////////////////////////////////////////

char * ScanName
(
    // return a pointer to the first unused character
    char	*buf,		// destination buffer, terminated with NULL
    uint	buf_size,	// size of 'buf', must be >0
    ccp		str,		// source string
    ccp		end,		// NULL or end of 'str'
    bool	allow_signs	// true: allow '-' and '+'
)
{
    DASSERT(buf);
    DASSERT(buf_size);
    if (!str)
    {
	*buf = 0;
	return 0;
    }

    //--- skip leading blanks

    if (end)
	while ( str < end && (uchar)*str <= ' ' )
	    str++;
    else
	while ( str < end && *str && (uchar)*str <= ' ' )
	    str++;

    char *dest = buf, *dest_end = buf + buf_size - 1;
    while ( end ? str < end : *str )
    {
	const int ch = *str;
	if ( isalnum(ch) || ch == '_' || allow_signs && ( ch =='-' || ch == '+' ) )
	{
	    str++;
	    if ( dest < dest_end )
		*dest++ = toupper(ch);
	}
	else
	    break;
    }
    *dest = 0;

    if (end)
	while ( str < end && (uchar)*str <= ' ' )
	    str++;
    else
	while ( str < end && *str && (uchar)*str <= ' ' )
	    str++;

    return (char*)str;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static char circ_buf[CIRC_BUF_SIZE];
static char *circ_ptr = circ_buf;

//-----------------------------------------------------------------------------

char * GetCircBuf
(
    // never returns NULL, but always ALIGN(4)

    u32		buf_size	// wanted buffer size, add 1 for NULL-term
				// if buf_size > CIRC_BUF_MAX_ALLOC:
				//  ==> ERROR0(ERR_OUT_OF_MEMORY)
)
{
    DASSERT( circ_ptr >= circ_buf && circ_ptr <= circ_buf + sizeof(circ_buf) );

    buf_size = buf_size + 3 & ~3;
    if ( buf_size > CIRC_BUF_MAX_ALLOC )
    {
	ERROR0(ERR_OUT_OF_MEMORY,
		"Circulary buffer too small: needed=%u, half-size=%zu\n",
		buf_size, sizeof(circ_buf)/2 );
	ASSERT(0);
	exit(ERR_OUT_OF_MEMORY);
    }

    if ( circ_buf + sizeof(circ_buf) - circ_ptr < buf_size )
	circ_ptr = circ_buf;

    char *result = circ_ptr;
    circ_ptr = result + buf_size;
    DASSERT( circ_ptr >= circ_buf && circ_ptr <= circ_buf + sizeof(circ_buf) );

    noPRINT("CIRC-BUF: %3u -> %3zu / %3zu / %3zu\n",
		buf_size, result-circ_buf, circ_ptr-circ_buf, sizeof(circ_buf) );
    return result;
}

//-----------------------------------------------------------------------------

void ReleaseCircBuf
(
    ccp	    end_buf,		// pointer to end of previous alloced buffer
    uint    release_size	// number of bytes to give back from end
)
{
    DASSERT( circ_ptr >= circ_buf && circ_ptr <= circ_buf + sizeof(circ_buf) );

    if ( end_buf == circ_ptr
	&& release_size <= CIRC_BUF_MAX_ALLOC
	&& circ_ptr >= circ_buf + release_size )
    {
	circ_ptr -= release_size;
	DASSERT( circ_ptr >= circ_buf && circ_ptr <= circ_buf + sizeof(circ_buf) );
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PrintVersion
(
    char		* buf,		// result buffer
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    u32			version		// version number to print
)
{
    if (!buf)
	buf = GetCircBuf( buf_size = 20 );

    version = htonl(version); // we use big endian here
    const u8 * v = (const u8 *)&version;

    if (v[2])
    {
	if ( !v[3] || v[3] == 0xff )
	    snprintf(buf,buf_size,"%u.%02x.%02x",v[0],v[1],v[2]);
	else
	    snprintf(buf,buf_size,"%u.%02x.%02x.beta%u",v[0],v[1],v[2],v[3]);
    }
    else
    {
	if ( !v[3] || v[3] == 0xff )
	    snprintf(buf,buf_size,"%u.%02x",v[0],v[1]);
	else
	    snprintf(buf,buf_size,"%u.%02x.beta%u",v[0],v[1],v[3]);
    }

    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintID
(
    const void		* id,		// ID to convert in printable format
    size_t		id_len,		// len of 'id'
    void		* buf		// Pointer to buffer, size >= id_len + 1
					// If NULL, a local circulary static buffer is used
)
{
    if (!buf)
	buf = GetCircBuf( id_len + 1);

    ccp src = id;
    char * dest = buf;
    while ( id_len-- > 0 )
    {
	const char ch = *src++;
	*dest++ = ch >= ' ' && ch < 0x7f ? ch : '.';
    }
    *dest = 0;

    return buf;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    alloc info			///////////////
///////////////////////////////////////////////////////////////////////////////

bool alloc_info_calced		= false;
uint alloc_info_overhead	=   8;	// default values
uint alloc_info_block_size	=  16;
uint alloc_info_add1		=   7;
uint alloc_info_add2		=   8;
uint alloc_info_mask		= ~15;

///////////////////////////////////////////////////////////////////////////////

uint SetupAllocInfo()
{
    if (!alloc_info_calced)
    {
	alloc_info_calced = true;

	enum { MAX = 200 };

	char *alloc[MAX+1];
	memset(alloc,0,sizeof(alloc));

	uint i, mask_found = ~0;
	for ( i = 1; i <= MAX; i++ )
	{
	    alloc[i] = (char*)MALLOC(i);
	    noPRINT("%4x: %p [%zx]\n",i,alloc[i],alloc[i]-alloc[ i>0 ? i-1 : 0 ] );
	    if ( i > 1 )
	    {
		int diff = abs ( alloc[i-1] - alloc[i] );
		mask_found &= diff - 1;
	    }
	}
	const int lo0 = FindLowest0Bit(&mask_found,1);
	const int hi1 = FindHighest1Bit(&mask_found,1);
	PRINT("mask: %08x, lo0=%d, hi1=%d\n",mask_found,lo0,hi1);

	if ( hi1 + 1 == lo0 )
	{
	    // -> maske besteht aus hi1 '1' gefolgt von ausnahmslos '0'

	    alloc_info_mask = ~mask_found;
	    alloc_info_block_size = mask_found + 1;
	    PRINT("alloc_info_block_size = %d, alloc_info_mask = 0x%x\n",
			alloc_info_block_size, alloc_info_mask );

	    int found = 0x100;
	    for ( i = 2; i <= MAX; i++ )
	    {
		const int diff = abs ( alloc[i-1] - alloc[i] ) - i;
		if ( diff >= 0 && found > diff )
		    found = diff;
	    }

	    if ( found < MAX )
	    {
		alloc_info_overhead = found;
		found %= alloc_info_block_size;
		alloc_info_add1 = found - 1;
		alloc_info_add2 = alloc_info_block_size - found;
		PRINT("alloc_info_overhead = %d, alloc_info_add = %d, %d\n",
			alloc_info_overhead, alloc_info_add1, alloc_info_add2 );
	    }
	}

	for ( i = 1; i <= MAX; i++ )
	    FREE(alloc[i]);
    }

    return alloc_info_block_size;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    SplitArg_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetSplitArg ( SplitArg_t *sa )
{
    DASSERT(sa);
    FREE(sa->argv);
    FREE(sa->temp);
    InitializeSplitArg(sa);
}

///////////////////////////////////////////////////////////////////////////////

int ScanSplitArg
(
    // returns arg->argc;

    SplitArg_t	    *arg,	// pointer to object
    bool	    init_arg,	// true: call InitializeSplitArg()
    ccp		    src,	// NULL or source string
    ccp		    src_end,	// end of 'src'; if NULL: calculated by strlen(src)
    char	    *work_buf	// pointer to a buffer for strings, may be 'src'
				// size must be: length of 'src' + 1
				// if NULL, a temporary buffer is alloced.
)
{
    DASSERT(arg);
    if (init_arg)
	InitializeSplitArg(arg);
    else
	ResetSplitArg(arg);

    if (!src)
	src = EmptyString;
    if (!src_end)
	src_end = src + strlen(src);

 #ifdef TEST0
    enum { N_PREDEF = 5 };
 #else
    enum { N_PREDEF = 250 };
 #endif
    char *argv_predef[N_PREDEF+1],
	**argv_buf  = argv_predef,
	**argv_dest = argv_predef,
	**argv_end  = argv_predef + N_PREDEF;
    *argv_dest++ = progname ? (char*)progname : "";
    uint argv_size = 0;

    if (!work_buf)
    {
	arg->temp_size = src_end - src + 1;
	arg->temp = work_buf = MALLOC(arg->temp_size);
    }
    char *dest = work_buf;


    //--- main loop

    for(;;)
    {
	while ( src < src_end
		&& ( !*src || *src == ' ' || *src == '\t' || *src == '\r' || *src == '\n' ))
	{
	    src++;
	}
	if ( src == src_end )
	    break;

	if ( argv_dest == argv_end )
	{
	    PRINT("NEED MORE: %zu/%zu size=%u: |%s|\n",
			argv_dest-argv_buf, argv_end-argv_buf, argv_size, src );
	    const uint idx = argv_dest - argv_buf;
	    if (argv_size)
	    {
		argv_size += N_PREDEF;
		argv_buf = REALLOC(argv_buf,(argv_size+1)*sizeof(*argv_buf));
	    }
	    else
	    {
		argv_size = 2*N_PREDEF;
		char **temp = MALLOC((argv_size+1)*sizeof(*argv_buf));
		memcpy(temp,argv_buf,idx*sizeof(*argv_buf));
		argv_buf = temp;
	    }
	    argv_dest = argv_buf + idx;
	    argv_end  = argv_buf + argv_size;
	}

	*argv_dest++ = dest;
	while ( src < src_end )
	{
	    const char ch = *src;
	    if ( !ch || ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' )
		break;
	    src++;
	    if ( ch == '\\' && src < src_end )
		*dest++ = *src++;
	    else if ( ch == '\'' )
	    {
		while ( src < src_end && *src != '\'' )
		    *dest++ = *src++;
		if ( src < src_end )
		    src++;
	    }
	    else if ( ch == '"'  )
	    {
		while ( src < src_end && *src != '"' )
		{
		    char ch = *src++;
		    if ( ch == '\\' )
		    {
			uint code;
			src = ScanEscape(&code,src,src_end);
			ch = code;
		    }
		    *dest++ = ch;
		}
		if ( src < src_end )
		    src++;
	    }
	    else
		*dest++ = ch;
	}
	*dest++ = 0;
    }

    *argv_dest = 0;
    arg->argc = argv_dest - argv_buf;
    if ( argv_size )
    {
	DASSERT( argv_buf != argv_predef );
	arg->argv_size = argv_size;
	arg->argv = argv_buf;
    }
    else
    {
	DASSERT( argv_buf == argv_predef );
	arg->argv_size = arg->argc;
	arg->argv = MEMDUP(argv_buf,(arg->argv_size+1)*sizeof(*argv_dest));
    }

    return arg->argc;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct mem_t			///////////////
///////////////////////////////////////////////////////////////////////////////

mem_t EmptyMem	= {EmptyString,0};
mem_t NullMem	= {0,0};

///////////////////////////////////////////////////////////////////////////////

mem_t MidMem ( const mem_t src, int begin, int count )
{
    begin = CheckIndex1(src.len,begin);
    if ( count < 0 )
    {
	count  = -count;
	begin -= count;
	if ( begin < 0 )
	{
	    count += begin;
	    begin  = 0;
	}
    }

    mem_t res;
    res.ptr = src.ptr + begin;
    const int max = src.len - begin;
    res.len = count < max ? count : max;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t BeforeMem ( const mem_t src, ccp ref )
{
    mem_t res;
    res.ptr = src.ptr;
    res.len = ref < src.ptr		? 0
	    : ref <= src.ptr + src.len	? ref - src.ptr
	    : src.len;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t BehindMem ( const mem_t src, ccp ref )
{
    mem_t res;
    if ( ref <= src.ptr )
	res = src;
    else if ( ref <= src.ptr + src.len )
    {
	res.ptr = ref;
	res.len = src.ptr + src.len - ref;
    }
    else
    {
	res.ptr = src.ptr + src.len;
	res.len = 0;
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

int CmpMem ( const mem_t s1, const mem_t s2 )
{
    if ( s1.len < s2.len )
    {
	const int stat = memcmp(s1.ptr,s2.ptr,s1.len);
	return stat ? stat : -1;
    }

    const int stat = memcmp(s1.ptr,s2.ptr,s2.len);
    return stat ? stat : s1.len > s2.len;
}

///////////////////////////////////////////////////////////////////////////////

int StrCmpMem ( const mem_t mem, ccp str )
{
    const uint slen = str ? strlen(str) : 0;

    if ( mem.len < slen )
    {
	const int stat = memcmp(mem.ptr,str,mem.len);
	return stat ? stat : -1;
    }

    const int stat = memcmp(mem.ptr,str,slen);
    return stat ? stat : mem.len > slen;
}

///////////////////////////////////////////////////////////////////////////////

bool LeftStrCmpMemEQ ( const mem_t mem, ccp str )
{
    const uint slen = str ? strlen(str) : 0;
    return mem.len <= slen && !memcmp(mem.ptr,str,mem.len);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct mem_list_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeMemList ( mem_list_t *ml )
{
    DASSERT(ml);
    memset(ml,0,sizeof(*ml));
}

///////////////////////////////////////////////////////////////////////////////

void ResetMemList ( mem_list_t *ml )
{
    DASSERT(ml);
    FREE(ml->list);
    FREE(ml->buf);
    memset(ml,0,sizeof(*ml));
}

///////////////////////////////////////////////////////////////////////////////

void MoveMemList ( mem_list_t *dest, mem_list_t *src )
{
    if ( dest != src )
    {
	if (!dest)
	    ResetMemList(src);
	else
	{
	    ResetMemList(dest);
	    if (src)
	    {
		memcpy(dest,src,sizeof(*dest));
		memset(src,0,sizeof(*src));
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void GrowBufMemList ( mem_list_t *ml, uint need_size )
{
    if ( ml->buf_size < need_size )
    {
	PRINT("MEM-LIST: MALLOC/BUF(%u)\n",need_size);
	char *new_buf = MALLOC(need_size);

	//--- copy existing strings

	char *bufptr = new_buf;
	mem_t *dest = ml->list;
	uint i;
	for ( i = 0; i < ml->used; i++, dest++ )
	    if ( dest->len )
	    {
		memcpy(bufptr,dest->ptr,dest->len);
		dest->ptr = bufptr;
		bufptr += dest->len;
		*bufptr++ = 0;
		DASSERT( bufptr <= new_buf + need_size );
		noPRINT("OLD: %p,%u\n",dest->ptr,dest->len);
	    }
	FREE(ml->buf);
	ml->buf = new_buf;
	ml->buf_used = bufptr - new_buf;
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrepareMemList ( mem_list_t *ml, uint n_elem, uint buf_size )
{
    DASSERT(ml);
    if ( ml->size < n_elem )
    {
	ml->size = n_elem;
	PRINT("MEM-LIST: REALLOC/LIST(%u)\n",ml->size);
	ml->list = REALLOC(ml->list,sizeof(*ml->list)*ml->size);
    }

    GrowBufMemList(ml,buf_size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InsertMemListN
(
    mem_list_t		*ml,		// valid destination mem_list
    int			pos,		// insert position => CheckIndex1()
    const mem_t		*src,		// source list
    uint		src_len,	// number of elements in source list
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(ml);
    DASSERT( src || !src_len );

    //--- count needed size and elements

    const mem_t	*sptr, *src_end = src + src_len;

    uint need_size = 0, add_elem = 0;
    for ( sptr = src; sptr < src_end; sptr++ )
    {
	if ( sptr->len )
	{
	    need_size += sptr->len + 1; // always terminate with NULL
	    add_elem++;
	}
	else if ( !src_ignore || src_ignore == 1 && sptr->ptr || src_ignore == 3 )
	    add_elem++;
    }

    if (!add_elem)
	return;


    //--- string pool management

    if (!ml->used)
	ml->buf_used = 0;

    char *new_buf, *bufptr;
    if ( ml->buf_used + need_size <= ml->buf_size )
    {
	new_buf = 0;
	bufptr = ml->buf + ml->buf_used;
	ml->buf_used += need_size;
    }
    else
    {
	need_size += ml->buf_used + 20;
	need_size += need_size/10;

	// don't use GrowBufMemList() here, because the old buffer is needed
	// BUT: char old_buf = ml->buf; ml->buf = 0; GrowBufMemList(ml,need_size)

	PRINT("MEM-LIST: MALLOC/BUF(%u)\n",need_size);
	bufptr = new_buf = MALLOC(need_size);

	//--- copy existing strings

	mem_t *dest = ml->list;
	uint i;
	for ( i = 0; i < ml->used; i++, dest++ )
	    if ( dest->len )
	    {
		memcpy(bufptr,dest->ptr,dest->len);
		dest->ptr = bufptr;
		bufptr += dest->len;
		*bufptr++ = 0;
		DASSERT( bufptr <= new_buf+need_size );
		noPRINT("OLD: %p,%u\n",dest->ptr,dest->len);
	    }
    }


    //--- realloc list if necessary

    const uint need_elem = ml->used + add_elem;
    if ( need_elem > ml->size )
    {
	ml->size = need_elem + 5 + need_elem/10;
	PRINT("MEM-LIST: REALLOC/LIST(%u)\n",ml->size);
	ml->list = REALLOC(ml->list,sizeof(*ml->list)*ml->size);
    }

    pos = CheckIndex1(ml->used,pos);
    mem_t *dest = ml->list + pos;
    if ( pos < ml->used )
	memmove(dest+add_elem,dest,sizeof(*dest)*(ml->used-pos));
    ml->used += add_elem;


    //--- iterate source list

    for ( sptr = src; sptr < src_end; sptr++ )
    {
	if (sptr->len)
	{
	    dest->ptr = bufptr;
	    memcpy(bufptr,sptr->ptr,sptr->len);
	    dest->len = sptr->len;
	    bufptr += sptr->len;
	    *bufptr++ = 0;
	    DASSERT( bufptr <= ( new_buf ? new_buf+need_size : ml->buf + ml->buf_size ) );
	    noPRINT("NEW: %p,%u\n",dest->ptr,dest->len);
	    dest++;
	    DASSERT( dest <= ml->list + ml->used );
	}
	else if ( !src_ignore || src_ignore == 1 && sptr->ptr )
	{
	    dest->ptr = sptr->ptr ? EmptyString : NULL;
	    dest->len = 0;
	    dest++;
	    DASSERT( dest <= ml->list + ml->used );
	}
	else if ( src_ignore == 3 )
	{
	    dest->ptr = EmptyString;
	    dest->len = 0;
	    dest++;
	    DASSERT( dest <= ml->list + ml->used );
	}
    }
    DASSERT_MSG( dest == ml->list + pos + add_elem,
		"DEST= %zu != %u+%u == %u",
		dest-ml->list, pos, add_elem, pos+add_elem );


    //--- clean

    if (new_buf)
    {
	DASSERT(ml->buf != new_buf);
	FREE(ml->buf);
	ml->buf = new_buf;
	ml->buf_size = need_size;
    }
    ml->buf_used = bufptr - ml->buf;
    DASSERT( ml->buf_used <= ml->buf_size );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CatMemListN
(
    mem_list_t		*dest,		// valid destination mem_list
    const mem_list_t	**src_list,	// list with mem lists, element may be NULL
    uint		n_src_list,	// number of elements in 'src'
    uint		src_ignore	// 0: insert all
					// 1: ignore NULL
					// 2: ignore NULL and empty
					// 3: replace NULL by EmptyString
)
{
    DASSERT(dest);
    DASSERT( src_list || !n_src_list );


    //--- count needed size and elements

    uint need_size = 0, need_elem = 0;
    bool dest_is_source = false;

    const mem_list_t **list_ptr, **src_list_end = src_list + n_src_list;
    for ( list_ptr = src_list; list_ptr < src_list_end; list_ptr++ )
    {
	if (!*list_ptr)
	    continue;

	if ( *list_ptr == dest )
	    dest_is_source = true;

	const mem_t *sptr = (*list_ptr)->list, *send = sptr + (*list_ptr)->used;
	for ( ; sptr < send; sptr++ )
	{
	    if ( sptr->len )
	    {
		need_size += sptr->len + 1; // always terminate with NULL
		need_elem++;
	    }
	    else if ( !src_ignore || src_ignore == 1 && sptr->ptr || src_ignore == 3 )
		need_elem++;
	}
    }

    PRINT("CatMemList(N=%u) n_elem=%u, size=%u, dest_is_source=%d\n",
		n_src_list,need_elem,need_size,dest_is_source);
    if (!need_elem)
    {
	dest->used = 0;
	return;
    }


    //--- setup dest buffers

    char *new_buf = dest_is_source || need_size > dest->buf_size
			? MALLOC(need_size) : dest->buf;
    mem_t *new_list = dest_is_source || need_elem > dest->size
			? MALLOC(sizeof(*new_list)*need_elem) : dest->list;



    //--- assign values

    char *bufptr = new_buf;
    mem_t *listptr = new_list;

    for ( list_ptr = src_list; list_ptr < src_list_end; list_ptr++ )
    {
	if (!*list_ptr)
	    continue;

	const mem_t *sptr = (*list_ptr)->list, *send = sptr + (*list_ptr)->used;
	for ( ; sptr < send; sptr++ )
	{
	    if (sptr->len)
	    {
		listptr->ptr = bufptr;
		memcpy(bufptr,sptr->ptr,sptr->len);
		listptr->len = sptr->len;
		bufptr += sptr->len;
		*bufptr++ = 0;
		DASSERT( bufptr <= new_buf + need_size );
		noPRINT("NEW: %p,%u\n",dest->ptr,dest->len);
		listptr++;
		DASSERT( listptr <= new_list + need_elem );
	    }
	    else if ( !src_ignore || src_ignore == 1 && sptr->ptr )
	    {
		listptr->ptr = sptr->ptr ? EmptyString : NULL;
		listptr->len = 0;
		listptr++;
		DASSERT( listptr <= new_list + need_elem );
	    }
	    else if ( src_ignore == 3 )
	    {
		listptr->ptr = EmptyString;
		listptr->len = 0;
		listptr++;
		DASSERT( listptr <= new_list + need_elem );
	    }
	}
    }
    DASSERT( bufptr == new_buf + need_size );
    DASSERT( listptr == new_list + need_elem );


    //--- terminate

    dest->buf_used = need_size;
    if ( new_buf != dest->buf )
    {
	FREE(dest->buf);
	dest->buf = new_buf;
	dest->buf_size = need_size;
    }

    dest->used = need_elem;
    if ( new_list != dest->list )
    {
	FREE(dest->list);
	dest->list = new_list;
	dest->size = need_elem;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint LeftMemList ( mem_list_t *ml, int count )
{
    DASSERT(ml);
    return ml->used = CheckIndex1(ml->used,count);
}

///////////////////////////////////////////////////////////////////////////////

uint RightMemList ( mem_list_t *ml, int count )
{
    DASSERT(ml);

    const uint begin = CheckIndex1End(ml->used,-count);
    if (begin)
    {
	ml->used -= begin;
	memmove( ml->list, ml->list + begin, ml->used * sizeof(*ml->list) );
    }
    return ml->used;
}

///////////////////////////////////////////////////////////////////////////////

uint MidMemList ( mem_list_t *ml, int begin, int count )
{
    DASSERT(ml);

    begin = CheckIndex1(ml->used,begin);
    if ( count < 0 )
    {
	count  = -count;
	begin -= count;
	if ( begin < 0 )
	{
	    count += begin;
	    begin  = 0;
	}
    }

    const uint max = ml->used - begin;
    if ( count > max )
	count = max;
    memmove( ml->list, ml->list + begin, count * sizeof(*ml->list) );
    return ml->used = count;
}

///////////////////////////////////////////////////////////////////////////////

uint ExtractMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    ml->used = CheckIndex2(ml->used,&begin,&end);
    memmove( ml->list, ml->list + begin, ml->used * sizeof(*ml->list) );
    return ml->used;
}

///////////////////////////////////////////////////////////////////////////////

uint ExtractEndMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    ml->used = CheckIndex2End(ml->used,&begin,&end);
    memmove( ml->list, ml->list + begin, ml->used * sizeof(*ml->list) );
    return ml->used;
}

///////////////////////////////////////////////////////////////////////////////

uint RemoveMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    const uint sub = CheckIndex2(ml->used,&begin,&end);
    memmove( ml->list + begin, ml->list + end, ( ml->used - end ) * sizeof(*ml->list) );
    return ml->used -= sub;
}

///////////////////////////////////////////////////////////////////////////////

uint RemoveEndMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    const uint sub = CheckIndex2End(ml->used,&begin,&end);
    memmove( ml->list + begin, ml->list + end, ( ml->used - end ) * sizeof(*ml->list) );
    return ml->used -= sub;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    MemPool_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetMemPool ( MemPool_t *mp )
{
    DASSERT(mp);
    MemPoolChunk_t *ptr = mp->chunk;
    while (ptr)
    {
	MemPoolChunk_t *next = ptr->next;
	FREE(ptr);
	ptr = next;
    }
    InitializeMemPool(mp,mp->chunk_size);
}

///////////////////////////////////////////////////////////////////////////////

void * MallocMemPool ( MemPool_t *mp, uint size )
{
    DASSERT(mp);
    if (!size)
	return (char*)EmptyString;

    if ( size > mp->space )
    {
	uint data_size = size > mp->chunk_size ? size : mp->chunk_size;
	if ( data_size < 0x400 )
	    data_size = 0x400;
	MemPoolChunk_t *chunk = MALLOC(sizeof(MemPoolChunk_t)+data_size);
	DASSERT(chunk);
	chunk->next = mp->chunk;
	mp->chunk = chunk;
	mp->space = data_size;
    }

    DASSERT(mp->chunk);
    DASSERT(mp->space >= size );

    mp->space -= size;
    return mp->chunk->data + mp->space;
}

///////////////////////////////////////////////////////////////////////////////

void * CallocMemPool ( MemPool_t *mp, uint size )
{
    DASSERT(mp);
    void *res = MallocMemPool(mp,size);
    DASSERT(res);
    memset(res,0,size);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * MallocMemPoolA ( MemPool_t *mp, uint size, uint align )
{
    DASSERT(mp);
    char *res = MallocMemPool(mp,size);
    DASSERT(res);
    const uint fix = mp->space - mp->space / align * align;
    res -= fix;
    mp->space -= fix;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * CallocMemPoolA ( MemPool_t *mp, uint size, uint align )
{
    DASSERT(mp);
    void *res = MallocMemPoolA(mp,size,align);
    DASSERT(res);
    memset(res,0,size);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * MemDupMemPool ( MemPool_t *mp, cvp source, uint size )
{
    DASSERT(mp);
    DASSERT(source||!size);

    char *res = MallocMemPool(mp,size+1);
    memcpy(res,source,size);
    res[size] = 0;
    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    CircBuf_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[CircBuf_t]]

void InitializeCircBuf ( CircBuf_t *cb, uint size )
{
    DASSERT(cb);
    memset(cb,0,sizeof(*cb));
    cb->size = size; //data is alloced on first insertion
}

///////////////////////////////////////////////////////////////////////////////

void ResetCircBuf ( CircBuf_t *cb )
{
    DASSERT(cb);
    FREE(cb->buf);
    cb->buf = 0;
    cb->used = cb->pos = 0;
}

///////////////////////////////////////////////////////////////////////////////

uint WriteCircBuf ( CircBuf_t *cb, cvp data, uint size )
{
    DASSERT(cb);
    DASSERT(data||!size);

    if ( !size || !cb->size )
	return 0;

    if (!cb->buf)
    {
	cb->buf = MALLOC(cb->size);
	DASSERT(cb->buf);
	cb->used = cb->pos = 0;
    }

    ccp d = data;
    if ( size >= cb->size )
    {
	cb->used = cb->size;
	cb->pos = 0;
	memcpy( cb->buf, d + (size-cb->size), cb->size );
	//fprintf(stderr,"INS*: %d+%d = %d / %d\n",cb->pos,cb->used,cb->pos+cb->used,cb->size);
	return cb->size;
    }

    const int ins_count = size;
    int ins_pos = cb->pos + cb->used;
    if ( ins_pos >= cb->size )
	ins_pos -= cb->size;

    if ( ins_pos + size > cb->size )
    {
	const uint copy_len = cb->size - ins_pos;
	//fprintf(stderr,"COPY %u..%u\n",ins_pos,ins_pos+copy_len);
	memcpy( cb->buf + ins_pos, d, copy_len );
	size	-= copy_len;
	d	+= copy_len;
	ins_pos	 = 0;
    }

    DASSERT( size > 0 );
    DASSERT( ins_pos <= cb->size );
    DASSERT( ins_pos + size <= cb->size );
    //fprintf(stderr,"COPY %u..%u\n",ins_pos,ins_pos+size);
    memcpy( cb->buf + ins_pos, d, size );

    cb->used += ins_count;
    if ( cb->used > cb->size )
    {
	cb->pos += cb->used - cb->size;
	if ( cb->pos >= cb->size )
	    cb->pos -= cb->size;
	cb->used = cb->size;
    }

    //fprintf(stderr,"INS: %d+%d = %d / %d\n",cb->pos,cb->used,cb->pos+cb->used,cb->size);
    return ins_count;
}

///////////////////////////////////////////////////////////////////////////////

uint PeakCircBuf ( CircBuf_t *cb, char *buf, uint buf_size )
{
    DASSERT(cb);
    DASSERT(buf||!buf_size);

    if ( !cb->size || !buf_size )
	return 0;

    if ( buf_size > cb->used )
	buf_size = cb->used;

    uint count = cb->size - cb->pos;
    if ( count > buf_size )
	count -= buf_size;

    memcpy( buf, cb->buf + cb->pos, count );
    if ( count < buf_size )
    {
	const uint copy_len = buf_size - count;
	memcpy( buf+count, cb->buf, copy_len );
	count += copy_len;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

uint ReadCircBuf ( CircBuf_t *cb, char *buf, uint buf_size )
{
    DASSERT(cb);
    const uint count = PeakCircBuf(cb,buf,buf_size);
    return DropCircBuf(cb,count);
}

///////////////////////////////////////////////////////////////////////////////

uint DropCircBuf ( CircBuf_t *cb, uint size )
{
    DASSERT(cb);

    if ( size >= cb->used )
    {
	const uint res = cb->used;
	cb->pos = cb->used = 0;
	return res;
    }

    cb->used -= size;
    cb->pos += size;
    if ( cb->pos >= cb->size )
	cb->pos -= cb->size;
    return size;
}

///////////////////////////////////////////////////////////////////////////////

void ClearCircBuf ( CircBuf_t *cb )
{
    DASSERT(cb);
    cb->used = cb->pos = 0;
}

///////////////////////////////////////////////////////////////////////////////

uint PurgeCircBuf ( CircBuf_t *cb )
{
    DASSERT(cb);

    if ( cb->used && cb->pos )
    {
	char *temp = MALLOC(cb->size);
	PeakCircBuf(cb,temp,cb->used);
	FREE(cb->buf);
	cb->buf = temp;
    }
    cb->pos = 0;
    return cb->used;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    enum CopyMode_t interface		///////////////
///////////////////////////////////////////////////////////////////////////////

void * CopyData
(
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode,		// copy mode
    bool		*res_alloced	// not NULL:
					//   store true, if data must be freed
					//   otherwise store false
)
{
    DASSERT( data || !size );
    if ( !data || !size )
    {
	if (res_alloced)
	    *res_alloced = false;
	return (void*)EmptyString;
    }

    if (res_alloced)
	*res_alloced = mode != CPM_LINK;

    return mode == CPM_MOVE || mode == CPM_LINK
		? (void*)data
		: MEMDUP(data,size);
}

///////////////////////////////////////////////////////////////////////////////

void FreeData
(
    const void		*data,		// data to free
    CopyMode_t		mode		// copy mode
)
{
    if ( mode != CPM_LINK && data != EmptyString )
	FREE((void*)data);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		ContainerData_t, Container_t		///////////////
///////////////////////////////////////////////////////////////////////////////

Container_t * CreateContainer
(
    // returns 'c' or the alloced container
    // 'c' is always initialized

    Container_t		*c,		// if NULL: alloc a container
    int			protect,	// initial value for protection
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
)
{
    if (!c)
	c = MALLOC(sizeof(*c));
    memset(c,0,sizeof(*c));
    c->protect_level = protect;

    ContainerData_t *cdata = CALLOC(sizeof(*cdata),1);
    c->cdata = cdata;
    cdata->data = CopyData(data,size,mode,&cdata->data_alloced);
    cdata->size = size;
    cdata->ref_count = 1;
    if ( protect > 0 )
	cdata->protect_count++;
    return c;
}

///////////////////////////////////////////////////////////////////////////////

bool AssignContainer
(
    // return TRUE on new ContainerData_t

    Container_t		*c,		// valid container; if NULL: only FREE(data)
    int			protect,	// new protection level
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
)
{
    if (c)
    {
	SetProtectContainer(c,protect);

	if (!InContainerS(c,data,size))
	{
	    UnlinkContainerData(c);
	    ContainerData_t *cdata = CALLOC(sizeof(*cdata),1);
	    c->cdata = cdata;
	    cdata->data = CopyData(data,size,mode,&cdata->data_alloced);
	    cdata->size = size;
	    cdata->ref_count = 1;
	    if ( protect > 0 )
		cdata->protect_count++;
	    return true;
	}
    }

    if ( mode == CPM_MOVE )
	FREE((void*)data);
    return false;
}

///////////////////////////////////////////////////////////////////////////////

void ResetContainer
(
    Container_t		*c		// container to reset => no data
)
{
    if (c)
    {
	UnlinkContainerData(c);
	memset(c,0,sizeof(*c));
    }
}

///////////////////////////////////////////////////////////////////////////////

void DeleteContainer
(
    Container_t		*c		// container to reset and to free => no data
)
{
    if (c)
    {
	UnlinkContainerData(c);
	FREE(c);
    }
}

///////////////////////////////////////////////////////////////////////////////

void UnlinkContainerData
(
    Container_t		*c		// container to reset => no data
)
{
    if ( c && c->cdata )
    {
	ContainerData_t *cdata = c->cdata;
	if ( c->protect_level > 0 )
	    cdata->protect_count--;
	c->cdata = 0;
	if ( !--cdata->ref_count )
	{
	    if (cdata->data_alloced)
		FREE((void*)cdata->data);
	    FREE(cdata);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ContainerData_t * LinkContainerData
(
    // increment 'ref_count' and return NULL or current container-data
    // => use CatchContainerData() to complete operation
    Container_t		*c		// NULL or valid container
)
{
    if ( !c || !c->cdata )
	return 0;

    c->cdata->ref_count++;
    return c->cdata;
}

//-----------------------------------------------------------------------------

ContainerData_t * MoveContainerData
(
    // return NULL or unlinked current container-data
    // => use CatchContainerData() to complete operation
    Container_t		*c		// NULL or valid container
)
{
    if ( !c || !c->cdata )
	return 0;

    ContainerData_t *cdata = c->cdata;
    cdata->ref_count++;
    UnlinkContainerData(c);
    return cdata;
}

//-----------------------------------------------------------------------------

Container_t * CatchContainerData
(
    // returns 'c' or the alloced container
    // 'c' is always initialized

    Container_t		*c,		// if NULL: alloc a container
    int			protect,	// initial value for protection
    ContainerData_t	*cdata		// if not NULL: catch this
)
{    if (!c)
	c = MALLOC(sizeof(*c));
    memset(c,0,sizeof(*c));
    c->protect_level = protect;

    if (cdata)
    {
	c->cdata = cdata;
	if ( protect > 0 )
	    cdata->protect_count++;
    }
    return c;
}

//-----------------------------------------------------------------------------

void FreeContainerData
(
    ContainerData_t	*cdata		// NULL or container-data to free
)
{
    if ( cdata && !--cdata->ref_count )
    {
	if (cdata->data_alloced)
	    FREE((void*)cdata->data);
	FREE(cdata);
    }
}

///////////////////////////////////////////////////////////////////////////////

bool ModifyAllContainer
(
    // prepare modification of container-data, create a copy if necessary
    // return true, if a new container-data is used

    Container_t		*c		// NULL or valid container
)
{
    if ( c && c->cdata && !ModificationAllowed(c) )
    {
	ContainerData_t *oldcd	= c->cdata;
	ContainerData_t *newcd	= CALLOC(sizeof(*newcd),1);
	newcd->data		= MEMDUP(oldcd->data,oldcd->size);
	newcd->size		= oldcd->size;
	newcd->data_alloced	= true;
	newcd->ref_count	= 1;
	if ( c->protect_level > 0 )
	    newcd->protect_count++;
	UnlinkContainerData(c);
	c->cdata = newcd;
	return true;
    }
    return false;
}

//-----------------------------------------------------------------------------

bool ModifyContainer
(
    // prepare modification of container-data, create an extract if necessary
    // return true, if a new container-data is used

    Container_t		*c,		// NULL or valid container
    const void		*data,		// data to copy/move/link
    uint		size,		// size of 'data'
    CopyMode_t		mode		// copy mode on creation
)
{
    if ( c && c->cdata && ( !ModificationAllowed(c) || !InContainerS(c,data,size) ))
    {
	ContainerData_t *newcd	= CALLOC(sizeof(*newcd),1);
	newcd->data		= CopyData(data,size,mode,&newcd->data_alloced);
	newcd->size		= size;
	newcd->ref_count	= 1;
	if ( c->protect_level > 0 )
	    newcd->protect_count++;
	UnlinkContainerData(c);
	c->cdata = newcd;
	return true;
    }

    if ( mode == CPM_MOVE )
	FREE((void*)data);
    return false;
}

///////////////////////////////////////////////////////////////////////////////

int SetProtectContainer
(
    // returns 'c' new protection level
    Container_t		*c,		// if NULL: alloc a container
    int			new_protect	// new protection value
)
{
    DASSERT(c);
    if (c->cdata)
    {
	if ( c->protect_level > 0 && new_protect <= 0 )
	    c->cdata->protect_count--;
	else if ( c->protect_level <= 0 && new_protect > 0 )
	    c->cdata->protect_count++;
    }
    return c->protect_level = new_protect;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint DumpInfoContainer
(
    // return the number of printed lines

    FILE		*f,		// valid output file
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent the output
    ccp			prefix,		// not NULL: use it as prefix behind indention
    const Container_t	*c,		// dump infos for this container, NULL allowed
    uint		hexdump_len	// max number of bytes used for a hexdump
)
{
    DASSERT(f);
    if (!c)
	return 0;

    indent = NormalizeIndent(indent);
    if (!colset)
	colset = GetColorSet0();

    fprintf(f,"%*s%s%s%smod=%d, PL=%d, ",
	indent,"",
	colset->caption, prefix ? prefix : "",
	colset->status, ModificationAllowed(c), c->protect_level );
    uint line_count = DumpInfoContainerData(f,0,0,0,c->cdata,hexdump_len,indent+2);
    if (!line_count)
    {
	fputs("<no-data>",f);
	line_count++;
    }
    fprintf(f,"%s\n",colset->reset);
    return line_count;
}

///////////////////////////////////////////////////////////////////////////////

uint DumpInfoContainerData
(
    // return the number of printed lines

    FILE		*f,		// valid output file
    const ColorSet_t	*colset,	// NULL or color set
    int			indent,		// indent of output
    ccp			prefix,		// not NULL: use it as prefix behind indention
    const
      ContainerData_t	*cdata,		// dump infos for this container-data, NULL allowed
    uint		hexdump_len,	// max number of bytes used for a hexdump
    int			hexdump_indent	// indent of hexdump
)
{
    DASSERT(f);
    if (!cdata)
	return 0;

    indent = NormalizeIndent(indent);
    if (!colset)
	colset = GetColorSet0();

    fprintf(f,"%*s%s%s%sPC=%d, RC=%d, alloc=%d, size=%u=0x%x, addr=%p\n",
	indent,"",
	colset->caption, prefix ? prefix : "", colset->status,
	cdata->protect_count, cdata->ref_count, cdata->data_alloced,
	cdata->size, cdata->size, cdata->data );

    if ( hexdump_len > 0 && cdata->data && cdata->size )
    {
	if ( hexdump_len > cdata->size )
	    hexdump_len = cdata->size;
	return 1 + HexDump16(f,hexdump_indent,0,cdata->data,hexdump_len);
    }
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			split strings			///////////////
///////////////////////////////////////////////////////////////////////////////

uint SplitByCharMem
(
    // 'mem_list' will be padded with {EmptyString,0}

    mem_t		*mem_list,	// array of mem_t
    uint		n_mem,		// number of elements of 'mem_list'
    mem_t		source,		// source string
    char		sep		// separator character
)
{
    DASSERT(mem_list);
    DASSERT(n_mem>1);

    ccp src = source.ptr;
    ccp end = src + source.len;

    mem_t *mem = mem_list;
    mem_t *last = mem + n_mem - 1;

    while ( mem < last )
    {
	mem->ptr = src;
	while ( src < end && *src != sep )
	    src++;

	mem->len = src - mem->ptr;
	mem++;

	if ( ++src > end )
	    break;
    }

    if ( src <= end )
    {
	mem->ptr = src;
	mem->len = end - src;
	mem++;
    }
    const uint result = mem - mem_list;

    while ( mem <= last )
    {
	mem->ptr = EmptyString;
	mem->len = 0;
	mem++;
    }

    return result;
}

///////////////////////////////////////////////////////////////////////////////

uint SplitByTextMem
(
    // 'mem_list' will be padded with {EmptyString,0}

    mem_t		*mem_list,	// array of mem_t
    uint		n_mem,		// number of elements of 'mem_list'
    mem_t		source,		// source string
    mem_t		sep		// separator text
)
{
    DASSERT(mem_list);
    DASSERT(n_mem>0);

    ccp src = source.ptr;
    ccp end = src + source.len;

    mem_t *mem = mem_list;
    mem_t *last = mem + n_mem - 1;

    if ( sep.len > 0 && sep.len <= source.len )
    {
	ccp cmp_max = end - sep.len;
	while ( mem < last )
	{
	    mem->ptr = src;
	    while ( src <= cmp_max && memcmp(src,sep.ptr,sep.len) )
		src++;

	    if ( src > cmp_max )
	    {
		mem->len = end - mem->ptr;
		mem++;
		src = end + 1;
		break;
	    }

	    mem->len = src - mem->ptr;
	    mem++;
	    src += sep.len;
	}
    }

    if ( src <= end )
    {
	mem->ptr = src;
	mem->len = end - src;
	mem++;
    }
    const uint result = mem - mem_list;

    while ( mem <= last )
    {
	mem->ptr = EmptyString;
	mem->len = 0;
	mem++;
    }


    return result;
}

///////////////////////////////////////////////////////////////////////////////

uint SplitByCharMemList
(
    mem_list_t		*ml,		// valid destination mem_list
    bool		init_ml,	// true: initialize 'ml' first
    mem_t		source,		// source string
    char		sep,		// separator character
    int			max_fields	// >0: max number of result fields
)
{
    DASSERT(ml);
    if (init_ml)
	InitializeMemList(ml);
    ml->used = 0;

    if ( max_fields <= 0 )
	max_fields = INT_MAX;
    GrowBufMemList(ml,source.len+1);

    ccp src = source.ptr;
    ccp end = src + source.len;

    while ( max_fields-- > 1 )
    {
	mem_t mem;
	mem.ptr = src;
	while ( src < end && *src != sep )
	    src++;

	mem.len = src - mem.ptr;
	AppendMemListN(ml,&mem,1,0);
	if ( ++src > end )
	    break;
    }

    if ( src <= end )
    {
	mem_t mem = { src, end - src };
	AppendMemListN(ml,&mem,1,0);
    }

    return ml->used;
}

///////////////////////////////////////////////////////////////////////////////

uint SplitByTextMemList
(
    mem_list_t		*ml,		// valid destination mem_list
    bool		init_ml,	// true: initialize 'ml' first
    mem_t		source,		// source string
    mem_t		sep,		// separator text
    int			max_fields	// >0: max number of result fields
)
{
    DASSERT(ml);
    if (init_ml)
	InitializeMemList(ml);
    ml->used = 0;

    if ( max_fields <= 0 )
	max_fields = INT_MAX;
    GrowBufMemList(ml,source.len+1);

    ccp src = source.ptr;
    ccp end = src + source.len;

    if ( sep.len > 0 && sep.len <= source.len )
    {
	ccp cmp_max = end - sep.len;
	while ( max_fields-- > 1 )
	{
	    mem_t mem;
	    mem.ptr = src;
	    while ( src <= cmp_max && memcmp(src,sep.ptr,sep.len) )
		src++;

	    if ( src > cmp_max )
	    {
		mem.len = end - mem.ptr;
		AppendMemListN(ml,&mem,1,0);
		src = end + 1;
		break;
	    }

	    mem.len = src - mem.ptr;
	    AppendMemListN(ml,&mem,1,0);
	    src += sep.len;
	}
    }

    if ( src <= end )
    {
	mem_t mem = { src, end - src };
	AppendMemListN(ml,&mem,1,0);
    }

    return ml->used;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 MatchPattern()			///////////////
///////////////////////////////////////////////////////////////////////////////

static ccp AnalyseBrackets
(
    ccp		pattern,
    ccp		* p_start,
    bool	* p_negate,
    int		* p_multiple
)
{
    ASSERT(pattern);

    bool negate = false;
    if ( *pattern == '^' )
    {
	pattern++;
	negate = true;
    }
    if (p_negate)
	*p_negate = negate;

    int multiple = 0;
    if ( *pattern == '+' )
    {
	pattern++;
	multiple = 1;
    }
    else if ( *pattern == '*' )
    {
	pattern++;
	multiple = 2;
    }
    if (p_multiple)
	*p_multiple = multiple;

    if (p_start)
	*p_start = pattern;

    if (*pattern) // ']' allowed in first position
	pattern++;
    while ( *pattern && *pattern++ != ']' ) // find end
	;

    return pattern;
}

///////////////////////////////////////////////////////////////////////////////

static bool MatchBracktes
(
    char	ch,
    ccp		pattern,
    bool	negate
)
{
    if (!ch)
	return false;

    bool ok = false;
    ccp p = pattern;
    for (; !ok && *p && ( p == pattern || *p != ']' ); p++ )
    {
	if ( *p == '-' )
	{
	    if ( ch <= *++p && ch >= p[-2] )
	    {
		if (negate)
		    return false;
		ok = true;
	    }
	}
	else
	{
	    if ( *p == '\\' )
		p++;

	    if ( *p == ch )
	    {
		if (negate)
		    return false;
		ok = true;
	    }
	}
    }
    return ok || negate;
}

///////////////////////////////////////////////////////////////////////////////

static bool MatchPatternHelper
(
    ccp		pattern,
    ccp		text,
    bool	skip_end,
    int		alt_depth,
    char	path_sep	// path separator character, standard is '/'
)
{
    ASSERT(pattern);
    ASSERT(text);
    noTRACE(" - %d,%d |%s|%s|\n",skip_end,alt_depth,pattern,text);

    char ch;
    while ( ( ch = *pattern++ ) != 0 )
    {
	switch (ch)
	{
	   case '*':
		if ( *pattern == '*' )
		{
		    pattern++;
		    if (*pattern)
			while (!MatchPatternHelper(pattern,text,skip_end,alt_depth,path_sep))
			    if (!*text++)
				return false;
		}
		else
		{
		    while (!MatchPatternHelper(pattern,text,skip_end,alt_depth,path_sep))
			if ( *text == path_sep || !*text++ )
			    return false;
		}
		return true;

	    case '#':
		if ( *text < '0' || *text > '9' )
		    return false;
		while ( *text >= '0' && *text <= '9' )
			if (MatchPatternHelper(pattern,++text,skip_end,alt_depth,path_sep))
			    return true;
		return false;

	    case ' ':
		if ( *text < 1 || * text > ' ' )
		    return false;
		text++;
		break;

	    case '?':
		if ( !*text || *text == path_sep )
		    return false;
		text++;
		break;

	    case '[':
		{
		    ccp start;
		    bool negate;
		    int multiple;
		    TRACELINE;
		    pattern = AnalyseBrackets(pattern,&start,&negate,&multiple);
		    TRACELINE;

		    if ( multiple < 2 && !MatchBracktes(*text++,start,negate) )
			return false;

		    if (multiple)
		    {
			while (!MatchPatternHelper(pattern,text,skip_end,alt_depth,path_sep))
			    if (!MatchBracktes(*text++,start,negate))
				return false;
			return true;
		    }
		}
		break;

	   case '{':
		for (;;)
		{
		    if (MatchPatternHelper(pattern,text,skip_end,alt_depth+1,path_sep))
			return true;
		    // skip until next ',' || '}'
		    int skip_depth = 1;
		    while ( skip_depth > 0 )
		    {
			ch = *pattern++;
			switch(ch)
			{
			    case 0:
				return false;

			    case '\\':
				if (!*pattern)
				    return false;
				pattern++;
				break;

			    case '{':
				skip_depth++;
				break;

			    case ',':
				if ( skip_depth == 1 )
				    skip_depth--;
				break;

			    case '}':
				if (!--skip_depth)
				    return false;
				break;

			    case '[': // [2do] forgotten marker?
				pattern = AnalyseBrackets(pattern,0,0,0);
				break;
			}
		    }
		}

	   case ',':
		if (alt_depth)
		{
		    alt_depth--;
		    int skip_depth = 1;
		    while ( skip_depth > 0 )
		    {
			ch = *pattern++;
			switch(ch)
			{
			    case 0:
				return false;

			    case '\\':
				if (!*pattern)
				    return false;
				pattern++;
				break;

			    case '{':
				skip_depth++;
				break;

			    case '}':
				skip_depth--;
				break;

			    case '[': // [2do] forgotten marker?
				pattern = AnalyseBrackets(pattern,0,0,0);
				break;
			}
		    }
		}
		else if ( *text++ != ch )
		    return false;
		break;

	   case '}':
		if ( !alt_depth && *text++ != ch )
		    return false;
		break;

	   case '$':
		if ( !*pattern && !*text )
		    return true;
		if ( *text++ != ch )
		    return false;
		break;

	   case '\\':
		ch = *pattern++;
		// fall through

	   default:
		if ( *text++ != ch )
		    return false;
		break;
	}
    }
    return skip_end || *text == 0;
}

///////////////////////////////////////////////////////////////////////////////

bool MatchPatternFull
(
    ccp		pattern,	// pattern text
    ccp		text		// raw text
)
{
    TRACE("MatchPattern(|%s|%s|)\n",pattern,text);
    if ( !pattern || !*pattern )
	return true;

    if (!text)
	text = "";

    if ( *pattern == '^' )
	pattern++;

    return MatchPatternHelper(pattern,text,false,0,'/');
}

///////////////////////////////////////////////////////////////////////////////

bool MatchPattern
(
    ccp		pattern,	// pattern text
    ccp		text,		// raw text
    char	path_sep	// path separator character, standard is '/'
)
{
    TRACE("MatchPattern(|%s|%s|%c|)\n",pattern,text,path_sep);
    if ( !pattern || !*pattern )
	return true;

    if (!text)
	text = "";

    const size_t plen = strlen(pattern);
    ccp last = pattern + plen - 1;
    char last_ch = *last;
    int count = 0;
    while ( last > pattern && *--last == '\\' )
	count++;
    if ( count & 1 )
	last_ch = 0; // no special char!

    if ( *pattern == path_sep || *pattern == '^' )
    {
	pattern++;
	return MatchPatternHelper(pattern,text,last_ch!='$',0,path_sep);
    }

    while (*text)
	if (MatchPatternHelper(pattern,text++,last_ch!='$',0,path_sep))
	    return true;

    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp MatchStringField ( const StringField_t * sf, ccp key )
{
    DASSERT(sf);
    DASSERT(key);

    ccp *ptr = sf->field, *end;
    for ( end = ptr + sf->used; ptr < end; ptr++ )
	if (MatchPattern(*ptr,key,'/'))
	    return *ptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * MatchParamField ( const ParamField_t * pf, ccp key )
{
    DASSERT(pf);
    DASSERT(key);

    ParamFieldItem_t *ptr = pf->field, *end;
    for ( end = ptr + pf->used; ptr < end; ptr++ )
	if (MatchPattern(ptr->key,key,'/'))
	    return ptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * MatchRuleLine
(
    // returns a pointer to the first non scanned char

    int		*status,	// not NULL: return match status here
				//   -2: no prefix found  (no ruile found)
				//   -1: empty line (no ruile found)
				//    0: rule found, but don't match
				//    1: rule found and match
    ccp		src,		// source line, scanned until CONTROL
    char	rule_prefix,	// NULL or a rule-beginning-char'
    ccp		path,		// path to verify
    char	path_sep	// path separator character, standard is '/'
)
{
    int local_status;
    if (!status)
	status = &local_status;

    while ( *src > 0 && *src <= ' ' && *src != '\n' )
	src++;

    if (rule_prefix)
    {
	if ( *src != rule_prefix )
	{
	    *status = -2;
	    return (char*)src;
	}
	src++;
	while ( *src > 0 && *src <= ' ' && *src != '\n' )
	    src++;
    }

    ccp start = src;
    while ( (uchar)*src >= ' ' )
	src++;

    char buf[1000];
    uint len = src - start;
    if (!len)
    {
	*status = -1;
	return (char*)src;
    }

    if ( len > sizeof(buf)-1 )
	 len = sizeof(buf)-1;
    memcpy(buf,start,len);
    buf[len] = 0;

    *status = MatchPattern(buf,path,path_sep);
    return (char*)src;
}


//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan keywords			///////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t *GetKewordById
(
    const KeywordTab_t	*key_tab,	// NULL or pointer to command table
    s64			id		// id to search
)
{
    if (key_tab)
    {
	const KeywordTab_t *key;
	for ( key = key_tab; *key->name1; key++ )
	    if ( key->id == id )
		return key;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t *GetKewordByIdAndOpt
(
    const KeywordTab_t	*key_tab,	// NULL or pointer to command table
    s64			id,		// id to search
    s64			opt		// opt to search
)
{
    if (key_tab)
    {
	const KeywordTab_t *key;
	for ( key = key_tab; *key->name1; key++ )
	    if ( key->id == id && key->opt == opt )
		return key;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t * ScanKeyword
(
    int			* res_abbrev,	// NULL or pointer to result 'abbrev_count'
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab	// valid pointer to command table
)
{
    DASSERT(arg);
    char key_buf[KEYWORD_NAME_MAX];

    char *dest = key_buf;
    char *end  = key_buf + sizeof(key_buf) - 1;
    while ( *arg && *arg != ',' && *arg != ';' && dest < end )
	*dest++ = toupper((int)*arg++);
    *dest = 0;
    const int key_len = dest - key_buf;

    int favor_count = 0, abbrev_count = 0;
    const KeywordTab_t *ct, *key_ct = 0;
    const KeywordTab_t *favor_ct = 0, *favor_prev = 0, *abbrev_ct = 0, *abbrev_prev = 0;
    for ( ct = key_tab; ct->name1; ct++ )
    {
	if ( !strcmp(ct->name1,key_buf) )
	{
	    key_ct = ct;
	    break;
	}

	bool name2_abbrev = false;
	if (ct->name2)
	{
	    if (!strcmp(ct->name2,key_buf))
	    {
		key_ct = ct;
		break;
	    }
	    name2_abbrev = !memcmp(ct->name2,key_buf,key_len);
	}

	if ( name2_abbrev && !memcmp(ct->name1,key_buf,strlen(ct->name1)) )
	{
	    if (!favor_ct)
	    {
		favor_prev = favor_ct = ct;
		favor_count++;
	    }
	    else if ( favor_prev->id != ct->id || favor_prev->opt != ct->opt )
	    {
		favor_prev = ct;
		favor_count++;
	    }
	}

	if ( *key_buf == '_' ) // no abbreviations for commands beginning with '_'
	    continue;

	if ( name2_abbrev || !memcmp(ct->name1,key_buf,key_len) )
	{
	    if (!abbrev_ct)
	    {
		abbrev_prev = abbrev_ct = ct;
		abbrev_count++;
	    }
	    else if ( abbrev_prev->id != ct->id || abbrev_prev->opt != ct->opt )
	    {
		abbrev_prev = ct;
		abbrev_count++;
	    }
	}
    }

    if (key_ct)
	abbrev_count = 0;
    else if ( favor_count == 1 )
    {
	key_ct = favor_ct;
	abbrev_count = 0;
    }
    else if ( abbrev_count == 1 )
	key_ct = abbrev_ct;
    else if (!abbrev_count)
	abbrev_count = -1;

    if (res_abbrev)
	*res_abbrev = abbrev_count;

    return key_ct;
}

///////////////////////////////////////////////////////////////////////////////

s64 ScanKeywordListEx
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab,	// valid pointer to command table
    KeywordCallbackFunc	func,		// NULL or calculation function
    bool		allow_prefix,	// allow '-' | '+' | '=' as prefix
    u32			max_number,	// allow numbers < 'max_number' (0=disabled)
    s64			result,		// start value for result

    uint		err_mode,	// bit field:
					//	1: continue on error
    ccp			err_msg,	// not NULL: print a warning message:
					//   "<ERR_MSG>: Unknown keyword: <KEY>"
    enumError		err_code,	// use 'err_code' for the 'err_msg'
    uint		*err_count	// not NULL: store errors here
)
{
    ASSERT(arg);

    char key_buf[KEYWORD_NAME_MAX];
    char *end  = key_buf + sizeof(key_buf) - 1;
    uint err_cnt = 0;

    for (;;)
    {
	while ( *arg > 0 && *arg <= ' ' || *arg == ',' || *arg == '.' )
	    arg++;

	if ( !*arg || *arg == '#' )
	    break;

	char *dest = key_buf;
	while ( *arg > ' ' && *arg != ',' && *arg != '.' && *arg != '#'
		&& ( *arg != '+' || dest == key_buf ) && dest < end )
	    *dest++ = *arg++;
	*dest = 0;
	char prefix = 0;
	int abbrev_count;
	const KeywordTab_t * cptr = ScanKeyword(&abbrev_count,key_buf,key_tab);
	if ( !cptr && allow_prefix && key_buf[1]
	    && (   *key_buf == '+' || *key_buf == '-'
		|| *key_buf == '/' || *key_buf == '=' ))
	{
	    prefix = *key_buf == '/' ? '-' : *key_buf;
	    cptr = ScanKeyword(&abbrev_count,key_buf+1,key_tab);
	}

	KeywordTab_t ct_num = { 0, key_buf, 0, 0 };
	if ( max_number && abbrev_count )
	{
	    char * start = key_buf + (prefix!=0);
	    ulong num = str2l(start,&dest,10);
	    if ( num < max_number && dest > start && !*dest )
	    {
		ct_num.id = num;
		cptr = &ct_num;
	    }
	}

	noPRINT("cptr=%p [%s], opt=%llx, prefix=%02x\n",
		cptr, cptr ? cptr->name1 : "", cptr ? cptr->opt : 0, prefix );
	if ( !cptr || cptr->opt && prefix && prefix != '+' )
	{
	    if (err_msg)
	    {
		if ( abbrev_count > 1 )
		    ERROR0(err_code,"%s: Ambiguous keyword: %s\n",err_msg,key_buf);
		else
		    ERROR0(err_code,"%s: Unknown keyword: %s\n",err_msg,key_buf);
	    }
	    err_cnt++;
	    if ( err_mode & 1 )
		continue;
	    result = M1(result);
	    break;
	}

	if (func)
	{
	    const s64 res = func(0,key_buf,key_tab,cptr,prefix,result);
	    if ( res == M1(res) )
	    {
		if ( !(err_mode & 1) )
		{
		    result = res;
		    break;
		}
	    }
	    else
		result = res;
	}
	else
	{
	    switch (prefix)
	    {
		case 0:
		case '+': result  =  result & ~cptr->opt | cptr->id; break;
		case '-': result &= ~cptr->id; break;
		case '=': result  =  cptr->id; break;
	    }
	}
    }

    if (err_count)
	*err_count = err_cnt;
   return result;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanKeywordListFunc
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab,	// valid pointer to command table
    KeywordCallbackFunc	func,		// calculation function
    void		* param,	// used define parameter for 'func'
    bool		allow_prefix	// allow '-' | '+' | '=' as prefix
)
{
    ASSERT(arg);
    ASSERT(func);

    char key_buf[KEYWORD_NAME_MAX];
    char *end  = key_buf + sizeof(key_buf) - 1;

    for (;;)
    {
	while ( *arg > 0 && *arg <= ' ' || *arg == ','  || *arg == '.' )
	    arg++;

	if (!*arg)
	    return ERR_OK;

	char *dest = key_buf;
	while ( *arg > ' ' && *arg != ',' && *arg != '.'
		&& ( *arg != '+' || dest == key_buf ) && dest < end )
	    *dest++ = *arg++;
	*dest = 0;
	char prefix = 0;
	int abbrev_count;
	const KeywordTab_t * cptr = ScanKeyword(&abbrev_count,key_buf,key_tab);
	if ( !cptr && allow_prefix && key_buf[1]
	    && ( *key_buf == '+' || *key_buf == '-' || *key_buf == '/' || *key_buf == '=' ))
	{
	    prefix = *key_buf == '/' ? '-' : *key_buf;
	    cptr = ScanKeyword(&abbrev_count,key_buf+1,key_tab);
	}

	const enumError err = func(param,key_buf,key_tab,cptr,prefix,0);
	if (err)
	    return err;
    }
}

///////////////////////////////////////////////////////////////////////////////

static s64 ScanKeywordListMaskHelper
(
    void		* param,	// NULL or user defined parameter
    ccp			name,		// normalized name of option
    const KeywordTab_t	* key_tab,	// valid pointer to command table
    const KeywordTab_t	* key,		// valid pointer to found command
    char		prefix,		// 0 | '-' | '+' | '='
    s64			result		// current value of result
)
{
    return key->opt
		? result & ~key->opt | key->id
		: key->id;
}

//-----------------------------------------------------------------------------

s64 ScanKeywordListMask
(
    ccp			arg,		// argument to scan
    const KeywordTab_t	* key_tab	// valid pointer to command table
)
{
    return ScanKeywordList(arg,key_tab,ScanKeywordListMaskHelper,false,0,0,0,0);
}

///////////////////////////////////////////////////////////////////////////////

uint CollectAmbiguousKeywords
(
    char		*buf,		// destination buffer, 200 bytes are good
    uint		buf_size,	// size of buffer
    const KeywordTab_t	* key_tab,	// NULL or pointer to command table
    ccp			key_arg		// analyzed command
)
{
    DASSERT(buf);
    DASSERT(buf_size>10);
    DASSERT(key_tab);
    DASSERT(key_arg);

    int n = 0;
    char *dest = buf, *buf_end = buf + buf_size - 2;
    const int arg_len = strlen(key_arg);
    int last_id = -1;

    const KeywordTab_t *ct;
    for ( ct = key_tab; ct->name1 && dest < buf_end; ct++ )
    {
	if ( ct->id != last_id )
	{
	    ccp ok = 0;
	    if (!strncasecmp(key_arg,ct->name1,arg_len))
		ok = ct->name1;
	    else if ( ct->name2 && !strncasecmp(key_arg,ct->name2,arg_len))
		ok = ct->name2;
	    if (ok)
	    {
		if (!n++)
		{
		    *dest++ = ' ';
		    *dest++ = '[';
		}
		else if ( n > 5 )
		{
		    dest = StringCopyE(dest,buf_end,",...");
		    break;
		}
		else
		    *dest++ = ',';
		dest = StringCopyE(dest,buf_end,ok);
		last_id = ct->id;
	    }
	}
    }
    if ( dest > buf+1 )
	*dest++ = ']';
    else
	dest = buf;
    *dest = 0;
    return dest - buf;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintKeywordError
(
    const KeywordTab_t	* key_tab,	// NULL or pointer to command table
    ccp			key_arg,	// analyzed command
    int			key_stat,	// status of ScanKeyword()
    ccp			prefix,		// NULL or first prefix for messages
    ccp			object		// NULL or object for error messages
					//	default= 'command'
)
{
    DASSERT(key_arg);

    if ( !object || !*object )
	object = "command";

    if ( key_stat <= 0 )
	return ERROR0(ERR_SYNTAX,"%sUnknown %s: %s\n",
		prefix ? prefix : "", object, key_arg );

    char buf[1000], *dest = buf;
    if (key_tab)
    {
	int n = 0;
	ccp buf_end = buf + sizeof(buf) - 2;
	const int arg_len = strlen(key_arg);
	const KeywordTab_t *ct;
	int last_id = -1;

	for ( ct = key_tab; ct->name1 && dest < buf_end; ct++ )
	{
	    if ( ct->id != last_id )
	    {
		ccp ok = 0;
		if (!strncasecmp(key_arg,ct->name1,arg_len))
		    ok = ct->name1;
		else if ( ct->name2 && !strncasecmp(key_arg,ct->name2,arg_len))
		    ok = ct->name2;
		if (ok)
		{
		    if (!n++)
		    {
			*dest++ = ' ';
			*dest++ = '[';
		    }
		    else if ( n > 5 )
		    {
			dest = StringCopyE(dest,buf_end,",...");
			break;
		    }
		    else
			*dest++ = ',';
		    dest = StringCopyE(dest,buf_end,ok);
		    last_id = ct->id;
		}
	    }
	}
	if ( dest > buf+1 )
	    *dest++ = ']';
	else
	    dest = buf;
    }
    *dest = 0;

    return ERROR0(ERR_SYNTAX,"%s%c%s abbreviation is ambiguous: %s%s\n",
		prefix ? prefix : "",
		toupper((int)*object), object+1, key_arg, buf );
}

///////////////////////////////////////////////////////////////////////////////

char * PrintKeywordList
(
    // returns a pointer to the result buffer

    char		*buf,		// result buffer
					// If NULL, a local circulary static buffer
					// with max CIRC_BUF_MAX_ALLOC bytes is used
    uint		buf_size,	// size of 'buf', at least 10 bytes if buf is set
    uint		*ret_length,	// not NULL: store result length here

    const KeywordTab_t	*tab,		// related keyword table
    u64			mode,		// mode to print
    u64			default_mode,	// NULL or default mode
    u64			hide_mode	// bit field to hide parameters
)
{
    DASSERT( !buf || buf_size >= 10 );
    char temp[CIRC_BUF_MAX_ALLOC];
    if (!buf)
    {
	buf = temp;
	buf_size = sizeof(temp);
    }

    char *dest = buf;
    char *end = buf + buf_size - 1;

    mode = mode | hide_mode;
    u64 mode1 = mode;

    for ( ; tab->name1 && dest < end; tab++ )
    {
	if ( !tab->id || tab->opt & hide_mode )
	    continue;

	if ( tab->opt ? (mode & tab->opt) == tab->id : mode & tab->id )
	{
	    if ( dest > buf )
		*dest++ = ',';
	    dest = StringCopyE(dest,end,tab->name1);
	    mode &= ~(tab->id|tab->opt);
	}
    }

    if ( default_mode && mode1 == (default_mode|hide_mode) )
	dest = StringCopyE(dest,end," (default)");
    else if (!mode1)
	dest = StringCopyE(dest,end,"(none)");

    *dest = 0;
    const uint len = dest - buf;
    if (ret_length)
	*ret_length = len;

    if ( buf == temp )
    {
	buf = GetCircBuf(len+1);
	memcpy(buf,temp,len+1);
    }
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

int ScanKeywordOffOn
(
    // returns 0 for '0|OFF';  1 for '1|ON;  -1 for empty;  -2 on error

    ccp			arg,		// argument to scan
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
)
{
    if (!arg||!*arg)
	return -1;

    static const KeywordTab_t keytab[] =
    {
	{ 0,	"OFF",	"0",	0 },
	{ 1,	"ON",	"1",	0 },
	{0,0,0,0}
    };

    int status;
    const KeywordTab_t *key = ScanKeyword(&status,arg,keytab);
    if (key)
	return key->id;

    if ( max_num > 0 )
    {
	char *end;
	const uint num = str2ul(arg,&end,10);
	if ( !*end && num <= max_num )
	    return num;
    }

    if (object)
	PrintKeywordError(keytab,arg,status,0,object);

    return -2;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			OFF/AUTO/ON/FORCE		///////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t KeyTab_OFF_AUTO_ON[] =
{
  { OFFON_OFF,		"OFF",		"-1",	0 },
  { OFFON_OFF,		"DISABLED",	0,	0 },
  { OFFON_AUTO,		"AUTO",		"0",	0 },
  { OFFON_ON,		"ON",		"1",	0 },
  { OFFON_ON,		"ENABLED",	0,	0 },
  { OFFON_FORCE,	"FORCE",	"2",	0 },
  { 0,0,0,0 }
};

//-----------------------------------------------------------------------------

int ScanKeywordOffAutoOn
(
    // returns one of OFFON_* (OFFON_ON on empty argument)

    ccp			arg,		// argument to scan
    int			on_empty,	// return this value on empty
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
)
{
    if ( !arg || !*arg )
	return on_empty;

    int status;
    const KeywordTab_t *key = ScanKeyword(&status,arg,KeyTab_OFF_AUTO_ON);
    if (key)
	return key->id;

    if ( max_num > 0 )
    {
	char *end;
	const int num = str2l(arg,&end,10);
	if ( !*end && num >= OFFON_OFF && num <= max_num )
	    return num;
    }

    if (object)
	PrintKeywordError(KeyTab_OFF_AUTO_ON,arg,status,0,object);

    return OFFON_ERROR;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan command lists		///////////////
///////////////////////////////////////////////////////////////////////////////

int ExecCommandOfList ( CommandList_t *cli )
{
    DASSERT(cli);
    if ( cli->n_bin )
    {
	printf("%3u. %s [n_bin=%u] = |%.*s|\n",
	    cli->cmd_count, cli->cmd, cli->n_bin, cli->param_len, cli->param );
	uint i;
	for ( i = 0; i <  cli->n_bin; i++ )
	{
	    printf(" > binary #%u, len=%u\n",i,cli->bin[0].len);
	    HexDump16( stdout,5,0, cli->bin[i].ptr,
			cli->bin[i].len < 0x20 ? cli->bin[i].len : 0x20 );
	}
    }
    else
	printf("%3u. %s = |%.*s|\n",
	    cli->cmd_count, cli->cmd, cli->param_len, cli->param );
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int ScanCommandList
(
    // returns -1 on 'cli->fail_count'; otherwise 'cli->cmd_count'

    CommandList_t	*cli,	    // valid command list
    CommandListFunc	func	    // NULL or function for each command
)
{
    DASSERT(cli);

    const char sep = cli->command_sep ? cli->command_sep : ';';
    ccp cmd = cli->command, end = cmd + cli->command_len;
    for(;;)
    {
	cli->n_bin = 0;
	memset(cli->bin,0,sizeof(cli->bin));

	//--- skip blanks and terminators

	while ( cmd < end && ( (uchar)*cmd <= ' ' || *cmd == sep ))
	    cmd++;
	cli->scanned_len = cmd - cli->command;
	if ( cmd == end )
	    break;

	cli->record = (u8*)cmd;


	//--- find end of command+param

	ccp endcmd = memchr(cmd,sep,end-cmd);
	if (!endcmd)
	{
	    if (!cli->is_terminated)
		break;
	    endcmd = end;
	}


	//--- scan and copy command

	ccp param = cmd;
	while ( param < endcmd && ( isalnum(*param) || *param == '-' || *param == '_' ) )
	    param++;
	cli->cmd_len = param - cmd;
	if ( cli->cmd_len > sizeof(cli->cmd)-1 )
	     cli->cmd_len = sizeof(cli->cmd)-1;

	if (cli->change_case)
	{
	    char *dest = cli->cmd;
	    uint count = cli->cmd_len;
	    if ( cli->change_case == 1 )
		while ( count-- > 0 )
		    *dest++ = tolower((int)*cmd++);
	    else
		while ( count-- > 0 )
		    *dest++ = toupper((int)*cmd++);
	    *dest = 0;
	}
	else
	{
	    memcpy(cli->cmd,cmd,cli->cmd_len);
	    cli->cmd[cli->cmd_len] = 0;
	}


	//--- scan binary

	while ( cli->n_bin < COMMAND_LIST_N_BIN )
	{
	    while ( param < endcmd && ( *param == ' ' || *param == '\t' ))
		param++;

	    if ( param == endcmd || *param != '\1' )
		break;

	    const uint bin_len = be16(++param);
	    param += 2;
	    if ( param + bin_len >= end )
		goto abort; // definitley incomplete

	    cli->bin[cli->n_bin].ptr = param;
	    cli->bin[cli->n_bin].len = bin_len;
	    param += bin_len;
	    cli->n_bin++;

	    endcmd = memchr(param,sep,end-param);
	    if (!endcmd)
	    {
		if (!cli->is_terminated)
		    break;
		endcmd = end;
	    }
	}


	//--- calc record length

	cli->input_len = (u8*)endcmd - cli->record;
	if ( endcmd < end )
	    cli->input_len++;


	//--- scan param

	while ( param < endcmd && (uchar)*param <= ' ' )
	    param++;
	if ( param < endcmd && *param == '=' )
	{
	    param++;
	    while ( param < endcmd && (uchar)*param <= ' ' )
		param++;
	}

	ccp endparam = endcmd-1;
	while ( endparam > param && (uchar)*endparam <= ' ' )
	    endparam--;
	endparam++;

	if (cli->term_param)
	{
	    if ( cli->term_param > 1 )
	    {
		((char*)endparam)[0] = sep;
		endparam++;
	    }
	    else
		((char*)endparam)[0] = 0;
	}

	cli->param       = param;
	cli->param_len   = endparam - param;
	cli->record_len  = endparam - (ccp)cli->record;
	cli->scanned_len = endcmd   - (ccp)cli->record;
	cmd              = endcmd;


	//--- execute command

	if ( func && func(cli) < 0 )
	    break;
	cli->cmd_count++;
    }
 abort:;
    return cli->fail_count ? -1 : cli->cmd_count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global commands			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError Command_ARGTEST ( int argc, char ** argv )
{
    printf("ARGUMENT TEST: %u arguments:\n",argc);

    int idx;
    for ( idx = 0; idx < argc; idx++ )
	printf("%4u.: |%s|\n",idx,argv[idx]);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError Command_COLORS
(
    int		level,		// only used, if mode==NULL
				//  <  0: status message (ignore mode)
				//  >= 1: include names
				//  >= 2: include alt names
				//  >= 3: include color names incl bold
				//  >= 4: include background color names
    uint	mode,		// output mode => see PrintColorSetHelper()
    uint	format		// output format => see PrintColorSetEx()
)
{
    if (!colout)
	SetupStdMsg();

    if ( level < 0 )
    {
	printf("%s--color=%d [%s], colorize=%d [%s] / "
		"stdout: tty=%d, mode=%d [%s], have-color=%d, n-colors=%u%s\n",
		colout->status,
		opt_colorize, GetColorModeName(opt_colorize,"?"),
		colorize_stdout, GetColorModeName(colorize_stdout,"?"),
		isatty(fileno(stdout)),
		colout->col_mode, GetColorModeName(colout->col_mode,"?"),
		colout->colorize, colout->n_colors, colout->reset );
	return ERR_OK;
    }

    if ( !mode && level > 0 )
	switch (level)
	{
	    case 1:  mode = 0x08; break;
	    case 2:  mode = 0x18; break;
	    case 3:  mode = 0x1b; break;
	    default: mode = 0x1f; break;
	}

    if (!format)
    {
	const ColorMode_t col_mode = colout ? colout->col_mode : COLMD_AUTO;
	PrintTextModes( stdout, 4, col_mode );
	PrintColorModes( stdout, 4, col_mode, GCM_ALT );
     #ifdef TEST
	PrintColorModes( stdout, 4, col_mode, GCM_ALT|GCM_SHORT );
     #endif
    }
    if ( mode || format )
	PrintColorSetEx(stdout,4,colout,mode,format);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		global commands: TESTCOLORS		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[color_info_t]]

typedef struct color_info_t
{
    bool	valid;		// >0: data below is valid

    u8		m_index;	// index of 0..255 for "\e[*m"

    s8		m_red;		// -1:invalid, 0..5:  red value of 'm_index'
    s8		m_green;	// -1:invalid, 0..5:  green value of 'm_index'
    s8		m_blue;		// -1:invalid, 0..5:  blue value of 'm_index'
    s8		m_gray;		// -1:invalid, 0..23: gray value of 'm_index'

    u32		ref_color;	// reference colors, based on 'm_index';
    u32		src_color;	// scanned color
}
color_info_t;

///////////////////////////////////////////////////////////////////////////////

static bool AssignColorInfo ( color_info_t *ci, u32 rgb )
{
    DASSERT(ci);
    memset(ci,0,sizeof(*ci));

    ci->src_color = rgb & 0xffffff;
    ci->m_index   = ConvertColorRGBToM256(ci->src_color);
    ci->ref_color = ConvertColorM256ToRGB(ci->m_index);

    const uint r = ci->ref_color >> 16 & 0xff;
    const uint g = ci->ref_color >>  8 & 0xff;
    const uint b = ci->ref_color       & 0xff;

    ci->m_red   = SingleColorToM6(r);
    ci->m_green = SingleColorToM6(g);
    ci->m_blue  = SingleColorToM6(b);

    ci->m_gray  = ( r + g + b + 21 ) / 30;
    if ( ci->m_gray > 23 )
	ci->m_gray = 23;
    else if ( ci->m_gray > 0 )
	ci->m_gray--;

    return ci->valid = 1;
}

///////////////////////////////////////////////////////////////////////////////

static bool ScanColorInfo ( color_info_t *ci, ccp arg )
{
    DASSERT(ci);
    memset(ci,0,sizeof(*ci));
    if ( arg && *arg )
    {
	arg = SkipControls(arg);

	ulong num, r, g, b;
	if ( *arg >= 'g' && *arg <= 'z' )
	{
	    const char mode = *arg;
	    arg = SkipControls(arg+1);

	    num = str2ul(arg,0,10);
	    switch (mode)
	    {
		case 'm':
		    return AssignColorInfo(ci,ConvertColorM256ToRGB(num));

		case 'g':
		    return AssignColorInfo(ci,ConvertColorM256ToRGB(232 + num % 24));

		case 'c':
		    r = num / 100 % 10;
		    g = num / 10  % 10;
		    b = num       % 10;
		    goto rgb5;
	    }
	}
	else
	{
	    char *end;
	    num = str2ul(arg,&end,16);

	    ci->valid = true;
	    if ( end - arg <= 3 )
	    {
		r = num >> 8 & 15;
		g = num >> 4 & 15;
		b = num      & 15;

		rgb5:
		if ( r > 5 ) r = 5;
		if ( g > 5 ) g = 5;
		if ( b > 5 ) b = 5;
		return AssignColorInfo(ci,ConvertColorM256ToRGB(16 + 36*r + 6*g + b));
	    }
	    return AssignColorInfo(ci,num);
	}
    }

    return ci->valid;
}

///////////////////////////////////////////////////////////////////////////////

static void PrintColorInfo ( FILE *f, color_info_t *ci )
{
    DASSERT(f);
    DASSERT(ci);

    if (ci->valid)
	fprintf(f," %06x -> %06x : %3u : %u,%u,%u : %2u"
		" \e[48;5;16;38;5;%um test \e[0m"
		" \e[48;5;244;38;5;%um test \e[0m"
		" \e[48;5;231;38;5;%um test \e[0m "
		" \e[38;5;16;48;5;%um test \e[0m"
		" \e[38;5;244;48;5;%um test \e[0m"
		" \e[38;5;231;48;5;%um test \e[0m\n",
		ci->src_color, ci->ref_color,
		ci->m_index, ci->m_red, ci->m_green, ci->m_blue, ci->m_gray,
		ci->m_index, ci->m_index, ci->m_index,
		ci->m_index, ci->m_index, ci->m_index );
    else
	fputs(" --\n",f);
}

///////////////////////////////////////////////////////////////////////////////

enumError Command_TESTCOLORS ( int argc, char ** argv )
{
 #if HAVE_PRINT
    GetColorByName(colout,"blue");
 #endif

    printf("TEST COLORS: %u arguments:\n",argc);

    bool sep = true;
    int idx;
    for ( idx = 0; idx < argc; idx++ )
    {
	ccp minus = strchr(argv[idx],'-');
	if ( minus || sep )
	{
	    putchar('\n');
	    sep = false;
	    if ( strlen(argv[idx]) == 1 )
		continue;
	}

	color_info_t ci1;
	ScanColorInfo(&ci1,argv[idx]);
	PrintColorInfo(stdout,&ci1);

	if (!minus)
	    continue;

	color_info_t ci2;
	ScanColorInfo(&ci2,minus+1);
	if ( ci2.ref_color == ci1.ref_color )
	    continue;

	int r1 = ci1.ref_color >> 16 & 0xff;
	int g1 = ci1.ref_color >>  8 & 0xff;
	int b1 = ci1.ref_color       & 0xff;
	int r2 = ci2.ref_color >> 16 & 0xff;
	int g2 = ci2.ref_color >>  8 & 0xff;
	int b2 = ci2.ref_color       & 0xff;

	int n_steps = abs( r2 - r1 );
	int diff = abs( g2 - g1 );
	if ( n_steps < diff )
	    n_steps = diff;
	diff = abs( b2 - b1 );
	if ( n_steps < diff )
	    n_steps = diff;

	u8 prev_m = ci1.m_index;
	u32 prev_rgb = ci1.ref_color;
	int i;
	for ( i = 1; i <= n_steps; i++ )
	{
	    uint r = ( r2 - r1 ) * i / n_steps + r1;
	    uint g = ( g2 - g1 ) * i / n_steps + g1;
	    uint b = ( b2 - b1 ) * i / n_steps + b1;
	    u8 new_m = ConvertColorRGB3ToM256(r,g,b);
	    if ( prev_m != new_m )
	    {
		prev_m = new_m;
		color_info_t ci3;
		AssignColorInfo(&ci3,r<<16|g<<8|b);
		if ( prev_rgb != ci3.ref_color )
		{
		    prev_rgb = ci3.src_color = ci3.ref_color;
		    PrintColorInfo(stdout,&ci3);
		}
	    }
	}
	sep = true;
    }
    putchar('\n');
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			color helpers			///////////////
///////////////////////////////////////////////////////////////////////////////
//
// https://en.wikipedia.org/wiki/ANSI_escape_code
// https://jonasjacek.github.io/colors/
//
// 0..15:  000000 800000 008000 808000  000080 800080 008080 c0c0c0
//	   808080 ff0000 00ff00 ffff00  0000ff ff00ff 00ffff ffffff
//		=> real colors varies on implementation
//
// 6x6x6: 0 95 135 175 215 255 == 00 5f 87 af d7 ff
//
// gray:  8 18 28 ... 238 == 08 12 1c 26 30 3a ... e4 ee f8
//
///////////////////////////////////////////////////////////////////////////////
//
//  mkw-ana testcol 000000-ffffff
//  mkw-ana testcol 500-550 550-050 050-055 055-005 005-505 505-500
//
///////////////////////////////////////////////////////////////////////////////

u32 ColorTab_M0_M15[16] =
{
	0x000000, 0x800000, 0x008000, 0x808000,
	0x000080, 0x800080, 0x008080, 0xc0c0c0,
	0x808080, 0xff0000, 0x00ff00, 0xffff00,
	0x0000ff, 0xff00ff, 0x00ffff, 0xffffff,
};

///////////////////////////////////////////////////////////////////////////////

u8 ConvertColorRGB3ToM256 ( u8 r, u8 g, u8 b )
{
    // 0..5
    const uint r6 = SingleColorToM6(r);
    const uint g6 = SingleColorToM6(g);
    const uint b6 = SingleColorToM6(b);

    u8	 m256	= 36*r6 + 6*g6 + b6 + 16;
    uint delta	= abs ( ( r6 ? 55 + 40 * r6 : 0 ) - r  )
		+ abs ( ( g6 ? 55 + 40 * g6 : 0 ) - g  )
		+ abs ( ( b6 ? 55 + 40 * b6 : 0 ) - b  );

    //printf("%02x>%u %02x>%u %02x>%u : m=%u delta=%d\n",r,r6,g,g6,b,b6,m256,delta);

    uint m, gray, prev = 0x1000000;
    for ( m = 232, gray = 8; m < 256; m++, gray += 10 )
    {
	const uint d = abs( gray - r ) + abs( gray - g ) + abs( gray - b );
	if ( d > prev )
	    break;
	if ( d <= delta )
	{
	    delta = d;
	    m256  = m;
	}
	prev = d;
    }

    return m256;
}

///////////////////////////////////////////////////////////////////////////////

u8 ConvertColorRGBToM256 ( u32 rgb )
{
    return ConvertColorRGB3ToM256( rgb >> 16, rgb >> 8, rgb );
}

///////////////////////////////////////////////////////////////////////////////

u32 ConvertColorM256ToRGB ( u8 m256 )
{
    if ( m256 < 16 )
	return ColorTab_M0_M15[m256];

    if ( m256 < 232 )
    {
	m256 -= 16;
	const uint r =  m256 / 36;
	const uint g =  m256 / 6 % 6;
	const uint b =  m256 % 6;
	
	u32 col = 0;
	if (r) col |= 0x370000 + 0x280000 * r;
	if (g) col |= 0x003700 + 0x002800 * g;
	if (b) col |= 0x000037 + 0x000028 * b;
	return col;
    }

    m256 -= 232;
    return 0x080808 + m256 * 0x0a0a0a;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			dynamic data support		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetDD ( DynData_t *dd )
{
    if (dd)
    {
	FREE(dd->data);
	dd->data = 0;
	dd->len = dd->size = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void InsertDD ( DynData_t *dd, const void *data, uint data_size )
{
    DASSERT(dd);
    DASSERT( !dd->data == !dd->size );
    DASSERT( dd->len <= dd->size );

    if ( !data || !data_size )
	dd->len = 0;
    else
    {
	if ( data_size > dd->size )
	{
	    FREE(dd->data);
	    dd->data = MALLOC(data_size);
	    dd->size = data_size;
	}

	memcpy(dd->data,data,data_size);
	dd->len = data_size;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ResetDDL ( DynDataList_t *ddl )
{
    if (ddl)
    {
	DynData_t *dd  = ddl->list;
	DynData_t *end = dd + ddl->used;
	for ( ; dd < end; dd++ )
	    FREE(dd->data);

	FREE(ddl->list);
	ddl->list = 0;
	ddl->used = ddl->size = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

DynData_t *InsertDDL
(
    DynDataList_t	*ddl,		// valid dynamic data list
    uint		index,		// element index to change
    const void		*data,		// NULL or data to assign
    uint		data_size	// size of data
)
{
    DASSERT(ddl);
    DASSERT( !ddl->list == !ddl->size );
    DASSERT( ddl->used <= ddl->size );

    if ( index >= ddl->size )
    {
	const uint new_size = 11 * index / 10 + 10;
	ddl->list = REALLOC( ddl->list, sizeof(*ddl->list) * new_size );
	memset( ddl->list + ddl->size, 0, (new_size - ddl->size) * sizeof(*ddl->list) );
	ddl->size = new_size;
    }

    if ( ddl->used <= index )
    {
	uint idx;
	for ( idx = ddl->used; idx < index; idx++ )
	    ddl->list[idx].len = 0;
	ddl->used = index+1;
    }

    DynData_t *dd = ddl->list + index;
    InsertDD(dd,data,data_size);
    return dd;
}

///////////////////////////////////////////////////////////////////////////////

DynData_t *GetDDL
(
    DynDataList_t	*ddl,		// valid dynamic data list
    uint		index,		// element index to get
    uint		create		// create an empty element if not exist
)
{
    DASSERT(ddl);
    DASSERT( !ddl->list == !ddl->size );
    DASSERT( ddl->used <= ddl->size );

    return index < ddl->used
		? ddl->list + ddl->used
		: create
		    ? InsertDDL(ddl, index >= create ? index : create-1, 0,0)
		    : 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  StringField_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetStringField ( StringField_t * sf )
{
    DASSERT(sf);
    if (sf)
    {
	if ( sf->used > 0 )
	{
	    DASSERT(sf->field);
	    ccp *ptr = sf->field, *end;
	    for ( end = ptr + sf->used; ptr < end; ptr++ )
		FreeString(*ptr);
	}
	FREE(sf->field);
    }
    InitializeStringField(sf);
}

///////////////////////////////////////////////////////////////////////////////

void MoveStringField ( StringField_t * dest, StringField_t * src )
{
    DASSERT(src);
    DASSERT(dest);
    if ( src != dest )
    {
	ResetStringField(dest);
	dest->field	= src->field;
	dest->used	= src->used;
	dest->size	= src->size;
	dest->func_cmp	= src->func_cmp;

	InitializeStringField(src);
	src->func_cmp	= dest->func_cmp;
    }
}

///////////////////////////////////////////////////////////////////////////////

int FindStringFieldIndex ( const StringField_t * sf, ccp key, int not_found_value )
{
    bool found;
    const int idx = FindStringFieldHelper(sf,&found,key);
    return found ? idx : not_found_value;
}

///////////////////////////////////////////////////////////////////////////////

ccp FindStringField ( const StringField_t * sf, ccp key )
{
    bool found;
    const int idx = FindStringFieldHelper(sf,&found,key);
    return found ? sf->field[idx] : 0;
}

///////////////////////////////////////////////////////////////////////////////

ccp * InsertStringFieldHelper ( StringField_t * sf, int idx )
{
    DASSERT(sf);
    DASSERT( sf->used <= sf->size );
    noPRINT("+SF: %u/%u/%u\n",idx,sf->used,sf->size);
    if ( sf->used == sf->size )
    {
	sf->size += 0x100;
	sf->field = REALLOC(sf->field,sf->size*sizeof(*sf->field));
    }
    DASSERT( idx <= sf->used );
    ccp * dest = sf->field + idx;
    memmove(dest+1,dest,(sf->used-idx)*sizeof(*dest));
    sf->used++;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

bool InsertStringField ( StringField_t * sf, ccp key, bool move_key )
{
    if (!key)
	return 0;

    bool found;
    int idx = FindStringFieldHelper(sf,&found,key);
    if (found)
    {
	if (move_key)
	    FreeString(key);
    }
    else
    {
	ccp * dest = InsertStringFieldHelper(sf,idx);
	*dest = move_key ? key : STRDUP(key);
    }

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoveStringField ( StringField_t * sf, ccp key )
{
    bool found;
    uint idx = FindStringFieldHelper(sf,&found,key);
    if (found)
    {
	sf->used--;
	ASSERT( idx <= sf->used );
	ccp * dest = sf->field + idx;
	FreeString(*dest);
	memmove(dest,dest+1,(sf->used-idx)*sizeof(*dest));
    }
    return found;
}

///////////////////////////////////////////////////////////////////////////////

uint RemoveStringFieldByIndex ( StringField_t * sf, int index, int n )
{
    DASSERT(sf);

    if ( index < 0 )
	index += sf->used;
    if ( n < 0 )
    {
	index += n;
	n -= n;
    }

    if ( index < 0 )
	index = 0;
    int last = index + n;
    if ( last > sf->used )
	last = sf->used;
    if ( index >= last )
	return 0;

    ccp * src = sf->field + index;
    ccp * end = sf->field + last;
    while ( src < end )
	FreeString(*src++);

    memmove( sf->field + index, src, ( sf->used - last) * sizeof(*sf->field) );
    const uint rm = last - index;
    sf->used -= rm;
    return rm;
}

///////////////////////////////////////////////////////////////////////////////

void AppendStringField ( StringField_t * sf, ccp key, bool move_key )
{
    DASSERT(sf);
    if (key)
    {
	DASSERT( sf->used <= sf->size );
	noPRINT(">SF: %u/%u\n",sf->used,sf->size);
	if ( sf->used == sf->size )
	{
	    sf->size += 0x100;
	    sf->field = REALLOC(sf->field,sf->size*sizeof(*sf->field));
	}
	TRACE("AppendStringField(%s,%d) %d/%d\n",key,move_key,sf->used,sf->size);
	ccp * dest = sf->field + sf->used++;
	*dest = move_key ? key : STRDUP(key);
    }
}

///////////////////////////////////////////////////////////////////////////////

void SortStringField ( StringField_t * sf )
{
    DASSERT(sf);
    if ( sf->used > 1 )
    {
	StringField_t temp;
	InitializeStringField(&temp);
	temp.func_cmp = sf->func_cmp;
	uint i;
	for ( i = 0; i < sf->used; i++ )
	    InsertStringField(&temp,sf->field[i],true);
	sf->used = 0;
	MoveStringField(sf,&temp);
    }
}

///////////////////////////////////////////////////////////////////////////////

uint FindStringFieldHelper ( const StringField_t * sf, bool * p_found, ccp key )
{
    DASSERT(sf);

    int (*cmp)( ccp s1, ccp s2 ) = sf->func_cmp ? sf->func_cmp : strcmp;

    int beg = 0;
    if ( sf && key )
    {
	int end = sf->used - 1;
	while ( beg <= end )
	{
	    const uint idx = (beg+end)/2;
	    const int stat = cmp(key,sf->field[idx]);
	    if ( stat < 0 )
		end = idx - 1;
	    else if ( stat > 0 )
		beg = idx + 1;
	    else
	    {
		noTRACE("FindStringFieldHelper(%s) FOUND=%d/%d/%d\n",
			key, idx, sf->used, sf->size );
		if (p_found)
		    *p_found = true;
		return idx;
	    }
	}
    }

    noTRACE("FindStringFieldHelper(%s) failed=%d/%d/%d\n",
		key, beg, sf->used, sf->size );

    if (p_found)
	*p_found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError LoadStringField
(
    StringField_t	* sf,		// string field
    bool		init_sf,	// true: initialize 'sf' first
    bool		sort,		// true: sort 'sf'
    ccp			filename,	// filename of source file
    bool		silent		// true: don't print open/read errors
)
{
    ASSERT(sf);
    ASSERT(filename);
    ASSERT(*filename);

    TRACE("LoadStringField(%p,%d,%d,%s,%d)\n",sf,init_sf,sort,filename,silent);

    if (init_sf)
	InitializeStringField(sf);

    FILE * f = fopen(filename,"rb");
    if (!f)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",filename);
	return ERR_CANT_OPEN;
    }

    char iobuf[10000];
    while (fgets(iobuf,sizeof(iobuf)-1,f))
    {
	char * ptr = iobuf;
	while (*ptr)
	    ptr++;
	if ( ptr > iobuf && ptr[-1] == '\n' )
	{
	    ptr--;
	    if ( ptr > iobuf && ptr[-1] == '\r' )
		ptr--;
	}

	if ( ptr > iobuf )
	{
	    *ptr++ = 0;
	    const size_t len = ptr-iobuf;
	    ptr = MALLOC(len);
	    memcpy(ptr,iobuf,len);
	    if (sort)
		InsertStringField(sf,ptr,true);
	    else
		AppendStringField(sf,ptr,true);
	}
    }

    fclose(f);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SaveStringField
(
    StringField_t	* sf,		// valid string field
    ccp			filename,	// filename of dest file
    bool		rm_if_empty	// true: rm dest file if 'sf' is empty
)
{
    ASSERT(sf);
    ASSERT(filename);
    ASSERT(*filename);

    TRACE("SaveStringField(%p,%s,%d)\n",sf,filename,rm_if_empty);

    if ( !sf->used && rm_if_empty )
    {
	unlink(filename);
	return ERR_OK;
    }

    FILE * f = fopen(filename,"wb");
    if (!f)
	return ERROR1(ERR_CANT_CREATE,"Can't create file: %s\n",filename);

    enumError err = WriteStringField(f,filename,sf,0,0);
    fclose(f);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError WriteStringField
(
    FILE		* f,		// open destination file
    ccp			filename,	// NULL or filename (needed on write error)
    StringField_t	* sf,		// valid string field
    ccp			prefix,		// not NULL: insert prefix before each line
    ccp			eol		// end of line text (if NULL: use LF)
)
{
    if (!prefix)
	prefix = "";
    if (!eol)
	eol = "\n";

    ccp *ptr = sf->field, *end;
    for ( end = ptr + sf->used; ptr < end; ptr++ )
	if ( fprintf(f,"%s%s%s",prefix,*ptr,eol) < 0 )
	    return ERROR1(ERR_WRITE_FAILED,
			"Error while writing string list: %s\n",
			filename ? filename : "?" );
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateStringField
(
    FILE		*f,		// output file
    ccp			name_prefix,	// NULL or prefix of name, up to 40 chars
    uint		tab_pos,	// tab pos of '='
    const StringField_t	*sf,		// valid string field
    EncodeMode_t	emode		// encoding mode
)
{
    DASSERT(f);
    DASSERT(sf);

    if (!name_prefix)
	name_prefix = EmptyString;

    int base = 8*tab_pos + 7 - strlen(name_prefix);
    fprintf(f,"%sn%.*s= %u\n", name_prefix, (base-1)/8, Tabs20, sf->used );

    char buf[5000], name[10];

    uint i;
    for ( i = 0; i < sf->used; i++ )
    {
	const uint len = snprintf(name,sizeof(name),"%u",i);
	mem_t mem = EncodeByModeMem(buf,sizeof(buf),sf->field[i],-1,emode);
	fprintf(f, "%s%s%.*s= %s\n",
		name_prefix, name, (base-len)/8, Tabs20, mem.ptr );
	if ( mem.ptr != buf )
	    FreeMem(&mem);
    }
}

///////////////////////////////////////////////////////////////////////////////

void RestoreStateStringField
(
    RestoreState_t	*rs,		// info data, may be modified
    ccp			name_prefix,	// NULL or prefix of name, up to 40 chars
    StringField_t	*sf,		// string field
    bool		init_sf,	// true: initialize 'sf', otherwise reset
    EncodeMode_t	emode		// encoding mode
)
{
    DASSERT(rs);
    DASSERT(sf);

    if (init_sf)
	InitializeStringField(sf);
    else
	ResetStringField(sf);

    if (!name_prefix)
	name_prefix = EmptyString;

    char name[50];
    snprintf(name,sizeof(name),"%sn",name_prefix);
    int n = GetParamFieldINT(rs,name,0);
    PRINT("RestoreStateStringField(), N=%d\n",n);
    if ( n > 0 )
    {
	sf->size = n;
	sf->field = MALLOC(sf->size*sizeof(*sf->field));

	uint i;
	for ( i = 0; i < n; i++ )
	{
	    snprintf(name,sizeof(name),"%s%u",name_prefix,i);
	    ParamFieldItem_t *pfi = GetParamField(rs,name);
	    if (pfi)
	    {
		mem_t mem = DecodeByModeMem(0,0,pfi->data,-1,emode);
		AppendStringField(sf,mem.ptr,true);
	    }
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  ParamField_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeParamField ( ParamField_t * pf )
{
    DASSERT(pf);
    memset(pf,0,sizeof(*pf));
}

///////////////////////////////////////////////////////////////////////////////

void ResetParamField ( ParamField_t * pf )
{
    DASSERT(pf);
    if (pf)
    {
	if ( pf->used > 0 )
	{
	    DASSERT(pf->field);
	    ParamFieldItem_t *ptr = pf->field, *end;
	    for ( end = ptr + pf->used; ptr < end; ptr++ )
	    {
		FreeString(ptr->key);
		if (pf->free_data)
		    FREE(ptr->data);
	    }
	}
	FREE(pf->field);
    }

    const bool free_data = pf->free_data;
    InitializeParamField(pf);
    pf->free_data = free_data;
}

///////////////////////////////////////////////////////////////////////////////

void MoveParamField ( ParamField_t * dest, ParamField_t * src )
{
    DASSERT(src);
    DASSERT(dest);
    if ( src != dest )
    {
	ResetParamField(dest);
	dest->field	= src->field;
	dest->used	= src->used;
	dest->size	= src->size;
	dest->free_data	= src->free_data;
	InitializeParamField(src);
	src->free_data	= dest->free_data;
    }
}

///////////////////////////////////////////////////////////////////////////////

int FindParamFieldIndex ( const ParamField_t * pf, ccp key, int not_found_value )
{
    bool found;
    const int idx = FindParamFieldHelper(pf,&found,key);
    return found ? idx : not_found_value;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * FindParamField ( const ParamField_t * pf, ccp key )
{
    bool found;
    const int idx = FindParamFieldHelper(pf,&found,key);
    return found ? pf->field + idx : 0;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * FindParamFieldByNum ( const ParamField_t * pf, uint num )
{
    DASSERT(pf);
    noTRACE("FindParamFieldByNum(%x)\n",num);

    ParamFieldItem_t *ptr = pf->field, *end;
    for ( end = ptr + pf->used; ptr < end; ptr++ )
	if ( ptr->num == num )
	    return ptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static ParamFieldItem_t * InsertParamFieldHelper ( ParamField_t * pf, int idx )
{
    DASSERT(pf);
    DASSERT( pf->used <= pf->size );
    noPRINT("+SF: %u/%u/%u\n",idx,pf->used,pf->size);
    if ( pf->used == pf->size )
    {
	pf->size += 0x100;
	pf->field = REALLOC(pf->field,pf->size*sizeof(*pf->field));
    }
    DASSERT( idx <= pf->used );
    ParamFieldItem_t * dest = pf->field + idx;
    memmove(dest+1,dest,(pf->used-idx)*sizeof(*dest));
    pf->used++;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * FindInsertParamField
	( ParamField_t * pf, ccp key, bool move_key, bool *old_found )
{
    if (!key)
	return 0;

    noTRACE("InsertParamField(%s,%x)\n",key,num);

    bool found;
    int idx = FindParamFieldHelper(pf,&found,key);
    ParamFieldItem_t *item;
    if (found)
    {
	if (move_key)
	    FreeString(key);
	item = pf->field + idx;
    }
    else
    {
	item = InsertParamFieldHelper(pf,idx);
	item->key  = move_key ? key : STRDUP(key);
	item->num  = 0;
	item->data = 0;
    }

    if (old_found)
	*old_found = found;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * CountParamField ( ParamField_t * pf, ccp key, bool move_key )
{
    ParamFieldItem_t *item = FindInsertParamField(pf,key,move_key,0);
    if (item)
	item->num++;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

bool InsertParamField
	( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data )
{
    if (!key)
	return 0;

    noTRACE("InsertParamField(%s,%x)\n",key,num);

    bool found;
    int idx = FindParamFieldHelper(pf,&found,key);
    if (found)
    {
	if (move_key)
	    FreeString(key);
	if (pf->free_data)
	    FREE((void*)data);
    }
    else
    {
	ParamFieldItem_t * dest = InsertParamFieldHelper(pf,idx);
	dest->key  = move_key ? key : STRDUP(key);
	dest->num  = num;
	dest->data = (void*)data;
    }

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * InsertParamFieldEx
	( ParamField_t * pf, ccp key, bool move_key, uint num, bool *p_found )
{
    if (!key)
	return 0;

    noTRACE("InsertParamField(%s,%x)\n",key,num);

    bool found;
    int idx = FindParamFieldHelper(pf,&found,key);
    if (p_found)
	*p_found = found;

    if (found)
    {
	if (move_key)
	    FreeString(key);
	return pf->field + idx;
    }

    ParamFieldItem_t * dest = InsertParamFieldHelper(pf,idx);
    dest->key  = move_key ? key : STRDUP(key);
    dest->num  = num;
    dest->data = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

bool ReplaceParamField
	( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data )
{
    if (!key)
	return 0;

    noTRACE("InsertParamField(%s,%x)\n",key,num);

    bool found;
    int idx = FindParamFieldHelper(pf,&found,key);
    if (found)
    {
	if (move_key)
	    FreeString(key);

	ParamFieldItem_t * dest = pf->field + idx;
	dest->num = num;
	FREE(dest->data);
	dest->data = (void*)data;
    }
    else
    {
	ParamFieldItem_t * dest = InsertParamFieldHelper(pf,idx);
	dest->key  = move_key ? key : STRDUP(key);
	dest->num  = num;
	dest->data = (void*)data;
    }

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoveParamField ( ParamField_t * pf, ccp key )
{
    bool found;
    uint idx = FindParamFieldHelper(pf,&found,key);
    if (found)
    {
	pf->used--;
	ASSERT( idx <= pf->used );
	ParamFieldItem_t * dest = pf->field + idx;
	FreeString(dest->key);
	if (pf->free_data)
	    FREE(dest->data);
	memmove(dest,dest+1,(pf->used-idx)*sizeof(*dest));
    }
    return found;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * AppendParamField
	( ParamField_t * pf, ccp key, bool move_key, uint num, cvp data )
{
    DASSERT(pf);
    if (key)
    {
	DASSERT( pf->used <= pf->size );
	noPRINT(">SF: %u/%u\n",pf->used,pf->size);
	if ( pf->used == pf->size )
	{
	    pf->size += 0x100;
	    pf->field = REALLOC(pf->field,pf->size*sizeof(*pf->field));
	}
	TRACE("AppendParamField(%s,%d) %d/%d\n",key,move_key,pf->used,pf->size);
	ParamFieldItem_t * dest = pf->field + pf->used++;
	dest->key  = move_key ? key : STRDUP(key);
	dest->num  = num;
	dest->data = (void*)data;
	return dest;
    }

    if (pf->free_data)
	FREE((void*)data);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint FindParamFieldHelper ( const ParamField_t * pf, bool * p_found, ccp key )
{
    DASSERT(pf);

    int (*cmp)( ccp s1, ccp s2 ) = pf->func_cmp ? pf->func_cmp : strcmp;

    int beg = 0;
    if ( pf && key )
    {
	int end = pf->used - 1;
	while ( beg <= end )
	{
	    uint idx = (beg+end)/2;
	    int stat = cmp(key,pf->field[idx].key);
	    if ( stat < 0 )
		end = idx - 1 ;
	    else if ( stat > 0 )
		beg = idx + 1;
	    else
	    {
		noTRACE("FindParamFieldHelper(%s) FOUND=%d/%d/%d\n",
			key, idx, pf->used, pf->size );
		if (p_found)
		    *p_found = true;
		return idx;
	    }
	}
    }

    noTRACE("FindParamFieldHelper(%s) failed=%d/%d/%d\n",
		key, beg, pf->used, pf->size );

    if (p_found)
	*p_found = false;
    return beg;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Memory Maps			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeMemMap ( MemMap_t * mm )
{
    DASSERT(mm);
    memset(mm,0,sizeof(*mm));
}

///////////////////////////////////////////////////////////////////////////////

void ResetMemMap ( MemMap_t * mm )
{
    DASSERT(mm);

    uint i;
    if (mm->field)
    {
	for ( i = 0; i < mm->used; i++ )
	    FREE(mm->field[i]);
	FREE(mm->field);
    }
    memset(mm,0,sizeof(*mm));
}

///////////////////////////////////////////////////////////////////////////////

void CopyMemMap
(
    MemMap_t		* mm1,		// merged mem map, not NULL, cleared
    const MemMap_t	* mm2,		// NULL or second mem map
    bool		use_tie		// TRUE: use InsertMemMapTie()
)
{   
    DASSERT(mm1);
    ResetMemMap(mm1);
    MergeMemMap(mm1,mm2,use_tie);
}

///////////////////////////////////////////////////////////////////////////////

void MergeMemMap
(
    MemMap_t		* mm1,		// merged mem map, not NULL, not cleared
    const MemMap_t	* mm2,		// NULL or second mem map
    bool		use_tie		// TRUE: use InsertMemMapTie()
)
{
    DASSERT(mm1);
    if (mm2)
    {
	uint i;
	for ( i = 0; i < mm2->used; i++ )
	{
	    const MemMapItem_t *src = mm2->field[i];
	    if (src)
	    {
		MemMapItem_t *it = use_tie
			? InsertMemMapTie(mm1,src->off,src->size)
			: InsertMemMap(mm1,src->off,src->size);
		if (it)
		    StringCopyS(it->info,sizeof(it->info),src->info);
	    }
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * FindMemMap ( MemMap_t * mm, off_t off, off_t size )
{
    DASSERT(mm);

    off_t off_end = off + size;
    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	MemMapItem_t * mi = mm->field[idx];
	if ( off_end <= mi->off )
	    end = idx - 1 ;
	else if ( off >= mi->off + mi->size )
	    beg = idx + 1;
	else
	    return mi;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint InsertMemMapIndex
(
    // returns the index of the new item

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
)
{
    DASSERT(mm);
    uint idx = FindMemMapHelper(mm,off,size);

    DASSERT( mm->used <= mm->size );
    if ( mm->used == mm->size )
    {
	mm->size += 64;
	mm->field = REALLOC(mm->field,mm->size*sizeof(*mm->field));
    }

    DASSERT( idx <= mm->used );
    MemMapItem_t ** dest = mm->field + idx;
    memmove(dest+1,dest,(mm->used-idx)*sizeof(*dest));
    mm->used++;

    MemMapItem_t * mi = MALLOC(sizeof(MemMapItem_t));
    mi->off  = off;
    mi->size = size;
    mi->overlap = 0;
    *dest = mi;
    return idx;
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * InsertMemMap
(
    // returns a pointer to a new item (never NULL)

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
)
{
    const uint idx = InsertMemMapIndex(mm,off,size);
    // a C sequence point is important here
    return mm->field[idx];
}

///////////////////////////////////////////////////////////////////////////////

static bool TieMemMap
(
    // returns true if element are tied togehther

    MemMap_t		* mm,		// mem map pointer
    uint		idx,		// tie element 'idx' and 'idx+1'
    bool		force		// always tie and not only if overlapped
)
{
    DASSERT(mm);
    DASSERT( idx+1 < mm->used );

    MemMapItem_t * i1 = mm->field[idx];
    MemMapItem_t * i2 = mm->field[idx+1];
    if ( force || i1->off + i1->size >= i2->off )
    {
	const off_t new_size = i2->off + i2->size - i1->off;
	if ( i1->size < new_size )
	     i1->size = new_size;
	FREE(i2);
	idx++;
	mm->used--;
	memmove( mm->field + idx,
		 mm->field + idx + 1,
		 ( mm->used - idx ) * sizeof(MemMapItem_t*) );

	return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

MemMapItem_t * InsertMemMapTie
(
    // returns a pointer to a new or existing item (never NULL)

    MemMap_t		* mm,		// mem map pointer
    off_t		off,		// offset of area
    off_t		size		// size of area
)
{
    uint idx = InsertMemMapIndex(mm,off,size);

    if ( idx > 0 && TieMemMap(mm,idx-1,false) )
	idx--;

    while ( idx + 1 < mm->used && TieMemMap(mm,idx,false) )
	;

    return mm->field[idx];
}

///////////////////////////////////////////////////////////////////////////////

void InsertMemMapWrapper
(
	void		* param,	// user defined parameter
	u64		offset,		// offset of object
	u64		size,		// size of object
	ccp		info		// info about object
)
{
    noTRACE("InsertMemMapWrapper(%p,%llx,%llx,%s)\n",param,offset,size,info);
    DASSERT(param);
    MemMapItem_t * mi = InsertMemMap(param,offset,size);
    StringCopyS(mi->info,sizeof(mi->info),info);
}

///////////////////////////////////////////////////////////////////////////////

uint FindMemMapHelper ( MemMap_t * mm, off_t off, off_t size )
{
    DASSERT(mm);

    int beg = 0;
    int end = mm->used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	MemMapItem_t * mi = mm->field[idx];
	if ( off < mi->off )
	    end = idx - 1 ;
	else if ( off > mi->off )
	    beg = idx + 1;
	else if ( size < mi->size )
	    end = idx - 1 ;
	else if ( size > mi->size )
	    beg = idx + 1;
	else
	{
	    TRACE("FindMemMapHelper(%llx,%llx) FOUND=%d/%d/%d\n",
		    (u64)off, (u64)size, idx, mm->used, mm->size );
	    return idx;
	}
    }

    TRACE("FindStringFieldHelper(%llx,%llx) failed=%d/%d/%d\n",
		(u64)off, (u64)size, beg, mm->used, mm->size );
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

uint CalcOverlapMemMap ( MemMap_t * mm )
{
    DASSERT(mm);

    uint i, count = 0;
    MemMapItem_t * prev = 0;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	ptr->overlap = 0;
	if ( prev && ptr->off < prev->off + prev->size )
	{
	    ptr ->overlap |= 1;
	    prev->overlap |= 2;
	    count++;
	}
	prev = ptr;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

u64 FindFreeSpaceMemMap
(
    // return address or NULL on not-found

    const MemMap_t	* mm,
    u64			addr_beg,	// first possible address
    u64			addr_end,	// last possible address
    u64			size,		// minimal size
    u64			align,		// aligning
    u64			*space		// not NULL: store available space here
)
{
    DASSERT(mm);

    if (!size)
	goto term;

    if (!align)
	align = 1;
    addr_beg = ALIGN64(addr_beg,align);

    uint i;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t *mi = mm->field[i];
	const u64 end = ALIGN64( mi->off + mi->size, align );
	if ( mi->off > addr_beg )
	{
	    u64 found_space = ALIGNOFF64(mi->off,align) - addr_beg;
	    if ( size <= found_space )
	    {
		if (space)
		    *space = found_space;
		return addr_beg;
	    }
	}
	if ( addr_beg < end )
	    addr_beg = end;
    }

    u64 found_space = ALIGNOFF64(addr_end,align) - addr_beg;
    if ( size <= found_space )
    {
	if (space)
	    *space = found_space;
	return addr_beg;
    }

 term:
    if (space)
	*space = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void PrintMemMap ( MemMap_t * mm, FILE * f, int indent, ccp info_head )
{
    DASSERT(mm);
    if ( !f || !mm->used )
	return;

    CalcOverlapMemMap(mm);
    indent = NormalizeIndent(indent);

    static char ovl[][3] = { "  ", "!.", ".!", "!!" };

    if (!info_head)
	info_head = "info";
    int i, max_ilen = strlen(info_head);
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	ptr->info[sizeof(ptr->info)-1] = 0;
	const int ilen = strlen(ptr->info);
	if ( max_ilen < ilen )
	    max_ilen = ilen;
    }

    fprintf(f,"%*s      unused :  off(beg) ..  off(end) :      size : %s\n%*s%.*s\n",
	    indent, "", info_head,
	    indent, "", max_ilen+52, Minus300 );

    off_t max_end = mm->begin;
    for ( i = 0; i < mm->used; i++ )
    {
	MemMapItem_t * ptr = mm->field[i];
	if ( !i && max_end > ptr->off )
	    max_end = ptr->off;
	const off_t end = ptr->off + ptr->size;
	if ( ptr->off > max_end )
	    fprintf(f,"%*s%s%10llx :%10llx ..%10llx :%10llx : %s\n",
		indent, "", ovl[ptr->overlap&3], (u64)( ptr->off - max_end ),
		(u64)ptr->off, (u64)end, (u64)ptr->size, ptr->info );
	else
	    fprintf(f,"%*s%s           :%10llx ..%10llx :%10llx : %s\n",
		indent, "", ovl[ptr->overlap&3],
		(u64)ptr->off, (u64)end, (u64)ptr->size, ptr->info );
	if ( max_end < end )
	    max_end = end;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    Memory Map: Scan addresses		///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanAddress
(
    // returns first not scanned char of 'arg'

    ScanAddr_t	* result,	// result, initialized by ScanAddress()
    ccp		arg,		// format: ADDR | ADDR:END | ADDR#SIZE
    uint	default_base	// base for numbers without '0x' prefix
				//  0: C like with octal support
				// 10: standard value for decimal numbers
				// 16: standard value for hex numbers
)
{
    DASSERT(result);
    memset(result,0,sizeof(*result));
    if (!arg)
	return 0;

    ccp end = ScanUINT(&result->addr,arg,default_base);
    if ( end == arg )
	return (char*)arg;
    arg = end;
    while ( *arg == ' ' || *arg == '\t' )
	arg++;
    result->size = 1;
    result->stat = 1;

    if ( *arg == ':' )
    {
	uint addr2;
	end = ScanUINT(&addr2,++arg,default_base);
	if ( end == arg )
	    return(char*) arg-1;

	result->size = addr2 > result->addr ? addr2 - result->addr : 0;
	result->stat = 2;
    }
    else if ( *arg == '#' )
    {
	end = ScanUINT(&result->size,++arg,default_base);
	result->stat = 2;
    }
    
    return (char*)end;
}

///////////////////////////////////////////////////////////////////////////////

uint InsertAddressMemMap
(
    // returns the number of added addresses

    MemMap_t	* mm,		// mem map pointer
    bool	use_tie,	// TRUE: use InsertMemMapTie()
    ccp		arg,		// comma separated list of addresses
				// formats: ADDR | ADDR:END | ADDR#SIZE
    uint	default_base	// base for numbers without '0x' prefix
				//  0: C like with octal support
				// 10: standard value for decimal numbers
				// 16: standard value for hex numbers
)
{
    DASSERT(mm);
    if (!arg)
	return 0;

    uint count = 0;
    while (*arg)
    {
	ScanAddr_t addr;
	arg = ScanAddress(&addr,arg,default_base);
	if (addr.stat)
	{
	    if (use_tie)
		InsertMemMapTie(mm,addr.addr,addr.size);
	    else
		InsertMemMap(mm,addr.addr,addr.size);
	    count++;
	}

	while ( *arg && *arg != ' ' && *arg != '\t' && *arg != ',' )
	    arg++;
	while ( *arg == ' ' || *arg == '\t' || *arg == ',' )
	    arg++;
    }

    return count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    DataBuf_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeDataBuf
	( DataBuf_t *db, uint buf_size, DataBufFill_t func, void *user_data )
{
    DASSERT(db);
    memset(db,0,sizeof(*db));

    if (buf_size)
    {
	db->size = buf_size;
	db->buf = MALLOC(buf_size);
	db->buf_end = db->buf + buf_size;
	db->data = db->data_end = db->buf;
    }

    db->fill_func = func;
    db->user_data = user_data;
    DASSERT(!DumpDataBuf(stderr,0,db,true));
}

///////////////////////////////////////////////////////////////////////////////

void ResetDataBuf ( DataBuf_t *db )
{
    if (db)
    {
	DASSERT(!DumpDataBuf(stderr,0,db,true));
	FREE(db->buf);
	memset(db,0,sizeof(*db));
	DASSERT(!DumpDataBuf(stderr,0,db,true));
    }
}

///////////////////////////////////////////////////////////////////////////////

void ResizeDataBuf ( DataBuf_t *db, uint new_buf_size )
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));
    DASSERT(new_buf_size>0);

    if (!db->buf)
	return;

    PackDataBuf(db);
    db->buf = REALLOC(db->buf,new_buf_size);
    db->buf_end = db->buf + new_buf_size;
    db->size = new_buf_size;
    if ( db->used > new_buf_size )
	 db->used = new_buf_size;
    db->data = db->buf;
    db->data_end = db->data + db->used;

    DASSERT(!DumpDataBuf(stderr,0,db,true));
}

///////////////////////////////////////////////////////////////////////////////

void ClearDataBuf ( DataBuf_t *db, uint new_buf_size_if_not_null )
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    db->offset = 0;
    db->data = db->data_end = db->buf;
    db->used = 0;
    db->offset = 0;
    if (new_buf_size_if_not_null)
	ResizeDataBuf(db,new_buf_size_if_not_null);

    DASSERT(!DumpDataBuf(stderr,0,db,true));
}

///////////////////////////////////////////////////////////////////////////////

void EmptyDataBuf ( DataBuf_t *db )
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    db->data = db->data_end = db->buf;
    db->used = 0;

    DASSERT(!DumpDataBuf(stderr,0,db,true));
}

///////////////////////////////////////////////////////////////////////////////

void PackDataBuf ( DataBuf_t *db )
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    if ( !db->used || !db->buf )
    {
	db->data = db->data_end = db->buf;
    }
    else if ( db->data > db->buf )
    {
	if ( db->data < db->data_end )
	{
	    // easy case: FREE1 DATA FREE2

	    memmove(db->buf,db->data,db->used);
	}
	else
	{
	    // case: DATA2 FREE DATA1

	    uint d1len = db->buf_end  - db->data;
	    uint d2len = db->data_end - db->buf;
	    DASSERT( d1len + d2len == db->used );

 #ifdef TEST0
	    u8 temp[10];
 #else
	    u8 temp[0x8000];
 #endif
	    if ( d2len <= sizeof(temp) && d2len < d1len )
	    {
		memcpy(temp,db->buf,d2len);
		memmove(db->buf,db->data,d1len);
		memcpy(db->buf+d1len,temp,d2len);
	    }
	    else if ( d1len <= sizeof(temp) )
	    {
		memcpy(temp,db->data,d1len);
		memmove(db->buf+d1len,db->buf,d2len);
		memcpy(db->buf,temp,d1len);
	    }
	    else
	    {
		noPRINT("PACK %u+%u/%u, temp=%zu\n",d2len,d1len,db->size,sizeof(temp));
		// use a dynamic buffer => [2do]] optimize later
		u8 *temp = MEMDUP(db->buf,d2len);
		memmove(db->buf,db->data,d1len);
		memcpy(db->buf+d1len,temp,d2len);
		FREE(temp);
	    }
	}
	db->data = db->buf;
	db->data_end = db->data + db->used;
    }

    DASSERT(!DumpDataBuf(stderr,0,db,true));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// debugging

bool IsValidDataBuf ( DataBuf_t *db )
{
    if ( !db || db->used > db->size )
	return false;

    if (db->buf)
    {
	if ( db->buf_end != db->buf + db->size
		|| db->data < db->buf || db->data >= db->buf_end )
	    return false;

	u8 *end = db->data + db->used;
	if ( end > db->buf_end )
	    end -= db->size;
	return end == db->data_end;
    }

    return !db->size && !db->buf_end && !db->data && !db->data_end;
}

///////////////////////////////////////////////////////////////////////////////

bool DumpDataBuf ( FILE *f, uint indent, DataBuf_t *db, bool if_invalid )
{
    DASSERT(f);
    DASSERT(db);
    if ( if_invalid && IsValidDataBuf(db) )
	return false;

    fflush(stdout);
    fflush(stderr);
    indent = NormalizeIndent(indent);
    if (!db)
    {
	fprintf(f,"%*sdata_buf_t: NULL\n",indent,"");
	return true;
    }

    if (!db->buf)
    {
	fprintf(f,"%*sdata_buf_t buf: Invalid buf\n",indent,"");
	return true;
    }

    fprintf(f,"%*sdata_buf_t %p .. %d, data %d .. %d, used %u/%u, off=%llu\n",
	indent,"", db->buf, (int)(db->buf_end - db->buf),
	(int)(db->data - db->buf), (int)(db->data_end - db->buf),
	db->used, db->size, db->offset );

    return true;
}

///////////////////////////////////////////////////////////////////////////////

void FillPatternDataBuf ( DataBuf_t *db )
{
    // fill the buffer with a test pattern:
    //	'-' for unused bytes
    //	">0123456789<" in a longer or shorter form for used bytes

    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    memset(db->buf,'-',db->size);

    if ( db->used <= 1 )
    {
	if ( db->used == 1 )
	    *db->data = '+';
	return;
    }

    u8 *ptr = db->data;
    u8 *end = ptr + db->used - 1;
    if ( end >= db->buf_end )
	end -= db->size;
    *ptr++ = '>';
    *end = '<';
    uint i = 0, len = end - ptr;
    if ( len > db->size )
	len += db->size;
    for(;;)
    {
	if ( ptr == db->buf_end )
	    ptr = db->buf;
	if ( ptr == end )
	    break;
	*ptr++ = 'a' + i/len;
	i += 26;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint InsertDataBuf
(
    // Insert at top of buffer.
    // Return the number of inserted bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    const void	*data,		// data to insert
    uint	data_size,	// size of 'data'
    bool	all_or_none	// true: insert complete data or nothing
)
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    uint space = db->size - db->used;
    if ( data_size > space )
    {
	if (all_or_none)
	    return 0;
	data_size = space;
    }

    if (!db->used)
	db->data = db->data_end = db->buf + data_size;
    db->used += data_size;

    uint copy_len = db->data - db->buf;
    if ( copy_len > data_size )
	 copy_len = data_size;
    memcpy(db->data-copy_len,data,copy_len);
    db->data -= copy_len;
    if ( data_size > copy_len )
    {
	const uint copy_len2 = data_size - copy_len;
	memcpy(db->buf_end-copy_len2,(u8*)data+copy_len,copy_len2);
	db->data = db->buf_end - copy_len2;
    }

    DASSERT(!DumpDataBuf(stderr,0,db,true));
    return data_size;
}

///////////////////////////////////////////////////////////////////////////////

uint AppendDataBuf
(
    // Append data at end of buffer
    // Return the number of inserted bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    const void	*data,		// data to insert
    uint	data_size,	// size of 'data'
    bool	all_or_none	// true: insert complete data or nothing
)
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    uint space = db->size - db->used;
    noPRINT("APPEND %u/%u [%u/%u]\n",data_size,space,db->used,db->size);
    if ( data_size > space )
    {
	if (all_or_none)
	    return 0;
	data_size = space;
    }

    if (!db->used)
	db->data = db->data_end = db->buf;
    db->used += data_size;

    uint copy_len = db->buf_end - db->data_end;
    if ( copy_len > data_size )
	 copy_len = data_size;
    memcpy(db->data_end,data,copy_len);
    db->data_end += copy_len;
    if ( data_size > copy_len )
    {
	const uint copy_len2 = data_size - copy_len;
	memcpy(db->buf,(u8*)data+copy_len,copy_len2);
	db->data_end = db->buf + copy_len2;
    }

    DASSERT(!DumpDataBuf(stderr,0,db,true));
    return data_size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void * GetPtrDataBuf
(
    // Return a pointer to the data and make sure,
    // that the data is not wrapped. Return NULL if failed.

    DataBuf_t	*db,		// valid pointer to data buffer
    uint	data_size,	// number of needed bytes
    uint	align,		// >1: Data alignment is also needed.
				//  Only 2,4,8,16,... are allowed, but not verified.
				//  Begin of buffer is always well aligned.
    bool	drop		// true: On success, drop data from buffer
				//       In real, the data is available and only
				//       the pointers and counters are updated.
)
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    if (!data_size)
	return db->data;
    if ( data_size > db->used )
    {
	if ( data_size <= db->size && db->fill_func )
	    db->fill_func(db,data_size-db->used,0);
	if ( data_size > db->used )
	    return 0;
    }

    if ( db->data + data_size > db->buf_end )
	PackDataBuf(db);
    else if ( align > 1 )
    {
	const uint offset = db->data - db->buf;
	if ( offset & (align-1) )
	    PackDataBuf(db);
    }

    u8 *res = db->data;
    if (drop)
    {
	db->data += data_size;
	db->used -= data_size;
	DASSERT(!DumpDataBuf(stderr,0,db,true));
    }

    return res;
}

///////////////////////////////////////////////////////////////////////////////

uint CopyDataBuf
(
    // Copy data from buffer, but don't remove it
    // Return the number of copied bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    void	*dest,		// destination buffer wit at least 'max_size' space
    uint	min_read,	// return with 0 read bytes if less bytes available
				// if 'min_read' > 'size', NULL is returned
    uint	max_read	// try to read upto 'max_read' bytes
)
{
    DASSERT(db);
    DASSERT(dest);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    if ( min_read > db->used )
    {
	if ( min_read <= db->size && db->fill_func )
	    db->fill_func(db,min_read-db->used,0);
	if ( min_read > db->used )
	    return 0;
    }

    if ( max_read > db->used )
	 max_read = db->used;
    uint copy_len1 = db->buf_end - db->data;
    if ( copy_len1 >= max_read )
	memcpy(dest,db->data,max_read);
    else
    {
	memcpy(dest,db->data,copy_len1);
	memcpy((u8*)dest+copy_len1,db->buf,max_read-copy_len1);
    }

    return max_read;
}

///////////////////////////////////////////////////////////////////////////////

uint GetDataBuf
(
    // Copy data from buffer and remove it
    // Return the number of copied bytes

    DataBuf_t	*db,		// valid pointer to data buffer
    void	*dest,		// destination buffer wit at least 'max_size' space
    uint	min_read,	// return with 0 read bytes if less bytes available
    uint	max_read	// try to read upto 'max_read' bytes
)
{
    DASSERT(db);
    DASSERT(dest);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    if ( min_read > db->used )
    {
	if ( min_read <= db->size && db->fill_func )
	    db->fill_func(db,min_read-db->used,0);
	if ( min_read > db->used )
	    return 0;
    }

    if ( max_read > db->used )
	 max_read = db->used;
    uint copy_len1 = db->buf_end - db->data;
    if ( copy_len1 >= max_read )
	memcpy(dest,db->data,max_read);
    else
    {
	memcpy(dest,db->data,copy_len1);
	memcpy((u8*)dest+copy_len1,db->buf,max_read-copy_len1);
    }

    db->used -= max_read;
    if (db->used)
    {
	db->data += max_read;
	if ( db->data >= db->buf_end )
	    db->data -= db->size;
    }
    else
	db->data = db->data_end = db->buf;

    db->offset += max_read;
    DASSERT(!DumpDataBuf(stderr,0,db,true));
    return max_read;
}

///////////////////////////////////////////////////////////////////////////////

uint SkipDataBuf
(
    // Skip 'skip_size' from input stream. Read more data if needed.
    // Return the number of really skiped bytes
    // If <skip_size is returned, no more data is available.

    DataBuf_t	*db,		// valid pointer to data buffer
    uint	skip_size	// number of bytes to drop
)
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    uint skipped = 0;
    for(;;)
    {
	uint stat = DropDataBuf(db,skip_size);
	skipped += stat;
	skip_size -= stat;
	if (!skip_size)
	    break;

	if (db->fill_func)
	{
	    const uint max_fill = skip_size < db->size ? skip_size : db->size;
	    db->fill_func(db,max_fill,0);
	}

	if (!db->used)
	    break;
    }

    db->offset += skipped;
    DASSERT(!DumpDataBuf(stderr,0,db,true));
    return skipped;
}

///////////////////////////////////////////////////////////////////////////////

uint DropDataBuf
(
    // Remove upto 'drop_size' bytes from buffer.
    // Return the number of dropped bytes.

    DataBuf_t	*db,		// valid pointer to data buffer
    uint	drop_size	// number of bytes to drop
)
{
    DASSERT(db);
    DASSERT(!DumpDataBuf(stderr,0,db,true));

    if ( drop_size >= db->used )
    {
	drop_size = db->used;
	db->used = 0;
	db->data = db->data_end = db->buf;
    }
    else
    {
	db->data += drop_size;
	if ( db->data >= db->buf_end )
	    db->data -= db->size;
	db->used -= drop_size;
    }
    db->offset += drop_size;

    DASSERT(!DumpDataBuf(stderr,0,db,true));
    return drop_size;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		ResizeElement_t, ResizeHelper_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeResize
(
    ResizeHelper_t	*r,		// data structure
    uint		s_size,		// size of source
    uint		d_size		// size of dest
)
{
    DASSERT(r);
    memset(r,0,sizeof(*r));

    r->src_size	= s_size;
    r->dest_size= d_size;
    if ( !s_size || !d_size )
	return;

    const uint divisor = gcd(s_size,d_size);
    s_size /= divisor;
    d_size /= divisor;

    r->sum_factor  = s_size;
    r->half_factor = r->sum_factor/2;

    r->idx_inc   = s_size / d_size;
    r->frac_inc  = s_size % d_size;
    r->frac_next = d_size;
}

///////////////////////////////////////////////////////////////////////////////

static bool CalcResize ( ResizeHelper_t *r )
{
    DASSERT(r);

    ResizeElement_t *re = r->elem;
    re[0].n_elem = re[1].n_elem = re[2].n_elem = 0;
    DASSERT(!re[3].n_elem);

    if ( r->src_idx >= r->src_size || r->dest_idx >= r->dest_size )
	return false;

    uint sum = r->sum_factor;
    if (r->src_frac)
    {
	uint max = r->frac_next - r->src_frac;
	if ( max > sum )
	    max = sum;
	re->n_elem  = 1;
	re->factor  = max;
	re++;
	sum -= max;
    }

    if ( sum >= r->frac_next )
    {
	const uint n = sum / r->frac_next;
	re->n_elem  = n;
	re->factor  = r->frac_next;
	re++;
	sum -= n * r->frac_next;
    }

    if ( sum > 0 )
    {
	re->n_elem  = 1;
	re->factor  = sum;
	re++;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool FirstResize ( ResizeHelper_t *r )
{
    DASSERT(r);

    r->dest_idx = r->src_idx = r->src_frac = 0;
    memset(r->elem,0,sizeof(r->elem));
    return CalcResize(r);
}

///////////////////////////////////////////////////////////////////////////////

bool NextResize ( ResizeHelper_t *r )
{
    DASSERT(r);

    r->dest_idx++;
    r->src_idx  += r->idx_inc;
    r->src_frac += r->frac_inc;
    if ( r->src_frac >= r->frac_next )
    {
	r->src_frac -= r->frac_next;
	r->src_idx++;
    }

    return CalcResize(r);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    GrowBuffer_t		///////////////
///////////////////////////////////////////////////////////////////////////////

static const uint grow_buffer_size_factor = 0x100;

///////////////////////////////////////////////////////////////////////////////

void InitializeGrowBuffer ( GrowBuffer_t *gb, uint max_buf_size )
{
    DASSERT(gb);
    memset(gb,0,sizeof(*gb));
    gb->grow_size = grow_buffer_size_factor;
    gb->max_size  = ( max_buf_size + grow_buffer_size_factor - 1 )
		    / grow_buffer_size_factor * grow_buffer_size_factor;
}

///////////////////////////////////////////////////////////////////////////////

void ResetGrowBuffer ( GrowBuffer_t *gb )
{
    DASSERT(gb);
    FREE(gb->buf);
    const uint grow_size = gb->grow_size;
    InitializeGrowBuffer(gb,gb->max_size);
    gb->grow_size = grow_size;
}

///////////////////////////////////////////////////////////////////////////////

uint GetSpaceGrowBuffer
(
    // returns the maximal possible space
    const GrowBuffer_t	*gb	// valid grow-buffer object
)
{
    DASSERT(gb);
    const uint max = gb->max_size > gb->size ? gb->max_size : gb->size;
    return max - gb->used;
}

///////////////////////////////////////////////////////////////////////////////

uint PrepareGrowBuffer
(
    // returns the number of available space, smaller than or equal 'size'

    GrowBuffer_t	*gb,	// valid grow-buffer object
    uint		size,	// needed size
    bool		force	// always alloc enough space and return 'size'
)
{
 #if HAVE_DASSERT
    DASSERT(gb);
    if (gb->buf)
    {
	DASSERT( gb->used <= gb->size );
	DASSERT( gb->ptr >= gb->buf );
	DASSERT( gb->ptr + gb->used <= gb->buf + gb->size );
    }
    else
    {
	DASSERT(!gb->ptr);
	DASSERT(!gb->size);
	DASSERT(!gb->used);
    }
 #endif

    if (!gb->buf)
    {
	//--- initial setup of buffer

	if ( gb->grow_size < grow_buffer_size_factor )
	    gb->grow_size = grow_buffer_size_factor;
	else
	    gb->grow_size = gb->grow_size
			    / grow_buffer_size_factor * grow_buffer_size_factor;
	gb->size = ( size / gb->grow_size + 1 ) * gb->grow_size;

	if ( gb->max_size < grow_buffer_size_factor )
	    gb->max_size = grow_buffer_size_factor;
	else
	    gb->max_size = gb->max_size
			   / grow_buffer_size_factor * grow_buffer_size_factor;
	if ( gb->size > gb->max_size )
	    gb->size = gb->max_size;

	if ( gb->size < size && force )
	    gb->size =	( size + gb->grow_size )
			/ gb->grow_size * gb->grow_size - 1;

	gb->buf = gb->ptr = MALLOC(gb->size+1);  // 1 extra byte for NULL-TERM
	gb->used = 0;
    }
    else
    {
	if (!gb->grow_size)
	    gb->grow_size = grow_buffer_size_factor;
	if (!gb->used)
	    gb->ptr = gb->buf;
    }
    DASSERT(gb->grow_size);

    uint avail = gb->size - gb->used;
    if ( size > avail && ( force || gb->size < gb->max_size ) )
    {
	//--- grow buffer

	gb->size = ( size + gb->used + gb->grow_size )
		   / gb->grow_size * gb->grow_size - 1;
	DASSERT( gb->used <= gb->size );
	u8 *newbuf = MALLOC(gb->size+1); // 1 extra byte for NULL-TERM
	memcpy(newbuf,gb->ptr,gb->used);
	FREE(gb->buf);
	gb->buf = gb->ptr = newbuf;
    }

    uint space = ( gb->buf + gb->size ) - ( gb->ptr + gb->used );
    if ( space < size && gb->buf != gb->ptr )
    {
	memmove(gb->buf,gb->ptr,gb->used);
	gb->ptr = gb->buf;
	space = gb->size - gb->used;
    }

    DASSERT( gb->used <= gb->size );
    DASSERT( gb->ptr >= gb->buf );
    DASSERT( gb->ptr + gb->used <= gb->buf + gb->size );

    return size < space ? size : space;
}

///////////////////////////////////////////////////////////////////////////////

uint InsertGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    const void		*data,	// data to insert
    uint		size	// size of 'data'
)
{
    DASSERT(gb);
    size = PrepareGrowBuffer(gb,size,false);
    DASSERT( gb->ptr + gb->used + size <= gb->buf + gb->size );
    memcpy( gb->ptr + gb->used, data, size );
    gb->used += size;
    gb->ptr[gb->used] = 0;
    if ( gb->max_used < gb->used )
	 gb->max_used = gb->used;

    DASSERT( gb->used <= gb->size );
    DASSERT( gb->ptr >= gb->buf );
    DASSERT( gb->ptr + gb->used <= gb->buf + gb->size );

    return size;
}

///////////////////////////////////////////////////////////////////////////////

char *tiparm ( ccp str, ... );

uint InsertCapNGrowBuffer
(
    // insert 'cap_1' or 'cap_n' depending of 'n' and the existence of 'cap_*'
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    int			n,	// repeat number; if <1: insert nothing
    mem_t		cap_1,	// default for N=1
    mem_t		cap_n	// default for N>1
)
{
    DASSERT(gb);

    if ( n <= 0 )
	return 0;

    if ( n == 1 && cap_1.len )
	return InsertMemGrowBuffer(gb,cap_1);

 #if DCLIB_TERMINAL
    if (cap_n.len)
    {
	ccp str = tiparm(cap_n.ptr,n);
	if (str)
	    return InsertGrowBuffer(gb,str,strlen(str));
    }
 #endif

    if (!cap_1.len)
	return 0;

    const uint total = cap_1.len * n;
    PrepareGrowBuffer(gb,total,true);
    u8 *ptr = gb->ptr + gb->used;
    while ( n-- > 0 )
    {
	memcpy(ptr,cap_1.ptr,cap_1.len);
	ptr += cap_1.len;
    }
    gb->used += total;
    gb->ptr[gb->used] = 0;
    if ( gb->max_used < gb->used )
	 gb->max_used = gb->used;
    return total;
}

///////////////////////////////////////////////////////////////////////////////

uint WriteCapN
(
    // print 'cap_1' or 'cap_n' depending of 'n' and the existence of 'cap_*'
    // returns the number of printed bytes

    FILE		*f,	// valid FILE
    int			n,	// repeat number; if <1: insert nothing
    mem_t		cap_1,	// default for N=1
    mem_t		cap_n	// default for N>1
)
{
    DASSERT(f);

    if ( n <= 0 )
	return 0;

    if ( n == 1 && cap_1.len )
	return fwrite(cap_1.ptr,cap_1.len,1,f);

 #if DCLIB_TERMINAL
    if (cap_n.len)
    {
	ccp str = tiparm(cap_n.ptr,n);
	if (str)
	    return fwrite(str,strlen(str),1,f);
    }
 #endif

    if (!cap_1.len)
	return 0;

    const uint total = cap_1.len * n;
    while ( n-- > 0 )
	fwrite(cap_1.ptr,cap_1.len,1,f);
    return total;
}

///////////////////////////////////////////////////////////////////////////////

uint InsertCatchedGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    SavedStdFiles_t	*ssf	// saved output and catched data
)
{
    DASSERT(gb);
    DASSERT(ssf);

    TermCatchStdFiles(ssf);
    uint stat = 0;
    if (ssf->data)
    {
	PrepareGrowBuffer(gb,ssf->size,true);
	stat = InsertGrowBuffer(gb,ssf->data,ssf->size);
	XFREE(ssf->data);
	ssf->data = 0;
	ssf->size = 0;
    }
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint PrintArgGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    ccp			format,	// format string for vfprintf()
    va_list		arg	// parameters for 'format'
)
{
    DASSERT(gb);
    DASSERT(format);

    char buf[10000];

    va_list arg2;
    va_copy(arg2,arg);
    int stat = vsnprintf(buf,sizeof(buf),format,arg2);
    va_end(arg2);

    if ( stat < sizeof(buf) )
	return InsertGrowBuffer(gb,buf,stat);


    //--- buffer too small, use dynamic memory

    noPRINT("PrintArgGrowBuffer() -> MALLOC(%u)\n",stat+1);

    char *dest = MALLOC(stat+1);
    stat = vsnprintf(dest,stat+1,format,arg);

    const uint res = InsertGrowBuffer(gb,dest,stat);
    FREE(dest);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

uint PrintGrowBuffer
(
    // returns the number of inserted bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    ccp			format,	// format string for vfprintf()
    ...				// arguments for 'vsnprintf(format,...)'
)
{
    DASSERT(gb);
    DASSERT(format);

    va_list arg;
    va_start(arg,format);
    enumError err = PrintArgGrowBuffer(gb,format,arg);
    va_end(arg);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

uint ConvertToCrLfGrowBuffer
(
    // returns the number of inserted CR bytes

    GrowBuffer_t	*gb,	// valid grow-buffer object
    uint		begin	// index to begin with conversion
)
{
    DASSERT(gb);
    uint count = 0;

    if ( begin < gb->used )
    {
	//--- count LF without CR

	u8 *ptr = gb->ptr + begin;
	u8 *end = gb->ptr + gb->used;
	for ( ; ptr < end; ptr++ )
	{
	    if ( *ptr == '\r' )
		ptr++;
	    else if ( *ptr == '\n' )
		count++;
	}

	if ( count > 0 )
	{
	    PrepareGrowBuffer(gb,count,true);
	    u8 *src  = gb->ptr + gb->used;
	    u8 *dest = src + count;
	    u8 *beg  = gb->ptr + begin;
	    while ( src > beg )
	    {
		const char ch = *--src;
		*--dest = ch;
		if ( ch == '\n' && src[-1] != '\r' )
		    *--dest = '\r';
	    }
	    DASSERT( src == dest );
	    gb->used += count;
	    gb->ptr[gb->used] = 0;
	    if ( gb->max_used < gb->used )
		gb->max_used = gb->used;
	}
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

uint ClearGrowBuffer ( GrowBuffer_t *gb )
{
    DASSERT(gb);
    gb->ptr = gb->buf;
    if (gb->ptr)
	*gb->ptr = 0;
    gb->used = 0;
    return gb->size;
}

///////////////////////////////////////////////////////////////////////////////

uint PurgeGrowBuffer ( GrowBuffer_t *gb )
{
    if (!gb->used)
	return ClearGrowBuffer(gb);

    if ( gb->ptr > gb->buf )
    {
	memmove(gb->buf,gb->ptr,gb->used);
	gb->ptr = gb->buf;
    }
    return gb->size - gb->used;
}

///////////////////////////////////////////////////////////////////////////////

uint DropGrowBuffer
(
    // returns the number of dropped bytes

    GrowBuffer_t	*gb,		// valid grow-buffer object
    uint		size		// number of bytes to drop
)
{
 #if HAVE_DASSERT
    DASSERT(gb);
    if (gb->buf)
    {
	DASSERT_MSG( gb->used < gb->size, "%d/%d\n",gb->used,gb->size);
	DASSERT( gb->ptr >= gb->buf );
	DASSERT( gb->ptr + gb->used <= gb->buf + gb->size );
    }
    else
    {
	DASSERT(!gb->ptr);
	DASSERT(!gb->size);
	DASSERT(!gb->used);
    }
 #endif

    if (!gb->buf)
	return 0;

    if ( size >= gb->used )
    {
	size = gb->used;
	gb->used = 0;
	gb->ptr = gb->buf;
	*gb->ptr = 0;
	return size;
    }

    gb->ptr  += size;
    gb->used -= size;
    return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void LogGrowBuffer
(
    FILE		*f,		// output file
    int			indent,		// indention
    const GrowBuffer_t	*gb,		// valid grow-buffer
    ccp			format,		// format string for vfprintf()
    ...					// arguments for 'vfprintf(format,...)'
)
{
    DASSERT(f);
    DASSERT(gb);
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sTB: size=%s/%s/%s  idx=%s  grow=%s/%s  disabled=%d",
	indent,"",
	PrintSize1024(0,0,gb->used,DC_SFORM_TINY|DC_SFORM_ALIGN),
	PrintSize1024(0,0,gb->max_used,DC_SFORM_TINY|DC_SFORM_ALIGN),
	PrintSize1024(0,0,gb->size,DC_SFORM_TINY|DC_SFORM_ALIGN),
	PrintNumberU6(0,0,gb->ptr-gb->buf,true),
	PrintSize1024(0,0,gb->grow_size,DC_SFORM_TINY|DC_SFORM_ALIGN),
	PrintSize1024(0,0,gb->max_size,DC_SFORM_TINY|DC_SFORM_ALIGN),
	gb->disabled );

    if (format)
    {
	fputs(" : ",f);
	va_list arg;
	va_start(arg,format);
	vfprintf(f,format,arg);
	va_end(arg);
    }

    fputc('\n',f);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __APPLE__

FILE * OpenFileGrowBuffer
(
    // returns a FILE opened by fmemopen() => call CloseFileGrowBuffer()

    GrowBuffer_t	*gb,		// valid grow-buffer object
    uint		size		// needed size
)
{
    DASSERT(gb);
    uint space = PurgeGrowBuffer(gb);
    if ( space < size )
	space = PrepareGrowBuffer(gb,size,true);
    return fmemopen( gb->ptr + gb->used, space, "w" );
}

///////////////////////////////////////////////////////////////////////////////

int CloseFileGrowBuffer
(
    // returns the number of wirtten bytes, or -1 on error
    GrowBuffer_t	*gb,		// valid grow-buffer object
    FILE		*f		// FILE returned by OpenFileGrowBuffer()
)
{
    DASSERT(gb);
    if (!f)
	return -1;

    const int stat = ftell(f);
    if ( stat >= 0 && gb->ptr + gb->used + stat <= gb->buf + gb->size )
	gb->used += stat;

    fclose(f);
    return stat;
}

#endif // !__APPLE__

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define GROW_BUFFER_CHUNK_SIZE 3*1024 // multiple of 3 because of BASE64 (3->4)

void SaveCurrentStateGrowBuffer
(
    FILE		*f,		// output file
    ccp			name_prefix,	// NULL or prefix of name
    uint		tab_pos,	// tab pos of '='
    const GrowBuffer_t	*gb		// valid grow-buffer
)
{
    DASSERT(f);
    DASSERT(gb);

    if (!name_prefix)
	name_prefix = EmptyString;

    const int base = 8*tab_pos + 7 - strlen(name_prefix);

    fprintf(f,
	"%s" "disabled%.*s= %u\n"
	"%s" "used%.*s= %u\n"
	"%s" "size%.*s= %u\n"
	"%s" "grow-size%.*s= %u\n"
	"%s" "max-size%.*s= %u\n"
	,name_prefix, (base-8)/8, Tabs20, gb->disabled
	,name_prefix, (base-4)/8, Tabs20, gb->used
	,name_prefix, (base-4)/8, Tabs20, gb->size
	,name_prefix, (base-9)/8, Tabs20, gb->grow_size
	,name_prefix, (base-8)/8, Tabs20, gb->max_size
	);

    char buf[4*GROW_BUFFER_CHUNK_SIZE/3+10], name[20];
    uint start,i;

    for ( start = i = 0; start < gb->used; i++ )
    {
	uint size = gb->used - start;
	if ( size > GROW_BUFFER_CHUNK_SIZE )
	    size = GROW_BUFFER_CHUNK_SIZE;

	const uint len = snprintf(name,sizeof(name),"data-%u",i);
	EncodeBase64(buf,sizeof(buf),(ccp)gb->ptr+start,size,0,true,0,0);
	fprintf(f, "%s%s%.*s= %s\n",
		name_prefix, name, (base-len)/8, Tabs20, buf );

	start += size;
    }
}

///////////////////////////////////////////////////////////////////////////////

void RestoreStateGrowBuffer
(
    GrowBuffer_t	*gb,		// valid grow-buffer
    ccp			name_prefix,	// NULL or prefix of name
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
)
{
    DASSERT(gb);
    DASSERT(rs);

    ResetGrowBuffer(gb);

    if (!name_prefix)
	name_prefix = EmptyString;

    char name[200]; // name buf
    snprintf(name,sizeof(name),"%sused",name_prefix);
    int size = GetParamFieldUINT(rs,name,0);
    snprintf(name,sizeof(name),"%sdisabled",name_prefix);
    gb->disabled = GetParamFieldINT(rs,name,gb->disabled) > 0;

    if ( size > 0 )
    {
	char buf[2*GROW_BUFFER_CHUNK_SIZE];

	PrepareGrowBuffer(gb,size,true);
	uint index = 0;
	while ( size > 0 )
	{
	    snprintf(name,sizeof(name),"%sdata-%u",name_prefix,index++);
	    int res = GetParamFieldBUF(buf,sizeof(buf),rs,name,ENCODE_BASE64URL,0);
	    if ( res < 0 )
		break;

	    InsertGrowBuffer(gb,buf,res);
	    size -= res;
	}

     #ifdef TEST0
	if ( gb->used > 10000 )
	{
	    fprintf(stderr,"--- ResetGrowBuffer() size = %d\n",gb->used);
	    fwrite(gb->ptr,1,gb->used,stderr);
	    fprintf(stderr,"--- ResetGrowBuffer() size = %d\n",gb->used);
	}
     #endif
    }


    //--- known as 'unused'

    if ( rs->log_mode & RSL_UNUSED_NAMES )
    {
	static ccp name_tab[] =
	{
	   "size",
	   "grow-size",
	   "max-size",
	    0
	};

	ccp *np;
	for ( np = name_tab; *np; np++ )
	{
	    StringCat2S(name,sizeof(name),name_prefix,*np);
	    GetParamField(rs,name);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    misc			///////////////
///////////////////////////////////////////////////////////////////////////////

int opt_new = 0;

///////////////////////////////////////////////////////////////////////////////

uint CreateUniqueId ( int range )
{
    static uint unique_id = 1;
    const uint ret = unique_id;
    unique_id += range > 0 ? range : 0;
    return ret;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

