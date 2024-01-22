#include "demuxer.h"

#include "../util/ff_helpers.h"

#include <filesystem> // For std::filesystem::filesystem_error
using filesystem_error = std::filesystem::filesystem_error;

extern "C"
{
#include "libavformat/avformat.h"
}

ff::demuxer::demuxer(const char* path, const dict& options)
	: file_path(path), p_fmt_ctx(nullptr)
{
	if (nullptr == path)
	{
		throw std::invalid_argument("Path cannot be nullptr");
	}

	// Open the file
	int ret = 0;
	if (options.empty())
	{
		ret = avformat_open_input(&p_fmt_ctx, path, nullptr, nullptr);
	}
	else
	{
		// avformat_open_input will change options nonetheless, so I will make a copy.
		dict cpy(options); auto* cpy_avd = cpy.get_av_dict();
		ret = avformat_open_input(&p_fmt_ctx, path, nullptr, &cpy_avd);
	}

	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOENT): // No such file.
			throw filesystem_error
			(
				"The file specified by path does not exist.", path,
				std::make_error_code(std::errc::no_such_file_or_directory)
			);
			break;
		case AVERROR(ENOMEM): // No memory
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL): // Invalid argument
			throw std::runtime_error("Unexpected error happened during a call to avformat_open_input(): Invalid argument.");
			break;
		default: // ?
			throw std::runtime_error("Unexpected error happened during a call to avformat_open_input(): Unknown error.");
		}
	}
}

ff::demuxer::demuxer(const char* path, dict& options)
	: file_path(path), p_fmt_ctx(nullptr)
{
	if (nullptr == path)
	{
		throw std::invalid_argument("Path cannot be nullptr");
	}
	if (options.empty())
	{
		throw std::invalid_argument("options cannot be empty");
	}

	// Open the file
	AVDictionary* avd = options.get_av_dict();
	int ret = avformat_open_input(&p_fmt_ctx, path, nullptr, &avd);

	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOENT): // No such file.
			throw filesystem_error
			(
				"The file specified by path does not exist.", path,
				std::make_error_code(std::errc::no_such_file_or_directory)
			);
			break;
		case AVERROR(ENOMEM): // No memory
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL): // Invalid argument
			throw std::runtime_error("Unexpected error happened during a call to avformat_open_input(): Invalid argument.");
			break;
		default: // ?
			throw std::runtime_error("Unexpected error happened during a call to avformat_open_input(): Unknown error.");
		}
	}

	// Set options arg to the options that were not used.
	options = avd;
}

ff::demuxer::~demuxer()
{
	ffhelpers::safely_close_input_format_context(&p_fmt_ctx);
	file_path = nullptr;
}
