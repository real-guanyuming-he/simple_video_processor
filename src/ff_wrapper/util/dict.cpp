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
#include "dict.h"

extern "C"
{
#include <libavutil/dict.h>
}

#include "ff_helpers.h"

ff::dict::dict() : p_dict(nullptr)
{
	auto ret = av_dict_set(&p_dict, nullptr, nullptr, NULL);
	if (ret < 0)
	{
		ON_FF_ERROR_WITH_CODE("Could not allocate an AVDictionary", ret);
	}
}

ff::dict::dict(const dict& other) : dict()
{
	auto ret = av_dict_copy(&p_dict, other.p_dict, NULL);
	if (ret < 0)
	{
		ON_FF_ERROR_WITH_CODE("Could not copy an AVDictionary", ret);
	}
}

ff::dict::~dict()
{
	if (nullptr != p_dict)
	{
		av_dict_free(&p_dict);
		p_dict = nullptr;
	}
}

bool ff::dict::query_entry(const std::string& key) const
{
	return false;
}

std::string ff::dict::get_value(const std::string& key) const
{
	return std::string();
}

void ff::dict::insert_entry(const std::string& key, const std::string& value)
{
}
