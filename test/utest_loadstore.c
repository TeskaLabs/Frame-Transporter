#include "testutils.h"

////

START_TEST(ft_skip_u8_utest)
{
}
END_TEST

///

Suite * ft_loadstore_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("loadstore");

	tc = tcase_create("loadstore-skip");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_skip_u8_utest);

	return s;
}
