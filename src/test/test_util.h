/*
* Copyright (C) Guanyuming He 2024
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

#ifndef NDEBUG
	#ifdef _WIN32
	#define FF_DEBUG_BREAK __debugbreak();
	#else
	#define FF_DEBUG_BREAK
	#endif // _WIN32
#else
	#define FF_DEBUG_BREAK
#endif // !NDEBUG

#define TEST_ASSERT_TRUE(expr, msg)\
if (!(expr))\
{\
	std::cerr << "Test failed at line " << __LINE__ <<\
		" in file " << __FILE__ << " with message:\n" <<\
		msg;\
	FF_DEBUG_BREAK\
	return -1; /* CTest treats non-zero return values as failures. */ \
}

#define TEST_ASSERT_FALSE(expr, msg)\
if (expr)\
{\
	std::cerr << "Test failed at line " << __LINE__ <<\
		" in file " << __FILE__ << " with message:\n" <<\
		msg;\
	FF_DEBUG_BREAK\
	return -1; /* CTest treats non-zero return values as failures. */ \
}

#define TEST_ASSERT_EQUALS(expected, actual, msg)\
TEST_ASSERT_TRUE(actual == expected, msg)

#define TEST_ASSERT_THROWS(expr, exception_type)\
{\
	bool thrown = false; \
	try\
	{\
	expr; \
	}\
	catch (const exception_type& e)\
	{\
	thrown = true; \
	}\
	if (!thrown)\
	{\
	std::cerr << "Test failed at line " << __LINE__ << \
	" in file " << __FILE__ << ",\n" << \
	"which should have thrown but did not."; \
	FF_DEBUG_BREAK\
	return -1; \
	}\
}

#include <iostream>