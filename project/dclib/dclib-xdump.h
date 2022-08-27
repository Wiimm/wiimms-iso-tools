
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

#ifndef DCLIB_XDUMP_H
#define DCLIB_XDUMP_H 1

#include "dclib-basics.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			consts and structs		///////////////
///////////////////////////////////////////////////////////////////////////////

#define XDUMP_MAX_BYTES_PER_LINE	600
#define XDUMP_GOOD_NUM_BUF_SIZE		(6*XDUMP_MAX_BYTES_PER_LINE+1)
#define XDUMP_GOOD_TEXT_BUF_SIZE	(XDUMP_MAX_BYTES_PER_LINE+1)

#define XDUMP_MAX_LINE_WIDTH		(XDUMP_GOOD_NUM_BUF_SIZE+XDUMP_GOOD_TEXT_BUF_SIZE+30)

///////////////////////////////////////////////////////////////////////////////
// [[XDumpCommand_t]]

typedef enum XDumpCommand_t
{
    XDUMPC_DUMP,
    XDUMPC_DIFF,
    XDUMPC_SCAN,
    XDUMPC__N
}
__attribute__ ((packed)) XDumpCommand_t;

extern const ccp XDumpCommandName[XDUMPC__N];

///////////////////////////////////////////////////////////////////////////////
// [[XDumpFormat_t]]

typedef enum XDumpFormat_t
{
    XDUMPF_AUTO,
    XDUMPF_INT_1,
    XDUMPF_INT_2,
    XDUMPF_INT_3,
    XDUMPF_INT_4,
    XDUMPF_INT_5,
    XDUMPF_INT_6,
    XDUMPF_INT_7,
    XDUMPF_INT_8,
    XDUMPF_FLOAT,
    XDUMPF_DOUBLE,
    XDUMPF__N,
}
__attribute__ ((packed)) XDumpFormat_t;

extern const ccp XDumpFormatName[XDUMPF__N];
extern const char XDumpFormatOption[XDUMPF__N+1];

///////////////////////////////////////////////////////////////////////////////
// [[XDumpOutputFunc]]

struct XDump_t;
typedef int (*XDumpOutputFunc)
(
    struct XDump_t	*xd,		// internal copy of params
    ccp			prefix,		// NULL or additional prefix (used by diff)
    ccp			dump,		// dump to print
    ccp			text		// text to print
);

int XDumpStandardOutput
(
    struct XDump_t	*xd,		// internal copy of params
    ccp			prefix,		// NULL or additional prefix (used by diff)
    ccp			dump,		// dump to print
    ccp			text		// text to print
);

///////////////////////////////////////////////////////////////////////////////
// [[XDumpDumpFunc]]

typedef int (*XDumpDumpFunc)
(
    // returns <0 on error, or number of dumped bytes

    struct XDump_t	*xd,		// current params
    const void		*data,		// data to dump
    uint		size,		// size of 'data'
    bool		term		// false: print only whole lines
					// true:  print complete data
);

///////////////////////////////////////////////////////////////////////////////
// [[XDump_t]]

typedef struct XDump_t
{
    //--- options

    XDumpCommand_t	cmd;		// command
    XDumpFormat_t	format;		// scan or output format
    dcEndian_t		endian;		// endian

    bool		mode_dec;	// print integers as decimal instead of hex
    bool		mode_zero;	// print leading zeros
    bool		mode_c;		// print in c format
    bool		mode_align;	// align addresses by 'addr % bytes_per_line'
    bool		mode_ignore;	// ignore lines with NULL bytes only

    bool		print_format;	// print an info line about the format
    bool		print_addr;	// print address field
    bool		print_number;	// print number field
    bool		print_endmarker;// print "/" as end of data marker
    bool		print_text;	// print ascii text field
    bool		print_summary;	// print a summary at end of dump
    bool		print_diff_sep;	// print an empty line after differ lines

    bool		have_trigger;	// true: trigger is active
    u64			trigger;	// the trigger value

    u64			start_addr;	// start address
    u64			assumed_size;	// assumed output size, used for 'addr_fw'
    uint		min_addr_fw;	// min field width of address
    uint		max_addr_fw;	// max field width of address; 0=AUTO
    uint		min_number_fw;	// >0: minimal field width for number dump
    uint		min_width;	// minimal output width in bytes per line
    uint		max_width;	// maximal output width in bytes per line
    int			extra_space;	// >0: print an extra space after each # column
					// =0: auto

    FILE		*f;		// not NULL: output file
    int			indent;		// indention before 'prefix'
    ccp			prefix;		// not NULL: prefix each line by this
    ccp			eol;		// NULL or eol

    XDumpOutputFunc	out_func;	// NULL or a private function


    //--- user parameters, not used, not changed

    int			user_int;
    void		*user_ptr;


    //--- runtime parameters

    ccp			num_format1;	// pointer to first numeric format
    ccp			num_format2;	// pointer to second numeric format
    uint		num_format_fw;	// current field width of a number
    uint		max_format_fw;	// max possible fw for format
    ccp			format_name;	// name of the format

    u64			written;	// total written bytes
    u64			addr;		// current address
    uint		addr_fw;	// field width of address
    const u64		*addr_break;	// NULL or null terminated list:
					//	break line at this addresses

    uint		bytes_per_col;	// bytes per column
    uint		cols_per_line;	// columns per line
    uint		bytes_per_line;	// bytes per line
    uint		col_space;	// >0: print an extra space after each # column
    uint		null_lines;	// number of currently skipped NULL lines

    bool		format_printed;	// true: format line printed
    uint		last_number_fw;	// last field with of number column
    uint		diff_count;	// number of lines with differences
    uint		diff_size;	//  <0: size1>size2 / >0: size1>size2

    //--- functions

    XDumpDumpFunc	dump_func;	// dump processing function
    const endian_func_t	*endian_func;	// endian functions
}
XDump_t;

//-----------------------------------------------------------------------------

extern ccp  hexdump_prefix;
extern ccp  hexdump_eol;
extern bool hexdump_align;

//-----------------------------------------------------------------------------

void InitializeXDump ( XDump_t *xd );

void InitializeXDumpEx
(
    XDump_t	*xd,
    FILE	*f,
    int		indent,
    u64		addr,
    int		addr_fw,
    int		row_len
);

void SetupXDump ( XDump_t *xd, XDumpCommand_t cmd );
static inline void ResetXDump ( XDump_t *xd ) {}

void PrintXDumpParam
(
    FILE		*f,		// output file, abort if NULL
    int			indent,		// indention
    const ColorSet_t	*colset,	// NULL or color set
    const XDump_t	*xd		// data to print
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			dump interface			///////////////
///////////////////////////////////////////////////////////////////////////////

int XDump
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version
    const void		*data,		// data to dump
    uint		size,		// size of 'data'
    bool		term		// false: print only whole lines
					// true:  print complete data
);

///////////////////////////////////////////////////////////////////////////////

int XDumpByFile
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version
    FILE		*f,		// input file
    u64			max_size	// >0: limit size to print
);

///////////////////////////////////////////////////////////////////////////////

int XDump16 // wrapper for a standard hexdump
(
    // returns <0 on error, or number of dumped bytes

    FILE		*f_out,		// output file
    int			indent,		// indention
    u64			addr,		// start address
    const void		*data,		// data
    uint		size		// size of 'data'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			diff interface			///////////////
///////////////////////////////////////////////////////////////////////////////

int XDiff
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version

    const void		*data1,		// data1 to compare
    uint		size1,		// size of 'data1'
    bool		term1,		// true: 'data1' completed

    const void		*data2,		// data2 to compare
    uint		size2,		// size of 'data2'
    bool		term2,		// true: 'data2' completed

    uint		limit_lines,	// >0: limit number of differ-lines
    bool		colorize	// true: use red and green on output
);

///////////////////////////////////////////////////////////////////////////////

int XDiffByFile
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version
    FILE		*f1,		// first input file
    FILE		*f2,		// second input file
    u64			max_size,	// >0: limit comparing size
    uint		limit_lines,	// >0: limit number of differ-lines
    bool		colorize	// true: use red and green on output
);

///////////////////////////////////////////////////////////////////////////////

int XDiff16
(
    // returns <0 on error, or number of dumped bytes

    FILE		*f_out,		// output file
    int			indent,		// indention
    u64			addr,		// start address

    const void		*data1,		// data
    uint		size1,		// size of 'data'

    const void		*data2,		// data
    uint		size2,		// size of 'data'

    uint		limit_lines,	// >0: limit number of differ-lines
    bool		colorize	// true: use red and green on output
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan interface			///////////////
///////////////////////////////////////////////////////////////////////////////

int XScan
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version
    FastBuf_t		*dest,		// not NULL: append scanned data here

    const void		*data,		// data to scan
    uint		size,		// size of 'data'
    bool		term,		// true: accept uncomplete last line
    u64			max_size	// >0: limit output size
);

///////////////////////////////////////////////////////////////////////////////

s64 XScanByFile
(
    // returns <0 on error, or number of written bytes
    XDump_t		*xd,		// params; if NULL: use local version
    FastBuf_t		*dest,		// not NULL: append scanned data here
    FILE		*f,		// input file
    u64			max_size	// >0: limit output size
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_XDUMP_H

