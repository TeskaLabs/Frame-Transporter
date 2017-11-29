# Object-oriented programming in a libft

libft is written in C and it heavily uses OOP (Object-oriented programming) paradigm.

It uses C standard revision ISO/IEC 9899:2011 aka C11.

libft symbol prefix is `ft_` or uppercase version `FT_`.


## Classes

Declaration of the class:

```
struct ft_class
{
	// members goes here
};
```

Classes are defined as C structures.

No typedef.

Inheritance is not supported.


## Methods

```
bool ft_class_execute(struct ft_class * this, const char * arg, ...);
```

Method name begins with a name of the class.

Method name shall be a _verb_ because methods do something.

Method name can also contain an clarifying postfixes (e.g. `ft_class_run_in_child()`).

Preferred way of reporting errors from methods is to return boolean value `false`.

The first argument of the method must be a pointer to a class instance and it has to be called `this`.

It is recommended to assert non-NULL value of this at the beginning of the method implementation (e.g. `assert(this != NULL);`).

## Attributes

```
struct ft_class
{
	int attribute1;
	const char * attribute2;
};
```

Attributes are direct members of the class.

The name shall be a _noun_.

The name can be extended by an clarifying postfixes.

The members shall be considered as private, so external access should be avoided. Use getters/setters methods instead.


## Subclasses

```
struct ft_class
{

	struct
	{
		int counter_1;
		int counter_2;
	} stats;
	
	struct
	{
		bool flag1:1;
		bool flag2:1;
		bool flag3:2;
	};
};
```

## Constructors

```
bool ft_class_init(struct ft_class * this);

struct ft_class my_instance;

bool ok = ft_class_init(&my_instance);
if (!ok) handle_error();
```

A constructor is a method of a given class that is responsible for initialization of the object instance in a given memory space.

A constructor **doesn't** handle a memory allocation for a object instance. It may however handle a memory allocations for internal members.

A constructor must return `bool` value `true` for successful initialization and `false` for any errors.

The name of the constructor shall contain `init` (e.g. `ft_class_init()`).

There can be more than one constructor for a class, a name postfix is used to clarify a difference. Examples `bool ft_class_init_connect()` and `bool ft_class_init_accept()`.


## Destructor

```
void ft_class_fini(struct ft_class * this);

struct ft_class my_instance;

ft_class_fini(&my_instance);
```

A destructor is a method of a given class that is responsible for a final clean-up at the end of the lifecycle of a object instance.

A destructor **doesn't** handle a memory deallocation for a object instance. It may however handle a memory deallocations for internal members.

There has to be a single destructor.

The name of the destructor shall be `fini` (e.g. `ft_class_fini()`).

The destructor return value must be defined as `void`.


##TODO:

- getters and setters
- constructors, destructors
- allocators, deallocators
- delegates
- strings (char * vs const char *)
