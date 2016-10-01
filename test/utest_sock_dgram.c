#include "testutils.h"

///

int sock_dgram_1_result_counter;
EVP_MD_CTX * sock_dgram_1_mdctx;

bool sock_dgram_1_delegate_read(struct ft_dgram * dgram, struct ft_frame * frame)
{
	bool ok;

//	ft_frame_fprintf(frame, stdout);
	ft_frame_flip(frame);

	for (struct ft_vec * dvec = ft_frame_get_vec(frame); dvec != NULL; dvec = ft_frame_next_vec(frame))
	{
		int rc = EVP_DigestUpdate(sock_dgram_1_mdctx, ft_vec_ptr(dvec), ft_vec_len(dvec));
		ck_assert_int_eq(rc, 1);
	}

	sock_dgram_1_result_counter += 1;

	ok = ft_dgram_cntl(dgram, FT_DGRAM_READ_STOP | FT_DGRAM_WRITE_STOP);
	ck_assert_int_eq(ok, true);

	ft_frame_return(frame);
	return true;
}

struct ft_dgram_delegate sock_dgram_1_delegate =
{
	.read = sock_dgram_1_delegate_read,
};

START_TEST(sock_dgram_1_utest)
{
	struct ft_dgram dgram_sock;
	int rc;
	bool ok;

	sock_dgram_1_result_counter = 0;

	ft_config.log_verbose = true;
	ft_config.log_trace_mask |= FT_TRACE_ID_DGRAM | FT_TRACE_ID_EVENT_LOOP;


	generate_random_file("./sock_dgram_1_utest.bin", 4096, 1);

	EVP_MD_CTX * mdctx = EVP_MD_CTX_create();
	ck_assert_ptr_ne(mdctx, NULL);

	rc = EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
	ck_assert_int_eq(rc, 1);

	digest_file(mdctx, "./sock_dgram_1_utest.bin");

	unsigned char digest1[EVP_MAX_MD_SIZE];
	EVP_DigestFinal(mdctx, digest1, NULL);
    EVP_MD_CTX_cleanup(mdctx);


	sock_dgram_1_mdctx = EVP_MD_CTX_create();
	ck_assert_ptr_ne(sock_dgram_1_mdctx, NULL);
	
	rc = EVP_DigestInit_ex(sock_dgram_1_mdctx, EVP_sha256(), NULL);
	ck_assert_int_eq(rc, 1);


	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_dgram_bind(&dgram_sock, &sock_dgram_1_delegate, &context, rp);

	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	FILE * p = popen("socat open:sock_dgram_1_utest.bin udp-sendto:127.0.0.1:12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fflush(p);

	ft_context_run(&context);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ck_assert_int_eq(dgram_sock.stats.read_events, 1);
	ck_assert_int_eq(dgram_sock.stats.write_events, 0);
	ck_assert_int_eq(dgram_sock.stats.read_bytes, 4096);
	ck_assert_int_eq(dgram_sock.stats.write_bytes, 0);

	ft_dgram_fini(&dgram_sock);

	ft_context_fini(&context);

	unsigned char digest2[EVP_MAX_MD_SIZE];
	EVP_DigestFinal(sock_dgram_1_mdctx, digest2, NULL);
    EVP_MD_CTX_cleanup(sock_dgram_1_mdctx);
    ck_assert_int_eq(memcmp(digest1, digest2, EVP_MD_size(EVP_sha256())), 0);

	ck_assert_int_eq(sock_dgram_1_result_counter, 1);
}
END_TEST

///


///

Suite * sock_dgram_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_dgram");

	tc = tcase_create("sock_dgram-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_dgram_1_utest);

	return s;
}
