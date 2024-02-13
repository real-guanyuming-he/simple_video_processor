# Under Development
The project is currently under heavy development and may be unstable. Wait until the first stable release.

# Introduction
The project consists of two pieces of software.
- An encapsulation of `FFmpeg` done in modern C++ (C++ 20), which is designed to be cross-platform.
- A GUI video processor on Windows, programmed with the above wrapper and Microsoft's `WinUI 3`.

The whole software is completely free and open source (in `GNU GPL v3`).

# Motivation
Initially, I wanted to create a simple video processor for my daily processing tasks, like resizing, compressing, reformating, and clipping. Later, I decided to learn the FFmpeg APIs and create a C++ wrapper for them.

This section elaborates on my motivation for both.

## Motivation for the video processor
As I entered university, I had more need to perform basic video-processing tasks. Sometimes I need to submit a video and make sure its size/length/format is within the limit.

As I searched for a suitable tool for these tasks, I noticed a sad trend of the video editors available --- that they had taken divergent paths based on their purpose.

1. Professional editors like Adobe Premiere became more and more professional and were obviously unsuitable for my tasks then.

2. General editors for the public have shifted their focus on assisting in the making of short videos, and have neglected to perform the simple yet fundamental tasks cleanly and well (e.g. they may integrate such tasks into a short-video production pipeline and you cannot just do that one task that you want).

Therefore, I decided to fill the gap by developing a free and open-source video processor that focuses on doing these tasks cleanly and well.

## Motivation for the wrapper
I started a precursor of the project one year and some months ago.

However, at that time I had yet to complete the software engineering modules at my university. After the completion of a few such modules and as I gained more experience in programming, I started to recognise many serious design flaws and bad development practices in that project. After thorough contemplation, I decided to abandon that project and start anew from the beginning.

In the new project, I can practice more on the theories I learned from my university modules. In addition, simultaneously studying the use of FFmpeg APIs and encapsulating them presents a substantial yet exciting challenge that I'd like to take. Besides programming, I am thrilled to employ my UX designing and prototyping knowledge taught in an HCI module at my university. Working on both creates a valuable opportunity for me to assess, challenge, and improve my abilities.

# Features
## Currently available features:
The wrapper is currently being developed. It now has:
- demuxer
- decoder
- encoder
- muxer (Can work for the combination of some codecs and some formats. Needs a better way to setup codec-specific extradata for some formats.)
- encapsulation of various data structures of `FFmpeg`.

It has the following features:
1. Encapsulation of `FFmpeg` C APIs done in modern C++ (C++ 20). The encapsulation tries to 
    - preserve as much of `FFmpeg`'s power as possible while providing object-oriented access to it;
    - use C++ classes to establish and maintain invariants that would otherwise be maintained by hand in C of the FFmpeg objects;
    - provide convenient helpers for the most common tasks and scenarios;
    - handle errors by exceptions and assertions;
    - provide fast and constexpr math whenever possible.

2. Thorough specification through comments.

3. Automatic unit testing and integration testing done with `CTest`.

4. The development of the whole project is carried out with good practices. `Git` is used to manage the source code. `CMake` is used to easily configure and build the project across platforms.

## Planned features:
The GUI is waiting to be developed.

1. Modern design principles are to be used. You can expect
    - Requirement Engineering
    - Prototyping
    - Usability Testing
2. Perform simple video processing tasks:
    - Reformating
    - Clipping
    - Concatenation
    - Resizing
    - Compressing
    - and more...

3. Have a modern GUI made in `WinUI 3`.

# Configuration and Building
This project uses `vcpkg` and `CMake` to manage the build. Before you can build this project, you must have the two installed on your system and configured as follows:

- Install `CMake` 3.27 or later
- Install the latest `vcpkg` and
    - use it to install `FFmpeg` x64 through `vcpkg install` with the components that you'd like to include. My current installation on Windows is from this command:
    `vcpkg install ffmpeg[core,avdevice,swresample,swscale,postproc,ffmpeg,ffprobe,version3,gpl,ass,fdk-aac,opus,vorbis,mp3lame,x264,x265,dav1d,aom,openjpeg,webp]:x64-windows`.
    - set the environment variable `VCPKG_ROOT` to its root directory
- Create a binary folder somewhere (recommended to be outside of src) and cd into it. For example,
```
mkdir bin
cd bin
```

Then, you can let CMake generate the project files from the `bin/` directory by invoking
```
cmake -G "Your build system" -S ../src --preset=vcpkg
```

How you build the software is up to you.

# Testing
The project uses CTest to automate testing. I have written some auxiliary macros inside `src/test/util/test_util.h` to make my test cases compatibale with CTest and enable them to log error messages to a local file `test_log.log`.

*Note: In order to run some tests, you must have also installed the ffmpeg CLI through your `vcpkg install` earlier. The CLI is used by those tests to generate test videos. In addition, the vcpkg root path must not contain any space (Because I don't know why adding quotations around the ffmpeg path will make std::system() fail, at least on Windows).*

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

However, the branch names do follow some conventions: (Development also includes testing below)
- Branch `cmake_xxx` contains changes that only affect the `CMakeLists.txt` file. Generally these changes rewrite/improve existing CMake tasks implemented in the file.
- Branch `ff_data_xxx` contains development of wrappers for multimedia data (e.g. Packets that contain compressed data and frames that contain decompressed data).
- Branch `ff_formats_xxx` contains development of formats (file I/O, demuxer, and muxer).
- Branch `ff_codec_xxx` contains development of decoders and encoders.
- Branch `ff_util_xxx` contains development of the utility for `ff_wrapper`.
- Branch `ff_sws...` contains development of the `scscale` encapsulations.
- Other branch `ff_...` contains miscellaneous development on `ff_wrapper`. For exsample, small, general bug fix; adjusting previous tests for changed API.