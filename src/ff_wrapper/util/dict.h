#pragma once
/*
* Copyright (C) Guanyuming He 2024
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

#include <string>

struct AVDictionary;

namespace ff
{
	/*
	* Represents a dictionary (map) : char* -> char*.
	* It encapsulates an AVDictionary that stores key-values pairs.
	* 
	* One particular useful improvement over AVDictionary is that 
	* the previously retrieved values from the dict do not become invalid if the dict is changed.
	* This is done by copying all out values into std::string.
	*/
	class FF_WRAPPER_API dict final
	{
	public:
		/*
		* Initializes the dict to empty
		*
		* @throws std::runtime_error with message on error.
		*/
		dict();

		/*
		* Copies the content of other to this.
		*
		* @throws std::runtime_error with message on error.
		*/
		dict(const dict& other);

		/*
		* Takes over the ownership of other's AVDictionary.
		*/
		dict(dict&& other) noexcept
			: p_dict(other.p_dict)
		{
			other.p_dict = nullptr;
		}

		/*
		* Releases the dictionary and sets the pointer to nullptr
		*/
		~dict();

	public:
		/*
		* @returns the number of entries in the dict.
		*/
		int num() const noexcept;

		/*
		* @returns true iff the dict is empty.
		*/
		bool empty() const noexcept { return 0 == num(); }

		/*
		* Queries the existence of an entry of the given key.
		* 
		* @param key of the entry to query
		* @returns true iff an entry with key exists in the dict.
		*/
		bool query_entry(const std::string& key) const noexcept;

		/*
		* Obtains the value associated with a given key.
		* 
		* @param key of the entry
		* @returns the value of the entry, or an empty string if there is no such entry.
		*/
		std::string get_value(const std::string& key) const noexcept;

		/*
		* Inserts a key-value entry into the dict. If there is already an entry with 
		* the given key, the entry will be overridden with the new value.
		* 
		* The method could be const theoretically because the class only stores a pointer.
		* It is declared non-const nevertheless for consistency.
		* 
		* @param key 
		* @param value
		* @throws std::runtime_error on error.
		*/
		void insert_entry(const std::string& key, const std::string& value);

	public:
		const ::AVDictionary* get_av_dict() const noexcept { return p_dict; }

	private:
		::AVDictionary* p_dict;
	};
}