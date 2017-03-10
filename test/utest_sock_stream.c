#include "testutils.h"

////

struct ft_stream established_sock;
int sock_stream_1_result_counter;

///

// This test is about incoming stream that 'hangs open' after all data are uploaded

struct ft_frame * sock_stream_1_on_get_read_frame(struct ft_stream * established_sock)
{
	struct ft_vec * dvec;

	struct ft_frame * frame = ft_pool_borrow(&established_sock->base.socket.context->frame_pool, FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);
	ck_assert_int_eq(frame->vec_position, 0);
	ck_assert_int_eq(frame->vec_limit, 0);

	dvec = ft_frame_create_vec(frame, 0, 5);
	ck_assert_ptr_ne(dvec, NULL);
	ck_assert_int_eq(dvec->position, 0);
	ck_assert_int_eq(dvec->limit, 5);
	ck_assert_int_eq(dvec->capacity, 5);

	ck_assert_int_eq(frame->vec_position, 0);
	ck_assert_int_eq(frame->vec_limit, 1);

	dvec = ft_frame_create_vec(frame, 5, 6);
	ck_assert_ptr_ne(dvec, NULL);
	ck_assert_int_eq(dvec->position, 0);
	ck_assert_int_eq(dvec->limit, 6);
	ck_assert_int_eq(dvec->capacity, 6);

	ck_assert_int_eq(frame->vec_position, 0);
	ck_assert_int_eq(frame->vec_limit, 2);

	return frame;
}

bool sock_stream_1_on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	bool ok;
	ck_assert_int_ne(frame->type, FT_FRAME_TYPE_FREE);

	if (frame->type == FT_FRAME_TYPE_RAW_DATA)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);

		ck_assert_int_eq(memcmp(frame->data, "1234\nABCDE\n", 11), 0);
		ft_frame_return(frame);

		sock_stream_1_result_counter += 1;

		ok = ft_stream_cntl(established_sock, FT_STREAM_WRITE_SHUTDOWN);
		ck_assert_int_eq(ok, true);

		return true;
	}


	if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);
		ck_assert_int_eq(established_sock->stats.read_bytes, 11);

		ok = ft_stream_cntl(established_sock, FT_STREAM_WRITE_SHUTDOWN);
		ck_assert_int_eq(ok, true);

		return false;
	}

	ck_abort();
	return false;
}

struct ft_stream_delegate sock_stream_1_sock_delegate = 
{
	.get_read_frame = sock_stream_1_on_get_read_frame,
	.read = sock_stream_1_on_read,
	.error = NULL
};

bool sock_stream_1_on_accept(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	ok = ft_stream_accept(&established_sock, &sock_stream_1_sock_delegate, listening_socket, fd, client_addr, client_addr_len);
	ck_assert_int_eq(ok, true);

	ft_listener_cntl(listening_socket, FT_LISTENER_STOP);

	return true;
}

struct ft_listener_delegate utest_1_listener_delegate = 
{
	.accept = sock_stream_1_on_accept,
};


START_TEST(sock_stream_1_utest)
{
	struct ft_listener listen_sock;
	int rc;
	bool ok;

	sock_stream_1_result_counter = 0;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);


	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_listener_init(&listen_sock, &utest_1_listener_delegate, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ok = ft_listener_cntl(&listen_sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\nABCDE\n");
	fflush(p);

	ft_context_run(&context);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ok = ft_listener_cntl(&listen_sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ft_listener_fini(&listen_sock);

	//TODO: Temporary
	ft_stream_fini(&established_sock);

	ft_context_fini(&context);

	ck_assert_int_eq(sock_stream_1_result_counter, 1);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

// This test is about incoming stream that terminates after all data are uploaded

bool sock_stream_2_on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	bool ok;

	ck_assert_int_ne(frame->type, FT_FRAME_TYPE_FREE);

	if (frame->type == FT_FRAME_TYPE_RAW_DATA)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);


		return false;
	}


	if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);


		ok = ft_stream_cntl(established_sock, FT_STREAM_WRITE_SHUTDOWN);
		ck_assert_int_eq(ok, true);

		return false;
	}

	ck_abort();
	return false;
}

struct ft_stream_delegate utest_2_listener_delegate = 
{
	.read = sock_stream_2_on_read,
};

bool sock_stream_2_on_accept(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	ok = ft_stream_accept(&established_sock, &utest_2_listener_delegate, listening_socket, fd, client_addr, client_addr_len);
	ck_assert_int_eq(ok, true);

	ft_stream_set_partial(&established_sock, true);

	// Stop listening
	ok = ft_listener_cntl(listening_socket, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	return true;
}

struct ft_listener_delegate sock_stream_2_listen_cb = 
{
	.accept = sock_stream_2_on_accept,
};


START_TEST(sock_stream_2_utest)
{
	struct ft_listener listen_sock;
	int rc;
	bool ok;

	//ft_config.log_verbose = true;
	//ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_listener_init(&listen_sock, &sock_stream_2_listen_cb, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ok = ft_listener_cntl(&listen_sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("cat /etc/hosts | nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	ft_context_run(&context);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ok = ft_listener_cntl(&listen_sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ft_listener_fini(&listen_sock);

	//TODO: Temporary
	ft_stream_fini(&established_sock);

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 1);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

void sock_stream_conn_fail_on_error(struct ft_stream * established_sock)
{
	printf("ERROR - NICE!\n");
}

bool sock_stream_conn_fail_on_read(struct ft_stream * stream, struct ft_frame * frame)
{
	return false;
}

struct ft_stream_delegate sock_stream_conn_fail_delegate = 
{
	.connected = NULL,
	.get_read_frame = NULL,
	.read = sock_stream_conn_fail_on_read,
	.error = sock_stream_conn_fail_on_error,
};

START_TEST(sock_stream_conn_fail_utest)
{
	struct ft_stream sock;
	bool ok;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12346", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_stream_connect(&sock, &sock_stream_conn_fail_delegate, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);

	ft_frame_format_simple(frame);
	struct ft_vec * dvec = ft_frame_get_vec(frame);
	ck_assert_ptr_ne(dvec, NULL);	

	ok = ft_vec_sprintf(dvec, "Virtually anything, will be lost anyway");

	ft_frame_flip(frame);

	ok = ft_stream_write(&sock, frame);
	ck_assert_int_eq(ok, true);

	ok = ft_stream_cntl(&sock, FT_STREAM_WRITE_SHUTDOWN);
	ck_assert_int_eq(ok, true);

	ft_context_run(&context);

	ft_stream_fini(&sock);

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 2);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

ssize_t sock_stream_ssl_1_read_counter;
EVP_MD_CTX * sock_stream_ssl_1_mdctx;

void sock_stream_ssl_1_on_connected(struct ft_stream * this)
{
	ck_assert_int_eq(this->flags.ssl_status, 2);
	ck_assert_int_eq(this->flags.connecting, false);	
}

bool sock_stream_ssl_1_on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	ck_assert_int_ne(frame->type, FT_FRAME_TYPE_FREE);

	if (frame->type == FT_FRAME_TYPE_RAW_DATA)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);


		ft_frame_flip(frame);
		ck_assert_int_gt(ft_frame_len(frame), 0);
		//ft_frame_fwrite(stdout, frame);

		for (struct ft_vec * dvec = ft_frame_get_vec(frame); dvec != NULL; dvec = ft_frame_next_vec(frame))
		{
			int rc = EVP_DigestUpdate(sock_stream_ssl_1_mdctx, ft_vec_ptr(dvec), ft_vec_len(dvec));
			ck_assert_int_eq(rc, 1);
		}

		sock_stream_ssl_1_read_counter += ft_frame_len(frame);
	}

	ft_frame_return(frame);
	return true;
}

void sock_stream_ssl_1_on_error(struct ft_stream * established_sock)
{
	ck_assert_int_eq(0,1);
}


static int sock_stream_ssl_client_verify_callback(X509_STORE_CTX *ctx, void *arg)
{
	struct ft_stream * this = ft_stream_from_x509_store_ctx(ctx);
	ck_assert_ptr_ne(this, NULL);

    return 1;
}


struct ft_stream_delegate sock_stream_ssl_1_delegate = 
{
	.connected = sock_stream_ssl_1_on_connected,
	.read = sock_stream_ssl_1_on_read,
	.error = sock_stream_ssl_1_on_error,
};


START_TEST(sock_stream_ssl_client_utest)
{
	struct ft_stream sock;
	bool ok;
	int rc;

	generate_random_file("./sock_stream_ssl_1_utest.bin", 4096, 100);

	EVP_MD_CTX * mdctx = EVP_MD_CTX_create();
	ck_assert_ptr_ne(mdctx, NULL);

	rc = EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL);
	ck_assert_int_eq(rc, 1);

	digest_file(mdctx, "./sock_stream_ssl_1_utest.bin");

	unsigned char digest1[EVP_MAX_MD_SIZE];
	EVP_DigestFinal(mdctx, digest1, NULL);
    EVP_MD_CTX_cleanup(mdctx);
	
	sock_stream_ssl_1_mdctx = EVP_MD_CTX_create();
	ck_assert_ptr_ne(sock_stream_ssl_1_mdctx, NULL);
	
	rc = EVP_DigestInit_ex(sock_stream_ssl_1_mdctx, EVP_sha256(), NULL);
	ck_assert_int_eq(rc, 1);

	ft_initialise();

//	ft_config.log_verbose = true;
//	ft_config.log_trace_mask |= FT_TRACE_ID_STREAM | FT_TRACE_ID_EVENT_LOOP;

	sock_stream_ssl_1_read_counter = 0;

	SSL_CTX * ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
	ck_assert_ptr_ne(ssl_ctx, NULL);

	SSL_CTX_set_cert_verify_callback(ssl_ctx, sock_stream_ssl_client_verify_callback, NULL);

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	int inf = 0, outf = 0;
	pid_t pid = popen2("openssl s_server -quiet -accept 12345 -tls1_2 -key ./ssl/key.pem -cert ./ssl/cert.pem -msg -HTTP", &inf, &outf);
	ck_assert_int_ge(pid, 0);
	ck_assert_int_ge(inf, 0);
	ck_assert_int_ge(outf, 0);
	usleep(60000);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_stream_connect(&sock, &sock_stream_ssl_1_delegate, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ft_stream_set_partial(&sock, true);

	ok = ft_stream_enable_ssl(&sock, ssl_ctx);
	ck_assert_int_eq(ok, true);


	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);

	ft_frame_format_simple(frame);
	struct ft_vec * dvec = ft_frame_get_vec(frame);
	ck_assert_ptr_ne(dvec, NULL);	

	ok = ft_vec_sprintf(dvec, "GET /sock_stream_ssl_1_utest.bin HTTP/1.0\r\n\r\n");
	ck_assert_int_eq(ok, true);

	ft_frame_flip(frame);

	ok = ft_stream_write(&sock, frame);
	ck_assert_int_eq(ok, true);

	ft_context_run(&context);

	FT_INFO("Stats: Re:%u We:%u+%u Rb:%lu Wb:%lu",
		sock.stats.read_events,
		sock.stats.write_events,
		sock.stats.write_direct,
		sock.stats.read_bytes,
		sock.stats.write_bytes
	);

	ck_assert_int_eq(sock.stats.read_bytes, 4096 * 100);

	unsigned char digest2[EVP_MAX_MD_SIZE];
	EVP_DigestFinal(sock_stream_ssl_1_mdctx, digest2, NULL);
    EVP_MD_CTX_cleanup(sock_stream_ssl_1_mdctx);
    ck_assert_int_eq(memcmp(digest1, digest2, EVP_MD_size(EVP_sha256())), 0);

	ck_assert_int_eq(sock_stream_ssl_1_read_counter, 4096 * 100);

	rc = kill(pid, SIGINT);
	ck_assert_int_eq(rc, 0);
	char buffer[128*1024];
	ssize_t x = read(outf, buffer, sizeof(buffer));
	ck_assert_int_gt(x, 0);
	close(inf);
	close(outf);

	ft_stream_fini(&sock);

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

SSL_CTX * sock_stream_ssl_server_ssl_ctx = NULL;
int sock_stream_ssl_server_utest_result_counter;

bool sock_stream_ssl_server_on_read(struct ft_stream * established_sock, struct ft_frame * frame)
{
	bool ok;
	ck_assert_int_ne(frame->type, FT_FRAME_TYPE_FREE);

	if (frame->type == FT_FRAME_TYPE_RAW_DATA)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);


		ck_assert_int_eq(memcmp(frame->data, "1234\nABCDE\n", 11), 0);
		ft_frame_return(frame);

		sock_stream_ssl_server_utest_result_counter += 1;

		ok = ft_stream_cntl(established_sock, FT_STREAM_WRITE_SHUTDOWN);
		ck_assert_int_eq(ok, true);

		return true;
	}


	if (frame->type == FT_FRAME_TYPE_END_OF_STREAM)
	{
		ck_assert_int_eq(established_sock->error.sys_errno, 0);
		ck_assert_int_eq(established_sock->error.ssl_error, 0);

		ck_assert_int_eq(established_sock->stats.read_bytes, 11);

		ok = ft_stream_cntl(established_sock, FT_STREAM_WRITE_SHUTDOWN);
		ck_assert_int_eq(ok, true);

		return false;
	}

	ck_abort();
	return false;
}


static int sock_stream_ssl_server_listen_verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
	struct ft_stream * this = ft_stream_from_x509_store_ctx(ctx);
	ck_assert_ptr_ne(this, NULL);

    sock_stream_ssl_server_utest_result_counter += 1;

    return preverify_ok;
}


struct ft_stream_delegate sock_stream_ssl_server_sock_delegate = 
{
	.read = sock_stream_ssl_server_on_read,
	.error = NULL
};


bool sock_stream_ssl_server_listen_on_accept(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	ok = ft_stream_accept(&established_sock, &sock_stream_ssl_server_sock_delegate, listening_socket, fd, client_addr, client_addr_len);
	ck_assert_int_eq(ok, true);

	ft_stream_set_partial(&established_sock, true);

	// Initiate SSL
	ok = ft_stream_enable_ssl(&established_sock, sock_stream_ssl_server_ssl_ctx);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(listening_socket, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	return true;
}

struct ft_listener_delegate utest_ssl_server_listener_delegate = 
{
	.accept = sock_stream_ssl_server_listen_on_accept,
};


START_TEST(sock_stream_ssl_server_utest)
{
	struct ft_listener listen_sock;
	int rc;
	bool ok;

	sock_stream_ssl_server_utest_result_counter = 0;

	ft_initialise();

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	// Initialize OpenSSL context
	sock_stream_ssl_server_ssl_ctx = SSL_CTX_new(SSLv23_server_method());
	ck_assert_ptr_ne(sock_stream_ssl_server_ssl_ctx, NULL);

	long ssloptions = SSL_OP_ALL | SSL_OP_NO_SESSION_RESUMPTION_ON_RENEGOTIATION | SSL_OP_NO_COMPRESSION
	| SSL_OP_CIPHER_SERVER_PREFERENCE | SSL_OP_SINGLE_DH_USE;
	SSL_CTX_set_options(sock_stream_ssl_server_ssl_ctx, ssloptions);

	rc = SSL_CTX_use_PrivateKey_file(sock_stream_ssl_server_ssl_ctx, "./ssl/key.pem", SSL_FILETYPE_PEM);
	ck_assert_int_eq(rc, 1);

	rc = SSL_CTX_use_certificate_file(sock_stream_ssl_server_ssl_ctx, "./ssl/cert.pem", SSL_FILETYPE_PEM);
	ck_assert_int_eq(rc, 1);

	rc = SSL_CTX_load_verify_locations(sock_stream_ssl_server_ssl_ctx, "./ssl/cert.pem", NULL);
	ck_assert_int_eq(rc, 1);

	SSL_CTX_set_verify(sock_stream_ssl_server_ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, sock_stream_ssl_server_listen_verify_callback);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_listener_init(&listen_sock, &utest_ssl_server_listener_delegate, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ok = ft_listener_cntl(&listen_sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("openssl s_client -quiet -key ./ssl/key.pem -cert ./ssl/cert.pem -connect localhost:12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\nABCDE\n");
	fflush(p);

	ft_context_run(&context);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ok = ft_listener_cntl(&listen_sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ft_listener_fini(&listen_sock);

	ft_stream_fini(&established_sock);

	ft_context_fini(&context);

	ck_assert_int_eq(sock_stream_ssl_server_utest_result_counter, 2);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);

}
END_TEST

///

Suite * sock_stream_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_stream");

	tc = tcase_create("sock_stream-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_stream_1_utest);
	tcase_add_test(tc, sock_stream_2_utest);
	tcase_add_test(tc, sock_stream_conn_fail_utest);

	tc = tcase_create("sock_stream-ssl");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_stream_ssl_client_utest);
	tcase_add_test(tc, sock_stream_ssl_server_utest);

	return s;
}
