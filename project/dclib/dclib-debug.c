
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

#define DCLIB_DEBUG_C 1
#include "dclib-debug.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

#include "dclib-basics.h"
#include "dclib-file.h"

#if ENABLE_HEXDUMP_WRAPPER > 0
  #include "dclib-xdump.h"
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			error messages			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp TRACE_PREFIX = "";
FILE *TRACE_FILE = 0;
FILE *MEM_LOG_FILE = 0;

ccp (*GetErrorNameHook)( int stat, ccp ret_not_found ) = 0;
ccp (*GetErrorTextHook)( int stat, ccp ret_not_found ) = 0;

ccp GENERIC_ERROR_MESSAGE = "generic err";

///////////////////////////////////////////////////////////////////////////////

ccp GetErrorName
(
    int		stat,		// error status, err := abs(stat)
    ccp		ret_not_found	// not NULL: return this, if no message is found
				//     NULL: return a generic message
)
{
    if (GetErrorNameHook)
    {
	ccp res = GetErrorNameHook(stat,0);
	if ( res && *res )
	    return res;
    }

    if ( stat < 0 )
	stat = -stat;

    switch(stat)
    {
	case ERR_OK:			return "OK";

	case ERR_DIFFER:		return "DIFFER";
	case ERR_NOTHING_TO_DO:		return "NOTHING TO DO";
	case ERR_SOURCE_FOUND:		return "SOURCE FOUND";
	case ERR_NO_SOURCE_FOUND:	return "NO SOURCE";
	case ERR_JOB_IGNORED:		return "JOB IGNORED";
	case ERR_SUBJOB_WARNING:	return "SUB JOB WARNINGS";
	case ERR_NOT_EXISTS:		return "NOT EXISTS";

	case ERR_WARNING:		return "WARNING";

	case ERR_WRONG_FILE_TYPE:	return "WRONG FILE TYPE";
	case ERR_INVALID_FILE:		return "INVALID FILE";
	case ERR_INVALID_VERSION:	return "INVALID VERSION";
	case ERR_INVALID_DATA:		return "INVALID DATA";

	case ERR_ENCODING:		return "ENCODING FAILED";
	case ERR_DECODING:		return "DECODING FAILED";
	case ERR_ALREADY_EXISTS:	return "FILE ALREADY EXISTS";
	case ERR_SUBJOB_FAILED:		return "SUB JOB FAILED";

	case ERR_CANT_REMOVE:		return "CAN'T REMOVE FILE";
	case ERR_CANT_RENAME:		return "CAN'T RENAME FILE";
	case ERR_CANT_CLOSE:		return "CAN'T CLOSE FILE";
	case ERR_CANT_CONNECT:		return "CAN'T CONNECT";
	case ERR_CANT_OPEN:		return "CAN'T OPEN FILE";
	case ERR_CANT_APPEND:		return "CAN'T APPEND FILE";
	case ERR_CANT_CREATE:		return "CAN'T CREATE FILE";
	case ERR_CANT_CREATE_DIR:	return "CAN'T CREATE DIRECTORY";

	case ERR_READ_FAILED:		return "READ FILE FAILED";
	case ERR_REMOVE_FAILED:		return "REMOVE FILE FAILED";
	case ERR_WRITE_FAILED:		return "WRITE FILE FAILED";
	case ERR_DATABASE:		return "DATABASE ACCESS FAILED";

	case ERR_MISSING_PARAM:		return "MISSING PARAMETERS";
	case ERR_SEMANTIC:		return "SEMANTIC ERROR";
	case ERR_SYNTAX:		return "SYNTAX ERROR";

	case ERR_INTERRUPT:		return "INTERRUPT";

	case ERR_ERROR:			return "ERROR";

	case ERR_NOT_IMPLEMENTED:	return "NOT IMPLEMENTED YET";
	case ERR_INTERNAL:		return "INTERNAL ERROR";
	case ERR_OUT_OF_MEMORY:		return "OUT OF MEMORY";

	case ERR_FATAL:			return "FATAL ERROR";
    }

    if ( ret_not_found != GENERIC_ERROR_MESSAGE )
	return ret_not_found;

    //--- generic messages

    if (GetErrorNameHook)
    {
	ccp res = GetErrorNameHook(stat,GENERIC_ERROR_MESSAGE);
	if ( res && *res )
	    return res;
    }

    if ( stat >= ERU_WARN_00 & stat <= ERU_WARN_XX )
    {
	const uint buf_size = 20;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER WARNING #%02u", stat - ERU_WARN_00 );
	return buf;
    }

    if ( stat >= ERU_ERROR1_00 & stat <= ERU_ERROR1_XX )
    {
	const uint buf_size = 20;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER ERROR #%02u", stat - ERU_ERROR1_00 );
	return buf;
    }

    if ( stat >= ERU_ERROR2_00 & stat <= ERU_ERROR2_XX )
    {
	const uint buf_size = 20;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER ERROR #%02u",
		stat - ERU_ERROR2_00 + ERU_ERROR1_XX+1 - ERU_ERROR1_00 );
	return buf;
    }

    if ( stat >= ERU_FATAL_00 & stat <= ERU_FATAL_XX )
    {
	const uint buf_size = 24;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER FATAL ERROR #%02u", stat - ERU_FATAL_00 );
	return buf;
    }

    ccp res = GetErrorName(stat+1,"");
    return res && *res ? res : "?";
}

///////////////////////////////////////////////////////////////////////////////

ccp GetErrorText
(
    int		stat,		// error status, err := abs(stat)
    ccp		ret_not_found	// not NULL: return this, if no message is found
				//     NULL: return a generic message
)
{
    if (GetErrorTextHook)
    {
	ccp res = GetErrorTextHook(stat,0);
	if (res)
	    return res;
    }

    if ( stat < 0 )
	stat = -stat;

    switch(stat)
    {
	case ERR_OK:			return "Ok";

	case ERR_DIFFER:		return "Files differ";
	case ERR_NOTHING_TO_DO:		return "Nothing to do";
	case ERR_SOURCE_FOUND:		return "Source found";
	case ERR_NO_SOURCE_FOUND:	return "No source found";
	case ERR_JOB_IGNORED:		return "Job ignored";
	case ERR_SUBJOB_WARNING:	return "Sub job had warnings";
	case ERR_NOT_EXISTS:		return "File does not exists";

	case ERR_WARNING:		return "Unspecific warning";

	case ERR_WRONG_FILE_TYPE:	return "Wrong type of file";
	case ERR_INVALID_FILE:		return "Invalid file";
	case ERR_INVALID_VERSION:	return "Invalid file version";
	case ERR_INVALID_DATA:		return "Invalid data";

	case ERR_ENCODING:		return "Encoding data failed";
	case ERR_DECODING:		return "Decoding data failed";
	case ERR_ALREADY_EXISTS:	return "File already exists";
	case ERR_SUBJOB_FAILED:		return "Sub job failed";

	case ERR_CANT_REMOVE:		return "Can't remove file";
	case ERR_CANT_RENAME:		return "Can't rename file";
	case ERR_CANT_CLOSE:		return "Can't close file";
	case ERR_CANT_CONNECT:		return "Can't connect";
	case ERR_CANT_OPEN:		return "Can't open file";
	case ERR_CANT_APPEND:		return "Can't open file for appending";
	case ERR_CANT_CREATE:		return "Can't create file";
	case ERR_CANT_CREATE_DIR:	return "Can't create directory";

	case ERR_READ_FAILED:		return "Reading from file failed";
	case ERR_REMOVE_FAILED:		return "Removing a file failed";
	case ERR_WRITE_FAILED:		return "Writing to file failed";
	case ERR_DATABASE:		return "Access to database failed";

	case ERR_MISSING_PARAM:		return "Missing at least one parameter";
	case ERR_SEMANTIC:		return "Semantic error";
	case ERR_SYNTAX:		return "Syntax error";

	case ERR_INTERRUPT:		return "Program interrupted by user";

	case ERR_ERROR:			return "Unspecific error";

	case ERR_NOT_IMPLEMENTED:	return "Not implemented yet";
	case ERR_INTERNAL:		return "Internal error";
	case ERR_OUT_OF_MEMORY:		return "Allocation of dynamic memory failed";

	case ERR_FATAL:			return "Unspecific fatal error";
    }

    if ( ret_not_found != GENERIC_ERROR_MESSAGE )
	return ret_not_found;

    //--- generic messages

    if (GetErrorTextHook)
    {
	ccp res = GetErrorTextHook(stat,GENERIC_ERROR_MESSAGE);
	if ( res && *res )
	    return res;
    }

    if ( stat >= ERU_WARN_00 & stat <= ERU_WARN_XX )
    {
	const uint buf_size = 20;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER WARNING #%02u", stat - ERU_WARN_00 );
	return buf;
    }

    if ( stat >= ERU_ERROR1_00 & stat <= ERU_ERROR1_XX )
    {
	const uint buf_size = 20;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER ERROR #%02u", stat - ERU_ERROR1_00 );
	return buf;
    }

    if ( stat >= ERU_ERROR2_00 & stat <= ERU_ERROR2_XX )
    {
	const uint buf_size = 20;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER ERROR #%02u",
		stat - ERU_ERROR2_00 + ERU_ERROR1_XX+1 - ERU_ERROR1_00 );
	return buf;
    }

    if ( stat >= ERU_FATAL_00 & stat <= ERU_FATAL_XX )
    {
	const uint buf_size = 24;
	char *buf = GetCircBuf(buf_size);
	snprintf(buf,buf_size,"USER FATAL ERROR #%02u", stat - ERU_FATAL_00 );
	return buf;
    }

    ccp res = GetErrorText(stat+1,0);
    return res && *res ? res : "?";
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void ListErrorCodes
(
    FILE	* f,		// valid output stream
    int		indent,		// indent of output
    bool	all		// true: print all user reserved messages too.
)
{
    if (!f)
	return;

    indent = NormalizeIndent(indent);
    fprintf(f,"\n%*sList of error messages:\n\n",indent,"");

    ccp p2 = all ? 0 : "";

    uint i, fw_name = 4, fw_text = 7;
    for ( i = 0; i < ERR__N; i++ )
    {
	ccp name = GetErrorName(i,p2);
	if (name)
	{
	    const uint len = strlen(name);
	    if ( fw_name < len )
		 fw_name = len;
	}

	ccp text = GetErrorText(i,p2);
	{
	    const uint len = strlen(text);
	    if ( fw_text < len )
		 fw_text = len;
	}
    }

    fprintf(f,
	"%*s err | %-*s | %-*s\n"
	"%*s-----+%.*s+%.*s\n",
	indent, "",
	fw_name, "name",
	fw_text, "message",
	indent, "",
	fw_name+2, Minus300,
	fw_text+2, Minus300 );

    for ( i = 0; i < ERR__N; i++ )
    {
	ccp name = GetErrorName(i,p2);
	ccp text = GetErrorText(i,p2);
	if ( name && *name || text && *text )
	    fprintf(f,"%*s%4u | %-*s | %-*s\n",
		indent, "", i,
		fw_name, name ? name : "-",
		fw_text, text ? text : "-" );

	if ( i == ERR_OK || i == ERR_WARNING || i == ERR_ERROR )
	    fprintf(f,"%*s-----+%.*s+%.*s\n",
		indent, "",
		fw_name+2, Minus300,
		fw_text+2, Minus300 );
    }

    fprintf(f,"%*s-----+%.*s+%.*s\n\n",
	indent, "",
	fw_name+2, Minus300,
	fw_text+2, Minus300 );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			error handling			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp progpath	= 0;
ccp progdir	= 0;
ccp progname	= 0;

ccp toolname	= 0;
ccp toolversion	= 0;
ccp tooltitle	= 0;

bool multi_processing	= false;
enumError last_error	= ERR_OK;
enumError max_error	= ERR_OK;
u32 error_count		= 0;

///////////////////////////////////////////////////////////////////////////////

enumError PrintError ( ccp func, ccp file, uint line,
			int syserr, enumError err_code, ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    const int stat = PrintErrorArg(func,file,line,syserr,err_code,format,arg);
    va_end(arg);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintErrorFile ( ccp func, ccp file, uint line, struct File_t *F,
			int syserr, enumError err_code, ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    const int stat = PrintErrorArg(func,file,line,syserr,err_code,format,arg);
    va_end(arg);
    return F ? RegisterFileError(F,stat) : stat;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintErrorArg ( ccp func, ccp file, unsigned int line,
		int syserr, enumError err_code, ccp format, va_list arg )
{
    fflush(stdout);
    if (!stdwrn)
    {
	SetupStdMsg();
	if (!stdwrn)
	{
	    stdwrn = stderr;
	    if (!stdwrn)
		return err_code;
	}
    }

    char msg[1000];
    if (!progname)
	progname = "?";
    const int plen = strlen(progname)+2;

    if ( !format && err_code > ERR_NOT_IMPLEMENTED )
	format = "Program is aborted immediately!";

    if (format)
    {
	vsnprintf(msg,sizeof(msg),format,arg);
	msg[sizeof(msg)-2] = 0;

	const int mlen = strlen(msg);
	if ( mlen > 0 && msg[mlen-1] != '\n' )
	{
	    msg[mlen]   = '\n';
	    msg[mlen+1] = 0;
	}
    }
    else
	StringCat2S(msg,sizeof(msg),GetErrorText(err_code,0),"\n");

    ccp prefix = err_code == ERR_OK ? "" : err_code <= ERR_WARNING ? "! " : "!! ";
    const int fw = GetTermWidth(80,40) - 1;

 #ifdef DEBUG
    TRACE("%s%s #%d [%s] in %s() @ %s#%d\n",
		prefix, err_code <= ERR_WARNING ? "WARNING" : "ERROR",
		err_code, GetErrorName(err_code,0), func, file, line );
    TRACE("%s%*s%s",prefix,plen,"",msg);
    if (syserr)
	TRACE("!! %s(), ERRNO=%d: %s\n",__FUNCTION__,syserr,strerror(syserr));
    fflush(TRACE_FILE);
 #endif

 #if defined(EXTENDED_ERRORS)
    if ( err_code > ERR_WARNING )
 #else
    if ( err_code >= ERR_NOT_IMPLEMENTED )
 #endif
    {
	ccp coln = EmptyString;
	ccp col1 = EmptyString;
	ccp col0 = EmptyString;
	if (IsFileColorized(stdwrn))
	{
	    coln = GetTextMode(1,TTM_COL_INFO);
	    col1 = GetTextMode(1, err_code > ERR_WARNING ? TTM_COL_WARN : TTM_COL_HINT );
	    col0 = TermTextModeReset;
	}

	if ( stdwrn == stderr )
	{
	    if ( err_code > ERR_WARNING )
		fprintf(stdwrn,"%s%s%s:%s ERROR #%d [%s] in %s() @ %s#%d%s\n",
		    prefix, coln, progname, col1, err_code,
		    GetErrorName(err_code,0), func, file, line, col0 );
	    else
		fprintf(stdwrn,"%s%s%s:%s WARNING in %s() @ %s#%d%s\n",
		    prefix, coln, progname, col1, func, file, line, col0 );
	}
	else
	{
	    if ( err_code > ERR_WARNING )
		fprintf(stdwrn,"%s%s%s:%s ERROR #%d [%s]%s\n",
		    prefix, coln, progname, col1, err_code,
		    GetErrorName(err_code,0), col0 );
	    else
		fprintf(stdwrn,"%s%s%s:%s WARNING%s\n",
		    prefix, coln, progname, col1, col0 );
	}

     #if defined(EXTENDED_ERRORS) && EXTENDED_ERRORS > 1
	fprintf(stdwrn,"%s -> %s/%s?annotate=%d#l%d\n",
		prefix, URI_VIEWVC, file, REVISION_NEXT, line );
     #endif

	fputs(prefix,stdwrn);
	PutLines(stdwrn,plen,fw,0,prefix,msg,0);
    }
    else
    {
	fprintf(stdwrn,"%s%s:",prefix,progname);
	PutLines(stdwrn,plen,fw,strlen(progname)+1,prefix,msg,0);
    }

    if (syserr)
    {
	fprintf(stdwrn,"%s%*s-> ",prefix,plen,"");
	snprintf(msg,sizeof(msg),"%s [%d]",strerror(syserr),syserr);
	PutLines(stdwrn,plen+3,fw,plen+3,prefix,msg,0);
    }
    fflush(stdwrn);

    if ( err_code > ERR_OK )
	error_count++;

    last_error = err_code;
    if ( max_error < err_code )
	max_error = err_code;

    if ( err_code > ERR_NOT_IMPLEMENTED )
	exit(err_code);

    return err_code;
}

///////////////////////////////////////////////////////////////////////////////

enumError PrintErrorStat ( enumError err, int verbose, ccp cmdname )
{
    if (   verbose > 0 && err >= ERR_WARNING
	|| verbose > 1 && err
	|| err == ERR_NOT_IMPLEMENTED )
    {
        if (!stdwrn)
	{
	    SetupStdMsg();
	    if (!stdwrn)
	    {
		stdwrn = stderr;
		if (!stdwrn)
		    return err;
	    }
	}

	fprintf(stdwrn,"%s: Command '%s' returns with status #%d [%s]\n",
			progname, cmdname, err, GetErrorName(err,0) );
    }
    return err;
}

///////////////////////////////////////////////////////////////////////////////

void mark_used ( ccp name, ... )
{
}

///////////////////////////////////////////////////////////////////////////////

#if HAVE_PRINT

 void PRINT_TIME ( time_t time, ccp title )
 {
    struct tm utc, loc;
    gmtime_r(&time,&utc);
    localtime_r(&time,&loc);
    PRINT(	"%s: UTC= %4u-%02u-%02u %2u:%02u:%02u dst=%d"
		"  LOCAL= %4u-%02u-%02u %2u:%02u:%02u dst=%d\n",
		title ? title : "TIME",
		utc.tm_year+1900, utc.tm_mon+1, utc.tm_mday,
		utc.tm_hour, utc.tm_min, utc.tm_sec, utc.tm_isdst,
		loc.tm_year+1900, loc.tm_mon+1, loc.tm_mday,
		loc.tm_hour, loc.tm_min, loc.tm_sec, loc.tm_isdst );
 }

#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    hexdump			///////////////
///////////////////////////////////////////////////////////////////////////////

#if ENABLE_HEXDUMP_WRAPPER > 1
 int enable_xdump_wrapper = 1;
#elif ENABLE_HEXDUMP_WRAPPER
 int enable_xdump_wrapper = 0;
#endif

ccp  hexdump_prefix = "";
ccp  hexdump_eol    = "\n";
bool hexdump_align  = false;

//-----------------------------------------------------------------------------

#if ENABLE_HEXDUMP_WRAPPER > 0

static void SetupXDumpWrapper
(
    XDump_t	*xd,
    FILE	*f,
    int		indent,
    u64		addr,
    int		addr_fw,
    int		row_len
)
{
    DASSERT(xd);
    InitializeXDump(xd);

    xd->f		= f;
    xd->prefix		= hexdump_prefix;
    xd->eol		= hexdump_eol;
    xd->mode_align	= hexdump_align;
    xd->indent		= indent;
    xd->start_addr	= addr;
    xd->min_addr_fw	= addr_fw;
    xd->min_width	= row_len;
    xd->format		= XDUMPF_INT_1;
    xd->print_format	= false;
    xd->print_summary	= false;
    xd->print_diff_sep	= false;

    hexdump_prefix	= EmptyString;
    hexdump_eol		= "\n";
    hexdump_align	= false;
}

#endif

//-----------------------------------------------------------------------------

uint HexDump16 ( FILE * f, int indent, u64 addr,
		 const void * data, size_t count )
{
    // return the number of printed lines
    return HexDump(f,indent,addr,4,16,data,count);
}

//-----------------------------------------------------------------------------

uint HexDump ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * p_data, size_t count )
{
    // return the number of printed lines
    if ( !f || !p_data || !count )
	return 0;

 #if ENABLE_HEXDUMP_WRAPPER == 1 || ENABLE_HEXDUMP_WRAPPER == 2
  if (enable_xdump_wrapper)
 #endif

 #if ENABLE_HEXDUMP_WRAPPER > 0
  {
    XDump_t xd;
    SetupXDumpWrapper(&xd,f,indent,addr,addr_fw,row_len);
    const int stat = XDump(&xd,p_data,count,true);
    return stat < 0 ? 0 : stat;
  }
 #endif

 #if ENABLE_HEXDUMP_WRAPPER < 3

    const int MAX_LEN = 256;
    char buf[MAX_LEN+2];

    const u8 * data = (const u8 *)p_data;

    indent = NormalizeIndent(indent);
    addr_fw = NormalizeIndent(addr_fw);

    const bool show_ascii = row_len >= 0;
    if ( row_len < 0 )
	row_len = -row_len;
    else if ( row_len < 1 )
	row_len = 16;
    else if ( row_len > MAX_LEN )
	row_len = MAX_LEN;

    uint skip = 0;
    if ( (s64)addr != -1 )
    {
	const int fw = snprintf(buf,sizeof(buf),"%llx",addr+count-1);
	if ( addr_fw < fw )
	     addr_fw = fw;

	if (hexdump_align)
	{
	    skip   = addr % row_len;
	    addr  -= skip;
	}
    }

    uint line_count = 0;
    while ( count > 0 )
    {
	if ( (s64)addr == -1 )
	    fprintf(f,"%s%*s", hexdump_prefix, indent,"" );
	else
	{
	    fprintf(f,"%s%*s%*llx:", hexdump_prefix, indent,"", addr_fw, addr );
	    addr += row_len;
	}
	char * dest = buf;
	if (!skip)
	    *dest++ = ':';

	int i;
	for ( i = 0; i < row_len; i++ )
	{
	    if ( skip > 0 )
	    {
		fprintf(f,"%s   ", i&3 ? "" : " " );
		*dest++ = ' ';
		if (!--skip)
		    *dest++ = ':';
	    }
	    else
	    {
		u8 ch = *data++;
		if ( count > 0 )
		{
		    count--;
		    fprintf(f,"%s%02x ", i&3 ? "" : " ", ch );
		    *dest++ = ch < ' ' || ch >= 0x7f ? '.' : ch;
		}
		else
		    fprintf(f,"%s   ", i&3 ? "" : " " );
	    }
	}
	*dest = 0;
	if (show_ascii)
	    fprintf(f,"%s:%s",buf,hexdump_eol);
	else
	    fputs(hexdump_eol,f);
	line_count++;
    }

    hexdump_prefix = "";
    hexdump_eol    = "\n";
    hexdump_align  = false;
    return line_count;
 #endif
}

//-----------------------------------------------------------------------------

void HexDiff16 ( FILE * f, int indent, u64 addr,
		 const void * data1, size_t count1,
		 const void * data2, size_t count2 )
{
    HexDiff(f,indent,addr,4,16,data1,count1,data2,count2);
}

//-----------------------------------------------------------------------------

void HexDiff ( FILE * f, int indent, u64 addr, int addr_fw, int row_len,
		const void * p_data1, size_t count1,
		const void * p_data2, size_t count2 )
{
    if (!f)
	return;

 #if ENABLE_HEXDUMP_WRAPPER == 1 || ENABLE_HEXDUMP_WRAPPER == 2
  if (enable_xdump_wrapper)
 #endif

 #if ENABLE_HEXDUMP_WRAPPER > 0
  {
    XDump_t xd;
    SetupXDumpWrapper(&xd,f,indent,addr,addr_fw,row_len);
    XDiff(&xd, p_data1,count1,true, p_data2,count2,true, false );
    return;
  }
 #endif

 #if ENABLE_HEXDUMP_WRAPPER < 3

    const int MAX_LEN = 256;
    char buf[MAX_LEN+2];

    const u8 * data1 = p_data1 ? (const u8 *)p_data1 : (u8*)EmptyString;
    const u8 * data2 = p_data2 ? (const u8 *)p_data2 : (u8*)EmptyString;

    indent  = NormalizeIndent(indent);
    addr_fw = NormalizeIndent(addr_fw);

    const bool show_ascii = row_len >= 0;
    if ( row_len < 0 )
	row_len = -row_len;
    else if ( row_len < 1 )
	row_len = 16;
    else if ( row_len > MAX_LEN )
	row_len = MAX_LEN;

    uint skip = 0;

    if ( (s64)addr != -1 )
    {
	const size_t count = count1 > count2 ? count1 : count2;
	const int fw = snprintf(buf,sizeof(buf),"%llx",addr+count-1);
	if ( addr_fw < fw )
	     addr_fw = fw;

	if (hexdump_align)
	{
	    skip   = addr % row_len;
	    addr  -= skip;
	}
    }

    while ( count1 > 0 && count2 > 0 )
    {
	size_t count = count1 < count2 ? count1 : count2;
	if ( count > row_len )
	    count = row_len;
	noPRINT(" cnt=%zd,%zd -> %zd;  cmp=%d\n",
		count1, count2, count, memcmp(data1,data2,count) );

	if (memcmp(data1,data2,count))
	{
	    //--- print first line

	    if (IS_M1(addr))
		fprintf(f,"%s%*s<", hexdump_prefix, indent,"" );
	    else
		fprintf(f,"%s%*s<%*llx:", hexdump_prefix, indent,"", addr_fw, addr );

	    char * dest = buf;
	    if (!skip)
		*dest++ = ':';

	    int i;
	    for ( i = 0; i < row_len; i++ )
	    {
		if ( skip > 0 )
		{
		    fprintf(f,"%s   ", i&3 ? "" : " " );
		    *dest++ = ' ';
		    if (!--skip)
			*dest++ = ':';
		}
		else
		{
		    u8 ch = data1[i];
		    if ( i < count1 )
		    {
			fprintf(f,"%s%02x ", i&3 ? "" : " ", ch );
			*dest++ = ch < ' ' || ch >= 0x7f ? '.' : ch;
		    }
		    else
			fprintf(f,"%s   ", i&3 ? "" : " " );
		}
	    }
	    *dest = 0;
	    if (show_ascii)
		fprintf(f,"%s:%s",buf,hexdump_eol);
	    else
		fputs(hexdump_eol,f);


	    //--- print second line

	    if (IS_M1(addr))
		fprintf(f,"%s%*s>", hexdump_prefix, indent,"" );
	    else
		fprintf(f,"%s%*s>%*llx:", hexdump_prefix, indent,"", addr_fw, addr );

	    dest = buf;
	    if (!skip)
		*dest++ = ':';

	    for ( i = 0; i < row_len; i++ )
	    {
		if ( skip > 0 )
		{
		    fprintf(f,"%s   ", i&3 ? "" : " " );
		    *dest++ = ' ';
		    if (!--skip)
			*dest++ = ':';
		}
		else
		{
		    u8 ch = data2[i];
		    if ( i < count2 )
		    {
			if ( ch == data1[i] )
			    fprintf(f,"%s . ", i&3 ? "" : " " );
			else
			    fprintf(f,"%s%02x ", i&3 ? "" : " ", ch );
			*dest++ = ch < ' ' || ch >= 0x7f ? '.' : ch;
		    }
		    else
			fprintf(f,"%s   ", i&3 ? "" : " " );
		}
	    }
	    *dest = 0;
	    if (show_ascii)
		fprintf(f,"%s:%s",buf,hexdump_eol);
	    else
		fputs(hexdump_eol,f);
	}
	data1 += count;
	data2 += count;
	count1 -= count;
	count2 -= count;
	if (!IS_M1(addr))
	    addr += count;

    }
    hexdump_prefix = "";
    hexdump_eol    = "\n";
    hexdump_align  = false;

 #endif
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 trace functions		///////////////
///////////////////////////////////////////////////////////////////////////////

u_msec_t GetTimerMSec();

///////////////////////////////////////////////////////////////////////////////

static void trace_helper ( int print_stderr, ccp format, va_list arg )
{
    if (!TRACE_FILE)
    {
	char extend[16];
	if (multi_processing)
	    snprintf(extend,sizeof(extend),"-%u",getpid());
	else
	    *extend = 0;
	
	char fname[1000];
	if ( progname && *progname && *progname != '?' )
	    snprintf( fname, sizeof(fname), "_trace-%s%s.tmp", progname, extend );
	else
	    snprintf( fname, sizeof(fname), "_trace-%s.tmp", extend );

	TRACE_FILE = fopen(fname,"wb");
	if (TRACE_FILE)
	    fcntl(fileno(TRACE_FILE),F_SETFD,FD_CLOEXEC);
	else
	    TRACE_FILE = stderr;
    }

    unsigned msec = GetTimerMSec();

    if ( print_stderr || TRACE_FILE == stderr )
    {
	fflush(stdout);
	fprintf(stderr,"%4d.%03d  %s",msec/1000,msec%1000,TRACE_PREFIX);
	va_list arg2;
	va_copy(arg2,arg);
	vfprintf(stderr,format,arg2);
	va_end(arg2);
	fflush(stderr);
    }

    if ( TRACE_FILE && TRACE_FILE != stderr )
    {
	fprintf(TRACE_FILE,"%4d.%03d  %s",msec/1000,msec%1000,TRACE_PREFIX);
	vfprintf(TRACE_FILE,format,arg);
	fflush(TRACE_FILE);
    }
}

///////////////////////////////////////////////////////////////////////////////

#undef TRACE_ARG_FUNC

void TRACE_ARG_FUNC ( ccp format, va_list arg )
{
    trace_helper(0,format,arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef TRACE_FUNC

void TRACE_FUNC ( ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    trace_helper(0,format,arg);
    va_end(arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef PRINT_ARG_FUNC

void PRINT_ARG_FUNC ( ccp format, va_list arg )
{
    trace_helper(1,format,arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef PRINT_FUNC

void PRINT_FUNC ( ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    trace_helper(1,format,arg);
    va_end(arg);
}

///////////////////////////////////////////////////////////////////////////////

#undef BINGO_FUNC

void BINGO_FUNC ( ccp func, int line, ccp src )
{
 #if defined(DEBUG) && defined(TEST)
    if (colerr)
	PRINT_FUNC("%sBINGO!%s %s() #%d @ %s\n",
		colerr->magenta, colerr->reset, func, line, src );
    else
	PRINT_FUNC("BINGO! %s() #%d @ %s\n", func, line, src );
 #else
    if (colerr)
	fprintf(stderr,"%sBINGO!%s %s() #%d @ %s\n",
		colerr->magenta, colerr->reset, func, line, src );
    else
	fprintf(stderr,"BINGO! %s() #%d @ %s\n", func, line, src );
 #endif
}

///////////////////////////////////////////////////////////////////////////////

#undef WAIT_ARG_FUNC

void WAIT_ARG_FUNC ( ccp format, va_list arg )
{
    if ( format && *format )
	trace_helper(1,format,arg);
    PRINT_FUNC(">>>>>> PRESS RETURN: ");
    getchar();
}

///////////////////////////////////////////////////////////////////////////////

#undef WAIT_FUNC

void WAIT_FUNC ( ccp format, ... )
{
    va_list arg;
    va_start(arg,format);
    WAIT_ARG_FUNC(format,arg);
    va_end(arg);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			alloc/free system		///////////////
///////////////////////////////////////////////////////////////////////////////

#if TRACE_ALLOC_MODE > 2
    #define MEM_FILLER_SIZE 8
    static u8 mem_filler[MEM_FILLER_SIZE];
#else
    #define MEM_FILLER_SIZE 0
#endif

#if TRACE_ALLOC_MODE > 1
    #define MPARAM ccp func, ccp file, uint line,
    #define MCALL  func,file,line,
    #define PRINT_OOM(...) \
	PrintError(func,file,line,0,ERR_OUT_OF_MEMORY,__VA_ARGS__)
#else
    #define MPARAM
    #define MCALL
    #define PRINT_OOM(...) \
	PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,__VA_ARGS__)
#endif

///////////////////////////////////////////////////////////////////////////////

void dclib_free ( void * ptr )
{
    free(ptr);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void * dclib_xcalloc ( size_t nmemb, size_t size )
{
    void * res = calloc(nmemb,size);
    if (!res)
	PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,
		"Out of memory while calloc() %zu bytes (%zu*%zu=0x%zx)\n",
		nmemb*size, nmemb, size, nmemb*size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * dclib_xmalloc ( size_t size )
{
    void * res = malloc(size);
    if (!res)
	PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,
		"Out of memory while malloc() %zu bytes (0x%zx)\n",
		size, size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * dclib_xrealloc ( void * ptr, size_t size )
{
    void * res = realloc(ptr,size);
    if ( !res && size )
	PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,
		"Out of memory while realloc() %zu bytes (0x%zx)\n",
		size, size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * dclib_xstrdup  ( ccp src )
{
    char * res = strdup(src?src:"");
    if (!res)
    {
	const uint size = src ? strlen(src)+1 : 0;
	PrintError(__FUNCTION__,__FILE__,__LINE__,0,ERR_OUT_OF_MEMORY,
		"Out of memory while strdup() %u bytes (0x%x)\n",
		size, size );
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void * dclib_calloc ( MPARAM size_t nmemb, size_t size )
{
    void * res = calloc(nmemb,size);
    if (!res)
	PRINT_OOM("Out of memory while calloc() %zu bytes (%zu*%zu=0x%zx)\n",
		nmemb*size, nmemb, size, nmemb*size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * dclib_malloc ( MPARAM size_t size )
{
    void * res = malloc(size);
    if (!res)
	PRINT_OOM("Out of memory while malloc() %zu bytes (0x%zx)\n",
		size, size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * dclib_realloc ( MPARAM void * ptr, size_t size )
{
    void * res = realloc(ptr,size);
    if ( !res && size )
	PRINT_OOM("Out of memory while realloc() %zu bytes (0x%zx)\n",
		size, size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

char * dclib_strdup  ( MPARAM ccp src )
{
    char * res = strdup(src?src:"");
    if (!res)
    {
	const uint size = src ? strlen(src)+1 : 0;
	PRINT_OOM("Out of memory while strdup() %u bytes (0x%x)\n",
		size, size );
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * dclib_strdup2 ( MPARAM ccp src1, ccp src2 )
{
    const uint len1 = src1 ? strlen(src1) : 0;
    const uint len2 = src2 ? strlen(src2) : 0;
    char * res = dclib_malloc( MCALL len1+len2+1 );
    if (len1)
	memcpy(res,src1,len1);
    if (len2)
	memcpy(res+len1,src2,len2);
    res[len1+len2] = 0;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * dclib_strdup3 ( MPARAM ccp src1, ccp src2, ccp src3 )
{
    const uint len1 = src1 ? strlen(src1) : 0;
    const uint len2 = src2 ? strlen(src2) : 0;
    const uint len3 = src3 ? strlen(src3) : 0;
    char * res = dclib_malloc( MCALL len1+len2+len3+1 );
    char * dest = res;
    if (len1)
    {
	memcpy(dest,src1,len1);
	dest += len1;
    }
    if (len2)
    {
	memcpy(dest,src2,len2);
	dest += len2;
    }
    if (len3)
    {
	memcpy(dest,src3,len3);
	dest += len3;
    }
    *dest = 0;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * dclib_memdup ( MPARAM const void * src, size_t copylen )
{
    char * dest = dclib_malloc( MCALL copylen+1);
    memcpy(dest,src,copylen);
    dest[copylen] = 0;
    return dest;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if TRACE_ALLOC_MODE > 2
///////////////////////////////////////////////////////////////////////////////
// [[mem_info_t]]

typedef struct mem_info_t
{
    ccp		func;
    ccp		file;
    uint	line;
    u8		* data;
    uint	size;
    bool	filler;
    uint	seqnum;

} mem_info_t;

//-----------------------------------------------------------------------------

static mem_info_t * mem_list = 0;
static uint  mem_used	= 0;
static uint  mem_size	= 0;
static uint  mem_seqnum	= 0;

///////////////////////////////////////////////////////////////////////////////

static FILE * OpenMemLog()
{
    static bool done = false;
    if (!done)
    {
	done = true;
	char fname_buf[1000], *fname;
	if ( progname && *progname && *progname != '?' )
	{
	    snprintf(fname_buf,sizeof(fname_buf),"_trace_alloc-%s.tmp",progname);
	    fname = fname_buf;
	}
	else
	    fname = "_trace_alloc.tmp";
	MEM_LOG_FILE = fopen(fname,"wb");
	if (MEM_LOG_FILE)
	    fcntl(fileno(MEM_LOG_FILE),F_SETFD,FD_CLOEXEC);
    }
    return MEM_LOG_FILE;
}

///////////////////////////////////////////////////////////////////////////////

static void MemLog
    ( ccp func, ccp file, uint line, ccp info, const u8 * ptr, bool no_stderr )
{
    static uint line_no = 0;

    if (!no_stderr)
    {
	fprintf(stderr,"MEM-ERR[%s,%p] %s() @ %s#%u\n",
		info, ptr, func, file, line );
	fflush(stderr);
    }

    if (OpenMemLog())
    {
	if (!line_no)
	    fputs("#index addr type function line file\n",MEM_LOG_FILE);

	fprintf(MEM_LOG_FILE,"%07u %p %s %s %u %s\n",
		line_no, ptr, info, func, line, file );
	fflush(MEM_LOG_FILE);
    }

    line_no++;
}

///////////////////////////////////////////////////////////////////////////////

static void MemLogItem
	( ccp func, ccp file, uint line, ccp info, const mem_info_t * mi )
{
    if (func)
	fprintf(stderr,"MEM-ERR[%s] %s() @ %s#%u\n",
		info, func, file, line );
    fprintf(stderr,"MEM-ERR[%s,%u] %s() @ %s#%u\n",
		info, mi->seqnum, mi->func, mi->file, mi->line );
    fflush(stderr);

    if (OpenMemLog())
    {
	if (func)
	    fprintf(MEM_LOG_FILE,"[%s] %s() @ %s#%u\n",
		    info, func, file, line );
	fprintf(MEM_LOG_FILE,"[%s,%u] %s() @ %s#%u\n",
		    info, mi->seqnum, mi->func, mi->file, mi->line );
	if (mi->filler)
	{
	    HexDump16( MEM_LOG_FILE, 10, 0,
			mi->data - MEM_FILLER_SIZE, 2*MEM_FILLER_SIZE );
	    HexDump16( MEM_LOG_FILE, 10, mi->size + MEM_FILLER_SIZE,
			mi->data + mi->size - MEM_FILLER_SIZE, 2 * MEM_FILLER_SIZE );
	}
	fputc('\n',MEM_LOG_FILE);
	fflush(MEM_LOG_FILE);
    }
}

///////////////////////////////////////////////////////////////////////////////

uint FindMemInfoHelper ( u8 * data, uint size )
{
    int beg = 0;
    int end = mem_used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	mem_info_t * mi = mem_list + idx;
	if ( data < mi->data )
	    end = idx - 1 ;
	else if ( data > mi->data )
	    beg = idx + 1;
	else if ( size < mi->size )
	    end = idx - 1 ;
	else if ( size > mi->size )
	    beg = idx + 1;
	else
	{
	    noTRACE("FindMemInfoHelper(%llx,%llx) FOUND=%d/%d/%d\n",
		    (u64)off, (u64)size, idx, mem_used, mem_size );
	    return idx;
	}
    }

    noTRACE("FindStringFieldHelper(%llx,%llx) failed=%d/%d/%d\n",
		(u64)off, (u64)size, beg, mem_used, mem_size );
    return beg;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeTraceAlloc()
{
    static bool done = false;
    if (done)
	return;
    done = true;

    OpenMemLog();

    memset(mem_filler,0xdc,sizeof(mem_filler));
    mem_filler[MEM_FILLER_SIZE/2] = 0xcd;
    mem_filler[MEM_FILLER_SIZE/3] = 0xcd;
}

///////////////////////////////////////////////////////////////////////////////

int CheckTraceAlloc ( ccp func, ccp file, uint line )
{
    int count = 0;

    mem_info_t *ptr, *end = mem_list + mem_used;
    for ( ptr = mem_list; ptr < end; ptr++ )
    {
	if (!ptr->filler)
	    continue;

	if (memcmp(ptr->data-MEM_FILLER_SIZE,mem_filler,sizeof(mem_filler)))
	{
	    MemLogItem(0,0,0,"BEG",ptr);
	    memcpy(ptr->data-MEM_FILLER_SIZE,mem_filler,sizeof(mem_filler));
	    count++;
	}

	if (memcmp(ptr->data+ptr->size,mem_filler,sizeof(mem_filler)))
	{
	    MemLogItem(0,0,0,"END",ptr);
	    memcpy(ptr->data+ptr->size,mem_filler,sizeof(mem_filler));
	    count++;
	}
    }

    if (count)
    {
	fprintf(stderr,"MEM-ERR: %u errors found -> %s() @ %s#%u\n",
		count, func, file, line );
	fflush(stderr);

	if (OpenMemLog())
	{
	    fprintf(MEM_LOG_FILE,"--- %u errors found -> %s() @ %s#%u\n\n",
		count, func, file, line );
	    fflush(MEM_LOG_FILE);
	}
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

void DumpTraceAlloc ( ccp func, ccp file, uint line, FILE * f )
{
    CheckTraceAlloc(func,file,line);

    if (!f)
	return;

    fprintf(f,"---- MEM-DUMP [N=%u] -----\n",mem_used);
    mem_info_t *ptr, *end = mem_list + mem_used;
    for ( ptr = mem_list; ptr < end; ptr++ )
    {
	ccp file = strrchr(ptr->file,'/');
	file = file ? file+1 : ptr->file;
	fprintf(f,"%5u %p %5u %-20.20s %5u %-.25s\n",
		ptr->seqnum, ptr->data, ptr->size,
		ptr->func, ptr->line, file );
    }
    fflush(stderr);
}

///////////////////////////////////////////////////////////////////////////////

mem_info_t * RegisterAlloc
	( ccp func, ccp file, uint line, cvp data, uint size, bool filler )
{
    if (!mem_seqnum)
	InitializeTraceAlloc();

    if (filler)
    {
	memcpy((u8*)data-MEM_FILLER_SIZE,mem_filler,MEM_FILLER_SIZE);
	memcpy((u8*)data+size,mem_filler,MEM_FILLER_SIZE);
    }

    uint idx = FindMemInfoHelper((u8*)data,size);

    ASSERT( mem_used <= mem_size );
    if ( mem_used == mem_size )
    {
	mem_size += 1000 + mem_size/2;
	const uint alloc_size = mem_size * sizeof(*mem_list);
	mem_list = realloc(mem_list,alloc_size);
	if (!mem_list)
	    PRINT_OOM("Out of memory while RegisterAlloc() %u bytes (0x%x)\n",
		alloc_size, alloc_size );
    }

    DASSERT( idx <= mem_used );
    mem_info_t * mi = mem_list + idx;
    memmove(mi+1,mi,(mem_used-idx)*sizeof(*mi));
    mem_used++;

    mi->func	= func;
    mi->file	= file;
    mi->line	= line;
    mi->data	= (u8*)data;
    mi->size	= size;
    mi->filler	= filler;
    mi->seqnum	= mem_seqnum++;

 #if TRACE_ALLOC_MODE > 3
    MemLog(func,file,line,"ALLOC",data,true);
 #endif
    return mi;
}

//
///////////////////////////////////////////////////////////////////////////////
#endif // TRACE_ALLOC_MODE > 2
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint trace_test_alloc
    ( ccp func, ccp file, uint line, const void *p_data, bool hexdump )
{
 #if TRACE_ALLOC_MODE > 2
    if (!p_data)
	return 0;
    const u8 *data = p_data;

    int beg = 0;
    int end = mem_used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	mem_info_t * mi = mem_list + idx;
	if ( data < mi->data )
	    end = idx - 1 ;
	else if ( data > mi->data )
	    beg = idx + 1;
	else
	{
	 #if TRACE_ALLOC_MODE > 3
	    MemLog(func,file,line,"TEST",data,true);
	 #endif

	    uint stat = 0;
	    if (memcmp(data-MEM_FILLER_SIZE,mem_filler,MEM_FILLER_SIZE))
	    {
		MemLogItem(func,file,line,"BEG",mi);
		stat = 1;
		HexDump16(stderr,0,0,data-MEM_FILLER_SIZE,16);
	    }

	    if (memcmp(data+mi->size,mem_filler,MEM_FILLER_SIZE))
	    {
		MemLogItem(func,file,line,"END",mi);
		stat |= 2;
		HexDump16(stderr,0,mi->size+sizeof(mem_filler),data+mi->size,16);
	    }

	    return stat;
	}
    }
    MemLog(func,file,line,"NOT FOUND",data,false);
    return 4;
 #else
    return 0;
 #endif
}

///////////////////////////////////////////////////////////////////////////////

static void * UnregisterAlloc ( ccp func, ccp file, uint line, u8 * data )
{
 #if TRACE_ALLOC_MODE > 2
    if (!data)
	return 0;

    int beg = 0;
    int end = mem_used - 1;
    while ( beg <= end )
    {
	uint idx = (beg+end)/2;
	mem_info_t * mi = mem_list + idx;
	if ( data < mi->data )
	    end = idx - 1 ;
	else if ( data > mi->data )
	    beg = idx + 1;
	else
	{
	 #if TRACE_ALLOC_MODE > 3
	    MemLog(func,file,line,"FREE",data,true);
	 #endif

	    if (mi->filler)
	    {
		if (memcmp(data-MEM_FILLER_SIZE,mem_filler,MEM_FILLER_SIZE))
		    MemLogItem(func,file,line,"BEG",mi);

		if (memcmp(data+mi->size,mem_filler,MEM_FILLER_SIZE))
		    MemLogItem(func,file,line,"END",mi);

		data -= MEM_FILLER_SIZE;
	    }

	    memmove(mi,mi+1,(--mem_used-idx)*sizeof(*mi));
	    return data;
	}
    }
    MemLog(func,file,line,"NOT FOUND",data,false);
 #endif
    return data;
}

///////////////////////////////////////////////////////////////////////////////

void trace_free ( ccp func, ccp file, uint line, void * ptr )
{
    free(UnregisterAlloc(func,file,line,ptr));
}

///////////////////////////////////////////////////////////////////////////////

void * trace_malloc  ( ccp func, ccp file, uint line, size_t size )
{
    u8 * res = malloc( size + 2 * MEM_FILLER_SIZE );
    if (!res)
	PRINT_OOM("Out of memory while allocate %zu+%u bytes (0x%zx)\n",
		size, 2 * MEM_FILLER_SIZE, size + 2 * MEM_FILLER_SIZE );

    res += MEM_FILLER_SIZE;
 #if TRACE_ALLOC_MODE > 2
    RegisterAlloc(func,file,line,res,size,true);
 #endif
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * trace_calloc  ( ccp func, ccp file, uint line, size_t nmemb, size_t size )
{
    uint total_size = nmemb * size;
    void * res = trace_malloc(func,file,line,total_size);
    memset(res,0,total_size);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * trace_realloc ( ccp func, ccp file, uint line, void *ptr, size_t size )
{
    ptr = UnregisterAlloc(func,file,line,ptr);
    u8 * res = realloc( ptr, size + 2 * MEM_FILLER_SIZE );
    if (!res)
	PRINT_OOM("Out of memory while re allocate %zu+%u bytes (0x%zx)\n",
		size, 2 * MEM_FILLER_SIZE, size + 2 * MEM_FILLER_SIZE );

    res += MEM_FILLER_SIZE;
 #if TRACE_ALLOC_MODE > 2
    RegisterAlloc(func,file,line,res,size,true);
 #endif
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * trace_strdup ( ccp func, ccp file, uint line, ccp src )
{
    const uint len = src ? strlen(src)+1 : 0;
    char * res = trace_malloc(func,file,line,len);
    memcpy(res,src,len);
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * trace_strdup2 ( ccp func, ccp file, uint line, ccp src1, ccp src2 )
{
    const uint len1 = src1 ? strlen(src1) : 0;
    const uint len2 = src2 ? strlen(src2) : 0;
    char * res = trace_malloc(func,file,line,len1+len2+1);
    if (len1)
	memcpy(res,src1,len1);
    if (len2)
	memcpy(res+len1,src2,len2);
    res[len1+len2] = 0;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

char * trace_strdup3 ( ccp func, ccp file, uint line, ccp src1, ccp src2, ccp src3 )
{
    const uint len1 = src1 ? strlen(src1) : 0;
    const uint len2 = src2 ? strlen(src2) : 0;
    const uint len3 = src3 ? strlen(src3) : 0;
    char * res = trace_malloc(func,file,line,len1+len2+len3+1);
    char * dest = res;
    if (len1)
    {
	memcpy(dest,src1,len1);
	dest += len1;
    }
    if (len2)
    {
	memcpy(dest,src2,len2);
	dest += len2;
    }
    if (len3)
    {
	memcpy(dest,src3,len3);
	dest += len3;
    }
    *dest = 0;
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void * trace_memdup ( ccp func, ccp file, uint line, const void * src, size_t copylen )
{
    char * dest = trace_malloc(func,file,line,copylen+1);
    memcpy(dest,src,copylen);
    dest[copylen] = 0;
    return dest;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 mem check			///////////////
///////////////////////////////////////////////////////////////////////////////

static u8	    mem_check_buf[0x100];
static const void * mem_check_ptr = 0;
static uint	    mem_check_size = 0;

///////////////////////////////////////////////////////////////////////////////

void MemCheckSetup ( const void * ptr, uint size )
{
    if ( ptr && size > 0 )
    {
	mem_check_ptr = ptr;
	if ( size > sizeof(mem_check_buf) )
	{
	    ERROR0(ERR_WARNING,
		"MemCheckSetup(), max watch size = %zx (<%x)",
		sizeof(mem_check_buf), size );
	    size = sizeof(mem_check_buf);
	}
	mem_check_size = size;
	memcpy(mem_check_buf,ptr,size);
    }
    else
	mem_check_size = 0;
}

///////////////////////////////////////////////////////////////////////////////

void MemCheck ( ccp func, ccp file, uint line )
{
    if (!mem_check_size)
	return;

    if ( memcmp(mem_check_buf,mem_check_ptr,mem_check_size) )
    {
	fprintf(stderr,"--- MemCheck: %p should be:\n",mem_check_ptr);
	HexDump16(stderr,0,0,mem_check_buf,mem_check_size);
	fprintf(stderr,"--- MemCheck: ... but is:\n");
	HexDump16(stderr,0,0,mem_check_ptr,mem_check_size);

	TRACE("--- MemCheck: %p should be:\n",mem_check_ptr);
	TRACE_HEXDUMP16(0,0,mem_check_buf,mem_check_size);
	TRACE("--- MemCheck: ... but is:\n");
	TRACE_HEXDUMP16(0,0,mem_check_ptr,mem_check_size);

	PrintError(func,file,line,0,ERR_FATAL,"MemCheck() failed!\n");
    }
    else
	TRACE("MemCheck OK: %s @ %s#%u\n",func,file,line);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////
