#include "testutils.h"

/* Use:

CK_RUN_CASE="base32-core" make
CK_RUN_SUITE="base32" make

*/

Suite * base32_tsuite(void);
Suite * fd_tsuite(void);
Suite * boolean_tsuite(void);
Suite * config_tsuite(void);
Suite * log_tsuite(void);
Suite * daemon_tsuite(void);
Suite * sock_listen_tsuite(void);
Suite * sock_stream_tsuite(void);
Suite * sock_dgram_tsuite(void);
Suite * sock_uni_tsuite(void);
Suite * fpool_tsuite(void);
Suite * frame_tsuite(void);
Suite * ft_context_tsuite(void);
Suite * ft_loadstore_tsuite(void);
Suite * proto_socks_tsuite(void);

///

Suite * sanity_tsuite(void)
{
	Suite *s = suite_create("sanity");
	return s;
}

int main()
{
	int number_failed;
	
	srand(time(NULL) + getpid());
	SRunner *sr = srunner_create(sanity_tsuite());

	srunner_add_suite(sr, config_tsuite());
	srunner_add_suite(sr, base32_tsuite());
	srunner_add_suite(sr, fd_tsuite());
	srunner_add_suite(sr, boolean_tsuite());
	srunner_add_suite(sr, log_tsuite());
	srunner_add_suite(sr, daemon_tsuite());
	srunner_add_suite(sr, sock_listen_tsuite());
	srunner_add_suite(sr, sock_stream_tsuite());
	srunner_add_suite(sr, sock_dgram_tsuite());
	srunner_add_suite(sr, sock_uni_tsuite());
	srunner_add_suite(sr, fpool_tsuite());
	srunner_add_suite(sr, frame_tsuite());
	srunner_add_suite(sr, ft_context_tsuite());
	srunner_add_suite(sr, ft_loadstore_tsuite());
	srunner_add_suite(sr, proto_socks_tsuite());

	srunner_run_all(sr, CK_VERBOSE /*CK_NORMAL*/);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
