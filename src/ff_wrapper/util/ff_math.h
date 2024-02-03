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
* Classes:
* rational: constexpr wrapper for AVRational.
* time: records both the time and the time base.
* 
* Header only. Functions are mostly constexpr.
*/

extern "C"
{
#include <libavutil/rational.h>
}

#include <type_traits> // for type traits
#include <numeric> // for gcd
#include <stdexcept> // for std::invalid_argument

namespace ff
{
	/*
	* Represents a rational number.
	* I aim to provide two versions, one using int as num and den,
	* and another using int64_t as num and den.
	* Providing the second version is to prevent data overflow
	* when an int64_t timestamp is multiplied with an int rational
	* 
	* av_..._q() functions are not constexpr. This class
	* make all such common operations constexpr by rewriting them.
	* 
	* No need to be DLL imported/exported, because this is a template
	* and header only.
	* 
	* Invariant: den != 0.
	* Established by constructors and assignment operators.
	* Maintained by
	*  1. arithmetic operators (Rationals is closed under +,-,*, and / detects 0).
	*  2. the only mutator, reduce(), that reduces one to lowest terms.
	*/
	template <typename T>
	requires std::same_as<T, int> || std::same_as<T, int64_t>
	class rational_temp final
	{
	public:
///////////////////////////// Default things /////////////////////////////
		~rational_temp() = default;

		constexpr rational_temp(const rational_temp&) noexcept = default;
		constexpr rational_temp& operator=(const rational_temp&) noexcept = default;

	public:
		/*
		* Initializes a zero (0/1).
		*/
		constexpr rational_temp() noexcept
			: num(0), den(1) {}

		/*
		* Initializes directly from an AVRational
		* 
		* @param avr
		* @throws std::invalid_argument if avr.den = 0
		*/
		constexpr rational_temp(AVRational avr)
			: num(avr.num), den(avr.den)
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
		constexpr rational_temp(T n, T d)
			: num(n), den(d)
		{
			if (d == 0)
			{
				throw std::invalid_argument("The denominator cannot be 0.");
			}
		}

		/*
		* @throws std::invalid_argument if den = 0
		*/
		constexpr rational_temp& operator=(const AVRational avr)
		{
			if (avr.den == 0)
			{
				throw std::invalid_argument("The denominator cannot be 0.");
			}

			num = avr.num;
			den = avr.den;
			return *this;
		}

	public:
///////////////////////////// Arithmetic operators /////////////////////////////
		constexpr bool operator==(const rational_temp right) const noexcept
		{
			// b,d != 0 -> (a/b = c/d <-> ad = bc)  
			return num * right.den == den * right.num;
		}
		constexpr bool operator==(const AVRational right) const noexcept
		{
			// b,d != 0 -> (a/b = c/d <-> ad = bc)  
			return num * right.den == den * right.num;
		}
		constexpr bool operator==(const int64_t right) const noexcept
		{
			// c != 0 -> a = b/c <-> ac = b
			return right * den == num;
		}
		constexpr bool operator!=(const rational_temp right) const noexcept
		{
			// b,d != 0 -> (a/b != c/d <-> ad != bc)  
			return num * right.den != den * right.num;
		}
		constexpr bool operator!=(const AVRational right) const noexcept
		{
			// b,d != 0 -> (a/b != c/d <-> ad != bc)  
			return num * right.den != den * right.num;
		}
		constexpr bool operator!=(const int64_t right) const noexcept
		{
			// c != 0 -> a != b/c <-> ac != b
			return right * den != num;
		}

		constexpr bool operator<(const rational_temp right) const noexcept
		{
			// bd > 0 -> ad < bc
			// bd < 0 -> ad > bc
			if (den * right.den > 0)
			{
				return num * right.den < den * right.num;
			}
			else
			{
				return num * right.den > den * right.num;
			}
		}
		constexpr bool operator<(const T right) const noexcept
		{
			// b > 0 -> a < bc
			// b < 0 -> a > bc
			if (den > 0)
			{
				return num < den * right;
			}
			else
			{
				return num > den * right;
			}
		}
		constexpr bool operator>(const rational_temp right) const noexcept
		{
			// bd > 0 -> ad > bc
			// bd < 0 -> ad < bc
			if (den * right.den > 0)
			{
				return num * right.den > den * right.num;
			}
			else
			{
				return num * right.den < den * right.num;
			}
		}
		constexpr bool operator>(const T right) const noexcept
		{
			// b > 0 -> a > bc
			// b < 0 -> a < bc
			if (den > 0)
			{
				return num > den * right;
			}
			else
			{
				return num < den * right;
			}
		}

		constexpr bool operator<=(const rational_temp right) const noexcept
		{
			// bd > 0 -> ad <= bc
			// bd < 0 -> ad >= bc
			if (den * right.den > 0)
			{
				return num * right.den <= den * right.num;
			}
			else
			{
				return num * right.den >= den * right.num;
			}
		}
		constexpr bool operator<=(const T right) const noexcept
		{
			// b > 0 -> a <= bc
			// b < 0 -> a >= bc
			if (den > 0)
			{
				return num <= den * right;
			}
			else
			{
				return num >= den * right;
			}
		}
		constexpr bool operator>=(const rational_temp right) const noexcept
		{
			// bd > 0 -> ad >= bc
			// bd < 0 -> ad <= bc
			if (den * right.den > 0)
			{
				return num * right.den >= den * right.num;
			}
			else
			{
				return num * right.den <= den * right.num;
			}
		}
		constexpr bool operator>=(const T right) const noexcept
		{
			// b > 0 -> a >= bc
			// b < 0 -> a <= bc
			if (den > 0)
			{
				return num >= den * right;
			}
			else
			{
				return num <= den * right;
			}
		}

		constexpr rational_temp operator+(const rational_temp right) const noexcept
		{
			// a/b + c/d = (ad+bc)/bd
			return reduce
			(rational_temp(
				num * right.den + den * right.num,
				den * right.den
			));
		}
		constexpr rational_temp operator-(const rational_temp right) const noexcept
		{
			// a/b + c/d = (ad-bc)/bd
			return reduce
			(rational_temp(
				num * right.den - den * right.num,
				den * right.den
			));
		}

		constexpr rational_temp operator*(const rational_temp right) const noexcept
		{
			// a/b * c/d = ac/bd
			return reduce
			(rational_temp(
				num * right.num,
				den * right.den
			));
		}

		template <typename U>
		constexpr rational_temp<int64_t> operator*(const rational_temp<U> right) const noexcept
		{
			int64_t a = num, b = den, c = right.get_num(), d = right.get_den();
			return rational_temp<int64_t>::reduce
			(rational_temp<int64_t>(
				a * c,
				b * d
			));
		}

		constexpr rational_temp operator*(const T n) const noexcept
		{
			return reduce(rational_temp(num * n, den));
		}
		constexpr rational_temp& operator*=(const T n) noexcept
		{
			num *= n;
			reduce();
			return *this;
		}

		/*
		* @throws std::invalid_argument if right = 0
		*/
		constexpr rational_temp operator/(const rational_temp right) const
		{
			if (right.num == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			// a/b / c/d = ad/bc
			return reduce
			(rational_temp(
				num * right.den,
				den * right.num
			));
		}
		/*
		* @throws std::invalid_argument if right = 0
		*/
		template <typename U>
		constexpr rational_temp<int64_t> operator/(const rational_temp<U> right) const
		{
			if (right.get_num() == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			int64_t a = num, b = den, c = right.get_num(), d = right.get_den();
			return rational_temp<int64_t>::reduce
			(rational_temp<int64_t>(
				a * d,
				b * c
			));
		}

		/*
		* @throws std::invalid_argument if n = 0
		*/
		constexpr rational_temp operator/(const T n) const
		{
			if (n == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			return reduce(rational_temp(num, den * n));
		}
		/*
		* @throws std::invalid_argument if n = 0
		*/
		constexpr rational_temp& operator/=(const T n)
		{
			if (n == 0)
			{
				throw std::invalid_argument("Cannot divide by 0.");
			}

			den *= n;
			reduce();
			return *this;
		}

	public:
///////////////////////////// Observers & Producers /////////////////////////////
		constexpr AVRational av_rational() const noexcept 
		{
			return AVRational
			{
				.num = static_cast<int>(num),
				.den = static_cast<int>(den)
			};
		}
		constexpr T get_num() const noexcept { return num; }
		constexpr T get_den() const noexcept { return den; }

		constexpr double to_double() const noexcept
		{
			return (double)num / (double)den;
		}

		/*
		* Round the rational to nearest integer (with half away from zero).
		* To round the rational towards 0, simply cast to_double() to int64_t.
		*/
		inline constexpr int64_t to_int64() const noexcept
		{
			// constexpr since C++ 23
			// return std::llround(to_double());
			double db = to_double();
			int64_t db_rounded_towards_0 = static_cast<int64_t>(db);
			double decimal = db - static_cast<double>(db_rounded_towards_0);

			if (decimal == .0)
			{
				return db_rounded_towards_0;
			}
			else if (decimal > .0)
			{
				if (decimal < .5)
				{
					return db_rounded_towards_0;
				}
				else
				{
					return db_rounded_towards_0 + 1;
				}
			}
			else // decimal < 0
			{
				if (decimal > -.5)
				{
					return db_rounded_towards_0;
				}
				else
				{
					return db_rounded_towards_0 - 1;
				}
			}
		}

		/*
		* Reduces the rational to lowest terms.
		*/
		static constexpr rational_temp reduce(rational_temp r)
		{
			auto gcd = std::gcd(r.num, r.den);
			return rational_temp(r.num / gcd, r.den / gcd);
		}
	public:
///////////////////////////// Mutators /////////////////////////////
		/*
		* Reduces itself to lowest terms
		*/
		constexpr void reduce()
		{
			auto gcd = std::gcd(num, den);
			num /= gcd; den /= gcd;
		}

	private:
		// numerator
		T num;
		// denominator
		T den;
	};

	using rational = ff::rational_temp<int>;
	using rational_64 = ff::rational_temp<int64_t>;

	// A zero constant.
	static constexpr rational zero_rational(0, 1);
	static constexpr rational_64 zero_rational_64(0, 1);
}

constexpr ff::rational operator*(int n, ff::rational r) noexcept
{
	r *= n;
	return r;
}
constexpr ff::rational operator/(int n, ff::rational r) noexcept
{
	r /= n;
	return r;
}
constexpr ff::rational_64 operator*(int64_t n, ff::rational_64 r) noexcept
{
	r *= n;
	return r;
}
constexpr ff::rational_64 operator/(int64_t n, ff::rational_64 r) noexcept
{
	r /= n;
	return r;
}