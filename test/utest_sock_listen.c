#include "testutils.h"

////

bool sock_listen_utest_accept_cb(struct ft_listener * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	int flags = fcntl(fd, F_GETFL, 0);
	flags &= ~O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);

	char buffer[1024];
	int rc = read(fd, buffer, sizeof(buffer));
	if (rc < 0) FT_WARN_ERRNO_P(errno, ">>>>> '%d'\n", rc);
	else if (rc > 0)
	{
		buffer[rc] = '\0';
		listening_socket->base.socket.data = strdup(buffer);
	}

	ev_break(listening_socket->base.socket.context->ev_loop, EVBREAK_ALL);
	
	return false;
}

struct ft_listener_delegate listener_delegate = 
{
	.accept = sock_listen_utest_accept_cb,
};



START_TEST(sock_listen_single_utest)
{
	struct ft_listener sock;
	int rc;

	bool ok;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345", AF_INET, SOCK_STREAM, IPPROTO_TCP);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = ft_listener_init(&sock, &listener_delegate, &context, rp);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(sock.stats.accept_events, 0);

	freeaddrinfo(rp);

	ok = ft_listener_cntl(&sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(&sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(&sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(&sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(&sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(&sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_cntl(&sock, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	ck_assert_int_eq(rc, 0);

	ck_assert_str_eq(sock.base.socket.data, "1234\n");

	ok = ft_listener_cntl(&sock, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(sock.stats.accept_events, 1);

	ft_listener_fini(&sock);

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

START_TEST(sock_listen_list_utest)
{
	struct ft_list listeners;
	int rc;

	bool ok;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_init(&listeners);
	ck_assert_int_eq(ok, true);

	const char * listen_sockets_charv[] = {
		"/tmp/sock_listen_list_utest.sock",
		"127.0.0.1:12345",
		"localhost:12346",
		NULL
	};

	rc = ft_listener_list_extend_autov(&listeners, &listener_delegate, &context, SOCK_STREAM, listen_sockets_charv);
	ck_assert_int_gt(rc, 2); // Can be 3 or 4
	ck_assert_int_lt(rc, 5);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_START);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	ck_assert_int_eq(rc, 0);

	ok = ft_listener_list_cntl(&listeners, FT_LISTENER_STOP);
	ck_assert_int_eq(ok, true);

//	ck_assert_int_eq(sock.stats.accept_events, 1);

	ft_list_fini(&listeners);

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
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
	tcase_add_test(tc, sock_listen_list_utest);

	return s;
}
