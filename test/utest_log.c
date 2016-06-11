#include <libsccmn.h>
#include <check.h>

////

START_TEST(log_normal_debug_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	logging_set_verbose(true);
	L_DEBUG("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	L_DEBUG_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_set_verbose(false);
	L_DEBUG("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	L_DEBUG_P("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST


START_TEST(log_normal_info_utest)
{
	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	logging_set_verbose(true);
	L_INFO("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_WARN("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_ERROR("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_FATAL("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_AUDIT("Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_INFO_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_WARN_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_ERROR_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_FATAL_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
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

	logging_set_verbose(true);
	L_AUDIT_ERRNO(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	logging_set_verbose(false);
	L_AUDIT_ERRNO_P(EAGAIN, "Log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);
}
END_TEST

///

START_TEST(log_reopen_nofile_utest)
{
	ck_assert_ptr_eq(libsccmn_config.log_f, NULL);
	ck_assert_ptr_eq(libsccmn_config.log_filename, NULL);

	bool ok = logging_reopen();
	ck_assert_int_eq(ok, true);

	ck_assert_ptr_eq(libsccmn_config.log_f, NULL);
	ck_assert_ptr_eq(libsccmn_config.log_filename, NULL);
}
END_TEST


START_TEST(log_reopen_wfile_utest)
{
	ck_assert_ptr_eq(libsccmn_config.log_f, NULL);
	ck_assert_ptr_eq(libsccmn_config.log_filename, NULL);

	bool ok = logging_set_filename("./log.txt");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(libsccmn_config.log_f, NULL);
	ck_assert_ptr_ne(libsccmn_config.log_filename, NULL);

	L_INFO_ERRNO(EAGAIN, "Log message test!");

	ok = logging_reopen();
	ck_assert_int_eq(ok, true);

	ok = logging_set_filename(NULL);
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_eq(libsccmn_config.log_f, NULL);
	ck_assert_ptr_eq(libsccmn_config.log_filename, NULL);

	int rc = unlink("./log.txt");
	ck_assert_int_eq(rc, 0);
}
END_TEST


START_TEST(log_reopen_sighup_utest)
{
	ck_assert_ptr_eq(libsccmn_config.log_f, NULL);
	ck_assert_ptr_eq(libsccmn_config.log_filename, NULL);

	bool ok = logging_set_filename("./log.txt");
	ck_assert_int_eq(ok, true);
	ck_assert_ptr_ne(libsccmn_config.log_f, NULL);
	ck_assert_str_eq(libsccmn_config.log_filename, "./log.txt");

	L_INFO("Log message test!");

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	ev_feed_signal(SIGHUP);
	ev_run(context.ev_loop, EVBREAK_ONE);

	L_INFO("Log message test!");

	ev_feed_signal(SIGHUP);
	ev_run(context.ev_loop, EVBREAK_ONE);

	int rc = system("grep \"Log file is reopen\" ./log.txt");
	ck_assert_int_eq(rc, 0);

	rc = unlink("./log.txt");
	ck_assert_int_eq(rc, 0);
}
END_TEST

///

START_TEST(log_openssl_info_utest)
{
	unsigned long code;

	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,3,__FILE__,__LINE__);
	ERR_put_error(1,2,4,__FILE__,__LINE__);
	ERR_put_error(1,2,5,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002003);

	logging_set_verbose(true);
	L_INFO_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,6,__FILE__,__LINE__);
	ERR_put_error(1,2,7,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002006);

	logging_set_verbose(false);
	L_INFO_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	L_INFO_OPENSSL_P("OpenSSL log message test (no SSL error)!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 3);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

}
END_TEST

START_TEST(log_openssl_warn_utest)
{
	unsigned long code;

	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,3,__FILE__,__LINE__);
	ERR_put_error(1,2,4,__FILE__,__LINE__);
	ERR_put_error(1,2,5,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002003);

	logging_set_verbose(true);
	L_WARN_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,6,__FILE__,__LINE__);
	ERR_put_error(1,2,7,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002006);

	logging_set_verbose(false);
	L_WARN_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	L_WARN_OPENSSL_P("OpenSSL log message test (no SSL error)!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 3);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

}
END_TEST

START_TEST(log_openssl_error_utest)
{
	unsigned long code;

	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,3,__FILE__,__LINE__);
	ERR_put_error(1,2,4,__FILE__,__LINE__);
	ERR_put_error(1,2,5,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002003);

	logging_set_verbose(true);
	L_ERROR_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,6,__FILE__,__LINE__);
	ERR_put_error(1,2,7,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002006);

	logging_set_verbose(false);
	L_ERROR_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	L_ERROR_OPENSSL_P("OpenSSL log message test (no SSL error)!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 3);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

}
END_TEST

START_TEST(log_openssl_fatal_utest)
{
	unsigned long code;

	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,3,__FILE__,__LINE__);
	ERR_put_error(1,2,4,__FILE__,__LINE__);
	ERR_put_error(1,2,5,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002003);

	logging_set_verbose(true);
	L_FATAL_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,6,__FILE__,__LINE__);
	ERR_put_error(1,2,7,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002006);

	logging_set_verbose(false);
	L_FATAL_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	L_FATAL_OPENSSL_P("OpenSSL log message test (no SSL error)!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 3);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

}
END_TEST

START_TEST(log_openssl_audit_utest)
{
	unsigned long code;

	libsccmn_config.log_flush_counter_max = 1;
	logging_flush();
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 0);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,3,__FILE__,__LINE__);
	ERR_put_error(1,2,4,__FILE__,__LINE__);
	ERR_put_error(1,2,5,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002003);

	logging_set_verbose(true);
	L_AUDIT_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 1);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

	ERR_put_error(1,2,6,__FILE__,__LINE__);
	ERR_put_error(1,2,7,__FILE__,__LINE__);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0x1002006);

	logging_set_verbose(false);
	L_AUDIT_OPENSSL("OpenSSL log message test!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 2);

	L_AUDIT_OPENSSL_P("OpenSSL log message test (no SSL error)!");
	ck_assert_int_eq(libsccmn_config.log_flush_counter, 3);

	code = ERR_peek_error();
	ck_assert_int_eq(code, 0);

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
	tcase_add_test(tc, log_normal_debug_utest);
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

	tc = tcase_create("log-openssl");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, log_openssl_info_utest);
	tcase_add_test(tc, log_openssl_warn_utest);
	tcase_add_test(tc, log_openssl_error_utest);
	tcase_add_test(tc, log_openssl_fatal_utest);
	tcase_add_test(tc, log_openssl_audit_utest);

	tc = tcase_create("log-reopen");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, log_reopen_nofile_utest);
	tcase_add_test(tc, log_reopen_wfile_utest);
	tcase_add_test(tc, log_reopen_sighup_utest);

	return s;
}
