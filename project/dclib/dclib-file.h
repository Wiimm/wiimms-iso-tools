
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

#ifndef DCLIB_FILE_H
#define DCLIB_FILE_H 1

#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>

#include "dclib-types.h"
#include "dclib-basics.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum FileMode_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[FileMode_t]]

typedef enum FileMode_t
{
    FM_TEST		= 0x00001,  // test mode, don't modify any file
    FM_SILENT		= 0x00002,  // suppress error messages
    FM_IGNORE		= 0x00004,  // ignore, if no file on OpenFile()
				    //   and return ERR_NOT_EXISTS

    FM_MODIFY		= 0x00010,  // open file for reading and writing
    FM_APPEND		= 0x00020,  // append to existing files
    FM_UPDATE		= 0x00040,  // update existing file
    FM_OVERWRITE	= 0x00080,  // overwrite existing files silently
    FM_NUMBER		= 0x00100,  // renumber filename for existing files
    FM_REMOVE		= 0x00200,  // remove file before open
    FM_MKDIR		= 0x00400,  // create path automatically
    FM_TCP		= 0x00800,  // detect 'tcp:' and 'unix:' prefix on open

    FM_STDIO		= 0x01000,  // detect and allow "-" as stdin or stdout
    FM_DEV		= 0x02000,  // allow character and block devices
    FM_SOCK		= 0x04000,  // allow sockets and connect as unix stream
    FM_SPC		= 0x08000,  // allow special files like pipes and sockets

    FM_TOUCH		= 0x40000,  // touch file (set timestamp) on close
    FM_TEMP		= 0x80000,  // temporary file: remove file on close

    FM_M_OPEN		= FM_TEST
			| FM_SILENT
			| FM_IGNORE
			| FM_MODIFY
			| FM_STDIO
			| FM_DEV
			| FM_SPC
			| FM_TCP
			| FM_TEMP,

    FM_M_CREATE		= FM_TEST
			| FM_SILENT
			| FM_MODIFY
			| FM_APPEND
			| FM_UPDATE
			| FM_OVERWRITE
			| FM_NUMBER
			| FM_REMOVE
			| FM_MKDIR
			| FM_STDIO
			| FM_DEV
			| FM_SOCK
			| FM_SPC
			| FM_TOUCH
			| FM_TEMP,

    FM_M_ALL		= FM_M_OPEN
			| FM_M_CREATE
}
FileMode_t;

//-----------------------------------------------------------------------------

ccp GetFileModeStatus
(
    char		*buf,		// result buffer
					// NULL: use a local circulary static buffer
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    FileMode_t		file_mode,	// filemode to print
    uint		print_mode	// 0: short vector, 1 char for 1 attrib
					// 1: long mode: keyword for each set attrib
);


ccp GetFileOpenMode
(
    bool		create,		// false: open, true: create
    FileMode_t		file_mode	// open modes
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct FileAttrib_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[FileAttrib_t]]

typedef struct FileAttrib_t
{
	time_t mtime;	// time of last content modification
	time_t ctime;	// time of file creation or last status change
	time_t atime;	// last ascces time
	time_t itime;	// insertion time (special time for archives)

	size_t size;	// file size
	mode_t mode;	// file mode
}
FileAttrib_t;

///////////////////////////////////////////////////////////////////////////////

FileAttrib_t * ClearFileAttrib
(
    FileAttrib_t	* dest		// NULL or destination attribute
);

FileAttrib_t * TouchFileAttrib
(
    FileAttrib_t	* dest		// valid destination attribute
);

FileAttrib_t * SetFileAttrib
(
    FileAttrib_t	* dest,		// valid destination attribute
    const FileAttrib_t	* src_fa,	// NULL or source attribute
    const struct stat	* src_stat	// NULL or source attribute
					// only used if 'src_fa==NULL'
);

FileAttrib_t * MaxFileAttrib
(
    FileAttrib_t	* dest,		// valid source and destination attribute
    const FileAttrib_t	* src_fa,	// NULL or second source attribute
    const struct stat	* src_stat	// NULL or third source attribute
);

static inline FileAttrib_t * UseFileAttrib
(
    FileAttrib_t	* dest,		// valid destination attribute
    const FileAttrib_t	* src_fa,	// NULL or source attribute
    const struct stat	* src_stat,	// NULL or source attribute
    bool		use_max		// switch for SetFileAttrib() | MaxFileAttrib()
)
{
    return ( use_max ? MaxFileAttrib : SetFileAttrib )(dest,src_fa,src_stat);
}

FileAttrib_t * NormalizeFileAttrib
(
    FileAttrib_t	* fa		// valid attribute
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct File_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[File_t]]

typedef struct File_t
{
    //--- file handle

    FILE		*f;		// file descriptor

    //--- basic status

    ccp			fname;		// file name, alloced
    FileMode_t		fmode;		// file open mode
    struct stat		st;		// file status just before opened
    FileAttrib_t	fatt;		// file attributes

    //--- advanced status

    bool		is_stdio;	// true: read from stdin or to stdout
    bool		is_socket;	// true: file is a UNIX stream socket
    bool		is_reading;	// true: file opened for reading
    bool		is_writing;	// true: file opened for writing
    bool		is_seekable;	// true: file is seekable
    enumError		max_err;	// maximum file related error

    //--- file data

    u8			* data;		// NULL or file data
    size_t		size;		// size of 'data'
    bool		data_alloced;	// true: 'data' is alloced,
					//       free it on ResetFile()
}
File_t;

///////////////////////////////////////////////////////////////////////////////

void InitializeFile
(
   File_t		* f		// file structure
);

enumError ResetFile
(
   File_t		* f,		// file structure
   uint			set_time	// 0: don't set
					// 1: set time before closing using 'fatt'
					// 2: set current time before closing
);

enumError CloseFile
(
   File_t		* f,		// file structure
   uint			set_time	// 0: don't set
					// 1: set time before closing using 'fatt'
					// 2: set current time before closing
);

//-----------------------------------------------------------------------------

enumError OpenFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			fname,		// file to open
    FileMode_t		file_mode,	// open modes
    off_t		limit,		// >0: don't open, if file size > limit
    ccp			limit_message	// NULL or a with LF terminated text.
					//   It is printed after the message.
);

//-----------------------------------------------------------------------------

enumError CheckCreateFile
(
    // returns:
    //   ERR_DIFFER:		[FM_STDIO] source is "-" => 'st' is zeroed
    //	 ERR_WARNING:		[FM_DEV,FM_SPC] not a regular file
    //   ERR_WRONG_FILE_TYPE:	file exists, but is not a regular file
    //   ERR_ALREADY_EXISTS:	file already exists
    //   ERR_INVALID_VERSION:	already exists, but FM_NUMBER set => no msg printed
    //   ERR_CANT_CREATE:	FM_UPDATE is set, but file don't exist
    //   ERR_OK:		file not exist or can be overwritten

    ccp			fname,		// filename to open
    FileMode_t		file_mode,	// open modes
    struct stat		*st		// not NULL: store file status here
);

enumError CreateFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			fname,		// file to open
    FileMode_t		file_mode	// open modes
);

static inline enumError AppendFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			fname,		// file to open
    FileMode_t		file_mode	// open modes
)
{
    return CreateFile(f,initialize,fname,file_mode|FM_APPEND);
}

//-----------------------------------------------------------------------------

enumError WriteFileAt
(
    File_t		* F,		// file to write
    size_t		* cur_offset,	// pointer to current file offset, modified
    size_t		offset,		// offset to write
    const void		* data,		// data to write
    size_t		size		// size of 'data'
);

//-----------------------------------------------------------------------------

enumError SetFileSize
(
    File_t		* F,		// file to write
    size_t		* cur_offset,	// pointer to current file offset, modified
    size_t		size		// offset to write
);

//-----------------------------------------------------------------------------

enumError SkipFile
(
    File_t		*F,		// file to write
    size_t		skip		// number of bytes to skip
);

//-----------------------------------------------------------------------------

enumError RegisterFileError
(
    File_t		* f,		// file structure
    enumError		new_error	// new error code
);

///////////////////////////////////////////////////////////////////////////////

s64 GetFileSize
(
    ccp			path1,		// NULL or part 1 of path
    ccp			path2,		// NULL or part 2 of path
    s64			not_found_val,	// return value if no regular file found
    FileAttrib_t	* fatt,		// not NULL: store file attributes
    bool		fatt_max	// true: store max values to 'fatt'
);

//-----------------------------------------------------------------------------

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
);

//-----------------------------------------------------------------------------

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
);

//-----------------------------------------------------------------------------

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
);

//-----------------------------------------------------------------------------

enumError SaveFile
(
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    FileMode_t		file_mode,	// open modes
    const void		* data,		// data to write
    uint		data_size,	// size of 'data'
    FileAttrib_t	* fatt		// not NULL: set timestamps using this attrib
);

//-----------------------------------------------------------------------------

enumError OpenWriteFile
(
    File_t		* f,		// file structure
    bool		initialize,	// true: initialize 'f'
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    FileMode_t		file_mode,	// open modes
    const void		* data,		// data to write
    uint		data_size	// size of 'data'
);

//-----------------------------------------------------------------------------

uint CloseAllExcept
(
    int		*list,		// exception list of unordered FD, max contain -1
    uint	n_list,		// number of elements in 'list'
    int		min,		// close all until and including this fd
    int		add		// if a FD was found, incremet 'min' to 'fd+add'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct MemFile_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[MemFile_t]]

typedef struct MemFile_t
{
    u8		*data;		// pointer to alloced data
    uint	size;		// size of 'data'
    uint	max_size;	// >0: max allowed size of 'data'
    uint	fend;		// current end of file position, always '<=size'
    uint	fpos;		// current file pointer, always '<=fend'

    ccp		err_name;	// not NULL: print error messages
    bool	err_name_alloced; // true: 'err_name' must be freed
    bool	zero_extend;	// true: assume unlimted file on Read*()
    bool	eof;		// set by Read*(), if read behind 'fend'

    FileAttrib_t fatt;		// file attribute for loaded files
}
MemFile_t;

///////////////////////////////////////////////////////////////////////////////

bool IsValidMemFile ( MemFile_t *mf );

void DumpMemFile
(
    FILE	*f,		// valid output stream
    int		indent,		// indent of output
    MemFile_t	*mf		// valid MemFile_t data
);

bool AssertMemFile
(
    MemFile_t	*mf		// valid MemFile_t data
);

///////////////////////////////////////////////////////////////////////////////

u8 * GetDataMemFile
(
    // return NULL on err or a pointer to the data posisiton.

    MemFile_t	*mf,		// valid MemFile_t data
    uint	fpos,		// fpos to read from
    uint	size		// needed data size
);

void InitializeMemFile
(
    MemFile_t	*mf,		// structure to initialize
    uint	start_size,	// >0: alloc this for data
    uint	max_size	// >0: file never grows above 'max_size'
);

void ResetMemFile
(
    MemFile_t	*mf		// valid MemFile_t data
);

///////////////////////////////////////////////////////////////////////////////

enumError WriteMemFileAt
(
    MemFile_t	*mf,		// valid MemFile_t data
    uint	fpos,		// fpos to write
    const void	*data,		// data to write
    uint	size		// size to write
);

static inline enumError WriteMemFile
(
    MemFile_t	*mf,		// valid MemFile_t data
    const void	*data,		// data to write
    uint	size		// size to write
)
{
    DASSERT(mf);
    return WriteMemFileAt(mf,mf->fpos,data,size);
}

///////////////////////////////////////////////////////////////////////////////

uint ReadMemFileAt
(
    MemFile_t	*mf,		// valid MemFile_t data
    uint	fpos,		// fpos to read from
    void	*buf,		// destination buffer
    uint	size		// size to read
);

static inline uint ReadMemFile
(
    MemFile_t	*mf,		// valid MemFile_t data
    void	*buf,		// destination buffer
    uint	size		// size to read
)
{
    DASSERT(mf);
    return ReadMemFileAt(mf,mf->fpos,buf,size);
}

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
);

enumError SaveMemFile
(
    MemFile_t	*mf,		// MemFile_t data

    ccp		path1,		// NULL or part #1 of path
    ccp		path2,		// NULL or part #2 of path
    FileMode_t	fmode,		// mode for file creation
    bool	sparse		// true: enable sparse support
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct LineBuffer_t		///////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __APPLE__
///////////////////////////////////////////////////////////////////////////////
// [[LineBuffer_t]]

typedef struct LineBuffer_t
{
    FILE		*fp;		// current file pointer
    FILE		*old_fp;	// NULL or file before opened
    FILE		**old_fp_pos;	// not NULL: store 'old_fp' on close()
    int			redirect;	// >0: redirect to 'old_fp' (waste if old_fp==0)

    GrowBuffer_t	buf;		// grow buffer to hold lines
    u8			*prev_buf_ptr;	// previous 'buf.ptr', if changed recalc 'line'

    uint		max_lines;	// max numbers of lines
    uint		used_lines;	// numbers of used lines
    uint		max_line_size;	// >0: max size of a single line
    mem_t		*line;		// list with 'max_lines' elements
    bool		open_line;	// true: last line not terminated yet
    uint		seq_count;	// increased by 1 on every change
    uint		prev_seq_count;	// previous 'seq_count', if changed recalc 'line'
    uint		line_count;	// increased for each insertet LF, no internal impact
}
LineBuffer_t;

//-----------------------------------------------------------------------------

void InitializeLineBuffer ( LineBuffer_t *lb, uint max_buf_size );
void ResetLineBuffer ( LineBuffer_t *lb );
void ClearLineBuffer ( LineBuffer_t *lb );

void RedirectLineBuffer
(
    LineBuffer_t	*lb,		// valid line buffer
    FILE		*f,		// output file, if NULL use 'lb->old_fd'
    int			max_lines	// >=0: max number of redirected lines
);

LineBuffer_t * OpenLineBuffer
(
    LineBuffer_t	*lb,		// line buffer; if NULL, malloc() one
    bool		init_lb,	// true: initialize 'lb'
    FILE		**fp_pos,	// NULL or external place of the file pointer
    uint		max_lines,	// max numbers of lines
    uint		max_line_size,	// >0: max size of a single line
    uint		max_buf_size	// >0: max total buffer size
);

int CloseLineBuffer ( LineBuffer_t *lb );
ssize_t WriteLineBuffer ( LineBuffer_t *lb, ccp buf, size_t size );

void FixMemListLineBuffer ( LineBuffer_t *lb, bool force );
uint GetMemListLineBuffer
(
    // returns the number of elements

    LineBuffer_t	*lb,		// line buffer; if NULL, malloc() one
    mem_t		**res,		// not NULL: store pointer to first element here
    int			max_lines	// !0: limit lines, <0: count from end
);

uint PrintLineBuffer
(
    // returns number of written lines

    FILE		*f,		// output stream
    int			indent,		// indention
    LineBuffer_t	*lb,		// line buffer; if NULL, malloc() one
    int			max_lines,	// !0: limit lines, <0: count from end
    ccp			open_line	// not NULL: append this text for an open line
);

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
);

int ExistDirectory
(
    // check only real file (stat(fname))

    ccp			fname,		// NULL or path
    int			answer_if_empty	// answer, if !fname || !*fname
);

///////////////////////////////////////////////////////////////////////////////

typedef enum FindConfigFile_t
{
	FCF_HOME	= 0x001, //  ~/.<SHARE>/<FILENAME>
	FCF_REL_SHARE	= 0x002, //  <PROGPATH>/../share/<SHARE>/<FILENAME>
	FCF_LOC_SHARE	= 0x004, //  /usr/local/share/<SHARE>/<FILENAME>
	FCF_USR_SHARE	= 0x008, //  /usr/share/<SHARE>/<FILENAME>
	FCF_PROG_SHARE	= 0x010, //  <PROGPATH>/share/<PROGNAME><EXT>
	FCF_PROG	= 0x020, //  <PROGPATH>/<PROGNAME><EXT>

	FCF_STD_SHARE	= FCF_REL_SHARE
			| FCF_LOC_SHARE
			| FCF_USR_SHARE,

	FCF_STD_PROG	= FCF_PROG_SHARE
			| FCF_PROG,

	FCF_ALL		= 0x03f,

	FCF_F_DEBUG	= 0x100, // enable logging

}
FindConfigFile_t;

//-----------------------------------------------------------------------------

// Managed and read by user, ignored by FindConfigFile()
// Intention: if set globally, call FindConfigFile() with FCF_F_DEBUG

extern bool enable_config_search_log;

//-----------------------------------------------------------------------------

ccp FindConfigFile
(
    // returns NULL or found file (alloced)

    FindConfigFile_t mode,	// bit field: select search destinations
    ccp		share_name,	// NULL or <SHARE>
    ccp		file_name,	// NULL or <FILENAME>
    ccp		prog_ext	// <EXT>, if NULL use '.' + <FILENAME>
);

//-----------------------------------------------------------------------------

struct dirent;
typedef int (*SearchFilesFunc) ( ccp basepath, struct dirent *dent, void * param );

int SearchFiles
(
    ccp			path1,	// not NULL: part #1 of base path
    ccp			path2,	// not NULL: part #2 of base path
    ccp			match,	// not NULL: filter files by MatchPattern()
				// if empty: extract from combined path
    SearchFilesFunc	func,	// callback function, never NULL
    void		*param	// third parameter of func()
);

///////////////////////////////////////////////////////////////////////////////

enumError CreatePath
(
    ccp			path,		// path to create
    bool		is_pure_dir	// true: 'path' don't contains a filename part
);

enumError RemoveSource
(
    ccp			fname,		// file to remove
    ccp			dest_fname,	// NULL or dest file name
					//   If real pathes are same: don't remove
    bool		print_log,	// true: print a log message
    bool		testmode	// true: don't remove, log only
);

///////////////////////////////////////////////////////////////////////////////

char * FindFilename
(
    // Find the last part of the path, trailing '/' are ignored
    // return poiner to found string

    ccp		path,			// path to analyze, valid string
    uint	* result_len		// not NULL: return length of found string
);

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
);

///////////////////////////////////////////////////////////////////////////////
#if 0
///////////////////////////////////////////////////////////////////////////////

char * SplitSubPath
(
    char		* buf,		// destination buffer
    size_t		buf_size,	// size of 'buf'
    ccp			path		// source path, if NULL: use 'buf'
);

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
);

enumError SaveFile
(
    ccp			path1,		// NULL or part #1 of path
    ccp			path2,		// NULL or part #2 of path
    bool		overwrite,	// true: force overwriting
    const void		* data,		// data to write
    uint		data_size,	// size of 'data'
    FileAttrib_t	* fatt		// not NULL: set timestamps using this attribs
);

///////////////////////////////////////////////////////////////////////////////
#endif
///////////////////////////////////////////////////////////////////////////////

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
);

///////////////////////////////////////////////////////////////////////////////
#ifdef __CYGWIN__

 uint IsWindowsDriveSpec
 (
    // returns the length of the found windows drive specification (0|2|3)

    ccp			src		// valid string
 );

 uint NormalizeFilenameCygwin
 (
    // returns a pointer to the NULL terminator within 'buf'

    char		* buf,		// valid destination buffer
    uint		buf_size,	// size of buf
    ccp			src		// NULL or source
 );

 char * AllocNormalizedFilenameCygwin
 (
    // returns an alloced buffer with the normalized filename

    ccp source				// NULL or string
 );

#endif // __CYGWIN__

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    FDList_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[FDList_t]]

struct pollfd;

typedef struct FDList_t
{
    bool	use_poll;	// false: use select(), true: use poll()

    u64		now_usec;	// set on Clear() and Wait(), result of GetTimeUSec(false)
    u64		timeout_usec;	// next timeout, based on GetTimeUSec(false)

    //--- select() params

    int		max_fd;		// highest-numbered file descriptor, add 1 on select()
    fd_set	readfds;	// ready to read
    fd_set	writefds;	// ready to write
    fd_set	exceptfds;	// ready to except

    //--- poll() params

    struct pollfd *poll_list;	// list of poll parameters
    uint	poll_used;	// poll_list: number of used elements
    uint	poll_size;	// poll_list: number of alloced elements

    //--- statistics

    uint	n_sock;		// current number of registered sockets
				// equals poll_used if poll is used
    uint	wait_count;	// total number of waits
    u64		wait_usec;	// total wait time in usec
    u64		cur_usec;	// current wait time in usec
}
FDList_t;

///////////////////////////////////////////////////////////////////////////////

void ClearFDList ( FDList_t *fdl );
void InitializeFDList ( FDList_t *fdl, bool use_poll );
void ResetFDList ( FDList_t *fdl );

//-----------------------------------------------------------------------------

// announce new sockets
void AnnounceFDList ( FDList_t *fdl, uint n );
struct pollfd * AllocFDList ( FDList_t *fdl, uint n );

uint AddFDList
(
    // returns the pool-index if available, ~0 otherwise

    FDList_t	*fdl,		// valid socket list
    int		sock,		// socket to add
    uint	events		// bit field: POLLIN|POLLOUT|POLLERR
);

uint GetEventFDList
(
    // returns bit field: POLLIN|POLLOUT|POLLERR

    FDList_t	*fdl,		// valid socket list
    int		sock,		// socket to look for
    uint	poll_index	// if use_poll: use the index for a fast search
);

//-----------------------------------------------------------------------------

// use select() or poll()
int WaitFDList ( FDList_t *fdl );

// use pselect() or ppoll()
int PWaitFDList ( FDList_t *fdl, const sigset_t *sigmask );

// return ptr to file path, if begins with 1 of: file: unix: / ./ ../
ccp CheckUnixSocketPath
(
    ccp		src,	    // NULL or source path to analyse
    int		tolerance   // <1: 'unix:', 'file:', '/', './' and '../' detected
			    //  1: not 'NAME:' && at relast one '/'
			    //  2: not 'NAME:'
);

// return mem_t to file path, if begins with 1 of: file: unix: / ./ ../
mem_t CheckUnixSocketPathMem
(
    mem_t	src,	    // NULL or source path to analyse
    int		tolerance   // <1: 'unix:', 'file:', '/', './' and '../' detected
			    //  1: not 'NAME:' && at relast one '/'
			    //  2: not 'NAME:'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Catch Output			///////////////
///////////////////////////////////////////////////////////////////////////////

struct CatchOutput_t;

typedef int (*CatchOutputFunc)
(
    struct CatchOutput_t *ctrl,		// control struct incl. data
    int			call_mode	// 0:init, 1:new data, 2:term
);

///////////////////////////////////////////////////////////////////////////////

typedef struct CatchOutput_t
{
    int			mode;		// -1: disabled, 1:stdout, 2:stderr
    int			pipe_fd[2];	// pipe
    CatchOutputFunc	func;		// function
    GrowBuffer_t	buf;		// output buffer
    void		*user_ptr;	// pointer by user
}
CatchOutput_t;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int CatchIgnoreOutput
(
    struct CatchOutput_t *ctrl,		// control struct incl. data
    int			call_mode	// 0:init, 1:new data, 2:term
);

//-----------------------------------------------------------------------------

void ResetCatchOutput ( CatchOutput_t *co, uint n );

//-----------------------------------------------------------------------------

enumError CatchOutput
(
    ccp			command,	// command to execute
    int			argc,		// num(arguments) in 'argv'; -1: argv is NULL terminated
    char *const*	argv,		// list of arguments

    CatchOutputFunc	stdout_func,	// NULL or function to catch stdout
    CatchOutputFunc	stderr_func,	// NULL or function to catch stderr
    void		*user_ptr,	// NULL or user defined parameter
    bool		silent		// true: suppress error messages
);

//-----------------------------------------------------------------------------

enumError CatchOutputLine
(
    ccp			command_line,	// command line to execute
    CatchOutputFunc	stdout_func,	// NULL or function to catch stdout
    CatchOutputFunc	stderr_func,	// NULL or function to catch stderr
    void		*user_ptr,	// NULL or user defined parameter
    bool		silent		// true: suppress error messages
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan sections			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct SectionInfo_t
{
    FILE		*f;		// source file
    ccp			section;	// section name, alloced
    ccp			path;		// path name, NULL or alloced
    int			index;		// index of section, -1 if not available
    ParamField_t	param;		// parameter list
    void		*user_param;	// user defined parameter
}
SectionInfo_t;

typedef int (*SectionFunc) ( SectionInfo_t *si );

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
);

int ScanSections
(
    // return the last returned value by On*()

    FILE	*f,		// source file
    SectionFunc	OnSection,	// not NULL: call this function for each section
				// on result: <0: abort, 0:next section, 1:scan param
    SectionFunc	OnParams,	// not NULL: call this function after param scan
				// on result: <0: abort, continue
    void	*user_param	// user defined parameter

);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan socket type		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[socket_info_t]]

#define AF_INET_ANY (AF_INET6+100)

typedef struct socket_info_t
{
    bool is_valid;	// true: structure is valid

    int  sock;		// -1 or used socket
    int  protocol;	// -1 | AF_UNIX | AF_INET | AF_INET6 | AF_INET_ANY
    int	 type;		// -1 | SOCK_STREAM | SOCK_DGRAM | SOCK_RAW | SOCK_SEQPACKET
    ccp  address;	// NULL or pointer to relevant address or path
    bool alloced;	// true: address is alloced
}
socket_info_t;

///////////////////////////////////////////////////////////////////////////////

static inline void InitializeSocketInfo ( socket_info_t *si )
{
    DASSERT(si);
    memset(si,0,sizeof(*si));
    si->sock = si->protocol = si->type = -1;
}

///////////////////////////////////////////////////////////////////////////////

static inline void ResetSocketInfo ( socket_info_t *si )
{
    if (si)
    {
	if ( si->alloced )
	    FreeString(si->address);
	InitializeSocketInfo(si);
    }
}

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
);

///////////////////////////////////////////////////////////////////////////////

bool GetSocketInfoBySocket
(
    // returns TRUE, if infos retrived

    socket_info_t	*si,		// result, not NULL, will be initialized
    int			sock,		// socket id, maybe -1
    bool		get_name	// true: alloc 'address' and copy name
					// -> call ResetSocketInfo() to free mem
);

///////////////////////////////////////////////////////////////////////////////

// returns a static string
ccp PrintSocketInfo ( int protocol, int type );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_FILE_H

