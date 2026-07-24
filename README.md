# VHLog
VHLog is a lightweight C++ 23 async log library.

### Cloning VHLog
You can clone the repository as usual:

```bash
$ git clone https://github.com/martuscellifaria/VHLog.git
```

### Embedding VHLog on your project
You can embed VHLog in your project just like any other C++ library. Just setup your build system to the include VHLog.h and VHLog.cpp files, with the third_party directory as well (you can check the CMakeLists.txt provided on the root of this project as well), and just place #include "VHLog.h" on top of the file you desire.
You can take a look at examples/HelloVHLog.cpp.

### Build the example
In order to build the example, if you are on Linux and have ninja installed, you can follow the buildscript provided.
```bash
$ sh buildscript.sh
```

Alternatively, you can also use cmake and make:
```bash
$ mkdir build
$ cd build
$ cmake ..
$ make
```

You may also be able to produce a Visual Studio Solution (.sln) on Windows following the steps:
```powershell
$ mkdir build
$ cd build
$ cmake ..
```

You can take the buildscript.sh file as example.
```bash
$ cat buildscript.sh
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
ninja
ln -sf build/compile_commands.json ../
```

### Debugging
If you want to run VHLog with debug information, you can run debBuildscript.sh, or just change the CMAKE_BUILD_TYPE flag to RelWithDebInfo. This will produce a slightly bigger binary, that you can run with a debugger such as gdb, and then set breakpoints and so on.

### Supported platforms
Linux, Windows, MacOS

### Usage
You have to instantiate VHLog at first, add a sink, and then you should be ready.
```c++

#include "VHLog.h"

int main() {
    VHLogger vladoLog = VHLogger(false); //The boolean here filters the logging from DEBUGLV out (for production).
    vladoLog.addConsoleSink();
    vladoLog.log(VHLogLevel::INFOLV, "Hi VHLog!");
}
```

### Available sinks
Up to this point, VHLog has a console sink, a rotating file sink and a null sink. Multi-sink is also possible, if you call addLogSink multiple times.

### Log levels
The available log levels are DEBUG, INFO, WARNING, ERROR, FATAL. Debug level messages can be filtered out by passing a boolean with value false onto the VHLogger constructor. The default constructor has it set to true, so debug level messages are active by default.

### Benchmarking
You can also compile the benchmarking binary VHLogBench by passing -DVHLOG_BENCHMARK=ON to your cmake command:

```bash
$ cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DVHLOG_BENCHMARK=ON
```

If you build the benchmarking binary with asio, please do not forget to start a server before running VHLogBench.

#### Results for Linux on x86 architecture

```
$ neofetch
OS: Arch Linux x86_64
Host: VIWZ1 Lenovo IdeaPad Z400 Touch
Kernel: 6.17.9-arch1-1
CPU: Intel i7-3632QM (8) @ 3.200GHz
GPU: NVIDIA GeForce GT 635M
GPU: Intel 3rd Gen Core processor Graphics Controller
Memory: 1856MiB / 15842MiB
```

My development machine is quite old, but I was able to get the following results.

| Sink Type | Number of Threads | Write frequency |
|-----------|-------------------|-----------------|
| Console Only (ST) | 1 | 1,086,755 msg/s |
| File+Console | 1 | 1,075,431 msg/s |
| Console Only | 1 | 1,072,382 msg/s |
| File+Console (ST) | 1 | 1,064,352 msg/s |
| Null Sink | 1 | 1,062,986 msg/s |
| Basic File Sink | 1 | 1,014,550 msg/s |
| Null Sink (ST) | 1 | 1,014,316 msg/s |
| TCP Sink | 1 | 1,011,901 msg/s |
| Basic File (ST) | 1 | 1,011,720 msg/s |
| Date-based Rotating | 1 | 1,007,281 msg/s |
| Null Sink | 4 | 893,645 msg/s |
| All Sinks | 1 | 893,605 msg/s |
| Basic File Sink | 4 | 861,905 msg/s |
| All Sinks | 4 | 836,710 msg/s |
| File+Console | 4 | 820,406 msg/s |
| Date-based Rotating | 4 | 819,022 msg/s |
| TCP Sink | 4 | 817,783 msg/s |
| Console Only | 4 | 799,960 msg/s |

#### Results on Arm architecture
I also tested on an Arm single board computer running Ubuntu 24.04.

```
$ neofetch
OS: Ubuntu 24.04.3 LTS aarch64
Host: ArmSoM Sige7
Kernel: 6.1.0-1025-rockchip
CPU: Rockchip RK3588 (8) @ 1.800GHz
Memory: 784MiB / 31772MiB
```
| Sink Type | Number of Threads | Write frequency |
|-----------|-------------------|-----------------|
| Basic File (ST) | 1 | 991,143 msg/s |
| All Sinks | 1 | 953,673 msg/s |
| File+Console (ST) | 1 | 909,355 msg/s |
| Console Only (ST) | 1 | 909,185 msg/s |
| Null Sink (ST) | 1 | 903,679 msg/s |
| Null Sink | 1 | 879,545 msg/s |
| Basic File Sink | 1 | 824,960 msg/s |
| Date-based Rotating | 1 | 824,019 msg/s |
| File+Console | 1 | 811,880 msg/s |
| Console Only | 1 | 806,500 msg/s |
| TCP Sink | 1 | 803,426 msg/s |
| Basic File Sink | 4 | 783,945 msg/s |
| File+Console | 4 | 743,791 msg/s |
| TCP Sink | 4 | 734,690 msg/s |
| Console Only | 4 | 689,051 msg/s |
| All Sinks | 4 | 682,823 msg/s |
| Date-based Rotating | 4 | 656,783 msg/s |
| Null Sink | 4 | 626,046 msg/s |

The results show consistency among different architectures.

### VHLog Name
VHLog is named after Vladimir Herzog, a brazilian journalist murdered by the military dictatorship in the year of 1975.
