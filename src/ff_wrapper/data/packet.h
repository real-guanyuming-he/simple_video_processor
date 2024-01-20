#pragma once
/*
* Copyright (C) Guanyuming He 2024
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

struct AVPacket;

#include "../util/ff_object.h"

namespace ff
{
	class packet final : public ff_object
	{
	public:
		/*
		* Constructs the packet. Optionally allocates memory for the packet
		* if the argument is true. By default that is false.
		* 
		* @param allocate_packet decides whether the packet will be allocated.
		*/
		packet(bool allocate_packet = false);

		/*
		* Takes ownership of the incoming in_packet.
		* Typically the packet comes from a demuxer.
		* 
		* @param has_data: set it to true if in_packet has data stored. 
		If so the object's state will be ready.
		*/
		packet(::AVPacket* in_packet, bool has_data = true) noexcept;

	private:
		/*
		* Allocates the packet by calling av_packet_alloc()
		*/
		virtual void internal_allocate_object_memory() override;

		/*
		* Allocates the packet's data (payload) by calling av_new_packet() and passing size to it.
		* 
		* @param size is the size of the space allocated for the data
		* @param additional_information is not used.
		*/
		virtual void internal_allocate_resources_memory(uint64_t size, void* additional_information = nullptr) override;

		/*
		* Frees the packet by calling av_packet_free()
		*/
		virtual void internal_release_object_memory() override;

		/*
		* Frees the packet's data (payload) by calling av_packet_unref()
		*/
		virtual void internal_release_resources_memory() override;

	private:
		::AVPacket* p_av_packet;
	};
}