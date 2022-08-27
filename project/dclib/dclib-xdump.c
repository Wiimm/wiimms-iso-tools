
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

#include <stdio.h>
#include <stddef.h>
#include "dclib-xdump.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    data			///////////////
///////////////////////////////////////////////////////////////////////////////

const ccp XDumpCommandName[XDUMPC__N] =
{
	"DUMP",
	"DIFF",
	"SCAN",
};

//-----------------------------------------------------------------------------

const ccp XDumpFormatName[XDUMPF__N] =
{
	"AUTO",
	"INT_1",
	"INT_2",
	"INT_3",
	"INT_4",
	"INT_5",
	"INT_6",
	"INT_7",
	"INT_8",
	"FLOAT",
	"DOUBLE",
};

const char XDumpFormatOption[XDUMPF__N+1] = "A12345678FD";

//
///////////////////////////////////////////////////////////////////////////////
///////////////			output helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

static bool PrintFormat
(
    struct XDump_t	*xd,		// internal copy of params
    int			force		// >0: ignore 'format_printed'
					// >1: print always
)
{
    DASSERT(xd);
    if ( !xd->f || !xd->format_name )
	return false;

    if	( force > 1
	|| ( force > 0 && xd->print_format )
	|| ( xd->print_format && !xd->format_printed )
	)
    {
	xd->format_printed = true;
	fprintf(xd->f,"%s%*s%sFORMAT: %s",
		xd->prefix, xd->indent,"",
		xd->mode_c ? "//" : "#",
		xd->format_name );

	if ( xd->format > XDUMPF_INT_1 )
	    fprintf(xd->f," %s%s",
		xd->endian == DC_BIG_ENDIAN ? "BE" : "LE",
		xd->eol );
	else
	    fputs(xd->eol,xd->f);

	return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

int XDumpStandardOutput
(
    struct XDump_t	*xd,		// internal copy of params
    ccp			prefix,		// NULL or additional prefix (used by diff)
    ccp			dump,		// dump to print
    ccp			text		// NULL or text to print
)
{
    DASSERT(xd);
    DASSERT(dump);

    if (xd->f)
    {
	fprintf(xd->f,"%s%*s%s",
		xd->prefix, xd->indent,"", prefix ? prefix : "" );

	if (xd->print_addr)
	{
	    if (xd->mode_c)
		fprintf(xd->f,"/*%*llx*/ ",xd->addr_fw,xd->addr);

	    else
		fprintf(xd->f,"%*llx:%s",
			xd->addr_fw, xd->addr,
			xd->extra_space >= 0 ? " " : "" );
	}

	if (xd->print_number)
	{
	    if (xd->min_number_fw)
		fprintf(xd->f,"%-*s",xd->min_number_fw,dump);
	    else
		fputs(dump,xd->f);
	}

	if ( text && xd->print_text )
	{
	    if (xd->mode_c)
		fprintf(xd->f," // %s",text);
	    else
		fprintf(xd->f,"%s :%s:",
			xd->extra_space >= 0 ? " " : "", text );
	}

	fputs(xd->eol,xd->f);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static void print_text ( char *buf, const u8 *data, int n )
{
    while ( n-- > 0 )
    {
	const u8 ch = *data++;
	*buf++ = ch >= ' ' && ch < 0x7f ? ch : '.';
    }
    *buf = 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			XParam_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[XParam_t]]

typedef struct XParam_t
{
    const u8	*d;		// pointer to input data
    const u8	*end;		// end of input data

    uint	size;		// size of data to dump
    bool	term;		// true: data complete
    bool	recalc_align;	// true: recalculate alignment
    bool	next_line;	// true: continue at next line
    const u64	*addr_break;	// NULL or pointer to next break

    uint	skip;		// number of columns to skip
    uint	line_size;	// current line size (source bytes)
    uint	max_col;	// max number of columns to print
    int		extra_space;	// extra space counter

    //  buffers
    char text_buf[XDUMP_GOOD_TEXT_BUF_SIZE];
    char num_buf[XDUMP_GOOD_NUM_BUF_SIZE];
}
XParam_t;

///////////////////////////////////////////////////////////////////////////////

static void SetupXParam
	( XParam_t *p, XDump_t *xd, cvp data, uint size, bool term )
{
    DASSERT(p);
    DASSERT(xd);
    DASSERT(data);

    if ( size > INT_MAX )
    {
	size = INT_MAX; // limit because of return value
	term = false;
    }

    memset( p, 0, offsetof(XParam_t,num_buf) + 15 & ~7 );
    p->size		= size;
    p->term		= term;
    p->d		= data;
    p->end		= p->d + size;
    p->recalc_align	= xd->mode_align;
    p->addr_break	= xd->addr_break;
}

///////////////////////////////////////////////////////////////////////////////

static uint SetupXParamLine ( XParam_t *p, XDump_t *xd )
{
    DASSERT(p);
    DASSERT(xd);

    if ( p->recalc_align )
    {
	p->recalc_align = false;
	p->skip = ( xd->addr % xd->bytes_per_line ) / xd->bytes_per_col;
	const int delta = p->skip * xd->bytes_per_col;
	xd->addr -= delta;
	if (xd->print_number)
	    p->size += delta;
    }

    p->next_line	= false;
    p->line_size	= xd->bytes_per_line < p->size
			? xd->bytes_per_line
			: p->term ? p->size : 0;

    if ( p->addr_break )
    {
	while ( *p->addr_break && xd->addr >= *p->addr_break )
	    p->addr_break++;

	if (!*p->addr_break)
	    p->addr_break = 0;
	else
	{
	    const s64 max = *p->addr_break - xd->addr;
	    if ( p->line_size > max )
		p->line_size = max;
	}
    }

    p->max_col		= p->line_size / xd->bytes_per_col;
    p->line_size	= p->max_col * xd->bytes_per_col;
    p->extra_space	= xd->col_space ? xd->col_space+1 : 0;

    if (xd->print_text)
    {
	if (p->skip)
	{
	    const uint skip_size = p->skip * xd->bytes_per_col;
	    if ( skip_size < p->line_size )
	    {
		memset(p->text_buf,' ',skip_size);
		print_text(p->text_buf+skip_size,p->d,p->line_size-skip_size);
	    }
	    else
		*p->text_buf = 0; // should never happen!
	}
	else
	    print_text(p->text_buf,p->d,p->line_size);
    }

    return p->max_col;
}

///////////////////////////////////////////////////////////////////////////////

static bool PrintNullSummary ( XDump_t *xd )
{
    if ( !xd->null_lines || !xd->out_func )
	return false;

    char buf[100];
    const uint skipped = xd->null_lines * xd->bytes_per_line;
    snprintf(buf,sizeof(buf),"%s#NULL: 0x%x bytes",
		xd->extra_space >= 0 ? " " : "", skipped );

    xd->addr -= skipped;
    xd->out_func(xd,0,buf,0);
    xd->addr += skipped;
    xd->null_lines = 0;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

static bool CheckXParamNull ( XParam_t *p, XDump_t *xd )
{
    DASSERT(p);
    DASSERT(xd);

    static u8 nullbuf[XDUMP_MAX_BYTES_PER_LINE] = {};


    const bool skip = p->line_size == xd->bytes_per_line
			&& !memcmp(p->d,nullbuf,xd->bytes_per_line);
    if (skip)
    {
	xd->null_lines++;
	xd->addr += p->line_size;
	p->d     += p->line_size;
	p->size  -= p->line_size;

	return true;
    }

    if ( xd->null_lines && ( !skip || p->size <= xd->bytes_per_line ))
	PrintNullSummary(xd);

    return skip;
}

///////////////////////////////////////////////////////////////////////////////

static void AbortXParamLine ( XParam_t *p, XDump_t *xd, uint col )
{
    const uint col1 = col + 1;
    if ( col1 < p->max_col )
    {
	p->max_col = col1;
	p->line_size = col1 * xd->bytes_per_col;
	p->text_buf[p->line_size] = 0;
	p->recalc_align = xd->mode_align;
	p->next_line	= true;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			output functions		///////////////
///////////////////////////////////////////////////////////////////////////////

static int DumpInt32
(
    // returns <0 on error, or number of dumped bytes

    XDump_t		*xd,		// current params
    const void		*p_data,	// data to dump
    uint		p_size,		// size of 'p_data'
    bool		p_term		// false: print only whole lines
					// true:  print complete data
)
{
    DASSERT(xd);
    if ( !p_data || !xd->out_func )
	return 0;

    XParam_t p;
    SetupXParam(&p,xd,p_data,p_size,p_term);

    while(SetupXParamLine(&p,xd))
    {
	if ( xd->mode_ignore && CheckXParamNull(&p,xd) )
	    continue;

	if (xd->print_number)
	{
	    char *num = p.num_buf;
	    uint col;
	    for ( col = 0; col < xd->cols_per_line; col++ )
	    {
		if (!--p.extra_space)
		{
		    p.extra_space = xd->col_space;
		    *num++ = ' ';
		}

		if ( p.skip > 0 )
		{
		    p.skip--;
		    memset(num,' ',xd->num_format_fw);
		    num += xd->num_format_fw;
		}
		else if ( col < p.max_col )
		{
		    u32 val;
		    switch(xd->bytes_per_col)
		    {
		      case 2:  val = xd->endian_func->rd16(p.d); p.d += 2; break;
		      case 3:  val = xd->endian_func->rd24(p.d); p.d += 3; break;
		      case 4:  val = xd->endian_func->rd32(p.d); p.d += 4; break;
		      default: val = *p.d++; break;
		    }
		    num += sprintf(num,xd->num_format1,val);
		    if ( xd->have_trigger && val == (u32)xd->trigger )
			AbortXParamLine(&p,xd,col);
		}
		else if ( p.term && !p.next_line && !xd->mode_c )
		{
		    if ( p.d < p.end )
			num += sprintf(num, "%*c%u", xd->num_format_fw-1,
					'>', (int)(p.end-p.d) );
		    else if ( xd->print_endmarker )
			num += sprintf(num,"%*c", xd->num_format_fw, '/' );
		    else if ( xd->print_text )
		    {
			memset(num,' ',xd->num_format_fw);
			num += xd->num_format_fw;
		    }
		    p.term = false; // print only once
		}
		else if ( xd->print_text )
		{
		    memset(num,' ',xd->num_format_fw);
		    num += xd->num_format_fw;
		}
	    }
	    if (!xd->print_text)
		while ( num > p.num_buf && num[-1] == ' ' )
		    num--;
	    *num = 0;
	    ASSERT( num < p.num_buf + sizeof(p.num_buf));
	    xd->last_number_fw = num - p.num_buf;
	}
	else
	{
	    p.d += p.line_size - p.skip * xd->bytes_per_col;
	    p.skip = 0;
	}

	int stat = xd->out_func(xd,0,p.num_buf,p.text_buf);
	if ( stat < 0 )
	    return stat;

	xd->addr += p.line_size;
	p.size   -= p.line_size;
    }
    const uint written = p.d - (u8*)p_data;
    xd->written += written;
    return written;
}

///////////////////////////////////////////////////////////////////////////////

static int DumpInt64
(
    // returns <0 on error, or number of dumped bytes

    XDump_t		*xd,		// current params
    const void		*p_data,	// data to dump
    uint		p_size,		// size of 'p_data'
    bool		p_term		// false: print only whole lines
					// true:  print complete data
)
{
    DASSERT(xd);
    if ( !p_data || !xd->out_func )
	return 0;

    XParam_t p;
    SetupXParam(&p,xd,p_data,p_size,p_term);

    while(SetupXParamLine(&p,xd))
    {
	if ( xd->mode_ignore && CheckXParamNull(&p,xd) )
	    continue;

	if (xd->print_number)
	{
	    char *num = p.num_buf;
	    uint col;
	    for ( col = 0; col < xd->cols_per_line; col++ )
	    {
		if (!--p.extra_space)
		{
		    p.extra_space = xd->col_space;
		    *num++ = ' ';
		}

		if ( p.skip > 0 )
		{
		    p.skip--;
		    memset(num,' ',xd->num_format_fw);
		    num += xd->num_format_fw;
		}
		else if ( col < p.max_col )
		{
		    u64 val;
		    switch(xd->bytes_per_col)
		    {
		      case 5:  val = xd->endian_func->rd40(p.d); p.d += 5; break;
		      case 6:  val = xd->endian_func->rd48(p.d); p.d += 6; break;
		      case 7:  val = xd->endian_func->rd56(p.d); p.d += 7; break;
		      default: val = xd->endian_func->rd64(p.d); p.d += 8; break;
		    }
		    num += sprintf(num,xd->num_format1,val);
		    if ( xd->have_trigger && val == (u32)xd->trigger )
			AbortXParamLine(&p,xd,col);
		}
		else if ( p.term && !p.next_line && !xd->mode_c )
		{
		    if ( p.d < p.end )
			num += sprintf(num, "%*c%u", xd->num_format_fw-1,
					'>', (int)(p.end-p.d) );
		    else if ( xd->print_endmarker )
			num += sprintf(num,"%*c", xd->num_format_fw, '/' );
		    else if (xd->print_text)
		    {
			memset(num,' ',xd->num_format_fw);
			num += xd->num_format_fw;
		    }
		    p.term = false; // print only once
		}
		else if (xd->print_text)
		{
		    memset(num,' ',xd->num_format_fw);
		    num += xd->num_format_fw;
		}
	    }
	    *num = 0;
	    ASSERT( num < p.num_buf + sizeof(p.num_buf));
	    xd->last_number_fw = num - p.num_buf;
	}
	else
	{
	    p.d += p.line_size - p.skip * xd->bytes_per_col;
	    p.skip = 0;
	}

	int stat = xd->out_func(xd,0,p.num_buf,p.text_buf);
	if ( stat < 0 )
	    return stat;

	xd->addr += p.line_size;
	p.size   -= p.line_size;
    }

    const uint written = p.d - (u8*)p_data;
    xd->written += written;
    return written;
}

///////////////////////////////////////////////////////////////////////////////

static int DumpFloat
(
    // returns <0 on error, or number of dumped bytes

    XDump_t		*xd,		// current params
    const void		*p_data,	// data to dump
    uint		p_size,		// size of 'p_data'
    bool		p_term		// false: print only whole lines
					// true:  print complete data
)
{
    DASSERT(xd);
    if ( !p_data || !xd->out_func )
	return 0;

    XParam_t p;
    SetupXParam(&p,xd,p_data,p_size,p_term);

    while(SetupXParamLine(&p,xd))
    {
	if ( xd->mode_ignore && CheckXParamNull(&p,xd) )
	    continue;

	if (xd->print_number)
	{
	    char *num = p.num_buf;
	    uint col;
	    for ( col = 0; col < xd->cols_per_line; col++ )
	    {
		if (!--p.extra_space)
		{
		    p.extra_space = xd->col_space;
		    *num++ = ' ';
		}

		if ( p.skip > 0 )
		{
		    p.skip--;
		    memset(num,' ',xd->num_format_fw);
		    num += xd->num_format_fw;
		}
		else if ( col < p.max_col )
		{
		    double val;
		    ccp format;
		    if ( xd->bytes_per_col == 4 )
		    {
			val = xd->endian_func->rdf4(p.d);
			format	= fabs(val) <= 9999.0 && fabs(val) >= 1e-3
				? xd->num_format1 : xd->num_format2;
			p.d += 4;
		    }
		    else
		    {
			val = xd->endian_func->rdf8(p.d);
			format	= fabs(val) <= 999999999.0 && fabs(val) >= 1e-6
				? xd->num_format1 : xd->num_format2;
			p.d += 8;
		    }
		    num += sprintf(num,format,val);
		}
		else if ( p.term && !p.next_line && !xd->mode_c )
		{
		    if ( p.d < p.end )
			num += sprintf(num, "%*c%u", xd->num_format_fw-1,
					'>', (int)(p.end-p.d) );
		    else if ( xd->print_text )
			num += sprintf(num,"%*c", xd->num_format_fw, '/' );
		    else if (xd->print_text)
		    {
			memset(num,' ',xd->num_format_fw);
			num += xd->num_format_fw;
		    }
		    p.term = false; // print only once
		}
		else if (xd->print_text)
		{
		    memset(num,' ',xd->num_format_fw);
		    num += xd->num_format_fw;
		}
	    }
	    *num = 0;
	    ASSERT( num < p.num_buf + sizeof(p.num_buf));
	    xd->last_number_fw = num - p.num_buf;
	}
	else
	{
	    p.d += p.line_size - p.skip * xd->bytes_per_col;
	    p.skip = 0;
	}

	int stat = xd->out_func(xd,0,p.num_buf,p.text_buf);
	if ( stat < 0 )
	    return stat;

	xd->addr += p.line_size;
	p.size   -= p.line_size;
    }

    const uint written = p.d - (u8*)p_data;
    xd->written += written;
    return written;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    setup			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeXDump ( XDump_t *xd )
{
    DASSERT(xd);
    memset(xd,0,sizeof(*xd));

    xd->f		= stdout;
    xd->print_format	=
    xd->print_addr	=
    xd->print_number	=
    xd->print_endmarker	=
    xd->print_text	=
    xd->print_diff_sep	=
    xd->print_summary	= true;
}

///////////////////////////////////////////////////////////////////////////////

ccp  hexdump_prefix = "";
ccp  hexdump_eol    = "\n";
bool hexdump_align  = false;

//-----------------------------------------------------------------------------

void InitializeXDumpEx
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

///////////////////////////////////////////////////////////////////////////////

void SetupXDump ( XDump_t *xd, XDumpCommand_t cmd )
{
    DASSERT(xd);
    char buf[100];

    xd->cmd = (uint)cmd < XDUMPC__N ? cmd : XDUMPC_DIFF;

    if ( xd->endian != DC_SECOND_ENDIAN )
	xd->endian = DC_DEFAULT_ENDIAN;
    xd->endian_func = xd->endian == DC_LITTLE_ENDIAN ? &le_func : &be_func;


    //--- address field width

    xd->addr = xd->start_addr;
    xd->addr_fw = xd->min_addr_fw;
    if ( !xd->min_addr_fw || xd->addr_fw < xd->min_addr_fw )
    {
	uint len = snprintf(buf,sizeof(buf),"%llx",xd->addr);
	if ( xd->addr_fw < len )
	    xd->addr_fw = len;

	const u64 size = xd->assumed_size ? xd->assumed_size : 0x100;
	len = snprintf(buf,sizeof(buf),"%llx", xd->addr + size - 1 );
	if ( xd->addr_fw < len )
	    xd->addr_fw = len;

	if (xd->max_addr_fw)
	{
	    if ( xd->addr_fw > xd->max_addr_fw )
		 xd->addr_fw = xd->max_addr_fw;
	    if ( xd->addr_fw < xd->min_addr_fw )
		 xd->addr_fw = xd->min_addr_fw;
	}
    }


    //--- misc

    xd->indent = NormalizeIndent(xd->indent);
    if (!xd->out_func)
	xd->out_func = XDumpStandardOutput;
    if (!xd->prefix)
	xd->prefix = EmptyString;
    if (!xd->eol)
	xd->eol = "\n";

    xd->num_format2 = 0;
    xd->format_printed = 0;


    //--- format

    if ( (uint)xd->format >= XDUMPF__N )
	xd->format = XDUMPF_AUTO;

    if ( xd->format == XDUMPF_AUTO && xd->cmd != XDUMPC_SCAN )
	xd->format = XDUMPF_INT_1;

    switch(xd->format)
    {
	case XDUMPF_INT_2:
	    xd->bytes_per_col	= 2;
	    xd->dump_func	= DumpInt32;
	    xd->format_name	= xd->mode_dec ? "DEC2" : "HEX2";
	    xd->max_format_fw	= 7;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%5u,"  : "0x%04x," )
		: xd->mode_zero	? ( xd->mode_dec ? " %05u" : " %04x" )
		:		  ( xd->mode_dec ? " %5u"  : " %4x" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1);
	    break;

	case XDUMPF_INT_3:
	    xd->bytes_per_col	= 3;
	    xd->dump_func	= DumpInt32;
	    xd->format_name	= xd->mode_dec ? "DEC3" : "HEX3";
	    xd->max_format_fw	= 9;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%8u,"  : "0x%06x," )
		: xd->mode_zero	? ( xd->mode_dec ? " %08u" : " %06x" )
		:		  ( xd->mode_dec ? " %8u"  : " %6x" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1);
	    break;

	case XDUMPF_INT_4:
	    xd->bytes_per_col	= 4;
	    xd->dump_func	= DumpInt32;
	    xd->format_name	= xd->mode_dec ? "DEC4" : "HEX4";
	    xd->max_format_fw	= 11;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%10u,"  : "0x%08x," )
		: xd->mode_zero	? ( xd->mode_dec ? " %010u" : " %08x" )
		:		  ( xd->mode_dec ? " %10u"  : " %8x" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1);
	    break;

	case XDUMPF_INT_5:
	    xd->bytes_per_col	= 5;
	    xd->dump_func	= DumpInt64;
	    xd->format_name	= xd->mode_dec ? "DEC5" : "HEX5";
	    xd->max_format_fw	= 13;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%12llu,"  : "0x%010llx," )
		: xd->mode_zero	? ( xd->mode_dec ? " %012llu" : " %010llx" )
		:		  ( xd->mode_dec ? " %12llu"  : " %10llx" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1ull);
	    break;

	case XDUMPF_INT_6:
	    xd->bytes_per_col	= 6;
	    xd->dump_func	= DumpInt64;
	    xd->format_name	= xd->mode_dec ? "DEC6" : "HEX6";
	    xd->max_format_fw	= 16;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%15llu,"  : "0x%012llx," )
		: xd->mode_zero	? ( xd->mode_dec ? " %015llu" : " %012llx" )
		:		  ( xd->mode_dec ? " %15llu"  : " %12llx" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1ull);
	    break;

	case XDUMPF_INT_7:
	    xd->bytes_per_col	= 7;
	    xd->dump_func	= DumpInt64;
	    xd->format_name	= xd->mode_dec ? "DEC7" : "HEX7";
	    xd->max_format_fw	= 18;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%17llu,"  : "0x%014llx," )
		: xd->mode_zero	? ( xd->mode_dec ? " %017llu" : " %014llx" )
		:		  ( xd->mode_dec ? " %17llu"  : " %14llx" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1ull);
	    break;

	case XDUMPF_INT_8:
	    xd->bytes_per_col	= 8;
	    xd->dump_func	= DumpInt64;
	    xd->format_name	= xd->mode_dec ? "DEC8" : "HEX8";
	    xd->max_format_fw	= 20;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%19llu,"  : "0x%016llx," )
		: xd->mode_zero	? ( xd->mode_dec ? " %019llu" : " %016llx" )
		:		  ( xd->mode_dec ? " %19llu"  : " %16llx" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1ull);
	    break;

	case XDUMPF_FLOAT:
	    xd->bytes_per_col	= 4;
	    xd->dump_func	= DumpFloat;
	    xd->format_name	= "FLOAT";
	    xd->max_format_fw	= 12;
	    xd->num_format1	= xd->mode_c ? "%11.5g," : " %11.5f";
	    xd->num_format2	= xd->mode_c ? "%11.5g," : " %11.5g";
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1.1);
	    break;

	case XDUMPF_DOUBLE:
	    xd->bytes_per_col = 8;
	    xd->dump_func	= DumpFloat;
	    xd->format_name	= "DOUBLE";
	    xd->max_format_fw	= 21;
	    xd->num_format1	= xd->mode_c ? "%20.8g," : " %20.8f";
	    xd->num_format2	= xd->mode_c ? "%20.8g," : " %20.8g";
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1.1);
	    break;

	default:
	//case XDUMPF_AUTO:
	//case XDUMPF_INT_1:
	    xd->bytes_per_col	= 1;
	    xd->dump_func	= DumpInt32;
	    xd->format_name	= xd->mode_dec ? "DEC1" : "HEX1";
	    xd->max_format_fw	= 5;
	    xd->num_format1
		= xd->mode_c	? ( xd->mode_dec ? "%3u,"  : "0x%02x," )
		: xd->mode_zero	? ( xd->mode_dec ? " %03u" : " %02x" )
		:		  ( xd->mode_dec ? " %3u"  : " %2x" );
	    xd->num_format_fw	= snprintf(buf,sizeof(buf),xd->num_format1,1);
	    break;
    }


    //--- bytes per line

    uint min = xd->min_width ? xd->min_width : 16;
    uint max = xd->max_width ? xd->max_width : min + 2*xd->bytes_per_col/3;
    if ( max > XDUMP_MAX_BYTES_PER_LINE )
	 max = XDUMP_MAX_BYTES_PER_LINE;
    if ( min > max || !xd->min_width )
	 min = max;

    uint ncols = ( min + xd->bytes_per_col - 1 ) / xd->bytes_per_col;
    uint ncols_max = max / xd->bytes_per_col;
    if ( ncols > ncols_max )
	 ncols = ncols_max;

    xd->cols_per_line  = ncols ? ncols : 1;
    xd->bytes_per_line = xd->cols_per_line * xd->bytes_per_col;


    //--- extra space

    if ( xd->extra_space < 0 || xd->cols_per_line <= 3 )
	xd->col_space = 0;
    else
    {
	xd->col_space = xd->extra_space;
	if (!xd->col_space)
	{
	    xd->col_space = 4 / xd->bytes_per_col;
	    if ( xd->col_space < 2 )
		 xd->col_space = 2;
	}

	if ( xd->col_space >= xd->cols_per_line )
	    xd->col_space = 0;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			print xdump params		///////////////
///////////////////////////////////////////////////////////////////////////////

void PrintXDumpParam
(
    FILE		*f,		// output file, abort if NULL
    int			indent,		// indention
    const ColorSet_t	*colset,	// NULL or color set
    const XDump_t	*xd		// data to print
)
{
    if ( !f || !xd )
	return;

    indent = NormalizeIndent(indent);
    if (!colset)
	colset = GetColorSet0();

    fprintf(f,
	"%*s%s" "Command:      %s%s%s\n"
	"%*s%s" "Format:       %s%s, %s%s%s => %s%s\n"
	"%*s%s" "Endian:       %s%s%s\n",
	indent,"", colset->name, colset->value,
	    (uint)xd->cmd < XDUMPC__N ? XDumpCommandName[xd->cmd] : "?",
	    colset->reset,
	indent,"", colset->name, colset->value,
	    (uint)xd->format < XDUMPF__N ? XDumpFormatName[xd->format] : "?",
	    xd->mode_dec ? "decimal" : "hexadecimal",
	    xd->mode_zero ? " with leading zeros" : "",
	    xd->mode_c ? ", print in C syntax" : "",
	    xd->format_name ? xd->format_name : "?",
	    colset->reset,
	indent,"", colset->name, colset->value,
	    xd->endian == DC_LITTLE_ENDIAN ? "LITTLE"
		: xd->endian == DC_BIG_ENDIAN ? "BIG" : "?",
	    colset->reset );

    if (xd->num_format2)
	fprintf(f,
		"%*s%s" "Num format:   %s'%s' & '%s' (fw=%u,max=%u)%s\n",
		indent,"", colset->name, colset->value,
		xd->num_format1 ? xd->num_format1 : "?",
		xd->num_format2, xd->num_format_fw, xd->max_format_fw,
		colset->reset );
    else
	fprintf(f,
		"%*s%s" "Num format:   %s'%s' (fw=%u,max=%u)%s\n",
		indent,"", colset->name, colset->value,
		xd->num_format1 ? xd->num_format1 : "?",
		xd->num_format_fw, xd->max_format_fw,
		colset->reset );

    if (xd->assumed_size)
	fprintf(f, "%*s%s"
		"Address:      %sstart=0x%llx, current=0x%llx, size=0x%llx%s%s\n",
		indent,"", colset->name, colset->value,
		xd->start_addr, xd->addr, xd->assumed_size,
		xd->mode_align ? ", aligned" : "",
		colset->reset );
    else
	fprintf(f, "%*s%s"
		"Address:      %sstart=0x%llx, current=0x%llx%s%s\n",
		indent,"", colset->name, colset->value,
		xd->start_addr, xd->addr,
		xd->mode_align ? ", aligned" : "",
		colset->reset );

    if (xd->have_trigger)
	fprintf(f, "%*s%s"
		"Line trigger: %s%llu = 0x%llx%s%s\n",
		indent,"", colset->name, colset->value,
		xd->trigger, xd->trigger,
		xd->trigger == '\n' ? " (LF)" :
		xd->trigger == '\r' ? " (CR)" :
		xd->trigger == '\t' ? " (TAB)" :
		xd->trigger == ' '  ? " (SPACE)" :
		xd->trigger == 0    ? " (NULL)" :
		"",
		colset->reset );

    fprintf(f,
	"%*s%s" "Address fw:   %smin=%u, max=%u, active=%u%s\n",
	indent,"", colset->name, colset->value,
	xd->min_addr_fw, xd->max_addr_fw, xd->addr_fw,
	colset->reset );

    fprintf(f,
	"%*s%s" "Print:        %s%sformat, %saddress, %snumbers, %stext, %ssummary, %sseparator%s\n",
	indent,"", colset->name, colset->value,
	xd->print_format   ? "" : "no ",
	xd->print_addr     ? "" : "no ",
	xd->print_number   ? "" : "no ",
	xd->print_text     ? "" : "no ",
	xd->print_summary  ? "" : "no ",
	xd->print_diff_sep ? "" : "no ",
	colset->reset );

    fprintf(f,
	"%*s%s" "Output file:  %s%s, indent=%d, prefix=%s, eol=%s%s\n",
	indent,"", colset->name, colset->value,
	xd->f == stdout ? "STDOUT"
		: xd->f == stderr ? "STDERR"
		: xd->f ? "FILE" : "NONE",
	xd->indent,
	!xd->prefix ? "NULL" : !*xd->prefix ? "EMPTY" : "DEFINED",
	!xd->eol ? "NULL"
		: !*xd->eol			? "EMPTY"
		: !strcmp(xd->eol,"\n")		? "LF"
		: !strcmp(xd->eol,"\r")		? "CR"
		: !strcmp(xd->eol,"\r\n")	? "CR+LF"
		: "DEFINED",
	colset->reset );

    fprintf(f,
	"%*s%s" "Bytes/line:   %smin=%u, max=%u%s\n"
	"%*s%s" "        =>:   %sbytes/column = %u, columns/line = %u, bytes/line = %u%s\n"
	"%*s%s" "Extra space:  %severy %u column => every %u column%s\n",
	indent,"", colset->name, colset->value,
		xd->min_width, xd->max_width,
		colset->reset,
	indent,"", colset->name, colset->value,
		xd->bytes_per_col, xd->cols_per_line, xd->bytes_per_line,
		colset->reset,
	indent,"", colset->name, colset->value,
		xd->extra_space, xd->col_space,
		colset->reset );

    if (xd->mode_ignore)
	fprintf(f,
		"%*s" "%sNULL bytes:   %sPrint a summary for lines with NULL bytes only.%s\n",
		indent,"", colset->name, colset->value, colset->reset );
    fprintf(f,
	"%*s%s" "Functions:    %sdump=%s, output=%s, endian=%s%s\n",
	indent,"", colset->name, colset->value,
	!xd->dump_func ? "-" : "SET",
	!xd->out_func ? "-"
		: xd->out_func == XDumpStandardOutput
		? "STANDARD" : "SET",
	!xd->endian_func ? "-"
		: xd->endian_func == &le_func ? "LITTLE"
		: xd->endian_func == &be_func ? "BIG"
		: "SET",
	colset->reset );

    fprintf(f,
	"%*s%s" "User param:   %sint=%d, ptr=%s%s\n",
	indent,"", colset->name, colset->value,
	xd->user_int, xd->user_ptr ? "SET" : "-",
	colset->reset );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			dump interface			///////////////
///////////////////////////////////////////////////////////////////////////////

static void SetupRemainder
	( XDump_t *xd, const XDump_t *src, XDumpCommand_t cmd, uint size )
{
    DASSERT(xd);
    DASSERT(src);

    memcpy(xd,src,sizeof(*xd));
    xd->format		= XDUMPF_INT_1;
    xd->start_addr	= xd->addr;
    xd->assumed_size	= size;
    xd->min_width	= size;
    xd->max_width	= size;
    xd->extra_space	= 0;
    xd->mode_align	= false;
    xd->mode_ignore	= false;

    if (xd->print_text)
	xd->min_number_fw = xd->last_number_fw;

    SetupXDump(xd,cmd);
}

///////////////////////////////////////////////////////////////////////////////

static int XDumpHelper
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; SetupXDump() already done!
    const void		*data,		// data to dump
    uint		size,		// size of 'data'
    bool		term		// false: print only whole lines
					// true:  print complete data
)
{
    DASSERT(xd);
    DASSERT(data||!size);

    PrintFormat(xd,0);
    int stat = xd->dump_func(xd,data,size,term);
    if ( stat < 0 )
	return stat;

    if (term)
    {
	PrintNullSummary(xd);

	if ( stat < size )
	{
	    XDump_t local;
	    SetupRemainder(&local,xd,XDUMPC_DUMP,size-stat);
	    PrintFormat(&local,0);

	    int stat2	= local.dump_func(&local,data+stat,local.min_width,term);
	    xd->addr	= local.addr;
	    xd->written	= local.written;

	    if ( stat2 < 0 )
		return stat2;
	    stat += stat2;
	}

	if ( xd->f && xd->print_summary )
	{
	    fprintf(xd->f,"%s%*s",xd->prefix,xd->indent,"");
	    if ( xd->print_addr )
	    {
		if (xd->mode_c)
		    fprintf(xd->f,"//%*llx// [%llu Bytes]%s",
			xd->addr_fw, xd->addr,
			xd->written, xd->eol );

		else
		    fprintf(xd->f,"%*llx:%s [%llu Bytes]%s",
			xd->addr_fw, xd->addr,
			xd->extra_space >= 0 ? " " : "",
			xd->written, xd->eol );
	    }
	    else
	    {
		if (xd->mode_c)
		    fprintf(xd->f,"// [%llu Bytes]%s",
			xd->written, xd->eol );

		else
		    fprintf(xd->f,"%s [%llu Bytes]%s",
			xd->extra_space >= 0 ? " " : "",
			xd->written, xd->eol );
	    }
	}
    }
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int XDump
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version
    const void		*data,		// data to dump
    uint		size,		// size of 'data'
    bool		term		// false: print only whole lines
					// true:  print complete data
)
{
    XDump_t local_xd;
    if (!xd)
    {
	InitializeXDump(&local_xd);
	xd = &local_xd;
    }
    SetupXDump(xd,XDUMPC_DUMP);

    return XDumpHelper(xd,data,size,term);
}

///////////////////////////////////////////////////////////////////////////////

int XDumpByFile
(
    // returns <0 on error, or number of dumped bytes
    XDump_t		*xd,		// params; if NULL: use local version
    FILE		*f,		// input file
    u64			max_size	// >0: limit size to print
)
{
    DASSERT(f);

    XDump_t local_xd;
    if (!xd)
    {
	InitializeXDump(&local_xd);
	xd = &local_xd;
    }
    SetupXDump(xd,XDUMPC_DUMP);

    char buf[0x4100];
    uint start = 0;
    if (!max_size)
	max_size = M1(max_size);

    uint count = 0;
    while ( max_size > 0 )
    {
	uint max_read = ( sizeof(buf) - start ) / 0x1000 * 0x1000;
	if ( max_read > max_size )
	    max_read = max_size;

	size_t stat = fread(buf+start,1,max_read,f);
	if (!stat)
	    break;
	max_size -= stat;
	stat += start;

	int dstat = XDumpHelper(xd,buf,stat,false);
	if ( dstat < 0 )
	    return dstat;
	count += dstat;

	start = stat - dstat;
	if (start)
	    memmove(buf,buf+dstat,start);
    }

    int dstat = XDumpHelper(xd,buf,start,true);
    return dstat < 0 ? dstat : count + dstat;
}

///////////////////////////////////////////////////////////////////////////////

int XDump16 // wrapper for a standard hexdump
(
    // returns <0 on error, or number of dumped bytes

    FILE		*f_out,		// output file
    int			indent,		// indention
    u64			addr,		// start address
    const void		*data,		// data
    uint		size		// size of 'data'
)
{
    XDump_t xd;
    InitializeXDump(&xd);
    xd.f		= f_out;
    xd.indent		= indent;
    xd.start_addr	= addr;
    xd.assumed_size	= size;

    return XDump(&xd,data,size,true);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			diff interface			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef struct XDiff_t
{
    const void		*data;		// data to compare
    uint		size;		// size of 'data'
    bool		term;		// 'data' reached end

    XDump_t		xd;		// dump parameters

    //--- buffers to catch output
    char num_buf[XDUMP_GOOD_NUM_BUF_SIZE+4];
    char text_buf[XDUMP_GOOD_TEXT_BUF_SIZE];
}
XDiff_t;

///////////////////////////////////////////////////////////////////////////////

static int XDiffCatchOutput
(
    struct XDump_t	*xd,		// internal copy of params
    ccp			prefix,		// NULL or additional prefix (used by diff)
    ccp			dump,		// dump to print
    ccp			text		// text to print
)
{
    DASSERT(xd);
    DASSERT(dump);
    DASSERT(text);

    XDiff_t *diff = xd->user_ptr;
    DASSERT(diff);

    StringCopyS(diff->num_buf,sizeof(diff->num_buf),dump);
    StringCopyS(diff->text_buf,sizeof(diff->text_buf),text);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static void SetupXDiff
(
    XDiff_t		*xdiff,		// data to setup

    XDump_t		*xd,		// NULL or template

    const void		*data,		// data to compare
    uint		size,		// size of 'data'
    bool		term		// 'data' reached end
)
{
    DASSERT(xdiff);

    xdiff->data = data;
    xdiff->size = size;
    xdiff->term = term;
    *xdiff->num_buf = *xdiff->text_buf = 0;

    if (xd)
	memcpy(&xdiff->xd,xd,sizeof(xdiff->xd));
    else
	InitializeXDump(&xdiff->xd);

    xd = &xdiff->xd;
    SetupXDump(xd,XDUMPC_DIFF);

    xd->user_ptr	= xdiff;
    xd->out_func	= XDiffCatchOutput;
    xd->mode_ignore	= false;
}

///////////////////////////////////////////////////////////////////////////////

static bool XDiffText
(
    // returns true if differ

    XDump_t		*xd,		// original params; SetupXDump() already done!
    XDiff_t		*d1,		// first diff file
    XDiff_t		*d2,		// second diff file
    bool		colorize	// true: use red and green on output
)
{
    DASSERT(xd);
    DASSERT(d1);
    DASSERT(d2);

    char *n1 = d1->num_buf;
    char *n2 = d2->num_buf;

    int len1 = strlen(n1);
    int len2 = strlen(n2);
    if ( len1 < len2 )
    {
	memset( n1 + len1, ' ', len2-len1 );
	n1[len2] = 0;
	len1 = len2;
    }
    else if ( len1 > len2 )
    {
	memset( n2 + len2, ' ', len1-len2 );
	n2[len1] = 0;
    }

    if ( xd->min_number_fw < len1 )
	 xd->min_number_fw = len1;

    if (!strcmp(n1,n2))
	return false;

    for(;;)
    {
	while ( *n1 == ' ' && *n2 == ' ' )
	    n1++, n2++;
	if ( !*n1 || !*n2 )
	    break;

	char *x = n2;
	while ( (u8)*n2 > ' ' && *n1 == *n2 && *n2 != '/' && *n2 != '/' )
	    n1++, n2++;
	if ( n2 > x && (u8)*n2 <= ' ' )
	{
	    while ( x < n2 )
		*x++ = ' ';
	    x[-1] = '.';
	}
	else
	    while ( (u8)*n1 > ' ' || (u8)*n2 > ' ' )
		n1++, n2++;
    }

    xd->out_func(xd,"< ",d1->num_buf,d1->text_buf);
    xd->out_func(xd,"> ",d2->num_buf,d2->text_buf);
    if ( xd->f && xd->print_diff_sep )
	fputs(xd->eol,xd->f);

    xd->diff_count++;
    return true;
}


///////////////////////////////////////////////////////////////////////////////

#undef OPTIMIZED_SEARCH
#define OPTIMIZED_SEARCH 0

static int XDiffHelper
(
    // returns <0 on error, or number of dumped bytes

    XDump_t		*xd,		// original params; SetupXDump() already done!
    XDiff_t		*d1,		// first diff file
    XDiff_t		*d2,		// second diff file
    uint		limit_lines,	// >0: limit number of differ-lines
    bool		colorize	// true: use red and green on output
)
{
    DASSERT(xd);
    DASSERT(d1);
    DASSERT(d2);
    DASSERT( d1->xd.bytes_per_col	== d1->xd.bytes_per_col );
    DASSERT( d1->xd.bytes_per_line	== d1->xd.bytes_per_line );
    DASSERT( d1->xd.format		== d1->xd.format );


    //--- size calculations & setup

    uint count = 0;
    int size1 = d1->size;
    int size2 = d2->size;
    if ( !d1->term && !d2->term )
    {
	if ( size1 < size2 )
	    size2 = size1;
	else if ( size1 > size2 )
	    size1 = size2;
    }

    if (!d1->term)
	size1 = size1 / xd->bytes_per_line * xd->bytes_per_line;

    if (!d2->term)
	size2 = size2 / xd->bytes_per_line * xd->bytes_per_line;

    const u8 *p1 = d1->data;
    const u8 *p2 = d2->data;

    const u64 *save_addr_break	= xd->addr_break;
    const u64 *addr_break	= save_addr_break;
    xd->addr_break		= 0;


    //--- main loop

    int done = 0;
    while ( size1 > 0 && size2 > 0
	|| !done && ( size1 > 0 || size2 > 0 ) )
    {
	//--- optimized search

    #if OPTIMIZED_SEARCH
	uint same = CountEqual(p1,p2, size1 < size2 ? size1 : size2 );
	if ( same >= xd->bytes_per_line )
	{
	    same = same / xd->bytes_per_line * xd->bytes_per_line;
	    p1		+= same;
	    p2		+= same;
	    xd->addr	+= same;
	    count	+= same;
	    size1	-= same;
	    size2	-= same;
	}
    #endif

	//---
	int dumpsize1 = size1 < (int)xd->bytes_per_line ? size1 : xd->bytes_per_line;
	int dumpsize2 = size2 < (int)xd->bytes_per_line ? size2 : xd->bytes_per_line;

	bool have_break = false;
	if (addr_break)
	{
	    while ( *addr_break && xd->addr >= *addr_break )
		addr_break++;

	    if (!*addr_break)
		addr_break = 0;
	    else
	    {
		const s64 max = *addr_break - xd->addr;
		if ( dumpsize1 > max || dumpsize2 > max )
		{
		    have_break = true;
		    if ( dumpsize1 > max ) dumpsize1 = max;
		    if ( dumpsize2 > max ) dumpsize2 = max;
		}
	    }
	}

    #if !OPTIMIZED_SEARCH
	const uint diffsize = dumpsize1 < dumpsize2 ? dumpsize1 : dumpsize2;
    #endif
	uint donesize = dumpsize1 > dumpsize2 ? dumpsize1 : dumpsize2;

//DEL fprintf(stderr,">>> %d,%d done=%d, donesize=%d, diffsize=%d\n",size1,size2,done,donesize,diffsize);

    #if !OPTIMIZED_SEARCH
	if ( diffsize > 0
		&& ( diffsize < xd->bytes_per_line || memcmp(p1,p2,diffsize) ))
    #endif
	{
	    if ( dumpsize1 != dumpsize2 && !xd->diff_size )
	    {
		xd->diff_size = dumpsize1 - dumpsize2;
		if (xd->diff_size)
		    xd->diff_count++;
	    }

	    if ( dumpsize1 >= (int)xd->bytes_per_col )
	    {
		const int stat = xd->dump_func(&d1->xd,p1,dumpsize1,true);
		if ( stat >= 0 && stat < dumpsize1 && donesize > stat )
		    donesize = stat;
	    }
	    else if ( dumpsize1 > 0 )
	    {
		SetupRemainder(&d1->xd,&d1->xd,XDUMPC_DIFF,dumpsize1);
		d1->xd.dump_func(&d1->xd,p1,dumpsize1,true);
	    }
	    else
	    {
		done++;
		strcpy(d1->num_buf,"/");
		*d1->text_buf = 0;
	    }

	    if ( dumpsize2 >= (int)xd->bytes_per_col )
	    {
		const int stat = xd->dump_func(&d2->xd,p2,dumpsize2,true);
		if ( stat >= 0 && stat < dumpsize2 && donesize > stat )
		    donesize = stat;
	    }
	    else if ( dumpsize2 > 0 )
	    {
		SetupRemainder(&d2->xd,&d2->xd,XDUMPC_DIFF,dumpsize2);
		d2->xd.dump_func(&d2->xd,p2,dumpsize2,true);
	    }
	    else
	    {
		done++;
		strcpy(d2->num_buf,"/");
		*d2->text_buf = 0;
	    }

	    XDiffText(xd,d1,d2,colorize);
	    if ( limit_lines && !--limit_lines )
		break;
	}

	p1		+= donesize;
	p2		+= donesize;
	xd->addr	+= donesize;
	count		+= donesize;
	size1		-= donesize;
	size2		-= donesize;

	if (have_break)
	{
	    if (!d1->term)
		size1 = size1 / xd->bytes_per_line * xd->bytes_per_line;

	    if (!d2->term)
		size2 = size2 / xd->bytes_per_line * xd->bytes_per_line;
	}
    }

    xd->addr_break = save_addr_break;
    return count;
}

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
)
{
    // fast test for identical data
    if ( term1 && term2 && size1 == size2 && ( !size1 || !memcmp(data1,data2,size1) ))
	return 0;

    XDump_t local_xd;
    if (!xd)
    {
	InitializeXDump(&local_xd);
	xd = &local_xd;
    }
    SetupXDump(xd,XDUMPC_DIFF);

    XDiff_t diff1, diff2;
    SetupXDiff(&diff1,xd,data1,size1,term1);
    SetupXDiff(&diff2,xd,data2,size2,term2);
    return XDiffHelper(xd,&diff1,&diff2,limit_lines,colorize);
}

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
)
{
    DASSERT(f1);
    DASSERT(f2);

    XDump_t local_xd;
    if (!xd)
    {
	InitializeXDump(&local_xd);
	xd = &local_xd;
    }
    SetupXDump(xd,XDUMPC_DIFF);

    char buf1[0x4100];
    char buf2[sizeof(buf1)];

    XDiff_t diff1, diff2;
    SetupXDiff(&diff1,xd,buf1,0,false);
    SetupXDiff(&diff2,xd,buf2,0,false);

    uint start1 = 0, start2 = 0;
    if (!max_size)
	max_size = M1(max_size);

    uint count = 0;
    while ( max_size > 0 )
    {
	uint max_read1 = ( sizeof(buf1) - start1 ) / 0x1000 * 0x1000;
	if ( max_read1 > max_size )
	     max_read1 = max_size;
	size_t stat1 = fread(buf1+start1,1,max_read1,f1);

	uint max_read2 = ( sizeof(buf2) - start2 ) / 0x1000 * 0x1000;
	if ( max_read2 > max_size )
	     max_read2 = max_size;
	size_t stat2 = fread(buf2+start2,1,max_read2,f2);

	uint stat = stat1 < stat2 ? stat1 : stat2;
	max_size -= stat;

	diff1.size = stat1 + start1;
	diff1.term = !stat1;
	diff2.size = stat2 + start2;
	diff2.term = !stat2;

	int dstat = XDiffHelper(xd,&diff1,&diff2,limit_lines,colorize);

	if ( dstat <= 0 )
	    return dstat;
	count += dstat;
	if (xd->diff_size)
	    return count;

	start1 = stat1 + start1 - dstat;
	if (start1)
	    memmove(buf1,buf1+dstat,start1);

	start2 = stat2 + start2 - dstat;
	if (start2)
	    memmove(buf2,buf2+dstat,start2);
    }
    return count;
}

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
)
{
    XDump_t xd;
    InitializeXDump(&xd);
    xd.f		= f_out;
    xd.indent		= indent;
    xd.start_addr	= addr;
    xd.assumed_size	= size1 > size2 ? size1 : size2;

    return XDiff( &xd,	data1,size1,true,
			data2,size2,true, limit_lines, colorize );
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			scan interface			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[XScan_t]]

typedef struct XScan_t
{
    FastBuf_t		*dest;		// not NULL: append scanned data here & ignore 'fout'
    FILE		*fout;		// valid output file
    u64			written;	// number of written bytes

    bool		auto_format;	// true: detect '#FORMAT:'
    XDumpFormat_t	format;		// current scan format
    dcEndian_t		endian;		// current endian
    uint		num_base;	// 8:octal, 10:decimal, 16:hexadecimal
    ccp			num_allowed;	// string with allowed first characters

    ccp			prefix;		// not NULL: skip this prefix
    uint		prefix_len;	// length of prefix

    const endian_func_t	*endian_func;	// endian functions
}
XScan_t;

///////////////////////////////////////////////////////////////////////////////

static void UpdateXScan ( XScan_t *scan )
{
    DASSERT(scan);

    if ( !scan->dest && !scan->fout )
	scan->fout = stdout;

    if (scan->prefix)
    {
	ccp prefix = scan->prefix;
	while ( *prefix > 0 && *prefix <= ' ' )
	    prefix++;
	scan->prefix = *prefix ? prefix : 0;
	scan->prefix_len = strlen(prefix);
    }
    else
	scan->prefix_len = 0;

    scan->endian_func = scan->endian == DC_LITTLE_ENDIAN ? &le_func : &be_func;

    if ( scan->format == XDUMPF_AUTO )
	scan->format = XDUMPF_INT_1;

    if ( scan->num_base < 2 || scan->num_base > 36 )
	scan->num_base = 16;

    scan->num_allowed
	= scan->format == XDUMPF_FLOAT || scan->format == XDUMPF_DOUBLE
	? "+-."
	: "+-~!";
}

///////////////////////////////////////////////////////////////////////////////

static void SetupXScan
(
    XScan_t		*scan,		// data to setup
    XDump_t		*xd,		// NULL or template
    FastBuf_t		*dest		// not NULL: append scanned data here
)
{
    DASSERT(scan);
    memset(scan,0,sizeof(*scan));

    if (xd)
    {
	scan->fout		= xd->f;
	scan->auto_format	= xd->format == XDUMPF_AUTO;
	scan->format		= xd->format;
	scan->endian		= xd->endian;
	scan->num_base		= xd->mode_dec ? 10 : 16;
	scan->prefix		= xd->prefix;
    }
    else
    {
	scan->fout		= stdout;
	scan->auto_format	= true;
	scan->format		= XDUMPF_INT_1;
	scan->endian		= DC_DEFAULT_ENDIAN;
	scan->num_base		= 16;
	scan->prefix		= 0;
    }

    scan->dest = dest;
    UpdateXScan(scan);
}

///////////////////////////////////////////////////////////////////////////////

static void XScanFormat
(
    // returns <0 on error, or number of dumped bytes

    XScan_t		*scan,		// valid scan parameters
    ccp			src,		// source pointer
    ccp			eol		// end of line
)
{
    DASSERT(scan);
    DASSERT(src);
    DASSERT(eol);
    DASSERT( src <= eol );

    noPRINT(stderr,"FORMAT: %.*s\n",(int)(eol-src),src);

    enum { MD_HEX, MD_DEC, MD_OCT, MD_NUM, MD_ENDIAN };

    static const KeywordTab_t keytab[] =
    {
	{ MD_HEX,	"HEX",		0,		XDUMPF_AUTO },
	{ MD_HEX,	"HEX-1",	"HEX1",		XDUMPF_INT_1 },
	{ MD_HEX,	"HEX-2",	"HEX2",		XDUMPF_INT_2 },
	{ MD_HEX,	"HEX-3",	"HEX3",		XDUMPF_INT_3 },
	{ MD_HEX,	"HEX-4",	"HEX4",		XDUMPF_INT_4 },
	{ MD_HEX,	"HEX-5",	"HEX5",		XDUMPF_INT_5 },
	{ MD_HEX,	"HEX-6",	"HEX6",		XDUMPF_INT_6 },
	{ MD_HEX,	"HEX-7",	"HEX7",		XDUMPF_INT_7 },
	{ MD_HEX,	"HEX-8",	"HEX8",		XDUMPF_INT_8 },

	{ MD_DEC,	"DEC",		0,		XDUMPF_AUTO },
	{ MD_DEC,	"DEC-1",	"DEC1",		XDUMPF_INT_1 },
	{ MD_DEC,	"DEC-2",	"DEC2",		XDUMPF_INT_2 },
	{ MD_DEC,	"DEC-3",	"DEC3",		XDUMPF_INT_3 },
	{ MD_DEC,	"DEC-4",	"DEC4",		XDUMPF_INT_4 },
	{ MD_DEC,	"DEC-5",	"DEC5",		XDUMPF_INT_5 },
	{ MD_DEC,	"DEC-6",	"DEC6",		XDUMPF_INT_6 },
	{ MD_DEC,	"DEC-7",	"DEC7",		XDUMPF_INT_7 },
	{ MD_DEC,	"DEC-8",	"DEC8",		XDUMPF_INT_8 },

	{ MD_OCT,	"OCT",		0,		XDUMPF_AUTO },
	{ MD_OCT,	"OCT-1",	"OCT1",		XDUMPF_INT_1 },
	{ MD_OCT,	"OCT-2",	"OCT2",		XDUMPF_INT_2 },
	{ MD_OCT,	"OCT-3",	"OCT3",		XDUMPF_INT_3 },
	{ MD_OCT,	"OCT-4",	"OCT4",		XDUMPF_INT_4 },
	{ MD_OCT,	"OCT-5",	"OCT5",		XDUMPF_INT_5 },
	{ MD_OCT,	"OCT-6",	"OCT6",		XDUMPF_INT_6 },
	{ MD_OCT,	"OCT-7",	"OCT7",		XDUMPF_INT_7 },
	{ MD_OCT,	"OCT-8",	"OCT8",		XDUMPF_INT_8 },

	{ MD_NUM,	"1",		0,		XDUMPF_INT_1 },
	{ MD_NUM,	"2",		0,		XDUMPF_INT_2 },
	{ MD_NUM,	"3",		0,		XDUMPF_INT_3 },
	{ MD_NUM,	"4",		0,		XDUMPF_INT_4 },
	{ MD_NUM,	"5",		0,		XDUMPF_INT_5 },
	{ MD_NUM,	"6",		0,		XDUMPF_INT_6 },
	{ MD_NUM,	"7",		0,		XDUMPF_INT_7 },
	{ MD_NUM,	"8",		0,		XDUMPF_INT_8 },

	{ MD_NUM,	"FLOAT",	0,		XDUMPF_FLOAT },
	{ MD_NUM,	"DOUBLE",	0,		XDUMPF_DOUBLE },

	{ MD_ENDIAN,	"LE",		"LITTLE",	DC_LITTLE_ENDIAN },
	{ MD_ENDIAN,	"BE",		"BIG",		DC_BIG_ENDIAN },

	{0,0,0,0}
    };

    for(;;)
    {
	while ( src < eol && !isalnum((int)*src) )
	    src++;

	char namebuf[20], *name = namebuf;
	while ( src < eol && (u8)*src > ' ' )
	{
	    if ( name < namebuf + sizeof(namebuf) - 1 )
		*name++ = toupper((int)*src);
	    src++;
	}
	*name = 0;

	if ( name == namebuf )
	    break;

	noPRINT(stderr,"SCAN FORMAT: %s\n",namebuf);
	const KeywordTab_t *key = ScanKeyword(0,namebuf,keytab);
	if (!key)
	    continue;

	XDumpFormat_t format = key->opt;
	switch(key->id)
	{
	    case MD_HEX:
		scan->num_base = 16;
		break;

	    case MD_DEC:
		scan->num_base = 10;
		break;

	    case MD_OCT:
		scan->num_base = 8;
		break;

	    case MD_NUM:
		break;

	    case MD_ENDIAN:
		scan->endian = key->opt;
		format = XDUMPF_AUTO;
		break;
	}
	if ( format != XDUMPF_AUTO )
	    scan->format = format;

	noPRINT(stderr,"F: %lld %lld => base=%d, format=%d, endian=%d\n",
		key->id, key->opt,
		scan->num_base, scan->format, scan->endian );
    }
    UpdateXScan(scan);
}

///////////////////////////////////////////////////////////////////////////////

static size_t XScanHelper
(
    // returns <0 on error, or number of dumped bytes

    XScan_t		*scan,		// valid scan parameters
    ccp			source,		// source pointer
    ccp			end,		// end of source
    bool		term,		// true: accept uncomplete last line
    u64			max_size	// >0: limit output size
)
{
    DASSERT(scan);
    DASSERT( scan->dest || scan->fout );
    DASSERT(source);
    DASSERT(end);
    DASSERT( source <= end );

    if (!max_size)
	max_size = M1(max_size);
    max_size -= scan->written;

    ccp src = source;
    u8 destbuf[0x4000+10];
    u8 *destmax = destbuf + sizeof(destbuf) - 10;
    u8 *dest = destbuf;

    while ( src < end && max_size > 0 )
    {
	//--- skip all leading contral characters

	while ( src < end && (u8)*src <= ' ' )
	    src++;


	//--- find end of line

	bool found_eol = term;
	ccp eol = src;
	while ( eol < end )
	{
	    const u8 ch = *eol;
	    if ( !ch || ch == '\n' || ch == '\r' )
	    {
		found_eol = true;
		break;
	    }
	    eol++;
	}
	if (!found_eol)
	    break;

	PRINT0("LINE: |%.*s|\n",(int)(eol-src),src);


	//--- skip prefix

	if ( scan->prefix_len
		&& scan->prefix_len <= eol-src
		&& !memcmp(scan->prefix,src,scan->prefix_len) )
	{
	    src += scan->prefix_len;
	    while ( src < eol && (u8)*src <= ' ' )
		src++;
	}

	if ( src == eol )
	    continue;


	//--- check for "#FORMAT:"

	if (!strncasecmp(src,"#FORMAT:",8))
	{
	    if (scan->auto_format)
		XScanFormat(scan,src+8,eol);
	    src = eol;
	    continue;
	}


	//--- skip address column

	ccp ptr = src;
	while ( ptr < eol && isalnum((int)*ptr) )
	    ptr++;
	noPRINT(stderr,"ADDR: |%.*s|\n",(int)(ptr-src),src);
	if ( ptr > src && ptr < eol && *ptr == ':' )
	    src = ptr + 1;


	//--- scan numbers until unknown character

	noPRINT(stderr,"SCAN: |%.*s| base=%d\n",(int)(eol-src),src,scan->num_base);

	for(;;)
	{
	    while ( src < eol && ( (u8)*src <= ' ' || *src == ',' ) )
		src++;
	    if ( src == eol
		|| (u8)TableNumbers[(u8)*src] >= scan->num_base
		&& !strchr(scan->num_allowed,*src) )
	    {
		break;
	    }

	    if ( dest >= destmax )
	    {
		size_t len = dest - destbuf;
		if ( len > max_size )
		     len = max_size;
		if (scan->dest)
		    AppendFastBuf(scan->dest,destbuf,len);
		else
		    fwrite(destbuf,1,len,scan->fout);
		max_size -= len;
		scan->written += len;
		dest = destbuf;
	    }

	    //u64 num;
	    //double d;
	    char *end;

	    switch(scan->format)
	    {
	     case XDUMPF_INT_2:
		scan->endian_func->wr16(dest,str2ll(src,&end,scan->num_base));
		dest += 2;
		break;

	     case XDUMPF_INT_3:
		scan->endian_func->wr24(dest,str2ll(src,&end,scan->num_base));
		dest += 3;
		break;

	     case XDUMPF_INT_4:
		scan->endian_func->wr32(dest,str2ll(src,&end,scan->num_base));
		dest += 4;
		break;

	     case XDUMPF_INT_5:
		scan->endian_func->wr40(dest,str2ll(src,&end,scan->num_base));
		dest += 5;
		break;

	     case XDUMPF_INT_6:
		scan->endian_func->wr48(dest,str2ll(src,&end,scan->num_base));
		dest += 6;
		break;

	     case XDUMPF_INT_7:
		scan->endian_func->wr56(dest,str2ll(src,&end,scan->num_base));
		dest += 7;
		break;

	     case XDUMPF_INT_8:
		scan->endian_func->wr64(dest,str2ll(src,&end,scan->num_base));
		dest += 8;
		break;

	     case XDUMPF_FLOAT:
		scan->endian_func->wrf4(dest,strtod(src,&end));
		dest += 4;
		break;

	     case XDUMPF_DOUBLE:
		scan->endian_func->wrf8(dest,strtod(src,&end));
		dest += 8;
		break;

	     default:
		*dest++ = str2ll(src,&end,scan->num_base);
		break;
	    }
	    if ( src == end )
		break;
	    src = end;
	}

	//--- prepare next line

	src = eol;
    }

    if ( dest > destbuf )
    {
	size_t len = dest - destbuf;
	if ( len > max_size )
	     len = max_size;
	if (scan->dest)
	    AppendFastBuf(scan->dest,destbuf,len);
	else
	    fwrite(destbuf,1,len,scan->fout);
	scan->written += len;
    }

    return src - source;
}

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
)
{
    if ( !data || !size )
	return 0;

    XScan_t scan;
    SetupXScan(&scan,xd,dest);
    return XScanHelper(&scan,data,data+size,term,max_size);
}

///////////////////////////////////////////////////////////////////////////////

s64 XScanByFile
(
    // returns <0 on error, or number of written bytes
    XDump_t		*xd,		// params; if NULL: use local version
    FastBuf_t		*dest,		// not NULL: append scanned data here
    FILE		*f,		// input file
    u64			max_size	// >0: limit output size
)
{
    if (!f)
	return 0;

    if (!max_size)
	max_size = M1(max_size);

    XScan_t scan;
    SetupXScan(&scan,xd,dest);

    char buf[0x4100];
    uint start = 0;

    while ( scan.written < max_size )
    {
	const uint max_read = ( sizeof(buf) - start ) / 0x1000 * 0x1000;

	size_t stat = fread(buf+start,1,max_read,f);
	if (!stat)
	    break;
	stat += start;

	const bool eof = feof(f) != 0;
	int scanstat = XScanHelper(&scan,buf,buf+stat,eof,max_size);
	if ( scanstat < 0 )
	    return scanstat;

	start = stat - scanstat;
	if (start)
	    memmove(buf,buf+scanstat,start);
	if (eof)
	    break;
    }

    if (start)
    {
	int scanstat = XScanHelper(&scan,buf,buf+start,true,max_size);
	if ( scanstat < 0 )
	    return scanstat;
    }

    return scan.written;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////
