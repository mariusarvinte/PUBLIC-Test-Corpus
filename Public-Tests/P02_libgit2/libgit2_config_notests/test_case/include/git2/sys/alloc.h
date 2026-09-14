/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef INCLUDE_sys_git_alloc_h__
#define INCLUDE_sys_git_alloc_h__

#include "git2/common.h"

/**
 * @file git2/sys/alloc.h
 * @brief Custom memory allocators
 * @defgroup git_merge Git merge routines
 * @ingroup Git
 *
 * Users can configure custom allocators; this is particularly
 * interesting when running in constrained environments, when calling
 * from another language, or during testing.
 * @{
 */
GIT_BEGIN_DECL

/**
 * An instance for a custom memory allocator
 *
 * Setting the pointers of this structure allows the developer to implement
 * custom memory allocators. The global memory allocator can be set by using
 * the `git_libgit2_set_allocator` function. Keep in mind
 * that all fields need to be set to a proper function.
 */
typedef struct git_allocator {
	/** Allocate `n` bytes of memory */
	void * GIT_CALLBACK(gmalloc)(size_t n);

	/**
	 * This function shall deallocate the old object `ptr` and return a
	 * pointer to a new object that has the size specified by `size`. In
	 * case `ptr` is `NULL`, a new array shall be allocated.
	 */
	void * GIT_CALLBACK(grealloc)(void *ptr, size_t size);

	/**
	 * This function shall free the memory pointed to by `ptr`. In case
	 * `ptr` is `NULL`, this shall be a no-op.
	 */
	void GIT_CALLBACK(gfree)(void *ptr);
} git_allocator;

/**
 * Initialize the allocator structure to use the `stdalloc` pointer.
 *
 * Set up the structure so that all of its members are using the standard
 * "stdalloc" allocator functions. The structure can then be used with
 * `git_allocator_setup`.
 *
 * @param allocator The allocator that is to be initialized.
 * @return An error code or 0.
 */
int git_stdalloc_init_allocator(git_allocator *allocator);

/** @} */
GIT_END_DECL

#endif
