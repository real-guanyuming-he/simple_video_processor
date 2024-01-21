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

#include "../../ff_wrapper/util/ff_object.h"

#include <bitset>

namespace ff
{
	class test_ff_object : public ff_object
	{
	public:
		// just forward the parameters to the base's constructor
		ff::test_ff_object() : ff_object() {}

		// Only test if they are called correctly
		virtual void internal_allocate_object_memory() override
		{
			which_methods_called[0] = true;
		}
		virtual void internal_allocate_resources_memory(uint64_t size, void* additional_information) override
		{
			which_methods_called[1] = true;
		}
		virtual void internal_release_object_memory() noexcept override
		{
			which_methods_called[2] = true;
		}
		virtual void internal_release_resources_memory() noexcept override
		{
			which_methods_called[3] = true;
		}

		// bits correspond to the methods in the order they are defined above
		std::bitset<4> which_methods_called;
	};
}

int main()
{
	// Test if the states are handled correctly.
	{
		// Test creation state
		{
			ff::test_ff_object obj{};
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be instantiated as DESTROYED by default"
			)
		}

		// Test state transitions
		{
			// Test the four methods
			ff::test_ff_object obj{};
			obj.allocate_object_memory();
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::OBJECT_CREATED, obj.get_object_state(),
				"ff_objects should be OBJECT_CREATED after allocating object memory"
			)

			obj.allocate_resources_memory();
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::READY, obj.get_object_state(),
				"ff_objects should be READY after allocating resources memory"
			)

			obj.release_resources_memory();
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::OBJECT_CREATED, obj.get_object_state(),
				"ff_objects should be OBJECT_CREATED after releasing resources memory"
			)

			obj.release_object_memory();
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be DESTROYED after releasing object memory"
			)

			// Test destroy()
			obj.internal_allocate_object_memory();
			obj.destroy();
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be DESTROYED after calling destroy()"
			)

			obj.allocate_object_memory();
			obj.allocate_resources_memory();
			obj.destroy();
			TEST_ASSERT_EQUALS
			(
				ff::ff_object::ff_object_state::DESTROYED, obj.get_object_state(),
				"ff_objects should be DESTROYED after calling destroy()"
			)
		}
	}

	// Test if the internal_ methods are called correctly.
	{
		// default construction
		ff::test_ff_object obj;
		TEST_ASSERT_TRUE
		(
			obj.which_methods_called.none(),
			"None of the four methods should be called after default construction"
		)

		// allocate object memory
		obj.allocate_object_memory();
		TEST_ASSERT_EQUALS
		(
			0b0001, obj.which_methods_called,
			"Object allocation method should have been called"
		)

		// allocate resource memory
		obj.allocate_resources_memory();
		TEST_ASSERT_EQUALS
		(
			0b0011, obj.which_methods_called,
			"Both allocation methods should have been called"
		)

		// release resource memory
		obj.release_resources_memory();
		TEST_ASSERT_EQUALS
		(
			0b1011, obj.which_methods_called,
			"Object release method should have been called"
		)

		// release object memory
		obj.release_object_memory();
		TEST_ASSERT_EQUALS
		(
			0b1111, obj.which_methods_called,
			"Object release method should have been called"
		)
	}

	return 0;
}