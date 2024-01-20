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

#include "ff_object.h"

namespace ff
{
	ff_object::ff_object() noexcept
	{
		state = ff_object_state::DESTROYED;
	}

	void ff_object::allocate_object_memory()
	{
		FF_ASSERT
		(
			state == ff_object_state::DESTROYED,
			"Can only allocated object memory if the object is destroyed"
		)

			internal_allocate_object_memory();
		state = ff_object_state::OBJECT_CREATED;
	}

	void ff_object::allocate_resources_memory(uint64_t size, void* additional_information)
	{
		FF_ASSERT
		(
			state == ff_object_state::OBJECT_CREATED,
			"Can only allocated resource memory if the object is created"
		)

			internal_allocate_resources_memory(size, additional_information);
		state = ff_object_state::READY;
	}

	void ff_object::release_resources_memory()
	{
		FF_ASSERT
		(
			state == ff_object_state::READY,
			"Can only release resources memory if the object is ready"
		)

			internal_release_resources_memory();
		state = ff_object_state::OBJECT_CREATED;
	}

	void ff_object::release_object_memory()
	{
		FF_ASSERT
		(
			state == ff_object_state::OBJECT_CREATED,
			"Can only release object memory if the object is created"
		)

			internal_release_object_memory();
		state = ff_object_state::DESTROYED;
	}

	void ff_object::destroy()
	{
		switch (state)
		{
		case ff_object_state::READY:
			internal_release_resources_memory();
			[[fallthrough]];
		case ff_object_state::OBJECT_CREATED:
			internal_release_object_memory();
			state = ff_object_state::DESTROYED;
			//case ff_object_state::READY:
				// do nothing
		}
	}
}