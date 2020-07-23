
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

#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>

#ifdef SYSTEM_LINUX
 #include <linux/route.h>
#endif

#include "dclib-network.h"

//
///////////////////////////////////////////////////////////////////////////////
///////////////			SendRawUDP()			///////////////
///////////////////////////////////////////////////////////////////////////////

enumError SendRawUDP
(
    // Capability CAP_NET_RAW needed (or effective user ID of 0).

    int		sock,		// RAW socket to use,
				// if -1: open and close private socket

    ccp		send_addr,	// NULL or sender address:port
    u32		send_ip4,	// fall back IP4, if 'send_addr' empty
    u16		send_port,	// default sender port

    ccp		recv_addr,	// NULL or receiver address:port
    u32		recv_ip4,	// fall back IP4, if 'recv_addr' empty
    u16		recv_port,	// default receiver port

    const void	*data,		// data to send
    uint	size,		// size of 'data'
    uint	log_mode	// 0:silent, >0:print errors,
				// >=0x10: hexdump data, 'lmode' bytes max
)
{
    DASSERT( data || !size );


    //--- get addresses

    NetworkHost_t send_host;
    if ( send_addr && *send_addr )
    {
	if (!ResolveHost(&send_host,true,send_addr,send_port,false,false))
	{
	    if (log_mode)
		ERROR1(ERR_ERROR,"Can't resolve sender host: %s\n",send_addr);
	    return ERR_ERROR;
	}
    }
    else
    {
	send_host.sa.sin_family	= AF_INET;
	send_host.sa.sin_addr.s_addr = htonl(send_ip4);
	send_host.sa.sin_port = htons(send_port);
    }


    NetworkHost_t recv_host;
    if ( recv_addr && *recv_addr )
    {
	if (!ResolveHost(&recv_host,true,recv_addr,recv_port,false,false))
	{
	    if (log_mode)
		ERROR1(ERR_ERROR,"Can't resolve sender host: %s\n",recv_addr);
	    return ERR_ERROR;
	}
    }
    else
    {
	recv_host.sa.sin_family	= AF_INET;
	recv_host.sa.sin_addr.s_addr = htonl(recv_ip4);
	recv_host.sa.sin_port = htons(recv_port);
    }

    return SendRawUDPsa(sock,&send_host.sa,&recv_host.sa,data,size,log_mode);
}

///////////////////////////////////////////////////////////////////////////////

enumError SendRawUDPsa
(
    // Capability CAP_NET_RAW needed (or effective user ID of 0).

    int			sock,		// RAW socket to use,
					// if -1: open and close private socket

    struct sockaddr_in	*sa_send,	// sockaddr of sender
    struct sockaddr_in	*sa_recv,	// sockaddr of receiver

    const void		*data,		// data to send
    uint		size,		// size of 'data'
    uint		log_mode	// 0:silent, >0:print errors,
					// >=0x10: hexdump data, 'lmode' bytes max
)
{
    DASSERT(sa_send);
    DASSERT(sa_recv);
    DASSERT( data || !size );
    if ( size > MAX_UDP_PACKET_DATA )
	size = MAX_UDP_PACKET_DATA;

    TRACE("Send %zu+%u bytes from %s to %s\n",
		sizeof(ip4_head_t) + sizeof(udp_head_t), size,
		PrintIP4sa(0,0,sa_send),
		PrintIP4sa(0,0,sa_recv) );


    //--- open socket

    enumError err = ERR_OK;

    const bool own_sock = sock == -1;
    if (own_sock)
    {
	sock = socket(AF_INET,SOCK_RAW,IPPROTO_RAW);
	if ( sock == -1 )
	{
	    if (log_mode)
		ERROR1(ERR_CANT_CREATE,"Can't create RAW socket.\n");
	    err = ERR_CANT_CREATE;
	    goto abort;
	}

	int on = 1;
	setsockopt(sock,IPPROTO_IP,IP_HDRINCL,&on,sizeof(on));
    }


    //--- setup packet

    udp_packet_t pkt;
    const uint total_len = SetupRawUDPsa(&pkt,sa_send,sa_recv,data,size);
    if ( log_mode >= 0x10 )
	HexDump16( stdwrn, 0, 0, &pkt,
			total_len < log_mode ? total_len : log_mode );


    //--- send data

    //HEXDUMP16(0,0,&pkt,total_len);
    //HexDump16(stderr,0,0,&pkt,total_len);

    int stat = sendto(sock,&pkt,total_len,0,(struct sockaddr*)sa_recv,sizeof(*sa_recv));
    if ( stat == -1 )
    {
	ERROR1(ERR_ERROR,"Can't send to sock %d\n",sock);
	if (log_mode)
		ERROR1(ERR_WRITE_FAILED,"Can't send to socket %d\n",sock);
	err = ERR_WRITE_FAILED;
	goto abort;
    }


    //--- close socket and terminate

 abort:
    if ( own_sock && sock != -1 )
	close(sock);

    return err;
}

///////////////////////////////////////////////////////////////////////////////

uint SetupRawUDPsa
(
    // returns total packet len (mabe with limited 'size')

    udp_packet_t	*pkt,		// paket header to setup (cleared and written)

    struct sockaddr_in	*sa_send,	// sockaddr of sender
    struct sockaddr_in	*sa_recv,	// sockaddr of receiver
    const void		*data,		// data to send
    uint		size		// size of 'data'
)
{
    DASSERT(pkt);
    DASSERT(sa_send);
    DASSERT(sa_recv);
    DASSERT( data || !size );
    if ( size > MAX_UDP_PACKET_DATA )
	size = MAX_UDP_PACKET_DATA;


    //--- determine sender ip4

    u32 sender_ip4 = sa_send->sin_addr.s_addr;
    if (!sender_ip4)
    {
	RouteIP4_t rt;
	if (FindGatewayIP4(&rt,sa_recv->sin_addr.s_addr))
	    sender_ip4 = GetIP4ByInterface(rt.iface);
    }


    //--- create data structure

    static const u8 ip4_head[]
	= { 0x45,0x00, 0x00,0x00,  0x12,0x34, 0x40,0x00, 0x40,0x11 };

    const uint total_len = sizeof(ip4_head_t) + sizeof(udp_head_t) + size;

    memset(pkt,0,sizeof(*pkt));
    memcpy(&pkt->ip4,ip4_head,sizeof(ip4_head));
    memcpy(pkt->data,data,size);

    pkt->ip4.ip_src	= sender_ip4;
    pkt->ip4.ip_dest	= sa_recv->sin_addr.s_addr;
    pkt->ip4.total_len	= htons(total_len);
    pkt->ip4.checksum	= CalcChecksumIP4(&pkt->ip4,sizeof(pkt->ip4));

    pkt->udp.port_src	= sa_send->sin_port;
    pkt->udp.port_dest	= sa_recv->sin_port;
    pkt->udp.data_len	= htons(size+8);
    pkt->udp.checksum	= CalcChecksumUDP(&pkt->ip4,&pkt->udp,data);


    //--- terminate

    //HEXDUMP16(0,0,&pkt,total_len);
    //HexDump16(stderr,0,0,&pkt,total_len);

    return total_len;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Routing Support			///////////////
///////////////////////////////////////////////////////////////////////////////

bool OpenRouteScanIP4
(
    // returns TRUE on success and FALSE if no element is found
    RouteIP4_t	*rt	// valid data, will be initialized
)
{
    DASSERT(rt);
    memset(rt,0,sizeof(*rt));

    rt->index = -1;
    rt->col_iface = 0;
    rt->col_dest  = 1;
    rt->col_mask  = 7;
    rt->col_gate  = 2;
    rt->col_flags = 3;

 #ifdef __CYGWIN__
    return false;
 #else
    rt->f = fopen("/proc/net/route","rb");
    if (!rt->f)
	return false;

    char *ptr = fgets(rt->buf,sizeof(rt->buf),rt->f);
    if (!ptr)
	return false;

    uint col;
    for ( col = 0; *ptr; col++ )
    {
	while ( *ptr == ' ' || *ptr == '\t' )
	    ptr++;
	if (!*ptr)
	    break;
	char *beg = ptr;
	while ( *ptr && *ptr != ' ' && *ptr != '\t' )
	    ptr++;
	if (*ptr)
	    *ptr++ = 0;

	if      (!strcasecmp(beg,"Iface"))	rt->col_iface	= col;
	else if (!strcasecmp(beg,"Destination"))rt->col_dest	= col;
	else if (!strcasecmp(beg,"Mask"))	rt->col_mask	= col;
	else if (!strcasecmp(beg,"Gateway"))	rt->col_gate	= col;
	else if (!strcasecmp(beg,"Flags"))	rt->col_flags	= col;
    }

    TRACE("f=%p, COLS: if=%d, dest=%d, mask=%d, gate=%d, flags=%d\n",
	rt->f, rt->col_iface, rt->col_dest,
	rt->col_mask, rt->col_gate, rt->col_flags );

    return NextRouteScanIP4(rt);

 #endif // !__CYGWIN__
}

///////////////////////////////////////////////////////////////////////////////

bool NextRouteScanIP4
(
    // returns TRUE on success and FALSE if no more element is found
    RouteIP4_t	*rt	// valid data
)
{
    DASSERT(rt);

 #ifdef __CYGWIN__
    return false;
 #else
    char *ptr = fgets(rt->buf,sizeof(rt->buf),rt->f);
    if (!ptr)
    {
	rt->index = -1;
	return false;
    }

    uint col;
    for ( col = 0; *ptr; col++ )
    {
	while ( *ptr == ' ' || *ptr == '\t' )
	    ptr++;
	if (!*ptr)
	    break;
	char *beg = ptr;
	while ( *ptr && *ptr != ' ' && *ptr != '\t' )
	    ptr++;
	if (*ptr)
	    *ptr++ = 0;

	if ( col == rt->col_iface )	rt->iface = beg;
	else if ( col==rt->col_dest )	rt->dest  = strtoul(beg,0,16);
	else if ( col==rt->col_mask )	rt->mask  = strtoul(beg,0,16);
	else if ( col==rt->col_gate )	rt->gate  = strtoul(beg,0,16);
	else if ( col==rt->col_flags )	rt->flags = strtoul(beg,0,16);
    }

    return true;
 #endif // !__CYGWIN__
}

///////////////////////////////////////////////////////////////////////////////

void CloseRouteScanIP4
(
    RouteIP4_t	*rt	// valid data
)
{
 #ifndef __CYGWIN__
    DASSERT(rt);
    if (rt->f)
    {
	fclose(rt->f);
	rt->f = 0;
    }
 #endif // !__CYGWIN__
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

bool FindGatewayIP4
(
			// returns TRUE if gateway found
    RouteIP4_t	*rt,	// valid data, will be initialized
    u32		ip4	// search gateway for this address (network byte order)
)
{
    DASSERT(rt);

    bool found;
    for ( found = OpenRouteScanIP4(rt); found; found = NextRouteScanIP4(rt) )
    {
     #ifdef __CYGWIN__
	if ( rt->dest == ( ip4 & rt->mask ))
	    break;
     #else
	if ( !(rt->flags & RTF_UP) )
	    continue;

	if ( rt->flags & RTF_HOST )
	{
	    if ( rt->dest == ip4 )
		break;
	}
	else if ( rt->dest == ( ip4 & rt->mask ))
	    break;
     #endif
    }
    CloseRouteScanIP4(rt);
    return found;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

u32 GetIP4ByInterface
(			// return 0 or IP4 in network byte order
    ccp		iface	// interface name. If NULL: Use default gateway
)
{
    RouteIP4_t rt;
    if ( !iface || !*iface )
    {
	if (!FindGatewayIP4(&rt,0))
	    return 0;
	iface = rt.iface;
    }

    int sock = socket(AF_INET,SOCK_DGRAM,0);
    if ( sock == -1 )
	return 0;

    struct ifreq ifr;
    memset(&ifr,0,sizeof(ifr));
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name,iface,sizeof(ifr.ifr_name));
    int stat = ioctl(sock,SIOCGIFADDR,&ifr);
    close(sock);
    if ( stat == -1 )
	return 0;

    struct sockaddr_in *sa = (struct sockaddr_in*)&ifr.ifr_addr;
    return sa->sin_addr.s_addr;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////				END			///////////////
///////////////////////////////////////////////////////////////////////////////

