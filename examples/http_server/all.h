#ifndef FT_HTTP_SERVER_ALL_H_
#define FT_HTTP_SERVER_ALL_H_

#include <ft.h>
#include "http_parser.h"

// class config
struct config
{
	bool initialized;
	const char * config_file;

	bool ssl_used;
	const char * ssl_key_file;
	const char * ssl_cert_file;
};

// class listen
struct listen
{
	struct ft_list listeners;
};

bool listen_init(struct listen *, struct ft_context * context);
void listen_fini(struct listen *);

bool listen_start(struct listen *);
bool listen_stop(struct listen *);

bool listen_extend_http(struct listen * , struct ft_context * context, const char * value);
bool listen_extend_https(struct listen * , struct ft_context * context, const char * value);

// class connection
struct connection
{
	struct ft_stream stream;
	http_parser http_request_parser;
	struct ft_frame * response_frame;
	bool idling;
};

bool connection_init_http(struct connection * , struct ft_listener * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
bool connection_init_https(struct connection * , struct ft_listener * listening_socket, int fd, const struct sockaddr * peer_addr, socklen_t peer_addr_len);
void connection_fini_list(struct ft_list * list, struct ft_list_node * node);

bool connection_is_closed(struct connection *);

void connection_terminate(struct connection *);

// class app
struct application
{
	struct config config;
	struct ft_context context;
	struct listen listen;
	struct ft_list connections;

	SSL_CTX * ssl_ctx;
};

extern struct application app;

bool application_configure(struct application *);


#endif //FT_HTTP_SERVER_ALL_H_
