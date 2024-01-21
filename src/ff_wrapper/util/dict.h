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
struct AVDictionaryEntry;

namespace ff
{
	/*
	* Represents a dictionary (map) : char* -> char*.
	* It encapsulates an AVDictionary that stores key-values pairs.
	* 
	* One particular useful improvement over AVDictionary is that 
	* the previously retrieved values from the dict do not become invalid if the dict is changed.
	* This is done by copying all out values into std::string.
	* 
	* In addition, the encapsulation does not allow multiple values for a key. Values should be concatenated 
	* into one string to be given to a key.
	*/
	class FF_WRAPPER_API dict final
	{
	public:
		/*
		* There is no way to create an empty AVDictionary,
		* so this ctor simply sets the pointer to nullptr.
		* 
		* However, the methods of this class acts as if the dict is empty,
		if p_dict is nullptr.
		*/
		dict() noexcept
			: p_dict(nullptr) {};

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
		* @returns the number of entries in the dict. Or 0 if the pointer is nullptr.
		*/
		int num() const noexcept;

		/*
		* @returns true iff the dict is empty or nullptr.
		*/
		bool empty() const noexcept { return 0 == num(); }

		/*
		* Queries the existence of an entry of the given key.
		* 
		* @param key of the entry to query
		* @returns true iff an entry with key exists in the dict or the pointer is nullptr.
		*/
		bool query_entry(const char* key) const noexcept;
		/*
		* The same as query_entry(key.c_str());
		*/
		bool query_entry(const std::string& key) const noexcept
		{
			return query_entry(key.c_str());
		}

		/*
		* Obtains the value associated with a given key.
		* 
		* @param key of the entry
		* @returns the value of the entry, 
		or an empty string if there is no such entry or the pointer is nullptr.
		*/
		std::string get_value(const char* key) const noexcept;
		/*
		* The same as get_value(key.c_str());
		*/
		std::string get_value(const std::string& key) const noexcept
		{
			return get_value(key.c_str());
		}

		/*
		* The same as insert_entry(key.c_str(), value.c_str());
		*/
		void insert_entry(const std::string& key, const std::string& value)
		{
			insert_entry(key.c_str(), value.c_str());
		}

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
		void insert_entry(const char* key, const char* value);

		/*
		* Deletes the entry of key in the dict.
		* Does nothing if there is no such entry in the dict or the pointer is nullptr.
		* 
		* @param key
		*/
		void delete_entry(const char* key) noexcept;
		/*
		* The same as delete_entry(key.c_str());
		*/
		void delete_entry(const std::string& key) noexcept
		{
			delete_entry(key.c_str());
		}

	public:
		const ::AVDictionary* get_av_dict() const noexcept { return p_dict; }

	///////////////////// Private Helpers ///////////////////////
	private:
		/*
		* Calls av_dict_get with the key and the match case flag.
		* Assumes that p_dict is not nullptr.
		* Note: Adding a new entry to a dictionary invalidates all existing entries
		previously returned with av_dict_get() or av_dict_iterate().

		* @returns the entry returned by av_dict_get. nullptr if not found.
		*/
		::AVDictionaryEntry* internal_query_entry(const char* key) const noexcept;

	private:
		::AVDictionary* p_dict;
	};
}