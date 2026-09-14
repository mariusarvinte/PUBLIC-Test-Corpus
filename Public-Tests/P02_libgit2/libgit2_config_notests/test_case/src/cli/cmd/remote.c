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

#define COMMAND_NAME "remote"

static char *subcmd = NULL;
static char **subcmd_args = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_ARG, "subcommand", 0, &subcmd, 0, CLI_OPT_USAGE_REQUIRED,
	  "command", "the subcommand (add, remove, rename, set-url, show)" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &subcmd_args, 0, CLI_OPT_USAGE_DEFAULT,
	  "args", "arguments for the subcommand" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Manage remotes.\n");
	printf("\n");

	printf("Subcommands:\n");
	printf("    add <name> <url>              Add a new remote\n");
	printf("    remove <name>                 Remove a remote\n");
	printf("    rename <old> <new>            Rename a remote\n");
	printf("    set-url [--push] <name> <url> Set remote URL\n");
	printf("    show [-v|--verbose]           Show remotes\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Subcommand: add <name> <url> */
static int cmd_add(git_repository *repo, int argc, char **argv)
{
	git_remote *remote = NULL;
	char *name, *url;
	int ret = 0;

	if (argc != 2)
		return cli_error_usage_message("add requires <name> and <url>");

	name = argv[0];
	url = argv[1];

	if (git_remote_create(&remote, repo, name, url) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Added remote '%s' at '%s'\n", name, url);

done:
	git_remote_free(remote);
	return ret;
}

/* Subcommand: remove <name> */
static int cmd_remove(git_repository *repo, int argc, char **argv)
{
	char *name;

	if (argc != 1)
		return cli_error_usage_message("remove requires <name>");

	name = argv[0];

	if (git_remote_delete(repo, name) < 0)
		return cli_error_git();

	printf("Removed remote '%s'\n", name);
	return 0;
}

/* Subcommand: rename <old> <new> */
static int cmd_rename(git_repository *repo, int argc, char **argv)
{
	char *old, *new;
	git_strarray problems = { 0 };
	size_t i;
	int ret = 0;

	if (argc != 2)
		return cli_error_usage_message(
		        "rename requires <old> and <new>");

	old = argv[0];
	new = argv[1];

	ret = git_remote_rename(&problems, repo, old, new);

	if (ret < 0) {
		cli_error_git();

		/* Print any problems that occurred */
		for (i = 0; i < problems.count; i++) {
			fprintf(stderr, "%s\n", problems.strings[i]);
		}

		git_strarray_dispose(&problems);
		return CLI_EXIT_GIT;
	}

	printf("Renamed remote '%s' to '%s'\n", old, new);
	git_strarray_dispose(&problems);
	return 0;
}

/* Subcommand: set-url [--push] <name> <url> */
static int cmd_seturl(git_repository *repo, int argc, char **argv)
{
	int i, push = 0;
	char *name = NULL, *url = NULL;
	int ret;

	for (i = 0; i < argc; i++) {
		char *arg = argv[i];

		if (!strcmp(arg, "--push")) {
			push = 1;
		} else if (arg[0] != '-' && name == NULL) {
			name = arg;
		} else if (arg[0] != '-' && url == NULL) {
			url = arg;
		} else {
			fprintf(stderr, "%s: invalid argument to set-url: %s\n",
			        PROGRAM_NAME, arg);
			return CLI_EXIT_USAGE;
		}
	}

	if (name == NULL || url == NULL)
		return cli_error_usage_message(
		        "set-url requires <name> and <url>");

	if (push)
		ret = git_remote_set_pushurl(repo, name, url);
	else
		ret = git_remote_set_url(repo, name, url);

	if (ret < 0)
		return cli_error_git();

	printf("Set %s URL for remote '%s' to '%s'\n", push ? "push" : "fetch",
	       name, url);
	return 0;
}

/* Subcommand: show [-v|--verbose] */
static int cmd_show(git_repository *repo, int argc, char **argv)
{
	int i, verbose = 0;
	const char *arg, *name, *fetch, *push;
	git_strarray remotes = { 0 };
	git_remote *remote = NULL;
	int ret = 0;

	/* Parse verbose flag */
	for (i = 0; i < argc; i++) {
		arg = argv[i];
		if (!strcmp(arg, "-v") || !strcmp(arg, "--verbose")) {
			verbose = 1;
		}
	}

	if (git_remote_list(&remotes, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	for (i = 0; i < (int)remotes.count; i++) {
		name = remotes.strings[i];

		if (!verbose) {
			printf("%s\n", name);
			continue;
		}

		if (git_remote_lookup(&remote, repo, name) < 0) {
			ret = cli_error_git();
			goto done;
		}

		fetch = git_remote_url(remote);
		if (fetch)
			printf("%s\t%s (fetch)\n", name, fetch);

		push = git_remote_pushurl(remote);
		/* Use fetch URL if no distinct push URL has been set */
		push = push ? push : fetch;
		if (push)
			printf("%s\t%s (push)\n", name, push);

		git_remote_free(remote);
		remote = NULL;
	}

done:
	git_remote_free(remote);
	git_strarray_dispose(&remotes);
	return ret;
}

int remote(int argc, char **argv)
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

	if (!subcmd)
		return cli_error_usage_message("a subcommand is required");

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
	if (!strcmp(subcmd, "add")) {
		ret = cmd_add(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "remove") || !strcmp(subcmd, "rm")) {
		ret = cmd_remove(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "rename")) {
		ret = cmd_rename(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "set-url")) {
		ret = cmd_seturl(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "show")) {
		ret = cmd_show(repo, subcmd_argc, subcmd_args);
	} else {
		fprintf(stderr, "%s: unknown subcommand '%s'\n", PROGRAM_NAME,
		        subcmd);
		ret = CLI_EXIT_USAGE;
	}

done:
	git_repository_free(repo);
	return ret;
}
