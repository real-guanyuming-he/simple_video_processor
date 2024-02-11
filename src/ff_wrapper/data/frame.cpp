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

ff::frame ff::frame::shared_ref() const
{
	if (!ready())
	{
		throw std::logic_error("The frame is not ready to be shared.");
	}

	auto* shared = av_frame_clone(p_frame);
	if (!shared)
	{
		throw std::bad_alloc();
	}

	return ff::frame(shared, video_or_audio);
}

void ff::frame::reset_time(int64_t pts, int64_t duration, ff::rational time_base)
{
	if (time_base <= 0)
	{
		throw std::invalid_argument("Time base must be > 0.");
	}

	p_frame->pts = pts;
	p_frame->duration = duration;
	p_frame->time_base = time_base.av_rational();
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
		p_frame->width = dp.width;
		p_frame->height = dp.height;
	}
	else // Audio
	{
		p_frame->nb_samples = dp.num_samples;
		dp.ch_layout.set_av_channel_layout(p_frame->ch_layout);
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

	internal_find_num_planes();
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

void ff::frame::internal_find_num_planes() noexcept(FF_ASSERTION_DISABLED)
{
	// Can call it when
	// 1. Created (only inside internal_allocate_resources_memory())
	// 2. Ready (after a copy, resource take over, etc.)
	FF_ASSERT(!destroyed(), "I should not call it when destroyed.");

	// AV_NUM_DATA_POINTERS = max number of possible planes
	for (num_planes = 0; num_planes < AV_NUM_DATA_POINTERS; ++num_planes)
	{
		if (nullptr == p_frame->data[num_planes])
		{
			// Found the first invalid plane.
			break;
		}
	}

	FF_ASSERT(0 != num_planes, "Should have some data.");
}

ff::frame::frame(bool allocate_frame) :
	ff_object(), p_frame(nullptr), num_planes(0)
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
		internal_find_num_planes();
	}
	else
	{
		state = ff::ff_object::ff_object_state::OBJECT_CREATED;
	}
}

ff::frame::frame(const frame& other)
	: ff_object(other), p_frame(nullptr), 
	video_or_audio(other.video_or_audio), num_planes(other.num_planes)
{
	if (other.destroyed())
	{
		return;
	}

	internal_allocate_object_memory();
	// Copy properties that do not affect the data.
	// Its comment/doc does not specify that this may fail.
	av_frame_copy_props(p_frame, other.p_frame);

	if (other.created())
	{
		return;
	}

	// Must allocate memory as av_frame_copy doesn't allocate anything.
	auto dp = other.get_data_properties();
	internal_allocate_resources_memory(0, const_cast<data_properties*>(&dp));
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
	: ff_object(std::move(other)), p_frame(other.p_frame), 
	video_or_audio(other.video_or_audio), num_planes(other.num_planes)
{
	other.p_frame = nullptr;
}

ff::frame& ff::frame::operator=(const frame& right)
{
	ff_object::operator=(right);

	if (right.destroyed())
	{
		return *this;
	}

	internal_allocate_object_memory();
	// Copy properties that do not affect the data.
	// Its comment/doc does not specify that this may fail.
	av_frame_copy_props(p_frame, right.p_frame);

	if (right.created())
	{
		return *this;
	}

	video_or_audio = right.video_or_audio;
	num_planes = right.num_planes;

	// Must allocate memory as av_frame_copy doesn't allocate anything.
	auto dp = right.get_data_properties();
	internal_allocate_resources_memory(0, const_cast<data_properties*>(&dp));
	int ret = av_frame_copy(p_frame, right.p_frame);
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

	return *this;
}

ff::frame& ff::frame::operator=(frame&& right) noexcept
{
	ff_object::operator=(std::move(right));

	p_frame = right.p_frame;
	video_or_audio = right.video_or_audio;
	num_planes = right.num_planes;

	right.p_frame = nullptr;

	return *this;
}

int ff::frame::number_planes() const
{
	if (!ready())
	{
		throw std::logic_error("The frame is not ready");
	}

	return num_planes;
}

int ff::frame::line_size(int ind) const
{
	if (!ready())
	{
		throw std::logic_error("The frame is not ready");
	}

	if (!video_or_audio && ind != 0)
	{
		throw std::out_of_range("audio frame may only have line_size[0].");
	}

	if (ind < 0 || ind >= num_planes)
	{
		throw std::out_of_range("ind is out of range.");
	}

	return p_frame->linesize[ind];
}

void* ff::frame::data(int ind)
{
	if (!ready())
	{
		throw std::logic_error("The frame is not ready");
	}
	if (ind < 0 || ind >= num_planes)
	{
		throw std::out_of_range("ind is out of range.");
	}

	return p_frame->data[ind];
}

const void* ff::frame::data(int ind) const
{
	if (!ready())
	{
		throw std::logic_error("The frame is not ready");
	}
	if (ind < 0 || ind >= num_planes)
	{
		throw std::out_of_range("ind is out of range.");
	}

	return p_frame->data[ind];
}