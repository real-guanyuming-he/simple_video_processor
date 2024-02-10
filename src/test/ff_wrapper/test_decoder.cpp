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
	/*
	* No dedicated tests for codec_base, because
	* that's an abstract class and decoder and encoder are the only possible implementations of it.
	* 
	* The tests for class decoder and encoder cover all of codec_base's methods.
	*/

	/*
	* Decoding in this unit is like:
	* Feed one packet and decode until the decoder is hungry.
	* Lemma 1: every packet will be fed successfully.
	* Proof.
	*	1. Establishment: initially the decoder is hungry so it will succeed.
	*	2. Maintanence: after a packet is fed, the decoder will be used until it is hungry again, 
	* which means the next will succeed, too.
	*/

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
		// Different decoder may be used depending on the FFmpeg build.
		// Could be av1, dav1d, etc. Only assert that it's not empty.
		TEST_ASSERT_FALSE(std::string(d5.get_name()).empty(), "Should fill the name.");

		ff::decoder d6("flac");
		TEST_ASSERT_TRUE(d6.created(), "Should be created.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_FLAC, d6.get_id(), "Should fill the ID.");
		TEST_ASSERT_EQUALS(std::string("flac"), d6.get_name(), "Should be created with the same name.");
	}

	// Test setting up a decoder/allocate_resources_memory()/create_codec_context()
	// and calling methods at wrong times.
	{
		ff::decoder d1(AVCodecID::AV_CODEC_ID_MPEG4);
		d1.create_codec_context();
		// Can only set properties when created()
		TEST_ASSERT_THROWS(d1.set_codec_properties(ff::codec_properties()), std::logic_error);

		TEST_ASSERT_TRUE(d1.ready(), "Should be ready.");
		auto p1 = d1.get_codec_properties();
		TEST_ASSERT_TRUE(p1.is_video(), "Should be created with correct properties.");

		ff::decoder d2("aac");
		d2.create_codec_context();

		TEST_ASSERT_TRUE(d2.ready(), "Should be ready.");
		auto p2 = d2.get_codec_properties();
		TEST_ASSERT_TRUE(p2.is_audio(), "Should be created with correct properties.");
	}

	// The demuxer requires the absolute path to files,
	// hence obtain this.
	fs::path working_dir(fs::current_path());

	// Test creating decoder directly from demuxer streams
	{
		// h.265
		fs::path test1_path(working_dir / "decoder_creation_x265.mp4");
		std::string test1_path_str(test1_path.generic_string());
		create_test_video(test1_path_str, "libx265", "red", 800, 600, 24, 1);
		ff::demuxer dem1(test1_path);
		ff::decoder dec1(dem1.get_stream(0));

		TEST_ASSERT_TRUE(dec1.ready(), "Should immediately be ready.");
		auto sp = dem1.get_stream(0).properties();
		auto dp = dec1.get_codec_properties();
		TEST_ASSERT_EQUALS(sp.type(), dp.type(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp.time_base(), dp.time_base(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp.v_pixel_format(), dp.v_pixel_format(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp.v_width(), dp.v_width(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp.v_height(), dp.v_height(), "Should actually have copied the settings.");

		// avi
		fs::path test2_path(working_dir / "decoder_creation.avi");
		std::string test2_path_str(test2_path.generic_string());
		create_test_video(test2_path_str, "mjpeg", "green", 800, 600, 24, 1);
		ff::demuxer dem2(test2_path);
		ff::decoder dec2(dem2.get_stream(0));

		TEST_ASSERT_TRUE(dec2.ready(), "Should immediately be ready.");
		auto sp2 = dem2.get_stream(0).properties();
		auto dp2 = dec2.get_codec_properties();
		TEST_ASSERT_EQUALS(sp2.type(), dp2.type(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.time_base(), dp2.time_base(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.v_pixel_format(), dp2.v_pixel_format(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.v_width(), dp2.v_width(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.v_height(), dp2.v_height(), "Should actually have copied the settings.");

		// mp3
		fs::path test3_path(working_dir / "decoder_creation_mp3.mp3");
		std::string test3_path_str(test3_path.generic_string());
		create_test_audio(test3_path_str, "libmp3lame", 1, 32000, 64000);
		ff::demuxer dem3(test3_path);
		ff::decoder dec3(dem3.get_stream(0));

		TEST_ASSERT_TRUE(dec3.ready(), "Should immediately be ready.");
		auto sp3 = dem3.get_stream(0).properties();
		auto dp3 = dec3.get_codec_properties();
		TEST_ASSERT_EQUALS(sp3.type(), dp3.type(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp3.time_base(), dp3.time_base(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.a_sample_format(), dp2.a_sample_format(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.a_sample_rate(), dp2.a_sample_rate(), "Should actually have copied the settings.");
		TEST_ASSERT_EQUALS(sp2.a_channel_layout_ref(), dp2.a_channel_layout_ref(), "Should actually have copied the settings.");
	}

	// Actual decoding test.
	// Decoding video with libx265 (hevc).
	{
		fs::path test1_path(working_dir / "decoder_test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		create_test_video(test1_path_str, "libx265", "green", 800, 600, 24, 5);
		ff::demuxer dem1(test1_path);

		// Create the decoder from ID.
		ff::stream vs = dem1.get_stream(0);
		TEST_ASSERT_TRUE(vs.is_video(), "Should be a video stream.");
		ff::decoder dec1(vs.codec_id());
		TEST_ASSERT_EQUALS(vs.codec_id(), dec1.get_id(), "Should be equal");
		TEST_ASSERT_TRUE(dec1.created(), "The decoder should be created now.");

		// Although the recommendation is to only set the needed ones and leave others their default values,
		// libx265 really needs some extra data that is hard to copy by hand.
		// So I make a full copy here.
		auto sp = vs.properties();
		// Should not stretch samples.
		TEST_ASSERT_EQUALS(ff::rational(1, 1), sp.v_sar(), "Should be equal");
		TEST_ASSERT_EQUALS(800, sp.v_width(), "Should be equal");
		TEST_ASSERT_EQUALS(600, sp.v_height(), "Should be equal");
		dec1.set_codec_properties(sp);

		// Create the decoder context so it can be ready.
		dec1.create_codec_context();
		TEST_ASSERT_TRUE(dec1.ready(), "The decoder should be ready now.");
		// Test if the properties really have changed to what's set.
		auto dp = dec1.get_codec_properties();
		TEST_ASSERT_EQUALS(sp.type(), dp.type(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_pixel_format(), dp.v_pixel_format(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_width(), dp.v_width(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_height(), dp.v_height(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(sp.v_sar(), dp.v_sar(), "Should really have set the properties.");

		// The decoding process.
		TEST_ASSERT_TRUE(dec1.hungry(), "Should be hungry initially.");
		TEST_ASSERT_FALSE(dec1.full(), "Should not be full initially.");
		TEST_ASSERT_FALSE(dec1.no_more_food(), "I have not signaled it.");

		ff::packet pkt;
		do
		{
			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				// The demuxer has run out of packets.
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
					TEST_ASSERT_EQUALS(dp.v_width(), fdp.width, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_height(), fdp.height, "Should get the properties right.");
				}
			} while (!f.destroyed());

			// the decoder must be hungry again.
			TEST_ASSERT_TRUE(dec1.hungry() && !dec1.full(), "Must be hungry now.");

		} while (!dem1.eof());

		// EOF from the demuxer. No more packets will be fed to the decoder.
		dec1.signal_no_more_food();
		
		TEST_ASSERT_TRUE(dec1.no_more_food(), "It must have responded to my signal.");
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
				TEST_ASSERT_EQUALS(dp.v_width(), fdp.width, "Should get the properties right.");
				TEST_ASSERT_EQUALS(dp.v_height(), fdp.height, "Should get the properties right.");
			}
		} while (!f.destroyed());
	}

	// Decoding audio with aac decoder
	{
		fs::path test1_path(working_dir / "decoder_test2.m4a");
		std::string test1_path_str(test1_path.generic_string());
		create_test_audio(test1_path_str, "aac", 3, 48000, 96000);

		ff::demuxer dem1(test1_path);

		auto as = dem1.get_stream(0);
		TEST_ASSERT_TRUE(as.is_audio(), "Should be an audio stream.");
		ff::decoder dec1(as.codec_id());
		TEST_ASSERT_EQUALS(as.codec_id(), dec1.get_id(), "Should be equal");
		TEST_ASSERT_TRUE(dec1.created(), "The decoder should be created now.");
		
		// the recommendation is to only set the needed ones and leave others their default values
		auto ap = as.properties();
		TEST_ASSERT_EQUALS(96000, ap.a_sample_rate(), "Should be equal");
		ff::codec_properties temp;
		temp.set_type(ap.type());
		temp.set_a_sample_rate(ap.a_sample_rate());
		temp.set_a_sample_format(ap.a_sample_format());
		temp.set_a_channel_layout(ap.a_channel_layout());
		dec1.set_codec_properties(temp);

		dec1.create_codec_context();
		TEST_ASSERT_TRUE(dec1.ready(), "The decoder should be ready now.");
		// Test if the properties really have changed to what's set.
		auto dp = dec1.get_codec_properties();
		TEST_ASSERT_EQUALS(ap.type(), dp.type(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(ap.a_sample_format(), dp.a_sample_format(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(ap.a_sample_rate(), dp.a_sample_rate(), "Should really have set the properties.");
		TEST_ASSERT_EQUALS(ap.a_channel_layout(), dp.a_channel_layout(), "Should really have set the properties.");
	
		// The decoding process.
		TEST_ASSERT_TRUE(dec1.hungry(), "Should be hungry initially.");
		TEST_ASSERT_FALSE(dec1.full(), "Should not be full initially.");
		TEST_ASSERT_FALSE(dec1.no_more_food(), "I have not signaled it.");

		ff::packet pkt;
		do
		{
			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				// The demuxer has run out of packets.
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
					TEST_ASSERT_TRUE(!fdp.v_or_a, "Should got the type right.");
					TEST_ASSERT_EQUALS(dp.a_sample_format(), fdp.fmt, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.a_channel_layout(), fdp.ch_layout, "Should get the properties right.");
				}
			} while (!dec1.hungry()); 
			// Note the above loop uses hungry() while the one in the video decoding test
			// uses f.destroyed(). They should mean the same now.

			// the decoder must be hungry again.
			TEST_ASSERT_TRUE(f.destroyed() && !dec1.full(), "Must be hungry now.");

		} while (!dem1.eof());

		// EOF from the demuxer. No more packets will be fed to the decoder.
		dec1.signal_no_more_food();

		TEST_ASSERT_TRUE(dec1.no_more_food(), "It must have responded to my signal.");
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
				TEST_ASSERT_TRUE(!fdp.v_or_a, "Should got the type right.");
				TEST_ASSERT_EQUALS(dp.a_sample_format(), fdp.fmt, "Should get the properties right.");
				TEST_ASSERT_EQUALS(dp.a_channel_layout(), fdp.ch_layout, "Should get the properties right.");
			}
		} while (!f.destroyed());
	}

	// Copying a decoder is forbidden/deleted
	
	// Test moving
	{
		// moving a created decoder
		ff::decoder d1("opus");
		ff::decoder m1(std::move(d1));
		TEST_ASSERT_TRUE(m1.created(), "Should get the status.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_OPUS, m1.get_id(), "Should get the desc.");
		TEST_ASSERT_EQUALS(std::string("opus"), m1.get_name(), "Should get the desc.");
		TEST_ASSERT_TRUE(d1.destroyed(), "Moved should be destroyed");

		// moving a ready decoder
		ff::decoder d2(AVCodecID::AV_CODEC_ID_JPEG2000);
		d2.create_codec_context();
		ff::decoder m2(std::move(d2));
		TEST_ASSERT_TRUE(m2.ready(), "Should get the status.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_JPEG2000, m2.get_id(), "Should get the desc.");
		TEST_ASSERT_TRUE(d2.destroyed(), "Moved should be destroyed");

		m2 = std::move(m1);
		TEST_ASSERT_TRUE(m2.created(), "Should get the status.");
		TEST_ASSERT_EQUALS(AVCodecID::AV_CODEC_ID_OPUS, m2.get_id(), "Should get the desc.");
		TEST_ASSERT_EQUALS(std::string("opus"), m2.get_name(), "Should get the desc.");
		TEST_ASSERT_TRUE(d1.destroyed(), "Moved should be destroyed");
	}

	// Test resetting
	{
		fs::path test1_path(working_dir / "decoder_test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		// Already created.
		//create_test_video(test1_path_str, "libx265", "green", 800, 600, 24, 5);
		ff::demuxer dem1(test1_path);

		// Create the decoder from ID.
		ff::stream vs = dem1.get_stream(0);
		ff::decoder dec1(vs.codec_id());

		// Although the recommendation is to only set the needed ones and leave others their default values,
		// libx265 really needs some extra data that is hard to copy by hand.
		// So I make a full copy here.
		auto sp = vs.properties();
		dec1.set_codec_properties(sp);

		// Create the decoder context so it can be ready.
		dec1.create_codec_context();
		auto dp = dec1.get_codec_properties();

		// Test resetting after draining is completed
		
		// First decoding 
		ff::packet pkt;
		do
		{
			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				// The demuxer has run out of packets.
				break;
			}

			bool full = !dec1.feed_packet(pkt);

			// Whether full or not, decode until it is hungry.
			ff::frame f;
			do
			{
				f = dec1.decode_frame();
			} while (!f.destroyed());

		} while (!dem1.eof());

		// EOF from the demuxer. No more packets will be fed to the decoder.
		dec1.signal_no_more_food();

		// Drain all frames from its stomach.
		ff::frame f;
		do
		{
			f = dec1.decode_frame();
		} while (!f.destroyed());

		// Now reset the decoder.
		dec1.reset();
		// Let the demuxer seek backwards to somewhere near the start
		dem1.seek(0, 1, false);

		TEST_ASSERT_TRUE(dec1.hungry(), "Should be hungry after a reset.");
		TEST_ASSERT_FALSE(dec1.full(), "Should not be full after a reset.");
		TEST_ASSERT_FALSE(dec1.no_more_food(), "I have not signaled it after a reset.");

		// decode again and test the decoded frames
		bool first_frame = true;
		do
		{
			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				// The demuxer has run out of packets.
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
					if (first_frame)
					{
						TEST_ASSERT_TRUE(f.av_frame()->pts <= 1, "Should start after the seeked position.");
						first_frame = false;
					}
					TEST_ASSERT_TRUE(fdp.v_or_a, "Should got the type right.");
					TEST_ASSERT_EQUALS(dp.v_pixel_format(), fdp.fmt, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_width(), fdp.width, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_height(), fdp.height, "Should get the properties right.");
				}
			} while (!dec1.hungry());

			// the decoder must be hungry again.
			TEST_ASSERT_TRUE(f.destroyed() && !dec1.full(), "Must be hungry now.");

		} while (!dem1.eof());

		// Now without signaling no_more_food,
		// reset the decoder.
		dec1.reset();
		// Seek the demuxer again.
		dem1.seek(0, 24, false);

		TEST_ASSERT_TRUE(dec1.hungry(), "Should be hungry after a reset.");
		TEST_ASSERT_FALSE(dec1.full(), "Should not be full after a reset.");
		TEST_ASSERT_FALSE(dec1.no_more_food(), "I have not signaled it after a reset.");

		// Decode again and test the decoded frames
		first_frame = true;
		do
		{
			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				// The demuxer has run out of packets.
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
					if (first_frame)
					{
						TEST_ASSERT_TRUE(f.av_frame()->pts <= 1, "Should start after the seeked position.");
						first_frame = false;
					}
					TEST_ASSERT_TRUE(fdp.v_or_a, "Should got the type right.");
					TEST_ASSERT_EQUALS(dp.v_pixel_format(), fdp.fmt, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_width(), fdp.width, "Should get the properties right.");
					TEST_ASSERT_EQUALS(dp.v_height(), fdp.height, "Should get the properties right.");
				}
			} while (!f.destroyed());

			// the decoder must be hungry again.
			TEST_ASSERT_TRUE(dec1.hungry() && !dec1.full(), "Must be hungry now.");

		} while (!dem1.eof());
	}

	// Full and hungry tests
	{
		fs::path test1_path(working_dir / "decoder_test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		// Already created.
		//create_test_video(test1_path_str, "libx265", "green", 800, 600, 24, 5);
		ff::demuxer dem1(test1_path);

		// Create the decoder from ID.
		ff::stream vs = dem1.get_stream(0);
		ff::decoder dec1(vs.codec_id());

		// Although the recommendation is to only set the needed ones and leave others their default values,
		// libx265 really needs some extra data that is hard to copy by hand.
		// So I make a full copy here.
		auto sp = vs.properties();
		dec1.set_codec_properties(sp);

		// Create the decoder context so it can be ready.
		dec1.create_codec_context();
		auto dp = dec1.get_codec_properties();

		// First, feed a decoder until it's full.
		// And keep feeding it after that.
		ff::packet pkt;
		bool full = false;
		do
		{
			pkt = dem1.demux_next_packet();

			// If the demuxer reaches the end, seek to the start to keep feeding.
			if (dem1.eof()) 
			{
				FF_ASSERT(pkt.destroyed(), "Should return a destroyed pkt when EOF.");
				dem1.seek(0, 1, false);
				continue;
			}

			full = !dec1.feed_packet(pkt);

		} while (!dec1.full());

		TEST_ASSERT_TRUE(full, "should be consistent");
		// Now dec1.feed_packet(pkt); should keep returning false.
		for (int i = 0; i < 10; ++i)
		{
			TEST_ASSERT_FALSE(dec1.feed_packet(pkt), "should not accept food while full.");
		}
		TEST_ASSERT_FALSE(dec1.hungry(), "Should not be hungry when full.");

		// Now decode until it is hungry
		ff::frame f;
		while (!dec1.hungry())
		{
			f = dec1.decode_frame();
		}
		TEST_ASSERT_TRUE(dec1.hungry() && f.destroyed(), "should be hungry and return a destroyed frame.");

		// Now keep decoding. It should keep returning a destroyed frame.
		for (int i = 0; i < 10; ++i)
		{
			TEST_ASSERT_TRUE(dec1.decode_frame().destroyed(), "should not be able to decode when hungry.");
		}
		TEST_ASSERT_FALSE(dec1.full(), "Should not be full when hungry.");
	}

	// Test giving invalid packets
	{
		fs::path test1_path(working_dir / "decoder_test1.mp4");
		std::string test1_path_str(test1_path.generic_string());
		// Already created.
		//create_test_video(test1_path_str, "libx265", "green", 800, 600, 24, 5);
		ff::demuxer dem1(test1_path);

		// Create the decoder from ID.
		ff::stream vs = dem1.get_stream(0);
		ff::decoder dec1(vs.codec_id());

		// Although the recommendation is to only set the needed ones and leave others their default values,
		// libx265 really needs some extra data that is hard to copy by hand.
		// So I make a full copy here.
		auto sp = vs.properties();
		dec1.set_codec_properties(sp);

		// Create the decoder context so it can be ready.
		dec1.create_codec_context();
		auto dp = dec1.get_codec_properties();

		// First feed some valid packets and decode all of them to prevent the decoder from being full.
		constexpr int num_decodings = 3;
		int i = 0;
		ff::packet pkt;
		do
		{
			if (i >= num_decodings)
			{
				break;
			}

			pkt = dem1.demux_next_packet();

			if (pkt.destroyed())
			{
				// The demuxer has run out of packets.
				break;
			}

			// Whether full or not, decode until it is hungry.
			do
			{
				dec1.decode_frame();
			} while (!dec1.hungry());

			++i;

		} while (!dem1.eof());

		// now feed invalid packets to it.
		ff::packet inv_pkt(true);
		inv_pkt.allocate_resources_memory(16);
		inv_pkt->pts = -1;

		// Try to feed the invalid pkt to the decoder.
		TEST_ASSERT_THROWS(dec1.feed_packet(inv_pkt), std::invalid_argument);
	}

	return 0;
}