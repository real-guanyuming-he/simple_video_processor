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

#include "util.h"

extern "C"
{
#include <libavutil/channel_layout.h>
}

namespace ff
{
	/*
	* 
	* IT CAN BE RATHER INCONVENIENT TO USE SOMETIMES.
	* CHOOSE TO USE IT OR NOT YOURSELF DEPENDING ON THE SITUATION.
	* To give an example of the inconvenience,
	* FFmpeg internal structs like AVCodecParameters store an object of AVChannelLayout directly inside.
	* To make an object of my wrapper out of one these AVChannelLayout objects,
	* I must make a copy, because I cannot directly reinterpret_cast a ptr/ref to the object.
	* Accessing such a ptr/ref would be an undefined behaviour, 
	as this wrapper is not similar to AVChannelLayout and the access is not one of 
	* [basic.lval/11] in ISO/IEC 14882:2020.
	* DON'T USE IT WHEN UNNECESSARY. E.g. I provide a_channel_layout_ref in codec_properties
	* that returns a ref to AVChannelLayout directly.
	* 
	* Represents an audio channel layout.
	* Encapsulates AVChannelLayout.
	* 
	* FFmpeg documentation restricts the ways in which one can
	*	1. create an AVChannelLayout
	*	2. manipulate an AVChannelLayout
	*	3. copy an AVChannelLayout
	*	4. destroy an AVChannelLayout
	* Hence this encapsulation.
	* 
	* Invariants: It is a valid AVChannelLayout
	*/
	class FF_WRAPPER_API channel_layout final
	{
	public:
		/*
		* Creation:
		* AVChannelLayout can be initialized as follows:
			1. default initialization with {0}, followed by setting all used fields correctly;
			2. by assigning one of the predefined AV_CHANNEL_LAYOUT_* initializers;
		See the static constexpr variables.
			3. with a constructor function, such as av_channel_layout_default(), av_channel_layout_from_mask() or av_channel_layout_from_string().
		*/

		/*
		* Legitimate way NO.1 to create one:
		* default initialization with {0}, followed by setting all used fields correctly;
		*
		* Creates a default channel layout with the fields passed in.
		* Now I do not support the use of map and opaque.
		* 
		* @param order Channel order used in this layout.
		* @param num_channels Number of channels in this layout.
		* @param mask It is a bitmask, where the position of each set bit means that the
        * AVChannel with the corresponding value is present.
		* @throws std::invalid_argument if the fields result in an invalid channel layout.
		*/
		channel_layout(AVChannelOrder order, int num_channels, uint64_t mask = 0);

		/*
		* Legitimate way NO.2 to create one:
		*/
		enum default_layouts
		{
			// Use regex \#define ([\w]+)[^\n]+(\n)
			// to search against these macro definitions in libavutil/channel_layout.h
			// and replace them with FF_$1,$2 to generate these

			FF_AV_CHANNEL_LAYOUT_MONO,
			FF_AV_CHANNEL_LAYOUT_STEREO,
			FF_AV_CHANNEL_LAYOUT_2POINT1,
			FF_AV_CHANNEL_LAYOUT_2_1,
			FF_AV_CHANNEL_LAYOUT_SURROUND,
			FF_AV_CHANNEL_LAYOUT_3POINT1,
			FF_AV_CHANNEL_LAYOUT_4POINT0,
			FF_AV_CHANNEL_LAYOUT_4POINT1,
			FF_AV_CHANNEL_LAYOUT_2_2,
			FF_AV_CHANNEL_LAYOUT_QUAD,
			FF_AV_CHANNEL_LAYOUT_5POINT0,
			FF_AV_CHANNEL_LAYOUT_5POINT1,
			FF_AV_CHANNEL_LAYOUT_5POINT0_BACK,
			FF_AV_CHANNEL_LAYOUT_5POINT1_BACK,
			FF_AV_CHANNEL_LAYOUT_6POINT0,
			FF_AV_CHANNEL_LAYOUT_6POINT0_FRONT,
			FF_AV_CHANNEL_LAYOUT_3POINT1POINT2,
			FF_AV_CHANNEL_LAYOUT_HEXAGONAL,
			FF_AV_CHANNEL_LAYOUT_6POINT1,
			FF_AV_CHANNEL_LAYOUT_6POINT1_BACK,
			FF_AV_CHANNEL_LAYOUT_6POINT1_FRONT,
			FF_AV_CHANNEL_LAYOUT_7POINT0,
			FF_AV_CHANNEL_LAYOUT_7POINT0_FRONT,
			FF_AV_CHANNEL_LAYOUT_7POINT1,
			FF_AV_CHANNEL_LAYOUT_7POINT1_WIDE,
			FF_AV_CHANNEL_LAYOUT_7POINT1_WIDE_BACK,
			FF_AV_CHANNEL_LAYOUT_5POINT1POINT2_BACK,
			FF_AV_CHANNEL_LAYOUT_OCTAGONAL,
			FF_AV_CHANNEL_LAYOUT_CUBE,
			FF_AV_CHANNEL_LAYOUT_5POINT1POINT4_BACK,
			FF_AV_CHANNEL_LAYOUT_7POINT1POINT2,
			FF_AV_CHANNEL_LAYOUT_7POINT1POINT4_BACK,
			FF_AV_CHANNEL_LAYOUT_HEXADECAGONAL,
			FF_AV_CHANNEL_LAYOUT_STEREO_DOWNMIX,
			FF_AV_CHANNEL_LAYOUT_22POINT2,
			FF_AV_CHANNEL_LAYOUT_7POINT1_TOP_BACK,
			FF_AV_CHANNEL_LAYOUT_AMBISONIC_FIRST_ORDER,
		};
		explicit channel_layout(default_layouts l);

		/*
		* Legitimate way NO.3 to create one:
		* with a constructor function, such as av_channel_layout_default()
		* 
		* Creates a default channel layout with 1 channel.
		*/
		inline channel_layout()
		{
			av_channel_layout_default(&cl, 1);
		}

		/*
		* Copies src.
		*/
		explicit channel_layout(const AVChannelLayout& src);

		/*
		* Copying an AVChannelLayout via assigning is forbidden, 
		av_channel_layout_copy() must be used instead (and its return value should be checked)
		*/
		channel_layout(const channel_layout& other);
		/*
		* Copying an AVChannelLayout via assigning is forbidden,
		av_channel_layout_copy() must be used instead (and its return value should be checked)
		*/
		channel_layout& operator=(const channel_layout& right);

		/*
		* Takes over other, and set other's order to native,
		* so that other won't be uninited by av_channel_layout_uninit(&cl).
		*/
		inline channel_layout(channel_layout&& other) noexcept
			: cl(other.cl)
		{
			other.cl.order = AV_CHANNEL_ORDER_NATIVE;
		}

		/*
		* Uninits self,
		* takes over right, and set right's order to native,
		* so that right won't be uninited by av_channel_layout_uninit(&cl).
		*/
		inline channel_layout& operator=(channel_layout&& right) noexcept
		{
			av_channel_layout_uninit(&cl);
			cl = right.cl;
			right.cl.order = AV_CHANNEL_ORDER_NATIVE;
			return *this;
		}

		/*
		* The channel layout must be unitialized with av_channel_layout_uninit()
		*/
		inline ~channel_layout() noexcept
		{
			av_channel_layout_uninit(&cl);
		}

	public:
		// @returns if the two channel layouts are equal
		bool operator==(const channel_layout& right) const noexcept(FF_ASSERTION_DISABLED);
		// @returns if the two channel layouts are equal
		bool operator==(const AVChannelLayout& right) const noexcept;

		AVChannelLayout& av_ch_layout() { return cl; }
		const AVChannelLayout& av_ch_layout() const { return cl; }

	public:
		/*
		* Copy this to dst.
		* Does free dst before copying.
		*/
		inline void set_av_channel_layout(AVChannelLayout& dst) const;

	public:
		/*
		* My wrapper for ::av_channel_layout_copy that interprets errors
		* as exceptions.
		*/
		static void av_channel_layout_copy(AVChannelLayout& dst, const AVChannelLayout& src);

	private:
		AVChannelLayout cl;
	};
}

// Some global operators
inline bool FF_WRAPPER_API operator==(const AVChannelLayout& left, const ff::channel_layout& right) noexcept
{
	return right == left;
}
inline bool FF_WRAPPER_API operator==(const AVChannelLayout& left, const AVChannelLayout& right) noexcept
{
	return !av_channel_layout_compare(&left, &right);
}