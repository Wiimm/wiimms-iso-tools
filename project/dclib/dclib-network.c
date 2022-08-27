
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

#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <stddef.h>
#include <ifaddrs.h>

#include "dclib-network.h"
#include "dclib-punycode.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__
 #ifndef SOCK_NONBLOCK
    #define SOCK_NONBLOCK 0
 #endif
#endif

#ifndef POLLRDHUP
    #define POLLRDHUP 0
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			host name support		///////////////
///////////////////////////////////////////////////////////////////////////////

bool ResolveHost
(
    // returns TRUE, if a hostname is detected
    // if result is FALSE => check host->filename

    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	init_host,	// true: initialize 'host'
    ccp		name,		// name to analyze
    int		default_port,	// use this as port, if no other is found
    bool	check_filename,	// true: check for unix filename
    bool	store_name	// true: setup 'host->name'
)
{
    return ResolveHostMem( host, init_host, MemByString(name),
				default_port, check_filename, store_name );
}

///////////////////////////////////////////////////////////////////////////////

bool ResolveHostMem
(
    // returns TRUE, if a hostname is detected
    // if result is FALSE => check host->filename

    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	init_host,	// true: initialize 'host'
    mem_t	name,		// name to analyze
    int		default_port,	// use this as port, if no other is found
    bool	check_filename,	// true: check for unix filename
    bool	store_name	// true: setup 'host->name'
)
{
    Setup_ConnectTCP_Hook(false);

    DASSERT(host);
    if (!init_host)
	FREE((char*)host->name);
    InitializeHost(host);
    host->port = default_port;

    if (check_filename)
    {
	mem_t res = CheckUnixSocketPathMem(name,0);
	host->filename = res.ptr;
	if (host->filename)
	     return false;
    }

    char buf[1000];

    ccp ptr = name.ptr;
    ccp end = ptr + name.len;
    if ( ptr < end
	&& ( *ptr >= 'a' && *ptr <= 'z' || *ptr >= 'A' && *ptr <= 'Z' ))
    {
	while ( ptr < end
	      && ( *ptr >= 'a' && *ptr <= 'z'
		|| *ptr >= 'A' && *ptr <= 'Z'
		|| *ptr >= '0' && *ptr <= '9'
		|| *ptr == '_' || *ptr == '-'
		))
	{
	    ptr++;
	}

	uint len = ptr - name.ptr;
	if ( len && len < sizeof(buf) && ptr+1 < end && *ptr == ':' && ptr[1] == '/' )
	{
	    memcpy(buf,name.ptr,len);
	    buf[len] = 0;
	    struct servent *s = getservbyname(buf,0);
	    if (s)
	    {
		const int port = ntohs(s->s_port);
		if ( port > 0 )
		    host->port = port;
	    }

	    ptr += 2;
	    if ( *ptr == '/' )
		ptr++;
	    name = BehindMem(name,ptr);
	    check_filename = false;
	}
	else
	    ptr = name.ptr;
    }

    while ( ptr < end
	      && ( *ptr >= 'a' && *ptr <= 'z'
		|| *ptr >= 'A' && *ptr <= 'Z'
		|| *ptr >= '0' && *ptr <= '9'
		|| *ptr == '_' || *ptr == '-' || *ptr == '.'
		))
    {
	ptr++;
    }

    uint len = ptr - name.ptr;
    if ( len > 0 && len < sizeof(buf) )
    {
	memcpy(buf,name.ptr,len);
	buf[len] = 0;
	if (store_name)
	    host->name = MEMDUP(buf,len);

	struct hostent *h = gethostbyname(buf);
	if ( h && h->h_addrtype == AF_INET && h->h_length == 4 )
	{
	    host->ip4 = ntohl(*(u32*)h->h_addr_list[0]);
	    host->ip4_valid = true;
	}

	if ( ptr < end && *ptr == ':' )
	{
	    ccp pptr = ptr+1;
	    if ( pptr < end && *pptr >= '0' && *pptr <= '9' )
	    {
		char *end;
		const uint port = strtoul(pptr,&end,10);
		if ( port <= 0xffff && end > pptr )
		{
		    host->port = port;
		    ptr = end;
		}
	    }
	    else
	    {
		ccp pstart = pptr;
		while ( pptr < end
		     && ( *pptr >= 'a' && *pptr <= 'z'
			|| *pptr >= 'A' && *pptr <= 'Z'
			|| *pptr >= '0' && *pptr <= '9'
			|| *pptr == '_' || *pptr == '-'
			))
		{
		    pptr++;
		}
		len = pptr - pstart;
		if ( len > 0 && len < sizeof(buf) )
		{
		    memcpy(buf,pstart,len);
		    buf[len] = 0;
		    struct servent *s = getservbyname(buf,0);
		    if (s)
		    {
			const int port = ntohs(s->s_port);
			if ( port > 0 )
			    host->port = port;
			ptr = pptr;
		    }
		}
	    }
	}

	host->sa.sin_family = AF_INET;
	host->sa.sin_addr.s_addr = htonl(host->ip4);
	host->sa.sin_port = htons(host->port);
    }

    if ( *ptr && check_filename )
    {
	host->filename = name.ptr;
	host->ip4_valid = false;
    }

    host->not_scanned = BehindMem(name,ptr);
    return host->ip4_valid;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum PrintModeIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////

PrintModeIP_t ScanPrintModeIP ( PrintModeIP_t base, ccp arg )
{
    if (arg)
    {
	bool neg = false;
	while (*arg)
	{
	    PrintModeIP_t val  = 0;
	    PrintModeIP_t mask = 0;
	    switch(*arg++)
	    {
		case '-': neg = !neg; break;

		case 'n': val = PMIP_NEVER;    mask = PMIP_M_BITS; break;
		case 'r': val = PMIP_RELEVANT; mask = PMIP_M_BITS; break;
		case 'i': val = PMIP_IF_SET;   mask = PMIP_M_BITS; break;
		case 'a': val = PMIP_ALWAYS;   mask = PMIP_M_BITS; break;

		case 'p': val = PMIP_PORT;      break;
		case 's': val = PMIP_SERVICE;   break;
		case 'm': val = PMIP_MASK;      break;
		case 'b': val = PMIP_BRACKETS;  break;
		case 'f': val = PMIP_FULL_IPV6; break;

		case '0': val = PMIP_0DOTS; mask = PMIP_M_DOTS; break;
		case '1': val = PMIP_1DOT;  mask = PMIP_M_DOTS; break;
		case '2': val = PMIP_2DOTS; mask = PMIP_M_DOTS; break;
		case '3': val = PMIP_3DOTS; mask = PMIP_M_DOTS; break;
	    }

	    if (mask) // ignore 'neg'
	    {
		base = base & ~mask | val;
		neg = false;
	    }
	    else if (val)
	    {
		if (neg)
		    base &= ~val;
		else
		    base |= val;
		neg = false;
	    }
	}
    }

    return base;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintPrintModeIP ( PrintModeIP_t val, bool align )
{
    char buf[20], *dest = buf;

    DASSERT( PMIP_M_BITS == 3 );
    *dest++ = "nria"[ val & PMIP_M_BITS ];

    if ( val & PMIP_PORT )	*dest++ = 'p'; else if (align) *dest++ = '-';
    if ( val & PMIP_SERVICE )	*dest++ = 's'; else if (align) *dest++ = '-';
    if ( val & PMIP_MASK )	*dest++ = 'm'; else if (align) *dest++ = '-';
    if ( val & PMIP_BRACKETS )	*dest++ = 'b'; else if (align) *dest++ = '-';
    if ( val & PMIP_FULL_IPV6 )	*dest++ = 'f'; else if (align) *dest++ = '-';

    if ( val & PMIP_0DOTS )
	*dest++ = '0';
    else if ( val & PMIP_1DOT )
	*dest++ = '1';
    else if ( val & PMIP_2DOTS )
	*dest++ = '2';
    else if (align)
	*dest++ = '3';

    DASSERT( dest < buf+sizeof(buf) );
    return CopyCircBuf0(buf,dest-buf);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum IPClass_t			///////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetIPClassColor ( const ColorSet_t *colset, IPClass_t ipc )
{
    if ( !colset || !colset->colorize )
	return "";

    switch (ipc)
    {
	case IPCL_INVALID:	return colset->b_red;
	case IPCL_LOOPBACK:	return colset->b_cyan;
	case IPCL_LOCAL:	return colset->b_green;
	case IPCL_STANDARD:	return colset->reset;
	case IPCL_SPECIAL:	return colset->b_yellow;
	default:		return colset->b_magenta;
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct SplitIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////

bool SplitIP ( SplitIP_t *sip, ccp addr )
{
    DASSERT(sip);
    memset(sip,0,sizeof(*sip));
    if (!addr)
	return false;

    while ( isblank((int)*addr) )
	addr++;

    int bits = -1;
    ccp slash = strchr(addr,'/');
    if (slash)
    {
	if (strchr(slash,'.'))
	{
	    // is there a netmask?
	    struct in_addr inp;
	    if (inet_aton(slash+1,&inp))
	    {
		const int bits2 = Count1Bits32(inp.s_addr);
		const ipv4_t mask = GetIP4Mask(bits2);
		if ( mask == inp.s_addr )
		    bits = bits2;
	    }
	}

	if ( bits < 0 )
	{
	    const int val = strtol(slash+1,0,10);
	    if ( val >= 0 && val <= 128 )
		bits = val;
	}
    }

    ccp have_port = 0;
    if ( *addr == '[' )
    {
	ccp close = strchr(++addr,']');
	if (!close)
	    return false;

	StringCopySM(sip->name,sizeof(sip->name),addr,close-addr);
	if ( close[1] == ':' )
	    have_port = close+2;
    }
    else
    {
	char *end_name = slash
	    ? StringCopySM(sip->name,sizeof(sip->name),addr,slash-addr)
	    : StringCopyS(sip->name,sizeof(sip->name),addr);

	if ( end_name > sip->name && end_name[-1] == '.' )
	{
	    // remove trailing point if not a valid IPv4 (domains only)
	    if ( CheckIP4(sip->name,end_name-sip->name) <= CIP4_INVALID )
		*--end_name = 0;
	}

	ccp point = strchr(sip->name,'.');
	if (point)
	{
	    char *colon = strchr(point,':');
	    if (colon)
	    {
		*colon++ = 0;
		have_port = colon;
	    }
	}
    }

    if (!*sip->name)
	return false;

    if (have_port)
    {
	if ( *have_port >= '0' && *have_port <= '9' )
	{
	    const uint port = strtol(have_port,0,10);
	    if ( port <= 0xffff )
		sip->port = port;
	}
	else
	{
	    char serv[100];
	    if (slash)
	    {
		StringCopySM(serv,sizeof(serv),have_port,slash-have_port);
		have_port = serv;
	    }

	    struct servent result_buf;
	    struct servent *result = 0;
	    char buf[1000];
	    if ( !getservbyname_r(have_port,0,&result_buf,buf,sizeof(buf),&result) && result )
		sip->port = ntohs(result->s_port);
	}
    }

    sip->bits = bits;
    return true;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct NamesIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////

struct ip_names_list_t
{
    int offset;
    char name[8];
};

//-----------------------------------------------------------------------------

#undef RECORD
#define RECORD(v) { offsetof(NamesIP_t,v), #v },

static const struct ip_names_list_t ip_names_list[] =
{
    RECORD(binaddr)
    RECORD(ipvers)
    RECORD(addr1)
    RECORD(addr2)
    RECORD(bits)
    RECORD(mask1)
    RECORD(mask2)
    {-1,""}
};

#undef RECORD

#undef GET_NAME_IP_PTR
#define GET_NAME_IP_PTR(nip,offset) ((ccp*)( (char*)(nip)+offset ))

///////////////////////////////////////////////////////////////////////////////

void SetupNamesIP ( NamesIP_t *dest, const NamesIP_t *src )
{
    if (!dest)
	return;

    if (src)
    {
	for ( const struct ip_names_list_t *in = ip_names_list; in->offset >= 0; in++ )
	{
	    ccp *s = GET_NAME_IP_PTR(src,in->offset);
	    ccp *d = GET_NAME_IP_PTR(dest,in->offset);
	    *d = *s && **s ? *s : in->name;
	}
    }
    else
    {
	for ( const struct ip_names_list_t *in = ip_names_list; in->offset >= 0; in++ )
	{
	    ccp *d = GET_NAME_IP_PTR(dest,in->offset);
	    *d = in->name;
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetupNamesByListIP ( NamesIP_t *dest, const exmem_list_t *src )
{
    if (!dest)
	return;

    // [[2do]] ??? qqq
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintNamesIP ( const NamesIP_t *nip )
{
    return nip
	? PrintCircBuf("ba=%s, iv=%s, a1=%s, a2=%s, b=%s, m1=%s, m2=%s\n",
		nip->binaddr, nip->ipvers, nip->addr1, nip->addr2,
		nip->bits, nip->mask1, nip->mask2 )
	: 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct BinIP_t			///////////////
///////////////////////////////////////////////////////////////////////////////

BinIP_t GetIPMask ( const BinIP_t *src )
{
    BinIP_t res = {0};
    if (src)
    {
	if ( src->ipvers == 4 )
	    res.ip4 = GetIP4Mask(src->bits);
	else if ( src->ipvers == 6 )
	    res.ip6 = GetIP6Mask(src->bits);
	else
	    goto abort;

	res.ipvers = src->ipvers;
	res.bits   = src->bits;
    }

 abort:
    return res;
}

///////////////////////////////////////////////////////////////////////////////

void ApplyIPMask ( BinIP_t *bip )
{
    if (bip)
    {
	BinIP_t mask = GetIPMask(bip);
	bip->ip6_64[0] &= mask.ip6_64[0];
	bip->ip6_64[1] &= mask.ip6_64[1];
    }
}

///////////////////////////////////////////////////////////////////////////////

IPClass_t GetIPClassBIP ( const BinIP_t * bip )
{
    struct ip4_tab
    {
	ipv4_t    addr;
	ipv4_t    mask;
	IPClass_t ipc;
    };

    static const struct ip4_tab ip4_tab[] =
    {
	{ 0x00000000, 0xffffffff, IPCL_INVALID	}, // 0.0.0.0
	{ 0x7f000000, 0xff000000, IPCL_LOOPBACK	}, // 127.0.0.0/8
	{ 0x0a000000, 0xff000000, IPCL_LOCAL	}, // 10.0.0.0/8
	{ 0xac100000, 0xfff00000, IPCL_LOCAL	}, // 172.16.0.0/12
	{ 0xc0a80000, 0xffff0000, IPCL_LOCAL	}, // 192.168.0.0/16
	{ 0xa9fe0000, 0xffff0000, IPCL_LOCAL	}, // 169.254.0.0/16
	{ 0xe0000000, 0xe0000000, IPCL_SPECIAL	}, // 224.0.0.0/3
	{ 0,0,IPCL__N },
    };

    struct ip6_tab
    {
	IPClass_t ipc;
	ccp	  name;
	ipv6_t	  addr;
	ipv6_t	  mask;
    };

    static struct ip6_tab ip6_tab[] =
    {
	{ IPCL_INVALID,  "::" },
	{ IPCL_LOOPBACK, "::1" },
	{ IPCL_LOCAL,    "fe80::/10" },
	{ IPCL_SPECIAL,  "f00::/8" },
	{ 0,0 },
    };

    if (bip)
    {
	if ( bip->ipvers == 4 )
	{
	    const ipv4_t addr = ntohl(bip->ip4);
	    for ( const struct ip4_tab *ptr = ip4_tab; ptr->ipc < IPCL__N; ptr++ )
		if ( ( addr & ptr->mask ) == ptr->addr )
		    return ptr->ipc;
	    return IPCL_STANDARD;
	}
	else if ( bip->ipvers == 6 )
	{
	    ipv6_t addr;
	    memcpy(&addr,bip->ip6_64,sizeof(addr));
	    PRINT0("> %016llx.%016llx\n",ntoh64(addr.v64[0]),ntoh64(addr.v64[1]));

	    static bool init_done = false;
	    if (!init_done)
	    {
		init_done = true;
		ManageIP_t mip = {{0}};
		for ( struct ip6_tab *ptr = ip6_tab; ptr->name; ptr++ )
		{
		    ScanMIP(&mip,ptr->name);
		    memcpy(&ptr->addr,mip.bin.ip6_64,sizeof(ptr->addr));
		    ptr->mask = GetIP6Mask(mip.bin.bits);
		}
		ResetMIP(&mip);
	    }

	    for ( struct ip6_tab *ptr = ip6_tab; ptr->name; ptr++ )
	    {
		PRINT0("  %016llx.%016llx / %016llx.%016llx : %s\n",
			ntoh64(ptr->addr.v64[0]), ntoh64(ptr->addr.v64[1]),
			ntoh64(ptr->mask.v64[0]), ntoh64(ptr->mask.v64[1]),
			ptr->name );

		if (   ( addr.v64[0] & ptr->mask.v64[0] ) == ptr->addr.v64[0]
		    && ( addr.v64[1] & ptr->mask.v64[1] ) == ptr->addr.v64[1] )
		{
		    return ptr->ipc;
		}
	    }
	    return IPCL_STANDARD;
	}
    }

    return IPCL_INVALID;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintBIP ( const BinIP_t * bip )
{
    ccp res;
    int bitlen = 32;
    char buf[INET6_ADDRSTRLEN+1];

    switch ( bip ? bip->ipvers : 0 )
    {
	case 4:
	    res = inet_ntop(AF_INET,&bip->ip4,buf,sizeof(buf));
	    break;

	case 6:
	    res = inet_ntop(AF_INET6,bip->ip6_32,buf,sizeof(buf));
	    bitlen = 128;
	    break;

	default:
	    res = 0;
	    break;
    }

    if (!res)
	return 0;

    return bip->bits < bitlen
	? PrintCircBuf("%s/%u",res,bip->bits)
	: CopyCircBuf(res,strlen(res)+1);
}

///////////////////////////////////////////////////////////////////////////////

bool IsSameBIP ( const BinIP_t *a1, const BinIP_t *a2, uint use_mask )
{
    if ( a1 && a2 && a1->ipvers == a2->ipvers )
    {
	int bits;
	switch ( use_mask & 3 )
	{
	    case 1:  bits = a1->bits; break;
	    case 2:  bits = a2->bits; break;
	    case 3:  bits = a1->bits < a2->bits ? a1->bits : a2->bits; break;
	    default: bits = 128; break;
	}

	if ( a1->ipvers == 4 )
	{
	    ipv4_t mask = GetIP4Mask(bits);
	    return ( a1->ip4 & mask ) == ( a2->ip4 & mask );
	}
	else if ( a1->ipvers == 6 )
	{
	    ipv6_t mask = GetIP6Mask(bits);
	    return !( ( a1->ip6.v64[0] ^ a2->ip6.v64[0] ) & mask.v64[0] )
		&& !( ( a1->ip6.v64[1] ^ a2->ip6.v64[1] ) & mask.v64[1] );
	}
    }
    return false;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct BinIPList_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetBIL ( BinIPList_t *bil )
{
    if (bil)
    {
	ClearBIL(bil);
	FREE(bil->list);
	InitializeBIL(bil);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClearBIL ( BinIPList_t *bil )
{
    if (bil)
    {
	BinIPItem_t *it = bil->list;
	for ( int i = 0; i < bil->used; i++, it++ )
	{
	    FreeString(it->name);
	    it->name = 0;
	}
	bil->used = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SetSizeBIL ( BinIPList_t *bil, int size )
{
    DASSERT(bil);
    if ( size < 0 )
	size = 0;

    if ( bil->used > size )
    {
	for ( int i = size; i < bil->used; i++ )
	{
	    BinIPItem_t *it = bil->list + i;
	    FreeString(it->name);
	}
	bil->used = size;
    }

    if ( bil->size != size )
    {
	bil->size = size;
	bil->list = REALLOC( bil->list, size * sizeof(*bil->list) );
    }

    DASSERT( !bil->size || bil->list );
    DASSERT( bil->used <= bil->size );
}

///////////////////////////////////////////////////////////////////////////////

void SetMinSizeBIL ( BinIPList_t *bil, int min_size )
{
    DASSERT(bil);
    if ( min_size > bil->size )
    {
	const int good_size = bil->size / 8 + 10;
	if ( min_size < good_size )
	     min_size = good_size;
	SetSizeBIL(bil,min_size);
    }
}

///////////////////////////////////////////////////////////////////////////////

BinIPItem_t * CreateItemBIL ( BinIPList_t *bil, int n_item )
{
    DASSERT(bil);
    if ( n_item < 0 )
	n_item = 0;

    SetMinSizeBIL(bil,bil->used+n_item);
    BinIPItem_t *res = bil->list + bil->used;
    if ( n_item > 0 )
	memset( res, 0, n_item * sizeof(*res) );
    bil->used += n_item;

    DASSERT( !bil->size || bil->list );
    DASSERT( bil->used <= bil->size );
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if 0

BinIPItem_t * InsertBIL ( BinIPList_t *bil, int index, ccp key, CopyMode_t copy_mode )
{
    DASSERT(bil);
    // ???
    return 0;
}

#endif
///////////////////////////////////////////////////////////////////////////////
#if 0

BinIPItem_t * InsertByKeyBIL ( BinIPList_t *bil, ccp key, CopyMode_t copy_mode )
{
    DASSERT(bil);
    // ???
    return 0;
}

#endif
///////////////////////////////////////////////////////////////////////////////
#if 0

int FindIndexBIL ( const BinIPList_t *bil, ccp key )
{
    DASSERT(bil);
    // ???
    return 0;
}

#endif
///////////////////////////////////////////////////////////////////////////////

BinIPIterate_t FindAddressBIL ( const BinIPList_t *bil, BinIP_t *addr, uint use_mask )
{
    BinIPIterate_t iter;
    if ( bil && bil->used && addr && addr->ipvers )
    {
	iter.valid	= true;
	iter.bil	= bil;
	iter.cur	= bil->list - 1;
	iter.addr	= *addr;
	iter.use_mask	= use_mask;
	FindNextAddressBIL(&iter);
    }
    else
	memset(&iter,0,sizeof(iter));
    return iter;
}

//-----------------------------------------------------------------------------

void FindNextAddressBIL ( BinIPIterate_t *iter )
{
    if ( iter && iter->valid && iter->bil && iter->cur )
    {
	for ( uint idx = iter->cur + 1 - iter->bil->list; idx < iter->bil->used; idx++ )
	{
	    BinIPItem_t *it = iter->bil->list + idx;
	    if (IsSameBIP(&it->bip,&iter->addr,iter->use_mask))
	    {
		iter->cur = it;
		return;
	    }
	}
    }

    iter->valid = false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

BinIP_t GetBIPBySA ( sockaddr_t *addr, sockaddr_t *mask )
{
    BinIP_t res = {0};
    if (addr)
    {
	if ( addr->sa_family == AF_INET )
	{
	    res.ipvers	= 4;
	    res.bits	= GetBitsBySA(mask,32);
	    res.ip4	= ((sockaddr_in4_t*)addr)->sin_addr.s_addr;
	}
	else if ( addr->sa_family == AF_INET6 )
	{
	    res.ipvers	= 6;
	    res.bits	= GetBitsBySA(mask,128);
	    memcpy(&res.ip6,&((sockaddr_in6_t*)addr)->sin6_addr,sizeof(res.ip6));
	}
    }
    return res;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint ScanInterfaceAddresses ( BinIPList_t *bil, bool init_bil )
{
    DASSERT(bil);
    if (init_bil)
	InitializeBIL(bil);
    else
	ClearBIL(bil);

    struct ifaddrs *ifaddr;
    if (getifaddrs(&ifaddr))
	return -1;

    for ( const struct ifaddrs *ifa = ifaddr; ifa; ifa = ifa->ifa_next )
    {
	BinIP_t temp = GetBIPBySA(ifa->ifa_addr,ifa->ifa_netmask);
	if (temp.ipvers)
	{
	    BinIPItem_t *it	= CreateItemBIL(bil,1);
	    it->name		= STRDUP(ifa->ifa_name);
	    it->bip		= temp;
	}
    }
    freeifaddrs(ifaddr);

    return bil->used;
}

///////////////////////////////////////////////////////////////////////////////

const BinIPList_t * GetInterfaceAddressList ( bool update )
{
    static BinIPList_t *list = 0;

    if ( !list || update )
    {
	bool init = !list;
	if (init)
	    list = MALLOC(sizeof(*list));
	ScanInterfaceAddresses(list,init);
    }
    return list;
}

///////////////////////////////////////////////////////////////////////////////

bool IsLocalBIL ( BinIP_t *addr, uint use_mask )
{
    const BinIPList_t *ial = GetInterfaceAddressList(false);
    BinIPIterate_t iter = FindAddressBIL(ial,addr,use_mask);
    return iter.valid;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct ManageIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetMIP ( ManageIP_t *mip )
{
    if (mip)
    {
	FreeString(mip->name);
	FreeString(mip->decoded_name);
	memset(mip,0,sizeof(*mip));
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClearNameMIP ( ManageIP_t *mip )
{
    if (mip)
    {
	FreeString(mip->name);
	FreeString(mip->decoded_name);
	mip->name = mip->decoded_name = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

ManageIP_t GetCopyMIP ( const ManageIP_t *mip )
{
    ManageIP_t res;
    if (mip)
    {
	memcpy(&res,mip,sizeof(res));
	if (res.name)
	    res.name = STRDUP(res.name);
	if (res.decoded_name)
	    res.decoded_name = STRDUP(res.decoded_name);
    }
    else
	memset(&res,0,sizeof(res));

    return res;
}

///////////////////////////////////////////////////////////////////////////////

bool AssignMIP ( ManageIP_t *dest, const ManageIP_t *src )
{
    if (dest)
    {
	ClearNameMIP(dest);
	*dest = GetCopyMIP(src);
	return dest->bin.ipvers != 0;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////

bool AssignBinMIP ( ManageIP_t *mip, cvp bin, int bits )
{
    DASSERT(mip);
    ResetMIP(mip);
    if (!bin)
	return false;

    const BinIP_t *bip = bin;
    if ( bip->ipvers == 4 )
	mip->relevant_bits = 32;
    else if ( bip->ipvers == 6 )
	mip->relevant_bits = 128;
    else
	return false;

    memcpy(&mip->bin,bin,sizeof(mip->bin));
    if ( bits >= 0 )
    {
	mip->have_bits = true;
	mip->bin.bits = bits < mip->relevant_bits ? bits : mip->relevant_bits;
    }

    if ( mip->bin.bits < mip->relevant_bits )
	mip->have_bits = true;
    else
	mip->bin.bits = mip->relevant_bits;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool AssignSockaddrMIP ( ManageIP_t *mip, cvp sockaddr, u16 port, int bits )
{
    DASSERT(mip);
    ResetMIP(mip);
    if (!sockaddr)
	return false;

    const sockaddr_in46_t *sa = sockaddr;
    if ( sa->family == AF_INET )
    {
	mip->bin.ipvers	= 4;
	mip->bin.bits	= 32;
	mip->bin.ip4	= sa->in4.sin_addr.s_addr;
	mip->port	= ntohs(sa->in4.sin_port);
    }
    else if ( sa->family == AF_INET6 )
    {
	mip->bin.ipvers	= 6;
	mip->bin.bits	= 128;
	memcpy(mip->bin.ip6_8,sa->in6.sin6_addr.s6_addr,sizeof(mip->bin.ip6_8));
	mip->port	= ntohs(sa->in6.sin6_port);
    }

    mip->relevant_bits = mip->bin.bits;
    if ( bits >= 0 )
    {
	mip->have_bits = true;
	if ( mip->bin.bits > bits )
	     mip->bin.bits = bits;
    }

    if ( !mip->port )
	mip->port = port;

    return true;
}

///////////////////////////////////////////////////////////////////////////////

bool ScanMIP ( ManageIP_t *mip, ccp addr )
{
    DASSERT(mip);
    ResetMIP(mip);

    SplitIP_t sip;
    if (!SplitIP(&sip,addr))
	return false;

    if (strchr(sip.name,':'))
    {
	if ( inet_pton(AF_INET6,sip.name,mip->bin.ip6_32) != 1 )
	    return false;
	mip->bin.ipvers = 6;
	mip->relevant_bits = 128;
    }
    else
    {
	struct in_addr inp;
	if (!dclib_aton(sip.name,&inp))
	    return false;

	mip->bin.ipvers = 4;
	mip->bin.ip4 = inp.s_addr;
	mip->relevant_bits = 32;
    }

    mip->bin.bits = mip->relevant_bits;
    if ( sip.bits >= 0 )
    {
	mip->have_bits = true;
	if ( sip.bits < mip->relevant_bits )
	    mip->bin.bits = sip.bits;
    }

    mip->port = sip.port;
    return true;
}

///////////////////////////////////////////////////////////////////////////////

ManageIP_t GetSockNameMIP ( int sock )
{
    ManageIP_t mip = {{0}};
    sockaddr_in46_t sa;
    socklen_t sa_len = sizeof(sa);
    if (!getsockname(sock,(sockaddr_t*)&sa,&sa_len))
	AssignSockaddrMIP(&mip,&sa,0,-1);
    return mip;
}

///////////////////////////////////////////////////////////////////////////////

ManageIP_t GetPeerNameMIP ( int sock )
{
    ManageIP_t mip = {{0}};
    sockaddr_in46_t sa;
    socklen_t sa_len = sizeof(sa);
    if (!getpeername(sock,(sockaddr_t*)&sa,&sa_len))
	AssignSockaddrMIP(&mip,&sa,0,-1);
    return mip;
}

///////////////////////////////////////////////////////////////////////////////

ManageIP_t GetAddrInfoMIP ( ccp addr, int protocol, IpMode_t ipm )
{
    ManageIP_t mip = {{0}};

    SplitIP_t sip;
    if (SplitIP(&sip,addr))
    {
	struct addrinfo hints
		= { .ai_family = AF_UNSPEC,
		    .ai_protocol = protocol < 0 ? IPPROTO_TCP : protocol };
	struct addrinfo *result;
	if (!getaddrinfo(sip.name,0,&hints,&result))
	{
	    struct addrinfo *ip4 = 0, *ip6 = 0;
	    for ( struct addrinfo *rp = result; rp; rp = rp->ai_next)
	    {
		if ( !ip4 && rp->ai_family == AF_INET )
		    ip4 = rp;
		else if ( !ip6 && rp->ai_family == AF_INET6 )
		    ip6 = rp;
	    }

	    switch (ipm)
	    {
		case IPV4_ONLY:  break; // nothing to do
		case IPV6_ONLY:  ip4 = ip6; break;
		case IPV4_FIRST: if (!ip4) ip4 = ip6; break;
		case IPV6_FIRST: if (ip6) ip4 = ip6; break;
	    }

	    if (ip4)
		AssignSockaddrMIP(&mip,ip4->ai_addr,sip.port,sip.bits);

	    freeaddrinfo(result);
	}
    }

    return mip;
}

///////////////////////////////////////////////////////////////////////////////

sockaddr_in46_t GetSockaddrMIP ( const ManageIP_t *mip )
{
    DASSERT(mip);
    sockaddr_in46_t sa = { .family = 0 };

    if ( mip->bin.ipvers == 4 )
    {
	sa.in4.sin_family	= AF_INET;
	sa.in4.sin_addr.s_addr	= mip->bin.ip4;
	sa.in4.sin_port		= htons(mip->port);
    }
    else if ( mip->bin.ipvers == 6 )
    {
	sa.in6.sin6_family	= AF_INET6;
	sa.in6.sin6_port	= htons(mip->port);
	memcpy(sa.in6.sin6_addr.s6_addr,mip->bin.ip6_8,sizeof(sa.in6.sin6_addr));
    }

    return sa;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetNameInfoMIP ( ManageIP_t *mip )
{
    DASSERT(mip);
    if ( !mip->name && mip->bin.ipvers )
    {
	char host1[NI_MAXHOST+20], host2[NI_MAXHOST+20];
	sockaddr_in46_t sa = GetSockaddrMIP(mip);

	if (!getnameinfo( (sockaddr_t*)&sa,sizeof(sa), host1,sizeof(host1), 0,0, 0 ))
	{
	    mip->name = STRDUP(host1);

	    FreeString(mip->decoded_name);
	    const uint stat = Domain2UTF8(host2,sizeof(host2),host1,-1);
	    mip->decoded_name = stat && *host2 && strcmp(host1,host2) ? STRDUP(host2) : 0;
	}
    }
    return mip->name;
}

///////////////////////////////////////////////////////////////////////////////

void ApplyMaskMIP ( ManageIP_t *mip, int bits )
{
    DASSERT(mip);
    if ( bits >= 0 && bits < mip->bin.bits )
	mip->bin.bits = bits;
    ApplyIPMask(&mip->bin);
}

///////////////////////////////////////////////////////////////////////////////

void MaskPrefixMIP ( ManageIP_t *mip )
{
    DASSERT(mip);
    if ( mip->bin.ipvers == 4 || mip->bin.ip6_64[0] != 0ull )
	ApplyMaskMIP(mip,64);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintAddrMIP ( const ManageIP_t *mip, PrintModeIP_t pmode )
{
    DASSERT(mip);

    ccp res;
    char buf[INET6_ADDRSTRLEN+1];

    switch (mip->bin.ipvers)
    {
     case 4:
	res = buf;
	if ( pmode & PMIP_0DOTS )
	    snprintf(buf,sizeof(buf),"%u",ntohl(mip->bin.ip4));
	else if ( pmode & PMIP_1DOT )
	    snprintf(buf,sizeof(buf), "%u.%u",
			mip->bin.ip4x.v8[0], ntohl(mip->bin.ip4)&0xffffff );
	else if ( pmode & PMIP_2DOTS )
	    snprintf(buf,sizeof(buf), "%u.%u.%u",
			mip->bin.ip4x.v8[0], mip->bin.ip4x.v8[1], ntohs(mip->bin.ip4x.v16[1]) );
	else
	    snprintf(buf,sizeof(buf), "%u.%u.%u.%u",
			mip->bin.ip4x.v8[0], mip->bin.ip4x.v8[1],
			mip->bin.ip4x.v8[2], mip->bin.ip4x.v8[3] );
	break;

     case 6:
	if ( pmode & PMIP_FULL_IPV6 )
	{
	    snprintf(buf,sizeof(buf), "%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x",
			ntohs(mip->bin.ip6.v16[0]),
			ntohs(mip->bin.ip6.v16[1]),
			ntohs(mip->bin.ip6.v16[2]),
			ntohs(mip->bin.ip6.v16[3]),
			ntohs(mip->bin.ip6.v16[4]),
			ntohs(mip->bin.ip6.v16[5]),
			ntohs(mip->bin.ip6.v16[6]),
			ntohs(mip->bin.ip6.v16[7]) );
	    res = buf;
	}
	else
	    res = inet_ntop(AF_INET6,mip->bin.ip6_32,buf,sizeof(buf));
	break;

     default:
	res = 0;
    }

    if (!res)
	return 0;

    char port[100];
    *port = 0;
    if ( mip->port && pmode & (PMIP_SERVICE|PMIP_PORT) )
    {
	if ( pmode & PMIP_SERVICE )
	{
	    struct servent result_buf;
	    struct servent *result;
	    char buf[1000];
	    const int stat = getservbyport_r(htons(mip->port),0,&result_buf,buf,sizeof(buf),&result);
	    if ( !stat && result )
		snprintf(port,sizeof(port),":%s",result_buf.s_name);
	}

	if (!*port)
	    snprintf(port,sizeof(port),":%u",mip->port);
	pmode |= PMIP_BRACKETS;
    }

    char bits[20];
    if (PrintBitsMIP(mip,pmode))
    {
	if ( mip->bin.ipvers == 4 && pmode & PMIP_MASK )
	{
	    ipv4x_t mask;
	    mask.ip4 = GetIP4Mask(mip->bin.bits);
	    snprintf(bits,sizeof(bits),"/%d.%d.%d.%d",
		mask.v8[0], mask.v8[1], mask.v8[2], mask.v8[3] );
	}
	else
	    snprintf(bits,sizeof(bits),"/%u",mip->bin.bits);
    }
    else
	*bits = 0;

    return pmode & PMIP_BRACKETS && mip->bin.ipvers == 6
	? PrintCircBuf("[%s]%s%s",res,port,bits)
	: PrintCircBuf("%s%s%s",res,port,bits);
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintVersAddrMIP ( const ManageIP_t *mip, PrintModeIP_t pmode )
{
    DASSERT(mip);
    ccp res = PrintAddrMIP(mip,pmode);
    return res ? PrintCircBuf("IPv%u %s",mip->bin.ipvers,res) : 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

ccp GetSqlSetAddrMIP ( const ManageIP_t *mip, const NamesIP_t *names )
{
    if (!mip)
	return 0;

    NamesIP_t nip;
    SetupNamesIP(&nip,names);

    if ( mip->bin.ipvers == 4 )
    {
	const ipv4_t mask = GetIP4Mask(mip->bin.bits);
	return PrintCircBuf("%s=4,%s=0,%s=0x%08x",
				nip.ipvers, nip.addr1,
				nip.addr2, ntohl( mip->bin.ip4 & mask ));
    }
    else if ( mip->bin.ipvers == 6 )
    {
	const ipv6_t mask = GetIP6Mask(mip->bin.bits);
	return PrintCircBuf("%s=6,%s=0x%llx,%s=0x%llx",
				nip.ipvers,
				nip.addr1, ntoh64( mip->bin.ip6_64[0] & mask.v64[0] ),
				nip.addr2, ntoh64( mip->bin.ip6_64[1] & mask.v64[1] ));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetSqlSetMaskMIP ( const ManageIP_t *mip, const NamesIP_t *names )
{
    if (!mip)
	return 0;

    NamesIP_t nip;
    SetupNamesIP(&nip,names);

    if ( mip->bin.ipvers == 4 )
    {
	const ipv4_t mask = GetIP4Mask(mip->bin.bits);
	return PrintCircBuf("%s=4,%s=0,%s=0x%08x,%s=0,%s=0x%08x",
				nip.ipvers, nip.addr1,
				nip.addr2, ntohl( mip->bin.ip4 & mask ),
				nip.mask1, nip.mask2, ntohl(mask));
    }
    else if ( mip->bin.ipvers == 6 )
    {
	const ipv6_t mask = GetIP6Mask(mip->bin.bits);
	return PrintCircBuf("%s=6,%s=0x%llx,%s=0x%llx,%s=0x%llx,%s=0x%llx",
				nip.ipvers,
				nip.addr1, ntoh64( mip->bin.ip6_64[0] & mask.v64[0] ),
				nip.addr2, ntoh64( mip->bin.ip6_64[1] & mask.v64[1] ),
				nip.mask1, ntoh64(mask.v64[0]),
				nip.mask2, ntoh64(mask.v64[1]) );
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetSqlSetBinMIP ( const ManageIP_t *mip, const NamesIP_t *names )
{
    if (!mip)
	return 0;

    NamesIP_t nip;
    SetupNamesIP(&nip,names);

    return PrintCircBuf("%s=unhex('%02x%02x%016llx%016llx')",
		nip.binaddr, mip->bin.ipvers, mip->bin.bits,
		ntoh64(mip->bin.ip6_64[0]), ntoh64(mip->bin.ip6_64[0]) );
}

///////////////////////////////////////////////////////////////////////////////

ccp GetSqlCondAddrMIP ( const ManageIP_t *mip, const NamesIP_t *names )
{
    if (!mip)
	return 0;

    NamesIP_t nip;
    SetupNamesIP(&nip,names);

    if ( mip->bin.ipvers == 4 )
	return PrintCircBuf("%s=4 && %s=0x%08x",
			nip.ipvers, nip.addr2, ntohl(mip->bin.ip4) );
    else if ( mip->bin.ipvers == 6 )
	return PrintCircBuf("%s=6 && %s=0x%llx && %s=0x%llx",
			nip.ipvers,
			nip.addr1, ntoh64(mip->bin.ip6_64[0]),
			nip.addr2, ntoh64(mip->bin.ip6_64[1]) );

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetSqlCondMaskMIP ( const ManageIP_t *mip, const NamesIP_t *names )
{
    if (!mip)
	return 0;

    NamesIP_t nip;
    SetupNamesIP(&nip,names);

    if ( mip->bin.ipvers == 4 )
	return PrintCircBuf("%s=4 && %s=(%s&0x%08x)",
			nip.ipvers,
			nip.addr2, nip.mask2, ntohl(mip->bin.ip4) );
    else if ( mip->bin.ipvers == 6 )
	return PrintCircBuf("%s=6 && %s=(%s&0x%llx) && %s=(%s&0x%llx)",
			nip.ipvers,
			nip.addr1, nip.mask1, ntoh64(mip->bin.ip6_64[0]),
			nip.addr2, nip.mask2, ntoh64(mip->bin.ip6_64[1]) );

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

ccp GetSqlCondBinMaskMIP ( const ManageIP_t *mip, const NamesIP_t *names )
{
    if (!mip)
	return 0;

    NamesIP_t nip;
    SetupNamesIP(&nip,names);

    if ( mip->bin.ipvers == 4 )
	return PrintCircBuf("compare_binaddr(%s,4,0,0x%08x,%s,%s)",
			nip.binaddr, ntohl(mip->bin.ip4),
			nip.mask1, nip.mask2 );
    else if ( mip->bin.ipvers == 6 )
	return PrintCircBuf("compare_binaddr(%s,6,0x%llx,0x%llx,%s,%s)",
			nip.binaddr,
			ntoh64(mip->bin.ip6_64[0]), ntoh64(mip->bin.ip6_64[1]),
			nip.mask1, nip.mask2 );

    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct ResolveIP_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void ResetRIP ( ResolveIP_t *rip )
{
    if (rip)
    {
	ResetMIP(&rip->mip4);
	ResetMIP(&rip->mip6);
	memset(rip,0,sizeof(*rip));
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClearNamesRIP ( ResolveIP_t *rip )
{
    if (rip)
    {
	ClearNameMIP(&rip->mip4);
	ClearNameMIP(&rip->mip6);
    }
}

///////////////////////////////////////////////////////////////////////////////

int SetupListRIP ( ResolveIP_t *rip )
{
    DASSERT(rip);
    memset(rip->mip_list,0,sizeof(rip->mip_list));

    ManageIP_t **mp = rip->mip_list;
    switch (rip->ip_mode)
    {
	case IPV6_FIRST:
	    if (rip->mip6.bin.ipvers)
		*mp++ = &rip->mip6;
	    // fall though
	case IPV4_ONLY:
	    if (rip->mip4.bin.ipvers)
		*mp++ = &rip->mip4;
	    break;

	case IPV4_FIRST:
	    if (rip->mip4.bin.ipvers)
		*mp++ = &rip->mip4;
	    // fall though
	case IPV6_ONLY:
	    if (rip->mip6.bin.ipvers)
		*mp++ = &rip->mip6;
	    break;
    }

    return mp - rip->mip_list;
}

///////////////////////////////////////////////////////////////////////////////

ResolveIP_t GetCopyRIP ( const ResolveIP_t *rip )
{
    ResolveIP_t res;
    if (rip)
    {
	res.ip_mode = rip->ip_mode;
	res.mip4 = GetCopyMIP(&rip->mip4);
	res.mip6 = GetCopyMIP(&rip->mip6);
	SetupListRIP(&res);
    }
    else
	memset(&res,0,sizeof(res));

    return res;
}

///////////////////////////////////////////////////////////////////////////////

void AssignRIP ( ResolveIP_t *dest, const ResolveIP_t *src )
{
    if (dest)
    {
	ClearNamesRIP(dest);
	*dest = GetCopyRIP(src);
    }
}

///////////////////////////////////////////////////////////////////////////////

int ScanRIP
(
    // return 0..2 = number of records

    ResolveIP_t	*rip,		// valid data, ResetRIP()
    ccp		addr,		// address to scan
    int		protocol,	// protocol to use: 0=all, -1=default (IPPROTO_TCP at the moment)
    IpMode_t	ip_mode,	// IPv4 and/or IPV6, order of 'mip_list'
    bool	get_names	// true: call GetNameInfoMIP()
)
{
    DASSERT(rip);
    ResetRIP(rip);

    SplitIP_t sip;
    if (!SplitIP(&sip,addr))
	return 0;
    NormalizeNameSIP(&sip);

    rip->ip_mode = ip_mode;

    struct addrinfo hints = { .ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM };
    struct addrinfo *result;
    char aname[300];
    Domain2ASCII(aname,sizeof(aname),sip.name,-1);
    if (!getaddrinfo(aname,0,&hints,&result))
    {
	struct addrinfo *ip4 = 0, *ip6 = 0;
	for ( struct addrinfo *rp = result; rp; rp = rp->ai_next)
	{
	    if ( !ip4 && rp->ai_family == AF_INET )
		ip4 = rp;
	    else if ( !ip6 && rp->ai_family == AF_INET6 )
		ip6 = rp;
	}

	if (ip4)
	    AssignSockaddrMIP(&rip->mip4,ip4->ai_addr,sip.port,sip.bits);

	if (ip6)
	    AssignSockaddrMIP(&rip->mip6,ip6->ai_addr,sip.port,sip.bits);

	if (get_names)
	{
	    GetNameInfoMIP(&rip->mip4);
	    GetNameInfoMIP(&rip->mip6);
	}

	freeaddrinfo(result);
    }

    return SetupListRIP(rip);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if 0

void DumpRIP ( FILE *f, int indent, const ResolveIP_t *rip )
{
    if ( !f || !rip )
	return;
    indent = NormalizeIndent(indent);
}

#endif
//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct AllowIP4_t		///////////////
///////////////////////////////////////////////////////////////////////////////

const KeywordTab_t AllowIP4KeyTable[] =
{
	{ 0,			"0",		0,		1 },
	{ ALLOW_MODE_DENY,	"DENY",		0,		1 },
	{ ALLOW_MODE_ALLOW,	"ALLOW",	0,		0 },
	{ ALLOW_MODE_AKEY,	"AKEY",		"JACC",		0 },

	{ ALLOW_MODE_LOCAL,	"LOCAL",	0,		0 },
	{ ALLOW_MODE_LAN,	"LAN",		0,		0 },
	{ ALLOW_MODE_EXTERN,	"EXTERN",	0,		0 },

	{ ALLOW_MODE_PUBLIC,	"PUBLIC",	0,		0 },
	{ ALLOW_MODE_USER,	"USER",		0,		0 },
	{ ALLOW_MODE_MOD,	"MOD",		"MODERATOR",	0 },
	{ ALLOW_MODE_ADMIN,	"ADMIN",	"ADMINISTRATOR",0 },
	{ ALLOW_MODE_DEVELOP,	"DEVELOP",	"DEVELOPER",	0 },

	{ ALLOW_MODE_LOG,	"LOG",		0,		0 },
	{ ALLOW_MODE_VERBOSE,	"VERBOSE",	0,		0 },

	{ ALLOW_RELEASE,	"RELEASE",	0,		0 },
	{ ALLOW_DEBUG,		"DEBUG",	0,		0 },

	{ ALLOW_MODE_NOT,	"NOT",		0,		0 },
	{ ALLOW_MODE_SET,	"SET",		0,		ALLOW_MODE__OP },
	{ ALLOW_MODE_AND,	"AND",		0,		ALLOW_MODE__OP },
	{ 0,			"OR",		0,		ALLOW_MODE__OP },
	{ ALLOW_MODE_CONTINUE,	"CONTINUE",	0,		0 },
	{0,0,0,0}
};

///////////////////////////////////////////////////////////////////////////////

void InitializeAllowIP4 ( AllowIP4_t *ai )
{
    DASSERT(ai);
    memset(ai,0,sizeof(*ai));
    ai->ref_counter	= 1;
    ai->fallback_mode	= ALLOW_MODE_DENY;
    ai->allow_mode	= ALLOW_MODE_ALLOW|ALLOW_MODE_AKEY;
}

///////////////////////////////////////////////////////////////////////////////

void ResetAllowIP4 ( AllowIP4_t *ai )
{
    if (ai)
    {
	FREE(ai->list);
	ai->list = 0;
	ai->used = ai->size = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void ClearAllowIP4 ( AllowIP4_t *ai )
{
    if (ai)
    {
	FREE(ai->list);
	InitializeAllowIP4(ai);
    }
}

///////////////////////////////////////////////////////////////////////////////

AllowIP4_t * NewReferenceAllowIP4 ( AllowIP4_t *ai )
{
    if (ai)
	ai->ref_counter++;
    else
    {
	ai = MALLOC(sizeof(*ai));
	DASSERT(ai);
	InitializeAllowIP4(ai);

    }
    return ai;
}

///////////////////////////////////////////////////////////////////////////////

AllowIP4_t * DeleteReferenceAllowIP4 ( AllowIP4_t *ai )
{
    if ( ai && !--ai->ref_counter )
    {
	ResetAllowIP4(ai);
	FREE(ai);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

enumError ScanLineAllowIP4
(
    AllowIP4_t		*ai,	// valid structure; new elements are appended
    mem_t		line,	// single line to analyse
    const KeywordTab_t	*tab	// NULL or keyword table.
				// If NULL, a local table with keywords 0 DENY
				// ALLOW SET REMOVE AND OR and CONTINUE is used.
)
{
    if ( !line.ptr || !line.len )
	return ERR_OK;

    ccp ptr = line.ptr;
    ccp end = ptr + line.len;

    //--- find first character
    while ( ptr < end && *(uchar*)ptr <= ' ' )
	ptr++;
    if ( ptr == end || *ptr == '#' )
	return ERR_OK;

    NetworkHost_t host;
    ResolveHostMem(&host,true,BehindMem(line,ptr),0,false,false);
    if (!host.ip4_valid)
	return ERR_ERROR;
    line = host.not_scanned;

    u32 mask = M1(mask);
    if ( line.len && *line.ptr == '/' )
	line = ScanNetworkMaskMem(&mask,line);


    //--- scan options

    if (!tab)
	tab = AllowIP4KeyTable;

    char arg[1000];
    StringCopySMem(arg,sizeof(arg),line); // to be NULL terminated
    s64 res = ScanKeywordList(arg,tab,0,true,0,0,"AllowIP4",ERR_SYNTAX);

    if ( res == M1(res) )
	return ERR_SYNTAX;

    if ( res & ALLOW_MODE_NOT )
	res = res ^ ALLOW_MODE__MASK;

 #if defined(DEBUG) && DEBUG > 0 || defined(IS_DEVELOP) && IS_DEVELOP > 0
    if ( res & ALLOW_RELEASE )
	return ERR_OK;
 #else
    if ( res & ALLOW_DEBUG )
	return ERR_OK;
 #endif

    //--- add item

    if ( ai->used == ai->size )
    {
	ai->size = 11*ai->size/10  + 10;
	ai->list = REALLOC(ai->list,ai->size*sizeof(*ai->list));
    }
    DASSERT( ai->used < ai->size );
    DASSERT( ai->list );

    AllowIP4Item_t *it = ai->list + ai->used++;
    //memset(it,0,sizeof(*it));
    it->addr	= host.ip4 & mask;
    it->mask	= mask;
    it->mode	= res;
    it->count	= 0;

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ScanFileAllowIP4
(
    AllowIP4_t		*ai,	// valid structure; new elements are appended
    ccp			fname,	// ffilename of the source
    FileMode_t		fmode,	// flags for OpenFile()
    const KeywordTab_t	*tab	// NULL or keyword table -> see ScanLineAllowIP4()
)
{
    File_t F;
    enumError err = OpenFile(&F,true,fname,fmode,0,0);
    if (err)
	return err;

    char iobuf[10000];
    while (fgets(iobuf,sizeof(iobuf)-1,F.f))
    {
	err = ScanLineAllowIP4(ai,MemByString(iobuf),tab);
	if (err)
	    break;
    }

    CloseFile(&F,0);
    return err;
}

///////////////////////////////////////////////////////////////////////////////

void ResetCountersAllowIP4
(
    AllowIP4_t		*ai		// NULL or dest for addition
)
{
    if (ai)
    {
	uint i;
	AllowIP4Item_t *p = ai->list;
	for ( i = 0; i < ai->used; i++, p++ )
	    p->count = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void AddCountersAllowIP4
(
    AllowIP4_t		*dest,		// NULL or dest for addition
    const AllowIP4_t	*src		// NULL or source for addition
)
{
    if ( dest && dest->used && src && src->used )
    {
	uint is;
	const AllowIP4Item_t *ps = src->list;
	for ( is = 0; is < src->used; is++, ps++ )
	{
	    uint id;
	    AllowIP4Item_t *pd = dest->list;
	    for ( id = 0; id < dest->used; id++, pd++ )
		if (  pd->addr == ps->addr
		   && pd->mask == ps->mask
		   && pd->mode == ps->mode
		   )
		{
		    pd->count += ps->count;
		    break;
		}
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static uint print_mode_helper
	( char *buf, uint bufsize, const KeywordTab_t *keytab, u64 mode )
{
    ASSERT(buf);
    ASSERT(bufsize>1);
    char *dest = buf, *end = buf + bufsize-1;
    if (keytab)
    {
	mode &= ~(ALLOW_MODE_SET|ALLOW_MODE_AND);
	for ( ; mode && keytab->name1; keytab++ )
	{
	    if ( keytab->id & mode )
	    {
		dest = StringCat2E(dest,end,keytab->name1," ");
		mode &= ~keytab->id;
	    }
	}
    }
    *dest = 0;
    return dest-buf;
}

//-----------------------------------------------------------------------------

void DumpAllowIP4
(
    FILE		*f,		// NULL or output file
    int			indent,		// indention
    const AllowIP4_t	*ai,		// NULL or source
    const KeywordTab_t	*keytab,	// NULL or key table for output
    ccp			title,		// not NULL: Add title
    bool		print_tab_head	// true: add table headings and separators
)
{
    if ( !f || !ai )
	return;

    char buf[200];
    uint i, fw_mode = 5, fw_settings = 8;
    for ( i = 0; i < ai->used; i++ )
    {
	const AllowIP4Item_t *it = ai->list + i;
	uint len = snprintf(buf,sizeof(buf),"%llx",it->mode);
	if ( fw_mode < len )
	     fw_mode = len;

	len = print_mode_helper(buf,sizeof(buf),keytab,it->mode);
	if ( fw_settings < len )
	     fw_settings = len;
    }

    indent = NormalizeIndent(indent);
    fprintf(f,	"%*sAllowIP4[%s]: %u reference%s, %u/%u elements,"
		" fallback:%llx, allow:%llx\n",
		indent,"", title ? title : "",
		ai->ref_counter, ai->ref_counter == 1 ? "" : "s",
		ai->used, ai->size,
		ai->fallback_mode, ai->allow_mode );

    const uint seplen = 52 + fw_mode + fw_settings;
    if (print_tab_head)
	fprintf(f,"\n"
		"%*s counter  ipv4 address        ipv4/netmask  > %-*s"
		"  settings\n%*s%.*s\n",
		indent,"", fw_mode+4, "operation",
		indent,"", seplen, Minus300 );

    for ( i = 0; i < ai->used; i++ )
    {
	const AllowIP4Item_t *it = ai->list + i;
	print_mode_helper(buf,sizeof(buf),keytab,it->mode);
	fprintf(f, "%*s%8s  %-15s %08x/%08x > %s %*llx  %s\n",
		indent, "",
		PrintNumberU7(0,0,it->count,DC_SFORM_ALIGN|DC_SFORM_DASH),
		PrintIP4(0,0,it->addr,-1),
		it->addr, it->mask,
		it->mode & ALLOW_MODE_SET ? "SET"
			:  it->mode & ALLOW_MODE_AND ? "AND" : "OR ",
		fw_mode, it->mode, *buf ? buf : "-" );
    }

    if (print_tab_head)
	fprintf(f,"%*s%.*s\n", indent,"", seplen, Minus300 );
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u64 GetAllowIP4ByAddr ( const AllowIP4_t *ai, u32 addr, u64 if_not_found )
{
    DASSERT(ai);
    bool found = false;
    uint i;
    u64 res = 0;
    AllowIP4Item_t *it = (AllowIP4Item_t*)ai->list;
    for ( i = 0; i < ai->used; i++, it++ )
    {
	if ( ( addr & it->mask ) == it->addr )
	{
	    it->count++;
	    found = true;

	    if ( it->mode & ALLOW_MODE_SET )
		res = it->mode;
	    else if ( it->mode & ALLOW_MODE_AND )
		res &= it->mode;
	    else
		res |= it->mode;
	    if ( !(it->mode & ALLOW_MODE_CONTINUE) )
		break;
	}
    }

    return found ? res & ALLOW_MODE__MASK : if_not_found;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetAllowIP4ByName ( const AllowIP4_t *ai, mem_t name, u64 if_not_found )
{
    DASSERT(ai);
    NetworkHost_t host;
    ResolveHostMem(&host,true,name,0,false,false);
    return host.ip4_valid
	? GetAllowIP4ByAddr(ai,host.ip4,if_not_found)
	: if_not_found;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetAutoAllowIP4ByAddr ( const AllowIP4_t *ai, u32 addr )
{
    if (!ai)
	return ALLOW_MODE_ALLOW;

    const u64 res = GetAllowIP4ByAddr(ai,addr,M1(res));
    return IS_M1(res) ? ai->fallback_mode : res & ALLOW_MODE__MASK;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetAutoAllowIP4ByName ( const AllowIP4_t *ai, mem_t name )
{
    if (!ai)
	return ALLOW_MODE_ALLOW;

    NetworkHost_t host;
    ResolveHostMem(&host,true,name,0,false,false);
    return host.ip4_valid
	? GetAllowIP4ByAddr(ai,host.ip4,ai->fallback_mode)
	: ai->fallback_mode;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetAutoAllowIP4BySock ( const AllowIP4_t *ai, int sock )
{
    if (!ai)
	return ALLOW_MODE_ALLOW;

    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(sa);
    if ( getpeername(sock,(struct sockaddr*)&sa,&sa_len) || sa.sin_family != AF_INET )
	return ai->fallback_mode;

    return GetAutoAllowIP4ByAddr(ai,ntohl(sa.sin_addr.s_addr));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateAllowIP4
(
    FILE		*f,		// output file
    ccp			section,	// not NULL and not empty: create own section
    ccp			name_prefix,	// NULL or prefix of name
    uint		tab_pos,	// tab pos of '='
    const AllowIP4_t	*ai		// valid struct
)
{
    DASSERT(f);
    DASSERT(ai);

    if ( section && *section )
	fprintf(f,"\n#%.50s\n[%s]\n\n",Minus300,section);

    if (!name_prefix)
	name_prefix = EmptyString;

    const int base = 8*tab_pos + 7 - strlen(name_prefix);

    fprintf(f,
	"%s" "fallback%.*s= %#llx\n"
	"%s" "allow%.*s= %#llx\n"
	"%s" "n-rules%.*s= %u\n"
	,name_prefix, (base-8)/8, Tabs20, ai->fallback_mode
	,name_prefix, (base-5)/8, Tabs20, ai->allow_mode
	,name_prefix, (base-7)/8, Tabs20, ai->used
	);

    uint i;
    char name[30];

    for ( i = 0; i < ai->used; i++ )
    {
	const AllowIP4Item_t *it = ai->list + i;

	const uint len = snprintf(name,sizeof(name),"rule-%u",i);
	fprintf(f, "%s%s%.*s= 0x%08x,0x%08x,%#llx,%llu\n",
		name_prefix, name, (base-len)/8, Tabs20,
		it->addr, it->mask, it->mode, it->count );
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool ScanAllowIP4Item
(
    // return true on success; counter is optional

    AllowIP4Item_t	*it,	// store data here, cleared first
    ccp			line	// NULL or line to scan
)
{
    DASSERT(it);
    memset(it,0,sizeof(*it));
    if (line)
    {
	char *end;
	it->addr = str2ul(line,&end,10);
	if ( end > line )
	{
	    line = SkipControls1(end,',');
	    it->mask = str2ul(line,&end,10);
	    if ( end > line )
	    {
		line = SkipControls1(end,',');
		it->mode = str2ull(line,&end,10);
		if ( end > line )
		{
		    line = SkipControls1(end,',');
		    it->count = str2ull(line,0,10); // optional -> no error check
		    return true;
		}
	    }
	}
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateAllowIP4
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
)
{
    DASSERT(rs);
    if (user_table)
	RestoreStateAllowIP4Ex(rs,(AllowIP4_t*)user_table,0);
}

///////////////////////////////////////////////////////////////////////////////

void RestoreStateAllowIP4Ex
(
    RestoreState_t	*rs,		// info data, modified
    AllowIP4_t		*ai,		// valid struct
    ccp			name_prefix	// NULL or prefix of name
)
{
    DASSERT(ai);
    DASSERT(rs);

    ClearAllowIP4(ai);

    if (!name_prefix)
	name_prefix = EmptyString;

    char name[200]; // name buf

    snprintf(name,sizeof(name),"%sfallback",name_prefix);
    ai->fallback_mode	= GetParamFieldU64(rs,name,ai->fallback_mode);

    snprintf(name,sizeof(name),"%sallow",name_prefix);
    ai->allow_mode	= GetParamFieldU64(rs,name,ai->allow_mode);

    snprintf(name,sizeof(name),"%sn-rules",name_prefix);
    const uint n_rules	= GetParamFieldUINT(rs,name,0);

    uint i;
    for ( i = 0; i < n_rules; i++ )
    {
	snprintf(name,sizeof(name),"%srule-%u",name_prefix,i);
	ParamFieldItem_t *pfi = GetParamField(rs,name);
	AllowIP4Item_t ait;
	if ( pfi && ScanAllowIP4Item(&ait,(ccp)pfi->data) )
	{
	    if ( ai->used == ai->size )
	    {
		ai->size = 11*ai->size/10 + 10;
		ai->list = REALLOC(ai->list,ai->size*sizeof(*ai->list));
	    }
	    DASSERT( ai->used < ai->size );
	    DASSERT( ai->list );
	    ai->list[ai->used++] = ait;
	}
    }
}

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
)
{
    DASSERT(host);

    int sock = socket(AF_INET,type,protocol);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Can't create socket: %s\n",
			GetHostName(host));
	return -1;
    }

    if (connect(sock,(struct sockaddr *)&host->sa,sizeof(host->sa)))
    {
	if (!silent)
	    ERROR1(ERR_CANT_CONNECT,
			"Can't connect to %s:%d\n",
			GetHostName(host), host->port );
	close(sock);
	return -1;
    }

    return sock;
}

///////////////////////////////////////////////////////////////////////////////

int ConnectByHostTCP
(
    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	silent		// true: suppress error messages
)
{
    DASSERT(host);
    return ConnectByHost(host,SOCK_STREAM,IPPROTO_TCP,silent);
}

///////////////////////////////////////////////////////////////////////////////

int ConnectTCP
(
    ccp		addr,		// optional service + server + optional host
				//   e.g.: "service://host:port/....."
    int		default_port,	// default port
    bool	silent		// true: suppress error messages
)
{
    Setup_ConnectTCP_Hook(false);

    ccp unix_path = CheckUnixSocketPath(addr,0);
    if (unix_path)
	return ConnectUnixTCP(unix_path,silent);

    if (!strncasecmp(addr,"tcp:",4))
	addr += 4;

    NetworkHost_t host;
    int sock = -1;
    if (ResolveHost(&host,true,addr,default_port,false,true))
	sock = ConnectByHost(&host,SOCK_STREAM,IPPROTO_TCP,silent);
    ResetHost(&host);
    return sock;
}

///////////////////////////////////////////////////////////////////////////////

void Setup_ConnectTCP_Hook ( bool force )
{
    if ( force || !ConnectTCP_Hook )
	ConnectTCP_Hook = ConnectTCP;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int ConnectByHostUDP
(
    NetworkHost_t *host,	// valid pointer to NetworkHost_t
    bool	silent		// true: suppress error messages
)
{
    DASSERT(host);
    return ConnectByHost(host,SOCK_DGRAM,IPPROTO_UDP,silent);
}

///////////////////////////////////////////////////////////////////////////////

int ConnectUDP
(
    ccp		name,		// optional service + server + optional host
				//   e.g.: "service://host:port/....."
    int		default_host,	// default host
    bool	silent		// true: suppress error messages
)
{
    NetworkHost_t host;
    int sock = -1;
    if (ResolveHost(&host,true,name,default_host,false,true))
	sock = ConnectByHost(&host,SOCK_DGRAM,IPPROTO_UDP,silent);
    ResetHost(&host);
    return sock;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int WillReceiveNotBlock
(
    // returns: -1:error, 0:may block, 1:will not block

    int		fd,		// file handle of source
    uint	msec		// timeout: 0:no timeout, >0: milliseconds
)
{
    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(fd,&read_set);

    struct timeval timeout;
    timeout.tv_sec  = msec/1000;
    timeout.tv_usec = (msec%1000) * 1000;

    errno = 0;
    const int stat = select(fd+1,&read_set,0,0,&timeout);
    return stat < 0 ? stat : stat > 0 && FD_ISSET(fd,&read_set);
}

///////////////////////////////////////////////////////////////////////////////

ssize_t ReceiveTimeout
(
    // returns: <0: on error, 0:timeout, >0: bytes read

    int		fd,		// file handle of source
    void	*buf,		// valid pointer to buffer
    uint	size,		// size to receive
    int		flags,		// flags for recv()
    int		msec		// timeout: -1:unlimited, 0:no timeout, >0: milliseconds
)
{
    if ( fd == -1 )
	return -1;
    if ( !buf || !size )
	return 0;

    if ( msec >= 0 )
    {
	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(fd,&read_set);

	struct timeval timeout;
	timeout.tv_sec  = msec/1000;
	timeout.tv_usec = (msec%1000) * 1000;

	errno = 0;
	const int stat = select(fd+1,&read_set,0,0,&timeout);
	if ( stat <= 0 )
	    return stat;
    }

    return recv(fd,buf,size,flags);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int WillSendNotBlock
(
    // returns: -1:error, 0:may block, 1:will not block

    int		fd,		// file handle of source
    uint	msec		// timeout: 0:no timeout, >0: milliseconds
)
{
    fd_set write_set;
    FD_ZERO(&write_set);
    FD_SET(fd,&write_set);

    struct timeval timeout; // kein Warten
    timeout.tv_sec  = msec/1000;
    timeout.tv_usec = (msec%1000) * 1000;

    errno = 0;
    const int stat = select(fd+1,0,&write_set,0,&timeout);
    return stat < 0 ? stat : stat > 0 && FD_ISSET(fd,&write_set);
}

///////////////////////////////////////////////////////////////////////////////

ssize_t SendTimeout
(
    // returns: <0: on error, 0:timeout, >0: bytes written

    int		fd,		// file hande of destination
    const void	*data,		// valid pointer to data
    uint	size,		// size to receive
    int		flags,		// flags for send()
    int		msec,		// total timeout: -1:unlimited, 0:no timeout, >0: milliseconds
    bool	all		// true: send all data until done, total timeout or error
)
{
    if ( fd == -1 )
	return -1;
    if ( !data || !size )
	return 0;

    //--- send loop for complete sending

    if (all)
    {
	uint max_time = msec < 0 ? 0 : GetTimerMSec() + msec;
	ssize_t total = 0;
	while (size)
	{
	    ssize_t stat = SendTimeout(fd,data,size,flags,msec,false);
	    if ( stat < 0 )
		return stat;
	    data = (u8*)data + stat;
	    size -= stat;
	    total += stat;

	    if ( msec >= 0 )
	    {
		msec = max_time - GetTimerMSec();
		if ( msec < 0 )
		    break;
	    }
	}
	return total;
    }

    //--- send, single try

    if ( msec >= 0 )
    {
	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(fd,&write_set);

	struct timeval timeout;
	timeout.tv_sec  = msec/1000;
	timeout.tv_usec = (msec%1000) * 1000;

	errno = 0;
	const int stat = select(fd+1,0,&write_set,0,&timeout);
	if ( stat <= 0 )
	    return stat;
    }

    return send(fd,data,size,flags);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		IP interface: check sums		///////////////
///////////////////////////////////////////////////////////////////////////////

u16 CalcChecksumIP4
(
    const void		*data,	// ip4 header
				// make sure, that the checksum member is NULL
    uint		size	// size of ip4 header
)
{
    // http://en.wikipedia.org/wiki/IPv4_header_checksum

    DASSERT(data);
    const u16 *ptr = (u16*)data;
    u32 csum = 0;

    while ( size > 1 )
    {
	csum += ntohs(*ptr++);
	size -= 2;
    }

    if (size)
	csum += *(u8*)ptr << 8;

    return ~( ( csum >> 16 ) + ( csum & 0xffff ));
}

///////////////////////////////////////////////////////////////////////////////

u16 CalcChecksumUDP
(
    const ip4_head_t	*ip4,	// IP4 header
    const udp_head_t	*udp,	// UDP header
    const void		*data	// UDP data, size is 'udp->data_len'
)
{
    DASSERT(ip4);
    DASSERT(udp);
    DASSERT( data || !udp->data_len );

    int dlen = ntohs(udp->data_len);
    u32 csum = ntohs(((u16*)&ip4->ip_src)[0])
	     + ntohs(((u16*)&ip4->ip_src)[1])
	     + ntohs(((u16*)&ip4->ip_dest)[0])
	     + ntohs(((u16*)&ip4->ip_dest)[1])
	     + ip4->protocol
	     + ntohs(udp->port_src)
	     + ntohs(udp->port_dest)
	     + 2*dlen;

    dlen -= 8;
    const u16 *ptr = (u16*)data;
    while ( dlen > 1 )
    {
	csum += ntohs(*ptr++);
	dlen -= 2;
    }

    if ( dlen > 0 )
	csum += *(u8*)ptr << 8;

    while ( csum >= 0x10000 )
	csum = (u32)((u16*)&csum)[0] + (u32)((u16*)&csum)[1];

    return csum == 0xffff ? 0xffff : htons(~csum);
}

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
)
{
    if ( !data || !size )
	return ERR_NOTHING_TO_DO;

    char pathbuf[PATH_MAX];
    ccp path = PathCatPP(pathbuf,sizeof(pathbuf),path1,path2);
    PRINT("SendUnixUDP(%s)\n",path);

    static int sock = -1;
    if ( sock == -1 )
    {
	sock = socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);
	if ( sock == -1 )
	{
	    if (!silent)
		ERROR1(ERR_CANT_CREATE,
			    "Can't create UNIX stream socket: %s\n",path);
	    return ERR_CANT_CREATE;
	}
    }

    struct sockaddr_un sa;
    memset(&sa,0,sizeof(sa));
    sa.sun_family = AF_UNIX;
    StringCopyS(sa.sun_path,sizeof(sa.sun_path),path);

    if ( sendto(sock,data,size,0,(struct sockaddr*)&sa,sizeof(sa)) < 0 )
    {
	if (!silent)
	    ERROR1(ERR_WRITE_FAILED,
			"Can't send %u bytes to UNIX stream socket: %s\n",
			size, path );
	return ERR_WRITE_FAILED;
    }

    return ERR_OK;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Transfer Statistics		///////////////
///////////////////////////////////////////////////////////////////////////////

TransferStats_t * Add2TransferStats
(
    // return dest
    // calculate: dest += src

    TransferStats_t		*dest,	// NULL or destination and first source
    const TransferStats_t	*src	// NULL or second source
)
{
    if ( dest && src )
    {
	dest->conn_count += src->conn_count;
	dest->recv_count += src->recv_count;
	dest->recv_size  += src->recv_size;
	dest->send_count += src->send_count;
	dest->send_size  += src->send_size;
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

TransferStats_t * Sub2TransferStats
(
    // return dest
    // calculate: dest -= src

    TransferStats_t		*dest,	// NULL or destination and first source
    const TransferStats_t	*src	// NULL or second source
)
{
    if ( dest && src )
    {
	dest->conn_count -= src->conn_count;
	dest->recv_count -= src->recv_count;
	dest->recv_size  -= src->recv_size;
	dest->send_count -= src->send_count;
	dest->send_size  -= src->send_size;
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

TransferStats_t * Add3TransferStats
(
    // return dest
    // calculate: dest = src1 + src2

    TransferStats_t		*dest,	// NULL or destination (maybe same as source)
    const TransferStats_t	*src1,	// NULL or first source
    const TransferStats_t	*src2	// NULL or second source
)
{
    if (dest)
    {
	if (!src1)
	{
	    if (src2)
		memcpy(dest,src2,sizeof(*dest));
	    else
		memset(dest,0,sizeof(*dest));
	}
	else if (!src2)
	{
	    DASSERT(src1);
	    memcpy(dest,src1,sizeof(*dest));
	}
	else
	{
	    DASSERT(src1);
	    DASSERT(src2);
	    dest->conn_count = src1->conn_count + src2->conn_count;
	    dest->recv_count = src1->recv_count + src2->recv_count;
	    dest->recv_size  = src1->recv_size  + src2->recv_size;
	    dest->send_count = src1->send_count + src2->send_count;
	    dest->send_size  = src1->send_size  + src2->send_size;
	}
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

TransferStats_t * Sub3TransferStats
(
    // return dest
    // calculate: dest = src1 - src2

    TransferStats_t		*dest,	// NULL or destination (maybe same as source)
    const TransferStats_t	*src1,	// NULL or first source
    const TransferStats_t	*src2	// NULL or second source
)
{
    if (dest)
    {
	if (!src1)
	{
	    if (src2)
	    {
		TransferStats_t temp;
		memset(&temp,0,sizeof(temp));
		Sub3TransferStats(dest,&temp,src2);
	    }
	    else
		memset(dest,0,sizeof(*dest));
	}
	else if (!src2)
	{
	    DASSERT(src1);
	    memcpy(dest,src1,sizeof(*dest));
	}
	else
	{
	    DASSERT(src1);
	    DASSERT(src2);
	    dest->conn_count = src1->conn_count - src2->conn_count;
	    dest->recv_count = src1->recv_count - src2->recv_count;
	    dest->recv_size  = src1->recv_size  - src2->recv_size;
	    dest->send_count = src1->send_count - src2->send_count;
	    dest->send_size  = src1->send_size  - src2->send_size;
	}
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

TransferStats_t * SumTransferStats
(
    // return dest
    // calculate: dest = sum(all_sources)

    TransferStats_t		*dest,	// NULL or destination (maybe same as source)
    const TransferStats_t * const *src,	// NULL or source list, elements may be NULL
    int				n_src	// number of element; if -1: term at first NULL
)
{
    if (dest)
    {
	TransferStats_t temp;
	memset(&temp,0,sizeof(temp));

	if (src)
	{
	    if ( n_src < 0 )
		while ( *src )
		    Add2TransferStats(&temp,*src++);
	    else
		for ( ; *src; src++ )
		    if (*src)
			Add2TransferStats(&temp,*src);
	}
	memcpy(dest,&temp,sizeof(*dest));
    }
    return dest;
}

///////////////////////////////////////////////////////////////////////////////

ccp PrintTransferStatsSQL
(
    // print statistic as SQL assigning list.
    // arguments are speparated by a comma.

    char			*buf,		// result buffer
						// NULL: use a local circulary static buffer
    size_t			buf_size,	// size of 'buf', ignored if buf==NULL
    const TransferStats_t	*ts,		// valid source
    ccp				prefix		// not NULL: prefix member names
)
{
    DASSERT(ts);
    const bool alloced = !buf;
    if (alloced)
	buf = GetCircBuf( buf_size = CIRC_BUF_MAX_ALLOC );

    if (!prefix)
	prefix = "";

    const uint len
	= snprintf( buf, buf_size,
		"%sconn=%u, %srecv_n=%u, %srecv=%llu, %ssend_n=%u, %ssend=%llu",
		prefix, ts->conn_count,
		prefix, ts->recv_count,
		prefix, ts->recv_size,
		prefix, ts->send_count,
		prefix, ts->send_size );

    if ( alloced && len < buf_size )
	ReleaseCircBuf(buf+buf_size,buf_size-len-1);
    return buf;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static inline u32 calc_rate ( u64 count, u64 duration )
	{ return ( count * USEC_PER_SEC + duration/2 ) / duration; }

static inline double calc_drate ( u64 count, u64 duration )
	{ return (double)( count * USEC_PER_SEC ) / duration; }

///////////////////////////////////////////////////////////////////////////////

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
)
{
    DASSERT(f);
    DASSERT(name);
    DASSERT(stat);

    if (!prefix)
	prefix = "";

    if (!colset)
	colset = GetColorSet0();
    ccp color1 = colset->status;
    ccp color0 = colset->reset;

    if (stat->conn_count)
    {
	fprintf(f,
		"%s%s#STAT-%-5s %u connect%s,"
		" %u packet%s (%s) received, %u packet%s (%s) send.%s\n",
		prefix, color1, name,
		stat->conn_count, stat->conn_count == 1 ? "" : "s",
		stat->recv_count, stat->recv_count == 1 ? "" : "s",
		PrintSize1024(0,0,stat->recv_size,0),
		stat->send_count, stat->send_count == 1 ? "" : "s",
		PrintSize1024(0,0,stat->send_size,0),
		color0 );
    }
    else if ( stat->recv_count || stat->send_count )
    {
	fprintf(f,
		"%s%s#STAT-%-5s %u packet%s (%s) received, %u packet%s (%s) send.%s\n",
		prefix, color1, name,
		stat->recv_count, stat->recv_count == 1 ? "" : "s",
		PrintSize1024(0,0,stat->recv_size,0),
		stat->send_count, stat->send_count == 1 ? "" : "s",
		PrintSize1024(0,0,stat->send_size,0),
		color0 );
    }
    else
	return;

    if ( duration > 0 && prev )
    {
	TransferStats_t delta;
	Sub3TransferStats(&delta,stat,prev);

	char duration_buf[10];
	if (!duration_info)
	{
	    PrintTimerUSec6(duration_buf,sizeof(duration_buf),duration,false);
	    duration_info = duration_buf;
	}

	if ( delta.recv_count )
	{
	    const double crate = calc_drate( delta.recv_count, duration );
	    const u32 srate = calc_rate( delta.recv_size, duration );
	    fprintf(f,
		"%s%s#STAT-%-5s   >Last "
		"%s: %u (%4.2f/s) packet%s with %s (%s/s) received.%s\n",
		prefix, color1, name,
		duration_info,
		delta.recv_count, crate,
		delta.recv_count == 1 ? "" : "s",
		PrintSize1024(0,0,delta.recv_size,0),
		PrintSize1024(0,0,srate,0),
		color0 );
	}

	if ( delta.send_count )
	{
	    const double crate = calc_drate( delta.send_count, duration );
	    const u32 srate = calc_rate( delta.send_size, duration );
	    fprintf(f,
		"%s%s#STAT-%-5s   >Last "
		"%s: %u (%4.2f/s) packet%s with %s (%s/s) send.%s\n",
		prefix, color1, name,
		duration_info,
		delta.send_count, crate,
		delta.send_count == 1 ? "" : "s",
		PrintSize1024(0,0,delta.send_size,0),
		PrintSize1024(0,0,srate,0),
		color0 );
	}
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateTransferStats
(
    FILE		*f,		// output file
    const TransferStats_t *stat		// valid stat object
)
{
    DASSERT(f);
    DASSERT(stat);

    fprintf(f,
       "conn-count	= %u\n"
       "recv-count	= %u\n"
       "recv-size	= %llu\n"
       "send-count	= %u\n"
       "send-size	= %llu\n"
	,stat->conn_count
	,stat->recv_count
	,stat->recv_size
	,stat->send_count
	,stat->send_size
	);
}

///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateTransferStats1
(
    // print as single line
    FILE		*f,		// output file
    ccp			name,		// var name; use "tfer-stat" if NULL or EMPTY
    const TransferStats_t *stat		// valid stat object
)
{
    DASSERT(f);
    DASSERT(stat);

    if ( !name || !*name )
	name = "tfer-stat";
    const int tab_size = ( 23 - strlen(name) ) / 8;

    fprintf(f,
	"%s%.*s= %u,%u,%llu,%u,%llu\n"
	,name
	,tab_size > 0 ? tab_size : 0
	,"\t\t"
	,stat->conn_count
	,stat->recv_count
	,stat->recv_size
	,stat->send_count
	,stat->send_size
	);
}

///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateTransferStatsN
(
    // print multiple stats each as single line
    FILE		*f,		// output file
    ccp			name,		// var name; use "tfer-stat-" if NULL or EMPTY
    const TransferStats_t *stat,	// list of valid stat objects
    uint		n_stat		// number of elements in 'stat'
)
{
    DASSERT(f);
    DASSERT(stat||!n_stat);

    if ( !name || !*name )
	name = "tfer-stat-";
    const int tab_size = ( 23 - strlen(name) ) / 8;

    fprintf(f,"%sn%.*s= %u\n",
	name, tab_size > 0 ? tab_size : 0, "\t\t", n_stat );

    uint i;
    char buf[100];
    for ( i = 0; i < n_stat; i++ )
    {
	snprintf(buf,sizeof(buf),"%s%u",name,i);
	SaveCurrentStateTransferStats1(f,buf,stat++);
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateTransferStats
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
)
{
    DASSERT(rs);

    TransferStats_t *stat = (TransferStats_t*)user_table;
    if (!stat)
	return;

    stat->conn_count = GetParamFieldUINT( rs, "conn-count", stat->conn_count );
    stat->recv_count = GetParamFieldUINT( rs, "recv-count", stat->recv_count );
    stat->send_count = GetParamFieldUINT( rs, "send-count", stat->send_count );
    stat->recv_size  = GetParamFieldU64 ( rs, "recv-size",  stat->recv_size  );
    stat->send_size  = GetParamFieldU64 ( rs, "send-size",  stat->send_size  );
}

///////////////////////////////////////////////////////////////////////////////

void RestoreStateTransferStats1
(
    // scan single line
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    ccp			name,		// var name; use "tfer-stat" if NULL or EMPTY
    TransferStats_t	*stat,		// valid stat object
    bool		fall_back	// true: fall back to RestoreStateTransferStats()
)
{
    DASSERT(rs);
    DASSERT(stat);

    if ( !name || !*name )
	name = "tfer-stat";

    char buf[100];
    int res = GetParamFieldBUF( buf, sizeof(buf), rs, name, ENCODE_OFF, 0 );
    if ( res > 0 )
    {
	memset(stat,0,sizeof(*stat));

	char *ptr;
	stat->conn_count = strtoul (buf,&ptr,10); if (*ptr == ',' ) ptr++;
	stat->recv_count = strtoul (ptr,&ptr,10); if (*ptr == ',' ) ptr++;
	stat->recv_size  = strtoull(ptr,&ptr,10); if (*ptr == ',' ) ptr++;
	stat->send_count = strtoul (ptr,&ptr,10); if (*ptr == ',' ) ptr++;
	stat->send_size  = strtoull(ptr,&ptr,10);
	return;
    }

    if (fall_back)
	RestoreStateTransferStats(rs,stat);
}

///////////////////////////////////////////////////////////////////////////////

uint RestoreStateTransferStatsN
(
    // print multiple stats each as single line
    // returns the number of read elements; all others are set to NULL
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    ccp			name,		// var name; use "tfer-stat-" if NULL or EMPTY
    TransferStats_t	*stat,		// list of valid stat objects
    uint		n_stat		// number of elements in 'stat'
)
{
    DASSERT(rs);
    DASSERT(stat||!n_stat);

    if ( !name || !*name )
	name = "tfer-stat-";

    memset(stat,0,n_stat*sizeof(*stat));

    char buf[100];
    snprintf(buf,sizeof(buf),"%sn",name);
    uint n_read = GetParamFieldUINT(rs,buf,0);
    if ( n_read > n_stat )
	 n_read = n_stat;

    uint i;
    for ( i = 0; i < n_read; i++ )
    {
	snprintf(buf,sizeof(buf),"%s%u",name,i);
	RestoreStateTransferStats1(rs,buf,stat++,false);
    }

    return n_read;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  TCP HANDLER			///////////////
///////////////////////////////////////////////////////////////////////////////

#define LOG_TCP HAVE_PRINT0

#undef INC_TCP_INDENT
#undef DEC_TCP_INDENT
#undef LOG_TCP_BUFFER

#if LOG_TCP
    static int tcp_indent = 5;
    #define INC_TCP_INDENT tcp_indent += 2
    #define DEC_TCP_INDENT tcp_indent -= 2
    #define LOG_TCP_BUFFER(tb,f,...) LogGrowBuffer(stderr,tcp_indent,tb,f,__VA_ARGS__)
    #define LOG_TCP_STREAM(ts,r,f,...) LogTCPStream(stderr,tcp_indent,ts,r,f,__VA_ARGS__)
    #define LOG_TCP_HANDLER(th,r,f,...) LogTCPHandler(stderr,tcp_indent,th,r,f,__VA_ARGS__)
#else
    #define INC_TCP_INDENT
    #define DEC_TCP_INDENT
    #define LOG_TCP_BUFFER(tb,f,...)
    #define LOG_TCP_STREAM(ts,r,f,...)
    #define LOG_TCP_HANDLER(th,r,f,...)
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    TCPStream_t			///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeTCPStream ( TCPStream_t *ts, int sock )
{
    DASSERT(ts);
    memset(ts,0,sizeof(*ts));
    ts->unique_id = CreateUniqueId();
    ts->sock = sock;
    ts->poll_index = M1(ts->poll_index);
    LOG_TCP_STREAM(ts,0,"%s","INIT()");
    INC_TCP_INDENT;
    InitializeGrowBuffer(&ts->ibuf,0x4000);
    InitializeGrowBuffer(&ts->obuf,0x4000);
    DEC_TCP_INDENT;
}

///////////////////////////////////////////////////////////////////////////////

void ResetTCPStream
(
    TCPStream_t *ts		// valid TCP handler
)
{
    DASSERT(ts);
    LogTCPStreamActivity(ts,"ResetTCPStream");
    TCPHandler_t *th = ts->handler;

    if (th)
    {
	if (th->OnDestroyStream)
	    th->OnDestroyStream(ts);
	th->used_streams--;
    }

    if ( ts->sock != -1 )
    {
	shutdown(ts->sock,SHUT_RDWR);
	close(ts->sock);
	ts->sock = -1;
    }

    INC_TCP_INDENT;
    ResetGrowBuffer(&ts->ibuf);
    ResetGrowBuffer(&ts->obuf);
    DEC_TCP_INDENT;

    if (th)
    {
	// chain is only valid if part of TCP-handler
	if (ts->prev)
	    ts->prev->next = ts->next;
	if (ts->next)
	    ts->next->prev = ts->prev;
	if ( th->first == ts )
	    th->first = ts->next;
    }
    memset(ts,0,sizeof(*ts));
}

///////////////////////////////////////////////////////////////////////////////

void DestroyTCPStream
(
    TCPStream_t *ts		// valid TCP handler
)
{
    DASSERT(ts);
    LogTCPStreamActivity(ts,"DestroyTCPStream");
    ResetTCPStream(ts);
    FREE(ts);
}

///////////////////////////////////////////////////////////////////////////////

ssize_t UpdateRecvStatTCPStream
(
    // returns 'send_stat'

    TCPStream_t		*ts,		// valid TCP handler
    ssize_t		recv_stat,	// result of recv(), update stats on >0
    u64			now_usec	// NULL or current time
)
{
    DASSERT(ts);
    if ( recv_stat > 0 )
    {
	ts->receive_usec = now_usec ? now_usec : GetTimeUSec(false);
	ts->stat.recv_count++;
	ts->stat.recv_size += recv_stat;

	if (ts->xstat)
	{
	    ts->xstat->recv_count++;
	    ts->xstat->recv_size += recv_stat;
	}

	if (ts->handler)
	{
	    ts->handler->stat.recv_count++;
	    ts->handler->stat.recv_size += recv_stat;
	}
    }
    return recv_stat;
}

///////////////////////////////////////////////////////////////////////////////

ssize_t UpdateSendStatTCPStream
(
    // returns 'send_stat'

    TCPStream_t		*ts,		// valid TCP handler
    ssize_t		send_stat,	// result of send(), update stats on >0
    u64			now_usec	// NULL or current time
)
{
    DASSERT(ts);
    if ( send_stat > 0 )
    {
	ts->send_usec = now_usec ? now_usec : GetTimeUSec(false);
	ts->stat.send_count++;
	ts->stat.send_size += send_stat;

	if (ts->xstat)
	{
	    ts->xstat->send_count++;
	    ts->xstat->send_size += send_stat;
	}

	if (ts->handler)
	{
	    ts->handler->stat.send_count++;
	    ts->handler->stat.send_size += send_stat;
	}
    }
    return send_stat;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int WriteDirectGrowBuffer
(
    // write() data direct without blocking and without calling any notifier.
    // If output buf is not empty or send failed, append the data to 'gb'.
    // Returns the number of bytes added to the grow buffer or -1 on error.
    // The data is send+stored completely (returns 'size') or not at all
    // (returns '0'). -1 is returned if the socket is invalid.

    GrowBuffer_t	*gb,		// grow buffer to cache data
    int			fd,		// destination file
    bool		flush_output,	// true: try to flush 'gb' before
    cvp			data,		// data to send
    uint		size,		// size of 'data', If NULL && flush: flush only
    uint		*send_count	// not NULL: add num of sent bytes
)
{
    DASSERT(gb);
    DASSERT(data||!size);

    const u8 *d = data;
    if ( fd != -1 )
    {
	int final_stat = 0;
	if ( gb->disabled <= 0 )
	{
	    if ( flush_output && gb->used )
	    {
		ssize_t stat = write(fd,gb->ptr,gb->used);
		if ( stat > 0 )
		{
		    if (send_count)
			*send_count += stat;
		    DropGrowBuffer(gb,stat);
		}
	    }

	    if ( !gb->used && size )
	    {
		ssize_t stat = write(fd,d,size);
		if ( stat > 0 )
		{
		    if (send_count)
			*send_count += stat;

		    size -= stat;
		    if (!size) // likely
			return stat;

		    d += stat;
		    final_stat = stat;

		    // for this case: force growing buffer (because of 'all or none')
		    PrepareGrowBuffer(gb,size,true);
		}
	    }
	}

	if ( size && size <= GetSpaceGrowBuffer(gb) )
	    final_stat += InsertGrowBuffer(gb,d,size);
	return final_stat;
    }

    return -1;
}

///////////////////////////////////////////////////////////////////////////////

int SendDirectGrowBuffer
(
    // send() data direct without blocking and without calling any notifier.
    // If output buf is not empty or send failed, append the data to 'gb'.
    // Returns the number of bytes added to the grow buffer or -1 on error.
    // The data is send+stored completely (returns 'size') or not at all
    // (returns '0'). -1 is returned if the socket is invalid.

    GrowBuffer_t	*gb,		// grow buffer to cache data
    int			sock,		// destination socket
    bool		flush_output,	// true: try to flush 'gb' before
    cvp			data,		// data to send
    uint		size,		// size of 'data', If NULL && flush: flush only
    uint		*send_count	// not NULL: add num of sent bytes
)
{
    DASSERT(gb);
    DASSERT(data||!size);

    const u8 *d = data;
    if ( sock != -1 )
    {
	int final_stat = 0;
	if ( gb->disabled <= 0 )
	{
	    if ( flush_output && gb->used )
	    {
		ssize_t stat = send(sock,gb->ptr,gb->used,MSG_DONTWAIT);
		if ( stat > 0 )
		{
		    if (send_count)
			*send_count += stat;
		    DropGrowBuffer(gb,stat);
		}
	    }

	    if ( !gb->used && size )
	    {
		ssize_t stat = send(sock,d,size,MSG_DONTWAIT);
		if ( stat > 0 )
		{
		    if (send_count)
			*send_count += stat;

		    size -= stat;
		    if (!size) // likely
			return stat;

		    d += stat;
		    final_stat = stat;

		    // for this case: force growing buffer (because of 'all or none')
		    PrepareGrowBuffer(gb,size,true);
		}
	    }
	}

	if ( size && size <= GetSpaceGrowBuffer(gb) )
	    final_stat += InsertGrowBuffer(gb,d,size);
	return final_stat;
    }

    return -1;
}

///////////////////////////////////////////////////////////////////////////////

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
)
{
    DASSERT(ts);
    DASSERT(data||!size);

    uint count = 0;
    const int stat
	= SendDirectGrowBuffer(&ts->obuf,ts->sock,flush_output,data,size,&count);
    if ( count > 0 )
	UpdateSendStatTCPStream(ts,count,0);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int PrintArgDirectTCPStream
(
    // Printing interface for SendDirectTCPStream()

    TCPStream_t		*ts,		// valid TCP handler
    bool		flush_output,	// true: try to flush out-buf before
    ccp			format,		// format string for vsnprintf()
    va_list		arg		// parameters for 'vfprintf(...,format,...)'
)
{
    DASSERT(ts);

    if (!format)
	return SendDirectTCPStream(ts,flush_output,0,0);

    char buf[10000];

    va_list arg2;
    va_copy(arg2,arg);
    int stat = vsnprintf(buf,sizeof(buf),format,arg2);
    va_end(arg2);

    if ( stat < sizeof(buf) )
	return SendDirectTCPStream(ts,flush_output,buf,stat);


    //--- buffer too small, use dynamic memory

    noPRINT("PrintArgGrowBuffer() -> MALLOC(%u)\n",stat+1);

    char *temp = MALLOC(stat+1);
    stat = vsnprintf(temp,stat+1,format,arg);
    stat = SendDirectTCPStream(ts,flush_output,temp,stat);
    FREE(temp);
    return stat;
}

///////////////////////////////////////////////////////////////////////////////

int PrintDirectTCPStream
(
    // Printing interface for SendDirectTCPStream()

    TCPStream_t		*ts,		// valid TCP handler
    bool		flush_output,	// true: try to flush out-buf before
    ccp			format,		// format string for vfprintf()
    ...					// arguments for 'vfprintf(...,format,...)'
)
{
    DASSERT(ts);

    if (format)
    {
	va_list arg;
	va_start(arg,format);
	const int stat = PrintArgDirectTCPStream(ts,flush_output,format,arg);
	va_end(arg);
	return stat;
    }

    return SendDirectTCPStream(ts,flush_output,0,0);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int PrintBinary1TCPStream
(
    TCPStream_t	*ts,		// NULL or destination
    ccp		cmd,		// command name
    cvp		bin_data,	// pointer to binary data
    uint	bin_size,	// size of binary data
    ccp		format,		// NULL or format string with final ';'
    ...				// parameters for 'format'
)
{
    DASSERT(cmd);
    DASSERT(bin_data||!bin_size);

    if (!ts)
	return -1;

    char buf[10000], *end_buf = buf + sizeof(buf) - 4;
 #if 0 // gcc-12: throws error: 'buf' may be used uninitialized
    char *dest = StringCopyE(buf,end_buf,cmd);
 #else
    char *dest = StringCopyS(buf,sizeof(buf)-4,cmd);
 #endif
    if ( dest + 6 + bin_size >= end_buf )
	return -1;

    *dest++ = ' ';
    *dest++ = 1;
    write_be16(dest,bin_size);
    dest += 2;
    memcpy(dest,bin_data,bin_size);
    dest += bin_size;

    *dest++ = ' ';
    *dest++ = '=';

    int len;
    if (format)
    {
	va_list arg;
	va_start(arg,format);
	len = vsnprintf(dest,end_buf-dest,format,arg);
	va_end(arg);

	if ( len < 0 )
	    return -1;
	len += dest - buf;
	if ( len >= sizeof(buf) )
	    return -1;
    }
    else
    {
	*dest++ = ';';
	len = dest - buf;
    }

    return SendDirectTCPStream(ts,false,buf,len);
}

///////////////////////////////////////////////////////////////////////////////

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
{
    DASSERT(cmd);
    DASSERT(bin1_data||!bin1_size);
    DASSERT(bin2_data||!bin2_size);

    if (!ts)
	return -1;

    char buf[10000], *end_buf = buf + sizeof(buf) - 4;
 #if 0 // gcc-12: throws error: 'buf' may be used uninitialized
    char *dest = StringCopyE(buf,end_buf,cmd);
 #else
    char *dest = StringCopyS(buf,sizeof(buf)-4,cmd);
 #endif
    if ( dest + 9 + bin1_size + bin2_size >= end_buf )
	return -1;

    *dest++ = ' ';
    *dest++ = 1;
    write_be16(dest,bin1_size);
    dest += 2;
    memcpy(dest,bin1_data,bin1_size);
    dest += bin1_size;

    *dest++ = 1;
    write_be16(dest,bin2_size);
    dest += 2;
    memcpy(dest,bin2_data,bin2_size);
    dest += bin2_size;

    *dest++ = ' ';
    *dest++ = '=';

    int len;
    if (format)
    {
	va_list arg;
	va_start(arg,format);
	len = vsnprintf(dest,end_buf-dest,format,arg);
	va_end(arg);

	if ( len < 0 )
	    return -1;
	len += dest - buf;
	if ( len >= sizeof(buf) )
	    return -1;
    }
    else
    {
	*dest++ = ';';
	len = dest - buf;
    }

    return SendDirectTCPStream(ts,false,buf,len);
}

///////////////////////////////////////////////////////////////////////////////
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
{
    DASSERT(f);
    DASSERT(ts);
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sTS: id:%x, sock=%d, %u packets received (%s), %u packets send (%s)",
	indent,"",
	ts->unique_id, ts->sock,
	ts->stat.recv_count, PrintSize1024(0,0,ts->stat.recv_size,0),
	ts->stat.send_count, PrintSize1024(0,0,ts->stat.send_size,0) );

    if (format)
    {
	fputs(" : ",f);
	va_list arg;
	va_start(arg,format);
	vfprintf(f,format,arg);
	va_end(arg);
    }

    fputc('\n',f);

    if (recurse>0)
    {
	LogGrowBuffer(f,indent+2,&ts->ibuf,"in buf");
	LogGrowBuffer(f,indent+2,&ts->obuf,"out buf");
    }
}

///////////////////////////////////////////////////////////////////////////////

void LogTCPStreamActivity
(
    const TCPStream_t	*ts,		// valid TCP handler
    ccp			activity
)
{
    if (!ts->tracelog)
	return;

    char hbuf[200];
    if (ts->handler)
    {
	const TCPHandler_t *th = ts->handler;
	const Socket_t     *so = th->listen;

	DASSERT( TCP_HANDLER_MAX_LISTEN >= 3 );
	snprintf(hbuf,sizeof(hbuf),
		" | TH#%u: %u/%u/%u, sock=%d,%d,%d",
		th->unique_id,
		th->used_streams, th->max_used_streams, th->total_streams,
		so[0].sock, so[1].sock, so[2].sock  );
    }
    else
	*hbuf = 0;

    TraceLogPrint(ts->tracelog,
	"%p %-20s sock=%d%s\n", ts, activity, ts->sock, hbuf );
}

///////////////////////////////////////////////////////////////////////////////

char * BufInfoHeadTCPStream
(
    // returns a pointer to the head line

    uint		line		// 0|1
)
{
    switch (line)
    {
     case 0:
	return	"connect                in buf   out buf"
		"    received data        send data";

     case 1:
	return	"  time sock status   use/ max  use/ max"
		"   time   pkt  size   time   pkt  size  stream info";
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

char * BufInfoTCPStream
(
    // returns a pointer to the buffer

    char		* buf,		// result (BUF_INFO_TCP_STREAM_SIZE bytes are good)
					// If NULL, a local circulary static buffer is used
    size_t		buf_size,	// size of 'buf', ignored if buf==NULL
    const TCPStream_t	*ts,		// valid TCP handler
    u64			now_usec	// NULL or current time, GetTimeUSec(false)
)
{
    DASSERT(ts);

    if (!now_usec)
	now_usec = GetTimeUSec(false);

    char connect_usec[7];
    if ( ts->connect_usec )
	PrintTimerUSec6(connect_usec,sizeof(connect_usec),
			now_usec - ts->connect_usec, true );
    else
	connect_usec[0] = '-', connect_usec[1] = 0;

    char receive_usec[7];
    if ( ts->receive_usec )
	PrintTimerUSec6(receive_usec,sizeof(receive_usec),
			now_usec - ts->receive_usec, true );
    else
	receive_usec[0] = '-', receive_usec[1] = 0;

    char send_usec[7];
    if ( ts->send_usec )
	PrintTimerUSec6(send_usec,sizeof(send_usec),
			now_usec - ts->send_usec, true );
    else
	send_usec[0] = '-', send_usec[1] = 0;

    if (!buf)
	buf = GetCircBuf( buf_size = BUF_INFO_TCP_STREAM_SIZE );

    snprintf(buf,buf_size,
	"%6.6s%5d %c%c%c%c%c%c%c %s/%s %s/%s %6s %s %s %6s %s %s  %s"
	,connect_usec
	,ts->sock
	,ts->protect > 0	? 'P' : '-'
	,ts->auto_close > 0	? 'A' : '-'
	,ts->rescan > 0		? 'S' : '-'
	,ts->eof > 0		? 'X' : '-'
	,ts->error > 0		? 'E' : '-'
	,ts->ibuf.disabled	? 'D' : 'i'
	,ts->obuf.disabled	? 'D' : 'o'
	,PrintNumberU4(0,0,ts->ibuf.used,true)
	,PrintNumberU4(0,0,ts->ibuf.max_used,true)
	,PrintNumberU4(0,0,ts->obuf.used,true)
	,PrintNumberU4(0,0,ts->obuf.max_used,true)
	,receive_usec
	,PrintNumberU5(0,0,ts->stat.recv_count,true)
	,PrintSize1024(0,0,ts->stat.recv_size,DC_SFORM_TINY|DC_SFORM_ALIGN)
	,send_usec
	,PrintNumberU5(0,0,ts->stat.send_count,true)
	,PrintSize1024(0,0,ts->stat.send_size,DC_SFORM_TINY|DC_SFORM_ALIGN)
	,ts->info
	);

    return buf;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    TCPHandler_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void AddSocketTCPStream
(
    TCPStream_t		*ts,		// valid TCP handler
    FDList_t		*fdl		// valid file descriptor list
)
{
    DASSERT(ts);
    DASSERT(fdl);

    if ( ts->OnFDList && ts->OnFDList(ts,fdl,TCP_FM_ADD_SOCK,false) < 0 || ts->sock == -1 )
	return;

    uint events = POLLERR|POLLRDHUP;
    if ( !ts->eof && !ts->ibuf.disabled && GetSpaceGrowBuffer(&ts->ibuf) )
	events |= POLLIN;
    if ( !ts->obuf.disabled && ts->obuf.used )
	events |= POLLOUT;

    ts->poll_index = AddFDList(fdl,ts->sock,events);

    if ( ts->OnTimeout && ts->trigger_usec && fdl->timeout_usec > ts->trigger_usec )
	fdl->timeout_usec = ts->trigger_usec;
}

///////////////////////////////////////////////////////////////////////////////

void CheckSocketTCPStream
(
    TCPStream_t		*ts,		// valid TCP handler
    FDList_t		*fdl,		// valid socket list
    bool		check_timeout	// true: enable timeout checks
)
{
    DASSERT(ts);
    DASSERT(fdl);

    if ( ts->OnFDList && ts->OnFDList(ts,fdl,TCP_FM_CHECK_SOCK,check_timeout) < 0
	|| ts->sock == -1 )
    {
	return;
    }

    const u64 now_usec = GetTimeUSec(false);

    typeof(ts->rescan) rescan = ts->rescan;
    ts->rescan = 0;

    const uint revents = GetEventFDList(fdl,ts->sock,ts->poll_index);

    if ( !ts->ibuf.disabled && revents & POLLIN )
    {
	noPRINT("RECV: %d\n",ts->sock);
	OnReceivedStream(ts,now_usec);
	rescan = 0;
    }

    if ( !ts->obuf.disabled && ts->obuf.used && revents & POLLOUT )
	OnWriteStream(ts,now_usec);

    if ( revents & (POLLERR|POLLHUP|POLLRDHUP|POLLNVAL) )
    {
	ts->error |= 1;
	OnCloseStream(ts,now_usec);
    }

    if ( rescan && ts->OnReceived )
    {
	uchar ch = 0;
	ts->OnReceived(ts,&ch,0);
    }

    if (check_timeout)
	CheckTimeoutTCPStream(ts);

    if (ts->rescan)
	ts->trigger_usec = ts->accept_usec = now_usec;
}

///////////////////////////////////////////////////////////////////////////////

#undef PRINT_TIMEOUT
#define PRINT_TIMEOUT(ts,now,info) \
    noPRINT("%s[%d]: now=%lld, trig=%lld, accept=%lld, OnTimeout=%d\n", \
	info, ts->sock, now, ts->trigger_usec ? ts->trigger_usec - now : 0, \
	ts->accept_usec  ? ts->accept_usec  - now : 0, ts->OnTimeout != 0 )

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void CheckTimeoutTCPStream ( TCPStream_t *ts )
{
    DASSERT(ts);

    if ( ts->OnFDList && ts->OnFDList(ts,0,TCP_FM_TIMEOUT,true) < 0 || ts->sock == -1 )
	return;

    const u64 now_usec = GetTimeUSec(false);
    PRINT_TIMEOUT(ts,now_usec,"TIMEOUT");

    if (  ts->trigger_usec && ts->trigger_usec <= now_usec
       || ts->accept_usec  && ts->accept_usec  <= now_usec
       )
    {
	ts->trigger_usec = ts->accept_usec = 0;
	if (ts->OnTimeout)
	    ts->OnTimeout(ts,now_usec);
    }
}

///////////////////////////////////////////////////////////////////////////////

u64 GetLastActivityTCPStream ( TCPStream_t *ts )
{
    DASSERT(ts);
    const u64 ref = ts->connect_usec > ts->receive_usec
		  ? ts->connect_usec : ts->receive_usec;
    return ref > ts->send_usec ? ref : ts->send_usec;
}

///////////////////////////////////////////////////////////////////////////////

u64 GetTimeoutTCPStream ( TCPStream_t *ts )
{
    DASSERT(ts);
    return ts->timeout_usec
	? GetLastActivityTCPStream(ts) + ts->timeout_usec
	: 0;
}

///////////////////////////////////////////////////////////////////////////////

int OnTimeoutTCPStream ( TCPStream_t *ts, u64 now_usec )
{
    DASSERT(ts);
    if ( ts->sock != -1 && ts->timeout_usec )
    {
	if (!now_usec)
	    now_usec = GetTimeUSec(false);

	if ( now_usec >= GetLastActivityTCPStream(ts) + ts->timeout_usec )
	    OnCloseStream(ts,now_usec);
	else
	    SetupTriggerTCPStream(ts,now_usec);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

void SetupTriggerTCPStream ( TCPStream_t *ts, u64 now_usec )
{
    DASSERT(ts);

    const u64 next = GetTimeoutTCPStream(ts);
    if (next)
    {
	if (!now_usec)
	    now_usec = GetTimeUSec(false);

	if ( ts->accept_usec <= now_usec || ts->accept_usec > next )
	    ts->accept_usec = next;
	ts->trigger_usec = ts->accept_usec + ts->delay_usec;

	noPRINT("next=%lld [%lld], accept=%lld [%lld]\n",
		next, next-now_usec, ts->accept_usec, ts->accept_usec-now_usec );
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    TCPHandler_t		///////////////
///////////////////////////////////////////////////////////////////////////////

void InitializeTCPHandler
(
    TCPHandler_t *th,		// not NULL
    uint	data_size	// size of 'TCPStream_t::data'
)
{
    DASSERT(th);
    memset(th,0,sizeof(*th));
    th->unique_id	= CreateUniqueId();
    th->data_size	= data_size;
    th->OnAllowStream	= IsStreamAllowed;

    uint i;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	th->listen[i].sock = -1;
    LOG_TCP_HANDLER(th,0,"INIT(%u)",data_size);
}

///////////////////////////////////////////////////////////////////////////////

void ResetTCPHandler
(
    TCPHandler_t *th		// valid TCP handler
)
{
    DASSERT(th);
    th->allow_ip4 = DeleteReferenceAllowIP4(th->allow_ip4);
    UnlistenAllTCP(th);

    while (th->first)
	DestroyTCPStream(th->first);
    usleep(1);

    InitializeTCPHandler(th,th->data_size);
}

///////////////////////////////////////////////////////////////////////////////

uint CloseTCPHandler
(
    // returns the number of waiting clients (obuf not empty and !disabled)
    TCPHandler_t *th		// valid TCP handler
)
{
    DASSERT(th);
    UnlistenAllTCP(th);

    uint count = 0;
    u64 now = GetTimeUSec(false);
    TCPStream_t *ts;
    for ( ts = th->first; ts; ts = ts->next )
    {
	OnCloseStream(ts,now);
	if ( ts->obuf.used && !ts->obuf.disabled )
	    count++;
    }
    return count;
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * AddTCPStream
(
    TCPHandler_t *th,		// valid TCP handler
    TCPStream_t	 *ts		// the new stream
)
{
    DASSERT(th);
    DASSERT(ts);

    ts->handler = th;
    ts->prev = 0;
    ts->next = th->first;
    if (th->first)
	th->first->prev = ts;
    th->first = ts;

    if ( th->max_used_streams < ++th->used_streams )
	th->max_used_streams = th->used_streams;
    th->total_streams++;
    th->stat.conn_count++;
    ts->connect_usec = GetTimeUSec(false);

    LogTCPStreamActivity(ts,"AddTCPStream");
    return ts;
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * CreateTCPStream
(
    TCPHandler_t	*th,		// valid TCP handler
    int			sock,		// -1 or valid socket
    u64			allow_mode,	// stored in 'allow_mode'
    const Socket_t	*listen		// NULL or related listen object
)
{
    DASSERT(th);

    TCPStream_t *ts = listen && listen->OnCreateStream
			? listen->OnCreateStream(th,sock) : 0;
    if (!ts)
    {
	if (th->OnCreateStream)
	    ts = th->OnCreateStream(th,sock);

	if (!ts)
	{
	    ts = CALLOC(sizeof(*ts)+th->data_size,1);
	    InitializeTCPStream(ts,sock);
	}
    }

    if ( th->tracelog.level >= 0 )
    {
	ts->tracelog = &th->tracelog;
	LogTCPStreamActivity(ts,"CreateTCPStream");
    }

    ts->allow_mode = allow_mode;
    AddTCPStream(th,ts);

    if ( listen && listen->OnAddedStream )
	listen->OnAddedStream(ts);
    else if (th->OnAddedStream)
	th->OnAddedStream(ts);

    return ts;
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * ConnectUnixTCPStream
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			path,		// NULL or path part 1 to socket file
    bool		silent		// suppress error messages
)
{
    DASSERT(th);
    DASSERT(path);

    int sock = socket(AF_UNIX,SOCK_STREAM,0);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CREATE,"Can't create UNIX stream socket: %s\n",path);
	return 0;
    }

    struct sockaddr_un sa;
    memset(&sa,0,sizeof(sa));
    sa.sun_family = AF_UNIX;
    StringCopyS(sa.sun_path,sizeof(sa.sun_path),path);

    int stat = connect(sock,(struct sockaddr*)&sa,sizeof(sa));
    if (stat)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't connect UNIX stream socket: %s\n",path);
	shutdown(sock,SHUT_RDWR);
	close(sock);
	return 0;
    }

    return CreateTCPStream(th,sock,ALLOW_MODE_ALLOW,0);
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * ConnectTCPStream
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			addr,		// address -> NetworkHost_t
    u16			default_port,	// default port
    bool		silent		// suppress error messages
)
{
    DASSERT(th);
    DASSERT(addr);

    ccp unix_path = CheckUnixSocketPath(addr,0);
    if (unix_path)
	return ConnectUnixTCPStream(th,unix_path,silent);

    if (!strncasecmp(addr,"tcp:",4))
	addr += 4;

    NetworkHost_t nh;
    ResolveHost(&nh,true,addr,default_port,false,false);
    PRINT("SINGLE/CONNECT/TCP: %s -> %s\n",
		addr, PrintIP4(0,0,nh.ip4,nh.port) );

    int sock = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CREATE,"Can't create TCP socket: %s\n",
			PrintIP4(0,0,nh.ip4,nh.port));
	return 0;
    }

    struct sockaddr_in sa;
    memset(&sa,0,sizeof(sa));
    sa.sin_family	= AF_INET;
    sa.sin_addr.s_addr	= htonl(nh.ip4);
    sa.sin_port		= htons(nh.port);

    int stat = connect(sock,(struct sockaddr*)&sa,sizeof(sa));
    if ( stat && errno != EINPROGRESS )
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't connect TCP socket: %s\n",
			PrintIP4(0,0,nh.ip4,nh.port));
	shutdown(sock,SHUT_RDWR);
	close(sock);
	ResetHost(&nh);
	return 0;
    }

    return CreateTCPStream(th,sock,ALLOW_MODE_ALLOW,0);
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * FindTCPStreamByUniqueID
(
    TCPHandler_t	*th,		// valid TCP handler
    uint		unique_id	// id to search
)
{
    TCPStream_t *ts;
    for ( ts = th->first; ts; ts = ts->next )
	if ( ts->unique_id == unique_id )
	    return ts;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint NumOfSocketsTCP ( const TCPHandler_t *th )
{
    uint i, count = 0;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	if ( th->listen[i].sock != -1 )
	    count++;

    const TCPStream_t *ts;
    for ( ts = th->first; ts; ts = ts->next )
	if ( ts->sock != -1
		&& ( !ts->eof && !ts->ibuf.disabled
			|| !ts->obuf.disabled && ts->obuf.used ))
	{
	    count++;
	}
    return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

Socket_t * GetUnusedListenSocketTCP ( TCPHandler_t *th, bool silent )
{
    DASSERT(th);

    uint i;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	if ( th->listen[i].sock == -1 )
	    return th->listen+i;

    if (!silent)
	ERROR0(ERR_CANT_CREATE,"No socket available.\n");
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

enumError ListenUnixTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			path		// path to unix file
)
{
    DASSERT(th);
    DASSERT(path);
    PRINT("LISTEN/UNIX: %s\n",path);

    Socket_t *lsock = GetUnusedListenSocketTCP(th,false);
    if (!lsock)
	return ERR_CANT_CREATE;
    unlink(path);

    int sock = socket(AF_UNIX,SOCK_STREAM,0);
    if ( sock == -1 )
	return ERROR1(ERR_CANT_CREATE,"Can't create UNIX socket: %s\n",path);

    struct sockaddr_un sa;
    memset(&sa,0,sizeof(sa));
    sa.sun_family = AF_UNIX;
    StringCopyS(sa.sun_path,sizeof(sa.sun_path),path);

    int stat = bind(sock,(struct sockaddr*)&sa,sizeof(sa));
    if ( stat == -1 )
    {
	close(sock);
	return ERROR1(ERR_CANT_CREATE,"Can't bind UNIX socket: %s\n",path);
    }

    stat = listen(sock,50);
    if ( stat == -1 )
    {
	close(sock);
	return ERROR1(ERR_CANT_CREATE,"Can't listen UNIX socket: %s\n",path);
    }
    //SetSocketBlocking(cmdi_sock,false);

    DASSERT(lsock);
    lsock->sock = sock;
    lsock->is_unix = true;
    snprintf(lsock->info,sizeof(lsock->info),"UNIX socket %d",sock);
    PRINT("LISTEN: %s [idx=%zu]\n",lsock->info,lsock-th->listen);
    LOG_TCP_HANDLER(th,0,"LISTEN/UNIX[%u] %s",listen_idx,path);

    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////

enumError ListenTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    ccp			addr,		// IPv4 address with optional port -> NetworkHost_t
					// fall back to ListenUnixTCP() if addr begins with
					// 'unix:' or '/' or './ or '../'
    u16			default_port	// default port
)
{
    DASSERT(th);
    DASSERT(addr);

    Socket_t *lsock = GetUnusedListenSocketTCP(th,false);
    if (!lsock)
	return ERR_CANT_CREATE;

    const bool allow_unix = memcmp(addr,"tcp:",4) != 0;
    if (!allow_unix)
    {
	addr += 4;
	while ( *addr == '/' )
	    addr++;
    }

    NetworkHost_t nh;
    if ( !ResolveHost(&nh,true,addr,default_port,allow_unix,true) && nh.filename )
    {
	ccp unix_path = nh.filename;
	ResetHost(&nh);
	return ListenUnixTCP(th,unix_path);
    }
    ccp path = GetHostName(&nh);
    PRINT("LISTEN/TCP: %s:%u -> %s\n",nh.name,default_port,PrintIP4(0,0,nh.ip4,nh.port));

    int sock = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);
    if ( sock == -1 )
    {
	ERROR1(ERR_CANT_CREATE,"Can't create TCP socket: %s\n",path);
	ResetHost(&nh);
	return ERR_CANT_CREATE;
    }

    int on = 1;
    setsockopt( sock, SOL_SOCKET, SO_REUSEADDR, (ccp)&on, sizeof(on) );

    struct sockaddr_in sa;
    memset(&sa,0,sizeof(sa));
    sa.sin_family	= AF_INET;
    sa.sin_addr.s_addr	= htonl(nh.ip4);
    sa.sin_port		= htons(nh.port);

    int stat = bind(sock,(struct sockaddr*)&sa,sizeof(sa));
    if ( stat == -1 )
    {
	ERROR1(ERR_CANT_CREATE,"Can't bind TCP socket: %s\n",
			PrintIP4(0,0,nh.ip4,nh.port));
	close(sock);
	ResetHost(&nh);
	return ERR_CANT_CREATE;
    }

    stat = listen(sock,50);
    if ( stat == -1 )
    {
	ERROR1(ERR_CANT_CREATE,"Can't listen TCP socket: %s\n",
			PrintIP4(0,0,nh.ip4,nh.port));
	close(sock);
	ResetHost(&nh);
	return ERR_CANT_CREATE;
    }
    //SetSocketBlocking(cmdi_sock,false);

    DASSERT(lsock);
    lsock->sock = sock;
    lsock->is_unix = false;
    snprintf(lsock->info,sizeof(lsock->info),"TCP socket %d",sock);
    PRINT("LISTEN: %s [idx=%zu]\n",lsock->info,lsock-th->listen);

    ResetHost(&nh);
    return ERR_OK;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

int UnlistenTCP
(
    // returns the index of the closed socket, or if not found

    TCPHandler_t	*th,		// valid TCP handler
    int			sock		// socket to close
)
{
    uint i;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	if ( th->listen[i].sock == sock )
	{
	    close(sock);
	    th->listen[i].sock = -1;
	    return i;
	}
    return -1;
}

///////////////////////////////////////////////////////////////////////////////

uint UnlistenAllTCP
(
    // returns the number of closed sockets

    TCPHandler_t	*th		// valid TCP handler
)
{
    uint i, count = 0;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	if ( th->listen[i].sock != -1 )
	{
	    socket_info_t si;
	    GetSocketInfoBySocket(&si,th->listen[i].sock,true);
	    if ( si.is_valid && si.protocol == AF_UNIX
			&& si.address && *si.address )
	    {
		PRINT("unlink(%s)\n",si.address);
		unlink(si.address);
	    }
	    ResetSocketInfo(&si);

	    close(th->listen[i].sock);
	    th->listen[i].sock = -1;
	    count++;
	}
    return count;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void AddSocketsTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    FDList_t		*fdl		// valid file descriptor list
)
{
    DASSERT(th);
    DASSERT(fdl);

    AnnounceFDList(fdl,TCP_HANDLER_MAX_LISTEN+th->used_streams+1);

    if ( !th->max_conn || th->used_streams < th->max_conn )
    {
	uint i;
	for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	{
	    Socket_t *lsock = th->listen + i;
	    lsock->poll_index = AddFDList(fdl,lsock->sock,POLLIN);
	}
    }

    TCPStream_t *ts;
    for ( ts = th->first; ts; ts = ts->next )
	AddSocketTCPStream(ts,fdl);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool IsStreamAllowed
(
    // returns true if access is allowed

    struct TCPHandler_t	*th,		// valid TCP handler
    const Socket_t	*lsock,		// valid listen-socket
    int			sock,		// valid socket to verify
    u64			*allow		// not NULL: store allow code here
)
{
    DASSERT(th);
    if ( !th->allow_ip4 || lsock && lsock->is_unix )
    {
	// no checks for unix sockets
	if (allow)
	    *allow = ALLOW_MODE_ALLOW;
	return true;
    }

    const u64 res = GetAutoAllowIP4BySock(th->allow_ip4,sock);
    if (allow)
	*allow = res;
    return ( res & th->allow_ip4->allow_mode ) != 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TCPStream_t * OnAcceptStream
(
    TCPHandler_t	*th,		// valid TCP handler
    const Socket_t	*lsock,		// listen-socket
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
)
{
    DASSERT(th);
    DASSERT(lsock);
    DASSERT( lsock >= th->listen );
    DASSERT( lsock < th->listen + TCP_HANDLER_MAX_LISTEN );
    DASSERT( lsock->sock != -1 );
    PRINT("OnAcceptStream() lsock=%d,%s\n",lsock->sock,lsock->info);

    TCPStream_t *ts = NULL;
    int new_sock = accept(lsock->sock,0,0);
    if ( new_sock != -1 )
    {
	RegisterFileId(new_sock);

	u64 allow_code = 0;
	const u64 is_allowed = th->OnAllowStream
			? th->OnAllowStream(th,lsock,new_sock,&allow_code)
			: ALLOW_MODE_ALLOW;
	if ( !is_allowed || th->OnAcceptStream && th->OnAcceptStream(th,lsock,new_sock) )
	{
	    PRINT("NOT ACCEPTED: %d -> %d\n",lsock->sock,new_sock);
	    shutdown(new_sock,SHUT_RDWR);
	    close(new_sock);
	}
	else
	{
	    PRINT("ACCEPTED: %d -> %d [alloe=%llx]\n",lsock->sock,new_sock,allow_code);
	    ts = CreateTCPStream(th,new_sock,allow_code,lsock);
	    if (ts)
	    {
		if (!ts->info[0])
		{
		    if (lsock->info[0])
			snprintf(ts->info,sizeof(ts->info),
					"by %s [%x]",
					lsock->info, ts->unique_id );
		    else
			snprintf(ts->info,sizeof(ts->info),
					"by socket %d [%x]",
					lsock->sock, ts->unique_id );
		}
	    }
	    else
	    {
		shutdown(new_sock,SHUT_RDWR);
		close(new_sock);
	    }
	}
    }
    return ts;
}

///////////////////////////////////////////////////////////////////////////////

void OnCloseStream
(
    TCPStream_t		*ts,		// valid TCP stream
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
)
{
    DASSERT(ts);
    if ( ts->sock != -1 )
    {
	PRINT("CLOSE: %d\n",ts->sock);
	if (ts->OnClose)
	    ts->OnClose(ts,now_usec);

	if ( ts->sock != -1 )
	{
	    // if not closed by OnClose()
	    shutdown(ts->sock,SHUT_RDWR);
	    close(ts->sock);
	    ts->sock = -1;
	}

	if ( !ts->protect && ts->handler )
	    ts->handler->need_maintenance |= 1;

	ts->OnReceived	= 0;
	ts->OnSend	= 0;
	ts->OnTimeout	= 0;
	ts->OnClose	= 0;
	ts->OnFDList	= 0;
    }
}

///////////////////////////////////////////////////////////////////////////////

void SetNotSocketStream
(
    TCPStream_t		*ts		// valid TCP stream
)
{
    DASSERT(ts);
    if ( ts->sock != -1 )
    {
	ts->not_socket = true;

	int flags = fcntl(ts->sock,F_GETFL,0);
	if ( flags != -1 )
	{
	    flags |= O_NONBLOCK;
	    fcntl(ts->sock,F_SETFL,flags);
	}
    }
}

///////////////////////////////////////////////////////////////////////////////

void OnReceivedStream
(
    TCPStream_t		*ts,		// valid TCP stream
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
)
{
    DASSERT(ts);

    uint max_read = GetSpaceGrowBuffer(&ts->ibuf);
    if (max_read)
    {
	u8 buf[0x4000];
	if ( max_read > sizeof(buf) )
	     max_read = sizeof(buf);

	ssize_t stat;
    retry:
	if (ts->not_socket)
	    stat = read(ts->sock,buf,max_read);
	else
	    stat = recv(ts->sock,buf,max_read,MSG_DONTWAIT);

	if ( stat < 0 && !ts->not_socket && errno == ENOTSOCK )
	{
	    SetNotSocketStream(ts);
	    goto retry;
	}

 #if HAVE_PRINT
	noPRINT("RECV[%d] %lld/%u, err=%d\n",ts->sock,(s64)stat,max_read,errno);
	if ( stat < 0 )
	    PRINT("!! %s(), ERRNO=%d: %s\n",__FUNCTION__,errno,strerror(errno));
 #endif

	if ( stat >= 0 )
	{
	    if ( stat > 0 )
		UpdateRecvStatTCPStream(ts,stat,now_usec);
	    else
		ts->eof |= 1;

	    if (ts->OnReceived)
		stat = ts->OnReceived(ts,buf,stat);
	    if ( stat > 0 )
		InsertGrowBuffer(&ts->ibuf,buf,stat);

//X	    if ( ts->eof && !ts->ibuf.used && !ts->obuf.used )
//X		OnCloseStream(ts,now_usec);
	}
	else if ( errno != EWOULDBLOCK )
	    OnCloseStream(ts,now_usec);
    }
}

///////////////////////////////////////////////////////////////////////////////

void OnWriteStream
(
    TCPStream_t		*ts,		// valid TCP stream
    u64			now_usec	// time for timestamps, GetTimeUSec(false)
)
{
    DASSERT(ts);

  restart:;
    ssize_t stat;
    if (ts->not_socket)
	stat = write(ts->sock,ts->obuf.ptr,ts->obuf.used);
    else
	stat = send(ts->sock,ts->obuf.ptr,ts->obuf.used,MSG_DONTWAIT);
    noPRINT("SEND[%d] %lld/%u, err=%d\n",ts->sock,(s64)stat,ts->obuf.used,errno);

    if ( stat < 0 )
    {
	if ( !ts->not_socket && errno == ENOTSOCK )
	{
	    SetNotSocketStream(ts);
	    goto restart;
	}

	PRINT("!! %s(), ERRNO=%d: %s\n",__FUNCTION__,errno,strerror(errno));
	OnCloseStream(ts,now_usec);
    }
    else if (!stat)
	ts->obuf.disabled |= 1;
    else
    {
	UpdateSendStatTCPStream(ts,stat,now_usec);

	if (ts->OnSend)
	    stat = ts->OnSend(ts,ts->obuf.ptr,stat);
	if ( stat > 0 )
	    DropGrowBuffer(&ts->obuf,stat);
	if ( ts->auto_close && !ts->obuf.used )
	    OnCloseStream(ts,now_usec);
    }
}

///////////////////////////////////////////////////////////////////////////////

void CheckSocketsTCP
(
    TCPHandler_t	*th,		// valid TCP handler
    FDList_t		*fdl,		// valid socket list
    bool		check_timeout	// true: enable timeout checks
)
{
    DASSERT(th);
    DASSERT(fdl);

    const u64 now_usec = GetTimeUSec(false);

    uint i;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
    {
	Socket_t *lsock = th->listen + i;
	if ( lsock->sock != -1 )
	{
	    const uint revents = GetEventFDList(fdl,lsock->sock,lsock->poll_index);
	    if ( revents & POLLIN )
		OnAcceptStream(th,lsock,now_usec);
	}
    }

    TCPStream_t *next = th->first;
    while (next)
    {
	// Do it in this way, because 'ts' may become invalid
	TCPStream_t *ts = next;
	next = ts->next;
	CheckSocketTCPStream(ts,fdl,check_timeout);
    }
}

///////////////////////////////////////////////////////////////////////////////

void CheckTimeoutTCP ( TCPHandler_t *th )
{
    DASSERT(th);

    TCPStream_t *next = th->first;
    while (next)
    {
	// Do it in this way, because 'ts' may becomes invalid
	TCPStream_t *ts = next;
	next = ts->next;
	CheckTimeoutTCPStream(ts);
    }
}

///////////////////////////////////////////////////////////////////////////////

bool MaintainTCP ( TCPHandler_t *th )
{
    DASSERT(th);

    th->need_maintenance = 0;
    const u64 now_usec = GetTimeUSec(false);
    TCPStream_t *next = th->first;
    while (next)
    {
	// Do it in this way, because 'ts' may becomes invalid
	TCPStream_t *ts = next;
	next = ts->next;

	if ( ts->sock == -1 && ts->protect <= 0 )
	    DestroyTCPStream(ts);
	else if (ts->OnMaintenance)
	    ts->OnMaintenance(ts,now_usec);
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////

void ManageSocketsTCP
(
    // call  CheckSocketsTCP(), CheckTimeoutTCP(), MaintainTCP()

    TCPHandler_t	*th,		// valid TCP handler
    FDList_t		*fdl,		// valid socket list
    int			stat		// result of a WAIT function
)
{
    if ( stat > 0 )
	CheckSocketsTCP(th,fdl,true);
    else
	CheckTimeoutTCP(th);

    if (th->need_maintenance)
	MaintainTCP(th);
}

///////////////////////////////////////////////////////////////////////////////
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
{
    DASSERT(f);
    DASSERT(th);
    indent = NormalizeIndent(indent);

    fprintf(f,"%*sTH: id:%x, n=%u/%u/%u, listen=",
	indent,"", th->unique_id,
	th->used_streams, th->max_used_streams, th->total_streams );

    uint i;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
	fprintf(f,"%d,",th->listen[i].sock);

    fprintf(f," max=%u, dsize=%u, ready=%u",
	th->max_conn, th->data_size, NumOfSocketsTCP(th) );

    if (format)
    {
	fputs(" : ",f);
	va_list arg;
	va_start(arg,format);
	vfprintf(f,format,arg);
	va_end(arg);
    }

    fprintf(f,"\n%*s    %u connects, %u packets received (%s), %u packets send (%s)\n",
	indent,"",
	th->stat.conn_count,
	th->stat.recv_count, PrintSize1024(0,0,th->stat.recv_size,0),
	th->stat.send_count, PrintSize1024(0,0,th->stat.send_size,0) );

    if (recurse>0)
    {
	recurse--;
	uint idx = 0;
	const TCPStream_t *ts;
	for ( ts = th->first; ts; ts = ts->next )
	    LogTCPStream(f,indent+2,ts,recurse,"Stream #%u",idx++);
    }
}

///////////////////////////////////////////////////////////////////////////////

void PrintStreamTableTCPHandler
(
    FILE		*f,		// output file
    const ColorSet_t	*colset,	// NULL (no colors) or valid color set
    int			indent,		// indention
    const TCPHandler_t	*th		// valid TCP handler
)
{
    DASSERT(f);
    DASSERT(th);
    indent = NormalizeIndent(indent);
    if (!colset)
	colset = GetColorSet0();

    const u64 now_usec = GetTimeUSec(false);

    const TCPStream_t *ts;
    ccp line1 = BufInfoHeadTCPStream(0);
    ccp line2 = BufInfoHeadTCPStream(1);
    const uint sep_len = strlen(line2)+sizeof(ts->info)-13;

    fprintf(f,"%s%*s%.*s\n%s%*s%s\n%s%*s%s\n%s%*s%.*s%s\n",
	colset->heading, indent,"", sep_len, Minus300,
	colset->heading, indent,"", line1,
	colset->heading, indent,"", line2,
	colset->heading, indent,"", sep_len, Minus300,
	colset->reset );

    char buf[BUF_INFO_TCP_STREAM_SIZE];
    for ( ts = th->first; ts; ts = ts->next )
    {
	BufInfoTCPStream(buf,BUF_INFO_TCP_STREAM_SIZE,ts,now_usec);
	fprintf(f,"%*s%s\n",indent,"",buf);
    }


    if (th->used_streams)
	fprintf(f,
	    "%s%*s%.*s%s\n",
	    colset->heading, indent,"", sep_len, Minus300,
	    colset->reset );

    if ( th->total_streams > 1 || th->total_streams > th->used_streams )
    {
	snprintf(buf,sizeof(buf),"TOTAL: %u current client%s, %u max",
		th->used_streams, th->used_streams == 1 ? "" : "s",
		th->max_used_streams );

	fprintf(f,
	    "%s%*s%-45s%s %s%13s %s  Summary of %u stream%s\n%s%*s%.*s%s\n",
	    colset->status, indent,"", buf,
	    PrintNumberU7(0,0,th->stat.recv_count,true),
	    PrintSize1024(0,0,th->stat.recv_size,DC_SFORM_TINY|DC_SFORM_ALIGN),
	    PrintNumberU7(0,0,th->stat.send_count,true),
	    PrintSize1024(0,0,th->stat.send_size,DC_SFORM_TINY|DC_SFORM_ALIGN),
	    th->total_streams, th->total_streams == 1 ? "" : "s",
	    colset->heading, indent,"", sep_len,Minus300,
	    colset->reset );
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool GetSocketInfoBySocket
(
    // returns TRUE, if infos retrived

    socket_info_t	*si,		// result, not NULL, will be initialized
    int			sock,		// socket id, maybe -1
    bool		get_name	// true: alloc 'address' and copy name
					// -> call ResetSocketInfo() to free mem
)
{
    DASSERT(si);
    InitializeSocketInfo(si);
    if ( sock == -1 )
	return false;
    si->sock = sock;

    DASSERT( sizeof(struct sockaddr_un) >= sizeof(struct sockaddr_in) );
    DASSERT( sizeof(struct sockaddr_un) >= sizeof(struct sockaddr_in6) );

    struct sockaddr_un sa;
    memset(&sa,0,sizeof(sa));
    socklen_t sa_len = sizeof(sa);
    if (getsockname(sock,(struct sockaddr*)&sa,&sa_len))
	return false;

    char buf[INET6_ADDRSTRLEN+10];
    switch (sa.sun_family)
    {
	case AF_UNIX:
	    si->protocol = AF_UNIX;
	    if ( get_name && sa.sun_path[0] )
	    {
		si->address = STRDUP(sa.sun_path);
		si->alloced = true;
	    }
	    break;

	case AF_INET:
	    si->protocol = AF_INET;
	    if (get_name)
	    {
		const struct sockaddr_in *sin = (struct sockaddr_in*)&sa;
		PrintIP4sa(buf,sizeof(buf),sin);
		si->address = STRDUP(buf);
		si->alloced = true;
	    }
	    break;

	case AF_INET6:
	    si->protocol = AF_INET6;
	    if (get_name)
	    {
		const struct sockaddr_in6 *sin6 = (struct sockaddr_in6*)&sa;
		if (inet_ntop(AF_INET6,&sin6->sin6_addr,buf+1,sizeof(buf)-1))
		{
		    *buf = '[';
		    char *dest = buf + strlen(buf);
		    snprintf(dest,buf+sizeof(buf)-dest,"]:%u",ntohs(sin6->sin6_port));
		    si->address = STRDUP(buf);
		    si->alloced = true;
		}
	    }
	    break;

	default:
	    return false;
    }

    int socktype = -1;
    socklen_t psize = sizeof(socktype);
    if (!getsockopt(sock,SOL_SOCKET,SO_TYPE,&socktype,&psize))
	si->type = socktype;

    return si->is_valid = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

uint GetSocketNameBySA
(
    char	*buf,		// dest buffer
    uint	bufsize,	// size of 'dest'
    const struct sockaddr *sa,	// socket address
    socklen_t	sa_len		// length of 'sa'
)
{
    DASSERT(buf);
    DASSERT(bufsize>0);
    DASSERT(sa);
    DASSERT(sa_len>0);

    if ( sa_len >= sizeof(struct sockaddr_un) )
    {
	const struct sockaddr_un *sun = (struct sockaddr_un*)sa;
	if ( sun->sun_family == AF_UNIX && sun->sun_path[0] )
	    return snprintfS(buf,bufsize,"unix:%s",sun->sun_path);
    }

    if ( sa_len >= sizeof(struct sockaddr_in) )
    {
	const struct sockaddr_in *sin = (struct sockaddr_in*)sa;
	if ( sin->sin_family == AF_INET && bufsize > 5 )
	{
	    strcpy(buf,"tcp:");
	    return PrintIP4sa(buf+4,bufsize-4,sin) - buf;
	}
    }

    if ( sa_len >= sizeof(struct sockaddr_in6) )
    {
	const struct sockaddr_in6 *sin = (struct sockaddr_in6*)sa;
	if ( sin->sin6_family == AF_INET6 && bufsize >= 5 + INET6_ADDRSTRLEN )
	{
	    // [[2do]] ??? "[addr]:port"
	    strcpy(buf,"tcp6:");
	    if (inet_ntop(AF_INET6,&sin->sin6_addr,buf+5,bufsize-5))
		return strlen(buf);
	}
    }

    *buf = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint GetSocketName ( char *buf, uint bufsize, int sock )
{
    DASSERT(buf);
    DASSERT(bufsize>0);

    if ( sock != -1 )
    {
	DASSERT( sizeof(struct sockaddr_un) >= sizeof(struct sockaddr_in) );
	DASSERT( sizeof(struct sockaddr_un) >= sizeof(struct sockaddr_in6) );
	struct sockaddr_un sun;
	memset(&sun,0,sizeof(sun));
	socklen_t sun_len = sizeof(sun);
	if (!getsockname(sock,(struct sockaddr*)&sun,&sun_len))
	    return GetSocketNameBySA(buf,bufsize,(struct sockaddr*)&sun,sun_len);
    }
    *buf = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

uint GetPeerName ( char *buf, uint bufsize, int sock )
{
    DASSERT(buf);
    DASSERT(bufsize>0);

    if ( sock != -1 )
    {
	DASSERT( sizeof(struct sockaddr_un) >= sizeof(struct sockaddr_in) );
	struct sockaddr_un sun;
	memset(&sun,0,sizeof(sun));
	socklen_t sun_len = sizeof(sun);
	if (!getpeername(sock,(struct sockaddr*)&sun,&sun_len))
	    return GetSocketNameBySA(buf,bufsize,(struct sockaddr*)&sun,sun_len);
    }
    *buf = 0;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool DeleteSocketFile ( int sock )
{
    if ( sock != -1 )
    {
	DASSERT( sizeof(struct sockaddr_un) >= sizeof(struct sockaddr_in) );
	struct sockaddr_un sun;
	memset(&sun,0,sizeof(sun));
	socklen_t sun_len = sizeof(sun);
	if (!getsockname(sock,(struct sockaddr*)&sun,&sun_len))
	{
	    if ( sun.sun_family == AF_UNIX )
	    {
		unlink(sun.sun_path);
		return true;
	    }
	}
    }
    return false;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    TCP: send single strings		///////////////
///////////////////////////////////////////////////////////////////////////////

static int OnTimeoutSingleTCP ( struct TCPStream_t * ts, u64 now_usec )
{
    PRINT("OnTimeoutSingleTCP(%p,%llu) sock=%d, to=%lld,%lld d=%lld,%lld\n",
		ts, now_usec, ts->sock,
		ts->trigger_usec, ts->trigger_usec - now_usec,
		ts->accept_usec, ts->accept_usec - now_usec );
    DASSERT(ts);
    OnCloseStream(ts,now_usec);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int OnReceivedSingleTCP ( struct TCPStream_t * ts, u8 *buf, uint size )
{
    PRINT("OnReceivedSingleTCP(%p,%p,%u) to=%lld\n",ts,buf,size,*(u64*)ts->data);
    DASSERT(ts);
    ts->accept_usec = GetTimeUSec(false) + *(u64*)ts->data;
    ts->trigger_usec = ts->accept_usec + 100000;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int OnSendSingleTCP ( struct TCPStream_t * ts, u8 *buf, uint size )
{
    PRINT("OnSendSingleTCP(%p,%p,%u) to=%lld\n",ts,buf,size,*(u64*)ts->data);
    DASSERT(ts);
    ts->accept_usec = GetTimeUSec(false) + *(u64*)ts->data;
    ts->trigger_usec = ts->accept_usec + 100000;
    return size;
}

///////////////////////////////////////////////////////////////////////////////

static TCPStream_t * SendSingleTCPHelper
(
    TCPHandler_t	*th,		// valid handle
    int			sock,		// valid socket

    const void		*data,		// data to send
    uint		size,		// size of 'data'
    u64			timeout_usec,	// timeout before closing the connection
    TransferStats_t	*xstat,		// NULL or pointer to summary statistics

    bool		silent		// suppress error messages
)
{
    DASSERT(th);
    DASSERT(sock!=-1);
    DASSERT(data||!size);

    PRINT("SendSingleStringTCPHelper(%p,%d,%lld,%d)\n",th,sock,timeout_usec,silent);

    th->stat.conn_count++;
    TCPStream_t *ts = CALLOC(sizeof(*ts)+sizeof(u64),1);
    InitializeTCPStream(ts,sock);
    AddTCPStream(th,ts);

    ts->xstat		= xstat;
    ts->OnTimeout	= OnTimeoutSingleTCP;
    ts->OnReceived	= OnReceivedSingleTCP;
    ts->OnSend		= OnSendSingleTCP;
    ts->accept_usec	= GetTimeUSec(false) + timeout_usec;
    ts->trigger_usec	= ts->accept_usec + 100000;
    *(u64*)ts->data	= timeout_usec;
    snprintf(ts->info,sizeof(ts->info),"send single, %u bytes",size);

 #if 1
    ts->obuf.max_size	= size > 10240 ? size : 10240;
    SendDirectTCPStream(ts,false,data,size);
 #else
    ts->obuf.max_size	= size;
    InsertGrowBuffer(&ts->obuf,data,size);
 #endif
    return ts;
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * SendSingleUnixTCP
(
    TCPHandler_t	*th,		// valid handle
    ccp			path,		// path to socket file

    const void		*data,		// data to send
    uint		size,		// size of 'data'
    u64			timeout_usec,	// timeout before closing the connection
    TransferStats_t	*xstat,		// NULL or pointer to summary statistics

    bool		silent		// suppress error messages
)
{
    DASSERT(th);
    DASSERT(path);
    DASSERT(data||!size);

    PRINT("SINGLE/CONNECT/UNIX: %s\n",path);

    int sock = socket(AF_UNIX,SOCK_STREAM,0);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CREATE,"Can't create UNIX socket: %s\n",path);
	return 0;
    }

    struct sockaddr_un sa;
    memset(&sa,0,sizeof(sa));
    sa.sun_family = AF_UNIX;
    StringCopyS(sa.sun_path,sizeof(sa.sun_path),path);

    int stat = connect(sock,(struct sockaddr*)&sa,sizeof(sa));
    if (stat)
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't connect UNIX socket: %s\n",path);
	shutdown(sock,SHUT_RDWR);
	close(sock);
	return 0;
    }

    return SendSingleTCPHelper(th,sock,data,size,timeout_usec,xstat,silent);
}

///////////////////////////////////////////////////////////////////////////////

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
)
{
    DASSERT(th);
    DASSERT(addr);
    DASSERT(data||!size);

    ccp unix_path = CheckUnixSocketPath(addr,0);
    if (unix_path)
	return SendSingleUnixTCP(th,unix_path,data,size,timeout_usec,xstat,silent);

    if (!strncasecmp(addr,"tcp:",4))
	addr += 4;

    NetworkHost_t nh;
    ResolveHost(&nh,true,addr,default_port,false,false);
    PRINT("SINGLE/CONNECT/TCP: %s -> %s\n",
		addr, PrintIP4(0,0,nh.ip4,nh.port) );

    int sock = socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK,0);
    if ( sock == -1 )
    {
	if (!silent)
	    ERROR1(ERR_CANT_CREATE,"Can't create TCP socket: %s\n",
			PrintIP4(0,0,nh.ip4,nh.port));
	return 0;
    }

    struct sockaddr_in sa;
    memset(&sa,0,sizeof(sa));
    sa.sin_family	= AF_INET;
    sa.sin_addr.s_addr	= htonl(nh.ip4);
    sa.sin_port		= htons(nh.port);

    int stat = connect(sock,(struct sockaddr*)&sa,sizeof(sa));
    if ( stat && errno != EINPROGRESS )
    {
	if (!silent)
	    ERROR1(ERR_CANT_OPEN,"Can't connect TCP socket: %s\n",
			PrintIP4(0,0,nh.ip4,nh.port));
	shutdown(sock,SHUT_RDWR);
	close(sock);
	ResetHost(&nh);
	return 0;
    }

    TCPStream_t *ts = SendSingleTCPHelper(th,sock,data,size,timeout_usec,xstat,silent);
    ResetHost(&nh);
    return ts;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			TCP: CommandTCP			///////////////
///////////////////////////////////////////////////////////////////////////////

void SetTimeoutCommandTCP
(
    TCPStream_t	*ts,		// valid stream
    u64		now_usec	// NULL or current time in usec
)
{
    DASSERT(ts);

    const CommandTCPInfo_t *ci = (CommandTCPInfo_t*)ts->data;
    if (!ci->timeout_usec)
	ts->accept_usec = ts->trigger_usec = 0;
    else
    {
	if (!now_usec)
	    now_usec = GetTimeUSec(false);
	ts->accept_usec = now_usec + ci->timeout_usec;
	ts->trigger_usec = ts->accept_usec + 100000;
    }
}

///////////////////////////////////////////////////////////////////////////////

static char * ScanLineCommandTCP
(
    // return first non scanned character, or NULL on error

    struct TCPStream_t *ts,
    char	*line,
    char	*line_end,
    u64		now_usec
)
{
    DASSERT(ts);
    DASSERT(line);
    DASSERT(line_end);
    DASSERT( line <= line_end );

    *line_end = 0;
    const CommandTCPInfo_t *ci = (CommandTCPInfo_t*)ts->data;

    if (ci->OnScanArg)
    {
	SplitArg_t sa;
	ScanSplitArg(&sa,true,line,line_end,line);
	sa.argv[0] = ts->info;
	const int stat = ci->OnScanArg(ts,sa.argc,sa.argv,now_usec);
	ResetSplitArg(&sa);
	return stat > ERR_WARNING ? line : line_end;
    }

    if (ci->OnScanLine)
	return ci->OnScanLine(ts,line,line_end,now_usec);

    //--- fall back: mirror
    InsertGrowBuffer(&ts->obuf,line,line_end-line);
    InsertCharGrowBuffer(&ts->obuf,'\n');
    return line_end;
}

///////////////////////////////////////////////////////////////////////////////

static void ScanBufferCommandTCP ( struct TCPStream_t * ts, bool finish )
{
    DASSERT(ts);

    //--- setup

    const u64 now_usec = GetTimeUSec(false);

    const CommandTCPInfo_t *ci = (CommandTCPInfo_t*)ts->data;
    const u8 comma = ci->comma_is_eol ? ',' : ';';


    //--- skip uneeded data

    u8 *beg = ts->ibuf.ptr, *end = beg + ts->ibuf.used;
    while ( beg < end )
    {
	const u8 ch = *beg;
	if ( ch > ' ' && ch != ';' && ch != comma )
	    break;
	beg++;
    }
    DropGrowBuffer(&ts->ibuf,beg-ts->ibuf.ptr);


    //--- find end of command

    u8 *ptr;
    for ( ptr = beg; ptr < end; ptr++ )
    {
	const u8 ch = *ptr;
	if ( !ch || ch == '\n' || ch == '\r' || ch == ';' || ch == comma )
	{
	    DropGrowBuffer(&ts->ibuf,ptr+1-beg);
	    ScanLineCommandTCP(ts,(char*)beg,(char*)ptr,now_usec);
	    return;
	}
    }

    if ( ptr > beg && ( finish || ts->eof ) )
    {
	DropGrowBuffer(&ts->ibuf,ptr-beg);
	ScanLineCommandTCP(ts,(char*)beg,(char*)ptr,now_usec);
    }
}

///////////////////////////////////////////////////////////////////////////////

static int OnCommandTCP ( struct TCPStream_t * ts )
{
    DASSERT(ts);
    SetTimeoutCommandTCP(ts,0);

    while ( ts->sock != -1 && !ts->obuf.used )
    {
	const uint watermark = ts->ibuf.used;
	ScanBufferCommandTCP(ts,false);
	if ( watermark == ts->ibuf.used )
	    break;
    }

    if ( ts->eof && !ts->obuf.used && !ts->ibuf.used )
	OnCloseStream(ts,0);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////

static int OnReceivedCommandTCP ( struct TCPStream_t * ts, u8 *buf, uint size )
{
    DASSERT(ts);
    noPRINT("---------- OnReceivedCommandTCP(%p,%p,%u), used=%d,%d\n",
		ts, buf, size, ts->ibuf.used, ts->obuf.used );
    InsertGrowBuffer(&ts->ibuf,buf,size);
    return OnCommandTCP(ts);
}

///////////////////////////////////////////////////////////////////////////////

static int OnSendCommandTCP ( struct TCPStream_t * ts, u8 *buf, uint size )
{
    DASSERT(ts);
    noPRINT("---------- OnSendCommandTCP(%p,%p,%u), used=%d,%d\n",
		ts, buf, size, ts->ibuf.used, ts->obuf.used );

    DropGrowBuffer(&ts->obuf,size);
    return OnCommandTCP(ts);
}

///////////////////////////////////////////////////////////////////////////////

static int OnTimeoutCommandTCP ( struct TCPStream_t * ts, u64 now_usec )
{
    DASSERT(ts);
    noPRINT("---------- OnTimeoutCommandTCP(%p,%llu)\n",ts,now_usec);

    if ( ts->sock != -1 )
    {
	if (!ts->obuf.used)
	{
	    ScanBufferCommandTCP(ts,true);
	    if (ts->obuf.used)
	    {
		SetTimeoutCommandTCP(ts,now_usec);
		return 0;
	    }
	}

	OnCloseStream(ts,now_usec);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////

int OnCreateCommandTCP ( TCPStream_t *ts )
{
    noPRINT("---------- OnCreateCommandTCP(%p)\n",ts);
    DASSERT(ts);
    ts->OnReceived	= OnReceivedCommandTCP;
    ts->OnSend		= OnSendCommandTCP;
    ts->OnTimeout	= OnTimeoutCommandTCP;

    CommandTCPInfo_t *ci = (CommandTCPInfo_t*)ts->data;
    ci->OnScanLine	= 0;
    ci->timeout_usec	= 10*USEC_PER_SEC;
    ci->comma_is_eol	= false;

    SetTimeoutCommandTCP(ts,0);
    return 0;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SaveCurrentState*()		///////////////
///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateSocket
(
    FILE		*f,		// output file
    const Socket_t	*sock		// valid socket object
)
{
    DASSERT(f);
    DASSERT(sock);

    char info[3*sizeof(sock->info)];
    PrintEscapedString(info,sizeof(info),sock->info,-1,CHMD__ALL,0,0);

    fprintf(f,
       "sock		= %d\n"
       "is-unix		= %d\n"
       "info		= %s\n"
	,sock->sock
	,sock->is_unix
	,info
	);
}

///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateTCPStream
(
    FILE		*f,		// output file
    const TCPStream_t	*ts,		// valid TCP stream
    TCPStreamSaveFunc	func		// NULL or function for 'TCPStream_t.data' extend
)
{
    DASSERT(f);
    DASSERT(ts);

    char info[1000];
    PrintEscapedString(info,sizeof(info),ts->info,-1,CHMD__ALL,0,0);

    fprintf(f,
       "sock		= %d\n"
       "rescan		= %u\n"
       "eof		= %u\n"
       "protect		= %u\n"
       "auto-close	= %u\n"
       "accept-usec	= %llu\n"
       "trigger-usec	= %llu\n"
       "delay-usec	= %llu\n"
       "timeout-usec	= %llu\n"
       "connect-usec	= %llu\n"
       "receive-usec	= %llu\n"
       "send-usec	= %llu\n"
       "info		= %s\n"
	,ts->sock
	,ts->rescan
	,ts->eof
	,ts->protect
	,ts->auto_close
	,ts->accept_usec
	,ts->trigger_usec
	,ts->delay_usec
	,ts->timeout_usec
	,ts->connect_usec
	,ts->receive_usec
	,ts->send_usec
	,info
	);

    if (func)
	func(f,ts);

    SaveCurrentStateTransferStats1(f,"tfer-stat",&ts->stat);
    SaveCurrentStateGrowBuffer(f,"ibuf-",2,&ts->ibuf);
    SaveCurrentStateGrowBuffer(f,"obuf-",2,&ts->obuf);
}

///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateCommandTCP
(
    FILE		*f,		// output file
    const TCPStream_t	*ts		// valid TCP stream
)
{
    DASSERT(f);
    DASSERT(ts);

    const CommandTCPInfo_t *ci = (CommandTCPInfo_t*)ts->data;

    fprintf(f,
       "cmd-comma	= %d\n"
       "cmd-timeout	= %llu\n"
	,ci->comma_is_eol
	,ci->timeout_usec
	);
}

///////////////////////////////////////////////////////////////////////////////

void SaveCurrentStateTCPHandler
(
    FILE		*f,		// output file
    ccp			sect_name,	// section base name
    const TCPHandler_t	*th,		// valid TCP handler
    TCPStreamSaveFunc	func		// NULL or function for 'TCPStream_t.data' extend
)
{
    DASSERT(f);
    DASSERT(sect_name);
    DASSERT(th);

    fprintf(f,
       "\n#%.50s\n"
       "[%s]\n"
       "\n"
       "data-size	= %u\n"
       "max-conn	= %u\n"
       "maintenance	= %u\n"
       "used-streams	= %u\n"
       "max-used	= %u\n"
       "total-streams	= %u\n"
	,Minus300
	,sect_name
	,th->data_size
	,th->max_conn
	,th->need_maintenance
	,th->used_streams
	,th->max_used_streams
	,th->total_streams
	);

    SaveCurrentStateTransferStats1(f,"tfer-stat",&th->stat);

    uint i;
    for ( i = 0; i < TCP_HANDLER_MAX_LISTEN; i++ )
    {
	if ( th->listen[i].sock != -1 )
	{
	    fprintf(f,"\n[%s/socket:%u]\n\n",sect_name,i);
	    SaveCurrentStateSocket(f,th->listen+i);
	}
    }

    const TCPStream_t *ts;
    for ( ts = th->first, i = 0; ts; ts = ts->next, i++ )
    {
	fprintf(f,"\n[%s/stream:%u]\n\n",sect_name,i);
	SaveCurrentStateTCPStream(f,ts,func);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			RestoreState*()			///////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateSocket
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
)
{
    DASSERT(rs);

    Socket_t *sock = (Socket_t*)user_table;
    if (!sock)
	return;

    int fd = GetParamFieldINT(rs,"sock",-1);
    if ( fd != -1 && fcntl(fd,F_GETFD) != -1 )
    {
	sock->sock = fd;
	sock->is_unix = GetParamFieldINT(rs,"is-unix",0);
	GetParamFieldBUF(sock->info,sizeof(sock->info),rs,"info",ENCODE_STRING,"?");
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

TCPStream_t * RestoreStateTCPStream
(
    // return 'ts' or the new TCPStream_t
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    TCPStream_t		*ts,		// if NULL: create it
    uint		extra_size	// if 'ts' alloced: alloc some byte more
)
{
    DASSERT(rs);
    TRACE("RestoreStateTCPStream()\n");

    int fd = ts ? ts->sock : -1;
    if ( fd == -1 )
    {
	fd = GetParamFieldINT(rs,"sock",-1);
	if ( fd != -1 && fcntl(fd,F_GETFD) == -1 )
	    fd = -1;
    }

    if (!ts)
    {
	ts = CALLOC(sizeof(*ts)+extra_size,1);
	InitializeTCPStream(ts,fd);
    }
    else
	ts->sock = fd;

    DASSERT(ts);
    ts->protect		= GetParamFieldUINT(rs, "protect",	ts->protect);
    ts->auto_close	= GetParamFieldUINT(rs, "auto-close",	ts->auto_close);
    ts->rescan		= GetParamFieldUINT(rs, "rescan",	ts->rescan );
    ts->eof		= GetParamFieldUINT(rs, "eof",		ts->eof );
    ts->accept_usec	= GetParamFieldU64 (rs, "accept-usec",	ts->accept_usec );
    ts->trigger_usec	= GetParamFieldU64 (rs, "trigger-usec",	ts->trigger_usec );
    ts->delay_usec	= GetParamFieldU64 (rs, "delay-usec",	ts->delay_usec );
    ts->timeout_usec	= GetParamFieldU64 (rs, "timeout-usec",	ts->timeout_usec );
    ts->connect_usec	= GetParamFieldU64 (rs, "connect-usec",	ts->connect_usec );
    ts->receive_usec	= GetParamFieldU64 (rs, "receive-usec",	ts->receive_usec );
    ts->send_usec	= GetParamFieldU64 (rs, "send-usec",	ts->send_usec );

    GetParamFieldBUF(ts->info,sizeof(ts->info),rs,"info",ENCODE_STRING,"?");

    RestoreStateTransferStats1(rs,"tfer-stat",&ts->stat,true);
    RestoreStateGrowBuffer(&ts->ibuf,"ibuf-",rs);
    RestoreStateGrowBuffer(&ts->obuf,"obuf-",rs);
    return ts;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateTCPHandler_base
(
    TCPHandler_t	*th,	// valid TCPHandler_t
    RestoreState_t	*rs	// info data, can be modified (cleaned after call)
)
{
    DASSERT(th);
    DASSERT(rs);
    TRACE("RestoreStateTCPHandler_base()\n");

    //ResetTCPHandler(th); // don't call, because call back functions are cleared

    struct uint_parm_t
    {
	ccp name;
	uint *param;
    };

    const struct uint_parm_t uint_parm_tab[] =
    {
	{ "max-conn",		&th->max_conn },
	{ "maintenance",	&th->need_maintenance },
	{ "max-used",		&th->max_used_streams },
	{ "total-streams",	&th->total_streams },
	{0,0}
    };

    const struct uint_parm_t *up;
    for ( up = uint_parm_tab; up->name; up++ )
	*up->param = GetParamFieldUINT(rs,up->name,*up->param);

    RestoreStateTransferStats1(rs,"tfer-stat",&th->stat,true);

    if ( rs->log_mode & RSL_UNUSED_NAMES )
    {
	//  known as 'unused'
	GetParamField(rs,"data-size");
	GetParamField(rs,"used-streams");
    }
}

///////////////////////////////////////////////////////////////////////////////

Socket_t * RestoreStateTCPHandler_socket
(
    TCPHandler_t	*th,		// valid TCPHandler_t
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
)
{
    DASSERT(th);
    DASSERT(rs);
    TRACE("RestoreStateTCPHandler_socket()\n");

    Socket_t *sock = GetUnusedListenSocketTCP(th,false);
    if (sock)
	RestoreStateSocket(rs,sock);
    return sock;
}

///////////////////////////////////////////////////////////////////////////////

TCPStream_t * RestoreStateTCPHandler_stream
(
    TCPHandler_t	*th,		// valid TCPHandler_t
    RestoreState_t	*rs		// info data, can be modified (cleaned after call)
)
{
    DASSERT(th);
    DASSERT(rs);
    TRACE("RestoreStateTCPHandler_stream()\n");

    int fd = GetParamFieldINT(rs,"sock",-1);
    bool protect = GetParamFieldUINT(rs,"protect",0);
 #ifdef TEST
    if ( !protect && fd == -1 )
	return 0;
 #else
    if ( !protect && ( fd == -1 || fcntl(fd,F_GETFD) == -1 ))
	return 0;
 #endif

    TCPStream_t *ts = CreateTCPStream(th,fd,ALLOW_MODE_ALLOW,0);
    RestoreStateTCPStream(rs,ts,th->data_size);
    return ts;
}

///////////////////////////////////////////////////////////////////////////////

void RestoreStateTCPHandler
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
)
{
    DASSERT(rs);
    TRACE("Scan TCP Handler (%s/%s,%d)\n",rs->sect,rs->path,rs->index);

    TCPHandler_t *th = (TCPHandler_t*)user_table;
    if (!th)
	return;

    if (!*rs->path)
	RestoreStateTCPHandler_base(th,rs);
    else if (!strcmp(rs->path,"socket"))
	RestoreStateTCPHandler_socket(th,rs);
    else if (!strcmp(rs->path,"stream"))
	RestoreStateTCPHandler_stream(th,rs);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

void RestoreStateCommandTCP
(
    RestoreState_t	*rs,		// info data, can be modified (cleaned after call)
    cvp			user_table	// pointer provided by RestoreStateTab_t[]
)
{
    DASSERT(rs);

    TCPHandler_t *th = (TCPHandler_t*)user_table;
    if (!th)
	return;

    if (!strcmp(rs->path,"stream"))
    {
	TCPStream_t *ts = RestoreStateTCPHandler_stream(th,rs);
	if (ts)
	{
	    CommandTCPInfo_t *ci = (CommandTCPInfo_t*)ts->data;
	    ci->comma_is_eol = GetParamFieldINT( rs, "cmd-comma",   ci->comma_is_eol ) > 0;
	    ci->timeout_usec = GetParamFieldU64( rs, "cmd-timeout", ci->timeout_usec );
	}
    }
    else
	RestoreStateTCPHandler(rs,user_table);
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    Misc			///////////////
///////////////////////////////////////////////////////////////////////////////

bool fix_ether_head_vlan ( ether_head_vlan_t *head )
{
    DASSERT(head);
    const u16 vlan_tag_type = ntohs(head->vlan_tag_type);
    if ( vlan_tag_type == 0x8100 )
    {
	head->have_vlan	= true;
	head->head_size	= 18;
	return true;
    }

    head->ether_type	= head->vlan_tag_type;
    head->have_vlan	= false;
    head->head_size	= 14;
    return false;
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_ether_head_vlan ( ether_head_vlan_t *dest, const void * src )
{
    if (dest)
    {
	if (src)
	    memcpy(dest,src,sizeof(*dest));

	fix_ether_head_vlan(dest);
	dest->vlan_tag_type	= ntohs(dest->vlan_tag_type);
	dest->vlan_param	= ntohs(dest->vlan_param);
	dest->ether_type	= ntohs(dest->ether_type);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_ether_head ( ether_head_t *dest, const void * src )
{
    if (dest)
    {
	if (src)
	    memcpy(dest,src,sizeof(*dest));

	dest->ether_type = ntohs(dest->ether_type);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_ip4_head ( ip4_head_t *dest, const void * src )
{
    if (dest)
    {
	if (src)
	    memcpy(dest,src,sizeof(*dest));

	dest->total_len	= ntohs(dest->total_len);
	dest->id	= ntohs(dest->id);
	dest->frag_off	= ntohs(dest->frag_off);
	dest->checksum	= ntohs(dest->checksum);
	dest->ip_src	= ntohl(dest->ip_src);
	dest->ip_dest	= ntohl(dest->ip_dest);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_udp_head ( udp_head_t *dest, const void * src )
{
    if (dest)
    {
	const udp_head_t *usrc = src ? src : dest;

	dest->port_src	= ntohs(usrc->port_src);
	dest->port_dest	= ntohs(usrc->port_dest);
	dest->data_len	= ntohs(usrc->data_len);
	dest->checksum	= ntohs(usrc->checksum);
    }
}

///////////////////////////////////////////////////////////////////////////////

void ntoh_tcp_head ( tcp_head_t *dest, const void * src )
{
    if (dest)
    {
	const tcp_head_t *usrc = src ? src : dest;

	dest->port_src	= ntohs(usrc->port_src);
	dest->port_dest	= ntohs(usrc->port_dest);
	dest->seq_num	= ntohl(usrc->seq_num);
	dest->acc_num	= ntohl(usrc->acc_num);
	dest->data_off	= usrc->data_off;
	dest->flags	= usrc->flags;
	dest->window	= ntohs(usrc->window);
	dest->checksum	= ntohs(usrc->checksum);
	dest->urgent	= ntohs(usrc->urgent);
    }
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Linux support			///////////////
///////////////////////////////////////////////////////////////////////////////

#if defined(SYSTEM_LINUX) || defined(__CYGWIN__)
  #include "dclib-network-linux.c"
#endif // SYSTEM_LINUX

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////
