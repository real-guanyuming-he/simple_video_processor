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

#include "stream.h"

extern "C"
{
#include <libavformat/avformat.h>
}

bool ff::stream::is_video() const noexcept
{
    return AVMEDIA_TYPE_VIDEO == p_stream->codecpar->codec_type;
}

bool ff::stream::is_audio() const noexcept
{
    return AVMEDIA_TYPE_AUDIO == p_stream->codecpar->codec_type;
}

bool ff::stream::is_subtitle() const noexcept
{
    return AVMEDIA_TYPE_SUBTITLE == p_stream->codecpar->codec_type;
}

ff::time ff::stream::duration() const noexcept
{
    // If the time base is not fixed, this value may be nonpositive.
    auto b = time_base();
    if (b <= ff::zero_rational)
    {
        return ff::time();
    }

    return ff::time(p_stream->duration, time_base());
}

ff::rational ff::stream::time_base() const noexcept
{
    return ff::rational(p_stream->time_base);
}

const int ff::stream::codec_id() const noexcept
{
    return int(p_stream->codecpar->codec_id);
}