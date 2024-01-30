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
#include "dict.h"
#include "ff_helpers.h"

extern "C"
{
#include <libavutil/dict.h>
}

#include "ff_helpers.h"
#include <stdexcept>

ff::dict::dict(const dict& other) : dict()
{
	if (nullptr == other.p_dict)
	{
		p_dict = nullptr;
		return;
	}

	auto ret = av_dict_copy(&p_dict, other.p_dict, NULL);
	if (ret < 0)
	{
		ON_FF_ERROR_WITH_CODE("Could not copy an AVDictionary", ret);
	}
}

ff::dict::~dict()
{
	ffhelpers::safely_free_dict(&p_dict);
}

ff::dict& ff::dict::operator=(dict&& right) noexcept
{
	this->operator=(right.p_dict);
	return *this;
}

ff::dict& ff::dict::operator=(::AVDictionary* dict) noexcept
{
	ffhelpers::safely_free_dict(&p_dict);
	p_dict = dict;
	return *this;
}

int ff::dict::num() const noexcept
{
	if (nullptr == p_dict)
	{
		return 0;
	}
	return av_dict_count(p_dict);
}

bool ff::dict::query_entry(const char* key) const noexcept
{
	if (nullptr == p_dict)
	{
		return false;
	}

	const auto* entry = internal_query_entry(key);
	return nullptr != entry;
}

std::string ff::dict::get_value(const char* key) const noexcept
{
	if (nullptr == p_dict)
	{
		return std::string();
	}

	const AVDictionaryEntry* entry = internal_query_entry(key);
	if (nullptr == entry)
	{
		return std::string();
	}
	return std::string(entry->value);
}

void ff::dict::insert_entry(const char* key, const char* value)
{
	auto ret = av_dict_set
	(
		&p_dict,
		key,
		value,
		AV_DICT_MATCH_CASE
	);
	if (ret < 0)
	{
		ON_FF_ERROR_WITH_CODE("Could not set an AVDictionary", ret);
	}
}

void ff::dict::delete_entry(const char* key) noexcept
{
	if (nullptr == p_dict)
	{
		return;
	}

	av_dict_set
	(
		&p_dict,
		key,
		nullptr,
		AV_DICT_MATCH_CASE
	);
}

AVDictionaryEntry* ff::dict::internal_query_entry(const char* key) const noexcept
{
	return av_dict_get
	(
		p_dict,
		key,
		nullptr,
		AV_DICT_MATCH_CASE // Match case
	);
}
