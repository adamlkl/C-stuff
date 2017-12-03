/*!
 * @file cs2014coin-make.c
 * @brief This is the implementation of the cs2014 coin maker
 *
 * It should go without saying that these coins are for play:-)
 *
 * This is part of CS2014
 *    https://down.dsg.cs.tcd.ie/cs2014/examples/c-progs-2/README.html
 */

/*
 * Author: Leong Kai Ler
 * Copyright (c) 2017 stephen.farrell@cs.tcd.ie
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "mbedtls/config.h"

#include "mbedtls/platform.h"

#include <stdio.h>
#define mbedtls_printf printf

#include "mbedtls/error.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>

#define FORMAT_PEM      0
#define FORMAT_DER      1

#include "cs2014coin.h"
#include "cs2014coin-int.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>

/*!
 * @brief make a coin
 * @param bits specifies how many bits need to be zero in the hash-output
 * @param buf is an allocated buffer for the coid
 * @param buflen is an in/out parameter reflecting the buffer-size/actual-coin-size
 * @return the random byte
 *
 * Make me a coin of the required quality/strength
 *
 */

unsigned char rndbyte()
{
    unsigned long int s;
    syscall(SYS_getrandom, &s, sizeof(unsigned long int), 0);
    unsigned char byte=(s>>16)%256;
    return(byte);
}

int cs2014coin_make(int bits, unsigned char *buf, int *buflen) {

    mbedtls_md_context_t sha_ctx;
    //initialise buffer
    //Don't need to do anything for the ciphersuite
    unsigned char *tmp = calloc (*buflen,1);
    if (!tmp)
    {
        return (1);
    }

    //difficulty in terms of bits
    for (int count = 4;count<8;count++)
    {
        tmp[count]=(bits>>(7-count)*8)&0xFF;
    }

    //length of public key
    int pubKeyLen = 158;
    for (int count = 8;count<12;count++)
    {
        tmp[count]=(pubKeyLen>>(11-count)*8)&0xFF;
    }

    //public key value
    mbedtls_pk_context key;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "gen key";

    mbedtls_pk_init(  &key );
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    //generating a public key
    unsigned char keyArr[pubKeyLen];
    mbedtls_ctr_drbg_seed( &ctr_drbg,mbedtls_entropy_func, &entropy,(const unsigned char *) pers, strlen( pers ) ) ;
    mbedtls_pk_setup( &key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
    mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP521R1, mbedtls_pk_ec(key), mbedtls_ctr_drbg_random,&ctr_drbg);
    mbedtls_pk_write_pubkey_der(&key,keyArr,sizeof(keyArr));

    for (int count=0; count<sizeof(keyArr);count++ )
    {
        tmp[12+count]=keyArr[count];
    }

    //length of nonce
    int nonceLength=32;

    for (int count=12+sizeof(keyArr);count<(12+sizeof(keyArr))+4;count++)
    {
        tmp[count]=(nonceLength>>(173-count)*8)&0xFF;
    }

    int validHash=0;

    while(!validHash)
    {
        char *nonce =  calloc(nonceLength,1);
        int integer=0;

        //generate nonce value
        for (int count=0; count<nonceLength; count++)
        {
            tmp[count+174]=nonce[count]=rndbyte();
        }

        //length of PoW hash
        int hashLen = 32;

        for (int count=0; count<4; count++)
        {
            tmp[206+count]=(hashLen>>(209-count)*8)&0xFF;
        }

        //generate PoW Hash
        unsigned char *hashBuf = calloc(hashLen, 1);

        mbedtls_md_init(&sha_ctx);
        mbedtls_md_setup(&sha_ctx,mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),1);

        mbedtls_md_starts( &sha_ctx );
        mbedtls_md_update( &sha_ctx, tmp, 242 );
        mbedtls_md_finish( &sha_ctx, hashBuf );

        if (zero_bits(bits,hashBuf,hashLen)) {
            validHash = 1;
            for (int count = 0; count < hashLen; count++)
            {
                tmp[count + 210] = hashBuf[count];
            }
        }
    }

    //length of signature
    unsigned char *bufHashed = calloc (32,1);

    size_t sigLen;
    mbedtls_md_starts( &sha_ctx );
    mbedtls_md_update( &sha_ctx, tmp, 242 );
    mbedtls_md_finish( &sha_ctx, bufHashed );

    unsigned char sign[sigLen];
    mbedtls_pk_sign(&key, MBEDTLS_MD_SHA256, bufHashed, 0, sign, &sigLen, mbedtls_ctr_drbg_random, &ctr_drbg);

    int fullSize =  246+sigLen;

	//tmp[242] = (unsigned char) sigLen;
    for (int count=0; count<4; count++)
    {
        tmp[242+count]=(sigLen>>(245-count)*8)&0xFF;
    }

    for (int count=0; count<sigLen; count++)
    {
        tmp[246+count]=sign[count];
    }

    memcpy(buf,tmp,fullSize);
    *buflen=fullSize;
    dumpbuf("Coin here: ", buf, (fullSize));
    return(0);
}
