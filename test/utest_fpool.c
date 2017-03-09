#include "testutils.h"

////

START_TEST(fpool_alloc_up_utest)
{
	bool ok;
	size_t x;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	x = ft_pool_count_available_frames(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	x = ft_pool_count_zones(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	const int frame_count = 32;
	struct ft_frame * frames[frame_count];

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_RAW_DATA);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	x = ft_pool_count_zones(&context.frame_pool);
	ck_assert_int_eq(x, 2);

	x = ft_pool_count_available_frames(&context.frame_pool);
	ck_assert_int_gt(x, 100);

	for (int i = 0; i<frame_count; i += 1)
	{
		ft_frame_return(frames[i]);
	}

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST


START_TEST(fpool_alloc_down_utest)
{
	bool ok;
	size_t x;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	x = ft_pool_count_available_frames(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	x = ft_pool_count_zones(&context.frame_pool);
	ck_assert_int_eq(x, 0);	

	const int frame_count = 32;
	struct ft_frame * frames[frame_count];

	// First cycle

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
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
		ft_frame_return(frames[i]);
	}

	// Second cycle

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i]->borrowed_by_file, NULL);
		ck_assert_int_ne(frames[i]->borrowed_by_line, 0);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	x = ft_pool_count_zones(&context.frame_pool);
	ck_assert_int_eq(x, 2);

	x = ft_pool_count_available_frames(&context.frame_pool);
	ck_assert_int_gt(x, 100);

	for (int i = frame_count-1; i>=0; i -= 1)
	{
		ft_frame_return(frames[i]);
	}

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST


int ft_pool_alloc_custom_counter = 0;

static struct ft_poolzone * ft_pool_alloc_custom(struct ft_pool * frame_pool)
{
	ft_pool_alloc_custom_counter += 1;
	return ft_poolzone_new_mmap(frame_pool, 1, true, MAP_PRIVATE | MAP_ANON);
}

START_TEST(fpool_alloc_custom_advice_utest)
{
	bool ok;

	ev_tstamp heartbeat_interval_backup = ft_config.heartbeat_interval;
	ft_config.heartbeat_interval = 0.1;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	ft_pool_set_alloc(&context.frame_pool, ft_pool_alloc_custom);

	const int frame_count = 32;
	struct ft_frame * frames[frame_count];

	for (int i = 0; i<frame_count; i += 1)
	{
		frames[i] = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_END_OF_STREAM);
		ck_assert_ptr_ne(frames[i], NULL);

		ck_assert_ptr_eq(frames[i]->next, NULL);
		ck_assert_int_eq(frames[i]->type, FT_FRAME_TYPE_END_OF_STREAM);
		ck_assert_ptr_ne(frames[i]->borrowed_by_file, NULL);
		ck_assert_int_ne(frames[i]->borrowed_by_line, 0);

		for (int j = 0; j < i; j += 1)
			ck_assert_ptr_ne(frames[j], frames[i]);
	}

	for (int i = 0; i<frame_count; i += 1)
	{
		ft_frame_return(frames[i]);
	}

	ck_assert_int_eq(ft_pool_alloc_custom_counter, frame_count);

	// Simulate heartbeat
	ft_config.fpool_zone_free_timeout = 0.2;
	ev_ref(context.ev_loop);
	while (context.frame_pool.zones != NULL)
	{
		ev_run(context.ev_loop, EVRUN_ONCE);
	}

	ft_config.heartbeat_interval = heartbeat_interval_backup;

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST


START_TEST(fpool_double_return)
{
	bool ok;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	struct ft_frame * frame = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);

	ft_frame_return(frame);
	ft_frame_return(frame);

	ft_context_fini(&context);

}
END_TEST

START_TEST(fpool_default)
{
	bool ok;

	struct ft_frame * frame = ft_frame_borrow(FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_eq(frame, NULL);

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	frame = ft_frame_borrow(FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_ne(frame, NULL);

	ft_frame_return(frame);

	ft_context_fini(&context);

	frame = ft_frame_borrow(FT_FRAME_TYPE_RAW_DATA);
	ck_assert_ptr_eq(frame, NULL);
}
END_TEST

///

struct fpool_pubsub_memory_data
{
	bool inc;
	int frames_available;
};

void fpool_pubsub_memory_cb_utest(struct ft_subscriber * subscriber, struct ft_pubsub * pubsub, const char * topic, void * data)
{
	struct fpool_pubsub_memory_data * cnt_data = subscriber->data;
	ck_assert_ptr_ne(cnt_data, NULL);

	struct ft_pubsub_message_pool_lowmem * lowmem = data;
	ck_assert_ptr_ne(lowmem, NULL);

	cnt_data->inc = lowmem->inc;
	cnt_data->frames_available = lowmem->frames_available;
}


START_TEST(fpool_pubsub_memory)
{
	bool ok;

	struct ft_frame * frames[3000];	

	struct fpool_pubsub_memory_data cnt_data;

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	struct ft_subscriber sub1;
	ok = ft_subscriber_init(&sub1, fpool_pubsub_memory_cb_utest);
	ck_assert_int_eq(ok, true);
	sub1.data = &cnt_data;

	ok = ft_subscriber_subscribe(&sub1, &context.pubsub, FT_PUBSUB_TOPIC_POOL_LOWMEM);
	ck_assert_int_eq(ok, true);

	for (int i=0; i<2061; i+=1)
	{
		cnt_data.frames_available = 1000000000;

		frames[i] = ft_pool_borrow(&context.frame_pool, FT_FRAME_TYPE_RAW_DATA);
		ck_assert_ptr_ne(frames[i], NULL);

		if (i > 1960)
		{
			ck_assert_int_eq(cnt_data.frames_available, 2060-i);
			ck_assert_int_eq(cnt_data.inc, false);
		}

		else
		{
			ck_assert_int_eq(cnt_data.frames_available, 1000000000);	
		}
	}

	for (int i=0; i<2061; i+=1)
	{
		cnt_data.frames_available = 1000000000;

		ft_frame_return(frames[i]);

		if (i < 99)
		{
			ck_assert_int_eq(cnt_data.frames_available, i+1);
			ck_assert_int_eq(cnt_data.inc, true);
		}

		else
		{
			ck_assert_int_eq(cnt_data.frames_available, 1000000000);	
		}

	}
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
	tcase_add_test(tc, fpool_default);
	tcase_add_test(tc, fpool_pubsub_memory);

	tc = tcase_create("fpool-exit");
	suite_add_tcase(s, tc);
	tcase_add_test_raise_signal(tc, fpool_double_return, SIGABRT);

	return s;
}
