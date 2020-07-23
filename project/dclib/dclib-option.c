
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

#include "dclib-option.h"
#include <getopt.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    GenericOpt_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeGenericOpt ( GenericOpt_t *go, char first_char )
{
    DASSERT(go);
    memset(go,0,sizeof(*go));
    InitializeMemPool(&go->str_pool,2000);

    char *dest = go->sopt;
    if (first_char)
	*dest++ = first_char;
    *dest++ = ':';
    *dest = 0;
}

///////////////////////////////////////////////////////////////////////////////

void ResetGenericOpt ( GenericOpt_t *go )
{
    DASSERT(go);
    ResetMemPool(&go->str_pool);
    FREE(go->lopt);
    InitializeGenericOpt(go,0);
}

///////////////////////////////////////////////////////////////////////////////

void AddGenericOpt
(
    GenericOpt_t	*go,		// data structure
    const GenericOptParm_t *gp,		// list to add, list end on name==NULL
    bool		rm_minus	// remove '-' on options for alternatives
)
{
    DASSERT(go);
    DASSERT(gp);

    char * sopt_ptr = go->sopt + strlen(go->sopt);
    char * sopt_end = go->sopt + sizeof(go->sopt) - 3;

    for ( ; gp->name; gp++ )
    {
	ccp ptr = gp->name;
	for(;;)
	{
	    while ( *ptr == '|' )
		ptr++;

	    uint have_minus = 0;
	    char name_buf[50], *name_ptr = name_buf;
	    while ( *ptr && *ptr != '|' )
	    {
		char ch = *ptr++;
		if ( ch == '-' )
		    have_minus++;
		if ( name_ptr < name_buf + sizeof(name_buf) - 1 )
		    *name_ptr++ = ch;
	    }
	    *name_ptr = 0;
	    uint len = name_ptr - name_buf;
	    if (!len)
		break;

	    //--- short option

	    if ( len == 1 )
	    {
		uchar ch = *name_buf;
		if ( ch > ' ' && ch < 0x80 && ch != ':'
			&& !go->sopt_id[ch] && sopt_ptr < sopt_end )
		{
		    go->sopt_id[ch] = gp->id;
		    *sopt_ptr++ = ch;
		    if ( gp->arg )
		    {
			*sopt_ptr++ = ':';
			if ( gp->arg > 1 )
			    *sopt_ptr++ = ':';
		    }
		}
	    }


	    //--- long option

	    for(;;)
	    {
	     #if 0 // avoid duplicates, needed?
		uint i;
		for ( i = 0; i < go->lopt_used; i++ )
		    if (!strcmp(name_buf,go->lopt[i].name))
			goto skip;
	     #endif

		if ( go->lopt_used == go->lopt_size )
		{
		    go->lopt_size += 100;
		    go->lopt = REALLOC( go->lopt, go->lopt_size * sizeof(*go->lopt) );
		}

		struct option *opt = go->lopt + go->lopt_used++;
		opt->name	= StrDupMemPool(&go->str_pool,name_buf);
		opt->has_arg	= gp->arg;
		opt->flag	= 0;
		opt->val	= gp->id;

	     //skip:;
		if ( !have_minus || !rm_minus )
		    break;
		have_minus = 0;

		char *src = name_buf, *dest = name_buf;
		while ( *src )
		{
		    if ( *src != '-' )
			*dest++ = *src;
		    src++;
		}
		*dest = 0;
	    }
	}
    }

    *sopt_ptr = 0;

    if ( go->lopt_used == go->lopt_size )
    {
	go->lopt_size += 100;
	go->lopt = REALLOC( go->lopt, go->lopt_size * sizeof(*go->lopt) );
    }
    struct option *opt = go->lopt + go->lopt_used;
    memset(opt,0,sizeof(*opt));
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    global options: parameters		///////////////
///////////////////////////////////////////////////////////////////////////////

GParamValueType_t GetGParamValueType ( GOPT_t gt )
{
    if ( gt & GOPT_RESERVED_BEG )
	gt = GetGOPType(gt);

    switch (gt)
    {
	case GOPT_IDX_INC0:
	case GOPT_IDX_INC:
	case GOPT_IDX_DEC0:
	case GOPT_IDX_DEC:
	case GOPT_IDX_SETP:
	case GOPT_IDX_SETM:
	case GOPT_IDX_SINT:
	    return GPT_SN;

	case GOPT_IDX_UINT:
	case GOPT_IDX_OFF_ON:
	case GOPT_IDX_OR:
	case GOPT_IDX_SETBIT:
	    return GPT_UN;

	case GOPT_IDX_SETP64:
	case GOPT_IDX_SETM64:
	case GOPT_IDX_S64:
	    return GPT_SN64;

	case GOPT_IDX_U64:
	case GOPT_IDX_OR64:
	case GOPT_IDX_SETBIT64:
	case GOPT_IDX_HEX:
	case GOPT_IDX_SIZE:
	    return GPT_UN64;

	case GOPT_IDX_DOUBLE:
	case GOPT_IDX_DURATION:
	    return GPT_D;

	case GOPT_IDX_STRING:
	    return GPT_STR;

	case GOPT_IDX_BIT:
	    return GPT_BIT;

	default:
	    return GPT_NONE;
     }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamINT ( const GParam_t *par, int *return_val )
{
    if (par)
    {
	DASSERT( par->vtype == GetGParamValueType(par->ptype) );
	switch (par->vtype)
	{
	    case GPT_NONE:
	    case GPT_STR:
	    case GPT_BIT:
		return GGP_NONE;

	    case GPT_SN:
		if (return_val)
		    *return_val = par->sn;
		return GGP_OK;

	    case GPT_UN:
		if (return_val)
		    *return_val = par->un;
		return GGP_SIGN;

	    case GPT_SN64:
		if (return_val)
		    *return_val = par->sn64;
		return GGP_CONVERT;

	    case GPT_UN64:
		if (return_val)
		    *return_val = par->un64;
		return GGP_CONVERT;

	    case GPT_D:
		if (return_val)
		    *return_val = par->d;
		return GGP_CONVERT;
	}
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamUINT ( const GParam_t *par, uint *return_val )
{
    if (par)
    {
	DASSERT( par->vtype == GetGParamValueType(par->ptype) );
	switch (par->vtype)
	{
	    case GPT_NONE:
	    case GPT_STR:
	    case GPT_BIT:
		return GGP_NONE;

	    case GPT_SN:
		if (return_val)
		    *return_val = par->sn;
		return GGP_SIGN;

	    case GPT_UN:
		if (return_val)
		    *return_val = par->un;
		return GGP_OK;

	    case GPT_SN64:
		if (return_val)
		    *return_val = par->sn64;
		return GGP_CONVERT;

	    case GPT_UN64:
		if (return_val)
		    *return_val = par->un64;
		return GGP_CONVERT;

	    case GPT_D:
		if (return_val)
		    *return_val = par->d;
		return GGP_CONVERT;
	}
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamS64 ( const GParam_t *par, s64 *return_val )
{
    if (par)
    {
	DASSERT( par->vtype == GetGParamValueType(par->ptype) );
	switch (par->vtype)
	{
	    case GPT_NONE:
	    case GPT_STR:
	    case GPT_BIT:
		return GGP_NONE;

	    case GPT_SN:
		if (return_val)
		    *return_val = par->sn;
		return GGP_OK;

	    case GPT_UN:
		if (return_val)
		    *return_val = par->un;
		return GGP_SIGN;

	    case GPT_SN64:
		if (return_val)
		    *return_val = par->sn64;
		return GGP_OK;

	    case GPT_UN64:
		if (return_val)
		    *return_val = par->un64;
		return GGP_SIGN;

	    case GPT_D:
		if (return_val)
		    *return_val = par->d;
		return GGP_CONVERT;
	}
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamU64 ( const GParam_t *par, u64 *return_val )
{
    if (par)
    {
	DASSERT( par->vtype == GetGParamValueType(par->ptype) );
	switch (par->vtype)
	{
	    case GPT_NONE:
	    case GPT_STR:
	    case GPT_BIT:
		return GGP_NONE;

	    case GPT_SN:
		if (return_val)
		    *return_val = par->sn;
		return GGP_SIGN;

	    case GPT_UN:
		if (return_val)
		    *return_val = par->un;
		return GGP_OK;

	    case GPT_SN64:
		if (return_val)
		    *return_val = par->sn64;
		return GGP_SIGN;

	    case GPT_UN64:
		if (return_val)
		    *return_val = par->un64;
		return GGP_OK;

	    case GPT_D:
		if (return_val)
		    *return_val = par->d;
		return GGP_CONVERT;
	}
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamDBL ( const GParam_t *par, double *return_val )
{
    if (par)
    {
	DASSERT( par->vtype == GetGParamValueType(par->ptype) );
	switch (par->vtype)
	{
	    case GPT_NONE:
	    case GPT_STR:
	    case GPT_BIT:
		return GGP_NONE;

	    case GPT_SN:
		if (return_val)
		    *return_val = par->sn;
		return GGP_CONVERT;

	    case GPT_UN:
		if (return_val)
		    *return_val = par->un;
		return GGP_CONVERT;

	    case GPT_SN64:
		if (return_val)
		    *return_val = par->sn64;
		return GGP_CONVERT;

	    case GPT_UN64:
		if (return_val)
		    *return_val = par->un64;
		return GGP_CONVERT;

	    case GPT_D:
		if (return_val)
		    *return_val = par->d;
		return GGP_OK;
	}
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamSTR ( const GParam_t *par, ccp *return_val )
{
    DASSERT( !par || par->vtype == GetGParamValueType(par->ptype) );
    if ( par && par->vtype == GPT_STR )
    {
	if (return_val)
	    *return_val = par->str;
	return GGP_OK;
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////

GetGParam_t GetGParamBIT ( const GParam_t *par, u32 *return_val, u32 *return_mask )
{
    DASSERT( !par || par->vtype == GetGParamValueType(par->ptype) );
    if ( par && par->vtype == GPT_BIT )
    {
	if (return_val)
	    *return_val = par->bit.val;
	if (return_mask)
	    *return_mask = par->bit.mask;
	return GGP_OK;
    }
    return GGP_NONE;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp PrintGParam ( const GParam_t *par )
{
    DASSERT(par);
    DASSERT( par->vtype == GetGParamValueType(par->ptype) );
    char buf[100];
    ccp extra = "";

    switch (par->ptype)
    {
	case GOPT_IDX_NULL:
	    snprintf(buf,sizeof(buf),"[-]");
	    break;

	case GOPT_IDX_INC0:
	case GOPT_IDX_DEC0:
	case GOPT_IDX_DEC:
	    extra = "?";
	case GOPT_IDX_INC:
	    snprintf(buf,sizeof(buf),"[%sINC] %d",extra,par->sn);
	    break;

	case GOPT_IDX_SETP:
	case GOPT_IDX_SETM:
	    extra = "?";
	case GOPT_IDX_SINT:
	    snprintf(buf,sizeof(buf),"[%sSINT] %d",extra,par->sn);
	    break;

	case GOPT_IDX_OR:
	case GOPT_IDX_SETBIT:
	case GOPT_IDX_OFF_ON:
	    extra = "?";
	case GOPT_IDX_UINT:
	    snprintf(buf,sizeof(buf),"[%sUINT] %u",extra,par->un);
	    break;

	case GOPT_IDX_BIT:
	    snprintf(buf,sizeof(buf),"[BIT] set=%x, clr=%x",
			par->bit.val, par->bit.mask & ~par->bit.val );
	    break;

	case GOPT_IDX_SETP64:
	case GOPT_IDX_SETM64:
	    extra = "?";
	case GOPT_IDX_S64:
	    snprintf(buf,sizeof(buf),"[%sS64] %lld",extra,par->sn64);
	    break;

	case GOPT_IDX_OR64:
	case GOPT_IDX_SETBIT64:
	    extra = "?";
	case GOPT_IDX_U64:
	    snprintf(buf,sizeof(buf),"[%sU64] %llu",extra,par->un64);
	    break;

	case GOPT_IDX_HEX:
	    snprintf(buf,sizeof(buf),"[HEX] 0x%llx",par->un64);
	    break;

	case GOPT_IDX_SIZE:
	    snprintf(buf,sizeof(buf),"[SIZE] %llu = %s",
		par->un64, PrintSize1024(0,0,par->un64,false) );
	    break;

	case GOPT_IDX_DOUBLE:
	    snprintf(buf,sizeof(buf),"[DBL] %12g",par->d);
	    break;

	case GOPT_IDX_DURATION:
	    snprintf(buf,sizeof(buf),"[DUR] %12g = %s",
		par->d, PrintTimerUSec6(0,0,double2usec(par->d),false) );
	    break;

	case GOPT_IDX_STRING:
	    snprintf(buf,sizeof(buf),"[STR] \"%s\"",par->str);
	    break;


	default:
	    snprintf(buf,sizeof(buf),"[?%u]",par->ptype);
	    break;
    }

    const uint size = strlen(buf) + 1;
    char *res = GetCircBuf(size);
    memcpy(res,buf,size);
    return res;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			ScanGOptionsHelper()		///////////////
///////////////////////////////////////////////////////////////////////////////

const GenericOptParm_t StandardGOptions[] =
{
	{ GOPT_O_TEST_OPT,	0, "_opt" },
	{ GOPT_O_HELP,		0, "h|help" },
	{ GOPT_O_CLEAR,		0, "c|clear" },
	{ GOPT_O_PAGER,		0, "p|pager" },
	{ GOPT_O_QUIT,		0, "Q|quit" },
	{ GOPT_O_QUIET,		0, "q|quiet" },
	{ GOPT_O_VERBOSE,	0, "v|verbose" },
	{0,0,0}
};

///////////////////////////////////////////////////////////////////////////////

static int ScanGOffOn ( GenericOpt_t *go, int opt_stat, bool use_max, ccp optarg )
{
    ccp object = "'off/on' value";

    char buf[100];
    if (go)
    {
	uint i;
	for ( i = 0; i < go->lopt_used; i++ )
	{
	    struct option *lp = go->lopt + i;
	    if ( lp->val == opt_stat && lp->has_arg && lp->name )
	    {
		snprintf(buf,sizeof(buf),"'off/on' value of --%s",lp->name);
		object = buf;
		break;
	    }
	}
    }

    const uint max_num = use_max ? ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE : 0;
    return ScanKeywordOffOn(optarg,max_num,object);
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanGOptionsHelper
(
    GOptions_t		*gopt,		// valid pointer
    GenericOpt_t	*go,		// NULL or list with options
    int			argc,		// argument counter
    char		**argv,		// list of arguments
    int			max_param,	// >=0: max allowed parameters
    ScanGOpt_t		scan_opt,	// scan options

    //--- call back functions
    ScanOptionFunc	OnAddOptions,	// not NULL: called once to add options
    ScanOptionFunc	OnOption,	// not NULL: called for each option
    ScanOptionFunc	OnArgument,	// not NULL: called for each argument
    void		*any_ptr	// NULL or user specific pointer for call back
)
{
    DASSERT(gopt);
    DASSERT(argc>1);
    DASSERT(argv);

    //--- setup options

    GenericOpt_t go0;
    if (!go)
    {
	go = &go0;
	InitializeGenericOpt( go, scan_opt & SGO_KNOWN ? '+' : 0 );
    }

    if (OnAddOptions)
	OnAddOptions(go,gopt,any_ptr,scan_opt,0);
    AddGenericOpt(go,StandardGOptions,true);

    if ( argc > 1 )
    {
	// skip command
	argc--;
	argv++;
    }

    optind = 0; // '0' is correct for GNU to reset scanning
 #if !defined(TEST) && !defined(DEBUG)
    opterr = 0; // suppress error messages to stderr
 #endif


    //--- check options

    InitializeGOptions(gopt);

    int err = 0, test_opt = 0;
    enumError stat = ERR_OK;
    while ( stat == ERR_OK )
    {
      int opt_stat = getopt_long(argc,argv,go->sopt,go->lopt,0);
      if ( opt_stat == -1 )
	break;

      // translate short option names
      if ( (uint)opt_stat < sizeof(go->sopt_id)/sizeof(*go->sopt_id) && go->sopt_id[opt_stat] )
	opt_stat = go->sopt_id[opt_stat];

      if ( opt_stat >= GOPT_RESERVED_BEG && opt_stat < GOPT_RESERVED_END )
      {
	const uint pidx = opt_stat & GOPT_M_INDEX;
	if ( pidx >= GOPT_N_PARAM )
	{
	    printf("%s!!! %s: Parameter index %u must be <%u: %s%s\n",
			colout->warn, *argv,
			pidx, GOPT_N_PARAM, argv[optind-1],
			colout->reset );
	    stat = ERR_SYNTAX;
	    break;
	}
	GParam_t *par = gopt->param + pidx;

	const uint ptype = GetGOPType(opt_stat);
	switch(ptype)
	{
	 //--- increment & decrement: indicator 'ptype' is always GOPT_IDX_INC

	 case GOPT_IDX_INC0:
	    if ( par->ptype != GOPT_IDX_INC )
	    {
		par->ptype = GOPT_IDX_INC;
		par->sn = 1;
	    }
	    else
		par->sn = par->sn >= 0 ? par->sn+1 : 0;
	    break;

	 case GOPT_IDX_INC:
	    if ( par->ptype != GOPT_IDX_INC )
	    {
		par->ptype = GOPT_IDX_INC;
		par->sn = 1;
	    }
	    else
		par->sn = par->sn > 0 ? par->sn+1 : 1;
	    break;

	 case GOPT_IDX_DEC0:
	    if ( par->ptype != GOPT_IDX_INC )
	    {
		par->ptype = GOPT_IDX_INC;
		par->sn = -1;
	    }
	    else
		par->sn = par->sn <= 0 ? par->sn-1 : 0;
	    break;

	 case GOPT_IDX_DEC:
	    if ( par->ptype != GOPT_IDX_INC )
	    {
		par->ptype = GOPT_IDX_INC;
		par->sn = -1;
	    }
	    else
		par->sn = par->sn < 0 ? par->sn-1 : -1;
	    break;


	 //--- set values: indicator 'ptype' is GOPT_IDX_SINT or GOPT_IDX_S64

	 case GOPT_IDX_SETP:
	    par->ptype = GOPT_IDX_SINT;
	    par->un  = ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
	    break;

	 case GOPT_IDX_SETM:
	    par->ptype = GOPT_IDX_SINT;
	    par->sn  = - ( ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE );
	    break;

	 case GOPT_IDX_SETP64:
	    par->ptype = GOPT_IDX_S64;
	    par->un64  = ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
	    break;

	 case GOPT_IDX_SETM64:
	    par->ptype = GOPT_IDX_S64;
	    par->sn64  = - ( ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE );
	    break;


	 //--- set bits: indicator 'ptype' is GOPT_IDX_UINT or GOPT_IDX_U64

	 case GOPT_IDX_OR:
	    if ( par->ptype != GOPT_IDX_UINT )
	    {
		par->ptype = GOPT_IDX_UINT;
		par->un = 0;
	    }
	    par->un |= ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
	    break;

	 case GOPT_IDX_SETBIT:
	    {
		if ( par->ptype != GOPT_IDX_UINT )
		{
		    par->ptype = GOPT_IDX_UINT;
		    par->un = 0;
		}
		const uint bitnum = ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
		if ( bitnum < 8*sizeof(par->un) )
		    par->un |= 1u << bitnum;
	    }
	    break;

	 case GOPT_IDX_OR64:
	    if ( par->ptype != GOPT_IDX_U64 )
	    {
		par->ptype = GOPT_IDX_U64;
		par->un64 = 0;
	    }
	    par->un64 |= ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
	    break;

	 case GOPT_IDX_SETBIT64:
	    if ( par->ptype != GOPT_IDX_U64 )
	    {
		par->ptype = GOPT_IDX_U64;
		par->un64 = 0;
	    }
	    {
		const uint bitnum = ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
		if ( bitnum < 64 )
		    par->un64 |= 1llu << bitnum;
	    }
	    break;

	 case GOPT_IDX_BIT:
	    {
		if ( par->ptype != GOPT_IDX_BIT )
		{
		    par->ptype = GOPT_IDX_BIT;
		    par->bit.val = par->bit.mask = 0;
		}

		const int stat = ScanGOffOn(go,opt_stat,false,optarg);
		if ( stat >= 0 )
		{
		    const uint bitnum = ( opt_stat & GOPT_M_VALUE ) >> GOPT_S_VALUE;
		    if ( bitnum < 32 )
		    {
			const u32 m = 1u << bitnum;
			par->bit.mask |= m;
			if ( stat > 0 )
			    par->un |= m;
			else
			    par->un &= ~m;
		    }
		}
	    }
	    break;


	 //--- scan parameters

	 case GOPT_IDX_SINT:
	    par->ptype = GOPT_IDX_SINT;
	    par->sn = str2l(optarg,0,10);
	    break;

	 case GOPT_IDX_UINT:
	    par->ptype = GOPT_IDX_UINT;
	    par->un = str2ul(optarg,0,10);
	    break;

	 case GOPT_IDX_OFF_ON:
	    {
		const int stat = ScanGOffOn(go,opt_stat,true,optarg);
		par->un = stat >= 0 ? stat : 0;
		par->ptype = GOPT_IDX_UINT;
	    }
	    break;

	 case GOPT_IDX_S64:
	    par->ptype = GOPT_IDX_S64;
	    par->sn64 = str2ll(optarg,0,10);
	    break;

	 case GOPT_IDX_U64:
	    par->ptype = GOPT_IDX_U64;
	    par->un64 = str2ull(optarg,0,10);
	    break;

	 case GOPT_IDX_HEX:
	    par->ptype = GOPT_IDX_HEX;
	    par->un64 = str2ull(optarg,0,16);
	    break;

	 case GOPT_IDX_SIZE:
	    par->ptype = GOPT_IDX_SIZE;
	    ScanSizeU64(&par->un64,optarg,1,1,0); break;
	    break;

	 case GOPT_IDX_DOUBLE:
	    par->ptype = GOPT_IDX_DOUBLE;
	    par->d = strtod(optarg,0);
	    break;

	 case GOPT_IDX_DURATION:
	    par->ptype = GOPT_IDX_DURATION;
	    ScanDuration(&par->d,optarg,1,SDUMD_M_ALL); break;
	    break;

	 case GOPT_IDX_STRING:
	    par->ptype = GOPT_IDX_STRING;
	    par->str = optarg;
	    break;

	 default:
	    printf("!!! %s: Undefinef GOPT_* type 0x%02x [0x%02x]: %s\n",
			*argv, GetGOPType(opt_stat), opt_stat, argv[optind-1] );
	    stat = ERR_ERROR;
	}
	par->vtype = GetGParamValueType(par->ptype);
	continue;
      }

      switch (opt_stat)
      {
	case '?':
	    if ( optopt && (uint)optopt < 0x7f )
		printf("%s!!! %s: Invalid option: %s-%c [0x%02x]%s\n",
			colout->warn, *argv,
			colout->value, optopt, optopt,
			colout->reset );
	    else
		printf("%s!!! %s: Invalid option: %s%s%s\n",
			colout->warn, *argv,
			colout->value, argv[optind-1],
			colout->reset );
	    stat = ERR_SYNTAX;
	    break;

	case ':':
	    printf("%s!!! %s: Missing parameter for %s%s%s\n",
			colout->warn, *argv,
			colout->value, argv[optind-1],
			colout->reset );
	    stat = ERR_SYNTAX;
	    break;


    //--- standard options

	case GOPT_O_QUIET:
	    gopt->verbose = gopt->verbose > -1 ? -1 : gopt->verbose - 1;
	    break;
	case GOPT_O_VERBOSE:
	    gopt->verbose = gopt->verbose <  0 ?  0 : gopt->verbose + 1;
	    break;

	case GOPT_O_HELP:	gopt->help++; scan_opt |= SGO_PAGER; break;
	case GOPT_O_CLEAR:	gopt->clear++; break;
	case GOPT_O_PAGER:	gopt->pager++; break;
	case GOPT_O_QUIT:	gopt->quit++; break;
	case GOPT_O_TEST_OPT:	test_opt++; break;


    //--- misc

	default:
	    if (OnOption)
	    {
		const int ostat = OnOption(go,gopt,any_ptr,opt_stat,optarg);
		if ( ostat <= 0 )
		{
		    if (!ostat)
			printf("!!! %s: Internal error while scanning options [0x%02x]!\n",
				*argv, opt_stat );
		    stat = ERR_ERROR;
		}
	    }
	    break;
      }
    }

    if (test_opt)
    {
	printf("\nOPTIONS [N=%u/%u]: %s\n",go->lopt_used,go->lopt_size,go->sopt);
	const struct option *opt;
	for ( opt = go->lopt; opt->name; opt++ )
	    printf("  %04x %u %s\n", opt->val, opt->has_arg, opt->name );
    }

    char **dest = argv + optind;
    gopt->argv = dest;

    uint i;
    for ( i = optind; i < argc; i++ )
    {
	char *arg = argv[i];
	if ( !OnArgument || !OnArgument(go,gopt,any_ptr,dest-gopt->argv,arg) )
	    *dest++ = arg;
    }
    gopt->argc = dest - gopt->argv;

    if (err)
    {
	printf("%s!!! %s: Syntax error!%s\n",colout->warn,*argv,colout->reset);
	stat = ERR_SYNTAX;
	goto abort;
    }

    if ( max_param >= 0 && gopt->argc > max_param )
    {
	if (!max_param)
	    printf(
			"%s!!! %s: No parameters allowed, but %u found!%s\n",
			colout->warn, *argv, gopt->argc, colout->reset );
	else
	    printf(
			"%s!!! %s: Maximal %u parameter%s allowed, but %u found!%s\n",
			colout->warn, *argv,
			max_param, max_param == 1 ? "" : "s",
			gopt->argc, colout->reset );
	stat = ERR_SYNTAX;
	goto abort;
    }

    if ( scan_opt & SGO_PAGER && !gopt->pager )
	gopt->pager++;

 abort:
    ResetGenericOpt(go);
    if (stat)
	InitializeGOptions(gopt);
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

