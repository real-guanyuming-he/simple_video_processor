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
#include "../util/channel_layout.h"

extern "C"
{
#include <libavutil/frame.h> // For those enums
}

namespace ff
{
	/*
	* Represents either a video or an audio frame.
	* 
	* In order to access the data stored in a frame, you have to understand some terminologies 
	* and how a video/audio frame is stored in memory.
	* 
	* A piece of audio may be planar, then a frame for it has multiple planes of the same size.
	* A video frame may divide data into planes by pixel components (e.g. a plane of Y/U/V each).
	* Therefore, a video plane still has the same dimensions as the picture.
	* However, for better performance, each row (line) of a plane may be padded with extra bits.
	* Hence, a new variable, line_size is introduced to measure the length of a padded row (line).
	* You may get it by calling line_size().
	* Some texts may call the line_size stride. Here I call it line_size to concur with FFmpeg.
	* For audio, only line_size(0) is valid. Each plane is of the same size, so only line_size(0) is used.
	* 
	* Some video frames may be stored up-side-down (bottom-up), as if flipped along the central horizontal axis.
	* For such frame data, the pointer returned by data() points to the end of a data plane,
	* and line_size() returns a negative number, so that ptr+line_size always navigates to the next line.
	* 
	* Invariants: 
	*	those of ff_object, and
	*	if READY, then data is ref counted through AVBuffer (i.e. it cannot store custom data).
	*/
	class FF_WRAPPER_API frame : public ff_object
	{
	public:
		/*
		* This struct independently determines how much space is needed for the data of a frame.
		*/
		struct data_properties
		{
			data_properties() = delete;

			/*
			* Set up properties for video frame data.
			*/
			inline data_properties(int f, int w, int h, int align = 0) noexcept
				: v_or_a(true), fmt(f), align(align),
				width(w), height(h) {}

			/*
			* Set up properties for audio sample data.
			*/
			inline data_properties(int f, int num, const channel_layout& ch_layout, int align = 0) noexcept
				: v_or_a(false), fmt(f), align(align),
				num_samples(num), ch_layout(ch_layout) {}

			// Pixel format or audio sample format, depending on v_or_a.
			int fmt;
			// Alignment of the buffer. Set to 0 to automatically choose one for the current CPU. 
			// It is highly recommended to give 0 here unless you know what you are doing.
			int align;

			// Video only
			int width = 0, height = 0;
			// Audio only
			channel_layout ch_layout; int num_samples = 0;

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

		frame& operator=(const frame& right);
		frame& operator=(frame&& right) noexcept;

		/*
		* Destroys everything related to the frame and the frame itself.
		*/
		inline ~frame() noexcept { destroy(); }

	public:
		AVFrame* av_frame() noexcept { return p_frame; }
		const AVFrame* av_frame() const noexcept { return p_frame; }

		bool v_or_a() const noexcept { return video_or_audio; }

		/*
		* @returns number of data planes.
		* @throws std::logic_error if the frame is not ready.
		*/
		int number_planes() const;

		/*
		* See the comment for the class for more about how data is stored.
		* 
		* @param ind the index of the data plane.
		* @throws std::logic_error if the frame is not ready.
		* @throws std::out_of_range if ind is out of range.
		* @returns the line size (stride) of the ind^th data plane.
		* If the frame is stored up-side-down, then a negative number
		* whose absolute value is the length is returned.
		*/
		int line_size(int ind = 0) const;

		/*
		* Gets one of the data plane that the frame stores.
		* For audio, pass 0 to get the entire data.
		* See the comment for the class for more about how data is stored.
		* 
		* @param ind index of the data plane.
		* @returns a pointer to the start/end of the data plane.
		* @throws std::logic_error if the frame is not READY.
		* @throws std::out_of_range if ind is out of range
		*/
		void* data(int ind = 0);

		/*
		* Gets one of the data plane that the frame stores.
		* For audio, pass 0 to get the entire data.
		* 
		* @param ind index of the data plane.
		* @returns a const pointer to the start/end of the data plane.
		* @throws std::logic_error if the frame is not READY.
		* @throws std::out_of_range if ind is out of range
		*/
		const void* data(int ind = 0) const;

		/*
		* Note: the info returned does not include the alignment of the data.
		* 
		* @returns the properties of the data the frame holds currently.
		* @throws std::logic_error if the frame is not READY.
		*/
		data_properties get_data_properties() const;

		/*
		* Note: It is highly recommended to use the copy ctor/ass operator to copy a frame.
		*
		* @returns a frame that references the same data as this does.
		* @throws std::logic_error if this isn't ready.
		*/
		frame shared_ref() const;

	public:
		/*
		* Clears all data it stores so it can be reused to store some other data.
		*/
		inline void clear_data() noexcept
		{
			release_resources_memory();
		}

		/*
		* Allocates data for the frame given the properties of the frame.
		* @param dp properties of the frame.
		* @throws std::logic_error if the frame is not CREATED.
		*/
		inline void allocate_data(const data_properties& dp)
		{
			allocate_resources_memory(0, const_cast<data_properties*>(&dp));
		}
		

	private: // Inherited through ff_object.
		void internal_allocate_object_memory() override;

		/*
		* Allocates a buffer for the frame given the properties of the frame.
		* @param size not used
		* @param additional_information a pointer to data_properties
		*/
		void internal_allocate_resources_memory(uint64_t size, void* additional_information) override;

		void internal_release_object_memory() noexcept override;

		void internal_release_resources_memory() noexcept override;

	private:
		/*
		* Finds the number of planes for me when the class takes over external data.
		*/
		void internal_find_num_planes() noexcept(FF_ASSERTION_DISABLED);

	private:
		::AVFrame* p_frame;

		// Number of picture/channel planes. Only meaningful when the frame is READY.
		int num_planes = 0;

		// Only meaningful when the frame is READY.
		bool video_or_audio = true;

//////////////////////////////////////// TESTING ONLY ////////////////////////////////////////
#ifdef FF_TESTING
	public:
		auto& t_get_ref_frame() { return p_frame; }
#endif // FF_TESTING

	};
}