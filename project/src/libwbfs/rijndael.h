#ifndef RIJNDAEL_H
#define RIJNDAEL_H

#include "libwbfs_os.h" // os dependent definitions

///////////////////////////////////////////////////////////////////////////////

typedef struct aes_key_t
{
	int Nk;
	int Nb;
	int Nr;
	u8  fi[24];
	u8  ri[24];
	u32 fkey[120];
	u32 rkey[120];

} aes_key_t;

//-----------------------------------------------------------------------------

void wd_aes_set_key
(
	aes_key_t * akey,
	const void *key
);

void wd_aes_decrypt
(
	const aes_key_t * akey,
	const void *p_iv,
	const void *p_inbuf,
	void *p_outbuf,
	u64 len
);

void wd_aes_encrypt
(
	const aes_key_t * akey,
	const void *p_iv,
	const void *p_inbuf,
	void *p_outbuf,
	u64 len
);

///////////////////////////////////////////////////////////////////////////////

#endif // RIJNDAEL_H
