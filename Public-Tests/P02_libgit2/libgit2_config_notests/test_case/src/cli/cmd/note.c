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

#define COMMAND_NAME "note"

static char *subcmd = NULL;
static char *notes_ref = NULL;
static int force = 0;
static char **subcmd_args = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_VALUE, "ref", 0, &notes_ref, 0, CLI_OPT_USAGE_DEFAULT,
	  "ref", "notes reference to use" },
	{ CLI_OPT_TYPE_SWITCH, "force", 'f', &force, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "overwrite an existing note when adding" },
	{ CLI_OPT_TYPE_ARG, "subcommand", 0, &subcmd, 0, CLI_OPT_USAGE_REQUIRED,
	  "command", "the subcommand (add, show, list, remove)" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &subcmd_args, 0, CLI_OPT_USAGE_DEFAULT,
	  "args", "arguments for the subcommand" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Manage notes attached to objects.\n");
	printf("\n");

	printf("Subcommands:\n");
	printf("    add <object> <message>        Add a note to an object\n");
	printf("    show <object>                 Show the note for an object\n");
	printf("    list                          List notes\n");
	printf("    remove <object>               Remove the note for an object\n");
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

static int
resolve_object_oid(git_oid *out, git_repository *repo, const char *spec)
{
	git_object *obj = NULL;
	int ret = 0;

	if (git_revparse_single(&obj, repo, spec) < 0) {
		ret = cli_error_git();
		goto done;
	}

	git_oid_cpy(out, git_object_id(obj));

done:
	git_object_free(obj);
	return ret;
}

static int default_signatures(
        git_signature **author,
        git_signature **committer,
        git_repository *repo)
{
	if (git_signature_default_from_env(author, committer, repo) < 0)
		return cli_error_git();

	return 0;
}

static int cmd_add(git_repository *repo, int argc, char **argv)
{
	git_signature *author = NULL, *committer = NULL;
	git_oid target, note_commit;
	int ret = 0;

	if (argc != 2)
		return cli_error_usage_message(
		        "add requires <object> and <message>");

	if ((ret = resolve_object_oid(&target, repo, argv[0])) != 0 ||
	    (ret = default_signatures(&author, &committer, repo)) != 0)
		goto done;

	if (git_note_create(
	            &note_commit, repo, notes_ref, author, committer, &target,
	            argv[1], force) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Added note for '%s'\n", argv[0]);

done:
	git_signature_free(author);
	git_signature_free(committer);
	return ret;
}

static int cmd_show(git_repository *repo, int argc, char **argv)
{
	git_note *note = NULL;
	git_oid target;
	const char *message;
	int ret = 0;
	size_t len;

	if (argc != 1)
		return cli_error_usage_message("show requires <object>");

	if ((ret = resolve_object_oid(&target, repo, argv[0])) != 0)
		goto done;

	if (git_note_read(&note, repo, notes_ref, &target) < 0) {
		ret = cli_error_git();
		goto done;
	}

	message = git_note_message(note);
	if (!message)
		goto done;

	if (fputs(message, stdout) < 0) {
		ret = cli_error_os();
		goto done;
	}

	len = strlen(message);
	if (len == 0 || message[len - 1] != '\n') {
		if (fputc('\n', stdout) == EOF)
			ret = cli_error_os();
	}

done:
	git_note_free(note);
	return ret;
}

static int cmd_list(git_repository *repo, int argc, char **argv)
{
	git_note_iterator *iter = NULL;
	git_oid note_oid, annotated_oid;
	char note_hex[GIT_OID_SHA1_HEXSIZE + 1];
	char annotated_hex[GIT_OID_SHA1_HEXSIZE + 1];
	int error, ret = 0;

	(void)argv;

	if (argc != 0)
		return cli_error_usage_message(
		        "list does not accept any parameters");

	if (git_note_iterator_new(&iter, repo, notes_ref) < 0) {
		ret = cli_error_git();
		goto done;
	}

	while ((error = git_note_next(&note_oid, &annotated_oid, iter)) == 0) {
		git_oid_tostr(note_hex, sizeof(note_hex), &note_oid);
		git_oid_tostr(
		        annotated_hex, sizeof(annotated_hex), &annotated_oid);
		printf("%s %s\n", note_hex, annotated_hex);
	}

	if (error != GIT_ITEROVER)
		ret = cli_error_git();

done:
	git_note_iterator_free(iter);
	return ret;
}

static int cmd_remove(git_repository *repo, int argc, char **argv)
{
	git_signature *author = NULL, *committer = NULL;
	git_oid target;
	int ret = 0;

	if (argc != 1)
		return cli_error_usage_message("remove requires <object>");

	if ((ret = resolve_object_oid(&target, repo, argv[0])) != 0 ||
	    (ret = default_signatures(&author, &committer, repo)) != 0)
		goto done;

	if (git_note_remove(repo, notes_ref, author, committer, &target) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Removed note for '%s'\n", argv[0]);

done:
	git_signature_free(author);
	git_signature_free(committer);
	return ret;
}

int note_cmd(int argc, char **argv)
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
		return cli_error_usage_message("a subcommand is required");

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	subcmd_argc = count_args(subcmd_args);

	if (!strcmp(subcmd, "add")) {
		ret = cmd_add(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "show")) {
		ret = cmd_show(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "list")) {
		ret = cmd_list(repo, subcmd_argc, subcmd_args);
	} else if (!strcmp(subcmd, "remove") || !strcmp(subcmd, "rm")) {
		ret = cmd_remove(repo, subcmd_argc, subcmd_args);
	} else {
		fprintf(stderr, "%s: unknown subcommand '%s'\n", PROGRAM_NAME,
		        subcmd);
		ret = CLI_EXIT_USAGE;
	}

done:
	git_repository_free(repo);
	return ret;
}
