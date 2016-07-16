#include "testutils.h"

////

START_TEST(ft_fd_nonblock_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = ft_fd_nonblock(fd);
	ck_assert_int_eq(ret, true);

	close(fd);
}
END_TEST


START_TEST(ft_socket_keepalive_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = ft_socket_keepalive(fd);
	ck_assert_int_eq(ret, true);

	close(fd);
}
END_TEST


START_TEST(ft_fd_cloexec_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = ft_fd_cloexec(fd);
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
	tcase_add_test(tc, ft_fd_nonblock_utest);
	tcase_add_test(tc, ft_socket_keepalive_utest);
	tcase_add_test(tc, ft_fd_cloexec_utest);

	return s;
}
