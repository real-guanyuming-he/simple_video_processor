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

struct AVStream;

namespace ff
{
	/*
	* Wrapper for AVStream.
	* Does not own the stream, because each stream is or will be bound to and managed by
	* an AVFormatContext (a muxer or demuxer in my wrapper).
	* 
	* However, derived classes may have to ability to create a new stream. 
	* Those things have to be done for muxing.
	*/
	class stream
	{
	public:
		stream() noexcept
			: p_stream(nullptr) {}
		stream(::AVStream* st)
			: p_stream(st) {}

		~stream() = default;

	public:
		bool is_video() const noexcept;
		bool is_audio() const noexcept;
		bool is_subtitle() const noexcept;

	private:
		::AVStream* p_stream;
	};
}