#ifndef FT_SOCK_UNI_H_
#define FT_SOCK_UNI_H_

union ft_uni_socket
{
	struct ft_socket socket_base;
	struct ft_stream stream; // stream	
	struct ft_dgram dgram; // dgram	
};


static inline void ft_uni_socket_fini(union ft_uni_socket * this)
{
	assert(this != NULL);
	if (this->socket_base.clazz == ft_dgram_class)
	{
		return ft_dgram_fini(&this->dgram);
	}
	else if (this->socket_base.clazz == ft_stream_class)
	{
		return ft_stream_fini(&this->stream);
	}

	FT_ERROR_P("Unknown/unsupported class '%s'", this->socket_base.clazz);
}

#endif // FT_SOCK_UNI_H_
