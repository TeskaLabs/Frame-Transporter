#ifndef FT_PROTO_SOCKS_H_
#define FT_PROTO_SOCKS_H_

extern struct ft_stream_delegate ft_stream_proto_socks_delegate;

struct ft_proto_socks_delegate
{
	/*
	*  addrtype:
	*           - '4' - IPv4
	*           - '6' - IPv6
	*           - 'D' - Domain name (DNS name)
	*
	*  Returns:
	*           - True = connection has been processed, it is asynchronous operation, call 'ft_proto_socks_stream_send_final_response' when connection is really established)
	*           - False = connection failed (error responses will be sent and socket will be closed)
	*/
	bool (*connect)(struct ft_stream * stream, char addrtype, char * host, unsigned int port);

	void (*error)(struct ft_stream * stream); // Don't use this for close() or shutdown, it will be done automatically and it can lead to wierd results
};

struct ft_proto_socks
{
	// Common fields
	const struct ft_proto_socks_delegate * delegate;

	//SOCKS4 headers
	uint8_t VN;
	uint8_t CD; // or CMD in SOCKS5
	uint16_t DSTPORT; // or CMD in SOCKS5
	uint32_t DSTIP;

	//SOCKS5 headers
	bool authorized;
	uint8_t SELMETHOD;
	uint8_t ATYP;

	void * data;
};

bool ft_proto_socks_init(struct ft_proto_socks *, const struct ft_proto_socks_delegate * delegate, struct ft_stream * stream);
void ft_proto_socks_fini(struct ft_proto_socks *);

// It is advised to switch to new protocol delegate on the stream after this one is called
// reply is from SOCKS5, 0 means success, anything else failure - it is handled properly for SOCKS4 too
bool ft_proto_socks_stream_send_final_response(struct ft_stream * stream, int reply);

#endif //FT_PROTO_SOCKS_H_
