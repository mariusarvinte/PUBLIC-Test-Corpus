/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <stdio.h>
#include <string.h>
#include <git2.h>
#include "common.h"
#include "cmd.h"

#define COMMAND_NAME "merge"

static int no_commit = 0;
static char **merge_heads = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "no-commit", 0, &no_commit, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "don't create a merge commit" },
	{ CLI_OPT_TYPE_ARGS, "heads", 0, &merge_heads, 0,
	  CLI_OPT_USAGE_REQUIRED, "commit",
	  "commit(s) to merge into current branch" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Join two or more development histories together.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Helper to resolve a ref-ish string to an annotated commit */
static int resolve_refish(
        git_annotated_commit **commit,
        git_repository *repo,
        const char *refish)
{
	git_reference *ref = NULL;
	git_object *obj = NULL;
	int ret = 0;

	/* Try as a reference first */
	ret = git_reference_dwim(&ref, repo, refish);
	if (ret == 0) {
		ret = git_annotated_commit_from_ref(commit, repo, ref);
		git_reference_free(ref);
		return ret;
	}

	/* Try as a revspec */
	if ((ret = git_revparse_single(&obj, repo, refish)) < 0) {
		cli_error_git();
		return ret;
	}

	ret = git_annotated_commit_lookup(commit, repo, git_object_id(obj));
	git_object_free(obj);

	if (ret < 0)
		cli_error_git();

	return ret;
}

/* Perform a fast-forward merge */
static int perform_fastforward(
        git_repository *repo,
        const git_oid *target_oid,
        int is_unborn)
{
	git_checkout_options ff_checkout_options = GIT_CHECKOUT_OPTIONS_INIT;
	git_reference *target_ref = NULL;
	git_reference *new_target_ref = NULL;
	git_object *target = NULL;
	int ret = 0;

	if (is_unborn) {
		const char *symbolic_ref;
		git_reference *head_ref = NULL;

		/* HEAD reference is unborn, lookup manually */
		if ((ret = git_reference_lookup(&head_ref, repo, "HEAD")) < 0) {
			cli_error_git();
			return ret;
		}

		/* Grab the reference HEAD should be pointing to */
		symbolic_ref = git_reference_symbolic_target(head_ref);

		/* Create our master reference on the target OID */
		ret = git_reference_create(
		        &target_ref, repo, symbolic_ref, target_oid, 0, NULL);
		git_reference_free(head_ref);

		if (ret < 0) {
			cli_error_git();
			return ret;
		}
	} else {
		/* HEAD exists, just lookup and resolve */
		if ((ret = git_repository_head(&target_ref, repo)) < 0) {
			cli_error_git();
			return ret;
		}
	}

	/* Lookup the target object */
	if ((ret = git_object_lookup(
	             &target, repo, target_oid, GIT_OBJECT_COMMIT)) < 0) {
		cli_error_git();
		goto done;
	}

	/* Checkout the result so the workdir is in the expected state */
	ff_checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;
	if ((ret = git_checkout_tree(repo, target, &ff_checkout_options)) < 0) {
		cli_error_git();
		goto done;
	}

	/* Move the target reference to the target OID */
	if ((ret = git_reference_set_target(
	             &new_target_ref, target_ref, target_oid, NULL)) < 0) {
		cli_error_git();
		goto done;
	}

	git_reference_free(new_target_ref);

done:
	git_reference_free(target_ref);
	git_object_free(target);
	return ret;
}

/* Output conflicts */
static void output_conflicts(git_index *index)
{
	git_index_conflict_iterator *conflicts = NULL;
	const git_index_entry *ancestor = NULL;
	const git_index_entry *our = NULL;
	const git_index_entry *their = NULL;
	int ret = 0;

	if (git_index_conflict_iterator_new(&conflicts, index) < 0) {
		cli_error_git();
		return;
	}

	while ((ret = git_index_conflict_next(
	                &ancestor, &our, &their, conflicts)) == 0) {
		fprintf(stderr, "conflict: a:%s o:%s t:%s\n",
		        ancestor ? ancestor->path : "NULL",
		        our ? our->path : "NULL", their ? their->path : "NULL");
	}

	if (ret != GIT_ITEROVER)
		cli_error_git();

	git_index_conflict_iterator_free(conflicts);
}

/* Create merge commit */
static int create_merge_commit(
        git_repository *repo,
        git_index *index,
        git_annotated_commit **annotated,
        size_t annotated_count,
        const char *merge_head_name)
{
	git_oid tree_oid, commit_oid;
	git_tree *tree = NULL;
	git_signature *sign = NULL;
	git_reference *head_ref = NULL;
	git_commit **parents = NULL;
	char *msg = NULL;
	size_t msglen = 0;
	size_t i;
	int ret = 0;

	/* Grab our needed references */
	if ((ret = git_repository_head(&head_ref, repo)) < 0) {
		cli_error_git();
		return ret;
	}

	/* Grab a signature */
	if ((ret = git_signature_default_from_env(&sign, NULL, repo)) < 0) {
		cli_error_git();
		goto done;
	}

	/* Prepare merge commit message */
	msglen = snprintf(NULL, 0, "Merge '%s'", merge_head_name);
	if (msglen > 0)
		msglen++;
	msg = malloc(msglen);
	if (!msg) {
		ret = -1;
		goto done;
	}
	snprintf(msg, msglen, "Merge '%s'", merge_head_name);

	/* Setup parent commits */
	parents = calloc(annotated_count + 1, sizeof(git_commit *));
	if (!parents) {
		ret = -1;
		goto done;
	}

	if ((ret = git_reference_peel(
	             (git_object **)&parents[0], head_ref, GIT_OBJECT_COMMIT)) <
	    0) {
		cli_error_git();
		goto done;
	}

	for (i = 0; i < annotated_count; i++) {
		if ((ret = git_commit_lookup(
		             &parents[i + 1], repo,
		             git_annotated_commit_id(annotated[i]))) < 0) {
			cli_error_git();
			goto done;
		}
	}

	/* Prepare our commit tree */
	if ((ret = git_index_write_tree(&tree_oid, index)) < 0) {
		cli_error_git();
		goto done;
	}

	if ((ret = git_tree_lookup(&tree, repo, &tree_oid)) < 0) {
		cli_error_git();
		goto done;
	}

	/* Commit time! */
	ret = git_commit_create(
	        &commit_oid, repo, git_reference_name(head_ref), sign, sign,
	        NULL, msg, tree, annotated_count + 1,
	        (const git_commit **)parents);

	if (ret < 0) {
		cli_error_git();
		goto done;
	}

	/* Cleanup repository state */
	git_repository_state_cleanup(repo);

	printf("Merge made\n");

done:
	if (parents) {
		for (i = 0; i <= annotated_count; i++)
			git_commit_free(parents[i]);
		free(parents);
	}
	free(msg);
	git_tree_free(tree);
	git_signature_free(sign);
	git_reference_free(head_ref);
	return ret;
}

int merge(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	cli_opt invalid_opt;
	git_annotated_commit **annotated = NULL;
	size_t annotated_count = 0;
	git_index *index = NULL;
	git_repository_state_t state;
	git_merge_analysis_t analysis;
	git_merge_preference_t preference;
	int ret = 0;
	size_t i;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (!merge_heads)
		return cli_error_usage_message("a commit to merge is required");

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Check repository state */
	state = git_repository_state(repo);
	if (state != GIT_REPOSITORY_STATE_NONE) {
		fprintf(stderr, "%s: repository is in unexpected state %d\n",
		        PROGRAM_NAME, state);
		ret = CLI_EXIT_ERROR;
		goto done;
	}

	/* Count merge heads */
	annotated_count = 0;
	while (merge_heads[annotated_count])
		annotated_count++;

	/* Resolve all merge heads to annotated commits */
	annotated = calloc(annotated_count, sizeof(git_annotated_commit *));
	if (!annotated) {
		ret = cli_error_message("out of memory");
		goto done;
	}

	for (i = 0; i < annotated_count; i++) {
		if ((ret = resolve_refish(
		             &annotated[i], repo, merge_heads[i])) < 0) {
			fprintf(stderr, "failed to resolve '%s'\n",
			        merge_heads[i]);
			goto done;
		}
	}

	/* Perform merge analysis */
	if ((ret = git_merge_analysis(
	             &analysis, &preference, repo,
	             (const git_annotated_commit **)annotated,
	             annotated_count)) < 0) {
		cli_error_git();
		goto done;
	}

	/* Handle the merge based on analysis */
	if (analysis & GIT_MERGE_ANALYSIS_UP_TO_DATE) {
		printf("Already up-to-date\n");
		ret = 0;
		goto done;
	} else if (
	        analysis & GIT_MERGE_ANALYSIS_UNBORN ||
	        (analysis & GIT_MERGE_ANALYSIS_FASTFORWARD &&
	         !(preference & GIT_MERGE_PREFERENCE_NO_FASTFORWARD))) {
		const git_oid *target_oid;

		if (analysis & GIT_MERGE_ANALYSIS_UNBORN)
			printf("Unborn\n");
		else
			printf("Fast-forward\n");

		/* Since this is a fast-forward, there can be only one merge
		 * head */
		target_oid = git_annotated_commit_id(annotated[0]);
		ret = perform_fastforward(
		        repo, target_oid,
		        (analysis & GIT_MERGE_ANALYSIS_UNBORN));
		goto done;
	} else if (analysis & GIT_MERGE_ANALYSIS_NORMAL) {
		git_merge_options merge_opts = GIT_MERGE_OPTIONS_INIT;
		git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;

		merge_opts.flags = 0;
		merge_opts.file_flags = GIT_MERGE_FILE_STYLE_DIFF3;

		checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE |
		                                  GIT_CHECKOUT_ALLOW_CONFLICTS;

		if (preference & GIT_MERGE_PREFERENCE_FASTFORWARD_ONLY) {
			printf("Fast-forward is preferred, but only a merge is possible\n");
			ret = -1;
			goto done;
		}

		if ((ret = git_merge(
		             repo, (const git_annotated_commit **)annotated,
		             annotated_count, &merge_opts, &checkout_opts)) <
		    0) {
			cli_error_git();
			goto done;
		}
	}

	/* Get the repository index */
	if ((ret = git_repository_index(&index, repo)) < 0) {
		cli_error_git();
		goto done;
	}

	if (git_index_has_conflicts(index)) {
		/* Handle conflicts */
		printf("Conflicts detected:\n");
		output_conflicts(index);
		ret = -1;
		goto done;
	} else if (!no_commit) {
		ret = create_merge_commit(
		        repo, index, annotated, annotated_count,
		        merge_heads[0]);
	}

done:
	if (annotated) {
		for (i = 0; i < annotated_count; i++)
			git_annotated_commit_free(annotated[i]);
		free(annotated);
	}
	git_index_free(index);
	git_repository_free(repo);
	return ret;
}
