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
#include "../util/ff_object.h"

struct AVOutputFormat;

namespace ff
{
	class packet;

	/*
	* A muxer of this class muxes packets into a local file.
	* 
	* ff_object states:
	*	DESTROYED: only has information to identify itself. Its desc may or may not be created
	* depending on the constructor called.
	*	OBJECT_CREATED: Its context is created by allocate_object_memory() but cannot be used yet. 
	 During this time, fill all the necessary info (e.g. stream info).
	*	READY: The context has been prepared for the info you filled and the file header been written
	* by calling allocate_resources_memory(). The muxer is ready for packets.
	*/
	class FF_WRAPPER_API muxer : public media_base, public ff_object
	{
	public:
		/*
		* Creates a DESTROYED muxer.
		*/
		muxer() noexcept 
			: media_base(), ff_object() {}

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
			const char* file_path, 
			const char* fmt_name = nullptr, 
			const char* fmt_mime_type = nullptr
		);

		/*
		* Destroys the muxer completely.
		*/
		~muxer() { destroy(); }

	public:
////////////////////////// Inherited via media_base //////////////////////////

		std::string description() const override;
		std::vector<std::string> short_names() const override;
		std::vector<std::string> extensions() const override;

	private:
////////////////////////// Inherited via ff_object //////////////////////////

		/*
		* Allocate the fmt ctx and assigns desc (oformat) to it.
		*/
		void internal_allocate_object_memory() override;
		/*
		* Assumes that you have filled all the info needed for the muxer.
		* This methods prepares the fmt ctx and writes the file header.
		* 
		* @param TBD: Decide how the parameters are going to be used.
		*/
		void internal_allocate_resources_memory(uint64_t size, void* additional_information) override;
		/*
		* Closes the file and releases the fmt ctx.
		*/
		void internal_release_object_memory() noexcept override;
		/*
		* Releases all the streams created for the muxer.
		* You should NOT try to use the muxer again after calling this.
		* If you call this during muxing, then the resources will be safely released,
		* but the file written will have undefined content.
		*/
		void internal_release_resources_memory() noexcept override;

	public:
		/*
		* Mux a packet into the file.
		* 
		* @throw std::invalid_argument if the packet is invalid.
		*/
		void mux_packet(const packet& pkt);

		/*
		* After you have no packets to give, you MUST call this method
		* to finalize the muxing.
		*/
		void finalize();

	private:
		const AVOutputFormat* p_muxer_desc = nullptr;
		const char* file_path = nullptr;
	};
}