#include "testutils.h"
#include <../src/_ft_internal.h>

////

static bool frame_core_utest_vprintf(struct ft_vec * dvec, const char * fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	bool ok = ft_vec_vsprintf(dvec, fmt, ap);
	va_end(ap);

	return ok;
}

START_TEST(frame_core_utest)
{
	int rc;
	bool ok;
	size_t pos;
	struct frame frame;
	uint8_t * frame_data;

	rc = posix_memalign((void **)&frame_data, MEMPAGE_SIZE, FRAME_SIZE); 
	ck_assert_int_eq(rc, 0);

	_frame_init(&frame, frame_data, MEMPAGE_SIZE, NULL);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);

	frame_format_simple(&frame);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 1);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 0);

	struct ft_vec * dvec = frame_current_dvec(&frame);
	ck_assert_ptr_ne(dvec, NULL);
	ck_assert_int_eq(dvec->position, 0);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	ok = ft_vec_cat(dvec, "0123456789", 10);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(dvec->position, 10);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 10);

	ok = ft_vec_cat(dvec, "ABCDEFG", 7);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(dvec->position, 17);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 17);

	ok = ft_vec_strcat(dvec, "abcdefg");
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(dvec->position, 24);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 24);

	ok = frame_core_utest_vprintf(dvec, "%s %d %d", "Hello world", 1, 777);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(dvec->position, 41);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 41);

	ok = ft_vec_sprintf(dvec, "%s %d %d", "Another text", 12311, 743427);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(dvec->position, 66);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 66);

	free(frame_data);
}
END_TEST


START_TEST(frame_flip_utest)
{
	int rc;
	size_t pos;
	struct frame frame;
	uint8_t * frame_data;

	rc = posix_memalign((void **)&frame_data, MEMPAGE_SIZE, FRAME_SIZE); 
	ck_assert_int_eq(rc, 0);

	_frame_init(&frame, frame_data, MEMPAGE_SIZE, NULL);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);
	
	frame_format_simple(&frame);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 1);

	struct ft_vec * dvec = frame_current_dvec(&frame);
	ck_assert_int_eq(dvec->position, 0);
	ck_assert_int_eq(dvec->limit, frame.capacity - sizeof(struct ft_vec));
	ck_assert_int_eq(dvec->limit, dvec->capacity);

	ft_vec_pos(dvec, 333);
	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 333);

	ft_vec_flip(dvec);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 1);

	pos = frame_total_start_to_position(&frame);
	ck_assert_int_eq(pos, 0);

	pos = frame_total_position_to_limit(&frame);
	ck_assert_int_eq(pos, 333);

	free(frame_data);
}
END_TEST


START_TEST(frame_add_dvec_utest)
{
	int rc;
	struct frame frame;
	uint8_t * frame_data;
	struct ft_vec * dvec;

	rc = posix_memalign((void **)&frame_data, MEMPAGE_SIZE, FRAME_SIZE); 
	ck_assert_int_eq(rc, 0);

	_frame_init(&frame, frame_data, MEMPAGE_SIZE, NULL);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);

	dvec = frame_add_dvec(&frame, 0, FRAME_SIZE);
	ck_assert_ptr_eq(dvec, NULL);

	dvec = frame_add_dvec(&frame, FRAME_SIZE, 1);
	ck_assert_ptr_eq(dvec, NULL);

	for (int i=0; i<30; i+= 1)
	{
		dvec = frame_add_dvec(&frame, 0, 30);
		ck_assert_ptr_ne(dvec, NULL);
	}

	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 30);
}
END_TEST


START_TEST(frame_format_empty_utest)
{
	int rc;
	size_t x;
	struct frame frame;
	uint8_t * frame_data;
	struct ft_vec * dvec;

	rc = posix_memalign((void **)&frame_data, MEMPAGE_SIZE, FRAME_SIZE); 
	ck_assert_int_eq(rc, 0);

	_frame_init(&frame, frame_data, MEMPAGE_SIZE, NULL);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);

	frame_format_empty(&frame);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);

	dvec = frame_add_dvec(&frame, 0, 30);
	ck_assert_ptr_ne(dvec, NULL);

	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 1);

	frame_format_empty(&frame);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);

	x = frame_total_start_to_position(&frame);
	ck_assert_int_eq(x, 0);

	x = frame_total_position_to_limit(&frame);
	ck_assert_int_eq(x, 0);
}
END_TEST


START_TEST(frame_format_simple_utest)
{
	int rc;
	size_t x;
	struct frame frame;
	uint8_t * frame_data;
	struct ft_vec * dvec;

	rc = posix_memalign((void **)&frame_data, MEMPAGE_SIZE, FRAME_SIZE); 
	ck_assert_int_eq(rc, 0);

	_frame_init(&frame, frame_data, MEMPAGE_SIZE, NULL);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 0);

	frame_format_simple(&frame);
	ck_assert_int_eq(frame.vec_position, 0);
	ck_assert_int_eq(frame.vec_limit, 1);

	x = frame_total_start_to_position(&frame);
	ck_assert_int_eq(x, 0);

	x = frame_total_position_to_limit(&frame);
	ck_assert_int_eq(x, 4056);

	x = frame_currect_dvec_size(&frame);
	ck_assert_int_eq(x, 4056);

	dvec = frame_current_dvec(&frame);
	ck_assert_ptr_ne(dvec, NULL);

	ft_vec_pos(dvec, 10);

	x = frame_currect_dvec_size(&frame);
	ck_assert_int_eq(x, 4046);

	x = frame_total_start_to_position(&frame);
	ck_assert_int_eq(x, 10);

	ft_vec_advance(dvec, 33);

	x = frame_total_start_to_position(&frame);
	ck_assert_int_eq(x, 43);

	x = frame_currect_dvec_size(&frame);
	ck_assert_int_eq(x, 4013);

	frame_flip(&frame);

	x = frame_total_start_to_position(&frame);
	ck_assert_int_eq(x, 0);

	x = frame_currect_dvec_size(&frame);
	ck_assert_int_eq(x, 43);

	x = frame_total_position_to_limit(&frame);
	ck_assert_int_eq(x, 43);
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
	tcase_add_test(tc, frame_flip_utest);
	tcase_add_test(tc, frame_add_dvec_utest);

	tc = tcase_create("frame-format");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, frame_format_empty_utest);
	tcase_add_test(tc, frame_format_simple_utest);

	return s;
}
