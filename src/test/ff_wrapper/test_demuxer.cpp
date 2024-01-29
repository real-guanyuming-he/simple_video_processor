/*
* Copyright (C) 2024 Guanyuming He
* This file is licensed under the GNU General Public License v3.
*
* This file is part of ff_wrapper.
* ff_wrapper is free software:
* you can redistribute it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
*
* ff_wrapper is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
* You should have received a copy of the GNU General Public License along with ff_wrapper.
* If not, see <https://www.gnu.org/licenses/>.
*/

#include "../test_util.h"

#include "../../ff_wrapper/formats/demuxer.h"

#include <cstdlib> // For std::system().
#include <filesystem> // For path handling as a demuxer requires an absolute path.
#include <format>

namespace fs = std::filesystem;

// For some unknown reason, setting PATH for FFmpeg doesn't make it discoverable for std::system().
// Therefore, I specified the full path as this macro, FFMPEG_EXECUTABLE_PATH in CMake.
// The commands are defined as macros as std::format requires a constexpr fmt str.
#define TEST_VIDEO_FMT_STR FFMPEG_EXECUTABLE_PATH " -f lavfi -i testsrc=duration={}:size={}x{}:rate={} "

// @returns the ffmpeg command's return value via std::system()
int create_test_video(const std::string& file_path, int w, int h, int rate, int duration)
{
	std::string cmd
	(
		std::format(TEST_VIDEO_FMT_STR,
			duration, w, h, rate)
	);
	cmd += std::string("\"") + file_path + '\"';

	return std::system(cmd.c_str());
}

int main()
{
	// The demux requires the absolute path to files,
	// hence obtain this.
	fs::path working_dir(fs::current_path());

	/*
	* Testing to the demuxer is mostly integration test,
	* as demuxing a file is quite complex.
	*/
	
	// Open some simple video with only 1 video stream
	{
		fs::path test1_path(working_dir / "test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		create_test_video(test1_path_str, 1280, 720, 24, 5);
		ff::demuxer d1(test1_path_str.c_str());

		TEST_ASSERT_EQUALS(1, d1.num_streams(), "should get num streams right");
		TEST_ASSERT_THROWS(d1.get_stream(-1), std::invalid_argument);
		TEST_ASSERT_THROWS(d1.get_stream(1), std::invalid_argument);
		ff::stream s1 = d1.get_stream(0);
		TEST_ASSERT_TRUE(s1.is_video() && (!s1.is_audio()) && (!s1.is_subtitle()), "should get the type right");
	}

	return 0;
}