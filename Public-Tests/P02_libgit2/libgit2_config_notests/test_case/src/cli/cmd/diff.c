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

#define COMMAND_NAME "diff"

static const char *colors[] = {
	"\033[m", /* reset */
	"\033[1m", /* bold */
	"\033[31m", /* red */
	"\033[32m", /* green */
	"\033[36m" /* cyan */
};

enum {
	OUTPUT_DIFF = (1 << 0),
	OUTPUT_STAT = (1 << 1),
	OUTPUT_SHORTSTAT = (1 << 2),
	OUTPUT_NUMSTAT = (1 << 3),
	OUTPUT_SUMMARY = (1 << 4)
};

enum { CACHE_NORMAL = 0, CACHE_ONLY = 1, CACHE_NONE = 2 };

/* Options */
static int color = -1;
static int cached = 0;
static int output = 0;
static int format = GIT_DIFF_FORMAT_PATCH;
static int find_renames = 0;
static int find_copies = 0;
static int find_copies_harder = 0;
static char *treeish1 = NULL;
static char *treeish2 = NULL;
static char **pathspecs = NULL;

/* Diff options */
static git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
static git_diff_find_options findopts = GIT_DIFF_FIND_OPTIONS_INIT;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "patch", 'p', &output, OUTPUT_DIFF,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate patch" },
	{ CLI_OPT_TYPE_SWITCH, "cached", 0, &cached, CACHE_ONLY,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show diff of index" },
	{ CLI_OPT_TYPE_SWITCH, "stat", 0, &output, OUTPUT_STAT,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate diffstat" },
	{ CLI_OPT_TYPE_SWITCH, "numstat", 0, &output, OUTPUT_NUMSTAT,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate numeric diffstat" },
	{ CLI_OPT_TYPE_SWITCH, "shortstat", 0, &output, OUTPUT_SHORTSTAT,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate short diffstat" },
	{ CLI_OPT_TYPE_SWITCH, "summary", 0, &output, OUTPUT_SUMMARY,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate summary" },
	{ CLI_OPT_TYPE_SWITCH, "name-only", 0, &format,
	  GIT_DIFF_FORMAT_NAME_ONLY, CLI_OPT_USAGE_DEFAULT, NULL,
	  "show only names of changed files" },
	{ CLI_OPT_TYPE_SWITCH, "name-status", 0, &format,
	  GIT_DIFF_FORMAT_NAME_STATUS, CLI_OPT_USAGE_DEFAULT, NULL,
	  "show names and status of changed files" },
	{ CLI_OPT_TYPE_SWITCH, "raw", 0, &format, GIT_DIFF_FORMAT_RAW,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate diff in raw format" },
	{ CLI_OPT_TYPE_SWITCH, "color", 0, &color, 0, CLI_OPT_USAGE_DEFAULT,
	  NULL, "use color output" },
	{ CLI_OPT_TYPE_SWITCH, "no-color", 0, &color, -1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "do not use color output" },
	{ CLI_OPT_TYPE_BOOL, "renames", 'M', &find_renames, 0,
	  CLI_OPT_USAGE_DEFAULT, NULL, "detect renames" },
	{ CLI_OPT_TYPE_BOOL, "copies", 'C', &find_copies, 0,
	  CLI_OPT_USAGE_DEFAULT, NULL, "detect copies" },
	{ CLI_OPT_TYPE_SWITCH, "find-copies-harder", 0, &find_copies_harder, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "find copies harder" },
	{ CLI_OPT_TYPE_ARG, "tree1", 0, &treeish1, 0, CLI_OPT_USAGE_DEFAULT,
	  "tree-ish", "first tree-ish to compare" },
	{ CLI_OPT_TYPE_ARG, "tree2", 0, &treeish2, 0, CLI_OPT_USAGE_DEFAULT,
	  "tree-ish", "second tree-ish to compare" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "paths", 0, &pathspecs, 0, CLI_OPT_USAGE_DEFAULT,
	  "paths", "limit diff to specific paths" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Show changes between commits, commit and working tree, etc.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Color printer for diff output */
static int color_printer(
        const git_diff_delta *delta,
        const git_diff_hunk *hunk,
        const git_diff_line *line,
        void *data)
{
	int *last_color = data, color_code = 0;

	(void)delta;
	(void)hunk;

	if (*last_color >= 0) {
		switch (line->origin) {
		case GIT_DIFF_LINE_ADDITION:
			color_code = 3;
			break;
		case GIT_DIFF_LINE_DELETION:
			color_code = 2;
			break;
		case GIT_DIFF_LINE_ADD_EOFNL:
			color_code = 3;
			break;
		case GIT_DIFF_LINE_DEL_EOFNL:
			color_code = 2;
			break;
		case GIT_DIFF_LINE_FILE_HDR:
			color_code = 1;
			break;
		case GIT_DIFF_LINE_HUNK_HDR:
			color_code = 4;
			break;
		default:
			break;
		}

		if (color_code != *last_color) {
			if (*last_color == 1 || color_code == 1)
				fputs(colors[0], stdout);
			fputs(colors[color_code], stdout);
			*last_color = color_code;
		}
	}

	/* Print the diff line */
	if (line->origin == GIT_DIFF_LINE_CONTEXT ||
	    line->origin == GIT_DIFF_LINE_ADDITION ||
	    line->origin == GIT_DIFF_LINE_DELETION)
		fputc(line->origin, stdout);

	fwrite(line->content, 1, line->content_len, stdout);

	return 0;
}

/* Print diff stats */
static void diff_print_stats(git_diff *diff)
{
	git_diff_stats *stats = NULL;
	git_buf buf = GIT_BUF_INIT;
	git_diff_stats_format_t statsformat = 0;
	int ret;

	if ((ret = git_diff_get_stats(&stats, diff)) < 0) {
		cli_error_git();
		return;
	}

	if (output & OUTPUT_STAT)
		statsformat |= GIT_DIFF_STATS_FULL;
	if (output & OUTPUT_SHORTSTAT)
		statsformat |= GIT_DIFF_STATS_SHORT;
	if (output & OUTPUT_NUMSTAT)
		statsformat |= GIT_DIFF_STATS_NUMBER;
	if (output & OUTPUT_SUMMARY)
		statsformat |= GIT_DIFF_STATS_INCLUDE_SUMMARY;

	if ((ret = git_diff_stats_to_buf(&buf, stats, statsformat, 80)) < 0) {
		cli_error_git();
		goto done;
	}

	fputs(buf.ptr, stdout);

done:
	git_buf_dispose(&buf);
	git_diff_stats_free(stats);
}

/* Helper to convert treeish to tree */
static int
treeish_to_tree(git_tree **out, git_repository *repo, const char *treeish)
{
	git_object *obj = NULL;
	int ret = 0;

	if ((ret = git_revparse_single(&obj, repo, treeish)) < 0) {
		cli_error_git();
		return ret;
	}

	if ((ret = git_object_peel((git_object **)out, obj, GIT_OBJECT_TREE)) <
	    0) {
		cli_error_git();
		git_object_free(obj);
		return ret;
	}

	git_object_free(obj);
	return 0;
}

int diff(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	cli_opt invalid_opt;
	git_tree *t1 = NULL, *t2 = NULL;
	git_diff *diff = NULL;
	int ret = 0;
	size_t i;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Set up pathspec */
	if (pathspecs) {
		i = 0;
		while (pathspecs[i])
			i++;
		diffopts.pathspec.strings = pathspecs;
		diffopts.pathspec.count = i;
	}

	/* Parse tree-ish arguments if provided */
	if (treeish1 && (ret = treeish_to_tree(&t1, repo, treeish1)) < 0)
		goto done;
	if (treeish2 && (ret = treeish_to_tree(&t2, repo, treeish2)) < 0)
		goto done;

	/* Determine what type of diff to perform */
	if (t1 && t2) {
		/* tree-to-tree diff */
		ret = git_diff_tree_to_tree(&diff, repo, t1, t2, &diffopts);
	} else if (cached != CACHE_NORMAL) {
		/* diff against index */
		if (!t1 && (ret = treeish_to_tree(&t1, repo, "HEAD")) < 0)
			goto done;

		if (cached == CACHE_NONE)
			ret = git_diff_tree_to_workdir(
			        &diff, repo, t1, &diffopts);
		else
			ret = git_diff_tree_to_index(
			        &diff, repo, t1, NULL, &diffopts);
	} else if (t1) {
		/* tree to workdir with index */
		ret = git_diff_tree_to_workdir_with_index(
		        &diff, repo, t1, &diffopts);
	} else {
		/* index to workdir */
		ret = git_diff_index_to_workdir(&diff, repo, NULL, &diffopts);
	}

	if (ret < 0) {
		cli_error_git();
		goto done;
	}

	/* Apply rename/copy detection if requested */
	if (find_renames || find_copies || find_copies_harder) {
		if (find_renames)
			findopts.flags |= GIT_DIFF_FIND_RENAMES;
		if (find_copies)
			findopts.flags |= GIT_DIFF_FIND_COPIES;
		if (find_copies_harder)
			findopts.flags |= GIT_DIFF_FIND_COPIES_FROM_UNMODIFIED;

		if ((ret = git_diff_find_similar(diff, &findopts)) < 0) {
			cli_error_git();
			goto done;
		}
	}

	/* Default to diff output if nothing specified */
	if (!output)
		output = OUTPUT_DIFF;

	/* Print stats if requested */
	if (output != OUTPUT_DIFF)
		diff_print_stats(diff);

	/* Print diff if requested */
	if ((output & OUTPUT_DIFF) != 0) {
		if (color >= 0)
			fputs(colors[0], stdout);

		if ((ret = git_diff_print(
		             diff, format, color_printer, &color)) < 0) {
			cli_error_git();
			goto done;
		}

		if (color >= 0)
			fputs(colors[0], stdout);
	}

done:
	git_diff_free(diff);
	git_tree_free(t1);
	git_tree_free(t2);
	git_repository_free(repo);
	return ret;
}
