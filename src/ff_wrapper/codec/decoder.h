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

extern "C"
{
#include <libavcodec/codec_id.h>
}

struct AVCodec;
struct AVCodecContext;

namespace ff
{

	/*
	* A decoder. Consists of two parts,
	* a description (which identifies the decoder) of the decoder and 
	* the context where an instance of the decoder lies in.
	* 
	* States from ff_object:
	* DESTROYED: both the description and the context are not created/are destroyed.
	* OBJECT_CREATED: only the description is created. The decoder can be identified now.
	* READY: both the description and the context are created. The decoder is ready for use now.
	* 
	* This class uses the void pointer parameter of allocate_resources_memory()
	* to pass in a dict of options. For type-safety, I provide a wrapper,
	* create_decoder_context(), which declares a dict& parameter as a safer forwarder.
	* You should use this method() instead and avoid calling allocate_resources_memory() directly.
	*/
	class FF_WRAPPER_API decoder : public ff_object
	{
	public:
		// Constructors must provide identification info of the decoder.
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
		* Destroys the decoder, clears the description and everything else completely.
		*/
		~decoder() noexcept { destroy(); }

	private:
		/*
		* According to the info passed through one of the constructors,
		* identify the encoder and create its description.
		*/
		void internal_allocate_object_memory() override;

		/*
		* A wrapper for allocate_resources_memory() for type-safety.
		* @param options the options to be used to create the decoder. Cannot be empty.
		* Unused options will be stored back.
		*/
		void create_decoder_context(ff::dict& options)
		{
			if (options.empty())
			{
				throw std::invalid_argument("Dict cannot be empty.");
			}

			auto* pavd = options.get_av_dict();
			allocate_resources_memory(0, &pavd);
			options = pavd;
		}
		/*
		* A wrapper for allocate_resources_memory() for type-safety.
		* @param options the options to be used to create the decoder. Can be empty.
		*/
		inline void create_decoder_context(const ff::dict& options = dict())
		{
			AVDictionary** ppavd = nullptr;
			if (!options.empty())
			{
				ff::dict cpy(options);
				auto* pavd = cpy.get_av_dict();
				ppavd = &pavd;
			}

			allocate_resources_memory(0, ppavd);
		}
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

	private:
		// The identification info about the decoder that is given through constructors.
		AVCodecID codec_id = AVCodecID::AV_CODEC_ID_NONE; const char* codec_name = nullptr;

		// Description and information about this decoder.
		// This is probably preallocated in static storage inside FFmpeg,
		// so it doesn't need to be freed. Just reset the pointer.
		const AVCodec* p_codec_desc = nullptr;
		// Context where a decoder lies in.
		AVCodecContext* p_codec_ctx = nullptr;
	};
	
}