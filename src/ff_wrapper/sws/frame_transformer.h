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

/*
* Contains the definition of class frame_transformer
*/

#include "../util/util.h"
#include "../data/frame.h"

struct SwsContext;

namespace ff
{
	class decoder;
	class encoder;

	/*
	* Transforms a video ff::frame in one or more of the following ways:
	*	1. Changes the pixel format of the frame.
	*	2. Scales the resolution of the frame.
	* 
	* An object of frame_transformer performs a fixed transformation.
	* The transformation is defined by the arguments you give to one of its constructors.
	* 
	* Invariants: 
		1. sws_ctx != nullptr,
		2. src_w, src_h, dst_w, dst_h > 0,
		3. src_fmt and dst_fmt are supported.
	*/
	class FF_WRAPPER_API frame_transformer final
	{
	public:
		enum algorithms : int
		{
			// Go to the algorithm macro definitions in <libswscale/swscale.h>
			// Copy and paste them here
			// Search against regex #define ([\w]+)[\s]+([\w]+)
			// and replace with FF_$1 = $2,

			FF_SWS_FAST_BILINEAR = 1,
			FF_SWS_BILINEAR = 2,
			FF_SWS_BICUBIC = 4,
			FF_SWS_X = 8,
			FF_SWS_POINT = 0x10,
			FF_SWS_AREA = 0x20,
			FF_SWS_BICUBLIN = 0x40,
			FF_SWS_GAUSS = 0x80,
			FF_SWS_SINC = 0x100,
			FF_SWS_LANCZOS = 0x200,
			FF_SWS_SPLINE = 0x400,
		};

	public:
		// You must provide transformation information.
		frame_transformer() = delete;

		/*
		* Performs such a transformation that transforms 
		* video frames of src_properties to video frames of dst_properties
		* 
		* @throws std::invalid_argument if either src_properties or dst_properties
		* is not for video frames.
		* @throws std::domain_error if either src's pixel format is not supported as input
		* or dst's pixel format is not supported as output.
		* You can query if a pixel format is supported as input/output by calling
		* query_input/output_pixel_format_support
		*/
		frame_transformer
		(
			const frame::data_properties& dst_properties,
			const frame::data_properties& src_properties,
			// bicubic is widely applicable and has good performance.
			algorithms algorithm = FF_SWS_BICUBIC 
		);

		/*
		* Performs such a transformation that transforms
		* video frames from decoder to video frames into encoder.
		*
		* @throws std::invalid_argument if either decoder or encoder
		* is not for video.
		* @throws std::logic_error if either decoder or encoder
		* is not ready.
		* @throws std::domain_error if either decoder's pixel format is not supported as input
		* or encoder's pixel format is not supported as output.
		* You can query if a pixel format is supported as input/output by calling
		* query_input/output_pixel_format_support
		*/
		frame_transformer
		(
			const encoder& enc,
			const decoder& dec,
			// bicubic is widely applicable and has good performance.
			algorithms algorithm = FF_SWS_BICUBIC
		);

		/*
		* Performs such a transformation that transforms
		* video frames of src_w, src_h, src_fmt into frames of dst_w, dst_h, dst_fmt.
		*
		* @throws std::invalid_argument if either decoder or encoder
		* is not for video.
		* @throws std::domain_error if either decoder's pixel format is not supported as input
		* or encoder's pixel format is not supported as output.
		* You can query if a pixel format is supported as input/output by calling
		* query_input/output_pixel_format_support
		*/
		frame_transformer
		(
			int dst_w, int dst_h, AVPixelFormat dst_fmt,
			int src_w, int src_h, AVPixelFormat src_fmt,
			// bicubic is widely applicable and has good performance.
			algorithms algorithm = FF_SWS_BICUBIC
		);

		~frame_transformer();

	public:
		/*
		* Converts src.
		* 
		* @param src the frame to be converted. src's properties will also be 
		* copied to it.
		* @returns the dst frame converted.
		* @throws std::invalid_argument if src does not match the properties you gave
		* to a constructor.
		*/
		ff::frame convert_frame(const ff::frame& src);

		/*
		* Converts src and writes the result to dst.
		*
		* @param src the frame to be converted.
		* @param dst the frame where the result will be contained in. If dst is destroyed()
		* or created(), then it will be made ready with the dst properties. 
		* src's properties will also be copied to it.
		* @throws std::invalid_argument if src/dst does not match the properties you gave
		* to a constructor.
		*/
		void convert_frame(ff::frame& dst, const ff::frame& src);

	public:
		inline ff::frame::data_properties src_properties() const noexcept
		{
			return ff::frame::data_properties(src_fmt, src_w, src_h);
		}
		inline ff::frame::data_properties dst_properties() const noexcept
		{
			return ff::frame::data_properties(dst_fmt, dst_w, dst_h);
		}
		
	public:
		// @returns true iff fmt is supported as an input pixel format
		static bool query_input_pixel_format_support(AVPixelFormat fmt);

		// @returns true iff fmt is supported as an output pixel format
		static bool query_output_pixel_format_support(AVPixelFormat fmt);

	private:
		SwsContext* sws_ctx;

		// These are recorded to check if frames to be converted have the same properties.

		int src_w, src_h;
		int dst_w, dst_h;
		AVPixelFormat src_fmt, dst_fmt;

	private:
		/*
		* Common piece of code among constructors.
		*/
		void internal_create_sws_context
		(
			int dst_w, int dst_h, AVPixelFormat dst_fmt,
			int src_w, int src_h, AVPixelFormat src_fmt,
			algorithms algorithm
		);
	};
}
