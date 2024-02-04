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

/*
* FFmpeg is written in C, so it uses C-style memory management.
* Because this wrapper library aims to provide an encapsulation of the FFmpeg C APIs
* in modern C++, I must invent a comprehensive framework to take over the memory management.
* 
* In this file, a base class, ff_object, is defined to provide abstraction for 
* most FFmpeg objects that involve two-levels of memory management:
* 1. memory allocation for the object itself
* 2. memory allocation for the resources that the object uses
*/

#include "util.h"

// For uint64_t, etc.
#include <cstdint>

namespace ff
{
	/*
	* This class is defined to provide abstraction for
	* most FFmpeg objects that involve two-levels of memory management:
	* 1. memory allocation for the object itself
	* 2. memory allocation for the resources that the object uses
	*
	* The class uses states to control the memory allocation
	* 1. DESTROYED. The object has no memory allocated for it. The value of its pointers are all nullptr.
	* 2. OBJECT_CREATED. The object has memory allocated for it, but it has no resources allocated.
	* The value of the object pointer is not nullptr.
	* 3. READY. The object has memory allocated for it and it is using some resources allocated.
	*
	* Thread Safety: NOT thread-safe.
	*
	* Testing: Define FF_TESTING to access all members through t_get_ref_...()
	*/
	class FF_WRAPPER_API ff_object
	{
	public:
		/*
		* Default constructor does nothing
		* This way, when base's default ctor is called by derived classes implicitly, nothing will be done.
		* The user can then call with a different argument/call different ctors explicitly to do otherwise.
		*
		* One might expect the default constructor to allocate memory for the object,
		* which is plausible, but which is impossible because the allocation method invokes a virtual method.
		* During construction (or destruction), a virtual method call invokes the most derived class up to the
		* class being constructed. If internal_allocate_object_memory is called here,
		* then the pure virtual version will be called, making the program ill-formed.
		*/
		ff_object() noexcept;

		/*
		* Only copies the state.
		*
		* Derived classes should do everything else.
		*/
		ff_object(const ff_object& other) noexcept 
			: state(other.state) {}

		/*
		* Does not allocate any memory. Simply copies the state of other
		* and sets other's to DESTROYED.
		* Derived class should call this to handle the state.
		*
		* Derived classes should copy the pointers.
		*/
		ff_object(ff_object&& other) noexcept 
			: state(other.state) 
		{
			other.state = ff_object_state::DESTROYED;
		}

		/*
		* Destroys the object by calling destroy(). Then,
		* copies the state.
		*
		* Derived classes should do everything else, including the state control.
		*/
		ff_object& operator=(const ff_object& right) noexcept;

		/*
		* Destroys the object by calling destroy(). Then,
		* 
		* Only copies the state and sets right's to DESTORYED.
		* Derived class should call this to handle the state.
		*/
		ff_object& operator=(ff_object&& right) noexcept;

		/*
		* The destructor.
		*
		* Derived classes should call destroy() themselves doing so here
		* would call this's internal_... methods.
		*/
		virtual ~ff_object() {}

		////////////////////////// State Controls //////////////////////////
	public:
		// The class uses states to control the memory allocations.
		enum class ff_object_state
		{
			// The object has no memory allocated for it. The value of its pointers are all nullptr.
			DESTROYED,
			// The object has memory allocated for it, but it has no resources allocated.
			// The value of the object pointer is not nullptr.
			OBJECT_CREATED,
			// The object has memory allocated for it and it is using some resources allocated.
			READY
		};

		ff_object_state get_object_state() const noexcept { return state; }
		bool destroyed() const noexcept { return ff_object_state::DESTROYED == state; }
		bool created() const noexcept { return ff_object_state::OBJECT_CREATED == state; }
		bool ready() const noexcept { return ff_object_state::READY == state; }

	protected:
		// Declare the state as protected to give more memory allocation control to 
		// derived classes.
		ff_object_state state;

		//////////////////////////// Memory Allocation Control ////////////////////////////////
	protected:
		/*
		* Derived classes should override these internal_ methods that do not care about the state
		* but only do the task.
		*
		* Users should call another set of methods (without the internal_ prefix)
		* that decide how to do the task based on the current state and also adjust the state accordingly.
		*/

		/*
		* Allocates memory for the object.
		*/
		virtual void internal_allocate_object_memory() = 0;
		/*
		* Allocates memory for the resources in some way.
		*
		* @param size: whether used or not depends on the derived class
		* @param additional_information: whether used or not depend on the derived class
		*/
		virtual void internal_allocate_resources_memory
		(
			uint64_t size, void * additional_information
		) = 0;

		/*
		* Releases the memory allocated for the object and sets the object pointer to nullptr.
		* Should be called after releasing resources, not before.
		*/
		virtual void internal_release_object_memory() noexcept = 0;
		/*
		* Releases only the memory allocated for the resources
		* Should be called before releasing the object memory.
		*/
		virtual void internal_release_resources_memory() noexcept = 0;

	public:
		/*
		* If the object is DESTROYED, then it allocates memory for the object,
		* and adjusts the state to ff_object_state::OBJECT_CREATED.
		*
		* @note assertion fails if the state is not ff_object_state::DESTROYED
		*/
		void allocate_object_memory();

		/*
		* If the object is OBJECT_CREATED, then allocates memory for the resources in some way,
		* and adjusts the state to ff_object_state::READY.
		* For type safety, if additional_information is used to point to some real type T,
		* then the derived class is encouraged to provide a wrapper for allocate_resources_memory()
		* that directly accepts a parameter of some type related to T.
		*
		* @param size: whether used or not depends on the derived class. Therefore, 0 may be allowed
		* or may not be depending on the derived class.
		* @param additional_information: whether used or not depends on the derived class
		* @note assertion fails if the state is not ff_object_state::OBJECT_CREATED
		*/
		void allocate_resources_memory
		(
			uint64_t size = 0,
			void *additional_information = nullptr
		);

		/*
		* If the object is READY, then releases the memory allocated for the resources
		* and adjusts the state to ff_object_state::OBJECT_CREATED.
		*
		* @note assertion fails if the state is not ff_object_state::READY
		*/
		void release_resources_memory() noexcept(FF_ASSERTION_DISABLED);

		/*
		* If the object is OBJECT_CREATED, then releases the memory allocated for the resources
		* and adjusts the state to ff_object_state::DESTROYED.
		*
		* @note assertion fails if the state is not ff_object_state::OBJECT_CREATED
		*/
		void release_object_memory() noexcept(FF_ASSERTION_DISABLED);

		/*
		* Destroys the object by releasing all memory allocated for it and its resources.
		* Can be called on any state. If called on DESTROYED state, then the method does nothing.
		*
		* After the call, sets state to DESTROYED.
		*/
		void destroy() noexcept;

		////////////////////////////// Testing Only ////////////////////////////
#ifdef FF_TESTING
	// Expose all members for testers
	public:
		auto& t_get_ref_state() { return state; }
		const auto& t_get_ref_state() const { return state; }
#endif // FF_TESTING

	};
}