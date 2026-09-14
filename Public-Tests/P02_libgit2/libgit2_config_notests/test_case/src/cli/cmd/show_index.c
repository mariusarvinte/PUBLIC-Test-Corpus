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

#define COMMAND_NAME "show-index"

static const cli_opt_spec opts[] = { CLI_COMMON_OPT, { 0 } };

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Display the contents of the index.\n");
	printf("\n");
	printf("This command shows detailed information about each entry in the\n");
	printf("git index (staging area), including the blob SHA, file mode,\n");
	printf("file size, and timestamp information.\n");
	printf("\n");
}

int show_index(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	git_index *index = NULL;
	size_t i, ecount;
	char out[GIT_OID_SHA1_HEXSIZE + 1];
	cli_opt invalid_opt;
	int ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	out[GIT_OID_SHA1_HEXSIZE] = '\0';

	/* Open the repository in the current directory */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Get the repository index */
	if (git_repository_index(&index, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/* Read the index from disk */
	if (git_index_read(index, 0) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/* Display all index entries */
	ecount = git_index_entrycount(index);
	if (!ecount) {
		printf("Empty index\n");
	}

	for (i = 0; i < ecount; ++i) {
		const git_index_entry *e = git_index_get_byindex(index, i);

		git_oid_fmt(out, &e->id);

		printf("File Path: %s\n", e->path);
		printf("    Stage: %d\n", git_index_entry_stage(e));
		printf(" Blob SHA: %s\n", out);
		printf("File Mode: %07o\n", e->mode);
		printf("File Size: %d bytes\n", (int)e->file_size);
		printf("Dev/Inode: %d/%d\n", (int)e->dev, (int)e->ino);
		printf("  UID/GID: %d/%d\n", (int)e->uid, (int)e->gid);
		printf("    ctime: %d\n", (int)e->ctime.seconds);
		printf("    mtime: %d\n", (int)e->mtime.seconds);
		printf("\n");
	}

done:
	git_index_free(index);
	git_repository_free(repo);
	return ret;
}
