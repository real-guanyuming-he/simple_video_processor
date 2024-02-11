cmake_minimum_required(VERSION 3.27)

# Enable the use of IDE source folders (mostly for Visual Studio filters)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project("simple_video_processor"
    VERSION 0.0.1
    LANGUAGES C CXX)

# Enable Testing
enable_testing()

################################# Source Paths #################################
set(FFWrapperName "ff_wrapper")

cmake_path(APPEND 
    CMAKE_SOURCE_DIR ${FFWrapperName}
    OUTPUT_VARIABLE SrcFFWrapperRootPath)
cmake_path(APPEND 
    SrcFFWrapperRootPath "util"
    OUTPUT_VARIABLE SrcFFWrapperUtilPath)
#message("SrcFFWrapperUtilPath=${SrcFFWrapperUtilPath}")
cmake_path(APPEND 
    SrcFFWrapperRootPath "data"
    OUTPUT_VARIABLE SrcFFWrapperDataPath)
cmake_path(APPEND 
    SrcFFWrapperRootPath "formats"
    OUTPUT_VARIABLE SrcFFWrapperFormatsPath)
cmake_path(APPEND 
    SrcFFWrapperRootPath "codec"
    OUTPUT_VARIABLE SrcFFWrapperCodecPath)
cmake_path(APPEND 
    SrcFFWrapperRootPath "sws"
    OUTPUT_VARIABLE SrcFFWrapperSwsPath)

# The FFmpeg wrapper library
set(FFWrapperSourceFiles
# Util
    "${SrcFFWrapperUtilPath}/ff_helpers.h"
    "${SrcFFWrapperUtilPath}/ff_helpers.cpp"
    "${SrcFFWrapperUtilPath}/ff_math.h"
    "${SrcFFWrapperUtilPath}/ff_time.h"
    "${SrcFFWrapperUtilPath}/ff_time.cpp"
    "${SrcFFWrapperUtilPath}/ff_object.h"
    "${SrcFFWrapperUtilPath}/ff_object.cpp"
    "${SrcFFWrapperUtilPath}/util.h"
    "${SrcFFWrapperUtilPath}/dict.h"
    "${SrcFFWrapperUtilPath}/dict.cpp"
    # Put these two here because FFmpeg put channel layout in libavutil
    "${SrcFFWrapperUtilPath}/channel_layout.h"
    "${SrcFFWrapperUtilPath}/channel_layout.cpp"
# Data
    "${SrcFFWrapperDataPath}/packet.h"
    "${SrcFFWrapperDataPath}/packet.cpp"
    "${SrcFFWrapperDataPath}/frame.h"
    "${SrcFFWrapperDataPath}/frame.cpp"
# Formats
    "${SrcFFWrapperFormatsPath}/stream.h"
    "${SrcFFWrapperFormatsPath}/stream.cpp"
    "${SrcFFWrapperFormatsPath}/muxer.h"
    "${SrcFFWrapperFormatsPath}/muxer.cpp"
    "${SrcFFWrapperFormatsPath}/demuxer.h"
    "${SrcFFWrapperFormatsPath}/demuxer.cpp"
    "${SrcFFWrapperFormatsPath}/media_base.h"
    "${SrcFFWrapperFormatsPath}/media_base.cpp"
# Codec
    "${SrcFFWrapperCodecPath}/codec_base.h"
    "${SrcFFWrapperCodecPath}/codec_base.cpp"
    "${SrcFFWrapperCodecPath}/decoder.h"
    "${SrcFFWrapperCodecPath}/decoder.cpp"
    "${SrcFFWrapperCodecPath}/encoder.h"
    "${SrcFFWrapperCodecPath}/encoder.cpp"
    "${SrcFFWrapperCodecPath}/codec_properties.h"
    "${SrcFFWrapperCodecPath}/codec_properties.cpp"
# SwScale
    "${SrcFFWrapperSwsPath}/frame_transformer.h"
    "${SrcFFWrapperSwsPath}/frame_transformer.cpp")
    
add_library(${FFWrapperName} SHARED
    ${FFWrapperSourceFiles})
# Add IDE subfolders (mostly for Visual Studio filters)
# according to the directory structure.
source_group(TREE ${SrcFFWrapperRootPath}
    FILES ${FFWrapperSourceFiles})

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
#    "avdevice"
    "avfilter"
    "avformat"
    "avutil"
#    "postproc"
    "swresample"
    "swscale"
)
set(ThirdPartyLibNames
# Subtitle
#    "ass"
# Audio
    "opus"
    "vorbis"
    "libmp3lame"
# Video
    "libx264"
    "libx265"
    "dav1d"
    "libwebp"
    "libpng16")
set(AllLibNames
    ${FFmpegLibNames} ${ThirdPartyLibNames})

# Find libs for both debug and release
foreach(LibName IN LISTS AllLibNames)
    find_library(${LibName}_Path
        NAMES ${LibName}
        PATHS ${MyVcpkgLibPath}
        NO_DEFAULT_PATH)
    find_library(${LibName}_PathDbg
        NAMES ${LibName}d ${LibName}
        PATHS ${MyVcpkgLibPathDbg}
        NO_DEFAULT_PATH)
endforeach()

target_include_directories(${FFWrapperName} PRIVATE ${VcpkgIncludePath})

# The use of target_link_directories is discouraged when I can find the full paths of the libs
# target_link_directories(${FFWrapperName} PRIVATE 
#     $<IF:$<CONFIG:Debug>,${MyVcpkgDebugBinPath},${MyVcpkgBinPath}>)

foreach(LibName IN LISTS AllLibNames)
    target_link_libraries(${FFWrapperName} PRIVATE
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
    target_link_libraries(${FFWrapperName} PRIVATE
        ${AdditionalLibsWindows})
endif()

################################# Other Compiler/Linker Options #################################

# Use the C++ 20 standard
set_property(TARGET ${FFWrapperName}
    PROPERTY CXX_STANDARD 20)

# Define FF_WRAPPER_EXPORT for ff_wrapper on Windows to correctly export its symbols into a DLL
if(WIN32)
    target_compile_definitions(${FFWrapperName}
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
# Test rational
add_executable(test_rational
    "${TestSrcFFWrapperPath}/test_rational.cpp")
# Test time
add_executable(test_time
    "${TestSrcFFWrapperPath}/test_time.cpp")
# Test demuxer
add_executable(test_demuxer
    "${TestSrcFFWrapperPath}/test_demuxer.cpp")
# Test frame
add_executable(test_frame
    "${TestSrcFFWrapperPath}/test_frame.cpp")
# Test packet
add_executable(test_packet
    "${TestSrcFFWrapperPath}/test_packet.cpp")
# Test decoder
add_executable(test_decoder
    "${TestSrcFFWrapperPath}/test_decoder.cpp")
# Test encoder
add_executable(test_encoder
    "${TestSrcFFWrapperPath}/test_encoder.cpp")
# Test muxer
add_executable(test_muxer
    "${TestSrcFFWrapperPath}/test_muxer.cpp")


set(ListTestTargets
    "test_ff_object"
    "test_dict"
    "test_rational"
    "test_time"
    "test_demuxer"
    "test_frame"
    "test_packet"
    "test_decoder"
    "test_encoder"
    "test_muxer")

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

    # Use the C++ 20 standard
    set_property(TARGET ${test}
        PROPERTY CXX_STANDARD 20)

    # Many need FFmpeg headers for their types.
    # Include this for all for convenience.
    target_include_directories(${test}
        PRIVATE ${VcpkgIncludePath})
endforeach()

################################# Exclusive Test Settings #################################

# Putting FFmpeg to PATH doesn't work for std::system() some times.
# Therefore, define the full path as a macro.
cmake_path(APPEND
    MyVcpkgInstalledRootPath "tools" "ffmpeg" "ffmpeg"
    OUTPUT_VARIABLE MyFFmpegExecutablePath)
# Append OS-specific executable extension
if(WIN32)
    cmake_path(APPEND_STRING
        MyFFmpegExecutablePath ".exe"
        OUTPUT_VARIABLE MyFFmpegExecutablePath)
endif()
# Put quotation marks around it to make it a string literal in C++.
set(MyFFmpegExecutablePath "\"${MyFFmpegExecutablePath}\"")

# Define the macro for all the tests that use FFmpeg CLI
target_compile_definitions("test_demuxer"
    PRIVATE "FFMPEG_EXECUTABLE_PATH=${MyFFmpegExecutablePath}")
target_compile_definitions("test_decoder"
    PRIVATE "FFMPEG_EXECUTABLE_PATH=${MyFFmpegExecutablePath}")
target_compile_definitions("test_encoder"
    PRIVATE "FFMPEG_EXECUTABLE_PATH=${MyFFmpegExecutablePath}")
target_compile_definitions("test_muxer"
    PRIVATE "FFMPEG_EXECUTABLE_PATH=${MyFFmpegExecutablePath}")

# Needs to use some FFmpeg APIs in these tests
target_link_libraries("test_frame" PRIVATE
    $<IF:$<CONFIG:Debug>,${avutil_PathDbg},${avutil_Path}>)
target_link_libraries("test_packet" PRIVATE
    $<IF:$<CONFIG:Debug>,${avcodec_PathDbg},${avcodec_Path}>)

# Debug Output
#message("VcpkgIncludePath=${VcpkgIncludePath}")
#message("MyVcpkgInstalledRootPath=${MyVcpkgInstalledRootPath}")
#message("MyVcpkgLibPathDbg=${MyVcpkgLibPathDbg}")
#message("MyVcpkgLibPath=${MyVcpkgLibPath}")