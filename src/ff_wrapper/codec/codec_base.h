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
#include "../util/dict.h"
#include "../util/ff_object.h"
#include "../codec/codec_properties.h"

extern "C"
{
#include <libavcodec/codec_id.h>
}

#include <vector>

struct AVCodec;
struct AVCodecContext;

namespace ff
{
	class channel_layout;

	/*
	* Encapsulates the basic functions of a FFmpeg codec.
	* Derived classes can only be either a decoder or an encoder.
	* 
	* Consists of two parts,
	* a description (which identifies the codec) of the codec and 
	* the context where an instance of the codec lies in.
	* 
	* States from ff_object:
	* DESTROYED: both the description and the context are not created/are destroyed.
	* OBJECT_CREATED: only the description is created. The codec can be identified now.
	* READY: both the description and the context are created. The codec is ready for use now.
	* NOTE: DO NOT use the codec again after a call to release_resources_memory(), which closes the context created.
	* Due the the design of FFmpeg, a codec is not supported to be reused after its context is closed.
	* 
	* This class uses the void pointer parameter of allocate_resources_memory()
	* to pass in a dict of options. For type-safety, I provide a wrapper,
	* create_codec_context(), which declares a dict& parameter as a safer forwarder.
	* You should use this method() instead and avoid calling allocate_resources_memory() directly.
	* 
	* how to use i.e. decode/encode:
	* Terminologies:
	* food = packet for decoder / frame for encoder.
	* output = frame for decoder / packet for encoder.
	* The following process is described for decoding. Encoding is exactly the same 
	* except that exchanges are made: packet/frame, feed_packet()/feed_frame(), decode_frame()/encode_packet()
	*	1. When a codec is made ready for the first time, it is hungry (hungry()).
	*	2. When a codec is hungry, you must feed a packet to it by calling feed_packet() before you can decode. 
	*	3. After that, it is no longer hungry. You can try to decode a frame then by calling decode_frame(). 
	However, during the call, the codec may realize that it still needs more packets, in which case
	it will fail to decode a frame and set hungry to true again. 
	*	4. You can choose to keep feeding it without decoding, and eventually it will not be able to eat more.
	Then the last feeding attempt will fail (return false) and it will set full to true and hungry to false.
	*	5. When a codec is full, you must start decoding frames by calling decode_frame(), which will cancel being full.
	You can immediately try to feed it again after you have decoded one frame, but the attempt may discover
	that the codec is still full and set full back. The FFmpeg documentation suggests that you decode until it is hungry again.
	*	6. After you have no packets to send, you should tell the codec so by calling signal_no_more_food(). 
	After that, repeatedly call decode_frame() for it to pop out what's left in its stomach 
	until that returns a DESTROYED frame (then it has no more in its stomach).
	*
	* Invariants:
	*	1. those of ff_object.
	*	2. when ready(), !(hungry && full); no_more_food -> !hungry.
	* 2 is established by member initializers, and maintained by the four private methods
	* , signal_no_more_food(), as well as reset().
	*/
	class FF_WRAPPER_API codec_base : public ff_object
	{
	public:
		// Constructors must provide identification info for the codec_base.
		codec_base() = delete;

		/*
		* Only sets the ID field.
		* A derived class should identify the codec_base by ID and find the description of the codec_base.
		* @throws std::invalid_argument if the no codec_base of the ID can be found.
		*/
		explicit codec_base(AVCodecID ID)
			: codec_id(ID) {}
		/*
		* Only sets the ID field.
		* A derived class should identify the codec_base by name and find the description of the codec_base.
		* @throws std::invalid_argument if the no codec_base of the name can be found.
		*/
		explicit codec_base(const char* name)
			: codec_name(name) {}

		// Even if the properties can be copied,
		// the internal state and buffers of a codec_base cannot.
		codec_base(const codec_base&) = delete;

		/*
		* Takes over other and set all other's pointers to nullptr.
		*/
		codec_base(codec_base&& other) noexcept;

		/*
		* Destroys itself, takes over other, and set all other's pointers to nullptr.
		*/
		codec_base& operator=(codec_base&& right) noexcept;

		/*
		* Destroy should be called by the derived class.
		*/
		~codec_base() noexcept {}

	public:
		/*
		* @returns the ID that uniquely identifies the codec
		*/
		inline AVCodecID get_id() const noexcept { return codec_id; }
		/*
		* @returns the name that uniquely identifies the codec
		*/
		inline const char* get_name() const noexcept { return codec_name; }

	public:
		/*
		* See the comments for this class for what being hungry means.
		* To make it inline, I will not throw an exception if it's not ready, but you have to know
		* that the return value then doesn't mean anything.
		* Anyway, even if you don't care, I will check whenever you feed/use the codec.
		*
		* @returns if the codec is hungry for food before it can produce an output.
		*/
		inline bool hungry() const noexcept { return is_hungry; }
		/*
		* See the comments for this class for what being full means.
		* To make it inline, I will not throw an exception if it's not ready, but you have to know
		* that the return value then doesn't mean anything.
		* Anyway, even if you don't care, I will check whenever you feed/use the codec.
		*
		* @returns if the codec is full from food and needs to output
		before it can eat again.
		*/
		inline bool full() const noexcept { return is_full; }

		/*
		* @returns if the user has signaled that no more food will be given for the codec.
		*/
		inline bool no_more_food() const noexcept { return signaled_no_more_food; }

	protected:
/////////////////////////////// Derived from ff_object ///////////////////////////////
		/*
		* Opens the codec ctx based on the description found through allocate_object_memory()
		the properties set through 
		and the options passed in as a pointer of a dictionary through the parameter additional_information
		*/
		void internal_allocate_resources_memory(uint64_t size, void* additional_information) override;

		/*
		* Releases the description of the codec.
		*/
		void internal_release_object_memory() noexcept override;

		/*
		* Releases the context of the codec.
		*/
		void internal_release_resources_memory() noexcept override;

	public:
/////////////////////////////// codec property methods ///////////////////////////////
		/*
		* @returns if the video pixel format is supported by this codec
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for videos.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		bool is_v_pixel_format_supported(AVPixelFormat fmt) const;
		/*
		* @returns if the video frame rate is supported by this codec
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for videos.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		bool is_v_frame_rate_supported(ff::rational fr) const;
		/*
		* @returns if the audio sample format is supported by this codec
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		bool is_a_sample_format_supported(AVSampleFormat fmt) const;
		/*
		* @returns if the audio sample rate is supported by this codec
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		bool is_a_sample_rate_supported(int rate) const;

		/*
		* Exactly the same as is_a_channel_layout_supported(layout.av_ch_layout());
		*/
		inline bool is_a_channel_layout_supported(const channel_layout& layout) const
		{
			return is_a_channel_layout_supported(layout.av_ch_layout());
		}
		/*
		* @returns if the audio channel layout is supported by this codec
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		bool is_a_channel_layout_supported(const AVChannelLayout& layout) const;

		/*
		* @returns the list of all supported video pixel formats
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for videos.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		std::vector<AVPixelFormat> supported_v_pixel_formats() const;
		/*
		* @returns the list of all supported video frame rates
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for videos.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		std::vector<ff::rational> supported_v_frame_rates() const;
		/*
		* @returns the list of all supported audio sample formats
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		std::vector<AVSampleFormat> supported_a_sample_formats() const;
		/*
		* @returns the list of all supported audio sample rates.
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		std::vector<int> supported_a_sample_rates() const;
		/*
		* @returns the list of (const pointers to) all supported audio channel layouts.
		Because these layouts are stored permanently somewhere in the codec desc,
		I can simply store references (ptrs instead as refs may not have size) to them.
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		std::vector<const AVChannelLayout*> supported_a_channel_layouts() const;

		/*
		* @returns the first supported pixel format (usually the best).
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for videos.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		AVPixelFormat first_supported_v_pixel_format() const;
		/*
		* @returns the first supported video frame rate (usually the best).
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for videos.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		ff::rational first_supported_v_frame_rate() const;
		/*
		* @returns the first supported audio sample format (usually the best). 
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		AVSampleFormat first_supported_a_sample_format() const;
		/*
		* @returns the first supported audio sample rate (usually the best).
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		int first_supported_a_sample_rate() const;
		/*
		* @returns (a const reference to) the first supported audio channel layout (usually the best).
		* @throws std::logic_error if destroyed().
		* @throws std::logic_error if the codec is not for audios.
		* @throws std::domain_error if the codec does not know the supported values.
		*/
		const AVChannelLayout& first_supported_a_channel_layout() const;

		/*
		* @returns the properties of the codec, which describes how decoding/encoding is done.
		* @throws std::logic_error if the codec is destroyed.
		* Note: if you get it when the codec is not ready, more properties may be set after 
		* this. Call it when the codec is ready to get all the properties.
		*/
		codec_properties get_codec_properties() const;

		/*
		* Sets the properties that describes how decoding/encoding is done to create the context.
		* You can only set the properties when the codec is created yet not ready.
		* That is, before you create the context.
		*
		* @param p the properties. It is highly recommended to mannually set only
		the needed fields of codec_properties instead of using one directly obtained from
		sources that seems to be compatiable (use *ALL* the properties of a decoder for an encoder).
		That's because some options may not be compatible between encoders and decoders or between codecs and de/muxers.
		* @throws std::logic_error if not created.
		* @throws std::invalid_argument if p's type is not the same as the codec's
		*/
		void set_codec_properties(const codec_properties& p);

/////////////////////////////// decode/encode methods ///////////////////////////////
		/*
		* Tell the codec that no more food will be fed to it.
		*
		* @throws std::logic_error if it's not ready().
		* @throws std::logic_error if you have already signaled it.
		* You can only signal it once per decoding/encoding.
		*/
		virtual void signal_no_more_food();

		/*
		* After you believe a decoding/encoding process has completed or you no longer need it,
		* call this to reset the codec so that it can start decoding/encoding food
		* from/for a new stream with the same properties as the current codec_properties.
		*
		* You can call it even if you have not called signal_no_more_food() --- if you
		* just want to discard the current progress.
		*
		* After the call, hungry() = true, full() = false, and no_more_food() = false.
		*
		* @throws std::logic_error if it's not ready.
		*/
		virtual void reset();

		/*
		* A wrapper for allocate_resources_memory() for type-safety.
		* @param options the options to be used to create the codec. Cannot be empty.
		* Unused options will be stored back.
		*/
		void create_codec_context(ff::dict& options);
		/*
		* A wrapper for allocate_resources_memory() for type-safety.
		* @param options the options to be used to create the codec. Can be empty.
		*/
		void create_codec_context(const ff::dict& options = dict());

	public:
		AVCodecContext* av_codec_ctx() noexcept { return p_codec_ctx; }
		const AVCodecContext* av_codec_ctx() const noexcept { return p_codec_ctx; }

	protected:
		// The identification info about the codec that is given through constructors.
		AVCodecID codec_id = AVCodecID::AV_CODEC_ID_NONE; const char* codec_name = nullptr;

		// Description and information about this codec.
		// This is probably preallocated in static storage inside FFmpeg,
		// so it doesn't need to be freed. Just reset the pointer.
		const AVCodec* p_codec_desc = nullptr;
		// Context where a codec lies in.
		AVCodecContext* p_codec_ctx = nullptr;

		bool is_hungry = true;
		bool is_full = false;
		bool signaled_no_more_food = false;

	protected:
		// I should only call them inside 
		// feed_packet(), decode_frame(), signal_no_more_food(), and reset().

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
		* Called when the codec is no longer hungry but not yet full.
		*/
		inline void cancel_hungry() noexcept
		{
			is_hungry = false;
		}

		inline void cancel_full() noexcept
		{
			is_full = false;
		}

		// Start draining the codec.
		// See https://ffmpeg.org/doxygen/6.0/group__lavc__encdec.html
		// Should only call it inside signal_no_more_food();
		virtual void start_draining() = 0;
	};
}