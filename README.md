# AhoiLog
AhoiLog is a lightweight C++ async log library targeting C++ 23 and developed by AhoiLabs using the AhoiCpp plugin for Neovim.

### Cloning AhoiLog
You can clone the repository as usual:

```bash
$ git clone https://github.com/martuscellifaria/AhoiLog.git
```

### Embedding AhoiLog on your project
You can embed AhoiLog in your project just like any other C++ library. Just setup your build system to the include AhoiLog.h and AhoiLog.cpp files, with the third_party directory as well (you can check the CMakeLists.txt provided on the root of this project as well), and just place #include "AhoiLog.h" on top of the file you desire.
You can take a look at App/example.cpp.

### Build the example and the library
In order to build the example, if you are on Linux and have ninja installed, you can follow the buildscript provided.
```bash
$ python3 build.py
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

### Supported platforms
Linux, Windows, MacOS

### Usage
You have to instantiate AhoiLog at first, add a sink, and then you should be ready.
```c++

#include "AhoiLog.h"

int main() {
    AhoiLog ahoi_log = AhoiLog(false); //The boolean here filters the logging from DEBUG out (for production).
    ahoi_log.add_console_sink();
    ahoi_log.log(AhoiLogLevel::INFO, "AhoiLog!");
}
```

### Available sinks
Up to this point, AhoiLog has a console sink, a rotating file sink and a null sink. Multi-sink is also possible, if you call addLogSink multiple times.

### Log levels
The available log levels are DEBUG, INFO, WARNING, ERROR, FATAL. Debug level messages can be filtered out by passing a boolean with value false onto the AhoiLogger constructor. The default constructor has it set to true, so debug level messages are active by default.
