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
* ff_math.h:
* Defines math helpers for ffmpeg programming.
* 
* Header only. Functions are mostly constexpr.
*/

extern "C"
{
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
	* make all such common operations constexpr by rewriting them.
	* 
	* Invariant: den != 0.
	* Established by constructors and assignment operators.
	* Maintained by
	*  1. arithmetic operators (Rationals is closed under +,-,*, and / detects 0).
	*  2. the only mutator, reduce(), that reduces one to lowest terms.
	*/
	class rational final
	{
	public:
///////////////////////////// Default things /////////////////////////////
		~rational() = default;

		constexpr rational(const rational&) noexcept = default;
		constexpr rational& operator=(const rational&) noexcept = default;

	public:
		/*
		* Initializes a zero (0/1).
		*/
		constexpr rational() noexcept
			: r{ .num = 0, .den = 1 } {}

		/*
		* Initializes directly from an AVRational
		* 
		* @param avr
		* @throws std::invalid_argument if avr.den = 0
		*/
		constexpr rational(AVRational avr)
			: r(avr) 
		{
			if (avr.den == 0)
			{
				throw std::invalid_argument("The denominator cannot be 0.");
			}
		}

		/*
		* Initializes the rational with explicit numerator and denominator.
		* 
		* @param num numerator
		* @param den denominator
		* @throws std::invalid_argument if den = 0
		*/
		constexpr rational(int num, int den)
			: r{ .num=num, .den=den } 
		{
			if (den == 0)
			{
				throw std::invalid_argument("The denominator cannot be 0.");
			}
		}

		/*
		* @throws std::invalid_argument if den = 0
		*/
		constexpr rational& operator=(const AVRational avr)
		{
			if (avr.den == 0)
			{
				throw std::invalid_argument("The denominator cannot be 0.");
			}

			r.num = avr.num;
			r.den = avr.den;
			return *this;
		}

	public:
///////////////////////////// Arithmetic operators /////////////////////////////
		constexpr bool operator==(const rational right) const noexcept
		{
			// b,d != 0 -> (a/b = c/d <-> ad = bc)  
			return r.num * right.r.den == r.den * right.r.num;
		}
		constexpr bool operator==(const AVRational right) const noexcept
		{
			// b,d != 0 -> (a/b = c/d <-> ad = bc)  
			return r.num * right.den == r.den * right.num;
		}
		constexpr bool operator!=(const rational right) const noexcept
		{
			// b,d != 0 -> (a/b != c/d <-> ad != bc)  
			return r.num * right.r.den != r.den * right.r.num;
		}
		constexpr bool operator!=(const AVRational right) const noexcept
		{
			// b,d != 0 -> (a/b != c/d <-> ad != bc)  
			return r.num * right.den != r.den * right.num;
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

		constexpr bool operator<=(const rational right) const noexcept
		{
			// bd > 0 -> ad <= bc
			// bd < 0 -> ad >= bc
			if (r.den * right.r.den > 0)
			{
				return r.num * right.r.den <= r.den * right.r.num;
			}
			else
			{
				return r.num * right.r.den >= r.den * right.r.num;
			}
		}
		constexpr bool operator>=(const rational right) const noexcept
		{
			// bd > 0 -> ad >= bc
			// bd < 0 -> ad <= bc
			if (r.den * right.r.den > 0)
			{
				return r.num * right.r.den >= r.den * right.r.num;
			}
			else
			{
				return r.num * right.r.den <= r.den * right.r.num;
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
			return reduce(rational(r.num * n, r.den));
		}
		constexpr rational& operator*=(const int n) noexcept
		{
			r.num *= n;
			reduce();
			return *this;
		}

		/*
		* @throws std::invalid_argument if right = 0
		*/
		constexpr rational operator/(const rational right) const
		{
			if (right.num() == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			// a/b / c/d = ad/bc
			return reduce
			(rational(
				r.num * right.r.den,
				r.den * right.r.num
			));
		}
		/*
		* @throws std::invalid_argument if n = 0
		*/
		constexpr rational operator/(const int n) const
		{
			if (n == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			return reduce(rational(r.num, r.den * n));
		}
		/*
		* @throws std::invalid_argument if n = 0
		*/
		constexpr rational& operator/=(const int n)
		{
			if (n == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			r.den *= n;
			reduce();
			return *this;
		}

	public:
///////////////////////////// Observers & Producers /////////////////////////////
		constexpr AVRational av_rational() const noexcept { return r; }
		constexpr int num() const noexcept { return r.num; }
		constexpr int den() const noexcept { return r.den; }

		constexpr double to_double() const noexcept
		{
			return (double)r.num / (double)r.den;
		}

		/*
		* Reduces the rational to lowest terms.
		*/
		static constexpr rational reduce(rational r)
		{
			int gcd = std::gcd(r.r.num, r.r.den);
			return rational(r.r.num / gcd, r.r.den / gcd);
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

	// Suitable for 24,25,30,60,120 and many other common fps values.
	static constexpr rational ff_common_video_time_base(1, 600);

	// Suitable for many common audio sample rates
	static constexpr rational ff_common_audio_time_base(1, 90000);

	static constexpr int common_audio_sample_rate = 44100;

}