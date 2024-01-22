#include "media_base.h"

extern "C"
{
#include <libavformat/avformat.h>
}

std::vector<const char*> ff::media_base::string_to_list(const char* str)
{
	throw std::exception("Not implemented");
}

const char* ff::media_base::get_file_path() const noexcept
{
	return p_fmt_ctx->url;
}

int ff::media_base::num_streams() const noexcept
{
	return p_fmt_ctx->nb_streams;
}

int64_t ff::media_base::start_time() const noexcept
{
	return p_fmt_ctx->start_time;
}

int64_t ff::media_base::bit_rate() const noexcept
{
	return p_fmt_ctx->bit_rate;
}

int64_t ff::media_base::duration() const noexcept
{
	return p_fmt_ctx->duration;
}
