#include "_ft_internal.h"

const char * ft_sockaddr_str(const struct sockaddr * client_addr, socklen_t client_addr_len)
{
	static __thread char addrbuf[1024] = "-";	

	if (client_addr == NULL)
	{
		strcpy(addrbuf, "(null)");
		return addrbuf;
	}

	switch (client_addr->sa_family)
	{
		case AF_INET:
		case AF_INET6:
		{
			char hoststr[128];
			char portstr[16];
			int rc = getnameinfo(
				client_addr, client_addr_len,
				hoststr, sizeof(hoststr),
				portstr, sizeof(portstr), 
				NI_NUMERICHOST | NI_NUMERICSERV
			);
			if (rc != 0)
				snprintf(addrbuf, sizeof(addrbuf)-1, "(AF_INET/%s)", gai_strerror(rc));
			else
			{
				if (client_addr->sa_family == AF_INET6)
					snprintf(addrbuf, sizeof(addrbuf)-1, "[%s]:%s", hoststr, portstr);
				else
					snprintf(addrbuf, sizeof(addrbuf)-1, "%s:%s", hoststr, portstr);
			}
			break;
		}

		//TODO: AF_LOCAL

		default:
		{
			snprintf(addrbuf, sizeof(addrbuf)-1, "(family:%d)", client_addr->sa_family);
		}

	}

	return addrbuf;
}
