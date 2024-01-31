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

#include "decoder.h"
#include "../util/ff_helpers.h"
#include "../util/dict.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

ff::decoder::decoder(AVCodecID ID)
	: codec_id(ID)
{
	// Find the description.
	allocate_object_memory();
}

ff::decoder::decoder(const char* name)
	: codec_name(name)
{
	// Find the description.
	allocate_object_memory();
}

ff::decoder::~decoder() noexcept
{
	destroy();
}

void ff::decoder::create_decoder_context(ff::dict& options)
{
	if (options.empty())
	{
		throw std::invalid_argument("Dict cannot be empty.");
	}

	auto* pavd = options.get_av_dict();
	allocate_resources_memory(0, &pavd);
	options = pavd;
}

void ff::decoder::create_decoder_context(const ff::dict& options = dict())
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

void ff::decoder::internal_allocate_object_memory()
{
	// A constructor either identifies the decoder by ID or by name.
	// When the ID is available, use it. Otherwise, use the name.

	if (codec_id != AVCodecID::AV_CODEC_ID_NONE)
	{
		// Use the ID
		p_codec_desc = avcodec_find_decoder(codec_id);
	}
	else if(codec_name != nullptr)
	{
		// Use the name
		p_codec_desc = avcodec_find_decoder_by_name(codec_name);
	}
	else // Neither is available. This should not happen.
	{
		throw std::runtime_error("Unexpected error happened when trying to find a decoder: "
			"both the ID and the name are not available.");
	}
	
	// If could not identify the decoder given the information
	if (nullptr == p_codec_desc)
	{
		// I viewed the source code of FFmpeg and ascertained that 
		// avcodec_find_en/decoder...() will not allocate memory, so
		// when it returns nullptr, it can only mean no codec could be found.

		throw std::runtime_error("Unexpected error happened when trying to find a decoder: "
			"could not identify a decoder with the information.");
	}

	// If succeeded, then fill in missing identification information.
	if (codec_id == AVCodecID::AV_CODEC_ID_NONE)
	{
		codec_id = p_codec_desc->id;
	}
	if (nullptr == codec_name)
	{
		codec_name = p_codec_desc->name;
	}
}

void ff::decoder::internal_allocate_resources_memory(uint64_t size, void *additional_information)
{
	auto ppavd = static_cast<::AVDictionary**>(additional_information);

	// Allocate the context.
	p_codec_ctx = avcodec_alloc_context3(p_codec_desc);
	if (nullptr == p_codec_ctx)
	{
		// descs are managed by FFmpeg and should be correct.
		// So errors can only be caused by memory allocation.
		throw std::bad_alloc();
	}

	// Open the decoder in the context with the options.
	int ret = avcodec_open2(p_codec_ctx, p_codec_desc, ppavd);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL):
			throw std::runtime_error("Could not open a decoder: probably bad/unsupported options");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when opening a decoder: ", ret);
		}
	}
}

void ff::decoder::internal_release_object_memory() noexcept
{
	// Just reset the desc.
	p_codec_desc = nullptr;
}

void ff::decoder::internal_release_resources_memory() noexcept
{
	ffhelpers::safely_free_codec_context(&p_codec_ctx);
}

ff::codec_properties ff::decoder::get_decoder_properties() const
{
	if (!ready())
	{
		throw std::logic_error("Properties can only be obtained when the decoder is ready.");
	}

	return codec_properties(p_codec_ctx);
}
