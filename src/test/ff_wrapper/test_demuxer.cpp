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

#include "../../ff_wrapper/util/util.h"
#include "../test_util.h"

#include "../../ff_wrapper/formats/demuxer.h"

#include <cstdlib> // For std::system().
#include <filesystem> // For path handling as a demuxer requires an absolute path.
#include <format> // For std::format().

namespace fs = std::filesystem;

// For some unknown reason, sometimes, setting PATH for FFmpeg doesn't make it discoverable for std::system().
// Therefore, I specified the full path as this macro, FFMPEG_EXECUTABLE_PATH in CMake.
// The commands are defined as macros, because std::format requires a constexpr fmt str.

#define LAVFI_VIDEO_FMT_STR " -f lavfi -i testsrc=duration={}:size={}x{}:rate={} "
#define LAVFI_AUDIO_FMT_STR " -f lavfi -i sine=duration={}:frequency={}:sample_rate={} "

#define TEST_VIDEO_FMT_STR \
FFMPEG_EXECUTABLE_PATH LAVFI_VIDEO_FMT_STR " -y "

#define TEST_AUDIO_FMT_STR \
FFMPEG_EXECUTABLE_PATH LAVFI_AUDIO_FMT_STR " -y "

#define TEST_AV_FMT_STR \
FFMPEG_EXECUTABLE_PATH LAVFI_VIDEO_FMT_STR LAVFI_AUDIO_FMT_STR " -y "

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

// @returns the ffmpeg command's return value via std::system()
int create_test_audio(const std::string& file_path, int duration, int frequency, int sample_rate)
{
	std::string cmd
	(
		std::format(TEST_AUDIO_FMT_STR,
			duration, frequency, sample_rate)
	);
	cmd += std::string("\"") + file_path + '\"';

	return std::system(cmd.c_str());
}

// @returns the ffmpeg command's return value via std::system()
int create_test_av
(
	const std::string& file_path, int duration,
	int w, int h, int rate,
	int frequency, int sample_rate
)
{
	std::string cmd
	(
		std::format(TEST_AV_FMT_STR,
			duration, w, h, rate,
			duration, frequency, sample_rate)
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
	* as well as some whitebox test.
	* This is because demuxing a file is quite complex.
	*/
	
	// Some simple video with only 1 video stream
	{
		fs::path test1_path(working_dir / "test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		create_test_video(test1_path_str, 1280, 720, 24, 5);
		ff::demuxer d1(test1_path_str.c_str());

		// stream information
		TEST_ASSERT_EQUALS(1, d1.num_streams(), "should get num streams right");
		TEST_ASSERT_EQUALS(1, d1.num_videos(), "should get num streams of specific type right");
		TEST_ASSERT_THROWS(d1.get_stream(-1), std::out_of_range);
		TEST_ASSERT_THROWS(d1.get_stream(1), std::out_of_range);
		ff::stream s1 = d1.get_stream(0);
		TEST_ASSERT_TRUE(s1.is_video() && (!s1.is_audio()) && (!s1.is_subtitle()), "should get the type right");
		// EOF
		TEST_ASSERT_FALSE(d1.eof(), "Should not have eof when created");

		// Test getting packets out.
		ff::packet pkt;
		// These variables are used to test the order of the packets.
		ff::time prev_pts, prev_dts;
		ff::time cur_pts, cur_dts;
		do
		{
			pkt = d1.demux_next_packet();
			if (pkt.destroyed() && d1.eof())
			{
				break;
			}

			TEST_ASSERT_TRUE(pkt.linked_to_stream(), "packets coming from a demuxer should be linked to its stream.");
			TEST_ASSERT_EQUALS(0, pkt->stream_index, "stream index should match");

			cur_pts = pkt.pts(); 
			cur_dts = pkt.dts();
			// dts can be negative in ffmpeg, and such dts effectively mean 0.
			// such negative values are introduced because ffmpeg requires dts < pts, and for
			// pts = 0 (i.e. the first frame), dts has to be negative.
			// Thus, only make the assertion when dts > 0
			if (cur_dts > 0)
			{
				TEST_ASSERT_TRUE(cur_dts >= prev_dts, "Should have increasingly later packets");
			}
			// pts does not have to be monotonically increasing.
			//TEST_ASSERT_TRUE(cur_pts >= prev_pts, "Should have increasingly later packets");
			prev_pts = cur_pts;
			prev_dts = cur_dts;

		} while (!pkt.destroyed());

		TEST_ASSERT_TRUE(d1.eof(), "Should have eof after the last packet is out.");
	}

	// with more than 1 stream
	{
		fs::path test_path(working_dir / "test2.wmv");
		std::string test_path_str(test_path.generic_string());
		create_test_av(test_path_str, 4, 800, 600, 24, 1000, 44000);
		ff::demuxer d1(test_path_str.c_str());

		// stream information
		TEST_ASSERT_EQUALS(2, d1.num_streams(), "should get num streams right");
		TEST_ASSERT_EQUALS(1, d1.num_videos(), "should get num streams of specific type right");
		TEST_ASSERT_EQUALS(1, d1.num_audios(), "should get num streams of specific type right");
		TEST_ASSERT_THROWS(d1.get_video(-1), std::out_of_range);
		TEST_ASSERT_THROWS(d1.get_audio(1), std::out_of_range);
		TEST_ASSERT_THROWS(d1.get_subtitle(0), std::out_of_range);
		ff::stream v1 = d1.get_video(0);
		ff::stream a1 = d1.get_audio(0);
		int v1i = d1.get_video_ind(0);
		int a1i = d1.get_audio_ind(0);
		TEST_ASSERT_TRUE(v1.is_video() && (!v1.is_audio()) && (!v1.is_subtitle()), "should get the type right");
		TEST_ASSERT_TRUE(a1.is_audio() && (!a1.is_video()) && (!a1.is_subtitle()), "should get the type right");
		// EOF
		TEST_ASSERT_FALSE(d1.eof(), "Should not have eof when created");

		// Test getting packets out.
		ff::packet pkt;
		// These variables are used to test the order of the packets.
		ff::time prev_dts_a, prev_dts_v;
		ff::time cur_dts_a, cur_dts_v;
		do
		{
			pkt = d1.demux_next_packet();
			if (pkt.destroyed() && d1.eof())
			{
				break;
			}

			TEST_ASSERT_TRUE(pkt.linked_to_stream(), "packets coming from a demuxer should be linked to its stream.");

			if (pkt->stream_index == v1i)
			{
				cur_dts_v = pkt.dts();
				// dts can be negative in ffmpeg, and such dts effectively mean 0.
				// such negative values are introduced because ffmpeg requires dts < pts, and for
				// pts = 0 (i.e. the first frame), dts has to be negative.
				// Thus, only make the assertion when dts > 0
				if (cur_dts_v > 0)
				{
					TEST_ASSERT_TRUE(cur_dts_v >= prev_dts_v, "Should have increasingly later packets");
				}

				prev_dts_v = cur_dts_v;
			}
			else if (pkt->stream_index == a1i)
			{
				cur_dts_a = pkt.dts();
				// dts can be negative in ffmpeg, and such dts effectively mean 0.
				// such negative values are introduced because ffmpeg requires dts < pts, and for
				// pts = 0 (i.e. the first frame), dts has to be negative.
				// Thus, only make the assertion when dts > 0
				if (cur_dts_a > 0)
				{
					TEST_ASSERT_TRUE(cur_dts_a >= prev_dts_a, "Should have increasingly later packets");
				}

				prev_dts_a = cur_dts_a;
			}
			else // pkt->stream_index != v1i && pkt->stream_index != a1i
			{
				TEST_ASSERT_TRUE(false, "stream index should match");
			}

		} while (!pkt.destroyed());

		TEST_ASSERT_TRUE(d1.eof(), "Should have eof after the last packet is out.");
	}

	return 0;
}