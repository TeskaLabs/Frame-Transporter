#include "testutils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

////

START_TEST(ft_iphashmap_ip4_utest)
{
	bool ok;
	int rc;
	struct ft_iphashmap iphmap;
	struct ft_iphashmap_entry * e, * e1;
	struct in_addr addr4_1, addr4_2;
	void * data;

	rc = inet_aton("1.2.3.4", &addr4_1);
	ck_assert_int_ne(rc, 0);

	rc = inet_aton("1.2.3.5", &addr4_2);
	ck_assert_int_ne(rc, 0);


	ok = ft_iphashmap_init(&iphmap, 4);
	ck_assert_int_eq(ok, true);

	e = ft_iphashmap_get_ip4(&iphmap, addr4_1);
	ck_assert_ptr_eq(e, NULL);

	ok = ft_iphashmap_pop_ip4(&iphmap, addr4_1, &data);
	ck_assert_int_eq(ok, false);

	e = ft_iphashmap_add_ip4(&iphmap, addr4_1);
	ck_assert_ptr_ne(e, NULL);

	e1 = ft_iphashmap_get_ip4(&iphmap, addr4_1);
	ck_assert_ptr_eq(e1, e);

	ok = ft_iphashmap_pop_ip4(&iphmap, addr4_1, &data);
	ck_assert_int_eq(ok, true);

	ok = ft_iphashmap_pop_ip4(&iphmap, addr4_1, &data);
	ck_assert_int_eq(ok, false);

	e = ft_iphashmap_get_ip4(&iphmap, addr4_1);
	ck_assert_ptr_eq(e, NULL);


	e = ft_iphashmap_get_ip4(&iphmap, addr4_2);
	ck_assert_ptr_eq(e, NULL);

	e = ft_iphashmap_add_ip4(&iphmap, addr4_2);
	ck_assert_ptr_ne(e, NULL);

	e1 = ft_iphashmap_get_ip4(&iphmap, addr4_2);
	ck_assert_ptr_eq(e1, e);


	ft_iphashmap_fini(&iphmap);
}
END_TEST


START_TEST(ft_iphashmap_ip6_utest)
{
	bool ok;
	int rc;
	struct ft_iphashmap iphmap;
	struct ft_iphashmap_entry * e, * e1;
	struct in6_addr addr4_1, addr4_2;
	void * data;

	rc = inet_pton(AF_INET6, "::1", &addr4_1);
	ck_assert_int_ne(rc, 0);

	rc = inet_pton(AF_INET6, "1234::1", &addr4_2);
	ck_assert_int_ne(rc, 0);


	ok = ft_iphashmap_init(&iphmap, 4);
	ck_assert_int_eq(ok, true);


	e = ft_iphashmap_get_ip6(&iphmap, addr4_1);
	ck_assert_ptr_eq(e, NULL);

	ok = ft_iphashmap_pop_ip6(&iphmap, addr4_1, &data);
	ck_assert_int_eq(ok, false);

	e = ft_iphashmap_add_ip6(&iphmap, addr4_1);
	ck_assert_ptr_ne(e, NULL);

	e1 = ft_iphashmap_get_ip6(&iphmap, addr4_1);
	ck_assert_ptr_eq(e1, e);

	ok = ft_iphashmap_pop_ip6(&iphmap, addr4_1, &data);
	ck_assert_int_eq(ok, true);

	ok = ft_iphashmap_pop_ip6(&iphmap, addr4_1, &data);
	ck_assert_int_eq(ok, false);

	e = ft_iphashmap_get_ip6(&iphmap, addr4_1);
	ck_assert_ptr_eq(e, NULL);


	e = ft_iphashmap_get_ip6(&iphmap, addr4_2);
	ck_assert_ptr_eq(e, NULL);

	e = ft_iphashmap_add_ip6(&iphmap, addr4_2);
	ck_assert_ptr_ne(e, NULL);

	e1 = ft_iphashmap_get_ip6(&iphmap, addr4_2);
	ck_assert_ptr_eq(e1, e);


	ft_iphashmap_fini(&iphmap);
}
END_TEST


START_TEST(ft_iphashmap_str_utest)
{
	bool ok;
	struct ft_iphashmap iphmap;
	struct ft_iphashmap_entry * e;


	ok = ft_iphashmap_init(&iphmap, 4);
	ck_assert_int_eq(ok, true);

	e = ft_iphashmap_add_p(&iphmap, AF_INET, "1.2.3.4");
	ck_assert_ptr_ne(e, NULL);

	e = ft_iphashmap_add_p(&iphmap, AF_INET6, "1.2.3.4");
	ck_assert_ptr_eq(e, NULL); // Invalid family

	e = ft_iphashmap_add_p(&iphmap, AF_INET6, "::1");
	ck_assert_ptr_ne(e, NULL);

	e = ft_iphashmap_add_p(&iphmap, AF_INET, "::11");
	ck_assert_ptr_eq(e, NULL); // Invalid family


	e = ft_iphashmap_add_p(&iphmap, AF_UNSPEC, "::11");
	ck_assert_ptr_ne(e, NULL);

	e = ft_iphashmap_add_p(&iphmap, AF_UNSPEC, "1.2.3.5");
	ck_assert_ptr_ne(e, NULL);

	e = ft_iphashmap_add_p(&iphmap, AF_UNSPEC, "xxxx");
	ck_assert_ptr_eq(e, NULL);


	ft_iphashmap_fini(&iphmap);
}
END_TEST


START_TEST(ft_iphashmap_load_utest)
{
	bool ok;
	struct ft_iphashmap iphmap;
	ssize_t count;

	ok = ft_iphashmap_init(&iphmap, 10000);
	ck_assert_int_eq(ok, true);

	count = ft_iphashmap_load(&iphmap, "utest_iphashmap_ips.txt");
	ck_assert_int_eq(count, iphmap.count);
	ck_assert_int_gt(count, 500);

	FILE * f = fopen("utest_iphashmap_ips.txt", "r");
	ck_assert_ptr_ne(f, NULL);

	char line[256];
	while (fgets(line, sizeof(line), f))
	{
		char * c = line;
		while (*c != '\0')
		{
			if (*c == '\n') *c = '\0';
			if (*c == '\r') *c = '\0';
			c += 1;
		}
		ok = ft_iphashmap_pop_p(&iphmap, AF_UNSPEC, line, NULL);
		ck_assert_int_eq(ok, true);
	}
	fclose(f);

	ck_assert_int_eq(iphmap.count, 0);
	ck_assert_ptr_eq(iphmap.buckets, NULL);

	ft_iphashmap_fini(&iphmap);
}
END_TEST


START_TEST(ft_iphashmap_sockaddr_utest)
{
	bool ok;
	int rc;
	struct ft_iphashmap iphmap;
	struct ft_iphashmap_entry * e, *e1;


	ok = ft_iphashmap_init(&iphmap, 4);
	ck_assert_int_eq(ok, true);

	e = ft_iphashmap_add_p(&iphmap, AF_INET, "127.0.0.1");
	ck_assert_ptr_ne(e, NULL);

	e = ft_iphashmap_add_p(&iphmap, AF_INET6, "::17");
	ck_assert_ptr_ne(e, NULL);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	struct addrinfo * res;

	rc = getaddrinfo("127.0.0.1", "444", &hints, &res);
	ck_assert_int_eq(rc, 0);

	e1 = ft_iphashmap_get_sa(&iphmap, res->ai_addr, res->ai_addrlen);
	ck_assert_ptr_ne(e1, NULL);

	hints.ai_family = AF_INET6;
	rc = getaddrinfo("::17", "444", &hints, &res);
	ck_assert_int_eq(rc, 0);

	e1 = ft_iphashmap_get_sa(&iphmap, res->ai_addr, res->ai_addrlen);
	ck_assert_ptr_ne(e1, NULL);

	ft_iphashmap_fini(&iphmap);
}
END_TEST

START_TEST(ft_iphashmap_unique_utest)
{
	bool ok;
	struct ft_iphashmap iphmap;
	struct ft_iphashmap_entry * e, *e1;

	ok = ft_iphashmap_init(&iphmap, 4);
	ck_assert_int_eq(ok, true);
	ck_assert_int_eq(iphmap.count, 0);

	e = ft_iphashmap_add_p(&iphmap, AF_INET, "127.0.0.1");
	ck_assert_ptr_ne(e, NULL);
	ck_assert_int_eq(iphmap.count, 1);

	e1 = ft_iphashmap_add_p(&iphmap, AF_INET, "127.0.0.1");
	ck_assert_ptr_eq(e, e1);
	ck_assert_int_eq(iphmap.count, 1);

	ft_iphashmap_fini(&iphmap);
}
END_TEST


START_TEST(ft_iphashmap_clear_utest)
{
	bool ok;
	struct ft_iphashmap iphmap;
	ssize_t count;

	ok = ft_iphashmap_init(&iphmap, 10000);
	ck_assert_int_eq(ok, true);

	count = ft_iphashmap_load(&iphmap, "utest_iphashmap_ips.txt");
	ck_assert_int_eq(count, iphmap.count);

	ft_iphashmap_clear(&iphmap);

	ck_assert_int_eq(iphmap.count, 0);
	ck_assert_ptr_eq(iphmap.buckets, NULL);

	count = ft_iphashmap_load(&iphmap, "utest_iphashmap_ips.txt");
	ck_assert_int_eq(count, iphmap.count);

	ft_iphashmap_fini(&iphmap);
}
END_TEST
///

Suite * iphashmap_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("iphashmap");

	tc = tcase_create("core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_iphashmap_ip4_utest);
	tcase_add_test(tc, ft_iphashmap_ip6_utest);
	tcase_add_test(tc, ft_iphashmap_str_utest);
	tcase_add_test(tc, ft_iphashmap_load_utest);
	tcase_add_test(tc, ft_iphashmap_sockaddr_utest);
	tcase_add_test(tc, ft_iphashmap_unique_utest);
	tcase_add_test(tc, ft_iphashmap_clear_utest);

	return s;
}
