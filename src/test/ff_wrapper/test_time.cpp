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

#include "../../ff_wrapper/util/ff_time.h"

int main()
{
	// Methods that are not tested
	// are trivial return or method calls.
	// They are tested by inspection.

	// Test creation
	{
		int64_t stamp = 827598256; ff::rational base(1, 123);
		
		ff::time t1(stamp, base);
		TEST_ASSERT_EQUALS(stamp, t1.timestamp_accurate().get_num(), "Should be equal");
		TEST_ASSERT_EQUALS(base, t1.time_base(), "Should be equal");
		
		ff::time t2(ff::rational_64(stamp, 1), base);
		TEST_ASSERT_EQUALS(stamp, t2.timestamp_accurate().get_num(), "Should be equal");
		TEST_ASSERT_EQUALS(base, t2.time_base(), "Should be equal");

		ff::time t3(ff::rational_64(0, 1), base);
		TEST_ASSERT_EQUALS(0, t3.timestamp_accurate().get_num(), "Should be equal");
		TEST_ASSERT_EQUALS(base, t3.time_base(), "Should be equal");

		// bad because time base = 0
		TEST_ASSERT_THROWS(ff::time(123, ff::zero_rational), std::invalid_argument);
	}

	// Test change time base
	{
		ff::rational b1(9, 199), b2(6, 166), b3(1, 177), b4(7, 177);
		
		// bases are coprime
		ff::time t1(971, b1);
		auto expected = t1.to_absolute();
		t1.change_time_base(ff::common_audio_time_base_64000);
		TEST_ASSERT_EQUALS(expected, t1.to_absolute(), "Expect not to change the absolute value.");

		ff::time t2(ff::rational_64(756, 32), b3);
		expected = t2.to_absolute();
		t2.change_time_base(b2);
		TEST_ASSERT_EQUALS(expected, t2.to_absolute(), "Expect not to change the absolute value.");

		// bases are not coprime
		ff::time t3(ff::rational_64(912, 13), b4);
		expected = t3.to_absolute();
		t3.change_time_base(b3);
		TEST_ASSERT_EQUALS(expected, t3.to_absolute(), "Expect not to change the absolute value.");
	}

	// Test to_string()
	{
		// seconds
		ff::time t1(1200, ff::rational(1, 120));
		TEST_ASSERT_EQUALS("00:00:10.00", t1.to_string(), "Should be equal");

		// minutes
		ff::time t2(26000, ff::rational(1, 26));
		TEST_ASSERT_EQUALS("00:16:40.00", t2.to_string(), "Should be equal");
		
		// hours
		ff::time t3(1440000, ff::rational(1, 12));
		TEST_ASSERT_EQUALS("33:20:00.00", t3.to_string(), "Should be equal");

		// miliseconds
		ff::time t4(1440000, ff::rational(1, 70000));
		TEST_ASSERT_EQUALS("00:00:20.57", t4.to_string(), "Should be equal");

		// negative
		ff::time t5(-1200000, ff::rational(1, 7));
		TEST_ASSERT_EQUALS("-47:37:08.57", t5.to_string(), "Should be equal");
	}

	return 0;
}