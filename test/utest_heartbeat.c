#include <libsccmn.h>
#include <check.h>

////

static int heartbeat_core_utest_counter = 0;
static int heartbeat_core_utest_counter2 = 0;

void heartbeat_core_utest_cb(struct ev_loop * loop, struct heartbeat_watcher * watcher, ev_tstamp now)
{
	heartbeat_core_utest_counter += 1;

	if (heartbeat_core_utest_counter >= 3) ev_break(loop, EVBREAK_ALL);
}

void heartbeat_core_utest_cb2(struct ev_loop * loop, struct heartbeat_watcher * watcher, ev_tstamp now)
{
	heartbeat_core_utest_counter2 += 1;
}


START_TEST(heartbeat_core_utest)
{
	struct heartbeat hb;

	heartbeat_init(&hb, 0.1);

	struct ev_loop * loop = ev_default_loop(0);
	ck_assert_ptr_ne(loop, NULL);

	heartbeat_start(loop, &hb);

	struct heartbeat_watcher hbw1;
	heartbeat_add(&hb, &hbw1, heartbeat_core_utest_cb);

	struct heartbeat_watcher hbw2;
	heartbeat_add(&hb, &hbw2, heartbeat_core_utest_cb2);

	struct heartbeat_watcher hbw3;
	heartbeat_add(&hb, &hbw3, heartbeat_core_utest_cb2);

	struct heartbeat_watcher hbw4;
	heartbeat_add(&hb, &hbw4, NULL); // Watcher with no callback

	ev_ref(loop);
	ev_run(loop, 0);

	heartbeat_stop(loop, &hb);

	ck_assert_int_eq(heartbeat_core_utest_counter, 3);
	ck_assert_int_eq(heartbeat_core_utest_counter2, 6);

	ev_loop_destroy(loop);
}
END_TEST

///

static void assert_watcher_count(struct heartbeat * hb, int expected)
{
	int fwd_cnt = 0;
	for (struct heartbeat_watcher * watcher = hb->first_watcher; watcher != NULL; watcher = watcher->next)
	{
		fwd_cnt += 1;
	}
	ck_assert_int_eq(fwd_cnt, expected);

	int bwd_cnt = 0;
	for (struct heartbeat_watcher * watcher = hb->last_watcher; watcher != NULL; watcher = watcher->prev)
	{
		bwd_cnt += 1;
	}
	ck_assert_int_eq(bwd_cnt, expected);
}

START_TEST(heartbeat_list1_utest)
{
	struct heartbeat hb;
	struct heartbeat_watcher hbw1;

	heartbeat_init(&hb, 0.1);
	assert_watcher_count(&hb, 0);

	// Add one	
	heartbeat_add(&hb, &hbw1, NULL);
	assert_watcher_count(&hb, 1);

	// Remove one
	heartbeat_remove(&hb, &hbw1);
	assert_watcher_count(&hb, 0);
}
END_TEST

START_TEST(heartbeat_list2_utest)
{
	struct heartbeat hb;
	struct heartbeat_watcher hbw1;
	struct heartbeat_watcher hbw2;

	heartbeat_init(&hb, 0.1);
	assert_watcher_count(&hb, 0);

	// Add one	
	heartbeat_add(&hb, &hbw1, NULL);
	assert_watcher_count(&hb, 1);

	// Add second	
	heartbeat_add(&hb, &hbw2, NULL);
	assert_watcher_count(&hb, 2);

	// Remove second
	heartbeat_remove(&hb, &hbw2);
	assert_watcher_count(&hb, 1);

	// Remove first
	heartbeat_remove(&hb, &hbw1);
	assert_watcher_count(&hb, 0);
}
END_TEST

START_TEST(heartbeat_list3_utest)
{
	struct heartbeat hb;
	struct heartbeat_watcher hbw1;
	struct heartbeat_watcher hbw2;

	heartbeat_init(&hb, 0.1);
	assert_watcher_count(&hb, 0);

	// Add one	
	heartbeat_add(&hb, &hbw1, NULL);
	assert_watcher_count(&hb, 1);

	// Add second	
	heartbeat_add(&hb, &hbw2, NULL);
	assert_watcher_count(&hb, 2);

	// Remove first
	heartbeat_remove(&hb, &hbw1);
	assert_watcher_count(&hb, 1);

	// Remove second
	heartbeat_remove(&hb, &hbw2);
	assert_watcher_count(&hb, 0);
}
END_TEST

START_TEST(heartbeat_list4_utest)
{
	struct heartbeat hb;
	struct heartbeat_watcher hbw1;
	struct heartbeat_watcher hbw2;
	struct heartbeat_watcher hbw3;

	heartbeat_init(&hb, 0.1);
	assert_watcher_count(&hb, 0);

	// Add one	
	heartbeat_add(&hb, &hbw1, NULL);
	assert_watcher_count(&hb, 1);

	// Add second	
	heartbeat_add(&hb, &hbw2, NULL);
	assert_watcher_count(&hb, 2);

	// Add third	
	heartbeat_add(&hb, &hbw3, NULL);
	assert_watcher_count(&hb, 3);

	// Remove second (middle)
	heartbeat_remove(&hb, &hbw2);
	assert_watcher_count(&hb, 2);

	// Remove third
	heartbeat_remove(&hb, &hbw3);
	assert_watcher_count(&hb, 1);

	// Remove first
	heartbeat_remove(&hb, &hbw1);
	assert_watcher_count(&hb, 0);
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

	tc = tcase_create("heartbeat-list");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, heartbeat_list1_utest);
	tcase_add_test(tc, heartbeat_list2_utest);
	tcase_add_test(tc, heartbeat_list3_utest);
	tcase_add_test(tc, heartbeat_list4_utest);

	return s;
}
