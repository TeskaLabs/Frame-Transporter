#include <libsccmn.h>
#include <check.h>

////

static int heartbeat_core_utest_counter = 0;
static int heartbeat_core_utest_counter2 = 0;

void heartbeat_core_utest_cb(struct ev_loop * loop, ev_tstamp now)
{
	heartbeat_core_utest_counter += 1;

	if (heartbeat_core_utest_counter >= 3) ev_break(loop, EVBREAK_ALL);
}

void heartbeat_core_utest_cb2(struct ev_loop * loop, ev_tstamp now)
{
	heartbeat_core_utest_counter2 += 1;
}


START_TEST(heartbeat_core_utest)
{
	bool ok;
	struct heartbeat hb;

	ok = heartbeat_init(&hb, 10, 0.1);
	ck_assert_int_eq(ok, true);


	struct ev_loop * loop = ev_default_loop(0);
	ck_assert_ptr_ne(loop, NULL);

	ok = heartbeat_start(loop, &hb);
	ck_assert_int_eq(ok, true);

	ok = heartbeat_add(&hb, heartbeat_core_utest_cb);
	ck_assert_int_eq(ok, true);

	ok = heartbeat_add(&hb, heartbeat_core_utest_cb2);
	ck_assert_int_eq(ok, true);

	ok = heartbeat_add(&hb, heartbeat_core_utest_cb2);
	ck_assert_int_eq(ok, true);

	ev_run(loop, 0);

	ok = heartbeat_stop(loop, &hb);
	ck_assert_int_eq(ok, true);

	ok = heartbeat_fini(&hb);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(heartbeat_core_utest_counter, 3);
	ck_assert_int_eq(heartbeat_core_utest_counter2, 6);
}
END_TEST

///

Suite * heartbeat_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("heartbeat");

	tc = tcase_create("heartbeat-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, heartbeat_core_utest);

	return s;
}
