# Frame transporter aka libft

The high-performance networking library for C and POSIX.

## Features

* High-speed event loop provided by [libev](http://software.schmorp.de/pkg/libev.html)
* Event-driven [network socket](https://en.wikipedia.org/wiki/Network_socket) abstraction
* Non-blocking I/O, [asynchronous I/O](https://en.wikipedia.org/wiki/Asynchronous_I/O)
* SSL/TLS encryption by [OpenSSL](https://www.openssl.org)
* [Reactor pattern](https://en.wikipedia.org/wiki/Reactor_pattern)
* Fixed-size block memory pool
* Tracing, logging and auditing

## The concept

![image](./doc/images/stream_socket.png)  

## Copyright and License

Copyright (c) 2016-2017, TeskaLabs Ltd, [Ales Teska](https://github.com/ateska)  
The project is released under the BSD license.

---
[![Build Status](https://travis-ci.org/TeskaLabs/Frame-Transporter.svg?branch=master)](https://travis-ci.org/TeskaLabs/Frame-Transporter)
[![codecov](https://codecov.io/gh/TeskaLabs/Frame-Transporter/branch/master/graph/badge.svg)](https://codecov.io/gh/TeskaLabs/Frame-Transporter)
[![Coverity](https://scan.coverity.com/projects/9946/badge.svg)](https://scan.coverity.com/projects/teskalabs-frame_transporter)
