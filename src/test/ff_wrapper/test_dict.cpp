/*
* Copyright (C) Guanyuming He 2024
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

#include "../../ff_wrapper/util/dict.h"

#include <iostream>

int main()
{

	// Test creation
	{
		ff::dict d;
		TEST_ASSERT_TRUE(d.empty(), "Should be empty")

		TEST_ASSERT_FALSE(d.query_entry("anything"), "should not contain any entry")
		TEST_ASSERT_TRUE(d.get_value("everything").empty(), "should return an empty string")
	}

	// Test adding entries
	{
		ff::dict d;

		d.insert_entry("abc", "def");
		TEST_ASSERT_EQUALS(1, d.num(), "should have 1 entry")
		TEST_ASSERT_TRUE(d.query_entry("abc"), "should have the entry")
		TEST_ASSERT_EQUALS("def", d.get_value("abc"), "should have the same value")

		d.insert_entry("123", "456");
		TEST_ASSERT_EQUALS(2, d.num(), "should have 2 entries")
		TEST_ASSERT_TRUE(d.query_entry("123"), "should have the entry")
		TEST_ASSERT_EQUALS("456", d.get_value("123"), "should have the same value")

			// Override entry
		d.insert_entry("123", "999");
		TEST_ASSERT_EQUALS(2, d.num(), "should have 2 entries")
		TEST_ASSERT_TRUE(d.query_entry("123"), "should have the entry")
		TEST_ASSERT_EQUALS("999", d.get_value("123"), "should have the new value")

		TEST_ASSERT_FALSE(d.query_entry("anything"), "should not contain not inserted entry")
		TEST_ASSERT_TRUE(d.get_value("everything").empty(), "should return an empty string")
	}

	// Test deleting entries
	{
		ff::dict d;
		d.insert_entry("...", "...");
		d.insert_entry("take", "control");

		d.delete_entry("...");

		TEST_ASSERT_EQUALS(1, d.num(), "should have 1 entry")
		TEST_ASSERT_TRUE(d.query_entry("take"), "should have undeleted entry")
		TEST_ASSERT_FALSE(d.query_entry("..."), "should not have deleted entry")
	}

	// Test copy/move ctor
	{
		ff::dict d;
		ff::dict d1(d);
		TEST_ASSERT_EQUALS(nullptr, d1.get_av_dict(), "should have nullptr")

		d.insert_entry("qwe", "asd");
		d.insert_entry("asd", "asd");
		ff::dict d2(d);
		TEST_ASSERT_EQUALS(2, d2.num(), "should have 2 entries")
		TEST_ASSERT_EQUALS("asd", d2.get_value("qwe"), "should have the entry")
		TEST_ASSERT_EQUALS("asd", d2.get_value("asd"), "should have the entry")

		ff::dict d3(std::move(d2));
		TEST_ASSERT_EQUALS(nullptr, d2.get_av_dict(), "should have nullptr")
		TEST_ASSERT_EQUALS(2, d3.num(), "should have 2 entries")
		TEST_ASSERT_EQUALS("asd", d3.get_value("qwe"), "should have the entry")
		TEST_ASSERT_EQUALS("asd", d3.get_value("asd"), "should have the entry")
	}

	return 0;
}