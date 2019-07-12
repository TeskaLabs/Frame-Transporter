# Frame transporter aka libft

High-performance asynchronous I/O written in C aimed for POSIX (e.g. Linux and macOS) and Windows.

The _libft_ is designed for easy implementation of common network protocols on top of event-driven programming paradigm.
It means that you can quickly build for example a high-performance web server, MQTT client and so on.

The _libft_ is a workhorse behind several enterprise products that see a large-scale production deployments e.g. in a telco segment.


## Features

* [Event-driven](https://en.wikipedia.org/wiki/Event-driven) with the event loop provided by [libev](http://software.schmorp.de/pkg/libev.html) on POSIX and [IOCP](https://en.wikipedia.org/wiki/Input/output_completion_port) on Windows
* Non-blocking I/O, [asynchronous I/O](https://en.wikipedia.org/wiki/Asynchronous_I/O), [reactor pattern](https://en.wikipedia.org/wiki/Reactor_pattern)
* SSL/TLS encryption by [OpenSSL](https://www.openssl.org)
* Streamers (Streams such as TCP)
* Messengers (Datagrams such as UDP)
* Timers
* Signals (POSIX)
* [Memory pool](https://en.wikipedia.org/wiki/Memory_pool) with fixed-size blocks
* Protocol stacks
* Logging
* Configuration
* Strictly single threaded


### Why another asynchronous I/O library?

We used for a long time an excellent [libev](http://software.schmorp.de/pkg/libev.html) but it is not well portable to Windows, because it lacks IOCP.  
Then there is a famous [libuv](https://libuv.org) but it is already rather complex and it employs the proactor pattern and hence _libuv_ is multi threaded.


The _libft_ is designed to provide a higher level of abstraction than _libev_, which offers an event loop.
The _libft_ provides streams, messages (or datagrams), memory pool and other tools needed for asynchronous I/O.
It is also designed to run on POSIX systems as well as on Windows.


## Copyright and License

Copyright (c) 2016-2019, TeskaLabs Ltd, [Ales Teska](https://github.com/ateska)  
The project is released under the BSD license.

