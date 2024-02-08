#include "muxer.h"

#include "../util/ff_helpers.h"

extern "C"
{
#include <libavformat/avformat.h>
}

ff::muxer::muxer(const char* fp, const char* fmt_name, const char* fmt_mime_type)
	: file_path(fp)
{
	if (nullptr == file_path)
	{
		throw std::invalid_argument("The file path cannot be nullptr.");
	}

	p_muxer_desc = av_guess_format(fmt_name, file_path, fmt_mime_type);
	if (nullptr == p_muxer_desc)
	{
		throw std::invalid_argument("The names you gave could not identify a muxer.");
	}

	allocate_object_memory();
}

std::string ff::muxer::description() const
{
	if (nullptr == p_muxer_desc)
	{
		throw std::logic_error("Desc is not ready.");
	}

	return std::string(p_muxer_desc->long_name);
}

std::vector<std::string> ff::muxer::short_names() const
{
	if (nullptr == p_muxer_desc)
	{
		throw std::logic_error("Desc is not ready.");
	}

	return media_base::string_to_list(std::string(p_muxer_desc->name));
}

std::vector<std::string> ff::muxer::extensions() const
{
	if (nullptr == p_muxer_desc)
	{
		throw std::logic_error("Desc is not ready.");
	}

	return media_base::string_to_list(std::string(p_muxer_desc->extensions));
}

void ff::muxer::internal_allocate_object_memory()
{
	p_fmt_ctx = avformat_alloc_context();

	// Assign the desc identified by the names given to the constructor
	FF_ASSERT(nullptr != p_muxer_desc, "Now the desc should be available.");
	p_fmt_ctx->oformat = p_muxer_desc;

	// Open the output file.
	FF_ASSERT(nullptr != file_path, "The file_path cannot be nullptr");
	// Although the documentation says pb must be opened by avio_open2,
	// I found in the source that avio_open = avio_open2 with the last two parameters being NULL.
	int ret = avio_open(&p_fmt_ctx->pb, file_path, AVIO_FLAG_WRITE);
	if (ret < 0) // Failed to open the file
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(ENOENT): // No such file or directory
			throw std::runtime_error("I intend to create a new file when the path does not exist. "
				"I don't know what the error indicates if my code is correct then.");
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error: Could not open the output file for muxer.", ret);
		}
	}
}

void ff::muxer::internal_allocate_resources_memory(uint64_t size, void* additional_information)
{
	throw std::runtime_error("Not implemented");
}

void ff::muxer::internal_release_object_memory() noexcept
{
	// Close the file first.
	ffhelpers::safely_free_avio_context(&p_fmt_ctx->pb);
	// Then free the fmt ctx.
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
