# libft reference - Context

## Context class

#### `struct ft_context`;

#### `bool ft_context_init(struct ft_context * );`

Construct a new context object. Returns true for success or false for error.


#### `void ft_context_fini(struct ft_context * );`

Destructor of a context object.


### Event loop


#### `void ft_context_run(struct ft_context * );`

Start an event loop (see libev).
The method will return when the event loop exits.


### Default context

#### `extern struct ft_context * ft_context_default;`

Global pointer to a first context object created within the application.


### The example of use

	#include <ft.h>
	
	struct ft_context context;
	
	int main(int argc, char const *argv[])
	{
		bool ok;
		
		// Initialize 
		ft_initialise();
	
		// Initialize context
		ok = ft_context_init(&context);
		if (!ok) return EXIT_FAILURE;
		
		// Enter event loop
		ft_context_run(&context);

		// Clean-up
		ft_context_fini(&context);

		return EXIT_SUCCESS;
	}

