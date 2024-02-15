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
#include <libavutil/audio_fifo.h>
#include <libavutil/samplefmt.h>
}

struct AVAudioFifo;

namespace ff
{
	/*
	* Encapsulation of avutil's audio FIFO.
	* 
	* Besides wrapping the existing functions, I
	* want to build a small extra that allows a user to create a 
	* FIFO of fixed size by providing an additional method that will throw
	* if the size would be exceeded to write the buffer.
	* 
	* Invariants:
	*	p_fifo != nullptr (the FIFO buffer is created).
	*	max_num_samples = allocated space > 0
	*	0 <= stored_num_samples <= max_num_samples
	*/
	class FF_WRAPPER_API audio_fifo_buffer
	{
	private:
		static constexpr int default_initial_num_samples = 32;

		inline void check_invariants()
		{
			FF_ASSERT(nullptr != p_fifo,
				"The invariants should hold.");
			FF_ASSERT(max_num_samples == av_audio_fifo_size(p_fifo) + av_audio_fifo_space(p_fifo),
				"The invariants should hold.");
			FF_ASSERT(stored_num_samples == av_audio_fifo_size(p_fifo),
				"The invariants should hold.");
		}

	public:
		// Must provide information to init the buffer
		audio_fifo_buffer() = delete;

		/*
		* Creates such a fifo buffer that can hold at most
		* initial_size samples of sample_fmt per channel
		* for num_channels.
		* 
		* Note: once created, you can only enlarge a FIFO buffer, not shrink it.
		* So choose the initial size wisely.
		*/
		audio_fifo_buffer
		(
			AVSampleFormat sample_fmt,
			int num_channels,
			int initial_size = default_initial_num_samples
		);

		~audio_fifo_buffer() noexcept;

////////////////////////////// Observers //////////////////////////////
	public:
		/*
		* @returns the number of samples (per channel) the fifo buffer can contain at most.
		*/
		inline int max_size() const noexcept
		{
			return max_num_samples;
		}
		/*
		* @returns the number of samples (per channel) the fifo buffer currently contains.
		*/
		inline int size() const noexcept
		{
			return stored_num_samples;
		}
		/*
		* @returns the maximum number of samples that can be added to the buffer currently.
		*/
		inline int available_size() const noexcept
		{
			return max_num_samples - stored_num_samples;
		}

////////////////////////////// Mutators //////////////////////////////
	public:
		/*
		* Clears the stored samples in the buffer.
		* After this call, size() = 0 and available_size() = max_size().
		*/
		void clear() noexcept;

		/*
		* Enlarges the fifo buffer so that it can contain new_size number of samples.
		* After the call, the data will not be changed.
		* max_size() will be changed to new_size.
		* 
		* @param new_size the number of samples that you want the buffer to hold after the call.
		* @throws std::invalid_argument if new_size <= current max_size().
		*/
		void enlarge(int new_size);
		
		/*
		* Adds data to the buffer.
		* 
		* @param data pointer to the audio data planes
		* @param num_samples_to_add
		* @throws std::invalid_argument if num_samples_to_add > available_size() or num_samples_to_add <= 0.
		*/
		void add_data(const void* const * data, int num_samples_to_add);

		/*
		* Adds data from the buffer.
		*
		* @param data pointer to audio planes where the popped data will be stored.
		* @param num_samples_to_pop
		* @throws std::invalid_argument if num_samples_to_pop > size() or num_samples_to_pop <= 0.
		*/
		void pop_data(void* const * data, int num_samples_to_pop);

		/*
		* Discards data from the buffer.
		*
		* @param num_samples_to_discard
		* @throws std::invalid_argument if num_samples_to_discard > size() or num_samples_to_discard <= 0.
		*/
		void discard_data(int num_samples_to_discard);

		/*
		* Peeks data in the buffer.
		* 
		* @param data pointer to audio planes where the popped data will be stored.
		* @param num_samples_to_peek
		* @param offset from the start (first in) of the buffer.
		* @throws std::invalid_argument if offset + num_samples_to_peek > size() or 
		*	num_samples_to_peek <= 0 or 
		*	offset < 0.
		*/
		void peek_data(void* const * data, int num_samples_to_peek, int offset = 0);

	private:
		AVAudioFifo* p_fifo;

		// To avoid function calls, I defined these fields and access them as inline methods: ..._size().
		// If I didn't, then the ..._size() methods could not be inline,
		// which would result in function calls.
		// Additionally, to retrieve the sizes, I would have to call
		// av_audio_fifo_...().
		int max_num_samples, stored_num_samples;
	};
}