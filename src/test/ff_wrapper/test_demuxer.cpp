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

int create_test_video(const std::string& file_path, int w, int h, int rate, int duration)
{
	std::string cmd
	(
		std::format("ffmpeg -f lavfi -i testsrc=duration={}:size={}x{}:rate={} ",
			duration, w, h, rate)
	);
	cmd += file_path;

	std::system(cmd.c_str());
}

int main()
{
	fs::path working_dir(fs::current_path());
	
	// Open some simple video with only 1 video stream
	{
		fs::path test1_path(working_dir / "test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		create_test_video(test1_path_str, 1280, 720, 24, 5);
		ff::demuxer d1(test1_path_str.c_str());

		
	}

	return 0;
}