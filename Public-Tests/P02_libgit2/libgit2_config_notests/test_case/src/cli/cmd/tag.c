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

#define COMMAND_NAME "tag"

static int show_annotated = 0;
static int show_force = 0;
static int show_num_lines = 0;
static char *pattern = NULL;
static char *message = NULL;
static char *delete_tag = NULL;
static char **tag_args = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "annotate", 'a', &show_annotated, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "create an annotated tag" },
	{ CLI_OPT_TYPE_SWITCH, "force", 'f', &show_force, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "replace existing tag" },
	{ CLI_OPT_TYPE_VALUE, "message", 'm', &message, 0,
	  CLI_OPT_USAGE_DEFAULT, "message", "tag message" },
	{ CLI_OPT_TYPE_VALUE, "list", 'l', &pattern, 0, CLI_OPT_USAGE_DEFAULT,
	  "pattern", "list tags matching pattern" },
	{ CLI_OPT_TYPE_VALUE, "delete", 'd', &delete_tag, 0,
	  CLI_OPT_USAGE_DEFAULT, "tagname", "delete a tag" },
	{ CLI_OPT_TYPE_VALUE, NULL, 'n', &show_num_lines, 0,
	  CLI_OPT_USAGE_DEFAULT, "n", "print n lines of tag message" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &tag_args, 0, CLI_OPT_USAGE_DEFAULT,
	  "args", "tag name and optional target" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Create, list, or delete tags.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Print tag/commit message lines */
static void print_list_lines(const char *message, int num_lines)
{
	const char *msg = message;
	int num = num_lines - 1;

	if (!msg)
		return;

	/* First line - headline */
	while (*msg && *msg != '\n')
		printf("%c", *msg++);

	/* Skip over newlines */
	while (*msg && *msg == '\n')
		msg++;

	printf("\n");

	/* Print just headline? */
	if (num == 0)
		return;
	if (*msg && msg[1])
		printf("\n");

	/* Print individual commit/tag lines */
	while (*msg && num-- >= 2) {
		printf("    ");

		while (*msg && *msg != '\n')
			printf("%c", *msg++);

		/* Handle consecutive newlines */
		if (*msg && *msg == '\n' && msg[1] == '\n') {
			num--;
			printf("\n");
		}
		while (*msg && *msg == '\n')
			msg++;

		printf("\n");
	}
}

/* Print a tag object */
static void print_tag(git_tag *tag, int num_lines)
{
	printf("%-16s", git_tag_name(tag));

	if (num_lines) {
		const char *msg = git_tag_message(tag);
		print_list_lines(msg, num_lines);
	} else {
		printf("\n");
	}
}

/* Print a commit (lightweight tag target) */
static void print_commit(git_commit *commit, const char *name, int num_lines)
{
	printf("%-16s", name);

	if (num_lines) {
		const char *msg = git_commit_message(commit);
		print_list_lines(msg, num_lines);
	} else {
		printf("\n");
	}
}

/* Process and print a single tag */
static int print_tag_ref(git_repository *repo, const char *name, int num_lines)
{
	git_object *obj = NULL;
	int ret = 0;

	if (git_revparse_single(&obj, repo, name) < 0) {
		ret = cli_error_git();
		goto done;
	}

	switch (git_object_type(obj)) {
	case GIT_OBJECT_TAG:
		print_tag((git_tag *)obj, num_lines);
		break;
	case GIT_OBJECT_COMMIT:
		print_commit((git_commit *)obj, name, num_lines);
		break;
	default:
		printf("%s\n", name);
	}

done:
	git_object_free(obj);
	return ret;
}

/* List tags */
static int cmd_list_tags(git_repository *repo)
{
	git_strarray tag_names = { 0 };
	size_t i;
	int ret = 0;

	if (git_tag_list_match(&tag_names, pattern ? pattern : "*", repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	for (i = 0; i < tag_names.count; i++) {
		if ((ret = print_tag_ref(
		             repo, tag_names.strings[i], show_num_lines)) < 0)
			goto done;
	}

done:
	git_strarray_dispose(&tag_names);
	return ret;
}

/* Delete a tag */
static int cmd_delete_tag(git_repository *repo, const char *tag_name)
{
	git_object *obj = NULL;
	git_buf abbrev_oid = { 0 };
	int ret = 0;

	if (!tag_name)
		return cli_error_usage_message("tag name required for delete");

	if (git_revparse_single(&obj, repo, tag_name) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_object_short_id(&abbrev_oid, obj) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_tag_delete(repo, tag_name) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Deleted tag '%s' (was %s)\n", tag_name, abbrev_oid.ptr);

done:
	git_buf_dispose(&abbrev_oid);
	git_object_free(obj);
	return ret;
}

/* Create lightweight tag */
static int cmd_create_lightweight_tag(
        git_repository *repo,
        const char *tag_name,
        const char *target)
{
	git_oid oid;
	git_object *obj = NULL;
	int ret = 0;

	if (!tag_name)
		return cli_error_usage_message("tag name required");

	if (!target)
		target = "HEAD";

	if (git_revparse_single(&obj, repo, target) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_tag_create_lightweight(&oid, repo, tag_name, obj, show_force) <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Created lightweight tag '%s'\n", tag_name);

done:
	git_object_free(obj);
	return ret;
}

/* Create annotated tag */
static int cmd_create_annotated_tag(
        git_repository *repo,
        const char *tag_name,
        const char *target)
{
	git_oid oid;
	git_object *obj = NULL;
	git_signature *tagger = NULL;
	int ret = 0;

	if (!tag_name)
		return cli_error_usage_message("tag name required");

	if (!message)
		return cli_error_usage_message(
		        "message required for annotated tag (use -m)");

	if (!target)
		target = "HEAD";

	if (git_revparse_single(&obj, repo, target) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_signature_default(&tagger, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_tag_create(
	            &oid, repo, tag_name, obj, tagger, message, show_force) <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Created annotated tag '%s'\n", tag_name);

done:
	git_object_free(obj);
	git_signature_free(tagger);
	return ret;
}

int tag(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	cli_opt invalid_opt;
	int ret = 0;
	const char *tag_name = NULL;
	const char *target = NULL;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1,
	            CLI_OPT_PARSE_DEFAULT))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	/* Open repository */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Extract tag name and target from args */
	if (tag_args) {
		tag_name = tag_args[0];
		if (tag_args[1])
			target = tag_args[1];
	}

	/* Determine action */
	if (delete_tag) {
		/* Delete tag */
		ret = cmd_delete_tag(repo, delete_tag);
	} else if (tag_name) {
		/* Create tag */
		if (show_annotated || message) {
			ret = cmd_create_annotated_tag(repo, tag_name, target);
		} else {
			ret = cmd_create_lightweight_tag(
			        repo, tag_name, target);
		}
	} else {
		/* List tags */
		ret = cmd_list_tags(repo);
	}

done:
	git_repository_free(repo);
	return ret;
}
