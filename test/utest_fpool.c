#include <libsccmn.h>
#include <check.h>

////

START_TEST(fpool_alloc_utest)
{
	struct frame_pool fpool;
	bool ok;

	ok = frame_pool_init(&fpool, 3);
	ck_assert_int_eq(ok, true);

	struct frame * frame1 = frame_pool_borrow(&fpool);
	ck_assert_ptr_ne(frame1, NULL);

	struct frame * frame2 = frame_pool_borrow(&fpool);
	ck_assert_ptr_ne(frame2, NULL);
	ck_assert_ptr_ne(frame2, frame1);

	struct frame * frame3 = frame_pool_borrow(&fpool);
	ck_assert_ptr_ne(frame3, NULL);
	ck_assert_ptr_ne(frame3, frame1);
	ck_assert_ptr_ne(frame3, frame2);

	struct frame * frame4 = frame_pool_borrow(&fpool);
	ck_assert_ptr_eq(frame4, NULL);

	frame_pool_fini(&fpool);
}
END_TEST

///

Suite * fpool_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("fpool");

	tc = tcase_create("fpool-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, fpool_alloc_utest);

	return s;
}
