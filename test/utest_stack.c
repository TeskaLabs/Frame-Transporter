#include "testutils.h"

START_TEST(stack_1_utest)
{
	bool ok;
	struct ft_stack stack;

	ok = ft_stack_init(&stack, NULL);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(ft_stack_size(&stack), 0);

	ft_stack_fini(&stack);

}
END_TEST

///

START_TEST(stack_2_utest)
{
	bool ok;
	struct ft_stack stack;

	ok = ft_stack_init(&stack, NULL);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(ft_stack_size(&stack), 0);

	struct ft_stack_node * n1 = ft_stack_node_new(2);
	ok = ft_stack_push(&stack, n1);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(ft_stack_size(&stack), 1);

	struct ft_stack_node * n2 = ft_stack_pop(&stack);
	ck_assert_ptr_eq(n2, n1);

	ck_assert_int_eq(ft_stack_size(&stack), 0);

	ft_stack_node_del(n1);

	ft_stack_fini(&stack);
}
END_TEST

///

struct ft_stack stack_3;
int stack_3_cntr = 0;

void stack_3_utest_on_remove_callback(struct ft_stack * stack, struct ft_stack_node * node)
{
	ck_assert_ptr_eq(&stack_3, stack);
	stack_3_cntr += 1;
}

START_TEST(stack_3_utest)
{
	bool ok;

	ok = ft_stack_init(&stack_3, stack_3_utest_on_remove_callback);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(ft_stack_size(&stack_3), 0);

	struct ft_stack_node * n1  = ft_stack_node_new(2);
	ok = ft_stack_push(&stack_3, n1);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(ft_stack_size(&stack_3), 1);

	ft_stack_fini(&stack_3);

	ck_assert_int_eq(stack_3_cntr, 1);
}
END_TEST

///

START_TEST(stack_4_utest)
{
	bool ok;
	struct ft_stack stack;

	ok = ft_stack_init(&stack, NULL);
	ck_assert_int_eq(ok, true);

	ck_assert_int_eq(ft_stack_size(&stack), 0);

	for (int i = 0; i < 100; i += 1)
	{
		struct ft_stack_node * n = ft_stack_pop(&stack);
		ck_assert_ptr_eq(n, NULL);
	}

	ft_stack_fini(&stack);

}
END_TEST

///

Suite * stack_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("stack");

	tc = tcase_create("stack-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, stack_1_utest);
	tcase_add_test(tc, stack_2_utest);
	tcase_add_test(tc, stack_3_utest);
	tcase_add_test(tc, stack_4_utest);

	return s;
}
