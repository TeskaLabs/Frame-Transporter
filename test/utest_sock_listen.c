#include <libsccmn.h>
#include <check.h>

////

static bool resolve(struct addrinfo **res, const char * host)
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

	int rc = getaddrinfo(host, "12345", &hints, res);
	if (rc != 0)
	{
		*res = NULL;
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
		return false;
    }

    return true;
}

////

bool sock_listen_utest_cb(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	int flags = fcntl(fd, F_GETFL, 0);
	flags &= ~O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	char buffer[1024];
	int rc = read(fd, buffer, sizeof(buffer));
	if (rc < 0) L_WARN_ERRNO_P(errno, ">>>>> '%d'\n", rc);
	else if (rc > 0)
	{
		buffer[rc] = '\0';
		listening_socket->data = strdup(buffer);
	}

	ev_break(listening_socket->context->ev_loop, EVBREAK_ALL);
	
	return false;
}


START_TEST(sock_listen_single_utest)
{
	struct listening_socket sock;
	int rc;

	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = listening_socket_init(&sock, &context, rp, sock_listen_utest_cb);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ok = listening_socket_start(&sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_stop(&sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_start(&sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_stop(&sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_stop(&sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_start(&sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_start(&sock);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	ck_assert_int_eq(rc, 0);

	ck_assert_str_eq(sock.data, "1234\n");

	ok = listening_socket_stop(&sock);
	ck_assert_int_eq(ok, true);

	listening_socket_close(&sock);

	context_fini(&context);
}
END_TEST


START_TEST(sock_listen_chain_utest)
{
	int rc;
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "localhost");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	struct listening_socket_chain * chain = NULL;
	rc = listening_socket_chain_extend(&chain, &context, rp, sock_listen_utest_cb);
	ck_assert_int_gt(rc, 0);

	freeaddrinfo(rp);

	rp = NULL;
	ok = resolve(&rp, NULL);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	rc = listening_socket_chain_extend(&chain, &context, rp, sock_listen_utest_cb);
	ck_assert_int_gt(rc, 0);

	freeaddrinfo(rp);


	listening_socket_chain_start(chain);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	ck_assert_int_eq(rc, 0);

	listening_socket_chain_stop(chain);

	bool found = false;
	for (struct listening_socket_chain * i = chain; i != NULL; i = i->next)
	{
		if (i->listening_socket.data == NULL) continue;
		ck_assert_str_eq(i->listening_socket.data, "1234\n");
		found = true;
	}
	ck_assert_int_eq(found, true);

	listening_socket_chain_del(chain);

	context_fini(&context);
}
END_TEST


START_TEST(sock_listen_null_chain_utest)
{
	struct listening_socket_chain * chain = NULL;

	listening_socket_chain_start(chain);
	listening_socket_chain_stop(chain);
	listening_socket_chain_del(chain);
}
END_TEST

START_TEST(sock_listen_chain_resolve_utest)
{
	int rc;
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);


	struct listening_socket_chain * chain = NULL;
	rc = listening_socket_chain_extend_getaddrinfo(&chain, &context, AF_INET, SOCK_STREAM, "127.0.0.1", "12345", sock_listen_utest_cb);
	ck_assert_int_gt(rc, 0);

	rc = listening_socket_chain_extend_getaddrinfo(&chain, &context, AF_INET, SOCK_STREAM, NULL, "12345", sock_listen_utest_cb);
	ck_assert_int_gt(rc, 0);

	listening_socket_chain_start(chain);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	ck_assert_int_eq(rc, 0);

	listening_socket_chain_stop(chain);

	bool found = false;
	for (struct listening_socket_chain * i = chain; i != NULL; i = i->next)
	{
		if (i->listening_socket.data == NULL) continue;
		ck_assert_str_eq(i->listening_socket.data, "1234\n");
		found = true;
	}
	ck_assert_int_eq(found, true);

	listening_socket_chain_del(chain);

	context_fini(&context);
}
END_TEST

///

Suite * sock_listen_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_listen");

	tc = tcase_create("sock_listen-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_listen_single_utest);
	tcase_add_test(tc, sock_listen_chain_utest);
	tcase_add_test(tc, sock_listen_null_chain_utest);
	tcase_add_test(tc, sock_listen_chain_resolve_utest);

	return s;
}
