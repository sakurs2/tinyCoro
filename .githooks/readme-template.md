<p align="center">
  <img src="./resource/tinycoro.png" alt="tinycoro Logo" height="200">
</p>

-----------------

# tinyCoro C++20 coroutine library with liburing

[![C++20](https://img.shields.io/badge/dialect-C%2B%2B20-blue)](https://en.cppreference.com/w/cpp/20)[![MIT license](https://img.shields.io/github/license/max0x7ba/atomic_queue)](https://github.com/sakurs2/tinyCoro/blob/master/LICENSE)
![platform Linux x86_64](https://img.shields.io/badge/platform-Linux%20x86_64--bit-yellow)
<!-- support below -->
<!-- [![Build Status](link/actions/workflows/cmake.yml/badge.svg)](link/actions/workflows/cmake.yml) -->
<!-- [![CI](https://github.com/jbaldwin/libcoro/actions/workflows/ci-coverage.yml/badge.svg)](https://github.com/jbaldwin/libcoro/actions/workflows/ci-coverage.yml)
[![Coverage Status](https://coveralls.io/repos/github/jbaldwin/libcoro/badge.svg?branch=main)](https://coveralls.io/github/jbaldwin/libcoro?branch=main)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/c190d4920e6749d4b4d1a9d7d6687f4f)](https://www.codacy.com/gh/jbaldwin/libcoro/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=jbaldwin/libcoro&amp;utm_campaign=Badge_Grade) -->

tinyCoro是一个linux系统环境下的以C++20协程技术和linux io_uring技术相结合的高性能异步协程库，其部分代码参考[libcoro](https://github.com/jbaldwin/libcoro)，意图为开发者提供较异步io更便捷，性能更优的库支持。高效且全能的io_uring和C++20无栈协程的轻量级切换相组合使得tinyCoro可以轻松应对I/O密集型负载，而C++20协程的特性使得用户可以以同步的方式编写异步执行的代码，大大降低了后期维护的工作量，且代码逻辑非常简单且清晰，下图展示了tinyCoro的任务调度机制：

![tinycoro调度机制](./resource/tinycoro_core.png)

tinyCoro的设计并不复杂，每个执行引擎拥有一个工作线程和io_uring实例来处理外部任务，而调度器通过创建多个执行引擎来提高系统的并发能力，设计框架图如下所示：

![tinyCoro框架图](./resource/tinycoro_arch.png)

经过测试由tinyCoro实现的echo server在1kbyte负载和100个并发连接下可达到100wQPS。

## Overview

## Usage
