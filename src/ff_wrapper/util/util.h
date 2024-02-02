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
* This file defines utility things.
* If a file in the library needs these things, it should include this file.
*/

// Creating a dynamic library on different platforms require different attribute settings
// These macros handle that.
// Put FF_WRAPPER_EXPORT before all APIs that are to be exposed in this dynamic library.
// 
// I originally only export non-inline functions. However, after some thought,
// I instead export some inline functions as well (In particular, for most classes (to be exported) 
// that has both inline and non-inline methods, I export the whole class). 
// I believe this cound be useful for using the DLL from a non-C++ lanaguage.
#ifdef _WIN32

	// On Windows, additional dll attributes needs to be specified.
	// FF_WRAPPER_EXPORT's being defined or not indicates whethers symbols are to be exported/imported
	#ifdef FF_WRAPPER_EXPORT
	#define FF_WRAPPER_API __declspec(dllexport)
	#else
	#define FF_WRAPPER_API __declspec(dllimport)
	#endif // FF_WRAPPER_EXPORT

#else
	#define FF_WRAPPER_API
#endif // _WIN32

// My own assertion framework. The built-in assert() in cassert
// cannot accept an argument for message.
// In addition, I want to detect assertions during testing.

// First, define a convenient debug break macro
// that will break for me inside a debugger during testing.
// Especially useful if I can't easily reproduce some shit that happens.
#ifndef NDEBUG
	#ifdef _WIN32
	#define FF_DEBUG_BREAK __debugbreak();
	#else
	#define FF_DEBUG_BREAK
	#endif // _WIN32
#else
	#define FF_DEBUG_BREAK
#endif // !NDEBUG

// Define the assert macro if NDEBUG is not defined
// Therefore, do not use assert for errors that may be caused by users of the library.
// Use exceptions instead.
#ifndef NDEBUG

#include <fstream> // For accessing assertion log files
#include <ctime> // To log time

static constexpr const auto FF_ASSERTION_LOG_FILE_NAME = "ff_assertion_log.log";

#ifndef FF_TESTING // Normal assertion
	#define FF_ASSERT(expr, msg)\
	if (!(expr))\
	{\
		auto cur_time = std::time(nullptr);\
		std::ofstream log_file(FF_ASSERTION_LOG_FILE_NAME, std::ios::app | std::ios::out);\
		log_file << std::asctime(std::localtime(&cur_time)) <<\
			" Assertion failed at line " << __LINE__ <<\
			" in file " << __FILE__ << " with message:\n\t" <<\
			msg;\
		log_file.flush();\
		log_file.close();\
		std::terminate();\
	}
#else // Assertion during testing
	#define FF_ASSERT(expr, msg)\
	if (!(expr))\
	{\
		auto cur_time = std::time(nullptr);\
		std::ofstream log_file(FF_ASSERTION_LOG_FILE_NAME, std::ios::app | std::ios::out);\
		std::cerr << std::asctime(std::localtime(&cur_time)) << " [TESTING] "\
			"Assertion failed at line " << __LINE__ <<\
			" in file " << __FILE__ << " with message:\n\t" <<\
			msg;\
		log_file.flush();\
		log_file.close();\
		FF_DEBUG_BREAK\
		exit(-1);/* CTest treats non-zero return values as failures. */\
	}
#endif // !FF_TESTING

#else
	#define FF_ASSERT(expr, msg)
#endif // !NDEBUG

// FF_ASSERT may throw. Need a constexpr to indicate noexcept when assertion is not enabled (e.g. Release build)
#ifdef NDEBUG
#define FF_ASSERTION_ENABLED false
#define FF_ASSERTION_DISABLED true
#else
#define FF_ASSERTION_ENABLED true
#define FF_ASSERTION_DISABLED false
#endif // NDEBUG

// Will be used to turn path macros that I predefine by CMake into string literals.
#ifndef FF_STRINGIFY
	#define FF_STRINGIFY(x) #x
#endif // !FF_STRINGIFY

// Used to expand the macro before stringifying it.
#ifndef FF_EXPAND_STRINGIFY
#define FF_EXPAND_STRINGIFY(x) FF_STRINGIFY(x)
#endif // !FF_EXPAND_STRINGIF
