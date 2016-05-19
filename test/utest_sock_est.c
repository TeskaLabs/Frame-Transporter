#include <libsccmn.h>
#include <check.h>

////

static bool resolve(struct addrinfo **res, const char * host, const char * port)
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

////

struct established_socket established_sock;
struct frame_pool fpool;

size_t sock_est_1_on_read_advise(struct established_socket * established_sock, struct frame * frame)
{
	return 5;
}

bool sock_est_1_on_read(struct established_socket * established_sock, struct ev_loop * loop, struct frame * frame)
{
	ck_assert_int_ne(frame->type, frame_type_FREE);

	size_t cap = frame_total_position(frame);
	if (cap < 5) return false;

	ck_assert_int_eq(memcmp(frame->data, "1234\n", 5), 0);

	frame_pool_return(frame);

	bool ok = established_socket_shutdown(loop, established_sock);
	ck_assert_int_eq(ok, true);

	return true;
}

void sock_est_1_on_close(struct established_socket * established_sock, struct ev_loop * loop)
{
	ev_break(loop, EVBREAK_ALL);
}

struct established_socket_cb sock_est_1_sock_cb = 
{
	.read_advise = sock_est_1_on_read_advise,
	.read = sock_est_1_on_read,
	.close = sock_est_1_on_close
};

void sock_est_1_on_accept(struct ev_loop * loop, struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	ok = established_socket_init(&established_sock, &sock_est_1_sock_cb, &fpool, loop, listening_socket, fd, client_addr, client_addr_len);
	ck_assert_int_eq(ok, true);

	ok = established_socket_read_start(loop, &established_sock);
	ck_assert_int_eq(ok, true);
}


START_TEST(sock_est_1_utest)
{
	struct listening_socket listen_sock;
	int rc;
	bool ok;

	struct heartbeat hb;
	heartbeat_init(&hb, 0.1);

	ok = frame_pool_init(&fpool, &hb, NULL);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = listening_socket_init(&listen_sock, rp, sock_est_1_on_accept, 10);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	struct ev_loop * loop = ev_default_loop(0);
	ck_assert_ptr_ne(loop, NULL);

	ok = listening_socket_start(loop, &listen_sock);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\n");
	fflush(p);

	ev_run(loop, 0);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ok = listening_socket_stop(loop, &listen_sock);
	ck_assert_int_eq(ok, true);

	listening_socket_close(&listen_sock);

	frame_pool_fini(&fpool, &hb);

	ev_loop_destroy(loop);
}
END_TEST

///

Suite * sock_est_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_est");

	tc = tcase_create("sock_est-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_est_1_utest);

	return s;
}
