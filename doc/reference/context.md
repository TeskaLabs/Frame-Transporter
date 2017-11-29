# libft reference - Context

libft objects operate within a libft context. The application typically has a one global context (more than one context is supported). The context operates within a single thread.

A libft context consists of:

 - an event loop
 - signal handlers
 - a frame memory pool
 - a shutdown counter

## Context class

`struct ft_context;`

`bool ft_context_init(struct ft_context * );`

Construct a new context object. Returns true for success or false for error.


`void ft_context_fini(struct ft_context * );`

Destructor of a context object.



### Event loop


`void ft_context_run(struct ft_context * );`

Start an event loop (see libev).
The method will return when the event loop exits.


### The example of use

	#include <ft.h>
	
	struct ft_context context;
	
	int main(int argc, char const *argv[])
	{
		bool ok;
		
		// Initialize libft
		ft_initialise();
	
		// Initialize the context
		ok = ft_context_init(&context);
		if (!ok) return EXIT_FAILURE;
		
		// Start the event loop
		ft_context_run(&context);

		// Clean up
		ft_context_fini(&context);

		return EXIT_SUCCESS;
	}

