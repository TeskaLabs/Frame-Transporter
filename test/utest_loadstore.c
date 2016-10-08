#include "testutils.h"

////

START_TEST(ft_skip_u8_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u8(cursor);

	ck_assert_ptr_eq(cursor, buffer + 1);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_skip_u16_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u16(cursor);

	ck_assert_ptr_eq(cursor, buffer + 2);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_skip_u24_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u24(cursor);

	ck_assert_ptr_eq(cursor, buffer + 3);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_skip_u32_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u32(cursor);

	ck_assert_ptr_eq(cursor, buffer + 4);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

////

START_TEST(ft_store_u8_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xDD);

	uint8_t * cursor = buffer;
	cursor = ft_store_u8(cursor, 0xF1);

	ck_assert_ptr_eq(cursor, buffer + 1);
	ck_assert_int_eq(buffer[0], 0xF1);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_store_u16_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xDD);

	uint8_t * cursor = buffer;
	cursor = ft_store_u16(cursor, 0xF35F);

	ck_assert_ptr_eq(cursor, buffer + 2);
	ck_assert_int_eq(buffer[0], 0xF3);
	ck_assert_int_eq(buffer[1], 0x5F);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_store_u24_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	uint8_t * cursor = buffer;
	cursor = ft_store_u24(cursor, 0xF3579F);

	ck_assert_ptr_eq(cursor, buffer + 3);
	ck_assert_int_eq(buffer[0], 0xF3);
	ck_assert_int_eq(buffer[1], 0x57);
	ck_assert_int_eq(buffer[2], 0x9F);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_store_u32_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	uint8_t * cursor = buffer;
	cursor = ft_store_u32(cursor, 0xF3579ACF);

	ck_assert_ptr_eq(cursor, buffer + 4);
	ck_assert_int_eq(buffer[0], 0xF3);
	ck_assert_int_eq(buffer[1], 0x57);
	ck_assert_int_eq(buffer[2], 0x9A);
	ck_assert_int_eq(buffer[3], 0xCF);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///

START_TEST(ft_load_u8_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0xF1;

	uint8_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u8(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 1);
	ck_assert_int_eq(target, 0xF1);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_load_u16_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0xF3;
	buffer[1] = 0x5F;

	uint16_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u16(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 2);
	ck_assert_int_eq(target, 0xF35F);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_load_u24_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0xF3;
	buffer[1] = 0x57;
	buffer[2] = 0x9F;

	uint32_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u24(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 3);
	ck_assert_int_eq(target, 0xF3579F);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

START_TEST(ft_load_u32_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0xF3;
	buffer[1] = 0x57;
	buffer[2] = 0x9A;
	buffer[3] = 0xCF;

	uint32_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u32(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 4);
	ck_assert_int_eq(target, 0xF3579ACF);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
}
END_TEST

///
Suite * ft_loadstore_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("loadstore");

	tc = tcase_create("loadstore-skip");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_skip_u8_utest);
	tcase_add_test(tc, ft_skip_u16_utest);
	tcase_add_test(tc, ft_skip_u24_utest);
	tcase_add_test(tc, ft_skip_u32_utest);

	tc = tcase_create("loadstore-store");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_store_u8_utest);
	tcase_add_test(tc, ft_store_u16_utest);
	tcase_add_test(tc, ft_store_u24_utest);
	tcase_add_test(tc, ft_store_u32_utest);

	tc = tcase_create("loadstore-load");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_load_u8_utest);
	tcase_add_test(tc, ft_load_u16_utest);
	tcase_add_test(tc, ft_load_u24_utest);
	tcase_add_test(tc, ft_load_u32_utest);

	return s;
}
