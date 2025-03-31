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
