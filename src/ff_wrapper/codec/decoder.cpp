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

#include "decoder.h"

void ff::decoder::internal_allocate_object_memory()
{
	throw std::runtime_error("Not implemented.");
}

void ff::decoder::internal_allocate_resources_memory(uint64_t size, void* additional_information)
{
	throw std::runtime_error("Not implemented.");
}

void ff::decoder::internal_release_object_memory() noexcept
{
	throw std::runtime_error("Not implemented.");
}

void ff::decoder::internal_release_resources_memory() noexcept
{
	throw std::runtime_error("Not implemented.");
}
