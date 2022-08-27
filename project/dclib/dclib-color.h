
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

#ifndef SUPPORT_36_COLORS
  #define SUPPORT_36_COLORS 1
#endif

//-----------------------------------------------------------------------------
// [[ColorMode_t]]

typedef enum ColorMode_t
{
    COLMD_OFF		= -1,	// don't support colors
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

#define N_COLORSET_GREY    10
#define N_COLORSET_HL_LINE  4
#define N_COLORSET_ORDER    9

extern int termios_valid; // valid if >0

void ResetTermios(void);
bool EnableSingleCharInput(void);

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

extern uint opt_width, opt_max_width, opt_height, opt_max_height;
int ScanOptWidth	( ccp arg );
int ScanOptMaxWidth	( ccp arg );
int ScanOptHeight	( ccp arg );
int ScanOptMaxHeight	( ccp arg );

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

bool GetAutoTermSize(void);		// return true if changed
void EnableAutoTermSize(void);

///////////////////////////////////////////////////////////////////////////////
// [[good_term_width_t]]

typedef struct good_term_width_t
{
    int  def_width;
    int  min_width;
    int  max_width;
    int  good_width;
    bool setup_done;
}
good_term_width_t;

extern good_term_width_t good_term_width;

// If !gtw: Use good_term_width
int GetGoodTermWidth ( good_term_width_t *gtw, bool recalc );

///////////////////////////////////////////////////////////////////////////////

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
#define TERM_TEXT_MODE_BEG0	"\e[0;"		// begin of text mode sequence with reset
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
extern ccp TermClearDisplay;	// clear entire display

extern ccp TermSetWrap;		// enabled line wrapping
extern ccp TermResetWrap;	// disable line wrapping

extern ccp TermTextModeBeg;	// begin of text mode sequence
extern ccp TermTextModeBeg0;	// begin of text mode sequence with reset
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
	// [[new-color]]

	TTM_COL_SETUP		= TTM_RESET | TTM_BOLD    | TTM_BLUE,
	TTM_COL_RUN		= TTM_RESET | TTM_BOLD    | TTM_WHITE,
	TTM_COL_ABORT		= TTM_RESET | TTM_BOLD    | TTM_MAGENTA,
	TTM_COL_FINISH		= TTM_RESET | TTM_BOLD    | TTM_CYAN,
	TTM_COL_SCRIPT		= TTM_RESET | TTM_NO_BOLD | TTM_BLACK | TTM_BG_WHITE,
	TTM_COL_OPEN		= TTM_RESET | TTM_NO_BOLD | TTM_RED,
	TTM_COL_CLOSE		= TTM_RESET | TTM_NO_BOLD | TTM_RED,
	TTM_COL_FILE		= TTM_RESET | TTM_NO_BOLD | TTM_YELLOW,
	TTM_COL_JOB		= TTM_RESET | TTM_BOLD    | TTM_BLUE,
	TTM_COL_DEBUG		= TTM_RESET | TTM_BOLD    | TTM_RED,
	TTM_COL_LOG		= TTM_RESET | TTM_BOLD    | TTM_WHITE	| TTM_BG_BLUE,

	TTM_COL_MARK		= TTM_RESET | TTM_BOLD    | TTM_WHITE,
	TTM_COL_INFO		= TTM_RESET | TTM_BOLD    | TTM_CYAN,
	TTM_COL_HINT		= TTM_RESET | TTM_BOLD    | TTM_YELLOW,
	TTM_COL_WARN		= TTM_RESET | TTM_BOLD    | TTM_RED,
	TTM_COL_ERR		= TTM_RESET | TTM_BOLD    | TTM_MAGENTA,
	TTM_COL_BAD		= TTM_RESET | TTM_BOLD    | TTM_MAGENTA,

	TTM_COL_NAME		= TTM_RESET | TTM_NO_BOLD | TTM_CYAN,
	TTM_COL_OP		= TTM_RESET | TTM_BOLD    | TTM_WHITE,
	TTM_COL_VALUE		= TTM_RESET | TTM_NO_BOLD | TTM_GREEN,
	TTM_COL_SUCCESS		= TTM_RESET | TTM_BOLD    | TTM_GREEN,
	TTM_COL_ERROR		= TTM_RESET | TTM_NO_BOLD | TTM_BLACK	| TTM_BG_YELLOW,
	TTM_COL_FAIL		= TTM_RESET | TTM_NO_BOLD | TTM_WHITE	| TTM_BG_RED,
	TTM_COL_FAIL2		= TTM_RESET | TTM_BOLD    | TTM_WHITE	| TTM_BG_RED,
	TTM_COL_FATAL		= TTM_RESET | TTM_BOLD    | TTM_WHITE	| TTM_BG_MAGENTA,

	TTM_COL_SELECT		= TTM_RESET | TTM_NO_BOLD | TTM_YELLOW,
	TTM_COL_DIFFER		= TTM_RESET | TTM_BOLD    | TTM_YELLOW	| TTM_BG_BLUE,
	TTM_COL_HL_LINE0	= TTM_RESET | TTM_NO_BOLD | TTM_WHITE,
	TTM_COL_HL_LINE1	= TTM_RESET | TTM_BOLD    | TTM_CYAN,
	TTM_COL_HL_LINE2	= TTM_RESET | TTM_BOLD    | TTM_GREEN,
	TTM_COL_HL_LINE3	= TTM_RESET | TTM_BOLD    | TTM_MAGENTA,

	TTM_COL_ORDER0		= TTM_RESET | TTM_NO_BOLD | TTM_WHITE,
	TTM_COL_ORDER1		= TTM_RESET | TTM_BOLD    | TTM_BLUE,
	TTM_COL_ORDER2		= TTM_RESET | TTM_BOLD    | TTM_CYAN,
	TTM_COL_ORDER3		= TTM_RESET | TTM_BOLD    | TTM_GREEN,
	TTM_COL_ORDER4		= TTM_RESET | TTM_BOLD    | TTM_YELLOW,
	TTM_COL_ORDER5		= TTM_RESET | TTM_BOLD    | TTM_RED,
	TTM_COL_ORDER6		= TTM_RESET | TTM_BOLD    | TTM_MAGENTA,
	TTM_COL_ORDER7		= TTM_RESET | TTM_BOLD    | TTM_WHITE,
	TTM_COL_ORDER8		= TTM_RESET | TTM_BOLD    | TTM_WHITE,

	TTM_COL_STAT_LINE	= TTM_RESET | TTM_BOLD    | TTM_WHITE	| TTM_BG_BLUE,
	TTM_COL_WARN_LINE	= TTM_RESET | TTM_BOLD    | TTM_RED	| TTM_BG_BLUE,
	TTM_COL_PROC_LINE	= TTM_RESET | TTM_BOLD    | TTM_WHITE	| TTM_BG_MAGENTA,
	TTM_COL_CITE		= TTM_RESET | TTM_NO_BOLD | TTM_GREEN,
	TTM_COL_STATUS		= TTM_RESET | TTM_BOLD    | TTM_GREEN,
	TTM_COL_HIGHLIGHT	= TTM_RESET | TTM_BOLD    | TTM_YELLOW,
	TTM_COL_HIDE		= TTM_RESET | TTM_BOLD    | TTM_BLACK,

	TTM_COL_HEADING		= TTM_RESET | TTM_BOLD    | TTM_BLUE,
	TTM_COL_CAPTION		= TTM_RESET | TTM_BOLD    | TTM_CYAN,
	TTM_COL_SECTION		= TTM_RESET | TTM_BOLD    | TTM_GREEN,
	TTM_COL_SYNTAX		= TTM_RESET | TTM_BOLD    | TTM_WHITE,
	TTM_COL_CMD		= TTM_RESET | TTM_NO_BOLD | TTM_CYAN,
	TTM_COL_OPTION		= TTM_RESET | TTM_NO_BOLD | TTM_GREEN,
	TTM_COL_PARAM		= TTM_RESET | TTM_NO_BOLD | TTM_YELLOW,

	TTM_COL_ON		= TTM_RESET | TTM_BOLD    | TTM_GREEN,
	TTM_COL_OFF		= TTM_RESET | TTM_BOLD    | TTM_RED,
}
TermTextMode_t;

//-----------------------------------------------------------------------------
// [[TermColorIndex_t]]

typedef enum TermColorIndex_t
{
 #if SUPPORT_36_COLORS

    TCI_GRAY0,			// gray scale
    TCI_GRAY1,
    TCI_GRAY2,
    TCI_GRAY3,
    TCI_GRAY4,
    TCI_GRAY5,
    TCI_GRAY6,
    TCI_GRAY7,

    TCI_RED,			// normal colors in rainbow order
    TCI_RED_ORANGE,
    TCI_ORANGE,
    TCI_ORANGE_YELLOW,
    TCI_YELLOW,
    TCI_YELLOW_GREEN,
    TCI_GREEN,
    TCI_GREEN_CYAN,
    TCI_CYAN,
    TCI_CYAN_BLUE,
    TCI_BLUE,
    TCI_BLUE_MAGENTA,
    TCI_MAGENTA,
    TCI_MAGENTA_RED,

    TCI_B_RED,			// bold colors in rainbow order
    TCI_B_RED_ORANGE,
    TCI_B_ORANGE,
    TCI_B_ORANGE_YELLOW,
    TCI_B_YELLOW,
    TCI_B_YELLOW_GREEN,
    TCI_B_GREEN,
    TCI_B_GREEN_CYAN,
    TCI_B_CYAN,
    TCI_B_CYAN_BLUE,
    TCI_B_BLUE,
    TCI_B_BLUE_MAGENTA,
    TCI_B_MAGENTA,
    TCI_B_MAGENTA_RED,

    TCI__IGNORE,		// ignore this param, for GetColorMode()

    //--- alternative names

    TCI_BLACK		= TCI_GRAY0,
    TCI_DARKGRAY	= TCI_GRAY2,
    TCI_LIGHTGRAY	= TCI_GRAY5,
    TCI_WHITE		= TCI_GRAY7,

    //--- counters

    TCI__N	= TCI__IGNORE,	// number of supported colors
    TCI__N_FONT	= TCI_B_RED,	// number of recommended font colors
    TCI__N_BG	= TCI__N,	// number of recommended background colors
    TCI__N_GREY = TCI_RED,	// number of gray tones

 #else // !SUPPORT_36_COLORS

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
    TCI__N_GREY = TCI_RED,	// number of gray tones

 #endif // !SUPPORT_36_COLORS
}
TermColorIndex_t;

#if SUPPORT_36_COLORS
  extern const char ColorIndexName[TCI__N][16];
#else
  extern const char ColorIndexName[TCI__N][11];
#endif

extern const int Color18Index[18+1]; // 18 elements + -1 as terminator

//-----------------------------------------------------------------------------
// [[TermColorIndexEx_t]]

typedef enum TermColorIndexEx_t
{
    TCE_GREY0,
    TCE_GREY1,
    TCE_GREY2,
    TCE_GREY3,
    TCE_GREY4,
    TCE_GREY5,
    TCE_GREY6,
    TCE_GREY7,
    TCE_GREY8,
    TCE_GREY9,

    TCE_RED_MAGENTA,
    TCE_RED,
    TCE_RED_ORANGE,
     TCE_ORANGE_RED,
     TCE_ORANGE,
     TCE_ORANGE_YELLOW,
    TCE_YELLOW_ORANGE,
    TCE_YELLOW,
    TCE_YELLOW_GREEN,
     TCE_GREEN_YELLOW,
     TCE_GREEN,
     TCE_GREEN_CYAN,
    TCE_CYAN_GREEN,
    TCE_CYAN,
    TCE_CYAN_BLUE,
     TCE_BLUE_CYAN,
     TCE_BLUE,
     TCE_BLUE_MAGENTA,
    TCE_MAGENTA_BLUE,
    TCE_MAGENTA,
    TCE_MAGENTA_RED,

    TCE_B_RED_MAGENTA,
    TCE_B_RED,
    TCE_B_RED_ORANGE,
     TCE_B_ORANGE_RED,
     TCE_B_ORANGE,
     TCE_B_ORANGE_YELLOW,
    TCE_B_YELLOW_ORANGE,
    TCE_B_YELLOW,
    TCE_B_YELLOW_GREEN,
     TCE_B_GREEN_YELLOW,
     TCE_B_GREEN,
     TCE_B_GREEN_CYAN,
    TCE_B_CYAN_GREEN,
    TCE_B_CYAN,
    TCE_B_CYAN_BLUE,
     TCE_B_BLUE_CYAN,
     TCE_B_BLUE,
     TCE_B_BLUE_MAGENTA,
    TCE_B_MAGENTA_BLUE,
    TCE_B_MAGENTA,
    TCE_B_MAGENTA_RED,

    TCE__N,

    TCE_BLACK = TCE_GREY0,
    TCE_WHITE = TCE_GREY9,
}
TermColorIndexEx_t;

//-----------------------------------------------------------------------------
// [[TermColorWarn_t]]

typedef enum TermColorWarn_t
{
    TCW_MARK,
    TCW_INFO,
    TCW_HINT,
    TCW_WARN,
    TCW_ERR,
    TCW_BAD,
    TCW__N,
}
TermColorWarn_t;

//-----------------------------------------------------------------------------
// [[TermColorId_t]]

typedef struct TermColorId_t
{
    union
    {
	u8 col[TCE__N];
	u8 gray[N_COLORSET_GREY];
    };
}
TermColorId_t;

// values 0x00-0x07 for standard, 0x10-0x17 for bold
extern const TermColorId_t color8id;

// values for 38;5;x or 48;5;x
extern const TermColorId_t color256id;

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
    // [[new-color]]

    ccp reset;	    // always first color!!

    ccp setup;
    ccp run;
    ccp abort;
    ccp finish;
    ccp script;
    ccp open;
    ccp close;
    ccp file;
    ccp job;
    ccp debug;
    ccp log;

    union
    {
	ccp warn_level[TCW__N];
	struct
	{
	    ccp mark;
	    ccp info;
	    ccp hint;
	    ccp warn;
	    ccp err;
	    ccp bad;
	};
    };

    ccp name;
    ccp op;
    ccp value;
    ccp success;
    ccp error;
    ccp fail;
    ccp fail2;
    ccp fatal;

    ccp hl_line[N_COLORSET_HL_LINE];
    ccp order[N_COLORSET_ORDER];
    ccp select;
    ccp differ;
    ccp stat_line;
    ccp warn_line;
    ccp proc_line;
    ccp cite;
    ccp status;
    ccp highlight;
    ccp hide;

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

    ccp grey[N_COLORSET_GREY];

    ccp	red_magenta;
    ccp	red;
    ccp	red_orange;
    ccp	orange_red;
    ccp	orange;
    ccp	orange_yellow;
    ccp	yellow_orange;
    ccp	yellow;
    ccp	yellow_green;
    ccp	green_yellow;
    ccp	green;
    ccp	green_cyan;
    ccp	cyan_green;
    ccp	cyan;
    ccp	cyan_blue;
    ccp	blue_cyan;
    ccp	blue;
    ccp	blue_magenta;
    ccp	magenta_blue;
    ccp	magenta;
    ccp	magenta_red;

    ccp	b_red_magenta;
    ccp	b_red;
    ccp	b_red_orange;
    ccp	b_orange_red;
    ccp	b_orange;
    ccp	b_orange_yellow;
    ccp	b_yellow_orange;
    ccp	b_yellow;
    ccp	b_yellow_green;
    ccp	b_green_yellow;
    ccp	b_green;
    ccp	b_green_cyan;
    ccp	b_cyan_green;
    ccp	b_cyan;
    ccp	b_cyan_blue;
    ccp	b_blue_cyan;
    ccp	b_blue;
    ccp	b_blue_magenta;
    ccp	b_magenta_blue;
    ccp	b_magenta;
    ccp	b_magenta_red;

    ccp matrix[TCI__N_FONT][TCI__N_BG];


    //--- line support (mutable)

    ccp line_begin;	// use this at beginning of line
    ccp line_reset;	// use this in line to reset line color
    ccp line_end;	// use this at end of line
}
ColorSet_t;

//-----------------------------------------------------------------------------
// [[ColorSelect_t]]

typedef enum ColorSelect_t
{
    COLSEL_COLOR	= 0x01,  // standard colors
    COLSEL_B_COLOR	= 0x02,  // bold colors
    COLSEL_GREY		= 0x04,  // grey scale

    COLSEL_NAME1	= 0x08,  // semantic color names, part 1
    COLSEL_NAME2	= 0x10,  // semantic color names, part 2
    COLSEL_NAME		= 0x18,  // semantic color names, both parts

    COLSEL_F_ALT	= 0x20,

    COLSEL_M_COLOR	= COLSEL_COLOR|COLSEL_B_COLOR,
    COLSEL_M_NONAME	= COLSEL_COLOR|COLSEL_B_COLOR|COLSEL_GREY,
    COLSEL_M_MODE	= 0x0f,
    COLSEL_M_ALL	= 0x1f,
}
ColorSelect_t;

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
    ccp			col_name,	// name of color
    ccp			col_string	// escape string for the color
);

//-----------------------------------------------------------------------------

const ColorSet_t * GetColorSet ( ColorMode_t col_mode );
const ColorSet_t * GetColorSet0(void);
const ColorSet_t * GetColorSet8(void);
const ColorSet_t * GetColorSet256(void);
const ColorSet_t * GetColorSetAuto ( bool force_on );

const ColorSet_t * GetFileColorSet ( FILE *f );
bool SetupColorSet ( ColorSet_t *cc, FILE *f );
int GetColorOffsetByName ( ccp name ); // returns -1 if not found

// returns EmptyString[] if not found
ccp GetColorByName ( const ColorSet_t *colset, ccp name );
ccp GetColorByOffsetOrName ( const ColorSet_t *colset, int * offset, ccp name );

extern const char EmptyString[];
static inline ccp GetColorByOffset ( const ColorSet_t *colset, int offset )
	{ return offset > 0 ? *(ccp*)((u8*)colset+offset) : EmptyString; }

static inline ccp GetColorByValidOffset ( const ColorSet_t *colset, int offset )
	{ return *(ccp*)((u8*)colset+offset); }

// both will change line_col & line_reset
void ResetLineColor ( const ColorSet_t *colset );
void SetLineColor ( const ColorSet_t *colset, int level );

void PrintColorSet
(
    FILE		*f,		// valid output file
    int			indent,		// indention of output
    const ColorSet_t	*cs,		// valid color set; if NULL: use std colors
    ColorSelect_t	select,		// select color groups
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
    ColorSelect_t	select,		// select color groups
    int			multi_assign	// >0: Allow A=B="...";  >1: reorder records too
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			color helpers			///////////////
///////////////////////////////////////////////////////////////////////////////

extern u32 ColorTab_M0_M15[16]; // first 16 colors of "\e[m"

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
    bool		print_alt;	// print alternative names
    bool		allow_sep;	// allow separator lines

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
void ViewColors18	( ColorView_t *cv );
void ViewColorsDC	( ColorView_t *cv, bool optimized );
void ViewColors256	( ColorView_t *cv );
void ViewColorsPredef	( ColorView_t *cv, ColorSelect_t select );

#define VIEW_COLOR_PAGES 10
void ViewColorsPage	( ColorView_t *cv, uint page );

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
//	Special output for catcable messages. Initialized by SetupStdMsg()
//	and set to the first not NULL of stdout, stderr and stdlog.
//
//   stdwrn, colwrn:
//	Output file of PrintError() functions.
//	Initialized to a copy of stderr.
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

void CloseStdLog(void);
void SetupStdMsg(void);

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
