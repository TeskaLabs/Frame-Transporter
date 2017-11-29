# Frame transporter aka libft

The high-performance networking library for C and POSIX.

It is designed for implementation of various communication protocols on a client and server side in a event-driven programming paradigm. It means that you can quickly build for example a high-performance web server or MQTT client.

libft is a workhorse behind several enterprise products that see a large-scale production deployments e.g. in a telco segment.

## Features

* [Event-driven](https://en.wikipedia.org/wiki/Event-driven) with the event loop provided by [libev](http://software.schmorp.de/pkg/libev.html)
* [Network socket](https://en.wikipedia.org/wiki/Network_socket) abstraction (IPv4, IPv6, Unix sockets)
* Non-blocking I/O, [asynchronous I/O](https://en.wikipedia.org/wiki/Asynchronous_I/O)
* SSL/TLS encryption by [OpenSSL](https://www.openssl.org)
* [Reactor pattern](https://en.wikipedia.org/wiki/Reactor_pattern)
* Fixed-size block memory pool
* Procedural wire protocol parsing and constructing
* Tracing, logging and auditing


## Copyright and License

Copyright (c) 2016-2017, TeskaLabs Ltd, [Ales Teska](https://github.com/ateska)  
The project is released under the BSD license.

---
[![Build Status](https://travis-ci.org/TeskaLabs/Frame-Transporter.svg?branch=master)](https://travis-ci.org/TeskaLabs/Frame-Transporter)
[![codecov](https://codecov.io/gh/TeskaLabs/Frame-Transporter/branch/master/graph/badge.svg)](https://codecov.io/gh/TeskaLabs/Frame-Transporter)
[![Coverity](https://scan.coverity.com/projects/9946/badge.svg)](https://scan.coverity.com/projects/teskalabs-frame_transporter)
