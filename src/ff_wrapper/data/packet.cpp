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
	: p_av_packet(nullptr)
{
	if (allocate_packet)
	{
		// internal_allocate_object_memory()
		// will call this class's version.
		allocate_object_memory();
	}
}

ff::packet::packet(::AVPacket* in_packet, const stream* stream, bool has_data) noexcept
	: p_av_packet(in_packet), p_stream(stream)
{
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
	: ff_object(other), p_av_packet(nullptr)
{
	if (nullptr == other.p_av_packet)
	{
		return;
	}
	
	p_av_packet = av_packet_clone(other.p_av_packet);
	if (nullptr == p_av_packet)
	{
		throw std::bad_alloc();
	}
	p_stream = other.p_stream;
}

ff::packet& ff::packet::operator=(const packet& right)
{
	ff_object::operator=(right);

	if (nullptr == right.p_av_packet)
	{
		return *this;
	}

	p_av_packet = av_packet_clone(right.p_av_packet);
	if (!p_av_packet)
	{
		throw std::bad_alloc();
	}
	p_stream = right.p_stream;
	
	return *this;
}

void ff::packet::internal_allocate_object_memory()
{
	p_av_packet = av_packet_alloc();
	// Check memory allocation
	if (nullptr == p_av_packet)
	{
		throw std::bad_alloc();
	}
}

void ff::packet::internal_allocate_resources_memory(uint64_t size, void* additional_information)
{
	auto ret = av_new_packet(p_av_packet, static_cast<int>(size));
	if (ret != 0)
	{
		ON_FF_ERROR_WITH_CODE("Unable to allocate avpacket", ret);
	}
}

void ff::packet::internal_release_object_memory() noexcept
{
	av_packet_free(&p_av_packet);
}

void ff::packet::internal_release_resources_memory() noexcept
{
	// p_av_packet should not be nullptr
	// this is ensured by the state machine of ff_object.
	av_packet_unref(p_av_packet);
}

ff::time ff::packet::pts() const
{
	if (!linked_to_stream())
	{
		throw std::logic_error("Not linked to any stream.");
	}

	return ff::time(p_av_packet->pts, p_stream->time_base());
}

ff::time ff::packet::dts() const
{
	if (!linked_to_stream())
	{
		throw std::logic_error("Not linked to any stream.");
	}

	return ff::time(p_av_packet->dts, p_stream->time_base());
}
