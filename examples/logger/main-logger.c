#include <stdlib.h>
#include <stdbool.h>

#include <ft.h>

///

int main(int argc, char const *argv[])
{
	bool ok;
	struct ft_context context;

	// compile message to print
	int msglen = 1;
	for(int i=1; i<argc; i+=1)
	{
		msglen += 1;
		msglen += strlen(argv[i]);
	}

	if (msglen == 1) return EXIT_SUCCESS;

	char msg[msglen];
	char * c = msg;
	for(int i=1; i<argc; i+=1)
	{
		strcpy(c, argv[i]);
		c += strlen(argv[i]);
		*c = ' ';
		c += 1;
		*c = '\0';
	}	

	ft_initialise();
	
	// Initialize a context
	ok = ft_context_init(&context);
	if (!ok) return EXIT_FAILURE;

	ok = ft_log_syslog_backend_init(&context);
	if (!ok) return EXIT_FAILURE;	

	ft_log_backend_switch(&ft_log_syslog_backend);

	FT_INFO("%s", msg);

	// Enter event loop
	ft_context_run(&context);

	// Clean up
	ft_context_fini(&context);

	return EXIT_SUCCESS;
}


