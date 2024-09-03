/* wc_kyber.h
 *
 * Copyright (C) 2006-2024 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

/*!
    \file wolfssl/wolfcrypt/wc_kyber.h
*/


#ifndef WOLF_CRYPT_WC_KYBER_H
#define WOLF_CRYPT_WC_KYBER_H

#include <wolfssl/wolfcrypt/types.h>
#include <wolfssl/wolfcrypt/random.h>
#include <wolfssl/wolfcrypt/sha3.h>
#include <wolfssl/wolfcrypt/kyber.h>

#ifdef WOLFSSL_HAVE_KYBER

#ifdef noinline
    #define KYBER_NOINLINE noinline
#elif defined(_MSC_VER)
    #define KYBER_NOINLINE __declspec(noinline)
#elif defined(__GNUC__)
    #define KYBER_NOINLINE __attribute__((noinline))
#else
    #define KYBER_NOINLINE
#endif

/* Define algorithm type when not excluded. */

#ifndef WOLFSSL_NO_KYBER512
#define WOLFSSL_KYBER512
#endif
#ifndef WOLFSSL_NO_KYBER768
#define WOLFSSL_KYBER768
#endif
#ifndef WOLFSSL_NO_KYBER1024
#define WOLFSSL_KYBER1024
#endif

enum {
    /* Flags of Kyber keys. */
    KYBER_FLAG_PRIV_SET = 0x0001,
    KYBER_FLAG_PUB_SET  = 0x0002,
    KYBER_FLAG_BOTH_SET = 0x0003,
    KYBER_FLAG_H_SET    = 0x0004,

    /* 2 bits of random used to create noise value. */
    KYBER_CBD_ETA2          = 2,
    /* 3 bits of random used to create noise value. */
    KYBER_CBD_ETA3          = 3,

    /* Number of bits to compress to. */
    KYBER_COMP_4BITS    =  4,
    KYBER_COMP_5BITS    =  5,
    KYBER_COMP_10BITS   = 10,
    KYBER_COMP_11BITS   = 11,
};


/* SHAKE128 rate. */
#define XOF_BLOCK_SIZE      168

/* Modulus of co-efficients of polynomial. */
#define KYBER_Q             3329


/* Kyber-512 parameters */
#ifdef WOLFSSL_KYBER512
/* Number of bits of random to create noise from. */
#define KYBER512_ETA1       KYBER_CBD_ETA3
#endif /* WOLFSSL_KYBER512 */

/* Kyber-768 parameters */
#ifdef WOLFSSL_KYBER768
/* Number of bits of random to create noise from. */
#define KYBER768_ETA1       KYBER_CBD_ETA2
#endif /* WOLFSSL_KYBER768 */

/* Kyber-1024 parameters */
#ifdef WOLFSSL_KYBER1024
/* Number of bits of random to create noise from. */
#define KYBER1024_ETA1      KYBER_CBD_ETA2
#endif /* WOLFSSL_KYBER1024 */



/* The data type of the pseudo-random function. */
#define KYBER_PRF_T     wc_Shake

/* Kyber key. */
struct KyberKey {
    /* Type of key: KYBER512, KYBER768, KYBER1024 */
    int type;
    /* Dynamic memory allocation hint. */
    void* heap;
#if defined(WOLF_CRYPTO_CB)
    /* Device Id. */
    int devId;
#endif
    /* Flags indicating what is stored in the key. */
    int flags;

    /* A pseudo-random function object. */
    KYBER_PRF_T prf;

    /* Private key as a vector. */
    sword16 priv[KYBER_MAX_K * KYBER_N];
    /* Public key as a vector. */
    sword16 pub[KYBER_MAX_K * KYBER_N];
    /* Public seed. */
    byte pubSeed[KYBER_SYM_SZ];
    /* Public hash - hash of encoded public key. */
    byte h[KYBER_SYM_SZ];
    /* Randomizer for decapsulation. */
    byte z[KYBER_SYM_SZ];
};

#ifdef __cplusplus
    extern "C" {
#endif

WOLFSSL_LOCAL
void kyber_init(void);
WOLFSSL_LOCAL
void kyber_keygen(sword16* priv, sword16* pub, sword16* e, const sword16* a,
    int kp);
WOLFSSL_LOCAL
void kyber_encapsulate(const sword16* pub, sword16* bp, sword16* v,
    const sword16* at, sword16* sp, const sword16* ep, const sword16* epp,
    const sword16* m, int kp);
WOLFSSL_LOCAL
void kyber_decapsulate(const sword16* priv, sword16* mp, sword16* bp,
    const sword16* v, int kp);

WOLFSSL_LOCAL
int kyber_gen_matrix(KYBER_PRF_T* prf, sword16* a, int kp, byte* seed,
    int transposed);
WOLFSSL_LOCAL
int kyber_get_noise(KYBER_PRF_T* prf, int kp, sword16* vec1, sword16* vec2,
    sword16* poly, byte* seed);

#ifdef USE_INTEL_SPEEDUP
WOLFSSL_LOCAL
int kyber_kdf(byte* seed, int seedLen, byte* out, int outLen);
#endif
WOLFSSL_LOCAL
void kyber_prf_init(KYBER_PRF_T* prf);
WOLFSSL_LOCAL
int kyber_prf_new(KYBER_PRF_T* prf, void* heap, int devId);
WOLFSSL_LOCAL
void kyber_prf_free(KYBER_PRF_T* prf);

WOLFSSL_LOCAL
int kyber_cmp(const byte* a, const byte* b, int sz);

WOLFSSL_LOCAL
void kyber_vec_compress_10(byte* r, sword16* v, unsigned int kp);
WOLFSSL_LOCAL
void kyber_vec_compress_11(byte* r, sword16* v);
WOLFSSL_LOCAL
void kyber_vec_decompress_10(sword16* v, const unsigned char* b,
    unsigned int kp);
WOLFSSL_LOCAL
void kyber_vec_decompress_11(sword16* v, const unsigned char* b);

WOLFSSL_LOCAL
void kyber_compress_4(byte* b, sword16* p);
WOLFSSL_LOCAL
void kyber_compress_5(byte* b, sword16* p);
WOLFSSL_LOCAL
void kyber_decompress_4(sword16* p, const unsigned char* b);
WOLFSSL_LOCAL
void kyber_decompress_5(sword16* p, const unsigned char* b);

WOLFSSL_LOCAL
void kyber_from_msg(sword16* p, const byte* msg);
WOLFSSL_LOCAL
void kyber_to_msg(byte* msg, sword16* p);
WOLFSSL_LOCAL
void kyber_from_bytes(sword16* p, const byte* b, int k);
WOLFSSL_LOCAL
void kyber_to_bytes(byte* b, sword16* p, int k);

#ifdef USE_INTEL_SPEEDUP
WOLFSSL_LOCAL
void kyber_keygen_avx2(sword16* priv, sword16* pub, sword16* e,
    const sword16* a, int kp);
WOLFSSL_LOCAL
void kyber_encapsulate_avx2(const sword16* pub, sword16* bp, sword16* v,
    const sword16* at, sword16* sp, const sword16* ep, const sword16* epp,
    const sword16* m, int kp);
WOLFSSL_LOCAL
void kyber_decapsulate_avx2(const sword16* priv, sword16* mp, sword16* bp,
    const sword16* v, int kp);

WOLFSSL_LOCAL
unsigned int kyber_rej_uniform_n_avx2(sword16* p, unsigned int len,
    const byte* r, unsigned int rLen);
WOLFSSL_LOCAL
unsigned int kyber_rej_uniform_avx2(sword16* p, unsigned int len, const byte* r,
    unsigned int rLen);
WOLFSSL_LOCAL
void kyber_redistribute_21_rand_avx2(const word64* s, byte* r0, byte* r1,
    byte* r2, byte* r3);
void kyber_redistribute_17_rand_avx2(const word64* s, byte* r0, byte* r1,
    byte* r2, byte* r3);
void kyber_redistribute_16_rand_avx2(const word64* s, byte* r0, byte* r1,
    byte* r2, byte* r3);
void kyber_redistribute_8_rand_avx2(const word64* s, byte* r0, byte* r1,
    byte* r2, byte* r3);

WOLFSSL_LOCAL
void kyber_sha3_blocksx4_avx2(word64* s);
WOLFSSL_LOCAL
void kyber_sha3_128_blocksx4_seed_avx2(word64* s, byte* seed);
WOLFSSL_LOCAL
void kyber_sha3_256_blocksx4_seed_avx2(word64* s, byte* seed);

WOLFSSL_LOCAL
void kyber_cbd_eta2_avx2(sword16* p, const byte* r);
WOLFSSL_LOCAL
void kyber_cbd_eta3_avx2(sword16* p, const byte* r);

WOLFSSL_LOCAL
void kyber_from_msg_avx2(sword16* p, const byte* msg);
WOLFSSL_LOCAL
void kyber_to_msg_avx2(byte* msg, sword16* p);

WOLFSSL_LOCAL
void kyber_from_bytes_avx2(sword16* p, const byte* b);
WOLFSSL_LOCAL
void kyber_to_bytes_avx2(byte* b, sword16* p);

WOLFSSL_LOCAL
void kyber_compress_10_avx2(byte* r, const sword16* p, int n);
WOLFSSL_LOCAL
void kyber_decompress_10_avx2(sword16* p, const byte* r, int n);
WOLFSSL_LOCAL
void kyber_compress_11_avx2(byte* r, const sword16* p, int n);
WOLFSSL_LOCAL
void kyber_decompress_11_avx2(sword16* p, const byte* r, int n);

WOLFSSL_LOCAL
void kyber_compress_4_avx2(byte* r, const sword16* p);
WOLFSSL_LOCAL
void kyber_decompress_4_avx2(sword16* p, const byte* r);
WOLFSSL_LOCAL
void kyber_compress_5_avx2(byte* r, const sword16* p);
WOLFSSL_LOCAL
void kyber_decompress_5_avx2(sword16* p, const byte* r);


WOLFSSL_LOCAL
int kyber_cmp_avx2(const byte* a, const byte* b, int sz);
#endif

#ifdef __cplusplus
    } /* extern "C" */
#endif

#endif /* WOLFSSL_HAVE_KYBER */

#endif /* WOLF_CRYPT_WC_KYBER_H */

