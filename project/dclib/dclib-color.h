
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

#ifndef DCLIB_COLOR_H
#define DCLIB_COLOR_H 1

#define _GNU_SOURCE 1
#include <string.h>

#include "dclib-types.h"
#include "dclib-debug.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    color modes			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ColorMode_t]]

typedef enum ColorMode_t
{
    COLMD_OFF		= -1,	// don't support clors
    COLMD_AUTO		=  0,	// auto detect
    COLMD_ON,			// on, auto select mode
    COLMD_8_COLORS,		// force 8 color mode
    COLMD_256_COLORS,		// force 256 color mode
}
ColorMode_t;

//-----------------------------------------------------------------------------

struct KeywordTab_t;
extern const struct KeywordTab_t color_mode_keywords[];
extern ColorMode_t opt_colorize; // <0:no color, 0:auto, >0:force color

int ScanOptColorize ( ColorMode_t *opt, ccp arg, ccp err_prefix );
ccp GetColorModeName ( ColorMode_t col_mode, ccp res_not_found );

//-----------------------------------------------------------------------------
// returns COLMD_OFF | COLMD_8_COLORS | COLMD_256_COLORS

ColorMode_t NormalizeColorMode ( ColorMode_t col_mode, int n_colors );

ColorMode_t NormalizeColorModeByTermName
(
    ColorMode_t		col_mode,	// predefind mode
    ccp			term_name	// find "256color" in terminal name.
					// if NULL: use getenv("TERM")
					// assume 8 or 256 colors!
);

struct TerminalInfo_t;
ColorMode_t NormalizeColorModeByTerm
(
    ColorMode_t		col_mode,	// predefind mode
    const struct
     TerminalInfo_t	*term		// if NULL: use getenv("TERM") to find terminal
);

// iteration
ColorMode_t GetNextColorMode ( ColorMode_t col_mode );
ColorMode_t GetPrevColorMode ( ColorMode_t col_mode );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			terminal & colors		///////////////
///////////////////////////////////////////////////////////////////////////////

extern int termios_valid; // valid if >0

void ResetTermios();
bool EnableSingleCharInput();

///////////////////////////////////////////////////////////////////////////////

// Sequence counter for output to stdout (and stderr).
// Functions shall increment it on output. so that other functions
// know about an destroid screen.

extern uint stdout_seq_count;

//-----------------------------------------------------------------------------
// [[term_size_t]]

typedef struct term_size_t
{
    uint width;
    uint height;
}
term_size_t;

extern uint opt_width, opt_height;
int ScanOptWidth ( ccp arg );
int ScanOptHeight ( ccp arg );

term_size_t GetTermSize   ( int default_width, int default_height );
term_size_t GetTermSizeFD ( int fd, int default_width, int default_height );

int GetTermWidth ( int default_value, int min_value );
int GetTermWidthFD ( int fd, int default_value, int min_value );

int GetTermHeight ( int default_value, int min_value );
int GetTermHeightFD ( int fd, int default_value, int min_value );

//-----------------------------------

extern term_size_t auto_term_size;	// default = {80,25};
extern uint auto_term_resized;		// incremented for each resize, init by 0
extern uint auto_term_size_dirty;	// >0: SIGWINCH received, auto_term_size is dirty

bool GetAutoTermSize();			// return true if changed
void EnableAutoTermSize();

//-----------------------------------------------------------------------------

//----- some basic terminal codes

#define TERM_CURSOR_UP		"\e[A"		// move cursor up
#define TERM_CURSOR_DOWN	"\e[B"		// move cursor down
#define TERM_CURSOR_RIGHT	"\e[C"		// move cursor right
#define TERM_CURSOR_LEFT	"\e[D"		// move cursor left

#define TERM_CURSOR_UP_N	"\e[%dA"	// move cursor N* up
#define TERM_CURSOR_DOWN_N	"\e[%dB"	// move cursor N* down
#define TERM_CURSOR_RIGHT_N	"\e[%dC"	// move cursor N* right
#define TERM_CURSOR_LEFT_N	"\e[%dD"	// move cursor N* left

#define TERM_CLEAR_EOL		"\e[K"		// clear from cursor to end of line
#define TERM_CLEAR_BOL		"\e[1K"		// clear from beginning of line to cursor
#define TERM_CLEAR_LINE		"\e[2K"		// clear entire line

#define TERM_CLEAR_EOD		"\e[J"		// clear from cursor to end of display
#define TERM_CLEAR_BOD		"\e[1J"		// clear from beginning of display to cursor
#define TERM_CLEAR_DISPLAY	"\e[2J"		// clear Entire display

#define TERM_SET_WRAP		"\e[?7h"	// enabled line wrapping
#define TERM_RESET_WRAP		"\e[?7l"	// disable line wrapping

#define TERM_TEXT_MODE_BEG	"\e["		// begin of text mode sequence
#define TERM_TEXT_MODE_END	"m"		// end of text mode sequence
#define TERM_TEXT_MODE_RESET	"\e[m"		// reset text mode

// pointers are initialized with codes above

extern ccp TermCursorUp;	// move cursor up
extern ccp TermCursorDown;	// move cursor down
extern ccp TermCursorRight;	// move cursor right
extern ccp TermCursorLeft;	// move cursor left

extern ccp TermCursorUpN;	// move cursor N* up    (format string with 1 '%d')
extern ccp TermCursorDownN;	// move cursor N* down  (format string with 1 '%d')
extern ccp TermCursorRightN;	// move cursor N* right (format string with 1 '%d')
extern ccp TermCursorLeftN;	// move cursor N* left  (format string with 1 '%d')

extern ccp TermClearEOL;	// clear from cursor to end of line
extern ccp TermClearBOL;	// clear from beginning of line to cursor
extern ccp TermClearLine;	// clear entire line

extern ccp TermClearEOD;	// clear from cursor to end of display
extern ccp TermClearBOD;	// clear from beginning of display to cursor
extern ccp TermClearDisplay;	// clear Entire display

extern ccp TermSetWrap;		// enabled line wrapping
extern ccp TermResetWrap;	// disable line wrapping

extern ccp TermTextModeBeg;	// begin of text mode sequence
extern ccp TermTextModeEnd;	// end of text mode sequence
extern ccp TermTextModeReset;	// reset text mode

//-----------------------------------------------------------------------------
// [[TermTextMode_t]]

typedef enum TermTextMode_t
{
	//--- standard fg colors: \e[30..37m

	TTM_BLACK		= 0x08,
	TTM_RED			= 0x09,
	TTM_GREEN		= 0x0a,
	TTM_YELLOW		= 0x0b,
	TTM_BLUE		= 0x0c,
	TTM_MAGENTA		= 0x0d,
	TTM_CYAN		= 0x0e,
	TTM_WHITE		= 0x0f,


	//--- standard bg colors: \e[40..47m

	TTM_BG_BLACK		= 0x80,
	TTM_BG_RED		= 0x90,
	TTM_BG_GREEN		= 0xa0,
	TTM_BG_YELLOW		= 0xb0,
	TTM_BG_BLUE		= 0xc0,
	TTM_BG_MAGENTA		= 0xd0,
	TTM_BG_CYAN		= 0xe0,
	TTM_BG_WHITE		= 0xf0,

	//--- grayscale: \e[48;5;<VAL>m

	TTM_GRAY0		= 0x04,	// black
	TTM_GRAY1		= 0x05, // dark gray
	TTM_GRAY2		= 0x06, // light gray
	TTM_GRAY3		= 0x07, // real white

	TTM_BG_GRAY0		= 0x40,	// black
	TTM_BG_GRAY1		= 0x50, // dark gray
	TTM_BG_GRAY2		= 0x60, // light gray
	TTM_BG_GRAY3		= 0x70, // real white

	//--- misc

	TTM_RESET		= 0x0100,
	TTM_BOLD		= 0x0200,
	TTM_NO_BOLD		= 0x0400,
	TTM_UL			= 0x0800,
	TTM_NO_UL		= 0x1000,

	//--- flags and masks ans shifts

	TTM_F_COLOR		= 0x08,
	TTM_M_COLOR		= 0x07,
	TTM_F_GRAYSCALE		= 0x04,	// only valid if !TTM_F_COLOR
	TTM_M_GRAYSCALE		= 0x03,

	TTM_F_BG_COLOR		= 0x80,
	TTM_M_BG_COLOR		= 0x70,
	TTM_F_BG_GRAYSCALE	= 0x40,	// only valid if !TTM_F_BG_COLOR
	TTM_M_BG_GRAYSCALE	= 0x30,

	TTM_SHIFT_BG		=    4,


	//--- and her are some recommendations for unique colors
	//--- these Colors are also used to setup 'ColorSet_t'

	TTM_COL_SETUP		= TTM_BOLD	| TTM_BLUE	| TTM_BG_BLACK,
	TTM_COL_FINISH		= TTM_BOLD	| TTM_CYAN	| TTM_BG_BLACK,
	TTM_COL_OPEN		= TTM_NO_BOLD	| TTM_RED	| TTM_BG_BLACK,
	TTM_COL_CLOSE		= TTM_NO_BOLD	| TTM_RED	| TTM_BG_BLACK,
	TTM_COL_FILE		= TTM_NO_BOLD	| TTM_YELLOW	| TTM_BG_BLACK,
	TTM_COL_JOB		= TTM_BOLD	| TTM_BLUE	| TTM_BG_BLACK,

	TTM_COL_INFO		= TTM_BOLD	| TTM_CYAN	| TTM_BG_BLACK,
	TTM_COL_HINT		= TTM_BOLD	| TTM_YELLOW	| TTM_BG_BLACK,
	TTM_COL_WARN		= TTM_BOLD	| TTM_RED	| TTM_BG_BLACK,
	TTM_COL_DEBUG		= TTM_BOLD	| TTM_RED	| TTM_BG_BLACK,

	TTM_COL_NAME		= TTM_NO_BOLD	| TTM_CYAN	| TTM_BG_BLACK,
	TTM_COL_VALUE		= TTM_NO_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_SUCCESS		= TTM_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_FAIL		= TTM_NO_BOLD	| TTM_WHITE	|  TTM_BG_RED,
	TTM_COL_FAIL2		= TTM_BOLD	| TTM_WHITE	|  TTM_BG_RED,
	TTM_COL_MARK		= TTM_BOLD	| TTM_WHITE	| TTM_BG_BLACK,
	TTM_COL_BAD		= TTM_BOLD	| TTM_MAGENTA	| TTM_BG_BLACK,

	TTM_COL_SELECT		= TTM_NO_BOLD	| TTM_YELLOW	| TTM_BG_BLACK,
	TTM_COL_DIFFER		= TTM_BOLD	| TTM_YELLOW	|  TTM_BG_BLUE,
	TTM_COL_STAT_LINE	= TTM_BOLD	| TTM_WHITE	|  TTM_BG_BLUE,
	TTM_COL_WARN_LINE	= TTM_BOLD	| TTM_RED	|  TTM_BG_BLUE,
	TTM_COL_PROC_LINE	= TTM_BOLD	| TTM_WHITE	|  TTM_BG_MAGENTA,
	TTM_COL_CITE		= TTM_NO_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_STATUS		= TTM_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_HIGHLIGHT	= TTM_BOLD	| TTM_YELLOW	| TTM_BG_BLACK,

	TTM_COL_HEADING		= TTM_BOLD	| TTM_BLUE	| TTM_BG_BLACK,
	TTM_COL_CAPTION		= TTM_BOLD	| TTM_CYAN	| TTM_BG_BLACK,
	TTM_COL_SECTION		= TTM_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_SYNTAX		= TTM_BOLD	| TTM_WHITE	| TTM_BG_BLACK,
	TTM_COL_CMD		= TTM_NO_BOLD	| TTM_CYAN	| TTM_BG_BLACK,
	TTM_COL_OPTION		= TTM_NO_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_PARAM		= TTM_NO_BOLD	| TTM_YELLOW	| TTM_BG_BLACK,

	TTM_COL_ON		= TTM_BOLD	| TTM_GREEN	| TTM_BG_BLACK,
	TTM_COL_OFF		= TTM_BOLD	| TTM_RED	| TTM_BG_BLACK,
}
TermTextMode_t;

//-----------------------------------------------------------------------------
// [[TermColorIndex_t]]

typedef enum TermColorIndex_t
{
    TCI_BLACK,			// gray scale
    TCI_DARKGRAY,
    TCI_LIGHTGRAY,
    TCI_WHITE,

    TCI_RED,			// normal colors in rainbow order
    TCI_ORANGE,
    TCI_YELLOW,
    TCI_GREEN,
    TCI_CYAN,
    TCI_BLUE,
    TCI_MAGENTA,

    TCI_B_RED,			// bold colors in rainbow order
    TCI_B_ORANGE,
    TCI_B_YELLOW,
    TCI_B_GREEN,
    TCI_B_CYAN,
    TCI_B_BLUE,
    TCI_B_MAGENTA,

    TCI__IGNORE,		// ignore this param, for GetColorMode()

    //--- counters

    TCI__N	= TCI__IGNORE,	// number of supported colors
    TCI__N_FONT	= TCI_B_RED,	// number of recommended font colors
    TCI__N_BG	= TCI__N,	// number of recommended background colors
}
TermColorIndex_t;

extern const char ColorIndexName[TCI__N][11];

//-----------------------------------------------------------------------------
// [[ColorSet_t]]

typedef struct ColorSet_t
{
    //--- header

    ColorMode_t	col_mode;
    u8		colorize;
    u16		n_colors;

    //--- some strings for formatted output if colors active

    ccp space;	    // pointer to 'EmptyString' or to 'Space200';
    ccp tab;	    // pointer to 'EmptyString' or to 'TAB20';
    ccp lf;	    // pointer to 'EmptyString' or to 'LF20';

    //--- data, always begins with 'reset'

    ccp reset;

    ccp setup;
    ccp finish;
    ccp open;
    ccp close;
    ccp file;
    ccp job;

    ccp info;
    ccp hint;
    ccp warn;
    ccp debug;

    ccp name;
    ccp value;
    ccp success;
    ccp fail;
    ccp fail2;
    ccp mark;
    ccp bad;

    ccp select;
    ccp differ;
    ccp stat_line;
    ccp warn_line;
    ccp proc_line;
    ccp cite;
    ccp status;
    ccp highlight;

    ccp heading;
    ccp caption;
    ccp section;
    ccp syntax;
    ccp cmd;
    ccp option;
    ccp param;

    ccp on;
    ccp off;

    ccp	black;
    ccp	b_black;
    ccp	white;
    ccp	b_white;

    ccp	red;
    ccp	red_orange;
    ccp	orange;
    ccp	orange_yellow;
    ccp	yellow;
    ccp	yellow_green;
    ccp	green;
    ccp	green_cyan;
    ccp	cyan;
    ccp	cyan_blue;
    ccp	blue;
    ccp	blue_magenta;
    ccp	magenta;
    ccp	magenta_red;

    ccp	b_red;
    ccp	b_red_orange;
    ccp	b_orange;
    ccp	b_orange_yellow;
    ccp	b_yellow;
    ccp	b_yellow_green;
    ccp	b_green;
    ccp	b_green_cyan;
    ccp	b_cyan;
    ccp	b_cyan_blue;
    ccp	b_blue;
    ccp	b_blue_magenta;
    ccp	b_magenta;
    ccp	b_magenta_red;

    ccp matrix[TCI__N_FONT][TCI__N_BG];
}
ColorSet_t;

//-----------------------------------------------------------------------------

ccp GetTextMode
(
    // returns a pointer to an internal circulary buffer (not GetCircBuf())
    ColorMode_t		col_mode,	// if < COLMD_ON: return an empty string
    TermTextMode_t	mode		// mode for text generation
);

//-----------------------------------------------------------------------------
// [[GetColorOption_t]]

typedef enum GetColorOption_t
{
    GCM_ALT	= 0x01,  // use recommended font color if font+bg is not readable
    GCM_SHORT	= 0x02,  // allow alternative short codes (codes 30-37 and 40-47)
}
GetColorOption_t;

ccp GetColorMode
(
    // returns a pointer to an internal circulary buffer (not GetCircBuf())

    ColorMode_t		col_mode,	// if < COLMD_ON: return an empty string
    TermColorIndex_t	font_index,	// index of font color
    TermColorIndex_t	bg_index,	// index of background color
    GetColorOption_t	option		// execution options
);

//-----------------------------------------------------------------------------

typedef void (*PrintColorFunc)
(
    // On start of group: col_name := NULL, col_string := NULL

    FILE		*f,		// valid output file
    int			indent,		// normalized indention of output
    const ColorSet_t	*cs,		// valid color set, never NULL
    uint		mode,		// output mode of PrintColorSetHelper()
    ccp			col_name,	// name of color
    ccp			col_string	// escape string for the color
);

//-----------------------------------------------------------------------------

const ColorSet_t * GetColorSet ( ColorMode_t col_mode );
const ColorSet_t * GetColorSet0();
const ColorSet_t * GetColorSet8();
const ColorSet_t * GetColorSet256();
const ColorSet_t * GetColorSetAuto ( bool force_on );

const ColorSet_t * GetFileColorSet ( FILE *f );
bool SetupColorSet ( ColorSet_t *cc, FILE *f );
ccp GetColorByName ( const ColorSet_t *colset, ccp name );

void PrintColorSet
(
    FILE		*f,		// valid output file
    int			indent,		// indention of output
    const ColorSet_t	*cs		// valid color set; if NULL: use std colors
);

void PrintColorSetEx
(
    FILE		*f,		// valid output file
    int			indent,		// indention of output
    const ColorSet_t	*cs,		// valid color set; if NULL: use std colors
    uint		mode,		// output mode => see PrintColorSetHelper()
    uint		format		// output format
					//   0: colored list
					//   1: shell definitions
);

void PrintColorSetHelper
(
    FILE		*f,		// valid output file
    int			indent,		// indention of output
    const ColorSet_t	*cs,		// valid color set; if NULL: use std colors
    PrintColorFunc	func,		// output function, never NULL
    uint		mode		// output mode (bit field, NULL=default):
					//   1: print normal colors (e.g. RED)
					//   2: print bold colors (e.g. B_RED)
					//   4: print background (e.g. BLUE_RED)
					//   8: print color names (e.g. HIGHLIGHT)
					//      16: include alternative names too (e.g. HL)
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			color helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

u32 ColorTab_M0_M15[16]; // first 16 colors of "\e[m"

// return a m256 index
u8 ConvertColorRGB3ToM256 ( u8 r, u8 g, u8 b );
u8 ConvertColorRGBToM256  ( u32 rgb );

u32 ConvertColorM256ToRGB ( u8 m256 );

static inline uint SingleColorToM6 ( u8 col )
	{ return col > 114 ? ( col - 35 ) / 40 : col > 47; }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    ViewColor*()		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ColorView_t]] : parameters to view color sets

typedef struct ColorView_t
{
    FILE		*f;		// valid output file
    int			indent;		// indention of output

    const struct TerminalInfo_t
			*term;		// not NULL: us it to find 'colset'
    ColorMode_t		col_mode;	// color mode -> NormalizeColorMode()
    const ColorSet_t	*colset;	// not NULL: ignore 'col_mode'

    TermColorIndex_t	std_col;	// standard bg or fg color, TCI__IGNORE allowed
    int			order;		// 0|'r' | 1|'g' | 2|'b'
    GetColorOption_t	col_option;	// color option
    uint		mode;		// function specific mode (modulo 60)

    void		*user_ptr;	// any pointer
    int			user_int;	// any number or id
}
ColorView_t;

///////////////////////////////////////////////////////////////////////////////

static inline void InitializeColorView ( ColorView_t *cv )
	{ DASSERT(cv); memset(cv,0,sizeof(*cv)); cv->std_col = TCI__IGNORE; }

void SetupColorView ( ColorView_t *cv );
void SetupColorViewMode ( ColorView_t *cv, ColorMode_t col_mode );

void ViewColorsAttrib8	( ColorView_t *cv );
void ViewColorsCombi8	( ColorView_t *cv );
void ViewColorsDC	( ColorView_t *cv );
void ViewColors256	( ColorView_t *cv );
void ViewColorsPredef	( ColorView_t *cv, uint mode ); // mode: 1=name, 2=colors

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// legacy functions (deprecated) ==> wrapper for ViewColors*()

void PrintTextModes
(
    // => ViewColorsAttrib8() & ViewColorsCombi8()

    FILE		*f,		// valid output file
    int			indent,		// indention of output
    ColorMode_t		col_mode	// color mode -> NormalizeColorMode()
);

void PrintColorModes
(
    FILE		*f,		// valid output file
    int			indent,		// indention of output
    ColorMode_t		col_mode,	// color mode -> NormalizeColorMode()
    GetColorOption_t	option		// execution options (GCM_ALT ignored)
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    std*, col*			///////////////
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// We have 5 standard output files
//
//   stdout, colout:
//	Standard output like usual.
//
//   stderr, colerr:
//	Standard error output like usual. It is usually not redirected by
//	RedirectStdFiles() and points always to its original.
//
//   stdlog, collog:
//	Logging output, may be NULL to tell: no logging.
//	RedirectStdFiles() don't cange it, if it is NULL.
//
//   stdmsg, colmsg:
//	Special output for catcable messages. Initalized by SetupStdMsg()
//	and set to the first not NULL of stdout, stderr and stdlog.
//
//   stdwrn, colwrn:
//	Output file of PrintError() functions.
//	Initalized to a copy of stderr.
//
////////////////////////////////////////////////////////////////////////////


extern FILE *stdlog;		// NULL or logfile
extern bool close_stdlog;	// true: close file behing 'stdlog'

extern FILE *stdmsg;		// 'stdlog' or 'stdout' or 'stderr' or NULL
extern FILE *stdwrn;		// used by PrintError(), initialized with 'stderr'

extern ColorMode_t colorize_stdout;	// <0:no color, 0:auto, >0:force color
extern ColorMode_t colorize_stderr;
extern ColorMode_t colorize_stdlog;
extern ColorMode_t colorize_stdmsg;
extern ColorMode_t colorize_stdwrn;

extern const ColorSet_t *colout;	// color set for 'stdout'
extern const ColorSet_t *colerr;	// color set for 'stderr'
extern const ColorSet_t *collog;	// color set for 'stdlog'
extern const ColorSet_t *colmsg;	// color set for 'stdmsg'
extern const ColorSet_t *colwrn;	// color set for 'stdwrn'

//-----------------------------------------------------------------------------

ColorMode_t GetFileColorized ( FILE *f );
static inline bool IsFileColorized ( FILE *f )
	{ return GetFileColorized(f) >= COLMD_ON; }

enumError OpenStdLog
(
    ccp fname,		// If NULL or empty: don't open and use 'fallback'
			// If first char == '+': open file in append mode
    FILE *fallback	// Fall back to this file, if no file name given
			// If NULL: just clode 'stdlog'.
);

void CloseStdLog();
void SetupStdMsg();

//-----------------------------------------------------------------------------
// [[SavedStdFiles_t]]

typedef struct SavedStdFiles_t
{
    //-- 5 output streams

    FILE *std_out;
    FILE *std_err;
    FILE *std_log;
    FILE *std_msg;
    FILE *std_wrn;

    //-- 5 related color sets

    const ColorSet_t *col_out;
    const ColorSet_t *col_err;
    const ColorSet_t *col_log;
    const ColorSet_t *col_msg;
    const ColorSet_t *col_wrn;

    //--- misc

    uint stdout_seq_count;

    //--- support for CatchStdFiles()

    FILE	*f;		// NULL or result of open_memstream()
    char	*data;		// data pointer
    size_t	size;		// size of 'data'
    uint	marker;		// marker, free member, e.g. grow_buffer->used
}
SavedStdFiles_t;

void SaveStdFiles ( SavedStdFiles_t *ssf );
void RestoreStdFiles ( SavedStdFiles_t *ssf );

void RedirectStdFiles
(
    SavedStdFiles_t	*ssf,	// not NULL: save old output here
    FILE		*f,	// use this as out,err,log and msg, never NULL
    const ColorSet_t	*colset,// new colset; if NULL: use GetFileColorSet(f)
    bool		err_too	// true: redirect stderr too
);

#ifndef __APPLE__
 enumError CatchStdFiles
 (
    SavedStdFiles_t	*ssf,	// save old output and data here, never NULL
    const ColorSet_t	*colset	// new colset; if NULL: no color support
 );
#endif

// call XFREE(ssf->data) after 'data' is used.
void TermCatchStdFiles ( SavedStdFiles_t *ssf );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_COLOR_H
