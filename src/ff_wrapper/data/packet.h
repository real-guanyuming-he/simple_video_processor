#pragma once
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

struct AVPacket;

#include "../util/ff_object.h"
#include "../util/ff_time.h"

namespace ff
{
	class stream;

	class FF_WRAPPER_API packet final : public ff_object
	{
	public:
		~packet() { destroy(); }

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
		* @param in_packet the incoming packet
		* @param stream the stream to link it with. Such packets usually come from a demuxer,
		* and it's natural to link it with an output av stream.
		* @param has_data: set it to true if in_packet has data stored. 
		If so the object's state will be ready.
		*/
		packet(::AVPacket* in_packet, const stream* stream = nullptr, bool has_data = true) noexcept;

		/*
		* Copies other with av_packet_clone().
		* If other's avpkt is nullptr, set this's to nullptr as well.
		*/
		packet(const packet& other);
		/*
		* Simply copies other's pointers and sets other's to nullptr.
		* Calls base's move ctor to handle the state.
		*/
		packet(packet&& other) noexcept
			: ff_object(other), p_av_packet(other.p_av_packet), p_stream(other.p_stream)
		{
			other.p_av_packet = nullptr;
			other.p_stream = nullptr;
		}

		packet& operator=(const packet& right);
		/*
		* Destroys itself and copies right's pointers.
		* Sets right's to nullptr afterwards.
		* Calls base's move ass operator to handle the state.
		*/
		inline packet& operator=(packet&& right) noexcept
		{
			// base's already calls destroy().
			ff_object::operator=(std::move(right));

			p_av_packet = right.p_av_packet;
			p_stream = right.p_stream;
			right.p_av_packet = nullptr;
			right.p_stream = nullptr;

			return *this;
		}

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
		virtual void internal_release_object_memory() noexcept override;

		/*
		* Frees the packet's data (payload) by calling av_packet_unref()
		*/
		virtual void internal_release_resources_memory() noexcept override;

	public:
		/*
		* @returns the presentation time of the packet.
		* @throws std::logic_error if it's not linked to a stream (so timebase cannot be obtained).
		*/
		ff::time pts() const;
		/*
		* @returns the decompression time of the packet.
		* @throws std::logic_error if it's not linked to a stream (so timebase cannot be obtained).
		*/
		ff::time dts() const;

	public:
		const ::AVPacket* get_av_packet() const noexcept { return p_av_packet; }
		::AVPacket* get_av_packet() noexcept { return p_av_packet; }

		const ::AVPacket* operator->() const noexcept { return p_av_packet; }
		::AVPacket* operator->()  noexcept { return p_av_packet; }

		const stream* get_stream() const noexcept { return p_stream; }
		bool linked_to_stream() const noexcept { return nullptr != p_stream; }
		void link_to_stream(const stream* stream) noexcept { p_stream = stream; }

	private:
		::AVPacket* p_av_packet;
		const stream* p_stream = nullptr;
	};
}