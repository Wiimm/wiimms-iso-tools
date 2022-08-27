
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
 *        Copyright (c) 2012-2022 by Dirk Clemens <wiimm@wiimm.de>         *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   See file gpl-2.0.txt or http://www.gnu.org/licenses/gpl-2.0.txt       *
 *                                                                         *
 ***************************************************************************/

#define _GNU_SOURCE 1

#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <stddef.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "dclib-basics.h"
#include "dclib-debug.h"
#include "dclib-file.h"
#include "dclib-utf8.h"
#include "dclib-network.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////		some basic string functions		///////////////
///////////////////////////////////////////////////////////////////////////////

const char EmptyQuote[]	  = "\"\"";	// Ignored by FreeString()
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

const char DoubleLine300_3[901] = // 300 * '═' (U+2550) + NULL
	"══════════════════════════════════════════════════"
	"══════════════════════════════════════════════════"
	"══════════════════════════════════════════════════"
	"══════════════════════════════════════════════════"
	"══════════════════════════════════════════════════"
	"══════════════════════════════════════════════════";

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

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetupMultiProcessing()
{
    ProgInfo.multi_processing = true;
}

///////////////////////////////////////////////////////////////////////////////

void SetupProgname ( int argc, char ** argv, ccp tname, ccp tvers, ccp ttitle )
{
    if (tname)
	ProgInfo.toolname = tname;
    if (tvers)
	ProgInfo.toolversion = tvers;
    if (ttitle)
	ProgInfo.tooltitle = ttitle;

    if (!ProgInfo.progpath)
    {
	char path[PATH_MAX];
	int res = GetProgramPath( path, sizeof(path), true, argc > 0 ? argv[0] : 0 );
	if ( res > 0 )
	    ProgInfo.progpath = STRDUP(path);
    }

    if ( argc > 0 && *argv && **argv )
    {
	ccp slash = strrchr(*argv,'/');
	ProgInfo.progname = slash ? slash + 1 : *argv;
    }
    else
	ProgInfo.progname = ProgInfo.toolname && *ProgInfo.toolname
				? ProgInfo.toolname : "?";
}

///////////////////////////////////////////////////////////////////////////////

ccp ProgramPath()
{
    static bool done = false;
    if ( !ProgInfo.progpath && !done )
    {
	done = true;

	char path[PATH_MAX];
	int res = GetProgramPath( path, sizeof(path), true, 0 );
	if ( res > 0 )
	    ProgInfo.progpath = STRDUP(path);
    }
    return ProgInfo.progpath;
}

///////////////////////////////////////////////////////////////////////////////

ccp ProgramDirectory()
{
    if (!ProgInfo.progdir)
    {
	ccp path = ProgramPath();
	if (path)
	{
	    ccp end = strrchr(path,'/');
	    const uint len = end ? end - path : strlen(path);
	    ProgInfo.progdir = MEMDUP(path,len);
	}
    }
    return ProgInfo.progdir;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void DefineLogDirectory ( ccp path0, bool force )
{
    PRINT("DefineLogDirectory(%s,%d)\n",path0,force);
    if ( path0 && *path0 && ( force || !ProgInfo.logdir ))
    {
	char path[PATH_MAX];
	StringCopyS(path,sizeof(path),path0);
	if (!ExistDirectory(path,0))
	{
	    char *slash = strrchr(path,'/');
	    if (slash)
		*slash = 0;
	    else
	    {
		path[0] = '.';
		path[1] = 0;
	    }

	    if (!ExistDirectory(path,0))
		return;
	}

	int len = strlen(path);
	while ( len > 0 && path[len-1] == '/' )
	     path[--len] = 0;

	if ( len > 0 )
	{
	    FreeString(ProgInfo.logdir);
	    ProgInfo.logdir = MEMDUP(path,len);
	    PRINT("#LOG-DIR: %s\n",ProgInfo.logdir);
	}
    }
}

//-----------------------------------------------------------------------------

ccp GetLogDirectory()
{
    return ProgInfo.logdir ? ProgInfo.logdir : ".";
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

#ifdef __APPLE__

    //--- read apple path of executable

    extern int _NSGetExecutablePath( char* buf, uint32_t * bufsize );

    uint32_t size = sizeof(path);
    if (!_NSGetExecutablePath(path,&size))
    {
	if ( realpath(path,path2) && *path2 )
	    return StringCopyS(buf,buf_size,path2) - buf;
	return StringCopyS(buf,buf_size,path) - buf;
    }

#else // !__APPLE__

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

#endif // !__APPLE__


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
    noTRACE("FreeString(%p) EmptyString=%p MinusString=%p, IsCircBuf=%d\n",
	    str, EmptyString, MinusString, IsCircBuf(str) );
    if ( str != EmptyString && str != EmptyQuote && str != MinusString && !IsCircBuf(str) )
	FREE((char*)str);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * StringCopyS ( char * buf, ssize_t buf_size, ccp src )
{
    DASSERT(buf);
    if ( !buf || buf_size <= 0 )
	return buf;

    if (src)
    {
	size_t slen = strlen(src);
	if ( slen >= buf_size )
	    slen = buf_size-1;
	memcpy(buf,src,slen);
	buf += slen;
    }

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCopySM ( char * buf, ssize_t buf_size, ccp src, ssize_t max_copy )
{
    DASSERT(buf);
    if ( !buf || buf_size <= 0 )
	return buf;

    if (src)
    {
	size_t slen = strlen(src);
	if ( slen >= buf_size )
	    slen = buf_size-1;
	if ( max_copy >= 0 && slen >= max_copy )
	    slen = max_copy;
	memcpy(buf,src,slen);
	buf += slen;
    }

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCopySL ( char * buf, ssize_t buf_size, ccp src, ssize_t src_len )
{
    DASSERT(buf);
    if ( !buf || buf_size <= 0 )
	return buf;

    if (src)
    {
	if ( src_len < 0 )
	    src_len = strlen(src);
	if ( src_len >= buf_size )
	    src_len = buf_size-1;
	memcpy(buf,src,src_len);
	buf += src_len;
    }

    *buf = 0;
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat2E ( char * buf, ccp buf_end, ccp src1, ccp src2 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    DASSERT(buf);
    DASSERT(buf<buf_end);
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

char * StringCat2S ( char * buf, ssize_t buf_size, ccp src1, ccp src2 )
{
    return StringCat2E(buf,buf+buf_size,src1,src2);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCatSep2E ( char * buf, ccp buf_end, ccp sep, ccp src1, ccp src2 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    DASSERT(buf);
    DASSERT(buf<buf_end);

    if ( !sep || !*sep )
	return StringCat2E(buf,buf_end,src1,src2);

    char *buf0 = buf;
    buf_end--;

    if (src1)
	while( buf < buf_end && *src1 )
	    *buf++ = *src1++;

    if (src2)
    {
	if ( buf > buf0 )
	    while ( buf < buf_end && *sep )
		*buf++ = *sep++;

	while( buf < buf_end && *src2 )
	    *buf++ = *src2++;
    }

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCatSep2S ( char * buf, ssize_t buf_size, ccp sep, ccp src1, ccp src2 )
{
    return StringCatSep2E(buf,buf+buf_size,sep,src1,src2);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat3E ( char * buf, ccp buf_end, ccp src1, ccp src2, ccp src3 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    DASSERT(buf);
    DASSERT(buf<buf_end);
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

char * StringCat3S ( char * buf, ssize_t buf_size, ccp src1, ccp src2, ccp src3 )
{
    return StringCat3E(buf,buf+buf_size,src1,src2,src3);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCatSep3E
	( char * buf, ccp buf_end, ccp sep, ccp src1, ccp src2, ccp src3 )
{
    // RESULT: end of copied string pointing to NULL
    // 'src*' may be a NULL pointer.

    DASSERT(buf);
    DASSERT(buf<buf_end);

    if ( !sep || !*sep )
	return StringCat3E(buf,buf_end,src1,src2,src3);

    buf_end--;
    char *buf0 = buf;

    if (src1)
	while( buf < buf_end && *src1 )
	    *buf++ = *src1++;

    if (src2)
    {
	if ( buf > buf0 )
	{
	    ccp ptr = sep;
	    while ( buf < buf_end && *ptr )
		*buf++ = *ptr++;
	}

	while( buf < buf_end && *src2 )
	    *buf++ = *src2++;
    }

    if (src3)
    {
	if ( buf > buf0 )
	    while ( buf < buf_end && *sep )
		*buf++ = *sep++;

	while( buf < buf_end && *src3 )
	    *buf++ = *src3++;
    }

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * StringCatSep3S
	( char * buf, ssize_t buf_size, ccp sep, ccp src1, ccp src2, ccp src3 )
{
    return StringCatSep3E(buf,buf+buf_size,sep,src1,src2,src3);
}

///////////////////////////////////////////////////////////////////////////////

char * StringCat2A ( ccp src1, ccp src2 )
{
    mem_t mem = MemCat2A( MemByString0(src1), MemByString0(src2) );
    return (char*)mem.ptr;
}

//-----------------------------------------------------------------------------

char * StringCat3A ( ccp src1, ccp src2, ccp src3 )
{
    mem_t mem = MemCat3A( MemByString0(src1), MemByString0(src2), MemByString0(src3) );
    return (char*)mem.ptr;
}

///////////////////////////////////////////////////////////////////////////////

char * StringCatSep2A ( ccp sep, ccp src1, ccp src2 )
{
    mem_t mem = MemCatSep2A( MemByString0(sep), 
			     MemByString0(src1),
			     MemByString0(src2) );
    return (char*)mem.ptr;
}

//-----------------------------------------------------------------------------

char * StringCatSep3A ( ccp sep, ccp src1, ccp src2, ccp src3 )
{
    mem_t mem = MemCatSep3A( MemByString0(sep),
			     MemByString0(src1),
			     MemByString0(src2),
			     MemByString0(src3) );
    return (char*)mem.ptr;
}

///////////////////////////////////////////////////////////////////////////////

ccp StringCenterE 
(
    char		* buf,		// result buffer; if NULL, circ-buf is used
    ccp			buf_end,	// end of 'buf', NULL if buf==NULL
    ccp			src,		// source string
    int			width		// wanted width
)
{
    if (!src)
	src = "";
    int n2 = strlen(src);
    if ( n2 >= width )
    {
	if (!buf)
	    return CopyCircBuf(src,n2+1);

	StringCopyEM(buf,buf_end,src,n2);
	return buf;
    }

    n2 = width - n2;
    int n1 = n2/2;
    n2 -= n1;

    if (!buf)
    {
	buf = GetCircBuf(width+1);
	buf_end = buf + width;
    }    
    else
	buf_end--;

    char *dest = buf;
    while ( n1-- > 0 && dest < buf_end )
	*dest++ = ' ';

    while ( *src && dest < buf_end )
	*dest++ = *src++;

    while ( n2-- > 0 && dest < buf_end )
	*dest++ = ' ';

    *dest = 0;
    return buf;
};

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

char * StringLowerS ( char * buf, ssize_t buf_size, ccp src )
{
    return StringLowerE(buf,buf+buf_size,src);
}

//-----------------------------------------------------------------------------

char * MemLowerE ( char * buf, ccp buf_end, mem_t src )
{
    // RESULT: end of copied string pointing to NULL
    // 'src.ptr' may be a NULL pointer.

    if ( buf >= buf_end )
	return (char*)buf_end - 1;

    DASSERT(buf);
    DASSERT(buf<buf_end);
    buf_end--;

    if ( src.ptr )
    {
	ccp src_end = src.ptr + src.len;
	while( buf < buf_end && src.ptr < src_end )
	    *buf++ = tolower((int)*src.ptr++);
    }

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * MemLowerS ( char * buf, ssize_t buf_size, mem_t src )
{
    return MemLowerE(buf,buf+buf_size,src);
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

char * StringUpperS ( char * buf, ssize_t buf_size, ccp src )
{
    return StringUpperE(buf,buf+buf_size,src);
}

//-----------------------------------------------------------------------------

char * MemUpperE ( char * buf, ccp buf_end, mem_t src )
{
    // RESULT: end of copied string pointing to NULL
    // 'src.ptr' may be a NULL pointer.

    if ( buf >= buf_end )
	return (char*)buf_end - 1;

    DASSERT(buf);
    DASSERT(buf<buf_end);
    buf_end--;

    if ( src.ptr )
    {
	ccp src_end = src.ptr + src.len;
	while( buf < buf_end && src.ptr < src_end )
	    *buf++ = toupper((int)*src.ptr++);
    }

    *buf = 0;
    return buf;
}

//-----------------------------------------------------------------------------

char * MemUpperS ( char * buf, ssize_t buf_size, mem_t src )
{
    return MemUpperE(buf,buf+buf_size,src);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Signals & Sleep			///////////////
///////////////////////////////////////////////////////////////////////////////

volatile int SIGINT_level	= 0;
volatile int SIGHUP_level	= 0;
volatile int SIGALRM_level	= 0;
volatile int SIGCHLD_level	= 0;
volatile int SIGPIPE_level	= 0;
volatile int SIGUSR1_level	= 0;
volatile int SIGUSR2_level	= 0;

// no timout for SIGPIPE
u_sec_t SIGINT_sec		= 0;
u_sec_t SIGHUP_sec		= 0;
u_sec_t SIGALRM_sec		= 0;
u_sec_t SIGCHLD_sec		= 0;
u_sec_t SIGUSR1_sec		= 0;
u_sec_t SIGUSR2_sec		= 0;

int  SIGINT_level_max		= 3;
bool ignore_SIGINT		= false;
bool ignore_SIGHUP		= false;

int  redirect_signal_pid	= 0;
LogFile_t log_signal_file	= {0};

///////////////////////////////////////////////////////////////////////////////

static void inc_sig_handler ( volatile int *level, volatile uint *sec )
{
    DASSERT(level);
    DASSERT(sec);

    const uint now = GetTimeSec(false);

    if ( now - *sec > 30 )
	*level = 0;
    *sec = now;
    (*level)++;
}

///////////////////////////////////////////////////////////////////////////////

static void sig_handler ( int signum )
{
    fflush(stdout);
    static bool had_sigpipe = false;

    if (redirect_signal_pid)
    {
	kill(redirect_signal_pid,signum);
	return;
    }

    switch(signum)
    {
      case SIGINT:
	if (ignore_SIGINT)
	    break;
	// fall through

      case SIGTERM:
	inc_sig_handler(&SIGINT_level,&SIGINT_sec);
	ccp signame = signum == SIGINT ? "INT" : "TERM";
	if ( SIGINT_level > 1 )
	    PrintLogFile(&log_signal_file,"#SIGNAL %s: level set to %d/%d\n",
			signame, SIGINT_level, SIGINT_level_max );
	if ( SIGINT_level >= SIGINT_level_max )
	{
	    PrintLogFile(&log_signal_file,"#SIGNAL %s: TERMINATE IMMEDIATELY!\n",signame);
	    exit(0);
	}
	break;

      case SIGHUP:
	if (!ignore_SIGHUP)
	{
	    inc_sig_handler(&SIGHUP_level,&SIGHUP_sec);
	    PrintLogFile(&log_signal_file,"#SIGNAL HUP: level set to %d\n",SIGHUP_level);
	}
	break;

      case SIGCHLD:
	SIGCHLD_level++;
	PrintLogFile(&log_signal_file,"#SIGNAL CHLD: level set to %d\n",SIGCHLD_level);
	break;

      case SIGALRM:
	inc_sig_handler(&SIGALRM_level,&SIGALRM_sec);
	PrintLogFile(&log_signal_file,"#SIGNAL ALRM: level set to %d\n",SIGALRM_level);
	break;

      case SIGUSR1:
	inc_sig_handler(&SIGUSR1_level,&SIGUSR1_sec);
	PrintLogFile(&log_signal_file,"#SIGNAL USR1: level set to %d\n",SIGUSR1_level);
	break;

      case SIGUSR2:
	inc_sig_handler(&SIGUSR2_level,&SIGUSR2_sec);
	PrintLogFile(&log_signal_file,"#SIGNAL USR2: level set to %d\n",SIGUSR2_level);
	break;

      case SIGPIPE:
	SIGPIPE_level++;
	if (!had_sigpipe)
	{
	    PrintLogFile(&log_signal_file,"#SIGNAL SIGPIPE: %d\n",SIGPIPE_level);
	    had_sigpipe = true;
	}
	if ( SIGPIPE_level >= 5 )
	{
	    PrintLogFile(&log_signal_file,"#SIGNAL SIGPIPE: TERMINATE IMMEDIATELY!\n");
	    exit(0);
	}
	break;

      default:
	PrintLogFile(&log_signal_file,"#SIGNAL: %d\n",signum);
    }
    fflush(stderr);
}

///////////////////////////////////////////////////////////////////////////////

void SetupSignalHandler ( int max_sigint_level, FILE *log_file )
{
    SIGINT_level_max = max_sigint_level;
    if (log_file)
    {
	log_signal_file.log = log_file;
	log_signal_file.flush = true;
    }

    static const int sigtab[] = { SIGTERM, SIGINT, SIGHUP, SIGALRM, SIGPIPE, -1 };
    int i;
    for ( i = 0; sigtab[i] >= 0; i++ )
    {
	struct sigaction sa;
	memset(&sa,0,sizeof(sa));
	sa.sa_handler = &sig_handler;
	sigaction(sigtab[i],&sa,0);
    }
}

///////////////////////////////////////////////////////////////////////////////

int SleepNSec ( s_nsec_t nsec )
{
    if ( nsec > 0 )
    {
	struct timespec ts;
	ts.tv_sec  = nsec / NSEC_PER_SEC;
	ts.tv_nsec = nsec % NSEC_PER_SEC;
	return nanosleep(&ts,0);
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				PathCat*()		///////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufPP ( char * buf, ssize_t bufsize, ccp path1, ccp path2 )
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

ccp PathCatPP ( char * buf, ssize_t bufsize, ccp path1, ccp path2 )
{
    // concatenate path + path; returns path1 | path2 | buf

    if ( !path1 || !*path1 )
	return path2 ? path2 : EmptyString;

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

ccp PathCatDirP ( char * buf, ssize_t bufsize, ccp path1, ccp path2 )
{
    // concatenate path + path; returns path1 | path2 | buf

    if ( !path1 || !*path1 )
	return path2 ? path2 : EmptyString;

    if ( !path2 || !*path2 || !IsDirectory(path1,0) )
	return path1;

    return PathCatPP(buf,bufsize,path1,path2);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathAllocPP ( ccp path1, ccp path2 )
{
    char buf[PATH_MAX];
    return STRDUP(PathCatPP(buf,sizeof(buf),path1,path2));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * PathCatBufPPE ( char * buf, ssize_t bufsize, ccp path1, ccp path2, ccp ext )
{
    // concatenate path + path; returns (by definition) path1 | path2 | buf

    DASSERT(buf);
    DASSERT(bufsize);

    char *ptr = path1 ? StringCopyS(buf,bufsize-1,path1) : buf;
    *ptr = 0;

    if (path2)
    {
	if ( ptr > buf && ptr[-1] != '/' )
	    *ptr++ = '/';
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

char * PathCatBufBP ( char *buf, ssize_t bufsize, ccp base, ccp path )
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

ccp PathCatBP ( char *buf, ssize_t bufsize, ccp base, ccp path )
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

char * PathCatBufBPP ( char * buf, ssize_t bufsize, ccp base, ccp path1, ccp path2 )
{
    ccp path = PathCatPP(buf,bufsize,path1,path2);
    return PathCatBufBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatBPP ( char * buf, ssize_t bufsize, ccp base, ccp path1, ccp path2 )
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

char * PathCatBufBPPE ( char * buf, ssize_t bufsize, ccp base, ccp path1, ccp path2, ccp ext )
{
    ccp path = PathCatPPE(buf,bufsize,path1,path2,ext);
    return PathCatBufBP(buf,bufsize,base,path);
}

///////////////////////////////////////////////////////////////////////////////

ccp PathCatBPPE ( char * buf, ssize_t bufsize, ccp base, ccp path1, ccp path2, ccp ext )
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

char * NewFileExtS ( char * buf, ssize_t bufsize, ccp path, ccp ext )
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

char * SkipEscapes ( ccp str )
{
    if (str)
    {
	while ( *str == '\e' )
	{
	    str++;
	    if ( *str == '[' )
		str++;
	    while ( *str > 0 && *str < 0x40 )
		str++;
	    if (*str)
		str++;
	}
    }
    return (char*)str;
}

//-----------------------------------------------------------------------------

char * SkipEscapesE ( ccp str, ccp end )
{
    if (str)
    {
	if (!end)
	    return SkipEscapes(str);

	while ( str < end && *str == '\e' )
	{
	    str++;
	    if ( str < end && *str == '[' )
		str++;
	    while ( str < end && *str > 0 && *str < 0x40 )
		str++;
	    if ( str < end )
		str++;
	}
    }
    return (char*)str;
}

//-----------------------------------------------------------------------------

char * SkipEscapesS ( ccp str, int size )
{
    return size < 0 ? SkipEscapes(str) : SkipEscapesE(str,str+strlen(str));
}

//-----------------------------------------------------------------------------

mem_t SkipEscapesM ( mem_t mem )
{
    mem_t res;
    res.ptr = SkipEscapesE(mem.ptr,mem.ptr+mem.len);
    res.len = mem.ptr + mem.len - res.ptr;
    return res;
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
	buf = GetCircBuf( id_len + 1 );

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
///////////////			    circ buf			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[CircBuf]]

static __thread char circ_buf[CIRC_BUF_SIZE];
static __thread char *circ_ptr = 0;

///////////////////////////////////////////////////////////////////////////////

bool IsCircBuf ( cvp ptr )
{
    return ptr && (ccp)ptr >= circ_buf && (ccp)ptr < circ_buf + sizeof(circ_buf);
}

///////////////////////////////////////////////////////////////////////////////

char * GetCircBuf
(
    // Never returns NULL, but always ALIGN(4)

    u32		buf_size	// wanted buffer size, caller must add 1 for NULL-term
				// if buf_size > CIRC_BUF_MAX_ALLOC:
				//  ==> ERROR0(ERR_OUT_OF_MEMORY)
)
{
    if (!circ_ptr)
	circ_ptr = circ_buf;
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

///////////////////////////////////////////////////////////////////////////////

char * CopyCircBuf
(
    // Never returns NULL, but always ALIGN(4)

    cvp		data,		// source to copy
    u32		data_size	// see GetCircBuf()
)
{
    char *dest = GetCircBuf(data_size);
    memcpy(dest,data,data_size);
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

char * CopyCircBuf0
(
    // Like CopyCircBuf(), but an additional char is alloced and set to NULL
    // Never returns NULL, but always ALIGN(4).

    cvp		data,		// source to copy
    u32		data_size	// see GetCircBuf()
)
{
    char *dest = GetCircBuf(data_size+1);
    memcpy(dest,data,data_size);
    dest[data_size] = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

char * PrintCircBuf
(
    // returns CopyCircBuf0() or EmptyString.

    ccp		format,		// format string for vsnprintf()
    ...				// arguments for 'format'
)
{
    char buf[CIRC_BUF_MAX_ALLOC-1];
    va_list arg;
    va_start(arg,format);
    const int len = vsnprintf(buf,sizeof(buf),format,arg);
    va_end(arg);

    return len > 0 ? CopyCircBuf0(buf,len) : (char*)EmptyString;
}

///////////////////////////////////////////////////////////////////////////////

char * AllocCircBuf
(
    // Never returns NULL. Call FreeString(RESULT) to free possible alloced memory.

    cvp		src,		// NULL or source
    int		src_len,	// len of 'src'; if -1: use strlen(source)
    bool	try_circ	// use circ-buffer, if result is small enough
)
{
    if (!src)
	return (char*)EmptyString;

    if ( src_len < 0 )
	src_len = strlen(src);

    return try_circ && src_len <= CIRC_BUF_MAX_ALLOC
	? CopyCircBuf(src,src_len)
	: MEMDUP(src,src_len);
}

///////////////////////////////////////////////////////////////////////////////

char * AllocCircBuf0
(
    // Never returns NULL. Call FreeString(RESULT) to free possible alloced memory.
    // Like AllocCircBuf(), but an additional char is alloced and set to NULL

    cvp		src,		// NULL or source
    int		src_len,	// len of 'src'; if -1: use strlen(source)
    bool	try_circ	// use circ-buffer, if result is small enough
)
{
    if (!src)
	return (char*)EmptyString;

    if ( src_len < 0 )
	src_len = strlen(src);

    return try_circ && src_len < CIRC_BUF_MAX_ALLOC
	? CopyCircBuf0(src,src_len)
	: MEMDUP(src,src_len); // MEMDUP() always add 0-term
}

///////////////////////////////////////////////////////////////////////////////

void ReleaseCircBuf
(
    ccp	    end_buf,		// pointer to end of previous alloced buffer
    uint    release_size	// number of bytes to give back from end
)
{
    if (!circ_ptr)
	circ_ptr = circ_buf;
    DASSERT( circ_ptr >= circ_buf && circ_ptr <= circ_buf + sizeof(circ_buf) );

    if ( end_buf == circ_ptr
	&& release_size <= CIRC_BUF_MAX_ALLOC
	&& circ_ptr >= circ_buf + release_size )
    {
	circ_ptr -= release_size;
	DASSERT( circ_ptr >= circ_buf && circ_ptr <= circ_buf + sizeof(circ_buf) );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PrintMode_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupPrintMode ( PrintMode_t * pm )
{
    DASSERT(pm);

    //-- stdout

    if (!pm->fout)
    {
	pm->fout = stdout;
	pm->cout = colout;
    }
    if (!pm->cout)
	pm->cout = GetColorSet0();


    //-- stderr

    //if (!pm->ferr)	pm->ferr	= stderr;
    if (!pm->cerr)
	pm->cerr = GetColorSet0();


    //-- stdlog

    if (pm->flog)
    {
	if (!pm->clog)
	    pm->clog = GetColorSet0();
    }
    else if (pm->ferr)
    {
	pm->flog = pm->ferr;
	pm->clog = pm->cerr;
    }
    else
    {
	pm->flog = pm->fout;
	pm->clog = pm->cout;
    }


    //-- misc

    if (!pm->prefix)	pm->prefix	= EmptyString;
    if (!pm->eol)	pm->eol		=	"\n";

    pm->indent = NormalizeIndent(pm->indent);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			get line by list		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetLineByListHelper ( const int *list, ccp beg, ccp mid, ccp end, ccp line )
{
    DASSERT(list);
    DASSERT(beg);
    DASSERT(mid);
    DASSERT(end);
    DASSERT(line);

    char buf[CIRC_BUF_MAX_ALLOC+20];
    char *endbuf = buf + CIRC_BUF_MAX_ALLOC;
    char *dest = buf;

    for (;;)
    {
	int wd = *list++;
	if ( wd < 0 )
	    break;

	dest = StringCopyE(dest,endbuf,beg);
	beg = mid;
	while ( wd-- > 0 )
	    dest = StringCopyE(dest,endbuf,line);
    }

    dest = StringCopyE(dest,endbuf,end);
    *dest++ = 0;
    return dest <= endbuf ? CopyCircBuf(buf,dest-buf) : EmptyString;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct FastBuf_t		///////////////
///////////////////////////////////////////////////////////////////////////////

FastBuf_t * InitializeFastBuf ( cvp mem, uint size )
{
    DASSERT(mem);
    DASSERT( size >= sizeof(FastBuf_t) );

    FastBuf_t *fb = (FastBuf_t*)mem;

    int buf_size = (int)size - offsetof(FastBuf_t,fast_buf);
    if ( buf_size < sizeof(fb->fast_buf) )
    {
	PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,
		"Out of memory, FastBuf_t is at least %zd bytes to short.",
			sizeof(fb->fast_buf) - size );
	ASSERT(0);
	return 0;
    }

    fb->fast_buf_size = buf_size;
    fb->buf = fb->ptr = fb->fast_buf;
    fb->end = fb->buf + buf_size - 1;
    DASSERT( fb->buf < fb->end );
    return fb;
}

//-----------------------------------------------------------------------------

FastBuf_t * InitializeFastBufAlloc ( FastBuf_t *fb, uint size )
{
    DASSERT(fb);
    InitializeFastBuf(fb,sizeof(*fb));

    const uint new_size = GetGoodAllocSize(size+10);
    fb->buf = MALLOC(new_size);
    fb->ptr = fb->buf;
    fb->end = fb->buf + new_size - 1;
    return fb;
}

///////////////////////////////////////////////////////////////////////////////

#define RESET_FASTBUF(fb) fb->buf = fb->ptr = fb->fast_buf; \
			  fb->end = fb->buf + fb->fast_buf_size - 1;

void ResetFastBuf ( FastBuf_t *fb )
{
    DASSERT(fb);
    if ( fb->buf != fb->fast_buf )
	FREE(fb->buf);

    RESET_FASTBUF(fb)
}

///////////////////////////////////////////////////////////////////////////////

ccp GetFastBufStatus ( const FastBuf_t *fb )
{
    char info[100];
    int len = snprintf(info,sizeof(info),"%s=%u/%u",
		fb->buf == fb->fast_buf ? "FastBuf" : "Alloc",
		(uint)(fb->ptr - fb->buf), (uint)(fb->end - fb->buf) );
    return CopyCircBuf(info,len+1);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * MoveFromFastBufString ( FastBuf_t *fb )
{
    DASSERT(fb);
    *fb->ptr = 0;

    char *res;
    if ( fb->buf == fb->fast_buf )
    {
	res = MEMDUP(fb->buf,fb->ptr-fb->buf);
	fb->ptr = fb->buf;
    }
    else
    {
	res = fb->buf;
	RESET_FASTBUF(fb)
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t MoveFromFastBufMem ( FastBuf_t *fb )
{
    DASSERT(fb);
    *fb->ptr = 0;

    mem_t res;
    res.len = fb->ptr - fb->buf;

    if ( fb->buf == fb->fast_buf )
    {
	res.ptr = MEMDUP(fb->buf,res.len);
	fb->ptr = fb->buf;
    }
    else
    {
	res.ptr = fb->buf;
	RESET_FASTBUF(fb)
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * GetSpaceFastBuf ( FastBuf_t *fb, uint size )
{
    DASSERT(fb);
    if ( size > fb->end - fb->ptr )
    {
	const uint used = fb->ptr - fb->buf;
	const uint new_size = GetGoodAllocSize( used + used/4 + size + 1000 );
	if ( fb->buf == fb->fast_buf )
	{
	    fb->buf = MALLOC(new_size);
	    memcpy(fb->buf,fb->fast_buf,used);
	}
	else
	    fb->buf = REALLOC(fb->buf,new_size);
	fb->ptr = fb->buf + used;
	fb->end = fb->buf + new_size - 1;
    }

    char *res = fb->ptr;
    fb->ptr += size;
    return res;
}

//-----------------------------------------------------------------------------

int ReserveSpaceFastBuf ( FastBuf_t *fb, uint size )
{
    DASSERT(fb);
    if ( size > 0 )
	fb->ptr = GetSpaceFastBuf(fb,size);
    return fb->end - fb->ptr;
}

///////////////////////////////////////////////////////////////////////////////

uint InsertFastBuf ( FastBuf_t *fb, int index, cvp source, int size )
{
    DASSERT(fb);
    DASSERT(source||!size);

    if ( size < 0 )
	size = strlen(source);

    const uint len = fb->ptr - fb->buf;
    index = CheckIndex1(len,index);
    noPRINT("index=%d/%d/%zd, size=%d\n",index,len,fb->end-fb->buf,size);

    char *ptr = GetSpaceFastBuf(fb,size);
    char *dest = fb->buf + index;
    DASSERT( dest >= fb->buf );
    DASSERT( dest + size + len <= fb->end );

    memmove(dest+size,dest,ptr-dest);
    memcpy(dest,source,size);
    return index;
}

///////////////////////////////////////////////////////////////////////////////

char * AppendFastBuf ( FastBuf_t *fb, cvp source, int size )
{
    DASSERT(fb);
    DASSERT(source||!size);

    if ( size < 0 )
	size = strlen(source);

    char *dest = GetSpaceFastBuf(fb,size);
    memcpy(dest,source,size);
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

void AppendUTF8CharFastBuf ( FastBuf_t *fb, u32 code )
{
    char buf[8];
    const uint len = PrintUTF8Char(buf,code) - buf;

    char *dest = GetSpaceFastBuf(fb,len);
    memcpy(dest,buf,len);
}

///////////////////////////////////////////////////////////////////////////////

void AppendBE16FastBuf ( FastBuf_t *fb, u16 num )
{
    DASSERT(fb);

    char *dest = GetSpaceFastBuf(fb,sizeof(num));
    write_be16(dest,num);
}

//-----------------------------------------------------------------------------

void AppendBE32FastBuf ( FastBuf_t *fb, u32 num )
{
    DASSERT(fb);

    char *dest = GetSpaceFastBuf(fb,sizeof(num));
    write_be32(dest,num);
}

///////////////////////////////////////////////////////////////////////////////

void AppendInt64FastBuf ( FastBuf_t *fb, u64 val, IntMode_t mode )
{
    DASSERT(fb);

    switch(mode)
    {
	case IMD_UNSET:
	    AppendFastBuf(fb,&val,sizeof(val));
	    break;

	case IMD_BE1:
	case IMD_LE1:	AppendCharFastBuf(fb,val); break;

	case IMD_BE2:	write_be16(GetSpaceFastBuf(fb,2),val); break;
	case IMD_BE3:	write_be24(GetSpaceFastBuf(fb,3),val); break;
	case IMD_BE4:	write_be32(GetSpaceFastBuf(fb,4),val); break;
	case IMD_BE5:	write_be40(GetSpaceFastBuf(fb,5),val); break;
	case IMD_BE6:	write_be48(GetSpaceFastBuf(fb,6),val); break;
	case IMD_BE7:	write_be56(GetSpaceFastBuf(fb,7),val); break;
	case IMD_BE0:
	case IMD_BE8:	write_be64(GetSpaceFastBuf(fb,8),val); break;

	case IMD_LE2:	write_le16(GetSpaceFastBuf(fb,2),val); break;
	case IMD_LE3:	write_le24(GetSpaceFastBuf(fb,3),val); break;
	case IMD_LE4:	write_le32(GetSpaceFastBuf(fb,4),val); break;
	case IMD_LE5:	write_le40(GetSpaceFastBuf(fb,5),val); break;
	case IMD_LE6:	write_le48(GetSpaceFastBuf(fb,6),val); break;
	case IMD_LE7:	write_le56(GetSpaceFastBuf(fb,7),val); break;
	case IMD_LE0:
	case IMD_LE8:	write_le64(GetSpaceFastBuf(fb,8),val); break;
    }
}

///////////////////////////////////////////////////////////////////////////////

char * WriteFastBuf ( FastBuf_t *fb, uint offset, cvp source, int size )
{
    DASSERT(fb);

    uint oldlen = fb->ptr - fb->buf;
    uint minlen = offset + size;
    if ( minlen > oldlen )
    {
	char *ptr = GetSpaceFastBuf(fb,minlen-oldlen);
	memset(ptr,0,minlen-oldlen);
    }
    DASSERT( fb->ptr - fb->buf >= minlen );
    char *dest = fb->buf + offset;
    if (source)
	memcpy( dest, source, size );
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

uint AssignFastBuf ( FastBuf_t *fb, cvp source, int size )
{
    DASSERT(fb);

    if ( size < 0 )
	size = strlen(source);

    if ( source == fb->buf )
    {
	const uint used = fb->ptr - fb->buf;
	if ( size < used )
	    fb->ptr = fb->buf + size;
	else
	    size = used;
    }
    else
    {
	fb->ptr = fb->buf;
	if (source)
	    AppendFastBuf(fb,source,size);
	*fb->ptr = 0;
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////

uint AlignFastBuf ( FastBuf_t *fb, uint align, u8 fill )
{
    DASSERT(fb);
    const uint len = fb->ptr - fb->buf;
    const uint append = ALIGN32(len,align) - len;
    if ( append > 0 )
    {
	char *dest = GetSpaceFastBuf(fb,append);
	memset(dest,fill,append);
    }

    return fb->ptr - fb->buf;
}

///////////////////////////////////////////////////////////////////////////////

uint DropFastBuf ( FastBuf_t *fb, int index, int size )
{
    DASSERT(fb);
    if (size)
    {
	const uint len = fb->ptr - fb->buf;
	index = CheckIndex1(len,index);
	if ( size < 0 )
	{
	    size = -size;
	    if ( size > index )
		size = index;
	    index -= size;
	}
	int index2 = CheckIndex1(len,index+size);
	size = index2 - index;
	if (size)
	{
	    memmove(fb->buf+index,fb->buf+index2,len-index2+1);
	    fb->ptr -= size;
	}
    }

    return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CopyFastBuf ( FastBuf_t *dest, const FastBuf_t *src )
{
    if ( dest && dest != src )
    {
	if (!src)
	    ClearFastBuf(dest);
	else
	    AssignFastBuf( dest, src->buf, src->ptr - src->buf );
    }
}

///////////////////////////////////////////////////////////////////////////////

void MoveFastBuf ( FastBuf_t *dest, FastBuf_t *src )
{
    if ( dest && src && dest != src )
    {
	if ( src->buf != src->fast_buf )
	{
	    // src is allocated
	    if ( dest->buf != dest->fast_buf )
		FREE(dest->buf);
	    dest->buf = src->buf;
	    dest->ptr = src->ptr;
	    dest->end = src->end;
	    InitializeFastBuf( src, offsetof(FastBuf_t,fast_buf) + src->fast_buf_size );
	}
	else
	    AssignFastBuf( dest, src->buf, src->ptr - src->buf );
    }
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
		const int diff = abs ( (int)(uintptr_t)(alloc[i-1])
				     - (int)(uintptr_t)(alloc[i]) );
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
		const int diff = abs ( (int)(uintptr_t)(alloc[i-1])
				     - (int)(uintptr_t)(alloc[i]) ) - i;
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
    *argv_dest++ = ProgInfo.progname ? (char*)ProgInfo.progname : "";
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
///////////////			PointerList_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetPointerMgr ( PointerList_t *pl )
{
    if (pl)
    {
	FREE(pl->list);
	InitializePointerMgr(pl,pl->grow);
    }
}

///////////////////////////////////////////////////////////////////////////////

void AddToPointerMgr ( PointerList_t *pl, const void *info )
{
    DASSERT(pl);
    if (!info)
	return;

    // no duplicates!
    for ( int i = 0; i < pl->used; i++ )
	if ( pl->list[i] == info )
	    return;

    if ( pl->used == pl->size )
    {
	if ( pl->grow < 10 )
	    pl->grow = 10;
	pl->size += pl->size/2 + pl->grow;
	pl->list = REALLOC ( pl->list, (pl->size+1) * sizeof(*pl->list) );
    }
    DASSERT( pl->used < pl->size );
    void **dest = pl->list + pl->used++;
    *dest++ = (void*)info;
    *dest = 0;
}

///////////////////////////////////////////////////////////////////////////////

void AddListToPointerMgr ( PointerList_t *pl, const void **list, int n_elem )
{
    if (list)
    {
	if ( n_elem < 0 )
	    while (*list)
		AddToPointerMgr(pl,*list++);
	else
	    while( --n_elem >= 0 )
		AddToPointerMgr(pl,*list++);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ** DupPointerMgr ( PointerList_t *pl, bool reset )
{
    DASSERT(pl);
    void **dest = MEMDUP( pl->list, (pl->used+1) * sizeof(*dest) );
    if (reset)
	ResetPointerMgr(pl);
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

void ** ReplacePointerMgr
	( PointerList_t *pl, bool reset, const PointerList_t **old_list )
{
    FREE(old_list);
    return DupPointerMgr(pl,reset);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ArgManager_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetArgManager ( ArgManager_t *am )
{
    static char *null_list[] = {0};

    if (am)
    {
	if (am->size)
	{
	    uint i;
	    for ( i = 0; i < am->argc; i++ )
		FreeString(am->argv[i]);
	    if ( am->argv != null_list )
		FREE(am->argv);
	}

	const typeof(am->force_case) force_case = am->force_case;
	memset(am,0,sizeof(*am));
	am->force_case = force_case;
	am->argv = null_list;
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetupArgManager
	( ArgManager_t *am, LowerUpper_t force_case, int argc, char ** argv, bool clone )
{
    DASSERT(am);
    memset(am,0,sizeof(*am));
    am->force_case = force_case;
    if (clone)
	CloneArgManager(am,argc,argv);
    else
	AttachArgManager(am,argc,argv);
}

///////////////////////////////////////////////////////////////////////////////

void AttachArgManager ( ArgManager_t *am, int argc, char ** argv )
{
    DASSERT(am);
    ResetArgManager(am);
    am->argc = argc;
    am->argv = argv;
}

///////////////////////////////////////////////////////////////////////////////

void CloneArgManager ( ArgManager_t *am, int argc, char ** argv )
{
    DASSERT(am);
    AttachArgManager(am,argc,argv);
    PrepareEditArgManager(am,0);
}

///////////////////////////////////////////////////////////////////////////////

void CopyArgManager ( ArgManager_t *dest, const ArgManager_t *src )
{
    DASSERT(dest);
    if (!src)
	ResetArgManager(dest);
    else if( dest != src )
	AttachArgManager(dest,src->argc,src->argv);
}

///////////////////////////////////////////////////////////////////////////////

void MoveArgManager ( ArgManager_t *dest, ArgManager_t *src )
{
    DASSERT(dest);
    ResetArgManager(dest);
    if (src)
    {
	dest->argv = src->argv;
	dest->argc = src->argc;
	dest->size = src->size;
	memset(src,0,sizeof(*src));
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrepareEditArgManager ( ArgManager_t *am, int needed_space )
{
    DASSERT(am);

    uint new_size = am->argc;
    if ( needed_space > 0 )
	new_size += needed_space;
    new_size = GetGoodAllocSize2(new_size+(new_size>>4)+10,sizeof(*am->argv));
    PRINT("PrepareEditArgManager(,%d) n=%d, size: %d -> %d\n",
		needed_space, am->argc, am->size, new_size );

    if (am->size)
	am->argv = REALLOC(am->argv,sizeof(*am->argv)*new_size);
    else
    {
	char **old = am->argv;
	am->argv = MALLOC(sizeof(*am->argv)*new_size);

	uint i;
	for ( i = 0; i < am->argc; i++ )
	    am->argv[i] = old[i] ? STRDUP(old[i]) : 0;
    }
    am->size = new_size - 1;
    DASSERT( am->argc <= am->size );
    am->argv[am->argc] = 0;
}

///////////////////////////////////////////////////////////////////////////////

uint AppendArgManager ( ArgManager_t *am, ccp arg1, ccp arg2, bool move_arg )
{
    DASSERT(am);
    PRINT("AppendArgManager(,%s,%s,%d)\n",arg1,arg2,move_arg);
    return InsertArgManager(am,am->argc,arg1,arg2,move_arg);
}

///////////////////////////////////////////////////////////////////////////////

static char ** insert_arg_manager
	( ArgManager_t *am, char **dest, ccp p_arg, bool move_arg )
{
    DASSERT(am);
    if (p_arg)
    {
	char *arg = (char*)p_arg;
	if (!move_arg)
	    arg = STRDUP(arg);

	if ( am->force_case == LOUP_LOWER || am->force_case == LOUP_UPPER )
	{
	    char * end = arg + strlen(arg) + 1;
	    if (am->force_case == LOUP_LOWER)
		StringLowerE(arg,end,arg);
	    else
		StringUpperE(arg,end,arg);
	}
	*dest++ = arg;
    }
    return dest;
}

//-----------------------------------------------------------------------------

uint InsertArgManager
	( ArgManager_t *am, int pos, ccp arg1, ccp arg2, bool move_arg )
{
    DASSERT(am);
    PRINT("InsertArgManager(,%d,%s,%s,%d)\n",pos,arg1,arg2,move_arg);

    pos = CheckIndex1(am->argc,pos);
    const int n = (arg1!=0) + (arg2!=0);
    if (n)
    {
	PrepareEditArgManager(am,n);
	DASSERT( pos>=0 && pos <= am->argc );
	char **dest = am->argv + pos;
	if ( pos < am->argc )
	    memmove( dest+n, dest, (u8*)(am->argv+am->argc) - (u8*)dest );

	dest = insert_arg_manager(am,dest,arg1,move_arg);
	dest = insert_arg_manager(am,dest,arg2,move_arg);

	am->argc += n;
	am->argv[am->argc] = 0;
    }
    return pos+n;
}

//-----------------------------------------------------------------------------

uint InsertListArgManager
	( ArgManager_t *am, int pos, int argc, char ** argv, bool move_arg )
{
    DASSERT(am);

    pos = CheckIndex1(am->argc,pos);
    if ( argc > 0 )
    {
	PrepareEditArgManager(am,argc);
	DASSERT( pos>=0 && pos <= am->argc );
	char **dest = am->argv + pos;
	if ( pos < am->argc )
	    memmove( dest+argc, dest, (u8*)(am->argv+am->argc) - (u8*)dest );
	pos += argc;
	am->argc += argc;
	am->argv[am->argc] = 0;

	if ( move_arg && am->force_case == LOUP_AUTO )
	    memcpy( dest, argv, argc*sizeof(*dest) );
	else
	    while ( argc-- > 0 ) 
		dest = insert_arg_manager(am,dest,*argv++,move_arg);
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////

uint ReplaceArgManager
	( ArgManager_t *am, int pos, ccp arg1, ccp arg2, bool move_arg )
{
    PRINT("ReplaceArgManager(,%d,%s,%s,%d)\n",pos,arg1,arg2,move_arg);

    pos = CheckIndex1(am->argc,pos);
    const int n = (arg1!=0) + (arg2!=0);
    if (n)
    {
	PrepareEditArgManager(am,n);
	DASSERT( pos>=0 && pos <= am->argc );
	char **dest = am->argv + pos;
	if (arg1)
	{
	    if ( pos++ < am->argc )
		FreeString(*dest);
	    *dest++ = move_arg ? (char*)arg1 : STRDUP(arg1);
	}
	if (arg2)
	{
	    if ( pos++ < am->argc )
		FreeString(*dest);
	    *dest++ = move_arg ? (char*)arg2 : STRDUP(arg2);
	}
	if ( pos > am->argc )
	{
	    am->argc = pos;
	    am->argv[am->argc] = 0;
	}
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////

uint RemoveArgManager ( ArgManager_t *am, int pos, int count )
{
    DASSERT(am);

 #if HAVE_PRINT
    const int oldpos = pos;
 #endif
    const int n = CheckIndexC(am->argc,&pos,count);
    PRINT("RemoveArgManager(,%d,%d) : remove n=%d: %d..%d\n",
		oldpos, count, n, pos, pos+n );

    DASSERT( pos >= 0 && pos <= am->argc );
    DASSERT( n >= 0 && pos+n <= am->argc );

    if ( n > 0 )
    {
	PrepareEditArgManager(am,0);
	char **dest = am->argv + pos;

	uint i;
	for ( i = 0; i <n; i++ )
	    FreeString(dest[i]);
	am->argc -= n;
	memmove( dest, dest+n, (u8*)(am->argv+am->argc) - (u8*)dest );
	am->argv[am->argc] = 0;
    }
    return pos;
}

///////////////////////////////////////////////////////////////////////////////

uint ScanSimpleArgManager ( ArgManager_t *am, ccp src )
{
    uint count = 0;
    if (src)
    {
	for(;;)
	{
	    while ( *src == ' ' || *src == '\t' )
		src++;
	    if (!*src)
		break;

	    ccp start = src;
	    while ( *src && *src != ' ' && *src != '\t' )
		src++;

	    ccp arg = MEMDUP(start,src-start);
	    AppendArgManager(am,arg,0,true);
	}
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

uint ScanQuotedArgManager ( ArgManager_t *am, ccp src, bool is_utf8 )
{
    uint count = 0;
    if (src)
    {
	char buf[10000], *bufend = buf + sizeof(buf) - 2;

	for(;;)
	{
	    while (isspace(*src))
		src++;
	    if (!*src)
		break;

	    char *dest = buf;
	    while ( dest < bufend )
	    {
		if ( *src == '\'' || *src == '\"' )
		{
		    uint scanned;
		    dest += ScanEscapedString(dest,bufend-dest,src,-1,is_utf8,-1,&scanned);
		    src += scanned;
		}
		else
		{
		    while ( dest < bufend && *src && !isspace(*src) && *src != '\'' && *src != '\"' )
		    {
			if ( *src == '\\' && src[1] )
			    src++;
			*dest++ = *src++;
		    }
		}
		if ( !*src || isspace(*src) )
		    break;
	    }
	    ccp arg = MEMDUP(buf,dest-buf);
	    AppendArgManager(am,arg,0,true);
	}
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanFileArgManager
(
    ArgManager_t	*am,		// valid arg-manager
    int			pos,		// insert position, relative to end if <0
    ccp			fname,		// filename to open
    int			silent,		// 0: print all error messages
					// 1: suppress file size warning
					// 2: suppress all error messages
    int			*n_added
)
{
    if (n_added)
	*n_added = 0;
    pos = CheckIndex1(am->argc,pos);

    u8 *data;
    size_t size;
    enumError err = LoadFileAlloc(fname,0,0,&data,&size,0,silent,0,0);

    if (!err)
    {
	if (memchr(data,0,size))
	{
	    if ( silent < 2 )
		ERROR0(ERR_WARNING,"Binary file ignored: %s\n",fname);
	    err = ERR_WARNING;
	}
	else
	{
	    ArgManager_t temp = { .force_case = am->force_case };
	    ScanQuotedArgManager(&temp,(ccp)data,true);
	    uint newpos = InsertListArgManager(am,pos,temp.argc,temp.argv,true);
	    if (n_added)
		*n_added = newpos - pos;
	    FREE(temp.argv);
	}
    }

    FREE(data);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError ExpandAtArgManager
(
    ArgManager_t	*am,		// valid arg-manager
    arg_expand_mode_t	expand_mode,	// objects to be replaced
    int			recursion,	// maximum recursion depth
    int			silent		// 0: print all error messages
					// 1: suppress file size warning
					// 2: suppress all error messages
)
{
    if (!am)
	return ERR_MISSING_PARAM;

    expand_mode &= AMXM_ALL;
    if (!expand_mode)
	return ERR_OK;

    enumError max_err = ERR_OK;

    for ( int try = 0;; try++ )
    {
	bool dirty = false;

	int pos = 0;
	while ( pos < am->argc )
	{
	    ccp arg = am->argv[pos];
	    if (arg)
	    {
		int n_rm = 0;
		ccp fname = 0;
		int len = strlen(arg);

		if ( len >= 1 && *arg == '@' )
		{
		    if ( len > 1 && expand_mode & AMXM_P1 )
		    {
			n_rm = 1;
			fname = arg+1;
		    }
		    else if ( len == 1 && expand_mode & AMXM_P2 && pos+1 < am->argc )
		    {
			n_rm = 2;
			fname = am->argv[pos+1];
		    }
		}
		else if ( len >= 2 && !memcmp(arg,"-@",2) )
		{
		    if ( len > 2 && expand_mode & AMXM_S1 )
		    {
			n_rm = 1;
			fname = arg+2;
		    }
		    else if ( len == 2 && expand_mode & AMXM_S2 && pos+1 < am->argc )
		    {
			n_rm = 2;
			fname = am->argv[pos+1];
		    }
		}
		else if ( len >= 3 && !memcmp(arg,"--@",3) )
		{
		    if ( len > 3 && arg[3] == '=' && expand_mode & AMXM_L1 )
		    {
			n_rm = 1;
			fname = arg+4;
		    }
		    else if ( len == 3 && expand_mode & AMXM_L2 && pos+1 < am->argc )
		    {
			n_rm = 2;
			fname = am->argv[pos+1];
		    }
		}

		if (n_rm)
		{
		    PRINT0("rm @%u %d, file=%s\n",pos,n_rm,fname);
		    int n_added;
		    enumError err = ScanFileArgManager(am,pos,fname,silent,&n_added);
		    if ( max_err < err )
			 max_err = err;
		    if (n_added)
		    {
			pos += n_added;
			dirty = true;
		    }

		    // remove after insert to keep fname valid!
		    PRINT0("rm @%u %d\n",pos,n_rm);
		    RemoveArgManager(am,pos,n_rm);
		}
		else
		    pos++;

	    } // if (arg)
	} // while

	if ( !dirty || try >= recursion )
	    break;
    }

    return max_err;
}    

///////////////////////////////////////////////////////////////////////////////

bool CheckFilterArgManager ( const ArgManager_t *filter, ccp name )
{
    if ( !name || !*name )
	return false;

    if (!filter->argc)
	return true;

    char buf[200];
    if ( filter->force_case == LOUP_LOWER )
    {
	StringLowerS(buf,sizeof(buf),name);
	name = buf;
    }
    else if ( filter->force_case == LOUP_UPPER )
    {
	StringUpperS(buf,sizeof(buf),name);
	name = buf;
    }

    bool fallback_status = true;
    for ( int i = 0; i < filter->argc; i++ )
    {
	ccp arg = filter->argv[i];
	const bool status = *arg != '/';
	if (status)
	    fallback_status = false;
	else
	    arg++;

	const uint len = strlen(arg);
	if ( *arg == '=' )
	{
	    if ( !strcmp(arg+1,name) )
		return status;
	}
	else if ( *arg == '^' )
	{
	    if ( len > 1 && !strncmp(arg+1,name,len-1) )
		return status;
	}
	else
	    if ( len && strstr(name,arg) )
		return status;
    }

    return fallback_status;
}

///////////////////////////////////////////////////////////////////////////////

void DumpArgManager ( FILE *f, int indent, const ArgManager_t *am, ccp title )
{
    if ( !f || !am )
	return;

    indent = NormalizeIndent(indent);
    if (title)
	fprintf(f,"%*s" "ArgManager[%s]: N=%d",
		indent,"", title, am->argc );
    else
	fprintf(f,"%*s" "ArgManager: N=%d", indent,"", am->argc );

    if (am->size)
	fprintf(f,"/%u\n",am->size);
    else
	fputc('\n',f);

    if (am->argc)
    {
	char buf[10];
	int fw = snprintf(buf,sizeof(buf),"%u",am->argc-1)+2;

	uint i;
	for ( i = 0; i < am->argc; i++ )
	    fprintf(f,"%*s%*u: |%s|\n", indent,"", fw,i, am->argv[i] );
    }
}

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

///////////////////////////////////////////////////////////////////////////////

mem_t MemCat2A ( const mem_t m1, const mem_t m2 )
{
    const uint len1 = m1.len >= 0 ? m1.len : strlen(m1.ptr);
    const uint len2 = m2.len >= 0 ? m2.len : strlen(m2.ptr);

    char *dest;
    mem_t res;
    res.len = len1 + len2;
    res.ptr = dest = MALLOC(res.len+1);
    dest[res.len] = 0;

    if (len1)
	memcpy(dest,m1.ptr,len1);
    if (len2)
	memcpy(dest+len1,m2.ptr,len2);

    return res;
}

//-----------------------------------------------------------------------------

mem_t MemCat3A ( const mem_t m1, const mem_t m2, const mem_t m3 )
{
    const uint len1 = m1.len >= 0 ? m1.len : strlen(m1.ptr);
    const uint len2 = m2.len >= 0 ? m2.len : strlen(m2.ptr);
    const uint len3 = m3.len >= 0 ? m3.len : strlen(m3.ptr);

    char *dest;
    mem_t res;
    res.len = len1 + len2 + len3;
    res.ptr = dest = MALLOC(res.len+1);
    dest[res.len] = 0;

    if (len1)
	memcpy(dest,m1.ptr,len1);
    if (len2)
	memcpy(dest+len1,m2.ptr,len2);
    if (len3)
	memcpy(dest+len1+len2,m3.ptr,len3);

    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t MemCatSep2A ( const mem_t sep, const mem_t m1, const mem_t m2 )
{
    const uint slen = sep.len >= 0 ? sep.len : strlen(sep.ptr);
    if (!slen)
	return MemCat2A(m1,m2);

    const uint len1 = m1.len >= 0 ? m1.len : strlen(m1.ptr);
    const uint len2 = m2.len >= 0 ? m2.len : strlen(m2.ptr);

    char *dest;
    mem_t res;
    res.len = len1 + len2;
    if ( len1 && len2 )
	res.len += slen;

    res.ptr = dest = MALLOC(res.len+1);
    dest[res.len] = 0;

    if (len1)
    {
	memcpy(dest,m1.ptr,len1);
	dest += len1;
    }

    if (len2)
    {
	if (len1)
	{
	    memcpy(dest,sep.ptr,slen);
	    dest += slen;
	}

	memcpy(dest,m2.ptr,len2);
    }

    return res;
}

//-----------------------------------------------------------------------------

mem_t MemCatSep3A ( const mem_t sep, const mem_t m1, const mem_t m2, const mem_t m3 )
{
    const uint slen = sep.len >= 0 ? sep.len : strlen(sep.ptr);
    if (!slen)
	return MemCat3A(m1,m2,m3);

    const uint len1 = m1.len >= 0 ? m1.len : strlen(m1.ptr);
    const uint len2 = m2.len >= 0 ? m2.len : strlen(m2.ptr);
    const uint len3 = m3.len >= 0 ? m3.len : strlen(m3.ptr);

    char *dest;
    mem_t res;
    res.len = len1 + len2 + len3
		+ ( (len1>0) + (len2>0) + (len3>0) - 1 ) * slen;

    res.ptr = dest = MALLOC(res.len+1);

    if (len1)
    {
	memcpy(dest,m1.ptr,len1);
	dest += len1;
    }

    if (len2)
    {
	if ( dest > res.ptr )
	{
	    memcpy(dest,sep.ptr,slen);
	    dest += slen;
	}

	memcpy(dest,m2.ptr,len2);
	dest += len2;
    }

    if (len3)
    {
	if ( dest > res.ptr )
	{
	    memcpy(dest,sep.ptr,slen);
	    dest += slen;
	}

	memcpy(dest,m3.ptr,len3);
	dest += len3;
    }

    *dest = 0;
    ASSERT( dest == res.ptr + res.len );
    return res;
}

///////////////////////////////////////////////////////////////////////////////
// UTF8 support

mem_t MidMem8 ( const mem_t src, int begin, int count )
{
    if ( !src.ptr || !src.len )
	return src;

    ccp src_end = src.ptr + src.len;
    const int u8len = ScanUTF8LengthE(src.ptr,src_end);
    begin = CheckIndex1(u8len,begin);
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
    res.ptr = SkipUTF8CharE(src.ptr,src_end,begin);
    const int max = u8len - begin;
    res.len = SkipUTF8CharE( res.ptr, src_end, count < max ? count : max ) - res.ptr;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t ExtractMem8 ( const mem_t src, int begin, int end )
{
    if ( !src.ptr || !src.len )
	return src;

    ccp src_end = src.ptr + src.len;
    const int u8len = ScanUTF8LengthE(src.ptr,src_end);
    const int count = CheckIndex2(u8len,&begin,&end);

    mem_t res;
    res.ptr = SkipUTF8CharE( src.ptr, src_end, begin );
    res.len = SkipUTF8CharE( res.ptr, src_end, count ) - res.ptr;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

mem_t ExtractEndMem8 ( const mem_t src, int begin, int end )
{
    if ( !src.ptr || !src.len )
	return src;

    ccp src_end = src.ptr + src.len;
    const int u8len = ScanUTF8LengthE(src.ptr,src_end);
    const int count = CheckIndex2End(u8len,&begin,&end);

    mem_t res;
    res.ptr = SkipUTF8CharE( src.ptr, src_end, begin );
    res.len = SkipUTF8CharE( res.ptr, src_end, count ) - res.ptr;
    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct exmem_t			///////////////
///////////////////////////////////////////////////////////////////////////////

exmem_t EmptyExMem = {{EmptyString,0},0,false,false,false,false};
exmem_t NullExMem  = {{0,0},0,false,false,false,false};

///////////////////////////////////////////////////////////////////////////////

void ResetExMem ( const exmem_t *em )
{
    if (em)
    {
	if (em->is_alloced)
	    FreeString(em->data.ptr);
	memset((exmem_t*)em,0,sizeof(*em));
    }
}

///////////////////////////////////////////////////////////////////////////////

void FreeExMem ( const exmem_t *em )
{
    if (em)
    {
	if (em->is_alloced)
	{
	    exmem_t *em2 = (exmem_t*)em;
	    FreeString(em2->data.ptr);
	    em2->data.ptr = 0;
	    em2->data.len = 0;
	    em2->is_alloced = false;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void FreeExMemCM ( const exmem_t *em, CopyMode_t copy_mode )
{
    if ( em && copy_mode == CPM_MOVE )
    {
	if (em->is_alloced)
	    FreeString(em->data.ptr);
	memset((exmem_t*)em,0,sizeof(*em));
    }
}

///////////////////////////////////////////////////////////////////////////////

exmem_t AllocExMemS
(
    cvp		src,		// NULL or source
    int		src_len,	// len of 'src'; if -1: use strlen(source)
    bool	try_circ,	// use circ-buffer, if result is small enough
    cvp		orig,		// NULL or data to compare
    int		orig_len	// len of 'orig'; if -1: use strlen(source)
)
{
    exmem_t em = {{0}};
    if (src)
    {
	if ( src_len < 0 )
	    src_len = strlen(src);
	if (!src_len)
	    em.data.ptr = EmptyString;
	else
	{
	    em.data.len = src_len;
	    if ( orig
		&& src_len == ( orig_len < 0 ? strlen(orig) : orig_len )
		&& !memcmp(src,orig,src_len)
		)
	    {
		em.data.ptr = orig;
		em.is_original = true;
	    }
	    else if ( try_circ && src_len <= CIRC_BUF_MAX_ALLOC )
	    {
		em.data.ptr = CopyCircBuf(src,src_len);
		em.is_circ_buf = true;
	    }
	    else
	    {
		em.data.ptr = MEMDUP(src,src_len);
		em.is_alloced = true;
	    }
	}
    }

    return em;
}

///////////////////////////////////////////////////////////////////////////////

void AssignExMem ( exmem_t *dest, const exmem_t *source, CopyMode_t copy_mode )
{
    if ( dest && dest != source )
    {
	if (dest->is_alloced)
	    FreeString(dest->data.ptr);
	const bool dest_key_alloced = dest->is_key_alloced;
	if (source)
	{
	    switch (copy_mode)
	    {
		case CPM_COPY:
		    memset(dest,0,sizeof(*dest));
		    dest->data.len	= source->data.len;
		    dest->data.ptr	= MEMDUP(source->data.ptr,source->data.len);
		    dest->is_alloced	= true;
		    dest->attrib	= source->attrib;
		    break;

		case CPM_MOVE:
		    *dest = *source;
		    memset((exmem_t*)source,0,sizeof(*source));
		    break;

		case CPM_LINK:
		    *dest = *source;
		    dest->is_original	= false;
		    dest->is_alloced	= false;
		    break;
	    }
	}
	else
	    memset(dest,0,sizeof(*dest));
	dest->is_key_alloced = dest_key_alloced;
    }
}

///////////////////////////////////////////////////////////////////////////////

void AssignExMemS ( exmem_t *dest, cvp source, int slen, CopyMode_t copy_mode )
{
    if (dest)
    {
	ccp free_str = dest->is_alloced ? dest->data.ptr : 0;
	const bool dest_key_alloced = dest->is_key_alloced;
	memset(dest,0,sizeof(*dest));
	dest->is_key_alloced = dest_key_alloced;

	if (source)
	{
	    dest->data.len = slen < 0 ? strlen(source) : slen;
	    switch (copy_mode)
	    {
		case CPM_COPY:
		    dest->data.ptr	= MEMDUP(source,dest->data.len);
		    dest->is_alloced	= true;
		    break;

		case CPM_MOVE:
		    dest->is_alloced	= true;
		    // fall through
		case CPM_LINK:
		    dest->data.ptr	= source;
		    break;
	    }
	}
	FreeString(free_str);
    }
}

///////////////////////////////////////////////////////////////////////////////

#if 0 // [[obsolete]] since 2021-11
exmem_t CopyExMemS ( cvp string, int slen, CopyMode_t copy_mode )
{
    exmem_t em = {{0}};

    if (string)
    {
	em.data.len = slen < 0 ? strlen(string) : slen;

	switch (copy_mode)
	{
	    case CPM_COPY:
		em.data.ptr = MEMDUP(string,em.data.len);
		em.is_alloced = true;
		break;

	    case CPM_MOVE:
		em.is_alloced = true;
		// fall through
	    case CPM_LINK:
		em.data.ptr = string;
		break;
	}
    }

    return  em;
}
#endif

///////////////////////////////////////////////////////////////////////////////

ccp PrintExMem ( const exmem_t * em ) // print to circ-buf
{
    if (!em)
	return MinusString;

    char ebuf[60];
    uint len;
    PrintEscapedString(ebuf,sizeof(ebuf),em->data.ptr,em->data.len,CHMD__MODERN,'"',&len);

    return PrintCircBuf(
	"[%c%c%c%c:%x] %u/%u %s",
	em->is_key_alloced	? 'a' : '-',	// flags: same as in DumpEML()
	em->is_alloced		? 'A' : '-',
	em->is_circ_buf		? 'C' : '-',
	em->is_original		? 'O' : '-',
	em->attrib,
	len, em->data.len, ebuf );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint GetMemSrcN ( const mem_src_t *src )
{
    if (!src)
	return 0;

    if ( src->n_src >= 0 )
	return src->n_src;

    const mem_t *ptr = src->src;
    while ( ptr->ptr )
	ptr++;
    const uint n_src = ptr - src->src;

    if ( src->allow_write )
	*(uint*)&src->n_src = n_src;
    return n_src;
}

///////////////////////////////////////////////////////////////////////////////

uint GetMemSrcLen ( mem_t sep, const mem_src_t *src )
{
    if ( !src || src->n_src <= 0 )
	return 0;

    int n_valid_src = 0, sum_len = 0;
    int n_src = GetMemSrcN(src);
    for ( const mem_t *ptr = src->src; n_src > 0; ptr++, n_src-- )
    {
	int slen = ptr->len;
	if ( slen < 0 )
	{
	    slen = ptr->ptr ? strlen(ptr->ptr) : 0;
	    if (src->allow_write)
		*(int*)&ptr->len = slen;
	}
	if ( slen > 0 )
	{
	    n_valid_src++;
	    sum_len += slen;
	}
    }

    if ( n_valid_src > 1 )
	sum_len += (n_valid_src-1)*sep.len;
    return sum_len;
}

///////////////////////////////////////////////////////////////////////////////

exmem_t GetExmemDestBuf ( const exmem_dest_t * dest, uint len )
{
    exmem_t res = { .data.len = len };
    len++; // for additional 0-term 

    if (!dest)
	goto malloc;
    else if ( dest->buf && len <= dest->buf_size )
    {
	res.data.ptr = dest->buf;
    }
    else if ( dest->try_circ && len <= CIRC_BUF_MAX_ALLOC )
    {
	res.is_circ_buf = true;
	res.data.ptr = GetCircBuf(len);
    }
    else
    {
     malloc:;
	res.is_alloced = true;
	res.data.ptr = MALLOC(len);
    }

    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

exmem_t ExMemCat
(
    exmem_dest_t	*dest,	// kind of destination, if NULL then MALLOC()
    mem_t		sep,	// insert separators between sources with len>0
    const mem_src_t	*src	// sources. NULL allowed
)
{
    const uint total_size = GetMemSrcLen(sep,src);
    exmem_t res = GetExmemDestBuf(dest,total_size);
    char *bufdest = (char*)res.data.ptr; 

    int n_src = GetMemSrcN(src);
    for ( const mem_t *ptr = src->src; n_src > 0; ptr++, n_src-- )
    {
	int slen = ptr->len;
	if ( slen < 0 )
	    slen = ptr->ptr ? strlen(ptr->ptr) : 0;
	if ( slen > 0 )
	{
	    if ( sep.len && bufdest > res.data.ptr )
	    {
		memcpy(bufdest,sep.ptr,sep.len);
		bufdest += sep.len;
	    }

	    memcpy(bufdest,ptr->ptr,slen);
	    bufdest += slen;
	    ASSERT( bufdest <= res.data.ptr + res.data.len );
	}
    }
    ASSERT( bufdest == res.data.ptr + res.data.len );
    *bufdest = 0;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

exmem_t ExMemCat2
(
    exmem_dest_t	*dest,	// kind of destination, if NULL then MALLOC()
    mem_t		sep,	// insert separators between sources with len>0
    mem_t		src1,	// first source
    mem_t		src2	// second source
)
{
    mem_t list[] = { src1, src2 };
    mem_src_t src = { .src = list, .n_src = 2, true };
    return ExMemCat(dest,sep,&src);
}

///////////////////////////////////////////////////////////////////////////////

exmem_t ExMemCat3
(
    exmem_dest_t	*dest,	// kind of destination, if NULL then MALLOC()
    mem_t		sep,	// insert separators between sources with len>0
    mem_t		src1,	// first source
    mem_t		src2,	// second source
    mem_t		src3	// third source
)
{
    mem_t list[] = { src1, src2, src3 };
    mem_src_t src = { .src = list, .n_src = 3, true };
    return ExMemCat(dest,sep,&src);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct mem_list_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetMemList ( mem_list_t *ml )
{
    if (ml)
    {
	FREE(ml->list);
	FREE(ml->buf);
	memset(ml,0,sizeof(*ml));
    }
}

///////////////////////////////////////////////////////////////////////////////
// [[?]]

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
// [[?]]

void NeedBufMemList ( mem_list_t *ml, uint need_size, uint extra_size )
{
    need_size += ml->buf_used;
    if ( ml->buf_size < need_size )
    {
	need_size = GetGoodAllocSize(need_size+extra_size);
	PRINT("MEM-LIST: MALLOC/BUF(%u/%u>%u)\n",ml->buf_used,ml->buf_size,need_size);
	char *new_buf = MALLOC(need_size);

	//--- copy existing strings

	char *bufptr = new_buf;
	mem_t *dest = ml->list;
	uint i;
	for ( i = 0; i < ml->used; i++, dest++ )
	    if (dest->len)
	    {
		memcpy(bufptr,dest->ptr,dest->len);
		dest->ptr = bufptr;
		bufptr += dest->len;
		*bufptr++ = 0;
		DASSERT( bufptr <= new_buf + need_size );
	    }
	FREE(ml->buf);
	ml->buf      = new_buf;
	ml->buf_used = bufptr - new_buf;
	ml->buf_size = need_size;
    }
}

///////////////////////////////////////////////////////////////////////////////
// [[?]]

void NeedElemMemList ( mem_list_t *ml, uint need_elem, uint need_size )
{
    DASSERT(ml);
    need_elem += ml->used;
    if ( ml->size < need_elem )
    {
	ml->size = GetGoodAllocSize2(need_elem,sizeof(*ml->list));
	PRINT("MEM-LIST: REALLOC/LIST(%u)\n",ml->size);
	ml->list = REALLOC(ml->list,sizeof(*ml->list)*ml->size);
    }

    NeedBufMemList(ml,need_size,0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[?]]

void InsertMemListN
(
    mem_list_t		*ml,		// valid destination mem_list
    int			pos,		// insert position => CheckIndex1()
    const mem_t		*src,		// source list
    uint		src_len,	// number of elements in source list
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
	else if (  src_ignore == MEMLM_ALL
		|| src_ignore == MEMLM_IGN_NULL && sptr->ptr
		|| src_ignore == MEMLM_REPL_NULL
		)
	{
	    add_elem++;
	}
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

	// don't use NeedBufMemList() here, because the old buffer is needed
	// BUT POSSIBLE: char old_buf = ml->buf; ml->buf = 0; GrowBufMemList(ml,need_size)

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
	else if ( src_ignore == MEMLM_ALL || src_ignore == MEMLM_IGN_NULL && sptr->ptr )
	{
	    dest->ptr = sptr->ptr ? EmptyString : NULL;
	    dest->len = 0;
	    dest++;
	    DASSERT( dest <= ml->list + ml->used );
	}
	else if ( src_ignore == MEMLM_REPL_NULL )
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
// [[?]]

void CatMemListN
(
    mem_list_t		*dest,		// valid destination mem_list
    const mem_list_t	**src_list,	// list with mem lists, element may be NULL
    uint		n_src_list,	// number of elements in 'src'
    mem_list_mode_t	src_ignore	// how to manage NULL and empty strings
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
	    else if (  src_ignore == MEMLM_ALL
		    || src_ignore == MEMLM_IGN_NULL && sptr->ptr
		    || src_ignore == MEMLM_REPL_NULL
		    )
	    {
		need_elem++;
	    }
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
	    else if ( src_ignore == MEMLM_ALL || src_ignore == MEMLM_IGN_NULL && sptr->ptr )
	    {
		listptr->ptr = sptr->ptr ? EmptyString : NULL;
		listptr->len = 0;
		listptr++;
		DASSERT( listptr <= new_list + need_elem );
	    }
	    else if ( src_ignore == MEMLM_REPL_NULL )
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
// [[?]]

uint LeftMemList ( mem_list_t *ml, int count )
{
    DASSERT(ml);
    return ml->used = CheckIndex1(ml->used,count);
}

///////////////////////////////////////////////////////////////////////////////
// [[?]]

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
// [[?]]

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
// [[?]]

uint ExtractMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    ml->used = CheckIndex2(ml->used,&begin,&end);
    memmove( ml->list, ml->list + begin, ml->used * sizeof(*ml->list) );
    return ml->used;
}

///////////////////////////////////////////////////////////////////////////////
// [[?]]

uint ExtractEndMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    ml->used = CheckIndex2End(ml->used,&begin,&end);
    memmove( ml->list, ml->list + begin, ml->used * sizeof(*ml->list) );
    return ml->used;
}

///////////////////////////////////////////////////////////////////////////////
// [[?]]

uint RemoveMemList ( mem_list_t *ml, int begin, int end )
{
    DASSERT(ml);
    const uint sub = CheckIndex2(ml->used,&begin,&end);
    memmove( ml->list + begin, ml->list + end, ( ml->used - end ) * sizeof(*ml->list) );
    return ml->used -= sub;
}

///////////////////////////////////////////////////////////////////////////////
// [[?]]

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
    DASSERT( mode == CPM_COPY || mode == CPM_MOVE || mode == CPM_LINK );

    if ( !data || !size )
    {
	if (res_alloced)
	    *res_alloced = false;
	return (void*)EmptyString;
    }

    if (res_alloced)
	*res_alloced = mode != CPM_LINK;

    return mode == CPM_COPY ? MEMDUP(data,size) : (void*)data;
}

///////////////////////////////////////////////////////////////////////////////

char * CopyString
(
    ccp			string,		// string to copy/move/link
    CopyMode_t		mode,		// copy mode
    bool		*res_alloced	// not NULL:
					//   store true, if data must be freed
					//   otherwise store false
)
{
    DASSERT( mode == CPM_COPY || mode == CPM_MOVE || mode == CPM_LINK );

    if ( !string || !*string )
    {
	if (res_alloced)
	    *res_alloced = false;
	return string ? (char*)EmptyString : 0;
    }

    if (res_alloced)
	*res_alloced = mode != CPM_LINK;

    return mode == CPM_COPY ? STRDUP(string) : (char*)string;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		ContainerData_t, Container_t		///////////////
///////////////////////////////////////////////////////////////////////////////

Container_t * CreateContainer
(
    // returns 'c' or the alloced container
    // 'c' is always initialized

    Container_t		*c,		// valid container, alloc one if NULL
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
    c->cdata		= cdata;

    cdata->data		= CopyData(data,size,mode,&cdata->data_alloced);
    cdata->size		= size;
    cdata->ref_count	= 1;
    if ( protect > 0 )
	cdata->protect_count++;
    return c;
}

///////////////////////////////////////////////////////////////////////////////

Container_t * InheritContainer
(
    // returns 'c' or the alloced container

    Container_t		*c,		// valid container, alloc one if NULL
    int			protect,	// initial value for protection
    const Container_t	*parent,	// NULL or valid parent container
    const void		*data,		// data to copy/move/link
    uint		size		// size of 'data'
)
{
    if (!c)
	c = CALLOC(sizeof(*c),1);

    if ( InContainerS(parent,data,size) || !InContainerS(c,data,size) )
    {
	UnlinkContainerData(c);
	UseContainerData(c,protect,parent);
    }
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
	    c->cdata		= cdata;
	    cdata->data		= CopyData(data,size,mode,&cdata->data_alloced);
	    cdata->size		= size;
	    cdata->ref_count	= 1;
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

    const Container_t	*c		// NULL or valid container
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

    Container_t		*c,		// valid container, alloc one if NULL
    int			protect,	// initial value for protection
    ContainerData_t	*cdata		// if not NULL: catch this
)
{
    if (!c)
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ContainerData_t * DisjoinContainerData
(
    // Disjoin data container date without freeing it. Call either
    // JoinContainerData() or FreeContainerData() to finish operation.
    // Reference counters are not modified.
    // Return the data container or NULL

    Container_t		*c		// NULL or valid container
)
{
    ContainerData_t *res = 0;
    if ( c && c->cdata )
    {
	const int old_protect = SetProtectContainer(c,0);
	res = c->cdata;
	c->cdata = 0;
	SetProtectContainer(c,old_protect);
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void JoinContainerData
(
    // Join a data container, that was diosjoined by DisjoinContainerData()
    // Reference counters are not modified.

    Container_t		*c,		// container to reset => no data
    ContainerData_t	*cdata		// NULL or container-data to join
)
{
    if (c)
    {
	UnlinkContainerData(c);
	DASSERT(!c->cdata);
	if (cdata)
	{
	    const int old_protect = SetProtectContainer(c,0);
	    c->cdata = cdata;
	    SetProtectContainer(c,old_protect);
	}
    }
    else
	FreeContainerData(cdata);
}

///////////////////////////////////////////////////////////////////////////////

void FreeContainerData
(
    // Decrement the reference counter of an already disjoined data container
    // and free it if unused.

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
    Container_t		*c,		// valid container, alloc one if NULL
    int			new_protect	// new protection value
)
{
    DASSERT(c);
    if (c->cdata)
    {
	if ( new_protect > 0 )
	{
	    if ( c->protect_level <= 0 )
		c->cdata->protect_count++;
	}
	else
	{
	    if ( c->protect_level > 0 )
		c->cdata->protect_count--;
	}
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
    ml->buf_used = 0;
    NeedBufMemList(ml,source.len+1,0);

    if ( max_fields <= 0 )
	max_fields = INT_MAX;

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
    ml->buf_used = 0;
    NeedBufMemList(ml,source.len+1,0);

    if ( max_fields <= 0 )
	max_fields = INT_MAX;

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

bool HaveWildcards ( mem_t str )
{
    if (!str.ptr)
	return false;
    if ( str.len < 0 )
	str.len = strlen(str.ptr);

    for ( ccp w = PATTERN_WILDCARDS; *w; w++ )
	if (memchr(str.ptr,*w,str.len))
	    return true;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

char * FindFirstWildcard ( mem_t str )
{
    if (!str.len)
	return 0;

    ccp end = str.ptr + str.len; 
    for ( ccp ptr = str.ptr; ptr < end; ptr++ )
	if (strchr(PATTERN_WILDCARDS,*ptr))
	    return (char*)ptr;
    return 0;
}

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

	    case '\t':
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
		for(;;)
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
				//   -2: no prefix found  (no rule found)
				//   -1: empty line (no rule found)
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

    for(;;)
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

    for(;;)
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

ccp GetKeywordError
(
    const KeywordTab_t	* key_tab,	// NULL or pointer to command table
    ccp			key_arg,	// analyzed command
    int			key_stat,	// status of ScanKeyword()
    ccp			object		// NULL or object for error messages
					//	default= 'command'
)
{
    DASSERT(key_arg);

    if ( !object || !*object )
	object = "command";

    if ( key_stat <= 0 )
	return PrintCircBuf("Unknown %s: %s",object,key_arg);

    char buf[1000], *dest = buf;
    if (key_tab)
    {
	int n = 0;
	char *buf_end = buf + sizeof(buf) - 2;
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

    return PrintCircBuf("%c%s abbreviation is ambiguous: %s%s",
		toupper((int)*object), object+1, key_arg, buf );
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
	char *buf_end = buf + sizeof(buf) - 2;
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

    return buf == temp ? CopyCircBuf(temp,len+1) : buf;
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
  { OFFON_OFF,		"DISABLED",	"NO",	0 },
  { OFFON_AUTO,		"AUTO",		"0",	0 },
  { OFFON_ON,		"ON",		"1",	0 },
  { OFFON_ON,		"ENABLED",	"YES",	0 },
  { OFFON_FORCE,	"FORCE",	"2",	0 },
  { 0,0,0,0 }
};

//-----------------------------------------------------------------------------

int ScanKeywordOffAutoOnEx
(
    // returns one of OFFON_*

    const KeywordTab_t	*keytab,	// Keyword table. If NULL, then use KeyTab_OFF_AUTO_ON[]
    ccp			arg,		// argument to scan
    int			on_empty,	// return this value on empty
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
)
{
    if ( !arg || !*arg )
	return on_empty;

    if (!keytab)
	keytab = KeyTab_OFF_AUTO_ON;

    int status;
    const KeywordTab_t *key = ScanKeyword(&status,arg,keytab);
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
	PrintKeywordError(keytab,arg,status,0,object);

    return OFFON_ERROR;
}

//-----------------------------------------------------------------------------

ccp GetKeywordOffAutoOn ( OffOn_t value )
{
    const KeywordTab_t *key = GetKewordById(KeyTab_OFF_AUTO_ON,value);
    return key ? key->name1 : "?";
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			LOWER/AUTO/UPPER		///////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t KeyTab_LOWER_AUTO_UPPER[] =
{
  { LOUP_LOWER,		"LOWER",	"LOWERCASE",	0 },
  { LOUP_LOWER,		"LOCASE",	"-1",		0 },
  { LOUP_AUTO,		"AUTO",		"0",		0 },
  { LOUP_UPPER,		"UPPER",	"UPPERCASE",	0 },
  { LOUP_UPPER,		"UPCASE",	"1",		0 },
  { 0,0,0,0 }
};

//-----------------------------------------------------------------------------

int ScanKeywordLowerAutoUpper
(
    // returns one of LOUP_*

    ccp			arg,		// argument to scan
    int			on_empty,	// return this value on empty
    uint		max_num,	// >0: additionally accept+return number <= max_num
    ccp			object		// NULL (silent) or object for error messages
)
{
    if ( !arg || !*arg )
	return on_empty;

    int status;
    const KeywordTab_t *key = ScanKeyword(&status,arg,KeyTab_LOWER_AUTO_UPPER);
    if (key)
	return key->id;

    if ( max_num > 0 )
    {
	char *end;
	const int num = str2l(arg,&end,10);
	if ( !*end && num >= LOUP_LOWER && num <= max_num )
	    return num;
    }

    if (object)
	PrintKeywordError(KeyTab_LOWER_AUTO_UPPER,arg,status,0,object);

    return LOUP_ERROR;
}

//-----------------------------------------------------------------------------

ccp GetKeywordLowerAutoUpper ( LowerUpper_t value )
{
    const KeywordTab_t *key = GetKewordById(KeyTab_OFF_AUTO_ON,value);
    return key ? key->name1 : "?";
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

    static uint log_pid	= 0;
    static uint log_seq	= 0;
    static FILE *log	= 0;

    if ( cli->log_fname && cli->log_level && !log_pid )
    {
	log_pid = getpid();
	log = fopen(cli->log_fname,"ab");
	if (log)
	{
	    fcntl(fileno(log),F_SETFD,FD_CLOEXEC);
	    fprintf(log,"\n#\f\n#OPEN %s, pid=%d\n\n",
			PrintTimeByFormat("%F %T %z",time(0)), log_pid );
	}
    }
    uint log_level = log ? cli->log_level : 0;

    cli->read_cmd_len = 0;
    const char sep = cli->command_sep ? cli->command_sep : ';';
    ccp cmd = cli->command, end = cmd + cli->command_len;
    if ( log_level > 0 )
    {
	fprintf(log,"\n++ %u.%u len=%u\n",
			log_pid, ++log_seq, cli->command_len );
	fflush(log);
    }

    for(;;)
    {
	cli->n_bin = 0;
	memset(cli->bin,0,sizeof(cli->bin));

	//--- skip blanks and terminators => find begin of record

	while ( cmd < end && ( (uchar)*cmd <= ' ' || *cmd == sep ))
	    cmd++;
	cli->read_cmd_len = cmd - cli->command; // definitley scanned!

	if ( cmd >= end )
	    break;

	cli->record = (u8*)cmd;


	//--- find end of command+param

	ccp endcmd = memchr(cmd,sep,end-cmd);
	if (!endcmd)
	{
	    if (!cli->is_terminated)
		goto abort;
	    endcmd = end;
	}


	//--- scan and copy command

	ccp param = cmd;
	while ( param < endcmd && ( isalnum((int)*param) || *param == '-' || *param == '_' ) )
	    param++;
	cli->cmd_len = param - cmd;
	if ( cli->cmd_len > sizeof(cli->cmd)-1 )
	     cli->cmd_len = sizeof(cli->cmd)-1;

	if ( log_level > 1 )
	{
	    fprintf(log,">%.*s\n",cli->cmd_len,cmd);
	    fflush(log);
	}

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
		break; // no binary!

	    //-- From now on for binary examintation, only 'end' is relevant,
	    //-- because binary data may contain the separator character.

	    if ( param+3 > end )
	    {
		if ( log_level > 1 )
		{
		    fprintf(log,"bin incomplete\n");
		    fflush(log);
		}

		if (cli->is_terminated)
		{
		    // now we have a conflict:
		    fprintf(stderr,
			"#WARN: Conflict in ExecCommandOfList():"
			" Stream is terminated, but binary not: %s\n",
			cli->user_ts ? cli->user_ts->info : "" );

		    if (log)
		    {
			fprintf(log,"#WARN: Conflict: Stream is terminated, but binary not!\n");
			fflush(log);
		    }
		}
		goto abort; // incomplete
	    }

	    const uint bin_len = be16(++param);
	    param += 2;
	    if ( log_level > 2 )
	    {
		fprintf(log,"bin %u,%u..%u/%u%s\n",
			bin_len,
			(uint)(param - cli->command),
			(uint)(param - cli->command + bin_len),
			cli->command_len,
			param + bin_len > end ? " incomplete" : "" );
		fflush(log);
	    }
	    if ( param + bin_len > end )
		goto abort; // incomplete

	    cli->bin[cli->n_bin].ptr = param;
	    cli->bin[cli->n_bin].len = bin_len;
	    param += bin_len;
	    cli->n_bin++;

	    //-- rescan 'endcmd' after binary block

	    endcmd = memchr(param,sep,end-param);
	    if (!endcmd)
	    {
		if (!cli->is_terminated)
		    goto abort; // incomplete
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

	if ( log_level > 2 && endparam > param )
	{
	    const uint plen = endparam - param, max = 25;
	    if ( plen <= max )
		fprintf(log,"par %.*s\n",plen,param);
	    else
		fprintf(log,"par %.*s…+%u\n",max,param,plen-max);
	    fflush(log);
	}

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

	cli->param		= param;
	cli->param_len		= endparam - param;
	cli->record_len		= endparam - (ccp)cli->record;
	cli->read_cmd_len	= endcmd   - (ccp)cli->command;
	cmd			= endcmd;


	//--- execute command

	if ( log_level > 3 )
	{
	    fprintf(log,"func\n");
	    fflush(log);
	    const int stat = func(cli);
	    fprintf(log,"s=%d\n",stat);
	    fflush(log);
	    if ( stat < 0 )
		break;
	}
	else if ( func && func(cli) < 0 )
	    break;

	cli->cmd_count++;
    }

 abort:;
    if ( log_level > 1 || log && cli->read_cmd_len > cli->command_len )
    {
	fprintf(log,"-- %u,%u/%u%s\n",
		cli->cmd_count,
		cli->read_cmd_len, cli->command_len,
		cli->read_cmd_len > cli->command_len ? " #FAIL!" : "" );
	fflush(log);
    }

    // should never happen
    if ( cli->read_cmd_len > cli->command_len )
	 cli->read_cmd_len = cli->command_len;

    return cli->fail_count ? -1 : cli->cmd_count;
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

	int (*func_cmp)( ccp s1, ccp s2 ) = sf->func_cmp;
	InitializeStringField(sf);
	sf->func_cmp = func_cmp;
    }
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
	ccp *dest = sf->field + sf->used++;
	*dest = move_key ? key : STRDUP(key);
    }
}

///////////////////////////////////////////////////////////////////////////////

void AppendUniqueStringField ( StringField_t * sf, ccp key, bool move_key )
{
    DASSERT(sf);
    if (!key)
	return;

    ccp * src = sf->field;
    ccp * end = src + sf->used;
    while ( src < end )
	if (!strcmp(*src++,key))
	{
	    if (move_key)
		FreeString(key);
	    return;
	}

    AppendStringField(sf,key,move_key);
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
    DASSERT(filename);
    DASSERT(*filename);

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
	char *ptr = iobuf;
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
		mem_t mem = DecodeByModeMem(0,0,pfi->data,-1,emode,0);
		AppendStringField(sf,mem.ptr,true);
	    }
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  ParamField_t			///////////////
///////////////////////////////////////////////////////////////////////////////

#undef PARAM_FIELD_GROW
#define PARAM_FIELD_GROW(s) (s)/4+100

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

	const bool free_data = pf->free_data;
	int (*func_cmp)( ccp s1, ccp s2 ) = pf->func_cmp;
	InitializeParamField(pf);
	pf->free_data = free_data;
	pf->func_cmp = func_cmp;
    }
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
	pf->size += PARAM_FIELD_GROW(pf->size);
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
	( ParamField_t * pf, ccp key, bool move_key, uint num, bool *old_found )
{
    if (!key)
	return 0;

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
	item       = InsertParamFieldHelper(pf,idx);
	item->key  = move_key ? key : STRDUP(key);
	item->num  = num;
	item->data = 0;
    }

    if (old_found)
	*old_found = found;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * IncrementParamField ( ParamField_t * pf, ccp key, bool move_key )
{
    ParamFieldItem_t *item = FindInsertParamField(pf,key,move_key,0,0);
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
	    pf->size += PARAM_FIELD_GROW(pf->size);
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

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetParamFieldStr ( ParamField_t *pf, uint mark, ccp key, ccp def )
{
    DASSERT(pf);
    DASSERT(key);

    ParamFieldItem_t *it = FindParamField(pf,key);
    if ( it && it->data )
    {
	it->num |= mark;
	return (ccp)it->data;
    }
    return def;
}

///////////////////////////////////////////////////////////////////////////////

int GetParamFieldInt ( ParamField_t *pf, uint mark, ccp key, int def )
{
    DASSERT(pf);
    DASSERT(key);

    ParamFieldItem_t *it = FindParamField(pf,key);
    if ( it && it->data )
    {
	it->num |= mark;
	char *end;
	const int num = str2l((ccp)it->data,&end,10);
	if (!*end)
	    return num;
    }
    return def;
}

///////////////////////////////////////////////////////////////////////////////

int GetParamFieldIntMM
	( ParamField_t *pf, uint mark, ccp key, int def, int min, int max )
{
    const int num = GetParamFieldInt(pf,mark,key,def);
    return num >= min && num <= max ? num : def;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  exmem_list_t			///////////////
///////////////////////////////////////////////////////////////////////////////

#undef  EXMEM_LIST_GROW
#define EXMEM_LIST_GROW(s) (s)/2+50

//-----------------------------------------------------------------------------

void ResetEML ( exmem_list_t * eml )
{
    if (eml)
    {
	if ( eml->used > 0 )
	{
	    DASSERT(eml->list);
	    exmem_key_t *ptr = eml->list, *end;
	    for ( end = ptr + eml->used; ptr < end; ptr++ )
	    {
		if (ptr->data.is_key_alloced)
		    FreeString(ptr->key);
		if (ptr->data.is_alloced)
		    FreeString(ptr->data.data.ptr);
	    }
	}
	FREE(eml->list);

	int (*func_cmp)( ccp s1, ccp s2 ) = eml->func_cmp;
	InitializeEML(eml);
	eml->func_cmp = func_cmp;
    }
}

///////////////////////////////////////////////////////////////////////////////

void MoveEML ( exmem_list_t * dest, exmem_list_t * src )
{
    DASSERT(dest);
    if ( src != dest )
    {
	ResetEML(dest);
	if (src)
	{
	    *dest = *src;
	    InitializeEML(src);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

uint FindHelperEML ( const exmem_list_t * eml, ccp key, bool * p_found )
{
    DASSERT(eml);

    int (*cmp)( ccp s1, ccp s2 ) = eml->func_cmp ? eml->func_cmp : strcmp;

    if ( eml->is_unsorted )
    {
	uint i;
	exmem_key_t *ptr = eml->list;
	for ( i = 0; i < eml->used; i++, ptr++ )
	    if (!cmp(key,ptr->key))
	    {
		if (p_found)
		    *p_found = true;
		return i;
	    }

	if (p_found)
	    *p_found = false;
	return eml->used;
    }

    int beg = 0;
    if (key)
    {
	int end = eml->used - 1;
	while ( beg <= end )
	{
	    const uint idx = (beg+end)/2;
	    const int stat = cmp(key,eml->list[idx].key);
	    if ( stat < 0 )
		end = idx - 1 ;
	    else if ( stat > 0 )
		beg = idx + 1;
	    else
	    {
		noTRACE("FindHelperEML(%s) FOUND=%d/%d/%d\n",
			key, idx, eml->used, eml->size );
		if (p_found)
		    *p_found = true;
		return idx;
	    }
	}
    }

    noTRACE("FindHelperEML(%s) failed=%d/%d/%d\n",
		key, beg, eml->used, eml->size );

    if (p_found)
	*p_found = false;
    return beg;
}

///////////////////////////////////////////////////////////////////////////////

int FindIndexEML ( const exmem_list_t * eml, ccp key, int not_found_value )
{
    bool found;
    const int idx = FindHelperEML(eml,key,&found);
    return found ? idx : not_found_value;
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * FindEML ( const exmem_list_t * eml, ccp key )
{
    bool found;
    const int idx = FindHelperEML(eml,key,&found);
    return found ? eml->list + idx : 0;
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * FindAttribEML ( const exmem_list_t * eml, u32 attrib )
{
    DASSERT(eml);
    noTRACE("FindAttribEML(%x)\n",attrib);

    exmem_key_t *ptr = eml->list, *end;
    for ( end = ptr + eml->used; ptr < end; ptr++ )
	if ( ptr->data.attrib == attrib )
	    return ptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static exmem_key_t * InsertHelperEML
	( exmem_list_t * eml, int idx,
			ccp key, CopyMode_t cm_key,
			const exmem_t *data, CopyMode_t cm_data )
{
    DASSERT(eml);
    DASSERT( eml->used <= eml->size );
    noPRINT("+SF: %u/%u/%u\n",idx,eml->used,eml->size);
    if ( eml->used == eml->size )
    {
	eml->size += EXMEM_LIST_GROW(eml->size);
	eml->list = REALLOC(eml->list,eml->size*sizeof(*eml->list));
    }

    DASSERT( idx <= eml->used );
    exmem_key_t * dest = eml->list + idx;
    memmove(dest+1,dest,(eml->used-idx)*sizeof(*dest));
    eml->used++;

    dest->data = CopyExMem(data,cm_data);
    dest->key  = cm_key == CPM_COPY ? STRDUP(key) : key;
    dest->data.is_key_alloced = cm_key != CPM_LINK;

    return dest;
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * FindInsertEML
	( exmem_list_t * eml, ccp key, CopyMode_t cm_key, bool *old_found )
{
    if (!key)
	return 0;

    bool found;
    int idx = FindHelperEML(eml,key,&found);
    exmem_key_t *item;
    if (found)
    {
	if ( cm_key == CPM_MOVE )
	    FreeString(key);
	item = eml->list + idx;
    }
    else
	item = InsertHelperEML(eml,idx,key,cm_key,0,0);

    if (old_found)
	*old_found = found;
    return item;
}

///////////////////////////////////////////////////////////////////////////////

bool InsertEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data )
{
    DASSERT(eml);

    if (!key)
    {
	FreeExMemCM(data,cm_data);
	return 0;
    }

    bool found;
    int idx = FindHelperEML(eml,key,&found);
    if (found)
    {
	if ( cm_key == CPM_MOVE )
	    FreeString(key);
	FreeExMemCM(data,cm_data);
    }
    else
	InsertHelperEML(eml,idx,key,cm_key,data,cm_data);

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

bool ReplaceEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data )
{
    DASSERT(eml);

    if (!key)
    {
	FreeExMemCM(data,cm_data);
	return 0;
    }

    bool found;
    int idx = FindHelperEML(eml,key,&found);
    if (found)
    {
	if ( cm_key == CPM_MOVE )
	    FreeString(key);

	exmem_key_t * dest = eml->list + idx;
	AssignExMem(&dest->data,data,cm_data);
    }
    else
	InsertHelperEML(eml,idx,key,cm_key,data,cm_data);

    return !found;
}

///////////////////////////////////////////////////////////////////////////////

bool OverwriteEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data )
{
    DASSERT(eml);

    if (!key)
    {
	FreeExMemCM(data,cm_data);
	return 0;
    }

    bool found;
    int idx = FindHelperEML(eml,key,&found);
    if (found)
    {
	if ( cm_key == CPM_MOVE )
	    FreeString(key);

	exmem_key_t * dest = eml->list + idx;
	AssignExMem(&dest->data,data,cm_data);
    }

    return found;
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * AppendEML ( exmem_list_t * eml, ccp key, CopyMode_t cm_key,
					const exmem_t *data, CopyMode_t cm_data )
{
    DASSERT(eml);

    if (!key)
    {
	FreeExMemCM(data,cm_data);
	return 0;
    }

    if ( eml->used == eml->size )
    {
	eml->size += EXMEM_LIST_GROW(eml->size);
	eml->list = REALLOC(eml->list,eml->size*sizeof(*eml->list));
    }
    TRACE("AppendEML(%s,%d) %d/%d\n",key,cm_key,eml->used,eml->size);

    exmem_key_t * dest = eml->list + eml->used++;
    dest->data = CopyExMem(data,cm_data);
    dest->key  = cm_key == CPM_COPY ? STRDUP(key) : key;
    dest->data.is_key_alloced = cm_key != CPM_LINK;

    eml->is_unsorted = true;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoveByIndexEML ( exmem_list_t * eml, int index )
{
    if ( index < 0 )
	index += eml->used;
    if ( index >= 0 && index < eml->used )
    {
	DASSERT( eml->used > 0 );
	eml->used--;
	exmem_key_t * dest = eml->list + index;
	if (dest->data.is_key_alloced)
	    FreeString(dest->key);
	FreeExMem(&dest->data);
	if (eml->used)
	    memmove(dest,dest+1,(eml->used-index)*sizeof(*dest));
	else
	    eml->is_unsorted = false;
	return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool RemoveByKeyEML ( exmem_list_t * eml, ccp key )
{
    bool found;
    const uint index = FindHelperEML(eml,key,&found);
 #if 1
    return found && RemoveByIndexEML(eml,index);
 #else // [[obsolete]] 2021-11
    if (found)
    {
	DASSERT(eml->used);
	eml->used--;
	DASSERT( index <= eml->used );
	exmem_key_t * dest = eml->list + index;
	if (dest->data.is_key_alloced)
	    FreeString(dest->key);
	FreeExMem(&dest->data);
	if (eml->used)
	    memmove(dest,dest+1,(eml->used-idx)*sizeof(*dest));
	else
	    eml->is_unsorted = false;
    }
    return found;
 #endif
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * MatchEML ( const exmem_list_t * eml, ccp key )
{
    DASSERT(eml);
    if (key)
    {
	exmem_key_t *ptr = eml->list, *end;
	for ( end = ptr + eml->used; ptr < end; ptr++ )
	    if (MatchPattern(ptr->key,key,'/'))
		return ptr;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void DumpEML ( FILE *f, int indent, const exmem_list_t * eml, ccp info )
{
    if ( f && eml && eml->used )
    {
	indent = NormalizeIndent(indent);
	fprintf(f,"%*s%s%s" "exmem list %p, N=%u/%u%s\n",
		indent,"", info ? info : "", info ? " : " : "",
		eml, eml->used, eml->size, eml->is_unsorted ? ", not sorted" : "" );

	char buf[20];
	const int fw_idx = indent + snprintf(buf,sizeof(buf),"%d",eml->used-1) + 1;

	exmem_key_t *ptr, *end = eml->list + eml->used;
	int fw_key = 0;
	for ( ptr = eml->list; ptr < end; ptr++ )
	{
	    if (ptr->key)
	    {
		const int klen = strlen(ptr->key);
		if ( fw_key < klen )
		     fw_key = klen;
	    }
	}
	if ( fw_key > 30 )
	     fw_key = 30;

	uint idx = 0;
	for (  ptr = eml->list; ptr < end; ptr++, idx++ )
	    fprintf(f,"%*d [%c%c%c%c] %-*s = [%x] %s\n",
		fw_idx, idx,
		ptr->data.is_key_alloced? 'a' : '-',	// flags: same as in PrintExMem()
		ptr->data.is_alloced	? 'A' : '-',
		ptr->data.is_circ_buf	? 'C' : '-',
		ptr->data.is_original	? 'O' : '-',
		fw_key, ptr->key ? ptr->key : "",
		ptr->data.attrib, ptr->data.data.ptr );
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

exmem_t ResolveSymbolsEML ( const exmem_list_t * eml, ccp source )
{
    exmem_t em = {{0}};

    if (source)
    {
	char name[100];

	int try;
	for ( try = 0; try < 50; try++ )
	{
	    ccp p1 = strstr(source,"$(");
	    if (!p1)
		continue;
	    ccp p2 = strchr(p1,')');
	    if (!p2)
		break;

	    const int plen = p2 - p1 - 2;
	    DASSERT(plen>=0);

	    StringCopySM(name,sizeof(name),p1+2,plen);
	    const exmem_key_t *ref = FindEML(eml,name);
	    mem_t replace = ref ? ref->data.data : NullMem;

	    char *temp = MEMDUP3(source, p1-source,
				replace.ptr, replace.len,
				p2+1, strlen(p2+1) );
	    if (em.is_alloced)
		FreeString(source);
	    source = temp;
	    em.is_alloced = true;
	}
    }

    em.data.ptr = source;
    em.data.len = strlen(source);
    em.is_original = !em.is_alloced;
    return em;
}

///////////////////////////////////////////////////////////////////////////////

uint ResolveAllSymbolsEML ( exmem_list_t * eml )
{
    ASSERT(eml);
    int try, count = 0;
    char name[100];
    exmem_key_t *ptr, *end = eml->list + eml->used;

    for ( try = 0; try < 20; try++ )
    {
	int try_again = 0;

	for ( ptr = eml->list; ptr < end; ptr++ )
	{
	    for( int try2 = 0; try2 < 10; try2++ )
	    {
		ccp str = ptr->data.data.ptr;
		if (!str)
		    break;

		ccp p1 = strstr(str,"$(");
		if (!p1)
		    break;
		ccp p2 = strchr(p1,')');
		if (!p2)
		    break;
		const int plen = p2 - p1 - 2;
		ASSERT(plen>=0);

		StringCopySM(name,sizeof(name),p1+2,plen);
		const exmem_key_t *ref = FindEML(eml,name);
		mem_t replace = ref && ref != ptr ? ref->data.data : NullMem;

		char *temp = MEMDUP3( str, p1-str,
				    replace.ptr, replace.len,
				    p2+1, strlen(p2+1) );
		if (ptr->data.is_alloced)
		    FreeString(str);
		ASSERT(temp);
		ptr->data.data.ptr = temp;
		ptr->data.data.len = strlen(temp);
		ptr->data.is_alloced = true;
		ptr->data.is_original = ptr->data.is_circ_buf = false;
		try_again++;
		count++;
	    }
	}

	if (!try_again)
	    break;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

// add symbols 'home', 'etc', 'install' = 'prog' and 'cwd'
void AddStandardSymbolsEML ( exmem_list_t * eml, bool overwrite )
{
    DASSERT(eml);

    char buf[PATH_MAX];
    bool (*func)(exmem_list_t*,ccp,CopyMode_t,const exmem_t*,CopyMode_t)
	= overwrite ? ReplaceEML : InsertEML;
    PRINT0("overwrite=%d, func=%p, ins=%p, repl=%p\n",overwrite,func,InsertEML,ReplaceEML);

    exmem_t temp = ExMemByString("/etc");
    func(eml,"etc",CPM_LINK,&temp,CPM_LINK);

    ccp home = getenv("HOME");
    temp = ExMemByString0(home);
    func(eml,"home",CPM_LINK,&temp,CPM_COPY);

    // https://wiki.archlinux.org/index.php/XDG_Base_Directory
    ccp xdg = getenv("XDG_CONFIG_HOME");
    if ( xdg && *xdg )
    {
	temp = ExMemByString0(xdg);
	ccp colon = strchr(xdg,':');
	if (colon)
	{
	    if ( colon == xdg )
		goto xdg_home;
	    temp.data.len = colon - xdg;
	}
	func(eml,"xdg_home",CPM_LINK,&temp,CPM_COPY);
    }
    else if (home)
    {
     xdg_home:;
	xdg = PathCatPP(buf,sizeof(buf),home,".config");
	temp = ExMemByString0(xdg);
	func(eml,"xdg_home",CPM_LINK,&temp,CPM_COPY);
    }

    xdg = getenv("XDG_CONFIG_DIRS");
    if ( xdg && *xdg )
    {
	temp = ExMemByString0(xdg);
	ccp colon = strchr(xdg,':');
	if (colon)
	{
	    if ( colon == xdg )
		goto xdg_etc;
	    temp.data.len = colon - xdg;
	}
	func(eml,"xdg_etc",CPM_LINK,&temp,CPM_COPY);
    }
    else
    {
     xdg_etc:;
	temp = ExMemByString("/etc/xdg");
	func(eml,"xdg_etc",CPM_LINK,&temp,CPM_LINK);
    }

    ccp install = ProgramDirectory();
    temp = ExMemByString0(install);
    func(eml,"install",CPM_LINK,&temp,CPM_COPY);
    func(eml,"prog",CPM_LINK,&temp,CPM_COPY);

    if (getcwd(buf,sizeof(buf)))
    {
	temp = ExMemByString0(buf);
	func(eml,"cwd",CPM_LINK,&temp,CPM_COPY);
    }

 #ifdef __CYGWIN__
    ccp prog_files = getenv("PROGRAMFILES");
    if ( !prog_files || !*prog_files )
	prog_files = "C:/Program Files";
    temp = GetNormalizeFilenameCygwin(prog_files,true);
    func(eml,"programfiles",CPM_LINK,&temp,CPM_COPY);
    FreeExMem(&temp);
 #endif
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MultiColumn			///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetMultiColumn ( MultiColumn_t *mc )
{
    if (mc)
    {
	ResetEML(&mc->list);
	memset(mc,0,sizeof(*mc));
    }
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * PutLineMC
	( MultiColumn_t *mc, uint opt, ccp key, ccp value, CopyMode_t cm_value )
{
    DASSERT(mc);
    DASSERT(key);
    DASSERT(value);

    exmem_t data = ExMemByString(value);
    exmem_key_t *res = AppendEML( &mc->list, key, CPM_LINK, &data, cm_value );
    res->data.attrib = opt;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

exmem_key_t * PrintLineMC
	( MultiColumn_t *mc, uint opt, ccp key, ccp format, ... )
{
    char buf[1000];
    va_list arg;
    va_start(arg,format);
    vsnprintf(buf,sizeof(buf),format,arg);
    va_end(arg);
    return PutLineMC(mc,opt,key,buf,CPM_COPY);
}

///////////////////////////////////////////////////////////////////////////////
// [[mc_len_t]]

typedef struct mc_len_t
{
    int key;	// key length
    int val;	// val length
}
mc_len_t;

//-----------------------------------------------------------------------------
// [[mc_param_t]]

#define MULTI_COLUMN_MAX_COLS 100

typedef struct mc_param_t
{
    FILE	*f;		// output file
    const ColorSet_t
		*colset;	// color set for the frame
    int		debug;		// debug level: 0..2

    int		frame_mode;	// 0:ASCII, 1:single line, 2:double line
    ccp		prefix;		// line prefix
    int		prefix_fw;	// field width of 'prefix'
    int		indent;		// indention excluding prefix
    ccp		sep;		// separator for frame_mode 0 and 1
    int		sep_fw;		// field width of 'sep'
    ccp		eol;		// end-of-line string
    int		fw;		// total field width

    ccp		head_key;	// header string for key column
    int		head_key_fw;	// field width of 'head_key'
    ccp		head_val;	// header string for value column
    int		head_val_fw;	// field width of 'head_val'

    int		row_size;	// additional size for a row
    int		col_size;	// additional size for a column

    int		n_rows;		// number of rows
    int		n_cols;		// number of columns

    mc_len_t	*len_list;	// list of estimatest lengths
    mc_len_t	fw_col[MULTI_COLUMN_MAX_COLS];
				// field width for each column
}
mc_param_t;

//-----------------------------------------------------------------------------

static void mc_print_sep ( const mc_param_t *p )
{
    fprintf(p->f,"%s%*s",p->prefix,p->indent,"");

    const mc_len_t *fw = p->fw_col;
    for ( int col = 0; col < p->n_cols; col++, fw++ )
    {
	if (col)
	    fputs("-+-",p->f);
	fprintf(p->f,"%.*s",fw->key+fw->val+p->sep_fw,Minus300);
    }
    fputs(p->eol,p->f);
}

//-----------------------------------------------------------------------------

static void mc_print_line1 ( const mc_param_t *p, ccp template )
{
    fprintf(p->f,"%s%*s%s%.3s",
		p->prefix, p->indent,"",
		p->colset->heading, template );

    const mc_len_t *fw = p->fw_col;
    for ( int col = 0; col < p->n_cols; col++, fw++ )
    {
	if (col)
	    fprintf(p->f,"%.3s",template+3);
	fprintf(p->f,"%.*s", 3*(fw->key+fw->val+p->sep_fw+2), ThinLine300_3 );
    }

    fprintf(p->f,"%.3s%s%s",template+6,p->colset->reset,p->eol);
}

//-----------------------------------------------------------------------------

static void mc_print_line2 ( const mc_param_t *p, ccp template )
{
    fprintf(p->f,"%s%*s%s%.3s",
		p->prefix, p->indent,"",
		p->colset->heading, template );

    const mc_len_t *fw = p->fw_col;
    for ( int col = 0; col < p->n_cols; col++, fw++ )
    {
	if (col)
	    fprintf(p->f,"%.3s",template+6);
	fprintf(p->f,"%.*s%.3s%.*s",
		3*(fw->key+2), DoubleLine300_3,
		template+3,
		3*(fw->val+2), DoubleLine300_3 );
    }

    fprintf(p->f,"%.3s%s%s",template+9,p->colset->reset,p->eol);
}

//-----------------------------------------------------------------------------

int PrintMultiColumn ( const MultiColumn_t *mc )
{
    const u_usec_t start_time = GetTimerUSec();
    if ( !mc || !mc->list.used )
	return -1;


    //--- setup

    mc_param_t p = {0};
    p.f		= mc->f ? mc->f : stdout;
    p.colset	= mc->colset ? mc->colset : GetColorSet0();
    p.debug	= mc->debug;
    p.indent	= NormalizeIndent(mc->indent);

    p.prefix = mc->prefix;
    if (p.prefix)
	p.prefix_fw = ScanEUTF8Length(p.prefix);
    else
    {
	p.prefix = EmptyString;
	p.prefix_fw = 0;
    }

    p.sep	= mc->sep ? mc->sep : " ";
    p.sep_fw	= ScanEUTF8Length(p.sep);
    p.eol	= mc->eol ? mc->eol : "\n";
    p.fw	= !mc->fw ? 79 : mc->fw > 10 ? mc->fw : 10;

    if ( mc->print_header )
    {
	p.head_key = mc->head_key ? mc->head_key : "Name";
	p.head_key_fw = strlene8(p.head_key);

	p.head_val = mc->head_val ? mc->head_val : "Value";
	p.head_val_fw = strlene8(p.head_val);
    }
    else
    {
	p.head_key = p.head_val = EmptyString;
	// p.head_key_fw = p.head_val_fw = 0; // initial value
    }

    switch (mc->frame_mode)
    {
     case 0:
	p.frame_mode	=  0;
	p.row_size	= -3;
	p.col_size	=  3 + p.sep_fw;
	break;

     case 1:
	p.frame_mode	= 1;
	p.row_size	= 1;
	p.col_size	= 3 + p.sep_fw;
	break;

     default:
	p.frame_mode	= 2;
	p.row_size	= 1;
	p.col_size	= 6;
	break;
    }

    if ( p.debug > 1 )
	fprintf(p.f,
		"#> debug=%d, pre=%s, indent=%d, eol=%s, fw=%d,"
		" print[Fhf]=%d%d%d, min_rows=%d, max_cols=%d%s",
		p.debug, QuoteStringS(p.prefix,0,CHMD__MODERN),
		p.indent, QuoteStringS(p.eol,0,CHMD__MODERN), p.fw,
		p.frame_mode, mc->print_header, mc->print_footer,
		mc->min_rows, mc->max_cols, p.eol );


    //--- find min width

    p.len_list = MALLOC( mc->list.used * sizeof(*p.len_list) );
    mc_len_t *len_ptr = p.len_list;

    int min_fw = INT_MAX;
    const exmem_key_t *list_end = mc->list.list + mc->list.used;
    for ( const exmem_key_t *ptr = mc->list.list; ptr < list_end; ptr++ )
    {
	len_ptr->key = strlene8(ptr->key);
	len_ptr->val = strlene8(ptr->data.data.ptr);
	const int len = len_ptr->key + len_ptr->val;
	if ( min_fw > len )
	     min_fw = len;
	len_ptr++;
    }
    min_fw += p.col_size;

    const int avail_chars = p.fw - p.prefix_fw - p.indent - p.row_size;
    int max_cols = ( avail_chars + p.col_size - 1 ) / min_fw;
    if ( max_cols > MULTI_COLUMN_MAX_COLS )
	 max_cols = MULTI_COLUMN_MAX_COLS;

    const int min_rows = mc->min_rows > 1 ? mc->min_rows : 1;
    const int temp = ( mc->list.used + min_rows - 1 ) / min_rows + 1;
    if ( temp > 0 && max_cols > temp )
	max_cols = temp;

    if ( p.debug > 1 )
	fprintf(p.f,
		"#> src-lines=%d, min-fw=%d, min-rows=%d, max-cols=%d/%d%s",
		mc->list.used, min_fw,
		min_rows, max_cols, MULTI_COLUMN_MAX_COLS, p.eol );


    //--- evaluate number of columns

    for ( p.n_cols = max_cols; p.n_cols > 0; p.n_cols-- )
    {
	memset(p.fw_col,0,sizeof(p.fw_col));
	mc_len_t len = { p.head_key_fw, p.head_val_fw };

	p.n_rows = ( mc->list.used - 1 ) / p.n_cols + 1;
	p.n_cols = ( mc->list.used - 1 ) / p.n_rows + 1;
	if ( p.debug > 1 )
	    fprintf(p.f, "#> … try %2u columns and %3u rows, %3u cells total%s",
			p.n_cols, p.n_rows, p.n_cols*p.n_rows, p.eol );

	int idx, row, col, total_fw = 0;
	len_ptr = p.len_list;
	for ( idx = row = col = 0; idx < mc->list.used; idx++, len_ptr++ )
	{
	    if ( len.key < len_ptr->key )
		 len.key = len_ptr->key;
	    if ( len.val < len_ptr->val )
		 len.val = len_ptr->val;

	    if ( ++row >= p.n_rows )
	    {
		total_fw += len.key + len.val + p.col_size;
		if ( total_fw > avail_chars )
		    break;
		p.fw_col[col] = len;

		len.key = p.head_key_fw;
		len.val = p.head_val_fw;
		row = 0;
		col++;
	    }
	}
	if ( col < p.n_cols )
	{
	    p.fw_col[col] = len;
	    total_fw += len.key + len.val + p.col_size;
	}
	if ( total_fw <= avail_chars )
	    break;
    }

    if ( p.n_cols < 1 )
    {
	p.n_rows = mc->list.used;
	p.n_cols = 1;
    }

    if ( p.debug > 1 )
    {
	fprintf(p.f,"#> cols=%d, rows=%d, fw", p.n_cols, p.n_rows );
	int total = 0;
	const mc_len_t *fw = p.fw_col;
	for ( int col = 0; col < p.n_cols; col++, fw++ )
	{
	    fprintf( p.f, "%c%u+%u", col ? ',' : '=', fw->key, fw->val );
	    total += fw->key + fw->val + p.col_size;
	}
	fprintf(p.f," = %d/%d%s",total,avail_chars,p.eol);
    }

    //--- print columns

    const u_usec_t output_time = GetTimerUSec();

    uint out_lines = p.n_rows;
    //----------------------
    if ( p.frame_mode >= 2 )
    //----------------------
    {
	mc_print_line2(&p,"╔╤╦╗");
	out_lines++;

	if (mc->print_header)
	{
	    fprintf(p.f,"%s%*s%s║%s",p.prefix,p.indent,"",p.colset->heading,p.colset->reset);
	    const mc_len_t *fw = p.fw_col;
	    for ( int col = 0; col < p.n_cols; col++, fw++ )
		 fprintf(p.f," %s %s│%s %s %s║%s",
				AlignEUTF8ToCircBuf(p.head_key,-fw->key,fw->key),
				p.colset->heading, p.colset->reset,
				AlignEUTF8ToCircBuf(p.head_val,-fw->val,fw->val),
				p.colset->heading, p.colset->reset );
	    fputs(p.eol,p.f);

	    mc_print_line2(&p,"╠╪╬╣");
	    out_lines += 2;
	}

	for ( int row = 0; row < p.n_rows; row++ )
	{
	    fprintf(p.f,"%s%*s%s║%s",p.prefix,p.indent,"",p.colset->heading,p.colset->reset);
	    const exmem_key_t *ptr = mc->list.list + row;
	    const mc_len_t *fw = p.fw_col;
	    for ( int col = 0; col < p.n_cols; col++, ptr += p.n_rows, fw++ )
	    {
		if ( ptr < list_end )
		{
		    fprintf(p.f," %s %s│%s ",
				AlignEUTF8ToCircBuf(ptr->key,-fw->key,fw->key),
				p.colset->heading, p.colset->reset );

		    if ( ptr->data.attrib > 1 )
		    {
			const int fw_attrib = ptr->data.attrib < fw->val ? ptr->data.attrib : fw->val;
			const int fw_val = p.len_list[ptr-mc->list.list].val;
			if ( fw_attrib <= fw_val )
			    goto std2;
			fprintf(p.f,"%s%*s %s║%s",
				AlignEUTF8ToCircBuf(ptr->data.data.ptr,fw_attrib,fw_attrib),
				fw_attrib - fw->val, "",
				p.colset->heading, p.colset->reset );
		    }
		    else
		    {
		     std2:;
			const int fw_val = ptr->data.attrib ? -fw->val : fw->val;
			fprintf(p.f,"%s %s║%s",
				AlignEUTF8ToCircBuf(ptr->data.data.ptr,fw_val,fw->val),
				p.colset->heading, p.colset->reset );
		    }
		}
		else
		    fprintf(p.f," %*s %s│%s %*s %s║%s",
				fw->key, "", p.colset->heading, p.colset->reset,
				fw->val, "", p.colset->heading, p.colset->reset );
	    }
	    fputs(p.eol,p.f);
	}

	mc_print_line2(&p,"╚╧╩╝");
	out_lines++;
    }
    //---------------------------
    else if ( p.frame_mode >= 1 )
    //---------------------------
    {
	mc_print_line1(&p,"┌┬┐");
	out_lines++;

	if (mc->print_header)
	{
	    fprintf(p.f,"%s%*s%s│%s",p.prefix,p.indent,"",p.colset->heading,p.colset->reset);
	    const mc_len_t *fw = p.fw_col;
	    for ( int col = 0; col < p.n_cols; col++, fw++ )
		 fprintf(p.f," %s%s%s %s│%s",
				AlignEUTF8ToCircBuf(p.head_key,-fw->key,fw->key),
				p.sep,
				AlignEUTF8ToCircBuf(p.head_val,-fw->val,fw->val),
				p.colset->heading, p.colset->reset );

	    fputs(p.eol,p.f);

	    mc_print_line1(&p,"├┼┤");
	    out_lines += 2;
	}

	for ( int row = 0; row < p.n_rows; row++ )
	{
	    fprintf(p.f,"%s%*s%s│%s",p.prefix,p.indent,"",p.colset->heading,p.colset->reset);
	    const exmem_key_t *ptr = mc->list.list + row;
	    const mc_len_t *fw = p.fw_col;
	    for ( int col = 0; col < p.n_cols; col++, ptr += p.n_rows, fw++ )
	    {
		if ( ptr < list_end )
		{
		    fprintf(p.f," %s%s",
				AlignEUTF8ToCircBuf(ptr->key,-fw->key,fw->key),
				p.sep );

		    if ( ptr->data.attrib > 1 )
		    {
			const int fw_attrib = ptr->data.attrib < fw->val ? ptr->data.attrib : fw->val;
			const int fw_val = p.len_list[ptr-mc->list.list].val;
			if ( fw_attrib <= fw_val )
			    goto std1;
			fprintf(p.f,"%s%*s %s│%s",
				AlignEUTF8ToCircBuf(ptr->data.data.ptr,fw_attrib,fw_attrib),
				fw_attrib - fw->val, "",
				p.colset->heading, p.colset->reset );
		    }
		    else
		    {
		     std1:;
			const int fw_val = ptr->data.attrib ? -fw->val : fw->val;
			fprintf(p.f,"%s %s│%s",
				AlignEUTF8ToCircBuf(ptr->data.data.ptr,fw_val,fw->val),
				p.colset->heading, p.colset->reset );
		    }
		}
		else
		    fprintf(p.f," %*s %s│%s",
				fw->key + fw->val + p.sep_fw, "",
				p.colset->heading, p.colset->reset );
	    }
	    fputs(p.eol,p.f);
	}

	mc_print_line1(&p,"└┴┘");
	out_lines++;
    }
    //-------------------
    else // !p.frame_mode
    //-------------------
    {
	if (mc->print_header)
	{
	    mc_print_sep(&p);

	    fprintf(p.f,"%s%*s",p.prefix,p.indent,"");
	    const mc_len_t *fw = p.fw_col;
	    for ( int col = 0; col < p.n_cols; col++, fw++ )
	    {
		if (col)
		    fputs(" | ",p.f);
		fprintf(p.f,"%s%s%s",
				AlignEUTF8ToCircBuf(p.head_key,-fw->key,fw->key),
				p.sep,
				AlignEUTF8ToCircBuf(p.head_val,-fw->val,fw->val) );
	    }
	    fputs(p.eol,p.f);
	    out_lines += 2;
	}

	if ( mc->print_header || mc->print_footer )
	{
	    mc_print_sep(&p);
	    out_lines++;
	}

	for ( int row = 0; row < p.n_rows; row++ )
	{
	    fprintf(p.f,"%s%*s",p.prefix,p.indent,"");
	    const exmem_key_t *ptr = mc->list.list + row;
	    const mc_len_t *fw = p.fw_col;
	    for ( int col = 0; col < p.n_cols; col++, ptr += p.n_rows, fw++ )
	    {
		if (col)
		    fputs(" | ",p.f);
		if ( ptr < list_end )
		{
		    fprintf(p.f,"%s%s",
				AlignEUTF8ToCircBuf(ptr->key,-fw->key,fw->key),
				p.sep );


		    if ( ptr->data.attrib > 1 )
		    {
			const int fw_attrib = ptr->data.attrib < fw->val ? ptr->data.attrib : fw->val;
			const int fw_val = p.len_list[ptr-mc->list.list].val;
			if ( fw_attrib <= fw_val )
			    goto std0;
			fprintf(p.f,"%s%*s",
				AlignEUTF8ToCircBuf(ptr->data.data.ptr,fw_attrib,fw_attrib),
				fw_attrib - fw->val, "" );
		    }
		    else
		    {
		     std0:;
			const int fw_val = ptr->data.attrib ? -fw->val : fw->val;
			fputs(AlignEUTF8ToCircBuf(ptr->data.data.ptr,fw_val,fw->val),p.f);
		    }
		}
	    }
	    fputs(p.eol,p.f);
	}

	if (mc->print_footer)
	{
	    mc_print_sep(&p);
	    out_lines++;
	}
    }
    //---------------------
    // end-if !p.frame_mode
    //---------------------


    //--- terminate

    if ( p.debug > 0 )
    {
	const u_usec_t now = GetTimerUSec();
	fprintf(p.f,"#> setup in %llu us, %u ouput lines in %llu us, total %llu us%s",
		output_time - start_time,
		out_lines, now - output_time,
		now - start_time,
		p.eol );
    }

    FREE(p.len_list);
    return out_lines;
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
	PrintNumberU6(0,0,gb->ptr-gb->buf,DC_SFORM_ALIGN),
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

ccp GetIpModeName ( IpMode_t ipm )
{
    switch (ipm)
    {
	case IPV4_ONLY:  return "IPv4 only";
	case IPV6_ONLY:  return "IPv6 only";
	case IPV4_FIRST: return "IPv4 first";
	case IPV6_FIRST: return "IPv6 first";
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ipv4_t GetIP4Mask ( int bits )
{
    return bits > 0 && bits < 32
	? htonl( 0xffffffffu << (32-bits) )
	: bits ? 0xffffffffu : 0;
}

///////////////////////////////////////////////////////////////////////////////

ipv6_t GetIP6Mask ( int bits )
{
    ipv6_t ip6;
    memset(&ip6,~0,sizeof(ip6));
    if ( bits >= 0 && bits < 128 )
    {
	int idx = bits/32;
	bits -= idx*32;
	const u32 mask = htonl( bits ? 0xffffffffu << (32-bits) : 0 );
	ip6.v32[idx++] &= mask;
	while ( idx < 4 )
	    ip6.v32[idx++] = 0;
    }
    return ip6;
}

///////////////////////////////////////////////////////////////////////////////

int GetBitsByMask4 ( ipv4_t ip4_nbo )
{
    const int res = FindHighest0BitBE(&ip4_nbo,4);
    return 31-res;
}

///////////////////////////////////////////////////////////////////////////////

int GetBitsByMask6 ( ipv6_t ip6_nbo )
{
    const int res = FindHighest0BitBE(&ip6_nbo,16);
    return 127-res;
}

///////////////////////////////////////////////////////////////////////////////

int GetBitsBySA ( sockaddr_t *sa, int return_on_error )
{
    if (sa)
    {
	if ( sa->sa_family == AF_INET )
	{
	    sockaddr_in4_t *sa4 = (sockaddr_in4_t*)sa;
	    return GetBitsByMask4(sa4->sin_addr.s_addr);
	}

	if ( sa->sa_family == AF_INET6 )
	{
	    sockaddr_in6_t *sa6 = (sockaddr_in6_t*)sa;
	    return GetBitsByMask6(*(ipv6_t*)&sa6->sin6_addr);
	}
    }
    return return_on_error;
}

///////////////////////////////////////////////////////////////////////////////

uint CreateUniqueIdN ( int range )
{
    static uint unique_id = 1;

    if ( range <= 0 )
	return unique_id;

    uint ret = unique_id;
    unique_id += range;
    if ( unique_id < ret ) // overflow
    {
	ret = 1;
	unique_id = ret + range;
    }
    return ret;
}

///////////////////////////////////////////////////////////////////////////////

void Sha1Hex2Bin ( sha1_hash_t bin, ccp src, ccp end )
{
    DASSERT(bin);
    DASSERT(src);

    uint i;
    for ( i = 0; i < 20; i++ )
	bin[i] = ScanDigits(&src,end,16,2,0);
}

//-----------------------------------------------------------------------------

void Sha1Bin2Hex ( sha1_hex_t hex, cvp bin )
{
    DASSERT(hex);
    DASSERT(bin);

    snprintf(hex,sizeof(sha1_hex_t),"%08x%08x%08x%08x%08x",
	be32(bin), be32(bin+4), be32(bin+8), be32(bin+12), be32(bin+16) );
}

//-----------------------------------------------------------------------------

ccp GetSha1Hex ( cvp bin )
{
    DASSERT(bin);

    char *hex = GetCircBuf(sizeof(sha1_hex_t));
    Sha1Bin2Hex(hex,bin);
    return hex;
}

///////////////////////////////////////////////////////////////////////////////

void * dc_memrchr ( cvp src, int ch, size_t size )
{
    if ( !src || !size )
	return 0;

    ccp ptr = src + size;
    while ( ptr > (ccp)src )
	if ( *--ptr == ch )
	    return (void*)ptr;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

bool IsMemConst ( cvp mem, uint size, u8 val )
{
    if (!size)
	return true;

    const u8 *src = (u8*)mem;
    return *src == val && !memcmp(src,src+1,size-1);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

