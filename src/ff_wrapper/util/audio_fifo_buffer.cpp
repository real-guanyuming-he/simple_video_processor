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

#include "audio_fifo_buffer.h"
#include "ff_helpers.h"

extern "C"
{
#include <libavutil/error.h>
}

#include <stdexcept>

ff::audio_fifo_buffer::audio_fifo_buffer
(
	AVSampleFormat sample_fmt, 
	int num_channels, 
	int initial_size
) : max_num_samples(initial_size), stored_num_samples(0)
{
	p_fifo = av_audio_fifo_alloc
	(
		sample_fmt, num_channels, initial_size
	);

	if (nullptr == p_fifo)
	{
		throw std::bad_alloc();
	}

	// Should establish the invariants
	check_invariants();
}

ff::audio_fifo_buffer::~audio_fifo_buffer() noexcept
{
	ffhelpers::safely_free_audio_fifo(&p_fifo);
}

void ff::audio_fifo_buffer::clear() noexcept
{
	av_audio_fifo_reset(p_fifo);
	stored_num_samples = 0;

	// Should maintain the invariants
	check_invariants();
}

void ff::audio_fifo_buffer::enlarge(int new_size)
{
	if (new_size <= max_size())
	{
		throw std::invalid_argument("new_size must be > max_size()");
	}

	int ret = av_audio_fifo_realloc(p_fifo, new_size);
	if (ret < 0)
	{
		// Failure
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened: Could not realloc audio fifo buffer.", ret);
		}
	}

	// Don't forget to update the fields.
	max_num_samples = new_size;

	// Should maintain the invariants
	check_invariants();
}

void ff::audio_fifo_buffer::add_data(const void * const * data, int num_samples_to_add)
{
	if (num_samples_to_add > available_size() || num_samples_to_add <= 0)
	{
		throw std::invalid_argument("Invalid: num_samples_to_add > available_size() || num_samples_to_add <= 0");
	}

	int ret = av_audio_fifo_write(p_fifo, const_cast<void*const*>(data), num_samples_to_add);
	if (ret < 0)
	{
		// Failure
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened: Could not write to audio fifo buffer.", ret);
		}
	}

	// Don't forget to update the fields.
	stored_num_samples += num_samples_to_add;

	// Should maintain the invariants
	check_invariants();

}

void ff::audio_fifo_buffer::pop_data(void* const * data, int num_samples_to_pop)
{
	if (num_samples_to_pop > size() || num_samples_to_pop <= 0)
	{
		throw std::invalid_argument("Invalid: num_samples_to_pop > size() || num_samples_to_pop <= 0");
	}

	int ret = av_audio_fifo_read(p_fifo, data, num_samples_to_pop);
	if (ret < 0)
	{
		// Failure
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened: Could not read from audio fifo buffer.", ret);
		}
	}

	// Don't forget to update the fields.
	stored_num_samples -= num_samples_to_pop;

	// Should maintain the invariants
	check_invariants();
}

void ff::audio_fifo_buffer::discard_data(int num_samples_to_discard)
{
	if (num_samples_to_discard > size() || num_samples_to_discard <= 0)
	{
		throw std::invalid_argument("Invalid: num_samples_to_discard > size() || num_samples_to_discard <= 0");
	}

	int ret = av_audio_fifo_drain(p_fifo, num_samples_to_discard);
	if (ret < 0)
	{
		// Failure
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened: Could not discard from audio fifo buffer.", ret);
		}
	}

	// Don't forget to update the fields.
	stored_num_samples -= num_samples_to_discard;

	// Should maintain the invariants
	check_invariants();
}

void ff::audio_fifo_buffer::peek_data(void* const * data, int num_samples_to_peek, int offset)
{
	if (offset < 0)
	{
		throw std::invalid_argument("Invalid: offset < 0");
	}
	if (num_samples_to_peek <= 0)
	{
		throw std::invalid_argument("Invalid: num_samples_to_peek <= 0");
	}
	if (offset + num_samples_to_peek > size())
	{
		throw std::invalid_argument("Invalid: offset + num_samples_to_peek > size()");
	}

	int ret = av_audio_fifo_peek_at(p_fifo, data, num_samples_to_peek, offset);
	if (ret < 0)
	{
		// Failure
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened: Could not discard from audio fifo buffer.", ret);
		}
	}

	// Don't need to update any fields.

	// Fields are not updated.
	// check_invariants();
}
