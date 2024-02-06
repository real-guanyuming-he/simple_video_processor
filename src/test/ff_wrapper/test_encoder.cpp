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

#include "../../ff_wrapper/codec/encoder.h"
#include "../../ff_wrapper/formats/demuxer.h"
#include "../../ff_wrapper/codec/encoder.h"

#include <cstdlib> // For std::system().
#include <filesystem> // For path handling as a demuxer requires an absolute path.
#include <format> // For std::format().
#include <string>

namespace fs = std::filesystem;

// For some unknown reason, sometimes, setting PATH for FFmpeg doesn't make it discoverable for std::system().
// Therefore, I specified the full path as this macro, FFMPEG_EXECUTABLE_PATH in CMake.
// The commands are defined as macros, because std::format requires a constexpr fmt str.

#define LAVFI_VIDEO_FMT_STR " -f lavfi -i color={}:duration={}:size={}x{}:rate={} "
#define LAVFI_AUDIO_FMT_STR " -f lavfi -i sine=duration={}:frequency={}:sample_rate={} "

#define VIDEO_ENCODER_FMT_STR "-c:v {} "
#define AUDIO_ENCODER_FMT_STR "-c:a {} "

#define TEST_VIDEO_FMT_STR \
FFMPEG_EXECUTABLE_PATH LAVFI_VIDEO_FMT_STR VIDEO_ENCODER_FMT_STR "-y "

#define TEST_AUDIO_FMT_STR \
FFMPEG_EXECUTABLE_PATH LAVFI_AUDIO_FMT_STR AUDIO_ENCODER_FMT_STR "-y "

#define TEST_AV_FMT_STR \
FFMPEG_EXECUTABLE_PATH LAVFI_VIDEO_FMT_STR LAVFI_AUDIO_FMT_STR \
VIDEO_ENCODER_FMT_STR AUDIO_ENCODER_FMT_STR "-y "

// @returns the ffmpeg command's return value via std::system()
int create_test_video
(
	const std::string& file_path, const std::string& encoder_name, const std::string& color_name,
	int w, int h, int rate, int duration
)
{
	std::string cmd
	(
		std::format
		(
			TEST_VIDEO_FMT_STR,
			color_name, duration, w, h, rate, encoder_name
		)
	);
	cmd += std::string("\"") + file_path + '\"';

	return std::system(cmd.c_str());
}

// @returns the ffmpeg command's return value via std::system()
int create_test_audio
(
	const std::string& file_path, const std::string& encoder_name,
	int duration, int frequency, int sample_rate
)
{
	std::string cmd
	(
		std::format
		(
			TEST_AUDIO_FMT_STR,
			duration, frequency, sample_rate, encoder_name
		)
	);
	cmd += std::string("\"") + file_path + '\"';

	return std::system(cmd.c_str());
}

// @returns the ffmpeg command's return value via std::system()
int create_test_av
(
	const std::string& file_path, int duration,
	const std::string& color_name, int w, int h, int rate,
	int frequency, int sample_rate,
	const std::string& venc_name, const std::string& aenc_name
)
{
	std::string cmd
	(
		std::format
		(
			TEST_AV_FMT_STR, duration,
			color_name, w, h, rate,
			duration, frequency, sample_rate,
			venc_name, aenc_name
		)
	);
	cmd += std::string("\"") + file_path + '\"';

	return std::system(cmd.c_str());
}

int main()
{
	/*
	* No dedicated tests for codec_base, because
	* that's an abstract class and encoder and encoder are the only possible implementations of it.
	*
	* The tests for class encoder and encoder cover all of codec_base's methods.
	*/

	// Test creation
	{
		// The default constructor is deleted.
		//ff::encoder d;

		// Test invalid identification info
		TEST_ASSERT_THROWS(ff::encoder e1(AVCodecID::AV_CODEC_ID_NONE), std::invalid_argument);
		TEST_ASSERT_THROWS(ff::encoder e2(AVCodecID(-1)), std::invalid_argument);
		TEST_ASSERT_THROWS(ff::encoder e3("this shit cannot be the name of some encoder"), std::invalid_argument);
		TEST_ASSERT_THROWS(ff::encoder e4(""), std::invalid_argument);

		// Test valid id info
		ff::encoder e5(AVCodecID::AV_CODEC_ID_H264);
		TEST_ASSERT_TRUE(e5.created(), "Should be created.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_H264, e5.get_id(), "Should be created with the ID.");
		// Different encoder may be used depending on the FFmpeg build.
		// Can only assert it's not empty.
		TEST_ASSERT_FALSE(std::string(e5.get_name()).empty(), "Should fill the name.");

		ff::encoder e6("vorbis");
		TEST_ASSERT_TRUE(e6.created(), "Should be created.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_VORBIS, e6.get_id(), "Should fill the ID.");
		TEST_ASSERT_EQUALS(std::string("vorbis"), e6.get_name(), "Should be created with the same name.");
	}

	// Test setting up a encoder/allocate_resources_memory()/create_codec_context()
	// and calling methods at wrong times.
	{
		ff::encoder e1(AVCodecID::AV_CODEC_ID_PNG);

		// Most encoders require that the properties be set properly for the ctx to be created.
		ff::codec_properties cp1;
		cp1.set_type_video();
		cp1.set_time_base(ff::common_video_time_base_600);
		cp1.set_v_pixel_format(AVPixelFormat::AV_PIX_FMT_RGB24);
		cp1.set_v_width(800);
		cp1.set_v_height(600);
		e1.set_codec_properties(cp1);

		e1.create_codec_context();
		// Can only set properties when created()
		TEST_ASSERT_THROWS(e1.set_codec_properties(ff::codec_properties()), std::logic_error);

		TEST_ASSERT_TRUE(e1.ready(), "Should be ready.");
		auto p1 = e1.get_codec_properties();
		TEST_ASSERT_TRUE(p1.is_video(), "Should be created with correct properties.");

		//ff::encoder e2("mp3");
		//ff::encoder e2("mp3lame");
		ff::encoder e2("libmp3lame");
		// Can only get properties when ready()
		TEST_ASSERT_THROWS(e2.get_codec_properties(), std::logic_error);

		// Most encoders require that the properties be set properly for the ctx to be created.
		ff::codec_properties cp2;
		cp2.set_type_audio();
		cp2.set_time_base(ff::common_audio_time_base_64000);
		cp2.set_a_sample_format(AVSampleFormat::AV_SAMPLE_FMT_S32P);
		cp2.set_a_sample_rate(32000);
		cp2.set_a_channel_layout(ff::channel_layout(ff::channel_layout::FF_AV_CHANNEL_LAYOUT_STEREO));
		e2.set_codec_properties(cp2);

		e2.create_codec_context();

		TEST_ASSERT_TRUE(e2.ready(), "Should be ready.");
		auto p2 = e2.get_codec_properties();
		TEST_ASSERT_TRUE(p2.is_audio(), "Should be created with correct properties.");
	}


	return 0;
}