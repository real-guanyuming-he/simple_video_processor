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

extern "C"
{
#include <libswscale/swscale.h>
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

	if (!query_input_pixel_format_support((AVPixelFormat)src_properties.fmt))
	{
		throw std::domain_error("The input pixel format is not supported.");
	}	
	if (!query_output_pixel_format_support((AVPixelFormat)dst_properties.fmt))
	{
		throw std::domain_error("The output pixel format is not supported.");
	}

	sws_ctx = sws_getContext
	(
		src_properties.width, src_properties.height, (AVPixelFormat)src_properties.fmt,
		dst_properties.width, dst_properties.height, (AVPixelFormat)dst_properties.fmt,
		(int)algorithm,
		nullptr, nullptr, nullptr
	);

	if (!sws_ctx)
	{
		throw std::runtime_error("Unexpected error happened: Could not create a sws ctx.");
	}

	src_w = src_properties.width;
	src_h = src_properties.height;
	dst_w = dst_properties.width;
	dst_h = dst_properties.height;
	src_fmt = (AVPixelFormat)src_properties.fmt;
	dst_fmt = (AVPixelFormat)dst_properties.fmt;
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
