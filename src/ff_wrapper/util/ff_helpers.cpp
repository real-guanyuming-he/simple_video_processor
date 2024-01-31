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

extern "C"
{
#include <libavutil/audio_fifo.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include "ff_helpers.h"

namespace ffhelpers
{
	void safely_free_dict(::AVDictionary** ppd) noexcept
	{
		if (nullptr != *ppd)
		{
			av_dict_free(ppd);
			*ppd = nullptr;
		}
	}

	void safely_close_input_format_context(AVFormatContext** ppfc) noexcept
	{
		if (*ppfc)
		{
			avformat_close_input(ppfc);
		}
	}

	void safely_free_format_context(::AVFormatContext** ppfc) noexcept
	{
		if (*ppfc)
		{
			avformat_free_context(*ppfc);
			*ppfc = nullptr;
		}
	}

	void safely_free_avio_context(AVIOContext** ppioct) noexcept
	{
		if (*ppioct)
		{
			avio_closep(ppioct);
		}
	}

	void safely_free_frame(AVFrame** ppf) noexcept
	{
		if (*ppf)
		{
			// the function already does the setting nullptr for us
			av_frame_free(ppf);
		}
	}

	void safely_free_packet(AVPacket** pppkt) noexcept
	{
		if (*pppkt)
		{
			// the functions already does the setting nullptr for us
			av_packet_free(pppkt);
		}
	}

	void safely_free_codec_context(AVCodecContext** ppcodctx) noexcept
	{
		if (*ppcodctx)
		{
			// the functions already does the setting nullptr for us
			avcodec_free_context(ppcodctx);
		}
	}

	void safely_free_codec_parameters(::AVCodecParameters** ppcp) noexcept
	{
		if (*ppcp)
		{
			// the functions already does the setting nullptr for us
			avcodec_parameters_free(ppcp);
		}
	}

	void safely_free_sws_context(::SwsContext** sws_ctx) noexcept
	{
		// Don't need to check for nullptr as the func does so
		sws_freeContext(*sws_ctx);
		sws_ctx = nullptr;
	}

	void safely_free_swr_context(::SwrContext** swr_ctx) noexcept
	{
		if (*swr_ctx)
		{
			// Don't need to set it to nullptr as this call does this for us.
			swr_free(swr_ctx);
		}
	}

	void safely_free_audio_fifo(::AVAudioFifo** fifo) noexcept
	{
		if (*fifo)
		{
			av_audio_fifo_free(*fifo);
			*fifo = nullptr;
		}
	}

	std::string ff_translate_error_code(int err_code)
	{
		std::string ret;
		ret.resize(AV_ERROR_MAX_STRING_SIZE);
		
		av_make_error_string(&ret[0], AV_ERROR_MAX_STRING_SIZE, err_code);

		return ret;
	}
}