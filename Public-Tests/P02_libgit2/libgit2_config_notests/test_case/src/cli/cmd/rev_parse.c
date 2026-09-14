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

#define COMMAND_NAME "rev-parse"

static int show_not = 0;
static char **revspecs = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "not", 0, &show_not, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "toggle the ^ prefix on output" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "revspec", 0, &revspecs, 0, CLI_OPT_USAGE_REQUIRED,
	  "revspec", "the revision specification" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Parse revision specifications.\n");
	printf("\n");
	printf("Parse revision (or range) specifications and display the\n");
	printf("resulting object IDs.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

static int parse_revision(git_repository *repo, const char *spec, int not )
{
	git_revspec rs;
	char str[GIT_OID_SHA1_HEXSIZE + 1];
	int ret = 0;

	if (git_revparse(&rs, repo, spec) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if ((rs.flags & GIT_REVSPEC_SINGLE) != 0) {
		git_oid_tostr(str, sizeof(str), git_object_id(rs.from));
		printf("%s%s\n", not ? "^" : "", str);
		git_object_free(rs.from);
	} else if ((rs.flags & GIT_REVSPEC_RANGE) != 0) {
		git_oid_tostr(str, sizeof(str), git_object_id(rs.to));
		printf("%s%s\n", not ? "^" : "", str);
		git_object_free(rs.to);

		if ((rs.flags & GIT_REVSPEC_MERGE_BASE) != 0) {
			git_oid base;

			if (git_merge_base(
			            &base, repo, git_object_id(rs.from),
			            git_object_id(rs.to)) < 0) {
				cli_error_git();
				git_object_free(rs.from);
				ret = CLI_EXIT_GIT;
				goto done;
			}

			git_oid_tostr(str, sizeof(str), &base);
			printf("%s%s\n", not ? "^" : "", str);
		}

		git_oid_tostr(str, sizeof(str), git_object_id(rs.from));
		printf("%s%s\n", not ? "" : "^", str);
		git_object_free(rs.from);
	} else {
		fprintf(stderr,
		        "%s: invalid results from git_revparse for '%s'\n",
		        PROGRAM_NAME, spec);
		ret = CLI_EXIT_ERROR;
	}

done:
	return ret;
}

int rev_parse(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	cli_opt invalid_opt;
	int ret = 0;
	int i;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (!revspecs) {
		ret = cli_error_usage_message(
		        "a revision specification is required");
		goto done;
	}

	/* Open the repository in the current directory */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Parse each revision specification */
	for (i = 0; revspecs[i]; i++) {
		if ((ret = parse_revision(repo, revspecs[i], show_not)) < 0)
			goto done;
	}

done:
	git_repository_free(repo);
	return ret;
}
