# Naming

## General

The project name: *Frame transporter*  
Prefix: *ft_*

## Syntax

| Type | Returns | Name | Arguments |
|------|--------------|------|-----------|
| Function | ... | ft\_[verb] | (...) |
| Macro | ... | FT\_[VERB] | (...) |
| + specialization | ... | ft\_[verb]\_[spec] | (...) |
| Object |  | struct ft\_[noun] |
| Class |  | struct ft\_[noun]\_class |
| Constructor | struct ft\_[noun] \* | ft\_[noun]\_new | (...) |
| + specialization | struct ft\_[noun] \*  | ft\_[noun]\_new\_[spec] | (...) |
| Destructor | void | ft\_[noun]\_del | (struct ft\_[noun] \* this) |
| Initialiser | bool | ft\_[noun]\_init | (struct ft\_[noun] \* this, ...) |
| + specialization | bool | ft\_[noun]\_init\_[spec] | (struct ft\_[noun] \* this, ...) |
| Finalizer | void | ft\_[noun]\_fini | (struct ft\_[noun] \* this) |
| Factory | struct ft\_[noun2] \* | ft\_[noun]\_create\_[noun2] | (...) |
| + specialization | struct ft\_[noun2] * | ft\_[noun]\_create\_[spec]\_[noun2] | (...) |
| Method | ... | ft\_[noun]\_[verb] | (struct ft\_[noun] \* this, ...) |
| + specialization | ... | ft\_[noun]\_[verb]\_[spec] | (struct ft\_[noun] \* this, ...) |
| Getter | ... | ft\_[noun]\_get\_[subject] | (struct ft\_[noun] \* this) |
| + boolean | bool | ft\_[noun]\_is\_[subject] | (struct ft\_[noun] \* this) |
| Setter | void | ft\_[noun]\_set\_[subject] | (struct ft\_[noun] \* this, ... value) |
| Counter | int / size_t / ... | ft\_[noun]\_count\_[subject] | (struct ft\_[noun] \* this) |
| Delegate | | struct ft\_[noun]\_delegate |
| - member | | ft\_[noun].delegate |


### Objects

C structures.  

See [Object Lifetime](https://en.wikipedia.org/wiki/Object_lifetime) for more info.


### Constructor

Returns a (usually) unpopulated object, dynamically allocated within on the heap or other memory.


### Destructor

Free an allocated space for the object.


### Initialiser

An initializer is code that runs on an object after a constructor or static allocation and can be used to succinctly set any number of fields on the object to specified values. The setting of these fields occurs after the constructor is called.


### Finalizer

A finalizer is executed during object destruction, prior to the object being deallocated, and is complementary to an initializer.


### Delegate

TODO

 