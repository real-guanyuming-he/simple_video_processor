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
* Now packet is fairly complex for its time related methods.
* Test a packet here.
*/

#include "../test_util.h"

#include "../../ff_wrapper/data/packet.h"

extern "C"
{
#include <libavcodec/packet.h> // For avpacket functions
}

int main()
{
	// Test creation
	{
		ff::packet p1(false);
		TEST_ASSERT_TRUE(p1.destroyed(), "Should be destroyed.");

		ff::packet p2(true);
		TEST_ASSERT_TRUE(p2.created(), "Should be created.");

		AVPacket* pkt = av_packet_alloc();
		av_new_packet(pkt, 0);
		ff::packet p3(pkt);

		TEST_ASSERT_TRUE(p3.ready(), "Should be ready.");
		TEST_ASSERT_EQUALS(pkt, p3.av_packet(), "Should take over the packet.");
	}
	
	// Test move
	{
		// moving a destroyed pkt
		ff::packet p1(false);
		ff::packet m1(std::move(p1));
		TEST_ASSERT_TRUE(p1.destroyed(), "Should be destroyed.");
		TEST_ASSERT_TRUE(m1.destroyed(), "Should get the status");

		// Test moving an allocated pkt
		ff::packet f2(true);
		auto* f2avp = f2.av_packet();
		ff::packet m2(std::move(f2));
		TEST_ASSERT_TRUE(f2.destroyed(), "Moved should be destroyed");
		TEST_ASSERT_TRUE(m2.created(), "Should get the status");
		TEST_ASSERT_EQUALS(f2avp, m2.av_packet(), "Should take over the avpacket");

		// moving a ready pkt
		AVPacket* pkt = av_packet_alloc();
		av_new_packet(pkt, 0);
		ff::packet p3(pkt);
		ff::packet m3(std::move(p3));
		TEST_ASSERT_TRUE(p3.destroyed(), "Moved should be destroyed");
		TEST_ASSERT_TRUE(m3.ready(), "Should get the status");
		TEST_ASSERT_EQUALS(pkt, m3.av_packet(), "Should take over the avpacket");

		// move ass operator
		AVPacket* pkt1 = av_packet_alloc();
		av_new_packet(pkt1, 0);
		ff::packet p4(pkt1);
		m3 = std::move(p4);
		TEST_ASSERT_TRUE(p4.destroyed(), "Moved should be destroyed");
		TEST_ASSERT_TRUE(m3.ready(), "Should get the status");
		TEST_ASSERT_EQUALS(pkt1, m3.av_packet(), "Should take over the avpacket");
	}

	// Test copy
	{
		// copying a destroyed pkt
		ff::packet p1(false);
		ff::packet c1(p1);
		TEST_ASSERT_TRUE(p1.destroyed(), "Should be destroyed.");
		TEST_ASSERT_TRUE(c1.destroyed(), "Should get the status");

		// Test copying an allocated pkt
		ff::packet f2(true);
		auto* f2avp = f2.av_packet();
		ff::packet c2(f2);
		TEST_ASSERT_TRUE(f2.created() && f2.av_packet() == f2avp, "should not be changed");
		TEST_ASSERT_TRUE(c2.created(), "Should get the status");
		TEST_ASSERT_TRUE(nullptr != c2.av_packet(), "Should have a new pkt");

		// copying a ready pkt
		AVPacket* pkt = av_packet_alloc();
		av_new_packet(pkt, 0);
		ff::packet p3(pkt);
		ff::packet c3(p3);
		TEST_ASSERT_TRUE(p3.ready() && pkt == p3.av_packet(), "should not be changed");
		TEST_ASSERT_TRUE(c3.ready(), "Should get the status");
		TEST_ASSERT_TRUE(c3.ready() && nullptr != c3.av_packet(), "Should have a new avpkt");

		// move ass operator
		c3 = p3;
		TEST_ASSERT_TRUE(p3.ready() && pkt == p3.av_packet(), "should not be changed");
		TEST_ASSERT_TRUE(c3.ready(), "Should get the status");
		TEST_ASSERT_TRUE(c3.ready() && nullptr != c3.av_packet(), "Should have a new avpkt");
	}

	// Data and allocation
	{
		ff::packet p1(true);
		TEST_ASSERT_THROWS(p1.data(), std::logic_error);
		TEST_ASSERT_THROWS(p1.data_size(), std::logic_error);

		p1.allocate_resources_memory(32);

		TEST_ASSERT_TRUE(p1.ready() && nullptr != p1.av_packet(), "Should be ready");
		TEST_ASSERT_TRUE(nullptr != p1.data(), "should have data.");
		TEST_ASSERT_TRUE(p1.data_size() >= 32, "should have size.");
	}

	// Time
	{
		ff::packet p1(false);
		// packet destroyed
		TEST_ASSERT_THROWS(p1.time_base(), std::logic_error);
		TEST_ASSERT_THROWS(p1.pts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.dts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.duration(), std::logic_error);
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(1, 1)), std::logic_error);

		p1.allocate_object_memory();
		// time base not set
		TEST_ASSERT_THROWS(p1.pts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.dts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.duration(), std::logic_error);
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(1,1)), std::logic_error);

		p1.allocate_resources_memory(16);

		p1->dts = 0;
		p1->pts = 3;
		// time base still not set
		TEST_ASSERT_THROWS(p1.pts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.dts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.duration(), std::logic_error);
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(1, 1)), std::logic_error);

		// invalid time base
		p1->time_base = ff::zero_rational.av_rational();
		TEST_ASSERT_THROWS(p1.pts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.dts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.duration(), std::logic_error);
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(1, 1)), std::logic_error);

		p1->time_base = ff::rational(-1, 1).av_rational();
		TEST_ASSERT_THROWS(p1.pts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.dts(), std::logic_error);
		TEST_ASSERT_THROWS(p1.duration(), std::logic_error);
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(1, 1)), std::logic_error);

		// valid time base
		p1->time_base = ff::common_video_time_base_600.av_rational();
		TEST_ASSERT_EQUALS(ff::common_video_time_base_600, p1.time_base(), "should be set");
		TEST_ASSERT_EQUALS(ff::time(3, ff::common_video_time_base_600), p1.pts(), "should have correct pts");
		TEST_ASSERT_EQUALS(0, p1.dts(), "should have correct dts");
		// Duration not set
		TEST_ASSERT_EQUALS(0, p1.duration(), "duration not set");

		p1->duration = 17;
		TEST_ASSERT_EQUALS(ff::time(17, ff::common_video_time_base_600), p1.duration(), "should have correct duration");

		// change time base with invalid time base
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(0, 1)), std::invalid_argument);
		TEST_ASSERT_THROWS(p1.change_time_base(ff::rational(-1, 1)), std::invalid_argument);

		// should preserve original values if the new time base is a multiple of the other.
		p1.change_time_base(ff::rational(1, 1200));
		TEST_ASSERT_EQUALS(ff::rational(1, 1200), p1.time_base(), "should really change the tb.");
		TEST_ASSERT_EQUALS(ff::time(3, ff::common_video_time_base_600), p1.pts(), "should preserve pts");
		TEST_ASSERT_EQUALS(0, p1.dts(), "should preserve dts");
		TEST_ASSERT_EQUALS(ff::time(17, ff::common_video_time_base_600), p1.duration(), "should preserve duration");

		// should do what av_packet_rescale_ts the new time base is not a multiple of the other.
		// That is, rounded to the nearest.
		ff::packet cpy(p1);
		auto cavp = cpy.av_packet();

		av_packet_rescale_ts(cavp, p1.time_base().av_rational(), ff::rational(1, 1440).av_rational());
		p1.change_time_base(ff::rational(1, 1440));
		TEST_ASSERT_EQUALS(ff::rational(1, 1440), p1.time_base(), "should really change the tb.");
		TEST_ASSERT_EQUALS(ff::time(cavp->dts, ff::rational(1, 1440)), p1.dts(), "should do what av_packet_rescale_ts does");
		TEST_ASSERT_EQUALS(ff::time(cavp->pts, ff::rational(1, 1440)), p1.pts(), "should do what av_packet_rescale_ts does");
		TEST_ASSERT_EQUALS(ff::time(cavp->duration, ff::rational(1, 1440)), p1.duration(), "should do what av_packet_rescale_ts does");

		av_packet_rescale_ts(cavp, p1.time_base().av_rational(), ff::rational(1, 144).av_rational());
		p1.change_time_base(ff::rational(1, 144));
		TEST_ASSERT_EQUALS(ff::rational(1, 144), p1.time_base(), "should really change the tb.");
		TEST_ASSERT_EQUALS(ff::time(cavp->dts, ff::rational(1, 144)), p1.dts(), "should do what av_packet_rescale_ts does");
		TEST_ASSERT_EQUALS(ff::time(cavp->pts, ff::rational(1, 144)), p1.pts(), "should do what av_packet_rescale_ts does");
		TEST_ASSERT_EQUALS(ff::time(cavp->duration, ff::rational(1, 144)), p1.duration(), "should do what av_packet_rescale_ts does");
	}

	return 0;
}