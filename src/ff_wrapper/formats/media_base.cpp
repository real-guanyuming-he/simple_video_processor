#include "media_base.h"

extern "C"
{
#include <libavformat/avformat.h>
}

std::vector<std::string> ff::media_base::string_to_list(const std::string& str, char separator)
{
	std::vector<std::string> ret;

	std::string::size_type start_ind = 0;
	std::string::size_type end_ind = 0;

	while (true)
	{
		end_ind = str.find_first_of(',', start_ind);

		// No more comma can be found.
		if (end_ind == std::string::npos)
		{
			// if the substring is not empty, then add it
			if (start_ind != str.size())
			{
				ret.emplace_back(str.substr(start_ind));
			}
			break;
		}

		// if the substring is not a comma only, then add it
		if (start_ind != end_ind)
		{
			ret.emplace_back(str.substr(start_ind, end_ind - start_ind));
		}
		// repeat from behind the ,
		start_ind = end_ind + 1;
	}

	return ret;
}

const char* ff::media_base::get_file_path() const noexcept
{
	return p_fmt_ctx->url;
}

int ff::media_base::num_streams() const noexcept
{
	return p_fmt_ctx->nb_streams;
}

ff::time ff::media_base::start_time() const noexcept
{
	return time(p_fmt_ctx->start_time, ff::av_time_base);
}

int64_t ff::media_base::bit_rate() const noexcept
{
	return p_fmt_ctx->bit_rate;
}

ff::time ff::media_base::duration() const noexcept
{
	return time(p_fmt_ctx->duration, ff::av_time_base);
}
