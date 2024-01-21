cmake_minimum_required(VERSION 3.27)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project("simple_video_processor"
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
cmake_path(APPEND 
    SrcFFWrapperPath "data"
    OUTPUT_VARIABLE SrcFFWrapperDataPath)
cmake_path(APPEND 
    SrcFFWrapperPath "formats"
    OUTPUT_VARIABLE SrcFFWrapperFormatsPath)

# The FFmpeg wrapper library
add_library(ff_wrapper SHARED
# Util
    "${SrcFFWrapperUtilPath}/ff_helpers.h"
    "${SrcFFWrapperUtilPath}/ff_helpers.cpp"
    "${SrcFFWrapperUtilPath}/ff_math_helpers.h"
    "${SrcFFWrapperUtilPath}/ff_object.h"
    "${SrcFFWrapperUtilPath}/ff_object.cpp"
    "${SrcFFWrapperUtilPath}/util.h"
    "${SrcFFWrapperUtilPath}/dict.h"
    "${SrcFFWrapperUtilPath}/dict.cpp"
# Data
    "${SrcFFWrapperDataPath}/packet.h"
    "${SrcFFWrapperDataPath}/packet.cpp"
# Formats
    "${SrcFFWrapperFormatsPath}/media_file_base.h"
    "${SrcFFWrapperFormatsPath}/muxer.h"
    "${SrcFFWrapperFormatsPath}/demuxer.h")

################################# Include/Library Paths #################################

# This path is the whole include/ that is above libavcodec
find_path(VcpkgIncludePath "libavcodec/avcodec.h")
# Because vcpkg messes up with CMAKE_PREFIX_PATH and put debug/lib before lib/
# even when the build config is not set, I have to walk around find_xxx()
# and using path operations to get both the debug and the release lib folders.
cmake_path(GET 
    VcpkgIncludePath PARENT_PATH
    MyVcpkgInstalledRootPath)

# These two are debug/lib and lib dirs
# Used because linking to dynamic libraries means to link to the static entry points of them
cmake_path(APPEND
    MyVcpkgInstalledRootPath "debug" "lib"
    OUTPUT_VARIABLE MyVcpkgLibPathDbg)
cmake_path(APPEND
    MyVcpkgInstalledRootPath "lib"
    OUTPUT_VARIABLE MyVcpkgLibPath)

# These two are debug/bin and /bin dirs
# Not used for now
# cmake_path(APPEND
#     MyVcpkgInstalledRootPath "debug" "bin"
#     OUTPUT_VARIABLE MyVcpkgDebugBinPath)
# # This is the /lib below /installed
# cmake_path(APPEND
#     MyVcpkgInstalledRootPath "bin"
#     OUTPUT_VARIABLE MyVcpkgBinPath)

message("MyVcpkgLibPathDbg=${MyVcpkgLibPathDbg}")

# Search for the libs only in the vcpkg/installed dir
set(FFmpegLibNames
    "avcodec"
    "avdevice"
    "avfilter"
    "avformat"
    "avutil"
    "swresample"
    "swscale")
set(ThirdPartyLibNames
    "ass")
set(AllLibNames
    ${FFmpegLibNames} ${ThirdPartyLibNames})

# Find libs for both debug and release
foreach(LibName IN LISTS AllLibNames)
    find_library(${LibName}_Path
        NAMES ${LibName}
        PATHS ${MyVcpkgLibPath}
        NO_DEFAULT_PATH)
    find_library(${LibName}_PathDbg
        NAMES ${LibName}
        PATHS ${MyVcpkgLibPathDbg}
        NO_DEFAULT_PATH)
endforeach()

target_include_directories(ff_wrapper PRIVATE ${VcpkgIncludePath})

# The use of target_link_directories is discouraged when I can find the full paths of the libs
# target_link_directories(ff_wrapper PRIVATE 
#     $<IF:$<CONFIG:Debug>,${MyVcpkgDebugBinPath},${MyVcpkgBinPath}>)

foreach(LibName IN LISTS AllLibNames)
    target_link_libraries(ff_wrapper PRIVATE
        $<IF:$<CONFIG:Debug>,${${LibName}_PathDbg},${${LibName}_Path}>)
endforeach()

# Additional libraries to add on Windows.
# Will have link errors otherwise.
if(WIN32)
    set(AdditionalLibsWindows
        "bcrypt"
        "strmiids" 
        "mfplat" "mfuuid")
endif()
if(WIN32)
    target_link_libraries(ff_wrapper PRIVATE
        ${AdditionalLibsWindows})
endif()

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

# Test ff_object
add_executable(test_ff_object 
    "${TestSrcFFWrapperPath}/test_ff_object.cpp")
# Test dict
add_executable(test_dict
    "${TestSrcFFWrapperPath}/test_dict.cpp")

set(ListTestTargets
    "test_ff_object"
    "test_dict")

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
#message("MyVcpkgLibPathDbg=${MyVcpkgLibPathDbg}")
#message("MyVcpkgLibPath=${MyVcpkgLibPath}")