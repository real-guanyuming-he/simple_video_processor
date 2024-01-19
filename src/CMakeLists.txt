cmake_minimum_required(VERSION 3.27)

# vcpkg does not automatically detect that it's referencing a static build,
# so I should set this variable manually.
# I don't need to modify the cache var of the same name, because inside the vcpkg toolchain file
# the cache var is modified accordingly.
set(VCPKG_TARGET_TRIPLET "x64-windows-static")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(videoprocessortemp
    VERSION 0.0.1
    LANGUAGES C CXX)

# Enable Testing
enable_testing()

################################# Source Paths #################################
set(FFWrapperName "ff_wrapper")

set(SrcFFWrapperPath ${FFWrapperName})
cmake_path(APPEND 
    SrcFFWrapperPath "util"
    OUTPUT_VARIABLE SrcFFWrapperUtilPath)
#message("SrcFFWrapperUtilPath=${SrcFFWrapperUtilPath}")

# The FFmpeg wrapper library
add_library(ff_wrapper SHARED
    "${SrcFFWrapperUtilPath}/ff_object.h"
    "${SrcFFWrapperUtilPath}/ff_object.cpp"
    "${SrcFFWrapperUtilPath}/util.h")

################################# Include/Library Paths #################################

# This path is the whole include/ that is above libavcodec
find_path(VcpkgIncludePath "libavcodec/avcodec.h")
# Because vcpkg messes up with CMAKE_PREFIX_PATH and put debug/lib before lib/
# even when the build config is not set, I have to walk around find_xxx()
# and using path operations to get both the debug and the release lib folders.
cmake_path(GET 
    VcpkgIncludePath PARENT_PATH
    MyVcpkgInstalledRootPath)

# This path is the whole /debug/lib below /installed
cmake_path(APPEND
    MyVcpkgInstalledRootPath "debug" "lib"
    OUTPUT_VARIABLE MyVcpkgDebugLibPath)
# This is the /lib below /installed
cmake_path(APPEND
    MyVcpkgInstalledRootPath "lib"
    OUTPUT_VARIABLE MyVcpkgLibPath)

set(FFmpegLibraries 
    "avcodec"
    "avdevice"
    "avfilter"
    "avformat"
    "avutil"
    "swresample"
    "swscale")
set(ThirdPartyLibraries
    "ass"
    "libpng16"
    "libsharpyuv"
    "libwebp" "libwebpdecoder" "libwebpdemux" "libwebpmux"
    "libx264"
    "openh264"
#    "srt"
    "x265-static")
set(AllLibraries
    ${FFmpegLibraries} ${ThirdPartyLibraries})

# FFmpeg libraries are not appended with 'd' under a debug build
# However, other libraries may
set(ThirdPartyLibrariesDebug
    "ass"
    "libpng16d"
    "libsharpyuv"
    "libwebp" "libwebpdecoder" "libwebpdemux" "libwebpmux"
    "libx264"
    "openh264"
#    "srt"
    "x265-static")
set(AllLibrariesDebug
    ${FFmpegLibraries} ${ThirdPartyLibrariesDebug})

target_include_directories(ff_wrapper PRIVATE ${VcpkgIncludePath})
target_link_directories(ff_wrapper PRIVATE 
    $<IF:$<CONFIG:Debug>,${MyVcpkgDebugLibPath},${MyVcpkgLibPath}>)
target_link_libraries(ff_wrapper PRIVATE
    $<IF:$<CONFIG:Debug>,${AllLibrariesDebug},${AllLibraries}>)

################################# Other Compiler/Linker Options #################################

# Use the C++ 17 standard
set_property(TARGET ff_wrapper
    PROPERTY CXX_STANDARD 17)

# Define FF_WRAPPER_EXPORT for ff_wrapper on Windows to correctly export its symbols into a DLL
if(WIN32)
    target_compile_definitions(ff_wrapper
        PRIVATE "FF_WRAPPER_EXPORT")
endif()

################################# Tests #################################

set(TestSrcPath "test")
cmake_path(APPEND
    TestSrcPath ${FFWrapperName}
    OUTPUT_VARIABLE TestSrcFFWrapperPath)

set(ListTestTargets)

# Test ff_object
add_executable(test_ff_object 
    "${TestSrcFFWrapperPath}/test_ff_object.cpp")
list(APPEND ListTestTargets
    test_ff_object)

################################# Common Test Settings #################################

foreach(test IN LISTS ListTestTargets)
    # Add a CTest with the same name as the target name
    add_test(NAME ${test}
    COMMAND ${test})

    # Make all ff_wrapper tests refer to the library
    target_link_libraries(${test}
        PRIVATE ${FFWrapperName})

    # Define the testing macro
    target_compile_definitions(${test}
        PRIVATE "FF_TESTING")
endforeach()

# Debug Output
#message("VcpkgIncludePath=${VcpkgIncludePath}")
#message("MyVcpkgInstalledRootPath=${MyVcpkgInstalledRootPath}")
#message("MyVcpkgDebugLibPath=${MyVcpkgDebugLibPath}")
#message("MyVcpkgLibPath=${MyVcpkgLibPath}")