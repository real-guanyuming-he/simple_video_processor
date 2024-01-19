/*
* Copyright (C) Guanyuming He 2024
* This file is licensed under the GNU General Public License v3.
*
* This file is part of PROJECT_NAME_REPLACE_LATER.
* PROJECT_NAME_REPLACE_LATER is free software:
* you can redistribute it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
*
* PROJECT_NAME_REPLACE_LATER is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
* You should have received a copy of the GNU General Public License along with PROJECT_NAME_REPLACE_LATER.
* If not, see <https://www.gnu.org/licenses/>.
*/

#include "../test_util.h"

#include "../../ff_wrapper/util/ff_object.h"

#include <bitset>

class test_ff_object : public ff_object
{
public:
	// just forward the parameters to the base's constructor
	test_ff_object(bool val = false) : ff_object(val) {}

	// Only test if they are called correctly
	virtual void internal_allocate_object_memory() override
	{
		which_methods_called[0] = true;
	}
	virtual void internal_allocate_resources_memory(uint64_t size, void* additional_information) override
	{
		which_methods_called[1] = true;
	}
	virtual void internal_release_object_memory() override
	{
		which_methods_called[2] = true;
	}
	virtual void internal_release_resources_memory() override
	{
		which_methods_called[3] = true;
	}

	// bits correspond to the methods in the order they are defined above
	std::bitset<4> which_methods_called;
};

int main()
{
	// Test if the states are handled correctly.
	{
		// Test creation state
		{
			test_ff_object obj{};
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be instantiated as DESTROYED by default"
			)
		}

		// Test state transitions
		{
			// Test the four methods
			test_ff_object obj{};
			obj.allocate_object_memory();
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::OBJECT_CREATED, obj.get_object_state(),
				"ff_objects should be OBJECT_CREATED after allocating object memory"
			)

			obj.allocate_resources_memory();
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::READY, obj.get_object_state(),
				"ff_objects should be READY after allocating resources memory"
			)

			obj.release_resources_memory();
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::OBJECT_CREATED, obj.get_object_state(),
				"ff_objects should be OBJECT_CREATED after releasing resources memory"
			)

			obj.release_object_memory();
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be DESTROYED after releasing object memory"
			)

			// Test destroy()
			obj.internal_allocate_object_memory();
			obj.destroy();
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be DESTROYED after calling destroy()"
			)

			obj.allocate_object_memory();
			obj.allocate_resources_memory();
			obj.destroy();
			TEST_ASSERT_EQUALS
			(
				ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be DESTROYED after calling destroy()"
			)
		}
	}

	// Test if the internal_ methods are called correctly.
	{
		// default construction
		test_ff_object obj;
		TEST_ASSERT_TRUE
		(
			obj.which_methods_called.none(),
			"None of the four methods should be called after default construction"
		)

		// create the object
		test_ff_object obj1{true};
		TEST_ASSERT_EQUALS
		(
			0b0001, obj.which_methods_called,
			"Only object memory should be allocated"
		)

		// allocate resource memory
		obj1.allocate_resources_memory();
		TEST_ASSERT_EQUALS
		(
			0b0011, obj.which_methods_called,
			"Both allocation methods have been called"
		)

		// release resource memory
		obj1.release_resources_memory();
		TEST_ASSERT_EQUALS
		(
			0b1011, obj.which_methods_called,
			"Object release method has been called"
		)

		// release resource memory
		obj1.release_object_memory();
		TEST_ASSERT_EQUALS
		(
			0b1111, obj.which_methods_called,
			"Object release method has been called"
		)
	}

	return 0;
}