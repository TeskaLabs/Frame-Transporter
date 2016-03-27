#include <check.h>

#include <libsccmn.h>

////

START_TEST(log_normal_info_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_INFO("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_INFO_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

START_TEST(log_normal_warn_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_WARN("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_WARN_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST


START_TEST(log_normal_error_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_ERROR("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_ERROR_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

START_TEST(log_normal_fatal_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_FATAL("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_FATAL_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

START_TEST(log_normal_audit_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_AUDIT("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_AUDIT_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

//

START_TEST(log_errno_info_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_INFO_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_INFO_ERRNO_P(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

START_TEST(log_errno_warn_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_WARN_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_WARN_ERRNO_P(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

START_TEST(log_errno_error_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_ERROR_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_ERROR_ERRNO_P(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

START_TEST(log_errno_fatal_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_FATAL_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_FATAL_ERRNO_P(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST


START_TEST(log_errno_audit_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	L_AUDIT_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_AUDIT_ERRNO_P(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

///

Suite * log_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("log");

	tc = tcase_create("log-normal");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, log_normal_info_utest);
	tcase_add_test(tc, log_normal_warn_utest);
	tcase_add_test(tc, log_normal_error_utest);
	tcase_add_test(tc, log_normal_fatal_utest);
	tcase_add_test(tc, log_normal_audit_utest);

	tc = tcase_create("log-errno");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, log_errno_info_utest);
	tcase_add_test(tc, log_errno_warn_utest);
	tcase_add_test(tc, log_errno_error_utest);
	tcase_add_test(tc, log_errno_fatal_utest);
	tcase_add_test(tc, log_errno_audit_utest);

	return s;
}
