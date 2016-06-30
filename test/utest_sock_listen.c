#include "testutils.h"

////

bool sock_listen_utest_accept_cb(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
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

struct listening_socket_cb sock_listen_cb = 
{
	.accept = sock_listen_utest_accept_cb,
};



START_TEST(sock_listen_single_utest)
{
	struct listening_socket sock;
	int rc;

	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = listening_socket_init(&sock, &sock_listen_cb, &context, rp);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(sock.stats.accept_events, 0);

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

	ck_assert_int_eq(sock.stats.accept_events, 1);

	listening_socket_fini(&sock);

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

	return s;
}
