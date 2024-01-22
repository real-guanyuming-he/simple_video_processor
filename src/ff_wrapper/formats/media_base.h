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

/*
* media_base.h
* Defines a base class for multimedia files.
*/

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
	class media_base
	{
	public:
		media_base() = default;
		virtual ~media_base() = default;

		// Sets the pointer
		media_base(AVFormatContext* fmt_ctx) noexcept
			: p_fmt_ctx(fmt_ctx) {}

	public:
		/*
		* @returns a description of the media file's format.
		*/
		virtual const char* description() const noexcept = 0;

		/*
		* @returns a list of short names that the format of the media file may be called
		* in a string, separated by ;
		*/
		virtual const char* short_names() const noexcept = 0;

		/*
		* @returns a list of file extensions that the format of the media file may use
		* in a string, separated by ;
		*/
		virtual const char* extensions() const noexcept = 0;

		/*
		* Converts a list represented by such a string as returned by the above two methods
		* into a vector.
		*/
		static std::vector<const char*> string_to_list(const char* str);

	public:
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
		int64_t start_time() const noexcept;

		/*
		* @returns the bit rate of the file, in bits/sec, or 0 if unavailable.
		*/
		int64_t bit_rate() const noexcept;

		/*
		* @returns the duration of the multimedia file, in AV_TIME_BASE, or 0 is unknown.
		*/
		int64_t duration() const noexcept;

	protected:
		::AVFormatContext* p_fmt_ctx = nullptr;
	};

}