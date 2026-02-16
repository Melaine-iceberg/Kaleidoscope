cmake_minimum_required(VERSION 3.26)

project(Kaleidoscope LANGUAGES CXX)

# 使用 C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()

# 添加LLVM支持
if(DEFINED ENV{LLVM_DIR} AND NOT "$ENV{LLVM_DIR}" STREQUAL "")
    set(LLVM_DIR "$ENV{LLVM_DIR}" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
else()
    set(LLVM_DIR "C:/LLVM/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
endif()
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# 手动指定ZLIB路径（修正为libz.dll.a）
set(ZLIB_ROOT "$ENV{ZLIB_ROOT}" CACHE PATH "Path to Zlib root")
find_package(ZLIB)
if(ZLIB_FOUND)
    set(_zlib_target ZLIB::ZLIB)
else()
    set(_zlib_target "")
endif()

add_executable(Kaleidoscope
        src/main.cpp
        src/Lexer.cpp
        src/Parser.cpp
        src/AST.cpp
        src/Optimize.cpp
        "src/control flow.cpp"
        src/Debug.cpp)

target_include_directories(Kaleidoscope
        PRIVATE
        ${LLVM_INCLUDE_DIRS}
        ${PROJECT_SOURCE_DIR}/include)

# 链接LLVM库（添加更多必要组件）
llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
    executionengine
    mcjit
    target
    x86codegen
    x86asmparser
    x86info
    passes
    analysis
    transformutils)
target_link_libraries(Kaleidoscope PRIVATE ${LLVM_LIBS} ${_zlib_target})
