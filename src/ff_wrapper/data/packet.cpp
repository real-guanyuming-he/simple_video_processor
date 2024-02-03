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

#include "packet.h"

#include "../util/ff_helpers.h"
#include "../formats/stream.h"

extern "C"
{
#include <libavcodec/packet.h>
}

#include <stdexcept>

ff::packet::packet(bool allocate_packet)
	: ff_object(), p_packet(nullptr)
{
	if (allocate_packet)
	{
		allocate_object_memory();
	}
}

ff::packet::packet(::AVPacket* in_packet, const ff::rational time_base, bool has_data)
	: p_packet(in_packet)
{
	if (nullptr == in_packet)
	{
		throw std::invalid_argument("packet cannot be nullptr");
	}

	p_packet->time_base = time_base.av_rational();

	if (has_data)
	{
		state = ff_object_state::READY;
	}
	else
	{
		state = ff_object_state::OBJECT_CREATED;
	}
}

ff::packet::packet(const packet& other)
	: ff_object(other), p_packet(nullptr)
{
	if (nullptr == other.p_packet)
	{
		return;
	}
	
	p_packet = av_packet_clone(other.p_packet);
	if (nullptr == p_packet)
	{
		throw std::bad_alloc();
	}
}

ff::packet& ff::packet::operator=(const packet& right)
{
	ff_object::operator=(right);

	if (nullptr == right.p_packet)
	{
		return *this;
	}

	p_packet = av_packet_clone(right.p_packet);
	if (!p_packet)
	{
		throw std::bad_alloc();
	}
	
	return *this;
}

void ff::packet::internal_allocate_object_memory()
{
	p_packet = av_packet_alloc();
	// Check memory allocation
	if (nullptr == p_packet)
	{
		throw std::bad_alloc();
	}
}

void ff::packet::internal_allocate_resources_memory(uint64_t size, void * additional_information)
{
	auto ret = av_new_packet(p_packet, static_cast<int>(size));
	if (ret != 0)
	{
		ON_FF_ERROR_WITH_CODE("Unable to allocate avpacket", ret);
	}
}

void ff::packet::internal_release_object_memory() noexcept
{
	av_packet_free(&p_packet);
}

void ff::packet::internal_release_resources_memory() noexcept
{
	// p_packet should not be nullptr
	// this is ensured by the state machine of ff_object.
	av_packet_unref(p_packet);
}

int ff::packet::data_size() const
{
	if (!ready())
	{
		throw std::logic_error("The packet is not ready.");
	}
	// If the data comes from a FFmpeg demuxer/encoder,
	// then it will be ref-counted and buf will not be nullptr.
	if (!ref_counted())
	{
		throw std::logic_error("The data does not come from a demuxer/encoder.");
	}

	return p_packet->size;
}

void* ff::packet::data()
{
	if (!ready())
	{
		throw std::logic_error("The packet is not ready.");
	}
	// If the data comes from a FFmpeg demuxer/encoder,
	// then it will be ref-counted and buf will not be nullptr.
	if (!ref_counted())
	{
		throw std::logic_error("The data does not come from a demuxer/encoder.");
	}

	return p_packet->buf;
}

const void* ff::packet::data() const
{
	if (!ready())
	{
		throw std::logic_error("The packet is not ready.");
	}
	// If the data comes from a FFmpeg demuxer/encoder,
	// then it will be ref-counted and buf will not be nullptr.
	if (!ref_counted())
	{
		throw std::logic_error("The data does not come from a demuxer/encoder.");
	}

	return p_packet->buf;
}

ff::rational ff::packet::time_base() const
{
	if (destroyed())
	{
		throw std::logic_error("The packet is destroyed.");
	}

	return ff::rational(p_packet->time_base);
}

ff::time ff::packet::pts() const
{
	if (destroyed())
	{
		throw std::logic_error("The packet is destroyed.");
	}
	if (p_packet->time_base.den == 0)
	{
		throw std::logic_error("Current time base is not valid.");
	}
	ff::rational tb(p_packet->time_base);
	if (tb <= 0)
	{
		throw std::logic_error("Current time base is non-positive");
	}

	return ff::time(p_packet->pts, tb);
}

ff::time ff::packet::dts() const
{
	if (destroyed())
	{
		throw std::logic_error("The packet is destroyed.");
	}
	if (p_packet->time_base.den == 0)
	{
		throw std::logic_error("Current time base is not valid.");
	}
	ff::rational tb(p_packet->time_base);
	if (tb <= 0)
	{
		throw std::logic_error("Current time base is non-positive");
	}

	return ff::time(p_packet->dts, tb);
}

ff::time ff::packet::duration() const
{
	if (destroyed())
	{
		throw std::logic_error("The packet is destroyed.");
	}
	if (p_packet->time_base.den == 0)
	{
		throw std::logic_error("Current time base is not valid.");
	}
	ff::rational tb(p_packet->time_base);
	if (tb <= 0)
	{
		throw std::logic_error("Current time base is non-positive");
	}

	return ff::time(p_packet->duration, tb);
}

void ff::packet::change_time_base(ff::rational new_tb)
{
	if (destroyed())
	{
		throw std::logic_error("The packet is destroyed.");
	}
	if (p_packet->time_base.den == 0)
	{
		throw std::logic_error("Current time base is not valid.");
	}
	ff::rational tb(p_packet->time_base);
	if (tb < 0)
	{
		throw std::logic_error("Current time base is non-positive.");
	}

	// Get pts and dts, and additionally duration
	auto pts = ff::time(p_packet->pts, tb);
	auto dts = ff::time(p_packet->dts, tb);
	auto duration = ff::time(p_packet->duration, tb);

	// Update their time base
	// std::invalid_argument is thrown if new_tb <= 0.
	pts.change_time_base(new_tb);
	dts.change_time_base(new_tb);

	// Update the fields
	p_packet->pts = pts.timestamp_approximate();
	p_packet->dts = dts.timestamp_approximate();
	p_packet->time_base = new_tb.av_rational();

	// Do it for duration, too, if it's valid.
	if (duration > 0)
	{
		duration.change_time_base(new_tb);
		p_packet->duration = duration.timestamp_approximate();
	}
}
