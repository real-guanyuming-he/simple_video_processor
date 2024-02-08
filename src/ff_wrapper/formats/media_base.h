#pragma once
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

/*
* media_base.h
* Defines a base class for multimedia files.
*/

#include "../util/util.h"
#include "../util/ff_time.h"

#include <string>
#include <vector>

struct AVFormatContext;

namespace ff
{
	/*
	* Defines a base class for multimedia files.
	* The class does not own the AVFormatContext in any way.
	* It merely assumes that it's valid (i.e. fully set up by a demuxer or muxer)
	and provides methods of common operations regarding it (e.g. get video info).
	*/
	class FF_WRAPPER_API media_base
	{
	public:
		media_base() = default;
		virtual ~media_base() = default;

		// Sets the pointer
		media_base(AVFormatContext* fmt_ctx) noexcept
			: p_fmt_ctx(fmt_ctx) {}

	public:
		inline ::AVFormatContext* av_fmt_ctx() noexcept { return p_fmt_ctx; }
		inline const ::AVFormatContext* av_fmt_ctx() const noexcept { return p_fmt_ctx; }

	public:
		/*
		* These don't test if p_fmt_ctx is nullptr for faster accesses.
		* After all, nullptr access should be captured by the debugger or the OS.
		*/

		/*
		* @returns a description of the media file's format.
		* @throws std::logic_error if the derived class is not ready for this.
		*/
		virtual std::string description() const = 0;

		/*
		* @returns a list of short names that the format of the media file may be called
		* @throws std::logic_error if the derived class is not ready for this.
		*/
		virtual std::vector<std::string> short_names() const = 0;

		/*
		* @returns a list of file extensions that the format of the media file may use
		* @throws std::logic_error if the derived class is not ready for this.
		*/
		virtual std::vector<std::string> extensions() const = 0;

		/*
		* Converts a list represented by such a string as obtained inside the above two methods
		* into a vector. The items are assumed to be separated by commas.
		*/
		static std::vector<std::string> string_to_list(const std::string& str, char separator = ',');

	public:
		/*
		* These don't test if p_fmt_ctx is nullptr for faster accesses.
		* After all, nullptr access should be captured by the debugger or the OS.
		*/

		/*
		* @returns the path to the file. Set by libavformat when opening the file;
		* set by the muxer before muxing.
		*/
		const char* get_file_path() const noexcept;

		/*
		* @returns the number of streams contained in the media file.
		*/
		int num_streams() const noexcept;

		/*
		* @returns the time of the first frame, in AV_TIME_BASE.
		*/
		ff::time start_time() const noexcept;

		/*
		* @returns the bit rate of the file, in bits/sec, or 0 if unavailable.
		*/
		int64_t bit_rate() const noexcept;

		/*
		* @returns the duration of the multimedia file, in AV_TIME_BASE, or 0 is unknown.
		*/
		ff::time duration() const noexcept;

	protected:
		::AVFormatContext* p_fmt_ctx = nullptr;
	};

}