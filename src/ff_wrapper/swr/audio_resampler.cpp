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

#include "audio_resampler.h"

#include "../util/ff_helpers.h"
#include "../util/channel_layout.h"

extern "C"
{
#include <libswresample/swresample.h>
}

ff::audio_resampler::audio_resampler
(
	const channel_layout& dst_ch_layout, 
	AVSampleFormat dst_sample_fmt, 
	int dst_sample_rate, 
	const channel_layout& src_ch_layout, 
	AVSampleFormat src_sample_fmt, 
	int src_sample_rate
)
{
	int ret = swr_alloc_set_opts2
	(
		&swr_ctx,
		&dst_ch_layout.av_ch_layout(),
		dst_sample_fmt,
		dst_sample_rate,
		&src_ch_layout.av_ch_layout(),
		src_sample_fmt,
		src_sample_rate,
		0, nullptr
	);

	if (ret < 0)
	{
		// Failure
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL):
			throw std::invalid_argument("One of the parameters you gave is invalid.");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: Could not create a swr context.", ret);
		}
	}

	// TODO: also considering initializing the FIFO buffer here.
}

ff::audio_resampler::~audio_resampler()
{
	ffhelpers::safely_free_swr_context(&swr_ctx);
	// TODO: free the FIFO buffer.
}
