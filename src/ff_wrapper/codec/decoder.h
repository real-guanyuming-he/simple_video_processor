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

#include "codec_base.h"
#include "../data/frame.h"

namespace ff
{
	class packet;
	class stream;

	/*
	* Represents a decoder, which is just an implementation of codec_base,
	* plus these two methods for decoding: 
	* feed_packet() and decode_frame()
	* 
	* Read the comments for codec_base for how to decode.
	* 
	* Invariants: those of codec_base.
	*/
	class FF_WRAPPER_API decoder final : public codec_base
	{
	public:
		// Constructors must provide identification info for the decoder.
		decoder() = delete;

		/*
		* Identify the decoder by ID and find the description of the decoder.
		* @throws std::invalid_argument if the no decoder of the ID can be found.
		*/
		explicit decoder(AVCodecID ID);
		/*
		* Identify the decoder by name and find the description of the decoder.
		* @throws std::invalid_argument if the no decoder of the name can be found.
		*/
		explicit decoder(const char* name);

		/*
		* Identify the decoder by a stream from a demuxer.
		* The properties of the stream are also completely copied into the decoder.
		* 
		* After this constructor returns, the decoder will immediately be ready.
		* The behaviour is undefined if the stream does not come from a demuxer.
		* 
		* @throws std::invalid_argument if the codec that encoded the stream is not supported.
		*/
		explicit decoder(const stream& s);

		// Even if the properties can be copied,
		// the internal state and buffers of a decoder cannot.
		decoder(const decoder&) = delete;

		/*
		* Takes over other and set all other's pointers to nullptr.
		*/
		decoder(decoder&& other) noexcept;

		/*
		* Destroys itself, takes over other, and set all other's pointers to nullptr.
		*/
		decoder& operator=(decoder&& right) noexcept;

		/*
		* Destroys the decoder, clears the description and everything else completely.
		*/
		~decoder() noexcept;

	public:
/////////////////////////////// The decoding process ///////////////////////////////
		/*
		* Feeds a packet (food) into the decoder.
		* See the comments for codec_base for how to feed a decoder.
		*
		* @returns true if the packet has been fed successfully. false if
		* the decoder is full or you have already declared no more packets will be fed.
		* @throws std::logic_error if the decoder is not ready.
		* @throws std::invalid_argument if the packet is not ready or if the packet does not agree with
		* the properties of the decoder or with previous packets fed to it.
		*/
		bool feed_packet(const packet& pkt);

		/*
		* Decodes the next frame from the packets it has been fed.
		* See the comments for codec_base for when to decode frames.
		*
		* @returns a frame decoded; a DESTROYED frame if the decoder is hungry, or
		* if the decoder has nothing left in its stomach after you started draining it.
		* @throws std::logic_error if the decoder is not ready.
		*/
		frame decode_frame();

	private:
/////////////////////////////// Derived from ff_object ///////////////////////////////
		/*
		* According to the info passed through one of the constructors,
		* identify the encoder and create its description.
		*/
		void internal_allocate_object_memory() override;

/////////////////////////////// Derived from codec_base ///////////////////////////////
		void start_draining() override;
	};

}