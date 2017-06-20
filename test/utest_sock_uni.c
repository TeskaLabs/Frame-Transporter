#include "testutils.h"

///

bool sock_uni_1_dgram_utest_on_read(struct ft_dgram * stream, struct ft_frame * frame)
{
	return false;
}

struct ft_dgram_delegate sock_uni_1_ft_dgram_delegate = 
{
	.read = sock_uni_1_dgram_utest_on_read
};


bool sock_uni_1_stream_utest_on_read(struct ft_stream * stream, struct ft_frame * frame)
{
	return false;
}

struct ft_stream_delegate sock_uni_1_ft_stream_delegate =
{
	.read = sock_uni_1_stream_utest_on_read
};


START_TEST(sock_uni_1_utest)
{
	bool ok;
	int rc;

	union ft_uni_socket unisock;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	ok = ft_dgram_init(&unisock.dgram, &sock_uni_1_ft_dgram_delegate, &context, AF_INET, SOCK_DGRAM, 0);
	ck_assert_int_eq(ok, true);

	ft_uni_socket_fini(&unisock);


	int socket_vector[2];
	rc = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_vector);
	ck_assert_int_eq(rc, 0);


	ok = ft_stream_init(&unisock.stream, &sock_uni_1_ft_stream_delegate, &context, socket_vector[0]);
	ck_assert_int_eq(ok, true);

	ft_uni_socket_fini(&unisock);

	close(socket_vector[1]);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

Suite * sock_uni_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("sock_uni");

	tc = tcase_create("sock_uni-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, sock_uni_1_utest);

	return s;
}
