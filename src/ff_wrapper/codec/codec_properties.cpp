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
#include "codec_properties.h"
#include "../util/ff_helpers.h"
#include "../util/channel_layout.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

#include <stdexcept>

ff::codec_properties::codec_properties()
	: p_params(avcodec_parameters_alloc()), tb()
{
	if (nullptr == p_params)
	{
		throw std::bad_alloc();
	}
}

ff::codec_properties::~codec_properties()
{
	ffhelpers::safely_free_codec_parameters(&p_params);
}

ff::codec_properties::codec_properties(::AVCodecParameters* p, ff::rational time_base, bool take_over)
	: tb(time_base)
{
	if (take_over)
	{
		p_params = p;
		return;
	}

	// No taking over, so make a copy.
	// Allocates the parameters first.
	p_params = avcodec_parameters_alloc();
	if (nullptr == p_params)
	{
		throw std::bad_alloc();
	}

	int ret = avcodec_parameters_copy(p_params, p);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: Could not copy AVCodecParameters", ret);
			break;
		}
	}
}

ff::codec_properties::codec_properties(const::AVCodecContext* codec_ctx)
	: codec_properties() // Must allocate the properties first
{
	tb = codec_ctx->time_base;

	int ret = avcodec_parameters_from_context(p_params, codec_ctx);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: Could not copy AVCodecParameters from AVCodecContext", ret);
			break;
		}
	}
}

ff::codec_properties::codec_properties(const codec_properties& other)
	: codec_properties() // must allocate a space first.
{
	tb = other.tb;

	int ret = avcodec_parameters_copy(p_params, other.p_params);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: Could not copy AVCodecParameters", ret);
			break;
		}
	}
}

void ff::codec_properties::set_a_channel_layout(const AVChannelLayout& ch)
{
	channel_layout::av_channel_layout_copy(p_params->ch_layout, ch);
}