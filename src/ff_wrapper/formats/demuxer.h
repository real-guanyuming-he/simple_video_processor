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

#include "../util/dict.h"
#include "media_base.h"

struct AVFormatContext;

namespace ff
{
	class demuxer : public media_base
	{
	public:
		/*
		* Inits all pointers to nullptr.
		*/
		demuxer() noexcept :
			media_base() {}

		/*
		* Opens a local multimedia file pointed to by path.
		*
		* @param the absolute path to the multimedia file.
		* @param options specifies how the file is opened. Is empty by default.
		* @throws std::invalid_argument if path is nullptr.
		* @throws std::filesystem::filesystem_error if file not found.
		*/
		demuxer(const char* path, const dict& options = dict());

		/*
		* Opens a local multimedia file pointed to by path.
		* 
		* @param the absolute path to the multimedia file.
		* @param options specifies how the file is opened. Cannot be empty.
		* After the constructor returns, the options argument will be replaced with a dict containing options that were not found.
		* @throws std::invalid_argument if path is nullptr or options is empty.
		* @throws std::filesystem::filesystem_error if file not found.
		*/
		demuxer(const char* path, dict& options);

		/*
		* Releases all resources and sets all pointers to nullptr.
		*/
		~demuxer() noexcept;

	public:
		const ::AVFormatContext* get_av_fmt_ctx() const noexcept { return p_fmt_ctx; }
	};
}