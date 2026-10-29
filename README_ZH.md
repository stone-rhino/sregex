# sregex

<p align="center">
    <img src="https://avatars.githubusercontent.com/u/215498892?s=400&u=8e3433ea2368169c950ca497194776511c8a1837&v=4" width=200 height=200/>
</p>

![GitHub License](https://img.shields.io/github/license/stone-rhino/sregex)

[简体中文] | [English](README.md)

Stone Rhino Regular Expression (sregex) 是一个高性能的多模式正则表达式匹配的 C++ 库。
sregex 的设计理念是解决 [hyperscan](https://github.com/intel/hyperscan) 不支持部分匹配，而 [PCRE2](https://github.com/PCRE2Project/pcre2) 在多模式部分匹配时性能较低的问题。 在流式数据防泄漏、敏感信息过滤等场景中，多模式的流式脱敏能力尤为重要， sregex 能够提供高性能的多模式正则部分匹配能力，协同 hyperscan、PCRE2 等库使用，能够在不同场景下实现高效的多模式正则匹配。在该项目提供的一个[示例](examples/streaming_redaction_with_dfa.cc)中，展示了如何使用 sregex 协同其他库实现流式数据的多模式脱敏。

## 功能列表

- 高性能的多模式正则匹配、部分匹配能力
- 可与 hyperscan、PCRE2 等库协同，支持流式数据的多模式脱敏
- 适用于数据防泄漏等、敏感信息过滤场景
- 提供正则表达式高级中间表示(HIR, High-level Intermediate Representation)，开发者可以基于 HIR 开发自定义的正则匹配逻辑

## 性能对比

性能基准测试的结果由项目提供的[基准测试程序](benches/sregex_pcre2_benchmark.cc)生成，展示了 sregex 在多模式正则匹配和部分匹配场景下的性能表现。
以下是基准测试的部分结果：

| 场景 | sregex(DFA) 耗时(ns) | PCRE2 耗时(ns) | PCRE2(JIT) 耗时(ns) |
|------|--------|-------|-------|
| 单模式-全匹配(常量字符串) | 101 | 146 | **25.5** 🟢 |
| 单模式-全匹配(邮箱格式) | 379 | 589 | **243** 🟢 |
| 单模式-全匹配(URL格式) | 77.6 | 366 | **79.3** 🟢 |
| 单模式-全匹配(TOKEN格式) | **4.64** 🟢 | 72.3 | 39.0 |
| 单模式-全匹配(重复字符串) | 6352 | 106478 | **5911** 🟢 |
| 单模式-全匹配(日志格式) | **51.8** 🟢 | 273 | 94.4 |
| 单模式-部分匹配(量词) | 34.8 🟢 | 226(soft)<br>87.6(hard) | 232(soft)<br>93.4(hard) |
| 单模式-部分匹配(分支) | 40 🟢 | 467(soft)<br>398(hard) | 495(soft)<br>389(hard) |
| 单模式-部分匹配(重复) | 250 🟢 | 22300(soft)<br>2903(hard) | 23010(soft)<br>2930(hard) |
| 多模式-全匹配 | 11.4 🟢 | 194(alternation)<br>486(per pattern) | 68.7(alternation)<br>197(per pattern) |
| 多模式-部分匹配 | 97.9 🟢 | 13834(alternation-soft)<br>5560(alternation-hard)<br>2475(per pattern-soft)<br>2378(per pattern-hard) | 14256(alternation-soft)<br>5527(alternation-hard)<br>2717(per pattern-soft)<br>2370(per pattern-hard) |

从基准测试结果可以看出，sregex 在多模式正则匹配和部分匹配场景下表现出色，尤其在多模式部分匹配的性能上明显优于 PCRE2，性能最多可提升数十倍。对于单模式全匹配，PCRE2 的 JIT 模式在某些场景下仍具有优势，但 sregex 的设计初衷并不是针对单模式全匹配进行优化，而是专注于多模式部分匹配的高性能实现。

## 快速开始

### 编译环境
* CMake 3.14 或更高版本 https://cmake.org/download/
* vcpkg (使用 cmake 安装) https://github.com/microsoft/vcpkg
* 支持 C++23 的编译器 (GCC 13.1 或更高版本) https://gcc.gnu.org/
* JDK 21 或更高版本
```shell
apt install openjdk-21-jdk-headless
```

### 构建

项目提供了 [CMake 预设(presets)](CMakePresets.json) 来简化构建过程，开发者可以通过指定不同的预设来构建库文件、基准测试、示例程序以及调试版本。
如果需要指定编译器的路径，可以将 `CMakeUserPresets.json.example` 复制为 `CMakeUserPresets.json`，并在该文件中修改编译器路径，例如：
```json
{
    "name": "my-release",
    "inherits": "release",
    "environment": {
    "CC": "/usr/local/bin/gcc",
    "CXX": "/usr/local/bin/g++",
    "LD_LIBRARY_PATH": "/usr/local/lib64"
    }
}
```
然后使用指定的 CMake 预设：
```shell
cmake --preset=my-release --fresh
```

#### 构建库文件

- 配置 CMake
```shell
cmake --preset=my-release --fresh
```

- 构建
```shell
cmake --build build
```

#### 构建基准测试

- 配置 CMake
```shell
cmake --preset=my-benches --fresh
```

- 构建
```shell
cmake --build build/release
```

- 执行基准测试
```shell
./build/release/benches/sregex_pcre2_benchmark
```

#### 构建调试版本（包含基准测试、示例程序以及单元测试）

- 配置 CMake
```shell
cmake --preset=my-debug --fresh
```

- 构建
```shell
cmake --build build/debug
```

### 集成到现有项目

- 安装 sregex 库
```shell
cmake --install build/release
```
安装完成后，sregex 库和头文件将可在系统的包含路径和库路径中使用。我们也可以通过指定 `--prefix` 选项将 sregex 安装到其他路径。例如，要将 sregex 安装到 `/specified/path`，我们可以运行：

```shell
cmake --install build/release --prefix=/specified/path
```

- 在现有项目中包含 sregex 头文件

```cpp
#include <sregex/sregex.h>
```

- 在现有项目中链接 sregex 库

```cmake
# 如果 sregex 安装在系统路径中
target_link_libraries(your_target_name PRIVATE sregex)
# 如果 sregex 安装在指定路径中
target_link_libraries(your_target_name PRIVATE /specified/path/lib/libsregex.a)
```

- 在现有项目中使用 sregex

1. 创建且仅需创建一次（不同模式创建不同实例） Sregex::Sregex 实例

```cpp
static Sregex::Sregex sregex("pattern");
```

2. 使用 Sregex 实例进行匹配

```cpp
bool match = sregex.isPartialMatch("input string");
```

请参考 [示例](examples/streaming_redaction_with_dfa.cc) 获取使用示例。

## 开源许可证

版权所有 (c) 2026 石犀及其贡献者。
sregex 采用 MIT 许可证发布。有关完整详情，请参阅随附的 [开源许可证](LICENSE) 文件。

## 联系方式

[石犀](https://www.srhino.com/)