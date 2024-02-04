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

#include "../../ff_wrapper/codec/decoder.h"
#include "../../ff_wrapper/formats/demuxer.h"

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
	// Test creation
	{
		// The default constructor is deleted.
		//ff::decoder d;

		// Test invalid identification info
		TEST_ASSERT_THROWS(ff::decoder d1(AVCodecID::AV_CODEC_ID_NONE), std::invalid_argument);
		TEST_ASSERT_THROWS(ff::decoder d2(AVCodecID(-1)), std::invalid_argument);
		TEST_ASSERT_THROWS(ff::decoder d3("this shit cannot be the name of some decoder"), std::invalid_argument);
		TEST_ASSERT_THROWS(ff::decoder d4(""), std::invalid_argument);

		// Test valid id info
		ff::decoder d5(AVCodecID::AV_CODEC_ID_AV1);
		TEST_ASSERT_TRUE(d5.created(), "Should be created.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_AV1, d5.get_id(), "Should be created with the ID.");
		TEST_ASSERT_EQUALS(std::string("av1"), d5.get_name(), "Should fill the name.");

		ff::decoder d6("flac");
		TEST_ASSERT_TRUE(d6.created(), "Should be created.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_FLAC, d6.get_id(), "Should fill the ID.");
		TEST_ASSERT_EQUALS(std::string("flac"), d6.get_name(), "Should be created with the same name.");
	}

	// Test setting up a decoder/allocate_resources_memory()/create_decoder_context()
	{
		ff::decoder d1(AVCodecID::AV_CODEC_ID_MPEG4);
		d1.create_decoder_context();

		TEST_ASSERT_TRUE(d1.ready(), "Should be ready.");
		auto p1 = d1.get_decoder_properties();
		TEST_ASSERT_TRUE(p1.is_video(), "Should be created with correct properties.");

		ff::decoder d2("aac");
		d2.create_decoder_context();

		TEST_ASSERT_TRUE(d2.ready(), "Should be ready.");
		auto p2 = d2.get_decoder_properties();
		TEST_ASSERT_TRUE(p2.is_audio(), "Should be created with correct properties.");
	}

	// The demuxer requires the absolute path to files,
	// hence obtain this.
	fs::path working_dir(fs::current_path());

	// Actual decoding test.
	{
		fs::path test1_path(working_dir / "decoder_test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		create_test_video(test1_path_str, "libx265", "green", 800, 600, 24, 5);
		ff::demuxer dem1(test1_path_str.c_str());

		// Create the decoder from ID.
		ff::stream vs = dem1.get_stream(0);
		ff::decoder dec1(vs.codec_id());
		TEST_ASSERT_EQUALS(vs.codec_id(), dec1.get_id(), "Should be equal");
		TEST_ASSERT_TRUE(dec1.created(), "The decoder should be created now.");

		// Although the recommendation is to only set the needed ones and leave others their default values,
		// libx265 really needs some extra data that is hard to copy by hand.
		// So I make a full copy here.
		auto sp = vs.properties();
		dec1.set_codec_properties(sp);

		// Create the decoder context so it can be ready.
		dec1.create_decoder_context();
		TEST_ASSERT_TRUE(dec1.ready(), "The decoder should be ready now.");
		// Test if the properties really have changed to what's set.
		auto dp = dec1.get_decoder_properties();
		TEST_ASSERT_EQUALS(sp.type(), dp.type(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_pixel_format(), dp.v_pixel_format(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_width(), dp.v_width(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_height(), dp.v_height(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_aspect_ratio(), dp.v_aspect_ratio(), "Should really have set the properties.");

		// The decoding process.
		TEST_ASSERT_TRUE(dec1.hungry(), "Should be hungry initially.");
		TEST_ASSERT_FALSE(dec1.full(), "Should not be full initially.");
		TEST_ASSERT_FALSE(dec1.no_more_packets(), "I have not signaled it.");

		ff::packet pkt;
		do
		{
			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				break;
			}

			bool full = !dec1.feed_packet(pkt);
			TEST_ASSERT_EQUALS(full, dec1.full(), "Should be consistent");

			// Whether full or not, decode until it is hungry.
			ff::frame f;
			do
			{
				f = dec1.decode_frame();
				// Check successfully decoded frames
				if (!f.destroyed())
				{
					TEST_ASSERT_TRUE(f.ready(), "Decoded frames should be ready.");
					// Check the properties
					auto fdp = f.get_data_properties();
					TEST_ASSERT_TRUE(fdp.v_or_a, "Should got the type right.");
					TEST_ASSERT_EQUALS(dp.v_pixel_format(), fdp.fmt, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_width(), fdp.v_width(), "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_height(), fdp.v_height(), "Should get the properties right.");
				}
			} while (!f.destroyed());

			// the decoder must be hungry again.
			TEST_ASSERT_TRUE(dec1.hungry() && !dec1.full(), "Must be hungry now.");

		} while (!dem1.eof());

		// EOF from the demuxer. No more packets will be fed to the decoder.
		dec1.signal_no_more_packets();
		
		TEST_ASSERT_TRUE(dec1.no_more_packets(), "It must have responded to my signal.");
		TEST_ASSERT_FALSE(dec1.hungry(), "It cannot be hungry at this state.");

		// Drain all frames from its stomach.
		ff::frame f;
		do
		{
			f = dec1.decode_frame();
			// Check successfully decoded frames
			if (!f.destroyed())
			{
				TEST_ASSERT_TRUE(f.ready(), "Decoded frames should be ready.");
				// Check the properties
				auto fdp = f.get_data_properties();
				TEST_ASSERT_TRUE(fdp.v_or_a, "Should got the type right.");
				TEST_ASSERT_EQUALS(dp.v_pixel_format(), fdp.fmt, "Should get the properties right.");
				TEST_ASSERT_EQUALS(dp.v_width(), fdp.v_width(), "Should get the properties right.");
				TEST_ASSERT_EQUALS(dp.v_height(), fdp.v_height(), "Should get the properties right.");
			}
		} while (!f.destroyed());
	}

	// Copying a decoder is forbidden/deleted

	// Test moving
	{

	}

	return 0;
}