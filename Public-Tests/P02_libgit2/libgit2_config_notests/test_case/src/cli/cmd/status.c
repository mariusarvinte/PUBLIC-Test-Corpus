/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"

#define COMMAND_NAME "status"

enum {
	FORMAT_DEFAULT = 0,
	FORMAT_LONG = 1,
	FORMAT_SHORT = 2,
	FORMAT_PORCELAIN = 3
};

static int format = FORMAT_DEFAULT;
static int show_branch = 0;
static int show_submod = 0;
static int repeat_interval = 0;
static int include_ignored = 0;
static int include_untracked = 1;
static int recurse_untracked = 0;
static int exclude_submodules = 0;
static char **pathspecs = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "short", 's', &format, FORMAT_SHORT,
	  CLI_OPT_USAGE_DEFAULT, NULL, "give the output in the short-format" },
	{ CLI_OPT_TYPE_SWITCH, "long", 0, &format, FORMAT_LONG,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "give the output in the long-format (default)" },
	{ CLI_OPT_TYPE_SWITCH, "porcelain", 0, &format, FORMAT_PORCELAIN,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "give the output in an easy-to-parse format for scripts" },
	{ CLI_OPT_TYPE_SWITCH, "branch", 'b', &show_branch, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show the branch" },
	{ CLI_OPT_TYPE_SWITCH, "ignored", 0, &include_ignored, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show ignored files as well" },
	{ CLI_OPT_TYPE_VALUE, "untracked-files", 0, NULL, 0,
	  CLI_OPT_USAGE_DEFAULT, "mode",
	  "show untracked files (no, normal, all)" },
	{ CLI_OPT_TYPE_SWITCH, "list-submodules", 0, &show_submod, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "list submodules" },
	{ CLI_OPT_TYPE_VALUE, "repeat", 0, &repeat_interval, 0,
	  CLI_OPT_USAGE_DEFAULT, "n", "repeat status output every n seconds" },
	{ CLI_OPT_TYPE_ARGS, "pathspec", 0, &pathspecs, 0,
	  CLI_OPT_USAGE_DEFAULT, "pathspec", "limit to specific paths" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Show the working tree status.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

static void show_branch_name(git_repository *repo, int format_type)
{
	int error = 0;
	const char *branch = NULL;
	git_reference *head = NULL;

	error = git_repository_head(&head, repo);

	if (error == GIT_EUNBORNBRANCH || error == GIT_ENOTFOUND)
		branch = NULL;
	else if (!error)
		branch = git_reference_shorthand(head);
	else {
		cli_error_git();
		goto done;
	}

	if (format_type == FORMAT_LONG)
		printf("# On branch %s\n",
		       branch ? branch : "Not currently on any branch.");
	else
		printf("## %s\n", branch ? branch : "HEAD (no branch)");

done:
	git_reference_free(head);
}

static void print_long(git_status_list *status)
{
	size_t i, maxi = git_status_list_entrycount(status);
	const git_status_entry *s;
	int header = 0, changes_in_index = 0;
	int changed_in_workdir = 0, rm_in_workdir = 0;
	const char *old_path, *new_path;

	/* Print index changes */
	for (i = 0; i < maxi; ++i) {
		char *istatus = NULL;

		s = git_status_byindex(status, i);

		if (s->status == GIT_STATUS_CURRENT)
			continue;

		if (s->status & GIT_STATUS_WT_DELETED)
			rm_in_workdir = 1;

		if (s->status & GIT_STATUS_INDEX_NEW)
			istatus = "new file: ";
		if (s->status & GIT_STATUS_INDEX_MODIFIED)
			istatus = "modified: ";
		if (s->status & GIT_STATUS_INDEX_DELETED)
			istatus = "deleted:  ";
		if (s->status & GIT_STATUS_INDEX_RENAMED)
			istatus = "renamed:  ";
		if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
			istatus = "typechange:";

		if (istatus == NULL)
			continue;

		if (!header) {
			printf("# Changes to be committed:\n");
			printf("#   (use \"git reset HEAD <file>...\" to unstage)\n");
			printf("#\n");
			header = 1;
		}

		old_path = s->head_to_index->old_file.path;
		new_path = s->head_to_index->new_file.path;

		if (old_path && new_path && strcmp(old_path, new_path))
			printf("#\t%s  %s -> %s\n", istatus, old_path,
			       new_path);
		else
			printf("#\t%s  %s\n", istatus,
			       old_path ? old_path : new_path);
	}

	if (header) {
		changes_in_index = 1;
		printf("#\n");
	}
	header = 0;

	/* Print workdir changes to tracked files */
	for (i = 0; i < maxi; ++i) {
		char *wstatus = NULL;

		s = git_status_byindex(status, i);

		if (s->status == GIT_STATUS_CURRENT ||
		    s->index_to_workdir == NULL)
			continue;

		if (s->status & GIT_STATUS_WT_MODIFIED)
			wstatus = "modified: ";
		if (s->status & GIT_STATUS_WT_DELETED)
			wstatus = "deleted:  ";
		if (s->status & GIT_STATUS_WT_RENAMED)
			wstatus = "renamed:  ";
		if (s->status & GIT_STATUS_WT_TYPECHANGE)
			wstatus = "typechange:";

		if (wstatus == NULL)
			continue;

		if (!header) {
			printf("# Changes not staged for commit:\n");
			printf("#   (use \"git add%s <file>...\" to update what will be committed)\n",
			       rm_in_workdir ? "/rm" : "");
			printf("#   (use \"git checkout -- <file>...\" to discard changes in working directory)\n");
			printf("#\n");
			header = 1;
		}

		old_path = s->index_to_workdir->old_file.path;
		new_path = s->index_to_workdir->new_file.path;

		if (old_path && new_path && strcmp(old_path, new_path))
			printf("#\t%s  %s -> %s\n", wstatus, old_path,
			       new_path);
		else
			printf("#\t%s  %s\n", wstatus,
			       old_path ? old_path : new_path);
	}

	if (header) {
		changed_in_workdir = 1;
		printf("#\n");
	}

	/* Print untracked files */
	header = 0;

	for (i = 0; i < maxi; ++i) {
		s = git_status_byindex(status, i);

		if (s->status == GIT_STATUS_WT_NEW) {
			if (!header) {
				printf("# Untracked files:\n");
				printf("#   (use \"git add <file>...\" to include in what will be committed)\n");
				printf("#\n");
				header = 1;
			}

			printf("#\t%s\n", s->index_to_workdir->old_file.path);
		}
	}

	header = 0;

	/* Print ignored files */
	for (i = 0; i < maxi; ++i) {
		s = git_status_byindex(status, i);

		if (s->status == GIT_STATUS_IGNORED) {
			if (!header) {
				printf("# Ignored files:\n");
				printf("#   (use \"git add -f <file>...\" to include in what will be committed)\n");
				printf("#\n");
				header = 1;
			}

			printf("#\t%s\n", s->index_to_workdir->old_file.path);
		}
	}

	if (!changes_in_index && changed_in_workdir)
		printf("no changes added to commit (use \"git add\" and/or \"git commit -a\")\n");
}

static void print_short(git_repository *repo, git_status_list *status)
{
	size_t i, maxi = git_status_list_entrycount(status);
	const git_status_entry *s;
	char istatus, wstatus;
	const char *extra, *a, *b, *c;

	for (i = 0; i < maxi; ++i) {
		s = git_status_byindex(status, i);

		if (s->status == GIT_STATUS_CURRENT)
			continue;

		a = b = c = NULL;
		istatus = wstatus = ' ';
		extra = "";

		if (s->status & GIT_STATUS_INDEX_NEW)
			istatus = 'A';
		if (s->status & GIT_STATUS_INDEX_MODIFIED)
			istatus = 'M';
		if (s->status & GIT_STATUS_INDEX_DELETED)
			istatus = 'D';
		if (s->status & GIT_STATUS_INDEX_RENAMED)
			istatus = 'R';
		if (s->status & GIT_STATUS_INDEX_TYPECHANGE)
			istatus = 'T';

		if (s->status & GIT_STATUS_WT_NEW) {
			if (istatus == ' ')
				istatus = '?';
			wstatus = '?';
		}
		if (s->status & GIT_STATUS_WT_MODIFIED)
			wstatus = 'M';
		if (s->status & GIT_STATUS_WT_DELETED)
			wstatus = 'D';
		if (s->status & GIT_STATUS_WT_RENAMED)
			wstatus = 'R';
		if (s->status & GIT_STATUS_WT_TYPECHANGE)
			wstatus = 'T';

		if (s->status & GIT_STATUS_IGNORED) {
			istatus = '!';
			wstatus = '!';
		}

		if (istatus == '?' && wstatus == '?')
			continue;

		/* Check submodule status */
		if (s->index_to_workdir &&
		    s->index_to_workdir->new_file.mode == GIT_FILEMODE_COMMIT) {
			unsigned int smstatus = 0;

			if (!git_submodule_status(
			            &smstatus, repo,
			            s->index_to_workdir->new_file.path,
			            GIT_SUBMODULE_IGNORE_UNSPECIFIED)) {
				if (smstatus & GIT_SUBMODULE_STATUS_WD_MODIFIED)
					extra = " (new commits)";
				else if (
				        smstatus &
				        GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED)
					extra = " (modified content)";
				else if (
				        smstatus &
				        GIT_SUBMODULE_STATUS_WD_WD_MODIFIED)
					extra = " (modified content)";
				else if (smstatus & GIT_SUBMODULE_STATUS_WD_UNTRACKED)
					extra = " (untracked content)";
			}
		}

		if (s->head_to_index) {
			a = s->head_to_index->old_file.path;
			b = s->head_to_index->new_file.path;
		}
		if (s->index_to_workdir) {
			if (!a)
				a = s->index_to_workdir->old_file.path;
			if (!b)
				b = s->index_to_workdir->old_file.path;
			c = s->index_to_workdir->new_file.path;
		}

		if (istatus == 'R') {
			if (wstatus == 'R')
				printf("%c%c %s %s %s%s\n", istatus, wstatus, a,
				       b, c, extra);
			else
				printf("%c%c %s %s%s\n", istatus, wstatus, a, b,
				       extra);
		} else {
			if (wstatus == 'R')
				printf("%c%c %s %s%s\n", istatus, wstatus, a, c,
				       extra);
			else
				printf("%c%c %s%s\n", istatus, wstatus, a,
				       extra);
		}
	}

	for (i = 0; i < maxi; ++i) {
		s = git_status_byindex(status, i);

		if (s->status == GIT_STATUS_WT_NEW)
			printf("?? %s\n", s->index_to_workdir->old_file.path);
	}
}

static int print_submod(git_submodule *sm, const char *name, void *payload)
{
	int *count = payload;
	(void)name;

	if (*count == 0)
		printf("# Submodules\n");
	(*count)++;

	printf("# - submodule '%s' at %s\n", git_submodule_name(sm),
	       git_submodule_path(sm));

	return 0;
}

int status(int argc, char **argv)
{
	git_repository *repo = NULL;
	git_status_list *status = NULL;
	git_status_options statusopt = GIT_STATUS_OPTIONS_INIT;
	git_strarray pathspec_array = { 0 };
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

	if (cli_repository_open(&repo, &open_opts) < 0)
		return cli_error_git();

	if (git_repository_is_bare(repo)) {
		ret = cli_error_message(
		        "cannot report status on bare repository");
		goto done;
	}

	/* Set up status options */
	statusopt.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
	statusopt.flags = GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX |
	                  GIT_STATUS_OPT_SORT_CASE_SENSITIVELY;

	if (include_untracked)
		statusopt.flags |= GIT_STATUS_OPT_INCLUDE_UNTRACKED;
	if (recurse_untracked)
		statusopt.flags |= GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS;
	if (include_ignored)
		statusopt.flags |= GIT_STATUS_OPT_INCLUDE_IGNORED;
	if (exclude_submodules)
		statusopt.flags |= GIT_STATUS_OPT_EXCLUDE_SUBMODULES;

	if (pathspecs) {
		size_t i = 0;
		while (pathspecs[i])
			i++;
		pathspec_array.strings = pathspecs;
		pathspec_array.count = i;
		statusopt.pathspec = pathspec_array;
	}

	do {
		if (repeat_interval)
			printf("\033[H\033[2J");

		if ((ret = git_status_list_new(&status, repo, &statusopt)) < 0) {
			cli_error_git();
			goto done;
		}

		if (format == FORMAT_DEFAULT)
			format = FORMAT_LONG;
		if (format == FORMAT_LONG)
			show_branch = 1;

		if (show_branch)
			show_branch_name(repo, format);

		if (show_submod) {
			int submod_count = 0;
			if ((ret = git_submodule_foreach(
			             repo, print_submod, &submod_count)) < 0) {
				cli_error_git();
				goto done;
			}
		}

		if (format == FORMAT_LONG)
			print_long(status);
		else
			print_short(repo, status);

		git_status_list_free(status);
		status = NULL;

		if (repeat_interval)
			sleep(repeat_interval);
	} while (repeat_interval);

done:
	git_status_list_free(status);
	git_repository_free(repo);
	return ret;
}
