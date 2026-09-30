#ifndef AFPTEST_SRP_H
#define AFPTEST_SRP_H

#include <stddef.h>

#include <gcrypt.h>

#include <atalk/srp.h>

/*! SRP-6a session key length, MGF1 over the <atalk/srp.h> group #2 */
#define SRP_SESSION_KEY_LEN  40
#define SRP_PASSWDLEN        255

/*! Wire step markers. */
#define SRP_CLIENT_PROOF     0x0003
#define SRP_SERVER_PROOF     0x0004

extern const unsigned char *srp_strip_leading_zeros(const unsigned char *buf,
                                                    size_t len,
                                                    size_t *out_len);
extern void srp_mpi_to_padded_buf(unsigned char *buf, size_t nbytes,
                                  gcry_mpi_t m);
extern int  srp_mgf1_sha1(const unsigned char *seed, size_t seed_len,
                          unsigned char *out, size_t out_len);
extern int  srp_derive_x(const char *username, const char *password,
                         const unsigned char *salt, unsigned char *x_out);
extern int  srp_compute_k(unsigned char *k_out);
extern int  srp_compute_u(const unsigned char *a_padded,
                          const unsigned char *b_padded,
                          unsigned char *u_out);
extern int  srp_compute_proofs(const char *username,
                               const unsigned char *salt,
                               const unsigned char *a_stripped,
                               size_t a_len,
                               const unsigned char *b_stripped,
                               size_t b_len,
                               const unsigned char *key,
                               unsigned char *m1_out);
extern int  srp_compute_server_proof(const unsigned char *a_stripped,
                                     size_t a_len,
                                     const unsigned char *m1,
                                     const unsigned char *key,
                                     unsigned char *m2_out);

#endif /* AFPTEST_SRP_H */
