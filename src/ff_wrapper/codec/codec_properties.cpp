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
#include <string.h>

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

	avcodec_parameters_copy(*p_params, *p);
}

ff::codec_properties::codec_properties(const::AVCodecContext* codec_ctx)
	: codec_properties() // Must allocate the properties first
{
	if (!ff::av_rational_invalid_or_zero(codec_ctx->time_base))
	{
		tb = codec_ctx->time_base;
	}

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

	avcodec_parameters_copy(*p_params, *(other.p_params));
}

void ff::codec_properties::set_a_channel_layout(const AVChannelLayout& ch)
{
	channel_layout::av_channel_layout_copy(p_params->ch_layout, ch);
}

void ff::codec_properties::alloc_and_zero_extradata(AVCodecParameters& p, size_t size, bool zero_all)
{
	// Only do this if p's extradata is not set.
	if (nullptr == p.extradata)
	{
		// Must be allocated with av_malloc() and will be freed by avcodec_parameters_free(). 
		// The allocated size of extradata must be at least extradata_size + AV_INPUT_BUFFER_PADDING_SIZE, 
		p.extradata = (uint8_t*)av_malloc(size + AV_INPUT_BUFFER_PADDING_SIZE);

		if (nullptr == p.extradata)
		{
			throw std::bad_alloc();
		}

		p.extradata_size = size;

		// with the padding bytes zeroed.
		if (zero_all)
		{
			// zero all bytes instead
			memset(p.extradata, 0, size + AV_INPUT_BUFFER_PADDING_SIZE);
		}
		else
		{
			// zero only the padding bytes
			memset(p.extradata + size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
		}
	}
}

ff::codec_properties ff::codec_properties::essential_properties() const
{
	codec_properties ret;

	// Copy the type
	ret.p_params->codec_type = p_params->codec_type;

	// Copy essential properties of all types.
	ret.p_params->format = p_params->format;
	ret.tb = tb;
	
	// Copy type specific properties.
	switch (type())
	{
	case AVMEDIA_TYPE_VIDEO: // Video
		ret.p_params->width = p_params->width;
		ret.p_params->height = p_params->height;
		ret.p_params->sample_aspect_ratio = p_params->sample_aspect_ratio;
		ret.p_params->framerate = p_params->framerate;
		break;
	case AVMEDIA_TYPE_AUDIO: // Audio
		ff::channel_layout::av_channel_layout_copy(ret.p_params->ch_layout, p_params->ch_layout);
		ret.p_params->sample_rate = p_params->sample_rate;
		break;
	}

	return ret;
}

void ff::codec_properties::avcodec_parameters_copy(AVCodecParameters& dst, const AVCodecParameters& src)
{
	int ret = ::avcodec_parameters_copy(&dst, &src);
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
