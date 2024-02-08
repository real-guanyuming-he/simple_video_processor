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

#include "../util/dict.h" // Use a dictionary to pass options
#include "media_base.h" // A demuxer inherits media base
#include "stream.h" // A demuxer has info about all streams in a file
#include "../data/packet.h" // A demuxer demuxes a file into packets

#include <vector> // Streams are stored in a vector

struct AVInputFormat;

namespace ff
{
	/*
	* Demuxer for local files.
	* 
	* Create a useable demuxer by giving it the path to the file you want to demux.
	* Then, it is ready to be used.
	* 
	* Demux by calling demux_next_packet() until eof().
	* Use seek() to seek.
	*/
	class FF_WRAPPER_API demuxer : public media_base
	{
	public:
		/*
		* Inits all pointers to nullptr.
		*/
		demuxer() noexcept :
			media_base() {}

		/*
		* Opens a local multimedia file pointed to by path.
		* The demuxer will be ready after the call.
		*
		* @param the absolute path to the multimedia file.
		* @param probe_stream_info: if set to true, then after the file is opened,
		the ctor probes the stream information of the file by reading and potentially decoding a few packets.
		* @param options specifies how the file is opened. Is empty by default.
		* @throws std::invalid_argument if path is nullptr.
		* @throws std::filesystem::filesystem_error if file not found.
		*/
		demuxer
		(
			const char* path, 
			bool probe_stream_info = true, 
			const dict& options = dict()
		);

		/*
		* Opens a local multimedia file pointed to by path.
		* The demuxer will be ready after the call.
		* 
		* @param the absolute path to the multimedia file.
		* @param probe_stream_info: if set to true, then after the file is opened,
		the ctor probes the stream information of the file by reading and potentially decoding a few packets.
		* @param options specifies how the file is opened. Cannot be empty.
		* After the constructor returns, the options argument will be replaced with a dict containing options that were not found.
		* @throws std::invalid_argument if path is nullptr or options is empty.
		* @throws std::filesystem::filesystem_error if file not found.
		*/
		demuxer
		(
			const char* path,
			dict& options,
			bool probe_stream_info = true
		);

		/*
		* Releases all resources and sets all pointers to nullptr.
		*/
		~demuxer() noexcept;

	public:
		/*
		* Read a few packets from the file and potentially decode them
		* in order to obtain accurate information of all streams.
		* Normally stream info is stored inside file header, but some part of it can be inaccurate.
		* And some files have no headers (e.g. MPEG).
		* Although packets are read, the logical position of the file pointer is not changed.
		* (i.e. where the next av_read_packet starts is not changed)
		* 
		* Internally, the updated stream information is reflected inside my fmt ctx,
		* but in this method I will also expose them through my stream class.
		* 
		* @param options to pass to the decoders.
		*/
		void probe_stream_information(const dict& options = dict());

		/*
		* Read a few packets from the file and potentially decode them
		* in order to obtain accurate information of all streams.
		* Normally stream info is stored inside file header, but some part of it can be inaccurate.
		* And some files have no headers (e.g. MPEG).
		* Although packets are read, the logical position of the file pointer is not changed.
		* (i.e. where the next av_read_packet starts is not changed)
		*
		* Internally, the updated stream information is reflected inside my fmt ctx,
		* but in this method I will also expose them through my stream class.
		*
		* @param options to pass to the decoders. Cannot be empty.
		* @throws std::invalid_argument if options is empty.
		*/
		void probe_stream_information(dict& options);

		/*
		* Demuxes a packet from the currently location in the file.
		* 
		* @returns the packet demuxed, or a DESTROYED packet if eof has been reached.
		* Since then, eof() remains true until it is reset by another related method.
		* If the packet is not DESTROYED, then it is linked to its corresponding stream.
		* @throws std::logic_error if the demuxer isn't ready (i.e. its ctx is nullptr)
		*/
		packet demux_next_packet();

		/*
		* Seeks to the first frame of the stream in the file that is
		* the first one beyond/before timestamp for direction true/false.
		* 
		* @param stream_ind which stream
		* @param timestamp seek to where, in the time base of the stream.
		* @param direction ture=forward;false=backward
		* 
		* @throws std::logic_error if the demuxer isn't ready (i.e. its ctx is nullptr)
		* @throws std::out_of_range if stream ind is wrong
		*/
		void seek(int stream_ind, int64_t timestamp, bool direction = true);

		bool eof() const noexcept { return eof_reached; }

	public:
		inline const ::AVInputFormat* av_input_fmt() const noexcept { return p_demuxer_desc; }
		inline const ::AVInputFormat* av_input_fmt() noexcept { return p_demuxer_desc; }

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

	private:
		/*
		* Streams of the file, in the order they are stored in the fmt ctx.
		*/
		std::vector<stream> streams;
		/*
		* Indices of video, audio, and subtitle streams, respectively,
		* in the order they appear in streams.
		*/
		std::vector<int> v_indices, a_indices, s_indices;
		/*
		* Best video, audio, and subtitile streams found by av_find_best_stream(), respectively.
		* Or -1 if not available.
		*/
		// TODO: Add the fields, probably with the decoder information returned by av_find_best_stream()

	private:
		const AVInputFormat* p_demuxer_desc = nullptr;

		/*
		* I will set to true during a call to demux_next_packet()
		* if the demuxer detects the eof.
		*/
		bool eof_reached = false;

	private:
		// Because of the two versions of methods that use dict,
		// here are these private methods that contain the common pieces of code between the versions.

		/*
		* common piece of code used in the constructors
		*/
		void internal_open_format(const char* path, bool probe_stream_info, ::AVDictionary** dict);
		/*
		* common piece of code used in probe_stream_information()
		*/
		void internal_probe_stream_info(::AVDictionary** dict);

	public:
		// Inherited via media_base
		virtual std::string description() const noexcept override;
		virtual std::vector<std::string> short_names() const noexcept override;
		virtual std::vector<std::string> extensions() const noexcept override;
	};
}