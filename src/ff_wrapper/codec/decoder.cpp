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
#include "../formats/stream.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

ff::decoder::decoder(AVCodecID ID)
	: codec_base(ID)
{
	// Find the description.
	allocate_object_memory();
}

ff::decoder::decoder(const char* name)
	: codec_base(name)
{
	// Find the description.
	allocate_object_memory();
}

ff::decoder::decoder(const stream& s)
	: decoder(s.codec_id())
{
	set_codec_properties(s.properties());
	// Create the decoder context for it to be ready.
	create_codec_context();
}

ff::decoder::decoder(decoder&& other) noexcept
	: codec_base(std::move(other))
{
}

ff::decoder& ff::decoder::operator=(decoder&& right) noexcept
{
	ff::codec_base::operator=(std::move(right));

	return *this;
}

ff::decoder::~decoder() noexcept
{
	destroy();
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

	// Allocate the context.
	p_codec_ctx = avcodec_alloc_context3(p_codec_desc);
	if (nullptr == p_codec_ctx)
	{
		// descs are managed by FFmpeg and should be correct.
		// So errors can only be caused by memory allocation.
		throw std::bad_alloc();
	}
}

bool ff::decoder::feed_packet(const packet& pkt)
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}
	if (!pkt.ready())
	{
		throw std::invalid_argument("The packet is not ready.");
	}

	// Full can only be set here and cancelled in decode_frame().
	if (full())
	{
		return false;
	}

	// The user has signaled that no more packet is coming.
	if (no_more_food())
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
		// I detect this already with no_more_food() earlier.
		// Should never reach here.
		FF_ASSERT(false, "Did I forget to set signaled_no_more_food?");
		return false;
	case AVERROR(ENOMEM):
		throw std::bad_alloc();
		break;
	case AVERROR(EINVAL):
		throw std::invalid_argument("Perhaps a bug in my code. Also a possible defect frame from you.");
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

	// Use an AVFrame here is more convenient.
	AVFrame* pf = av_frame_alloc();
	if (nullptr == pf)
	{
		throw std::bad_alloc();
	}

	if (internal_decode_frame(pf))
	{
		// Success
		return frame(pf, is_video());
	}
	else
	{
		// Failure
		// Free the AVFrame
		av_frame_free(&pf);
		return ff::frame(false);
	}
}

bool ff::decoder::decode_frame(frame& f)
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}

	// Handle f so that it's always created before avcodec_receive_frame()
	switch (f.get_object_state())
	{
	case ff_object::DESTROYED:
		f.allocate_object_memory();
		[[fallthrough]];
	case ff_object::OBJECT_CREATED:
		// Do nothing.
		break;
	case ff_object::READY:
		// Release its previous data.
		f.release_resources_memory();
		break;
	}

	// Hungry can only be set here and cancelled in feed_packet()
	if (hungry())
	{
		return false;
	}

	bool ret = internal_decode_frame(f.av_frame());

	if (ret) // Success
	{
		// Don't forget to make f ready
		// and set its internal fields
		f.internal_find_num_planes();
		f.set_v_or_a(is_video());
		f.state = ff_object::READY;
	}

	return ret;

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

bool ff::decoder::internal_decode_frame(AVFrame* f)
{
	int ret = avcodec_receive_frame(p_codec_ctx, f);
	if (0 == ret) // Success
	{
		cancel_full();
		return true;
	}

	// Failure
	switch (ret)
	{
	case AVERROR(EAGAIN):
		// Needs more packets.
		FF_ASSERT(!signaled_no_more_food, "After draining has started, EAGAIN can never be returned.");

		become_hungry();
		return false;
		break;
	case AVERROR_EOF:
		// Called during draining.
		return false;
		break;
	case AVERROR(EINVAL):
		FF_ASSERT(false, "The decoder has not been set up correctly. This should not happen.");
		break;
	default:
		// Other decoding errors.
		throw std::runtime_error("The decoding failed but the decoder was set up correctly. "
			"Perhaps the packet you gave was invalid.");
	}

	return false;
}
