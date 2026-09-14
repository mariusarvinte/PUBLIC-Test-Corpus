/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "transaction.h"

#include "repository.h"
#include "refdb.h"
#include "reflog.h"
#include "signature.h"
#include "config.h"

#include "git2/transaction.h"
#include "git2/signature.h"
#include "git2/sys/refs.h"
#include "git2/sys/refdb_backend.h"

typedef enum {
	TRANSACTION_NONE,
	TRANSACTION_REFS,
	TRANSACTION_CONFIG
} transaction_t;

typedef struct {
	char *name;
	void *payload;

	git_reference_t ref_type;
	union {
		git_oid id;
		char *symbolic;
	} target;
	git_reflog *reflog;

	char *message;
	git_signature *sig;

	unsigned int committed : 1, remove : 1;
} transaction_node;

GIT_HASHMAP_STR_SETUP(git_transaction_nodemap, transaction_node *);

struct git_transaction {
	transaction_t type;
	git_repository *repo;
	git_refdb *db;
	git_config *cfg;
	void *cfg_data;

	git_transaction_nodemap locks;
};

static void transaction_node_clear_target(transaction_node *node)
{
	if (node->ref_type == GIT_REFERENCE_SYMBOLIC)
		git__free(node->target.symbolic);

	node->target.symbolic = NULL;
	node->ref_type = GIT_REFERENCE_INVALID;
}

static void transaction_node_free(transaction_node *node)
{
	if (!node)
		return;

	git__free(node->name);
	transaction_node_clear_target(node);
	git_reflog_free(node->reflog);
	git__free(node->message);
	git_signature_free(node->sig);
	git__free(node);
}

static void transaction_free_nodes(git_transaction *tx)
{
	git_hashmap_iter_t iter = GIT_HASHMAP_ITER_INIT;
	transaction_node *node;

	while (git_transaction_nodemap_iterate(
	               &iter, NULL, &node, &tx->locks) == 0)
		transaction_node_free(node);
}

int git_transaction_config_new(
        git_transaction **out,
        git_config *cfg,
        void *data)
{
	git_transaction *tx;

	GIT_ASSERT_ARG(out);
	GIT_ASSERT_ARG(cfg);

	tx = git__calloc(1, sizeof(git_transaction));
	GIT_ERROR_CHECK_ALLOC(tx);

	tx->type = TRANSACTION_CONFIG;
	tx->cfg = cfg;
	tx->cfg_data = data;

	*out = tx;
	return 0;
}

int git_transaction_new(git_transaction **out, git_repository *repo)
{
	int error;
	git_transaction *tx = NULL;

	GIT_ASSERT_ARG(out);
	GIT_ASSERT_ARG(repo);

	tx = git__calloc(1, sizeof(git_transaction));
	if (!tx) {
		error = -1;
		goto on_error;
	}

	if ((error = git_repository_refdb(&tx->db, repo)) < 0)
		goto on_error;

	tx->type = TRANSACTION_REFS;
	tx->repo = repo;
	*out = tx;
	return 0;

on_error:
	git__free(tx);
	return error;
}

int git_transaction_lock_ref(git_transaction *tx, const char *refname)
{
	int error;
	transaction_node *node;

	GIT_ASSERT_ARG(tx);
	GIT_ASSERT_ARG(refname);

	node = git__calloc(1, sizeof(transaction_node));
	if (!node) {
		git_error_set_oom();
		return -1;
	}

	node->name = git__strdup(refname);
	if (!node->name) {
		git_error_set_oom();
		error = -1;
		goto cleanup;
	}

	if ((error = git_refdb_lock(&node->payload, tx->db, refname)) < 0)
		goto cleanup;

	if ((error = git_transaction_nodemap_put(
	             &tx->locks, node->name, node)) < 0)
		goto cleanup_unlock;

	return 0;

cleanup_unlock:
	git_refdb_unlock(tx->db, node->payload, false, false, NULL, NULL, NULL);

cleanup:
	transaction_node_free(node);

	return error;
}

static int
find_locked(transaction_node **out, git_transaction *tx, const char *refname)
{
	transaction_node *node;
	int error;

	error = git_transaction_nodemap_get(&node, &tx->locks, refname);

	if (error != 0) {
		git_error_set(
		        GIT_ERROR_REFERENCE,
		        "the specified reference is not locked");
		return GIT_ENOTFOUND;
	}

	*out = node;
	return 0;
}

static int copy_common(
        transaction_node *node,
        git_transaction *tx,
        const git_signature *sig,
        const char *msg)
{
	git_signature_free(node->sig);
	node->sig = NULL;

	git__free(node->message);
	node->message = NULL;

	if (sig && git_signature_dup(&node->sig, sig) < 0)
		return -1;

	if (!node->sig) {
		git_signature *tmp;
		int error;

		if (git_reference__log_signature(&tmp, tx->repo) < 0)
			return -1;

		error = git_signature_dup(&node->sig, tmp);
		git_signature_free(tmp);
		if (error < 0)
			return error;
	}

	if (msg) {
		node->message = git__strdup(msg);
		GIT_ERROR_CHECK_ALLOC(node->message);
	}

	return 0;
}

int git_transaction_set_target(
        git_transaction *tx,
        const char *refname,
        const git_oid *target,
        const git_signature *sig,
        const char *msg)
{
	int error;
	transaction_node *node;

	GIT_ASSERT_ARG(tx);
	GIT_ASSERT_ARG(refname);
	GIT_ASSERT_ARG(target);

	if ((error = find_locked(&node, tx, refname)) < 0)
		return error;

	if ((error = copy_common(node, tx, sig, msg)) < 0)
		return error;

	transaction_node_clear_target(node);
	git_oid_cpy(&node->target.id, target);
	node->ref_type = GIT_REFERENCE_DIRECT;

	return 0;
}

int git_transaction_set_symbolic_target(
        git_transaction *tx,
        const char *refname,
        const char *target,
        const git_signature *sig,
        const char *msg)
{
	int error;
	transaction_node *node;

	GIT_ASSERT_ARG(tx);
	GIT_ASSERT_ARG(refname);
	GIT_ASSERT_ARG(target);

	if ((error = find_locked(&node, tx, refname)) < 0)
		return error;

	if ((error = copy_common(node, tx, sig, msg)) < 0)
		return error;

	transaction_node_clear_target(node);
	node->target.symbolic = git__strdup(target);
	GIT_ERROR_CHECK_ALLOC(node->target.symbolic);
	node->ref_type = GIT_REFERENCE_SYMBOLIC;

	return 0;
}

int git_transaction_remove(git_transaction *tx, const char *refname)
{
	int error;
	transaction_node *node;

	if ((error = find_locked(&node, tx, refname)) < 0)
		return error;

	transaction_node_clear_target(node);
	node->remove = true;
	node->ref_type = GIT_REFERENCE_DIRECT; /* the id will be ignored */

	return 0;
}

static int dup_reflog(git_reflog **out, const git_reflog *in)
{
	git_reflog *reflog;
	git_reflog_entry *tgt = NULL;
	size_t len, i;
	int error = 0;

	reflog = git__calloc(1, sizeof(git_reflog));
	if (!reflog) {
		git_error_set_oom();
		return -1;
	}

	reflog->ref_name = git__strdup(in->ref_name);
	if (!reflog->ref_name) {
		git_error_set_oom();
		error = -1;
		goto on_error;
	}
	reflog->oid_type = in->oid_type;

	len = in->entries.length;
	if ((error = git_vector_init(&reflog->entries, len, NULL)) < 0)
		goto on_error;

	for (i = 0; i < len; i++) {
		const git_reflog_entry *src;

		tgt = git__calloc(1, sizeof(git_reflog_entry));
		if (!tgt) {
			git_error_set_oom();
			error = -1;
			goto on_error;
		}

		src = git_vector_get(&in->entries, i);
		git_oid_cpy(&tgt->oid_old, &src->oid_old);
		git_oid_cpy(&tgt->oid_cur, &src->oid_cur);

		tgt->msg = git__strdup(src->msg);
		if (!tgt->msg) {
			git_error_set_oom();
			error = -1;
			goto on_error_tgt;
		}

		if (git_signature_dup(&tgt->committer, src->committer) < 0 ||
		    (error = git_vector_insert(&reflog->entries, tgt)) < 0)
			goto on_error_tgt;

		tgt = NULL;
	}

	*out = reflog;
	return 0;

on_error_tgt:
	git_reflog_entry__free(tgt);

on_error:
	git_reflog_free(reflog);
	return error;
}

int git_transaction_set_reflog(
        git_transaction *tx,
        const char *refname,
        const git_reflog *reflog)
{
	int error;
	transaction_node *node;

	GIT_ASSERT_ARG(tx);
	GIT_ASSERT_ARG(refname);
	GIT_ASSERT_ARG(reflog);

	if ((error = find_locked(&node, tx, refname)) < 0)
		return error;

	git_reflog_free(node->reflog);
	node->reflog = NULL;

	if ((error = dup_reflog(&node->reflog, reflog)) < 0)
		return error;

	return 0;
}

static int update_target(git_refdb *db, transaction_node *node)
{
	git_reference *ref;
	int error, update_reflog;

	if (node->ref_type == GIT_REFERENCE_DIRECT) {
		ref = git_reference__alloc(node->name, &node->target.id, NULL);
	} else if (node->ref_type == GIT_REFERENCE_SYMBOLIC) {
		ref = git_reference__alloc_symbolic(
		        node->name, node->target.symbolic);
	} else {
		abort();
	}

	GIT_ERROR_CHECK_ALLOC(ref);
	update_reflog = node->reflog == NULL;

	if (node->remove) {
		error = git_refdb_unlock(
		        db, node->payload, 2, false, ref, NULL, NULL);
	} else if (node->ref_type == GIT_REFERENCE_DIRECT) {
		error = git_refdb_unlock(
		        db, node->payload, true, update_reflog, ref, node->sig,
		        node->message);
	} else if (node->ref_type == GIT_REFERENCE_SYMBOLIC) {
		error = git_refdb_unlock(
		        db, node->payload, true, update_reflog, ref, node->sig,
		        node->message);
	} else {
		abort();
	}

	git_reference_free(ref);
	node->committed = true;

	return error;
}

int git_transaction_commit(git_transaction *tx)
{
	transaction_node *node;
	git_hashmap_iter_t iter = GIT_HASHMAP_ITER_INIT;
	int error = 0;

	GIT_ASSERT_ARG(tx);

	if (tx->type == TRANSACTION_CONFIG) {
		error = git_config_unlock(tx->cfg, tx->cfg_data, true);
		tx->cfg = NULL;
		tx->cfg_data = NULL;

		return error;
	}

	while (git_transaction_nodemap_iterate(
	               &iter, NULL, &node, &tx->locks) == 0) {
		if (node->reflog) {
			if ((error = tx->db->backend->reflog_write(
			             tx->db->backend, node->reflog)) < 0)
				return error;
		}

		if (node->ref_type == GIT_REFERENCE_INVALID) {
			/* ref was locked but not modified */
			if ((error = git_refdb_unlock(
			             tx->db, node->payload, false, false, NULL,
			             NULL, NULL)) < 0) {
				return error;
			}
			node->committed = true;
		} else {
			if ((error = update_target(tx->db, node)) < 0)
				return error;
		}
	}

	return 0;
}

void git_transaction_free(git_transaction *tx)
{
	transaction_node *node;
	git_hashmap_iter_t iter = GIT_HASHMAP_ITER_INIT;

	if (!tx)
		return;

	if (tx->type == TRANSACTION_CONFIG) {
		if (tx->cfg)
			git_config_unlock(tx->cfg, tx->cfg_data, false);

		git__free(tx);
		return;
	}

	/* start by unlocking the ones we've left hanging, if any */
	while (git_transaction_nodemap_iterate(
	               &iter, NULL, &node, &tx->locks) == 0) {
		if (node->committed)
			continue;

		git_refdb_unlock(
		        tx->db, node->payload, false, false, NULL, NULL, NULL);
	}

	git_refdb_free(tx->db);
	transaction_free_nodes(tx);
	git_transaction_nodemap_dispose(&tx->locks);
	git__free(tx);
}
