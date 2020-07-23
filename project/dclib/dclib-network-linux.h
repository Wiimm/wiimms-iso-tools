
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

#ifndef DCLIB_NETWORK_LINUX_H
#define DCLIB_NETWORK_LINUX_H 1

#ifndef DCLIB_NETWORK_H
  #include "dclib-network.h"
#endif

//
///////////////////////////////////////////////////////////////////////////////
///////////////			IP interface			///////////////
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
);

//-----------------------------------------------------------------------------

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
);

//-----------------------------------------------------------------------------

uint SetupRawUDPsa
(
    // returns total packet len (mabe with limited 'size')

    udp_packet_t	*pkt,		// paket header to setup (cleared and written)

    struct sockaddr_in	*sa_send,	// sockaddr of sender
    struct sockaddr_in	*sa_recv,	// sockaddr of receiver
    const void		*data,		// data to send
    uint		size		// size of 'data'
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			Routing Support			///////////////
///////////////////////////////////////////////////////////////////////////////
// all IP4 in network byte order

typedef struct RouteIP4_t
{
    //--- returned data

    int	  index;	// entry index, starts with 0, incremented for each found
			//  -1: invalid data

    char  *iface;	// pointer into 'buf': interface name
    u32	  dest;		// destination address, network byte order
    u32	  mask;		// destination network mask, network byte order
    u32	  gate;		// gateway address, network byte order
    u32	  flags;	// routing flags

    //--- internal data

    FILE  *f;		// open file
    uint  col_iface;	// column index for 'iface'
    uint  col_dest;	// column index for 'dest'
    uint  col_mask;	// column index for 'mask'
    uint  col_gate;	// column index for 'gate'
    uint  col_flags;	// column index for 'flags'
    char  buf[200];	// line buffer for /proc/net/route
}
RouteIP4_t;

///////////////////////////////////////////////////////////////////////////////

bool OpenRouteScanIP4
(
    // returns TRUE on success and FALSE if no element is found
    RouteIP4_t	*rt	// valid data, will be initialized
);

bool NextRouteScanIP4
(
    // returns TRUE on success and FALSE if no more element is found
    RouteIP4_t	*rt	// valid data
);

void CloseRouteScanIP4
(
    RouteIP4_t	*rt	// valid data
);

///////////////////////////////////////////////////////////////////////////////

bool FindGatewayIP4
(
			// returns TRUE if gateway found
    RouteIP4_t	*rt,	// valid data, will be initialized
    u32		ip4	// search gateway for this address (network byte order)
);

u32 GetIP4ByInterface
(			// return 0 or IP4 in network byte order
    ccp		iface	// interface name. If NULL: Use default gateway
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    E N D			///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // DCLIB_NETWORK_LINUX_H


