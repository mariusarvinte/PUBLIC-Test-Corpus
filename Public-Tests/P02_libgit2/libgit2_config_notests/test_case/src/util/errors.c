/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "git2_util.h"

#include "errors.h"
#include "posix.h"
#include "str.h"
#include "runtime.h"

/*
 * Some static error data that is used when we're out of memory, TLS
 * has not been setup, or TLS has failed.
 */

static git_error oom_error = { "Out of memory", GIT_ERROR_NOMEMORY };

static git_error uninitialized_error = { "library has not been initialized",
	                                 GIT_ERROR_INVALID };

static git_error invalid_error = { "Couldn't get error state",
	                           GIT_ERROR_INVALID };

static git_error no_error = { "no error", GIT_ERROR_NONE };

#define IS_STATIC_ERROR(err)                                     \
	((err) == &oom_error || (err) == &uninitialized_error || \
	 (err) == &no_error)

static struct error_state *s_error_state;

struct error_state {
	/* The error message buffer. */
	git_str message;

	/* Error information, set by `git_error_set` and friends. */
	git_error error;

	/*
	 * The last error to occur; points to the error member of this
	 * struct _or_ a static error.
	 */
	git_error *last;
};

static void errorstate_dispose(void)
{
	if (!s_error_state)
		return;

	git_str_dispose(&s_error_state->message);
}

static struct error_state *errorstate_get(void)
{
	if (s_error_state)
		return s_error_state;

	/*
	 * Avoid git__malloc here, since if it fails, it sets an error
	 * message which would allocate here, which would fail, which
	 * would set an error message...
	 */
	if ((s_error_state = git__allocator.gmalloc(
	             sizeof(struct error_state))) == NULL)
		return NULL;

	memset(s_error_state, 0, sizeof(struct error_state));

	if (git_str_init(&s_error_state->message, 0) < 0) {
		git__allocator.gfree(s_error_state);
		return NULL;
	}

	return s_error_state;
}

static void GIT_SYSTEM_CALL errorstate_free(void)
{
	errorstate_dispose();
	git__free(s_error_state);
	s_error_state = NULL;
}

static void git_error_global_shutdown(void)
{
	errorstate_dispose();
	git__free(s_error_state);
	s_error_state = NULL;
}

int git_error_global_init(void)
{
	return git_runtime_shutdown_register(git_error_global_shutdown);
}

static void set_error_from_buffer(int error_class)
{
	struct error_state *errorstate = errorstate_get();
	git_error *error;
	git_str *buf;

	if (!errorstate)
		return;

	error = &errorstate->error;
	buf = &errorstate->message;

	error->message = buf->ptr;
	error->klass = error_class;

	errorstate->last = error;
}

static void set_error(int error_class, char *string)
{
	struct error_state *errorstate = errorstate_get();
	git_str *buf;

	if (!errorstate)
		return;

	buf = &errorstate->message;

	git_str_clear(buf);

	if (string)
		git_str_puts(buf, string);

	if (!git_str_oom(buf))
		set_error_from_buffer(error_class);
}

void git_error_set_oom(void)
{
	struct error_state *errorstate = errorstate_get();

	if (!errorstate)
		return;

	errorstate->last = &oom_error;
}

void git_error_set(int error_class, const char *message)
{
	struct error_state *errorstate = errorstate_get();
	int error_code = (error_class == GIT_ERROR_OS) ? errno : 0;
	git_str *buf;

	if (!errorstate)
		return;

	buf = &errorstate->message;

	git_str_clear(buf);

	if (message) {
		git_str_puts(buf, message);
		if (error_class == GIT_ERROR_OS)
			git_str_PUTS(buf, ": ");
	}

	if (error_class == GIT_ERROR_OS) {
		if (error_code)
			git_str_puts(buf, strerror(error_code));

		if (error_code)
			errno = 0;
	}

	if (!git_str_oom(buf))
		set_error_from_buffer(error_class);
}

void git_error_set_concat(int error_class, size_t count, const char *strings[])
{
	git_str buf = GIT_STR_INIT;

	if (git_str_puts_array(&buf, count, strings) < 0)
		goto done;

	git_error_set(error_class, git_str_cstr(&buf));

done:
	git_str_dispose(&buf);
}

int git_error_set_str(int error_class, const char *string)
{
	struct error_state *errorstate = errorstate_get();
	git_str *buf;

	GIT_ASSERT_ARG(string);

	if (!errorstate)
		return -1;

	buf = &errorstate->message;

	git_str_clear(buf);
	git_str_puts(buf, string);

	if (git_str_oom(buf))
		return -1;

	set_error_from_buffer(error_class);
	return 0;
}

void git_error_clear(void)
{
	struct error_state *errorstate = errorstate_get();

	if (!errorstate)
		return;

	if (errorstate->last != NULL) {
		set_error(0, NULL);
		errorstate->last = NULL;
	}

	errno = 0;
}

bool git_error_exists(void)
{
	struct error_state *errorstate;

	if ((errorstate = errorstate_get()) == NULL)
		return true;

	return errorstate->last != NULL;
}

const git_error *git_error_last(void)
{
	struct error_state *errorstate;

	/* If the library is not initialized, return a static error. */
	if (!git_runtime_init_count())
		return &uninitialized_error;

	if ((errorstate = errorstate_get()) == NULL)
		return &invalid_error;

	if (!errorstate->last)
		return &no_error;

	return errorstate->last;
}

int git_error_save(git_error **out)
{
	struct error_state *errorstate = errorstate_get();
	git_error *error, *dup;

	if (!errorstate) {
		*out = &invalid_error;
		return -1;
	}

	error = errorstate->last;

	if (!error || error == &no_error) {
		*out = &no_error;
		return 0;
	} else if (IS_STATIC_ERROR(error)) {
		*out = error;
		return 0;
	}

	if ((dup = git__malloc(sizeof(git_error))) == NULL) {
		*out = &oom_error;
		return -1;
	}

	dup->klass = error->klass;
	dup->message = git__strdup(error->message);

	if (!dup->message) {
		*out = &oom_error;
		return -1;
	}

	*out = dup;
	return 0;
}

int git_error_restore(git_error *error)
{
	struct error_state *errorstate = errorstate_get();

	GIT_ASSERT_ARG(error);

	if (IS_STATIC_ERROR(error) && errorstate)
		errorstate->last = error;
	else
		set_error(error->klass, error->message);

	git_error_free(error);
	return 0;
}

void git_error_free(git_error *error)
{
	if (!error)
		return;

	if (IS_STATIC_ERROR(error))
		return;

	git__free(error->message);
	git__free(error);
}

int git_error_system_last(void)
{
	return errno;
}

void git_error_system_set(int code)
{
	errno = code;
}

/* Deprecated error values and functions */

#ifndef GIT_DEPRECATE_HARD

#include "git2/deprecated.h"

const git_error *giterr_last(void)
{
	return git_error_last();
}

void giterr_clear(void)
{
	git_error_clear();
}

void giterr_set_str(int error_class, const char *string)
{
	git_error_set_str(error_class, string);
}

void giterr_set_oom(void)
{
	git_error_set_oom();
}
#endif
