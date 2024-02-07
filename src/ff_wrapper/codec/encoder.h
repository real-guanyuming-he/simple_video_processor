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
#include "../data/packet.h"

namespace ff
{
	class frame;
	class decoder;

	/*
	* Represents an encoder, which is just an implementation of codec_base,
	* plus these two methods for encoding: 
	* feed_frame() and encode_packet(),
	* and some additional for transcoding (i.e. encode frames decoded by a decoder with a different codec).
	* 
	* Read the comments for codec_base for how to encode.
	* 
	* Invariants: those of codec_base.
	*/
	class FF_WRAPPER_API encoder final : public codec_base
	{
	public:
		// Identification info must be provided.
		encoder() = delete;

		explicit inline encoder(const char* name)
			: codec_base(name) 
		{
			allocate_object_memory();
		}

		explicit inline encoder(AVCodecID ID)
			: codec_base(ID)
		{
			allocate_object_memory();
		}
		
		// not possible to copy the internal states of an encoder.
		encoder(const encoder&) = delete;
		encoder& operator=(const encoder&) = delete;

		inline encoder(encoder&& other) noexcept
			: codec_base(std::move(other)) {}

		inline encoder& operator=(encoder&& right) noexcept
		{
			codec_base::operator=(std::move(right));

			return *this;
		}

		/*
		* Destroys the encoder completely.
		*/
		~encoder() { destroy(); }

	public:
///////////////////////////// Encoding /////////////////////////////
		/*
		* Feeds a frame (food) into the encoder.
		* See the comments for codec_base for how to feed an encoder.
		*
		* @returns true if the frame has been fed successfully. false if
		* the encoder is full or you have already declared no more frame will be fed.
		* @throws std::logic_error if the encoder is not ready.
		* @throws std::invalid_argument if the frame is not ready or if the frame does not agree with
		* the properties of the encoder or with previous frames fed to it.
		*/
		bool feed_frame(const ff::frame& frame);

		/*
		* Encodes the next packets from the frames it has been fed.
		* See the comments for codec_base for when to decode frames.
		* 
		* Note: you must manually set the time base, dts, and pts of
		* the encoded packet if you are going to mux it.
		*
		* @returns a packet encoded; a DESTROYED packet if the encoder is hungry, or
		* if the encoder has nothing left in its stomach after you started draining it.
		* @throws std::logic_error if the encoder is not ready.
		*/
		ff::packet encode_packet();

	public:
///////////////////////////// Transcoding /////////////////////////////
		/*
		* A convenient helper for transcoding.
		* It queries if each essential property of the decoder is supported by the encoder.
		* If so, the decoder's option for the property is used here. 
		* Otherwise, the encoder automatically selects one from the supported options.
		* Essential properties (some are not used depending on the type):
		* type, format, width, height, frame_rate, sample_rate, channel_layout
		* 
		* @returns true if all the options checked are supported by the decoder; false if any of them 
		* is not supported and is changed to one of the supported.
		* @throws std::logic_error if dec is not ready or if this is not created.
		* @throws std::invalid_argument if dec and enc are of different types (e.g. dec is for video but this is for audio).
		* @throws std::domain_error if this does not support some option used by the decoder but for the property
		* it does not know which options are supported.
		*/
		bool set_properties_from_decoder(const decoder& dec);

	private:

		// Inherited via ff_object
		void internal_allocate_object_memory() override;

		// Inherited via codec_base
		void start_draining() override;

	};
}
