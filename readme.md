# Configure and Build
This project uses `vcpkg` and `CMake` to manage the build. Before you can build this project, you must have the two installed on your system and configured as follows:

- Install `CMake` 3.27 or later
- Install the latest `vcpkg` and
    - use it to install `FFmpeg` version 6, x64 static through `vcpkg install`
    - set the environment variable `VCPKG_ROOT` to its root directory

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