#include "testutils.h"

////

START_TEST(ft_parse_bool_01_utest)
{
	bool res = ft_parse_bool("0");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_02_utest)
{
	bool res = ft_parse_bool("1");
	ck_assert_int_eq(res, true);
}
END_TEST


START_TEST(ft_parse_bool_03_utest)
{
	bool res = ft_parse_bool("true");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(ft_parse_bool_04_utest)
{
	bool res = ft_parse_bool("false");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_05_utest)
{
	bool res = ft_parse_bool("True");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(ft_parse_bool_06_utest)
{
	bool res = ft_parse_bool("False");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_07_utest)
{
	bool res = ft_parse_bool("TRUE");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(ft_parse_bool_08_utest)
{
	bool res = ft_parse_bool("FALSE");
	ck_assert_int_eq(res, false);
}
END_TEST


START_TEST(ft_parse_bool_09_utest)
{
	bool res = ft_parse_bool("no");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_10_utest)
{
	bool res = ft_parse_bool("yes");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(ft_parse_bool_11_utest)
{
	bool res = ft_parse_bool("NO");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_12_utest)
{
	bool res = ft_parse_bool("YES");
	ck_assert_int_eq(res, true);
}
END_TEST


START_TEST(ft_parse_bool_13_utest)
{
	bool res = ft_parse_bool("yess");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_14_utest)
{
	bool res = ft_parse_bool("11");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_15_utest)
{
	bool res = ft_parse_bool(" yes");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(ft_parse_bool_16_utest)
{
	bool res = ft_parse_bool("yes ");
	ck_assert_int_eq(res, false);
}
END_TEST

///

Suite * boolean_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("boolean");

	tc = tcase_create("boolean-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_parse_bool_01_utest);
	tcase_add_test(tc, ft_parse_bool_02_utest);
	tcase_add_test(tc, ft_parse_bool_03_utest);
	tcase_add_test(tc, ft_parse_bool_04_utest);
	tcase_add_test(tc, ft_parse_bool_05_utest);
	tcase_add_test(tc, ft_parse_bool_06_utest);
	tcase_add_test(tc, ft_parse_bool_07_utest);
	tcase_add_test(tc, ft_parse_bool_08_utest);
	tcase_add_test(tc, ft_parse_bool_09_utest);
	tcase_add_test(tc, ft_parse_bool_10_utest);
	tcase_add_test(tc, ft_parse_bool_11_utest);
	tcase_add_test(tc, ft_parse_bool_12_utest);
	tcase_add_test(tc, ft_parse_bool_13_utest);
	tcase_add_test(tc, ft_parse_bool_14_utest);
	tcase_add_test(tc, ft_parse_bool_15_utest);
	tcase_add_test(tc, ft_parse_bool_16_utest);

	return s;
}
