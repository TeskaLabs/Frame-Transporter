#include "testutils.h"

const char * ft_pubsub_utest_TOPIC1 = "topic1";

////

START_TEST(ft_pubsub_1_utest)
{
	bool ok;
	struct ft_pubsub pubsub;

	ok = ft_pubsub_init(&pubsub);
	ck_assert_int_eq(ok, true);

	struct ft_subscriber sub1;
	ok = ft_subscriber_init(&sub1, NULL);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_subscribe(&sub1, &pubsub, ft_pubsub_utest_TOPIC1);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_subscribe(&sub1, &pubsub, ft_pubsub_utest_TOPIC1);
	ck_assert_int_eq(ok, false);

	ok = ft_pubsub_unsubscribe(&sub1, &pubsub);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_unsubscribe(&sub1, &pubsub);
	ck_assert_int_eq(ok, false);

	ok = ft_pubsub_subscribe(&sub1, &pubsub, ft_pubsub_utest_TOPIC1);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_unsubscribe(&sub1, &pubsub);
	ck_assert_int_eq(ok, true);

	ft_pubsub_fini(&pubsub);
}
END_TEST

///

START_TEST(ft_pubsub_2_utest)
{
	bool ok;
	struct ft_pubsub pubsub;

	ok = ft_pubsub_init(&pubsub);
	ck_assert_int_eq(ok, true);

	struct ft_subscriber sub1;
	ok = ft_subscriber_init(&sub1, NULL);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_subscribe(&sub1, &pubsub, ft_pubsub_utest_TOPIC1);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_publish(&pubsub, ft_pubsub_utest_TOPIC1, NULL);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_unsubscribe(&sub1, &pubsub);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_publish(&pubsub, ft_pubsub_utest_TOPIC1, NULL);
	ck_assert_int_eq(ok, true);

	ft_pubsub_fini(&pubsub);
}
END_TEST

///

struct ft_pubsub_3_data
{
	int counter;
};

void ft_pubsub_3_cb_utest(struct ft_subscriber * subscriber, struct ft_pubsub * pubsub, const char * topic, void * data)
{
	ck_assert_ptr_ne(subscriber, NULL);
	ck_assert_ptr_ne(pubsub, NULL);
	ck_assert_ptr_eq(topic, ft_pubsub_utest_TOPIC1);
	ck_assert_ptr_ne(data, NULL);

	struct ft_pubsub_3_data * x = data;
	x->counter += 1;
}

START_TEST(ft_pubsub_3_utest)
{
	bool ok;
	struct ft_pubsub pubsub;

	struct ft_pubsub_3_data data = {
		.counter = 0
	};

	ok = ft_pubsub_init(&pubsub);
	ck_assert_int_eq(ok, true);

	struct ft_subscriber sub1;
	ok = ft_subscriber_init(&sub1, ft_pubsub_3_cb_utest);
	ck_assert_int_eq(ok, true);

	struct ft_subscriber sub2;
	ok = ft_subscriber_init(&sub2, ft_pubsub_3_cb_utest);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_subscribe(&sub1, &pubsub, ft_pubsub_utest_TOPIC1);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_subscribe(&sub2, &pubsub, ft_pubsub_utest_TOPIC1);
	ck_assert_int_eq(ok, true);

	for (int i=1; i<1001; i++)
	{
		ok = ft_pubsub_publish(&pubsub, ft_pubsub_utest_TOPIC1, &data);
		ck_assert_int_eq(ok, true);
		ck_assert_int_eq(data.counter, i*2);
	}

	ok = ft_pubsub_unsubscribe(&sub2, &pubsub);
	ck_assert_int_eq(ok, true);

	for (int i=1; i<1001; i++)
	{
		ok = ft_pubsub_publish(&pubsub, ft_pubsub_utest_TOPIC1, &data);
		ck_assert_int_eq(ok, true);
		ck_assert_int_eq(data.counter, 2000+i);
	}


	ok = ft_pubsub_unsubscribe(&sub1, &pubsub);
	ck_assert_int_eq(ok, true);

	ok = ft_pubsub_publish(&pubsub, ft_pubsub_utest_TOPIC1, &data);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(data.counter, 3000);

	ft_pubsub_fini(&pubsub);
}
END_TEST

///

Suite * pubsub_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("pubsub");

	tc = tcase_create("pubsub-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_pubsub_1_utest);
	tcase_add_test(tc, ft_pubsub_2_utest);
	tcase_add_test(tc, ft_pubsub_3_utest);

	return s;
}
