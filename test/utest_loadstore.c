#include "testutils.h"

////

START_TEST(ft_skip_u8_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u8(cursor);

	ck_assert_ptr_eq(cursor, buffer + 1);
}
END_TEST

START_TEST(ft_skip_u16_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u16(cursor);

	ck_assert_ptr_eq(cursor, buffer + 2);
}
END_TEST

START_TEST(ft_skip_u24_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u24(cursor);

	ck_assert_ptr_eq(cursor, buffer + 3);
}
END_TEST

START_TEST(ft_skip_u32_utest)
{
	uint8_t buffer[1000];

	uint8_t * cursor = buffer;
	cursor = ft_skip_u32(cursor);

	ck_assert_ptr_eq(cursor, buffer + 4);
}
END_TEST

////

START_TEST(ft_store_u8_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	uint8_t * cursor = buffer;
	cursor = ft_store_u8(cursor, 0x56);

	ck_assert_ptr_eq(cursor, buffer + 1);
	ck_assert_int_eq(buffer[0], 0x56);
}
END_TEST

START_TEST(ft_store_u16_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	uint8_t * cursor = buffer;
	cursor = ft_store_u16(cursor, 0x1357);

	ck_assert_ptr_eq(cursor, buffer + 2);
	ck_assert_int_eq(buffer[0], 0x13);
	ck_assert_int_eq(buffer[1], 0x57);
}
END_TEST

START_TEST(ft_store_u24_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	uint8_t * cursor = buffer;
	cursor = ft_store_u24(cursor, 0x13579A);

	ck_assert_ptr_eq(cursor, buffer + 3);
	ck_assert_int_eq(buffer[0], 0x13);
	ck_assert_int_eq(buffer[1], 0x57);
	ck_assert_int_eq(buffer[2], 0x9A);
}
END_TEST

START_TEST(ft_store_u32_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	uint8_t * cursor = buffer;
	cursor = ft_store_u32(cursor, 0x13579ACE);

	ck_assert_ptr_eq(cursor, buffer + 4);
	ck_assert_int_eq(buffer[0], 0x13);
	ck_assert_int_eq(buffer[1], 0x57);
	ck_assert_int_eq(buffer[2], 0x9A);
	ck_assert_int_eq(buffer[3], 0xCE);
}
END_TEST

///

START_TEST(ft_load_u8_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0x56;

	uint8_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u8(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 1);
	ck_assert_int_eq(target, 0x56);
}
END_TEST

START_TEST(ft_load_u16_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0x13;
	buffer[1] = 0x57;

	uint16_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u16(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 2);
	ck_assert_int_eq(target, 0x1357);
}
END_TEST

START_TEST(ft_load_u24_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0x13;
	buffer[1] = 0x57;
	buffer[2] = 0x9A;

	uint32_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u24(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 3);
	ck_assert_int_eq(target, 0x13579A);
}
END_TEST

START_TEST(ft_load_u32_utest)
{
	uint8_t buffer[1000];
	memset(buffer, sizeof(buffer), 0xFE);

	buffer[0] = 0x13;
	buffer[1] = 0x57;
	buffer[2] = 0x9A;
	buffer[3] = 0xCE;

	uint32_t target;
	uint8_t * cursor = buffer;
	cursor = ft_load_u32(cursor, &target);

	ck_assert_ptr_eq(cursor, buffer + 4);
	ck_assert_int_eq(target, 0x13579ACE);
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
