# External dependencies

This directory is intended for third-party libraries cloned from Git.

## How to use

1. Clone a dependency (use `<leader>cpe on Neovim` and paste the git url, or clone manually from externals directory).

2. Integrate into your build:
- If it uses CMake: add `add_subdirectory(externals/<library-name>)` to the CMakeLists.txt file of the module that needs the external library.
- If it is a header-only library: add `target_include_directories(your_target PRIVATE externals/<library-name>/include)` or the path to the correspondent header.
- Due to C++ complexity and lack of standardization, some external libraries may need extra work to be integrated. Even in modern commercial IDEs it is not supposed to work out of the box.

## Example with nlohmann-json (header-only):
- Run `<leader>cpe`.
- Paste `https://github.com/nlohmann/json.git` to the dialog prompt.
- Then add the following to the target modules CMakeLists.txt: `target_include_directories(<YourTargetModuleName> PUBLIC ${CMAKE_SOURCE_DIR}/externals/json/include)`.
- If everything is fine, you can go to `<YourTargetModuleName>.cpp` and put `#include "nlohmann/json.hpp"` at the top of it.
- Now compile it with `<leader>cpc`.
- Note: For other libraries that need extra compilation, you may have to also paste `target_link_libraries(<YourTargetModuleName> <YourClonedExternalLibrary>)`
