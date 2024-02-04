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

#include "../util/util.h"
#include "../util/ff_object.h"
#include "../util/dict.h"
#include "../codec/codec_properties.h"
#include "../data/frame.h"

extern "C"
{
#include <libavcodec/codec_id.h>
}

struct AVCodec;
struct AVCodecContext;

namespace ff
{
	class packet;

	/*
	* A decoder. Consists of two parts,
	* a description (which identifies the decoder) of the decoder and 
	* the context where an instance of the decoder lies in.
	* 
	* States from ff_object:
	* DESTROYED: both the description and the context are not created/are destroyed.
	* OBJECT_CREATED: only the description is created. The decoder can be identified now.
	* READY: both the description and the context are created. The decoder is ready for use now.
	* NOTE: DO NOT use the decoder again after a call to release_resources_memory(), which closes the context created.
	* Due the the design of FFmpeg, a decoder is not supported to be reused after its context is closed.
	* 
	* This class uses the void pointer parameter of allocate_resources_memory()
	* to pass in a dict of options. For type-safety, I provide a wrapper,
	* create_decoder_context(), which declares a dict& parameter as a safer forwarder.
	* You should use this method() instead and avoid calling allocate_resources_memory() directly.
	* 
	* HOW TO DECODE:
	*	1. When a decoder is made ready for the first time, it is hungry (hungry()).
	*	2. When a decoder is hungry, you must feed a packet to it by calling feed_packet() before you can decode. 
	*	3. After that, it is no longer hungry. You can try to decode a frame then by calling decode_frame(). 
	However, during the call, the decoder may realize that it still needs more packets, in which case
	it will fail to decode a frame and set hungry to true again. 
	*	4. You can choose to keep feeding it without decoding, and eventually it will not be able to eat more.
	Then the last feeding attempt will fail (return false) and it will set full to true and hungry to false.
	*	5. When a decoder is full, you must start decoding frames by calling decode_frame(), which will cancel being full.
	You can immediately try to feed it again after you have decoded one frame, but the attempt may discover
	that the decoder is still full and set full back. The FFmpeg documentation suggests that you decode until it is hungry again.
	*	6. After you have no packets to send, check if the decoder already knows that by calling no_more_packets().
	Sometimes it does not know, and then you should tell the decoder so by calling signal_no_more_packets(). 
	After that, repeatedly call decode_frame() for it to pop out what's left in its stomach 
	until that returns a DESTROYED frame (then it has no more in its stomach).
	*
	* Invariants:
	*	1. those of ff_object.
	*	2. when ready(), !(hungry && full); no_more_packets -> !hungry.
	* 2 is established by member initializers, and maintained by the four private methods
	* , signal_no_more_food(), as well as reset().
	*/
	class FF_WRAPPER_API decoder : public ff_object
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

	private:
		/*
		* According to the info passed through one of the constructors,
		* identify the encoder and create its description.
		*/
		void internal_allocate_object_memory() override;

		/*
		* Allocates the context for the decoder.
		* Note: I provide a wrapper, create_decoder_context(), for type-safety. Call that instead of
		* calling allocate_resources_memory().
		* 
		* @param size not used.
		* @param additional_information a pointer to AVDictionary*. 
		* If it is not of this type, then the behaviour is undefined.
		*/
		void internal_allocate_resources_memory(uint64_t size, void* additional_information) override;

		/*
		* Releases the description of the decoder.
		*/
		void internal_release_object_memory() noexcept override;

		/*
		* Releases the context of the decoder.
		*/
		void internal_release_resources_memory() noexcept override;

	public:
		/*
		* @returns the ID that uniquely identifies the decoder
		*/
		inline AVCodecID get_id() const noexcept { return codec_id; }
		/*
		* @returns the name that uniquely identifies the decoder
		*/
		inline const char* get_name() const noexcept { return codec_name; }

	public:
		/*
		* A wrapper for allocate_resources_memory() for type-safety.
		* @param options the options to be used to create the decoder. Cannot be empty.
		* Unused options will be stored back.
		*/
		void create_decoder_context(ff::dict& options);
		/*
		* A wrapper for allocate_resources_memory() for type-safety.
		* @param options the options to be used to create the decoder. Can be empty.
		*/
		void create_decoder_context(const ff::dict& options = dict());

		/*
		* @returns the properties of the decoder.
		* @throws std::logic_error if the decoder is not ready.
		*/
		codec_properties get_decoder_properties() const;

		/*
		* Sets the properties of the decoder.
		* You can only set the properties when the decoder is created yet not ready.
		* That is, before you create the context.
		* 
		* @param p the properties. It is highly recommended to mannually set only
		* the needed fields of codec_properties instead of using one directly obtained from
		* sources that seems to be compatiable (e.g. an encoder). That's because some options 
		* may not be compatible between encoders and decoders or between codecs and de/muxers.
		* @throws std::logic_error if not created.
		*/
		void set_codec_properties(const codec_properties& p);

		/*
		* See the comments for this class for what being hungry means.
		* To make it inline, I will not throw an exception if it's not ready, but you have to know
		* that the return value then doesn't mean anything.
		* Anyway, even if you don't care, I will check whenever you feed/use the decoder.
		* 
		* @returns if the decoder is hungry for packets before it can decode a frame.
		*/
		inline bool hungry() const noexcept { return is_hungry; }
		/*
		* See the comments for this class for what being full means.
		* To make it inline, I will not throw an exception if it's not ready, but you have to know
		* that the return value then doesn't mean anything.
		* Anyway, even if you don't care, I will check whenever you feed/use the decoder.
		* 
		* @returns if the decoder is full from packets and needs to decode some frames
		before it can eat again.
		*/
		inline bool full() const noexcept { return is_full; }

		inline bool no_more_packets() const noexcept { return no_more_food; }

		/*
		* Feeds a packet into the decoder.
		* See the comments for this class for how to feed a decoder.
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
		* See the comments for this class for when to decode frames.
		* 
		* @returns a frame decoded; a DESTROYED frame if the decoder is hungry, or
		* if the decoder has nothing left in its stomach after you started draining it.
		* @throws std::logic_error if the decoder is not ready.
		*/
		frame decode_frame();

		/*
		* Tell the decoder that no more packets will be fed to it.
		* 
		* @throws std::logic_error if it's not ready().
		* @throws std::logic_error if you have already signaled it.
		* You can only signal it once per decoding.
		*/
		void signal_no_more_packets();

		/*
		* After you believe a decoding process has completed or you no longer need it,
		* call this to reset the decoder so that it can start decoding packets
		* from a new stream with the same properties as the current decoder's codec_properties.
		* 
		* You can call it even if you have not signal_no_more_packets() --- if you
		* just want to discard the current progress.
		* 
		* After the call, hungry() = true, full() = false, and no_more_packets() = false.
		* 
		* @throws std::logic_error if it's not ready.
		*/
		void reset();

	private:
		// The identification info about the decoder that is given through constructors.
		AVCodecID codec_id = AVCodecID::AV_CODEC_ID_NONE; const char* codec_name = nullptr;

		// Description and information about this decoder.
		// This is probably preallocated in static storage inside FFmpeg,
		// so it doesn't need to be freed. Just reset the pointer.
		const AVCodec* p_codec_desc = nullptr;
		// Context where a decoder lies in.
		AVCodecContext* p_codec_ctx = nullptr;

		bool is_hungry = true;
		bool is_full = false;
		bool no_more_food = false;

	private:
		// I should only call them inside 
		// feed_packet(), decode_frame(), signal_no_more_packets(), and reset().

		inline void become_full() noexcept
		{
			is_full = true;
			is_hungry = false;
		}

		inline void become_hungry() noexcept
		{
			is_hungry = true;
			is_full = false;
		}

		/*
		* Called when the decoder is no longer hungry but not yet full.
		*/
		inline void cancel_hungry() noexcept
		{
			is_hungry = false;
		}

		inline void cancel_full() noexcept
		{
			is_full = false;
		}

		// Start draining the decoder.
		// See https://ffmpeg.org/doxygen/6.0/group__lavc__encdec.html
		// Should only call it inside signal_no_more_packets();
		void start_draining();
	};
	
}