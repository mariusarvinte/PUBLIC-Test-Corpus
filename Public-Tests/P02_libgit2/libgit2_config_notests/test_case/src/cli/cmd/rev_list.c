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

#define COMMAND_NAME "rev-list"

static int show_topo_order = 0;
static int show_date_order = 0;
static int show_reverse = 0;
static char **revspecs = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "topo-order", 0, &show_topo_order, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show commits in topological order" },
	{ CLI_OPT_TYPE_SWITCH, "date-order", 0, &show_date_order, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show commits in date order" },
	{ CLI_OPT_TYPE_SWITCH, "reverse", 0, &show_reverse, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "reverse the commit ordering" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "revspec", 0, &revspecs, 0, CLI_OPT_USAGE_REQUIRED,
	  "revspec", "revision specification(s)" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("List commits in reverse chronological order.\n");
	printf("\n");
	printf("This command lists commit objects in reverse chronological order,\n");
	printf("starting from the given commits. You can specify ranges, exclusions,\n");
	printf("and use --not to toggle including/excluding commits.\n");
	printf("\n");
	printf("Revision specifications:\n");
	printf("  <commit>          - Include commit and its ancestors\n");
	printf("  ^<commit>         - Exclude commit and its ancestors\n");
	printf("  <commit>..<commit> - Range (exclude first, include second)\n");
	printf("  --not <commits>   - Toggle exclusion for following commits\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Helper: push or hide a commit in the revwalk */
static int push_commit(git_revwalk *walk, const git_oid *oid, int hide)
{
	if (hide)
		return git_revwalk_hide(walk, oid);
	else
		return git_revwalk_push(walk, oid);
}

/* Helper: parse and push a single revision spec */
static int
push_spec(git_repository *repo, git_revwalk *walk, const char *spec, int hide)
{
	git_object *obj = NULL;
	int ret;

	if ((ret = git_revparse_single(&obj, repo, spec)) < 0)
		return ret;

	ret = push_commit(walk, git_object_id(obj), hide);
	git_object_free(obj);
	return ret;
}

/* Helper: parse and push a range spec (commit..commit) */
static int
push_range(git_repository *repo, git_revwalk *walk, const char *range, int hide)
{
	git_revspec revspec;
	int ret = 0;

	if ((ret = git_revparse(&revspec, repo, range)) < 0)
		return ret;

	if (revspec.flags & GIT_REVSPEC_MERGE_BASE) {
		/* TODO: support "<commit>...<commit>" */
		cli_error_message(
		        "merge base ranges (...) are not yet supported");
		ret = GIT_EINVALIDSPEC;
		goto done;
	}

	/* Push the 'from' commit as hidden (exclude) */
	if ((ret = push_commit(walk, git_object_id(revspec.from), !hide)) < 0)
		goto done;

	/* Push the 'to' commit as visible (include) */
	ret = push_commit(walk, git_object_id(revspec.to), hide);

done:
	git_object_free(revspec.from);
	git_object_free(revspec.to);
	return ret;
}

/* Parse revision specifications and configure the revwalk */
static int parse_revspecs(git_repository *repo, git_revwalk *walk)
{
	int hide = 0;
	int i, ret;
	git_oid oid;

	for (i = 0; revspecs[i]; i++) {
		const char *curr = revspecs[i];

		/* Handle --not flag */
		if (!strcmp(curr, "--not")) {
			hide = !hide;
		}
		/* Handle ^commit syntax (exclude) */
		else if (curr[0] == '^') {
			if ((ret = push_spec(repo, walk, curr + 1, !hide)) <
			    0) {
				cli_error_git();
				return CLI_EXIT_GIT;
			}
		}
		/* Handle range syntax (commit..commit) */
		else if (strstr(curr, "..")) {
			if ((ret = push_range(repo, walk, curr, hide)) < 0) {
				cli_error_git();
				return CLI_EXIT_GIT;
			}
		}
		/* Handle regular revision spec or raw OID */
		else {
			/* Try parsing as a revision spec first */
			if (push_spec(repo, walk, curr, hide) == 0)
				continue;

			/* If that fails, try as a raw OID */
			if ((ret = git_oid_fromstr(&oid, curr)) < 0) {
				cli_error_git();
				return CLI_EXIT_GIT;
			}

			if ((ret = push_commit(walk, &oid, hide)) < 0) {
				cli_error_git();
				return CLI_EXIT_GIT;
			}
		}
	}

	return 0;
}

int rev_list(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	git_revwalk *walk = NULL;
	git_oid oid;
	git_sort_t sort = GIT_SORT_NONE;
	char buf[GIT_OID_SHA1_HEXSIZE + 1];
	cli_opt invalid_opt;
	int ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (!revspecs) {
		ret = cli_error_usage_message(
		        "at least one revision specification is required");
		goto done;
	}

	/* Open the repository in the current directory */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Create the revwalk */
	if (git_revwalk_new(&walk, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/* Configure sorting */
	if (show_topo_order)
		sort |= GIT_SORT_TOPOLOGICAL;
	if (show_date_order)
		sort |= GIT_SORT_TIME;
	if (show_reverse)
		sort |= GIT_SORT_REVERSE;

	git_revwalk_sorting(walk, sort);

	/* Parse and push revision specs */
	if ((ret = parse_revspecs(repo, walk)) < 0)
		goto done;

	/* Walk and print commits */
	buf[GIT_OID_SHA1_HEXSIZE] = '\0';
	while (git_revwalk_next(&oid, walk) == 0) {
		git_oid_fmt(buf, &oid);
		printf("%s\n", buf);
	}

done:
	git_revwalk_free(walk);
	git_repository_free(repo);
	return ret;
}
