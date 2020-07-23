
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

#include <sys/types.h>
#include <sys/ioctl.h>
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
	memset(dest,0,sizeof(*dest));
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * TouchFileAttrib
(
    FileAttrib_t	* dest		// valid destination attribute
)
{
    DASSERT(dest);
    dest->mtime = dest->ctime = dest->atime = dest->itime = time(0);
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
	memset(dest,0,sizeof(*dest));
	if ( S_ISREG(src_stat->st_mode) )
	{
	    dest->mtime = src_stat->st_mtime;
	    dest->ctime = src_stat->st_ctime;
	    dest->itime = dest->mtime > dest->ctime ? dest->mtime : dest->ctime;
	    dest->atime = src_stat->st_atime;
	    dest->size  = src_stat->st_size;
	}
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
	if ( dest->mtime < src_fa->mtime ) dest->mtime = src_fa->mtime;
	if ( dest->ctime < src_fa->ctime ) dest->ctime = src_fa->ctime;
	if ( dest->atime < src_fa->atime ) dest->atime = src_fa->atime;
	if ( dest->itime < src_fa->itime ) dest->itime = src_fa->itime;

	if ( dest->size  < src_fa->size )  dest->size  = src_fa->size;

	dest->mode = src_fa->mode;
    }

    if (src_stat)
    {
	if ( S_ISREG(src_stat->st_mode) )
	{
	    if ( dest->mtime < src_stat->st_mtime ) dest->mtime = src_stat->st_mtime;
	    if ( dest->ctime < src_stat->st_ctime ) dest->ctime = src_stat->st_ctime;
	    if ( dest->atime < src_stat->st_atime ) dest->atime = src_stat->st_atime;

	    if ( dest->itime < dest->mtime ) dest->itime = dest->mtime;
	    if ( dest->itime < dest->ctime ) dest->itime = dest->ctime;

	    if ( dest->size  < src_stat->st_size ) dest->size  = src_stat->st_size;
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

    time_t mtime = fa->mtime;
    if (!mtime)
    {
	mtime = fa->itime;
	if (!mtime)
	{
	    mtime = fa->ctime;
	    if (!mtime)
		mtime = fa->atime;
	}
    }
    fa->mtime = mtime;

    if (!fa->itime)
	fa->itime = fa->ctime > mtime ? fa->ctime : mtime;

    if (!fa->ctime)
	fa->ctime = fa->itime > mtime ? fa->itime : mtime;

    if (!fa->atime)
	fa->atime = fa->itime > fa->ctime ? fa->itime : fa->ctime;

    return fa;
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
		    else if ( set_time == 1 && f->fatt.mtime )
		    {
			struct utimbuf ubuf;
			ubuf.actime  = f->fatt.atime
				     ? f->fatt.atime : f->fatt.mtime;
			ubuf.modtime = f->fatt.mtime;
			utime(f->fname,&ubuf);
		    }
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
	PrintSize1024(sbuf,sizeof(sbuf),(u64)f->st.st_size,false);
	PrintSize1024(lbuf,sizeof(lbuf),(u64)limit,false);

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
	    memset(fatt,0,sizeof(*fatt));
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
    ASSERT(data);
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
	    ERROR0(ERR_SYNTAX,"File not found: %s\n",path);
	return ERR_SYNTAX;
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
    bool	init_mf,	// true: initalize 'mf' first

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

    if ( share_name && !*share_name ) share_name = progname;
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
	struct dirent dent;
	memset(&dent,0,sizeof(dent));
	for (;;)
	{
	    struct dirent *dptr;
	    stat = readdir_r(fdir,&dent,&dptr);
	    if ( stat || !dptr )
		break;

	    if ( !match || MatchPatternFull(match,dent.d_name) )
	    {
		stat = func(path,dptr,param);
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
					//   If real pathes are same: don't remove
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
	for (;;)
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
    bool		is_utf8		// true: enter UTF-8 mode
)
{
    DASSERT(buf);
    DASSERT(buf_size>1);

    char *dest = buf, *end = buf + buf_size - 1;
    TRACE("NormalizeFileName(%s,%d)\n",source,allow_slash);

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
			|| strchr("_+-=%'\"$%&,.!()[]{}<>",ch)
			|| ch == '/' && allow_slash )
		{
		    *dest++ = ch;
		    skip_space = false;
		}
	     #ifdef __CYGWIN__
		else if ( ch == '\\' && allow_slash )
		{
		    *dest++ = '/';
		    skip_space = false;
		}
	     #endif
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

    DASSERT( dest <= end );
    *dest = 0;
    return dest;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __CYGWIN__

 uint IsWindowsDriveSpec
 (
    // returns the length of the found windows drive specification (0|2|3)

    ccp			src		// valid string
 )
 {
    DASSERT(src);

    if ( ( *src >= 'a' && *src <= 'z' || *src >= 'A' && *src <= 'Z' )
	&& src[1] == ':' )
    {
	if (!src[2])
	    return 2;

	if ( src[2] == '/' || src[2] == '\\' )
	    return 3;
    }
    return 0;
 }

#endif // __CYGWIN__

///////////////////////////////////////////////////////////////////////////////

#ifdef __CYGWIN__

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
	*buf = 0;
	return 0;
    }

    char * end = buf + buf_size - 1;
    char * dest = buf;

    if (   ( *src >= 'a' && *src <= 'z' || *src >= 'A' && *src <= 'Z' )
	&& src[1] == ':'
	&& ( src[2] == 0 || src[2] == '/' || src[2] == '\\' ))
    {
	memcpy(buf,prefix,sizeof(prefix));
	dest = buf + sizeof(prefix)-1;
	*dest++ = tolower((int)*src); // cygwin needs the '(int)'
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
    }
    ASSERT( dest < buf + buf_size );

    while ( dest < end && *src )
	if ( *src == '\\' )
	{
	    *dest++ = '/';
	    src++;
	}
	else
	    *dest++ = *src++;

    *dest = 0;
    ASSERT( dest < buf + buf_size );
    return dest - buf;
 }

#endif // __CYGWIN__

///////////////////////////////////////////////////////////////////////////////

#ifdef __CYGWIN__

 char * AllocNormalizedFilenameCygwin
 (
    // returns an alloced buffer with the normalized filename

    ccp source				// NULL or string
 )
 {
    char buf[PATH_MAX];
    const uint len = NormalizeFilenameCygwin(buf,sizeof(buf),source);
    char * result = MEMDUP(buf,len); // MEMDUP alloces +1 byte and set it to NULL
    DASSERT(buf[len]==0);
    return result;
 }

#endif // __CYGWIN__

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
    uint	events	// bit field: POLLIN|POLLOUT|POLLERR
)
{
    if ( sock == -1 || !(events&(POLLIN|POLLOUT|POLLERR)) )
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
    if ( events & POLLERR )
	FD_SET(sock,&fdl->exceptfds);
    return ~0;
}

///////////////////////////////////////////////////////////////////////////////

uint GetEventFDList
(
    // returns bit field: POLLIN|POLLOUT|POLLERR

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
	revents |= POLLERR;
    return revents;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int WaitFDList
(
    FDList_t	*fdl		// valid socket list
)
{
    DASSERT(fdl);

    int stat;
    u64 now_usec = GetTimeUSec(false);

    if (fdl->use_poll)
    {
	int timeout;
	if ( fdl->timeout_usec > now_usec )
	{
	    u64 delta = ( fdl->timeout_usec - now_usec ) / 1000;
	    timeout = delta > 0x7fffffffull ? -1 : delta;
	}
	else
	    timeout = 0;

	stat = poll( fdl->poll_list, fdl->poll_used, timeout );
    }
    else
    {
	struct timeval tv, *ptv = &tv;
	if ( fdl->timeout_usec > now_usec )
	{
	    const u64 delta = fdl->timeout_usec - now_usec;
	    if ( delta > 1000000ull * 0x7fffffffull )
		ptv = NULL;
	    else
	    {
		tv.tv_sec  = delta / 1000000;
		tv.tv_usec = delta % 1000000;
	    }
	}
	else
	    tv.tv_sec = tv.tv_usec = 0;

	stat = select( fdl->max_fd+1, &fdl->readfds, &fdl->writefds,
			&fdl->exceptfds, ptv );
    }

    fdl->now_usec = GetTimeUSec(false);
    fdl->cur_usec = fdl->now_usec - now_usec;
    fdl->wait_usec += fdl->cur_usec;
    fdl->wait_count++;
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int PWaitFDList
(
    FDList_t	*fdl,		// valid socket list
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

    struct timespec ts, *pts = &ts;
    u64 now_usec = GetTimeUSec(false);
    if ( fdl->timeout_usec > now_usec )
    {
	const u64 delta = fdl->timeout_usec - now_usec;
	if ( delta > 1000000ull * 0x7fffffffull )
	    pts = NULL;
	else
	{
	    ts.tv_sec  = delta / 1000000;
	    ts.tv_nsec = delta % 1000000 * 1000;
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

    fdl->now_usec = GetTimeUSec(false);
    fdl->cur_usec = fdl->now_usec - now_usec;
    fdl->wait_usec += fdl->cur_usec;
    fdl->wait_count++;
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

	for (;;)
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
	    while ( isalnum((int)*ptr) || *ptr == '-' || *ptr == '_' )
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

	tab->func(&rs,tab->user_table);
	if ( log_mode & RSL_UNUSED_NAMES )
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
	ResetParamField(&rs.param);
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

    return DecodeByMode(buf,buf_size,(ccp)it->data,-1,decode);
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
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

