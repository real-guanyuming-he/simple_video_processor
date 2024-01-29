# Introduction
The project consists of two pieces of software.
- An encapsulation of `FFmpeg` done in modern C++ (C++ 20), which is designed to be cross-platform.
- A GUI video processor on Windows, programmed with the above wrapper and Microsoft's `WinUI 3`.

The whole software is completely free and open source (in `GNU GPL v3`).

# Motivation
TO BE WRITTEN.

# Configuration and Building
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

*Note: In order to run some tests, you must have also installed the ffmpeg CLI through your `vcpkg install` earlier. The CLI is used by those tests to generate test videos.*

# Project Structure
The files are arranged in this structure:
```
.gitignore
readme.md
LICENSE
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

# Git Branches
After a branch is used (and its changes tested), it is merged into master. Then, new branches are created from there. It is often confusing to reuse branches and I did so at the very beginning. Since this sentence is written, no branch will be reused and new branches will mostly be created from master.

However, the branch names do follow some conventions:
- Branch `cmake_xxx` contains changes that only affect the `CMakeLists.txt` file. Generally these changes rewrite/improve existing CMake tasks implemented in the file.
- Branch `ff_data_xxx` contains development of wrappers for multimedia data (e.g. Packets that contain compressed data and frames that contain decompressed data).
- Branch `ff_formats_xxx` contains development of formats (file I/O, demuxer, and muxer)
- Branch `ff_util_xxx` contains development of the utility for `ff_wrapper`.
