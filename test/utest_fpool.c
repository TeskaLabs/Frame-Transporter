#include "testutils.h"

////

START_TEST(fpool_alloc_up_utest)
{
	bool ok;
	size_t x;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	x = frame_pool_available_frames_count(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	x = frame_pool_zones_count(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	const int frame_count = 32;
	struct frame * frames[frame_count];

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_RAW_DATA);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	x = frame_pool_zones_count(&context.frame_pool);
	ck_assert_int_eq(x, 2);

	x = frame_pool_available_frames_count(&context.frame_pool);
	ck_assert_int_gt(x, 100);

	for (int i = 0; i<frame_count; i += 1)
	{
		frame_pool_return(frames[i]);
	}

	ft_context_fini(&context);
}
END_TEST


START_TEST(fpool_alloc_down_utest)
{
	bool ok;
	size_t x;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	x = frame_pool_available_frames_count(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	x = frame_pool_zones_count(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	const int frame_count = 32;
	struct frame * frames[frame_count];

	// First cycle

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_RAW_DATA);
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
		frames[i] = frame_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i]->borrowed_by_file, NULL);
		ck_assert_int_ne(frames[i]->borrowed_by_line, 0);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	x = frame_pool_zones_count(&context.frame_pool);
	ck_assert_int_eq(x, 2);

	x = frame_pool_available_frames_count(&context.frame_pool);
	ck_assert_int_gt(x, 100);

	for (int i = frame_count-1; i>=0; i -= 1)
	{
		frame_pool_return(frames[i]);
	}

	ft_context_fini(&context);
}
END_TEST


int frame_pool_zone_alloc_advice_custom_counter = 0;

static struct frame_pool_zone * frame_pool_zone_alloc_advice_custom(struct frame_pool * frame_pool)
{
	frame_pool_zone_alloc_advice_custom_counter += 1;
	return frame_pool_zone_new_mmap(frame_pool, 1, true, MAP_PRIVATE | MAP_ANON);
}

START_TEST(fpool_alloc_custom_advice_utest)
{
	bool ok;

	ev_tstamp heartbeat_interval_backup = ft_config.heartbeat_interval;
	ft_config.heartbeat_interval = 0.1;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	frame_pool_set_alloc_advise(&context.frame_pool, frame_pool_zone_alloc_advice_custom);

	const int frame_count = 32;
	struct frame * frames[frame_count];

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = frame_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_STREAM_END);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_STREAM_END);
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
	ft_config.fpool_zone_free_timeout = 0.2;
	ev_ref(context.ev_loop);
	while (context.frame_pool.zones != NULL)
	{
		ev_run(context.ev_loop, EVRUN_ONCE);
	}

	ft_config.heartbeat_interval = heartbeat_interval_backup;

	ft_context_fini(&context);
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
