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

#include "../../ff_wrapper/sws/frame_transformer.h"

int main()
{
	// Test consistency between querying supported formats
	// and if the formats are really supported (if not supported 
	// any ctor will throw an exception).
	{
		// This should be supported both as input and as output.
		AVPixelFormat a_supported_fmt = AV_PIX_FMT_RGB24;

		ff::frame::data_properties ip(a_supported_fmt, 800, 600);
		ff::frame::data_properties op(a_supported_fmt, 800, 600);

		// The comment for AV_PIX_FMT_NB says do not use if I'm linking to shared libav
		// But for this test a smaller number is okay.
		// Could be a problem if the number is larger than that in the lib I linked to.
		// Pay attention to the version.
		for (int i = 0; i < (int)AV_PIX_FMT_NB; ++i)
		{
			// input fmt
			ip.fmt = i;
			op.fmt = a_supported_fmt;
			if (ff::frame_transformer::query_input_pixel_format_support((AVPixelFormat)i))
			{
				// This should not throw
				ff::frame_transformer ft(op, ip);
			}
			else
			{
				// This should throw
				TEST_ASSERT_THROWS(ff::frame_transformer(op, ip), std::domain_error);
			}

			// output fmt
			ip.fmt = a_supported_fmt;
			op.fmt = i;
			if (ff::frame_transformer::query_output_pixel_format_support((AVPixelFormat)i))
			{
				// This should not throw
				ff::frame_transformer ft(op, ip);
			}
			else
			{
				// This should throw
				TEST_ASSERT_THROWS(ff::frame_transformer(op, ip), std::domain_error);
			}
		}
	}

	// Test the stored properties.
	{
		// This should be supported both as input and as output.
		AVPixelFormat a_supported_fmt = AV_PIX_FMT_RGB24;
		// This should be supported both as input and as output.
		AVPixelFormat another_supported_fmt = AV_PIX_FMT_YUV420P;

		ff::frame::data_properties ip(a_supported_fmt, 800, 600);
		ff::frame::data_properties op(another_supported_fmt, 400, 300);

		ff::frame_transformer t1(op, ip);
		
		TEST_ASSERT_EQUALS(op, t1.dst_properties(), "Should be equal");
		TEST_ASSERT_EQUALS(ip, t1.src_properties(), "Should be equal");
	}

	// Test converting frames
	{
		// This should be supported both as input and as output.
		AVPixelFormat a_supported_fmt = AV_PIX_FMT_RGB24;
		// This should be supported both as input and as output.
		AVPixelFormat another_supported_fmt = AV_PIX_FMT_YUV420P;

		ff::frame::data_properties ip(a_supported_fmt, 800, 600);
		ff::frame::data_properties op(another_supported_fmt, 400, 300);

		ff::frame_transformer t1(op, ip);

		// different resolution
		ff::frame::data_properties iip(a_supported_fmt, 1024, 800);
		// different format
		ff::frame::data_properties iop(a_supported_fmt, 400, 300);

		ff::frame invalid_in(true);
		invalid_in.allocate_data(iip);

		ff::frame invalid_out(true);
		invalid_out.allocate_data(iop);

		ff::frame actual_in(true);
		actual_in.allocate_data(ip);

		TEST_ASSERT_THROWS(t1.convert_frame(invalid_in), std::invalid_argument);
		TEST_ASSERT_THROWS(t1.convert_frame(invalid_out, actual_in), std::invalid_argument);

		// Valid frames
		ff::frame res = t1.convert_frame(actual_in);
		TEST_ASSERT_TRUE(res.ready(), "Should be made ready.");
		TEST_ASSERT_EQUALS(op, res.get_data_properties(), "Should have the properties.");

		ff::frame destroyed_out(false);
		t1.convert_frame(destroyed_out, actual_in);
		TEST_ASSERT_TRUE(destroyed_out.ready(), "Should be made ready.");
		TEST_ASSERT_EQUALS(op, destroyed_out.get_data_properties(), "Should have the properties.");

		ff::frame created_out(true);
		t1.convert_frame(created_out, actual_in);
		TEST_ASSERT_TRUE(created_out.ready(), "Should be made ready.");
		TEST_ASSERT_EQUALS(op, created_out.get_data_properties(), "Should have the properties.");

		ff::frame ready_out(true);
		ready_out.allocate_data(op);
		t1.convert_frame(ready_out, actual_in);
		TEST_ASSERT_TRUE(ready_out.ready(), "Should be ready.");
		TEST_ASSERT_EQUALS(op, ready_out.get_data_properties(), "Should have the properties.");
	}

	return 0;
}