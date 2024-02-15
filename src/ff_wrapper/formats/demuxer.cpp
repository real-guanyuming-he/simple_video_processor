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

#include <filesystem>
using filesystem_error = std::filesystem::filesystem_error;

extern "C"
{
#include "libavformat/avformat.h"
}

ff::demuxer::demuxer(const std::filesystem::path& path, bool probe_stream_info, const dict& options)
	: media_base(nullptr)
{
	const std::string path_str(path.generic_string());

	if (path.empty())
	{
		throw std::invalid_argument("Path cannot be empty");
	}

	// Open the file
	int ret = 0;
	if (options.empty())
	{
		internal_open_format(path_str, probe_stream_info, nullptr);
	}
	else
	{
		// make a copy lest the dict be changed.
		dict cpy(options); 
		auto* cpy_avd = cpy.get_av_dict();
		internal_open_format(path_str, probe_stream_info, &cpy_avd);
	}

	// Post creation. Set other fields.
	p_demuxer_desc = p_fmt_ctx->iformat;
}

ff::demuxer::demuxer(const std::filesystem::path& path, dict& options, bool probe_stream_info)
	: media_base(nullptr)
{
	const std::string path_str(path.generic_string());

	if (path.empty())
	{
		throw std::invalid_argument("Path cannot be empty");
	}
	if (options.empty())
	{
		throw std::invalid_argument("Options cannot be empty. If you want empty options, call the const dict version.");
	}

	// Open the file
	AVDictionary* pavd = options.get_av_dict();
	internal_open_format(path_str, probe_stream_info, &pavd);
	options = pavd;

	// Post creation. Set other fields.
	p_demuxer_desc = p_fmt_ctx->iformat;
}

ff::demuxer::~demuxer()
{
	ffhelpers::safely_close_input_format_context(&p_fmt_ctx);
}

void ff::demuxer::probe_stream_information(const dict& options)
{
	if (options.empty())
	{
		internal_probe_stream_info(nullptr);
	}
	else
	{
		// make a copy lest the dict be changed.
		dict cpy(options); 
		auto* cpy_avd = cpy.get_av_dict();
		internal_probe_stream_info(&cpy_avd);
	}
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

ff::packet ff::demuxer::demux_next_packet()
{
	FF_ASSERT(p_fmt_ctx != nullptr, "Must be ready after construction.");

	// Using an AVPacket is more convenient here.
	::AVPacket* av_pkt = av_packet_alloc();
	if (nullptr == av_pkt)
	{
		throw std::bad_alloc();
	}

	bool ret = internal_demux_packet(av_pkt);

	if (ret)
	{
		// Success
		return packet(av_pkt);
	}
	else
	{
		// Failure
		av_packet_free(&av_pkt);
		return packet(false);
	}

}

bool ff::demuxer::demux_next_packet(packet& pkt)
{
	FF_ASSERT(p_fmt_ctx != nullptr, "Must be ready after construction.");

	// Make sure pkt is created before storing demuxed data in it.
	switch (pkt.get_object_state())
	{
	case ff_object::DESTROYED:
		pkt.allocate_object_memory();
		[[fallthrough]];
	case ff_object::OBJECT_CREATED:
		// Do nothing.
		break;
	case ff_object::READY:
		// Release its data
		pkt.release_resources_memory();
		break;
	}

	bool ret = internal_demux_packet(pkt.av_packet());
	if (ret)
	{
		// Success
		pkt.state = ff_object::READY;
	}

	return ret;
}

void ff::demuxer::seek(int stream_ind, int64_t timestamp, bool direction)
{
	FF_ASSERT(p_fmt_ctx != nullptr, "Must be ready after construction.");

	if (stream_ind < 0 || stream_ind >= p_fmt_ctx->nb_streams)
	{
		throw std::out_of_range("Stream index is out of range.");
	}

	int ret = av_seek_frame
	(
		p_fmt_ctx, stream_ind, timestamp,
		direction ? NULL : AVSEEK_FLAG_BACKWARD
	);

	if (ret < 0) // Failure
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR_EOF:
			eof_reached = true;
			throw std::invalid_argument("The demuxer could not seek to the timestamp you provided until the end.");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened during a call to av_seek_frame()", ret);
			break;
		}
	}
	else // Success
	{
		// reset the eof status
		// Even if the last packet is sought,
		// eof still remains to be reached by the next attempt to demux a packet.
		eof_reached = false;
	}
}

void ff::demuxer::internal_open_format(const std::string& path, bool probe_stream_info, ::AVDictionary** dict)
{
	int ret = avformat_open_input(&p_fmt_ctx, path.c_str(), nullptr, dict);

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
			ON_FF_ERROR_WITH_CODE("Unexpected error happened during a call to avformat_open_input()", ret);
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
			ON_FF_ERROR_WITH_CODE("Unexpected error happened during a call to avformat_find_stream_info()", ret);
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

bool ff::demuxer::internal_demux_packet(AVPacket* pkt)
{
	int ret = av_read_frame(p_fmt_ctx, pkt);
	// ret=0 -> success
	if (0 == ret)
	{
		pkt->time_base = streams[pkt->stream_index]->time_base;
		return true;
	}
	// ret<0 -> Error or EOF

	// On error, pkt will be blank (as if it came from av_packet_alloc())
	if (nullptr == pkt->buf && 0 == pkt->size)
	{
		// Error
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR_EOF: // EOF
			eof_reached = true;
			return false;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened during a call to av_read_frame()", ret);
			break;
		}
	}
	else
	{
		// EOF
		eof_reached = true;
		return false;
	}
}

std::string ff::demuxer::description() const
{
	if (nullptr == p_demuxer_desc)
	{
		throw std::logic_error("Not ready.");
	}

	return std::string(p_demuxer_desc->long_name);
}

std::vector<std::string> ff::demuxer::short_names() const
{
	if (nullptr == p_demuxer_desc)
	{
		throw std::logic_error("Not ready.");
	}

	return media_base::string_to_list(std::string(p_demuxer_desc->name));
}

std::vector<std::string> ff::demuxer::extensions() const
{
	if (nullptr == p_demuxer_desc)
	{
		throw std::logic_error("Not ready.");
	}

	return media_base::string_to_list(std::string(p_demuxer_desc->extensions));
}
