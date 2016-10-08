#include "testutils.h"

////

START_TEST(config_init_core_utest)
{
	ck_assert_int_eq(ft_config.initialized, false);
	ft_initialise();
	ck_assert_int_eq(ft_config.initialized, true);


	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

Suite * config_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("config");

	tc = tcase_create("config-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, config_init_core_utest);

	return s;
}
