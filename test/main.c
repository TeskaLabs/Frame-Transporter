#include <stdlib.h>
#include <check.h>
#include <unistd.h>
#include <time.h>

/* Use:

CK_RUN_CASE="base32-core" make test
CK_RUN_SUITE="base32" make test

*/

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

	srunner_run_all(sr, CK_VERBOSE /*CK_NORMAL*/);
	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
