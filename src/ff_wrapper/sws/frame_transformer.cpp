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

#include "frame_transformer.h"
#include "../util/ff_helpers.h"
#include "../codec/encoder.h"
#include "../codec/decoder.h"

extern "C"
{
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h> // For accessing AVCodecContext.
}

ff::frame_transformer::frame_transformer
(
	const frame::data_properties& dst_properties,
	const frame::data_properties& src_properties,
	algorithms algorithm
)
{
	if (!src_properties.v_or_a)
	{
		throw std::invalid_argument("The src properties are not for video.");
	}
	if (!dst_properties.v_or_a)
	{
		throw std::invalid_argument("The dst properties are not for video.");
	}

	internal_create_sws_context
	(
		dst_properties.width, dst_properties.height, (AVPixelFormat)dst_properties.fmt,
		src_properties.width, src_properties.height, (AVPixelFormat)src_properties.fmt,
		algorithm
	);
}

ff::frame_transformer::frame_transformer(const encoder& enc, const decoder& dec, algorithms algorithm)
{
	if (!dec.ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}
	if (!enc.ready())
	{
		throw std::logic_error("The encoder is not ready.");
	}

	if (!dec.is_video())
	{
		throw std::invalid_argument("The decoder is not for video.");
	}
	if (!enc.is_video())
	{
		throw std::invalid_argument("The encoder is not for video.");
	}

	internal_create_sws_context
	(
		enc->width, enc->height, enc->pix_fmt,
		dec->width, dec->height, dec->pix_fmt,
		algorithm
	);
}

ff::frame_transformer::frame_transformer
(
	int dst_w, int dst_h, AVPixelFormat dst_fmt, 
	int src_w, int src_h, AVPixelFormat src_fmt, 
	algorithms algorithm
)
{
	internal_create_sws_context
	(
		dst_w, dst_h, dst_fmt,
		src_w, src_h, src_fmt,
		algorithm
	);
}

ff::frame_transformer::~frame_transformer()
{
	ffhelpers::safely_free_sws_context(&sws_ctx);
}

ff::frame ff::frame_transformer::convert_frame(const ff::frame& src)
{
	if (src.get_data_properties() != src_properties())
	{
		throw std::invalid_argument("Src does not match the properties you gave at first.");
	}

	ff::frame dst(true);
	dst.allocate_data(dst_properties());

	// I don't know what sws_scale_frame does exactly,
	// so I use this just to be safe.
	int ret = sws_scale
	(
		sws_ctx, 
		src->data, src->linesize,
		0, src->height,
		dst->data, dst->linesize
	);

	if (ret < 0) // Failure
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened during transforming frames", ret);
		}
	}

	// Copy src's properties to dst
	frame::av_frame_copy_props(dst, src);

	return dst;
}

void ff::frame_transformer::convert_frame(ff::frame& dst, const ff::frame& src)
{
	if (src.get_data_properties() != src_properties())
	{
		throw std::invalid_argument("Src does not match the properties you gave at first.");
	}

	if (dst.destroyed())
	{
		dst.allocate_object_memory();
		dst.allocate_data(dst_properties());
	}
	else if (dst.created())
	{
		dst.allocate_data(dst_properties());
	}
	else // dst.ready()
	{
		if (dst.get_data_properties() != dst_properties())
		{
			throw std::invalid_argument("Dst does not match the properties you gave at first.");
		}
	}

	int ret = sws_scale_frame(sws_ctx, dst.av_frame(), src.av_frame());
	if (ret < 0) // Failure
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened during transforming frames", ret);
		}
	}

	// Copy src's properties to dst
	frame::av_frame_copy_props(dst, src);
}

bool ff::frame_transformer::query_input_pixel_format_support(AVPixelFormat fmt)
{
	// Return a positive value if pix_fmt is a supported input format, 0 otherwise.
	return 0 != sws_isSupportedInput(fmt);
}

bool ff::frame_transformer::query_output_pixel_format_support(AVPixelFormat fmt)
{
	// Return a positive value if pix_fmt is a supported output format, 0 otherwise.
	return 0 != sws_isSupportedOutput(fmt);
}

void ff::frame_transformer::internal_create_sws_context
(
	int dst_w, int dst_h, AVPixelFormat dst_fmt, 
	int src_w, int src_h, AVPixelFormat src_fmt, 
	algorithms algorithm
)
{
	if (!query_input_pixel_format_support((AVPixelFormat)src_fmt))
	{
		throw std::domain_error("The input pixel format is not supported.");
	}
	if (!query_output_pixel_format_support((AVPixelFormat)dst_fmt))
	{
		throw std::domain_error("The output pixel format is not supported.");
	}

	sws_ctx = sws_getContext
	(
		src_w, src_h, src_fmt,
		dst_w, dst_h, dst_fmt,
		(int)algorithm,
		nullptr, nullptr, nullptr
	);

	if (!sws_ctx)
	{
		throw std::runtime_error("Unexpected error happened: Could not create a sws ctx.");
	}

	this->src_w = src_w;
	this->src_h = src_h;
	this->dst_w = dst_w;
	this->dst_h = dst_h;
	this->src_fmt = src_fmt;
	this->dst_fmt = dst_fmt;
}
