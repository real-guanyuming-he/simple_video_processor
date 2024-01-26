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
* Defines class time that represents a timestamp in a multimedia file
* as well as some helpers around it.
*/

#include "ff_math.h"
#include "util.h"

#include <string>

namespace ff
{
	// Suitable for 24,25,30,60,120 and many other common fps values.
	static constexpr rational common_video_time_base(1, 600);

	// Suitable for many common audio sample rates
	static constexpr rational common_audio_time_base(1, 90000);

	static constexpr int common_audio_sample_rate = 44100;

	/*
	* Represents a time stamp since the start of some multimedia file.
	* The class stores both the exact time in some time base and the time base.
	* 
	* In another words, an object of the class represents a timestamp RELATIVE to a time base.
	* Call to_absolute() to get the absolute timestamp in seconds.
	* 
	* A time base acts as a unit just like second. For example,
	A relative timestamp of value five means it is five time bases away from zero.
	Therefore, the value of a time bases represents its length in seconds.
	*
	* No need to be DLL exported/imported, as this is header only.
	* 
	* Invariant: time base > 0. I allow negative timestamps as FFmpeg allows it.
	*/
	class time final
	{
	public:
		time() = default;
		~time() = default;

		/*
		* Initializes directly from relative stamp and time base
		* @throws std::invalid_argument if timestamp < 0 or base <= 0
		*/
		constexpr time(const int64_t timestamp, const rational base)
			: t(timestamp, 1), b(base)
		{
			if (base <= zero_rational)
			{
				throw std::invalid_argument("time base must be positive.");
			}
		}

		/*
		* Initializes directly from relative stamp and time base
		* @throws std::invalid_argument if timestamp < 0 or base <= 0
		*/
		constexpr time(const rational_64 timestamp, const rational base)
			: t(timestamp), b(base)
		{
			if (base <= zero_rational)
			{
				throw std::invalid_argument("time base must be positive.");
			}
		}

		time(const time&) = default;
		time& operator=(const time&) = default;

	public:
///////////////////////////// Operator /////////////////////////////
		constexpr bool operator<(const time& right) const noexcept
		{
			return to_absolute() < right.to_absolute();
		}
		constexpr bool operator>(const time& right) const noexcept
		{
			return to_absolute() > right.to_absolute();
		}
		constexpr bool operator<=(const time& right) const noexcept
		{
			return to_absolute() <= right.to_absolute();
		}
		constexpr bool operator>=(const time& right) const noexcept
		{
			return to_absolute() >= right.to_absolute();
		}
		constexpr bool operator==(const time& right) const noexcept
		{
			return to_absolute() == right.to_absolute();
		}
		constexpr bool operator!=(const time& right) const noexcept
		{
			return to_absolute() != right.to_absolute();
		}

	public:
///////////////////////////// Observers & Producers /////////////////////////////
		/*
		* @returns the accurate relative timestamp with respect to the time base.
		*/
		constexpr rational_64 timestamp_accurate() const noexcept { return t; }
		/*
		* @returns the relative timestamp rounded to int64_t
		*/
		constexpr int64_t timestamp_approximate() const noexcept
		{
			return static_cast<int64_t>(t.get_num() / t.get_den());
		}

		/*
		* @returns the time base
		*/
		constexpr rational time_base() const noexcept { return b; }

		/*
		* @returns the absolute timestamp in seconds that the object represents
		* as an accurate rational.
		*/
		constexpr rational_64 to_absolute() const noexcept
		{
			return t * b;
		}
		/*
		* @returns the absolute timestamp in seconds that the object represents
		* as an approximate double.
		*/
		constexpr double to_absolute_double() const noexcept
		{
			return to_absolute().to_double();
		}

		/*
		* Formats the timestamp into a string
		*
		* @param fmt the format used, in std::format specification
		* index 0: total microseconds 
		* index 1: total miliseconds
		* index 2: total seconds
		* index 3: total minutes
		* index 4: total hours
		* index 5: remaining miliseconds
		* index 6: remaining seconds
		* index 7: remaining minutes
		* index 8: remaining hours
		* @throws std::format_error on bad format string.
		*/
		FF_WRAPPER_API std::string to_string
		(
			const std::string& fmt =
			"{7:02d}:{6:02d}:{5:05.2f}" // HH:MM:SS.mm
		) const;

	public:
///////////////////////////// Mutators /////////////////////////////
		/*
		* Changes the time base to a new one.
		* The new timestamp and time_base will be different iff new_tb != old tb
		* 
		* @param new_tb the new time base
		* @throws std::invalid_argument if new_tb <= 0
		*/
		constexpr void change_time_base(const rational new_tb)
		{
			if (new_tb <= zero_rational)
			{
				throw std::invalid_argument("time base must be positive.");
			}

			// How many new_tb's does the absolute time have?
			t = to_absolute() / new_tb;
			// Update the time base
			b = new_tb;
		}


	private:
		// the time stamp in the time base
		rational_64 t;
		// the time base
		rational b;
	};

}