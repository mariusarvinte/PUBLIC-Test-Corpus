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

#define COMMAND_NAME "submodule"

typedef int (*submodule_action_fn)(git_submodule *sm, void *payload);

typedef struct {
	submodule_action_fn fn;
	void *payload;
} submodule_action_data;

typedef struct {
	git_repository *repo;
	int init;
} submodule_update_data;

static char *subcmd = NULL;
static char **subcmd_args = NULL;
static int update_init = 0;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "init", 0, &update_init, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "initialize submodules before updating" },
	{ CLI_OPT_TYPE_ARG, "subcommand", 0, &subcmd, 0, CLI_OPT_USAGE_DEFAULT,
	  "subcommand", "the subcommand (status, add, init, update, sync)" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &subcmd_args, 0, CLI_OPT_USAGE_DEFAULT,
	  "args", "arguments for the subcommand" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Manage submodules.\n");
	printf("\n");

	printf("Subcommands:\n");
	printf("    status [<name>...]            Show submodule status\n");
	printf("    add <url> <path>              Add a new submodule\n");
	printf("    init [<name>...]              Copy submodule config into .git/config\n");
	printf("    update [--init] [<name>...]   Clone or update submodules\n");
	printf("    sync [<name>...]              Sync submodule URL configuration\n");
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

static int foreach_cb(git_submodule *sm, const char *name, void *payload)
{
	submodule_action_data *data = payload;
	GIT_UNUSED(name);
	return data->fn(sm, data->payload);
}

static int apply_to_submodules(
        git_repository *repo,
        char **names,
        submodule_action_fn fn,
        void *payload)
{
	submodule_action_data data = { fn, payload };
	git_submodule *sm = NULL;
	int argc, i, ret = 0;

	argc = count_args(names);
	if (argc == 0)
		return git_submodule_foreach(repo, foreach_cb, &data) < 0 ?
		               cli_error_git() :
		               0;

	for (i = 0; i < argc; i++) {
		if (git_submodule_lookup(&sm, repo, names[i]) < 0) {
			ret = cli_error_git();
			goto done;
		}

		if ((ret = fn(sm, payload)) != 0)
			goto done;

		git_submodule_free(sm);
		sm = NULL;
	}

done:
	git_submodule_free(sm);
	return ret;
}

static int print_status(git_submodule *sm, void *payload)
{
	git_repository *repo = payload;
	unsigned int status = 0;
	const git_oid *oid;
	char oid_str[GIT_OID_SHA1_HEXSIZE + 1] = { 0 };
	char prefix = ' ';

	if (git_submodule_status(
	            &status, repo, git_submodule_name(sm),
	            GIT_SUBMODULE_IGNORE_UNSPECIFIED) < 0)
		return cli_error_git();

	if (status & GIT_SUBMODULE_STATUS_WD_UNINITIALIZED)
		prefix = '-';
	else if (!GIT_SUBMODULE_STATUS_IS_UNMODIFIED(status))
		prefix = '+';

	oid = git_submodule_wd_id(sm);
	if (!oid)
		oid = git_submodule_index_id(sm);
	if (!oid)
		oid = git_submodule_head_id(sm);

	if (oid)
		git_oid_tostr(oid_str, sizeof(oid_str), oid);
	else
		strcpy(oid_str, "0000000000000000000000000000000000000000");

	printf("%c%.7s %s\n", prefix, oid_str, git_submodule_path(sm));
	return 0;
}

static int init_submodule(git_submodule *sm, void *payload)
{
	GIT_UNUSED(payload);

	if (git_submodule_init(sm, 0) < 0)
		return cli_error_git();

	printf("Initialized submodule '%s'\n", git_submodule_name(sm));
	return 0;
}

static int sync_submodule(git_submodule *sm, void *payload)
{
	GIT_UNUSED(payload);

	if (git_submodule_sync(sm) < 0)
		return cli_error_git();

	printf("Synchronized submodule '%s'\n", git_submodule_name(sm));
	return 0;
}

static int update_submodule(git_submodule *sm, void *payload)
{
	submodule_update_data *data = payload;

	if (git_submodule_update(sm, data->init, NULL) < 0)
		return cli_error_git();

	printf("Updated submodule '%s'\n", git_submodule_name(sm));
	return 0;
}

static int cmd_status(git_repository *repo, int argc, char **argv)
{
	if (argc < 0)
		return cli_error_usage_message("invalid status arguments");

	return apply_to_submodules(repo, argv, print_status, repo);
}

static int cmd_add(git_repository *repo, int argc, char **argv)
{
	git_submodule *sm = NULL;
	git_repository *smrepo = NULL;
	int ret = 0;

	if (argc != 2)
		return cli_error_usage_message("add requires <url> and <path>");

	if (git_submodule_add_setup(&sm, repo, argv[0], argv[1], 1) < 0 ||
	    git_submodule_clone(&smrepo, sm, NULL) < 0 ||
	    git_submodule_add_finalize(sm) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Added submodule '%s' at '%s'\n", argv[1], argv[0]);

done:
	git_repository_free(smrepo);
	git_submodule_free(sm);
	return ret;
}

static int cmd_init(git_repository *repo, int argc, char **argv)
{
	if (argc < 0)
		return cli_error_usage_message("invalid init arguments");

	return apply_to_submodules(repo, argv, init_submodule, NULL);
}

static int cmd_sync(git_repository *repo, int argc, char **argv)
{
	if (argc < 0)
		return cli_error_usage_message("invalid sync arguments");

	return apply_to_submodules(repo, argv, sync_submodule, NULL);
}

static int cmd_update(git_repository *repo, int argc, char **argv)
{
	submodule_update_data data = { repo, update_init };
	char **names = argv;

	if (argc > 0 && !strcmp(argv[0], "--init")) {
		data.init = 1;
		names = argv + 1;
	}

	return apply_to_submodules(repo, names, update_submodule, &data);
}

int submodule_cmd(int argc, char **argv)
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
		subcmd = "status";

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	subcmd_argc = count_args(subcmd_args);

	if (!strcmp(subcmd, "status")) {
		ret = cmd_status(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "add")) {
		ret = cmd_add(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "init")) {
		ret = cmd_init(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "update")) {
		ret = cmd_update(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "sync")) {
		ret = cmd_sync(repo, subcmd_argc, subcmd_args);
	} else {
		fprintf(stderr, "%s: unknown subcommand '%s'\n", PROGRAM_NAME,
		        subcmd);
		ret = CLI_EXIT_USAGE;
	}

done:
	git_repository_free(repo);
	return ret;
}
