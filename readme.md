# Introduction
The project consists of two pieces of software.
- An encapsulation of `FFmpeg` done in modern C++, which is designed to be cross-platform.
- A GUI video processor on Windows, programmed with the above wrapper and Microsoft's WinUI 3.

The whole software is completely free and open source.

# Motivation
TO BE WRITTEN.

# Configure and Build
This project uses `vcpkg` and `CMake` to manage the build. Before you can build this project, you must have the two installed on your system and configured as follows:

- Install `CMake` 3.27 or later
- Install the latest `vcpkg` and
    - use it to install `FFmpeg` x64 through `vcpkg install`
    - set the environment variable `VCPKG_ROOT` to its root directory

Then, you can let CMake generate the project files from the `bin/` directory by invoking
```
cmake -G "Your build system" -S ../src --preset=vcpkg
```

How you build the software is up to you.

# Testing
The project uses CTest to automate testing. I have written some auxiliary macros inside `src/test/util/test_util.h` to make my test cases compatibale with CTest and enable them to log error messages to a local file `test_log.log`.

# Project Structure
The files are arranged in this structure:
```
.gitignore
readme.md
LICENSES
...

src/
    CMakeLists.txt
    CMakePresets.json
    ...
    project_1/
        ...
    ...
    project_n/
        ...
    test/
        ...

bin/
    ...

doc/
    ...
```
, where `src/` contains the source, `bin/` contains the generated and built files, and `doc/` contains the documentation (including the generated documents).

`bin/` is excluded from Git's version control and is not visible on GitHub.