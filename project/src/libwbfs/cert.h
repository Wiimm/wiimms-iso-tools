
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

#ifndef WIT_CERT_H
#define WIT_CERT_H 1

#define _GNU_SOURCE 1

#include "file-formats.h"

// some info urls:
//  - http://wiibrew.org/wiki/Certificate_chain

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum cert_stat_flags_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum cert_stat_flags_t
{
    CERT_ERR_MASK	= 0x00ff,	// mask for error IDs

    CERT_F_BASE_OK	= 0x0100,	// signature base ok
    CERT_F_BASE_INVALID	= 0x0200,	// signature base wrong

    CERT_F_HASH_OK	= 0x0400,	// hash value ok 
    CERT_F_HASH_FAKED	= 0x0800,	// hash value is fake signed
    CERT_F_HASH_FAILED	= 0x1000,	// hash value invalid

    CERT_F_ERROR	= 0x2000,	// error while checking signature

} cert_stat_flags_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			enum cert_stat_t		///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum cert_stat_t
{
    CERT_SIG_OK			= CERT_F_BASE_OK | CERT_F_HASH_OK,
    CERT_SIG_FAKE_SIGNED	= CERT_F_BASE_OK | CERT_F_HASH_FAKED,
    CERT_HASH_FAILED		= CERT_F_BASE_OK | CERT_F_HASH_FAILED,

    CERT_HASH_OK		= CERT_F_BASE_INVALID | CERT_F_HASH_OK,
    CERT_FAKE_SIGNED		= CERT_F_BASE_INVALID | CERT_F_HASH_FAKED,
    CERT_SIG_FAILED		= CERT_F_BASE_INVALID | CERT_F_HASH_FAILED,

    CERT_ERR_TYPE_MISSMATCH	= CERT_F_ERROR + 1,
    CERT_ERR_NOT_SUPPORTED,
    CERT_ERR_NOT_FOUND,
    CERT_ERR_INVALID_SIG,

} cert_stat_t;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			struct cert_*_t			///////////////
///////////////////////////////////////////////////////////////////////////////
// [[cert_head_t]]

typedef struct cert_head_t
{
    u32			sig_type;	// signature type
    u8			cert_data[0];	// -> cert_data_t
}
__attribute__ ((packed)) cert_head_t;

///////////////////////////////////////////////////////////////////////////////
// [[cert_bin_t]]

typedef struct cert_bin_t
{
    u32			sig_type;	// signature type
    u8			sig_key[2][30];	// 2 signature keys
    u8			padding[0x40];
    u8			cert_data[0];	// -> cert_data_t
}
__attribute__ ((packed)) cert_bin_t;

///////////////////////////////////////////////////////////////////////////////
// [[cert_data_t]]

typedef struct cert_data_t
{
    char		issuer[0x40]; 	// signature issuer (chain name)
    u32			key_type;	// key type
    char		key_id[0x40];	// id of key
    u32			key_num;	// key number
    u8			public_key[];
}
__attribute__ ((packed)) cert_data_t;

///////////////////////////////////////////////////////////////////////////////
// [[cert_item_t]]

typedef struct cert_item_t
{
    char		name[0x82];	// concatenated name
    const cert_head_t	* head;		// pointer to cert head
    const cert_data_t	* data;		// pointer to cert data
    u32			sig_space;	// space for head->cert_data'
    u32			sig_size;	// used size of 'head->cert_data'
    u32			key_size;	// size of 'data->public_key'
    u32			data_size;	// size of 'data'
    u32			cert_size;	// total size of cert, 'head' is beginning
}
cert_item_t;

///////////////////////////////////////////////////////////////////////////////
// [[cert_chain_t]]

typedef struct cert_chain_t
{
    cert_item_t		* cert;		// pointer to certificate list
    int			used;		// used elements of 'cert'
    int			size;		// alloced elements of 'cert' 
}
cert_chain_t;

extern cert_chain_t global_cert;
extern cert_chain_t auto_cert;

const cert_item_t * FindCertItem
(
    ccp		issuer,		// name to search
    const cert_chain_t
		*cc,		// not NULL: search here first
    bool	search_global,	// if not found: search 'global_cert'
    bool	auto_auto	// if not found: search 'auto_cert' and add cert's
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			  cert data			///////////////
///////////////////////////////////////////////////////////////////////////////

extern cert_data_t root_cert;
extern u8 std_cert_chain[0xa00];

void setup_cert_data();

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: cert helpers		///////////////
///////////////////////////////////////////////////////////////////////////////

ccp cert_get_status_message
(
    cert_stat_t		stat,		// status
    ccp			ret_invalid	// return value if 'stat' unknown
);

///////////////////////////////////////////////////////////////////////////////

ccp cert_get_status_name
(
    cert_stat_t		stat,		// status
    ccp			ret_invalid	// return value if 'stat' unknown
);

///////////////////////////////////////////////////////////////////////////////
// [[sig_info_t]]

typedef struct sig_info_t
{
    uint		sig_type;	// signature type
    uint		sig_space;	// reserved space
    uint		sig_size;	// signature size
    uint		key_size;	// key size
    ccp			name;		// signature name
}
sig_info_t;

//-----------------------------------------------------------------------------

const sig_info_t * cert_get_signature_info ( uint sig_type );

ccp cert_get_signature_name
(
    u32			sig_type,	// signature type
    ccp			ret_invalid	// return value if 'sig_type' unknown
);

// returns 0 for unknown 'sig_type//key_type'
int cert_get_signature_space ( u32 sig_type );
int cert_get_signature_size  ( u32 sig_type );
int cert_get_pubkey_size     ( u32 key_type );

///////////////////////////////////////////////////////////////////////////////

cert_data_t * cert_get_data // return NULL if invalid
(
    const void		* head		// NULL or pointer to cert header (cert_head_t)
);

///////////////////////////////////////////////////////////////////////////////

cert_head_t * cert_get_next_head // return NULL if invalid
(
    const void		* data		// NULL or pointer to cert data (cert_data_t)
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: certificate chain	///////////////
///////////////////////////////////////////////////////////////////////////////

cert_chain_t * cert_initialize
(
    cert_chain_t	* cc		// NULL or pointer to structure
					// if NULL: alloc data
);

///////////////////////////////////////////////////////////////////////////////

void cert_reset
(
    cert_chain_t	* cc		// valid pointer to cert chain
);

///////////////////////////////////////////////////////////////////////////////

cert_item_t * cert_append_item
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    ccp			issuer,		// NULL or pointer to issuer
    ccp			key_id,		// NULL or valid pointer to key id
    bool		uniq		// true: avoid duplicates
);

///////////////////////////////////////////////////////////////////////////////

int cert_append_data
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    const void		* data,		// NULL or pointer to cert data
    size_t		data_size,	// size of 'data'
    bool		uniq		// true: avoid duplicates
);

///////////////////////////////////////////////////////////////////////////////

int cert_append_file
(
    cert_chain_t	* cc,		// valid pointer to cert chain
    ccp			filename,	// name of file
    bool		uniq		// true: avoid duplicates
);

///////////////////////////////////////////////////////////////////////////////

// issuer can be NULL
void cert_add_root ( ccp issuer );

//
///////////////////////////////////////////////////////////////////////////////
///////////////		    interface: check certificate	///////////////
///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const void		* sig_data,	// pointer to signature data
    u32			sig_data_size,	// size of 'sig_data'
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_cert
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const cert_item_t	* item,		// NULL or pointer to certificate data
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_ticket
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const wd_ticket_t	* ticket,	// NULL or pointer to ticket
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

///////////////////////////////////////////////////////////////////////////////

cert_stat_t cert_check_tmd
(
    const cert_chain_t	* cc,		// valid pointer to cert chain
    const wd_tmd_t	* tmd,		// NULL or pointer to tmd
    const cert_item_t	** cert_found	// not NULL: return value: found certificate
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    base64			///////////////
///////////////////////////////////////////////////////////////////////////////

typedef enum Base64Mode_t
{
	BASE64_NULL = -10,	// ASCII NUL
	BASE64_CONTROL,		// a control character
	BASE64_EOL,		// EOL (CR,LF)
	BASE64_SPACE,		// space, tabs
	BASE64_SEPARATE,	// separation characterts (comma,semicolon)
	BASE64_OTHER,		// other charatcers

	BASE64_MIN =  0,	// minimal digit value
	BASE64_MAX = 63,	// maximal digit value

} Base64Mode_t;

///////////////////////////////////////////////////////////////////////////////

uint CalcEncode64len
(
    // returns the needed buflen inclusive 0-Term

    uint	source_len,		// size of source data
    int		indent,			// >0: indent each line with spaces
    int		max_line_length,	// >0: force line breaks
    uint	*tupel_per_line		// not NULL: store num of tupel per line
);

//-----------------------------------------------------------------------------
// [[dclib]]

int Encode64buf
(
    // returns -1 if dest buffer is too small
    // otherwise it returns the number of written characters

    char	*buf,			// valid destination buffer
    uint	buf_size,		// size of buffer
    const void	*source,		// pointer to source data
    uint	source_len,		// size of source data
    int		indent,			// >0: indent each line with spaces
    int		max_line_length		// >0: force line breaks
);

//-----------------------------------------------------------------------------
// [[dclib]]

char * Encode64
(
    // returns a null terminated string (alloced)

    const void	*source,		// pointer to source data
    uint	source_len,		// size of source data
    int		indent,			// >0: indent each line with spaces
    int		max_line_length,	// >0: force line breaks
    uint	*return_len		// not NULL: store strlen(result)
);

//-----------------------------------------------------------------------------

uint PrintEncode64
(
    // returns a null terminated string (alloced)

    FILE	*f,			// destination file
    const void	*source,		// pointer to source data
    uint	source_len,		// size of source data
    int		indent,			// >0: indent each line with spaces
    int		max_line_length		// >0: force line breaks
)
;

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    etc				///////////////
///////////////////////////////////////////////////////////////////////////////

u32 cert_fake_sign 
(
    cert_item_t		* item		// pointer to certificate
);

//
///////////////////////////////////////////////////////////////////////////////
///////////////			    END				///////////////
///////////////////////////////////////////////////////////////////////////////

#endif // WIT_CERT_H
