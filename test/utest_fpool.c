#include <libsccmn.h>
#include <check.h>

////

START_TEST(fpool_alloc_up_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	const int frame_count = 32;
	struct frame * frames[frame_count];

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, frame_type_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);
		ck_assert_int_eq(frames[i]->type, frame_type_RAW_DATA);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	for (int i = 0; i<frame_count; i += 1)
	{
		frame_pool_return(frames[i]);
	}

	context_fini(&context);
}
END_TEST


START_TEST(fpool_alloc_down_utest)
{
	bool ok;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	const int frame_count = 32;
	struct frame * frames[frame_count];

	// First cycle

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, frame_type_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, frame_type_RAW_DATA);
		ck_assert_ptr_ne(frames[i]->borrowed_by_file, NULL);
		ck_assert_int_ne(frames[i]->borrowed_by_line, 0);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	for (int i = frame_count-1; i>=0; i -= 1)
	{
		frame_pool_return(frames[i]);
	}

	// Second cycle

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, frame_type_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, frame_type_RAW_DATA);
		ck_assert_ptr_ne(frames[i]->borrowed_by_file, NULL);
		ck_assert_int_ne(frames[i]->borrowed_by_line, 0);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	for (int i = frame_count-1; i>=0; i -= 1)
	{
		frame_pool_return(frames[i]);
	}

	context_fini(&context);
}
END_TEST


int frame_pool_zone_alloc_advice_custom_counter = 0;

static struct frame_pool_zone * frame_pool_zone_alloc_advice_custom(struct frame_pool * this)
{
	frame_pool_zone_alloc_advice_custom_counter += 1;
	return frame_pool_zone_new(1, true);
}

START_TEST(fpool_alloc_custom_advice_utest)
{
	bool ok;

	ev_tstamp heartbeat_interval_backup = libsccmn_config.heartbeat_interval;
	libsccmn_config.heartbeat_interval = 0.1;

	struct context context;
	ok = context_init(&context);
	ck_assert_int_eq(ok, true);

	frame_pool_set_alloc_advise(&context.frame_pool, frame_pool_zone_alloc_advice_custom);

	const int frame_count = 32;
	struct frame * frames[frame_count];

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, frame_type_STREAM_END);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, frame_type_STREAM_END);
		ck_assert_ptr_ne(frames[i]->borrowed_by_file, NULL);
		ck_assert_int_ne(frames[i]->borrowed_by_line, 0);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	for (int i = 0; i<frame_count; i += 1)
	{
		frame_pool_return(frames[i]);
	}

	ck_assert_int_eq(frame_pool_zone_alloc_advice_custom_counter, frame_count);

	// Simulate heartbeat
	libsccmn_config.fpool_zone_free_timeout = 0.2;
	ev_ref(context.ev_loop);
	while (context.frame_pool.zones != NULL)
	{
		ev_run(context.ev_loop, EVRUN_ONCE);
	}

	libsccmn_config.heartbeat_interval = heartbeat_interval_backup;

	context_fini(&context);
}
END_TEST

///

Suite * fpool_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("fpool");

	tc = tcase_create("fpool-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, fpool_alloc_up_utest);
	tcase_add_test(tc, fpool_alloc_down_utest);
	tcase_add_test(tc, fpool_alloc_custom_advice_utest);

	return s;
}
