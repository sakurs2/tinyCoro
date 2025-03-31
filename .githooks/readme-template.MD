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

tinyCoro是一个linux系统环境下的以**C++20协程技术和linux io_uring技术相结合**的高性能异步协程库，其部分代码参考[libcoro](https://github.com/jbaldwin/libcoro)，意图为开发者提供较异步io更便捷，性能更优的库支持。高效且全能的io_uring和C++20无栈协程的轻量级切换相组合使得tinyCoro可以轻松应对I/O密集型负载，而C++20协程的特性使得用户可以以同步的方式编写异步执行的代码，大大降低了后期维护的工作量，且代码逻辑非常简单且清晰，除此外**tinyCoro还提供了协程安全组件，以协程suspend代替线程阻塞便于用户构建协程安全且高效的代码。** 下图展示了tinyCoro的任务调度机制：

![tinycoro调度机制](./resource/tinycoro_core.png)

tinyCoro的设计并不复杂，每个执行引擎拥有一个工作线程和io_uring实例来处理外部任务，而调度器通过创建多个执行引擎来提高系统的并发能力，设计框架图如下所示：

![tinyCoro框架图](./resource/tinycoro_arch.png)

经过测试由tinyCoro实现的echo server在1kbyte负载和100个并发连接下可达到100wQPS。

## tinyCoroLab

为了使更多的同学了解C++协程和liburing，tinyCoro作者参照cmu15445 bustub研发了[tinyCoroLab](https://github.com/sakurs2/tinyCoroLab)课程，5大实验搭配精心编写的功能、内存安全以及性能测试，并配备内容丰富的实验文档，助力实验者对C++编程、异步编程和多线程编程技术更上一层楼！

TODO:添加文档部分

tinyCoroLab实验课程完全免费！！！快点击[tinyCoroLab](https://github.com/sakurs2/tinyCoroLab)开启属于你的tinyCoro之旅吧！

## Overview

- [tinyCoro C++20 coroutine library with liburing](#tinycoro-c20-coroutine-library-with-liburing)
  - [tinyCoroLab](#tinycorolab)
  - [Overview](#overview)
  - [Usage](#usage)
    - [event](#event)
    - [latch](#latch)
    - [wait\_group](#wait_group)
    - [mutex](#mutex)
    - [when\_all](#when_all)
    - [condition\_variable](#condition_variable)
    - [channel](#channel)
    - [tcp\_stdin\_echo\_client](#tcp_stdin_echo_client)
    - [tcp\_echo\_server](#tcp_echo_server)
  - [benchmark](#benchmark)
    - [实验环境](#实验环境)
      - [wsl2](#wsl2)
      - [ubuntu](#ubuntu)
    - [测试结果](#测试结果)
      - [128b-avg(req/s)](#128b-avgreqs)
      - [1k-avg(req/s)](#1k-avgreqs)
      - [16k-avg(req/s)](#16k-avgreqs)
  - [build](#build)
    - [构建环境](#构建环境)
    - [构建流程](#构建流程)
    - [cmake options](#cmake-options)
  - [support](#support)

## Usage

tinyCoro其本身不仅具有高效的任务执行机制，还额外实现了高效的协程同步组件，当线程因线程同步组件陷入阻塞态时会让出执行权，而通过使用协程同步组件，线程当前执行的协程如陷入阻塞，那么线程会转而执行下一个任务，进而实现对cpu的充分利用。

### event

event用于同步协程，只有event处于set状态时对event调用`wait`的协程才可以恢复执行，否则陷入suspend状态。event的模板参数非空情况下还可以用于通过`set`设置值并由`wait`将该值返回。

```C++
${EXAMPLE_CORO_EVENT}
```

### latch

latch与C++ std::latch功能相同，用于同步协程，只有当其内部计数通过`count_down`降至0时对其调用`wait`的协程才可以恢复运行，否则陷入suspend状态。

```C++
${EXAMPLE_CORO_LATCH}
```

### wait_group

wait_group的设计参考golang中的wait_group，与latch相比在其内部计数可以被增加，因此即使计数通过`done`降至0而又通过`add`增至1，那么此时对其调用`wait`的协程均会陷入suspend状态。

```C++
${EXAMPLE_CORO_WAIT_GROUP}
```

### mutex

mutex与C++中的mutex功能一致，如果协程试图获取一把处于加锁状态的锁，那么将陷入suspend状态，通过mutex保证协程并发安全性。

```C++
${EXAMPLE_CORO_MUTEX}
```

### when_all

when_all用于等待其列表中的协程全部执行完毕，在等待时会陷入suspend状态。

```C++
${EXAMPLE_CORO_WHEN_ALL}
```

### condition_variable

condition_variable与C++ std::condition_variable功能一致，并且tinyCoro的condition_variable搭配的是tinyCoro中的mutex，利用condition_variable可以构建协程安全的生产者消费者模型以及同步协程执行的有序性等功能。

```C++
${EXAMPLE_CORO_CONDITION_VARIABLE}
```

### channel

channel的设计参考golang channel，其本身为容量固定的阻塞式协程安全的多生产者多消费者模型，所有协程因对channel操作而阻塞时会陷入suspend状态。

```C++
${EXAMPLE_CORO_CHANNEL}
```

### tcp_stdin_echo_client

由tinyCoro实现的tcp客户端，不仅可以打印tcp服务端发来的消息，用户还能在终端输入文字来向tcp服务端发送消息。

```C++
${EXAMPLE_CORO_TCP_STDIN_CLIENT}
```

### tcp_echo_server

由tinyCoro实现的tcp服务端。

```C++
${EXAMPLE_CORO_TCP_ECHO_SERVER}
```

## benchmark

在liburing 2.10版本下对由tinyCoro实现的tcp_echo_server进行压测，压测工具使用[rust_echo_bench](https://github.com/haraldh/rust_echo_bench)，基线模型选用[rust_echo_server](https://github.com/haraldh/rust_echo_server)。

### 实验环境

benchmark分别会在wsl2和ubuntu物理机中测算。

#### wsl2

- **操作系统版本：** Ubuntu 22.04.5 LTS
- **内核版本：** 5.15.167.4-microsoft-standard-WSL2
- **系统详细信息：** Linux DESKTOP-59OORP1 5.15.167.4-microsoft-standard-WSL2 #1 SMP Tue Nov 5 00:21:55 UTC 2024 x86_64 x86_64 x86_64 GNU/Linux
- **内存大小：** 7.4Gi

#### ubuntu

- **操作系统版本：** Ubuntu 24.04.2 LTS
- **内核版本：** 6.8.0-55-generic
- **系统详细信息：** Linux ubuntu 6.8.0-55-generic #57-Ubuntu SMP PREEMPT_DYNAMIC Wed Feb 12 23:42:21 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux
- **内存大小：** 31Gi

### 测试结果

针对不同负载大小和不同并发连接数进行压测。

#### 128b-avg(req/s)

|  | tinycoro(wsl2) | tinycoro(ubuntu) | rust_echo_server(wsl2) | rust_echo_server(ubuntu) |  
|:-------:|:-------:|:-------:|:-------:|:-------:|
| 1   | 15906  | 81228  | 18286  | 88452 |
| 10  | 129679 | 595051 | 144092 | 701498|
| 50  | 1037350| 621318 | 1051087| 683328|
| 100 | 1102234|661939  | 1011170|662700 |

#### 1k-avg(req/s)

|  | tinycoro(wsl2) | tinycoro(ubuntu) | rust_echo_server(wsl2) | rust_echo_server(ubuntu) |  
|:-------:|:-------:|:-------:|:-------:|:-------:|
| 1   | 17082  | 77259  | 17885  | 83227 |
| 10  | 124883 | 586725 | 139094 | 683537|
| 50  | 986684 | 616243 | 1000662|669019 |
| 100 | 1034611| 637657 |988743  | 647984|

#### 16k-avg(req/s)

|  | tinycoro(wsl2) | tinycoro(ubuntu) | rust_echo_server(wsl2) | rust_echo_server(ubuntu) |  
|:-------:|:-------:|:-------:|:-------:|:-------:|
| 1   | 15991  | 47988  | read error | read error |
| 10  | 111741 | 518705 | read error | read error |
| 50  | 820905 | 464276 | read error | read error |
| 100 | 817026 | 437072 | read error | read error |

<div style="display: flex; justify-content: space-between; width: 100%;">
  <img src="./resource/benchmark_ubuntu.png" alt="benchmark_ubuntu" style="width: 48%; height: auto;">
  <img src="./resource/benchmark_wsl2.png" alt="benchmark_wsl2" style="width: 48%; height: auto;">
</div>

详细的benchmark过程请参考[benchmark/README.MD](https://github.com/sakurs2/tinyCoroLab/blob/master/benchmark/README.MD)。

## build

### 构建环境

- 操作系统：linux系统，最好是ubuntu新版本，wsl2与虚拟机均可
- 编译器：支持C++20版本的gcc编译器
- cmake：3.15版本以上

### 构建流程

首先克隆项目到本地：

```shell
git clone https://github.com/sakurs2/tinyCoro
```

初始化所有子项目：

```shell
cd tinyCoro
 git submodule update --init --recursive
```

构建liburing

```shell
cd third_party/liburing
./configure --cc=gcc --cxx=g++;
make -j$(nproc);
make liburing.pc
sudo make install;
```

回到项目根目录下执行项目构建：

```shell
mkdir build
cd build
cmake ..
make
```

### cmake options

| 名称 | 默认值 | 作用 |
|:-------:|:-------:|:-------:|
| ENABLE_UNIT_TESTS   | ON  | 允许构建测试  |
| ENABLE_DEBUG_MODE  | OFF | ON代表开启debug模式，否则为release模式 |
| ENABLE_BUILD_SHARED_LIBS  | OFF | 库的默认构建为动态库 |

## support

对于tinyCoro的任何疑问请访问[tinyCoro issurs](https://github.com/sakurs2/tinyCoro/issues)。

新特性添加以及bug修复请访问[tinyCoro pulls](https://github.com/sakurs2/tinyCoro/pulls)。
