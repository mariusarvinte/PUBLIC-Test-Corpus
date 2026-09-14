/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <stdio.h>
#include <git2.h>
#include "common.h"
#include "cmd.h"

#define COMMAND_NAME "reset"

static int reset_type = GIT_RESET_MIXED;
static char *target_spec = NULL;
static char **pathspecs = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "soft", 0, &reset_type, GIT_RESET_SOFT,
	  CLI_OPT_USAGE_DEFAULT, NULL, "move HEAD only" },
	{ CLI_OPT_TYPE_SWITCH, "mixed", 0, &reset_type, GIT_RESET_MIXED,
	  CLI_OPT_USAGE_DEFAULT | CLI_OPT_USAGE_CHOICE, NULL,
	  "reset HEAD and index (default)" },
	{ CLI_OPT_TYPE_SWITCH, "hard", 0, &reset_type, GIT_RESET_HARD,
	  CLI_OPT_USAGE_DEFAULT | CLI_OPT_USAGE_CHOICE, NULL,
	  "reset HEAD, index, and working tree" },
	{ CLI_OPT_TYPE_ARG, "target", 0, &target_spec, 0, CLI_OPT_USAGE_DEFAULT,
	  "target", "commit, tag, or other revision to reset to" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "pathspec", 0, &pathspecs, 0,
	  CLI_OPT_USAGE_DEFAULT, "pathspec",
	  "paths to reset in the index (requires explicit target)" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Reset the current HEAD, index, or working tree to a target state.\n");
	printf("\n");

	printf("When pathspecs are provided, only index entries are reset and an explicit\n");
	printf("target must be supplied, for example: %s reset HEAD -- <path>...\n",
	       PROGRAM_NAME);
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

static int load_target(git_object **out, git_repository *repo, const char *spec)
{
	if (!spec)
		spec = "HEAD";

	if (git_revparse_single(out, repo, spec) < 0)
		return cli_error_git();

	return 0;
}

int reset_cmd(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
	cli_opt invalid_opt;
	git_object *target = NULL;
	git_strarray pathspec = { 0 };
	int pathspec_count, ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	pathspec_count = count_args(pathspecs);

	if (pathspec_count > 0) {
		if (reset_type != GIT_RESET_MIXED) {
			ret = cli_error_usage_message(
			        "pathspec reset only supports the default mixed mode");
			goto done;
		}

		if (!target_spec) {
			ret = cli_error_usage_message(
			        "pathspec reset requires an explicit target");
			goto done;
		}

		if ((ret = load_target(&target, repo, target_spec)) != 0)
			goto done;

		pathspec.strings = pathspecs;
		pathspec.count = (size_t)pathspec_count;

		if (git_reset_default(repo, target, &pathspec) < 0) {
			ret = cli_error_git();
			goto done;
		}
	} else {
		if ((ret = load_target(&target, repo, target_spec)) != 0)
			goto done;

		if (git_reset(repo, target, (git_reset_t)reset_type, NULL) <
		    0) {
			ret = cli_error_git();
			goto done;
		}
	}

done:
	git_object_free(target);
	git_repository_free(repo);
	return ret;
}
