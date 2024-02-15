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

struct SwrContext;

extern "C"
{
#include <libavutil/samplefmt.h>
}

namespace ff
{
	class channel_layout;

	/*
	* Converts audio samples. The converted samples are stored into
	* a FIFO buffer for better access.
	* 
	* Once constructed, its parameters cannot be changed.
	* The convertions will be done according to the parameters.
	*/
	class audio_resampler
	{
	public:
		// Must provide parameters to init the resampler.
		audio_resampler() = delete;

		/*
		* Creates the resampler with the resample parameters.
		* 
		* TODO: consider adding some parameters for initializing the FIFO buffer.
		*/
		audio_resampler
		(
			const channel_layout& dst_ch_layout,
			AVSampleFormat dst_sample_fmt,
			int dst_sample_rate,
			const channel_layout& src_ch_layout,
			AVSampleFormat src_sample_fmt,
			int src_sample_rate
		);

		~audio_resampler() noexcept;

	private:
		SwrContext* swr_ctx;
		// TODO: add the FIFO buffer.
	};
}