#include <check.h>

#include <libsccmn.h>

////

START_TEST(pidfile_nofile_utest)
{
	bool ok;

	pidfile_set_filename(NULL);

	ok = pidfile_create();
	ck_assert_int_eq(ok, true);

	ok = pidfile_remove();
	ck_assert_int_eq(ok, true);
}
END_TEST


START_TEST(pidfile_wfile_utest)
{
	bool ok;

	pidfile_set_filename("./pidfile.pid");

	ok = pidfile_create();
	ck_assert_int_eq(ok, true);

	ok = pidfile_remove();
	ck_assert_int_eq(ok, true);

	pidfile_set_filename(NULL);

	errno = 0;
	unlink("./pidfile.pid");
	ck_assert_int_eq(errno, ENOENT);

}
END_TEST

///

Suite * daemon_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("daemon");

	tc = tcase_create("daemon-pidfile");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, pidfile_nofile_utest);
	tcase_add_test(tc, pidfile_wfile_utest);

	return s;
}
