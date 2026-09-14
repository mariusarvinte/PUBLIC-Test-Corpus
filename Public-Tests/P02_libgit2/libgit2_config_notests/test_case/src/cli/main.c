/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <stdio.h>
#include <git2.h>
#include "common.h"
#include "cmd/cmd.h"

int cli_opt__show_help = 0;
int cli_opt__use_pager = 1;

static int show_version = 0;
static char *command = NULL;
static char **args = NULL;

const cli_opt_spec cli_common_opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "version", 0, &show_version, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "display the version" },
	{ CLI_OPT_TYPE_ARG, "command", 0, &command, 0, CLI_OPT_USAGE_REQUIRED,
	  "command", "the command to run" },
	{ CLI_OPT_TYPE_ARGS, "args", 0, &args, 0, CLI_OPT_USAGE_DEFAULT, "args",
	  "arguments for the command" },
	{ 0 }
};

const cli_cmd_spec cli_cmds[] = {
	{ "add", add, "Add file contents to the index" },
	{ "blame", blame, "Show the origin of each line of a file" },
	{ "branch", branch_cmd, "List, create, or delete branches" },
	{ "cat-file", cat_file, "Display an object in the repository" },
	{ "cherry-pick", cherry_pick_cmd,
	  "Apply the changes introduced by an existing commit" },
	{ "checkout", checkout,
	  "Switches branches or restores working tree files" },
	{ "clone", clone, "Clone a repository into a new directory" },
	{ "commit", commit, "Record changes to the repository" },
	{ "config", config, "View or set configuration values " },
	{ "demo", demo, "Demonstrate libgit2 functionality" },
	{ "describe", describe,
	  "Give an object a human readable name based on an available ref" },
	{ "diff", diff, "Show changes between commits, trees, etc" },
	{ "fetch", fetch, "Download objects and refs from another repository" },
	{ "for-each-ref", for_each_ref, "Output information on each ref" },
	{ "hash-object", hash_object,
	  "Hash a raw object and product its object ID" },
	{ "help", help, "Display help information" },
	{ "index-pack", index_pack, "Create an index for a packfile" },
	{ "init", init, "Create a new git repository" },
	{ "log", log_cmd, "Show commit logs" },
	{ "ls-files", ls_files, "Show information about files in the index" },
	{ "ls-remote", ls_remote, "List references in a remote repository" },
	{ "merge", merge, "Join development histories together" },
	{ "note", note_cmd, "Manage notes attached to objects" },
	{ "push", push, "Update remote refs along with associated objects" },
	{ "rebase", rebase_cmd, "Reapply commits on top of another base tip" },
	{ "remote", remote, "Manage remotes" },
	{ "reset", reset_cmd, "Reset HEAD, index, or working tree" },
	{ "rev-list", rev_list, "List commits in reverse chronological order" },
	{ "rev-parse", rev_parse, "Parse revision specifications" },
	{ "show-index", show_index, "Display the contents of the index" },
	{ "stash", stash, "Stash changes in a dirty working directory" },
	{ "status", status, "Show the working tree status" },
	{ "submodule", submodule_cmd, "Manage submodules" },
	{ "tag", tag, "Create, list, or delete tags" },
	{ "version", version, "Show application version information" },
	{ "worktree", worktree_cmd, "Manage multiple working trees" },
	{ NULL }
};

/*
 * Reorder the argv as it was given, since git has the notion of global
 * options (like `--help` or `-c key=val`) that we want to pass to the
 * subcommand, and that can appear early in the arguments, before the
 * command name. Put the command-name in argv[1] to allow easier parsing.
 */
static void reorder_args(char **argv, size_t first)
{
	char *tmp;
	size_t i;

	if (first == 1)
		return;

	tmp = argv[first];

	for (i = first; i > 1; i--)
		argv[i] = argv[i - 1];

	argv[1] = tmp;
}

/*
 * When invoked without a command, or just with `--help`, we invoke
 * the help command; but we want to preserve only arguments that would
 * be useful for that.
 */
static void help_args(int *argc, char **argv)
{
	cli_opt__show_help = 0;

	argv[0] = "help";
	*argc = 1;
}

static void version_args(int *argc, char **argv)
{
	argv[0] = "version";
	*argc = 1;
}

int main(int argc, char **argv)
{
	const cli_cmd_spec *cmd;
	cli_opt_parser optparser;
	cli_opt opt;
	int ret = 0;

	if (git_libgit2_init() < 0) {
		cli_error_message("failed to initialize libgit2");
		exit(CLI_EXIT_GIT);
	}

	cli_opt_parser_init(
	        &optparser, cli_common_opts, argv + 1, argc - 1,
	        CLI_OPT_PARSE_GNU);

	/* Parse the top-level (common) options and command information */
	while (cli_opt_parser_next(&opt, &optparser)) {
		if (!opt.spec) {
			cli_opt_status_fprint(stderr, PROGRAM_NAME, &opt);
			cli_opt_usage_fprint(
			        stderr, PROGRAM_NAME, NULL, cli_common_opts,
			        CLI_OPT_USAGE_SHOW_HIDDEN);
			ret = CLI_EXIT_USAGE;
			goto done;
		}

		/*
		 * When we see a command, stop parsing and capture the
		 * remaining arguments as args for the command itself.
		 */
		if (command) {
			reorder_args(argv, optparser.idx);
			break;
		}
	}

	if (show_version) {
		version_args(&argc, argv);
		ret = version(argc, argv);
		goto done;
	}

	if (!command) {
		help_args(&argc, argv);
		ret = help(argc, argv);
		goto done;
	}

	if ((cmd = cli_cmd_spec_byname(command)) == NULL) {
		fprintf(stderr,
		        "%s: '%s' is not a %s command. See '%s help'.\n",
		        PROGRAM_NAME, command, PROGRAM_NAME, PROGRAM_NAME);
		ret = CLI_EXIT_ERROR;
		goto done;
	}

	ret = cmd->fn(argc - 1, &argv[1]);

done:
	git_libgit2_shutdown();
	return ret;
}
