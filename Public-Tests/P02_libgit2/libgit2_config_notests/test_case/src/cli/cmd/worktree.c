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

#define COMMAND_NAME "worktree"

static char *subcmd = NULL;
static char **subcmd_args = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_ARG, "subcommand", 0, &subcmd, 0, CLI_OPT_USAGE_DEFAULT,
	  "subcommand", "the subcommand (list, add, lock, unlock, prune)" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &subcmd_args, 0, CLI_OPT_USAGE_DEFAULT,
	  "args", "arguments for the subcommand" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Manage multiple working trees attached to a repository.\n");
	printf("\n");

	printf("Subcommands:\n");
	printf("    list                          List linked working trees\n");
	printf("    add [--lock] [--force] <path> [<branch>]\n");
	printf("                                  Add a new working tree\n");
	printf("    lock <name> [reason]          Lock a worktree\n");
	printf("    unlock <name>                 Unlock a worktree\n");
	printf("    prune [--force] <name>        Prune a missing or invalid worktree\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

static int count_args(char **args)
{
	int argc = 0;

	if (!args)
		return 0;

	while (args[argc])
		argc++;

	return argc;
}

static const char *basename_of(const char *path)
{
	const char *end, *base;

	if (!path || !*path)
		return path;

	end = path + strlen(path);
	while (end > path && end[-1] == '/')
		end--;

	base = end;
	while (base > path && base[-1] != '/')
		base--;

	return base;
}

static int cmd_list(git_repository *repo, int argc, char **argv)
{
	git_strarray names = { 0 };
	git_worktree *wt = NULL;
	git_buf reason = GIT_BUF_INIT;
	int locked, ret = 0;
	size_t i;

	(void)argv;

	if (argc != 0)
		return cli_error_usage_message(
		        "list does not accept any parameters");

	if (git_worktree_list(&names, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	for (i = 0; i < names.count; i++) {
		git_buf_dispose(&reason);

		if (git_worktree_lookup(&wt, repo, names.strings[i]) < 0) {
			ret = cli_error_git();
			goto done;
		}

		locked = git_worktree_is_locked(&reason, wt);
		if (locked < 0) {
			ret = cli_error_git();
			goto done;
		}

		printf("%s %s", git_worktree_name(wt), git_worktree_path(wt));
		if (locked > 0) {
			if (reason.size > 0)
				printf(" [locked: %s]", reason.ptr);
			else
				printf(" [locked]");
		}
		printf("\n");

		git_worktree_free(wt);
		wt = NULL;
	}

done:
	git_buf_dispose(&reason);
	git_worktree_free(wt);
	git_strarray_dispose(&names);
	return ret;
}

static int cmd_add(git_repository *repo, int argc, char **argv)
{
	git_worktree *wt = NULL;
	git_worktree_add_options opts = GIT_WORKTREE_ADD_OPTIONS_INIT;
	git_reference *ref = NULL;
	const char *path = NULL, *branch = NULL, *name;
	int force = 0;
	int i, ret = 0;

	for (i = 0; i < argc; i++) {
		char *arg = argv[i];

		if (!strcmp(arg, "--lock")) {
			opts.lock = 1;
		} else if (!strcmp(arg, "--force")) {
			force = 1;
		} else if (arg[0] != '-' && !path) {
			path = arg;
		} else if (arg[0] != '-' && !branch) {
			branch = arg;
		} else {
			fprintf(stderr, "%s: invalid argument to add: %s\n",
			        PROGRAM_NAME, arg);
			return CLI_EXIT_USAGE;
		}
	}

	if (!path)
		return cli_error_usage_message("add requires <path>");

	name = basename_of(path);
	if (!name || !*name)
		return cli_error_usage_message(
		        "could not derive worktree name from path");

	if (branch) {
		if (git_reference_dwim(&ref, repo, branch) < 0) {
			ret = cli_error_git();
			goto done;
		}

		opts.ref = ref;
	}

	if (force)
		opts.checkout_existing = 1;

	if (git_worktree_add(&wt, repo, name, path, &opts) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Added worktree '%s' at '%s'\n", git_worktree_name(wt),
	       git_worktree_path(wt));

done:
	git_reference_free(ref);
	git_worktree_free(wt);
	return ret;
}

static int
lookup_worktree(git_worktree **out, git_repository *repo, const char *name)
{
	if (git_worktree_lookup(out, repo, name) < 0)
		return cli_error_git();

	return 0;
}

static int cmd_lock(git_repository *repo, int argc, char **argv)
{
	git_worktree *wt = NULL;
	const char *name, *reason = NULL;
	int ret = 0;

	if (argc < 1 || argc > 2)
		return cli_error_usage_message("lock requires <name> [reason]");

	name = argv[0];
	if (argc == 2)
		reason = argv[1];

	if ((ret = lookup_worktree(&wt, repo, name)) != 0)
		goto done;

	if (git_worktree_lock(wt, reason) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Locked worktree '%s'\n", name);

done:
	git_worktree_free(wt);
	return ret;
}

static int cmd_unlock(git_repository *repo, int argc, char **argv)
{
	git_worktree *wt = NULL;
	int ret = 0;

	if (argc != 1)
		return cli_error_usage_message("unlock requires <name>");

	if ((ret = lookup_worktree(&wt, repo, argv[0])) != 0)
		goto done;

	ret = git_worktree_unlock(wt);
	if (ret < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (ret > 0)
		printf("Worktree '%s' was not locked\n", argv[0]);
	else
		printf("Unlocked worktree '%s'\n", argv[0]);

	ret = 0;

done:
	git_worktree_free(wt);
	return ret;
}

static int cmd_prune(git_repository *repo, int argc, char **argv)
{
	git_worktree *wt = NULL;
	git_worktree_prune_options opts = GIT_WORKTREE_PRUNE_OPTIONS_INIT;
	const char *name = NULL;
	int i, ret = 0;

	for (i = 0; i < argc; i++) {
		char *arg = argv[i];

		if (!strcmp(arg, "--force")) {
			opts.flags = GIT_WORKTREE_PRUNE_VALID |
			             GIT_WORKTREE_PRUNE_LOCKED |
			             GIT_WORKTREE_PRUNE_WORKING_TREE;
		} else if (arg[0] != '-' && !name) {
			name = arg;
		} else {
			fprintf(stderr, "%s: invalid argument to prune: %s\n",
			        PROGRAM_NAME, arg);
			return CLI_EXIT_USAGE;
		}
	}

	if (!name)
		return cli_error_usage_message("prune requires <name>");

	if ((ret = lookup_worktree(&wt, repo, name)) != 0)
		goto done;

	ret = git_worktree_prune(wt, &opts);
	if (ret < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Pruned worktree '%s'\n", name);

done:
	git_worktree_free(wt);
	return ret;
}

int worktree_cmd(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
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
		subcmd = "list";

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	subcmd_argc = count_args(subcmd_args);

	if (!strcmp(subcmd, "list")) {
		ret = cmd_list(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "add")) {
		ret = cmd_add(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "lock")) {
		ret = cmd_lock(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "unlock")) {
		ret = cmd_unlock(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "prune")) {
		ret = cmd_prune(repo, subcmd_argc, subcmd_args);
	} else {
		fprintf(stderr, "%s: unknown subcommand '%s'\n", PROGRAM_NAME,
		        subcmd);
		ret = CLI_EXIT_USAGE;
	}

done:
	git_repository_free(repo);
	return ret;
}
