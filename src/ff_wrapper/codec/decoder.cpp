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
#include "../data/packet.h"

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

ff::decoder::decoder(decoder&& other) noexcept
	: ff_object(std::move(other)),
	codec_id(other.codec_id), codec_name(other.codec_name),
	p_codec_desc(other.p_codec_desc), p_codec_ctx(other.p_codec_ctx),
	is_full(other.is_full), is_hungry(other.is_hungry), no_more_food(other.no_more_food)
{
	other.p_codec_ctx = nullptr;
	other.p_codec_desc = nullptr;
}

ff::decoder& ff::decoder::operator=(decoder&& right) noexcept
{
	ff_object::operator=(std::move(right));

	codec_id = right.codec_id;
	codec_name = right.codec_name;
	p_codec_desc = right.p_codec_desc;
	p_codec_ctx = right.p_codec_ctx;
	is_full = right.is_full;
	is_hungry = right.is_hungry;
	no_more_food = right.no_more_food;

	right.p_codec_ctx = nullptr;
	right.p_codec_desc = nullptr;

	return *this;
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

void ff::decoder::create_decoder_context(const ff::dict& options)
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
		throw std::invalid_argument("You did not provide valid identification info.");
	}
	
	// If could not identify the decoder given the information
	if (nullptr == p_codec_desc)
	{
		// I viewed the source code of FFmpeg and ascertained that 
		// avcodec_find_en/decoder...() will not allocate memory, so
		// when it returns nullptr, it can only mean no codec could be found.

		throw std::invalid_argument("No decoder matches the identification info you provided.");
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

void ff::decoder::set_codec_properties(const codec_properties& p)
{
	if (!ready())
	{
		throw std::logic_error("Properties can only be set when the decoder is ready.");
	}

	avcodec_parameters_to_context(p_codec_ctx, p.av_codec_parameters());
}

bool ff::decoder::feed_packet(const packet& pkt)
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}

	// Full can only be set here and cancelled in decode_frame().
	if (full())
	{
		return false;
	}

	// The user has signaled that no more packet is coming.
	if (no_more_packets())
	{
		return false;
	}

	int ret = avcodec_send_packet(p_codec_ctx, pkt.av_packet());
	if (0 == ret) // Success
	{
		cancel_hungry();
		return true;
	}

	// Failure
	switch (ret)
	{
	case AVERROR(EAGAIN):
		// The decoder is full.
		become_full();
		return false;
	case AVERROR_EOF:
		// I detect this already with no_more_packets() earlier.
		// Should never reach here.
		FF_ASSERT(false, "Did I forget to set no_more_food?");
		return false;
	case AVERROR(ENOMEM):
		throw std::bad_alloc();
		break;
	case AVERROR(EINVAL):
		throw std::runtime_error("The decoder has not been set up correctly. This should not happen.");
		break;
	default:
		// Other decoding errors.
		throw std::invalid_argument("The decoding failed but the decoder was set up correctly. "
			"Perhaps the packet you gave was invalid.");
	}

	return false;
}

ff::frame ff::decoder::decode_frame()
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}

	// Hungry can only be set here and cancelled in feed_packet()
	if (hungry())
	{
		return ff::frame(false);
	}

	ff::frame f(true);
	int ret = avcodec_receive_frame(p_codec_ctx, f.av_frame());
	if (0 == ret) // Success
	{
		cancel_full();
		return f;
	}

	// Failure
	switch (ret)
	{
	case AVERROR(EAGAIN):
		// Needs more packets.
		FF_ASSERT(!no_more_food, "After draining has started, EAGAIN can never be returned.");

		become_hungry();
		return ff::frame(false);
	case AVERROR_EOF:
		// The draining has completed.
		return ff::frame(false);
	case AVERROR(EINVAL):
		throw std::runtime_error("The decoder has not been set up correctly. This should not happen.");
		break;
	default:
		// Other decoding errors.
		throw std::runtime_error("The decoding failed but the decoder was set up correctly. "
			"Perhaps the packet you gave was invalid.");
	}
	
	return ff::frame(false);
}

void ff::decoder::signal_no_more_packets()
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}

	if (no_more_food)
	{
		// The user already signaled no more packets.
		throw std::logic_error("You can only signal no more packets once per decoding.");
	}

	// Set the states
	no_more_food = true;
	is_hungry = false;

	// Start draining the decoder.
	start_draining();
}

void ff::decoder::reset()
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}

	// Set the states
	cancel_hungry();
	no_more_food = true;

	// Reset the decoder.
	avcodec_flush_buffers(p_codec_ctx);
}

void ff::decoder::start_draining()
{
	FF_ASSERT(ready(), "Should not call it when not ready()");

	int ret = avcodec_send_packet(p_codec_ctx, nullptr);
	if (ret < 0)
	{
		// Failure. Should never happen.
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR_EOF:
			// Draining already started.
			FF_ASSERT(false, "Did I drain it more than once?");
			break;
		case AVERROR(EINVAL):
			// The decoder has not been correctly set up.
			FF_ASSERT(false, "Did I change the decoder or write a typo?");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when I tried to start draining.", ret);
		}
	}
}
