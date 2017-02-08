#include "testutils.h"

////

#define SYSLOG_SOCK "/tmp/libft_test_syslog.sock"

////

int strstrcount(const char * string, const char * substring)
{
	int count = -1;
	for(const char *tmp = string; tmp != NULL; tmp = strstr(tmp, substring))
	{
		count++;
		tmp++;
	}
	return count;
}

START_TEST(ft_log_syslog_utest)
{
	bool ok;
	int rc;

	unlink(SYSLOG_SOCK);
	ft_config.log_syslog.address = strdup(SYSLOG_SOCK);

	int infp, outfp, errfp;
	char * cmd[] = {"socat", "-u", "-d", "-v", "unix-recv:" SYSLOG_SOCK, "open:/dev/null", NULL};
	pid_t socat_pid = popen3(&infp, &outfp, &errfp, cmd[0], cmd);
	ck_assert_int_gt(socat_pid, 0);

	bool socat_ready = false;
	for (int i=0; i<100; i+=1)
	{
		if (access(SYSLOG_SOCK, F_OK ) != -1)
		{
			socat_ready = true;
			break;
		}
		usleep(2000);
	}
	ck_assert_int_eq(socat_ready, true);

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	ok = ft_log_syslog_backend_init(&context);
	ck_assert_int_eq(ok, true);

	ft_log_backend_switch(&ft_log_syslog_backend);
	mark_point();

	// Overfill the frame times
	for (int i=0; i<170; i +=1)
	{
		FT_WARN("Warning Syslog %d", i);
		FT_AUDIT("Audit to Syslog %d", i);
	}
	mark_point();

	ft_context_run(&context);
	mark_point();

	ft_log_backend_switch(NULL);
	mark_point();

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 170);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);

	rc = close(infp);
	ck_assert_int_eq(rc,0);

	usleep(20000);

	// Check syslog output
	rc = kill(socat_pid, SIGINT);
	ck_assert_int_eq(rc,0);

	int bufsize = 20*1024*1024;
	char * buffer = malloc(bufsize);
	char * t = buffer;
	int readsize = 0;
	while (true)
	{
		rc = read(errfp, t, (bufsize - readsize));
		if (rc == 0) break;
		readsize += rc;
		t += rc;
	}
	ck_assert_int_gt(readsize,1);

	int stat_loc;
	rc = waitpid(socat_pid, &stat_loc, 0);
	ck_assert_int_eq(rc, socat_pid);

	buffer[readsize] = '\0';

//	printf("%s\n", buffer);

	ck_assert_int_eq(strstrcount(buffer, "Warning Syslog"), 170);
	ck_assert_int_eq(strstrcount(buffer, "Audit to Syslog"), 170);
	ck_assert_int_eq(strstrcount(buffer, " length="), 340);

}
END_TEST

//

START_TEST(ft_log_syslog_sd_utest)
{
	bool ok;
	int rc;

	unlink(SYSLOG_SOCK);
	ft_config.log_syslog.address = strdup(SYSLOG_SOCK);

	int infp, outfp, errfp;
	char * cmd[] = {"socat", "-u", "-d", "-v", "unix-recv:" SYSLOG_SOCK, "open:/dev/null", NULL};
	pid_t socat_pid = popen3(&infp, &outfp, &errfp, cmd[0], cmd);
	ck_assert_int_gt(socat_pid, 0);

	bool socat_ready = false;
	for (int i=0; i<100; i+=1)
	{
		if (access(SYSLOG_SOCK, F_OK ) != -1)
		{
			socat_ready = true;
			break;
		}
		usleep(2000);
	}
	ck_assert_int_eq(socat_ready, true);

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	ok = ft_log_syslog_backend_init(&context);
	ck_assert_int_eq(ok, true);

	ft_log_backend_switch(&ft_log_syslog_backend);
	mark_point();

	const struct ft_log_sd sd_est[] = {
		{"Ca", "1.2.3.5:12345"},
		{"Ct", "GMYDQMRQGIYGCMBS"},
		{NULL}
	};
	FT_AUDIT_SD(sd_est, "EST");

	ft_context_run(&context);
	mark_point();

	ft_log_backend_switch(NULL);
	mark_point();

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);

	rc = close(infp);
	ck_assert_int_eq(rc,0);

	usleep(20000);

	// Check syslog output
	rc = kill(socat_pid, SIGINT);
	ck_assert_int_eq(rc,0);

	int bufsize = 20*1024*1024;
	char * buffer = malloc(bufsize);
	char * t = buffer;
	int readsize = 0;
	while (true)
	{
		rc = read(errfp, t, (bufsize - readsize));
		if (rc == 0) break;
		readsize += rc;
		t += rc;
	}
	ck_assert_int_gt(readsize,1);

	int stat_loc;
	rc = waitpid(socat_pid, &stat_loc, 0);
	ck_assert_int_eq(rc, socat_pid);

	buffer[readsize] = '\0';

	printf("%s\n", buffer);
}
END_TEST


START_TEST(ft_log_syslog_format_utest)
{
	bool ok;
	int rc;

	unlink(SYSLOG_SOCK);
	ft_config.log_syslog.address = strdup(SYSLOG_SOCK);

	int infp, outfp, errfp;
	char * cmd[] = {"socat", "-u", "-d", "-v", "unix-recv:" SYSLOG_SOCK, "open:/dev/null", NULL};
	pid_t socat_pid = popen3(&infp, &outfp, &errfp, cmd[0], cmd);
	ck_assert_int_gt(socat_pid, 0);

	bool socat_ready = false;
	for (int i=0; i<100; i+=1)
	{
		if (access(SYSLOG_SOCK, F_OK ) != -1)
		{
			socat_ready = true;
			break;
		}
		usleep(2000);
	}
	ck_assert_int_eq(socat_ready, true);

	struct ft_context context;
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	ok = ft_log_syslog_backend_init(&context);
	ck_assert_int_eq(ok, true);

	ft_log_backend_switch(&ft_log_syslog_backend);
	mark_point();

	ft_log_syslog_set_format('5');
	FT_INFO("Test syslog format RFC5424");

	ft_log_syslog_set_format('3');
	FT_INFO("Test syslog format RFC3164");

	ft_log_syslog_set_format('C');
	FT_INFO("Test syslog format 'C'");

	mark_point();

	ft_context_run(&context);
	mark_point();

	ft_log_backend_switch(NULL);
	mark_point();

	ft_context_fini(&context);

	ck_assert_int_eq(ft_log_stats.warn_count, 0);
	ck_assert_int_eq(ft_log_stats.error_count, 0);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);

	rc = close(infp);
	ck_assert_int_eq(rc,0);

	usleep(20000);

	// Check syslog output
	rc = kill(socat_pid, SIGINT);
	ck_assert_int_eq(rc,0);

	int bufsize = 20*1024*1024;
	char * buffer = malloc(bufsize);
	char * t = buffer;
	int readsize = 0;
	while (true)
	{
		rc = read(errfp, t, (bufsize - readsize));
		if (rc == 0) break;
		readsize += rc;
		t += rc;
	}
	ck_assert_int_gt(readsize,1);

	int stat_loc;
	rc = waitpid(socat_pid, &stat_loc, 0);
	ck_assert_int_eq(rc, socat_pid);

	buffer[readsize] = '\0';

	//<133>1 2017-02-08T01:10:29.273Z Aless-MBP.lan  28031 - [l@47278 l="INFO"] Test syslog format RFC5424
	//<133>Feb 8 02:10:29 Aless-MBP [28031]: [t="2017-02-08T02:10:29.273Z"] INFO: Test syslog format RFC3164
	//<133>Feb  8 01:10:29 [28031]: [l@47278 t="2017-02-08T01:10:29.273Z" l="INFO"] Test syslog format 'C'
	//printf("%s\n", buffer);

}
END_TEST
//

Suite * log_syslog_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("log-syslog");

	tc = tcase_create("log-syslog-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, ft_log_syslog_utest);
	tcase_add_test(tc, ft_log_syslog_sd_utest);
	tcase_add_test(tc, ft_log_syslog_format_utest);

	return s;
}
