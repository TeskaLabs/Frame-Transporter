#include "testutils.h"

////

START_TEST(ft_fd_nonblock_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = ft_fd_nonblock(fd, true);
	ck_assert_int_eq(ret, true);

	close(fd);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST


START_TEST(ft_socket_keepalive_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = ft_socket_keepalive(fd);
	ck_assert_int_eq(ret, true);

	close(fd);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST


START_TEST(ft_fd_cloexec_utest)
{
	int fd=socket(PF_INET, SOCK_STREAM, 0);
	ck_assert_int_ne(fd, -1);

	bool ret = ft_fd_cloexec(fd);
	ck_assert_int_eq(ret, true);

	close(fd);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
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
