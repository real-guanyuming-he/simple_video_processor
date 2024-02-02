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

/*
* Because a frame is more complicated than a packet,
* I create dedicated tests just for it, instead of testing it in the integration tests of the codecs.
*/

#include "../test_util.h"
#include "../../ff_wrapper/data/frame.h"

extern "C"
{
#include <libavutil/frame.h>
}

int main()
{
	// Test creation
	{
		ff::frame f1(false);
		TEST_ASSERT_TRUE(f1.destroyed(), "Should be created destroyed");

		ff::frame f2(true);
		TEST_ASSERT_TRUE(f2.created(), "Should be allocated.");

		AVFrame* avf = av_frame_alloc();
		avf->format = AVPixelFormat::AV_PIX_FMT_ABGR;
		avf->width = 800; avf->height = 600;
		av_frame_get_buffer(avf, 0);

		ff::frame f3(avf, true);
		TEST_ASSERT_TRUE(f3.ready(), "Should be ready.");
		TEST_ASSERT_TRUE(f3.v_or_a(), "Should set video or audio");
		TEST_ASSERT_TRUE(0 != f3.number_planes(), "Should set number planes");
		TEST_ASSERT_EQUALS(avf, f3.av_frame(), "Should take over the frame");
	}

	// Test move
	{
		// Test moving destroyed frame
		ff::frame f1(false);
		ff::frame m1(std::move(f1));
		TEST_ASSERT_TRUE(f1.destroyed() && m1.destroyed(), "Should both be destroyed");

		// Test moving allocated frame
		ff::frame f2(true);
		auto* f2avf = f2.av_frame();
		ff::frame m2(std::move(f2));
		TEST_ASSERT_TRUE(f2.destroyed(), "Moved should be destroyed");
		TEST_ASSERT_TRUE(m2.created(), "Should get the status");
		TEST_ASSERT_EQUALS(f2avf, m2.av_frame(), "Should take over the avframe");

		// Move frame with data
		AVFrame* avf = av_frame_alloc();
		avf->format = AVPixelFormat::AV_PIX_FMT_ABGR;
		avf->width = 800; avf->height = 600;
		av_frame_get_buffer(avf, 0);
		ff::frame f3(avf, true);
		auto f3dp = f3.get_data_properties();
		auto f3np = f3.number_planes();
		ff::frame m3(std::move(f3));
		auto m3dp = m3.get_data_properties();
		auto m3np = m3.number_planes();
		TEST_ASSERT_TRUE(f3.destroyed(), "Moved should be destroyed");
		TEST_ASSERT_TRUE(m3.ready(), "Should get the status");
		TEST_ASSERT_EQUALS(f3np, m3np, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.v_or_a, m3dp.v_or_a, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.fmt, m3dp.fmt, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.details.v.height, m3dp.details.v.height, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.details.v.width, m3dp.details.v.width, "Should equal");

		// Move ass operator
		m3 = std::move(m2);
		TEST_ASSERT_TRUE(m2.destroyed(), "Moved should be destroyed");
		TEST_ASSERT_TRUE(m3.created(), "Should get the status");
	}

	// Test copy
	{
		// Test copying destroyed frame
		ff::frame f1(false);
		ff::frame c1(f1);
		TEST_ASSERT_TRUE(f1.destroyed() && c1.destroyed(), "Should both be destroyed");

		// Test copying allocated frame
		ff::frame f2(true);
		ff::frame c2(f2);
		TEST_ASSERT_TRUE(f2.created() && c2.created(), "Should both be created");

		// Test copying frame with data
		AVFrame* avf = av_frame_alloc();
		avf->format = AVPixelFormat::AV_PIX_FMT_ABGR;
		avf->width = 800; avf->height = 600;
		av_frame_get_buffer(avf, 0);
		for (int i = 0; i < 800; ++i)
		{
			*(avf->data[0] + i) = (i + 1) % 256;
		}
		ff::frame f3(avf, true);
		auto f3dp = f3.get_data_properties();
		auto f3np = f3.number_planes();
		ff::frame c3(f3);
		auto f3dp_now = f3.get_data_properties();
		auto f3np_now = f3.number_planes();
		auto c3dp = c3.get_data_properties();
		auto c3np = c3.number_planes();

		TEST_ASSERT_TRUE(c3.ready(), "Should get the status");
		TEST_ASSERT_EQUALS(f3np, c3np, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.v_or_a, c3dp.v_or_a, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.fmt, c3dp.fmt, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.details.v.height, c3dp.details.v.height, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.details.v.width, c3dp.details.v.width, "Should equal");
		auto c3d = c3.data();
		for (int i = 0; i < 800; ++i)
		{
			TEST_ASSERT_EQUALS((i + 1) % 256, *(static_cast<uint8_t*>(c3d) + i), "Should copy the data");
		}

		TEST_ASSERT_TRUE(f3.ready(), "Should not change the copied");
		TEST_ASSERT_EQUALS(f3np, f3np_now, "Should not change the copied");
		TEST_ASSERT_EQUALS(f3dp.v_or_a, f3dp_now.v_or_a, "Should not change the copied");
		TEST_ASSERT_EQUALS(f3dp.fmt, f3dp_now.fmt, "Should not change the copied");
		TEST_ASSERT_EQUALS(f3dp.details.v.height, f3dp_now.details.v.height, "Should not change the copied");
		TEST_ASSERT_EQUALS(f3dp.details.v.width, f3dp_now.details.v.width, "Should not change the copied");
		*(static_cast<uint8_t*>(c3d) + 5) = 9;
		TEST_ASSERT_EQUALS((5 + 1) % 256, *(static_cast<uint8_t*>(f3.data()) + 5), "Should deeply copy the data");
	}

	// Test allocating memory and data accessors.
	{
		// Video allocation
		ff::frame::data_properties dp1(AVPixelFormat::AV_PIX_FMT_ARGB, 1280, 720, 0);
		ff::frame f1(true);
		TEST_ASSERT_THROWS(f1.number_planes(), std::logic_error);
		TEST_ASSERT_THROWS(f1.data(), std::logic_error);
		TEST_ASSERT_THROWS(f1.line_size(), std::logic_error);
		TEST_ASSERT_THROWS(f1.get_data_properties(), std::logic_error);

		f1.allocate_data(dp1);
		// Can't call it twice.
		// f1.allocate_data(dp1);
		auto f1dp = f1.get_data_properties();

		TEST_ASSERT_TRUE(f1.ready(), "Should be ready now");
		TEST_ASSERT_EQUALS(dp1.v_or_a, f1dp.v_or_a, "Should equal");
		TEST_ASSERT_EQUALS(dp1.fmt, f1dp.fmt, "Should equal");
		TEST_ASSERT_EQUALS(dp1.details.v.height, f1dp.details.v.height, "Should equal");
		TEST_ASSERT_EQUALS(dp1.details.v.width, f1dp.details.v.width, "Should equal");

		TEST_ASSERT_TRUE(f1.number_planes() > 0, "Should have data.");
		TEST_ASSERT_TRUE(std::abs(f1.line_size()) >= 1280, "Should have data");
		TEST_ASSERT_TRUE(f1.data() != nullptr, "Should have data.");

		TEST_ASSERT_THROWS(f1.data(-1), std::out_of_range);
		TEST_ASSERT_THROWS(f1.data(f1.number_planes()), std::out_of_range);
		TEST_ASSERT_THROWS(f1.line_size(f1.number_planes()+1), std::out_of_range);

		// Audio allocation
		ff::frame::data_properties dp2
		(
			AVSampleFormat::AV_SAMPLE_FMT_S16, 144, 
			AVChannelLayout AV_CHANNEL_LAYOUT_STEREO
		);
		ff::frame f2(true);

		f2.allocate_data(dp2);
		auto f2dp = f2.get_data_properties();

		TEST_ASSERT_TRUE(f2.ready(), "Should be ready now");
		TEST_ASSERT_EQUALS(dp2.v_or_a, f2dp.v_or_a, "Should equal");
		TEST_ASSERT_EQUALS(dp2.fmt, f2dp.fmt, "Should equal");
		TEST_ASSERT_EQUALS(dp2.details.a.num_samples, f2dp.details.a.num_samples, "Should equal");
		// Can't assert so
		//TEST_ASSERT_EQUALS(dp2.details.a.ch_layout_ref, f2dp.details.a.ch_layout_ref, "Should equal");

		TEST_ASSERT_TRUE(f2.number_planes() > 0, "Should have data.");
		TEST_ASSERT_TRUE(f2.line_size() != 0, "Should have data");
		TEST_ASSERT_TRUE(f2.data() != nullptr, "Should have data.");

		TEST_ASSERT_THROWS(f2.data(-1), std::out_of_range);
		TEST_ASSERT_THROWS(f2.data(f1.number_planes()), std::out_of_range);
		TEST_ASSERT_THROWS(f2.line_size(f1.number_planes() + 1), std::out_of_range);

	}

	// shared ref
	{
		AVFrame* avf = av_frame_alloc();
		avf->format = AVPixelFormat::AV_PIX_FMT_ARGB;
		avf->width = 1980; avf->height = 1080;
		av_frame_get_buffer(avf, 0);
		for (int i = 0; i < 1980; ++i)
		{
			*(avf->data[0] + i) = i % 256;
		}
		ff::frame f3(avf, true);
		auto f3dp = f3.get_data_properties();
		auto f3np = f3.number_planes();
		ff::frame r3(f3.shared_ref());
		auto f3dp_now = f3.get_data_properties();
		auto f3np_now = f3.number_planes();
		auto r3dp = r3.get_data_properties();
		auto r3np = r3.number_planes();

		TEST_ASSERT_TRUE(r3.ready(), "Should get the status");
		TEST_ASSERT_EQUALS(f3np, r3np, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.v_or_a, r3dp.v_or_a, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.fmt, r3dp.fmt, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.details.v.height, r3dp.details.v.height, "Should equal");
		TEST_ASSERT_EQUALS(f3dp.details.v.width, r3dp.details.v.width, "Should equal");
		auto r3d = r3.data();
		for (int i = 0; i < 1920; ++i)
		{
			TEST_ASSERT_EQUALS(i % 256, *(static_cast<uint8_t*>(r3d) + i), "Should copy the data");
			// Let's change it and see if f3's data changes, too
			*(static_cast<uint8_t*>(r3d) + i) = (i + 2) % 256;
		}

		TEST_ASSERT_TRUE(f3.ready(), "Should not change the src's state");
		TEST_ASSERT_EQUALS(f3np, f3np_now, "Should not change the src's state");
		TEST_ASSERT_EQUALS(f3dp.v_or_a, f3dp_now.v_or_a, "Should not change the src's state");
		TEST_ASSERT_EQUALS(f3dp.fmt, f3dp_now.fmt, "Should not change the src's state");
		TEST_ASSERT_EQUALS(f3dp.details.v.height, f3dp_now.details.v.height, "Should not change the src's state");
		TEST_ASSERT_EQUALS(f3dp.details.v.width, f3dp_now.details.v.width, "Should not change the src's state");

		auto f3d = f3.data();
		for (int i = 0; i < 1920; ++i)
		{
			TEST_ASSERT_EQUALS((i+2) % 256, *(static_cast<uint8_t*>(f3d) + i), "Should ref the same data");
		}
	}

	// Other methods
	{
		// clear_data
		ff::frame f1;
		ff::frame::data_properties dp1
		(
			AVSampleFormat::AV_SAMPLE_FMT_DBL, 1200,
			AVChannelLayout AV_CHANNEL_LAYOUT_SURROUND
		);
		f1.allocate_data(dp1);
		f1.clear_data();
		
		TEST_ASSERT_TRUE(f1.created(), "Should go back to CREATED state.");
		TEST_ASSERT_EQUALS(nullptr, f1.av_frame()->data[0], "Should really clear the data.");
	}

	return 0;
}
