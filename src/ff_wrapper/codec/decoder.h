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

#include "../util/ff_object.h"

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
	* create_decoder_context(), which declares a const dict& parameter as a safer forwarder.
	* You should use this method() instead and avoid calling allocate_resources_memory() directly.
	*/
	class decoder : public ff_object
	{
	public:
		decoder();
		~decoder();

	private:
		/*
		* According to the info passed through one of the constructors,
		* identify the encoder and create its description.
		*/
		void internal_allocate_object_memory() override;

		/*
		* Allocates the context for the decoder.
		* 
		* @param size not used.
		* @param additional_information a pointer to ff::dict. 
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
		// Description and information about this decoder.
		const AVCodec* p_codec_desc;
		// Context where the decoder lies in.
		const AVCodec* p_codec_ctx;
	};
	
}