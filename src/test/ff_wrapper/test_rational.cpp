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

#include "../test_util.h"

#include "../../ff_wrapper/util/ff_math.h"

int main()
{
	// Invalid rationals
	{
		// Should not compile
		// constexpr ff::rational r(1, 0);

		// Should throw
		TEST_ASSERT_THROWS(ff::rational(2, 0), std::invalid_argument);
	}

	/*
	* All tests from now on do not involve invalid rationals (i.e. den = 0)
	*/

	// Test the construction/assignment of rationals
	{
		ff::rational z;
		TEST_ASSERT_EQUALS(0, z.get_num(), "expected to have a zero created by default.");
		TEST_ASSERT_EQUALS(1, z.get_den(), "expected to have a denominator = 1.");

		ff::rational r1(AVRational{ .num = 1, .den = 2 });
		TEST_ASSERT_EQUALS(1, r1.get_num(), "expected to have exactly the same nums");
		TEST_ASSERT_EQUALS(2, r1.get_den(), "expected to have exactly the same dens");

		ff::rational r2(3, 9);
		TEST_ASSERT_EQUALS(3, r2.get_num(), "expected to have exactly the same nums");
		TEST_ASSERT_EQUALS(9, r2.get_den(), "expected to have exactly the same dens");

		r1 = ff::rational(4, 2);
		TEST_ASSERT_EQUALS(4, r1.get_num(), "expected to have exactly the same nums");
		TEST_ASSERT_EQUALS(2, r1.get_den(), "expected to have exactly the same dens");

		r2 = AVRational{ .num = 5, .den = 7 };
		TEST_ASSERT_EQUALS(5, r2.get_num(), "expected to have exactly the same nums");
		TEST_ASSERT_EQUALS(7, r2.get_den(), "expected to have exactly the same dens");
	}

	// Test equality/inequality
	{
		// Zero
		{
			ff::rational z1(0, 1), z2(0, 12345);
			TEST_ASSERT_TRUE(z1 == z2 && z2 == z1, "Expected to be equal to each other.");

			// integer
			TEST_ASSERT_TRUE(ff::zero_rational == 0, "Expected to be equal.");
		}

		// Exactly the same numbers | coprime number inequalities
		{
			ff::rational r1(7, 3), r2(7, 3);
			TEST_ASSERT_TRUE(r1 == r2 && r2 == r1, "Expected to be equal to each other.");

			r1 = ff::rational(11, 3);
			TEST_ASSERT_TRUE(r1 != r2 && r2 != r1, "Expected not to be equal to each other.");

			r2 = ff::rational(7, 2);
			TEST_ASSERT_TRUE(r1 != r2 && r2 != r1, "Expected not to be equal to each other.");

			// integer
			ff::rational r3(17, 1);
			TEST_ASSERT_TRUE(r3 == 17, "Expected to be equal.");
			TEST_ASSERT_TRUE(r3 != 18 && r3 != 16, "Expected not to be equal.");
		}

		// Not the same numbers, but still the same value |
		// gcd != 1 inequalities
		{
			ff::rational r1(1, 3), r2(2, 6);
			TEST_ASSERT_TRUE(r1 == r2 && r2 == r1, "Expected to be equal to each other.");

			r1 = ff::rational(25, 15); r2 = ff::rational(5, 3);
			TEST_ASSERT_TRUE(r1 == r2 && r2 == r1, "Expected to be equal to each other.");

			r2 = ff::rational(10, 5);
			TEST_ASSERT_TRUE(r1 != r2 && r2 != r1, "Expected not to be equal to each other.");

			r1 = ff::rational(15, 5);
			TEST_ASSERT_TRUE(r1 != r2 && r2 != r1, "Expected not to be equal to each other.");

			// integer
			ff::rational r3(25, 5), r4(26, 5);
			TEST_ASSERT_TRUE(r3 == 5, "Expected to be equal.");
			TEST_ASSERT_TRUE(r4 != 5 && r3 != 6, "Expected not to be equal.");
		}

		// Smaller/Larger
		{
			// Zero
			ff::rational z(0, 1), p1(1, 8), n1(-2, 9);
			TEST_ASSERT_TRUE(n1 < z && z > n1, "Should be smaller than 0");
			TEST_ASSERT_TRUE(ff::zero_rational < p1 && p1 > ff::zero_rational, "Should be bigger than 0");

			// Integer
			ff::rational i1(123, 2), i2(-2, 34), i3(17, 3), i4(33, 222);
			TEST_ASSERT_TRUE(i1 < 123, "Should be smaller");
			TEST_ASSERT_TRUE(i2 > -1, "Should be bigger");
			TEST_ASSERT_TRUE(i3 > 5, "Should be bigger");
			TEST_ASSERT_TRUE(i4 < 1, "Should be smaller");
			
			// Positive
			ff::rational r1(13, 15), r2(15, 17);
			TEST_ASSERT_TRUE(r1 < r2, "Should be smaller");
			TEST_ASSERT_TRUE(r2 > r1, "Should be bigger");

			ff::rational r3(1024, 2048), r4(515, 49);
			TEST_ASSERT_TRUE(r3 < r4, "Should be smaller");
			TEST_ASSERT_TRUE(r4 > r3, "Should be bigger");

			ff::rational r5(24, 66), r6(48, 132);
			TEST_ASSERT_TRUE(r5 <= r6, "Should be equal");
			TEST_ASSERT_TRUE(r5 >= r6, "Should be equal");

			// Negative & positive
			ff::rational nr2(7, -9), nr1(-8, 9);
			TEST_ASSERT_TRUE(nr1 < nr2, "Should be smaller");
			TEST_ASSERT_TRUE(nr2 > nr1, "Should be bigger");

			ff::rational nr4(-16, -8), nr3(-2, 1);
			TEST_ASSERT_TRUE(nr3 < nr4, "Should be smaller");
			TEST_ASSERT_TRUE(nr4 > nr3, "Should be bigger");

			ff::rational nr6(-132, 111), nr5(132, -111);
			TEST_ASSERT_TRUE(nr5 <= nr6, "Should be equal");
			TEST_ASSERT_TRUE(nr5 >= nr6, "Should be equal");
		}
	}

	// reduce
	{
		// Zero
		{
			ff::rational z1(0, 2); ff::rational zcpy(z1);
			z1.reduce();
			TEST_ASSERT_TRUE(z1 == zcpy && ff::rational::reduce(zcpy) == zcpy, "Reduce should not change coprime numbers");
		}

		// Coprime numbers should not be reduced
		{
			ff::rational r1(36, 31), r2(36, 31);
			r1.reduce();
			TEST_ASSERT_TRUE(r1 == r2 && ff::rational::reduce(r2) == r2, "Reduce should not change coprime numbers");

			ff::rational r3(1024, 1025), r4(1024, 1025);
			r3.reduce();
			TEST_ASSERT_TRUE(r3 == r4 && ff::rational::reduce(r4) == r4, "Reduce should not change coprime numbers");
		
			ff::rational r5(1, 5), r6(1, 5);
			r5.reduce();
			TEST_ASSERT_TRUE(r5 == r6 && ff::rational::reduce(r6) == r6, "Reduce should not change coprime numbers");
		}

		// Numbers that can be reduced
		{
			ff::rational r1(17, 11), r2(17 * 5, 11 * 5);
			r2.reduce();

			TEST_ASSERT_TRUE(r2.get_num() == r1.get_num() && r2.get_den() == r1.get_den(), "Should be reduced to lowest terms");

			ff::rational r3(16, 99), r4(16*3*4, 99*3*4);
			r4.reduce();

			TEST_ASSERT_TRUE(r4.get_num() == r3.get_num() && r4.get_den() == r3.get_den(), "Should be reduced to lowest terms");
		}
	}

	// Addition and subtraction
	{
		// Addition
		{
			// Zero
			ff::rational z(0, 3), rz1(184, 908), rz2(999, 11);
			TEST_ASSERT_EQUALS(rz1, z + rz1, "Zero should be the additive identity.");
			TEST_ASSERT_EQUALS(rz2, rz2 + z, "Zero should be the additive identity.");

			// cannot be reduced
			ff::rational r1(3, 1), r2(4, 1);
			TEST_ASSERT_EQUALS(ff::rational(14, 2), r1 + r2, "Addition should perform correctly");

			// can be reduced
			ff::rational r3(4, 2);
			TEST_ASSERT_EQUALS(ff::rational(12, 2), r2 + r3, "Addition should perform correctly");

			// can be reduced
			ff::rational r4(25, 55);
			TEST_ASSERT_EQUALS(ff::rational(27, 11), r4 + r3, "Addition should perform correctly");
		}

		// Subtraction
		{
			// Zero
			ff::rational z(0, 3), rz1(999, 333), rz2(123, 456);
			TEST_ASSERT_EQUALS(rz1, rz1 - z, "Zero should be the additive identity.");
			TEST_ASSERT_EQUALS(rz2, rz2 - z, "Zero should be the additive identity.");

			// cannot be reduced
			ff::rational r1(7, 1), r2(4, 1);
			TEST_ASSERT_EQUALS(ff::rational(3, 1), r1 - r2, "Subtraction should perform correctly");

			// can be reduced
			ff::rational r3(5, 10);
			TEST_ASSERT_EQUALS(ff::rational(13, 2), r1 - r3, "Subtraction should perform correctly");

			// can be reduced
			ff::rational r4(64, 1024);
			TEST_ASSERT_EQUALS(ff::rational(7, 16), r3 - r4, "Addition should perform correctly");
		}
	}

	// Multiplication and Division
	{
		// Multiplication
		{
			// Zero
			ff::rational z, r1(80124, 2881);
			TEST_ASSERT_EQUALS(z, z * r1, "Multiplication should perform correctly");

			// One
			ff::rational one(1, 1), r2(10931, 9572);
			TEST_ASSERT_EQUALS(r2, one * r2, "Multiplication should respect the multiplicative identity.");

			// Cannot be reduced
			ff::rational r3(17, 13), r4(5, 3);
			TEST_ASSERT_EQUALS(ff::rational(85, 39), r3 * r4, "Multiplication should perform correctly");

			// Can be reduced
			ff::rational r5(999, 666), r6(222, 444);
			TEST_ASSERT_EQUALS(ff::rational(3, 4), r5 * r6, "Multiplication should perform correctly");
		}

		// Division
		{
			// Zero
			ff::rational z, r1(1, 2);
			TEST_ASSERT_THROWS(r1 / z, std::invalid_argument);

			// One
			ff::rational one(1, 1), r2(3, 17);
			TEST_ASSERT_EQUALS(r2, r2 / one, "Division should respect the multiplicative identity.");

			// Cannot be reduced
			ff::rational r3(68, 23), r4(222, 1);
			TEST_ASSERT_EQUALS(ff::rational(34, 2553), r3 / r4, "Multiplication should perform correctly");

			// Can be reduced
			ff::rational r5(600, 21), r6(17, 34);
			TEST_ASSERT_EQUALS(ff::rational(400, 7), r5 / r6, "Multiplication should perform correctly");
		}
	}

	// Cross-type operations
	{
		ff::rational_64 big1(1e9, 10);
		ff::rational big2(1e8, 1);

		TEST_ASSERT_EQUALS(ff::rational_64(1e16, 1), big1 * big2, "Should not have overflow and should have the correct result.");
		TEST_ASSERT_EQUALS(ff::rational_64(1e16, 1), big2 * big1, "Should not have overflow and should have the correct result.");
		
		ff::rational_64 big3(1e16, 1);
		ff::rational big4(1e5, 1e2);
		TEST_ASSERT_EQUALS(ff::rational_64(1e13, 1), big3 / big4, "Should not overflow and should be correct");
		TEST_ASSERT_EQUALS(ff::rational_64(1, 1e13), big4 / big3, "Should not overflow and should be correct");
	}

	// Test rounding
	{
		// Is an integer
		ff::rational r1(1, 1);
		TEST_ASSERT_EQUALS(1, r1.to_int64(), "should be equal if the rational is also an integer");

		ff::rational r2(4, 2);
		TEST_ASSERT_EQUALS(2, r2.to_int64(), "should be equal if the rational is also an integer");

		// Positive, decimal < .5
		ff::rational r3(1, 3);
		TEST_ASSERT_EQUALS(0, r3.to_int64(), "should be rounded to the nearest");

		ff::rational r4(8, 6);
		TEST_ASSERT_EQUALS(1, r4.to_int64(), "should be rounded to the nearest");

		// Positive, decimal = .5
		ff::rational r5(1, 2);
		TEST_ASSERT_EQUALS(1, r5.to_int64(), "should be rounded away from 0 when decimal is half");

		ff::rational r6(20, 8);
		TEST_ASSERT_EQUALS(3, r6.to_int64(), "should be rounded away from 0 when decimal is half");

		// Positive, decimal > .5
		ff::rational r7(8, 5);
		TEST_ASSERT_EQUALS(2, r7.to_int64(), "should be rounded to the nearest");

		ff::rational r8(793, 5);
		TEST_ASSERT_EQUALS(159, r8.to_int64(), "should be rounded to the nearest");

		// Negative, decimal > -.5
		ff::rational r9(-1, 7);
		TEST_ASSERT_EQUALS(0, r9.to_int64(), "should be rounded to the nearest");

		ff::rational r10(-30, 13);
		TEST_ASSERT_EQUALS(-2, r10.to_int64(), "should be rounded to the nearest");

		// Negative, decimal = .5
		ff::rational r11(-1, 2);
		TEST_ASSERT_EQUALS(-1, r11.to_int64(), "should be rounded away from 0 when decimal is half");

		ff::rational r12(-21, 6);
		TEST_ASSERT_EQUALS(-4, r12.to_int64(), "should be rounded away from 0 when decimal is half");

		// Negative, decimal < -.5
		ff::rational r13(-13, 7);
		TEST_ASSERT_EQUALS(-2, r13.to_int64(), "should be rounded to the nearest");

		ff::rational r14(-821, 3);
		TEST_ASSERT_EQUALS(-274, r14.to_int64(), "should be rounded to the nearest");
	}

	return 0;
}