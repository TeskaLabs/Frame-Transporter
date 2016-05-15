#include <libsccmn.h>
#include <../src/all.h>
#include <check.h>

////

START_TEST(frame_core_utest)
{
	int rc;
	struct frame frame;
	uint8_t * frame_data;

	rc = posix_memalign((void **)&frame_data, MEMPAGE_SIZE, FRAME_SIZE); 
	ck_assert_int_eq(rc, 0);

	_frame_init(&frame, frame_data, NULL);

	size_t cap = frame_capacity(&frame);
	ck_assert_int_eq(cap, 0);

	free(frame_data);
}
END_TEST



///

Suite * frame_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("frame");

	tc = tcase_create("frame-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, frame_core_utest);

	return s;
}
