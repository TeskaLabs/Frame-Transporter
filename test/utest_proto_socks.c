#include "testutils.h"

////

struct ft_proto_socks_delegate proto_socks_error_utest_delegate;


START_TEST(proto_socks_error_utest)
{
	bool ok;
	int rc;

	struct ft_context context;
	struct ft_stream stream;
	struct ft_proto_socks protocol;

	// Initialize a context
	ok = ft_context_init(&context);
	ck_assert_int_eq(ok, true);

	// Initialize a stream socket
	int socket_vector[2];
	rc = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_vector);
	ck_assert_int_eq(rc, 0);

	ok = ft_stream_init(&stream, &ft_stream_proto_socks_delegate, &context, socket_vector[0]);
	ck_assert_int_eq(ok, true);

	// Initialize a protocol
	ok = ft_proto_socks_init(&protocol, &proto_socks_error_utest_delegate, &stream);
	ck_assert_int_eq(ok, true);
	stream.base.socket.protocol = &protocol;

	// Setup the test
	write(socket_vector[1], "xxxxxx", 5);

	// Run
	ft_context_run(&context);

	// Clean up
	ft_proto_socks_fini(&protocol);
	ft_stream_fini(&stream);

	close(socket_vector[1]);

	ck_assert_int_eq(ft_log_stats.warn_count, 1);
	ck_assert_int_eq(ft_log_stats.error_count, 1);
	ck_assert_int_eq(ft_log_stats.fatal_count, 0);
	
}
END_TEST


void proto_socks_error_utest_on_error(struct ft_stream * stream)
{
	FT_ERROR("ERROR!!!XXXX");
//	struct sock_relay * relay = (struct sock_relay *)established_sock->base.socket.data;
//	ft_stream_cntl(&relay->in, FT_STREAM_WRITE_SHUTDOWN);
//	if (relay->out.base.socket.clazz != NULL)
//		ft_stream_cntl(&relay->out, FT_STREAM_WRITE_SHUTDOWN);
}


static bool proto_socks_error_utest_on_connect(struct ft_stream * stream, char addrtype, char * host, unsigned int port)
{
	return true;
}


struct ft_proto_socks_delegate proto_socks_4_utest_delegate = {
	.connect = proto_socks_error_utest_on_connect,
	.error = proto_socks_error_utest_on_error
};


///

Suite * proto_socks_tsuite(void)
{
	TCase *tc;
	Suite *s = suite_create("proto_socks");

	tc = tcase_create("proto_socks-core");
	suite_add_tcase(s, tc);
	tcase_add_test(tc, proto_socks_error_utest);

	return s;
}
