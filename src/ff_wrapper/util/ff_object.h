/*
* Copyright (C) Guanyuming He 2024
* This file is licensed under the GNU General Public License v3.
* 
* This file is part of PROJECT_NAME_REPLACE_LATER.
* PROJECT_NAME_REPLACE_LATER is free software: 
* you can redistribute it and/or modify it under the terms of the GNU General Public License 
* as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
* 
* PROJECT_NAME_REPLACE_LATER is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; 
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
* See the GNU General Public License for more details.
* You should have received a copy of the GNU General Public License along with PROJECT_NAME_REPLACE_LATER. 
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
	* Default constructor has the param=false.
	* This way, when base's default ctor is called by derived classes implicitly, nothing will be done.
	* The user can then call with a different argument/call different ctors explicitly to do otherwise.
	* 
	* @param create_object: true=allocate memory for the object;false=doesn't do anything
	* @throws std::bad_alloc if allocation fails
	*/
	ff_object(bool create_object = false);

	/*
	* Only allocates memory for the object.
	* Should not copy the state of other because state is private
	* and derived classes cannot control the state directly.
	* 
	* Derived classes should copy the object and
	* control the resource allocation and copy themselves.
	*/
	ff_object(const ff_object& other) : ff_object(true) {}

	/*
	* Does not allocate any memory. Simply copies the state of other.
	* 
	* Derived classes should copy the pointers.
	*/
	ff_object(ff_object&& other) noexcept : state(other.state) {}

	/*
	* The destructor.
	* It releases all the memory allocated for the resources
	* as well as all the memory allocated for the FFmpeg object itself.
	* 
	* Derived classes should also make sure that all custom destruction logic
	* is executed in the destructor.
	*/
	virtual ~ff_object()
	{
		destroy();
	}

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

	ff_object_state get_object_state() const { return state; }

private:
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
	* 
	* @throws std::bad_alloc if the allocation fails.
	*/
	virtual void internal_allocate_object_memory() = 0;
	/*
	* Allocates memory for the resources in some way.
	* 
	* @param size: whether used or not depends on the derived class
	* @param additional_information: whether used or not depend on the derived class
	* @throws std::bad_alloc if the allocation fails.
	*/
	virtual void internal_allocate_resources_memory
	(
		uint64_t size,
		void* additional_information
	) = 0;

	/*
	* Releases the memory allocated for the object and sets the object pointer to nullptr.
	* Should be called after releasing resources, not before.
	*/
	virtual void internal_release_object_memory() = 0;
	/*
	* Releases only the memory allocated for the resources
	* Should be called before releasing the object memory.
	*/
	virtual void internal_release_resources_memory() = 0;

public:
	/*
	* If the object is DESTROYED, then it allocates memory for the object, 
	* and adjusts the state to ff_object_state::OBJECT_CREATED.
	* 
	* @throws std::bad_alloc if the allocation fails.
	* @note assertion fails if the state is not ff_object_state::DESTROYED
	*/
	void allocate_object_memory();

	/*
	* If the object is OBJECT_CREATED, then allocates memory for the resources in some way,
	* and adjusts the state to ff_object_state::READY.
	* 
	* @param size: whether used or not depends on the derived class
	* @param additional_information: whether used or not depend on the derived class
	* @throws std::bad_alloc if the allocation fails.
	* @note assertion fails if the state is not ff_object_state::OBJECT_CREATED
	*/
	void allocate_resources_memory
	(
		uint64_t size = 0,
		void* additional_information = nullptr
	);

	/*
	* If the object is READY, then releases the memory allocated for the resources
	* and adjusts the state to ff_object_state::OBJECT_CREATED.
	* 
	* @note assertion fails if the state is not ff_object_state::READY
	*/
	void release_resources_memory();

	/*
	* If the object is OBJECT_CREATED, then releases the memory allocated for the resources
	* and adjusts the state to ff_object_state::DESTROYED.
	* 
	* @note assertion fails if the state is not ff_object_state::OBJECT_CREATED
	*/
	void release_object_memory();

	/*
	* Destroys the object by releasing all memory allocated for it and its resources.
	* Can be called on any state. If called on DESTROYED state, then the method does nothing.
	* 
	* After the call, sets state to DESTROYED.
	*/
	void destroy();

////////////////////////////// Testing Only ////////////////////////////
#ifdef FF_TESTING
	// Expose all members for testers
public:
	auto& t_get_ref_state() { return state; }
	const auto& t_get_ref_state() const { return state; }
#endif // FF_TESTING

};