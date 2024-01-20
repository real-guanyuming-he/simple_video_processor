#pragma once
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

#include "../util/ff_object.h"

struct AVFormatContext;

namespace ff
{
	/*
	* Base class for all those who deal with multimedia files (i/o, muxing, demuxing)
	* 
	* Provides encapsulation for the common methods of AVFormatContext
	*/
	class FF_WRAPPER_API media_file_base : public ff_object
	{
	public:
		/*
		* Does nothing but setting the fields to nullptr.
		*/
		media_file_base() noexcept : 
			ff_object(), 
			p_fmt_ctx(nullptr), file_path(nullptr) {}

		/*
		* Opens the file indicated by path
		* and initialize the fmt ctx with it.
		*/
		media_file_base(const wchar_t* path);

		~media_file_base() = default;

	protected:
		::AVFormatContext* p_fmt_ctx;

		virtual void internal_allocate_object_memory() override;
		virtual void internal_allocate_resources_memory(uint64_t size, void* additional_information) override;
		virtual void internal_release_object_memory() override;
		virtual void internal_release_resources_memory() override;

	private:
		// Absolute path to the multimedia file.
		const wchar_t* file_path;
	public:
		/*
		* @returns the absolute path to the multimedia file,
		* or nullptr if not set.
		*/
		const wchar_t* get_file_path() const { return file_path; }
	};
}