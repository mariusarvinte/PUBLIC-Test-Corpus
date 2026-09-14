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

#define COMMAND_NAME "stash"

static char *subcmd = NULL;
static char **subcmd_args = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_ARG, "subcommand", 0, &subcmd, 0, CLI_OPT_USAGE_DEFAULT,
	  "subcommand", "the subcommand (list, push, pop, apply)" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &subcmd_args, 0, CLI_OPT_USAGE_DEFAULT,
	  "args", "arguments for the subcommand" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Stash the changes in a dirty working directory away.\n");
	printf("\n");

	printf("Usage:\n");
	printf("  %s stash list\n", PROGRAM_NAME);
	printf("  %s stash (pop | apply)\n", PROGRAM_NAME);
	printf("  %s stash [push]\n", PROGRAM_NAME);
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Subcommand: apply */
static int cmd_apply(git_repository *repo, int argc, char **argv)
{
	int ret = 0;

	(void)argv;

	if (argc > 0)
		return cli_error_usage_message(
		        "apply does not accept any parameters");

	if ((ret = git_stash_apply(repo, 0, NULL)) < 0)
		return cli_error_git();

	return 0;
}

/* Callback for listing stashes */
static int list_stash_cb(
        size_t index,
        const char *message,
        const git_oid *stash_id,
        void *payload)
{
	(void)stash_id;
	(void)payload;
	printf("stash@{%zu}: %s\n", index, message);
	return 0;
}

/* Subcommand: list */
static int cmd_list(git_repository *repo, int argc, char **argv)
{
	int ret = 0;

	(void)argv;

	if (argc > 0)
		return cli_error_usage_message(
		        "list does not accept any parameters");

	if ((ret = git_stash_foreach(repo, list_stash_cb, NULL)) < 0)
		return cli_error_git();

	return 0;
}

/* Subcommand: push */
static int cmd_push(git_repository *repo, int argc, char **argv)
{
	git_signature *signature = NULL;
	git_commit *stash = NULL;
	git_oid stashid;
	int ret = 0;

	(void)argv;

	if (argc > 0)
		return cli_error_usage_message(
		        "push does not accept any parameters");

	if ((ret = git_signature_default_from_env(&signature, NULL, repo)) <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	if ((ret = git_stash_save(
	             &stashid, repo, signature, NULL, GIT_STASH_DEFAULT)) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if ((ret = git_commit_lookup(&stash, repo, &stashid)) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Saved working directory %s\n", git_commit_summary(stash));

done:
	git_signature_free(signature);
	git_commit_free(stash);
	return ret;
}

/* Subcommand: pop */
static int cmd_pop(git_repository *repo, int argc, char **argv)
{
	int ret = 0;

	(void)argv;

	if (argc > 0)
		return cli_error_usage_message(
		        "pop does not accept any parameters");

	if ((ret = git_stash_pop(repo, 0, NULL)) < 0)
		return cli_error_git();

	printf("Dropped refs/stash@{0}\n");

	return 0;
}

int stash(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	cli_opt invalid_opt;
	int ret = 0;
	int subcmd_argc;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1,
	            CLI_OPT_PARSE_DEFAULT))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	/* Default to push if no subcommand specified */
	if (!subcmd)
		subcmd = "push";

	/* Open the repository */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Count subcommand arguments */
	subcmd_argc = 0;
	if (subcmd_args) {
		while (subcmd_args[subcmd_argc])
			subcmd_argc++;
	}

	/* Dispatch to subcommand */
	if (!strcmp(subcmd, "apply")) {
		ret = cmd_apply(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "list")) {
		ret = cmd_list(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "push")) {
		ret = cmd_push(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "pop")) {
		ret = cmd_pop(repo, subcmd_argc, subcmd_args);
	} else {
		fprintf(stderr, "%s: unknown subcommand '%s'\n", PROGRAM_NAME,
		        subcmd);
		ret = CLI_EXIT_USAGE;
	}

done:
	git_repository_free(repo);
	return ret;
}
