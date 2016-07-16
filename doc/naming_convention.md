# Naming

## General

The project name: *Frame transporter*  
Prefix: *ft_*

## Syntax

| Type | Returns | Name | Arguments |
|------|--------------|------|-----------|
| Function | `...` | `ft_<verb>` | `(...)` |
| Macro | `...` | `FT_<VERB>` | `(...)` |
| + specialization | `...` | `ft_<verb>_<spec>` | `(...)` |
| Object |  | `struct ft_<noun>` |
| Class (Not used) |  | `struct ft_class_<noun>` |
| Constructor | `struct ft_<noun> * ` | `ft_<noun>_new` | `(...)` |
| + specialization | `struct ft_<noun> * ` | `ft_<noun>_new_<spec>` | `(...)` |
| Destructor | `void ` | `ft_<noun>_del` | `(struct ft_<noun> * this)` |
| Initialiser | `bool ` | `ft_<noun>_init` | `(struct ft_<noun> * this, ...)` |
| + specialization | `bool ` | `ft_<noun>_init_<spec>` | `(struct ft_<noun> * this, ...)` |
| Finalizer | `void ` | `ft_<noun>_fini` | `(struct ft_<noun> * this)` |
| Factory | `struct ft_<noun> * ` | `ft_<...>_create_<noun>` | `(...)` |
| + specialization | `struct ft_<noun> * ` | `ft_<...>_create_<spec>_<noun>` | `(...)` |
| Methods | `void ` | `ft_<noun>_<verb>` | `(struct ft_<noun> * this, ...)` |
| Delegate | | `struct ft_<noun>_delegate` |
| - member | | `ft_<noun>.delegate` |
 


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



 
