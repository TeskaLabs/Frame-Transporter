#include <libsccmn.h>
#include <check.h>

////

static int heartbeat_core_utest_counter = 0;
static int heartbeat_core_utest_counter2 = 0;

void heartbeat_core_utest_cb(struct heartbeat_watcher * watcher, struct heartbeat * heartbeat, ev_tstamp now)
{
	heartbeat_core_utest_counter += 1;

	if (heartbeat_core_utest_counter >= 3) ev_break(heartbeat->context->ev_loop, EVBREAK_ALL);
}

void heartbeat_core_utest_cb2(struct heartbeat_watcher * watcher, struct heartbeat * heartbeat, ev_tstamp now)
{
	heartbeat_core_utest_counter2 += 1;
}


START_TEST(heartbeat_core_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct heartbeat_watcher hbw1;
	heartbeat_add(&context.heartbeat, &hbw1, heartbeat_core_utest_cb);

	struct heartbeat_watcher hbw2;
	heartbeat_add(&context.heartbeat, &hbw2, heartbeat_core_utest_cb2);

	struct heartbeat_watcher hbw3;
	heartbeat_add(&context.heartbeat, &hbw3, heartbeat_core_utest_cb2);

	struct heartbeat_watcher hbw4;
	heartbeat_add(&context.heartbeat, &hbw4, NULL); // Watcher with no callback

	ev_ref(context.ev_loop);
	ev_run(context.ev_loop, 0);

	ck_assert_int_eq(heartbeat_core_utest_counter, 3);
	ck_assert_int_eq(heartbeat_core_utest_counter2, 6);

	context_fini(&context);
}
END_TEST

///

static void assert_watcher_count(struct heartbeat * hb, int expected)
{
	const int system_hbs = 1; // Count of system heartbeats withing the context (1 for frame pool)
	int fwd_cnt = 0;
	for (struct heartbeat_watcher * watcher = hb->first_watcher; watcher != NULL; watcher = watcher->next)
	{
		fwd_cnt += 1;
	}
	ck_assert_int_eq(fwd_cnt, expected+system_hbs);

	int bwd_cnt = 0;
	for (struct heartbeat_watcher * watcher = hb->last_watcher; watcher != NULL; watcher = watcher->prev)
	{
		bwd_cnt += 1;
	}
	ck_assert_int_eq(bwd_cnt, expected+system_hbs);
}

START_TEST(heartbeat_list1_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct heartbeat_watcher hbw1;

	assert_watcher_count(&context.heartbeat, 0);

	// Add one	
	heartbeat_add(&context.heartbeat, &hbw1, NULL);
	assert_watcher_count(&context.heartbeat, 1);

	// Remove one
	heartbeat_remove(&context.heartbeat, &hbw1);
	assert_watcher_count(&context.heartbeat, 0);

	context_fini(&context);
}
END_TEST

START_TEST(heartbeat_list2_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct heartbeat_watcher hbw1;
	struct heartbeat_watcher hbw2;

	assert_watcher_count(&context.heartbeat, 0);

	// Add one	
	heartbeat_add(&context.heartbeat, &hbw1, NULL);
	assert_watcher_count(&context.heartbeat, 1);

	// Add second	
	heartbeat_add(&context.heartbeat, &hbw2, NULL);
	assert_watcher_count(&context.heartbeat, 2);

	// Remove second
	heartbeat_remove(&context.heartbeat, &hbw2);
	assert_watcher_count(&context.heartbeat, 1);

	// Remove first
	heartbeat_remove(&context.heartbeat, &hbw1);
	assert_watcher_count(&context.heartbeat, 0);

	context_fini(&context);
}
END_TEST

START_TEST(heartbeat_list3_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct heartbeat_watcher hbw1;
	struct heartbeat_watcher hbw2;

	assert_watcher_count(&context.heartbeat, 0);

	// Add one	
	heartbeat_add(&context.heartbeat, &hbw1, NULL);
	assert_watcher_count(&context.heartbeat, 1);

	// Add second	
	heartbeat_add(&context.heartbeat, &hbw2, NULL);
	assert_watcher_count(&context.heartbeat, 2);

	// Remove first
	heartbeat_remove(&context.heartbeat, &hbw1);
	assert_watcher_count(&context.heartbeat, 1);

	// Remove second
	heartbeat_remove(&context.heartbeat, &hbw2);
	assert_watcher_count(&context.heartbeat, 0);

	context_fini(&context);
}
END_TEST

START_TEST(heartbeat_list4_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	struct heartbeat_watcher hbw1;
	struct heartbeat_watcher hbw2;
	struct heartbeat_watcher hbw3;

	assert_watcher_count(&context.heartbeat, 0);

	// Add one	
	heartbeat_add(&context.heartbeat, &hbw1, NULL);
	assert_watcher_count(&context.heartbeat, 1);

	// Add second	
	heartbeat_add(&context.heartbeat, &hbw2, NULL);
	assert_watcher_count(&context.heartbeat, 2);

	// Add third	
	heartbeat_add(&context.heartbeat, &hbw3, NULL);
	assert_watcher_count(&context.heartbeat, 3);

	// Remove second (middle)
	heartbeat_remove(&context.heartbeat, &hbw2);
	assert_watcher_count(&context.heartbeat, 2);

	// Remove third
	heartbeat_remove(&context.heartbeat, &hbw3);
	assert_watcher_count(&context.heartbeat, 1);

	// Remove first
	heartbeat_remove(&context.heartbeat, &hbw1);
	assert_watcher_count(&context.heartbeat, 0);
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
