#pragma once
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

#include "media_base.h"
#include "stream.h"
#include "../util/dict.h"

#include <string>

struct AVOutputFormat;
namespace std { namespace filesystem { class path; } }

namespace ff
{
	class packet;
	class encoder;

	/*
	* A muxer of this class muxes packets into a local file.
	* 
	* Muxer states:
	*	1. Can only be constructed if you give a path to the output file.
	*	2. After that you fill all additional information. Mostly you create the streams.
	*	3. Then call prepare_muxer() to make it ready. The method also writes the file header.
	* 
	* Implementation Notes:
	* I originally wanted to derive it from ff_object since after construction it's created 
	* until it's made ready by prepare_muxer().
	* However, I discovered that that state machine is too much because a muxer will never
	* exit the ready state.
	*/
	class FF_WRAPPER_API muxer : public media_base
	{
	public:
		/*
		* You must provide a path to the output file.
		*/
		muxer() = delete;

		/*
		* Creates a muxer with the file path.
		* The muxer's format will be deduced from the file extension.
		* You can additionally provide the format name/mime type for better identification of the muxer.
		* Note: fmt_name provides decisive identification and mime_type has higher preference 
		* than file_path does. Therefore, if you provided any of the latter two and they do not agree with
		* what your path suggests, the final format will not be the one suggested by the file path.
		* The muxer will be CREATED after the call.
		* 
		* @param file_path path to the file.
		* @param fmt_name optional. Unique format name used in FFmpeg for the format.
		* @param fmt_mime_type optional. MIME type for the format.
		* @throws std::invalid_argument if the file_path is nullptr.
		* @throws std::invalid_argument if the names together provide no information on the format.
		*/
		explicit muxer
		(
			const std::filesystem::path& file_path, 
			const std::string& fmt_name = "",
			const std::string& fmt_mime_type = ""
		);

		/*
		* Destroys the muxer completely.
		*/
		~muxer() { destroy(); }

////////////////////////// Inherited via media_base //////////////////////////
	public:
		std::string description() const override;
		std::vector<std::string> short_names() const override;
		std::vector<std::string> extensions() const override;

////////////////////////// Muxing related //////////////////////////
	public:
		/*
		* After construction, call this to add stream info to the muxer
		* before you call prepare_muxer().
		* 
		* @param enc which encoder is used to encode the packets for the stream.
		* All properties will be copied to the stream.
		* @returns A ref to the stream added. You can add more information to the stream
		* through the return value.
		* @throws std::logic_error if you already called prepare_muxer();
		*/
		stream add_stream(const encoder& enc);

		/*
		* After construction, call this to add stream info to the muxer
		* before you call prepare_muxer().
		* 
		* This version is for remuxing. It determines the properties of 
		* an output stream by those of an input stream from a demuxer.
		* 
		* @param dem_s a stream from a demuxer. 
		* ONLY Essential properties of the demuxer stream will be copied to the new stream.
		* This behaviour is recommended by FFmpeg documentation.
		* The definition of essential properties are given in the comment for
		* codec_properties::essential_properties().
		* @returns A ref to the stream added. You can add more information to the stream
		* through the return value.
		* @throws std::logic_error if you already called prepare_muxer();
		*/
		stream add_stream(const stream& dem_s);

		/*
		* After you have filled the stream info through add_stream(),
		* call this the prepare the muxer for muxing.
		* 
		* Once it's called, the muxer enters the ready state.
		* A muxer will never exit the ready state until destroyed.
		* The ready state can only be entered from here.
		* 
		* @param options The options to give the muxer. May be empty.
		* @throws std::logic_error if you have not added any stream.
		* @throws std::logic_error if you have already called it.
		* @throws std::filesystem::filesystem_error on I/O error.
		*/
		void prepare_muxer(const dict& options = dict());

		/*
		* After you have filled the stream info through add_stream(),
		* call this the prepare the muxer for muxing.
		*
		* Once it's called, the muxer enters the ready state.
		* A muxer will never exit the ready state until destroyed.
		* The ready state can only be entered from here.
		*
		* @param options The options to give the muxer. Cannot be empty.
		* On success the argument will be replaced with a dict of options that were unused.
		* @throws std::invalid_argument if options is empty
		* @throws std::logic_error if you have not added any stream.
		* @throws std::logic_error if you have already called it.
		* @throws std::filesystem::filesystem_error on I/O error.
		*/
		void prepare_muxer(dict& options);

		/*
		* Mux a packet into the file.
		* 
		* @param pkt the packet to be fed into the muxer. It must be properly set up.
		* You can call packet::prepare_for_muxing() and pass the corresponding stream to set it up,
		* or do what the comment for that method says. The packets must also be coming with increasing dts, 
		* except in rare situations the dts can be non-decreasing. This parameter is nonconst because 
		* the underlying FFmpeg function takes a nonconst pointer.
		* @throws std::invalid_argument if the packet is invalid.
		* @throws std::logic_error if you have not prepared the demuxer yet.
		* @throws std::filesystem::filesystem_error on I/O error.
		*/
		void mux_packet(packet& pkt);

		/*
		* Flush any data buffered immediately to the output file.
		* @throws std::logic_error if you have not prepared the demuxer yet (i.e. muxing has not started yet).
		* @throws std::filesystem::filesystem_error on I/O error.
		*/
		void flush_demuxer();

		/*
		* After you have no packets to give, you MUST call this method
		* to finalize the muxing.
		* 
		* @throws std::logic_error if you have not called prepare_muxer();
		* @throws std::filesystem::filesystem_error on I/O error.
		*/
		void finalize();

////////////////////////// Stream related //////////////////////////
	public:
		/*
		* @returns the number of streams you have created.
		*/
		int num_streams() const noexcept { return streams.size(); }

		/*
		* @returns the stream specified by the index.
		* @throws std::out_of_range if the index is out of range
		*/
		inline stream get_stream(int index) const
		{
			if (index < 0 || index >= num_streams())
			{
				throw std::out_of_range("Stream index out of range.");
			}

			return streams[index];
		}

		int num_videos() const noexcept { return v_indices.size(); }
		int num_audios() const noexcept { return a_indices.size(); }
		int num_subtitles() const noexcept { return s_indices.size(); }

		/*
		* @returns the stream index of the i^th video stream.
		* @throws std::std::out_of_range if the index is out of range
		*/
		inline int get_video_ind(int i) const
		{
			if (i < 0 || i >= num_videos())
			{
				throw std::out_of_range("Stream index out of range.");
			}

			return v_indices[i];
		}
		/*
		* @returns the stream index of the i^th audio stream.
		* @throws std::std::out_of_range if the index is out of range
		*/
		inline int get_audio_ind(int i) const
		{
			if (i < 0 || i >= num_audios())
			{
				throw std::out_of_range("Stream index out of range.");
			}

			return a_indices[i];
		}
		/*
		* @returns the stream index of the i^th subtitle stream.
		* @throws std::std::out_of_range if the index is out of range
		*/
		inline int get_subtitle_ind(int i) const
		{
			if (i < 0 || i >= num_subtitles())
			{
				throw std::out_of_range("Stream index out of range.");
			}

			return s_indices[i];
		}
		/*
		* @returns the i^th video stream.
		* @throws std::invalid_argument if the index is out of range
		*/
		inline stream get_video(int i) const
		{
			return streams[get_video_ind(i)];
		}
		/*
		* @returns the i^th audio stream.
		* @throws std::invalid_argument if the index is out of range
		*/
		inline stream get_audio(int i) const
		{
			return streams[get_audio_ind(i)];
		}
		/*
		* @returns the i^th subtitle stream.
		* @throws std::invalid_argument if the index is out of range
		*/
		inline stream get_subtitle(int i) const
		{
			return streams[get_subtitle_ind(i)];
		}

////////////////////////// Other methods //////////////////////////
	public:
		/*
		* @param type of the stream.
		* @returns the ID of its desired encoder for the specific type of stream.
		* @throws std::domain_error if the ID could not be obtained.
		*/
		AVCodecID desired_encoder_id(AVMediaType type) const;

////////////////////////// Fields //////////////////////////
	private:
		/*
		* Streams of the file, in the order they are created and stored in the fmt ctx.
		*/
		std::vector<stream> streams;
		/*
		* Indices of video, audio, and subtitle streams, respectively,
		* in the order they appear in streams.
		*/
		std::vector<int> v_indices, a_indices, s_indices;

		const AVOutputFormat* p_muxer_desc = nullptr;
		bool ready = false;

////////////////////////// Internal helper methods //////////////////////////
	private:
		/*
		* Creates the muxer and its context. Set up a few things in the context.
		* After a call to this, you should fill additional info before you call
		* prepare_muxer().
		*/
		void internal_create_muxer(const std::string& path);

		/*
		* Closes the output file, and completely destroys the muxer and any resource it holds.
		*/
		void destroy();
		/*
		* Common piece of code for the two public methods with different dict parameters.
		* @param ppavd can be nullptr.
		*/
		void internal_prepare_muxer(::AVDictionary** ppavd);

		/*
		* Common piece of code for the two add_stream methods.
		* @param properties to set to the new stream.
		*/
		stream internal_create_stream(const ff::codec_properties& properties);
	};
}