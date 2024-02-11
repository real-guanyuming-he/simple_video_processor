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

#include "../../ff_wrapper/formats/muxer.h"
#include "../../ff_wrapper/data/frame.h"
#include "../../ff_wrapper/codec/encoder.h"

#include <cstdlib> // For std::system().
#include <filesystem> // For path handling as a demuxer requires an absolute path.
#include <format> // For std::format().
#include <string>

namespace fs = std::filesystem;

int main()
{
	fs::path working_dir(fs::current_path());

	// Test creation
	{
		fs::path test_path_1(working_dir / "muxer_test1.mp4");
		ff::muxer m1(test_path_1);

		TEST_ASSERT_EQUALS(0, m1.num_streams(), "Shoule be 0 when created.");
		TEST_ASSERT_EQUALS(0, m1.num_videos(), "Shoule be 0 when created.");
		TEST_ASSERT_EQUALS(0, m1.num_audios(), "Shoule be 0 when created.");
		TEST_ASSERT_EQUALS(0, m1.num_subtitles(), "Shoule be 0 when created.");

		// Because this demuxer isn't prepared and no streams have been added,
		// these should throw.
		TEST_ASSERT_THROWS(m1.prepare_muxer(), std::logic_error);
		ff::packet temp;
		TEST_ASSERT_THROWS(m1.mux_packet(temp), std::logic_error);
		TEST_ASSERT_THROWS(m1.flush_demuxer(), std::logic_error);
		TEST_ASSERT_THROWS(m1.finalize(), std::logic_error);
	}

	// Test adding streams and desired_stream_id()
	{
		fs::path test_path_2(working_dir / "muxer_test2.mp4");
		ff::muxer m2(test_path_2);

		// At least for mp4 FFmpeg the first two should succeed.
		ff::encoder venc(m2.desired_encoder_id(AVMEDIA_TYPE_VIDEO));
		ff::encoder aenc(m2.desired_encoder_id(AVMEDIA_TYPE_AUDIO));

		// Create a stream for each of them
		auto vs = m2.add_stream(venc);
		TEST_ASSERT_TRUE(vs.is_video(), "Should match the type.");
		TEST_ASSERT_EQUALS(venc.get_id(), vs.codec_id(), "Should match the encoder.");
		TEST_ASSERT_EQUALS(1, m2.num_streams(), "Should have recorded the addition.");
		TEST_ASSERT_EQUALS(1, m2.num_videos(), "Should have recorded the addition.");

		auto as = m2.add_stream(aenc);
		TEST_ASSERT_TRUE(as.is_audio(), "Should match the type.");
		TEST_ASSERT_EQUALS(aenc.get_id(), as.codec_id(), "Should match the encoder.");
		TEST_ASSERT_EQUALS(2, m2.num_streams(), "Should have recorded the addition.");
		TEST_ASSERT_EQUALS(1, m2.num_audios(), "Should have recorded the addition.");

		// Now create more streams to test the order
		auto vs2 = m2.add_stream(venc);
		TEST_ASSERT_TRUE(vs2.is_video(), "Should match the type.");
		TEST_ASSERT_EQUALS(venc.get_id(), vs2.codec_id(), "Should match the encoder.");
		TEST_ASSERT_EQUALS(3, m2.num_streams(), "Should have recorded the addition.");
		TEST_ASSERT_EQUALS(2, m2.num_videos(), "Should have recorded the addition.");

		auto as2 = m2.add_stream(aenc);
		TEST_ASSERT_TRUE(as2.is_audio(), "Should match the type.");
		TEST_ASSERT_EQUALS(aenc.get_id(), as2.codec_id(), "Should match the encoder.");
		TEST_ASSERT_EQUALS(4, m2.num_streams(), "Should have recorded the addition.");
		TEST_ASSERT_EQUALS(2, m2.num_audios(), "Should have recorded the addition.");

		TEST_ASSERT_EQUALS(4, m2.num_streams(), "Should have recorded the additions correctly.");
		auto s0 = m2.get_stream(0);
		TEST_ASSERT_EQUALS(vs.av_stream(), s0.av_stream(), "Should have the order correctly.");
		auto s1 = m2.get_stream(1);
		TEST_ASSERT_EQUALS(as.av_stream(), s1.av_stream(), "Should have the order correctly.");
		auto s2 = m2.get_stream(2);
		TEST_ASSERT_EQUALS(vs2.av_stream(), s2.av_stream(), "Should have the order correctly.");
		auto s3 = m2.get_stream(3);
		TEST_ASSERT_EQUALS(as2.av_stream(), s3.av_stream(), "Should have the order correctly.");

		TEST_ASSERT_EQUALS(vs.av_stream(), m2.get_video(0).av_stream(), "Should have the order correctly.");
		TEST_ASSERT_EQUALS(vs2.av_stream(), m2.get_video(1).av_stream(), "Should have the order correctly.");
		TEST_ASSERT_EQUALS(as.av_stream(), m2.get_audio(0).av_stream(), "Should have the order correctly.");
		TEST_ASSERT_EQUALS(as2.av_stream(), m2.get_audio(1).av_stream(), "Should have the order correctly.");

		// Don't know if mp4 has a desired subtitle encoder
		try
		{
			ff::encoder senc(m2.desired_encoder_id(AVMEDIA_TYPE_SUBTITLE));

			auto ss = m2.add_stream(senc);
			TEST_ASSERT_TRUE(ss.is_subtitle(), "Should match the type.");
			TEST_ASSERT_EQUALS(senc.get_id(), ss.codec_id(), "Should match the encoder.");
			TEST_ASSERT_EQUALS(5, m2.num_streams(), "Should have recorded the addition.");
			TEST_ASSERT_EQUALS(1, m2.num_subtitles(), "Should have recorded the addition.");

			auto s4 = m2.get_stream(4);
			TEST_ASSERT_EQUALS(ss.av_stream(), s4.av_stream(), "Should have the order correctly.");

			TEST_ASSERT_EQUALS(ss.av_stream(), m2.get_subtitle(0).av_stream(), "Should have the order correctly.");
		}
		catch (const std::domain_error&)
		{
			// Cannot find the desired subtitle encoder.
			// Do nothing.
		}
	}

	// Now mux some packets from "artificial" frames.
	{
		fs::path test_path_3(working_dir / "muxer_test3.mkv");
		ff::muxer m3(test_path_3);

		// mkv should be able to contain streams of any codec.
		ff::encoder venc3(AVCodecID::AV_CODEC_ID_AV1);
		ff::encoder aenc3(AVCodecID::AV_CODEC_ID_OPUS);

		// Set the properties of the encoders
		ff::codec_properties vp3;
		vp3.set_type_video();
		vp3.set_v_width(800); vp3.set_v_height(600);
		vp3.set_v_pixel_format(venc3.first_supported_v_pixel_format());
		vp3.set_time_base(ff::common_video_time_base_600);
		ff::codec_properties ap3;
		ap3.set_type_audio();
		// Should be 48000
		ap3.set_a_sample_rate(aenc3.first_supported_a_sample_rate());
		ap3.set_a_sample_format(aenc3.first_supported_a_sample_format());
		ap3.set_a_channel_layout(AVChannelLayout AV_CHANNEL_LAYOUT_STEREO);
		// Hence 96000
		ap3.set_time_base(ff::common_audio_time_base_96000);
		venc3.set_codec_properties(vp3);
		aenc3.set_codec_properties(ap3);

		// Create the encoder contexts
		venc3.create_codec_context();
		aenc3.create_codec_context();

		// Add streams to the muxer.
		m3.add_stream(venc3);
		m3.add_stream(aenc3);

		// Prepare the muxer
		m3.prepare_muxer();

		// Now produce the "artificial" frames
		ff::frame vf(true);
		ff::frame af(true);
		// Set the data properties
		ff::frame::data_properties vfd(vp3.v_pixel_format(), vp3.v_height(), vp3.v_width());
		constexpr int a_frame_rate = 100;
		const int a_num_samples_per_frame = ap3.a_sample_rate() / a_frame_rate;
		ff::frame::data_properties afd(ap3.a_sample_format(), a_num_samples_per_frame, ap3.a_channel_layout_ref());
		
	}

	return 0;
}