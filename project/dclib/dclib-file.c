
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <utime.h>
#include <errno.h>
#include <dirent.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/resource.h>

#include "dclib-basics.h"
#include "dclib-file.h"
#include "dclib-debug.h"
#include "dclib-network.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			termios support			///////////////
///////////////////////////////////////////////////////////////////////////////

static struct termios termios_data;
int termios_valid = 0;

void ResetTermios()
{
    if ( termios_valid > 0 )
	tcsetattr(0,TCSANOW,&termios_data);
}

static bool LoadTermios()
{
    if (!termios_valid)
    {
	termios_valid = -1;
	if ( isatty(0) && !tcgetattr(0,&termios_data) )
	    termios_valid = 1;
    }
    return termios_valid > 0;
}

bool EnableSingleCharInput()
{
    if (!LoadTermios())
	return false;

    struct termios tios;
    memcpy(&tios,&termios_data,sizeof(tios));
    tios.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(0,TCSANOW,&tios);
    return true;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    log helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

LogFile_t GlobalLogFile = {0};

///////////////////////////////////////////////////////////////////////////////

FILE * TryOpenFile ( FILE *f, ccp fname, ccp mode )
{
    if ( f == TRY_OPEN_FILE )
	f = fopen(fname,mode);
    else if (f)
    {
	rewind(f);
	fflush(f);
    }
    return f;
}

///////////////////////////////////////////////////////////////////////////////

int GetLogTimestamp ( char *buf, uint buf_size, TimestampMode_t ts_mode )
{
    DASSERT(buf);
    DASSERT(buf_size>2);

    uint len;
    if ( ts_mode >= TSM_USEC )
    {
	const u64 usec = GetTimeUSec(false);
	const uint sec = usec/1000000 % SEC_PER_DAY;
	len = snprintf(buf,buf_size,
		"%02u:%02u:%02u.%06llu ",
		sec / 3600,
		sec / 60 % 60,
		sec % 60,
		usec % 1000000 );
    }
    else if ( ts_mode >= TSM_MSEC )
    {
	const u64 msec = GetTimeMSec(false);
	const uint sec = msec/USEC_PER_MSEC % SEC_PER_DAY;
	len = snprintf(buf,buf_size,
		"%02u:%02u:%02u.%03llu ",
		sec / 3600,
		sec / 60 % 60,
		sec % 60,
		msec % 1000 );
    }
    else if ( ts_mode >= TSM_SEC )
    {
	const uint sec = GetTimeSec(false) % SEC_PER_DAY;
	len = snprintf(buf,buf_size,
		"%02u:%02u:%02u ",
		sec / 3600,
		sec / 60 % 60,
		sec % 60 );
    }
    else
    {
	*buf = 0;
	len = 0;
    }

    if ( len >= buf_size )
    {
	len = buf_size - 1;
	buf[len] = 0;
    }
    return len;
}

///////////////////////////////////////////////////////////////////////////////

int PrintLogTimestamp ( LogFile_t *lf )
{
    DASSERT(lf);
    DASSERT(lf->log);

    int stat = 0;
    if ( lf && lf->log )
    {
	char buf[50];
	stat = GetLogTimestamp(buf,sizeof(buf),lf->ts_mode);
	if ( stat > 0 )
	    fwrite(buf,stat,1,lf->log);
    }
    return stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int PutLogFile ( LogFile_t *lf, ccp text, int text_len )
{
    if (!lf)
	lf = &GlobalLogFile;

    if ( !lf->log || !text )
	return 0;

    //--- collect data first to have a single print

    char buf[2000];
    char *dest = buf + GetLogTimestamp( buf, sizeof(buf), lf->ts_mode );
    dest = StringCopyEMem( dest, buf+sizeof(buf), lf->tag );
    dest = StringCopyEM( dest, buf+sizeof(buf), text,
				text_len < 0 ? strlen(text) : text_len );

    const int len = dest - buf;
    fwrite(buf,len,1,lf->log);
    if (lf->flush)
	fflush(lf->log);
    if ( lf->log == stdout )
	stdout_seq_count++;
    return len;
}

///////////////////////////////////////////////////////////////////////////////

int PrintArgLogFile ( LogFile_t *lf, ccp format, va_list arg )
{
    int stat = 0;
    if (format)
    {
	char buf[2000];
	int len = vsnprintf(buf,sizeof(buf),format,arg);
	if ( len >= 0 )
	{
	    if ( len >= sizeof(buf)-1 )
	    {
		len = sizeof(buf)-1;
		buf[sizeof(buf)-1] = 0;
	    }
	    stat = PutLogFile(lf,buf,len);
	}
    }
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int PrintLogFile ( LogFile_t *lf, ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    const int stat = PrintArgLogFile(lf,format,arg);
    va_end(arg);
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			search tool			///////////////
///////////////////////////////////////////////////////////////////////////////

exmem_t SearchToolByPATH ( ccp tool )
{
    exmem_t dest = {};
    if ( tool && *tool )
    {
	char fname[PATH_MAX];
	if (strchr(tool,'/'))
	{
//PRINT1("SEARCH ABS: %s\n",tool);
	    struct stat st;
	    if ( !stat(tool,&st) && st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH) )
	    {
//PRINT1("FOUND ABS: %s\n",tool);
		AssignExMemS(&dest,tool,-1,CPM_COPY);
	    }
	}
	else
	{
	 #ifdef __CYGWINxxx__
	    if ( ProgInfo.progpath && *ProgInfo.progpath )
	    {
		PathCatPP(fname,sizeof(fname),ProgInfo.progpath,tool);
//PRINT1("SEARCH+: %s\n",fname);
		struct stat st;
		if ( !stat(fname,&st) && st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH) )
		{
//PRINT1("FOUND+: %s\n",fname);
		    AssignExMemS(&dest,fname,-1,CPM_COPY);
		    return dest;
		}
	    }
	 #endif

	    ccp path = getenv("PATH");
	    for(;;)
	    {
		while ( *path == ':' )
		    path++;
		ccp start = path;
		while ( *path && *path != ':' )
		    path++;
		const int len = path - start;
		if (!len)
		    break;

		const int flen = snprintf(fname,sizeof(fname),"%.*s/%s",len,start,tool);
//PRINT1("SEARCH: %s\n",fname);
		struct stat st;
		if ( !stat(fname,&st) && st.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH) )
		{
//PRINT1("FOUND: %s\n",fname);
		    AssignExMemS(&dest,fname,flen,CPM_COPY);
		    break;
		}
	    }
	}
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

exmem_t SearchToolByList ( ccp * list, int max )
{
    exmem_t dest = {};

    if (list)
    {
	if ( max < 0 )
	{
	    max = 0;
	    for ( ccp *ptr = list; *ptr; ptr++ )
		max++;
	}

	while ( max-- > 0 )
	{
	    ccp arg = *list++;
	    if (arg)
	    {
//PRINT1("SEARCH: %s\n",arg);
		if ( *arg == '>' )
		{
		    ccp env = getenv(arg+1);
		    if ( env && *env )
		    {
			AssignExMemS(&dest,env,-1,CPM_LINK);
			break;
		    }
		}
		else
		{
		    dest = SearchToolByPATH(arg);
		    if (dest.data.len)
		    {
			//AssignExMemS(&dest,arg,-1,CPM_LINK);
			break;
		    }
		}
	    }
	}
    }
    return dest;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			pipe to pager			///////////////
///////////////////////////////////////////////////////////////////////////////

static FILE *pager_file = 0;

///////////////////////////////////////////////////////////////////////////////

FILE * OpenPipeToPager()
{
    static bool done = false;
    if (!done)
    {
	done = true;

	// first: set less option -R to enable colors
	ccp less = getenv("LESS");
	if ( less && *less )
	{
	    char buf[200];
	    snprintf(buf,sizeof(buf),"%s -R",less);
	    setenv("LESS",buf,1);
	}
	else
	    setenv("LESS","-R",1);

	static ccp search_tab[] =
	{
	    ">DC_PAGER",
	    ">DC_PAGER1",
	    ">DC_PAGER2",
	    ">PAGER",
	 #ifdef __CYGWINxxx__
	    "/usr/bin/less",
	    "/cygdrive/c/Program Files/Wiimm/SZS/less",
	    "/usr/bin/more",
	    "/cygdrive/c/Program Files/Wiimm/SZS/more",
	 #endif
	    "less",
	    "more",
	    0
	};

	exmem_t pager = SearchToolByList(search_tab,-1);
	if (pager.data.ptr)
	{
	 #ifdef __CYGWINxxx__
PRINT1("popen(%s)\n",pager.data.ptr);
	    char buf[500];
	    StringCat3S(buf,sizeof(buf),"'",pager.data.ptr,"'");
PRINT1("popen(%s)\n",buf);
	    pager_file = popen(buf,"we");
PRINT_IF1(!pager_file,"popen(%s) FAILED\n",buf);
	 #else
	    pager_file = popen(pager.data.ptr,"we");
	 #endif
	}
	FreeExMem(&pager);
    }

    return pager_file;
}

///////////////////////////////////////////////////////////////////////////////

void ClosePagerFile()
{
    if (pager_file)
    {
	if ( stdout == pager_file ) stdout = 0;
	if ( stderr == pager_file ) stderr = 0;
	if ( stdwrn == pager_file ) stdwrn = 0;
	if ( stdlog == pager_file ) stdlog = 0;
	pclose(pager_file);
	pager_file = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

bool StdoutToPager()
{
    FILE *f = OpenPipeToPager();
    if (f)
    {
	fflush(stdout);
	stdout = f;

	if ( stderr && isatty(fileno(stderr)) )
	    stderr = f;

	if ( stdwrn && isatty(fileno(stdwrn)) )
	    stdwrn = f;

	if ( stdlog && isatty(fileno(stdlog)) )
	    stdlog = f;

	return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

void CloseStdoutToPager()
{
    if ( pager_file && pager_file == stdout )
	ClosePagerFile();
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum FileMode_t			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetFileModeStatus
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    FileMode_t		file_mode,	// filemode to print
    uint		print_mode	// 0: short vector: 1 char for 1 attrib
					// 1: long mode: keyword for each set attrib
)
{
    struct info_t
    {
	FileMode_t	mode;
	char		ch;
	ccp		name;
    };

    static const struct info_t info[] =
    {
	{ FM_TEST,	't', "test" },
	{ FM_SILENT,	's', "silent" },
	{ FM_IGNORE,	'i', "ignore" },
	{ FM_MODIFY,	'M', "modify" },
	{ FM_APPEND,	'A', "append" },
	{ FM_UPDATE,	'U', "update" },
	{ FM_OVERWRITE,	'O', "overwrite" },
	{ FM_NUMBER,	'N', "number" },
	{ FM_REMOVE,	'R', "remove" },
	{ FM_MKDIR,	'M', "mkdir" },
	{ FM_STDIO,	'S', "stdio" },
	{ FM_TCP,	'I', "tcp" },
	{ FM_DEV,	'D', "dev" },
	{ FM_SPC,	'P', "spc" },
	{ FM_TOUCH,	'H', "touch" },
	{ FM_TEMP,	'T', "temp" },
	{0,0,0}
    };

    if ( print_mode == 0 )
    {
	if ( !buf || !buf_size )
	    buf = GetCircBuf( buf_size = 16 );
	char *dest = buf, *end = buf + buf_size - 1;
	const struct info_t *ip;
	for ( ip = info; ip->mode && dest < end; ip++ )
	    *dest++ = ip->mode & file_mode ? ip->ch : '-';
	*dest = 0;
	return buf;
    }

    if ( !buf || !buf_size )
	buf = GetCircBuf( buf_size = 100 );

    char *dest = buf, *end = buf + buf_size - 1;
    const struct info_t *ip;
    for ( ip = info; ip->mode && dest < end; ip++ )
	if ( ip->mode & file_mode )
	{
	    if ( dest > buf )
		*dest++ = ',';
	    dest = StringCopyE(dest,end,ip->name);
	}
    *dest = 0;
    return buf;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetFileOpenMode
(
    bool		create,		// false: open, true: create
    FileMode_t		file_mode	// open modes
)
{
    if ( file_mode & FM_TEST )
	return "r";

    if (!create)
	return file_mode & FM_MODIFY ? "r+b" : "rb";

    switch ( (uint)file_mode & (FM_MODIFY|FM_APPEND) )
    {
	case FM_MODIFY | FM_APPEND : return "a+b";
	case FM_MODIFY             : return "w+b";
	case             FM_APPEND : return "ab";
	default                    : return "wb";
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct FileAttrib_t		///////////////
///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * ClearFileAttrib
(
    FileAttrib_t	* dest		// NULL or destination attribute
)
{
    if (dest)
    {
	memset(dest,0,sizeof(*dest));
	dest->atime.tv_nsec =
	dest->mtime.tv_nsec =
	dest->ctime.tv_nsec =
	dest->itime.tv_nsec =
     #if SUPPORT_UTIMENSAT
		UTIME_OMIT;
     #else
		-2;
     #endif
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * TouchFileAttrib
(
    FileAttrib_t	* dest		// valid destination attribute
)
{
    DASSERT(dest);
    dest->atime = GetClockTime(false);
    dest->mtime = dest->ctime = dest->itime = dest->atime;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * SetFileAttrib
(
    FileAttrib_t	* dest,		// valid destination attribute
    const FileAttrib_t	* src_fa,	// NULL or source attribute
    const struct stat	* src_stat	// NULL or source attribute
					// only used if 'src_fa==NULL'
)
{
    DASSERT(dest);

    if (src_fa)
	memcpy(dest,src_fa,sizeof(*dest));
    else if (src_stat)
    {
	ZeroFileAttrib(dest);
	if (S_ISREG(src_stat->st_mode))
	{
	 #if HAVE_STATTIME_NSEC
	    dest->atime = src_stat->st_atim;
	    dest->mtime = src_stat->st_mtim;
	    dest->ctime = src_stat->st_ctim;
	 #else
	    dest->atime.tv_sec = src_stat->st_atime;
	    dest->mtime.tv_sec = src_stat->st_mtime;
	    dest->ctime.tv_sec = src_stat->st_ctime;
	 #endif
	    dest->itime =  CompareTimeSpec(&dest->mtime,&dest->ctime) > 0
			? dest->mtime : dest->ctime;
	    dest->size  = src_stat->st_size;
	}
	else
	    ClearFileAttrib(dest);
	dest->mode = src_stat->st_mode;
    }

    return dest;
}

///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * MaxFileAttrib
(
    FileAttrib_t	* dest,		// valid source and destination attribute
    const FileAttrib_t	* src_fa,	// NULL or second source attribute
    const struct stat	* src_stat	// NULL or third source attribute
)
{
    DASSERT(dest);

    if (src_fa)
    {
	if (CompareTimeSpec(&dest->atime,&src_fa->atime)<0) dest->atime = src_fa->atime;
	if (CompareTimeSpec(&dest->mtime,&src_fa->mtime)<0) dest->mtime = src_fa->mtime;
	if (CompareTimeSpec(&dest->ctime,&src_fa->ctime)<0) dest->ctime = src_fa->ctime;
	if (CompareTimeSpec(&dest->itime,&src_fa->itime)<0) dest->itime = src_fa->itime;

	if ( dest->size < src_fa->size )
	     dest->size = src_fa->size;
	dest->mode = src_fa->mode;
    }

    if (src_stat)
    {
	if ( S_ISREG(src_stat->st_mode) )
	{
	 #if HAVE_STATTIME_NSEC
	    if ( CompareTimeSpec(&dest->atime,&src_stat->st_atim) < 0 )
		dest->atime = src_stat->st_atim;

	    if ( CompareTimeSpec(&dest->mtime,&src_stat->st_mtim) < 0 )
		dest->mtime = src_stat->st_mtim;

	    if ( CompareTimeSpec(&dest->ctime,&src_stat->st_ctim) < 0 )
		dest->ctime = src_stat->st_ctim;

	    if ( CompareTimeSpec(&dest->itime,&src_stat->st_mtim) < 0 )
		dest->itime = src_stat->st_mtim;
	    if ( CompareTimeSpec(&dest->itime,&src_stat->st_ctim) < 0 )
		dest->itime = src_stat->st_ctim;
	 #else
	    if ( CompareTimeSpecTime(&dest->atime,src_stat->st_atime) < 0 )
	    {
		dest->atime.tv_sec  = src_stat->st_atime;
		dest->atime.tv_nsec = 0;
	    }

	    if ( CompareTimeSpecTime(&dest->mtime,src_stat->st_mtime) < 0 )
	    {
		dest->mtime.tv_sec  = src_stat->st_mtime;
		dest->mtime.tv_nsec = 0;
	    }

	    if ( CompareTimeSpecTime(&dest->ctime,src_stat->st_ctime) < 0 )
	    {
		dest->ctime.tv_sec  = src_stat->st_ctime;
		dest->ctime.tv_nsec = 0;
	    }

	    if ( CompareTimeSpecTime(&dest->itime,src_stat->st_mtime) < 0 )
	    {
		dest->itime.tv_sec  = src_stat->st_mtime;
		dest->itime.tv_nsec = 0;
	    }
	    if ( CompareTimeSpecTime(&dest->itime,src_stat->st_ctime) < 0 )
	    {
		dest->itime.tv_sec  = src_stat->st_ctime;
		dest->itime.tv_nsec = 0;
	    }
	 #endif

	    if ( dest->size < src_stat->st_size )
		 dest->size = src_stat->st_size;
	}
	dest->mode = src_stat->st_mode;
    }

    return dest;
}

///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * NormalizeFileAttrib
(
    FileAttrib_t	* fa		// valid attribute
)
{
    DASSERT(fa);

    if ( fa->size < 0 )
	fa->size = 0;

    if (IsTimeSpecNull(&fa->mtime))
    {
	if (!IsTimeSpecNull(&fa->itime))
	    fa->mtime = fa->itime;
	else if (!IsTimeSpecNull(&fa->ctime))
	    fa->mtime = fa->ctime;
    }

    if (IsTimeSpecNull(&fa->itime))
	fa->itime = CompareTimeSpec(&fa->ctime,&fa->mtime) > 0
			? fa->ctime : fa->mtime;

    if (IsTimeSpecNull(&fa->ctime))
	fa->ctime = CompareTimeSpec(&fa->itime,&fa->mtime) > 0
			? fa->itime : fa->mtime;

    if (IsTimeSpecNull(&fa->atime))
	fa->atime = CompareTimeSpec(&fa->itime,&fa->ctime) > 0
			? fa->itime : fa->ctime;

    return fa;
}

///////////////////////////////////////////////////////////////////////////////
// struct timespec helpers

const struct timespec null_timespec = {0,0};

///////////////////////////////////////////////////////////////////////////////

int CompareTimeSpec0 ( const struct timespec *a, const struct timespec *b )
{
    if (!a) a = &null_timespec;
    if (!b) b = &null_timespec;
    return CompareTimeSpec(a,b);
}

///////////////////////////////////////////////////////////////////////////////

void SetAMTimes ( ccp fname, const struct timespec times[2] )
{
 #if SUPPORT_UTIMENSAT
    utimensat(AT_FDCWD,fname,times,0);
 #else
    struct timeval tv[2];
    tv[1].tv_sec  = times[1].tv_sec;
    tv[1].tv_usec = times[1].tv_nsec / 1000;
    if (IsTimeSpecNull(times+0))
    {
	tv[0].tv_sec  = times[0].tv_sec;
	tv[0].tv_usec = times[0].tv_nsec / 1000;
    }
    else
	tv[0] = tv[1];
    utimes(fname,tv);
 #endif
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		 simple UNIX+TCP socket connect		///////////////
///////////////////////////////////////////////////////////////////////////////
// not stored in dclib-network.c to allow static linking!

int ConnectUnixTCP
(
    ccp		fname,		// unix socket filename
    bool	silent		// true: suppress error messages
)
{
    DASSERT(fname);


    //--- check file name

    struct sockaddr_un sa;
    const uint flen = fname ? strlen(fname)+1 : 0;
    if ( flen > sizeof(sa.sun_path) )
    {
	if (!silent)
	    ERROR0(ERR_CANT_CONNECT,
			"Path name to long for UNIX/STREAM socket: %s\n",fname);
	return -1;
    }

    memset(&sa,0,sizeof(sa));
    memcpy(sa.sun_path,fname,flen);
    sa.sun_family = AF_UNIX;


    //--- create socket

    const int sock = socket(AF_UNIX,SOCK_STREAM,0);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Can't create UNIX/STREAM socket: %s\n",fname);
	return -1;
    }


    //--- connect

    if (connect(sock,(struct sockaddr *)&sa,sizeof(sa)))
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Can't connect to UNIX/STREAM socket: %s\n",fname);
	close(sock);
	return -1;
    }

    return sock;
}

///////////////////////////////////////////////////////////////////////////////

int (*ConnectTCP_Hook)
(
    // if set: ConnectNumericTCP() uses ConnectTCP()

    ccp		addr,		// TCP address: ['tcp':] IPv4 [:PORT]
    int		default_port,	// NULL or default port, if not found in 'addr'
    bool	silent		// true: suppress error messages
) = 0;

///////////////////////////////////////////////////////////////////////////////

char * ScanNumericIP4
(
    // returns next unread character or NULL on error

    ccp		addr,		// address to scan
    u32		*r_ipv4,	// not NULL: store result here (local endian)
    u32		*r_port,	// not NULL: scan port too (local endian)
    uint	default_port	// return this if no port found
)
{
    if (!addr)
    {
     abort:
	if (r_ipv4)
	    *r_ipv4 = 0;
	if (r_port)
	    *r_port = 0;
	return 0;
    }

    u32 num[4] = {0};
    uint i;
    ccp ptr = addr;
    for ( i = 0; i < 4; )
    {
	char *end;
	uint temp = str2ul(ptr,&end,10);
	if ( !end || end == ptr )
	    break;
	num[i++] = temp;
	ptr = addr = end;
	if ( *ptr != '.' )
	    break;
	ptr++;
    }

    uint ip4;
    switch (i)
    {
	case 1:
	    ip4 = num[0];
	    break;

	case 2:
	    ip4 = num[0] << 24 | num[1];
	    break;

	case 3:
	    ip4 = num[0] << 24 | num[1] << 16 | num[2];
	    break;

	case 4:
	    ip4 = num[0] << 24 | num[1] << 16 | num[2] << 8 | num[3];
	    break;

	default:
	    goto abort;
    }
    if (r_ipv4)
	*r_ipv4 = ip4;

    if (r_port)
    {
	if ( *addr == ':' )
	{
	    char *end;
	    uint num = str2ul(addr+1,&end,10);
	    if ( end && end > addr+1 && num < 0x10000 )
	    {
		addr = end;
		default_port = num;
	    }
	}
	*r_port = default_port;
    }

    return (char*)addr;
}

///////////////////////////////////////////////////////////////////////////////

mem_t ScanNumericIP4Mem
(
    // returns unread character or NullMem on error

    mem_t	addr,		// address to scan
    u32		*r_ipv4,	// not NULL: store result here (local endian)
    u32		*r_port,	// not NULL: scan port too (local endian)
    uint	default_port	// return this if no port found
)
{
    if ( !addr.ptr || !addr.len )
    {
     abort:
	if (r_ipv4)
	    *r_ipv4 = 0;
	if (r_port)
	    *r_port = 0;
	return NullMem;
    }

    u32 num[4] = {0};
    uint i;
    ccp ptr = addr.ptr;
    ccp end = ptr + addr.len;

    for ( i = 0; i < 4; )
    {
	uint temp;
	ccp next = ScanNumber(&temp,ptr,end,10,10);
	if ( !next || next == ptr )
	    break;
	num[i++] = temp;
	addr = BehindMem(addr,next);
	ptr = addr.ptr;
	if ( *ptr != '.' )
	    break;
	ptr++;
    }

    uint ip4;
    switch (i)
    {
	case 1:
	    ip4 = num[0];
	    break;

	case 2:
	    ip4 = num[0] << 24 | num[1];
	    break;

	case 3:
	    ip4 = num[0] << 24 | num[1] << 16 | num[2];
	    break;

	case 4:
	    ip4 = num[0] << 24 | num[1] << 16 | num[2] << 8 | num[3];
	    break;

	default:
	    goto abort;
    }
    if (r_ipv4)
	*r_ipv4 = ip4;

    if (r_port)
    {
	ptr = addr.ptr;
	if ( ptr < end && *ptr == ':' )
	{
	    ptr++;
	    uint num;
	    ccp next = ScanNumber(&num,ptr,end,10,10);
	    if ( next > ptr && num < 0x10000 )
	    {
		addr = BehindMem(addr,next);
		default_port = num;
	    }
	}
	*r_port = default_port;
    }

    return addr;
}

///////////////////////////////////////////////////////////////////////////////
// Returns a NETWORK MASK "/a.b.c.d" or as CIDR number "/num" between 0 and 32.
// An optional slash '/' at the beginning is skipped.
// Returns modified 'source' if a MASK or CDIR is detected.
// If no one is detected, source is unmodified and returned mask = ~0.

mem_t ScanNetworkMaskMem ( u32 *mask, mem_t source )
{
    u32 m = M1(m);
    if ( source.ptr && source.len )
    {
	ccp ptr = source.ptr;
	ccp end = ptr + source.len;
	if ( *ptr == '/' )
	    ptr++;

	uint num;
	ccp next = ScanNumber(&num,ptr,end,10,10);
	if ( next > ptr )
	{
	    if ( next < end && *next == '.' )
	    {
		// is a network mask
		source = ScanNumericIP4Mem(BehindMem(source,ptr),&m,0,0);
	    }
	    else if ( num <= 32 )
	    {
		m = num ? ~(( 1 << (32-num) ) - 1) : 0;
		source = BehindMem(source,next);
	    }
	}
    }

    if (mask)
	*mask = m;
    return source;
}

///////////////////////////////////////////////////////////////////////////////

int ConnectNumericTCP
(
    // like ConnectTCP(), but without name resolution (only numeric ip+port)

    ccp		addr,		// TCP address: ['tcp':] IPv4 [:PORT]
    int		default_port,	// NULL or default port, if not found in 'addr'
    bool	silent		// true: suppress error messages
)
{
    if (ConnectTCP_Hook)
	return ConnectTCP_Hook(addr,default_port,silent);

    ccp unix_path = CheckUnixSocketPath(addr,0);
    if (unix_path)
	return ConnectUnixTCP(unix_path,silent);

    if ( !strncasecmp(addr,"tcp:",4) )
	addr += 4;

    u32 ip4, port;
    ccp end = ScanNumericIP4(addr,&ip4,&port,default_port);
    if (!end)
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Invalid IPv4 address: %s\n",addr);
	return -1;
    }

    int sock = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Can't create socket: %s\n",addr);
	return -1;
    }

    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = htonl(ip4);
    sa.sin_port = htons(port);

    if (connect(sock,(struct sockaddr *)&sa,sizeof(sa)))
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Can't connect to %s\n",PrintIP4(0,0,ip4,port) );
	close(sock);
	return -1;
    }

    return sock;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct File_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeFile
(
   File_t		* f		// file structure
)
{
    DASSERT(f);
    memset(f,0,sizeof(*f));
    f->fname = EmptyString;
}

///////////////////////////////////////////////////////////////////////////////

enumError ResetFile
(
   File_t		* f,		// file structure
   uint			set_time	// 0: don't set
					// 1: set time before closing using 'fatt'
					// 2: set current time before closing
)
{
    DASSERT(f);
    const enumError err = CloseFile(f,set_time);
    FreeString(f->fname);
    if (f->data_alloced)
	FREE(f->data);
    InitializeFile(f);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseFile
(
   File_t		* f,		// file structure
   uint			set_time	// 0: don't set
					// 1: set time before closing using 'fatt'
					// 2: set current time before closing
)
{
    DASSERT(f);
    TRACE("CloseFile(%p,%d) fname=%s\n",f,set_time,f->fname);

    if (f->f)
    {
	if (!f->is_stdio)
	{
	    if ( fclose(f->f) && f->max_err <= ERR_WARNING )
		f->max_err = ERROR1(ERR_WRITE_FAILED,
				    "Error while closing file: %s\n",f->fname);

	    if (!f->is_socket)
	    {
		if ( f->fmode & FM_TEMP )
		{
		    TRACE("UNLINK TEMP %s\n",f->fname);
		    unlink(f->fname);
		}
		else if ( f->is_writing )
		{
		    if ( f->max_err > ERR_WARNING )
		    {
			TRACE("UNLINK %s\n",f->fname);
			unlink(f->fname);
		    }
		    else if ( set_time == 1 && !IsTimeSpecNull(&f->fatt.mtime) )
			SetAMTimes(f->fname,f->fatt.times);
		    else if ( set_time > 1 || f->fmode & FM_TOUCH )
		    {
			utime(f->fname,0);
		    }
		}
	    }
	}
	f->f = 0;
    }

    return f->max_err;
}

///////////////////////////////////////////////////////////////////////////////

static bool is_seekable
(
    File_t		* F		// file structure
)
{
    const mode_t mode = F->st.st_mode;
    F->is_seekable = ( S_ISREG(mode) || S_ISBLK(mode) || S_ISCHR(mode) )
		&& F->st.st_size
		&& lseek(fileno(F->f),0,SEEK_SET) != (off_t)-1;
    return F->is_seekable;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError OpenFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			fname,		// file to open
    FileMode_t		file_mode,	// open modes
    off_t		limit,		// >0: don't open, if file size > limit
    ccp			limit_message	// NULL or a with LF terminated text.
					//   It is printed after the message.
)
{
    DASSERT(f);
    DASSERT(fname);

    if (initialize)
	InitializeFile(f);
    else
	ResetFile(f,false);
    f->fmode = file_mode & FM_M_OPEN;

    TRACE("OpenFile(%p,%d,,%s,%llu) [%s] %s\n",
		f, initialize, GetFileModeStatus(0,0,file_mode,0), (u64)limit,
		GetFileModeStatus(0,0,f->fmode,1), fname );


    //--- test for TCP

    if ( file_mode & FM_TCP && !strncasecmp(fname,"tcp:",4) )
    {
	const int fd = ConnectNumericTCP(fname,0,(f->fmode&FM_SILENT)!=0);
	if ( fd == -1 )
	    return f->max_err = ERR_CANT_OPEN;

	f->f = fdopen(fd,"rw");
	f->is_socket = f->is_reading = f->is_writing = true;
	return f->max_err;
    }


    //--- test for stdin

    if ( *fname == '-' && !fname[1] )
    {
	f->fname = MinusString;
	if ( f->fmode & FM_STDIO )
	{
	    f->is_stdio = true;
	    f->f = stdin;
	    fstat(fileno(stdin),&f->st);
	    is_seekable(f);
	    return ERR_OK;
	}
    }

    f->fname = STRDUP(fname);


    //--- test 'unix:' prefix

    bool have_unix = file_mode & FM_TCP && !strncasecmp(fname,"unix:",5);
    if (have_unix)
	fname += 5;


    //--- does file exist?

    for(;;)
    {
	if (!stat(fname,&f->st))
	    break;

	if (have_unix)
	{
	    have_unix = false;
	    fname -= 5;
	    continue;
	}

     not_found:
	memset(&f->st,0,sizeof(f->st));
	if ( f->fmode & FM_IGNORE )
	    return f->max_err = ERR_NOT_EXISTS;

	if ( !(f->fmode & FM_SILENT) )
	    ERROR1(ERR_CANT_OPEN,"Can't find file: %s\n",fname);
	return f->max_err = ERR_CANT_OPEN;
    }


    //--- UNIX socket -> try STREAM connect

    if (S_ISSOCK(f->st.st_mode))
    {
	f->fmode &= ~(FM_REMOVE|FM_TEMP);

	const int fd = ConnectUnixTCP(fname,(f->fmode&FM_SILENT)!=0);
	if ( fd == -1 )
	    return f->max_err = ERR_CANT_OPEN;

	f->f = fdopen(fd,"rw");
	f->is_socket = f->is_reading = f->is_writing = true;
	return f->max_err;
    }

    if (have_unix)
	goto not_found;


    //--- check size limit

    SetFileAttrib(&f->fatt,0,&f->st);
    if ( limit && f->st.st_size > limit )
    {
	char sbuf[12], lbuf[12];
	PrintSize1024(sbuf,sizeof(sbuf),(u64)f->st.st_size,0);
	PrintSize1024(lbuf,sizeof(lbuf),(u64)limit,0);

	// be tolerant, if both values produce same text
	if (strcmp(sbuf,lbuf))
	{
	    if ( !(f->fmode & FM_SILENT) )
		ERROR0(ERR_CANT_OPEN,
			"File too large (size=%s, limit=%s): %s\n%s",
			sbuf, lbuf, fname,
			limit_message ? limit_message : "" );
	    return f->max_err = ERR_CANT_OPEN;
	}
    }


    //--- open file

    ccp omode = GetFileOpenMode(false,f->fmode);
    f->f = fopen(fname,omode);
    TRACE("OpenFile(%s,%s) : [%s], %p\n",
		fname, omode, GetFileModeStatus(0,0,f->fmode,1), f->f );

    if (!f->f)
    {
	if ( !(f->fmode & FM_SILENT) )
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",fname);
	return f->max_err = ERR_CANT_OPEN;
    }

    f->is_reading = true;
    if ( f->fmode & FM_MODIFY )
	f->is_writing = true;

    is_seekable(f);
    return f->max_err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError CheckCreateFile
(
    // returns:
    //   ERR_DIFFER:		[FM_STDIO] source is "-" => 'st' is zeroed
    //	 ERR_WARNING:		[FM_DEV,FM_SOCK,FM_SPC] not a regular file
    //   ERR_WRONG_FILE_TYPE:	file exists, but is not a regular file
    //   ERR_ALREADY_EXISTS:	file already exists
    //   ERR_INVALID_VERSION:	already exists, but FM_NUMBER set => no msg printed
    //   ERR_CANT_CREATE:	FM_UPDATE is set, but file don't exist
    //   ERR_OK:		file not exist or can be overwritten

    ccp			fname,		// filename to open
    FileMode_t		file_mode,	// open modes
    struct stat		*st		// not NULL: store file status here
)
{
    DASSERT(fname);

    if ( file_mode & FM_STDIO && fname[0] == '-' && !fname[1] )
    {
	if (st)
	    memset(st,0,sizeof(*st));
	return ERR_DIFFER;
    }

    struct stat local_st;
    if (!st)
	st = &local_st;

    if (!stat(fname,st))
    {
	if ( S_ISBLK(st->st_mode) || S_ISCHR(st->st_mode) )
	{
	    if ( file_mode & FM_DEV )
		return ERR_WARNING;

	    if ( !(file_mode & FM_SILENT) )
		ERROR0( ERR_ALREADY_EXISTS,
			"Can't write to %s device: %s\n",
			S_ISBLK(st->st_mode) ? "block" : "character", fname );
	    return ERR_WRONG_FILE_TYPE;
	}

	if ( S_ISSOCK(st->st_mode) )
	{
	    if ( file_mode & FM_SOCK )
		return ERR_WARNING;

	    if ( !(file_mode & FM_SILENT) )
		ERROR0( ERR_ALREADY_EXISTS,
			"Can't write to UNIX socket: %s\n", fname );
	    return ERR_WRONG_FILE_TYPE;
	}

	if (!S_ISREG(st->st_mode))
	{
	    if ( file_mode & FM_SPC )
		return ERR_WARNING;

	    if ( !(file_mode & FM_SILENT) )
		ERROR0( ERR_WRONG_FILE_TYPE,
		    "Not a plain file: %s\n", fname );
	    return ERR_WRONG_FILE_TYPE;
	}

	if (!( file_mode & (FM_OVERWRITE|FM_REMOVE|FM_UPDATE|FM_APPEND) ))
	{
	    if ( file_mode & FM_NUMBER )
		return ERR_INVALID_VERSION;

	    if ( !(file_mode & FM_SILENT) )
		ERROR0( ERR_ALREADY_EXISTS,
		    "File already exists: %s\n", fname );
	    return ERR_ALREADY_EXISTS;
	}
    }
    else
    {
	memset(st,0,sizeof(*st));
	if ( file_mode & FM_UPDATE )
	{
	    if ( !(file_mode & FM_SILENT) )
		ERROR0(ERR_CANT_CREATE,
		    "Try to update non existing file: %s\n", fname );
	    return ERR_CANT_CREATE;
	}
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			fname,		// file to open
    FileMode_t		file_mode	// open modes
)
{
    DASSERT(f);
    DASSERT(fname);

    if (initialize)
	InitializeFile(f);
    else
	ResetFile(f,false);
    f->fmode = file_mode & FM_M_CREATE;

    noPRINT("CreateFile(%p,%d,,%s) [%s] %s\n",
		f, initialize, GetFileModeStatus(0,0,file_mode,0),
		GetFileModeStatus(0,0,f->fmode,1), fname );


    //--- test for stdout

    if ( *fname == '-' && !fname[1] )
    {
	f->fname = MinusString;
	if ( f->fmode & FM_STDIO )
	{
	    f->is_stdio = true;
	    f->f = stdout;
	    fstat(fileno(stdout),&f->st);
	    is_seekable(f);
	    return ERR_OK;
	}
    }


    //--- mode calculations

//    if ( f->fmode & FM_UPDATE )
//	f->fmode = f->fmode & ~(FM_REMOVE|FM_MKDIR) | FM_OVERWRITE;

    enumError err = CheckCreateFile(fname,f->fmode,&f->st);

    char num_fname[PATH_MAX];
    if ( err == ERR_INVALID_VERSION )
    {
	NumberedFilename(num_fname,sizeof(num_fname),fname,0,0,false);
	fname = num_fname;
	err = CheckCreateFile( fname, f->fmode & ~FM_NUMBER, &f->st );
    }

    f->fname = STRDUP(fname);
    if ( f->fmode & (FM_MODIFY|FM_APPEND) )
	SetFileAttrib(&f->fatt,0,&f->st);

    if (err)
    {
	if ( err > ERR_WARNING )
	    return err;

	// is a device, socket or special file
	err = ERR_OK;
	f->fmode = f->fmode & ~(FM_REMOVE|FM_TEMP) | FM_OVERWRITE;
    }

    if ( f->fmode & FM_TEST )
    {
	// never create or remove a file in TEST mode.
	return ERR_OK;
    }

    if (S_ISSOCK(f->st.st_mode))
    {
	const int fd = ConnectUnixTCP(fname,(f->fmode&FM_SILENT)!=0);
	if ( fd == -1 )
	    return f->max_err = ERR_CANT_OPEN;

	f->f = fdopen(fd,"rw");
	f->is_socket = f->is_reading = f->is_writing = true;
	return ERR_OK;
    }

    if ( f->st.st_mode && f->fmode & FM_REMOVE )
    {
	unlink(fname);

	struct stat st;
	if (!stat(fname,&st))
	{
	    if ( !(f->fmode & FM_SILENT) )
		ERROR0(ERR_REMOVE_FAILED,"Can't remove file: %s\n",fname);
	    return f->max_err = ERR_REMOVE_FAILED;
	}
    }

    ccp omode = GetFileOpenMode(true,f->fmode);
    f->f = fopen(fname,omode);
    noPRINT("CreateFile(%s,%s) : [%s], %p\n",
		fname, omode, GetFileModeStatus(0,0,f->fmode,1), f->f );

    if ( !f->f )
    {
	if ( f->fmode & FM_MKDIR )
	{
	    CreatePath(fname,false);
	    f->f = fopen(fname,omode);
	}

	if ( !f->f )
	{
	    if ( !(f->fmode & FM_SILENT) )
		ERROR1(ERR_CANT_CREATE,"Can't create file: %s\n",fname);
	    return f->max_err = ERR_CANT_CREATE;
	}
    }

    f->is_writing = true;
    if ( f->fmode & FM_MODIFY)
	f->is_reading = true;

    if (!fstat(fileno(f->f),&f->st))
    {
	const mode_t mode = f->st.st_mode;
	f->is_seekable = S_ISREG(mode) || S_ISBLK(mode) || S_ISCHR(mode);
    }

    return f->max_err;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError WriteFileAt
(
    File_t		* F,		// file to write
    size_t		* cur_offset,	// pointer to current file offset, modified
    size_t		offset,		// offset to write
    const void		* data,		// data to write
    size_t		size		// size of 'data'
)
{
    DASSERT(F);
    DASSERT(F->f);
    DASSERT(data||!size);
    DASSERT(cur_offset);

    if (!size)
	return ERR_OK;

    if ( offset != *cur_offset )
    {
	if (F->is_seekable)
	{
	    noPRINT("FSEEK %zx -> %zx\n",*cur_offset,offset);
	    if (fseek(F->f,offset,SEEK_SET))
		return ERROR1(ERR_WRITE_FAILED,"Can't set file pointer: %s\n",F->fname);
	}
	else if ( offset < *cur_offset )
	{
	    noPRINT("FSEEK %zx -> %zx\n",*cur_offset,offset);
	    return ERROR0(ERR_WRITE_FAILED,
		"Can't set file pointer on non seekable file: %s\n",F->fname);
	}
	else
	{
	    char buf[0x8000] = {0};
	    size_t fill_size = offset - *cur_offset;
	    while (fill_size)
	    {
		const size_t write_size = fill_size < sizeof(buf) ? fill_size : sizeof(buf);
		noPRINT("FILL/ZERO %zx/%zx\n",write_size,fill_size);
		const size_t written = fwrite(buf,1,write_size,F->f);
		if ( written != write_size )
		    return ERROR1(ERR_WRITE_FAILED,"Writing %zu NULL bytes failed: %s\n",
				write_size, F->fname );
		fill_size -= write_size;
	    }
	}
    }

    noPRINT("WRITE %zx @%zx -> %zx\n",size,offset,offset+size);
    const size_t written = fwrite(data,1,size,F->f);
    if ( written != size )
	return ERROR1(ERR_WRITE_FAILED,"Writing %zu bytes at offset %zu failed: %s\n",
		size,offset,F->fname);

    *cur_offset = offset + size;
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SetFileSize
(
    File_t		* F,		// file to write
    size_t		* cur_offset,	// pointer to current file offset, modified
    size_t		size		// offset to write
)
{
    DASSERT(F);
    DASSERT(F->f);
    DASSERT(cur_offset);

    fflush(F->f);
    if (!F->is_seekable)
	return WriteFileAt(F,cur_offset,size,0,0);

    if (ftruncate(fileno(F->f),size))
    {
	if ( F->max_err < ERR_WRITE_FAILED )
	    F->max_err = ERR_WRITE_FAILED;
	return ERROR1( ERR_WRITE_FAILED,
			"Set file size to %llu failed: %s\n",
			(u64)size, F->fname );
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SkipFile
(
    File_t		*F,		// file to write
    size_t		skip		// number of bytes to skip
)
{
    DASSERT(F);
    DASSERT(F->f);

    if (!skip)
	return ERR_OK;

    if ( F->is_seekable && !fseek(F->f,skip,SEEK_CUR) )
	return ERR_OK;

    char buf[0x8000];
    while ( skip > 0 )
    {
	size_t max = skip < sizeof(buf) ? skip : sizeof(buf);
	size_t stat = fread(buf,1,max,F->f);
	if (!stat)
	{
	    if (feof(F->f))
		break;
	    return ERROR1(ERR_READ_FAILED,
				"Reading %zu to skip failed: %s\n",
				skip, F->fname );
	}
	skip -= stat;
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError RegisterFileError
(
    File_t		* f,		// file structure
    enumError		new_error	// new error code
)
{
    DASSERT(f);
    if ( f->max_err < new_error )
	 f->max_err = new_error;
    return f->max_err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

s64 GetFileSize
(
    ccp			path1,		// NULL or part 1 of path
    ccp			path2,		// NULL or part 2 of path
    s64			not_found_val,	// return value if no regular file found
    FileAttrib_t	* fatt,		// not NULL: store file attributes
    bool		fatt_max	// true: store max values to 'fatt'
)
{
    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    TRACE("GetFileSize(%s,%lld)\n",path,not_found_val);

    struct stat st;
    if ( stat(path,&st) || !S_ISREG(st.st_mode) )
    {
	if ( fatt && !fatt_max )
	    ClearFileAttrib(fatt);
	return not_found_val;
    }

    if (fatt)
    {
	if (fatt_max)
	    MaxFileAttrib(fatt,0,&st);
	else
	    SetFileAttrib(fatt,0,&st);
    }

    return st.st_size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError OpenReadFile
(
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    FileMode_t		file_mode,	// open modes
    off_t		limit,		// >0: don't open, if file size > limit
    ccp			limit_message,	// NULL or a with LF terminated text.
					//   It is printed after the message.

    u8			** res_data,	// store alloced data here (always NULL terminated)
    uint		* res_size,	// not NULL: store data size
    ccp			* res_fname,	// not NULL: store alloced filename
    FileAttrib_t	* res_fatt	// not NULL: store file attributes
)
{
    DASSERT( path1 || path2 );
    DASSERT(res_data);

    *res_data = 0;
    if (res_size)
	*res_size = 0;
    if (res_fname)
	*res_fname = 0;
    if (res_fatt)
	memset(res_fatt,0,sizeof(*res_fatt));

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    PRINT("OpenReadFile(%s,%s)\n",path,GetFileModeStatus(0,0,file_mode,0));

    File_t F;
    enumError err = OpenFile(&F,true,path,file_mode,limit,limit_message);
    if (err)
	return err;

    uint data_size = F.st.st_size;
    if (!F.is_seekable)
	data_size = limit;
    noPRINT("OpenReadFile() fsize=%u, seekable=%d\n",data_size,F.is_seekable);
    u8 *data = MALLOC(data_size+1);

    size_t read_size = fread(data,1,data_size,F.f);
    if ( read_size && !F.is_seekable )
    {
	PRINT("non seekable read: %zu\n",read_size);
	data_size = read_size;
	data = REALLOC(data,data_size+1);
    }
    data[data_size] = 0; // termination for text files

    if ( read_size != data_size )
    {
	if (!(file_mode&FM_SILENT))
	    FILEERROR1(&F,ERR_READ_FAILED,"Read failed: %s\n",path);
	ResetFile(&F,false);
	FREE(data);
	return ERR_READ_FAILED;
    }

    *res_data = data;
    if (res_size)
	*res_size = data_size;
    if (res_fname)
    {
	*res_fname = F.fname;
	F.fname = EmptyString;
    }
    if (res_fatt)
	SetFileAttrib(res_fatt,&F.fatt,0);

    ResetFile(&F,false);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError LoadFile
(
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    size_t		skip,		// skip num of bytes before reading
    void		* data,		// destination buffer, size = 'size'
    size_t		size,		// size to read
    int			silent,		// 0: print all error messages
					// 1: suppress file size warning
					// 2: suppress all error messages
    FileAttrib_t	* fatt,		// not NULL: store file attributes
    bool		fatt_max	// true: store *max* values to 'fatt'
)
{
    DASSERT(data);
    if (!size)
	return ERR_OK;

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    TRACE("LoadFile(%s,%zu,%zu,%d)\n",path,skip,size,silent);

    if (fatt)
    {
	struct stat st;
	if (!stat(path,&st))
	    UseFileAttrib(fatt,0,&st,fatt_max);
    }

    FILE * f = fopen(path,"rb");
    if (!f)
    {
	if (silent<2)
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",path);
	return ERR_CANT_OPEN;
    }

    if ( skip > 0 )
	fseek(f,skip,SEEK_SET);

    size_t read_stat = fread(data,1,size,f);
    fclose(f);

    if ( read_stat == size )
	return ERR_OK;

    enumError err = ERR_READ_FAILED;
    if ( silent == 1 && size > 0 )
	err = ERR_WARNING;
    else if ( silent < 2 )
	ERROR1(ERR_READ_FAILED,"Can't read file: %s\n",path);

    noPRINT("D=%p, s=%zu/%zu: %s\n",data,read_stat,size,path);
    if ( read_stat >= 0 && read_stat < size )
	memset((char*)data+read_stat,0,size-read_stat);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError LoadFileAlloc
(
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    size_t		skip,		// skip num of bytes before reading
    u8			** res_data,	// result: free existing data, store ptr to alloc data
					// always one more byte is alloced and set to NULL
    size_t		*  res_size,	// result: size of 'res_data'
    size_t		max_size,	// >0: a file size limit
    int			silent,		// 0: print all error messages
					// 1: suppress file size warning
					// 2: suppress all error messages
    FileAttrib_t	* fatt,		// not NULL: store file attributes
    bool		fatt_max	// true: store max values to 'fatt'
)
{
    DASSERT(res_data);
    DASSERT(res_size);

    //--- clear return data

    if (res_data)
	*res_data = 0;

    if (res_size)
	*res_size = 0;

    if ( fatt && !fatt_max )
	memset(fatt,0,sizeof(*fatt));


    //--- get file size

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);

    const s64 size = GetFileSize(path,0,-1,0,0);
    if ( size == -1 )
    {
	if ( silent < 2 )
	    ERROR0(ERR_CANT_OPEN,"File not found: %s\n",path);
	return ERR_CANT_OPEN;
    }

    if ( max_size && size > max_size )
    {
	if ( silent < 2 )
	    ERROR0(ERR_INVALID_FILE,"File too large: %s\n",path);
	return ERR_INVALID_FILE;
    }

    u8 *data = MALLOC(size+1);
    enumError err = LoadFile(path,0,skip,data,size,silent,fatt,fatt_max);
    if (err)
    {
	FREE(data);
	return err;
    }

    if (res_data)
    {
	data[size] = 0;
	*res_data = data;
    }
    else
	FREE(data);

    if (res_size)
	*res_size = size;

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError SaveFile
(
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    FileMode_t		file_mode,	// open modes
    const void		* data,		// data to write
    uint		data_size,	// size of 'data'
    FileAttrib_t	* fatt		// not NULL: set timestamps using this attrib
)
{
    DASSERT(data);

    File_t F;
    enumError err = OpenWriteFile(&F,true,path1,path2,file_mode,data,data_size);
    if ( err == ERR_OK )
    {
	if (fatt)
	    memcpy(&F.fatt,fatt,sizeof(F.fatt));
	err = ResetFile(&F,true);
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenWriteFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    FileMode_t		file_mode,	// open modes
    const void		* data,		// data to write
    uint		data_size	// size of 'data'
)
{
    DASSERT(f);
    DASSERT(data);

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    TRACE("SaveFile(%s) %s\n",GetFileModeStatus(0,0,file_mode,0),path);

    enumError err = CreateFile(f,true,path,file_mode);
    if ( err || !f->f )
    {
	ResetFile(f,0);
	return err;
    }

    const uint written = fwrite(data,1,data_size,f->f);
    if ( written != data_size )
    {
	ERROR1(ERR_WRITE_FAILED,"Write to file failed: %s\n",path);
	ResetFile(f,0);
	return ERR_WRITE_FAILED;
    }

    return ERR_OK;
};

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int CompareInt ( const int * a, const int *b )
{
    DASSERT( a && b );
    return *a < *b ? -1 : *a > *b;
}

///////////////////////////////////////////////////////////////////////////////

uint CloseAllExcept
(
    int		*list,		// exception list of unordered FD, max contain -1
    uint	n_list,		// number of elements in 'list'
    int		min,		// close all until and including this fd
    int		add		// if a FD was found, incremet 'min' to 'fd+add'
)
{
    //--- use a private copy of the except list

    enum { LOCAL_LIST_SIZE = 1000 };
    int local_list[LOCAL_LIST_SIZE];
    int *except_list = n_list < LOCAL_LIST_SIZE
			? local_list : MALLOC((n_list+1)*sizeof(*except_list));

    int *ptr = except_list;
    while ( n_list-- > 0 )
    {
	const int fd = *list++;
	if ( fd != -1 )
	    *ptr++ = fd;
    }
    *ptr = -1;

    //--- ... and sort it

    n_list = ptr - except_list;
    if ( n_list > 1 )
	qsort(except_list,n_list,sizeof(*except_list),(qsort_func)CompareInt);


    //--- adjust 'min' and 'add'

    if ( add <= 0 )
	add = 1;
    const int temp = ( n_list ? except_list[n_list-1] : 0 ) + add;
    if ( min < temp )
	min = temp;


    //--- close files

    ptr = except_list;
    int *end = ptr + n_list;
    int i, count = 0;
    for ( i = 0; i <= min; i++ )
    {
	while ( ptr < end && *ptr < i )
	    ptr++;
	if ( i != *ptr && !close(i) )
	{
	    count++;
	    const int temp = i + add;
	    if ( min < temp )
		min = temp;
	    //printf("closed: %d/%d\n",i,min);
	}
    }

    if ( except_list != local_list )
	FREE(local_list);
    return count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		CopyFile*(),  TransferFile()		///////////////
///////////////////////////////////////////////////////////////////////////////

#undef LOGPRINT
//#define LOGPRINT PRINT
#define LOGPRINT noPRINT

///////////////////////////////////////////////////////////////////////////////

bool IsSameFile ( ccp path1, ccp path2 )
{
    if (!strcmp(path1,path2))
	return true;

    struct stat st1, st2;
    if ( stat(path1,&st1) || stat(path2,&st2) )
	return false;

    return st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino;
}

///////////////////////////////////////////////////////////////////////////////

enumError CopyFileHelper
(
    ccp		src,		// source path
    ccp		dest,		// destination path
    int		open_flags,	// flags for open()
    mode_t	open_mode,	// mode for open()
    uint 	temp_and_move	// >0: ignore open_flags,
				//     create temp, then move temp to dest
)
{
    LOGPRINT("CopyFileHelper(%x,%x,%d) %s -> %s\n",
		open_flags, open_mode, temp_and_move>0, src, dest );

    int fd_dest, fd_src;
    char buf[16*1024], temp[PATH_MAX];

    if ( !dest || !src || !*dest || !*src )
	return ERR_MISSING_PARAM;

    if (IsSameFile(src,dest))
	return ERR_OK;

    fd_src = open(src,O_RDONLY);
    if ( fd_src < 0 )
	return ERR_CANT_OPEN;

    if (temp_and_move)
    {
	StringCat2S(temp,sizeof(temp),dest,"XXXXXX");
	fd_dest = mkstemp(temp);
	LOGPRINT("CopyFileHelper(TEMP) %s -> %d\n",temp,fd_dest);
    }
    else
	fd_dest = open(dest,open_mode,open_mode);
    if ( fd_dest < 0 )
	goto error;

    ssize_t nread;
    while ( nread = read(fd_src,buf,sizeof(buf)), nread > 0 )
    {
	char *ptr = buf;
	do
	{
	    ssize_t nwritten = write(fd_dest,ptr,nread);

	    if ( nwritten >= 0 )
	    {
		nread -= nwritten;
		ptr   += nwritten;
	    }
	    else if ( errno != EINTR )
		goto error;
	}
	while (nread > 0);
    }

    if ( nread == 0 )
    {
	if ( close(fd_dest) < 0 )
	{
	    fd_dest = -1;
	    goto error;
	}
	if (temp_and_move)
	{
	    chmod(temp,open_mode);
	    if (rename(temp,dest))
	    {
		unlink(temp);
		goto error;
	    }
	}
	close(fd_src);
	return ERR_OK;
    }

  error:;
    int saved_errno = errno;

    close(fd_src);
    if ( fd_dest >= 0 )
    {
	close(fd_dest);
	if (temp_and_move)
	    unlink(temp);
    }

    errno = saved_errno;
    return ERR_WRITE_FAILED;
}

//-----------------------------------------------------------------------------

enumError CopyFile
(
    ccp		src,		// source path
    ccp		dest,		// destination path
    mode_t	open_mode	// mode for open()
)
{
    return CopyFileHelper(src,dest,O_WRONLY|O_TRUNC|O_EXCL,open_mode,false);
}

//-----------------------------------------------------------------------------

enumError CopyFileCreate
(
    ccp		src,		// source path
    ccp		dest,		// destination path
    mode_t	open_mode	// mode for open()
)
{
    return CopyFileHelper(src,dest,O_WRONLY|O_CREAT|O_EXCL,open_mode,false);
}

//-----------------------------------------------------------------------------

enumError CopyFileTemp
(
    ccp		src,		// source path
    ccp		dest,		// destination path
    mode_t	open_mode	// mode for open()
)
{
    return CopyFileHelper(src,dest,0,open_mode,true);
}

///////////////////////////////////////////////////////////////////////////////

enumError TransferFile
(
    LogFile_t		*log,		// not NULL: log activities
    ccp			dest,		// destination path
    ccp			src,		// source path
    TransferMode_t	tfer_mode,	// transfer mode
    mode_t		open_mode	// mode for CopyFile*() -> open()
)
{
    if ( !dest || !*dest || !src || !*src )
	return ERR_MISSING_PARAM;

    if (IsSameFile(src,dest))
	return ERR_NOTHING_TO_DO;

    const bool testmode = ( tfer_mode & TFMD_F_TEST ) != 0;
 
    if ( tfer_mode & TFMD_J_MOVE1 )
    {
	LOGPRINT("TransferFile(MOVE1) %s -> %s\n",src,dest);
	struct stat st;
	if ( !stat(src,&st) && st.st_nlink == 1
		&& ( testmode || !rename(src,dest) ))
	{
	    if (log)
		PrintLogFile(log, "%s:\n\t< %s\n\t> %s\n",
				testmode ? "Would MOVE" : "MOVED", src, dest );
	    return ERR_OK;
	}
    }
    else if ( tfer_mode & TFMD_J_MOVE )
    {
	LOGPRINT("TransferFile(MOVE) %s -> %s\n",src,dest);
	if ( testmode || !rename(src,dest) )
	{
	    if (log)
		PrintLogFile(log, "%s:\n\t< %s\n\t> %s\n",
				testmode ? "Would MOVE" : "MOVED", src, dest );
	    return ERR_OK;
	}
    }

    if ( tfer_mode & TFMD_J_LINK )
    {
	LOGPRINT("TransferFile(RM_DEST) %s\n",dest);
	if ( tfer_mode & TFMD_J_RM_DEST && !testmode )
	    unlink(dest);

	LOGPRINT("TransferFile(LINK) %s -> %s\n",src,dest);
	if ( testmode || !link(src,dest) )
	{
	    if (log)
		PrintLogFile(log, "%s:\n\t< %s\n\t> %s\n",
				testmode ? "Would LINK" : "LINKED", src, dest );
	    return ERR_OK;
	}
    }

    if ( tfer_mode & TFMD_J_COPY )
    {
	LOGPRINT("TransferFile(COPY) %s -> %s\n",src,dest);
	if (testmode)
	{
	    if (log)
	    {
		if ( tfer_mode & TFMD_J_RM_DEST )
		    PrintLogFile(log, "Would REMOVE DEST: %s\n",dest);
		PrintLogFile(log, "Would COPY:\n\t< %s\n\t> %s\n",src,dest);
		if ( tfer_mode & TFMD_J_RM_SRC )
		    PrintLogFile(log, "Would REMOVE SRC: %s\n",src);
	    }
	    return ERR_OK;
	}
	else
	{
	    enumError err = tfer_mode & TFMD_J_RM_DEST
				? CopyFileTemp(src,dest,open_mode)
				: CopyFile(src,dest,open_mode);
	    if ( err == ERR_OK )
	    {
		if (log)
		{
		    if ( tfer_mode & TFMD_J_RM_DEST )
			PrintLogFile(log, "REMOVE DEST: %s\n",dest);
		    PrintLogFile(log, "COPY:\n\t< %s\n\t> %s\n",src,dest);
		}

		if ( tfer_mode & TFMD_J_RM_SRC )
		{
		    PRINT("TransferFile(RM_SRC) %s",src);
		    unlink(src);
		    if (log)
			PrintLogFile(log, "REMOVE SRC: %s\n",src);
		}
		return ERR_OK;
	    }
	}
    }

    return ERR_ERROR;
}

#undef LOGPRINT

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MemFile_t		///////////////
///////////////////////////////////////////////////////////////////////////////

bool IsValidMemFile ( MemFile_t *mf )
{
    return mf
	&& ( mf->data || !mf->size )
	&& mf->size >= mf->fend
	&& mf->fend >= mf->fpos;
}

///////////////////////////////////////////////////////////////////////////////

void DumpMemFile
(
    FILE	*f,		// valid output stream
    int		indent,		// indent of output
    MemFile_t	*mf		// valid MemFile_t data
)
{
    DASSERT(f);
    DASSERT(mf);

    indent = NormalizeIndent(indent);
    fprintf(f,"%*sfpos=0x%x/0x%x, size=0x%x/0x%x, zero=%d, valid=%d\n",
	indent,"",
	mf->fpos, mf->fend, mf->size, mf->max_size,
	mf->zero_extend, IsValidMemFile(mf) );
}

///////////////////////////////////////////////////////////////////////////////

bool AssertMemFile
(
    MemFile_t	*mf		// valid MemFile_t data
)
{
    if (IsValidMemFile(mf))
	return true;

    if (mf)
	DumpMemFile(stderr,0,mf);
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u8 * GetDataMemFile
(
    // return NULL on err or a pointer to the data posisiton.

    MemFile_t	*mf,		// valid MemFile_t data
    uint	fpos,		// fpos to read from
    uint	size		// needed data size
)
{
    DASSERT(mf);
    uint end = fpos + size;
    if ( end < fpos || end < size )
	return 0; // integer overflow

    if ( end > mf->max_size )
	return 0; // too large

    if ( end > mf->size )
    {
	// grow buffer
	uint new_size = ALIGN32( end + end/10 + 0x1000, 0x1000 );
	if ( new_size > mf->max_size )
	     new_size = mf->max_size;
	DASSERT( new_size >= end );
	mf->data = REALLOC(mf->data,new_size);
	DASSERT(mf->data);
	memset(mf->data+mf->size,0,new_size-mf->size);
	mf->size = new_size;
    }
    DASSERT( end <= mf->size );

    mf->fpos = fpos;
    if ( mf->fend < end )
	mf->fend = end;
    DASSERT(AssertMemFile(mf));

    return mf->data + fpos;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeMemFile
(
    MemFile_t	*mf,		// structure to initialize
    uint	start_size,	// >0: alloc this for data
    uint	max_size	// >0: file never grows above 'max_size'
)
{
    DASSERT(mf);
    memset(mf,0,sizeof(*mf));
    mf->max_size = max_size ? max_size : M1(mf->max_size);

    if ( start_size > 0 )
    {
	GetDataMemFile( mf, 0, start_size < mf->max_size ? start_size : mf->max_size );
	mf->fend = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void ResetMemFile
(
    MemFile_t	*mf		// valid MemFile_t data
)
{
    DASSERT(mf);
    FREE(mf->data);
    if (mf->err_name_alloced)
	FreeString(mf->err_name);
    InitializeMemFile(mf,0,mf->max_size);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError WriteMemFileAt
(
    MemFile_t	*mf,		// valid MemFile_t data
    uint	fpos,		// fpos to write
    const void	*data,		// data to write
    uint	size		// size to write
)
{
    DASSERT(mf);
    DASSERT(data||!size);

    u8 *dest = GetDataMemFile(mf,fpos,size);
    if (!dest)
    {
	if (mf->err_name)
	    ERROR0(ERR_WRITE_FAILED,"Memory file is full: %s\n",mf->err_name);
	return ERR_WRITE_FAILED;
    }

    mf->eof = false;
    if (size)
	memcpy(dest,data,size);
    mf->fpos = fpos + size;
    if ( mf->fend < mf->fpos )
	mf->fend = mf->fpos;

    DASSERT(AssertMemFile(mf));
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

uint ReadMemFileAt
(
    MemFile_t	*mf,		// valid MemFile_t data
    uint	fpos,		// fpos to read from
    void	*buf,		// destination buffer
    uint	size		// size to read
)
{
    DASSERT(mf);
    DASSERT(buf);

    uint read_count = 0;
    u8 *dest = buf;
    if ( fpos < mf->fend )
    {
	read_count = mf->fend - fpos;
	if ( read_count > size )
	    read_count = size;
	memcpy(dest,mf->data+fpos,read_count);
	mf->fpos = fpos + read_count;
	dest += read_count;
	size -= read_count;
    }

    mf->eof = size > 0;
    if ( mf->eof && mf->zero_extend )
    {
	memset(dest,0,size);
	mf->fpos   += size;
	read_count += size;
    }

    return read_count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError LoadMemFile
(
    MemFile_t	*mf,		// MemFile_t data
    bool	init_mf,	// true: initialize 'mf' first

    ccp		path1,		// NULL or part #1 of path
    ccp		path2,		// NULL or part #2 of path
    size_t	skip,		// >0: skip num of bytes before reading
    size_t	size,		// >0: max size to read; 0: read all (remaining)
    bool	silent		// true: suppress error messages
)
{
    DASSERT(mf);

    if (init_mf)
	InitializeMemFile(mf,0,0);
    else
	ResetMemFile(mf);

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    PRINT("LoadMemFile(silent=%d) %s\n",silent,path);

    struct stat st;
    if (stat(path,&st))
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't get file status: %s\n",path);
	return ERR_CANT_OPEN;
    }
    UseFileAttrib(&mf->fatt,0,&st,0);

    if ( skip >= st.st_size )
	return ERR_OK;

    const uint max_size = st.st_size - skip;
    if ( !size || size > max_size )
	 size = max_size;

    u8 *dest = GetDataMemFile(mf,0,size);
    if (!dest)
    {
	if (!silent)
	    ERROR0(ERR_READ_FAILED,"Memory file is too small: %s\n",path);
	return ERR_READ_FAILED;
    }

    FILE * f = fopen(path,"rb");
    if (!f)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't open file: %s\n",path);
	return ERR_CANT_OPEN;
    }

    if ( skip > 0 )
	fseek(f,skip,SEEK_SET);

    mf->fend = fread(dest,1,size,f);
    fclose(f);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError SaveMemFile
(
    MemFile_t	*mf,		// MemFile_t data

    ccp		path1,		// NULL or part #1 of path
    ccp		path2,		// NULL or part #2 of path
    FileMode_t	fmode,		// mode for file creation
    bool	sparse		// true: enable sparse support
)
{
    DASSERT(mf);

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    PRINT("SaveMemFile(fmode=0x%x,sparse=%d) %u/%u/%u %s\n",
		fmode, sparse, mf->fpos, mf->fend, mf->size, path );

    File_t F;
    enumError err = CreateFile(&F,true,path,fmode);
    if ( err || !F.f )
    {
	ResetFile(&F,0);
	return err;
    }

    if (!sparse)
    {
	const uint written = fwrite(mf->data,1,mf->fend,F.f);
	if ( written != mf->fend )
	{
	    if (!(fmode&FM_SILENT))
		ERROR1(ERR_WRITE_FAILED,"Write to file failed: %s\n",path);
	  err_exit:
	    ResetFile(&F,0);
	    return ERR_WRITE_FAILED;
	}
    }
    else
    {
	enum { BLOCK_SIZE = 512 };
	char zero[BLOCK_SIZE];
	memset(zero,0,sizeof(zero));

	size_t cur_offset = 0;
	uint off = 0;
	while ( off < mf->fend )
	{
	    //--- skip sparse blocks

	    while ( off < mf->fend )
	    {
		uint max = mf->fend - off;
		if ( max > BLOCK_SIZE )
		     max = BLOCK_SIZE;
		if (memcmp(mf->data+off,zero,max))
		    break;
		off += max;
	    }

	    //--- search next sparse block

	    uint start = off;
	    while ( off < mf->fend )
	    {
		uint max = mf->fend - off;
		if ( max > BLOCK_SIZE )
		     max = BLOCK_SIZE;
		if (!memcmp(mf->data+off,zero,max))
		    break;
		off += max;
	    }

	    err = WriteFileAt(&F,&cur_offset,start,mf->data+start,off-start);
	    if (err)
		goto err_exit;
	}

	err = SetFileSize(&F,&cur_offset,mf->fend);
	if (err)
	    goto err_exit;
    }
    return ResetFile(&F,true);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			TraceLog_t			///////////////
///////////////////////////////////////////////////////////////////////////////

bool OpenTraceLog  ( TraceLog_t *tl )
{
    if ( !tl || tl->level < 0 )
	return false;

    if ( !tl->log && tl->fname )
    {
	tl->log = fopen(tl->fname,"wb");
	if (tl->log)
	{
	    fcntl(fileno(tl->log),F_SETFD,FD_CLOEXEC);
	    fprintf(tl->log,"# %s, pid=%d\n",
		PrintTimeByFormat("%F %T %z",time(0)), getpid() );
	}
    }

    if (!tl->log)
    {
	tl->level = -1;
	return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TraceLogText ( TraceLog_t *tl, ccp text )
{
    if (!OpenTraceLog(tl))
	return false;

    if (text)
    {
	fputs(text,tl->log);
	fflush(tl->log);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool TraceLogPrint ( TraceLog_t *tl, ccp format, ... )
{
    if (!OpenTraceLog(tl))
	return false;

    if (format)
    {
	va_list arg;
	va_start(arg,format);
	vfprintf(tl->log,format,arg);
	va_end(arg);
	fflush(tl->log);
    }
    return true;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct LineBuffer_t		///////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __APPLE__
///////////////////////////////////////////////////////////////////////////////

void InitializeLineBuffer ( LineBuffer_t *lb, uint max_buf_size )
{
    DASSERT(lb);
    memset(lb,0,sizeof(*lb));
    InitializeGrowBuffer(&lb->buf,max_buf_size);
}

///////////////////////////////////////////////////////////////////////////////

void ResetLineBuffer ( LineBuffer_t *lb )
{
    DASSERT(lb);
    CloseLineBuffer(lb);
    ResetGrowBuffer(&lb->buf);
    FREE(lb->line);
    lb->line = 0;
    lb->seq_count++;
}

///////////////////////////////////////////////////////////////////////////////

void ClearLineBuffer ( LineBuffer_t *lb )
{
    DASSERT(lb);
    ClearGrowBuffer(&lb->buf);
    lb->used_lines = 0;
    lb->seq_count++;
}

///////////////////////////////////////////////////////////////////////////////

void RedirectLineBuffer
(
    LineBuffer_t	*lb,		// valid line buffer
    FILE		*f,		// output file, if NULL use 'lb->old_fd'
    int			max_lines	// >=0: max number of redirected lines
)
{
    DASSERT(lb);
    if ( lb->buf.used && max_lines )
    {
	if (!f)
	    f = lb->old_fp;
	if (f)
	{
	    if ( max_lines < 0 )
		fwrite(lb->buf.ptr,1,lb->buf.used,f);
	    else
	    {
		mem_t *line;
		uint n = GetMemListLineBuffer(lb,&line,-max_lines);
		while ( n-- > 0 )
		{
		    fprintf(f,"%.*s\n",line->len-1,line->ptr);
		    line++;
		}
//DEL		fprintf(f,"==> %zd %d\n",
//DEL			line[-1].ptr + line[-1].len - (ccp)lb->buf.ptr,
//DEL			lb->buf.used );
	    }

	    fflush(f);
	    if ( f == stdout )
		stdout_seq_count++;
	}
    }
    ClearGrowBuffer(&lb->buf);
    lb->used_lines = 0;
    lb->seq_count++;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

LineBuffer_t * OpenLineBuffer
(
    LineBuffer_t	*lb,		// line buffer; if NULL, malloc() one
    bool		init_lb,	// true: initialize 'lb'
    FILE		**fp_pos,	// NULL or external place of the file pointer
    uint		max_lines,	// max numbers of lines
    uint		max_line_size,	// >0: max size of a single line
    uint		max_buf_size	// >0: max total buffer size
)
{
    if (!lb)
	lb = MALLOC(sizeof(*lb));
    else if (!init_lb)
	ResetLineBuffer(lb);
    InitializeLineBuffer(lb,max_buf_size);

    DASSERT(lb);
    if (fp_pos)
    {
	lb->old_fp = *fp_pos;
	lb->old_fp_pos = fp_pos;
    }

    lb->max_lines = max_lines > 1 ? max_lines : 1;
    lb->max_line_size = max_line_size;
    lb->line = MALLOC(lb->max_lines*sizeof(*lb->line));

    static cookie_io_functions_t funcs =
    {
	0, // read
	(cookie_write_function_t*)WriteLineBuffer,
	0, // seek
	(cookie_close_function_t*)CloseLineBuffer
    };
    lb->fp = fopencookie(lb,"wb",funcs);
    if ( fp_pos && lb->fp )
	*fp_pos = lb->fp;
    return lb;
}

///////////////////////////////////////////////////////////////////////////////

int CloseLineBuffer ( LineBuffer_t *lb )
{
    DASSERT(lb);
    if (lb->fp)
    {
	if ( lb->old_fp_pos && *lb->old_fp_pos == lb->fp )
	    *lb->old_fp_pos = lb->old_fp;
	lb->fp = 0;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ssize_t WriteLineBuffer ( LineBuffer_t *lb, ccp buf, size_t size )
{
    DASSERT(lb);

    if ( lb->redirect > 0 )
    {
	if (lb->old_fp)
	{
	    ssize_t res = fwrite(buf,1,size,lb->old_fp);
	    fflush(lb->old_fp);
	    if ( lb->old_fp == stdout )
		stdout_seq_count++;
	    return res;
	}
    }
    else if (size)
    {
	if ( lb->open_line && lb->buf.used )
	    lb->buf.used--;

	if ( lb->buf.max_size )
	{
	    const int max_used = lb->buf.max_size - size - 1;
	    if ( max_used <= 0 )
	    {
		ClearGrowBuffer(&lb->buf);
		buf  -= max_used;
		size += max_used;
	    }
	    else if ( lb->buf.used > max_used )
	    {
		u8 *ptr = lb->buf.ptr;
		u8 *end = ptr + lb->buf.used;
		ptr += lb->buf.used - max_used;
		while ( ptr < end )
		    if ( *ptr++ == '\n' )
			break;
		DropGrowBuffer(&lb->buf,ptr-lb->buf.ptr);
	    }
	}

	uint new_lines = 0;
	ccp ptr = buf, end = ptr + size;
	while ( ptr < end )
	    if ( *ptr++ == '\n' )
		new_lines++;
	lb->line_count += new_lines;

	InsertGrowBuffer(&lb->buf,buf,size);
	lb->open_line = buf[size-1] != '\n';
	if (lb->open_line)
	    InsertCharGrowBuffer(&lb->buf,'\n');

	FixMemListLineBuffer(lb,true);
	lb->seq_count++;
	PRINT("WRITE %3zu bytes, is_open: %d, total lines: %d\n",
		size, lb->open_line, lb->used_lines );
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void FixMemListLineBuffer ( LineBuffer_t *lb, bool force )
{
    DASSERT(lb);
    if (!lb->buf.used)
	lb->used_lines = 0;
    else if ( force
		|| lb->prev_buf_ptr != lb->buf.ptr
		|| lb->prev_seq_count != lb->seq_count )
    {
	u8 *beg = lb->buf.ptr;
	u8 *ptr = beg + lb->buf.used;

	mem_t *dest = lb->line + lb->max_lines;
	while ( beg < ptr  && dest > lb->line )
	{
	    u8* end = ptr--;
	    while ( beg < ptr && ptr[-1] != '\n' )
		ptr--;

	    dest--;
	    dest->ptr = (ccp)ptr;
	    dest->len = end - ptr;
	}

	lb->used_lines = lb->line + lb->max_lines - dest;
	if ( dest > lb->line )
	    memmove(lb->line,dest,lb->used_lines*sizeof(*dest));

	if ( ptr > beg )
	{
	    // don't use DropGrowBuffer() to hold buf stable
	    lb->buf.used -= ptr - beg;
	    lb->buf.ptr = ptr;
	}

	lb->prev_buf_ptr = lb->buf.ptr;
	lb->prev_seq_count = lb->seq_count;
    }
}

///////////////////////////////////////////////////////////////////////////////

uint GetMemListLineBuffer
(
    // returns the number of elements

    LineBuffer_t	*lb,		// line buffer; if NULL, malloc() one
    mem_t		**res,		// store pointer to first element here
    int			max_lines	// !0: limit lines, <0: count from end
)
{
    DASSERT(lb);

    if ( lb->prev_buf_ptr != lb->buf.ptr )
	FixMemListLineBuffer(lb,false);

    uint pos = 0;
    if (!max_lines)
	max_lines = lb->used_lines;
    else if ( max_lines < 0 )
    {
	max_lines = -max_lines;
	if ( max_lines > lb->used_lines )
	    max_lines = lb->used_lines;
	pos = lb->used_lines - max_lines;
    }
    else if ( max_lines > lb->used_lines )
	max_lines = lb->used_lines;

    if (res)
	*res = lb->line + pos;
    return max_lines;
}

///////////////////////////////////////////////////////////////////////////////

uint PrintLineBuffer
(
    // returns number of written lines

    FILE		*f,		// output stream
    int			indent,		// indention
    LineBuffer_t	*lb,		// line buffer; if NULL, malloc() one
    int			max_lines,	// !0: limit lines, <0: count from end
    ccp			open_line	// not NULL: append this text for an open line
)
{
    DASSERT(f);
    DASSERT(lb);
    indent = NormalizeIndent(indent);

    mem_t *line;
    uint i, n = GetMemListLineBuffer(lb,&line,max_lines);
    for ( i = 1; i <= n; i++, line++ )
	fprintf(f,"%*s%.*s%s\n",
		indent,"",
		line->len-1, line->ptr,
		open_line && i == n && lb->open_line ? open_line : "" );
    return n;
}

///////////////////////////////////////////////////////////////////////////////
#endif // !__APPLE__
///////////////////////////////////////////////////////////////////////////////

//
///////////////////////////////////////////////////////////////////////////////
///////////////			file helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

const char inode_type_char[INTY__N+1] = "?-+SLFBCDR";

//-----------------------------------------------------------------------------

inode_type_t GetInodeType ( int ret_status, mode_t md )
{
    return ret_status	? INTY_NOTFOUND
	: S_ISREG(md)	? INTY_REG
	: S_ISDIR(md)	? INTY_DIR
	: S_ISCHR(md)	? INTY_CHAR
	: S_ISBLK(md)	? INTY_BLOCK
	: S_ISFIFO(md)	? INTY_FIFO
	: S_ISLNK(md)	? INTY_LINK
	: S_ISSOCK(md)	? INTY_SOCK
	:		  INTY_AVAIL;
}

//-----------------------------------------------------------------------------

inode_type_t GetInodeTypeByPath ( ccp path, mode_t *mode )
{
    struct stat st;
    const int ret_status = stat(path,&st);
    if (mode)
	*mode = ret_status ? 0 : st.st_mode;
    return GetInodeType(ret_status,st.st_mode);
}

//-----------------------------------------------------------------------------

inode_type_t GetInodeTypeByFD ( int fd, mode_t *mode )
{
    struct stat st;
    const int ret_status = fstat(fd,&st);
    if (mode)
	*mode = ret_status ? 0 : st.st_mode;
    return GetInodeType(ret_status,st.st_mode);
}

///////////////////////////////////////////////////////////////////////////////

int IsDirectory
(
    // analyse filename (last char == '/') and stat(fname)

    ccp			fname,		// NULL or path
    int			answer_if_empty	// answer, if !fname || !*fname
)
{
    if ( !fname || !*fname )
	return answer_if_empty;

    if ( *fname == '-' && !fname[1] )
	return false;

    if ( fname[strlen(fname)-1] == '/' )
	return true;

    struct stat st;
    return !stat(fname,&st) && S_ISDIR(st.st_mode);
}

///////////////////////////////////////////////////////////////////////////////

int ExistDirectory
(
    // check only real file (stat(fname))

    ccp			fname,		// NULL or path
    int			answer_if_empty	// answer, if !fname || !*fname
)
{
    if ( !fname || !*fname )
	return answer_if_empty;

    if ( *fname == '-' && !fname[1] )
	return false;

    struct stat st;
    return !stat(fname,&st) && S_ISDIR(st.st_mode);
}

///////////////////////////////////////////////////////////////////////////////

bool IsSameFilename ( ccp fn1, ccp fn2 )
{
    if ( !fn1 || !fn2 )
	return false;

    while ( fn1[0] == '.' && fn1[1] == '/' )
	fn1 += 2;
    while ( fn2[0] == '.' && fn2[1] == '/' )
	fn2 += 2;

    return !strcmp(fn1,fn2);
}

///////////////////////////////////////////////////////////////////////////////

// convert type get by readdir() to of struct stat.st_mode
uint ConvertDType2STMode ( uint d_type )
{
    switch (d_type)
    {
	case DT_BLK:	return S_IFBLK;
	case DT_CHR:	return S_IFCHR;
	case DT_DIR:	return S_IFDIR;
	case DT_FIFO:	return S_IFIFO;
	case DT_LNK:	return S_IFLNK;
	case DT_REG:	return S_IFREG;
	case DT_SOCK:	return S_IFSOCK;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool enable_config_search_log = false;

ccp FindConfigFile
(
    // returns NULL or found file (alloced)

    FindConfigFile_t mode,	// bit field: select search destinations
    ccp		share_name,	// NULL or <SHARE>
    ccp		file_name,	// NULL or <FILENAME>
    ccp		prog_ext	// <EXT>, if NULL use '.' + <FILENAME>
)
{
    struct stat st;
    char path[PATH_MAX];

    //--- empty strings like NULL

    if ( share_name && !*share_name ) share_name = ProgInfo.progname;
    if ( file_name  && !*file_name  ) file_name = 0;
    if ( prog_ext   && !*prog_ext   ) prog_ext = 0;

    if ( file_name && share_name )
    {
	//--- the first attemps need FILENAME

	if ( mode & FCF_HOME )
	{
	    ccp home = getenv("HOME");
	    if ( home && *home )
	    {
		snprintf(path,sizeof(path),"%s/.%s/%s",home,share_name,file_name);
		if ( mode & FCF_F_DEBUG )
		    fprintf(stderr,"SEARCH: %s\n",path);
		if ( !stat(path,&st) && S_ISREG(st.st_mode) )
		    return STRDUP(path);
	    }
	}

	if ( mode & FCF_REL_SHARE )
	{
	    ccp pdir = ProgramDirectory();
	    if (pdir)
	    {
		snprintf(path,sizeof(path),"%s/../share/%s/%s",pdir,share_name,file_name );
		if ( mode & FCF_F_DEBUG )
		    fprintf(stderr,"SEARCH: %s\n",path);
		if ( !stat(path,&st) && S_ISREG(st.st_mode) )
		    return STRDUP(path);
	    }
	}

	if ( mode & FCF_LOC_SHARE )
	{
	    snprintf(path,sizeof(path),"/usr/local/share/%s/%s",share_name,file_name);
	    if ( mode & FCF_F_DEBUG )
		fprintf(stderr,"SEARCH: %s\n",path);
	    if ( !stat(path,&st) && S_ISREG(st.st_mode) )
		return STRDUP(path);
	}

	if ( mode & FCF_USR_SHARE )
	{
	    snprintf(path,sizeof(path),"/usr/share/%s/%s",share_name,file_name);
	    if ( mode & FCF_F_DEBUG )
		fprintf(stderr,"SEARCH: %s\n",path);
	    if ( !stat(path,&st) && S_ISREG(st.st_mode) )
		return STRDUP(path);
	}
    }

    if ( mode & (FCF_PROG_SHARE|FCF_PROG) && ( prog_ext || file_name ) )
    {
	ccp ppath = ProgramPathNoExt();
	if (ppath)
	{
	    if ( mode & FCF_PROG_SHARE )
	    {
		ccp pdir = ProgramDirectory();
		noPRINT("pdir=%s\n",pdir);
		if (pdir)
		{
		    ccp fname = strrchr(ppath,'/');
		    fname = fname ? fname+1 : ppath;

		    if (prog_ext)
			snprintf(path,sizeof(path),"%s/share/%s%s",
				pdir, fname, prog_ext );
		    else
			snprintf(path,sizeof(path),"%s/share/%s.%s",
				pdir, fname, file_name );
		    if ( mode & FCF_F_DEBUG )
			fprintf(stderr,"SEARCH: %s\n",path);
		    if ( !stat(path,&st) && S_ISREG(st.st_mode) )
			return STRDUP(path);
		}
	    }

	    if ( mode & FCF_PROG )
	    {
		if (prog_ext)
		    snprintf(path,sizeof(path),"%s%s",ppath,prog_ext);
		else
		    snprintf(path,sizeof(path),"%s.%s",ppath,file_name);
		if ( mode & FCF_F_DEBUG )
		    fprintf(stderr,"SEARCH: %s\n",path);
		if ( !stat(path,&st) && S_ISREG(st.st_mode) )
		    return STRDUP(path);
	    }
	}
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int SearchFiles
(
    ccp			path1,	// not NULL: part #1 of base path
    ccp			path2,	// not NULL: part #2 of base path
    ccp			match,	// not NULL: filter files by MatchPattern()
				// if empty: extract from combined path
    SearchFilesFunc	func,	// callback function, never NULL
    void		*param	// third parameter of func()
)
{
    DASSERT(func);

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);

    if ( match && !*match )
    {
	match = 0;
	if ( path != pathbuf )
	{
	    // path is modified!
	    StringCopyS(pathbuf,sizeof(pathbuf),path);
	    path = pathbuf;
	}

	char *found = strrchr(pathbuf,'/');
	if (found)
	{
	    if ( found == pathbuf )
		path = "/";
	    *found++ = 0;
	    match = *found ? found : 0;
	}
	else
	{
	    match = pathbuf;
	    path = ".";
	}
    }

    if ( !path || !*path )
	path = ".";

    DIR *fdir = opendir(path);
    int stat = errno;

    if (fdir)
    {
	for(;;)
	{
	    struct dirent *dent = readdir(fdir);
	    if (!dent)
		break;

	    if ( !match || MatchPatternFull(match,dent->d_name) )
	    {
		stat = func(path,dent,param);
		if ( stat < 0 )
		    break;
	    }
	}
	closedir(fdir);
    }
    return stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError CreatePath
(
    ccp			path,		// path to create
    bool		is_pure_dir	// true: 'path' don't contains a filename part
)
{
    TRACE("CreatePath(%s)\n",path);

    char buf[PATH_MAX];
    char *dest = StringCopyS(buf,sizeof(buf)-1,path);
    if (is_pure_dir)
    {
	*dest++ = '/';
	*dest = 0;
    }
    dest = buf;

    for(;;)
    {
	// skip slashes
	while ( *dest == '/' )
	    dest++;

	// search end of current directory
	while ( *dest && *dest != '/' )
	    dest++;
	if (!*dest)
	    break;

	*dest = 0;
	if ( mkdir(buf,0777) && errno != EEXIST && !IsDirectory(buf,0) )
	{
	    noTRACE("CREATE-DIR: %s -> err=%d (ENOTDIR=%d)\n",buf,errno,ENOTDIR);
	    if ( errno == ENOTDIR )
	    {
		while ( dest > buf && *dest != '/' )
		    dest--;
		if ( dest > buf )
		    *dest = 0;
	    }
	    return ERROR1( ERR_CANT_CREATE_DIR,
		errno == ENOTDIR
			? "Not a directory: %s\n"
			: "Can't create directory: %s\n", buf );
	}
	TRACE("CREATE-DIR: %s -> OK\n",buf);
	*dest++ = '/';
    }
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError RemoveSource
(
    ccp			fname,		// file to remove
    ccp			dest_fname,	// NULL or dest file name
					//   If real paths are same: don't remove
    bool		print_log,	// true: print a log message
    bool		testmode	// true: don't remove, log only
)
{
    DASSERT(fname);
    if (dest_fname)
    {
	if (!strcmp(fname,dest_fname))
	    return ERR_OK;

	char p1[PATH_MAX], p2[PATH_MAX];
	if ( realpath(fname,p1) && realpath(dest_fname,p2) && !strcmp(p1,p2) )
	    return ERR_OK;
    }

    if ( print_log || testmode )
	fprintf( stdlog ? stdlog : stdout,
		"%sREMOVE %s\n",
		testmode ? "WOULD " : "", fname );

    return !testmode && unlink(fname)
	? ERROR1( ERR_CANT_REMOVE, "Can't remove source file: %s\n", fname )
	: ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

char * FindFilename
(
    // Find the last part of the path, trailing '/' are ignored
    // return poiner to found string

    ccp		path,			// path to analyze, valid string
    uint	* result_len		// not NULL: return length of found string
)
{
    DASSERT(path);

    ccp end = path + strlen(path);
    if ( end > path && end[-1] == '/' )
	 end--;

    ccp start = end;
    while ( start > path && start[-1] != '/' )
	start--;
    if (result_len)
	*result_len = end - start;
    return (char*)start;
}

///////////////////////////////////////////////////////////////////////////////

uint NumberedFilename
(
    // return used index number or 0, if not modified.

    char		* buf,		// destination buffer
    size_t		bufsize,	// size of 'buf'
    ccp			source,		// source filename, may be part of 'buf'
					// if NULL or empty:
    ccp			ext,		// not NULL & not empty: file extension ('.ext')
    uint		ext_mode,	// only relevant, if 'ext' is neither NULL not empty
					//   0: use 'ext' only for detection
					//   1: replace 'source' extension by 'ext'
					//   2: append 'ext' if 'source' differ
					//   3: append 'ext' always
    bool		detect_stdio	// true: don't number "-"
)
{
    //--- analyse stdin/stdout

    char *end = StringCopyS(buf,bufsize,source);
    if ( !source || !*source || detect_stdio && *source == '-' && !source[1] )
	return 0;


    //--- analyse parameter 'ext'

    uint ext_len = 0;
    if ( ext && *ext )
    {
	ext_len = strlen(ext);
	if ( ext_mode >= 3 )
	{
	    // append extension always
	    StringCopyE(end,buf+bufsize,ext);
	}
	else if ( end - buf <= ext_len || memcmp(end-ext_len,ext,ext_len) )
	{
	    // extensions differ
	    if (!ext_mode)
		ext_len = 0;
	    else if  ( ext_mode == 2 )
		StringCopyE(end,buf+bufsize,ext);
	    else
	    {
		char *slash = strrchr(buf,'/');
		char *point = strrchr(buf,'.');
		if ( point && ( !slash || point > slash ) )
		    end = point;
		StringCopyE(end,buf+bufsize,ext);
	    }
	}
    }


    //--- does files exist? => if not: return without numbering

    struct stat st;
    if (stat(buf,&st))
	return 0;



    //--- split filename into dir + name + index + ext

    char temp[PATH_MAX];

    ccp dir = buf, search_dir;
    uint dir_len;
    ccp name = strrchr(buf,'/');
    if (name)
    {
	dir_len = name++ - dir;
	StringCopyS(temp,sizeof(temp),buf);
	if ( dir_len < sizeof(temp) )
	    temp[dir_len] = 0;
	search_dir = temp;
	dir_len++;
    }
    else
    {
	name = buf;
	dir_len = 0;
	dir = "";
	search_dir = ".";
    }

    uint name_len;
    if (ext_len)
    {
	name_len = strlen(name);
	if ( ext_len >= name_len )
	    goto find_ext;

	name_len -= ext_len;
	ext = name + name_len;
    }
    else
    {
     find_ext:
	ext = strrchr(name,'.');
	if ( ext && ext > name )
	{
	    name_len = ext - name;
	    ext_len = strlen(ext);
	}
	else
	{
	    name_len = strlen(name);
	    ext_len = 0;
	    ext = "";
	}
    }

    ccp ptr = name + name_len - 1;
    while ( ptr >= name && *ptr >= '0' && *ptr <= '9' )
	ptr--;
    name_len = ++ptr - name;
    uint index = strtoul(ptr,0,10);

    PRINT("|%s| : |%.*s|%.*s|#%u|%.*s|\n",
	search_dir, dir_len, dir, name_len, name, index, ext_len, ext );


    //--- iterate directory to find highest index

    DIR *fdir = opendir(search_dir);
    if (fdir)
    {
	for(;;)
	{
	    struct dirent *dent = readdir(fdir);
	    if (!dent)
		break;

	    ccp fname = dent->d_name;
	    const uint fname_len = strlen(fname);
	    if ( fname_len > name_len + ext_len
		&& !strncasecmp(name,fname,name_len) )
	    {
		ccp fext = fname + fname_len - ext_len;
		if (!strncasecmp(ext,fext,ext_len))
		{
		    char *end;
		    uint idx = strtoul(fname+name_len,&end,10);
		    if ( end == fext && idx > index )
			index = idx;
		}
	    }
	}
	closedir(fdir);
    }


    //--- create new filename

    index++;
    snprintf(temp,sizeof(temp),"%.*s%u%s",name_len,name,index,ext);
    StringCopyE(buf+dir_len,buf+bufsize,temp);

    return index;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static char * GetBlockDeviceHolderHelper
	( char *buf, char *endbuf, ccp name, ccp sep, int *level )
{
    DASSERT(buf);
    DASSERT(endbuf);
    DASSERT(level);

    char path[200];
    snprintf(path,sizeof(path),"/sys/class/block/%s/holders",name);
    if (IsDirectory(path,0))
    {
	DIR *fdir = opendir(path);
	if (fdir)
	{
	    for(;;)
	    {
		struct dirent *dent = readdir(fdir);
		if (!dent)
		    break;
		if ( dent->d_name[0] != '.' )
		{
		    buf = GetBlockDeviceHolderHelper(buf,endbuf,dent->d_name,sep,level);
		    (*level)++;
		    break;
		}
	    }
	    closedir(fdir);
	}
    }

    return StringCat2E(buf,endbuf,sep,name);
}

//-----------------------------------------------------------------------------

ccp GetBlockDeviceHolder ( ccp name, ccp sep, int *ret_level )
{
    static ParamField_t db = { .free_data = true };

    bool old_found;
    ParamFieldItem_t *item = FindInsertParamField(&db,name,false,0,&old_found);
    DASSERT(item);
    if (!old_found)
    {
	int level = 0;
	char buf[1000];
	ccp end = GetBlockDeviceHolderHelper(buf,buf+sizeof(buf)-strlen(sep)-1,name,sep,&level);
	ccp beg = buf + strlen(sep);
	if ( beg < end )
	{
	    item->num  = level;
	    item->data = MEMDUP(beg,end-beg+1);
	}
    }

    if (ret_level)
	*ret_level = item->num;
    return item->data;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SearchPaths()			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[search_paths_t]]

typedef struct search_paths_t
{
    mem_t		source;		// path to analyse, any string outside
    bool		allow_hidden;	// true: allow hidden dirs and files

    char		*path_buf;	// pointer to complete path, work buffer
    char		*path_star;	// begin of '**' for search_paths_dir()
    char		*path_ptr;	// current end of path
    char		*path_end;	// end of 'path_buf' -1

    SearchPathsFunc	func;		// function to call, never NULL
    void		*param;		// last param for func()

    search_paths_stat_t	status;		// search status
}
search_paths_t;

///////////////////////////////////////////////////////////////////////////////

static void search_paths_hit ( search_paths_t *sp, mem_t path, uint d_type )
{
    DASSERT(sp);
    enumError err = sp->func(path,d_type,sp->param);
    if ( err < 0 )
    {
	err = -err;
	sp->status.abort = true;
    }

    if ( err != ERR_JOB_IGNORED )
    {
	sp->status.hit_count++;
	if ( sp->status.max_err < err )
	     sp->status.max_err = err;
    }

    sp->status.func_count++;
}

///////////////////////////////////////////////////////////////////////////////

static void search_paths_dir ( search_paths_t *sp, ParamField_t *collect, int min, int max )
{
    DASSERT(sp);
    DASSERT(collect);
    PRINT0("%s%s** %d - %d%s\n",colerr->debug,sp->path_buf,min,max,colerr->reset);

    search_paths_t local = *sp;
    char *path_ptr = local.path_ptr;

    DIR *fdir = opendir( *local.path_buf ? local.path_buf: "." );
    local.status.dir_count++;
    if (fdir)
    {
	for(;;)
	{
	    struct dirent *dent = readdir(fdir);
	    if (!dent)
		break;

	    if ( dent->d_name[0] == '.'
		&& (  !local.allow_hidden
		   || !dent->d_name[1]
		   || dent->d_name[1] == '.' && !dent->d_name[2] ))
	    {
		continue;
	    }

	    local.path_ptr = StringCopyE(path_ptr,sp->path_end,dent->d_name);

	    uint st_mode = ConvertDType2STMode(dent->d_type);
	    if (!st_mode)
	    {
		struct stat st;
		if (stat(local.path_buf,&st))
		    continue;
		st_mode = st.st_mode;
	    }
	    if (!S_ISDIR(st_mode))
		continue;

	    DASSERT(local.path_star);
	    DASSERT( local.path_star >= local.path_buf );
	    DASSERT( local.path_star <= local.path_ptr );

	    PRINT0("FOUND: [t=%d,m:%x,add=%d,r=%d] %s\n",
			dent->d_type, st_mode, min<=0, max>0, local.path_star );
	    if ( min <= 0 )
		InsertParamField(collect,local.path_star,false,st_mode,0);
	    if ( max > 0 )
	    {
		*local.path_ptr++ = '/';
		*local.path_ptr = 0;
		search_paths_dir(&local,collect,min-1,max-1);
	    }
	}
	closedir(fdir);
    }

    sp->status = local.status;
}

///////////////////////////////////////////////////////////////////////////////

static void search_paths_helper ( search_paths_t *sp )
{
    ccp wc = FindFirstWildcard(sp->source);
    if (!wc)
    {
	char *end = StringCopyEM(sp->path_ptr,sp->path_end,sp->source.ptr,sp->source.len);
	struct stat st;
	if (!stat(sp->path_buf,&st))
	{
	    mem_t path = { .ptr = sp->path_buf, .len = end - sp->path_buf };
	    search_paths_hit(sp,path,st.st_mode);
	}
	return;
    }

    search_paths_t local = *sp;

    ccp pat = memrchr(local.source.ptr,'/',wc-local.source.ptr);
    pat = pat ? pat+1 : local.source.ptr;

    ccp source_end = local.source.ptr + local.source.len;
    ccp next = memchr(wc,'/',source_end-wc);
    if (!next)
	next = source_end;

    mem_t pattern;
    pattern.len = next - pat;
    pattern.ptr = MEMDUP(pat,pattern.len);
    const bool allow_hidden = local.allow_hidden || pattern.ptr[0] == '.';

    char *path_ptr = StringCopyEM(local.path_ptr,local.path_end,local.source.ptr,pat-local.source.ptr);
    local.path_ptr = path_ptr;

    local.source.ptr = next;
    local.source.len = source_end - next;
    const bool want_dir = local.source.len && local.source.ptr[0] == '/';

    PRINT0("SPLIT: %s  %s%s%s  %.*s  [want_dir=%d]\n",
		 local.path_buf,
		 colerr->highlight, pattern.ptr, colerr->reset,
		 local.source.len, local.source.ptr, want_dir );

    ParamField_t collect;
    InitializeParamField(&collect);
    collect.func_cmp = strcasecmp;


    //--- check for **...

    if ( pattern.ptr[0] == '*' && pattern.ptr[1] == '*' )
    {
	u32 n1 = 0, n2 = 100;
	char *ptr = (char*)pattern.ptr+2;
	if (*ptr)
	{
	    if (isdigit(*ptr))
		n1 = str2ul(ptr,&ptr,10);
	    if (*ptr)
	    {
		if ( *ptr != '-' )
		    goto terminate;
		if ( *++ptr )
		{
		    n2 = str2ul(ptr,&ptr,10);
		    if (*ptr)
			goto terminate;
		}
	    }
	    else
		n2 = n1;
	    if ( n2 > 100 )
		n2 = 100;
	    if ( n1 > n2 )
		goto terminate;
	}

	local.path_star = path_ptr;
	search_paths_dir(&local,&collect,n1,n2);
	PRINT0("%sDIR: %d records for **%s\n",colerr->highlight,collect.used,colerr->reset);

	ParamFieldItem_t *end = collect.field + collect.used;
	for ( ParamFieldItem_t *ptr = collect.field; !local.status.abort && ptr < end; ptr++ )
	{
	    local.path_ptr = StringCopyE(path_ptr,sp->path_end,ptr->key);
	    PRINT0("%sDIR: %s%s\n",colerr->highlight,local.path_buf,colerr->reset);
	    search_paths_helper(&local);
	    if (local.status.abort)
	        break;
	}
	goto terminate;
    }


    //--- scan directory

    DIR *fdir = opendir( *local.path_buf ? local.path_buf: "." );
    local.status.dir_count++;
    if (fdir)
    {
	for(;;)
	{
	    struct dirent *dent = readdir(fdir);
	    if (!dent)
		break;

	    if ( dent->d_name[0] == '.'
		&& (  !allow_hidden
		   || !dent->d_name[1]
		   || dent->d_name[1] == '.' && !dent->d_name[2] ))
	    {
		continue;
	    }

	    uint st_mode = ConvertDType2STMode(dent->d_type);
	    if ( st_mode && want_dir && !S_ISDIR(st_mode) && !S_ISLNK(st_mode) )
		continue;

	    if ( MatchPatternFull(pattern.ptr,dent->d_name) )
	    {
		if (!st_mode)
		{
		    local.path_ptr = StringCopyE(path_ptr,sp->path_end,dent->d_name);
		    struct stat st;
		    if (stat(local.path_buf,&st))
			continue;
		    if ( want_dir && !S_ISDIR(st.st_mode) && !S_ISLNK(st.st_mode) )
			continue;
		    st_mode = st.st_mode;
		}
		PRINT0("BINGO: [%d,%x] %s\n",dent->d_type,st_mode,dent->d_name);
		InsertParamField(&collect,dent->d_name,false,st_mode,0);
	    }
	}
	closedir(fdir);
    }

    if (!local.status.abort)
    {
	ParamFieldItem_t *end = collect.field + collect.used;
	for ( ParamFieldItem_t *ptr = collect.field; !local.status.abort && ptr < end; ptr++ )
	{
	    local.path_ptr = StringCopyE(path_ptr,sp->path_end,ptr->key);
	    if (local.source.len)
	    {
		search_paths_helper(&local);
		if (local.status.abort)
		    break;
	    }
	    else
	    {
		mem_t path = { .ptr = local.path_buf, .len = local.path_ptr - local.path_buf };
		search_paths_hit(&local,path,ptr->num);
	    }
	}
    }

 terminate:;
    ResetParamField(&collect);
    FreeString(pattern.ptr);
    sp->status = local.status;
}

///////////////////////////////////////////////////////////////////////////////

search_paths_stat_t SearchPaths
(
    ccp			path1,		// not NULL: part #1 of base path
    ccp			path2,		// not NULL: part #2 of base path
    bool		allow_hidden,	// allow hiddent directories and files
    SearchPathsFunc	func,		// callback function, never NULL
    void		*param		// last param for func()
)
{
    if (!func)
    {
	search_paths_stat_t stat = { .max_err = ERR_MISSING_PARAM };
	return stat;
    }

    char srcbuf[PATH_MAX+2];
    ccp source = PathCatPP(srcbuf,sizeof(srcbuf),path1,path2);

    char pathbuf[PATH_MAX];
    search_paths_t sp =
    {
	.source.ptr	= source,
	.source.len	= strlen(source),
	.allow_hidden	= allow_hidden,
	.path_buf	= pathbuf,
	.path_ptr	= pathbuf,
	.path_end	= pathbuf + sizeof(pathbuf) - 2,
	.func		= func,
	.param		= param,
    };

    search_paths_helper(&sp);
    PRINT0("abort=%d, max_err = %d, %u dirs scanned, %u func calls, %u hits\n",
		sp.abort, sp.max_err, sp.dir_count, sp.func_count, sp.hit_count );
    return sp.status;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static enumError insert_string_field
(
    mem_t	path,		// full path of existing file, never NULL
    uint	st_mode,	// copy of struct stat.st_mode, see "man 2 stat"
    void	*param		// user defined parameter
)
{
    StringField_t * sf = param;
    DASSERT(sf);
    InsertStringField(sf,path.ptr,false);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

uint InsertStringFieldExpand
	( StringField_t * sf, ccp path1, ccp path2, bool allow_hidden )
{
    DASSERT(sf);
    search_paths_stat_t stat = SearchPaths(path1,path2,allow_hidden,insert_string_field,sf);
    PRINT0("%d %d %d\n",stat.dir_count,stat.func_count,stat.hit_count);
    if (!stat.hit_count)
    {
	char srcbuf[PATH_MAX+2];
	ccp source = PathCatPP(srcbuf,sizeof(srcbuf),path1,path2);
	InsertStringField(sf,source,false);
	stat.hit_count++;
    }
    return stat.hit_count;
}

///////////////////////////////////////////////////////////////////////////////

static enumError append_string_field
(
    mem_t	path,		// full path of existing file, never NULL
    uint	st_mode,	// copy of struct stat.st_mode, see "man 2 stat"
    void	*param		// user defined parameter
)
{
    StringField_t * sf = param;
    DASSERT(sf);
    AppendStringField(sf,path.ptr,false);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

uint AppendStringFieldExpand
	( StringField_t * sf, ccp path1, ccp path2, bool allow_hidden )
{
    DASSERT(sf);
    search_paths_stat_t stat = SearchPaths(path1,path2,allow_hidden,append_string_field,sf);
    PRINT0("%d %d %d\n",stat.dir_count,stat.func_count,stat.hit_count);
    if (!stat.hit_count)
    {
	char srcbuf[PATH_MAX+2];
	ccp source = PathCatPP(srcbuf,sizeof(srcbuf),path1,path2);
	AppendStringField(sf,source,false);
	stat.hit_count++;
    }
    return stat.hit_count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct search_param_field_t
{
    ParamField_t	*pf;
    uint		num;
    cvp			data;
}
search_param_field_t;

//-----------------------------------------------------------------------------

static enumError insert_param_field
(
    mem_t	path,		// full path of existing file, never NULL
    uint	st_mode,	// copy of struct stat.st_mode, see "man 2 stat"
    void	*param		// user defined parameter
)
{
    search_param_field_t *spf = param;
    DASSERT(spf);
    InsertParamField(spf->pf,path.ptr,false,spf->num,spf->data);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

uint InsertParamFieldExpand
	( ParamField_t * pf, ccp path1, ccp path2, bool allow_hidden, uint num, cvp data )
{
    DASSERT(pf);
    search_param_field_t spf = { .pf = pf, .num = num, .data = data };
    search_paths_stat_t stat = SearchPaths(path1,path2,allow_hidden,insert_param_field,&spf);
    PRINT0("%d %d %d\n",stat.dir_count,stat.func_count,stat.hit_count);
    if (!stat.hit_count)
    {
	char srcbuf[PATH_MAX+2];
	ccp source = PathCatPP(srcbuf,sizeof(srcbuf),path1,path2);
	InsertParamField(pf,source,false,num,data);
	stat.hit_count++;
    }
    return stat.hit_count;
}
	
///////////////////////////////////////////////////////////////////////////////

static enumError append_param_field
(
    mem_t	path,		// full path of existing file, never NULL
    uint	st_mode,	// copy of struct stat.st_mode, see "man 2 stat"
    void	*param		// user defined parameter
)
{
    search_param_field_t *spf = param;
    DASSERT(spf);
    AppendParamField(spf->pf,path.ptr,false,spf->num,spf->data);
    return ERR_OK;
}

//-----------------------------------------------------------------------------

uint AppendParamFieldExpand
	( ParamField_t * pf, ccp path1, ccp path2, bool allow_hidden, uint num, cvp data )
{
    DASSERT(pf);
    search_param_field_t spf = { .pf = pf, .num = num, .data = data };
    search_paths_stat_t stat = SearchPaths(path1,path2,allow_hidden,append_param_field,&spf);
    PRINT0("%d %d %d\n",stat.dir_count,stat.func_count,stat.hit_count);
    if (!stat.hit_count)
    {
	char srcbuf[PATH_MAX+2];
	ccp source = PathCatPP(srcbuf,sizeof(srcbuf),path1,path2);
	AppendParamField(pf,source,false,num,data);
	stat.hit_count++;
    }
    return stat.hit_count;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			normalize filenames		///////////////
///////////////////////////////////////////////////////////////////////////////

char * NormalizeFileName
(
    // returns a pointer to the NULL terminator within 'buf'

    char		* buf,		// valid destination buffer
    uint		buf_size,	// size of buf
    ccp			source,		// NULL or source
    bool		allow_slash,	// true: allow '/' in source
    bool		is_utf8,	// true: enter UTF-8 mode
    trailing_slash	slash_mode	// manipulate trailing slash
)
{
    DASSERT(buf);
    DASSERT(buf_size>1);

    char *dest = buf, *end = buf + buf_size - 1;
    TRACE("NormalizeFileName(%s,%d,%d,%d)\n",source,allow_slash,is_utf8,slash_mode);

    if (source)
    {
     #ifdef __CYGWIN__
	if (allow_slash)
	{
	    const int drv_len = IsWindowsDriveSpec(source);
	    if (drv_len)
	    {
		dest = StringCopyE(dest,end,"/cygdrive/c/");
		if ( dest < end )
		    dest[-2] = tolower((int)*source);
		source += drv_len;
	    }
	}
     #endif

	bool skip_space = true;
	while ( *source && dest < end )
	{
	    unsigned char ch = *source++;
	    if ( ch == ':' )
	    {
		if (!skip_space)
		    *dest++ = ' ';
		if ( dest + 2 <= end )
		{
		    *dest++ = '-';
		    *dest++ = ' ';
		}
		skip_space = true;
	    }
	    else
	    {
		if ( isalnum(ch)
			|| ( is_utf8
				? ch >= 0x80
				:    ch == 0xe4 // ä
				  || ch == 0xf6 // ö
				  || ch == 0xfc // ü
				  || ch == 0xdf // ß
				  || ch == 0xc4 // Ä
				  || ch == 0xd6 // Ö
				  || ch == 0xdc // Ü
			    )
			|| strchr("_+-=%'\"$%&#,.!()[]{}<>",ch)
			)
		{
		    *dest++ = ch;
		    skip_space = false;
		}
	     #ifdef __CYGWIN__
		else if ( ( ch == '/' || ch == '\\' ) && allow_slash )
	     #else
		else if ( ch == '/' && allow_slash )
	     #endif
		{
		    if ( dest == buf || dest[-1] != '/' )
			*dest++ = '/';
		    skip_space = false;
		}
		else if (!skip_space)
		{
		    *dest++ = ' ';
		    skip_space = true;
		}
	    }
	}
    }
    if ( dest > buf && dest[-1] == ' ' )
	dest--;

    switch (slash_mode)
    {
     case TRSL_NONE:		// do nothing special
	break;

     case TRSL_AUTO:		// add trailing slash if it is a directory, remove otherwise
	*dest = 0;
	if (IsDirectory(buf,false))
	    goto add_slash;
	// fall through

     case TRSL_REMOVE:		// remove trailing slash always
	if ( dest > buf && dest[-1] == '/' )
	    dest--;
	break;

     case TRSL_ADD_AUTO:	// add trailing slash if it is a directory
	*dest = 0;
	if (!IsDirectory(buf,false))
	    break;
	// fall through

     case TRSL_ADD_ALWAYS:	// add trailing slash always
	add_slash:
	if ( dest > buf && dest < end && dest[-1] != '/' )
	    *dest++ = '/';
	break;
    }

    DASSERT( dest <= end );
    *dest = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint IsWindowsDriveSpec
(
    // returns the length of the found windows drive specification (0|2|3)

    ccp			src		// NULL or valid string
)
{
    if ( src
	&& src[1] == ':'
	&& ( *src >= 'a' && *src <= 'z' || *src >= 'A' && *src <= 'Z' ) )
    {
	if (!src[2])
	    return 2;

	if ( src[2] == '/' || src[2] == '\\' )
	    return 3;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint NormalizeFilenameCygwin
(
    // returns a pointer to the NULL terminator within 'buf'

    char		* buf,		// valid destination buffer
    uint		buf_size,	// size of buf
    ccp			src		// NULL or source
)
{
    static char prefix[] = "/cygdrive/";

    if ( buf_size < sizeof(prefix) + 5 || !src )
    {
	if ( buf && buf_size )
	    *buf = 0;
	return 0;
    }

    char * end = buf + buf_size - 1;
    char * dest = buf;

    if ( ( *src >= 'a' && *src <= 'z' || *src >= 'A' && *src <= 'Z' )
	&& src[1] == ':'
	&& ( src[2] == 0 || src[2] == '/' || src[2] == '\\' ))
    {
	memcpy(buf,prefix,sizeof(prefix));
	dest = buf + sizeof(prefix)-1;
	*dest++ = tolower((int)*src); // cygwin needs the '(int)'
 #ifdef __CYGWIN__
	// this can only be checked with real Cygwin.
	*dest = 0;
	if (IsDirectory(buf,false))
	{
	    *dest++ = '/';
	    src += 2;
	    if (*src)
		src++;
	}
	else
	    dest = buf;
 #else //
	*dest++ = '/';
	src += 2;
	if (*src)
	    src++;
 #endif

    }
    DASSERT( dest < buf + buf_size );

    while ( dest < end && *src )
	if ( *src == '\\' )
	{
	    *dest++ = '/';
	    src++;
	}
	else
	    *dest++ = *src++;

    *dest = 0;
    DASSERT( dest < buf + buf_size );
    return dest - buf;
}

///////////////////////////////////////////////////////////////////////////////

exmem_t GetNormalizeFilenameCygwin
(
    // returns an object. Call FreeExMem(RESULT) to possible alloced memory.

    ccp		source,		// NULL or source
    bool	try_circ	// use circ-buffer, if result is small enough
)
{
    char buf[PATH_MAX];
    const uint len = NormalizeFilenameCygwin(buf,sizeof(buf),source);
    return AllocExMemS(buf,len,try_circ,source,-1);
}

///////////////////////////////////////////////////////////////////////////////

char * AllocNormalizedFilenameCygwin
(
    // returns an alloced buffer with the normalized filename

    ccp		source,		// NULL or source
    bool	try_circ	// use circ-buffer, if result is small enough
)
{
    char buf[PATH_MAX];
    const uint len = NormalizeFilenameCygwin(buf,sizeof(buf),source);
    return AllocCircBuf(buf,len,try_circ);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			search file & config		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetSearchFile ( search_file_list_t *sfl )
{
    if (sfl)
    {
	if (sfl->list)
	{
	    uint i;
	    search_file_t *sf =  sfl->list;
	    for ( i = 0; i < sfl->used; i++, sf++ )
		if (sf->alloced)
		    FreeString(sf->fname);
	    FREE(sfl->list);
	}

	ResetEML(&sfl->symbols);
	InitializeSearchFile(sfl);
    }
}

///////////////////////////////////////////////////////////////////////////////

search_file_t * AppendSearchFile
(
    search_file_list_t *sfl,	// valid search list, new files will be appended
    ccp		fname,		// path+fname to add
    CopyMode_t	copy_mode,	// copy mode for 'fname'
    ccp		append_if_dir	// append this if 'fname' is a directory
)
{
    search_file_t *res = 0;
    if ( fname && *fname )
    {
 #if 0
	ccp rpath = realpath(fname,0);
	if (rpath)
	{
	    if ( copy_mode == CPM_MOVE )
		FreeString(fname);
	    fname = rpath;
	    copy_mode = CPM_MOVE;
	}
 #endif

	uint i;
	search_file_t *sf =  sfl->list;
	for ( i = 0; i < sfl->used; i++, sf++ )
	{
	    if (!strcmp(sf->fname,fname))
	    {
		res = sf;
		goto end;
	    }
	}
	DASSERT( i == sfl->used );


	//-- check for directory

	const inode_type_t itype = GetInodeTypeByPath(fname,0);
	if ( itype == INTY_DIR && append_if_dir )
	{
	    char buf[PATH_MAX];
	    ccp fname2 = PathCatPP(buf,sizeof(buf),fname,append_if_dir);
	    res = AppendSearchFile(sfl,fname2,CPM_COPY,0);
	}
	else
	{
	    if ( sfl->used == sfl->size )
	    {
		sfl->size += sfl->size/8 + 20;
		sfl->list = REALLOC(sfl->list,sfl->size*sizeof(*sfl->list));
	    }

	    res = sfl->list + sfl->used++;
	    memset(res,0,sizeof(*res));
	    res->itype = itype;
	    res->fname = CopyData(fname,strlen(fname)+1,copy_mode,&res->alloced);
	    fname = 0;
	}
    }

 end:
    if ( fname && copy_mode == CPM_MOVE )
	FreeString(fname);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void DumpSearchFile ( FILE *f, int indent,
			const search_file_list_t *sfl, bool show_symbols, ccp info )
{
    if ( f && sfl && ( sfl->used || show_symbols && sfl->symbols.used ))
    {
	indent = NormalizeIndent(indent);
	fprintf(f,"%*s%s%s" "SearchList %p, N=%u/%u\n",
		indent,"", info ? info : "", info ? " : " : "",
		sfl, sfl->used, sfl->size );

	char buf[20];
	const int fw_idx = indent
			+ snprintf(buf,sizeof(buf),"%d",sfl->used)
			+ ( show_symbols ? 3 : 1 );

	uint idx = 0;
	search_file_t *ptr, *end = sfl->list + sfl->used;
	for ( ptr = sfl->list; ptr < end; ptr++, idx++ )
	    fprintf(f,"%*d [%c,%c:%02x] %s\n",
		fw_idx, idx,
		ptr->alloced ? 'A' : '-',
		inode_type_char[ptr->itype], ptr->hint, ptr->fname );

	if (show_symbols)
	    DumpEML(f,indent+2,&sfl->symbols,"Symbols");
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int add_search
(
    // returns 0 if !stop_if_found, 2 if added+found, 1 if added, 0 else

    search_file_list_t *sfl,	// valid data
    ccp		config_fname,	// default filename (without path) of config file
    ccp		*list,		// NULL or list of filenames
    int		n_list,		// num of 'list' elements, -1:null terminated list
    ccp		prefix,		// prefix for relative filenames
    uint	hint,		// attributes
    int		stop_if_found	// >0: stop if file found
)
{
    DASSERT(sfl);
    if (!list)
	return 0;

    if ( n_list < 0 )
    {
	ccp *ptr;
	for ( ptr = list; *ptr; ptr++ )
	    ;
	n_list = ptr - list;
    }
    if (!n_list)
	return 0;

    int ret_val = 0;
    char buf[PATH_MAX];
    for ( ; n_list > 0; n_list--, list++ )
    {
	if (!*list)
	    continue;

	ccp path = **list == '/' || !prefix || !*prefix
		? *list
		: PathCatPP(buf,sizeof(buf),prefix,*list);
	exmem_t res = ResolveSymbolsEML(&sfl->symbols,path);
	search_file_t *sf = AppendSearchFile( sfl, res.data.ptr,
				res.is_alloced ? CPM_MOVE : CPM_COPY, config_fname );
	if (sf)
	{
	    sf->hint |= hint;
	    if ( sf->itype == INTY_REG && stop_if_found > 0 )
		return 2;
	    ret_val = 1;
	}
    }

    return stop_if_found > 0 ? ret_val : 0;
}

//-----------------------------------------------------------------------------

bool SearchConfig
(
    // for all paths:
    //	/...		is an absolute path
    //  $(home)/...	path relative to getenv("HOME")
    //  $(xdg_home)/...	path relative to first path of getenv("XDG_CONFIG_HOME")
    //  $(xdg_etc)/...	path relative to first path of getenv("XDG_CONFIG_DIRS")
    //  $(etc)/...	path relative to /etc directory
    //  $(install)/...	path relative to installation directory = ProgramDirectory()
    //  $(NAME)/...	path relative to symbol in sfl->symbols
    //  xx		relative paths otherwise

    search_file_list_t *sfl,
			// valid search list, new paths will be appended
			// sfl->symbols: home, etc and install are added (not replaced)
			//	It is used to resolve all $(NAME) references.

    ccp config_fname,	// default filename (without path) of config file

    ccp *option,	// NULL or filenames by option		=> CONF_HINT_OPT
     int n_option,	//  num of 'option' elements, -1:null terminated list
    ccp *xdg_home,	// NULL or $(xdg_home) based paths	=> CONF_HINT_HOME
     int n_xdg_home,	//  num of 'home' elements, -1:null terminated list
    ccp *home,		// NULL or $(home) based paths		=> CONF_HINT_HOME
     int n_home,	//  num of 'home' elements, -1:null terminated list
    ccp *etc,		// NULL or $(etc) based paths		=> CONF_HINT_ETC
     int n_etc,		//  num of 'etc' elements, -1:null terminated list
    ccp *install,	// NULL or $(install) based paths 	=> CONF_HINT_INST
     int n_install,	//  num of 'install' elements, -1:null terminated list
    ccp *misc,		// NULL or absolute paths		=> CONF_HINT_MISC
     int n_misc,	//  num of 'misc' elements, -1:null terminated list

    int stop_mode	// >0: stop if found, >1: stop on option
)
{
    // ==> see: https://wiki.archlinux.org/index.php/XDG_Base_Directory

    DASSERT(sfl);
    AddStandardSymbolsEML(&sfl->symbols,false);

    int stat = add_search(sfl,config_fname,option,n_option,0,CONF_HINT_OPT,stop_mode);
    if ( stat > 1 )
	return true;
    if ( stop_mode > 1 && stat )
	return false;

    if (add_search(sfl,config_fname,xdg_home,n_xdg_home,"$(xdg_home)",CONF_HINT_HOME,stop_mode)>1)
	return true;

    if (add_search(sfl,config_fname,home,n_home,"$(home)",CONF_HINT_HOME,stop_mode)>1)
	return true;

    if (add_search(sfl,config_fname,etc,n_etc,"$(xdg_etc)",CONF_HINT_ETC,stop_mode)>1)
	return true;

    if (add_search(sfl,config_fname,etc,n_etc,"$(etc)",CONF_HINT_ETC,stop_mode)>1)
	return true;

    if (add_search(sfl,config_fname,install,n_install,"$(install)",CONF_HINT_INST,stop_mode)>1)
	return true;

    return add_search(sfl,config_fname,misc,n_misc,0,CONF_HINT_MISC,stop_mode)>1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    FDList_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void ClearFDList ( FDList_t *fdl )
{
    DASSERT(fdl);

    fdl->max_fd = -1;
    FD_ZERO(&fdl->readfds);
    FD_ZERO(&fdl->writefds);
    FD_ZERO(&fdl->exceptfds);

    fdl->poll_used = 0;
    fdl->n_sock = 0;

    fdl->now_usec = GetTimeUSec(false);
    fdl->timeout_usec = M1(fdl->timeout_usec);
    fdl->timeout_nsec = M1(fdl->timeout_nsec);
}

///////////////////////////////////////////////////////////////////////////////

void InitializeFDList ( FDList_t *fdl, bool use_poll )
{
    DASSERT(fdl);
    memset(fdl,0,sizeof(*fdl));
    fdl->use_poll = use_poll;
    ClearFDList(fdl);
}

///////////////////////////////////////////////////////////////////////////////

void ResetFDList ( FDList_t *fdl )
{
    DASSERT(fdl);
    FREE(fdl->poll_list);
    InitializeFDList(fdl,fdl->use_poll);
}

///////////////////////////////////////////////////////////////////////////////

struct pollfd * AllocFDList ( FDList_t *fdl, uint n )
{
    DASSERT(fdl);
    uint min_size = fdl->poll_used + n;
    if ( fdl->poll_size < min_size )
    {
	fdl->poll_size = ( min_size/0x18 + 2 ) * 0x20;
	DASSERT( fdl->poll_size >= min_size );
	fdl->poll_list = REALLOC(fdl->poll_list,fdl->poll_size*sizeof(*fdl->poll_list));
    }
    DASSERT( fdl->poll_used + n <= fdl->poll_size );

    struct pollfd *ptr = fdl->poll_list + fdl->poll_used;
    fdl->poll_used += n;
    memset(ptr,0,n*sizeof(*ptr));
    return ptr;
}

///////////////////////////////////////////////////////////////////////////////

void AnnounceFDList ( FDList_t *fdl, uint n )
{
    if (fdl->use_poll)
    {
	AllocFDList(fdl,n);
	fdl->poll_used -= n;
    }
}

///////////////////////////////////////////////////////////////////////////////

uint AddFDList
(
    // returns the pool-index if available, ~0 otherwise

    FDList_t	*fdl,	// valid socket list
    int		sock,	// socket to add
    uint	events	// bit field: POLLIN|POLLPRI|POLLOUT|POLLRDHUP|...
)
{
    events &= ~(POLLERR|POLLHUP|POLLNVAL); // only result!
    if ( sock == -1 || !events )
	return ~0;

    fdl->n_sock++;
    if ( fdl->max_fd < sock )
	fdl->max_fd = sock;

    DASSERT(fdl);
    if (fdl->use_poll)
    {
	struct pollfd *pp = AllocFDList(fdl,1);
	DASSERT(pp);
	pp->fd = sock;
	pp->events = events;
	return pp - fdl->poll_list;
    }

    noPRINT("ADD-SOCK %d = %x\n",sock,events);
    if ( events & POLLIN )
	FD_SET(sock,&fdl->readfds);
    if ( events & POLLOUT )
	FD_SET(sock,&fdl->writefds);
    if ( events & POLLPRI )
	FD_SET(sock,&fdl->exceptfds);
    return ~0;
}

///////////////////////////////////////////////////////////////////////////////

uint GetEventFDList
(
    // returns bit field: POLLIN|POLLOUT|POLLERR|...

    FDList_t	*fdl,		// valid socket list
    int		sock,		// socket to look for
    uint	poll_index	// if use_poll: use the index for a fast search
)
{
    if ( sock == -1 )
	return 0;

    DASSERT(fdl);
    if (fdl->use_poll)
    {
	const struct pollfd *pp = fdl->poll_list;
	if ( poll_index < fdl->poll_used && pp[poll_index].fd == sock )
	    return pp[poll_index].revents;

	const struct pollfd *pend = pp + fdl->poll_used;
	for ( ; pp < pend; pp++ )
	    if ( pp->fd == sock )
		return pp->revents;
	return 0;
    }

    uint revents = 0;
    if (FD_ISSET(sock,&fdl->readfds))
	revents |= POLLIN;
    if (FD_ISSET(sock,&fdl->writefds))
	revents |= POLLOUT;
    if (FD_ISSET(sock,&fdl->exceptfds))
	revents |= POLLPRI;
    return revents;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static s_usec_t prepare_wait_fdl ( FDList_t *fdl )
{
    DASSERT(fdl);
    const s_usec_t now_usec = GetTimeUSec(false);

    if (fdl->timeout_nsec)
    {
	const u_usec_t wait_until
	    = ( (s_usec_t)fdl->timeout_nsec - (s_usec_t)GetTimerNSec() )
			/ NSEC_PER_USEC + now_usec + 1;
	if ( !fdl->timeout_usec || fdl->timeout_usec > wait_until )
	     fdl->timeout_usec = wait_until;
    }

    if ( fdl->min_wait_usec && fdl->timeout_usec )
    {
	const u_usec_t wait_until = now_usec + fdl->min_wait_usec;
	if ( fdl->timeout_usec < wait_until )
	     fdl->timeout_usec = wait_until;
    }

    return now_usec;
}

//-----------------------------------------------------------------------------

static void finish_wait_fdl ( FDList_t *fdl, u_usec_t start_usec )
{
    DASSERT(fdl);
    UpdateCurrentTime();
    fdl->now_usec	= current_time.usec;
    fdl->last_wait_usec	= fdl->now_usec - start_usec;
    fdl->wait_usec	+= fdl->last_wait_usec;
    fdl->wait_count++;

    UpdateCpuUsageIncrement();
}

///////////////////////////////////////////////////////////////////////////////

int WaitFDList
(
    FDList_t	*fdl		// valid socket list
)
{
    DASSERT(fdl);

    const s_usec_t now_usec = prepare_wait_fdl(fdl);

    int stat;
    if (fdl->use_poll)
    {
	int timeout;
	if ( !fdl->timeout_usec )
	    timeout = -1;
	else if ( fdl->timeout_usec > now_usec )
	{
	    u_msec_t delta = ( fdl->timeout_usec - now_usec ) / USEC_PER_MSEC;
	    timeout = delta < 0x7fffffff ? delta : 0x7fffffff;
	    if (!timeout)
		timeout = 1;
	}
	else
	    timeout = 0;

	if (fdl->debug_file)
	{
	    fprintf(fdl->debug_file,"POLL: timeout=%d\n",timeout);
	    fflush(fdl->debug_file);
	}
	stat = poll( fdl->poll_list, fdl->poll_used, timeout );
    }
    else
    {
	struct timeval tv, *ptv = &tv;
	if ( !fdl->timeout_usec )
	    ptv = NULL;
	else if ( fdl->timeout_usec > now_usec )
	{
	    const u_usec_t delta = fdl->timeout_usec - now_usec;
	    if ( delta > USEC_PER_YEAR )
	    {
		tv.tv_sec  = SEC_PER_YEAR;
		tv.tv_usec = 0;
	    }
	    else if ( delta > USEC_PER_MSEC )
	    {
		tv.tv_sec  = delta / USEC_PER_SEC;
		tv.tv_usec = delta % USEC_PER_SEC;
	    }
	    else
	    {
		tv.tv_sec  = 0;
		tv.tv_usec = USEC_PER_MSEC;
	    }
	}
	else
	    tv.tv_sec = tv.tv_usec = 0;

	if (fdl->debug_file)
	{
	    if (ptv)
		fprintf(fdl->debug_file,"# SELECT: usec=%lld, Δnow=%lld, timeout=%llu.%06llu\n",
				fdl->timeout_usec, fdl->timeout_usec - now_usec,
				(u64)ptv->tv_sec, (u64)ptv->tv_usec );
	    else
		fprintf(fdl->debug_file,"# SELECT: usec=%lld, Δnow=%lld, timeout=NULL\n",
				fdl->timeout_usec, fdl->timeout_usec - now_usec );
	    fflush(fdl->debug_file);
	}

	stat = select( fdl->max_fd+1, &fdl->readfds, &fdl->writefds,
			&fdl->exceptfds, ptv );
    }

    finish_wait_fdl(fdl,now_usec);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int PWaitFDList
(
    FDList_t		*fdl,		// valid socket list
    const sigset_t	*sigmask	// NULL or signal mask
)
{
    DASSERT(fdl);

 #ifdef __APPLE__
    // [[2do]] ??? untested work around
    if (fdl->use_poll)
    {
	sigset_t origmask;
	if (sigmask)
	    sigprocmask(SIG_SETMASK,sigmask,&origmask);
	const int stat = WaitFDList(fdl);
	if (sigmask)
	    sigprocmask(SIG_SETMASK,&origmask,NULL);
	return stat;
    }
 #endif

    const u_usec_t now_usec = prepare_wait_fdl(fdl);

    struct timespec ts, *pts = &ts;
    if ( fdl->timeout_usec > now_usec )
    {
	const u64 delta = fdl->timeout_usec - now_usec;
	if ( delta > 1000000ull * 0x7fffffffull )
	    pts = NULL;
	else if ( delta > USEC_PER_MSEC )
	{
	    ts.tv_sec  = delta / 1000000;
	    ts.tv_nsec = delta % 1000000 * 1000;
	}
	else
	{
	    ts.tv_sec  = 0;
	    ts.tv_nsec = NSEC_PER_MSEC;
	}
    }
    else
	ts.tv_sec = ts.tv_nsec = 0;

 #ifdef __APPLE__
    const int stat = pselect( fdl->max_fd+1, &fdl->readfds, &fdl->writefds,
				&fdl->exceptfds, pts, sigmask );
 #else
    const int stat = fdl->use_poll
		? ppoll( fdl->poll_list, fdl->poll_used, pts, sigmask )
		: pselect( fdl->max_fd+1, &fdl->readfds, &fdl->writefds,
				&fdl->exceptfds, pts, sigmask );
 #endif

    finish_wait_fdl(fdl,now_usec);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

mem_t CheckUnixSocketPathMem
(
    mem_t	src,	    // NULL or source path to analyse
    int		tolerance   // <1: 'unix:', 'file:', '/', './' and '../' detected
			    //  1: not 'NAME:' && at relast one '/'
			    //  2: not 'NAME:'
)
{
    if ( src.ptr && src.len )
    {
	ccp ptr = src.ptr;
	ccp end = ptr + src.len;

	switch (*ptr)
	{
	    case '/':
		return src;

	    case '.':
		{
		    ccp s = ptr+1;
		    if ( s < end && *s == '.' )
			s++;
		    if ( s < end && *s == '/' )
			return src;
		}
		break;

	    case 'f':
		if ( end-ptr >= 5 && !memcmp(ptr,"file:",5) )
		    return RightMem(src,-5);

	    case 'u':
		if ( end-ptr >= 5 && !memcmp(ptr,"unix:",5) )
		    return RightMem(src,-5);
	    break;
	}

	if ( tolerance > 0 )
	{
	    while ( ptr < end && isalnum((int)*ptr) )
		ptr++;
	    if ( ptr < end && *ptr != ':' && ( tolerance > 1 || strchr(ptr,'/')) )
		return BehindMem(src,ptr);
	}
    }

    return NullMem;
}

///////////////////////////////////////////////////////////////////////////////

ccp CheckUnixSocketPath
(
    ccp		src,	    // NULL or source path to analyse
    int		tolerance   // <1: 'unix:', 'file:', '/', './' and '../' detected
			    //  1: not 'NAME:' && at relast one '/'
			    //  2: not 'NAME:'
)
{
    mem_t res = CheckUnixSocketPathMem(MemByString(src),tolerance);
    return res.ptr;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Catch Output			///////////////
///////////////////////////////////////////////////////////////////////////////

int CatchIgnoreOutput
(
    struct CatchOutput_t *ctrl,		// control struct incl. data
    int			call_mode	// 0:init, 1:new data, 2:term
)
{
    DASSERT(ctrl);
    ClearGrowBuffer(&ctrl->buf);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void ResetCatchOutput ( CatchOutput_t *co, uint n )
{
    while ( n-- > 0 )
    {
	if ( co->pipe_fd[0] != -1 )
	{
	    close(co->pipe_fd[0]);
	    co->pipe_fd[0] = -1;
	}

	if ( co->pipe_fd[1] != -1 )
	{
	    close(co->pipe_fd[1]);
	    co->pipe_fd[1] = -1;
	}

	ResetGrowBuffer(&co->buf);
	co++;
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError CatchOutput
(
    ccp			command,	// command to execute
    int			argc,		// num(arguments) in 'argv'; -1: argv is NULL terminated
    char *const*	argv,		// list of arguments

    CatchOutputFunc	stdout_func,	// NULL or function to catch stdout
    CatchOutputFunc	stderr_func,	// NULL or function to catch stderr
    void		*user_ptr,	// NULL or user defined parameter
    bool		silent		// true: suppress error messages
)
{
    if ( !command || !*command )
	return ERR_NOTHING_TO_DO;


    //--- setup streams & pipes

    uint n_catch_out = 0;
    CatchOutput_t catch_out[2];
    memset(catch_out,0,sizeof(catch_out));

    uint i;
    for ( i = 1; i <= 2; i++ )
    {
	CatchOutputFunc func = i==1 ? stdout_func : stderr_func;
	if (!func)
	    continue;

	CatchOutput_t *co = catch_out + n_catch_out++;
	co->mode = i;
	co->func = func;
	co->user_ptr = user_ptr;
	InitializeGrowBuffer(&co->buf,16*KiB);

	if ( pipe(co->pipe_fd) == -1 )
	{
	    ResetCatchOutput(catch_out,n_catch_out);
	    if (!silent)
		ERROR1(ERR_CANT_CREATE,"Can't create pipe\n");
	    return ERR_CANT_CREATE;
	}
    }


    //--- setup arguments

    if (!argv)
	argc = 0;
    else if ( argc < 0 )
	for ( argc = 0; argv[argc]; argc++ )
	    ;

    const uint local_args_size = 100;
    char *local_args[local_args_size+2];
    char **args = local_args;
    if ( argc >= local_args_size )
	args = MALLOC((argc+2)*sizeof(*args));

    char **destarg = args;
    *destarg++ = (char*)command;

    for ( i = 0; i < argc; i++ )
	*destarg++ = argv[i];
    *destarg = 0;

    #ifdef TEST
    {
	printf("EXEC '%s' [argc=%u]\n",command,argc);
	uint i;
	for ( i = 0; i < argc+2; i++ )
	    printf("%3d: %s\n",i,args[i]);
    }
    #endif


    //--- fork

    enumError err = ERR_OK;

    int fork_stat = fork();
    if ( fork_stat < 0 )
    {
	if (!silent)
	    ERROR1(ERR_ERROR,"Can't fork: %s\n",command);
	err = ERR_ERROR;
    }
    else if (fork_stat)
    {
	PRINT("WAIT FOR %d\n",fork_stat);

	CatchOutput_t *co, *co_end = catch_out + n_catch_out;
	for ( co = catch_out; co < co_end; co++ )
	{
	    DASSERT(co->func);
	    co->func(co,0);
	    close(co->pipe_fd[1]);  // close childs (write) end
	    co->pipe_fd[1] = -1;
	}

	FDList_t fdl;
	InitializeFDList(&fdl,false);
	char buf[0x1000];

	for(;;)
	{
	    ClearFDList(&fdl);
	    fdl.timeout_usec = GetTimeUSec(false) + 60*USEC_PER_SEC;
	    for ( co = catch_out; co < co_end; co++ )
		AddFDList(&fdl,co->pipe_fd[0],POLLIN);
	    if ( fdl.max_fd == -1 )
		break;

	    const int stat = WaitFDList(&fdl);
	    if ( stat > 0 )
	    {
		for ( co = catch_out; co < co_end; co++ )
		{
		    int fd = co->pipe_fd[0];
		    uint event = GetEventFDList(&fdl,fd,0);
		    if ( event & POLLIN )
		    {
			ssize_t read_size = read(fd,buf,sizeof(buf));
			PRINT("READ %d[%d], %zd bytes\n",co->mode,fd,read_size);
			if ( read_size > 0 )
			{
			    PrepareGrowBuffer(&co->buf,read_size,true);
			    InsertGrowBuffer(&co->buf,buf,read_size);
			    if (co->func)
				co->func(co,1);
			}
			else
			{
			    close(fd);
			    co->pipe_fd[0] = -1;
			}
		    }
		}
	    }
	}
	ResetFDList(&fdl);

	int wait_stat;
	for(;;)
	{
	    int pid = waitpid(fork_stat,&wait_stat,0);

	    PRINT("RESUME: %d = %x: exited=%d, wait_stat=%d [%s]\n",
		wait_stat, wait_stat, WIFEXITED(wait_stat),
		WEXITSTATUS(wait_stat), GetErrorName(WEXITSTATUS(wait_stat),"?") );
	    if ( pid == fork_stat )
		break;
	}

	for ( co = catch_out; co < co_end; co++ )
	    if (co->func)
		co->func(co,2);
	ResetCatchOutput(catch_out,n_catch_out);

	if (WIFEXITED(wait_stat))
	    return WEXITSTATUS(wait_stat);
	if (!silent)
	    ERROR1(ERR_SYNTAX,"Child teminated with error: %s\n",command);
	return ERR_SYNTAX;
    }
    else
    {
	PRINT("CHILD\n");

	CatchOutput_t *co, *co_end = catch_out + n_catch_out;
	for ( co = catch_out; co < co_end; co++ )
	{
	    close(co->pipe_fd[0]);  // close parents (read) end
	    co->pipe_fd[0] = -1;

	    if ( dup2(co->pipe_fd[1],co->mode) == -1 )
	    {
		if (!silent)
		    ERROR1(ERR_CANT_CREATE,"dup2(,%d) failed: %s\n",
				co->mode, command );
		_exit(ERR_CANT_CREATE);
	    }
	}

	execvp(command,args);
	if (!silent)
	    ERROR1(ERR_CANT_CREATE,"Can't create process: %s\n",command);
	_exit(ERR_CANT_CREATE);
    }

    if ( args != local_args )
	FREE(args);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CatchOutputLine
(
    ccp			command_line,	// command line to execute
    CatchOutputFunc	stdout_func,	// NULL or function to catch stdout
    CatchOutputFunc	stderr_func,	// NULL or function to catch stderr
    void		*user_ptr,	// NULL or user defined parameter
    bool		silent		// true: suppress error messages
)
{
    SplitArg_t sa;
    ScanSplitArg(&sa,true,command_line,0,0);

    enumError err = ERR_SYNTAX;
    if ( sa.argc > 1 )
	err = CatchOutput( sa.argv[1], sa.argc-2, sa.argv+2,
				stdout_func, stderr_func, user_ptr, silent );
    ResetSplitArg(&sa);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan sections			///////////////
///////////////////////////////////////////////////////////////////////////////

bool FindSection
(
    // search file until next section found
    //  => read line and store data into 'si'
    //  => return TRUE, if section found

    SectionInfo_t	*si,		// valid section info
    char		*buf,		// use this buffer for line scanning
    uint		buf_size,	// size of 'buf'
    bool		scan_buf	// true: buffer contains already a valid
					//       and NULL terminated line
)
{
    DASSERT(si);
    DASSERT(si->f);
    DASSERT(buf);
    DASSERT( buf_size > 10 );

    FreeString(si->section);
    FreeString(si->path);
    si->section = si->path = 0;
    si->index = -1;
    ResetParamField(&si->param);

    buf_size--;
    for(;;)
    {
	if ( !scan_buf && !fgets(buf,buf_size,si->f) )
	    break;
	buf[buf_size] = 0;
	scan_buf = false;

	char *ptr = buf;
	while ( *ptr > 0 && *ptr <= ' ' )
	    ptr++;

	if ( *ptr != '[' )
	    continue;

	ccp sect = ++ptr;
	while ( isalnum((int)*ptr) || *ptr == '-' || *ptr == '_' )
	    ptr++;

	ccp path = ptr;
	if ( *ptr == '/' )
	{
	    *ptr++ = 0;
	    path = ptr;
	    while ( *ptr > ' ' && *ptr != ':' && *ptr != ']' )
		ptr++;
	}

	int index = -1;
	if ( *ptr == ':' )
	{
	    *ptr++ = 0;
	    index = str2ul(ptr,&ptr,10);
	}

	if ( *ptr != ']' )
	    continue;
	*ptr = 0;

	si->section = STRDUP(sect);
	if (*path)
	    si->path = STRDUP(path);
	si->index = index;
	return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

int ScanSections
(
    // return the last returned value by On*()

    FILE	*f,		// source file
    SectionFunc	OnSection,	// not NULL: call this function for each section
				// on result: <0: abort, 0:next section, 1:scan param
    SectionFunc	OnParams,	// not NULL: call this function after param scan
				// on result: <0: abort, continue
    void	*user_param	// user defined parameter

)
{
    DASSERT(f);

    SectionInfo_t si;
    memset(&si,0,sizeof(si));
    si.f = f;
    si.user_param = user_param;
    InitializeParamField(&si.param);
    si.param.free_data = true;

    int stat = 0;
    char buf[10000];
    bool scan_buf = false;
    while (FindSection(&si,buf,sizeof(buf),scan_buf))
    {
	scan_buf = false;
	if (OnSection)
	{
	    int stat = OnSection(&si);
	    if ( stat < 0 )
		break;
	    if (!stat)
		continue;
	}

	if (!OnParams)
	    continue;

	// scan param

	while (fgets(buf,sizeof(buf)-1,f))
	{
	    buf[sizeof(buf)-1] = 0;
	    char *ptr = buf;
	    while ( *ptr > 0 && *ptr <= ' ' )
		ptr++;
	    if ( *ptr == '[' )
	    {
		scan_buf = true;
		break;
	    }

	    //--- scan name
	    ccp name = ptr;
	    while ( *ptr > ' ' && *ptr != '=' )
		ptr++;
	    char *name_end = ptr;

	    //--- scan param
	    while ( *ptr == ' ' || *ptr == '\t' )
		ptr++;
	    if ( *ptr != '=' )
		continue;
	    if ( *++ptr == ' ' ) // skip max 1 space
		ptr++;

	    ccp value = ptr;
	    while ( *ptr && *ptr != '\n' && *ptr != '\r' )
		ptr++;

	    *name_end = *ptr = 0;
	    InsertParamField(&si.param,name,false,0,STRDUP(value));
	}

	stat = OnParams(&si);
	if ( stat < 0 )
	    break;
    }

    FreeString(si.section);
    FreeString(si.path);
    ResetParamField(&si.param);
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan configuration		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetRestoreState ( RestoreState_t *rs )
{
    if (rs)
    {
	ResetParamField(&rs->param);
	InitializeRestoreState(rs);
    }
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanRestoreState
(
    RestoreState_t	*rs,		// valid control
    void		*data,		// file data, modified, terminated by NULL or LF
    uint		size,		// size of file data
    void		**end_data	// not NULL: store end of analysed here
)
{
    DASSERT(rs);
    DASSERT(data||!size);
    if (!size)
	return ERR_OK;

    char *ptr = data;
    char *end = ptr + size;

    if (!rs->param.field)
	InitializeParamField(&rs->param);

    //--- scan name=value

    while ( ptr < end )
    {
	//--- skip lines and blanks
	while ( ptr < end && (uchar)*ptr <= ' ' )
	    ptr++;

	if ( *ptr == '[' )
	    break;

	//--- scan name
	ccp name = ptr;
	while ( *ptr >= '!' && *ptr <= '~' && *ptr != '=' )
	    ptr++;
	char *name_end = ptr;

	//--- scan param
	while ( *ptr == ' ' || *ptr == '\t' )
	    ptr++;
	if ( *ptr != '=' )
	    continue;
	if ( *++ptr == ' ' ) // skip max 1 space
	    ptr++;

	ccp value = ptr;
	while ( (uchar)*ptr >= ' ' || *ptr == '\t' )
	    ptr++;

	*name_end = *ptr = 0;
	InsertParamField(&rs->param,name,false,0,value);

	//--- next line
	while ( ptr < end && *ptr && *ptr != '\n' )
	    ptr++;
    }

    if (end_data)
	*end_data = ptr;

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError RestoreState
(
    const RestoreStateTab_t
			*table,		// section table, terminated by name==0
    void		*data,		// file data, modified, terminated by NULL or LF
    uint		size,		// size of file data
    RestoreStateLog_t	log_mode,	// print warnings (bit field)
    FILE		*log_file,	// error file; if NULL use stderr
    cvp			user_param	// pointer provided by user
)
{
    DASSERT(data||!size);
    if (!size)
	return ERR_OK;

    if (!log_file)
	log_file = stderr;

    char *ptr = data;
    char *end = ptr + size;
    if ( !*end && *end != '\n' )
	end[-1] = 0;

    while ( ptr < end )
    {
	//--- search next section

	while ( ptr < end && (uchar)*ptr <= ' ' )
	    ptr++;
	if ( *ptr != '[' )
	{
	    while ( ptr < end && *ptr && *ptr != '\n' )
		ptr++;
	    if ( ptr < end )
		ptr++;
	    continue;
	}

	ccp sect = ++ptr;
	while ( isalnum((int)*ptr) || *ptr == '-' || *ptr == '_' )
	    ptr++;

	ccp path = ptr;
	if ( *ptr == '/' )
	{
	    *ptr++ = 0;
	    path = ptr;
	    while ( isalnum((int)*ptr) || *ptr == '-' || *ptr == '_' || *ptr == '/' )
		ptr++;
	}

	int index = -1;
	if ( *ptr == ':' )
	{
	    *ptr++ = 0;
	    index = strtoul(ptr,0,10);
	}
	*ptr = 0;

	const RestoreStateTab_t *tab;
	for ( tab = table; tab->name; tab++ )
	    if (!strcmp(tab->name,sect))
		break;

	if (!tab->name)
	{
	    if ( log_mode & RSL_WARNINGS )
		fprintf(log_file,"! #RESTORE: Unknown section: %s\n",sect);
	    continue;
	}

	TRACE("SECT: [%s/%s:%d]\n",sect,path,index);
	if (!tab->func) // no need to analyse section
	    continue;


	//--- scan name=value

	RestoreState_t rs;
	rs.sect		= sect;
	rs.path		= path;
	rs.index	= index;
	rs.user_param	= user_param;
	rs.user_info	= 0;
	rs.log_mode	= log_mode;
	rs.log_file	= log_file;
	InitializeParamField(&rs.param);

	while ( ptr < end )
	{
	    //--- next line
	    while ( ptr < end && *ptr && *ptr != '\n' )
		ptr++;

	    //--- skip lines and blanks
	    while ( ptr < end && (uchar)*ptr <= ' ' )
		ptr++;

	    if ( *ptr == '[' )
		break;

	    //--- scan name
	    ccp name = ptr;
	    while ( *ptr >= '!' && *ptr <= '~' && *ptr != '=' )
		ptr++;
	    char *name_end = ptr;

	    //--- scan param
	    while ( *ptr == ' ' || *ptr == '\t' )
		ptr++;
	    if ( *ptr != '=' )
		continue;
	    if ( *++ptr == ' ' ) // skip max 1 space
		ptr++;

	    ccp value = ptr;
	    while ( (uchar)*ptr >= ' ' || *ptr == '\t' )
		ptr++;

	    *name_end = *ptr = 0;
	    InsertParamField(&rs.param,name,false,0,value);
	}

	#ifdef TEST0
	{
	    ParamFieldItem_t *ptr = rs.param.field, *end;
	    for ( end = ptr + rs.param.used; ptr < end; ptr++ )
		printf("%4u: %20s = %s|\n",ptr->num,ptr->key,(ccp)ptr->data);
	}
	#endif

	if (tab->func)
	    tab->func(&rs,tab->user_table);
	if ( log_file && log_mode & RSL_UNUSED_NAMES )
	{
	    ParamFieldItem_t *ptr = rs.param.field, *end;
	    for ( end = ptr + rs.param.used; ptr < end; ptr++ )
		if (!ptr->num)
		{
		    const uint len = strlen((ccp)ptr->data);
		    if ( len > 20 )
			fprintf(log_file,"! #RESTORE[%s]: Unused: %s [%u] %.20s...\n",
				PrintRestoreStateSection(&rs),
				ptr->key, len, (ccp)ptr->data );
		    else
			fprintf(log_file,"! #RESTORE[%s]: Unused: %s [%u] %s\n",
				PrintRestoreStateSection(&rs),
				ptr->key, len, (ccp)ptr->data );
		}
	}
	ResetRestoreState(&rs);
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintRestoreStateSection ( const RestoreState_t *rs )
{
    // print into cyclic buffer
    DASSERT(rs);

    uint len = strlen(rs->sect) + strlen(rs->path) + 2;

    char index[20];
    *index = 0;
    if ( rs->index >= 0 )
	len += snprintf(index,sizeof(index),":%u",rs->index);

    char *buf = GetCircBuf(len);
    if (*rs->path)
	snprintf(buf,len,"%s/%s%s",rs->sect,rs->path,index);
    else
	snprintf(buf,len,"%s%s",rs->sect,index);

    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ParamFieldItem_t * GetParamField
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name		// name of member
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = FindParamField(&rs->param,name);
    if (it)
	it->num++;
    else if ( rs->log_mode & RSL_MISSED_NAMES )
	fprintf(rs->log_file,"! #RESTORE[%s]: Missed: %s\n",
		PrintRestoreStateSection(rs), name );

    return it;
}

///////////////////////////////////////////////////////////////////////////////

int GetParamFieldINT
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    int			not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? str2l((ccp)it->data,0,10) : not_found;
}

///////////////////////////////////////////////////////////////////////////////

uint GetParamFieldUINT
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    uint		not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? str2ul((ccp)it->data,0,10) : not_found;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetParamFieldS64
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    s64			not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? str2ll((ccp)it->data,0,10) : not_found;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetParamFieldU64
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    u64			not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? str2ull((ccp)it->data,0,10) : not_found;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

float GetParamFieldFLOAT
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    float		not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? strtof((ccp)it->data,0) : not_found;
}

//-----------------------------------------------------------------------------

double GetParamFieldDBL
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    double		not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? strtod((ccp)it->data,0) : not_found;
}

//-----------------------------------------------------------------------------

long double GetParamFieldLDBL
(
    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    long double		not_found	// return this value if not found
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it ? strtold((ccp)it->data,0) : not_found;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int GetParamFieldBUF
(
    // returns >=0: length of read data; -1:nothing done (not_found==NULL)

    char		*buf,		// buffer to store result
    uint		buf_size,	// size of buffer

    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    EncodeMode_t	decode,		// decoding mode, fall back to OFF
					// supported: STRING, UTF8, BASE64, BASE64X
    ccp			not_found	// not NULL: store this value if not found
)
{
    DASSERT(buf);
    DASSERT(buf_size>1);
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    if (!it)
	return not_found ? StringCopyS(buf,buf_size,not_found) - buf : -1;

    return DecodeByMode(buf,buf_size,(ccp)it->data,-1,decode,0);
}

///////////////////////////////////////////////////////////////////////////////

mem_t GetParamFieldMEM
(
    // Returns the decoded 'source'. Result is NULL-terminated.
    // It points either to 'buf' or is alloced (on buf==NULL or too less space)
    // If alloced (mem.ptr!=buf) => call FreeMem(&mem)

    char		*buf,		// buffer to store result
    uint		buf_size,	// size of buffer

    const RestoreState_t *rs,		// valid restore-state structure
    ccp			name,		// name of member
    EncodeMode_t	decode,		// decoding mode, fall back to OFF
					// supported: STRING, UTF8, BASE64, BASE64X
    mem_t		not_found	// not NULL: return this value
)
{
    DASSERT(rs);
    DASSERT(name);

    ParamFieldItem_t *it = GetParamField(rs,name);
    return it
	? DecodeByModeMem(buf,buf_size,(ccp)it->data,-1,decode,0)
	: not_found;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		save + restore config by table		///////////////
///////////////////////////////////////////////////////////////////////////////

int srt_auto_dump = 0;
FILE *srt_auto_dump_file = 0;

///////////////////////////////////////////////////////////////////////////////

void DumpStateTable
(
    FILE	*f,		// valid output file
    int		indent,
    const SaveRestoreTab_t
		*srt		// list of variables
)
{
    if ( !f || !srt )
	return;

    indent = NormalizeIndent(indent);
    printf(
	"%*s off size *n(e) ty em name\n"
	"%*s----------------------------------\n",
	indent,"",
	indent,"" );

    for ( ; srt->type != SRT__TERM; srt++ )
    {
	if ( srt->type == SRT__IS_LIST )
	{
	    if ( srt->name )
		fprintf(f,"%*s# %s\n",indent,"",srt->name);
	    else
		putc('\n',f);
	}
	else
	    fprintf(f,"%*s%4u %4u *%-4d %2u %2u %s\n",
		indent,"",
		srt->offset, srt->size, srt->n_elem, srt->type, srt->emode,
		srt->name ? srt->name
			: srt->type == SRT_DEF_ARRAY ? " >ARRAY" : "-" );
    }
}


///////////////////////////////////////////////////////////////////////////////

static void print_scs_name ( FILE *f, int fw, int use_tab, ccp format, ... )
	__attribute__ ((__format__(__printf__,4,5)));

static void print_scs_name ( FILE *f, int fw, int use_tab, ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    const int len = vfprintf(f,format,arg);
    va_end(arg);

    if (use_tab)
    {
	const int tablen = ( use_tab - len ) / 8;
	fprintf(f,"%.*s= ", tablen<0 ? 0 : tablen, Tabs20 );
    }
    else
	fprintf(f,"%*s= ", len < fw ? fw-len : 0, "" );
}

//-----------------------------------------------------------------------------

static u64 SCS_get_uint ( const u8* data, int size )
{
    switch(size)
    {
	case 1: return *(u8*) data;
	case 2: return *(u16*)data;
	case 4: return *(u32*)data;
	case 8: return *(u64*)data;
    }
    return 0;
}

//-----------------------------------------------------------------------------

static u64 SCS_get_int ( const u8* data, int size )
{
    switch(size)
    {
	case 1: return *(s8*) data;
	case 2: return *(s16*)data;
	case 4: return *(s32*)data;
	case 8: return *(s64*)data;
    }
    return 0;
}

//-----------------------------------------------------------------------------

static void SCS_save_string ( FILE *f, ccp src, int slen, EncodeMode_t emode )
{
    if (!src)
	fputc('%',f);
    else
    {
	char buf[4000];
	mem_t res = EncodeByModeMem(buf,sizeof(buf),src,slen,emode);

	if (res.len)
	{
	    const bool quote = NeedsQuotesByEncoding(emode);
	    if (quote)
		fputc('"',f);
	    fwrite(res.ptr,1,res.len,f);
	    if (quote)
		fputc('"',f);
	}

	if ( res.ptr != buf )
	    FreeMem(&res);
    }
}

//
//-----------------------------------------------------------------------------

void SaveCurrentStateByTable
(
    FILE	*f,		// valid output file
    cvp		data0,		// valid pointer to source struct
    const SaveRestoreTab_t
		*srt,		// list of variables
    ccp		prefix,		// NULL or prefix for names
    uint	fw_name		// field width of name, 0=AUTO
				// tabs are used for multiple of 8 and for AUTO
)
{
    DASSERT(data0);
    DASSERT(srt);
    if ( !f || !data0 || !srt )
	return;

    if ( srt_auto_dump && srt_auto_dump_file )
    {
	if ( srt_auto_dump > 0 )
	    srt_auto_dump--;
	DumpStateTable(srt_auto_dump_file,0,srt);
    }

    if (!prefix)
	prefix = EmptyString;
    const uint prelen = strlen(prefix);

    if (!fw_name)
    {
	const SaveRestoreTab_t *ptr;
	for ( ptr = srt; ptr->type != SRT__TERM; ptr++ )
	    if (ptr->name)
	    {
		int slen = strlen(ptr->name);
		switch(srt->type)
		{
		    case SRT_STRING_FIELD:
		    case SRT_PARAM_FIELD:
			slen += 3;
			break;
		}

		if ( fw_name < slen )
		    fw_name = slen;
	    }
	fw_name += prelen;
	fw_name = (fw_name+7)/8*8;
    }

    const int use_tab = (fw_name&7) ? 0 : (fw_name/8)*8+7;
    const u8 *data	= data0;
    uint last_count	= 0;
    uint aelem_count	= 0;
    uint aelem_offset	= 0;

    for ( ; srt->type != SRT__TERM; srt++ )
    {
      if ( srt->type == SRT_DEF_ARRAY )
      {
	aelem_offset = srt->size;
	if ( srt->n_elem < 0 )
	{
	    aelem_count = -srt->n_elem;
	    if ( aelem_count > last_count )
		aelem_count = last_count;
	}
	else
	    aelem_count = srt->n_elem;
	continue;
      }

      if ( srt->type == SRT__IS_LIST )
      {
	if ( srt->name )
	    fprintf(f,"# %s\n",srt->name);
	else
	    putc('\n',f);
	continue;
      }

      if (!srt->name)
	continue;

      if ( srt->type < SRT__IS_LIST )
	print_scs_name(f,fw_name,use_tab,"%s%s",prefix,srt->name);

      int n_elem, elem_off;
      bool have_array = aelem_count && aelem_offset;
      if (have_array)
      {
	elem_off = aelem_offset;
	n_elem   = aelem_count;
      }
      else
      {
	elem_off = srt->size;
	n_elem   = srt->n_elem;
	if ( n_elem < 0 )
	{
	    have_array = true;
	    n_elem = -n_elem;
	    if ( n_elem > last_count )
		n_elem = last_count;
	}
	else if ( n_elem > 1 )
	    have_array = true;
      }

      uint i, offset;
      switch(srt->type)
      {
	case SRT_BOOL:
	    if (have_array)
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f,"%s%u", i ? "," : "", SCS_get_uint(d,srt->size)!=0 );
		fputc('\n',f);
	    }
	    else
		fprintf(f,"%u\n",SCS_get_uint(data+srt->offset,srt->size)!=0);
	    break;

	case SRT_COUNT:
	    last_count = 0;
	    // fall through
	case SRT_UINT:
	    if (have_array)
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f,"%s%llu", i ? "," : "", SCS_get_uint(d,srt->size) );
		fputc('\n',f);
	    }
	    else
	    {
		const u64 num = SCS_get_uint(data+srt->offset,srt->size);
		if ( srt->type == SRT_COUNT && num <= UINT_MAX )
		    last_count = num;
		fprintf(f,"%llu\n",num);
	    }
	    break;

	case SRT_HEX:
	    if (have_array)
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f,"%s%#llx", i ? "," : "", SCS_get_uint(d,srt->size) );
		fputc('\n',f);
	    }
	    else
		fprintf(f,"%#llx\n",SCS_get_uint(data+srt->offset,srt->size));
	    break;

	case SRT_INT:
	    if (have_array)
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f,"%s%lld", i ? "," : "", SCS_get_int(d,srt->size) );
		fputc('\n',f);
	    }
	    else
		fprintf(f,"%lld\n",SCS_get_int(data+srt->offset,srt->size));
	    break;

	case SRT_FLOAT:
	    if ( srt->size == sizeof(float) )
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f, "%s%.8g", i ? "," : "", *(float*)d );
	    }
	    else if ( srt->size == sizeof(double) )
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f, "%s%.16g", i ? "," : "", *(double*)d );
	    }
	    else if ( srt->size == sizeof(long double) )
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f, "%s%.20Lg", i ? "," : "", *(long double*)d );
	    }
	    else
		fputs("0.0",f);
	    fputc('\n',f);
	    break;

	case SRT_XFLOAT:
	    if ( srt->size == sizeof(float) )
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f, "%s%a", i ? "," : "", *(float*)d );
	    }
	    else if ( srt->size == sizeof(double) )
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f, "%s%a", i ? "," : "", *(double*)d );
	    }
	    else if ( srt->size == sizeof(long double) )
	    {
		const u8 *d = data + srt->offset;
		for ( i = 0; i < n_elem; i++, d += elem_off )
		    fprintf(f, "%s%La", i ? "," : "", *(long double*)d );
	    }
	    else
		fputs("0.0",f);
	    fputc('\n',f);
	    break;

	//-----

	case SRT_STRING_SIZE:
	    if ( n_elem > 0 )
		for ( i = 0, offset = srt->offset;; offset += elem_off )
		{
		    SCS_save_string(f,(char*)(data+offset),-1,srt->emode);
		    if ( ++i == n_elem )
			break;
		    fputc(',',f);
		}
	    fputc('\n',f);
	    break;

	case SRT_STRING_ALLOC:
	    if ( n_elem > 0 )
		for ( i = 0, offset = srt->offset;; offset += elem_off )
		{
		    SCS_save_string(f,*(ccp*)(data+offset),-1,srt->emode);
		    if ( ++i == n_elem )
			break;
		    fputc(',',f);
		}
	    fputc('\n',f);
	    break;

	case SRT_MEM:
	{
	    if ( n_elem > 0 )
		for ( i = 0, offset = srt->offset;; offset += elem_off )
		{
		    mem_t *mem = (mem_t*)(data+offset);
		    SCS_save_string(f,mem->ptr,mem->len,srt->emode);
		    if ( ++i == n_elem )
			break;
		    fputc(',',f);
		}
	    fputc('\n',f);
	    break;
	}

	//-----

	case SRT_STRING_FIELD:
	{
	    print_scs_name(f,fw_name,use_tab,"%s%s@n",prefix,srt->name);
	    const StringField_t *sf = (StringField_t*)(data+srt->offset);
	    fprintf(f,"%u\n",sf->used);

	    int i;
	    ccp *ptr = sf->field;
	    for ( i = 0; i < sf->used; i++, ptr++ )
	    {
		print_scs_name(f,fw_name,use_tab,"%s%s@%u",prefix,srt->name,i);
		SCS_save_string(f,*ptr,-1,srt->emode);
		fputc('\n',f);
	    }
	    break;
	}

	case SRT_PARAM_FIELD:
	{
	    print_scs_name(f,fw_name,use_tab,"%s%s@n",prefix,srt->name);
	    const ParamField_t *pf = (ParamField_t*)(data+srt->offset);
	    fprintf(f,"%u\n",pf->used);

	    int i;
	    ParamFieldItem_t *ptr = pf->field;
	    for ( i = 0; i < pf->used; i++, ptr++ )
	    {
		print_scs_name(f,fw_name,use_tab,"%s%s@%u",prefix,srt->name,i);
		fprintf(f,"%d ",ptr->num);
		SCS_save_string(f,ptr->key,-1,srt->emode);
		fputc('\n',f);
	    }
	    break;
	}
      }
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateByTable
(
    RestoreState_t		*rs,	// info data, can be modified (cleaned after call)
    void			*data0,	// valid pointer to destination struct
    const SaveRestoreTab_t	*srt,	// list of variables
    ccp				prefix	// NULL or prefix for names
)
{
    DASSERT(rs);
    DASSERT(srt);
    DASSERT(data0);
    if ( !rs || !data0 || !srt )
	return;

    if ( srt_auto_dump && srt_auto_dump_file )
    {
	if ( srt_auto_dump > 0 )
	    srt_auto_dump--;
	DumpStateTable(srt_auto_dump_file,0,srt);
    }

    char buf[4000];
    if (!prefix)
	prefix = EmptyString;

    uint last_count	= 0;
    uint aelem_count	= 0;
    uint aelem_offset	= 0;

    bool mode_add = false;

    for ( ; srt->type != SRT__TERM; srt++ )
    {
      if ( srt->type == SRT_DEF_ARRAY )
      {
	aelem_offset = srt->size;
	if ( srt->n_elem < 0 )
	{
	    aelem_count = -srt->n_elem;
	    if ( aelem_count > last_count )
		aelem_count = last_count;
	}
	else
	    aelem_count = srt->n_elem;
	continue;
      }

      if ( srt->type >= SRT_MODE__BEGIN && srt->type <= SRT_MODE__END )
      {
	switch (srt->type)
	{
	    case SRT_MODE_ASSIGN: mode_add = false; break;
	    case SRT_MODE_ADD:    mode_add = true;  break;
	}
	continue;
      }

      if ( srt->type == SRT__IS_LIST || !srt->name )
	continue;

      const u8 *data = data0;

      if ( srt->type < SRT__IS_LIST )
	StringCat2S(buf,sizeof(buf),prefix,srt->name);

      int n_elem, elem_off;
      if ( aelem_count && aelem_offset )
      {
	elem_off = aelem_offset;
	n_elem   = aelem_count;
      }
      else
      {
	elem_off = srt->size;
	n_elem = srt->n_elem;
	if ( n_elem < 0 )
	{
	    n_elem = -n_elem;
	    if ( n_elem > last_count )
		n_elem = last_count;
	}
      }

      switch(srt->type)
      {
	case SRT_COUNT:
	    last_count = 0;
	    // fall through
	case SRT_BOOL:
	case SRT_UINT:
	case SRT_HEX:
	    if (n_elem)
	    {
		ParamFieldItem_t *it = GetParamField(rs,buf);
		if (!it)
		    break;

		char *src = (char*)it->data;
		u8 *val = (u8*)(data+srt->offset);
		uint i = 0;
		while (*src)
		{
		    u64 num = str2ull(src,&src,10);
		    if ( !i && srt->type == SRT_COUNT && num <= UINT_MAX )
			last_count = num;

		    if (mode_add)
			switch(srt->size)
			{
			    case 1: *(u8*) val += num; break;
			    case 2: *(u16*)val += num; break;
			    case 4: *(u32*)val += num; break;
			    case 8: *(u64*)val += num; break;
			}
		    else
			switch(srt->size)
			{
			    case 1: *(u8*) val = num; break;
			    case 2: *(u16*)val = num; break;
			    case 4: *(u32*)val = num; break;
			    case 8: *(u64*)val = num; break;
			}
		    if ( ++i == n_elem )
			break;
		    val += elem_off;
		    if ( *src == ',' )
			src++;
		}
	    }
	    break;

	case SRT_INT:
	    if (n_elem)
	    {
		ParamFieldItem_t *it = GetParamField(rs,buf);
		if (!it)
		    break;

		char *src = (char*)it->data;
		u8 *val = (u8*)(data+srt->offset);
		uint i = 0;
		while (*src)
		{
		    s64 num = str2ll(src,&src,10);
		    if (mode_add)
			switch(srt->size)
			{
			    case 1: *(s8*) val += num; break;
			    case 2: *(s16*)val += num; break;
			    case 4: *(s32*)val += num; break;
			    case 8: *(s64*)val += num; break;
			}
		    else
			switch(srt->size)
			{
			    case 1: *(s8*) val = num; break;
			    case 2: *(s16*)val = num; break;
			    case 4: *(s32*)val = num; break;
			    case 8: *(s64*)val = num; break;
			}
		    if ( ++i == n_elem )
			break;
		    val += elem_off;
		    if ( *src == ',' )
			src++;
		}
	    }
	    break;

	case SRT_FLOAT:
	case SRT_XFLOAT:
	    if (n_elem)
	    {
		ParamFieldItem_t *it = GetParamField(rs,buf);
		if (!it)
		    break;

		char *src = (char*)it->data;
		u8 *val = (u8*)(data+srt->offset);
		uint i = 0;
		while (*src)
		{
		    if (mode_add)
		    {
			if ( srt->size == sizeof(float) )
			    *(float*)val += strtof(src,&src);
			else if ( srt->size == sizeof(double) )
			    *(double*)val += strtod(src,&src);
			else if ( srt->size == sizeof(long double) )
			    *(long double*)val += strtold(src,&src);
		    }
		    else
		    {
			if ( srt->size == sizeof(float) )
			    *(float*)val = strtof(src,&src);
			else if ( srt->size == sizeof(double) )
			    *(double*)val = strtod(src,&src);
			else if ( srt->size == sizeof(long double) )
			    *(long double*)val = strtold(src,&src);
		    }

		    if ( ++i == n_elem )
			break;
		    val += elem_off;
		    if ( *src == ',' )
			src++;
		}
	    }
	    break;

	//-----

	case SRT_STRING_SIZE:
	    if (n_elem)
	    {
		ParamFieldItem_t *it = GetParamField(rs,buf);
		mem_list_t ml;
		DecodeByModeMemList(&ml,2,it?(ccp)it->data:0,-1,srt->emode,0);

		char *val = (char*)(data+srt->offset);
		uint i;
		for ( i = 0; i < n_elem; i++, val += elem_off )
		    StringCopyS( val, srt->size, i<ml.used ? ml.list[i].ptr : 0 );
		ResetMemList(&ml);
	    }
	    break;

	case SRT_STRING_ALLOC:
	    if (n_elem)
	    {
		ParamFieldItem_t *it = GetParamField(rs,buf);
		mem_list_t ml;
		DecodeByModeMemList(&ml,2,it?(ccp)it->data:0,-1,srt->emode,0);

		ccp *val = (ccp*)(data+srt->offset);
		uint i;
		for ( i = 0; i < n_elem; i++ )
		{
		    FreeString(*val);
		    *val = 0;
		    if ( i < ml.used )
		    {
			mem_t *m = ml.list+i;
			if ( m->ptr )
			    *val =  m->ptr == EmptyString ? EmptyString : MEMDUP(m->ptr,m->len);
		    }
		    val = (ccp*)( (u8*)val + elem_off );
		}
		ResetMemList(&ml);
	    }
	    break;

	case SRT_MEM:
	    if (n_elem)
	    {
		ParamFieldItem_t *it = GetParamField(rs,buf);
		mem_list_t ml;
		DecodeByModeMemList(&ml,2,it?(ccp)it->data:0,-1,srt->emode,0);

		mem_t *val = (mem_t*)(data+srt->offset);
		uint i;
		for ( i = 0; i < n_elem; i++ )
		{
		    FreeString(val->ptr);
		    val->ptr = 0;
		    val->len = 0;
		    if ( i < ml.used )
		    {
			mem_t *m = ml.list+i;
			if ( m->ptr )
			    *val = m->ptr == EmptyString ? EmptyMem : DupMem(*m);
		    }
		    val = (mem_t*)( (u8*)val + elem_off );
		}
		ResetMemList(&ml);
	    }
	    break;

	//-----

	case SRT_STRING_FIELD:
	{
	    StringField_t *sf = (StringField_t*)(data+srt->offset);
	    ResetStringField(sf);

	    snprintf(buf,sizeof(buf),"%s%s@n",prefix,srt->name);
	    const int n = GetParamFieldINT(rs,buf,0);
	    int i;
	    for ( i = 0; i < n; i++ )
	    {
		snprintf(buf,sizeof(buf),"%s%s@%u",prefix,srt->name,i);
		mem_t res = GetParamFieldMEM(buf,sizeof(buf),rs,buf,srt->emode,NullMem);
		InsertStringField(sf,res.ptr,res.ptr!=buf);
	    }
	    break;
	}

	case SRT_PARAM_FIELD:
	{
	    ParamField_t *sf = (ParamField_t*)(data+srt->offset);
	    ResetParamField(sf);

	    snprintf(buf,sizeof(buf),"%s%s@n",prefix,srt->name);
	    const int n = GetParamFieldINT(rs,buf,0);
	    int i;
	    for ( i = 0; i < n; i++ )
	    {
		snprintf(buf,sizeof(buf),"%s%s@%u",prefix,srt->name,i);
		ParamFieldItem_t *it = GetParamField(rs,buf);
		if (it)
		{
		    char *str;
		    const int num = strtol((ccp)it->data,&str,10);
		    if ( *str == ' ' ) // space should always exist
			str++;

		    mem_t res = DecodeByModeMem(buf,sizeof(buf),str,-1,srt->emode,0);
		    InsertParamField(sf,res.ptr,false,num,0);
		    if ( res.ptr != buf )
			FreeString(res.ptr);
		}
	    }
	    break;
	}
      }
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan socket type		///////////////
///////////////////////////////////////////////////////////////////////////////

bool ScanSocketInfo
(
    // Syntax: [ PREFIX ',' ]... [ PREFIX ':' ] address
    // returns TRUE, if a protocol or a type is found

    socket_info_t	*si,		// result, not NULL, will be initialized
    ccp			address,	// address to analyze
    uint		tolerance	// tolerace:
					//   0: invalid without prefix
					//   1: analyse beginning of address part:
					//	'/' or './' or '../'	-> AF_UNIX
					//   2: estimate type
					//	'/' before first ':'	-> AF_UNIX
					//	1.2 | 1.2.3 | 1.2.3.4	-> AF_INET
					//	a HEX:IP combi		-> AF_INET6
					//	domain name		-> AF_INET_ANY
)
{
    DASSERT(si);
    InitializeSocketInfo(si);

    if (!address)
	return false;

    while ( *address && (uchar)*address <= ' ' )
	address++;

    static const KeywordTab_t keytab[] =
    {
	{ -1,		"STREAM",	0,		SOCK_STREAM },
	{ -1,		"DGRAM",	"DATAGRAM",	SOCK_DGRAM },

	{ AF_UNIX,	"UNIX",		"FILE",		-1 },
	{ AF_UNIX,	"UNIX-TCP",	"FILE-TCP",	SOCK_STREAM },
	 { AF_UNIX,	"UNIX-STREAM",	"FILE-STREAM",	SOCK_STREAM },
	{ AF_UNIX,	"UNIX-UDP",	"FILE-UDP",	SOCK_DGRAM },
	 { AF_UNIX,	"UNIX-DGRAM",	"FILE-DGRAM",	SOCK_DGRAM },
	 { AF_UNIX,	"UNIX-DATAGRAM","FILE-DATAGRAM",SOCK_DGRAM },
	{ AF_UNIX,	"UNIX-SEQ",	"FILE-SEQ",	SOCK_SEQPACKET },
	 { AF_UNIX,	"SEQ",		0,		SOCK_SEQPACKET },

	{ AF_INET_ANY,	"IP",		0,		-1 },
	{ AF_INET_ANY,	"TCP",		0,		SOCK_STREAM },
	{ AF_INET_ANY,	"UDP",		0,		SOCK_DGRAM },
	{ AF_INET_ANY,	"RAW",		0,		SOCK_RAW },

	{ AF_INET,	"IP4",		"IPV4",		-1 },
	{ AF_INET,	"TCP4",		"TCPV4",	SOCK_STREAM },
	{ AF_INET,	"UDP4",		"UDPV4",	SOCK_DGRAM },
	{ AF_INET,	"RAW4",		"RAWV4",	SOCK_RAW },

	{ AF_INET6,	"IP6",		"IPV6",		-1 },
	{ AF_INET6,	"TCP6",		"TCPV6",	SOCK_STREAM },
	{ AF_INET6,	"UDP6",		"UDPV6",	SOCK_DGRAM },
	{ AF_INET6,	"RAW6",		"RAWV6",	SOCK_RAW },

	{0,0,0,0}
    };

    bool ok = false;
    int proto = -1;
    int type  = -1;
    ccp start = address;
    for(;;)
    {
	char namebuf[10];
	char *dest = namebuf;
	ccp ptr = start;
	while ( dest < namebuf + sizeof(namebuf) - 1
	    && isalnum((int)*ptr) || *ptr == '-' )
	{
	    *dest++ = *ptr++;
	}
	if ( ptr == start || *ptr != ',' &&  *ptr != ':' )
	    break;

	*dest = 0;
	int abbrev;
	const KeywordTab_t *k = ScanKeyword(&abbrev,namebuf,keytab);
	if ( !k || abbrev )
	    break;

	if ( k->id == AF_INET_ANY ? ( proto == -1 ) : ( k->id >= 0 ) )
	    proto = k->id;
	if ( k->opt >= 0 )
	    type = k->opt;

	noPRINT("%3lld %3lld |%s|%s| -> %d,%d\n",
		k->id, k->opt, k->name1, k->name2 ? k->name2 : "",
		proto, type );
	start = ptr;
	if ( *start++ == ':' )
	{
	    ok = true;
	    break;
	}
    }

    if (!ok)
    {
	start = address;
	proto = -1;
	type  = -1;
    }

    if ( proto < 0 && tolerance > 0 )
    {
	if ( *start == '/' || !memcmp(start,"./",2) || !memcmp(start,"../",3) )
	{
	    proto = AF_UNIX;
	    goto proto_found;
	}

	if ( tolerance > 1 )
	{
	    ccp colon = strchr(start,':');
	    if (!colon)
		colon = start + strlen(start);

	    //--- test for UNIX

	    ccp slash = strchr(start,'/');
	    if ( slash && slash < colon )
	    {
		proto = AF_UNIX;
		goto proto_found;
	    }

	    //--- test for IPv4

	    ccp ptr = start;
	    uint count = 0;
	    while ( ptr < colon )
	    {
		const char ch = *ptr;
		if ( ch == '.' )
		    count++;
		else if ( ch < '0' || ch > '9' )
		    break;
		ptr++;
	    }
	    if ( ptr == colon && count < 4 )
	    {
		proto = AF_INET;
		if ( count > 0 )
		    goto proto_found;
	    }

	    //--- test for IPv6

	    ptr = start;
	    const bool have_bracket = *ptr == '[';
	    if (have_bracket)
		ptr++;
	    count = 0;
	    while (*ptr)
	    {
		const char ch = *ptr;
		if ( ch == ':' )
		    count++;
		else if ( ch != '.'
			&& !( ch >= '0' && ch <= '9' )
			&& !( ch >= 'A' && ch <= 'F' )
			&& !( ch >= 'a' && ch <= 'f' )
			)
		{
		    break;
		}
		ptr++;
	    }
	    if ( count > 1 && count < 8 && ( !have_bracket || *ptr == ']' ))
	    {
		proto = AF_INET6;
		goto proto_found;
	    }

	    //--- test for a domain name

	    ptr = start;
	    count = 0;
	    ccp beg_name = ptr;
	    while ( ptr < colon )
	    {
		const int ch = *ptr;
		if ( ptr > beg_name && ch == '.' )
		{
		    count++;
		    beg_name = ptr+1;
		}
		else if ( ptr == beg_name ? !isalpha(ch) : !isalnum(ch) )
		    break;
		ptr++;
	    }
	    if ( count > 0 && ptr == colon )
	    {
		proto = AF_INET_ANY;
		goto proto_found;
	    }
	}
    }

    if ( proto || type )
    {
     proto_found:
	si->is_valid	= true;
	si->protocol	= proto;
	si->type	= type;
	si->address	= start;
    }
    //printf(">>> v=%d {%d,%d}: %s\n",ok,proto,type,start);
    return si->is_valid;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintSocketInfo ( int protocol, int type )
{
    switch (protocol)
    {
      case AF_UNIX:
	switch(type)
	{
	  case SOCK_STREAM:	return "unix-tcp:";
	  case SOCK_DGRAM:	return "unix-udp:";
	  case SOCK_SEQPACKET:	return "unix-seq:";
	  default:		return "unix:";
	}
	break;

      case AF_INET:
	switch(type)
	{
	  case SOCK_STREAM:	return "tcp4:";
	  case SOCK_DGRAM:	return "udp4:";
	  case SOCK_RAW:	return "raw4:";
	  default:		return "ipv4:";
	}
	break;

      case AF_INET6:
	switch(type)
	{
	  case SOCK_STREAM:	return "tcp6:";
	  case SOCK_DGRAM:	return "udp6:";
	  case SOCK_RAW:	return "raw6:";
	  default:		return "ipv6:";
	}
	break;

      case AF_INET_ANY:
	switch(type)
	{
	  case SOCK_STREAM:	return "tcp:";
	  case SOCK_DGRAM:	return "udp:";
	  case SOCK_RAW:	return "raw:";
	  default:		return "ip:";
	}
	break;
    };

    return "";
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			stat_file_count_t		///////////////
///////////////////////////////////////////////////////////////////////////////

stat_file_count_t stat_file_count = {0};

///////////////////////////////////////////////////////////////////////////////

uint CountOpenFiles()
{
    DIR *fdir = opendir("/proc/self/fd");
    if (fdir)
    {
	uint count = 0;
	for(;;)
	{
	    struct dirent *dent = readdir(fdir);
	    if (!dent)
		break;
	    if (dent->d_name[0] != '.' )
		count++;
	}
	closedir(fdir);

	stat_file_count.cur_files = count;
	if ( stat_file_count.max_files < count )
	     stat_file_count.max_files = count;
    }

    return stat_file_count.cur_files;
}

///////////////////////////////////////////////////////////////////////////////

void RegisterFileId ( int fd )
{
    if ( (int)stat_file_count.max_files <= fd )
	stat_file_count.max_files = fd+1;
}

///////////////////////////////////////////////////////////////////////////////

void UpdateOpenFiles ( bool count_current )
{
    struct rlimit rlim;
    if (!getrlimit(RLIMIT_NOFILE,&rlim))
    {
	stat_file_count.cur_limit = rlim.rlim_cur;
	stat_file_count.max_limit = rlim.rlim_max;
    }

    if ( count_current || !stat_file_count.max_files )
	CountOpenFiles();
}

///////////////////////////////////////////////////////////////////////////////

uint SetOpenFilesLimit ( uint limit )
{
    struct rlimit rlim;
    if (!getrlimit(RLIMIT_NOFILE,&rlim))
    {
	rlim.rlim_cur = limit;
	setrlimit(RLIMIT_NOFILE,&rlim);
    }

    UpdateOpenFiles(false);
    return stat_file_count.cur_limit;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintOpenFiles ( bool count_current )
{
    if (!stat_file_count.cur_limit)
	UpdateOpenFiles(count_current);
    else if (count_current)
	CountOpenFiles();

    return PrintCircBuf("cur=%u, max=%u, limit=%u/%u",
	stat_file_count.cur_files,
	stat_file_count.max_files,
	stat_file_count.cur_limit,
	stat_file_count.max_limit );
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////			PrintScript			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetNamePSFF ( PrintScriptFF fform )
{
    switch(fform)
    {
	case PSFF_UNKNOWN:	return "TEXT";
	case PSFF_ASSIGN:	return "ASSIGN";
	case PSFF_CONFIG:	return "CONFIG";
	case PSFF_JSON:		return "JSON";
	case PSFF_BASH:		return "BASH";
	case PSFF_SH:		return "SH";
	case PSFF_PHP:		return "PHP";
	case PSFF_MAKEDOC:	return "MDOC";
	case PSFF_C:		return "C";
    }
    return "???";
}

///////////////////////////////////////////////////////////////////////////////

void PrintScriptHeader ( PrintScript_t *ps )
{
    DASSERT(ps);

    ps->index = 0;
    ps->add_index = false;
    ps->sep[0] = ps->sep[1] = 0;

    if ( !ps->var_name || !*ps->var_name )
	ps->var_name = "res";
    if ( !ps->var_prefix )
	ps->var_prefix = "";

    if ( ps->f )
    {
	switch (ps->fform)
	{
	    case PSFF_JSON:
		if (ps->create_array)
		    fputs("[",ps->f);
		ps->boc = 0;
		break;

	    case PSFF_SH:
	       ps->add_index =ps->create_array;
		ps->boc = "#";
	       break;

	    case PSFF_PHP:
		if (ps->create_array)
		    fprintf(ps->f,"$%s = array();\n\n",ps->var_name);
		ps->boc = "#";
		break;

	    case PSFF_MAKEDOC:
		if (ps->create_array)
		    fprintf(ps->f,"%s = @LIST\n\n",ps->var_name);
		ps->boc = "#!";
		break;

	    case PSFF_C:
		if (ps->create_array)
		    fprintf(ps->f,"%s = @LIST\n\n",ps->var_name);
		ps->boc = "//";
		break;

	    default:
		//fputc('\n',ps->f);
		ps->boc = "#";
		break;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrintScriptFooter ( PrintScript_t *ps )
{
    DASSERT(ps);

    if (ps->f)
    {
	switch (ps->fform)
	{
	    case PSFF_JSON:
		if (ps->create_array)
		    fputs("]\n",ps->f);
		break;

	    case PSFF_SH:
		if (ps->create_array)
		    fprintf(ps->f,"%sN=%u\n\n",ps->var_prefix,ps->index);
		break;

	    case PSFF_PHP:
		fputs("?>\n",ps->f);
		break;

	    default:
		break;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

int PutScriptVars
(
    PrintScript_t	*ps,		// valid control struct
    uint		mode,		// bit field: 1=open var, 2:close var
    ccp			text		// text with lines of format NAME=VALUE
)
{
    FILE * f = ps ? ps->f : 0;
    if (!f)
	return 0;
    DASSERT(ps);


    //--- begin of output

    if ( mode & 1 )
    {
	ps->count = 0;

	if ( ps->add_index )
	    snprintf( ps->prefix, sizeof(ps->prefix),
		"%s%u", ps->var_prefix, ps->index );
	else
	    StringCopyS( ps->prefix, sizeof(ps->prefix), ps->var_prefix );

	switch (ps->fform)
	{
	 case PSFF_JSON:
	    fprintf(f,"%s{",ps->sep);
	    ps->sep[0] = ',';
	    break;

	 case PSFF_PHP:
	    fputs("$d = new \\stdClass;\n",f);
	    break;

	 case PSFF_MAKEDOC:
	    fprintf(f,"d = @MAP\n");
	    break;

	 default:
	    break;
	}
    }


    //--- print var lines

    ccp ptr = text ? text : "";
    char varbuf[200];

    for(;;)
    {
	if ( ps->ena_empty && ps->boc )
	{
	    while ( *ptr == ' ' || *ptr == '\t' )
		ptr++;
	    if ( *ptr == '\n' )
	    {
		ptr++;
		fputc('\n',f);
		continue;
	    }
	}
	else
	    while ( *ptr && (uchar)*ptr <= ' ' )
		ptr++;

	if ( *ptr == '#' )
	{
	    ccp comment = ++ptr;
	    while ( (uchar)*ptr >= ' ' )
		ptr++;
	    if ( ps->ena_comments && ps->boc )
		fprintf(f,"%s%.*s\n",ps->boc,(int)(ptr-comment),comment);
	    goto next_line;
	}

	ccp varname = ptr;
	while ( *ptr && *ptr != '=' )
	    ptr++;
	if ( *ptr != '=' || ptr == varname )
	    break;

	const int varlen = ptr - varname;
	if ( ps->force_case <= LOUP_LOWER )
	{
	    MemLowerS(varbuf,sizeof(varbuf),MemByS(varname,varlen));
	    varname = varbuf;
	}
	else if ( ps->force_case >= LOUP_UPPER )
	{
	    varname = MemUpperS(varbuf,sizeof(varbuf),MemByS(varname,varlen));
	    varname = varbuf;
	}

	ccp quote, param;
	uint plen, free_param = 0;
	if ( *++ptr == '"' )
	{
	    ptr++;
	    param = ptr;
	    if (ps->auto_quote)
	    {
		quote = EmptyString;
		while ( *ptr && *ptr != '"' )
		{
		    if ( *ptr == '\\' && ptr[1] )
			ptr++;
		    ptr++;
		}
		param = EscapeString(param,ptr-param,EmptyString,EmptyString,CHMD__MODERN,
				ps->fform == PSFF_BASH ? '$' : '"', true, &plen );
		free_param++;
	    }
	    else
	    {
		quote = "\"";
		while ( *ptr && !( ptr[0] == '"' && ptr[1] == '\n' ))
		    ptr++;
		plen = ptr - param;
	    }
	    if ( *ptr == '"' )
		ptr++;
	}
	else
	{
	    param = ptr;
	    quote = EmptyString;
	    while ( *ptr && *ptr != '\n' )
		ptr++;
	    plen = ptr - param;
	}

	switch (ps->fform)
	{
	 case PSFF_ASSIGN:
	    fprintf(f, "%.*s = %s%.*s%s\n",
			varlen, varname, quote, plen, param, quote );
	    break;

	 case PSFF_CONFIG:
	    if ( ps->eq_tabstop > 0 )
	    {
		int tabs = ps->eq_tabstop - varlen/8;
		if ( tabs < 0 )
		    tabs = 0;
		fprintf(f, "%.*s%.*s= %.*s\n",
			varlen, varname, tabs, Tabs20, plen, param );
	    }
	    else
		fprintf(f, "%.*s = %.*s\n",
			varlen, varname, plen, param );
	    break;

	 case PSFF_JSON:
	    fprintf(f, "%s\"%.*s\":%s%.*s%s",
			ps->count ? "," : "",
			varlen, varname, quote, plen, param, quote );
	    break;

	 case PSFF_BASH:
	    if (ps->create_array)
	    {
		fprintf(f, "%s%.*s%s=(%s%.*s%s)\n",
			ps->prefix,
			varlen, varname, ps->index ? "+" : "", quote, plen, param, quote );
		break;
	    }
	    // fall through

	 case PSFF_SH:
	    fprintf(f, "%s%.*s=%s%.*s%s\n",
			ps->prefix, varlen, varname, quote, plen, param, quote );
	    break;

	 case PSFF_PHP:
	 case PSFF_C:
	    fprintf(f, "$d->%.*s = %s%.*s%s;\n",
			varlen, varname, quote, plen, param, quote );
	    break;

	 case PSFF_MAKEDOC:
	    fprintf(f, "d[\"%.*s\"] = %s%.*s%s\n",
			varlen, varname, quote, plen, param, quote );
	    break;

	 default:
	    fprintf(f, "%*.*s = %s%.*s%s\n",
			ps->var_size > 0 ? ps->var_size : 20,
			varlen, varname, quote, plen, param, quote );
	    break;
	}
	ps->count++;
	if (free_param)
	    FreeString(param);

	//-- skip to next line
     next_line:
	while ( *ptr && (uchar)*ptr <= ' ' && *ptr != '\n' )
	    ptr++;
	if ( *ptr == '\n' )
	    ptr++;
    }


    //--- end of output

    if ( mode & 2 )
    {
	switch (ps->fform)
	{
	 case PSFF_JSON:
	    fputs("}\n",f);
	    break;

	 case PSFF_PHP:
	    fprintf(f, "$%s%s = $d;\n\n",
			ps->var_name, ps->create_array ? "[]" : "" );
	    break;

	 case PSFF_MAKEDOC:
	    fprintf(f, "%s %s= move(d);\n\n",
			ps->var_name, ps->create_array ? "#" : "" );
	    break;

	 default:
	    fputc('\n',f);
	    break;
	}
	fflush(f);
    }

    ps->index++;
    return ps->count;
}

///////////////////////////////////////////////////////////////////////////////

int PrintScriptVars
(
    PrintScript_t	*ps,		// valid control struct
    uint		mode,		// bit field: 1=open var, 2:close var
    ccp			format,		// format of message
    ...					// arguments

)
{
    DASSERT(ps);

    char buf[5000];
    if (format)
    {
	va_list arg;
	va_start(arg,format);
	vsnprintf(buf,sizeof(buf),format,arg);
	va_end(arg);
    }
    else
	*buf = 0;

    return PutScriptVars(ps,mode,buf);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			CpuStatus_t			///////////////
///////////////////////////////////////////////////////////////////////////////

CpuStatus_t GetCpuStatus ( CpuStatus_t * prev_cpuinfo )
{
    //--- retrieve static data

    static int n_cpu = -1;
    static int clock_ticks = 100;

    char buf[1000];

    if ( n_cpu < 0 )
    {
	n_cpu = 0;
	FILE *f = fopen("/proc/cpuinfo","r");
	if (f)
	{
	    while (fgets(buf,sizeof(buf),f))
		if ( !memcmp(buf,"processor",9) && buf[9] <= ' ' )
		    n_cpu++;
	    fclose(f);
	}

	clock_ticks = sysconf(_SC_CLK_TCK);
	if ( clock_ticks <= 0 )
	    clock_ticks = 100;
    }


    //--- retrieve cpu data

    CpuStatus_t current =
    {
	.valid		= true,
	.nsec		= GetTimerNSec(),
	.n_cpu		= n_cpu,
	.clock_ticks	= clock_ticks
    };


    // man 5 proc | oo +/proc/loadavg
    FILE *f_load = fopen("/proc/loadavg","r");
    if (f_load)
    {
	if (fgets(buf,sizeof(buf),f_load))
	{
	    char *ptr = buf;
	    current.loadavg_1m  = strtod(ptr,&ptr);
	    current.loadavg_5m  = strtod(ptr,&ptr);
	    current.loadavg_15m = strtod(ptr,&ptr);

	    current.running_proc = strtol(ptr,&ptr,10);
	    current.total_proc   = strtol(ptr+1,&ptr,10);
	}
	fclose(f_load);
    }


    // man 5 proc | oo +/proc/stat
    FILE * f_stat = fopen("/proc/stat","r");
    if (f_stat)
    {
	while (fgets(buf,sizeof(buf),f_stat))
	    if ( !memcmp(buf,"cpu ",4) )
	    {
		char *ptr	= buf;
		current.user	= strtol(ptr+4,&ptr,10) * 10000 / clock_ticks;
		current.nice	= strtol(ptr,&ptr,10)   * 10000 / clock_ticks;
		current.system	= strtol(ptr,&ptr,10)   * 10000 / clock_ticks;
		current.idle	= strtol(ptr,&ptr,10)   * 10000 / clock_ticks;
		break;
	    }
	fclose(f_stat);
    }

    if (!prev_cpuinfo)
	return current; 


    //--- get delta for result

    if (!prev_cpuinfo->valid)
	*prev_cpuinfo = current;

    CpuStatus_t res = current;
    res.nsec	-= prev_cpuinfo->nsec;
    res.user	-= prev_cpuinfo->user;
    res.nice	-= prev_cpuinfo->nice;
    res.system	-= prev_cpuinfo->system;
    res.idle	-= prev_cpuinfo->idle;

    *prev_cpuinfo = current;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintCpuStatus ( const CpuStatus_t * cpuinfo )
{
    if ( !cpuinfo || !cpuinfo->valid )
	return "INVALID";

    return PrintCircBuf(
	"ticks=%d, cpus=%d, times=%lld+%lld+%lld+%lld=%lld, avg=%4.2f,%4.2f,%4.2f %u/%u, nsec=%llu",
	cpuinfo->clock_ticks,
	cpuinfo->n_cpu,
	cpuinfo->user, cpuinfo->nice, cpuinfo->system, cpuinfo->idle,
	cpuinfo->user + cpuinfo->nice + cpuinfo->system + cpuinfo->idle,
	cpuinfo->loadavg[0], cpuinfo->loadavg[1], cpuinfo->loadavg[2],
	cpuinfo->running_proc, cpuinfo->total_proc,
	cpuinfo->nsec );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			MemoryStatus_t			///////////////
///////////////////////////////////////////////////////////////////////////////

MemoryStatus_t GetMemoryStatus(void)
{
    MemoryStatus_t mem = {0};

    FILE *f = fopen("/proc/meminfo","r");
    if (f)
    {
	char buf[200];
	int count = 5; // abort, if all 5 params scanned

	while ( count > 0 && fgets(buf,sizeof(buf),f) )
	{
	    ccp colon = strchr(buf,':');
	    if (colon)
	    {
		u64 num = strtoul(colon+1,0,10) * 1024;
		switch (colon-buf)
		{
		    case 6:
			if (!memcmp(buf,"Cached",6)) { count--; mem.cached = num; break; }
			break;

		    case 7:
			if (!memcmp(buf,"MemFree",7)) { count--; mem.free = num; break; }
			if (!memcmp(buf,"Buffers",7)) { count--; mem.buffers = num; break; }
			break;

		    case 8:
			if (!memcmp(buf,"MemTotal",8)) { count--; mem.total = num; break; }
			break;

		    case 12:
			if (!memcmp(buf,"MemAvailable",12)) { count--; mem.avail = num; break; }
			break;
		}
	    }
	}
	fclose(f);
	mem.used = mem.total - mem.free - mem.buffers - mem.cached;
    }
    return mem;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintMemoryStatus ( const MemoryStatus_t * ms )
{
    MemoryStatus_t temp;
    if (!ms)
    {
	temp = GetMemoryStatus();
	ms = &temp;
    }
	
    const sizeform_mode_t mode = DC_SFORM_NARROW;
    return PrintCircBuf(
	"tot=%s, free=%s, avail=%s, used=%s, buf=%s, cache=%s\n",
	PrintNumberU7(0,0,ms->total,mode),
	PrintNumberU7(0,0,ms->free,mode),
	PrintNumberU7(0,0,ms->avail,mode),
	PrintNumberU7(0,0,ms->used,mode),
	PrintNumberU7(0,0,ms->buffers,mode),
	PrintNumberU7(0,0,ms->cached,mode) );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

