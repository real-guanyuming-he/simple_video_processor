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

extern "C"
{
#include <libavutil/frame.h> // For those enums
}

class AVFrame;
struct AVChannelLayout;

namespace ff
{
	/*
	* Represents either a video or an audio frame.
	* 
	* Invariants: 
	*	those of ff_object, and
	*	if READY, then data is ref counted through AVBuffer (i.e. it cannot store custom data).
	*/
	class FF_WRAPPER_API frame : public ff_object
	{
	public:
		struct data_properties
		{
			/*
			* Set up properties for video frame data.
			*/
			inline constexpr data_properties(int f, int w, int h, int align = 0) noexcept
				: v_or_a(true), fmt(f), align(align),
				details(w, h) {}

			/*
			* Set up properties for audio sample data.
			*/
			inline data_properties(int f, int num, const AVChannelLayout& ch_layout, int align = 0) noexcept
				: v_or_a(false), fmt(f), align(align),
				details(num, ch_layout) {}

			union d
			{
				inline constexpr d(int w, int h)
					: v{ .width = w,.height = h } {}
				inline d(int num, const AVChannelLayout& ch)
					: a{ .ch_layout_ref = &ch, .num_samples = num } {}

				struct
				{
					const AVChannelLayout* ch_layout_ref;
					int num_samples;
				} a;

				struct
				{
					int width;
					int height;
				} v;
			} details;

			// Pixel format or audio sample format, depending on v_or_a.
			int fmt;
			// Alignment of the buffer. Set to 0 to automatically choose one for the current CPU. 
			// It is highly recommended to give 0 here unless you know what you are doing.
			int align;

			bool v_or_a;
		};

	public:
		/*
		* Initializes a default frame.
		* @param allocate_frame whether a new frame instance is allocated or not (OBJECT_CREATED or not).
		* If not, the pointer will be nullptr.
		*/
		explicit frame(bool allocate_frame = true);

		/*
		* Takes the ownership of an avframe.
		* @param v_or_a whether the frame contains video or audio data. Not used if has_data = false.
		* @param has_data whether the frame contains any data
		* @throws std::invalid_argument if frame is nullptr.
		*/
		frame(::AVFrame* frame, bool v_or_a, bool has_data = true);

		/*
		* Copies another frame and its data, if it contains any.
		* If other's DESTROYED, set this to nullptr, too.
		* NOTE: Different from packet(const packet&), this copies the data as well.
		*/
		frame(const frame& other);
		/*
		* Takes over other's frame, and sets other to destroyed with its pointer set to nullptr.
		*/
		frame(frame&& other) noexcept;

		/*
		* Destroys everything related to the frame and the frame itself.
		*/
		inline ~frame() noexcept { destroy(); }

	public:
		bool v_or_a() const noexcept { return video_or_audio; }

		/*
		* Clears all data it stores so it can be reused to store some other data.
		*/
		void clear_data() noexcept { release_resources_memory(); }

		/*
		* Allocates data for the frame.
		* @param dp properties of the data.
		* @throws std::logic_error if the frame is not CREATED.
		*/
		inline void allocate_data(const data_properties& dp)
		{
			allocate_resources_memory(0, const_cast<data_properties*>(&dp));
		}

	public:
		/*
		* Note: the info returned does not include the alignment of the data.
		* 
		* @returns the properties of the data the frame holds currently.
		* @throws std::logic_error if the frame is not READY.
		*/
		data_properties get_data_properties() const;

	private:
		// Inherited via ff_object
		void internal_allocate_object_memory() override;

		/*
		* @param size not used
		* @param additional_information a pointer to data_properties
		*/
		void internal_allocate_resources_memory(uint64_t size, void* additional_information) override;

		void internal_release_object_memory() noexcept override;

		void internal_release_resources_memory() noexcept override;

	private:
		::AVFrame* p_frame;

		// Only meaningful when the frame is READY.
		bool video_or_audio = true;

	};
}