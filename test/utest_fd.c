#include "testutils.h"

////

START_TEST(set_socket_nonblocking_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = set_socket_nonblocking(fd);
	ck_assert_int_eq(ret, true);

	close(fd);
}
END_TEST


START_TEST(set_tcp_keepalive_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = set_tcp_keepalive(fd);
	ck_assert_int_eq(ret, true);

	close(fd);
}
END_TEST


START_TEST(set_close_on_exec_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = set_close_on_exec(fd);
	ck_assert_int_eq(ret, true);

	close(fd);
}
END_TEST

///

Suite * fd_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("fd");

	tc = tcase_create("core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, set_socket_nonblocking_utest);
	tcase_add_test(tc, set_tcp_keepalive_utest);
	tcase_add_test(tc, set_close_on_exec_utest);

	return s;
}
