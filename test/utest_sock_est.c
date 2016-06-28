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

///

// This test is about incoming stream that 'hangs open' after all data are uploaded

struct frame * sock_est_1_on_get_read_frame(struct established_socket * established_sock)
{
	struct frame_dvec * dvec;

	struct frame * frame = frame_pool_borrow(&established_sock->context->frame_pool, frame_type_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);
	ck_assert_int_eq(frame->dvec_position, 0);
	ck_assert_int_eq(frame->dvec_limit, 0);

	dvec = frame_add_dvec(frame, 0, 5);
	ck_assert_ptr_ne(dvec, NULL);
	ck_assert_int_eq(dvec->position, 0);
	ck_assert_int_eq(dvec->limit, 5);
	ck_assert_int_eq(dvec->capacity, 5);

	ck_assert_int_eq(frame->dvec_position, 0);
	ck_assert_int_eq(frame->dvec_limit, 1);

	dvec = frame_add_dvec(frame, 5, 6);
	ck_assert_ptr_ne(dvec, NULL);
	ck_assert_int_eq(dvec->position, 0);
	ck_assert_int_eq(dvec->limit, 6);
	ck_assert_int_eq(dvec->capacity, 6);

	ck_assert_int_eq(frame->dvec_position, 0);
	ck_assert_int_eq(frame->dvec_limit, 2);

	return frame;
}

bool sock_est_1_on_read(struct established_socket * established_sock, struct frame * frame)
{
	ck_assert_int_ne(frame->type, frame_type_FREE);

	if (frame->type == frame_type_RAW_DATA)
	{
		ck_assert_int_eq(established_sock->syserror, 0);

		ck_assert_int_eq(memcmp(frame->data, "1234\nABCDE\n", 11), 0);
		frame_pool_return(frame);

		established_socket_shutdown(established_sock);
		return true;
	}


	if (frame->type == frame_type_STREAM_END)
	{
		ck_assert_int_eq(established_sock->syserror, 0);
		ck_assert_int_eq(established_sock->stats.read_bytes, 11);

		established_socket_shutdown(established_sock);
		return false;
	}

	ck_abort();
	return false;
}

void sock_est_1_on_state_changed(struct established_socket * established_sock)
{
	if (established_sock->flags.read_shutdown == true)
		ev_break(established_sock->context->ev_loop, EVBREAK_ALL);
}

struct established_socket_cb sock_est_1_sock_cb = 
{
	.get_read_frame = sock_est_1_on_get_read_frame,
	.read = sock_est_1_on_read,
	.state_changed = sock_est_1_on_state_changed,
	.error = NULL
};

bool sock_est_1_on_accept(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	ok = established_socket_init_accept(&established_sock, &sock_est_1_sock_cb, listening_socket, fd, client_addr, client_addr_len);
	ck_assert_int_eq(ok, true);

	ok = established_socket_read_start(&established_sock);
	ck_assert_int_eq(ok, true);

	return true;
}

struct listening_socket_cb sock_est_1_listen_cb = 
{
	.accept = sock_est_1_on_accept,
};


START_TEST(sock_est_1_utest)
{
	struct listening_socket listen_sock;
	int rc;
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = listening_socket_init(&listen_sock, &sock_est_1_listen_cb, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ok = listening_socket_start(&listen_sock);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	fprintf(p, "1234\nABCDE\n");
	fflush(p);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ok = listening_socket_stop(&listen_sock);
	ck_assert_int_eq(ok, true);

	listening_socket_fini(&listen_sock);

	//TODO: Temporary
	established_socket_fini(&established_sock);

	context_fini(&context);
}
END_TEST

///

// This test is about incoming stream that terminates after all data are uploaded

bool sock_est_2_on_read(struct established_socket * established_sock, struct frame * frame)
{
	ck_assert_int_ne(frame->type, frame_type_FREE);

	if (frame->type == frame_type_RAW_DATA)
	{
		ck_assert_int_eq(established_sock->syserror, 0);

		return false;
	}


	if (frame->type == frame_type_STREAM_END)
	{
		ck_assert_int_eq(established_sock->syserror, 0);

		established_socket_shutdown(established_sock);
		return false;
	}

	ck_abort();
	return false;
}

void sock_est_2_on_state_changed(struct established_socket * established_sock)
{
	if (established_sock->flags.read_shutdown == true)
		ev_break(established_sock->context->ev_loop, EVBREAK_ALL);
}

struct established_socket_cb sock_est_2_sock_cb = 
{
	.get_read_frame = get_read_frame_simple,
	.read = sock_est_2_on_read,
	.state_changed = sock_est_2_on_state_changed,
	.error = NULL
};

bool sock_est_2_on_accept(struct listening_socket * listening_socket, int fd, const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	bool ok;

	ok = established_socket_init_accept(&established_sock, &sock_est_2_sock_cb, listening_socket, fd, client_addr, client_addr_len);
	ck_assert_int_eq(ok, true);

	established_sock.read_opportunistic = true;

	ok = established_socket_read_start(&established_sock);
	ck_assert_int_eq(ok, true);

	return true;
}

struct listening_socket_cb sock_est_2_listen_cb = 
{
	.accept = sock_est_2_on_accept,
};


START_TEST(sock_est_2_utest)
{
	struct listening_socket listen_sock;
	int rc;
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct addrinfo * rp = NULL;
	ok = resolve(&rp, "127.0.0.1", "12345");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(rp, NULL);

	ok = listening_socket_init(&listen_sock, &sock_est_2_listen_cb, &context, rp);
	ck_assert_int_eq(ok, true);

	freeaddrinfo(rp);

	ok = listening_socket_start(&listen_sock);
	ck_assert_int_eq(ok, true);

	FILE * p = popen("cat /etc/hosts | nc localhost 12345", "w");
	ck_assert_ptr_ne(p, NULL);

	ev_run(context.ev_loop, 0);

	rc = pclose(p);
	if ((rc == -1) && (errno == ECHILD)) rc = 0; // Override too quick execution error
	ck_assert_int_eq(rc, 0);

	ok = listening_socket_stop(&listen_sock);
	ck_assert_int_eq(ok, true);

	listening_socket_fini(&listen_sock);

	//TODO: Temporary
	established_socket_fini(&established_sock);

	context_fini(&context);
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
	tcase_add_test(tc, sock_est_2_utest);

	return s;
}
