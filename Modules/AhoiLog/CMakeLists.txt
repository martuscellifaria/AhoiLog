add_library(AhoiLog STATIC src/AhoiLog.cpp)
target_link_libraries(AhoiLog ahoicpp_externals)
target_compile_features(AhoiLog PUBLIC cxx_std_23)
target_include_directories(AhoiLog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/AhoiLog)
