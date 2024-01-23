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

#include "demuxer.h"

#include "../util/ff_helpers.h"

#include <filesystem> // For std::filesystem::filesystem_error
using filesystem_error = std::filesystem::filesystem_error;

extern "C"
{
#include "libavformat/avformat.h"
}

ff::demuxer::demuxer(const char* path, bool probe_stream_info, const dict& options)
	: media_base(nullptr)
{
	if (nullptr == path)
	{
		throw std::invalid_argument("Path cannot be nullptr");
	}

	// Open the file
	int ret = 0;
	AVDictionary** ppavd;
	if (options.empty())
	{
		ppavd = nullptr;
	}
	else
	{
		// make a copy lest the dict be changed.
		dict cpy(options); auto* cpy_avd = cpy.get_av_dict();
		ppavd = &cpy_avd;
	}

	internal_open_format(path, probe_stream_info, ppavd);
}

ff::demuxer::demuxer(const char* path, dict& options, bool probe_stream_info)
	: media_base(nullptr)
{
	if (nullptr == path)
	{
		throw std::invalid_argument("Path cannot be nullptr");
	}
	if (options.empty())
	{
		throw std::invalid_argument("Options cannot be empty. If you want empty options, call the const version.");
	}

	// Open the file
	AVDictionary* pavd = options.get_av_dict();
	internal_open_format(path, probe_stream_info, &pavd);
	options = pavd;
}

ff::demuxer::~demuxer()
{
	ffhelpers::safely_close_input_format_context(&p_fmt_ctx);
}

void ff::demuxer::probe_stream_information(const dict& options)
{
	AVDictionary** ppavd;
	if (options.empty())
	{
		ppavd = nullptr;
	}
	else
	{
		// make a copy lest the dict be changed.
		dict cpy(options); auto* cpy_avd = cpy.get_av_dict();
		ppavd = &cpy_avd;
	}
	internal_probe_stream_info(ppavd);
}

void ff::demuxer::probe_stream_information(dict& options)
{
	if (options.empty())
	{
		throw std::invalid_argument("Options cannot be empty. If you want empty options, call the const version.");
	}

	auto* pavd = options.get_av_dict();
	internal_probe_stream_info(&pavd);
	options = pavd;
}

void ff::demuxer::internal_open_format(const char* path, bool probe_stream_info, ::AVDictionary** dict)
{
	int ret = avformat_open_input(&p_fmt_ctx, path, nullptr, dict);

	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOENT): // No such file.
			throw filesystem_error
			(
				"The file specified by path does not exist.", path,
				std::make_error_code(std::errc::no_such_file_or_directory)
			);
			break;
		case AVERROR(ENOMEM): // No memory
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL): // Invalid argument
			throw std::runtime_error("Unexpected error happened during a call to avformat_open_input(): Invalid argument.");
			break;
		default: // ?
			throw std::runtime_error("Unexpected error happened during a call to avformat_open_input(): Unknown error.");
		}
	}

	if (probe_stream_info)
	{
		probe_stream_information();
	}
}

void ff::demuxer::internal_probe_stream_info(::AVDictionary** dict)
{
	int ret = avformat_find_stream_info(p_fmt_ctx, dict);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM): // No memory
			throw std::bad_alloc();
			break;
		default:
			throw std::runtime_error("Unexpected error happened during a call to avformat_find_stream_info().");
		}
	}

	streams.reserve(p_fmt_ctx->nb_streams);
	for (int i = 0; i < p_fmt_ctx->nb_streams; ++i)
	{
		AVStream* p_stream = p_fmt_ctx->streams[i];
		streams.emplace_back(p_stream);

		const stream& st = streams[i];
		if (st.is_video())
		{
			v_indices.push_back(i);
		}
		if (st.is_audio())
		{
			a_indices.push_back(i);
		}
		if (st.is_subtitle())
		{
			s_indices.push_back(i);
		}
	}
}

const char* ff::demuxer::description() const noexcept
{
	return p_fmt_ctx->iformat->long_name;
}

const char* ff::demuxer::short_names() const noexcept
{
	return p_fmt_ctx->iformat->name;
}

const char* ff::demuxer::extensions() const noexcept
{
	return p_fmt_ctx->iformat->extensions;
}
