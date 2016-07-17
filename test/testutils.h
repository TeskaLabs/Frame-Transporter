#ifndef __LIBSCCMN_TESTUTILS_H__
#define __LIBSCCMN_TESTUTILS_H__

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <check.h>

#include <ft.h>

bool resolve(struct addrinfo **res, const char * host, const char * port);
pid_t popen2(const char *command, int *infp, int *outfp);

void generate_random_file(const char * fname, size_t size, size_t nitems);
void digest_file(EVP_MD_CTX * mdctx, const char * fname);
void digest_print(unsigned char digest[EVP_MAX_MD_SIZE], unsigned int digest_size);

#endif // __LIBSCCMN_TESTUTILS_H__
