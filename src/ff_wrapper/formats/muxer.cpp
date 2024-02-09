#include "muxer.h"

#include "../util/ff_helpers.h"
#include "../codec/encoder.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <filesystem>

ff::muxer::muxer(const std::filesystem::path& file_path, const char* fmt_name, const char* fmt_mime_type)
	: media_base()
{
	const std::string path_str(file_path.generic_string());

	if (file_path.empty())
	{
		throw std::invalid_argument("The file path cannot be empty.");
	}

	p_muxer_desc = av_guess_format(fmt_name, path_str.c_str(), fmt_mime_type);
	if (nullptr == p_muxer_desc)
	{
		throw std::invalid_argument("The names you gave could not identify a muxer.");
	}

	internal_create_muxer(path_str);
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

void ff::muxer::internal_create_muxer(const std::string& path)
{
	p_fmt_ctx = avformat_alloc_context();

	// Assign the desc identified by the names given to the constructor
	FF_ASSERT(nullptr != p_muxer_desc, "Now the desc should be available.");
	p_fmt_ctx->oformat = p_muxer_desc;

	// Open the output file.
	FF_ASSERT(!path.empty(), "The file path cannot be empty.");
	// Although the documentation says pb must be opened by avio_open2,
	// I found in the source that avio_open = avio_open2 with the last two parameters being NULL.
	int ret = avio_open(&p_fmt_ctx->pb, path.c_str(), AVIO_FLAG_WRITE);
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

	// Make a copy of the file path and sets p_fmt_ctx->url to it.
	// The documentation requires that p_fmt_ctx->url is freeable by av_free(). Hence the copy.
	auto path_len = path.size();
	p_fmt_ctx->url = reinterpret_cast<char*>(av_malloc(path_len));
	if (nullptr == p_fmt_ctx->url) // Allocation failed.
	{
		throw std::bad_alloc();
	}
	::strcpy_s(p_fmt_ctx->url, path_len, path.c_str());
}

void ff::muxer::destroy()
{
	FF_ASSERT(nullptr != p_fmt_ctx, "Should have been created.");

	// Close the file first.
	ffhelpers::safely_free_avio_context(&p_fmt_ctx->pb);
	// Then free the fmt ctx.
	ffhelpers::safely_free_format_context(&p_fmt_ctx);
}

ff::stream ff::muxer::add_stream(const encoder& enc)
{
	FF_ASSERT(nullptr != p_fmt_ctx, "Should have been created.");

	if (ready)
	{
		throw std::runtime_error("You already prepared the muxer.");
	}

	auto ret = internal_create_stream();
	// Copy enc's properties to the new stream.
	ret.set_properties(enc.get_codec_properties());

	return ret;
}

ff::stream ff::muxer::add_stream(const stream& dem_s)
{
	FF_ASSERT(nullptr != p_fmt_ctx, "Should have been created.");

	if (ready)
	{
		throw std::runtime_error("You already prepared the muxer.");
	}

	auto ret = internal_create_stream();
	// Copy enc's ESSENTIAL properties to the new stream.
	// FFmpeg doc: It is advised to manually initialize only the relevant fields in AVCodecParameters, 
	// rather than using avcodec_parameters_copy() during remuxing: 
	// there is no guarantee that the codec context values remain valid for both input and output format contexts.
	ret.set_properties(dem_s.properties().essential_properties());

	return ret;
}

void ff::muxer::prepare_muxer(const dict& options)
{
	if (ready)
	{
		throw std::logic_error("You can only call it once.");
	}

	if (options.empty())
	{
		internal_prepare_muxer(nullptr);
	}
	else
	{
		dict cpy(options);
		auto* pavd = cpy.get_av_dict();
		internal_prepare_muxer(&pavd);
	}
}

void ff::muxer::prepare_muxer(dict& options)
{
	if (ready)
	{
		throw std::logic_error("You can only call it once.");
	}

	if (options.empty())
	{
		throw std::invalid_argument("Options cannot be empty. If you don't have any options, call the "
			"version that accepts a const dict.");
	}

	auto* pavd = options.get_av_dict();
	internal_prepare_muxer(&pavd);
	options = pavd;
}

void ff::muxer::mux_packet(packet& pkt)
{
	if (!ready)
	{
		throw std::runtime_error("You must prepare the muxer first.");
	}

	int ret = av_write_frame(p_fmt_ctx, pkt.av_packet());
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EINVAL):
			throw std::invalid_argument("The packet you gave is invalid.");
			break;
		case AVERROR(EIO):
			throw std::filesystem::filesystem_error
			(
				"Unexpected I/O error happened when muxing a packet", p_fmt_ctx->url,
				std::make_error_code(std::errc::io_error)
			);
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when muxing a packet", ret);
		}
	}
}

void ff::muxer::flush_demuxer()
{
	if (!ready)
	{
		throw std::runtime_error("You must prepare the muxer first.");
	}

	// Pass nullptr to flush the muxer.
	int ret = av_write_frame(p_fmt_ctx, nullptr);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EIO):
			throw std::filesystem::filesystem_error
			(
				"Unexpected I/O error happened when flushing the muxer.", p_fmt_ctx->url,
				std::make_error_code(std::errc::io_error)
			);
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when flushing the muxer", ret);
		}
	}
}

void ff::muxer::finalize()
{
	if (!ready)
	{
		throw std::runtime_error("You must prepare the muxer first.");
	}

	int ret = av_write_trailer(p_fmt_ctx);
	if (ret < 0)
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EIO):
			throw std::filesystem::filesystem_error
			(
				"Unexpected I/O error happened when writing file trailer", p_fmt_ctx->url,
				std::make_error_code(std::errc::io_error)
			);
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when writing file trailer", ret);
		}
	}
}

void ff::muxer::internal_prepare_muxer(::AVDictionary** ppavd)
{
	int ret = avformat_write_header(p_fmt_ctx, ppavd);
	if (ret < 0) // Failure
	{
		switch (ret)
		{
		case AVERROR(ENOMEM):
			throw std::bad_alloc();
			break;
		case AVERROR(EIO):
			throw std::filesystem::filesystem_error
			(
				"Unexpected I/O error happened when writing file header", p_fmt_ctx->url,
				std::make_error_code(std::errc::io_error)
			);
			break;
		default:
			ON_FF_ERROR_WITH_CODE("Unexpected error happened when writing file header", ret);
		}
	}

	FF_ASSERT(!ready, "Ready should not have been set.");
	ready = true;
}

ff::stream ff::muxer::internal_create_stream()
{
	auto* ps = avformat_new_stream(p_fmt_ctx, nullptr);
	if (nullptr == ps)
	{
		throw std::runtime_error("Unexpected error: Could not create a new stream.");
	}

	stream ret(ps);
	
	// Add the stream/index to the vectors for convenient access
	FF_ASSERT(streams.size() == ret->index, "Should store the streams in order.");
	streams.push_back(ret);
	switch (ret.type())
	{
	case AVMEDIA_TYPE_VIDEO:
		v_indices.push_back(ret->index);
		break;
	case AVMEDIA_TYPE_AUDIO:
		a_indices.push_back(ret->index);
		break;
	case AVMEDIA_TYPE_SUBTITLE:
		s_indices.push_back(ret->index);
		break;
	}

	return ps;
}
