
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

#ifndef DCLIB_OPTION_H
#define DCLIB_OPTION_H 1

#include "dclib-basics.h"
struct TCPStream_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    GenericOpt_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[GenericOptParm_t]]

typedef struct GenericOptParm_t
{
    u32		id;		// id of the option

    u8		arg;		// 0: no arguments
				// 1: always argument,
				// 2: optional argument
				// 3: short-opt: no arg / long-opt: optional arg

    ccp		name;		// pipe separated option names
}
GenericOptParm_t;

//-----------------------------------------------------------------------------
// [[GenericOpt_t]]

struct option;

typedef struct GenericOpt_t
{
    //--- short options

    char	sopt[0x100];	// short options string
    u32		sopt_id[0x80];

    //--- long options

    struct option *lopt;	// long options list
    uint	lopt_used;	// number of used 'lopt' elements
    uint	lopt_size;	// number of alloced 'lopt' elements

    //--- string pool

    MemPool_t	str_pool;	// pool for option names
}
GenericOpt_t;

//-----------------------------------------------------------------------------

void InitializeGenericOpt ( GenericOpt_t *go, char first_char );
void ResetGenericOpt ( GenericOpt_t *go );

void AddGenericOpt
(
    GenericOpt_t	*go,		// data structure
    const GenericOptParm_t *gp,		// list to add, list end on name==NULL
    bool		rm_minus	// remove '-' of options for alternatives
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    GOPT_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[GOPT_t]]

typedef enum GOPT_t
{
	GOPT_RESERVED_BEG	= 0x01000000,	// begin of reserved range
	GOPT_RESERVED_END	= 0x02000000,	// end of reserved range

	GOPT_M_INDEX		= 0x000000ff,	// mask to get the index
	GOPT_N_PARAM		=         60,	// max used index+1 = num of params
	 GOPT_M_VALUE		= 0x0003ff00,	// mask for GOPT_T_PLUS|GOPT_T_MINUS
	 GOPT_S_VALUE		=	   8,	// value is shifted by this num
	GOPT_M_TYPE		= 0x00fc0000,	// mask to isolate option type
	GOPT_I_TYPE		= 0x00040000,	// increment between types
	GOPT_S_TYPE		=	  18,	// type is shifted by this num


	//--- parameters by index (sn=signed num, un=unsigned num)

	GOPT_IDX_NULL = 0,	// type undefined

	GOPT_IDX_INC,		// increment 'sn', ptype := GOPT_T_INC
	GOPT_IDX_INC0,		// increment 'sn', ptype := GOPT_T_INC
	GOPT_IDX_DEC,		// decrement 'sn', ptype := GOPT_T_INC
	GOPT_IDX_DEC0,		// decrement 'sn', ptype := GOPT_T_INC

	GOPT_IDX_SETP,		// set 'sn' to value 0..0x3ff,   ptype := GOPT_IDX_SINT
	GOPT_IDX_SETM,		// set 'sn' to value -0..-0x3ff, ptype := GOPT_IDX_SINT
	GOPT_IDX_SETP64,	// set 'sn64' to value 0..0x3ff,   ptype := GOPT_IDX_S64
	GOPT_IDX_SETM64,	// set 'sn64' to value -0..-0x3ff, ptype := GOPT_IDX_S64

	GOPT_IDX_OR,		// set 'un' |= value 0..0x3ff, ptype := GOPT_IDX_UINT
	GOPT_IDX_SETBIT,	// set bit VALUE in 'un', ptype := GOPT_IDX_UINT
	GOPT_IDX_BIT,		// ana param and set/clear bit VALUE in 'un', ptype := GOPT_IDX_UINT
	GOPT_IDX_OR64,		// set 'un64' |= value 0..0x3ff, ptype := GOPT_IDX_U64
	GOPT_IDX_SETBIT64,	// set bit VALUE in 'un64', ptype := GOPT_IDX_UINT64

	GOPT_IDX_SINT,		// scan signed integer
	GOPT_IDX_UINT,		// scan unsigned integer
	GOPT_IDX_OFF_ON,	// scan 'off|onn|0|1' and otional a number, ptype := GOPT_IDX_UINT
	GOPT_IDX_S64,		// scan signed 64-bit integer
	GOPT_IDX_U64,		// scan unsigned 64-bit integer
	GOPT_IDX_HEX,		// scan unsigned 64-bit integer, assume hex
	GOPT_IDX_SIZE,		// scan size (double with optional SI) and store as 'un64'
	GOPT_IDX_DOUBLE,	// scan double float
	GOPT_IDX_DURATION,	// scan duration (double with optional SI) and store as 'd'
	GOPT_IDX_STRING,	// store ccp pointer to text

	GOPT_IDX_END,		// end of parameters


	//--- parameters types

	GOPT_T_NULL		= 0, // type undefined
	GOPT_T_INC		= GOPT_RESERVED_BEG | ( GOPT_IDX_INC	<< GOPT_S_TYPE ),
	GOPT_T_INC0		= GOPT_RESERVED_BEG | ( GOPT_IDX_INC0	<< GOPT_S_TYPE ),
	GOPT_T_DEC		= GOPT_RESERVED_BEG | ( GOPT_IDX_DEC	<< GOPT_S_TYPE ),
	GOPT_T_DEC0		= GOPT_RESERVED_BEG | ( GOPT_IDX_DEC0	<< GOPT_S_TYPE ),
	GOPT_T_SETP		= GOPT_RESERVED_BEG | ( GOPT_IDX_SETP	<< GOPT_S_TYPE ),
	GOPT_T_SETM		= GOPT_RESERVED_BEG | ( GOPT_IDX_SETM	<< GOPT_S_TYPE ),
	GOPT_T_SETP64		= GOPT_RESERVED_BEG | ( GOPT_IDX_SETP64	<< GOPT_S_TYPE ),
	GOPT_T_SETM64		= GOPT_RESERVED_BEG | ( GOPT_IDX_SETM64	<< GOPT_S_TYPE ),
	GOPT_T_OR		= GOPT_RESERVED_BEG | ( GOPT_IDX_OR	<< GOPT_S_TYPE ),
	GOPT_T_SETBIT		= GOPT_RESERVED_BEG | ( GOPT_IDX_SETBIT	<< GOPT_S_TYPE ),
	GOPT_T_BIT		= GOPT_RESERVED_BEG | ( GOPT_IDX_BIT	<< GOPT_S_TYPE ),
	GOPT_T_OR64		= GOPT_RESERVED_BEG | ( GOPT_IDX_OR64	<< GOPT_S_TYPE ),
	GOPT_T_SETBIT64		= GOPT_RESERVED_BEG | ( GOPT_IDX_SETBIT64 << GOPT_S_TYPE ),
	GOPT_T_SINT		= GOPT_RESERVED_BEG | ( GOPT_IDX_SINT	<< GOPT_S_TYPE ),
	GOPT_T_UINT		= GOPT_RESERVED_BEG | ( GOPT_IDX_UINT	<< GOPT_S_TYPE ),
	GOPT_T_OFF_ON		= GOPT_RESERVED_BEG | ( GOPT_IDX_OFF_ON	<< GOPT_S_TYPE ),
	GOPT_T_S64		= GOPT_RESERVED_BEG | ( GOPT_IDX_S64	<< GOPT_S_TYPE ),
	GOPT_T_U64		= GOPT_RESERVED_BEG | ( GOPT_IDX_U64	<< GOPT_S_TYPE ),
	GOPT_T_HEX		= GOPT_RESERVED_BEG | ( GOPT_IDX_HEX	<< GOPT_S_TYPE ),
	GOPT_T_SIZE		= GOPT_RESERVED_BEG | ( GOPT_IDX_SIZE	<< GOPT_S_TYPE ),
	GOPT_T_DOUBLE		= GOPT_RESERVED_BEG | ( GOPT_IDX_DOUBLE	<< GOPT_S_TYPE ),
	GOPT_T_DURATION		= GOPT_RESERVED_BEG | ( GOPT_IDX_DURATION << GOPT_S_TYPE ),
	GOPT_T_STRING		= GOPT_RESERVED_BEG | ( GOPT_IDX_STRING	<< GOPT_S_TYPE ),
	GOPT_T_END		= GOPT_RESERVED_BEG | ( GOPT_IDX_END	<< GOPT_S_TYPE ),


	//--- named options

	GOPT_O__BEGIN		= 0x100,
	GOPT_O_TEST_OPT,	// test: print all defined options: --@opt

	GOPT_O_HELP,		// force help
	GOPT_O_CLEAR,		// clear screen -> ScanGOptionsHelperTN()
	GOPT_O_PAGER,		// activate pager -> ScanGOptionsHelperTN()
	GOPT_O_WIDTH,		// limit terminal width -> ScanGOptionsHelperTN()
	GOPT_O_QUIT,		// quit if command done -> ScanGOptionsHelperTN()

	GOPT_O_QUIET,		// be quiet
	GOPT_O_VERBOSE,		// be verbose, or set by --verbose=#
	GOPT_O_DEBUG,		// increment debug level, or set by --debug=#

	GOPT_O__CONTINUE	// continue with this value on more options
}
GOPT_t;

static inline GOPT_t GetGOPType ( GOPT_t gt )
	{ return ( gt & GOPT_M_TYPE ) >> GOPT_S_TYPE; }

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    global options: parameters		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[GParamValueType_t]]

typedef enum GParamValueType_t
{
	GPT_NONE,	// no param set
	GPT_SN,		// value stored as 'sn'
	GPT_UN,		// value stored as 'un'
	GPT_SN64,	// value stored as 'sn64'
	GPT_UN64,	// value stored as 'un64'
	GPT_D,		// value stored as 'd'
	GPT_STR,	// value stored as 'str'
	GPT_BIT,	// value stored as 'bit.val' & 'bit.mask'
}
GParamValueType_t;

GParamValueType_t GetGParamValueType ( GOPT_t gt );

///////////////////////////////////////////////////////////////////////////////
// [[GParam_t]]

typedef struct GParam_t
{
    u16 ptype;		// GOPT_IDX_*
    u16 vtype;		// GParamValueType_t

    union
    {
	int	sn;	// used by GOPT_T_INC*, GOPT_T_DEC*, GOPT_T_SINT
	uint	un;	// used by GOPT_T_UINT
	s64	sn64;	// used by GOPT_T_S64, GOPT_T_SETM
	u64	un64;	// used by GOPT_T_U64, GOPT_T_HEX, GOPT_T_SIZE, GOPT_T_SETP
	double	d;	// used by GOPT_T_DOUBLE, GOPT_T_DURATION
	ccp	str;	// used by GOPT_T_STRING
	struct { u32 val; u32 mask; } bit;
			// used by GOPT_T_BIT
    };
}
GParam_t;

///////////////////////////////////////////////////////////////////////////////
// [[GetGParam_t]]

typedef enum GetGParam_t
{
	GGP_NONE = 0,	// no param set or param unusable, 'return_val' is untouched
	GGP_CONVERT,	// param set, but converted (different numeric type)
	GGP_SIGN,	// param returned, but sign maybe converted
	GGP_OK		// original or extended param returned
}
GetGParam_t;

// If gp==NULL: return GGP_NONE
// If return_val==NULL: analyse only
GetGParam_t GetGParamINT  ( const GParam_t *par, int    *return_val );
GetGParam_t GetGParamUINT ( const GParam_t *par, uint   *return_val );
GetGParam_t GetGParamS64  ( const GParam_t *par, s64    *return_val );
GetGParam_t GetGParamU64  ( const GParam_t *par, u64    *return_val );
GetGParam_t GetGParamDBL  ( const GParam_t *par, double *return_val );
GetGParam_t GetGParamCCP  ( const GParam_t *par, ccp    *return_val );
GetGParam_t GetGParamBIT  ( const GParam_t *par, u32    *return_val, u32 *return_mask );

ccp PrintGParam ( const GParam_t *par );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			global options			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[GOptions_t]]

typedef struct GOptions_t
{
    //--- parameters

    int		argc;		// number of parameters
    char	**argv;		// parameters without options


    //--- named options

    int		help;		// number of GOPT_O_HELP
    int		clear;		// number of GOPT_O_CLEAR
    int		pager;		// number of GOPT_O_PAGER
    int		quit;		// number of GOPT_O_QUIT
    int		debug;		// number of GOPT_O_DEBUG
    int		verbose;	// combi of GOPT_O_QUIET & GOPT_O_VERBOSE

    int		max_width;	// max terminal width
    int		width;		// terminal width

    //--- generic parameter support

    GParam_t param[GOPT_N_PARAM];
}
GOptions_t;

//-----------------------------------------------------------------------------

#define GOOD_INFO_MAX_WIDTH 120

static inline void InitializeGOptions ( GOptions_t *go, int max_width )
{
    DASSERT(go);
    memset(go,0,sizeof(*go));
    go->max_width = max_width;
    go->width = max_width < GOOD_INFO_MAX_WIDTH ? max_width : GOOD_INFO_MAX_WIDTH;
}

static inline const GParam_t * GetGParam ( const GOptions_t *go, uint par_index )
	{ return go && par_index < GOPT_N_PARAM ? go->param+par_index : 0; }

///////////////////////////////////////////////////////////////////////////////

extern const GenericOptParm_t StandardGOptions[];

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

typedef int (*ScanOptionFunc)
(
    // returns: 0 on not procecced / <0 on error / >0 on success

    GenericOpt_t	*go,		// valid pointer to generic options
    GOptions_t		*gopt,		// valid pointer to options data
    void		*any_ptr,	// user specific pointer
    int			num,		// numeric param:
					//   OnAddOptions():	'scan_opt'
					//   OnOption():	option id
					//   OnArgument():	argument index
    ccp			arg		// NULL or text argument
);

///////////////////////////////////////////////////////////////////////////////
// [[ScanGOpt_t]]

typedef enum ScanGOpt_t
{
	SGO_KNOWN	= 0x0001,   // scan only until first unknown parameter
	SGO_PAGER	= 0x0002,   // auto enable pager

	SGO__CONTINUE	= 0x0004    // define more flags based on this value
}
ScanGOpt_t;

///////////////////////////////////////////////////////////////////////////////

enumError ScanGOptionsHelper
(
    GOptions_t		*gopt,		// valid pointer
    GenericOpt_t	*go,		// NULL or list with options
					//	=> ResetGenericOpt() is called at finish
    int			argc,		// argument counter
    char		**argv,		// list of arguments
    int			max_param,	// >=0: max allowed parameters
    ScanGOpt_t		scan_opt,	// scan options

    //--- call back functions
    ScanOptionFunc	OnAddOptions,	// not NULL: called once to add options
    ScanOptionFunc	OnOption,	// not NULL: called for each option
    ScanOptionFunc	OnArgument,	// not NULL: called for each argument
    void		*any_ptr	// NULL or user specific pointer for call back
);

//-----------------------------------------------------------------------------

enumError ScanGOptionsHelperTN
(
    // like ScanGOptionsHelper(), but with TELNET support
    // for 'gopt->clear', 'gopt->pager' and 'gopt->quit',
    // if 'ts' is not NULL and telnet is active.

    GOptions_t		*gopt,		// valid pointer
    GenericOpt_t	*go,		// NULL or list with options
					//	=> ResetGenericOpt() is called at finish
    int			argc,		// argument counter
    char		**argv,		// list of arguments
    int			max_param,	// >=0: max allowed parameters
    ScanGOpt_t		scan_opt,	// scan options
    struct TCPStream_t	*ts,		// NULL or current stream with TELNET support

    //--- call back functions
    ScanOptionFunc	OnAddOptions,	// not NULL: called once to add options
    ScanOptionFunc	OnOption,	// not NULL: called for each option
    ScanOptionFunc	OnArgument,	// not NULL: called for each argument
    void		*any_ptr	// NULL or user specific pointer for call back
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_OPTION_H

