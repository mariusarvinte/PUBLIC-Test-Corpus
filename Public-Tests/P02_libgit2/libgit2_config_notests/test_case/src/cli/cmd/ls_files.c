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

#define COMMAND_NAME "ls-files"

static int error_unmatch = 0;
static char **pathspecs = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "error-unmatch", 0, &error_unmatch, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "if any pathspec doesn't match, exit with error" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "pathspec", 0, &pathspecs, 0,
	  CLI_OPT_USAGE_DEFAULT, "pathspec",
	  "files to show (default: all files)" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Show information about files in the index.\n");
	printf("\n");
	printf("This command lists files that are in the index (staged area).\n");
	printf("Without any pathspecs, it shows all tracked files. With pathspecs,\n");
	printf("it only shows files matching those patterns.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

static int print_paths(git_index *index)
{
	size_t i;
	const git_index_entry *entry;
	int ret = 0;

	/* If no pathspecs, print all entries in the index */
	if (!pathspecs) {
		size_t entry_count = git_index_entrycount(index);

		for (i = 0; i < entry_count; i++) {
			entry = git_index_get_byindex(index, i);
			printf("%s\n", entry->path);
		}
		return 0;
	}

	/* Print only files matching the pathspecs */
	for (i = 0; pathspecs[i]; i++) {
		const char *path = pathspecs[i];

		entry = git_index_get_bypath(
		        index, path, GIT_INDEX_STAGE_NORMAL);

		if (entry) {
			printf("%s\n", path);
		} else if (error_unmatch) {
			fprintf(stderr,
			        "error: pathspec '%s' did not match any file(s) known to git.\n",
			        path);
			fprintf(stderr, "Did you forget to 'git add'?\n");
			ret = CLI_EXIT_ERROR;
			goto done;
		}
	}

done:
	return ret;
}

int ls_files(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	git_index *index = NULL;
	cli_opt invalid_opt;
	int ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	/* Open the repository in the current directory */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Get the repository index */
	if (git_repository_index(&index, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/* Print the paths */
	ret = print_paths(index);

done:
	git_index_free(index);
	git_repository_free(repo);
	return ret;
}
