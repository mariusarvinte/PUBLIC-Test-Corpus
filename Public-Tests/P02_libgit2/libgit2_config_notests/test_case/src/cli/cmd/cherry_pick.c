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

#define COMMAND_NAME "cherry-pick"

enum {
	CHERRY_PICK_ACTION_START = 0,
	CHERRY_PICK_ACTION_CONTINUE = 1,
	CHERRY_PICK_ACTION_ABORT = 2
};

static int action = CHERRY_PICK_ACTION_START;
static char *mainline_str = NULL;
static char *target_spec = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "continue", 0, &action,
	  CHERRY_PICK_ACTION_CONTINUE, CLI_OPT_USAGE_DEFAULT, NULL,
	  "continue an in-progress cherry-pick" },
	{ CLI_OPT_TYPE_SWITCH, "abort", 0, &action, CHERRY_PICK_ACTION_ABORT,
	  CLI_OPT_USAGE_DEFAULT | CLI_OPT_USAGE_CHOICE, NULL,
	  "abort an in-progress cherry-pick" },
	{ CLI_OPT_TYPE_VALUE, "mainline", 'm', &mainline_str, 0,
	  CLI_OPT_USAGE_DEFAULT, "parent-number",
	  "select mainline parent for merge commits" },
	{ CLI_OPT_TYPE_ARG, "commit", 0, &target_spec, 0, CLI_OPT_USAGE_DEFAULT,
	  "commit", "commit to cherry-pick" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Apply the changes introduced by an existing commit.\n");
	printf("\n");

	printf("Examples:\n");
	printf("  %s cherry-pick <commit>\n", PROGRAM_NAME);
	printf("  %s cherry-pick -m 1 <merge-commit>\n", PROGRAM_NAME);
	printf("  %s cherry-pick --continue\n", PROGRAM_NAME);
	printf("  %s cherry-pick --abort\n", PROGRAM_NAME);
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

static int parse_mainline(unsigned int *out)
{
	char *end = NULL;
	long value;

	if (!mainline_str) {
		*out = 0;
		return 0;
	}

	value = strtol(mainline_str, &end, 10);
	if (!mainline_str[0] || (end && *end) || value <= 0) {
		fprintf(stderr, "%s: invalid mainline parent '%s'\n",
		        PROGRAM_NAME, mainline_str);
		return CLI_EXIT_USAGE;
	}

	*out = (unsigned int)value;
	return 0;
}

static int
resolve_commit(git_commit **out, git_repository *repo, const char *spec)
{
	git_object *obj = NULL;
	int ret = 0;

	if (git_revparse_single(&obj, repo, spec) < 0 ||
	    git_object_peel((git_object **)out, obj, GIT_OBJECT_COMMIT) < 0) {
		ret = cli_error_git();
		goto done;
	}

done:
	git_object_free(obj);
	return ret;
}

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

static int create_cherry_pick_commit(git_repository *repo, git_commit *picked)
{
	git_reference *head_ref = NULL;
	git_commit *parent = NULL;
	git_index *index = NULL;
	git_tree *tree = NULL;
	git_signature *author = NULL, *committer = NULL;
	git_buf message = GIT_BUF_INIT;
	git_oid tree_oid, commit_oid;
	const char *commit_message, *commit_encoding;
	int ret = 0;

	if (git_repository_head(&head_ref, repo) < 0 ||
	    git_reference_peel(
	            (git_object **)&parent, head_ref, GIT_OBJECT_COMMIT) < 0 ||
	    git_repository_index(&index, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_index_has_conflicts(index)) {
		fprintf(stderr,
		        "%s: cherry-pick stopped with conflicts; resolve them and run '%s cherry-pick --continue'\n",
		        PROGRAM_NAME, PROGRAM_NAME);
		output_conflicts(index);
		ret = CLI_EXIT_GIT;
		goto done;
	}

	if (git_index_write_tree(&tree_oid, index) < 0 ||
	    git_tree_lookup(&tree, repo, &tree_oid) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_oid_equal(&tree_oid, git_commit_tree_id(parent))) {
		git_repository_state_cleanup(repo);
		printf("Cherry-pick is empty\n");
		ret = 0;
		goto done;
	}

	if (git_signature_dup(&author, git_commit_author(picked)) < 0 ||
	    git_signature_default_from_env(&committer, NULL, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_repository_message(&message, repo) < 0)
		git_error_clear();

	commit_message = message.size > 0 ? message.ptr :
	                                    git_commit_message(picked);
	commit_encoding = git_commit_message_encoding(picked);

	if (git_commit_create(
	            &commit_oid, repo, "HEAD", author, committer,
	            commit_encoding, commit_message, tree, 1,
	            (const git_commit **)&parent) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_repository_state_cleanup(repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	git_repository_message_remove(repo);
	printf("applied %s\n", git_oid_tostr_s(&commit_oid));

done:
	git_buf_dispose(&message);
	git_signature_free(author);
	git_signature_free(committer);
	git_tree_free(tree);
	git_index_free(index);
	git_commit_free(parent);
	git_reference_free(head_ref);
	return ret;
}

static int cmd_start(git_repository *repo)
{
	git_commit *picked = NULL;
	git_cherrypick_options opts = GIT_CHERRYPICK_OPTIONS_INIT;
	git_repository_state_t state;
	unsigned int mainline = 0;
	int ret = 0;

	if (!target_spec)
		return cli_error_usage_message(
		        "a commit to cherry-pick is required");

	state = git_repository_state(repo);
	if (state != GIT_REPOSITORY_STATE_NONE) {
		fprintf(stderr, "%s: repository is in unexpected state %d\n",
		        PROGRAM_NAME, state);
		return CLI_EXIT_ERROR;
	}

	if ((ret = parse_mainline(&mainline)) != 0 ||
	    (ret = resolve_commit(&picked, repo, target_spec)) != 0)
		goto done;

	opts.mainline = mainline;

	if (git_cherrypick(repo, picked, &opts) < 0) {
		ret = cli_error_git();
		goto done;
	}

	ret = create_cherry_pick_commit(repo, picked);

done:
	git_commit_free(picked);
	return ret;
}

static int cmd_continue(git_repository *repo)
{
	git_commit *picked = NULL;
	git_repository_state_t state;
	int ret = 0;

	if (target_spec || mainline_str)
		return cli_error_usage_message(
		        "--continue does not accept commit arguments");

	state = git_repository_state(repo);
	if (state != GIT_REPOSITORY_STATE_CHERRYPICK &&
	    state != GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE)
		return cli_error_usage_message(
		        "there is no cherry-pick in progress");

	if ((ret = resolve_commit(&picked, repo, "CHERRY_PICK_HEAD")) != 0)
		goto done;

	ret = create_cherry_pick_commit(repo, picked);

done:
	git_commit_free(picked);
	return ret;
}

static int cmd_abort(git_repository *repo)
{
	git_object *head = NULL;
	git_repository_state_t state;
	int ret = 0;

	if (target_spec || mainline_str)
		return cli_error_usage_message(
		        "--abort does not accept commit arguments");

	state = git_repository_state(repo);
	if (state != GIT_REPOSITORY_STATE_CHERRYPICK &&
	    state != GIT_REPOSITORY_STATE_CHERRYPICK_SEQUENCE)
		return cli_error_usage_message(
		        "there is no cherry-pick in progress");

	if (git_revparse_single(&head, repo, "HEAD") < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_reset(repo, head, GIT_RESET_HARD, NULL) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Cherry-pick aborted\n");

done:
	git_object_free(head);
	return ret;
}

int cherry_pick_cmd(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
	cli_opt invalid_opt;
	int ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	if (action == CHERRY_PICK_ACTION_CONTINUE)
		ret = cmd_continue(repo);
	else if (action == CHERRY_PICK_ACTION_ABORT)
		ret = cmd_abort(repo);
	else
		ret = cmd_start(repo);

done:
	git_repository_free(repo);
	return ret;
}
