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
#include "../../ff_wrapper/codec/decoder.h"
#include "../../ff_wrapper/data/frame.h"

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

	/*
	* Encoding in this unit is like:
	* Feed one frame and encoder until the encoder is hungry.
	* Lemma 1: every frame will be fed successfully.
	* Proof.
	*	1. Establishment: initially the encoder is hungry so it will succeed.
	*	2. Maintanence: after a frame is fed, the encoder will be used until it is hungry again,
	* which means the next will succeed, too.
	*/

	FF_TEST_START

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
		ff::codec_properties cp1(e1.get_codec_properties());
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

		// Most encoders require that the properties be set properly for the ctx to be created.
		ff::codec_properties cp2(e2.get_codec_properties());
		cp2.set_time_base(ff::common_audio_time_base_64000);
		cp2.set_a_sample_format(AVSampleFormat::AV_SAMPLE_FMT_S32P);
		cp2.set_a_sample_rate(32000);
		cp2.set_a_channel_layout(ff::ff_AV_CHANNEL_LAYOUT_STEREO);
		e2.set_codec_properties(cp2);

		e2.create_codec_context();

		TEST_ASSERT_TRUE(e2.ready(), "Should be ready.");
		auto p2 = e2.get_codec_properties();
		TEST_ASSERT_TRUE(p2.is_audio(), "Should be created with correct properties.");
	}

	// Test ..._supported_... methods
	{
		// Some video encoder.
		ff::encoder e1(AVCodecID::AV_CODEC_ID_HEVC);

		try
		{
			auto e1pfs = e1.supported_v_pixel_formats();
			TEST_ASSERT_FALSE(e1pfs.empty(), "Valid codecs cannot support nothing.");
			for (auto pf : e1pfs)
			{
				TEST_ASSERT_TRUE(e1.is_v_pixel_format_supported(pf), "Should be consistent.");
			}
		}
		catch (const std::domain_error&) // don't know which are supported
		{
			// do nothing.
		}
		try
		{
			auto e1frs = e1.supported_v_frame_rates();
			TEST_ASSERT_FALSE(e1frs.empty(), "Valid codecs cannot support nothing.");
			for (auto fr : e1frs)
			{
				TEST_ASSERT_TRUE(e1.is_v_frame_rate_supported(fr), "Should be consistent.");
			}
		}
		catch (const std::domain_error&) // don't know which are supported
		{
			// do nothing.
		}

		// Some audio encoder.

		ff::encoder e2("aac");
		try
		{
			auto e2sfs = e2.supported_a_sample_formats();
			TEST_ASSERT_FALSE(e2sfs.empty(), "Valid codecs cannot support nothing.");
			for (auto sf : e2sfs)
			{
				TEST_ASSERT_TRUE(e2.is_a_sample_format_supported(sf), "Should be consistent.");
			}
		}
		catch (const std::domain_error&) // don't know which are supported
		{
			// do nothing.
		}

		try
		{
			auto e2srs = e2.supported_a_sample_rates();
			TEST_ASSERT_FALSE(e2srs.empty(), "Valid codecs cannot support nothing.");
			for (auto sr : e2srs)
			{
				TEST_ASSERT_TRUE(e2.is_a_sample_rate_supported(sr), "Should be consistent.");
			}
		}
		catch (const std::domain_error&) // don't know which are supported
		{
			// do nothing.
		}

		try
		{
			auto e2cls = e2.supported_a_channel_layouts();
			TEST_ASSERT_FALSE(e2cls.empty(), "Valid codecs cannot support nothing.");
			for (auto* cl : e2cls)
			{
				TEST_ASSERT_TRUE(e2.is_a_channel_layout_supported(*cl), "Should be consistent.");
			}
		}
		catch (const std::domain_error&) // don't know which are supported
		{
			// do nothing.
		}

	}

	// Some actual encoding.
	// First encoding some frames made by hand.
	{
		ff::encoder e1(AVCodecID::AV_CODEC_ID_HEVC);
		ff::codec_properties ep;

		ep.set_v_pixel_format(e1.supported_v_pixel_formats()[0]);
		ep.set_v_width(800);
		ep.set_v_height(600);
		ep.set_v_sar(ff::rational(1, 1));
		try
		{
			ep.set_v_frame_rate(e1.supported_v_frame_rates()[0]);
		}
		catch (const std::domain_error&) 
		// Don't know which frame rates are supported
		{
			ep.set_v_frame_rate(ff::rational(24));
		}
		ep.set_time_base(ff::common_video_time_base_600);
		// If type's not set, then it will throw
		TEST_ASSERT_THROWS(e1.set_codec_properties(ep), std::invalid_argument);
		ep.set_type_video();
		// If ID's not set, then it will throw
		TEST_ASSERT_THROWS(e1.set_codec_properties(ep), std::invalid_argument);
		ep.set_id(e1.get_id());

		e1.set_codec_properties(ep);
		e1.create_codec_context();

		TEST_ASSERT_TRUE(e1.ready(), "Should be ready.");
		TEST_ASSERT_TRUE(e1.hungry(), "Should be hungry initially.");
		TEST_ASSERT_FALSE(e1.full(), "Should not be full initially.");
		TEST_ASSERT_FALSE(e1.no_more_food(), "I have not signaled it.");

		// Let's encode 100 times
		constexpr int e1_pts_delta = 600 / 24;
		constexpr int encode_times = 100;
		ff::frame f(true);
		const ff::frame::data_properties fp
		(
			ep.v_pixel_format(),
			ep.v_width(), ep.v_height()
		);
		int64_t pts = 0;
		for (int i = 0; i < encode_times; ++i)
		{
			f.allocate_data(fp);

			f->pts = pts;
			TEST_ASSERT_TRUE(e1.feed_frame(f), "My lemma proven at the start of main()");
			TEST_ASSERT_FALSE(e1.hungry(), "Should not be hungry after eating");

			// encode until it's hungry
			ff::packet pkt;
			while (!e1.hungry())
			{
				pkt = e1.encode_packet();
				// May discover that it needs more.
				// Hence this check.
				if (!pkt.destroyed())
				{
					TEST_ASSERT_TRUE(pkt.ready(), "Should be ready.");
					// A packet is compressed so I won't know the actual size.
					TEST_ASSERT_TRUE(pkt.data_size() > 0, "Should have some data");

					TEST_ASSERT_FALSE(e1.full(), "Should not be full after encoding one.");
				}
			}
			TEST_ASSERT_TRUE(pkt.destroyed(), "Should return a destroyed pkt when hungry.");

			// Don't forget to do these
			pts += e1_pts_delta;
			f.release_resources_memory();
		}

		// And start draining.
		TEST_ASSERT_FALSE(e1.no_more_food(), "Still should be false.");
		e1.signal_no_more_food();
		TEST_ASSERT_TRUE(e1.no_more_food(), "Should be true now.");

		ff::frame f1(true);
		f1.allocate_data(fp);
		TEST_ASSERT_FALSE(e1.feed_frame(f1), "Should not accept any frame now.");

		ff::packet pkt;
		do
		{
			pkt = e1.encode_packet();
			if (!pkt.destroyed())
			{
				TEST_ASSERT_TRUE(pkt.ready(), "Should be ready.");
				// A packet is compressed so I won't know the actual size.
				TEST_ASSERT_TRUE(pkt.data_size() > 0, "Should have some data");
				TEST_ASSERT_FALSE(e1.full(), "Should not be full after encoding one.");
			}
		} while (!pkt.destroyed());

		TEST_ASSERT_TRUE(e1.encode_packet().destroyed(), "Should consistently return destroyed packets.");
	}
	
	// The demuxer requires the absolute path to files,
	// hence obtain this.
	fs::path working_dir(fs::current_path());

	// Now some genuien encoding (frames from a real decoder <- demuxer <- file).
	{
		fs::path test1_path(working_dir / "encoder_test1.mp3");
		std::string test1_path_str(test1_path.generic_string());
		create_test_audio(test1_path_str, "libmp3lame", 4, 32000, 64000);
		ff::demuxer dem1(test1_path);

		// Create the decoder from ID.
		ff::stream as = dem1.get_stream(0);
		ff::decoder dec1(as.codec_id());

		auto sp = as.properties();
		dec1.set_codec_properties(sp);

		// Create the decoder context so it can be ready.
		dec1.create_codec_context();
		auto dp = dec1.get_codec_properties();

		// Create the encoder from name
		ff::encoder enc1("libmp3lame");
		// Fill in the options (only necessary ones)
		enc1.set_properties_from_decoder(dec1);
		// Create the encoder context so it can be ready.
		enc1.create_codec_context();

		TEST_ASSERT_TRUE(enc1.ready(), "Should be ready.");
		TEST_ASSERT_TRUE(enc1.hungry(), "Should be hungry initially.");
		TEST_ASSERT_FALSE(enc1.full(), "Should not be full initially.");
		TEST_ASSERT_FALSE(enc1.no_more_food(), "I have not signaled it.");

		ff::packet pkt;
		do
		{
			pkt = dem1.demux_next_packet();
			if (pkt.destroyed()) // EOF
			{
				break;
			}

			dec1.feed_packet(pkt);
			// decode until hungry
			ff::frame f;
			do
			{
				f = dec1.decode_frame();
				if (f.destroyed()) // hungry now
				{
					break;
				}

				enc1.feed_frame(f);
				// encode until hungry
				ff::packet enc_pkt;
				do
				{
					enc_pkt = enc1.encode_packet();
					if (enc_pkt.destroyed()) // hungry now
					{
						TEST_ASSERT_TRUE(enc1.hungry(), "Must be consistent.");
						break;
					}

					// Test the encoded packet.
					TEST_ASSERT_TRUE(enc_pkt.ready(), "Should be ready.");
					// A packet is compressed so I won't know the actual size.
					TEST_ASSERT_TRUE(enc_pkt.data_size() > 0, "Should have some data");
					TEST_ASSERT_FALSE(enc1.full(), "Should not be full after encoding one.");
				} while (!enc1.hungry());

			} while (!dec1.hungry());

		} while (dem1.eof());

		// drain the decoder and continue encoding.
		dec1.signal_no_more_food();
		ff::frame f;
		do
		{
			f = dec1.decode_frame();
			if (f.destroyed()) // no more
			{
				break;
			}

			ff::packet enc_pkt;
			do
			{
				enc_pkt = enc1.encode_packet();
				if (enc_pkt.destroyed()) // hungry now
				{
					TEST_ASSERT_TRUE(enc1.hungry(), "Must be consistent.");
					break;
				}

				// Test the encoded packet.
				TEST_ASSERT_TRUE(enc_pkt.ready(), "Should be ready.");
				// A packet is compressed so I won't know the actual size.
				TEST_ASSERT_TRUE(enc_pkt.data_size() > 0, "Should have some data");
				TEST_ASSERT_FALSE(enc1.full(), "Should not be full after encoding one.");
			} while (!enc1.hungry());
		} while (!f.destroyed());

		// drain the encoder
		TEST_ASSERT_FALSE(enc1.no_more_food(), "Still should be false.");
		enc1.signal_no_more_food();
		TEST_ASSERT_TRUE(enc1.no_more_food(), "Should be true now.");

		ff::frame f1(true);
		f1.allocate_data(ff::frame::data_properties(dp.a_sample_format(), 16, dp.a_channel_layout()));
		TEST_ASSERT_FALSE(enc1.feed_frame(f1), "Should not accept any frame now.");

		ff::packet enc_pkt;
		do
		{
			enc_pkt = enc1.encode_packet();
			if (!enc_pkt.destroyed())
			{
				TEST_ASSERT_TRUE(enc_pkt.ready(), "Should be ready.");
				// A packet is compressed so I won't know the actual size.
				TEST_ASSERT_TRUE(enc_pkt.data_size() > 0, "Should have some data");
				TEST_ASSERT_FALSE(enc1.full(), "Should not be full after encoding one.");
			}
		} while (!enc_pkt.destroyed());

		TEST_ASSERT_TRUE(enc1.encode_packet().destroyed(), "Should consistently return destroyed packets.");
	}

	// reset() is implemented in codec_base and has been tested by the decoder tests.

	// moving is mostly covered by decoder tests,
	// but it doesn't hurt to additionally test if encoder created by moving
	// can encoder normally
	{
		ff::encoder e1(AVCodecID::AV_CODEC_ID_AV1);
		ff::codec_properties ep;

		ep.set_v_pixel_format(e1.supported_v_pixel_formats()[0]);
		ep.set_v_width(800);
		ep.set_v_height(600);
		ep.set_v_sar(ff::rational(1, 1));
		try
		{
			ep.set_v_frame_rate(e1.supported_v_frame_rates()[0]);
		}
		catch (const std::domain_error&)
			// Don't know which frame rates are supported
		{
			ep.set_v_frame_rate(ff::rational(24));
		}
		ep.set_time_base(ff::common_video_time_base_600);
		// If type's not set, then it will throw
		TEST_ASSERT_THROWS(e1.set_codec_properties(ep), std::invalid_argument);
		ep.set_type_video();

		// If ID's not set, then it will throw
		TEST_ASSERT_THROWS(e1.set_codec_properties(ep), std::invalid_argument);
		ep.set_id(e1.get_id());

		e1.set_codec_properties(ep);
		e1.create_codec_context();

		// Let's encode 5 times using e1
		constexpr int e1_pts_delta = 600 / 24;
		// Careful when enlarging it, because the encoding is painfully slow!
		constexpr int encode_times = 5; 
		ff::frame f(true);
		const ff::frame::data_properties fp
		(
			ep.v_pixel_format(),
			ep.v_width(), ep.v_height()
		);
		int64_t pts = 0;
		for (int i = 0; i < encode_times; ++i)
		{
			f.allocate_data(fp);

			f->pts = pts;
			TEST_ASSERT_TRUE(e1.feed_frame(f), "My lemma proven at the start of main()");
			TEST_ASSERT_FALSE(e1.hungry(), "Should not be hungry after eating");

			// encode until it's hungry
			ff::packet pkt;
			while (!e1.hungry())
			{
				pkt = e1.encode_packet();
				// May discover that it needs more.
				// Hence this check.
				if (!pkt.destroyed())
				{
					TEST_ASSERT_TRUE(pkt.ready(), "Should be ready.");
					// A packet is compressed so I won't know the actual size.
					TEST_ASSERT_TRUE(pkt.data_size() > 0, "Should have some data");

					TEST_ASSERT_FALSE(e1.full(), "Should not be full after encoding one.");
				}
			}
			TEST_ASSERT_TRUE(pkt.destroyed(), "Should return a destroyed pkt when hungry.");

			// Don't forget to do these
			pts += e1_pts_delta;
			f.release_resources_memory();
		}

		// then create m1 moved from e1
		ff::encoder m1(std::move(e1));
		TEST_ASSERT_TRUE(m1.ready() && e1.destroyed(), "Should be moved.");

		// and encode 5 times using m1.
		for (int i = 0; i < encode_times; ++i)
		{
			f.allocate_data(fp);

			f->pts = pts;
			TEST_ASSERT_TRUE(m1.feed_frame(f), "My lemma proven at the start of main()");
			TEST_ASSERT_FALSE(m1.hungry(), "Should not be hungry after eating");

			// encode until it's hungry
			ff::packet pkt;
			while (!m1.hungry())
			{
				pkt = m1.encode_packet();
				// May discover that it needs more.
				// Hence this check.
				if (!pkt.destroyed())
				{
					TEST_ASSERT_TRUE(pkt.ready(), "Should be ready.");
					// A packet is compressed so I won't know the actual size.
					TEST_ASSERT_TRUE(pkt.data_size() > 0, "Should have some data");

					TEST_ASSERT_FALSE(m1.full(), "Should not be full after encoding one.");
				}
			}
			TEST_ASSERT_TRUE(pkt.destroyed(), "Should return a destroyed pkt when hungry.");

			// Don't forget to do these
			pts += e1_pts_delta;
			f.release_resources_memory();
		}

		// And start draining.
		TEST_ASSERT_FALSE(m1.no_more_food(), "Still should be false.");
		m1.signal_no_more_food();
		TEST_ASSERT_TRUE(m1.no_more_food(), "Should be true now.");

		ff::frame f1(true);
		f1.allocate_data(fp);
		TEST_ASSERT_FALSE(m1.feed_frame(f1), "Should not accept any frame now.");

		ff::packet pkt;
		do
		{
			pkt = m1.encode_packet();
			if (!pkt.destroyed())
			{
				TEST_ASSERT_TRUE(pkt.ready(), "Should be ready.");
				// A packet is compressed so I won't know the actual size.
				TEST_ASSERT_TRUE(pkt.data_size() > 0, "Should have some data");
				TEST_ASSERT_FALSE(m1.full(), "Should not be full after encoding one.");
			}
		} while (!pkt.destroyed());

		TEST_ASSERT_TRUE(m1.encode_packet().destroyed(), "Should consistently return destroyed packets.");
	}

	// Full and hungry tests
	{
		ff::encoder e1(AVCodecID::AV_CODEC_ID_GIF);
		ff::codec_properties ep;

		ep.set_v_pixel_format(e1.supported_v_pixel_formats()[0]);
		ep.set_v_width(800);
		ep.set_v_height(600);
		ep.set_v_sar(ff::rational(1, 1));
		try
		{
			ep.set_v_frame_rate(e1.supported_v_frame_rates()[0]);
		}
		catch (const std::domain_error&)
			// Don't know which frame rates are supported
		{
			ep.set_v_frame_rate(ff::rational(24));
		}
		ep.set_time_base(ff::common_video_time_base_600);
		// If type's not set, then it will throw
		TEST_ASSERT_THROWS(e1.set_codec_properties(ep), std::invalid_argument);
		ep.set_type_video();

		// If ID's not set, then it will throw
		TEST_ASSERT_THROWS(e1.set_codec_properties(ep), std::invalid_argument);
		ep.set_id(e1.get_id());

		e1.set_codec_properties(ep);
		e1.create_codec_context();

		TEST_ASSERT_TRUE(e1.ready(), "Should be ready.");
		TEST_ASSERT_TRUE(e1.hungry(), "Should be hungry initially.");
		TEST_ASSERT_FALSE(e1.full(), "Should not be full initially.");
		TEST_ASSERT_FALSE(e1.no_more_food(), "I have not signaled it.");

		// Now keep trying to encode but it should not change the state at all
		for (int i = 0; i < 10; ++i)
		{
			TEST_ASSERT_TRUE(e1.encode_packet().destroyed(), "should not be able to encode when hungry.");
		}

		// Now feed until full.
		ff::frame f(true);
		f->time_base = ep.time_base().av_rational();
		int64_t pts = 0;
		constexpr int64_t pts_delta = 600 / 24;
		ff::frame::data_properties fp(ep.v_pixel_format(), ep.v_width(), ep.v_height());

		bool full = false;
		while (!e1.full())
		{
			f.allocate_data(fp);

			f->pts = pts;
			full = !e1.feed_frame(f);

			// Don't forget to do these
			pts += pts_delta;
			f.release_resources_memory();
		}
		TEST_ASSERT_TRUE(full, "should be consistent");

		// Now keep trying to feed, but should get false always.
		f.allocate_data(fp);
		for (int i = 0; i < 10; ++i)
		{
			TEST_ASSERT_FALSE(e1.feed_frame(f), "should not be able to be fed when full.");
		}
	}

	// Test feeding invalid frame
	{
		ff::encoder e1(AVCodecID::AV_CODEC_ID_AAC);
		ff::codec_properties ep(e1.get_codec_properties());
		ep.set_a_sample_format(e1.supported_a_sample_formats()[0]);
		ep.set_a_sample_rate(32000);
		try
		{
			ep.set_a_channel_layout(*e1.supported_a_channel_layouts()[0]);
		}
		catch (const std::domain_error&)
		{
			// Don't know which are supported
			ep.set_a_channel_layout(AVChannelLayout AV_CHANNEL_LAYOUT_STEREO);
		}
		ep.set_time_base(ff::common_audio_time_base_64000);

		e1.set_codec_properties(ep);
		e1.create_codec_context();
		TEST_ASSERT_TRUE(e1.hungry(), "should be hungry initially.");
		// Now keep trying to encode but it should not change the state at all
		for (int i = 0; i < 10; ++i)
		{
			TEST_ASSERT_TRUE(e1.encode_packet().destroyed(), "should not be able to encode when hungry.");
		}

		ff::frame f(true);
		f->time_base = ep.time_base().av_rational();
		constexpr int num_samples = 32;
		constexpr auto frame_duration = ff::time
		(
			num_samples,
			ff::rational(1, 32000)
		);
		constexpr auto pts_delta =
			ff::time::change_time_base(frame_duration, ff::common_audio_time_base_64000).timestamp_approximate();

		int64_t pts = 0;
		ff::frame::data_properties fp(ep.a_sample_format(), num_samples, ep.a_channel_layout());
		ff::frame::data_properties different_fp
		(
			AVPixelFormat::AV_PIX_FMT_RGB24,
			800, 600
		);

		// Feed once.
		f.allocate_data(fp);
		e1.feed_frame(f);
		f.release_resources_memory();

		// Encode until hungry
		while (!e1.hungry())
		{
			e1.encode_packet();
		}

		ff::frame f1(true);
		// feed an invalid packet (with different properties).
		f1.allocate_data(different_fp);
		TEST_ASSERT_THROWS(e1.feed_frame(f1), std::invalid_argument);
	}

	// Test the two versions of encode_packet()
	{
		// Feed the decoder until it's full before 
		// decoding a frame so that decode_frame will always decode one.

		ff::encoder e1(AVCodecID::AV_CODEC_ID_GIF);
		ff::codec_properties ep;

		ep.set_v_pixel_format(e1.supported_v_pixel_formats()[0]);
		ep.set_v_width(800);
		ep.set_v_height(600);
		ep.set_v_sar(ff::rational(1, 1));
		try
		{
			ep.set_v_frame_rate(e1.supported_v_frame_rates()[0]);
		}
		catch (const std::domain_error&)
			// Don't know which frame rates are supported
		{
			ep.set_v_frame_rate(ff::rational(24));
		}
		ep.set_time_base(ff::common_video_time_base_600);
		ep.set_type_video();
		ep.set_id(e1.get_id());

		e1.set_codec_properties(ep);
		e1.create_codec_context();

		ff::frame f(true);
		f->time_base = ep.time_base().av_rational();
		int64_t pts = 0;
		constexpr int64_t pts_delta = 600 / 24;
		ff::frame::data_properties fp(ep.v_pixel_format(), ep.v_width(), ep.v_height());

		/*
		* Feed the encoder frames until it's full.
		*/
		auto make_enc_full = [&]() -> void
		{
			do
			{
				f.allocate_data(fp);
				f->pts = pts;

				e1.feed_frame(f);

				pts += pts_delta;
				f.release_resources_memory();

			} while (!e1.full());
		};

		// Test the method that supports reusing packet
		ff::packet pkt(false);
		// Now f is destroyed.
		make_enc_full();
		TEST_ASSERT_TRUE(e1.encode_packet(pkt), "Should succeed");
		TEST_ASSERT_TRUE(pkt.ready(), "pkt should have been made ready.");

		pkt = ff::packet(true);
		// Now pkt is created;
		make_enc_full();
		TEST_ASSERT_TRUE(e1.encode_packet(pkt), "Should succeed");
		TEST_ASSERT_TRUE(pkt.ready(), "pkt should have been made ready.");

		// Now pkt is ready.
		const auto* prev_data = pkt.data();
		make_enc_full();
		TEST_ASSERT_TRUE(e1.encode_packet(pkt), "Should succeed");
		TEST_ASSERT_TRUE(pkt.ready(), "pkt should have been made ready.");
		// The new data might be allocated at the same location as the previous data's
		// TEST_ASSERT_TRUE(prev_data != pkt.data(), "Should have new data.");

		// Now decode until hungry to make the next call to decode_frame() fail
		while (!e1.hungry())
		{
			e1.encode_packet();
		}
		// Now pkt is ready.
		TEST_ASSERT_FALSE(e1.encode_packet(pkt), "Should fail");
		TEST_ASSERT_TRUE(pkt.created(), "pkt should have been made created");
		// Now f is created.
		TEST_ASSERT_FALSE(e1.encode_packet(pkt), "Should fail");
		TEST_ASSERT_TRUE(pkt.created(), "pkt should have been made created");
		pkt.release_object_memory();
		// Now pkt is destroyed.
		TEST_ASSERT_FALSE(e1.encode_packet(pkt), "Should fail");
		TEST_ASSERT_TRUE(pkt.created(), "pkt should have been made created");

		// Test the method that returns a frame.
		make_enc_full();
		pkt = e1.encode_packet();
		TEST_ASSERT_TRUE(pkt.ready(), "f should have been made ready.");

		// Now decode until hungry to make the next call to decode_frame() fail
		while (!e1.hungry())
		{
			e1.encode_packet();
		}
		pkt = e1.encode_packet();
		TEST_ASSERT_TRUE(pkt.destroyed(), "f should have been made destroyed.");
	}

	FF_TEST_END

	return 0;
}