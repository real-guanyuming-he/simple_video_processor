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
#include "../util/ff_helpers.h"
#include "../util/channel_layout.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

ff::codec_base::codec_base(codec_base&& other) noexcept
	: ff_object(std::move(other)),
	codec_id(other.codec_id), codec_name(other.codec_name),
	p_codec_desc(other.p_codec_desc), p_codec_ctx(other.p_codec_ctx),
	is_full(other.is_full), is_hungry(other.is_hungry), signaled_no_more_food(other.signaled_no_more_food)
{
	other.p_codec_ctx = nullptr;
	other.p_codec_desc = nullptr;
}

ff::codec_base& ff::codec_base::operator=(codec_base&& right) noexcept
{
	ff_object::operator=(std::move(right));

	codec_id = right.codec_id;
	codec_name = right.codec_name;
	p_codec_desc = right.p_codec_desc;
	p_codec_ctx = right.p_codec_ctx;
	is_full = right.is_full;
	is_hungry = right.is_hungry;
	signaled_no_more_food = right.signaled_no_more_food;

	right.p_codec_ctx = nullptr;
	right.p_codec_desc = nullptr;

	return *this;
}

ff::codec_properties ff::codec_base::get_codec_properties() const
{
	if (!ready())
	{
		throw std::logic_error("Properties can only be obtained when the decoder is ready.");
	}

	return codec_properties(p_codec_ctx);
}

void ff::codec_base::set_codec_properties(const codec_properties& p)
{
	if (!created())
	{
		throw std::logic_error("Properties can only be set when the decoder is just created.");
	}

	avcodec_parameters_to_context(p_codec_ctx, p.av_codec_parameters());
}

void ff::codec_base::internal_allocate_resources_memory(uint64_t size, void* additional_information)
{
	auto ppavd = static_cast<::AVDictionary**>(additional_information);

	// Open the codec ctx in the context with the options.
	int ret = avcodec_open2(p_codec_ctx, p_codec_desc, ppavd);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL):
			throw std::runtime_error("Could not open a codec ctx: probably bad/unsupported options");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when opening a codec ctx: ", ret);
		}
	}
}

void ff::codec_base::internal_release_object_memory() noexcept
{
	// Completely destroy the context.
	ffhelpers::safely_free_codec_context(&p_codec_ctx);
	// Reset the desc.
	p_codec_desc = nullptr;
}

void ff::codec_base::internal_release_resources_memory() noexcept
{
	avcodec_close(p_codec_ctx);
}

bool ff::codec_base::is_v_pixel_format_supported(AVPixelFormat fmt) const
{
	if (!created())
	{
		throw std::logic_error("The codec is not created.");
	}
	if (p_codec_desc->type != AVMediaType::AVMEDIA_TYPE_VIDEO)
	{
		throw std::logic_error("The codec is not for videos.");
	}

	if (!p_codec_desc->pix_fmts) // unknown
	{
		throw std::domain_error("Don't know which pix fmts are supported.");
	}
	else
	{
		const AVPixelFormat* pf = p_codec_desc->pix_fmts;
		while (*pf != -1) // the array is -1-terminated.
		{
			if (fmt == *pf)
			{
				return true;
			}
			++pf;
		}
	}

	return false;
}

bool ff::codec_base::is_a_sample_format_supported(AVSampleFormat fmt) const
{
	if (!created())
	{
		throw std::logic_error("The codec is not created.");
	}
	if (p_codec_desc->type != AVMediaType::AVMEDIA_TYPE_AUDIO)
	{
		throw std::logic_error("The codec is not for audios.");
	}

	if (!p_codec_desc->sample_fmts) // unknown
	{
		throw std::domain_error("Don't know which sample fmts are supported.");
	}
	else
	{
		const AVSampleFormat* pf = p_codec_desc->sample_fmts;
		while (*pf != -1) // the array is -1-terminated.
		{
			if ((AVSampleFormat)fmt == *pf)
			{
				return true;
			}
			++pf;
		}
	}

	return false;
}

bool ff::codec_base::is_a_sample_rate_supported(int rate) const
{
	if (!created())
	{
		throw std::logic_error("The codec is not created.");
	}
	if (p_codec_desc->type != AVMediaType::AVMEDIA_TYPE_AUDIO)
	{
		throw std::logic_error("The codec is not for audios.");
	}

	if (!p_codec_desc->supported_samplerates) // unknown
	{
		throw std::domain_error("Don't know which sample rates are supported.");
	}
	else
	{
		const int* psr = p_codec_desc->supported_samplerates;
		while (*psr != 0) // terminated by 0
		{
			if (rate == *psr)
			{
				return true;
			}
			++psr;
		}
	}

	return false;
}

bool ff::codec_base::is_a_channel_layout_supported(const channel_layout& layout) const
{
	if (!created())
	{
		throw std::logic_error("The codec is not created.");
	}
	if (p_codec_desc->type != AVMediaType::AVMEDIA_TYPE_AUDIO)
	{
		throw std::logic_error("The codec is not for audios.");
	}

	if (!p_codec_desc->ch_layouts)
	{
		throw std::domain_error("Don't know which channel layouts are supported.");
	}
	else
	{
		const AVChannelLayout* pcl = p_codec_desc->ch_layouts;
		while (pcl->nb_channels != 0 || pcl->order != 0) // terminated by a zeroed layout
		{
			if (!(layout == *pcl))
			{
				return true;
			}
			++pcl;
		}
	}

	return false;
}

void ff::codec_base::signal_no_more_food()
{
	if (!ready())
	{
		throw std::logic_error("The codec is not ready.");
	}

	if (signaled_no_more_food)
	{
		// The user already signaled no more packets.
		throw std::logic_error("You can only signal no more packets once per decoding.");
	}

	// Set the states
	signaled_no_more_food = true;
	is_hungry = false;

	// Start draining the decoder.
	start_draining();
}

void ff::codec_base::reset()
{
	if (!ready())
	{
		throw std::logic_error("The decoder is not ready.");
	}

	// Reset the states.
	become_hungry();
	signaled_no_more_food = false;

	avcodec_flush_buffers(p_codec_ctx);
}

void ff::codec_base::create_codec_context(ff::dict& options)
{
	if (options.empty())
	{
		throw std::invalid_argument("Dict cannot be empty.");
	}

	auto* pavd = options.get_av_dict();
	allocate_resources_memory(0, &pavd);
	options = pavd;
}

void ff::codec_base::create_codec_context(const ff::dict& options)
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
