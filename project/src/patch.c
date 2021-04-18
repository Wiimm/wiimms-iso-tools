
/***************************************************************************
 *                    __            __ _ ___________                       *
 *                    \ \          / /| |____   ____|                      *
 *                     \ \        / / | |    | |                           *
 *                      \ \  /\  / /  | |    | |                           *
 *                       \ \/  \/ /   | |    | |                           *
 *                        \  /\  /    | |    | |                           *
 *                         \/  \/     |_|    |_|                           *
 *                                                                         *
 *                           Wiimms ISO Tools                              *
 *                         https://wit.wiimm.de/                           *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *   This file is part of the WIT project.                                 *
 *   Visit https://wit.wiimm.de/ for project details and sources.          *
 *                                                                         *
 *   Copyright (c) 2009-2021 by Dirk Clemens <wiimm@wiimm.de>              *
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
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#include "patch.h"
#include "version.h"
#include "lib-std.h"
#include "lib-sf.h"
#include "wbfs-interface.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--enc				///////////////
///////////////////////////////////////////////////////////////////////////////

int opt_hook = 0; // <0: disabled, =0: auto, >0: enabled

///////////////////////////////////////////////////////////////////////////////

enumEncoding ScanEncoding ( ccp arg )
{
    static const KeywordTab_t tab[] =
    {
	{ 0,			"AUTO",		0,		ENCODE_MASK },

	{ ENCODE_CLEAR_HASH,	"NO-HASH",	"NOHASH",	ENCODE_CALC_HASH },
	{ ENCODE_CALC_HASH,	"HASHONLY",	0,		ENCODE_CLEAR_HASH },

	{ ENCODE_DECRYPT,	"DECRYPT",	0,		ENCODE_ENCRYPT },
	{ ENCODE_ENCRYPT,	"ENCRYPT",	0,		ENCODE_DECRYPT },

	{ ENCODE_NO_SIGN,	"NO-SIGN",	"NOSIGN",	ENCODE_SIGN },
	{ ENCODE_SIGN,		"SIGN",		"TRUCHA",	ENCODE_NO_SIGN },

	{ 0,0,0,0 }
    };

    const int stat = ScanKeywordListMask(arg,tab);
    if ( stat >= 0 )
	return stat & ENCODE_MASK;

    ERROR0(ERR_SYNTAX,"Illegal encoding mode (option --enc): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

enumEncoding encoding = ENCODE_DEFAULT;

int ScanOptEncoding ( ccp arg )
{
    const int new_encoding = ScanEncoding(arg);
    if ( new_encoding == -1 )
	return 1;
    encoding = new_encoding;
    return 0;
}

//-----------------------------------------------------------------------------

static const enumEncoding encoding_m_tab[]
	= { ENCODE_M_HASH, ENCODE_M_CRYPT, ENCODE_M_SIGN, 0 };

enumEncoding SetEncoding
	( enumEncoding val, enumEncoding set_mask, enumEncoding default_mask )
{
    TRACE("SetEncoding(%04x,%04x,%04x)\n",val,set_mask,default_mask);

    const enumEncoding * tab;
    for ( tab = encoding_m_tab; *tab; tab++ )
    {
	// set values
	if ( set_mask & *tab )
	    val = val & ~*tab | set_mask & *tab;

	// if more than 1 bit set: clear it
	if ( Count1Bits32( val & *tab ) > 1 )
	    val &= ~*tab;

	// if no bits are set: use default
	if ( !(val & *tab) )
	    val |= default_mask & *tab;
    }

    TRACE(" -> %04x\n", val & ENCODE_MASK );
    return val & ENCODE_MASK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--region			///////////////
///////////////////////////////////////////////////////////////////////////////

enumRegion ScanRegion ( ccp arg )
{
    static const KeywordTab_t tab[] =
    {
	{ REGION_JAP,		"JAPAN",	"JAP",		0 },
	{ REGION_USA,		"USA",		0,		0 },
	{ REGION_EUR,		"EUROPE",	"EUR",		0 },
	{ REGION_KOR,		"KOREA",	"KOR",		0 },

	{ REGION__AUTO,		"AUTO",		0,		0 },
	{ REGION__FILE,		"FILE",		0,		0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanKeywordListMask(arg,tab);
    if ( stat >= 0 )
	return stat;

    // try if arg is a number
    char * end;
    ulong num = strtoul(arg,&end,10);
    if ( end != arg && !*end )
	return num;

    ERROR0(ERR_SYNTAX,"Illegal region mode (option --region): '%s'\n",arg);
    return REGION__ERROR;
}

//-----------------------------------------------------------------------------

enumRegion opt_region = REGION__AUTO;

int ScanOptRegion ( ccp arg )
{
    const int new_region = ScanRegion(arg);
    if ( new_region == REGION__ERROR )
	return 1;
    opt_region = new_region;
    return 0;
}

//-----------------------------------------------------------------------------

ccp GetRegionName ( enumRegion region, ccp unkown_value )
{
    static ccp tab[] =
    {
	"Japan",
	"USA",
	"Europe",
	"Korea"
    };

    return (unsigned)region < sizeof(tab)/sizeof(*tab)
		? tab[region]
		: unkown_value;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static const RegionInfo_t RegionTable[] =
{
	// -> http://www.wiibrew.org/wiki/Title_Database#Region_Codes

	/*A*/ { REGION_EUR,  0, "ALL ", "All" },
	/*B*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*C*/ { REGION_EUR,  0, "CHIN", "Chinese" },
	/*D*/ { REGION_EUR, 1,  "GERM", "German" },
	/*E*/ { REGION_USA, 1,  "USA ", "NTSC/USA" },
	/*F*/ { REGION_EUR, 1,  "FREN", "French" },
	/*G*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*H*/ { REGION_EUR,  0, "NL  ", "Netherlands" },
	/*I*/ { REGION_EUR, 1,  "ITAL", "Italian" },
	/*J*/ { REGION_JAP, 1,  "JAPA", "Japan" },
	/*K*/ { REGION_KOR, 1,  "KORE", "Korea" },
	/*L*/ { REGION_JAP, 1,  "J>PL", "Japan->PAL" },
	/*M*/ { REGION_USA, 1,  "A>PL", "America->PAL" },
	/*N*/ { REGION_JAP, 1,  "J>US", "Japan->NTSC" },
	/*O*/ { REGION_EUR,  0, "-?- ", "-?-" },
	/*P*/ { REGION_EUR, 1,  "PAL ", "PAL" },
	/*Q*/ { REGION_KOR, 1,  "KO/J", "Korea (japanese)" },
	/*R*/ { REGION_EUR, 1,  "RUS ", "Russia" },
	/*S*/ { REGION_EUR, 1,  "SPAN", "Spanish" },
	/*T*/ { REGION_KOR, 1,  "KO/E", "Korea (english)" },
	/*U*/ { REGION_EUR,  0, "AUS ", "Australia" },
	/*V*/ { REGION_EUR,  0, "SCAN", "Scandinavian" },
	/*W*/ { REGION_EUR,  0, "TAIW", "Taiwan" },
	/*X*/ { REGION_EUR, 1,  "EURO", "Almost Europe" },
	/*Y*/ { REGION_EUR,  0, "EURO", "Almost Europe" },
	/*Z*/ { REGION_EUR,  0, "ANY ", "PAL or US" },

	/*?*/ { REGION_EUR,  0, "-?- ", "-?-" } // illegal region_code
};

//-----------------------------------------------------------------------------

const RegionInfo_t * GetRegionInfo ( char region_code )
{
    region_code = toupper((int)region_code);
    if ( region_code < 'A' || region_code > 'Z' )
	region_code = 'Z' + 1;
    return RegionTable + (region_code-'A');
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--common-key			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_ckey_index_t ScanCommonKey ( ccp arg )
{
    static const KeywordTab_t tab[] =
    {
	{ WD_CKEY_STANDARD,	"STANDARD",	0,		0 },
	{ WD_CKEY_KOREA,	"KOREAN",	0,		0 },
 #ifdef SUPPORT_CKEY_DEVELOP
	{ WD_CKEY_DEVELOPER,	"DEVELOPER",	0,		0 },
 #endif
	{ WD_CKEY__N,		"AUTO",		0,		0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanKeywordListMask(arg,tab);
    if ( stat >= 0 )
	return stat;

    // try if arg is a number
    char * end;
    ulong num = strtoul(arg,&end,10);
    if ( end != arg && !*end && num < WD_CKEY__N )
	return num;

    ERROR0(ERR_SYNTAX,"Illegal common key index (option --common-key): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

wd_ckey_index_t opt_common_key = WD_CKEY__N;

int ScanOptCommonKey ( ccp arg )
{
    const wd_ckey_index_t new_common_key = ScanCommonKey(arg);
    if ( new_common_key == -1 )
	return 1;
 #ifdef TEST
    printf("COMMON-KEY: %d -> %d\n",opt_common_key,new_common_key);
 #endif
    opt_common_key = new_common_key;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--ios				///////////////
///////////////////////////////////////////////////////////////////////////////

u64 opt_ios = 0;
bool opt_ios_valid = false;

//-----------------------------------------------------------------------------

bool ScanSysVersion ( u64 * ios, ccp arg )
{
    u32 stat, lo, hi = 1;

    arg = ScanNumU32(arg,&stat,&lo,0,~(u32)0);
    if (!stat)
	return false;

    if ( *arg == ':' || *arg == '-' )
    {
	arg++;
	hi = lo;
	arg = ScanNumU32(arg,&stat,&lo,0,~(u32)0);
	if (!stat)
	    return false;
    }

    if (ios)
	*ios = (u64)hi << 32 | lo;

    return !*arg;
}

//-----------------------------------------------------------------------------

int ScanOptIOS ( ccp arg )
{
    opt_ios = 0;
    opt_ios_valid = false;

    if ( !arg || !*arg )
	return 0;

    opt_ios_valid = ScanSysVersion(&opt_ios,arg);
    if (opt_ios_valid)
	return 0;

    ERROR0(ERR_SYNTAX,"Illegal system version (option --ios): %s\n",arg);
    return 1;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    --http --domain --wiimmfi		///////////////
///////////////////////////////////////////////////////////////////////////////

bool opt_http			= false;
ccp  opt_domain			= 0;
bool opt_security_fix		= false;
int  disable_patch_on_load	= 0;

static const char nin_wifi_net[] = "nintendowifi.net";
static uint nin_wifi_net_len = sizeof(nin_wifi_net) - 1;

static const char gamespy_com[] = "gamespy.com";
static uint gamespy_com_len = sizeof(gamespy_com) - 1;

static const char sake_gamespy_com[] = "sake.gamespy.com";
static uint sake_gamespy_com_len = sizeof(sake_gamespy_com) - 1;

///////////////////////////////////////////////////////////////////////////////
// [[domain]]

int ScanOptDomain ( bool http, bool security_fix, ccp domain )
{
    if (http)
	opt_http = true;

    if (security_fix)
	opt_security_fix = true;

    if ( domain && *domain )
    {
	if ( strlen(domain) > nin_wifi_net_len )
	    return ERROR0(ERR_SEMANTIC,"Domain '%s' is to long.",domain);
	opt_domain = domain;
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static u32 get_offset_of_id ( u8 *data, uint size, ccp id )
{
    const uint id_len = id[3] ? 4 : 3;
    u8 *found = memmem(data,size,id,id_len);
    if (!found)
	return 0;

    // calc forward and back to verify offset
    const u32 addr = GetDolAddrByOffset((dol_header_t*)data,found-data,4,0);
    const u32 offset = GetDolOffsetByAddr((dol_header_t*)data,addr,4,0);
    if ( !offset || memcmp(data+offset,id,id_len) )
	return 0;

    if ( id_len == 3 )
    {
	const char at3 = data[offset+3];
	if ( at3 < 'A' || at3 > 'Z' )
	    return 0;
    }

    return offset;
}

//-----------------------------------------------------------------------------

static bool PatchHost ( u8 *data, uint size, bool patch, PatchStat_t *pstat )
{
    struct tab_t
    {
	char id[4];		// relevant ID
	u32  addr_host;		// virtual address of 'Host' => patch it
    };

    static const struct tab_t tab[] =
    {
	{ "SC7",  0x8023c954 },  // Call of Duty Black Ops
	{ "RJA",  0x801b838c },  // Call of Duty: Modern Warfare - Reflex Edition
	{ "SM8",  0x80238c74 },  // Call of Duty: Modern Warfare 3
	{ "Band", 0x808e3b20 },  // Rock Band 3
	{ "Band", 0x808d6934 },  // The Beatles: Rock Band
	{ "",0}
    };

    if (!IsDolHeader(data,size,size))
	return false;
    const dol_header_t *dh = (dol_header_t*)data;

    const struct tab_t *ptr;
    for ( ptr = tab; ptr->id[0]; ptr++ )
    {
	const u32 off_host = GetDolOffsetByAddr(dh,ptr->addr_host,4,0);
	if ( !off_host || memcmp(data+off_host,"Host",4) )
	    continue;

	const u32 off_id = get_offset_of_id(data,size,ptr->id);
	if (!off_id)
	    continue;

	PRINT("FOUND: %08x %s : %08x %s\n",
	    off_id, PrintID(data+off_id,4,0),
	    off_host, PrintID(data+off_host,4,0) );

	if (pstat)
	    pstat->n_host++;
	if (patch)
	    data[off_host] = 'X';
	return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////

static bool PatchStrings ( u8 *data, uint size, bool patch, PatchStat_t *pstat )
{
    struct tab_t
    {
	char id[4];		// relevant ID
	u32  addr1;		// virtual address
	ccp  old1;		// old string
	ccp  new1;		// new string
	u32  addr2;		// virtual address
	ccp  old2;		// old string
	ccp  new2;		// new string
    };

    static const struct tab_t tab[] =
    {
	{ "RVY", // Call of Duty - World at War
	  0x8012b598,
		"https://naswii.nintendowifi.net/ac",
		"http://naswii.rvy.gs.wiimmfi.de/ac",
	  0x80128e40,
		"gpcm.gs.nintendowifi.net",
		"gpcm.rvy.gs.wiimmfi.de",
	},

	{"",0,0,0,0,0,0}
    };

    if (!IsDolHeader(data,size,size))
	return false;
    const dol_header_t *dh = (dol_header_t*)data;

    const struct tab_t *ptr;
    for ( ptr = tab; ptr->addr1; ptr++ )
    {
	const uint slen1 = strlen(ptr->old1);
	const u32 offset1 = GetDolOffsetByAddr(dh,ptr->addr1,slen1,0);
	if ( !offset1 || memcmp(data+offset1,ptr->old1,slen1) )
	    continue;
	PRINT("STRING1: %s\n",data+offset1);

	const uint slen2 = strlen(ptr->old2);
	const u32 offset2 = GetDolOffsetByAddr(dh,ptr->addr2,slen2,0);
	if ( !offset2 || memcmp(data+offset2,ptr->old2,slen2) )
	    continue;
	PRINT("STRING2: %s\n",data+offset2);

	const u32 off_id = get_offset_of_id(data,size,ptr->id);
	if (!off_id)
	    continue;

	PRINT("PATCH STRINGS:\n");
	if (pstat)
	    pstat->n_string++;
	if (patch)
	{
	    memset(data+offset1,0,slen1);
	    strncpy((char*)data+offset1,ptr->new1,slen1+1);
	    memset(data+offset2,0,slen2);
	    strncpy((char*)data+offset2,ptr->new2,slen2+1);
	}
	return true;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// [[domain]]

uint PatchDomain
	( bool is_dol, u8 *data, uint size, ccp title, bool patch, PatchStat_t *pstat )
{
    DASSERT(data);
    DASSERT( !opt_domain || strlen(opt_domain) <= nin_wifi_net_len );

    if (pstat)
	pstat->n_files++;

    if (is_dol)
    {
	if (PatchHost(data,size,patch,pstat))
	    return 4;
	if (PatchStrings(data,size,patch,pstat))
	    return 8;
    }


    //--- patch HTTPS & domains    

    char *end = (char*)data + size;

    char *next_n = 0, *update_n = (char*)data+1;
    char *next_h = memchr(data+1,'h',size-1);
    if (!next_h)
	next_h = end;

    int stat = 0;
    for(;;)
    {
      if (update_n)
      {
	if ( update_n < end )
	{
	    // search for 'nintendowifi.net'
	    next_n = memchr(update_n,'n',end-update_n);
	    if (!next_n)
		next_n = end;

	    // search for 'gamespy.com'
	    char *next_g = memchr(update_n,'g',end-update_n);
	    if ( next_g && next_g < next_n )
		next_n = next_g;
	}
	else
	    next_n = end;
	update_n = 0;
      }
      if ( next_h >= end && next_n >= end )
	break;

      ccp found = 0, info = 0;
      if ( next_h < next_n )
      {
	if ( opt_http && !memcmp(next_h,"https",5) )
	{
	    uint mode	= !memcmp(next_h+5,"://naswii.",10) ? 2
			: !next_h[-1] || !memcmp(next_h+5,"://",3) ? 1
			: 0;
	    if (mode)
	    {
		stat |= 1;
		found = next_h;
		info  = "HTTPS:";

		next_h += 4;
		char *dest = next_h++;
		if ( mode == 2 )
		{
		    strcpy(dest,"://nas.");
		    dest += 7;
		    next_h += 10;
		}

		if (pstat)
		    pstat->n_http++;
		if (patch)
		{
		    while (*next_h)
			*dest++ = *next_h++;
		    while ( dest < next_h )
			*dest++ = 0;
		}
		else
		    while (*next_h)
			next_h++;

		if ( next_n < next_h )
		    update_n = (char*)found; // recalc next_n, because position moved
	    }
	}

	next_h++;
	next_h = memchr(next_h,'h',end-next_h);
	if (!next_h)
	    next_h = end;
      }
      else
      {
	if ( opt_domain
		&& next_n[-1] == '.'
		&& !memcmp(next_n,nin_wifi_net,nin_wifi_net_len) )
	{
	    const char ch = next_n[nin_wifi_net_len];
	    if ( !ch || ch == '/' )
	    {
		stat |= 2;
		found = next_n;
		info  = "DOMAIN:";
		if (pstat)
		    pstat->n_domain++;

		if (patch)
		{
		    const uint dom_len = strlen(opt_domain);
		    memcpy(next_n,opt_domain,dom_len);

		    char *dest = next_n + dom_len;
		    next_n += nin_wifi_net_len;
		    while (*next_n)
			*dest++ = *next_n++;
		    while ( dest < next_n )
			*dest++ = 0;
		}
		else
		    while (*next_n)
			next_n++;
	    }
	}
	else if ( opt_domain
		&& !memcmp(next_n-5,sake_gamespy_com,sake_gamespy_com_len) )
	{
	    const char ch = next_n[gamespy_com_len];
	    if ( !ch || ch == '/' )
	    {
		const uint dom_len = strlen(opt_domain);
		if ( dom_len > gamespy_com_len )
		{
		    static int count = 0;
		    if (!count++)
			ERROR0(ERR_WARNING,"Can't replace '%s' by '%s': to long\n",
					gamespy_com, opt_domain );
		}
		else
		{
		    stat |= 2;
		    found = next_n;
		    info  = "DOMAIN:";
		    if (pstat)
			pstat->n_domain++;

		    if (patch)
		    {
			memcpy(next_n,opt_domain,dom_len);
			char *dest = next_n + dom_len;
			next_n += gamespy_com_len;
			while (*next_n)
			    *dest++ = *next_n++;
			while ( dest < next_n )
			    *dest++ = 0;
		    }
		    else
			while (*next_n)
			    next_n++;
		}
	    }
	}
	update_n = next_n + 1;
      }

      if ( found && verbose > 2 && title )
      {
	ccp beg = found - 1;
	while ( *beg > ' ' && *beg < 0x7f )
	    beg--;
	beg++;
	fprintf(stderr,"PATCHED %s/%-7s %6x  %s\n",title,info,(int)(beg-(ccp)data),beg);
      }
    }

    return stat;
}

///////////////////////////////////////////////////////////////////////////////

static uint security_patch ( AnalyzeFile_t *af, FILE *log );

int patch_main_and_rel ( wd_disc_t * disc, AnalyzeFile_t *af )
{
    DASSERT(disc);

    if ( !opt_http && !opt_domain && !opt_security_fix )
	return 0;

    AnalyzeFile_t local_af;
    if (!af)
    {
	InitializeAnaFile(&local_af);
	local_af.load_dol	= true;
	local_af.patch_dol	= true;
	local_af.allow_mkw	= true;
	af = &local_af;
    }
    
    wd_patch_main_t pm;
    wd_patch_main(&pm,disc,true,true);

    uint stat = 0;
    if ( pm.main )
    {
	stat |= PatchDomain(true,pm.main->data,pm.main->size,"MAIN",
				af->patch_dol,&af->pstat_main);
	if (opt_security_fix)
	{
	    af->dol_patch     = pm.main->data;
	    af->dol_load_size = pm.main->size;
	    stat |= security_patch(af,0);
	}
    }

    if ( pm.staticr )
	stat |= PatchDomain(false,pm.staticr->data,pm.staticr->size,"SREL",
				af->patch_dol,&af->pstat_staticr);

    PRINT("patch_main_and_rel(), stat=%x\n",stat);
    if ( af == &local_af )
	ResetAnaFile(af);
    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--modify			///////////////
///////////////////////////////////////////////////////////////////////////////

wd_modify_t ScanModify ( ccp arg )
{
    static const KeywordTab_t tab[] =
    {
	{ WD_MODIFY__NONE,	"NONE",		"-",	WD_MODIFY__ALL },
	{ WD_MODIFY__ALL,	"ALL",		0,	WD_MODIFY__ALL },
	{ WD_MODIFY__AUTO,	"AUTO",		0,	WD_MODIFY__ALL },
	{ WD_MODIFY__TT,	"TT",		0,	0 },

	{ WD_MODIFY_DISC,	"DISC",		0,	0 },
	{ WD_MODIFY_BOOT,	"BOOT",		0,	0 },
	{ WD_MODIFY_TICKET,	"TICKET",	"TIK",	0 },
	{ WD_MODIFY_TMD,	"TMD",		0,	0 },
	{ WD_MODIFY_WBFS,	"WBFS",		0,	0 },

	{ 0,0,0,0 }
    };

    const int stat = ScanKeywordList(arg,tab,0,true,0,0,0,0);
    if ( stat >= 0 )
	return ( stat & WD_MODIFY__ALL ? stat & WD_MODIFY__ALL : stat ) | WD_MODIFY__ALWAYS;

    ERROR0(ERR_SYNTAX,"Illegal modify mode (option --modify): '%s'\n",arg);
    return -1;
}

//-----------------------------------------------------------------------------

wd_modify_t opt_modify = WD_MODIFY__AUTO | WD_MODIFY__ALWAYS;

int ScanOptModify ( ccp arg )
{
    const int new_modify = ScanModify(arg);
    if ( new_modify == -1 )
	return 1;
    ASSERT( new_modify & WD_MODIFY__ALWAYS );
    opt_modify = new_modify;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    --name			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp modify_name = 0;
static char modify_name_buf[WII_TITLE_SIZE];

//-----------------------------------------------------------------------------

int ScanOptName ( ccp arg )
{
    if ( !arg || !*arg )
    {
	modify_name = 0;
	return 0;
    }

    const size_t max_len = sizeof(modify_name_buf) - 1;
    size_t len = strlen(arg);
    if ( len > max_len )
    {
	ERROR0(ERR_WARNING,"option --name: name is %zu characters to long:\n!\t-> %s\n",
		len - max_len, arg);
	len = max_len;
    }

    ASSERT( len < sizeof(modify_name_buf) );
    memset(modify_name_buf,0,sizeof(modify_name_buf));
    memcpy(modify_name_buf,arg,len);
    modify_name = modify_name_buf;

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--id  &  --xxx-id		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp modify_id		= 0;
ccp modify_disc_id	= 0;
ccp modify_boot_id	= 0;
ccp modify_ticket_id	= 0;
ccp modify_tmd_id	= 0;
ccp modify_wbfs_id	= 0;

static char modify_id_buf	[7];
static char modify_disc_id_buf	[7];
static char modify_boot_id_buf	[7];
static char modify_ticket_id_buf[5];
static char modify_tmd_id_buf	[5];
static char modify_wbfs_id_buf	[7];

//-----------------------------------------------------------------------------

static int ScanOptIdHelper
	( ccp p_arg, ccp opt_name, ccp *id_ptr, char *buf, int max_len )
{
    DASSERT(opt_name);
    DASSERT(id_ptr);
    DASSERT(buf);
    DASSERT( max_len == 4 || max_len == 6 ); // not really needed

    *id_ptr = 0;
    memset(buf,0,max_len+1);
    if ( !p_arg || !*p_arg )
	return 0;

    ccp plus = 0, arg = p_arg;
    char *dest = buf;
    while ( *arg )
    {
	if ( *arg == '+' && !plus )
	{
	    plus = dest;
	    arg++;
	    continue;
	}

	if ( dest >= buf + max_len )
	{
	    ERROR0(ERR_SYNTAX,"Option --%s: ID is %zu characters to long: %s\n",
		    opt_name, strlen(arg), p_arg );
	    return 1;
	}

	if ( *arg == '.'  || *arg == '_' )
	{
	    *dest++ = *arg++;
	}
	else if ( isalnum((int)*arg))
	{
	    *dest++ = toupper((int)*arg++);
	}
	else
	{
	    ERROR0(ERR_SYNTAX,"Option --%s: Illegal character '%c' at index #%u: %s\n",
			opt_name, *arg, (int)(arg-p_arg), p_arg );
	    return 1;
	}
    }

    if (plus)
    {
	char *dest2 = buf + max_len;
	while ( dest > plus )
	    *--dest2 = *--dest;
	while ( dest2 > plus )
	    *--dest2 = '.';
    }

    *id_ptr = buf;
    return 0;
}

//-----------------------------------------------------------------------------

int ScanOptId ( ccp arg )
{
    return ScanOptIdHelper( arg, "id", &modify_id,
			modify_id_buf, sizeof(modify_id_buf)-1 );
}

int ScanOptDiscId ( ccp arg )
{
    return ScanOptIdHelper( arg, "disc-id", &modify_disc_id,
			modify_disc_id_buf, sizeof(modify_disc_id_buf)-1 );
}

int ScanOptBootId ( ccp arg )
{
    return ScanOptIdHelper( arg, "boot-id", &modify_boot_id,
			modify_boot_id_buf, sizeof(modify_boot_id_buf)-1 );
}

int ScanOptTicketId ( ccp arg )
{
    return ScanOptIdHelper( arg, "ticket-id", &modify_ticket_id,
			modify_ticket_id_buf, sizeof(modify_ticket_id_buf)-1 );
}

int ScanOptTmdId ( ccp arg )
{
    return ScanOptIdHelper( arg, "tmd-id", &modify_tmd_id,
			modify_tmd_id_buf, sizeof(modify_tmd_id_buf)-1 );
}

int ScanOptWbfsId ( ccp arg )
{
    return ScanOptIdHelper( arg, "wbfs-id", &modify_wbfs_id,
			modify_wbfs_id_buf, sizeof(modify_wbfs_id_buf)-1 );
}

//-----------------------------------------------------------------------------

static void NormID
	( wd_modify_t condition, ccp *id_ptr, char *buf, int max_len )
{
    noPRINT("NormID() cond=%03x, len=%u, src=%s\n",condition,max_len,modify_id);
    DASSERT(id_ptr);

    ccp src = modify_id;
    char *dest = buf;

    if ( !*id_ptr && src && condition & opt_modify )
    {
	for ( ; max_len > 0 && *src && *dest; src++, dest++, max_len-- )
	    if ( *dest == '.' )
		*dest = *src;

	for ( ; max_len > 0 && *src; max_len-- )
	    *dest++ = *src++;
    }

    while ( max_len-- > 0 && *dest )
	dest++;

    while ( dest > buf && dest[-1] == '.' )
	dest--;
    *dest = 0;
    *id_ptr = dest == buf ? 0 : buf;
    noPRINT(" -> new-id=%s\n",*id_ptr);
}

void NormalizeIdOptions()
{
    NormID( WD_MODIFY__AUTO|WD_MODIFY_DISC, &modify_disc_id,
		modify_disc_id_buf, sizeof(modify_disc_id_buf)-1 );
    NormID( WD_MODIFY__AUTO|WD_MODIFY_BOOT, &modify_boot_id,
		modify_boot_id_buf, sizeof(modify_boot_id_buf)-1 );
    NormID( WD_MODIFY__AUTO|WD_MODIFY_TICKET, &modify_ticket_id,
		modify_ticket_id_buf, sizeof(modify_ticket_id_buf)-1 );
    NormID( WD_MODIFY__AUTO|WD_MODIFY_TMD,  &modify_tmd_id,
		modify_tmd_id_buf,  sizeof(modify_tmd_id_buf)-1 );
    NormID( WD_MODIFY__AUTO|WD_MODIFY_WBFS, &modify_wbfs_id,
		modify_wbfs_id_buf, sizeof(modify_wbfs_id_buf)-1 );
}

//-----------------------------------------------------------------------------

bool PatchId
(
    void	*dest_id,	// destination with 'maxlen' byte
    ccp		patch_id,	// NULL or patch string
    int		skip,		// 'patch_id' starts at index 'skip'
    int		maxlen		// length of destination ID
)
{
    ASSERT(dest_id);
    if ( !patch_id || maxlen < 1 )
	return false;

    PRINT("PATCH ID: %.*s -> %.*s\n",maxlen,(ccp)dest_id,maxlen,patch_id);

    while ( skip-- > 0 && *patch_id )
	patch_id++;

    char *dest;
    for ( dest = dest_id; *patch_id && maxlen > 0; patch_id++, dest++, maxlen-- )
    {
	if ( *patch_id != '.' )
	    *dest = *patch_id;
    }
    return true;
}

//-----------------------------------------------------------------------------

bool CopyPatchId
(
    void	*dest,		// destination with 'maxlen' byte
    const void	*src,		// source of ID. If NULL or empty: clear dest
    ccp		patch_id,	// NULL or patch string
    int		maxlen,		// length of destination ID
    bool	null_term	// true: Add an additional 0 byte to end of dest
)
{
    DASSERT(dest);
    DASSERT( maxlen>0 && maxlen <= 6 );

    if ( src && *(u8*)src )
    {
	memcpy(dest,src,maxlen);
	if (null_term)
	    ((u8*)dest)[maxlen] = 0;
	return PatchId(dest,patch_id,0,maxlen);
    }

    if (null_term)
	maxlen++;
    memset(dest,0,maxlen);
    return false;
}

//-----------------------------------------------------------------------------

bool CopyPatchWbfsId ( char *dest_id6, const void * source_id6 )
{
    return CopyPatchId(dest_id6,source_id6,modify_wbfs_id,6,true);
}

//-----------------------------------------------------------------------------

bool CopyPatchDiscId ( char *dest_id6, const void * source_id6 )
{
    return opt_hook >= 0
	&& CopyPatchId(dest_id6,source_id6,modify_disc_id,6,true);
}

//-----------------------------------------------------------------------------

bool PatchIdCond ( void * id, int skip, int maxlen, wd_modify_t condition )
{
    return ( opt_modify & condition ) && PatchId(id,modify_id,skip,maxlen);
}

//-----------------------------------------------------------------------------

bool PatchName ( void * name, wd_modify_t condition )
{
    ASSERT(name);
    if ( !modify_name || !(opt_modify & condition) )
	return false;

    ASSERT( strlen(modify_name) < WII_TITLE_SIZE );
    memcpy(name,modify_name,WII_TITLE_SIZE);
    return true;
}

//-----------------------------------------------------------------------------

bool PatchDiscHeader ( void * dhead, const void * patch_id, const void * patch_name )
{
    DASSERT(dhead);
    bool stat = false;

    if (patch_id)
	stat = wd_patch_id(dhead,dhead,patch_id,6);

    if (patch_name)
    {
	char * title = (char*)dhead + WII_TITLE_OFF;
	if (memcmp(title,patch_name,WII_TITLE_SIZE))
	{
	    stat = true;
	    strncpy(title,patch_name,WII_TITLE_SIZE);
	    title[WII_TITLE_SIZE-1] = 0;
	}
    }

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--trim				///////////////
///////////////////////////////////////////////////////////////////////////////

wd_trim_mode_t opt_trim = WD_TRIM_DEFAULT;

//-----------------------------------------------------------------------------

wd_trim_mode_t ScanTrim
(
    ccp arg,			// argument to scan
    ccp err_text_extend		// error message extention
)
{
    static const KeywordTab_t tab[] =
    {
	{ WD_TRIM_DEFAULT,	"DEFAULT",	0,	WD_TRIM_M_ALL },

	{ WD_TRIM_NONE,		"NONE",		"-",	WD_TRIM_ALL },
	{ WD_TRIM_ALL,		"ALL",		0,	WD_TRIM_ALL },
	{ WD_TRIM_FAST,		"FAST",		0,	WD_TRIM_ALL },

	{ WD_TRIM_DISC,		"DISC",		"D",	WD_TRIM_DEFAULT },
	{ WD_TRIM_PART,		"PARTITION",	"P",	WD_TRIM_DEFAULT },
	{ WD_TRIM_FST,		"FILESYSTEM",	"F",	WD_TRIM_DEFAULT },

	{ 0,			"BEGIN",	0,	WD_TRIM_DEFAULT | WD_TRIM_F_END },
	{ WD_TRIM_F_END,	"END",		0,	WD_TRIM_DEFAULT | WD_TRIM_F_END },

	{ 0,0,0,0 }
    };

    const int stat = ScanKeywordList(arg,tab,0,true,0,0,0,0);
    if ( stat >= 0 )
	return stat;

    ERROR0(ERR_SYNTAX,"Illegal trim mode%s: '%s'\n",err_text_extend,arg);
    return -1;
}

//-----------------------------------------------------------------------------

int ScanOptTrim
(
    ccp arg			// argument to scan
)
{
    const int new_trim = ScanTrim(arg," (option --trim)");
    if ( new_trim == -1 )
	return 1;
    opt_trim = new_trim;
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////	    --align & --align-part & --align-files	///////////////
///////////////////////////////////////////////////////////////////////////////

u32  opt_align1		= 0;
u32  opt_align2		= 0;
u32  opt_align3		= 0;
u32  opt_align_part	= 0;
bool opt_align_files	= false;

//-----------------------------------------------------------------------------

int ScanOptAlign ( ccp p_arg )
{
    opt_align1 = opt_align2 = opt_align3 = 0;

    static u32 * align_tab[] = { &opt_align1, &opt_align2, &opt_align3, 0 };
    u32 ** align_ptr = align_tab;

    ccp arg = p_arg, prev_arg = 0;
    char * dest_end = iobuf + sizeof(iobuf) - 1;

    while ( arg && *arg && *align_ptr )
    {
	ccp save_arg = arg;
	char * dest = iobuf;
	while ( dest < dest_end && *arg && *arg != ',' )
	    *dest++ = *arg++;
	*dest = 0;
	if ( dest > iobuf && ScanSizeOptU32(
				*align_ptr,	// u32 * num
				iobuf,		// ccp source
				1,		// default_factor1
				0,		// int force_base
				"align",	// ccp opt_name
				4,		// u64 min
				WII_SECTOR_SIZE,// u64 max
				0,		// u32 multiple
				1,		// u32 pow2
				true		// bool print_err
				))
	    return 1;

	if ( prev_arg && align_ptr[-1][0] >= align_ptr[0][0] )
	{
	    ERROR0(ERR_SEMANTIC,
			"Option --align: Ascending order expected: %.*s\n",
			(int)(arg - prev_arg), prev_arg );
	    return 1;
	}

	align_ptr++;
	if ( !*align_ptr || !*arg )
	    break;
	arg++;
	prev_arg = save_arg;
    }

    if (*arg)
    {
	ERROR0(ERR_SEMANTIC,
		"Option --align: End of parameter expected: %.20s\n",arg);
	return 1;
    }

    return 0;
}

//-----------------------------------------------------------------------------

int ScanOptAlignPart ( ccp arg )
{
    return ScanSizeOptU32(
		&opt_align_part,	// u32 * num
		arg,			// ccp source
		1,			// default_factor1
		0,			// int force_base
		"align-part",		// ccp opt_name
		WII_SECTOR_SIZE,	// u64 min
		WII_GROUP_SIZE,		// u64 max
		0,			// u32 multiple
		1,			// u32 pow2
		true			// bool print_err
		) != ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			--disc-size			///////////////
///////////////////////////////////////////////////////////////////////////////

u64 opt_disc_size = 0;

//-----------------------------------------------------------------------------

int ScanOptDiscSize ( ccp arg )
{
    return ScanSizeOptU64(&opt_disc_size,arg,GiB,0,"disc-size",
		0, WII_SECTOR_SIZE * (u64)WII_MAX_SECTORS,
		WII_SECTOR_SIZE, 0, true ) != ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    --add-files & --rm-files		///////////////
///////////////////////////////////////////////////////////////////////////////

StringField_t add_file;
StringField_t repl_file;

//-----------------------------------------------------------------------------

int ScanOptFile ( ccp arg, bool add )
{
    if ( arg && *arg )
    {
	StringField_t * sf = add ? &add_file : &repl_file;
	InsertStringField(sf,arg,false);
    }
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			security_patch()		///////////////
///////////////////////////////////////////////////////////////////////////////

static u8 get_opcode ( cvp instructionAddr )
{
    return be32(instructionAddr) >> 26 & 0x3f;
}

static u16 get_immediate_value ( cvp instructionAddr )
{
    return be32(instructionAddr) & 0xffff;
}

static u32 get_load_target_reg ( cvp instructionAddr )
{
    return be32(instructionAddr) >> 21 & 0x1f;
}

static u32 get_comparison_target_reg ( cvp instructionAddr )
{
    return be32(instructionAddr) >> 16 & 0x1f;
}

//-----------------------------------------------------------------------------

static uint security_patch ( AnalyzeFile_t *af, FILE *log )
{
    DASSERT(af);

    af->pstat_main.n_agent  = 0;
    af->pstat_main.n_p2p    = 0;
    af->pstat_main.n_master = 0;

    if ( !af->allow_mkw && af->dol_is_mkw || !af->dol && !af->dol_patch )
	return 0;

    u8 *dol_head  = af->dol_patch ? (u8*)af->dol_patch : (u8*)af->dol;
    u8 *dol_start = dol_head + DOL_HEADER_SIZE;
    u8 *dol_end   = dol_head + af->dol_load_size;
    if ( dol_start >= dol_end )
	return 0;

    const int MODIFED = 4;
    uint stat = 0;
    int log_count = 0;
    ccp fname;
    if (  af->sf && af->sf->f.fname )
    {
	fname = strrchr(af->sf->f.fname,'/');
	fname = fname ? fname+1 : af->sf->f.fname;
    }
    else
	fname = "main.dol";


    //--- opcode lists

    static const u8 gt2locator[] = { 0x38,0x61,0x00,0x08, 0x38,0xA0,0x00,0x14 };

    // opcode lists for p2p:
    static const u8 opCodeChainP2P_v1[22]
	= { 32,32,21,21, 21,21,20,20, 31,40,21,20, 20,31,31,10, 20,36,21,44, 36,16 };
    static const u8 opCodeChainP2P_v2[22]
	= { 32,32,21,21, 20,21,20,21, 31,40,21,20, 20,31,31,10, 20,36,21,44, 36,16 };

    // opcode lists for MASTER:
    static const u8 opCodeChainMASTER_v1[22]
	= { 21,21,21,21, 40,20,20,20, 20,31,31,14, 31,20,21,44, 21,36,36,18, 11,16 };
    static const u8 opCodeChainMASTER_v2[22]
	= { 21,21,21,21, 40,20,20,20, 20,31,31,14, 31,20,21,36, 21,44,36,18, 11,16 };


    //--- tests

    int hasGT2Error = 0;
    u8 *MASTERopcodeChain = 0;

    for ( u8 *cur = dol_start; cur < dol_end; )
    {
	static const char str[] = "<GT2> RECV-0x%02x <- [--------:-----] [pid=%u]";
	u8 *found = memmem(cur,dol_end-cur,str,sizeof(str)-1);
	if (!found)
	    break;
	hasGT2Error++;
	cur = found + sizeof(str)-1;
    }

    if ( hasGT2Error > 1 )
    {
	if (log)
	    fprintf(log,"FAIL: %4u patch targets : %s\n\n", hasGT2Error,fname);
	return stat;
    }

    for ( u8 *cur = dol_start; cur < dol_end; cur++ )
    {
	if ( !memcmp(cur, "User-Agent\x00\x00RVL SDK/", 20) )
	{
	    if (log)
	    {
		log_count++;
		fprintf(log,"PATCH:  UA[%d]   %#08zx : %s\n",
				hasGT2Error,cur-dol_head,fname);
	    }

	    stat = MODIFED;
	    af->pstat_main.n_agent++;
	    if (af->patch_dol)
	    {
		if (hasGT2Error)
		    memcpy( cur+12, "D-3-1", 5 );
		else
		    memcpy( cur+12, "D-3-0", 5 );
		cur += 12+5;
		while (*cur)
		    *cur++ = 0;
	    }
	}
    }

    if ( !af->pstat_main.n_agent && log )
    {
	static char ua1[] = "User-Agent";
	static char ua2[] = "User-agent";
	static char ua3[] = "user-agent";

	ccp found = memmem(dol_start,dol_end-dol_start,ua1,sizeof(ua1)-1);
	if (!found)
	    found = memmem(dol_start,dol_end-dol_start,ua2,sizeof(ua2)-1);
	if (!found)
	    found = memmem(dol_start,dol_end-dol_start,ua3,sizeof(ua3)-1);

	if (found)
	{
	    fprintf(log,"ALT-UA: alt '%.*s'  : %s\n",(int)sizeof(ua1)-1,found,fname);
	    while ( *found && found < (ccp)dol_end )
		found++;
	    while ( !*found && found < (ccp)dol_end )
		found++;
	    if ( found < (ccp)dol_end )
	    {
		fprintf(log,"ALT-UA: next string: ");
		for ( int i = 0; i< 80 && found < (ccp)dol_end; i++ )
		{
		    const uchar ch = *found++;
		    if ( ch < ' ' )
			break;
		    fputc(ch,log);
		}
		fputc('\n',log);
	    }
	}
	else
	    fprintf(log,"NO-UA:  no 'User-Agent'  : %s\n",fname);
    }

    if (!hasGT2Error)
    {
	if (log)
	    fprintf(log,"NO-GT2: no GT2 error     : %s\n\n",fname);
	return stat;
    }

    for ( u8 *cur = dol_start; cur < dol_end; cur += 4 )
    {
	if ( !memcmp(cur,&gt2locator,8))
	{
	    int found_opcode_chain_P2P_v1 = 1;
	    int found_opcode_chain_P2P_v2 = 1;

	    for ( int i = 0; i < 22; i++ )
	    {
		const u8 opcode = get_opcode( cur + i*4 + 12 );
		if ( opCodeChainP2P_v1[i] != opcode )
		{
		    found_opcode_chain_P2P_v1 = 0;
		    if (!found_opcode_chain_P2P_v2)
			break;
		}
		if ( opCodeChainP2P_v2[i] != opcode )
		{
		    found_opcode_chain_P2P_v2 = 0;
		    if (!found_opcode_chain_P2P_v1)
			break;
		}
	    }

	    // The code could be a few instructions further down, use a dynamic search.

	    int found_opcode_chain_MASTER_v1;
	    int found_opcode_chain_MASTER_v2;

	    for (int dynamic = 0; dynamic < 40; dynamic += 4)
	    {
		found_opcode_chain_MASTER_v1 = 1;
		found_opcode_chain_MASTER_v2 = 1;

		for ( int i = 0; i < 22; i++ )
		{
		    const u8 opcode = get_opcode( cur + i * 4 + 12 + dynamic );
		    if ( opCodeChainMASTER_v1[i] != opcode )
		    {
			found_opcode_chain_MASTER_v1 = 0;
			if (!found_opcode_chain_MASTER_v2)
			    break;
		    }
		    if ( opCodeChainMASTER_v2[i] != opcode )
		    {
			found_opcode_chain_MASTER_v2 = 0;
			if (!found_opcode_chain_MASTER_v1)
			    break;
		    }
		}

		if ( found_opcode_chain_MASTER_v1 || found_opcode_chain_MASTER_v2 )
		{
		    // we found the opcode chain, let's take a note of the offset
		    MASTERopcodeChain = cur + 12 + dynamic;
		    break;
		}
	    }

	    if ( found_opcode_chain_P2P_v1 || found_opcode_chain_P2P_v2 )
	    {
		// Opcodes match known opcode order for part
		// of DWCi_HandleGT2UnreliableMatchCommandMessage.

		if (
		    get_immediate_value( cur + 0x0c ) == 0x0c &&
		    get_immediate_value( cur + 0x10 ) == 0x18 &&
		    get_immediate_value( cur + 0x30 ) == 0x12 &&
		    get_immediate_value( cur + 0x48 ) == 0x5a &&
		    get_immediate_value( cur + 0x50 ) == 0x0c &&
		    get_immediate_value( cur + 0x58 ) == 0x12 &&
		    get_immediate_value( cur + 0x5c ) == 0x18 &&
		    get_immediate_value( cur + 0x60 ) == 0x18
		)
		{
		  if (log)
		  {
		    log_count++;
		    fprintf(log,"PATCH:  P2P[%c%c] %#08zx : %s\n",
					found_opcode_chain_P2P_v1 ? '1' : '-',
					found_opcode_chain_P2P_v2 ? '2' : '-',
					cur-dol_head,fname);
		  }

		  stat = MODIFED;
		  af->pstat_main.n_p2p++;
		  if (af->patch_dol)
		  {
		    // Immediate values match known values for
		    // DWCi_HandleGT2UnreliableMatchCommandMessage. So we're probably here.
		    u32 reg_load = get_load_target_reg( cur + 0x14 );
		    u32 reg_cmp = get_comparison_target_reg( cur + 0x48 );

		    // Found DWCi_HandleGT2UnreliableMatchCommandMessage, altering instructions
		    if (found_opcode_chain_P2P_v1)
		    {
			write_be32(cur+0x14, 0x88010011 | reg_cmp << 21 );	// lbz reg_cmp, 0x11(r1)
			write_be32(cur+0x18, 0x28000080 | reg_cmp << 16 );	// cmplwi reg_cmp, 0x80
			write_be32(cur+0x24, 0x41810064 );			// bgt- +0x64
			write_be32(cur+0x28, 0x60000000 );			// nop
			write_be32(cur+0x2c, 0x60000000 );			// nop
			write_be32(cur+0x34, 0x3C005A00 | reg_cmp << 21 );	// lis reg_cmp, 0x5a00
			write_be32(cur+0x48, 0x7C000000 | reg_cmp << 16
							| reg_load << 11 );	// cmpw reg_cmp, reg_load
		    }
		    else
		    {
			reg_load = 12;
			write_be32(cur+0x14, 0x88010011 | reg_cmp  << 21 );	// lbz reg_cmp, 0x11(r1)
			write_be32(cur+0x18, 0x28000080 | reg_cmp  << 16 );	// cmplwi reg_cmp, 0x80
			write_be32(cur+0x1c, 0x41810070 );
			write_be32(cur+0x24, be32(cur+0x28) );
			write_be32(cur+0x28, 0x8001000c | reg_load << 21 );
			write_be32(cur+0x2c, 0x3C005A00 | reg_cmp  << 21 );
			write_be32(cur+0x34, 0x7c000000 | reg_cmp  << 16
							| reg_load << 11 );
			write_be32(cur+0x48, 0x60000000 );
		    }
		  }
		}
	    }
	    else if ( found_opcode_chain_MASTER_v1 || found_opcode_chain_MASTER_v2 )
	    {
		// Opcodes match known opcode order for part of DWCi_QR2ClientMsgCallback.
		// Now compare all the immediate values:

		const u16 v3c = get_immediate_value( MASTERopcodeChain + 0x3c );
		const u16 v44 = get_immediate_value( MASTERopcodeChain + 0x44 );

		if (
		    get_immediate_value( MASTERopcodeChain + 0x10 ) == 0x12 &&
		    get_immediate_value( MASTERopcodeChain + 0x2c ) == 0x04 &&
		    get_immediate_value( MASTERopcodeChain + 0x48 ) == 0x18 &&
		    get_immediate_value( MASTERopcodeChain + 0x50 ) == 0x00 &&
		    get_immediate_value( MASTERopcodeChain + 0x54 ) == 0x18 &&
		    ( v3c == 0x12 && v44 == 0x0c || v3c == 0x0c && v44 == 0x12 )
		)
		{
		    if (log)
		    {
			log_count++;
			fprintf(log,"PATCH:  MAS[%c%c] %#08zx : %s\n",
					found_opcode_chain_MASTER_v1 ? '1' : '-',
					found_opcode_chain_MASTER_v2 ? '2' : '-',
					MASTERopcodeChain-dol_head, fname );
		    }

		    stat = MODIFED;
		    af->pstat_main.n_master++;
		    if (af->patch_dol)
		    {
			u8 *p = MASTERopcodeChain;
			if (found_opcode_chain_MASTER_v2)
			    write_be32(p+0x3c,be32(p+0x44));

			// MASTERopcodeChain is now 0x9c
			u32 rY = get_comparison_target_reg(MASTERopcodeChain);   // r8
			u32 rX = get_load_target_reg(MASTERopcodeChain);         // r7

			write_be32(p+0x00, 0x38000004 | rX << 21 );	// li rX, 4;
			write_be32(p+0x04, 0x7c00042c | rY << 21
						      | 3 << 16
						      | rX << 11 );	// lwbrx rY, r3, rX;
			write_be32(p+0x14, 0x9000000c | rY << 21
						      | 1 << 16 );	// stw rY, 0xc(r1);
			write_be32(p+0x18, 0x88000011 | rY << 21
						      | 1 << 16 );	// lbz rY, 0x11(r1);
			write_be32(p+0x28, 0x28000080 | rY << 16 );	// cmplwi rY, 0x80
			write_be32(p+0x38, 0x60000000 );		// nop
			write_be32(p+0x44, 0x41810014 );		// bgt- 0x800e5a94
		    }
		}
	    }
	}
    }

    if ( log && ( !af->pstat_main.n_p2p || !af->pstat_main.n_master ))
    {
	log_count++;
	fprintf(log,"FAIL: %12d %2d %2d : %s\n",
		hasGT2Error, af->pstat_main.n_p2p, af->pstat_main.n_master,
		fname );
    }

    if (log_count)
	fputc('\n',log);

    return stat;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			AnalyzeFile_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeAnaFile ( AnalyzeFile_t * af )
{
    DASSERT(af);
    memset(af,0,sizeof(*af));
    af->dol_is_mkw = -1;
    af->dol_region[0] = '?';
}

///////////////////////////////////////////////////////////////////////////////

void ResetAnaFile ( AnalyzeFile_t * af )
{
    if (af)
    {
	if ( af->dol && af->dol_alloced )
	    FREE(af->dol);
	if ( af->dol_patch && af->dol_patch_alloced )
	    FREE(af->dol_patch);
	InitializeAnaFile(af);
    }
}

///////////////////////////////////////////////////////////////////////////////

bool SetupDolPatchAnalyzeFile ( AnalyzeFile_t * af )
{
    DASSERT(af);

    if (!af->dol)
	return false;

    if (!af->dol_patch)
    {
	af->dol_patch = MALLOC(af->dol_load_size);
	af->dol_patch_alloced = true;
    }
    memcpy(af->dol_patch,af->dol,af->dol_load_size);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static int ana_file ( wd_iterator_t *it )
{
    //PRINT("===> %s : %x +%x\n",it->path,4*it->off4,it->size);

    if ( !it->part || it->icm != WD_ICM_FILE )
	return 0;

    AnalyzeFile_t *af = it->param;
    if (!strcmp(it->path,"DATA/sys/main.dol"))
    {
	if ( it->size >= DOL_HEADER_SIZE )
	{
	    DASSERT(af);
	    af->dol_file_size	= it->size;
	    af->dol_load_size	= af->load_dol ? it->size : DOL_HEADER_SIZE;
	    af->dol		= MALLOC(af->dol_load_size);
	    af->dol_alloced	= true;
	    wd_read_part(it->part,it->off4,af->dol,af->dol_load_size,false);
	}
    }
    else if (!strcasecmp(it->path,"DATA/files/rel/StaticR.rel"))
    {
	u8 *data = MALLOC(it->size);
	wd_read_part(it->part,it->off4,data,it->size,false);
	PatchDomain(false,data,it->size,"SREL",af->patch_dol,&af->pstat_staticr);
	FREE(data);
    }

    return 0;
}

//-----------------------------------------------------------------------------

void AnalyzeFile ( AnalyzeFile_t *af )
{
    if ( !af || !af->sf )
	return;

    SuperFile_t *sf = af->sf;


    //--- DOL support

    if ( ( sf->f.ftype & FT__ID_MASK ) == FT_ID_DOL )
    {
	uint dol_file_size = sf->f.fatt.size;
	if ( dol_file_size >= DOL_HEADER_SIZE )
	{
	    af->dol_file_size = dol_file_size;
	    af->dol_load_size = af->load_dol ? dol_file_size : DOL_HEADER_SIZE;
	    af->dol           = MALLOC(af->dol_load_size);
	    af->dol_alloced   = true;
	    ReadSF(sf,0,af->dol,af->dol_load_size);
	}
    }
    else if ( sf->f.ftype & FT_A_WII_ISO && !(sf->f.ftype & FT_M_NKIT) )
    {
	wd_disc_t * disc = OpenDiscSF(sf,true,true);
	if (disc)
	    wd_iterate_files(disc,ana_file,af,0,WD_IPM_PART_NAME);
    }

    if ( af->dol && !IsDolHeader(af->dol,af->dol_load_size,af->dol_file_size) )
    {
	FREE(af->dol);
	af->dol = 0;
	af->dol_load_size = 0;
	af->dol_file_size = 0;
    }

    static sha1_hash_t sha1_mkw[] =
    {
	// PAL
	{ 0x12, 0xb0, 0x2a, 0xf1, 0x9e,  0x07, 0xd3, 0x6d, 0x8a, 0x38,
	  0xc9, 0xa7, 0xfe, 0x89, 0x15,  0x55, 0x2f, 0x2e, 0x58, 0x5f },

	// USA
	{ 0x0a, 0xed, 0x61, 0x96, 0x28,  0x11, 0xa1, 0x64, 0xf5, 0x8c,
	  0xd7, 0xed, 0xfc, 0x71, 0x6e,  0xeb, 0x05, 0xb8, 0x4d, 0xab },

	// JAP
	{ 0x69, 0xed, 0xa1, 0xa4, 0x05,  0xde, 0x0e, 0xe1, 0x29, 0x09,
	  0x15, 0x4f, 0xd5, 0xdc, 0x2c,  0x33, 0x62, 0x9f, 0x6f, 0x37 },

	// KOR
	{ 0x86, 0x2a, 0x38, 0xf2, 0xd7,  0x56, 0xe5, 0x96, 0x2c, 0xa1,
	  0x9f, 0xd0, 0x62, 0xd0, 0xbd,  0x55, 0x67, 0x10, 0xed, 0x7e },
    };

    static char region[][4] = { "PAL", "USA", "JAP", "KOR", "" };

    if ( af->dol )
    {
	if ( af->dol_load_size >= 0x1100 )
	{
	    af->dol_is_mkw = 0;

	    sha1_hash_t hash;
	    SHA1((u8*)af->dol+0x100,0x1000,hash);

	    uint i;
	    for ( i = 0; region[i][0]; i++ )
		if (!memcmp(sha1_mkw+i,hash,sizeof(hash)))
		{
		    af->dol_is_mkw = 1;
		    StringCopyS(af->dol_region,sizeof(af->dol_region),region[i]);
		    break;
		}
	}

	if (SetupDolPatchAnalyzeFile(af))
	{
	    PatchDomain(true,(u8*)af->dol_patch,af->dol_load_size,0,af->patch_dol,&af->pstat_main);
	    if (!af->pstat_main.n_host)
		security_patch(af,0);
	}
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		      RewriteModifiedSF()		///////////////
///////////////////////////////////////////////////////////////////////////////

enumError RewriteModifiedSF
(
    SuperFile_t		* fi,		// valid input file
    SuperFile_t		* fo,		// NULL or output file
    struct WBFS_t	* wbfs,		// NULL or output WBFS
    u64			off		// offset: write_off := read_off + off
)
{
    ASSERT(fi);
    ASSERT(fi->f.is_reading);
    if (wbfs)
	fo = wbfs->sf;
    ASSERT(fo);
    ASSERT(fo->f.is_writing);
    PRINT("+++ RewriteModifiedSF(%p,%p,%p,%llx), oft=%d,%d\n",
		fi,fo,wbfs,off,fi->iod.oft,fo->iod.oft);

    wd_disc_t * disc = fi->disc1;
    if ( !fi->modified_list.used && ( !disc || !disc->reloc ))
    {
	TRACE("--- RewriteModifiedSF() ERR_OK: nothing to do\n");
	return ERR_OK;
    }

    UpdateSignatureFST(fi->fst); // NULL allowed

    if ( logging > 2 )
    {
	printf("\n Rewrite:\n\n");
	PrintMemMap(&fi->modified_list,stdout,3,0);
	if ( disc && disc->reloc )
	    wd_print_relocation(stdout,3,disc->reloc,true);
	putchar('\n');
    }

 #ifdef DEBUG
    fprintf(TRACE_FILE,"Rewrite:\n");
    PrintMemMap(&fi->modified_list,TRACE_FILE,3,0);
 #endif

    IOData_t saved_iod;
    memcpy(&saved_iod,&fo->iod,sizeof(saved_iod));
    WBFS_t * saved_wbfs = fo->wbfs;
    bool close_disc = false;

    if (wbfs)
    {
	TRACE(" - WBFS stat: w=%p, disc=#%d,%p, oft=%d\n",
		wbfs, wbfs->disc_slot, wbfs->disc, fo->iod.oft );
	if (!wbfs->disc)
	{
	    OpenWDiscSlot(wbfs,wbfs->disc_slot,0);
	    if (!wbfs->disc)
	    {
		TRACE("--- RewriteModifiedSF() ERR_CANT_OPEN: wbfs disc\n");
		return ERR_CANT_OPEN;
	    }
	    close_disc = true;
	}
	SetupIOD(fo,OFT_WBFS,OFT_WBFS);
	fo->wbfs = wbfs;
    }

    int idx;
    enumError err = ERR_OK;
    for ( idx = 0; idx < fi->modified_list.used && !err; idx++ )
    {
	const MemMapItem_t * mmi = fi->modified_list.field[idx];
	err = CopyRawData2(fi,mmi->off,fo,mmi->off+off,mmi->size);
    }

    if ( disc && disc->reloc )
    {
	const wd_reloc_t * reloc = disc->reloc;
	for ( idx = 0; idx < WII_MAX_SECTORS && !err; idx++, reloc++ )
	    if ( *reloc & WD_RELOC_F_LAST )
	    {
		u64 inoff = idx*WII_SECTOR_SIZE;
		err = CopyRawData2(fi,inoff,fo,inoff+off,WII_SECTOR_SIZE);
	    }
    }

    if (close_disc)
    {
	CloseWDisc(wbfs);
	wbfs->disc = 0;
    }

    memcpy(&fo->iod,&saved_iod,sizeof(fo->iod));
    fo->wbfs = saved_wbfs;
    TRACE("--- RewriteModifiedSF() err=%u: END\n",err);
    return err;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			write patch files		///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupWritePatch
(
    WritePatch_t	* pat		// patch data structure
)
{
    DASSERT(pat);
    memset(pat,0,sizeof(*pat));
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseWritePatch
(
    WritePatch_t	* pat		// patch data structure
)
{
    DASSERT(pat);

    enumError err = ERR_OK;
    if (pat->file)
    {
	wpat_toc_header_t thead;
	memset(&thead,0,sizeof(thead));
	thead.type_size = wpat_calc_size(WPAT_TOC_HEADER,sizeof(thead));
	thead.entry_offset4 = htonl(ftell(pat->file)>>2);
	memcpy(thead.magic,wpat_magic,sizeof(thead.magic));

	int n_entires = 0;
	// [[2do]] write toc files
	thead.n_entires = htonl(n_entires);

	if ( !err && fwrite(&thead,sizeof(thead),1,pat->file) != 1 )
	    err = ERROR1(ERR_WRITE_FAILED,
			"Writing patch toc header failed: %s",pat->fname);

	fclose(pat->file);
	pat->file = 0;
    }

    if (pat->fname)
    {
	FreeString(pat->fname);
	pat->fname = 0;
    }

    // [[2do]] clear dynamic data (toc)

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateWritePatch
(
    WritePatch_t	* pat,		// patch data structure
    ccp			filename	// filename of output file
)
{
    DASSERT(pat);
    DASSERT(filename);

    CloseWritePatch(pat);
    FILE * f = fopen(filename,"wb");
    if (!f)
	return ERROR1(ERR_CANT_CREATE,
		"Can't create patch file: %s\n",filename);

    return CreateWritePatchF(pat,f,filename);
}

///////////////////////////////////////////////////////////////////////////////

enumError CreateWritePatchF
(
    WritePatch_t	* pat,		// patch data structure
    FILE		* file,		// open output file
    ccp			filename	// NULL or known filename
)
{
    DASSERT(pat);
    DASSERT(file);

    CloseWritePatch(pat);
    SetupWritePatch(pat);
    pat->file = file;
    pat->fname = STRDUP( filename ? filename : "?" );

    //--- write patch header & creator comment

    wpat_header_t hd;
    memcpy(hd.magic,wpat_magic,sizeof(hd.magic));
    hd.type_size	= wpat_calc_size(WPAT_HEADER,sizeof(hd)-sizeof(hd.magic));
    hd.version		= htonl(WIT_PATCH_VERSION);
    hd.compatible	= htonl(WIT_PATCH_COMPATIBLE);

    if ( fwrite(&hd,sizeof(hd),1,pat->file) != 1 )
	return ERROR1(ERR_WRITE_FAILED,
		"Writing patch header failed: %s",pat->fname);

    return WritePatchComment(pat,"%s",
		"Creator: " TOOLSET_SHORT " v" VERSION " r" REVISION " " SYSTEM2 );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError WritePatchComment
(
    WritePatch_t	* pat,		// patch data structure
    ccp			format,		// format string
    ...					// arguments for 'format'
)
{
    DASSERT(pat);
    DASSERT(pat->file);

    struct obj_t
    {
	wpat_comment_t cm;
	char comment[1000];
    } obj;


    va_list arg;
    va_start(arg,format);
    const int comment_len = vsnprintf(obj.comment,sizeof(obj.comment),format,arg);
    va_end(arg);

    obj.cm.type_size = wpat_calc_size(WPAT_COMMENT,sizeof(wpat_comment_t)+comment_len);
    if ( fwrite(&obj,wpat_get_size(obj.cm.type_size),1,pat->file) != 1 )
	return ERROR1(ERR_WRITE_FAILED,
		"Writing patch header failed: %s",pat->fname);
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			read patch files		///////////////
///////////////////////////////////////////////////////////////////////////////

void SetupReadPatch
(
    ReadPatch_t		* pat		// patch data structure
)
{
    DASSERT(pat);
    memset(pat,0,sizeof(*pat));
}

///////////////////////////////////////////////////////////////////////////////

enumError CloseReadPatch
(
    ReadPatch_t		* pat		// patch data structure
)
{
    DASSERT(pat);

    enumError err = ERR_OK;
    if (pat->file)
    {
	fclose(pat->file);
	pat->file = 0;
    }

    if (pat->fname)
    {
	FreeString(pat->fname);
	pat->fname = 0;
    }

    // [[2do]] clear dynamic data (toc)

    return err;
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenReadPatch
(
    ReadPatch_t		* pat,		// patch data structure
    ccp			filename	// filename of input file
)
{
    DASSERT(pat);
    DASSERT(filename);

    CloseReadPatch(pat);
    FILE * f = fopen(filename,"rb");
    if (!f)
	return ERROR1(ERR_CANT_OPEN,
		"Can't open patch file: %s\n",filename);

    return OpenReadPatchF(pat,f,filename);
}

///////////////////////////////////////////////////////////////////////////////

enumError OpenReadPatchF
(
    ReadPatch_t		* pat,		// patch data structure
    FILE		* file,		// open input file
    ccp			filename	// NULL or known filename
)
{
    DASSERT(pat);
    DASSERT(file);

    CloseReadPatch(pat);
    SetupReadPatch(pat);
    pat->file = file;
    pat->fname = STRDUP( filename ? filename : "?" );

    int readlen = fread(pat->read_buf,1,sizeof(pat->read_buf),pat->file);
    if ( readlen < sizeof(wpat_header_t) )
	goto invalid;

    const wpat_header_t * hd = (wpat_header_t*)pat->read_buf;
    if (memcmp(hd->magic,wpat_magic,sizeof(hd->magic)))
	goto invalid;

    pat->cur_type   = hd->type_size.type;
    pat->cur_size   = wpat_get_size(hd->type_size);
    if ( pat->cur_type != WPAT_HEADER )
	goto invalid;

    pat->is_valid   = true;
    pat->version    = ntohl(hd->version);
    pat->compatible = ntohl(hd->compatible);
    SupportedVersionReadPatch(pat,true);
    return ERR_OK;

 invalid:
    return ERROR0(ERR_INVALID_FILE,"Invalid patch file: %s\n",pat->fname);

}

///////////////////////////////////////////////////////////////////////////////

enumError SupportedVersionReadPatch
(
    ReadPatch_t		* pat,		// patch data structure
    bool		silent		// true: suppress error message
)
{
    DASSERT(pat);
    pat->is_compatible = WIT_PATCH_VERSION >= pat->compatible
			&& pat->version >= WIT_PATCH_READ_COMPATIBLE;
    if (!pat->is_compatible)
    {
	if (silent)
	    return ERR_INVALID_VERSION;

	if ( WIT_PATCH_READ_COMPATIBLE < WIT_PATCH_VERSION )
	    return ERROR0(ERR_INVALID_VERSION,
		"Patch version %s not supported (compatible %s .. %s): %s\n",
		PrintVersion(0,0,pat->version),
		PrintVersion(0,0,WIT_PATCH_READ_COMPATIBLE),
		PrintVersion(0,0,WIT_PATCH_VERSION),
		pat->fname );

	return ERROR0(ERR_INVALID_VERSION,
		"Patch version %s not supported (%s expected): %s\n",
		PrintVersion(0,0,pat->version),
		PrintVersion(0,0,WIT_PATCH_VERSION),
		pat->fname );
    }

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError GetNextReadPatch
(
    ReadPatch_t		* pat		// patch data structure
)
{
    DASSERT(pat);

    pat->cur_type = 0;
    pat->cur_size = 0;
    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			     E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

