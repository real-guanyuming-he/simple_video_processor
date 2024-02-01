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

#include "frame.h"
#include "../util/ff_helpers.h"

extern "C"
{
#include <libavutil/channel_layout.h>
}

#include <stdexcept>

ff::frame::data_properties ff::frame::get_data_properties() const
{
	if (!ready())
	{
		throw std::logic_error("The frame is not ready.");
	}

	if (video_or_audio)
	{
		return data_properties
		(
			p_frame->format,
			p_frame->width, p_frame->height
		);
	}
	else
	{
		return data_properties
		(
			p_frame->format,
			p_frame->nb_samples, p_frame->ch_layout
		);
	}
}

void ff::frame::internal_allocate_object_memory()
{
	p_frame = av_frame_alloc();
	if (nullptr == p_frame)
	{
		throw std::bad_alloc();
	}
}

void ff::frame::internal_allocate_resources_memory(uint64_t size, void* additional_information)
{
	const auto& dp = *static_cast<data_properties*>(additional_information);

	video_or_audio = dp.v_or_a;
	p_frame->format = dp.fmt;
	if (dp.v_or_a) // Video
	{
		p_frame->width = dp.details.v.width;
		p_frame->height = dp.details.v.height;
	}
	else // Audio
	{
		p_frame->nb_samples = dp.details.a.num_samples;
		// The doc forbids copying by assignment.
		// p_frame->ch_layout = dp.details.a.ch_layout;
		// Instead, use
		if (av_channel_layout_copy(&p_frame->ch_layout, dp.details.a.ch_layout_ref) < 0)
		{
			throw std::bad_alloc();
		}
	}

	int ret = av_frame_get_buffer(p_frame, dp.align);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL):
			throw std::invalid_argument("Could not allocate a buffer because of invalid arguments (Probably yours).");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: could not allocate buffer for avframe", ret);
		}
	}
}

void ff::frame::internal_release_object_memory() noexcept
{
	ffhelpers::safely_free_frame(&p_frame);
}

void ff::frame::internal_release_resources_memory() noexcept
{
	// Invariant ensures that data can only be ref-counted.
	av_frame_unref(p_frame);
}

ff::frame::frame(bool allocate_frame) :
	ff_object(), p_frame(nullptr)
{
	if (allocate_frame)
	{
		allocate_object_memory();
	}
}

ff::frame::frame(::AVFrame* p_frame, bool v_or_a, bool has_data)
	: p_frame(p_frame)
{
	if (has_data)
	{
		state = ff::ff_object::ff_object_state::READY;
		video_or_audio = v_or_a;
	}
	else
	{
		state = ff::ff_object::ff_object_state::OBJECT_CREATED;
	}
}

ff::frame::frame(const frame& other)
	: ff_object(other), p_frame(nullptr)
{
	if (other.destroyed())
	{
		return;
	}

	allocate_object_memory();
	// Copy properties that do not affect the data.
	// Its comment/doc does not specify that this may fail.
	av_frame_copy_props(p_frame, other.p_frame);

	if (other.created())
	{
		return;
	}

	int ret = av_frame_copy(p_frame, other.p_frame);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: could not copy avframe's data", ret);
		}
	}
}

ff::frame::frame(frame&& other) noexcept
	: ff_object(std::move(other)), p_frame(other.p_frame), video_or_audio(other.video_or_audio)
{
	other.p_frame = nullptr;
}