# libft reference - Context

## `struct ft_context` - Context class

Example:

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



### Methods

#### `bool ft_context_init(struct ft_context * );`

Construct a new context object.  
Returns `true` for success or `false` for error.


#### `void ft_context_fini(struct ft_context * );`



#### `void ft_context_run(struct ft_context * );`


### Globals

#### `extern struct ft_context * ft_context_default;`

Default context.
