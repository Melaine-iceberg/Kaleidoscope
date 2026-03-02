# Kaleidoscope Compiler

This is a refactored version of the Kaleidoscope compiler from the LLVM tutorial. The code has been organized into multiple files for better maintainability.

## File Structure

### Header Files

- **Token.h** - Token enum definitions for the lexer
- **AST.h** - Abstract Syntax Tree node class definitions
- **Lexer.h** - Lexer function declarations and global state
- **Parser.h** - Parser function declarations

### Source Files

- **Lexer.cpp** - Lexical analysis implementation
- **Parser.cpp** - Parser implementation with AST node creation
- **main.cpp** - Main application logic and handler functions

## Build Instructions

Requirements:

- CMake 3.26+（已在本项目中使用）
- C++23 compatible compiler
- LLVM（建议使用 MSYS2 clang64 或官方 LLVM 二进制包）
- Ninja（若不在 PATH，需要显式指定 CMAKE_MAKE_PROGRAM）

### 推荐（Windows + MSYS2 clang64）

前置：已安装并配置 `C:\msys64\clang64`。

```bash
cmake -S . -B out/build/msys64-clang -G Ninja ^
  -DMSYS_CLANG64_ROOT=C:/msys64/clang64 ^
  -DCMAKE_MAKE_PROGRAM="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja/ninja.exe"
cmake --build out/build/msys64-clang
```

说明：

- 会自动使用 `C:\msys64\clang64\bin\clang++.exe`
- LLVM 会从 `C:\msys64\clang64\lib\cmake\llvm` 读取
- ZLIB 会从 `C:\msys64\clang64` 解析

### 备选（Windows + LLVM 官方二进制包）

前置：已安装到 `C:\LLVM`，并确保 clang++ 与 llvm-rc 可用。

```bash
cmake -S . -B out/build/clang-ninja -G Ninja ^
  -DCMAKE_CXX_COMPILER=C:/LLVM/bin/clang++.exe ^
  -DCMAKE_RC_COMPILER=C:/LLVM/bin/llvm-rc.exe
cmake --build out/build/clang-ninja
```

### WSL2 Fedora + Visual Studio 2026

前置：在 WSL2 Fedora 中安装依赖：

```bash
sudo dnf install clang llvm-devel libcxx-devel libcxxabi-devel zlib-devel cmake ninja-build gdb rsync zip
```

**方法 A — VS 2026 打开文件夹（推荐）**

1. 在 VS 中选择 **File → Open → Folder**，打开项目根目录
2. VS 自动识别 `CMakePresets.json`
3. 在 **Target System** 下拉框选择 `WSL: FedoraLinux-43`
4. 在 **Configure Preset** 下拉框选择 `Linux Clang Release (libc++)` 或 `Linux Clang Debug (libc++)`
5. 正常构建即可

**方法 B — WSL2 命令行**

```bash
cmake --preset linux-clang-release
cmake --build --preset linux-clang-release
```

## Usage

Run the executable and enter expressions:

```
ready> 2+3*4
Parsed a top-level expr.
ready> def foo(x y) x+y*2
Parsed a function definition.
ready>
```

## Architecture

The compiler is structured into distinct phases:

1. **Lexical Analysis** (Lexer.cpp) - Breaks input into tokens
2. **Parsing** (Parser.cpp) - Builds Abstract Syntax Tree from tokens
3. **Main Loop** (main.cpp) - Handles different input types (definitions, expressions, etc.)

This modular design makes it easy to extend with additional features like:

- Code generation to LLVM IR
- Optimization passes
- Additional language constructs

## Next Steps

This refactored code is ready for adding LLVM IR generation. The clean separation between lexing, parsing, and AST representation makes it straightforward to add a CodeGen phase that walks the AST and emits LLVM IR.
