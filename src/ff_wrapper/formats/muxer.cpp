#include "muxer.h"

#include "../util/ff_helpers.h"

ff::muxer::muxer(const char* file_path, const char* fmt_name, const char* fmt_mime_type)
{
	throw std::runtime_error("Not implemented");
}

std::string ff::muxer::description() const noexcept
{
	if (nullptr == p_muxer_desc)
	{
		throw std::logic_error("Desc is not ready.");
	}

	return std::string(p_muxer_desc->long_name);
}

std::vector<std::string> ff::muxer::short_names() const noexcept
{
	if (nullptr == p_muxer_desc)
	{
		throw std::logic_error("Desc is not ready.");
	}

	return media_base::string_to_list(std::string(p_muxer_desc->name));
}

std::vector<std::string> ff::muxer::extensions() const noexcept
{
	if (nullptr == p_muxer_desc)
	{
		throw std::logic_error("Desc is not ready.");
	}

	return media_base::string_to_list(std::string(p_muxer_desc->extensions));
}

void ff::muxer::internal_allocate_object_memory()
{
	throw std::runtime_error("Not implemented");
}

void ff::muxer::internal_allocate_resources_memory(uint64_t size, void* additional_information)
{
	throw std::runtime_error("Not implemented");
}

void ff::muxer::internal_release_object_memory() noexcept
{
	ffhelpers::safely_free_format_context(&p_fmt_ctx);
}

void ff::muxer::internal_release_resources_memory() noexcept
{
	// TBD
}

void ff::muxer::mux_packet(const packet& pkt)
{
	throw std::runtime_error("Not implemented");
}

void ff::muxer::finalize()
{
	throw std::runtime_error("Not implemented");
}
