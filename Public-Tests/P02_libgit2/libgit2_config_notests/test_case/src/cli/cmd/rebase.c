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

#define COMMAND_NAME "rebase"

enum {
	REBASE_ACTION_START = 0,
	REBASE_ACTION_CONTINUE = 1,
	REBASE_ACTION_ABORT = 2
};

static int action = REBASE_ACTION_START;
static char *onto_name = NULL;
static char *upstream_name = NULL;
static char *branch_name = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "continue", 0, &action, REBASE_ACTION_CONTINUE,
	  CLI_OPT_USAGE_DEFAULT, NULL, "continue an in-progress rebase" },
	{ CLI_OPT_TYPE_SWITCH, "abort", 0, &action, REBASE_ACTION_ABORT,
	  CLI_OPT_USAGE_DEFAULT | CLI_OPT_USAGE_CHOICE, NULL,
	  "abort an in-progress rebase" },
	{ CLI_OPT_TYPE_VALUE, "onto", 0, &onto_name, 0, CLI_OPT_USAGE_DEFAULT,
	  "newbase", "rebase onto the given branch or commit" },
	{ CLI_OPT_TYPE_ARG, "upstream", 0, &upstream_name, 0,
	  CLI_OPT_USAGE_DEFAULT, "upstream",
	  "upstream branch or commit to rebase onto" },
	{ CLI_OPT_TYPE_ARG, "branch", 0, &branch_name, 0, CLI_OPT_USAGE_DEFAULT,
	  "branch", "branch to rebase (defaults to current branch)" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Reapply commits on top of another base tip.\n");
	printf("\n");

	printf("Examples:\n");
	printf("  %s rebase <upstream>\n", PROGRAM_NAME);
	printf("  %s rebase --onto <newbase> <upstream> [<branch>]\n",
	       PROGRAM_NAME);
	printf("  %s rebase --continue\n", PROGRAM_NAME);
	printf("  %s rebase --abort\n", PROGRAM_NAME);
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

static int resolve_refish(
        git_annotated_commit **out,
        git_repository *repo,
        const char *refish)
{
	git_reference *ref = NULL;
	git_object *obj = NULL;
	int ret = 0;

	if (!refish) {
		*out = NULL;
		return 0;
	}

	ret = git_reference_dwim(&ref, repo, refish);
	if (ret == 0) {
		ret = git_annotated_commit_from_ref(out, repo, ref);
		git_reference_free(ref);
		if (ret < 0)
			return cli_error_git();
		return 0;
	}

	if ((ret = git_revparse_single(&obj, repo, refish)) < 0) {
		return cli_error_git();
	}

	ret = git_annotated_commit_lookup(out, repo, git_object_id(obj));
	git_object_free(obj);

	if (ret < 0)
		return cli_error_git();

	return 0;
}

static int current_has_conflicts(git_repository *repo)
{
	git_index *index = NULL;
	int ret;

	if (git_repository_index(&index, repo) < 0)
		return -1;

	ret = git_index_has_conflicts(index);
	git_index_free(index);
	return ret;
}

static int step_rebase(git_rebase *rebase, git_repository *repo)
{
	git_rebase_operation *operation = NULL;
	git_signature *committer = NULL;
	git_oid commit_id;
	int error, ret = 0;

	while ((error = git_rebase_next(&operation, rebase)) == 0) {
		error = current_has_conflicts(repo);
		if (error < 0) {
			ret = cli_error_git();
			goto done;
		}

		if (error) {
			fprintf(stderr,
			        "%s: rebase stopped with conflicts; resolve them and run '%s rebase --continue'\n",
			        PROGRAM_NAME, PROGRAM_NAME);
			return CLI_EXIT_GIT;
		}

		if ((ret = git_signature_default_from_env(
		             &committer, NULL, repo)) < 0) {
			ret = cli_error_git();
			goto done;
		}

		error = git_rebase_commit(
		        &commit_id, rebase, NULL, committer, NULL, NULL);
		git_signature_free(committer);
		committer = NULL;

		if (error == GIT_EAPPLIED) {
			printf("skipped already-applied commit %s\n",
			       git_oid_tostr_s(&operation->id));
			continue;
		}

		if (error == GIT_EUNMERGED) {
			fprintf(stderr,
			        "%s: rebase stopped with conflicts; resolve them and run '%s rebase --continue'\n",
			        PROGRAM_NAME, PROGRAM_NAME);
			ret = CLI_EXIT_GIT;
			goto done;
		}

		if (error < 0) {
			ret = cli_error_git();
			goto done;
		}

		printf("applied %s\n", git_oid_tostr_s(&commit_id));
	}

	if (error != GIT_ITEROVER) {
		ret = cli_error_git();
		goto done;
	}

done:
	git_signature_free(committer);
	return ret;
}

static int cmd_start(git_repository *repo)
{
	git_rebase *rebase = NULL;
	git_annotated_commit *branch = NULL, *upstream = NULL, *onto = NULL;
	git_signature *signature = NULL;
	int ret = 0;

	if (!upstream_name)
		return cli_error_usage_message(
		        "an upstream commit is required");

	if ((ret = resolve_refish(&branch, repo, branch_name)) < 0 ||
	    (ret = resolve_refish(&upstream, repo, upstream_name)) < 0 ||
	    (ret = resolve_refish(&onto, repo, onto_name)) < 0)
		goto done;

	if (git_rebase_init(&rebase, repo, branch, upstream, onto, NULL) < 0) {
		ret = cli_error_git();
		goto done;
	}

	ret = step_rebase(rebase, repo);
	if (ret != 0)
		goto done;

	if (git_signature_default_from_env(&signature, NULL, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_rebase_finish(rebase, signature) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Successfully rebased%s%s\n", branch_name ? " " : "",
	       branch_name ? branch_name : " current branch");

done:
	git_signature_free(signature);
	git_rebase_free(rebase);
	git_annotated_commit_free(branch);
	git_annotated_commit_free(upstream);
	git_annotated_commit_free(onto);
	return ret;
}

static int cmd_continue(git_repository *repo)
{
	git_rebase *rebase = NULL;
	git_signature *committer = NULL, *finisher = NULL;
	git_oid commit_id;
	int ret = 0;

	if (upstream_name || branch_name || onto_name)
		return cli_error_usage_message(
		        "--continue does not accept branch arguments");

	if (git_rebase_open(&rebase, repo, NULL) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_signature_default_from_env(&committer, NULL, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_rebase_commit(&commit_id, rebase, NULL, committer, NULL, NULL) <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	printf("applied %s\n", git_oid_tostr_s(&commit_id));

	ret = step_rebase(rebase, repo);
	if (ret != 0)
		goto done;

	if (git_signature_default_from_env(&finisher, NULL, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_rebase_finish(rebase, finisher) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Successfully rebased current branch\n");

done:
	git_signature_free(committer);
	git_signature_free(finisher);
	git_rebase_free(rebase);
	return ret;
}

static int cmd_abort(git_repository *repo)
{
	git_rebase *rebase = NULL;
	int ret = 0;

	if (upstream_name || branch_name || onto_name)
		return cli_error_usage_message(
		        "--abort does not accept branch arguments");

	if (git_rebase_open(&rebase, repo, NULL) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_rebase_abort(rebase) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Rebase aborted\n");

done:
	git_rebase_free(rebase);
	return ret;
}

int rebase_cmd(int argc, char **argv)
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

	if (action == REBASE_ACTION_CONTINUE)
		ret = cmd_continue(repo);
	else if (action == REBASE_ACTION_ABORT)
		ret = cmd_abort(repo);
	else
		ret = cmd_start(repo);

done:
	git_repository_free(repo);
	return ret;
}
