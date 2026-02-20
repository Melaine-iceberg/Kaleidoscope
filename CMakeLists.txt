cmake_minimum_required(VERSION 3.26)

if(CMAKE_GENERATOR MATCHES "Visual Studio")
    message(FATAL_ERROR "Visual Studio 生成器会产生 .slnx，请改用 Ninja 或 Unix Makefiles。")
endif()

option(USE_CLANG "Prefer clang/clang++ when available" ON)
option(USE_LIBCXX "Use libc++ with clang++" ON)
set(LIBCXX_INCLUDE_DIR "" CACHE PATH "Path to libc++ headers")
set(LIBCXX_LIB_DIR "" CACHE PATH "Path to libc++ libraries")
set(MSYS_CLANG64_ROOT "" CACHE PATH "Path to MSYS2 clang64 root")

if(CMAKE_GENERATOR MATCHES "Ninja" AND NOT DEFINED CMAKE_MAKE_PROGRAM)
    find_program(NINJA_EXE ninja
        PATHS
            "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja"
            "C:/Program Files/Microsoft Visual Studio/18/Insiders/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja"
            "C:/Program Files/Microsoft Visual Studio/18/Insiders/MSBuild/Google/Android/bin"
        NO_DEFAULT_PATH)
    if(NINJA_EXE)
        set(CMAKE_MAKE_PROGRAM "${NINJA_EXE}" CACHE FILEPATH "" FORCE)
    endif()
endif()

if(USE_CLANG AND NOT DEFINED CMAKE_CXX_COMPILER)
    if(WIN32)
        if(NOT MSYS_CLANG64_ROOT AND EXISTS "C:/msys64/clang64")
            set(MSYS_CLANG64_ROOT "C:/msys64/clang64" CACHE PATH "" FORCE)
        endif()
        if(MSYS_CLANG64_ROOT AND EXISTS "${MSYS_CLANG64_ROOT}/bin/clang++.exe")
            set(CMAKE_CXX_COMPILER "${MSYS_CLANG64_ROOT}/bin/clang++.exe" CACHE FILEPATH "" FORCE)
        endif()
        find_program(CLANGXX clang++)
        if(CLANGXX)
            set(CMAKE_CXX_COMPILER "${CLANGXX}" CACHE FILEPATH "" FORCE)
        else()
            find_program(CLANGCL clang-cl)
            if(CLANGCL)
                set(CMAKE_CXX_COMPILER "${CLANGCL}" CACHE FILEPATH "" FORCE)
            endif()
        endif()
    else()
        find_program(CLANGXX clang++)
        if(CLANGXX)
            set(CMAKE_CXX_COMPILER "${CLANGXX}" CACHE FILEPATH "" FORCE)
        endif()
    endif()
endif()

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

set(_extra_stdlib_flags "")
set(_extra_stdlib_libs "")
if(USE_LIBCXX AND CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
    list(APPEND _extra_stdlib_flags -stdlib=libc++)
    if(NOT APPLE)
        find_library(LIBCXXABI c++abi libc++abi PATHS ${LIBCXX_LIB_DIR} NO_DEFAULT_PATH)
        if(LIBCXXABI)
            list(APPEND _extra_stdlib_libs "${LIBCXXABI}")
        endif()
    endif()
endif()

if(WIN32 AND MSYS_CLANG64_ROOT AND EXISTS "${MSYS_CLANG64_ROOT}/bin/llvm-rc.exe" AND NOT DEFINED CMAKE_RC_COMPILER)
    set(CMAKE_RC_COMPILER "${MSYS_CLANG64_ROOT}/bin/llvm-rc.exe" CACHE FILEPATH "" FORCE)
endif()

if(USE_LIBCXX AND NOT LIBCXX_INCLUDE_DIR AND EXISTS "C:/LLVM/lib/clang/21/include")
    set(LIBCXX_INCLUDE_DIR "C:/LLVM/lib/clang/21/include" CACHE PATH "" FORCE)
endif()

if(USE_LIBCXX AND NOT LIBCXX_LIB_DIR AND EXISTS "C:/LLVM/lib/clang/21/lib/windows")
    set(LIBCXX_LIB_DIR "C:/LLVM/lib/clang/21/lib/windows" CACHE PATH "" FORCE)
endif()

if(USE_LIBCXX AND MSYS_CLANG64_ROOT)
    if(NOT LIBCXX_INCLUDE_DIR AND EXISTS "${MSYS_CLANG64_ROOT}/include/c++/v1")
        set(LIBCXX_INCLUDE_DIR "${MSYS_CLANG64_ROOT}/include/c++/v1" CACHE PATH "" FORCE)
    endif()
    if(NOT LIBCXX_LIB_DIR AND EXISTS "${MSYS_CLANG64_ROOT}/lib")
        set(LIBCXX_LIB_DIR "${MSYS_CLANG64_ROOT}/lib" CACHE PATH "" FORCE)
    endif()
endif()

if(USE_LIBCXX AND LIBCXX_LIB_DIR)
    find_library(LIBCXX_LIB c++ libc++ PATHS ${LIBCXX_LIB_DIR} NO_DEFAULT_PATH)
    if(LIBCXX_LIB)
        list(APPEND _extra_stdlib_libs "${LIBCXX_LIB}")
    endif()
endif()

# 添加LLVM支持
if(DEFINED ENV{LLVM_DIR} AND NOT "$ENV{LLVM_DIR}" STREQUAL "")
    set(LLVM_DIR "$ENV{LLVM_DIR}" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
elseif(MSYS_CLANG64_ROOT AND EXISTS "${MSYS_CLANG64_ROOT}/lib/cmake/llvm")
    set(LLVM_DIR "${MSYS_CLANG64_ROOT}/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
else()
    set(LLVM_DIR "C:/LLVM/lib/cmake/llvm" CACHE PATH "Path to LLVMConfig.cmake" FORCE)
endif()
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

if(WIN32)
    set(DIAGUIDS_LIB "")
    set(_dia_candidates
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/DIA SDK/lib/amd64/diaguids.lib"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/Professional/DIA SDK/lib/amd64/diaguids.lib"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/DIA SDK/lib/amd64/diaguids.lib"
        "C:/Program Files/Microsoft Visual Studio/18/Insiders/DIA SDK/lib/amd64/diaguids.lib"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/DIA SDK/lib/diaguids.lib"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/Professional/DIA SDK/lib/diaguids.lib"
        "C:/Program Files (x86)/Microsoft Visual Studio/2022/Community/DIA SDK/lib/diaguids.lib"
        "C:/Program Files/Microsoft Visual Studio/18/Insiders/DIA SDK/lib/diaguids.lib")
    foreach(_cand IN LISTS _dia_candidates)
        if(NOT DIAGUIDS_LIB AND EXISTS "${_cand}")
            set(DIAGUIDS_LIB "${_cand}")
        endif()
    endforeach()
    if(DIAGUIDS_LIB AND TARGET LLVMDebugInfoPDB)
        get_target_property(_pdb_libs LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)
        if(_pdb_libs)
            set(_new_pdb_libs "")
            foreach(_lib IN LISTS _pdb_libs)
                if(_lib MATCHES "diaguids\\.lib$")
                    list(APPEND _new_pdb_libs "${DIAGUIDS_LIB}")
                else()
                    list(APPEND _new_pdb_libs "${_lib}")
                endif()
            endforeach()
            set_target_properties(LLVMDebugInfoPDB PROPERTIES INTERFACE_LINK_LIBRARIES "${_new_pdb_libs}")
        endif()
    endif()
endif()

# 手动指定ZLIB路径（修正为libz.dll.a）
set(ZLIB_ROOT "$ENV{ZLIB_ROOT}" CACHE PATH "Path to Zlib root")
if(NOT ZLIB_ROOT AND MSYS_CLANG64_ROOT)
    set(ZLIB_ROOT "${MSYS_CLANG64_ROOT}" CACHE PATH "" FORCE)
endif()
find_package(ZLIB)
if(ZLIB_FOUND)
    set(_zlib_target ZLIB::ZLIB)
else()
    set(_zlib_target "")
endif()
if(MSYS_CLANG64_ROOT AND NOT ZLIB_FOUND)
    message(FATAL_ERROR "MSYS2 clang64 需要 ZLIB，请确认 C:/msys64/clang64 已安装 zlib。")
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

if(USE_LIBCXX AND LIBCXX_INCLUDE_DIR AND EXISTS "${LIBCXX_INCLUDE_DIR}")
    target_include_directories(Kaleidoscope SYSTEM PRIVATE ${LIBCXX_INCLUDE_DIR})
endif()

if(_extra_stdlib_flags)
    target_compile_options(Kaleidoscope PRIVATE ${_extra_stdlib_flags})
    target_link_options(Kaleidoscope PRIVATE ${_extra_stdlib_flags})
endif()

# 链接LLVM库（尽量收敛组件以提升跨平台可用性）
llvm_map_components_to_libnames(LLVM_LIBS
    core
    support
    irreader
    executionengine
    mcjit
    native
    nativecodegen
    target
    asmparser
    mc
    mcparser
    bitreader
    bitwriter
    analysis
    passes
    transformutils)
target_link_libraries(Kaleidoscope PRIVATE ${LLVM_LIBS} ${_zlib_target} ${_extra_stdlib_libs})
