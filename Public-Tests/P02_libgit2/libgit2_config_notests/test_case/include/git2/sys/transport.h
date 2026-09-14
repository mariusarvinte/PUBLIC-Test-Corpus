/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef INCLUDE_sys_git_transport_h
#define INCLUDE_sys_git_transport_h

#include "git2/net.h"
#include "git2/oidarray.h"
#include "git2/remote.h"
#include "git2/strarray.h"
#include "git2/transport.h"
#include "git2/types.h"

/**
 * @file git2/sys/transport.h
 * @brief Custom transport registration interfaces and functions
 * @defgroup git_transport Custom transport registration
 * @ingroup Git
 *
 * Callers can override the default implementation by
 * specifying a custom transport.
 * @{
 */

GIT_BEGIN_DECL

/**
 * The negotiation state during a fetch smart transport negotiation.
 */
typedef struct {
	const git_remote_head * const *refs;
	size_t refs_len;
	git_oid *shallow_roots;
	size_t shallow_roots_len;
	int depth;
} git_fetch_negotiation;

struct git_transport {
	unsigned int version; /**< The struct version */

	/**
	 * Connect the transport to the remote repository, using the given
	 * direction.
	 */
	int GIT_CALLBACK(connect)(
		git_transport *transport,
		const char *url,
		int direction,
		const git_remote_connect_options *connect_opts);

	/**
	 * Resets the connect options for the given transport.  This
	 * is useful for updating settings or callbacks for an already
	 * connected transport.
	 */
	int GIT_CALLBACK(set_connect_opts)(
		git_transport *transport,
		const git_remote_connect_options *connect_opts);

	/**
	 * Gets the capabilities for this remote repository.
	 *
	 * This function may be called after a successful call to
	 * `connect()`.
	 */
	int GIT_CALLBACK(capabilities)(
		unsigned int *capabilities,
		git_transport *transport);

#ifdef GIT_EXPERIMENTAL_SHA256
	/**
	 * Gets the object type for the remote repository.
	 *
	 * This function may be called after a successful call to
	 * `connect()`.
	 */
	int GIT_CALLBACK(oid_type)(
		git_oid_t *object_type,
		git_transport *transport);
#endif

	/**
	 * Get the list of available references in the remote repository.
	 *
	 * This function may be called after a successful call to
	 * `connect()`. The array returned is owned by the transport and
	 * must be kept valid until the next call to one of its functions.
	 */
	int GIT_CALLBACK(ls)(
		const git_remote_head ***out,
		size_t *size,
		git_transport *transport);

	/** Executes the push whose context is in the git_push object. */
	int GIT_CALLBACK(push)(
		git_transport *transport,
		git_push *push);

	/**
	 * Negotiate a fetch with the remote repository.
	 *
	 * This function may be called after a successful call to `connect()`,
	 * when the direction is GIT_DIRECTION_FETCH. The function performs a
	 * negotiation to calculate the `wants` list for the fetch.
	 */
	int GIT_CALLBACK(negotiate_fetch)(
		git_transport *transport,
		git_repository *repo,
		const git_fetch_negotiation *fetch_data);

	/**
	 * Return the shallow roots of the remote.
	 *
	 * This function may be called after a successful call to
	 * `negotiate_fetch`.
	 */
	int GIT_CALLBACK(shallow_roots)(
		git_oidarray *out,
		git_transport *transport);

	/**
	 * Start downloading the packfile from the remote repository.
	 *
	 * This function may be called after a successful call to
	 * negotiate_fetch(), when the direction is GIT_DIRECTION_FETCH.
	 */
	int GIT_CALLBACK(download_pack)(
		git_transport *transport,
		git_repository *repo,
		git_indexer_progress *stats);

	/** Checks to see if the transport is connected */
	int GIT_CALLBACK(is_connected)(git_transport *transport);

	/** Cancels any outstanding transport operation */
	void GIT_CALLBACK(cancel)(git_transport *transport);

	/**
	 * Close the connection to the remote repository.
	 *
	 * This function is the reverse of connect() -- it terminates the
	 * connection to the remote end.
	 */
	int GIT_CALLBACK(close)(git_transport *transport);

	/** Frees/destructs the git_transport object. */
	void GIT_CALLBACK(free)(git_transport *transport);
};

/** Current version for the `git_transport` structure */
#define GIT_TRANSPORT_VERSION 1

/** Static constructor for `git_transport` */
#define GIT_TRANSPORT_INIT {GIT_TRANSPORT_VERSION}

/**
 * Initializes a `git_transport` with default values. Equivalent to
 * creating an instance with GIT_TRANSPORT_INIT.
 *
 * @param opts the `git_transport` struct to initialize
 * @param version Version of struct; pass `GIT_TRANSPORT_VERSION`
 * @return Zero on success; -1 on failure.
 */
GIT_EXTERN(int) git_transport_init(
	git_transport *opts,
	unsigned int version);

/**
 * Function to use to create a transport from a URL. The transport database
 * is scanned to find a transport that implements the scheme of the URI (i.e.
 * file://) and a transport object is returned to the caller.
 *
 * @param out The newly created transport (out)
 * @param owner The git_remote which will own this transport
 * @param url The URL to connect to
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_transport_new(git_transport **out, git_remote *owner, const char *url);

/**
 * Add a custom transport definition, to be used in addition to the built-in
 * set of transports that come with libgit2.
 *
 * The caller is responsible for synchronizing calls to git_transport_register
 * and git_transport_unregister with other calls to the library that
 * instantiate transports.
 *
 * @param prefix The scheme to match, eg "file"
 * @param cb The callback used to create an instance of the transport
 * @param param A fixed parameter to pass to cb at creation time
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_transport_register(
	const char *prefix,
	git_transport_cb cb,
	void *param);

/**
 * Unregister a custom transport definition which was previously registered
 * with git_transport_register.
 *
 * The caller is responsible for synchronizing calls to git_transport_register
 * and git_transport_unregister with other calls to the library that
 * instantiate transports.
 *
 * @param prefix From the previous call to git_transport_register
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_transport_unregister(
	const char *prefix);

/* Transports which come with libgit2 (match git_transport_cb). The expected
 * value for "param" is listed in-line below. */

/**
 * Create an instance of the dummy transport.
 *
 * @param out The newly created transport (out)
 * @param owner The git_remote which will own this transport
 * @param payload You must pass NULL for this parameter.
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_transport_dummy(
	git_transport **out,
	git_remote *owner,
	/* NULL */ void *payload);

/**
 * Create an instance of the local transport.
 *
 * @param out The newly created transport (out)
 * @param owner The git_remote which will own this transport
 * @param payload You must pass NULL for this parameter.
 * @return 0 or an error code
 */
GIT_EXTERN(int) git_transport_local(
	git_transport **out,
	git_remote *owner,
	/* NULL */ void *payload);

/**
 * Get a copy of the remote connect options
 *
 * All data is copied and must be freed by the caller by calling
 * `git_remote_connect_options_dispose`.
 *
 * @param out options struct to fill
 * @param transport the transport to extract the data from.
 * @return 0 on success, or an error code
 */
GIT_EXTERN(int) git_transport_remote_connect_options(
		git_remote_connect_options *out,
		git_transport *transport);

/*
 *** End of base transport interface ***
 *** Begin interface for subtransports for the smart transport ***
 */

/** Actions that the smart transport can ask a subtransport to perform */
typedef enum {
	GIT_SERVICE_UPLOADPACK_LS = 1,
	GIT_SERVICE_UPLOADPACK = 2,
	GIT_SERVICE_RECEIVEPACK_LS = 3,
	GIT_SERVICE_RECEIVEPACK = 4
} git_smart_service_t;

/** @} */
GIT_END_DECL

#endif
