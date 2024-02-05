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
#pragma once

#include "../util/util.h"
#include "../util/ff_math.h"
#include "../util/channel_layout.h"

extern "C"
{
#include <libavcodec/codec_par.h>
}

class AVCodecContext;

namespace ff
{
	/*
	* Describes the properties of an ENCODED stream.
	* That is, the struct describes 
	* 1. how an encoder may encode frames to form such a stream and,
	* 2. how a decoder may decode frames from such a stream.
	* 
	* It encapsulates an AVCodecParameters. The only invariant is
	* that the pointer != nullptr. For convenience, I allow modification
	* of its fields. If you want an immutable one, define a const instance of the class.
	*/
	class FF_WRAPPER_API codec_properties
	{
	public:
		/*
		* Allocates the properties and sets all of their values to default.
		*/
		codec_properties();
		/*
		* Deallocates the properties.
		*/
		~codec_properties();

		/*
		* Inits from a pointer of the internal AVCodecParameters.
		* 
		* @param p a pointer of the internal AVCodecParameters
		* @param take_over if true then I will simply copy the pointer, which means the life
		* of the object is in this class's hand. If false I will make a copy of the parameters.
		*/
		explicit codec_properties(::AVCodecParameters* p, bool take_over = false);

		/*
		* Copies properties from an existing decoder/encoder
		*/
		explicit codec_properties(const ::AVCodecContext* codec_ctx);

		/*
		* Allocates a space and copies all properties from other to here.
		*/
		codec_properties(const codec_properties& other);
		/*
		* Takes other's pointer and set other's to nullptr
		*/
		inline codec_properties(codec_properties&& other) noexcept
			: p_params(other.p_params)
		{
			other.p_params = nullptr;
		}

	public:
		inline ::AVCodecParameters* av_codec_parameters() noexcept { return p_params; }
		inline const ::AVCodecParameters* av_codec_parameters() const noexcept { return p_params; }

	public:
		inline AVMediaType	type()				const noexcept { return p_params->codec_type; }
		inline bool			is_video()			const noexcept { return AVMEDIA_TYPE_VIDEO == p_params->codec_type; }
		inline bool			is_audio()			const noexcept { return AVMEDIA_TYPE_AUDIO == p_params->codec_type; }
		inline bool			is_subtitle()		const noexcept { return AVMEDIA_TYPE_SUBTITLE == p_params->codec_type; }

		inline AVPixelFormat	v_pixel_format()	const noexcept { return (AVPixelFormat)p_params->format; }
		inline int				v_width()			const noexcept { return p_params->width; }
		inline int				v_height()			const noexcept { return p_params->height; }
		inline AVFieldOrder		v_field_order()		const noexcept { return p_params->field_order; }
		inline AVColorRange		v_color_range()		const noexcept { return p_params->color_range; }
		inline AVColorSpace		v_color_space()		const noexcept { return p_params->color_space; }
		inline AVColorPrimaries	v_color_primaries()	const noexcept { return p_params->color_primaries; }
		inline AVChromaLocation	v_chroma_location()	const noexcept { return p_params->chroma_location; }

		inline AVSampleFormat			a_sample_format()	const noexcept { return (AVSampleFormat)p_params->format; }
		inline int						a_sample_rate()		const noexcept { return p_params->sample_rate; }
		inline const AVChannelLayout&	a_channel_layout_ref()	const noexcept { return p_params->ch_layout; }
		inline channel_layout			a_channel_layout()		const noexcept { return ff::channel_layout(p_params->ch_layout); }

		/*
		* @returns The sample aspect ratio (w/h). Or -1/1 if unknown.
		*/
		inline ff::rational v_sar() const noexcept
		{
			if (p_params->sample_aspect_ratio.den != 0)
			{
				return ff::rational(p_params->sample_aspect_ratio);
			}
			// Unknown.
			return ff::rational(-1, 1);
		}

	public:
		inline void set_type(AVMediaType type)	noexcept { p_params->codec_type = type; }
		inline void set_type_video()			noexcept { p_params->codec_type = AVMEDIA_TYPE_VIDEO; }
		inline void	set_type_audio()			noexcept { p_params->codec_type = AVMEDIA_TYPE_AUDIO; }
		inline void	set_type_subtitle()			noexcept { p_params->codec_type = AVMEDIA_TYPE_SUBTITLE; }

		inline void set_v_pixel_format(AVPixelFormat f)			noexcept { p_params->format = f; }
		inline void set_v_width(int w)							noexcept { p_params->width = w; }
		inline void set_v_height(int h)							noexcept { p_params->height = h; }
		inline void set_v_field_order(AVFieldOrder fo)			noexcept { p_params->field_order = fo; }
		inline void set_v_color_range(AVColorRange cr)			noexcept { p_params->color_range = cr; }
		inline void set_v_color_space(AVColorSpace cs)			noexcept { p_params->color_space = cs; }
		inline void set_v_color_primaries(AVColorPrimaries cp)	noexcept { p_params->color_primaries = cp; }
		inline void set_v_chroma_location(AVChromaLocation cl)	noexcept { p_params->chroma_location = cl; }
		inline void set_v_aspect_ratio(ff::rational ar)			noexcept { p_params->sample_aspect_ratio = ar.av_rational(); }

		inline void set_a_sample_format(AVSampleFormat f)	noexcept { p_params->format = f; }
		inline void set_a_sample_rate(int sr)				noexcept { p_params->sample_rate = sr; }
		
		/*
		* Frees current ch layout and copies ch to it.
		* @param ch the new channel layout.
		*/
		void set_a_channel_layout(const channel_layout& ch);

	private:
		class ::AVCodecParameters* p_params;
	};

}