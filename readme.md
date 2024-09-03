# Abandoned, yet again.
Now that I ascertained my desire pursuit academic development instead of software development, I will have to cease all development of this project and abandon it indefinitely.

# Introduction
The project consists of two pieces of software.
- An encapsulation of `FFmpeg`, `ff_wrapper`, done in modern C++ (C++ 20), which is intended to be cross-platform (because all it relies on are cross-platform), but has not been tested on `GNU/Linux` yet.
- A GUI video processor on Windows, `simple_video_processor`, programmed with `ff_wrapper` and `WinUI 3`.

`ff_wrapper` and `simple_video_processor` are both free/libre software (under `GNU GPL v3`). However, `simple_video_processor` can only be run on `Windows`, so you may not consider it 100% free.


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
+ The production of the wrapper follows a *software process* I designed, which includes multiple phases, each of one of the following:
    - software specification
    - software development
    - software validation (testing)
+ Encapsulation of `FFmpeg` C APIs done in modern C++ (C++ 20). The encapsulation tries to 
    - preserve as much of `FFmpeg`'s power as possible while providing object-oriented access to it;
    - use C++ classes to establish and maintain invariants that would otherwise be maintained by hand in C of the FFmpeg objects;
    - provide convenient helpers for the most common tasks and scenarios;
    - handle errors by exceptions and assertions;
    - provide fast and constexpr math whenever possible.
+ Thorough specification through comments.
+ Automatic unit testing and integration testing done with `CTest`.
+ The development of the whole project is carried out with good practices. `Git` is used to manage the source code. `CMake` is used to easily configure and build the project across platforms.

## Planned features:
The GUI is *waiting* to be developed.

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


# Software Process
This section honestly records all the activities that have led to the production of this software, which are technically known collectively as *the software process*.

In this section, two progress pointers, **ff_wrapper CURRENT PROGRESS** and **simple_video_processor CURRENT PROGRESS** are used to point to where the current progress for each of them is, respectively.

## The software process for `ff_wrapper`
For the wrapper I have employed a process different from most common processes. Usually software development happens after software specification, at least for the first time. Surprisingly even for myself, I decided to perform a certain amount of software development before I formally specify the requirements. **Why?**
1. As a wrapper, `ff_wrapper` must conform to the `FFmpeg` C APIs, which means I must understand the `FFmpeg` APIs to some extent before starting to specify the requirements. I could not at the start, and I decided that using these APIs is the best way to learn their interfaces. That's the first reason why I executed some development before everything else.
2. Besides conforming to the `FFmpeg` APIs, the wrapper must also make multimedia processing comfortable for a programmer who uses it. Unfortunately, I did not know much about what the right way was to form the interfaces so that the tasks could be comfortably done. Therefore, that's the second reason why some development was executed before everything else.

After this activity of development, `ff_wrapper` has reached a state where it has a usable demuxer, muxer, decoder, and encoder. I also have formed an understanding of the `FFmpeg` APIs and how multimedia file processing is done. Consequently, I planned to form the initial version of the formal requirements for `ff_wrapper` at this stage. **ff_wrapper CURRENT PROGRESS**.

## The software process for `simple_video_processor`
Not started. **simple_video_processor CURRENT PROGRESS**.


# Testing
Testing is the major way in which this project does software validation. Currently, tests are either written immediately after some requirements are formed (test-driven development) or written during development when new problems have been discovered.

The project uses `CTest` to automate testing. I have written some auxiliary macros inside `src/test/util/test_util.h` to make my test cases compatibale with `CTest` and enable them to log error messages to a local file `test_log.log`.

*Note: In order to run some tests, you must have also installed the ffmpeg CLI through your `vcpkg install` earlier. The CLI is used by those tests to generate test videos. In addition, the vcpkg root path must not contain any space (Because I don't know why adding quotations around the ffmpeg path will make std::system() fail, at least on Windows).*


# Configuration and Building
## The specifications
The specifications are typeset in $\LaTeX$ and I intend to generate `.pdf` files for them. Because `CMake` has no official support for $\LaTeX$ projects, you will have to compile the specifications yourself through the following process.

+ I use `hyperref` for the hyperlinks in `.pdf` so you will need to compile the `.tex` files at least twice.
+ Although currently no language other than English is needed, I plan to use `\XeLaTeX` (can't compile this in markdown) to make the production of specifications easy to be modified to contain Unicode characters in the future.

Therefore, assuming you are on the root directory, run this following command sequence to generate the final output
```
cd spec
xelatex "main_spec".tex
xelatex "main_spec".tex
```

## The source code
This project uses `vcpkg` and `CMake` to manage the build. Before you can build this project, you must have the two installed on your system and configured as follows:

- Install `CMake` 3.27 or later
- Install the latest `vcpkg` and
    - use it to install `FFmpeg` x64 through `vcpkg install` with the components that you'd like to include. My current installation on Windows is from this command:
    `vcpkg install ffmpeg[core,avdevice,swresample,swscale,postproc,ffmpeg,ffprobe,version3,gpl,ass,fdk-aac,opus,vorbis,mp3lame,x264,x265,dav1d,aom,openjpeg,webp]:x64-windows`. 
    - one thing to pay attention to is the `FFmpeg` version. I am currently programming under `FFmpeg` major version 6, but `vcpkg` will automatically download compile the latest `FFmpeg` source code. Do make sure that you are installing version 6 `FFmpeg` binaries.
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


# Project Structure
The files are arranged in this structure:
```
.gitignore
readme.md
LICENSE
...

spec/
    ff_wrapper/
        assets/
        ...
    simple_video_processor/
        assets/
        ...

src/
    CMakeLists.txt
    CMakePresets.json
    ...
    ff_wrapper/
        ...
    simple_video_processor/
        ...
    test/
        ff_wrapper/
            ...
        simple_video_processor/
            ...

bin/
    ...

doc/
    ...
```
, where 
+ `spec/` contains the specifications for the software,
+ `src/` contains the source, 
+ `bin/` contains the generated and built files, 
+ and `doc/` is planned to contain the documentation generated from the comments only (**currently not used**).

*Note: Unsurprisingly, `bin/` and `doc/` are excluded from Git's version control and are not accessible from GitHub.*


# Git Branches
After a branch is used (and its changes tested), it is merged into master. Then, new branches are created from there. It is often confusing to reuse branches and I did so at the very beginning. Since this sentence is written, no branch will be reused and new branches will mostly be created from master.

However, the branch names do follow some conventions:
+ Branch `spec_xxx` contains work related to software specification.
+ Branch `cmake_xxx` contains changes that only affect the `CMakeLists.txt` file. Generally these changes rewrite/improve existing CMake tasks implemented in the file.
+ Branch `ff_xxx` contains development of `ff_wrapper`
    - Branch `ff_data_xxx` contains development of wrappers for multimedia data (e.g. Packets that contain compressed data and frames that contain decompressed data).
    - Branch `ff_formats_xxx` contains development of formats (file I/O, demuxer, and muxer).
    - Branch `ff_codec_xxx` contains development of decoders and encoders.
    - Branch `ff_util_xxx` contains development of the utility for `ff_wrapper`.
    - Branch `ff_sws...` contains development of the `scscale` encapsulations.
    - Other branch `ff_...` contains miscellaneous development on `ff_wrapper`. For exsample, small, general bug fix; adjusting previous tests for changed API.

*Note that, for the time being, tests do not get dedicated branches. They are either written after some requirements are formed or written during development when new problems have been discovered.*
