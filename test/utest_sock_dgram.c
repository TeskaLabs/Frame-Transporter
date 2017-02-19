#include "testutils.h"
#include <sys/un.h>

///

int sock_dgram_1_result_counter;
EVP_MD_CTX * sock_dgram_1_mdctx;

bool sock_dgram_1_delegate_read(struct ft_dgram * dgram, struct ft_frame * frame)
{
	bool ok;
	char addrstr[1024];

	switch (frame->addr.ss_family)
	{
		case AF_INET:
		case AF_INET6:
		{
			char hoststr[NI_MAXHOST];
			char portstr[NI_MAXSERV];
			int rc = getnameinfo((struct sockaddr *)&frame->addr, frame->addrlen, hoststr, sizeof(hoststr), portstr, sizeof(portstr), NI_NUMERICHOST | NI_NUMERICSERV);
			if (rc == 0)
			{
				snprintf(addrstr, sizeof(addrstr)-1, "%s:%s", hoststr, portstr);
			}
			else
			{
				snprintf(addrstr, sizeof(addrstr)-1, "?:?(%s)", gai_strerror(rc));
			}
			break;
		}

		case AF_UNIX:
		{
			struct sockaddr_un * un_addr = (struct sockaddr_un *)&frame->addr;
			snprintf(addrstr, sizeof(addrstr)-1, "UNIX:'%s'", un_addr->sun_path);
			break;
		}

		default:
		{
			if ((frame->addr.ss_family == AF_UNSPEC) && (frame->type == FT_FRAME_TYPE_END_OF_STREAM))
				snprintf(addrstr, sizeof(addrstr)-1, "END_OF_STREAM");
			else
				snprintf(addrstr, sizeof(addrstr)-1, "UNKNOWN(%d)", frame->addr.ss_family);
		}
	}

	FT_INFO("Incoming frame: fd:%d %s f:%p ft:%08llx", dgram->read_watcher.fd, addrstr, frame, (unsigned long long) frame->type);

	if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
		return false;

//	ft_frame_fwrite(frame, stdout);
	ft_frame_flip(frame);

	for (struct ft_vec * dvec = ft_frame_get_vec(frame); dvec != NULL; dvec = ft_frame_next_vec(frame))
	{
		int rc = EVP_DigestUpdate(sock_dgram_1_mdctx, ft_vec_ptr(dvec), ft_vec_len(dvec));
		ck_assert_int_eq(rc, 1);
	}

	ft_frame_reset_vec(frame);

	sock_dgram_1_result_counter += 1;

	ok = ft_dgram_cntl(dgram, FT_DGRAM_READ_STOP);
	ck_assert_int_eq(ok, true);

	//ok  = ft_dgram_write(dgram, frame);
	//ck_assert_int_eq(ok, true);
	ft_frame_return(frame);
	mark_point();

	ok = ft_dgram_cntl(dgram, FT_DGRAM_SHUTDOWN);
	ck_assert_int_eq(ok, true);

	return true;
}

struct ft_dgram_delegate sock_dgram_1_delegate =
{
	.read = sock_dgram_1_delegate_read,
};

START_TEST(sock_dgram_1_utest)
{
	struct ft_dgram dgram_sock1;
	struct ft_dgram dgram_sock2;
	struct addrinfo * rp;
	int rc;
	bool ok;

	sock_dgram_1_result_counter = 0;

	//ft_config.log_verbose = true;
	// /ft_config.log_trace_mask |= FT_TRACE_ID_DGRAM | FT_TRACE_ID_EVENT_LOOP | FT_TRACE_ID_MEMPOOL;

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

	rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_dgram_init(&dgram_sock1, &sock_dgram_1_delegate, &context, rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_bind(&dgram_sock1, rp->ai_addr, rp->ai_addrlen);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_bind(&dgram_sock1, rp->ai_addr, rp->ai_addrlen);
	ck_assert_int_eq(ok, false);



	ok = ft_dgram_init(&dgram_sock2, &sock_dgram_1_delegate, &context, rp->ai_family, rp->ai_socktype, rp->ai_protocol);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_connect(&dgram_sock2, rp->ai_addr, rp->ai_addrlen);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_connect(&dgram_sock2, rp->ai_addr, rp->ai_addrlen);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);


	FILE * f = fopen("./sock_dgram_1_utest.bin", "rb");
	ck_assert_ptr_ne(f, NULL);

	ok = ft_frame_fread(frame, f);
	ck_assert_int_eq(ok, true);

	fclose(f);


	ft_frame_flip(frame);

	ok = ft_dgram_write(&dgram_sock2, frame);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_cntl(&dgram_sock2, FT_DGRAM_SHUTDOWN);
	ck_assert_int_eq(ok, true);


	ft_context_run(&context);

	ck_assert_int_eq(dgram_sock1.stats.read_events, 1);
	ck_assert_int_eq(dgram_sock1.stats.write_events, 1);
	ck_assert_int_eq(dgram_sock1.stats.read_bytes, 4096);
	ck_assert_int_eq(dgram_sock1.stats.write_bytes, 0);

	ck_assert_int_eq(dgram_sock2.stats.read_events, 0);
	ck_assert_int_eq(dgram_sock2.stats.write_events, 1);
	ck_assert_int_eq(dgram_sock2.stats.read_bytes, 0);
	ck_assert_int_eq(dgram_sock2.stats.write_bytes, 4096);

	ft_dgram_fini(&dgram_sock1);
	ft_dgram_fini(&dgram_sock2);

	ft_context_fini(&context);

	unsigned char digest2[EVP_MAX_MD_SIZE];
	EVP_DigestFinal(sock_dgram_1_mdctx, digest2, NULL);
    EVP_MD_CTX_cleanup(sock_dgram_1_mdctx);
    ck_assert_int_eq(memcmp(digest1, digest2, EVP_MD_size(EVP_sha256())), 0);

	ck_assert_int_eq(sock_dgram_1_result_counter, 1);

	ck_assert_int_eq(ft_log_stats.warn_count, 2);
	ck_assert_int_eq(ft_log_stats.error_count, 1);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///


START_TEST(sock_dgram_2_utest)
{
	struct ft_dgram dgram_sock1;
	struct ft_dgram dgram_sock2;

	int rc;
	bool ok;

	sock_dgram_1_result_counter = 0;

	// ft_config.log_verbose = true;
	// ft_config.log_trace_mask |= FT_TRACE_ID_DGRAM | FT_TRACE_ID_EVENT_LOOP;

	generate_random_file("./sock_dgram_2_utest.bin", 4096, 1);

	EVP_MD_CTX * mdctx = EVP_MD_CTX_create();
	ck_assert_ptr_ne(mdctx, NULL);

	rc = EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
	ck_assert_int_eq(rc, 1);

	digest_file(mdctx, "./sock_dgram_2_utest.bin");

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

	struct sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
	strncpy(addr.sun_path, "/tmp/libft_sock_dgram_2_utest.sock", sizeof (addr.sun_path));
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';
	unlink(addr.sun_path);

	ok = ft_dgram_init(&dgram_sock1, &sock_dgram_1_delegate, &context, AF_LOCAL, SOCK_DGRAM, 0);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_bind(&dgram_sock1, (const struct sockaddr *)&addr, SUN_LEN(&addr));
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_bind(&dgram_sock1, (const struct sockaddr *)&addr, SUN_LEN(&addr));
	ck_assert_int_eq(ok, false);

	ft_dgram_fini(&dgram_sock1);


	ok = ft_dgram_init(&dgram_sock1, &sock_dgram_1_delegate, &context, AF_LOCAL, SOCK_DGRAM, 0);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_bind(&dgram_sock1, (const struct sockaddr *)&addr, SUN_LEN(&addr));
	ck_assert_int_eq(ok, true);


	ok = ft_dgram_init(&dgram_sock2, &sock_dgram_1_delegate, &context, AF_LOCAL, SOCK_DGRAM, 0);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_connect(&dgram_sock2, (const struct sockaddr *)&addr, SUN_LEN(&addr));
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_connect(&dgram_sock2, (const struct sockaddr *)&addr, SUN_LEN(&addr));
	ck_assert_int_eq(ok, true);

	struct sockaddr_un addr2;
	addr2.sun_family = AF_LOCAL;
	strncpy(addr2.sun_path, "/tmp/libft_sock_dgram_2_2_utest.sock", sizeof (addr.sun_path));
	addr2.sun_path[sizeof(addr2.sun_path) - 1] = '\0';
	unlink(addr2.sun_path);

	ok = ft_dgram_bind(&dgram_sock2, (const struct sockaddr *)&addr2, SUN_LEN(&addr2));
	ck_assert_int_eq(ok, true);


	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);


	FILE * f = fopen("./sock_dgram_2_utest.bin", "rb");
	ck_assert_ptr_ne(f, NULL);

	ok = ft_frame_fread(frame, f);
	ck_assert_int_eq(ok, true);

	fclose(f);


	ft_frame_flip(frame);

	ok = ft_dgram_write(&dgram_sock2, frame);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_cntl(&dgram_sock2, FT_DGRAM_SHUTDOWN);
	ck_assert_int_eq(ok, true);

	ft_context_run(&context);

	ck_assert_int_eq(dgram_sock1.stats.read_events, 1);
	ck_assert_int_eq(dgram_sock1.stats.write_events, 1);
	ck_assert_int_eq(dgram_sock1.stats.read_bytes, 4096);
	ck_assert_int_eq(dgram_sock1.stats.write_bytes, 0);

	ck_assert_int_eq(dgram_sock2.stats.read_events, 0);
	ck_assert_int_eq(dgram_sock2.stats.write_events, 1);
	ck_assert_int_eq(dgram_sock2.stats.read_bytes, 0);
	ck_assert_int_eq(dgram_sock2.stats.write_bytes, 4096);

	ft_dgram_fini(&dgram_sock1);
	ft_dgram_fini(&dgram_sock2);

	ft_context_fini(&context);

	unsigned char digest2[EVP_MAX_MD_SIZE];
	EVP_DigestFinal(sock_dgram_1_mdctx, digest2, NULL);
    EVP_MD_CTX_cleanup(sock_dgram_1_mdctx);
    ck_assert_int_eq(memcmp(digest1, digest2, EVP_MD_size(EVP_sha256())), 0);

	ck_assert_int_eq(sock_dgram_1_result_counter, 1);

	ck_assert_int_eq(ft_log_stats.warn_count, 4);
	ck_assert_int_eq(ft_log_stats.error_count, 1);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

Suite * sock_dgram_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_dgram");

	tc = tcase_create("sock_dgram-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_dgram_1_utest);
	tcase_add_test(tc, sock_dgram_2_utest);

	return s;
}
