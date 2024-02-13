#include "muxer.h"

#include "../util/ff_helpers.h"
#include "../codec/encoder.h"

extern "C"
{
#include <libavformat/avformat.h>
}

#include <filesystem>

ff::muxer::muxer
(
	const std::filesystem::path& file_path, 
	const std::string& fmt_name, const std::string& fmt_mime_type
)
	: media_base()
{
	const std::string path_str(file_path.generic_string());

	if (file_path.empty())
	{
		throw std::invalid_argument("The file path cannot be empty.");
	}

	p_muxer_desc = av_guess_format
	(
		fmt_name.empty() ? nullptr : fmt_name.c_str(), 
		path_str.c_str(),
		fmt_mime_type.empty() ? nullptr : fmt_mime_type.c_str()
	);
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

bool ff::muxer::supports_video() const noexcept
{
	return p_muxer_desc->video_codec != AVCodecID::AV_CODEC_ID_NONE;
}

bool ff::muxer::supports_audio() const noexcept
{
	return p_muxer_desc->audio_codec != AVCodecID::AV_CODEC_ID_NONE;
}

bool ff::muxer::supports_subtitle() const noexcept
{
	return p_muxer_desc->subtitle_codec != AVCodecID::AV_CODEC_ID_NONE;
}

AVCodecID ff::muxer::desired_encoder_id(AVMediaType type) const
{
	auto ret = av_guess_codec(p_muxer_desc, nullptr, p_fmt_ctx->url, nullptr, type);
	if (AVCodecID::AV_CODEC_ID_NONE == ret)
	{
		throw std::domain_error("Could not obtain the ID for the desired encoder.");
	}

	return ret;
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
	auto path_len = path.size() + 1; // +1 for the /0.
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
		throw std::logic_error("You already prepared the muxer.");
	}

	// Copy enc's properties to the new stream.
	auto ret = internal_create_stream(enc.get_codec_properties());

	// TODO: FFmpeg/fftools/ffmpeg_mux_init.c
	// has this function of_add_attachments(), which appears to set up the extradata.
	// Investigate what it does to improve the following code.
	// Partial Investigation Result:
	//	1. in ffmpeg_opt.c, opt_attach() seems to be where the attachments are set.
	// Find out all the places this function is called.
	//	2. Debug FFmpeg in ubuntu and find out what changes the extradata.

	// This version of add_stream's being called means packets
	// will come from an encoder.
	// If so, extradata must be setup for some codecs.
	switch (enc.get_id())
	{
	case AV_CODEC_ID_H264:
		// Allocate 32 bytes and zero them all.
		codec_properties::alloc_and_zero_extradata(*ret->codecpar, 32, true);
		break;
	}

	return ret;
}

ff::stream ff::muxer::add_stream(const stream& dem_s)
{
	FF_ASSERT(nullptr != p_fmt_ctx, "Should have been created.");

	if (ready)
	{
		throw std::logic_error("You already prepared the muxer.");
	}

	// Copy enc's ESSENTIAL properties to the new stream.
	// FFmpeg doc: It is advised to manually initialize only the relevant fields in AVCodecParameters, 
	// rather than using avcodec_parameters_copy() during remuxing: 
	// there is no guarantee that the codec context values remain valid for both input and output format contexts.
	auto src_p = dem_s.properties();
	auto dst_p = src_p.essential_properties();
	// In addition, copy the extradata just to be safe.
	codec_properties::copy_extradata(dst_p, src_p);

	// Create the stream with the properties.
	auto ret = internal_create_stream(dst_p);

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

void ff::muxer::mux_packet_auto(packet& pkt)
{
	if (!ready)
	{
		throw std::logic_error("You must prepare the muxer first.");
	}

	if (manual_muxing_called)
	{
		throw std::logic_error("Do not call both mux_packet_auto() and mux_packet_manual()!");
	}

	auto_muxing_called = true;

	internal_sync_packet(pkt);

	int ret = av_interleaved_write_frame(p_fmt_ctx, pkt.av_packet());
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

void ff::muxer::mux_packet_manual(packet& pkt)
{
	if (!ready)
	{
		throw std::logic_error("You must prepare the muxer first.");
	}

	if (auto_muxing_called)
	{
		throw std::logic_error("Do not call both mux_packet_auto() and mux_packet_manual()!");
	}

	manual_muxing_called = true;

	internal_sync_packet(pkt);

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
		throw std::logic_error("You must prepare the muxer first.");
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
		throw std::logic_error("You must prepare the muxer first.");
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
	if (0 == num_streams())
	{
		throw std::logic_error("You have not added any streams.");
	}

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

ff::stream ff::muxer::internal_create_stream(const ff::codec_properties& properties)
{
	auto* ps = avformat_new_stream(p_fmt_ctx, nullptr);
	if (nullptr == ps)
	{
		throw std::runtime_error("Unexpected error: Could not create a new stream.");
	}

	stream ret(ps);

	ret.set_properties(properties);
	
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

void ff::muxer::internal_sync_packet(packet& pkt)
{
	if (last_dts != AV_NOPTS_VALUE)
	{
		// Keep dts monotonically increasing.
		if (pkt->dts == last_dts)
		{
			++pkt->dts;
			// Keep pts >= dts
			if (pkt->pts < pkt->dts)
			{
				++pkt->pts;
			}	
		}
	}

	last_dts = pkt->dts;
	last_pts = pkt->pts;
}
