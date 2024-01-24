#pragma once
/*
* Copyright (C) 2024 Guanyuming He
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

/*
* ff_helpers.h:
* Defines PRIVATE helpers for ffmpeg C API programming.
* 
* Note: Math helpers are defined in another header because constexpr and inline are used heavily there
*/

struct AVDictionary;
struct AVFormatContext;
struct AVIOContext;
struct AVFrame;
struct AVPacket;
struct AVCodecContext;
struct SwsContext;
struct SwrContext;
struct AVAudioFifo;
struct AVCodecParameters;

#include <string>

namespace ffhelpers
{
	// Any of these functions frees the ptr if it's not nullptr and then sets it to nullptr
#pragma region safely_free
	void safely_free_dict(::AVDictionary** ppd) noexcept;

	// If ppfc is opened by avformat_open_input, then call this.
	void safely_close_input_format_context(::AVFormatContext** ppfc) noexcept;

	// Unlike safely_close_input_format_context, it calls avformat_free_context()
	// for ppfc allocated by avformat_alloc_context().
	void safely_free_format_context(::AVFormatContext** ppfc) noexcept;

	void safely_free_avio_context(::AVIOContext** ppioct) noexcept;

	void safely_free_frame(::AVFrame** ppf) noexcept;

	void safely_free_packet(::AVPacket** pppkt) noexcept;

	void safely_free_codec_context(::AVCodecContext** ppcodctx) noexcept;

	void safely_free_sws_context(::SwsContext** sws_ctx) noexcept;

	void safely_free_swr_context(::SwrContext** swr_ctx) noexcept;

	void safely_free_audio_fifo(::AVAudioFifo** fifo) noexcept;
#pragma endregion

	// Translates the ffmpeg c api error code into string.
	std::string ff_translate_error_code(int err_code);

#define ON_FF_ERROR(msg) throw std::runtime_error(msg)
#define ON_FF_ERROR_WITH_CODE(msg, code) ON_FF_ERROR(std::string(msg) + ": " + ffhelpers::ff_translate_error_code(code))

	// Copied from https://ffmpeg.org/doxygen/5.1/codec__par_8c_source.html#l00031
	void codec_parameters_reset(::AVCodecParameters* par) noexcept;
}

