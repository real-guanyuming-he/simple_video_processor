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

#include <string>

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

	// Actual decoding test.
	

	// Copying a decoder is forbidden/deleted

	// Test moving
	{

	}

	return 0;
}