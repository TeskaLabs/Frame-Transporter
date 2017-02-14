#include "testutils.h"

//TODO: unit test of flag.running
//TODO: unit test of started_at
//TODO: unit test of shutdown_at
//TODO: unit test of forceful shutdown

///

START_TEST(ft_context_empty_utest)
{
	bool ok;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	// Will immediately stop since there is nothing to do (no watcher is active)
	ft_context_run(&context);

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

Suite * ft_context_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("ft_context");

	tc = tcase_create("context_evloop");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_context_empty_utest);

	return s;
}
