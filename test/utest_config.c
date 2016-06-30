#include "testutils.h"

////

START_TEST(config_init_core_utest)
{
	ck_assert_int_eq(libsccmn_config.initialized, false);
	libsccmn_init();
	ck_assert_int_eq(libsccmn_config.initialized, true);
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
