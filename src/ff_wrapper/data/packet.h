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

	/*
	* Represents a packet (i.e. compressed multimedia data).
	* 
	* I offer flexibility for handling complicated situations. Therefore,
	* then only invariants are those of ff_object.
	* You can decide how you are going to use a packet:
	* Whether you set some fields or not, my methods will detect if the fields are set before using them.
	* If you call any of them, I assume you have set the corresponding fields so I will throw exceptions if you have not.
	* 
	* My methods cover the access to the most important fields:
	*	1. Time related: time base, pts, dts, and duration.
	*	2. Data, if the packet comes from a demuxer/encoder or you allocated the data through my method.
	* Access to other fields are not wrapped and you have to use av_packet() to access them directly.
	*/
	class FF_WRAPPER_API packet final : public ff_object
	{
	public:
		inline ~packet() { destroy(); }

		/*
		* Constructs the packet. Optionally allocates memory for the packet
		* if the argument is true. By default that is false.
		* 
		* @param allocate_packet decides whether the packet will be allocated.
		*/
		explicit packet(bool allocate_packet = false);

		/*
		* Takes ownership of the incoming in_packet,
		* and additionally sets its time_base if its is invalid or 0.
		* Typically the packet comes from a demuxer.
		* 
		* @param in_packet the incoming packet. Cannot be nullptr.
		* @param time_base its new time base. Can give it an invalid one (i.e. non-positive)
		* as my methods will check it. Default is 0.
		* @param has_data: set it to true if in_packet has data stored. 
		If so the object's state will be ready.
		* @throws std::invalid_argument if in_packet is nullptr.
		*/
		packet
		(
			::AVPacket* in_packet, 
			const ff::rational time_base = ff::zero_rational, 
			bool has_data = true
		);

		/*
		* Copies other with av_packet_clone(), which makes this refer to the same data as other.
		* NOTE: it does NOT copy other's data!
		* If other's avpkt is nullptr, set this's to nullptr as well.
		*/
		packet(const packet& other);
		/*
		* Simply copies other's pointers and sets other's to nullptr.
		* Calls base's move ctor to handle the state.
		*/
		packet(packet&& other) noexcept
			: ff_object(std::move(other)), p_packet(other.p_packet)
		{
			other.p_packet = nullptr;
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

			p_packet = right.p_packet;
			right.p_packet = nullptr;

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
		* @throws std::invalid_argument if size = 0.
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
//////////////////////////////////////// Access to important fields ////////////////////////////////////////

		/*
		* @returns the size of the data that the packet holds.
		* @throws std::logic_error if the packet is not READY, if the data does not come from
		* a demuxer/encoder, or if the data is not allocated through allocate_resources_memory().
		*/
		int data_size() const;
		/*
		* @returns a pointer to the start of the data the packet holds.
		* @throws std::logic_error if the packet is not READY, if the data does not come from
		* a demuxer/encoder, or if the data is not allocated through allocate_resources_memory().
		*/
		void* data();
		/*
		* @returns a const pointer to the start of the data the packet holds.
		* @throws std::logic_error if the packet is not READY, or if the data does not come from
		* a demuxer/encoder.
		*/
		const void* data() const;

		/*
		* @returns the time base. Can be invalid if not set.
		* @throws std::logic_error if the packet is destroyed.
		*/
		ff::rational time_base() const;

		/*
		* @returns the presentation time of the packet.
		* @throws std::logic_error if the packet is destroyed.
		* @throws std::logic_error if its timebase's not valid (i.e. not set yet).
		*/
		ff::time pts() const;
		/*
		* @returns the decompression time of the packet.
		* @throws std::logic_error if the packet is destroyed.
		* @throws std::logic_error if its timebase's not valid (i.e. not set yet).
		*/
		ff::time dts() const;

		/*
		* @returns the duration of the packet. Can be non-positive if the duration is not known.
		* You should check the return value.
		* @throws std::logic_error if the packet is destroyed.
		* @throws std::logic_error if its timebase's not valid (i.e. not set yet).
		*/
		ff::time duration() const;

		/*
		* Changes the time base of the packet so that pts and dts are updated.
		* In addition, if its duration is set (i.e. > 0), then that's also updated.
		* 
		* @param tb the new time base
		* @throws std::logic_error if the packet is destroyed.
		* @throws std::logic_error if its current timebase's not valid (i.e. not set yet).
		* @throws std::invalid_argument if the new time base's not valid (i.e. non-positive).
		*/
		void change_time_base(ff::rational new_tb);

		/*
		* Resets the time fields to the given arguments.
		* 
		* @param dts new dts
		* @param pts new pts
		* @param duration new duration
		* @param time_base new time base
		* @throws std::invalid_argument if dts > pts
		* @throws std::invalid_argument if time_base <= 0
		* @throws std::invalid_argument if pts < 0
		*/
		void reset_time(int64_t dts, int64_t pts, int64_t duration, ff::rational time_base);

	public:
//////////////////////////////////////// Exposers ////////////////////////////////////////
		const ::AVPacket* av_packet() const noexcept { return p_packet; }
		::AVPacket* av_packet() noexcept { return p_packet; }

		const ::AVPacket* operator->() const noexcept { return p_packet; }
		::AVPacket* operator->()  noexcept { return p_packet; }

	public:
//////////////////////////////////////// Helpers ////////////////////////////////////////
		/*
		* Prepares the packet so that it can be fed into a muxer.
		* It is a convenient helper that 
		*	1. change's the packet's time base and time fields
		*	2. sets its reference to the muxer stream correctly.
		* 
		* @param muxer_stream the stream that the packet will belong to.
		*/
		void prepare_for_muxing(const stream& muxer_stream);

		/*
		* My wrapper for ::av_packet_copy_props()
		* that interprets errors as exceptions.
		*/
		static void av_packet_copy_props(AVPacket& dst, const AVPacket& src);

		/*
		* If !dst.destroyed() && !src.destroyed(),
		* then av_packet_copy_props(*dst.p_packet, *src.p_packet);
		* 
		* @throws std::logic_error if either is destroyed.
		*/
		static void av_packet_copy_props(packet& dst, const packet& src);

	private:
		::AVPacket* p_packet;

	private:
		/*
		* NOTE: only call it when ready(). I do not put an assertion in it
		* so that it can be inline.
		* 
		* @returns Whether the buf is ref-counted
		*/
		inline bool ref_counted() const noexcept { return nullptr != p_packet->buf; }
	};
}