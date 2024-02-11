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

#include "channel_layout.h"
#include "ff_helpers.h"

extern "C"
{
#include <libavutil/avutil.h> // For AVERROR
}

ff::channel_layout::channel_layout(AVChannelOrder order, int num_channels, uint64_t mask)
	: p_cl(nullptr), // First assign it a temp placeholder
	is_weak_ref(false)
{
	// Then create the new channel layout and,
	auto* temp = new AVChannelLayout();
	temp->order = order;
	temp->nb_channels = num_channels;
	temp->u.mask = mask;
	// assign p_cl with the new layout.
	p_cl = temp;

	// Check if the resulted layout is valid.
	if (!av_channel_layout_check(p_cl))
	{
		throw std::invalid_argument("The parameters you gave resulted in an invalid channel layout.");
	}
}

ff::channel_layout::channel_layout(default_layouts l)
	: p_cl(nullptr), // First assign it a temp placeholder
	is_weak_ref(true)
{
	switch (l)
	{
		// copy what's inside enum class default_layouts here
		// 1. First remove all commas without using regex because VS regex somehow doesn't recognise commas.
		// 2. search against this regex: FF_([\w]+)(\n)
		// and replace them with 
		// case default_layouts::FF_$1:\n cl = $1;\n break;

	case default_layouts::FF_AV_CHANNEL_LAYOUT_MONO:
		p_cl = &ff_AV_CHANNEL_LAYOUT_MONO;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_STEREO:
		p_cl = &ff_AV_CHANNEL_LAYOUT_STEREO;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_2POINT1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_2POINT1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_2_1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_2_1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_SURROUND:
		p_cl = &ff_AV_CHANNEL_LAYOUT_SURROUND;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_3POINT1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_3POINT1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_4POINT0:
		p_cl = &ff_AV_CHANNEL_LAYOUT_4POINT0;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_4POINT1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_4POINT1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_2_2:
		p_cl = &ff_AV_CHANNEL_LAYOUT_2_2;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_QUAD:
		p_cl = &ff_AV_CHANNEL_LAYOUT_QUAD;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_5POINT0:
		p_cl = &ff_AV_CHANNEL_LAYOUT_5POINT0;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_5POINT1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_5POINT1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_5POINT0_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_5POINT0_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_5POINT1_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_5POINT1_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_6POINT0:
		p_cl = &ff_AV_CHANNEL_LAYOUT_6POINT0;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_6POINT0_FRONT:
		p_cl = &ff_AV_CHANNEL_LAYOUT_6POINT0_FRONT;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_3POINT1POINT2:
		p_cl = &ff_AV_CHANNEL_LAYOUT_3POINT1POINT2;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_HEXAGONAL:
		p_cl = &ff_AV_CHANNEL_LAYOUT_HEXAGONAL;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_6POINT1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_6POINT1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_6POINT1_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_6POINT1_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_6POINT1_FRONT:
		p_cl = &ff_AV_CHANNEL_LAYOUT_6POINT1_FRONT;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT0:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT0;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT0_FRONT:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT0_FRONT;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT1:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT1;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT1_WIDE:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT1_WIDE;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT1_WIDE_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT1_WIDE_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_5POINT1POINT2_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_5POINT1POINT2_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_OCTAGONAL:
		p_cl = &ff_AV_CHANNEL_LAYOUT_OCTAGONAL;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_CUBE:
		p_cl = &ff_AV_CHANNEL_LAYOUT_CUBE;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_5POINT1POINT4_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_5POINT1POINT4_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT1POINT2:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT1POINT2;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT1POINT4_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT1POINT4_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_HEXADECAGONAL:
		p_cl = &ff_AV_CHANNEL_LAYOUT_HEXADECAGONAL;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_STEREO_DOWNMIX:
		p_cl = &ff_AV_CHANNEL_LAYOUT_STEREO_DOWNMIX;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_22POINT2:
		p_cl = &ff_AV_CHANNEL_LAYOUT_22POINT2;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_7POINT1_TOP_BACK:
		p_cl = &ff_AV_CHANNEL_LAYOUT_7POINT1_TOP_BACK;
		break;
	case default_layouts::FF_AV_CHANNEL_LAYOUT_AMBISONIC_FIRST_ORDER:
		p_cl = &ff_AV_CHANNEL_LAYOUT_AMBISONIC_FIRST_ORDER;
		break;
	default:
		throw std::invalid_argument("The layout you specified does not exist.");
	}

	FF_ASSERT(p_cl != nullptr, "Should cover all possibilities.");
}

ff::channel_layout::channel_layout(const AVChannelLayout& src, bool weak_ref)
	: p_cl(nullptr), // First assign it a temp placeholder
	is_weak_ref(weak_ref)
{
	if (!av_channel_layout_check(&src))
	{
		throw std::invalid_argument("The first parameter you gave is an invalid channel layout.");
	}

	if (is_weak_ref)
	{
		p_cl = &src;
	}
	else
	{
		// Create a new layout
		auto* temp = new AVChannelLayout();
		// copy src to it
		channel_layout::av_channel_layout_copy(*temp, src);
		// Then assign p_cl with the new layout.
		p_cl = temp; 
	}
}

ff::channel_layout::channel_layout(const channel_layout& other, bool weak_ref)
	: p_cl(nullptr), // First assign it a temp placeholder
	is_weak_ref(weak_ref)
{
	if (is_weak_ref)
	{
		p_cl = other.p_cl;
	}
	else
	{
		// Create a new layout
		auto* temp = new AVChannelLayout();
		// copy src to it
		channel_layout::av_channel_layout_copy(*temp, *other.p_cl);
		// Then assign p_cl with the new layout.
		p_cl = temp;
	}
}

ff::channel_layout& ff::channel_layout::operator=(const channel_layout& right)
{
	destroy();

	// Create a new layout
	auto* temp = new AVChannelLayout();
	// copy src to it
	channel_layout::av_channel_layout_copy(*temp, *right.p_cl);
	// Then assign p_cl with the new layout.
	p_cl = temp;

	// Don't forget to set is_weak_ref
	is_weak_ref = false;

	return *this;
}

ff::channel_layout& ff::channel_layout::operator=(channel_layout&& right) noexcept
{
	destroy();

	p_cl = right.p_cl;
	is_weak_ref = right.is_weak_ref;

	right.is_weak_ref = true;
	return *this;
}

bool ff::channel_layout::operator==(const channel_layout& right) const noexcept(FF_ASSERTION_DISABLED)
{
	int ret = av_channel_layout_compare(p_cl, right.p_cl);

	if (0 == ret) // Equal
	{
		return true;
	}
	else if (1 == ret) // Nonequal
	{
		return false;
	}
	
	FF_ASSERT(ret >= 0, "A negative error means one of them is invalid, " 
		"which should not happen. Being valid is the invariant of this class.");

	return false;
}

bool ff::channel_layout::operator==(const AVChannelLayout& right) const noexcept
{
	int ret = av_channel_layout_compare(p_cl, &right);

	if (0 == ret) // Equal
	{
		return true;
	}
	else // Nonequal or error.
	{
		// But I don't care about errors in this method.
		return false;
	}
}

inline void ff::channel_layout::set_av_channel_layout(AVChannelLayout& dst) const
{
	channel_layout::av_channel_layout_copy(dst, *p_cl);
}

void ff::channel_layout::av_channel_layout_copy(AVChannelLayout& dst, const AVChannelLayout& src)
{
	int ret = ::av_channel_layout_copy(&dst, &src);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: could not copy channel layout", ret);
		}
	}
}
