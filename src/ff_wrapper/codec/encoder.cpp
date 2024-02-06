#include "encoder.h"
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

#include "../data/frame.h"
#include "../util/ff_helpers.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

bool ff::encoder::feed_frame(const ff::frame& frame)
{
	if (!ready())
	{
		throw std::logic_error("The encoder is not ready.");
	}
	if (!frame.ready())
	{
		throw std::invalid_argument("The frame is not ready.");
	}

	// Full can only be set here and cancelled in decode_frame().
	if (full())
	{
		return false;
	}

	// The user has signaled that no more frame is coming.
	if (no_more_food())
	{
		return false;
	}

	int ret = avcodec_send_frame(p_codec_ctx, frame.av_frame());
	if (0 == ret) // Success
	{
		cancel_hungry();
		return true;
	}

	// Failure
	switch (ret)
	{
	case AVERROR(EAGAIN):
		// The encoder is full.
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
		FF_ASSERT(false, "The encoder has not been set up correctly. This should not happen.");
		break;
	default:
		// Other encoding errors.
		throw std::invalid_argument("The encoding failed but the encoder was set up correctly. "
			"Perhaps the frame you gave was invalid.");
	}

	return false;
}

ff::packet ff::encoder::encode_packet()
{
	if (!ready())
	{
		throw std::logic_error("The encoder is not ready.");
	}

	// Hungry can only be set here and cancelled in feed_packet()
	if (hungry())
	{
		return ff::packet(false);
	}

	AVPacket* pkt = av_packet_alloc();
	if (nullptr == pkt)
	{
		throw std::bad_alloc();
	}
	int ret = avcodec_receive_packet(p_codec_ctx, pkt);
	if (0 == ret) // Success
	{
		cancel_full();
		return ff::packet(pkt);
	}

	// Failure
	// First free the AVPacket
	av_packet_free(&pkt);
	switch (ret)
	{
	case AVERROR(EAGAIN):
		// Needs more frames.
		FF_ASSERT(!signaled_no_more_food, "After draining has started, EAGAIN can never be returned.");

		become_hungry();
		return ff::packet(false);
		break;
	case AVERROR_EOF:
		// Called during draining.
		return ff::packet(false);
		break;
	case AVERROR(EINVAL):
		FF_ASSERT(false, "The encoder has not been set up correctly. This should not happen.");
		break;
	default:
		// Other decoding errors.
		throw std::runtime_error("The encoding failed but the encoder was set up correctly. "
			"Perhaps the frame you gave was invalid.");
	}

	return ff::packet(false);
}

void ff::encoder::internal_allocate_object_memory()
{
	// A constructor either identifies the encoder by ID or by name.
	// When the ID is available, use it. Otherwise, use the name.

	if (codec_id != AVCodecID::AV_CODEC_ID_NONE)
	{
		// Use the ID
		p_codec_desc = avcodec_find_encoder(codec_id);
	}
	else if (codec_name != nullptr)
	{
		// Use the name
		p_codec_desc = avcodec_find_encoder_by_name(codec_name);
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

		throw std::invalid_argument("No encoder matches the identification info you provided.");
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

void ff::encoder::start_draining()
{
	FF_ASSERT(ready(), "Should not call it when not ready()");

	int ret = avcodec_send_frame(p_codec_ctx, nullptr);
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
