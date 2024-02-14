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
	class muxer;

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

		/*
		* When you know a name to identify the encoder.
		* 
		* @throws std::invalid_argument if no encoder matches the name.
		*/
		explicit inline encoder(const char* name)
			: codec_base(name) 
		{
			allocate_object_memory();
		}

		/*
		* When you know the ID to identify the encoder.
		* 
		* @throws std::invalid_argument if no encoder matches the ID.
		*/
		explicit inline encoder(AVCodecID ID)
			: codec_base(ID)
		{
			allocate_object_memory();
		}

		/*
		* When you have an output file and its muxer
		* and you want to know which encoder the muxer wants you to use
		* for a specific type of stream.
		* 
		* @param muxer the muxer
		* @param type the type of the stream.
		* @throws std::domain_error if the encoder for the type could not be found.
		*/
		encoder(const muxer& muxer, AVMediaType type);
		
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
		* To reuse a packet, call the version that accepts a packet& parameter.
		* 
		* Note: you must manually set (at least check) some properties (e.g. time fields) of
		* the encoded packet if you are going to mux it.
		* The encoder doesn't promise that all the properties are set up correctly.
		* I only set its time_base to the encoder's.
		*
		* @returns a packet encoded; a DESTROYED packet if the encoder is hungry, or
		* if the encoder has nothing left in its stomach after you started draining it.
		* @throws std::logic_error if the encoder is not ready.
		*/
		ff::packet encode_packet();

		/*
		* Encodes the next packets from the frames it has been fed.
		* See the comments for codec_base for when to decode frames.
		* This version allows you to reuse a packet allocated only once for
		* a whole encoding loop.
		*
		* Note: you must manually set (at least check) some properties (e.g. time fields) of
		* the encoded packet if you are going to mux it.
		* The encoder doesn't promise that all the properties are set up correctly.
		* This is especially important if you use this version to reuse a packet,
		* as the encoder doesn't promise that it will reset the properties during each call.
		* I only set its time_base to the encoder's.
		*
		* @param pkt where the encoded data will be stored inside.
		*	If pkt is destroyed, then pkt will be ready with the encoded data.
		*	If pkt is created, then pkt will be ready with the encoded data.
		*	If pkt is ready, then its previous data will be released and
		* the encoded data will be given to it.
		*	If the encoding fails (i.e. it returns false), then pkt will be created with no data.
		* @returns true if a packet has been encoded; false if the encoder is hungry, or
		* if the encoder has nothing left in its stomach after you started draining it.
		* @throws std::logic_error if the encoder is not ready.
		*/
		bool encode_packet(packet& pkt);

	public:
///////////////////////////// Transcoding /////////////////////////////
		/*
		* A convenient helper for transcoding.
		* It queries if each essential property of the decoder is supported by the encoder.
		* If so, the decoder's option for the property is used here. 
		* Otherwise, the encoder automatically selects one from the supported options.
		* The definition of essential properties are given in the comment for
		* codec_properties::essential_properties().
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

		/*
		* Common piece of code of the encode_packet() methods.
		*
		* @returns true iff a packet has been encoded.
		*/
		bool internal_encode_packet(AVPacket* pkt);
	};
}
