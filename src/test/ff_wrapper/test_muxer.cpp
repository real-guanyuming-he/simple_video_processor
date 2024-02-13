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
#include "../../ff_wrapper/sws/frame_transformer.h"

#include <iostream>
#include <cstdlib> // For std::system().
#include <filesystem> // For path handling as a demuxer requires an absolute path.
#include <format> // For std::format().
#include <string>

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace fs = std::filesystem;

int main()
{
	FF_TEST_START

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
		TEST_ASSERT_THROWS(m1.mux_packet_auto(temp), std::logic_error);
		TEST_ASSERT_THROWS(m1.mux_packet_manual(temp), std::logic_error);
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

		ff::encoder venc3(m3.desired_encoder_id(AVMEDIA_TYPE_VIDEO));
		//ff::encoder aenc3("libopus");

		// Set the properties of the encoders
		constexpr int v_frame_rate = 24;
		ff::codec_properties vp3 = venc3.get_codec_properties();
		vp3.set_v_width(800); vp3.set_v_height(600);
		// YUV420
		vp3.set_v_pixel_format(venc3.first_supported_v_pixel_format());
		vp3.set_v_frame_rate(ff::rational(v_frame_rate, 1));
		vp3.set_time_base(ff::common_video_time_base_600);
		vp3.set_v_sar(ff::rational(1, 1));
		
		//ff::codec_properties ap3 = aenc3.get_codec_properties();
		//// Should be 48000
		//ap3.set_a_sample_rate(aenc3.first_supported_a_sample_rate());
		//ap3.set_a_sample_format(aenc3.first_supported_a_sample_format());
		//ap3.set_a_channel_layout(ff::ff_AV_CHANNEL_LAYOUT_STEREO);
		//// Hence 96000
		//ap3.set_time_base(ff::common_audio_time_base_96000);

		// Create the encoder contexts
		venc3.set_codec_properties(vp3);
		//aenc3.set_codec_properties(ap3);
		venc3.create_codec_context();
		//aenc3.create_codec_context();

		// Creating the context may change some properties.
		// Obtain the final properties after the encoders are created.
		const auto cvp3 = venc3.get_codec_properties();
		//const auto cap3 = aenc3.get_codec_properties();

		// Add streams to the muxer.
		auto vs = m3.add_stream(venc3);
		//auto as = m3.add_stream(aenc3);

		// Prepare the muxer
		m3.prepare_muxer();

		// Now produce the "artificial" frames
		ff::frame af(true);
		// Set the data properties
		ff::frame::data_properties vfd(cvp3.v_pixel_format(), cvp3.v_width(), cvp3.v_height());
		//ff::frame::data_properties afd(cap3.a_sample_format(), cap3.a_frame_num_samples(), cap3.a_channel_layout_ref());
		/*const int a_frame_rate =
			cap3.a_sample_rate() /
			(cap3.a_frame_num_samples() * cap3.a_channel_layout_ref().nb_channels);*/
		const auto v_tb = cvp3.time_base();
		//const auto a_tb = cap3.time_base();
		const int v_pts_delta = v_tb.get_den() / v_frame_rate;
		//const int a_pts_delta = a_tb.get_den() / a_frame_rate;

		// Use swscale to convert this rgb frame to vf
		ff::frame imagef(true);
		ff::frame::data_properties ifd(AVPixelFormat::AV_PIX_FMT_RGB24, cvp3.v_width(), cvp3.v_height());
		constexpr int rgb_pixel_size = 3;
		imagef.allocate_data(ifd);
		auto* idata = imagef.data<uint8_t>();
		auto line_size = imagef.line_size();
		// Write the image.
		for (int row = 0; row < cvp3.v_height(); ++row)
		{
			for (int col = 0; col < cvp3.v_width(); ++col)
			{
				auto* pixel =
					idata +
					row * line_size +
					col * rgb_pixel_size;
				// r
				*pixel = 24;
				// g
				*(pixel + 1) = 255;
				// b
				*(pixel + 2) = 24;
			}
		}

		ff::frame_transformer trans(vfd, ifd);

		// Encode them and send them to the muxer
		// for this long
		constexpr int file_duration_secs = 2;
		constexpr int v_num_frames = v_frame_rate * file_duration_secs;
		//const int a_num_frames = a_frame_rate * file_duration_secs;
		// a_frame_rate = v_frame_rate
		for (int i = 0; i < v_num_frames; ++i)
		{
			// Convert the image to the target format.
			auto vf = trans.convert_frame(imagef);

			// Don't forget to set the time
			int64_t pts = (int64_t)i * (int64_t)v_pts_delta;
			int64_t duration = v_pts_delta;
			vf.reset_time
			(
				pts, v_tb
				//,duration
			);

			venc3.feed_frame(vf);
			while (!venc3.hungry())
			{
				ff::packet vpkt = venc3.encode_packet();
				if (!vpkt.destroyed())
				{
					// prepare the packet
					// pts should have been set by the encoder
					if (ff::av_rational_invalid_or_zero(vpkt->time_base))
					{
						vpkt->time_base = cvp3.time_base().av_rational();
					}
					vpkt.prepare_for_muxing(vs);

					// feed it to the muxer
					m3.mux_packet_auto(vpkt);
				}
			}

			// Don't forget to do this.
			vf.release_resources_memory();
		}

		//for (int i = 0; i < a_num_frames; ++i)
		//{
		//	// Don't care what's inside the data.
		//	// Just allocate them.
		//	af.allocate_data(afd);

		//	af->sample_rate = cap3.a_sample_rate();

		//	// Don't forget to set the time
		//	int64_t pts = (int64_t)i * (int64_t)a_pts_delta;
		//	int64_t duration = a_pts_delta;
		//	af.reset_time(pts, duration, a_tb);

		//	aenc3.feed_frame(af);
		//	while (!aenc3.hungry())
		//	{
		//		ff::packet apkt = aenc3.encode_packet();
		//		if (!apkt.destroyed())
		//		{
		//			// prepare the packet
		//			// pts should have been set by the encoder
		//			apkt.validify_dts();
		//			apkt->time_base = af->time_base;
		//			apkt.prepare_for_muxing(as);

		//			// feed it to the muxer
		//			m3.mux_packet_auto(apkt);
		//		}
		//	}

		//	// Don't forget to do this.
		//	af.release_resources_memory();
		//}

		// Now drain the encoders
		venc3.signal_no_more_food();
		//aenc3.signal_no_more_food();

		ff::packet vpkt(false);
		ff::packet apkt(false);

		auto vi = v_num_frames;
		//auto ai = a_num_frames;
		do
		{
			vpkt = venc3.encode_packet();
			if (!vpkt.destroyed())
			{
				// prepare the packet
				// pts should have been set by the encoder
				vpkt.validify_dts();
				vpkt->time_base = v_tb.av_rational();
				vpkt.prepare_for_muxing(vs);

				// feed it to the muxer
				m3.mux_packet_auto(vpkt);
			}

			++vi;
		} while (!vpkt.destroyed());

		// The muxer should protest if I call both auto and manual muxing methods.
		// This should happen before checking the packet.
		// Hence just use a destroyed temp packet.
		{
			ff::packet temp(false);
			TEST_ASSERT_THROWS(m3.mux_packet_manual(temp), std::logic_error);
		}

		//do
		//{
		//	apkt = aenc3.encode_packet();
		//	if (!apkt.destroyed())
		//	{
		//		// prepare the packet
		//		// pts should have been set by the encoder
		//		apkt.validify_dts();
		//		apkt->time_base = af->time_base;
		//		apkt.prepare_for_muxing(as);

		//		// feed it to the muxer
		//		m3.mux_packet_auto(apkt);
		//	}

		//	++ai;
		//} while (!apkt.destroyed());

		// Finalize the muxing
		m3.finalize();	
	}


	// Now the test won't pass.
	// Probably I didn't set up the encoder properly (but I can't think of why).
	// Investigating.
	FF_TEST_END

	return 0;

}