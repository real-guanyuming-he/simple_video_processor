#pragma once
/*
* Copyright (C) 2024 Guanyuming He
* This file is licensed under the GNU General Public License v3.
*
* This file is part of PROJECT_NAME_REPLACE_LATER.
* PROJECT_NAME_REPLACE_LATER is free software:
* you can redistribute it and/or modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
*
* PROJECT_NAME_REPLACE_LATER is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
* See the GNU General Public License for more details.
* You should have received a copy of the GNU General Public License along with PROJECT_NAME_REPLACE_LATER.
* If not, see <https://www.gnu.org/licenses/>.
*/

/*
* ff_math.h:
* Defines math helpers for ffmpeg programming.
* 
* Header only. Functions are mostly constexpr.
*/

extern "C"
{
#include <libavutil/avutil.h> // For AV_TIME_BASE macro
#include <libavutil/rational.h>
}

#include <numeric> // for gcd

namespace ff
{
	/*
	* Represents a rational number.
	* Pass by value is preferred. Should be 64 bit on most systems.
	* 
	* av_..._q() functions are not constexpr. This class
	* make all common operations constexpr.
	*/
	class rational final
	{
	public:
		constexpr rational() noexcept
			: r() {}

		/*
		* Initializes directly from an AVRational
		* 
		* @param avr
		*/
		constexpr rational(AVRational avr) noexcept
			: r(avr) {}

		/*
		* Initialize the rational with explicit numerator and denominator.
		* 
		* @param num numerator
		* @param den denominator
		*/
		constexpr rational(int num, int den) noexcept
			: r{ .num=num, .den=den } {}

		~rational() = default;
		constexpr rational(const rational&) noexcept = default;
		constexpr rational& operator=(const rational&) noexcept = default;

	public:
///////////////////////////// Operators /////////////////////////////
		constexpr bool operator==(const rational right) const noexcept
		{
			// b,d != 0 -> (a/b = c/d <-> ad = bc)  
			return r.num * right.r.den == r.den * right.r.num;
		}
		constexpr bool operator!=(const rational right) const noexcept
		{
			// b,d != 0 -> (a/b != c/d <-> ad != bc)  
			return r.num * right.r.den != r.den * right.r.num;
		}
		constexpr bool operator<(const rational right) const noexcept
		{
			// bd > 0 -> ad < bc
			// bd < 0 -> ad > bc
			if (r.den * right.r.den > 0)
			{
				return r.num * right.r.den < r.den * right.r.num;
			}
			else
			{
				return r.num * right.r.den > r.den * right.r.num;
			}
		}
		constexpr bool operator>(const rational right) const noexcept
		{
			// bd > 0 -> ad > bc
			// bd < 0 -> ad < bc
			if (r.den * right.r.den > 0)
			{
				return r.num * right.r.den > r.den * right.r.num;
			}
			else
			{
				return r.num * right.r.den < r.den * right.r.num;
			}
		}

		constexpr rational operator+(const rational right) const noexcept
		{
			// a/b + c/d = (ad+bc)/bd
			return reduce
			(rational(
				r.num * right.r.den + r.den * right.r.num,
				r.den * right.r.den
			));
		}
		constexpr rational operator-(const rational right) const noexcept
		{
			// a/b + c/d = (ad-bc)/bd
			return reduce
			(rational(
				r.num * right.r.den - r.den * right.r.num,
				r.den * right.r.den
			));
		}

		constexpr rational operator*(const rational right) const noexcept
		{
			// a/b * c/d = ac/bd
			return reduce
			(rational(
				r.num * right.r.num,
				r.den * right.r.den
			));
		}
		constexpr rational operator*(const int n) const noexcept
		{
			return rational(r.num * n, r.den * n);
		}
		constexpr rational& operator*=(const int n) noexcept
		{
			r.num *= n; r.den *= n;
			return *this;
		}

		constexpr rational operator/(const rational right) const noexcept
		{
			// a/b / c/d = ad/bc
			return reduce
			(rational(
				r.num * right.r.den,
				r.den * right.r.num
			));
		}
		constexpr rational operator/(const int n) const noexcept
		{
			return rational(r.num / n, r.den / n);
		}

		constexpr rational& operator/=(const int n) noexcept
		{
			r.num /= n; r.den /= n;
			return *this;
		}

	public:
///////////////////////////// Observers & Producers /////////////////////////////
		constexpr AVRational av_rational() const noexcept { return r; }

		constexpr double to_double() const noexcept
		{
			return (double)r.num / (double)r.den;
		}

		constexpr rational to_time_absolute(const rational time_base) const noexcept
		{
			return this->operator*(time_base);
		}

		// valid iff its denominator != 0.
		constexpr bool valid() const noexcept { return r.den != 0; }

		/*
		* Reduces the rational to lowest terms.
		*/
		static constexpr rational reduce(rational r)
		{
			int gcd = std::gcd(r.r.num, r.r.den);
			return r / gcd;
		}
	public:
///////////////////////////// Mutators /////////////////////////////
		/*
		* Reduces itself to lowest terms
		*/
		constexpr void reduce()
		{
			int gcd = std::gcd(r.num, r.den);
			r.num /= gcd; r.den /= gcd;
		}

	private:
		AVRational r;
	};

	// If a time base is not provided, one usually can use this instead.
	static constexpr rational ff_global_time_base(1, AV_TIME_BASE);

	// Suitable for 24,25,30,60,120 and many other common fps values.
	static constexpr rational ff_common_video_time_base( 1, 600 );

	// Suitable for many common audio sample rates
	static constexpr rational ff_common_audio_time_base( 1, 90000 );

	static constexpr int common_audio_sample_rate = 44100;

	// @returns how many number of bases secs equals to
	inline constexpr int64_t ff_seconds_to_time_in_base(double secs, AVRational base)
	{
		return static_cast<int64_t>(secs * (double)base.den / (double)base.num);
	}

}