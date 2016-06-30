#include "testutils.h"

///

bool resolve(struct addrinfo **res, const char * host, const char * port)
{
	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	int rc = getaddrinfo(host, port, &hints, res);
	if (rc != 0)
	{
		*res = NULL;
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
		return false;
	}

	return true;
}

///

#define READ 0
#define WRITE 1

pid_t popen2(const char *command, int *infp, int *outfp)
{
	int p_stdin[2], p_stdout[2];
	pid_t pid;

	if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
		return -1;

	pid = fork();

	if (pid < 0)
		return pid;
	else if (pid == 0)
	{
		close(p_stdin[WRITE]);
		dup2(p_stdin[READ], READ);
		close(p_stdout[READ]);
		dup2(p_stdout[WRITE], WRITE);

		execl("/bin/sh", "sh", "-c", command, NULL);
		perror("execl");
		exit(1);
	}

	if (infp == NULL)
		close(p_stdin[WRITE]);
	else
		*infp = p_stdin[WRITE];

	if (outfp == NULL)
		close(p_stdout[READ]);
	else
		*outfp = p_stdout[READ];

	return pid;
}

//

void generate_random_file(const char * fname, size_t size, size_t nitems)
{
	FILE * fo = fopen(fname, "wb");
	ck_assert_ptr_ne(fo, NULL);

	FILE * fi = fopen("/dev/urandom", "rb");
	ck_assert_ptr_ne(fi, NULL);

	for (int i = 0; i < nitems; i+=1)
	{
		char buffer[size];
		fread(buffer, size, 1, fi);
		fwrite(buffer, size, 1, fo);
	}

	fclose(fo);
	fclose(fi);
}


void digest_file(EVP_MD_CTX * mdctx, const char * fname)
{
	int rc;

	FILE * f = fopen(fname, "rb");
	ck_assert_ptr_ne(f, NULL);

	while (true)
	{
		char buffer[4096];
		size_t s = fread(buffer, 1, sizeof(buffer), f);
		if (s == 0) break;
		ck_assert_int_gt(s, 0);
		rc = EVP_DigestUpdate(mdctx, buffer, s);
		ck_assert_int_eq(rc, 1);
	}
}

void digest_print(unsigned char digest[EVP_MAX_MD_SIZE], unsigned int digest_size)
{
	char hex_digest[digest_size * 2 + 1];
	hex_digest[digest_size * 2] = '\0';

	int i, j;
	for(i=j=0; i<digest_size; i++)
	{
		char c;
		c = (digest[i] >> 4) & 0xf;
		c = (c>9) ? c+'a'-10 : c + '0';
		hex_digest[j++] = c;
		c = (digest[i] & 0xf);
		c = (c>9) ? c+'a'-10 : c + '0';
		hex_digest[j++] = c;
	}

	printf("%s\n", hex_digest);

}
