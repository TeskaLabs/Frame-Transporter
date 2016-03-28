#include <libsccmn.h>
#include <check.h>

////

static bool resolve(struct addrinfo **res)
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

	int rc = getaddrinfo("127.0.0.1", "12345", &hints, res);
	if (rc != 0)
	{
		*res = NULL;
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
		return false;
    }

    return true;
}

////

void sock_listen_utest_cb(struct ev_loop * loop, struct listening_socket * listening_socket, int fd, struct sockaddr_storage * client_addr, socklen_t client_addr_len)
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
	close(fd);
	ev_break (loop, EVBREAK_ALL);
}


START_TEST(sock_listen_utest)
{
	struct listening_socket sock;
	int rc;

	struct addrinfo * rp = NULL;
	bool ok = resolve(&rp);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = listening_socket_init(&sock, rp, sock_listen_utest_cb, 10);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	struct ev_loop * loop = ev_default_loop(0);
	ck_assert_ptr_ne(loop, NULL);

	ok = listening_socket_start(loop, &sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_stop(loop, &sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_start(loop, &sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_stop(loop, &sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_stop(loop, &sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_start(loop, &sock);
	ck_assert_int_eq(ok, true);

	ok = listening_socket_start(loop, &sock);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	rc = ev_run(loop, 0);

	rc = pclose(p);
	ck_assert_int_eq(rc, 0);

	ck_assert_str_eq(sock.data, "1234\n");

}
END_TEST

///

Suite * sock_listen_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_listen");

	tc = tcase_create("sock_listen-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_listen_utest);

	return s;
}
