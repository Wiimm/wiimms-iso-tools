
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

#ifndef DCLIB_NETWORK_H
#define DCLIB_NETWORK_H 1

#include "dclib-debug.h"
#include "dclib-system.h"
#include "dclib-basics.h"
#include "dclib-file.h"

#include <stddef.h>

//
///////////////////////////////////////////////////////////////////////////////
///////////////			definitions			///////////////
///////////////////////////////////////////////////////////////////////////////

#define GOOD_SOCKADR_SIZE sizeof(struct sockaddr_un)

//
///////////////////////////////////////////////////////////////////////////////
///////////////			host name support		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[NetworkHost_t]]

typedef struct NetworkHost_t
{
    bool	ip4_valid;	// true: 'ip4' is valid
    u32		ip4;		// ip4 or 0 if invalid
    int		port;		// port number, <0: invalid
    ccp		name;		// NULL or pointer to canonicalized name
    struct sockaddr_in sa;	// setup for AF_INET if 'ip4_valid'

    ccp		filename;	// not NULL: unix filename detected
				//	in this case: ip4_valid := FALSE
    mem_t	not_scanned;	// (0,0) or not by ResolveHost*() scanned name
}
NetworkHost_t;

//-----------------------------------------------------------------------------

static inline void InitializeHost ( NetworkHost_t *host )
	{ memset(host,0,sizeof(*host)); }

static inline void ResetHost ( NetworkHost_t *host )
	{ FREE((char*)host->name); memset(host,0,sizeof(*host)); }

static inline ccp GetHostName ( NetworkHost_t *host )
	{ return host->name ? host->name : PrintIP4(0,0,host->ip4,-1); }

//-----------------------------------------------------------------------------

bool ResolveHost
(
    // returns TRUE (=ip4_valid), if a hostname is detected
    // if result is FALSE => check host->filename

    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	init_host,	// true: initialize 'host'
    ccp		name,		// name to analyze
    int		default_port,	// use this as port, if no other is found
    bool	check_filename,	// true: check for unix filename
    bool	store_name	// true: setup 'host->name'
);

bool ResolveHostMem
(
    // returns TRUE (=ip4_valid), if a hostname is detected
    // if result is FALSE => check host->filename

    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	init_host,	// true: initialize 'host'
    mem_t	name,		// name to analyze
    int		default_port,	// use this as port, if no other is found
    bool	check_filename,	// true: check for unix filename
    bool	store_name	// true: setup 'host->name'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum PrintModeIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[PrintModeIP_t]]

typedef enum PrintModeIP_t
{
    // characters for ScanPrintModeIP(): nria
    PMIP_NEVER		= 0x000,  ///< '/Bits' unterdrücken.
    PMIP_RELEVANT	= 0x001,  ///< '/Bits' nur falls relevant.
    PMIP_IF_SET		= 0x002,  ///< '/Bits' nur falls gesetzt.
    PMIP_ALWAYS		= 0x003,  ///< '/Bits' immer ausgeben.
     PMIP_M_BITS	= 0x003,  ///< Maske für Bit-Modi.

    // characters for ScanPrintModeIP(): psmb
    PMIP_PORT		= 0x004,  ///< Flag: Print port if >0.
    PMIP_SERVICE	= 0x008,  ///< Flag: Print service instead of port number.
    PMIP_MASK		= 0x010,  ///< Flag: Print netmask instead of bits for IPv4.
    PMIP_BRACKETS	= 0x020,  ///< Flag: Print bracktes if [IPv6].

    // characters for ScanPrintModeIP(): f
    PMIP_FULL_IPV6	= 0x100,  ///< Print IPv6 without short numbers.

    // characters for ScanPrintModeIP(): 0123
    PMIP_0DOTS		= 0x200,  ///< Print IPv4 as single number (highest priority).
    PMIP_1DOT		= 0x400,  ///< Print IPv4 as "1.2" (A-Class notation).
    PMIP_2DOTS		= 0x800,  ///< Print IPv4 as "1.2.3" (B-Class notation).
    PMIP_3DOTS		=     0,  ///< Print IPv4 as "1.2.3.4" (C-Class notation, default).
     PMIP_M_DOTS	= 0xc00   ///< Maske für Dot-Modi.
}
PrintModeIP_t;

PrintModeIP_t ScanPrintModeIP ( PrintModeIP_t base, ccp arg );
ccp PrintPrintModeIP ( PrintModeIP_t val, bool align );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum IPClass_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[IPClass_t]]

typedef enum IPClass_t
{
    IPCL_INVALID,	// 0.0.0.0					::
    IPCL_LOOPBACK,	// 127.0/8					::1
    IPCL_LOCAL,		// 10.0/8 172.16/12 192.168/16 169.254/16	fe80::/10
    IPCL_STANDARD,	// *						*
    IPCL_SPECIAL,	// 224.0.0.0/3					f00::/8
    IPCL__N,
}
IPClass_t;

//-----------------------------------------------------------------------------

ccp GetIPClassColor ( const ColorSet_t *colset, IPClass_t ipc );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct SplitIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[SplitIP_t]]

typedef struct SplitIP_t
{
    int		bits;
    u16		port;
    char	name[260];
}
SplitIP_t;

//-----------------------------------------------------------------------------

bool SplitIP ( SplitIP_t *sip, ccp addr );

static inline void NormalizeNameSIP ( SplitIP_t *sip )
	{ DASSERT(sip); NormalizeIP4(sip->name,sizeof(sip->name),true,sip->name,-1); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct NamesIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[NamesIP_t]]

typedef struct NamesIP_t
{
    ccp binaddr;
    ccp ipvers;
    ccp addr1;
    ccp addr2;
    ccp bits;
    ccp mask1;
    ccp mask2;
}
NamesIP_t;

//-----------------------------------------------------------------------------

// reset if src==NULL
void SetupNamesIP ( NamesIP_t *dest, const NamesIP_t *src );
void SetupNamesByListIP ( NamesIP_t *dest, const exmem_list_t *src );

ccp PrintNamesIP ( const NamesIP_t *nip );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct BinIP_t			///////////////
///////////////////////////////////////////////////////////////////////////////

enum
{
    IP_SIZE_ADDR	= 39,	///< Maximale Zeichenlänge einer Adresse ohne Maske.
    IP_SIZE_ADDR_MASK	= 43,	///< Maximale Zeichenlänge einer Adresse mit Maske.
    IP_SIZE_PREFIX	= 22,	///< Maximale Zeichenlänge einer Adresse, die mit MaskPrefixMIP() gekürzt wurde.
    IP_SIZE_ADDR_BIN	= 22,	///< Size für eine binäre IP (ohne Maske).

    IP_SIZE_BINIP_C	= 20,	///< Size für binäres Tupel (padding,ipvers,bits,addr).
    IP_SIZE_BINIP	= 18,	///< Size für binäres Tupel (ipvers,bits,addr).
    IP_OFFSET_BINIP	=  2,	///< Offset für DB-Transfer.
};

///////////////////////////////////////////////////////////////////////////////
// [[BinIP_t]]

typedef struct BinIP_t
{
    u16 user_id;		// any id defined by user
    u8	ipvers;			// 0:invalid, 4:IPv4, 6:IPv6
    u8	bits;			// number of 1-bits in netmask

    union			// different views of ip address
    {
	u8	ip6_8[16];
	u16	ip6_16[8];
	u32	ip6_32[4];
	u64	ip6_64[2];
	ipv6_t	ip6;

	struct
	{
	    ipv4_t ip4_padding[3];
	    ipv4_t ip4;
	};

	struct
	{
	    ipv4_t  ip4x_padding[3];
	    ipv4x_t ip4x;
	};
    };
}
__attribute__ ((packed)) BinIP_t;

_Static_assert( offsetof(BinIP_t,ipvers) == IP_OFFSET_BINIP, "wrong order of BinIP_t members" );
_Static_assert( sizeof(BinIP_t) == IP_SIZE_BINIP_C, "wrong size of BinIP_t" );

//-----------------------------------------------------------------------------

static inline u8 * GetCorePtrBIP ( const BinIP_t *src ) { return (u8*)&src->ipvers; }
static inline uint GetCoreSizeBIP ( const BinIP_t *src ) { return IP_SIZE_BINIP; }

BinIP_t GetIPMask ( const BinIP_t *src );
void ApplyIPMask ( BinIP_t *bip );

IPClass_t GetIPClassBIP ( const BinIP_t * bip );
static inline ccp GetColorBIP( const ColorSet_t *colset, const BinIP_t * bip )
	{ return GetIPClassColor(colset,GetIPClassBIP(bip)); }

ccp PrintBIP ( const BinIP_t * bip );

// returns false, if !a1 || !a2 || invalid || differ_after_mask
// use_mask: bit 0 == 1 : use mask of a1; bit 1 == 2 : use mask of a2 
bool IsSameBIP ( const BinIP_t *a1, const BinIP_t *a2, uint use_mask );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct BinIPList_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[BinIPItem_t]]

typedef struct BinIPItem_t
{
    ccp		name;	// optional name, alloced
    BinIP_t	bip;	// IP data
}
BinIPItem_t;

///////////////////////////////////////////////////////////////////////////////
// [[BinIPList_t]]

typedef struct BinIPList_t
{
    BinIPItem_t	*list;	// list of elements
    uint	used;	// number of used elements in 'list'
    uint	size;	// number of allocated elements in 'list'
}
BinIPList_t;

///////////////////////////////////////////////////////////////////////////////
// [[BinIPIterate_t]]

typedef struct BinIPIterate_t
{
    bool		valid;		// iterator is valid
    const BinIPList_t	*bil;		// list data
    const BinIPItem_t	*cur;		// current element

    BinIP_t		addr;		// address to serch
    uint		use_mask;	// flags for IsSameBIP()
}
BinIPIterate_t;

///////////////////////////////////////////////////////////////////////////////

static inline void InitializeBIL ( BinIPList_t *bil )
	{ DASSERT(bil); memset(bil,0,sizeof(*bil)); }

void ResetBIL ( BinIPList_t *bil );
void ClearBIL ( BinIPList_t *bil );

void SetSizeBIL ( BinIPList_t *bil, int size );
void SetMinSizeBIL ( BinIPList_t *bil, int min_size );
BinIPItem_t * CreateItemBIL ( BinIPList_t *bil, int n_item );

#if 0 // not implemented
BinIPItem_t * InsertBIL ( BinIPList_t *bil, int index, ccp key, CopyMode_t copy_mode );
BinIPItem_t * InsertByKeyBIL ( BinIPList_t *bil, ccp key, CopyMode_t copy_mode );
static inline BinIPItem_t * Append ( BinIPList_t *bil, int index, ccp key, CopyMode_t copy_mode )
	{ return InsertBIL(bil,INT_MAX,key,copy_mode); }

int FindIndexBIL ( const BinIPList_t *bil, ccp key );
#endif

///////////////////////////////////////////////////////////////////////////////

// bits for use_mask:
//	bit 0 == 1 : use mask of BinIPList_t::items
//	bit 1 == 2 : use mask of addr 

BinIPIterate_t FindAddressBIL ( const BinIPList_t *bil, BinIP_t *addr, uint use_mask );
void FindNextAddressBIL ( BinIPIterate_t *iter );

uint ScanInterfaceAddresses ( BinIPList_t *bil, bool init_bil );
const BinIPList_t * GetInterfaceAddressList ( bool update );

bool IsLocalBIL ( BinIP_t *addr, uint use_mask );
static inline bool IsLocalAddress ( BinIP_t *addr ) { return IsLocalBIL(addr,2); }
static inline bool IsLocalNetwork ( BinIP_t *addr ) { return IsLocalBIL(addr,3); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct ManageIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ManageIP_t]]

typedef struct ManageIP_t
{
    BinIP_t	bin;		// binary data
    u16		port;		// 0: not specified
    bool	have_bits;	// true: bits were specified on input
    u8		relevant_bits;	// number of relevant bits
    ccp		name;		// NULL or getnameinfo(), alloced
    ccp		decoded_name;	// NULL or punycode-decoded 'name' if differ, alloced
}
ManageIP_t;

///////////////////////////////////////////////////////////////////////////////

static inline bool IsIPv4MIP ( const ManageIP_t *mip )
	{ return mip && mip->bin.ipvers == 4; }

static inline bool IsIPv6MIP ( const ManageIP_t *mip )
	{ return mip && mip->bin.ipvers == 6; }

//-----------------------------------------------------------------------------
// if bits==-1: ignore parameter

void ResetMIP ( ManageIP_t *mip );	// mip can be NULL
void ClearNameMIP ( ManageIP_t *mip );	// mip can be NULL

bool AssignMIP ( ManageIP_t *dest, const ManageIP_t *src );
bool AssignBinMIP ( ManageIP_t *mip, cvp bin, int bits );
bool AssignSockaddrMIP ( ManageIP_t *mip, cvp sockaddr, u16 port, int bits );

bool ScanMIP ( ManageIP_t *mip, ccp addr );

ManageIP_t GetCopyMIP ( const ManageIP_t *mip );
ManageIP_t GetSockNameMIP ( int sock );
ManageIP_t GetPeerNameMIP ( int sock );

// protocol: protocol to use: 0=all, -1=default (IPPROTO_TCP at the moment)
ManageIP_t GetAddrInfoMIP ( ccp addr, int protocol, IpMode_t ipm );

sockaddr_in46_t GetSockaddrMIP ( const ManageIP_t *mip );
ccp GetNameInfoMIP ( ManageIP_t *mip );

void ApplyMaskMIP ( ManageIP_t *mip, int bits );
void MaskPrefixMIP ( ManageIP_t *mip ); // includes ApplyMaskMIP()

//-----------------------------------------------------------------------------

static inline bool PrintBitsMIP ( const ManageIP_t *mip, PrintModeIP_t pmode )
{
    DASSERT(mip);
    pmode &= PMIP_M_BITS;
    return pmode > PMIP_NEVER
	&& (  mip->bin.bits < mip->relevant_bits
	   || pmode == PMIP_IF_SET && mip->have_bits
	   || pmode >= PMIP_ALWAYS
	   );
}

ccp PrintAddrMIP ( const ManageIP_t *mip, PrintModeIP_t pmode );
ccp PrintVersAddrMIP ( const ManageIP_t *mip, PrintModeIP_t pmode );

static inline IPClass_t GetIPClassMIP ( const ManageIP_t * mip )
	{ DASSERT(mip); return GetIPClassBIP(&mip->bin); }

//-----------------------------------------------------------------------------

ccp GetSqlSetAddrMIP ( const ManageIP_t *mip, const NamesIP_t *names );
ccp GetSqlSetMaskMIP ( const ManageIP_t *mip, const NamesIP_t *names );
ccp GetSqlSetBinMIP  ( const ManageIP_t *mip, const NamesIP_t *names );

ccp GetSqlCondAddrMIP ( const ManageIP_t *mip, const NamesIP_t *names );
ccp GetSqlCondMaskMIP ( const ManageIP_t *mip, const NamesIP_t *names );
ccp GetSqlCondBinMaskMIP ( const ManageIP_t *mip, const NamesIP_t *names );

static inline ccp GetSqlCondBinAddrMIP ( const ManageIP_t *mip, const NamesIP_t *names )
	{ return GetSqlSetBinMIP(mip,names); }

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct ResolveIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ResolveIP_t]]

typedef struct ResolveIP_t
{
    IpMode_t	ip_mode;	// used mode
    ManageIP_t	mip4;		// resolved IPv4
    ManageIP_t	mip6;		// resolved IPv6
    ManageIP_t	*mip_list[3];	// ordered and 0-termiated list of valid 'mip*'
}
ResolveIP_t;

//-----------------------------------------------------------------------------

void ResetRIP ( ResolveIP_t *rip );	 // rip can be NULL
void ClearNamesRIP ( ResolveIP_t *rip ); // rip can be NULL

ResolveIP_t GetCopyRIP ( const ResolveIP_t *rip );
void AssignRIP ( ResolveIP_t *dest, const ResolveIP_t *src );

int ScanRIP
(
    // return 0..2 = number of records

    ResolveIP_t	*rip,		// valid data, ResetRIP()
    ccp		addr,		// address to scan
    int		protocol,	// protocol to use: 0=all, -1=default (IPPROTO_TCP at the moment)
    IpMode_t	ip_mode,	// IPv4 and/or IPV6, order of 'mip_list'
    bool	get_names	// true: call GetNameInfoMIP()
);

// not implemented yet
// void DumpRIP ( FILE *f, int indent, const ResolveIP_t *rip );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct AllowIP4_t		///////////////
///////////////////////////////////////////////////////////////////////////////

// standard values
#define ALLOW_MODE_DENY		0	// access denied
#define ALLOW_MODE_ALLOW	0x0001	// access allowed
#define ALLOW_MODE_AKEY		0x0002	// access allowed by access key

// locations
#define ALLOW_MODE_EXTERN	0x0004	// keyword 'EXTERN' set
#define ALLOW_MODE_LAN		0x0008	// keyword 'LAN' set
#define ALLOW_MODE_LOCAL	0x0010	// keyword 'LOCAL' set

// users
#define ALLOW_MODE_PUBLIC	0x0020	// keyword 'PUBLIC' set
#define ALLOW_MODE_USER		0x0040	// keyword 'USER' set
#define ALLOW_MODE_MOD		0x0080	// keyword 'MOD' or 'MODERATOR' set
#define ALLOW_MODE_ADMIN	0x0100	// keyword 'ADMIN' or 'ADMINISTRATOR' set
#define ALLOW_MODE_DEVELOP	0x0200	// keyword 'DEVELOP' or 'DEVELOPER' set

// logging
#define ALLOW_MODE_LOG		0x0400	// keyword 'LOG' set
#define ALLOW_MODE_VERBOSE	0x0800	// keyword 'VERBOSE' set

// context
#define ALLOW_RELEASE		0x1000	// accept this record if neither DEBUG nor IS_DEVELOP is set
#define ALLOW_DEBUG		0x2000	// accept this record if DEBUG or IS_DEVELOP is set

// special processing values
#define ALLOW_MODE_NOT		0x1000000000000000ull	// negate bits
#define ALLOW_MODE_SET		0x2000000000000000ull	// set bits, ignore AND
#define ALLOW_MODE_AND		0x4000000000000000ull	// use AND operation (otherwise OR)
#define ALLOW_MODE_CONTINUE	0x8000000000000000ull	// continue search (otherwise break)

#define ALLOW_MODE__OP		0x6000000000000000ull	// mask for operation
#define ALLOW_MODE__MASK	0x0fffffffffffffffull	// mask out control bits

//-----------------------------------------------------------------------------

extern const KeywordTab_t AllowIP4KeyTable[];

//-----------------------------------------------------------------------------
// [[AllowIP4Item_t]]

typedef struct AllowIP4Item_t
{
    u32		addr;		// IP address
    u32		mask;		// netmask
    u64		mode;		// new mode, see ALLOW_MODE_*
    u64		count;		// incremented on IP match
}
AllowIP4Item_t;

//-----------------------------------------------------------------------------
// [[AllowIP4_t]]

typedef struct AllowIP4_t
{
    AllowIP4Item_t	*list;		// list of addresses, alloced
    uint		used;		// number of used elements
    uint		size;		// number of alloced elements

    int			ref_counter;	// reference counter

    // for automatic usage
    u64			fallback_mode;	// mode if IP not found
    u64			allow_mode;	// allow, if any of these bits is set
}
AllowIP4_t;

///////////////////////////////////////////////////////////////////////////////

void InitializeAllowIP4 ( AllowIP4_t *ai );
void ResetAllowIP4 ( AllowIP4_t *ai );
void ClearAllowIP4 ( AllowIP4_t *ai );

// if ai==NULL: create a new ai
AllowIP4_t * NewReferenceAllowIP4 ( AllowIP4_t *ai );

//returns always NULL
AllowIP4_t * DeleteReferenceAllowIP4 ( AllowIP4_t *ai );

void DumpAllowIP4
(
    FILE		*f,		// NULL or output file
    int			indent,		// indention
    const AllowIP4_t	*ai,		// NULL or source
    const KeywordTab_t	*keytab,	// NULL or key table for output
    ccp			title,		// not NULL: Add title
    bool		print_tab_head	// true: add table headings and separators
);

//-----------------------------------------------------------------------------

enumError ScanLineAllowIP4
(
    AllowIP4_t		*ai,	// valid structure; new elements are appended
    mem_t		line,	// single line to analyse
    const KeywordTab_t	*tab	// NULL or keyword table.
				// If NULL, a local table with keywords 0 DENY
				// ALLOW SET REMOVE AND OR and CONTINUE is used.
);

enumError ScanFileAllowIP4
(
    AllowIP4_t		*ai,	// valid structure; new elements are appended
    ccp			fname,	// ffilename of the source
    FileMode_t		fmode,	// flags for OpenFile()
    const KeywordTab_t	*tab	// NULL or keyword table -> see ScanLineAllowIP4()
);

//-----------------------------------------------------------------------------

void ResetCountersAllowIP4
(
    AllowIP4_t		*ai		// NULL or dest for addition
);

void AddCountersAllowIP4
(
    AllowIP4_t		*dest,		// NULL or dest for addition
    const AllowIP4_t	*src		// NULL or source for addition
);

//-----------------------------------------------------------------------------

// if found, result is masked by ALLOWIP4_MODE__MASK
u64 GetAllowIP4ByAddr ( const AllowIP4_t *ai, u32   addr, u64 if_not_found );
u64 GetAllowIP4ByName ( const AllowIP4_t *ai, mem_t name, u64 if_not_found );

// if ai==NULL => return ALLOW_MODE_ALLOW
// if found, result is masked by ALLOWIP4_MODE__MASK and by 'ai->allow_mode'
// if not found, 'ai->fallback_mode' is returned.
u64 GetAutoAllowIP4ByAddr ( const AllowIP4_t *ai, u32   addr );
u64 GetAutoAllowIP4ByName ( const AllowIP4_t *ai, mem_t name );
u64 GetAutoAllowIP4BySock ( const AllowIP4_t *ai, int   sock );

//-----------------------------------------------------------------------------

void SaveCurrentStateAllowIP4
(
    FILE		*f,		// output file
    ccp			section,	// not NULL: create own section
    ccp			name_prefix,	// NULL or prefix of name
    uint		tab_pos,	// tab pos of '='
    const AllowIP4_t	*ai		// valid struct
);

//-----------------------------------------------------------------------------

bool ScanAllowIP4Item
(
    // return true on success; counter is optional

    AllowIP4Item_t	*it,	// store data here, cleared first
    ccp			line	// NULL or line to scan
);

void RestoreStateAllowIP4
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

void RestoreStateAllowIP4Ex
(
    RestoreState_t	*rs,		// info data, modified
    AllowIP4_t		*ai,		// valid struct
    ccp			name_prefix	// NULL or prefix of name
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			TCP + UDP connections		///////////////
///////////////////////////////////////////////////////////////////////////////

int ConnectByHost
(
    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    int		type,		// AF_INET type, e.g. SOCK_STREAM, SOCK_DGRAM
    int		protocol,	// AF_INET protocol, e.g. IPPROTO_TCP, IPPROTO_UDP
    bool	silent		// true: suppress error messages
);

///////////////////////////////////////////////////////////////////////////////

int ConnectByHostTCP
(
    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	silent		// true: suppress error messages
);

int ConnectTCP
(
    ccp		name,		// optional service + server + optional host
				//   e.g.: "service://host:port/....."
    int		default_host,	// default host
    bool	silent		// true: suppress error messages
);

int ConnectNumericTCP
(
    // like ConnectTCP(), but without name resolution (only numeric ip+port)

    ccp		addr,		// TCP address: ['tcp':] IPv4 [:PORT]
    int		default_port,	// NULL or default port, if not found in 'addr'
    bool	silent		// true: suppress error messages
);

extern int (*ConnectTCP_Hook)
(
    // if set: ConnectNumericTCP() uses ConnectTCP()

    ccp		addr,		// TCP address: ['tcp':] IPv4 [:PORT]
    int		default_port,	// NULL or default port, if not found in 'addr'
    bool	silent		// true: suppress error messages
);

void Setup_ConnectTCP_Hook ( bool force );

int ConnectUnixTCP
(
    ccp		fname,		// unix socket filename
    bool	silent		// true: suppress error messages
);

///////////////////////////////////////////////////////////////////////////////

int ConnectByHostUDP
(
    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	silent		// true: suppress error messages
);

int ConnectUDP
(
    ccp		name,		// optional service + server + optional host
				//   e.g.: "service://host:port/....."
    int		default_host,	// default host
    bool	silent		// true: suppress error messages
);

///////////////////////////////////////////////////////////////////////////////

int WillReceiveNotBlock
(
    // returns: -1:error, 0:may block, 1:will not block

    int		fd,		// file handle of source
    uint	msec		// timeout: 0:no timeout, >0: milliseconds
);

int WillSendNotBlock
(
    // returns: -1:error, 0:may block, 1:will not block

    int		fd,		// file handle of source
    uint	msec		// timeout: 0:no timeout, >0: milliseconds
);

///////////////////////////////////////////////////////////////////////////////

ssize_t ReceiveTimeout
(
    // returns: <0: on error, 0:timeout, >0: bytes read

    int		fd,		// file handle of source
    void	*buf,		// valid pointer to buffer
    uint	size,		// size to receive
    int		flags,		// flags for recv()
    int		msec		// timeout: -1:unlimited, 0:no timeout, >0: milliseconds
);

ssize_t SendTimeout
(
    // returns: <0: on error, 0:timeout, >0: bytes written

    int		fd,		// file hande of destination
    const void	*data,		// valid pointer to data
    uint	size,		// size to receive
    int		flags,		// flags for send()
    int		msec,		// total timeout: -1:unlimited, 0:no timeout, >0: milliseconds
    bool	all		// true: send all data until done, total timeout or error
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			IP data structures		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[ether_head_t]]

typedef struct ether_head_t
{
    // http://en.wikipedia.org/wiki/Ethertype

    u8		mac_dest[6];
    u8		mac_src[6];
    u16		ether_type;
}
__attribute__ ((packed)) ether_head_t;

//-----------------------------------------------------------------------------
// [[ether_head_vlan_t]]

typedef struct ether_head_vlan_t
{
    // http://en.wikipedia.org/wiki/Ethertype

    u8		mac_dest[6];
    u8		mac_src[6];
    u16		vlan_tag_type;	// 0x8100 for VLAN
    u16		vlan_param;	// Bits 0-11: ID / Bits 12-15: flags
    u16		ether_type;

    //-- secondary params
    bool	have_vlan;	// true: VLAN trag available
    u8		head_size;	// >0: size of ethernet header (local endian): 14|18
}
__attribute__ ((packed)) ether_head_vlan_t;

//-----------------------------------------------------------------------------
// [[arp_head_t]]

typedef struct arp_head_t
{
 // http://de.wikipedia.org/wiki/Address_Resolution_Protocol

 /* 0x00 */	u16 hardware_addr_type;		// always 0x0001 for IPv4
 /* 0x02 */	u16 protocol_addr_type;		// always 0x8000 for IPv4
 /* 0x04 */	u8  mac_size;			// always 6 for IPv4
 /* 0x05 */	u8  addr_size;			// always 4 for IPv4
 /* 0x06 */	u16 operation;
 /* 0x08 */	u8  src_mac[6];			// MAC  of source
 /* 0x0e */	u32 src_ip;			// IPv4 of source
 /* 0x12 */	u8  dest_mac[6];		// MAC  of destination
 /* 0x18 */	u32 dest_ip;			// IPv4 of destination
 /* 0x1c */
}
__attribute__ ((packed)) arp_head_t;

//-----------------------------------------------------------------------------
// [[ip4_head_t]]

typedef struct ip4_head_t
{
    // http://de.wikipedia.org/wiki/IP-Paket

 /*00*/	u8	vers_ihl;	// high nibble: version, low nibble: IHL (Ip Header Len)
 /*01*/	u8	tos;
 /*02*/	u16	total_len;
 /*04*/	u16	id;
 /*06*/	u16	frag_off;	// low 12 bits: fragment offset, high 4 bits: flags
 /*08*/	u8	ttl;
 /*09*/	u8	protocol;
 /*0a*/	u16	checksum;
 /*0c*/	u32	ip_src;
 /*10*/	u32	ip_dest;
 /*14*/	// optional: options + padding
}
__attribute__ ((packed)) ip4_head_t;

//-----------------------------------------------------------------------------
// [[udp_head_t]]

typedef struct udp_head_t
{
    // http://de.wikipedia.org/wiki/User_Datagram_Protocol#UDP-Datagramm

    u16		port_src;
    u16		port_dest;
    u16		data_len;
    u16		checksum;
}
__attribute__ ((packed)) udp_head_t;

//-----------------------------------------------------------------------------
// [[udp_packet_t]]

// ethernet packet limit is 1500
#define MAX_UDP_PACKET_DATA ( 1500 - sizeof(ip4_head_t) - sizeof(udp_head_t) )

typedef struct udp_packet_t
{
    ip4_head_t	ip4;
    udp_head_t	udp;
    u8		data[MAX_UDP_PACKET_DATA];
}
__attribute__ ((packed)) udp_packet_t;

//-----------------------------------------------------------------------------
// [[tcp_flags_t]]

typedef enum tcp_flags_t
{
	TCP_F_FINISH	= 0x01,
	TCP_F_SYNC	= 0x02,
	TCP_F_RESET	= 0x04,
	TCP_F_PSH	= 0x08,
	TCP_F_ACK	= 0x10,
	TCP_F_URGENT	= 0x20,
	TCP_M_RESERVED	= 0xc0,
}
__attribute__ ((packed)) tcp_flags_t;

//-----------------------------------------------------------------------------
// [[tcp_head_t]]

typedef struct tcp_head_t
{
    // http://de.wikipedia.org/wiki/Transmission_Control_Protocol#Aufbau_des_TCP-Headers

    u16		port_src;
    u16		port_dest;
    u32		seq_num;
    u32		acc_num;
    u8		data_off;	// bit 4-7 only, multiply it by 4
    u8		flags;		// bit field (think big endian) -> tcp_flags_t
    u16		window;
    u16		checksum;
    u16		urgent;
    u8		data[];
}
__attribute__ ((packed)) tcp_head_t;

//-----------------------------------------------------------------------------

bool fix_ether_head_vlan  ( ether_head_vlan_t *head );
void ntoh_ether_head_vlan ( ether_head_vlan_t *dest, const void *src );
void ntoh_ether_head	  ( ether_head_t      *dest, const void *src );
void ntoh_ip4_head	  ( ip4_head_t        *dest, const void *src );
void ntoh_udp_head	  ( udp_head_t        *dest, const void *src );
void ntoh_tcp_head	  ( tcp_head_t        *dest, const void *src );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			IP interface			///////////////
///////////////////////////////////////////////////////////////////////////////

char * ScanNumericIP4
(
    // returns next unread character or NULL on error

    ccp		addr,		// address to scan
    u32		*r_ipv4,	// not NULL: store result here (local endian)
    u32		*r_port,	// not NULL: scan port too (local endian)
    uint	default_port	// return this if no port found
);

//-----------------------------------------------------------------------------

mem_t ScanNumericIP4Mem
(
    // returns unread character or NullMem on error

    mem_t	addr,		// address to scan
    u32		*r_ipv4,	// not NULL: store result here (local endian)
    u32		*r_port,	// not NULL: scan port too (local endian)
    uint	default_port	// return this if no port found
);

//-----------------------------------------------------------------------------

// Returns a NETWORK MASK "/a.b.c.d" or as CIDR number "/num" between 0 and 32.
// An optional slash '/' at the beginning is skipped.
// Returns modified 'source' if a MASK or CDIR is detected.
// If no one is detected, source is unmodified and returned mask = ~0.

mem_t ScanNetworkMaskMem ( u32 *mask, mem_t source );

//-----------------------------------------------------------------------------

u16 CalcChecksumIP4
(
    const void		*data,	// ip4 header
				// make sure, that the checksum member is NULL
    uint		size	// size of ip4 header
);

//-----------------------------------------------------------------------------

u16 CalcChecksumUDP
(
    const ip4_head_t	*ip4,	// IP4 header
    const udp_head_t	*udp,	// UDP header
    const void		*data	// UDP data, size is 'udp->data_len'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			UNIX sockets			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SendUnixUDP
(
    ccp		path1,		// NULL or part #1 of path
    ccp		path2,		// NULL or part #2 of path
    bool	silent,		// suppress all error messages
    cvp		data,		// data to send
    uint	size		// size of 'data'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Transfer Statistics		///////////////
///////////////////////////////////////////////////////////////////////////////
// [[TransferStats_t]]

typedef struct TransferStats_t
{
    u32 conn_count;	// number of connections
    u32 recv_count;	// number of received packets
    u64 recv_size;	// total size of received packets
    u32 send_count;	// number of send packets
    u64 send_size;	// total size of send packets
}
TransferStats_t;

//-----------------------------------------------------------------------------

static inline void InitializeTransferStats ( TransferStats_t *ts )
	{ DASSERT(ts); memset(ts,0,sizeof(*ts)); }

//-----------------------------------------------------------------------------

TransferStats_t * Add2TransferStats
(
    // return dest
    // calculate: dest += src

    TransferStats_t		*dest,	// NULL or destination and first source
    const TransferStats_t	*src	// NULL or second source
);

TransferStats_t * Sub2TransferStats
(
    // return dest
    // calculate: dest -= src

    TransferStats_t		*dest,	// NULL or destination and first source
    const TransferStats_t	*src	// NULL or second source
);

TransferStats_t * Add3TransferStats
(
    // return dest
    // calculate: dest = src1 + src2

    TransferStats_t		*dest,	// NULL or destination (maybe same as source)
    const TransferStats_t	*src1,	// NULL or first source
    const TransferStats_t	*src2	// NULL or second source
);

TransferStats_t * Sub3TransferStats
(
    // return dest
    // calculate: dest = src1 - src2

    TransferStats_t		*dest,	// NULL or destination (maybe same as source)
    const TransferStats_t	*src1,	// NULL or first source
    const TransferStats_t	*src2	// NULL or second source
);

TransferStats_t * SumTransferStats
(
    // return dest
    // calculate: dest = sum(all_sources)

    TransferStats_t		*dest,	// NULL or destination (maybe same as source)
    const TransferStats_t * const *src,	// NULL or source list, elements may be NULL
    int				n_src	// number of element; if -1: term at first NULL
);

ccp PrintTransferStatsSQL
(
    // print statistic as SQL assigning list.
    // arguments are speparated by a comma.

    char			*buf,		// result buffer
						// NULL: use a local circulary static buffer
    size_t			buf_size,	// size of 'buf', ignored if buf==NULL
    const TransferStats_t	*ts,		// valid source
    ccp				prefix		// not NULL: prefix member names
);

//-----------------------------------------------------------------------------

void PrintTranferStatistics
(
    FILE	*f,		// output file, never NULL
    ccp		prefix,		// NULL or prefix for each line
    ccp		name,		// statistics name, up to 5 chars including colon
    const TransferStats_t *stat,// statistics record

    TransferStats_t *prev,	// NULL statistics record with previous data
    u64		duration,	// delta duration in usec
    ccp		duration_info,	// text representation of 'duration'

    const ColorSet_t *colset	// NULL (no colors) or color set to use
);

//-----------------------------------------------------------------------------

void SaveCurrentStateTransferStats
(
    FILE		*f,		// output file
    const TransferStats_t *stat		// valid stat object
);

void SaveCurrentStateTransferStats1
(
    // print as single line
    FILE		*f,		// output file
    ccp			name,		// var name; use "tfer-stat" if NULL or EMPTY
    const TransferStats_t *stat		// valid stat object
);

void SaveCurrentStateTransferStatsN
(
    // print multiple stats each as single line
    FILE		*f,		// output file
    ccp			name,		// var name; use "tfer-stat-" if NULL or EMPTY
    const TransferStats_t *stat,	// list of valid stat objects
    uint		n_stat		// number of elements in 'stat'
);

//-----------------------------------------------------------------------------

void RestoreStateTransferStats
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

void RestoreStateTransferStats1
(
    // scan single line
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    ccp			name,		// var name; use "tfer-stat" if NULL or EMPTY
    TransferStats_t	*stat,		// valid stat object
    bool		fall_back	// true: fall back to RestoreStateTransferStats()
);

uint RestoreStateTransferStatsN
(
    // print multiple stats each as single line
    // returns the number of read elements; all others are set to NULL
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    ccp			name,		// var name; use "tfer-stat-" if NULL or EMPTY
    TransferStats_t	*stat,		// list of valid stat objects
    uint		n_stat		// number of elements in 'stat'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Socket_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// define some TCP call back function types

struct TCPStream_t;
typedef int (*TCPStreamFunc) ( struct TCPStream_t * ts );
typedef int (*TCPIOFunc)     ( struct TCPStream_t * ts, u8 *buf, uint size );
typedef int (*TCPTimeFunc)   ( struct TCPStream_t * ts, u64 now_usec );

typedef enum TCPFDList_t
{
    TCP_FM_ADD_SOCK,	// add sockets to 'fdl'
    TCP_FM_CHECK_SOCK,	// check sockts of 'fdl'
    TCP_FM_TIMEOUT,	// on timeout (fdl is NULL)
}
TCPFDList_t;

typedef int (*TCPFDListFunc)
(
    // if return <0, then don't do standard action with the stream
    struct TCPStream_t	*ts,		// valid TCP stream
    FDList_t		*fdl,		// valid socket list
    TCPFDList_t		mode,		// execution mode
    bool		check_timeout	// true: enable timeout checks
);

struct TCPHandler_t;
typedef struct TCPStream_t * (*TCPCreateFunc) ( struct TCPHandler_t * th, int sock );

///////////////////////////////////////////////////////////////////////////////
// [[Socket_t]]

typedef struct Socket_t
{
    int		sock;			// tcp socket
    uint	poll_index;		// if poll() is used: index of 'poll_list'
    bool	is_unix;		// true: Is a unix file socket
    char	info[23];		// info about this socket, for debugging

    // if defined, these function replace the TCPHandler_t functions.

    TCPCreateFunc OnCreateStream;	// not NULL: called to create
					// an initialize a stream object

    TCPStreamFunc OnAddedStream;	// not NULL: called after the stream
					// is created and added
}
Socket_t;

uint GetSocketNameBySA
	( char *buf, uint bufsize, const struct sockaddr *sa, socklen_t sa_len );
uint GetSocketName ( char *buf, uint bufsize, int sock );
uint GetPeerName ( char *buf, uint bufsize, int sock );
bool DeleteSocketFile ( int sock );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    TCPStream_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[TCPStream_t]]

typedef struct TCPStream_t
{
    //--- double linked list

    struct TCPStream_t *prev;	// link to previous stream, unused for the pool
    struct TCPStream_t *next;	// link to next stream
    struct TCPHandler_t *handler; // related TCP handler


    //--- base data

    uint	unique_id;	// unique id, created by CreateUniqueId()
    int		sock;		// tcp socket
    uchar	protect;	// >0: protect member, don't destroy it
    uchar	auto_close;	// >0: close stream after last byte sent
    uchar	rescan;		// rescan input buffer also without new data
    uchar	eof;		// >0: end of file, bit-0 is set by framework
    uchar	error;		// >0: connection error, bit-0 is set by framework
    uchar	not_socket;	// >0: fd is not a socket! // =0: unknown
    GrowBuffer_t ibuf;		// input buffer
    GrowBuffer_t obuf;		// output buffer
    u64		accept_usec;	// >0: next trigger, not used for timeout
    u64		trigger_usec;	// >0: next timeout and trigger time
    u64		delay_usec;	// >0: delay of trigger after accept_usec
    u64		timeout_usec;	// >0: auto disconnect after inactivity
    u64		allow_mode;	// >0: access allowed with this code
    uint	poll_index;	// if 'use_poll': index of 'poll_list'


    //--- call back functions

    TCPTimeFunc OnMaintenance;	// not NULL: called on maintenance
    TCPTimeFunc OnTimeout;	// not NULL: called on timeout
    TCPIOFunc	OnReceived;	// not NULL: called after read, but before buffer insert
    TCPIOFunc	OnSend;		// not NULL: called after write, but before buffer drop
    TCPTimeFunc	OnClose;	// not NULL: called when the stream is closed
    TCPFDListFunc OnFDList;	// not NULL: Call this for FDList actions

    //--- logging

    TraceLog_t	*tracelog;	// NULL or valid trace-log data

    //--- statistics

    char	info[32];	// info about this stream, for debugging
    u64		connect_usec;	// time of connection, GetTimeUSec(false)
    u64		receive_usec;	// time of last receive, GetTimeUSec(false)
    u64		send_usec;	// time of last send, GetTimeUSec(false)

    TransferStats_t stat;	// transfer statistics
    TransferStats_t *xstat;	// NULL or pointer to summary statistics

    //--- data extension

    u8		data[0];	// user specific data
}
TCPStream_t;

///////////////////////////////////////////////////////////////////////////////

void LogTCPStream
(
    FILE		*f,		// output file
    int			indent,		// indention
    const TCPStream_t	*ts,		// valid TCP handler
    int			recurse,	// >0: print stream list, >1: print buffer status
    ccp			format,		// format string for vfprintf()
    ...					// arguments for 'vfprintf(format,...)'
)
__attribute__ ((__format__(__printf__,5,6)));

//-----------------------------------------------------------------------------

void LogTCPStreamActivity
(
    const TCPStream_t	*ts,		// valid TCP handler
    ccp			activity
);

//-----------------------------------------------------------------------------

char * BufInfoHeadTCPStream
(
    // returns a pointer to the head line

    uint		line		// 0|1
);

#define BUF_INFO_TCP_STREAM_SIZE 120

char * BufInfoTCPStream
(
    // returns a pointer to the buffer

    char		* buf,		// result (BUF_INFO_TCP_STREAM_SIZE bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    const TCPStream_t	*ts,		// valid TCP handler
    u64			now_usec	// NULL or current time, GetTimeUSec(false)
);

//-----------------------------------------------------------------------------

void InitializeTCPStream ( TCPStream_t *ts, int sock );
void ResetTCPStream ( TCPStream_t *ts );
void DestroyTCPStream ( TCPStream_t *ts );

int SendDirectTCPStream
(
    // Send data direct without blocking and without calling any notifier.
    // If output buf is not empty or send failed, append the data to output buf.
    // Returns the number of bytes added to the output buffer or -1 on error.
    // The data is send+stored completely (returns 'size') or not at all
    // (returns '0'). -1 is returned if the socket is invalid.

    TCPStream_t		*ts,		// valid TCP handler
    bool		flush_output,	// true: try to flush out-buf before
    cvp			data,		// data to send
    uint		size		// size of 'data', If NULL && flush: flush only
);

int PrintArgDirectTCPStream
(
    // Printing interface for SendDirectTCPStream()

    TCPStream_t		*ts,		// valid TCP handler
    bool		flush_output,	// true: try to flush out-buf before
    ccp			format,		// format string for vsnprintf()
    va_list		arg		// parameters for 'vfprintf(...,format,...)'
);

int PrintDirectTCPStream
(
    // Printing interface for SendDirectTCPStream()

    TCPStream_t		*ts,		// valid TCP handler
    bool		flush_output,	// true: try to flush out-buf before
    ccp			format,		// format string for vfprintf()
    ...					// arguments for 'vfprintf(...,format,...)'
)
__attribute__ ((__format__(__printf__,3,4)));

//-----------------------------------------------------------------------------

int PrintBinary1TCPStream
(
    TCPStream_t	*ts,		// NULL or destination
    ccp		cmd,		// command name
    cvp		bin_data,	// pointer to binary data
    uint	bin_size,	// size of binary data
    ccp		format,		// NULL or format string with final ';'
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,5,6)));

int PrintBinary2TCPStream
(
    TCPStream_t	*ts,		// NULL or destination
    ccp		cmd,		// command name
    cvp		bin1_data,	// pointer to binary data
    uint	bin1_size,	// size of binary data
    cvp		bin2_data,	// pointer to binary data
    uint	bin2_size,	// size of binary data
    ccp		format,		// NULL or format string with final ';'
    ...				// parameters for 'format'
)
__attribute__ ((__format__(__printf__,7,8)));

//-----------------------------------------------------------------------------

static inline int FlushTCPStream
(
    // Flush the output buffer using SendDirectTCPStream() without data

    TCPStream_t		*ts		// valid TCP handler
)
{
    return SendDirectTCPStream(ts,true,0,0);
}

//-----------------------------------------------------------------------------

ssize_t UpdateRecvStatTCPStream
(
    // returns 'send_stat'

    TCPStream_t		*ts,		// valid TCP handler
    ssize_t		recv_stat,	// result of recv(), update stats on >0
    u64			now_usec	// NULL or current time
);

ssize_t UpdateSendStatTCPStream
(
    // returns 'send_stat'

    TCPStream_t		*ts,		// valid TCP handler
    ssize_t		send_stat,	// result of send(), update stats on >0
    u64			now_usec	// NULL or current time
);

//-----------------------------------------------------------------------------

void AddSocketTCPStream
(
    TCPStream_t		*ts,		// valid TCP handler
    FDList_t		*fdl		// valid file descriptor list
);

void CheckSocketTCPStream
(
    TCPStream_t		*ts,		// valid TCP handler
    FDList_t		*fdl,		// valid socket list
    bool		check_timeout	// true: enable timeout checks
);

//-----------------------------------------------------------------------------

void CheckTimeoutTCPStream ( TCPStream_t *ts );
u64  GetLastActivityTCPStream ( TCPStream_t *ts );
u64  GetTimeoutTCPStream ( TCPStream_t *ts );
int  OnTimeoutTCPStream ( TCPStream_t *ts, u64 now_usec );
void SetupTriggerTCPStream ( TCPStream_t *ts, u64 now_usec );

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    TCPHandler_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef int (*TCPHandlerFunc)
(
    // returns 0 on success, !=0 otherwise

    struct TCPHandler_t	*th,		// valid TCP handler
    const Socket_t	*lsock,		// valid listen-socket
    int			sock		// valid socket to verify
);

typedef bool (*TCPAllowFunc)
(
    // returns true if access is allowed

    struct TCPHandler_t	*th,		// valid TCP handler
    const Socket_t	*lsock,		// valid listen-socket
    int			sock,		// valid socket to verify
    u64			*allow		// not NULL: store allow code here
);

///////////////////////////////////////////////////////////////////////////////
// [[TCPHandler_t]]

#define TCP_HANDLER_MAX_LISTEN 3

typedef struct TCPHandler_t
{
    uint	unique_id;		// unique id, created by CreateUniqueId()
    uint	data_size;		// size of TCPStream_t::data
    uint	max_conn;		// max allowed connections

    TCPStream_t	*first;			// pointer to first active stream
    uint	need_maintenance;	// >0: a stream is ready for maintenance

    TCPAllowFunc   OnAllowStream;	// not NULL: called before allowing a connection
					// initialized with IsStreamAllowed()
    TCPHandlerFunc OnAcceptStream;	// not NULL: called before accpeting a connection
    TCPCreateFunc  OnCreateStream;	// not NULL: called to create
					// and initialize a stream object
    TCPStreamFunc  OnAddedStream;	// not NULL: called after the stream
					// is created and added
    TCPStreamFunc  OnDestroyStream;	// not NULL: called by ResetTCPStream()

    Socket_t listen[TCP_HANDLER_MAX_LISTEN];
					// sockets to listen

    AllowIP4_t	*allow_ip4;		// NULL or filter for accept (not for unix files).
					// Analysis is done by OnAllowStream()
					// before calling OnAcceptStream().

    //--- logging

    TraceLog_t	tracelog;		// trace activities

    //--- statistics

    uint	used_streams;		// number of currently used streams
    uint	max_used_streams;	// number of max used streams
    uint	total_streams;		// total number of used streams

    TransferStats_t stat;		// transfer statistics
}
TCPHandler_t;

///////////////////////////////////////////////////////////////////////////////

void LogTCPHandler
(
    FILE		*f,		// output file
    int			indent,		// indention
    const TCPHandler_t	*th,		// valid TCP handler
    int			recurse,	// >0: print stream list, >1: print buffer status
    ccp			format,		// format string for vfprintf()
    ...					// arguments for 'vfprintf(format,...)'
)
__attribute__ ((__format__(__printf__,5,6)));

//-----------------------------------------------------------------------------

void PrintStreamTableTCPHandler
(
    FILE		*f,		// output file
    const ColorSet_t	*colset,	// NULL (no colors) or valid color set
    int			indent,		// indention
    const TCPHandler_t	*th		// valid TCP handler
);

//-----------------------------------------------------------------------------

void InitializeTCPHandler
(
    TCPHandler_t *th,		// not NULL
    uint	data_size	// size of 'TCPStream_t::data'
);

void ResetTCPHandler
(
    TCPHandler_t *th		// valid TCP handler
);

uint CloseTCPHandler
(
    // returns the number of waiting clients (obuf not empty and !disabled)
    TCPHandler_t *th		// valid TCP handler
);

TCPStream_t * AddTCPStream
(
    TCPHandler_t *th,		// valid TCP handler
    TCPStream_t	 *ts		// the new stream
);

TCPStream_t * CreateTCPStream
(
    TCPHandler_t	*th,		// valid TCP handler
    int			sock,		// related socket
    u64			allow_mode,	// stored in 'allow_mode'
    const Socket_t	*listen		// NULL or related listen object
);

TCPStream_t * ConnectUnixTCPStream
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			path,		// NULL or path part 1 to socket file
    bool		silent		// suppress error messages
);

TCPStream_t * ConnectTCPStream
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			addr,		// address -> NetworkHost_t
    u16			default_port,	// default port
    bool		silent		// suppress error messages
);

TCPStream_t * FindTCPStreamByUniqueID
(
    TCPHandler_t	*th,		// valid TCP handler
    uint		unique_id	// id to search
);

//-----------------------------------------------------------------------------

uint NumOfSocketsTCP ( const TCPHandler_t *th );
Socket_t * GetUnusedListenSocketTCP ( TCPHandler_t *th, bool silent );

//-----------------------------------------------------------------------------

enumError ListenUnixTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			path		// path to unix file
);

enumError ListenTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			addr,		// IPv4 address with optional port -> NetworkHost_t
					// fall back to ListenUnixTCP() if addr begins with
					// 'unix:' or '/' or './ or '../'
    u16			default_port	// default port
);

int UnlistenTCP
(
    // returns the index of the closed socket, or if not found

    TCPHandler_t	*th,		// valid TCP handler
    int			sock		// socket to close
);

uint UnlistenAllTCP
(
    // returns the number of closed sockets

    TCPHandler_t	*th		// valid TCP handler
);

//-----------------------------------------------------------------------------

void AddSocketsTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    FDList_t		*fdl		// valid file descriptor list
);

//-----------------------------------------------------------------------------

bool IsStreamAllowed
(
    // returns true if access is allowed

    struct TCPHandler_t	*th,		// valid TCP handler
    const Socket_t	*lsock,		// valid listen-socket
    int			sock,		// valid socket to verify
    u64			*allow		// not NULL: store allow code here
);

TCPStream_t * OnAcceptStream
(
    TCPHandler_t	*th,		// valid TCP handler
    const Socket_t	*lsock,		// listen-socket
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
);

void OnCloseStream
(
    TCPStream_t		*ts,		// valid TCP stream
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
);

void SetNotSocketStream
(
    TCPStream_t		*ts		// valid TCP stream
);

void OnReceivedStream
(
    TCPStream_t		*ts,		// valid TCP stream
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
);

void OnWriteStream
(
    TCPStream_t		*ts,		// valid TCP stream
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
);

void CheckSocketsTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    FDList_t		*fdl,		// valid socket list
    bool		check_timeout	// true: enable timeout checks
);

void CheckTimeoutTCP ( TCPHandler_t *th );
bool MaintainTCP ( TCPHandler_t *th );

void ManageSocketsTCP
(
    // call  CheckSocketsTCP(), CheckTimeoutTCP(), MaintainTCP()

    TCPHandler_t	*th,		// valid TCP handler
    FDList_t		*fdl,		// valid socket list
    int			stat		// result of a WAIT function
);

///////////////////////////////////////////////////////////////////////////////
// special case: send single data packets

TCPStream_t * SendSingleUnixTCP
(
    TCPHandler_t	*th,		// valid handle
    ccp			path,		// path to socket file

    const void		*data,		// data to send
    uint		size,		// size of 'data'
    u64			timeout_usec,	// timeout before closing the connection
    TransferStats_t	*xstat,		// NULL or pointer to summary statistics

    bool		silent		// suppress error messages
);

TCPStream_t * SendSingleTCP
(
    TCPHandler_t	*th,		// valid handle
    ccp			addr,		// address -> NetworkHost_t
    u16			default_port,	// default port

    const void		*data,		// data to send
    uint		size,		// size of 'data'
    u64			timeout_usec,	// timeout before closing the connection
    TransferStats_t	*xstat,		// NULL or pointer to summary statistics

    bool		silent		// suppress error messages
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			TCP: CommandTCP			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef char * (*CommandTCPLineFunc)
(
    // return first non scanned character, or NULL on error

    TCPStream_t	*ts,		// valid stream data
    char	*line,		// begin of line
    char	*line_end,	// end of line
    u64		now_usec	// NULL or current time
);

///////////////////////////////////////////////////////////////////////////////

typedef enumError (*CommandTCPArgFunc)
(
    TCPStream_t	*ts,		// valid stream data
    int		argc,		// number of arguments in 'argv'
    char	**argv,		// array with 'argc' arguments + a NULL term
    u64		now_usec	// NULL or current time in usec
);

///////////////////////////////////////////////////////////////////////////////

typedef struct CommandTCPInfo_t
{
    //--- base parameters

    bool	comma_is_eol;		// true: comma is 'end of command line'
    u64		timeout_usec;		// wanted timeout, default 10s

    //--- scanning functions, high priority first

    CommandTCPArgFunc	OnScanArg;	// function to scan arguments
    CommandTCPLineFunc	OnScanLine;	// function to scan line

    //--- user specific data extension

    u8		data[0];
}
CommandTCPInfo_t;

///////////////////////////////////////////////////////////////////////////////

int OnCreateCommandTCP ( TCPStream_t *ts );

void SetTimeoutCommandTCP
(
    TCPStream_t	*ts,		// valid stream
    u64		now_usec	// NULL or current time in usec
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SaveCurrentState*()		///////////////
///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateSocket
(
    FILE		*f,		// output file
    const Socket_t	*sock		// valid socket object
);

///////////////////////////////////////////////////////////////////////////////

typedef void (*TCPStreamSaveFunc)
(
    FILE		*f,		// output file
    const TCPStream_t	*ts		// valid TCP stream
);

//-----------------------------------------------------------------------------

void SaveCurrentStateTCPStream
(
    FILE		*f,		// output file
    const TCPStream_t	*ts,		// valid TCP stream
    TCPStreamSaveFunc	func		// NULL or function for 'TCPStream_t.data' extend
);

void SaveCurrentStateTCPHandler
(
    FILE		*f,		// output file
    ccp			sect_name,	// section base name
    const TCPHandler_t	*th,		// valid TCP handler
    TCPStreamSaveFunc	func		// NULL or function for 'TCPStream_t.data' extend
);

void SaveCurrentStateCommandTCP
(
    FILE		*f,		// output file
    const TCPStream_t	*ts		// valid TCP stream
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			RestoreState*()			///////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateSocket
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * RestoreStateTCPStream
(
    // return 'ts' or the new TCPStream_t
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    TCPStream_t		*ts,		// if NULL: create it
    uint		extra_size	// if 'ts' alloced: alloc some byte more
);

///////////////////////////////////////////////////////////////////////////////

void RestoreStateTCPHandler_base
(
    TCPHandler_t *th,			// valid TCPHandler_t
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
);

Socket_t * RestoreStateTCPHandler_socket
(
    TCPHandler_t *th,			// valid TCPHandler_t
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
);

TCPStream_t * RestoreStateTCPHandler_stream
(
    TCPHandler_t *th,			// valid TCPHandler_t
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
);

void RestoreStateTCPHandler
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

///////////////////////////////////////////////////////////////////////////////

void RestoreStateCommandTCP
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			 Linux support			///////////////
///////////////////////////////////////////////////////////////////////////////

#if defined(SYSTEM_LINUX) || defined(__CYGWIN__)
  #include "dclib-network-linux.h"
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_NETWORK_H

