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

#include "../util/ff_time.h"
#include "../codec/codec_properties.h"

extern "C"
{
#include <libavcodec/codec_id.h> // For AVCodecID
}

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
	class FF_WRAPPER_API stream
	{
	public:
		stream() noexcept
			: p_stream(nullptr) {}
		stream(::AVStream* st)
			: p_stream(st) {}

		virtual ~stream() = default;

/////////////////////////////// Stream related ///////////////////////////////
	public:
		const ::AVStream* av_stream() const noexcept { return p_stream; }
		::AVStream* av_stream() noexcept { return p_stream; }

		const ::AVStream* operator->() const noexcept { return p_stream; }
		::AVStream* operator->() noexcept { return p_stream; }

		AVMediaType type() const noexcept;
		bool is_video() const noexcept;
		bool is_audio() const noexcept;
		bool is_subtitle() const noexcept;

		/*
		* @returns The duration of the stream.
		* It is not 100% accurate as it may be calculated from the file size and bitrate.
		* Can be equal to 0, in which case it is unknown.
		*/
		ff::time duration() const noexcept;
		/*
		* @returns The time base of the stream.
		* Can be nonpositive, in which case it is unknown or not fixed.
		*/
		ff::rational time_base() const noexcept;

		/*
		* @returns the properties of how the stream is, or is to be, encodeded.
		*/
		ff::codec_properties properties() const noexcept;

/////////////////////////////// Codec related ///////////////////////////////
	public:
		const AVCodecID codec_id() const noexcept;

	private:
		::AVStream* p_stream;
	};
}