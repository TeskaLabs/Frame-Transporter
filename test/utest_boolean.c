#include <check.h>

#include <libsccmn.h>

////

START_TEST(parse_boolean_01_utest)
{
	bool res = parse_boolean("0");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_02_utest)
{
	bool res = parse_boolean("1");
	ck_assert_int_eq(res, true);
}
END_TEST


START_TEST(parse_boolean_03_utest)
{
	bool res = parse_boolean("true");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(parse_boolean_04_utest)
{
	bool res = parse_boolean("false");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_05_utest)
{
	bool res = parse_boolean("True");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(parse_boolean_06_utest)
{
	bool res = parse_boolean("False");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_07_utest)
{
	bool res = parse_boolean("TRUE");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(parse_boolean_08_utest)
{
	bool res = parse_boolean("FALSE");
	ck_assert_int_eq(res, false);
}
END_TEST


START_TEST(parse_boolean_09_utest)
{
	bool res = parse_boolean("no");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_10_utest)
{
	bool res = parse_boolean("yes");
	ck_assert_int_eq(res, true);
}
END_TEST

START_TEST(parse_boolean_11_utest)
{
	bool res = parse_boolean("NO");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_12_utest)
{
	bool res = parse_boolean("YES");
	ck_assert_int_eq(res, true);
}
END_TEST


START_TEST(parse_boolean_13_utest)
{
	bool res = parse_boolean("yess");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_14_utest)
{
	bool res = parse_boolean("11");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_15_utest)
{
	bool res = parse_boolean(" yes");
	ck_assert_int_eq(res, false);
}
END_TEST

START_TEST(parse_boolean_16_utest)
{
	bool res = parse_boolean("yes ");
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
	tcase_add_test(tc, parse_boolean_01_utest);
	tcase_add_test(tc, parse_boolean_02_utest);
	tcase_add_test(tc, parse_boolean_03_utest);
	tcase_add_test(tc, parse_boolean_04_utest);
	tcase_add_test(tc, parse_boolean_05_utest);
	tcase_add_test(tc, parse_boolean_06_utest);
	tcase_add_test(tc, parse_boolean_07_utest);
	tcase_add_test(tc, parse_boolean_08_utest);
	tcase_add_test(tc, parse_boolean_09_utest);
	tcase_add_test(tc, parse_boolean_10_utest);
	tcase_add_test(tc, parse_boolean_11_utest);
	tcase_add_test(tc, parse_boolean_12_utest);
	tcase_add_test(tc, parse_boolean_13_utest);
	tcase_add_test(tc, parse_boolean_14_utest);
	tcase_add_test(tc, parse_boolean_15_utest);
	tcase_add_test(tc, parse_boolean_16_utest);

	return s;
}
