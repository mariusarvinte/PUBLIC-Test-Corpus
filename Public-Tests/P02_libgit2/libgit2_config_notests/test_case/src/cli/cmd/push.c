#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "push"

// Default to origin / main
static char *remote_repo = "origin";
static char *branch = "main";

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_ARG, "repository", 0, &remote_repo, 1,
	  CLI_OPT_USAGE_DEFAULT, "repository",
	  "The \"remote\" repository that is the source of a fetch or pull operation. Defaults to \"origin\"" },
	{ CLI_OPT_TYPE_ARG, "branch", 0, &branch, 1, CLI_OPT_USAGE_DEFAULT,
	  "branch",
	  "The branch to push to the remote repository. Defaults to \"main\"" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Update remote refs along with associated objects.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

int push(int argc, char **argv)
{
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
	git_push_options options;
	git_remote_callbacks callbacks;
	git_remote *remote = NULL;
	char base_refspec[] = "refs/head/";
	char *refspec;
	git_strarray refspecs;
	int ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1,
	            CLI_OPT_PARSE_DEFAULT))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (cli_repository_open(&repo, &open_opts) < 0)
		return cli_error_git();

	if ((ret = git_remote_lookup(&remote, repo, remote_repo)) < 0) {
		cli_error_git();
		goto done;
	}

	if ((ret = git_remote_init_callbacks(
	             &callbacks, GIT_REMOTE_CALLBACKS_VERSION)) < 0) {
		cli_error_git();
		goto done;
	}

#ifndef GIT_DEPRECATE_HARD
	if ((ret = git_push_init_options(&options, GIT_PUSH_OPTIONS_VERSION)) <
	    0) {
#else
	if ((ret = git_push_options_init(&options, GIT_PUSH_OPTIONS_VERSION)) <
	    0) {
#endif
		cli_error_git();
		goto done;
	}
	options.callbacks = callbacks;

	refspec = malloc(
	        (strlen(base_refspec) + strlen(branch)) * sizeof(char) + 1);
	sprintf(refspec, "refs/heads/%s", branch);
	refspecs.strings = &refspec;
	refspecs.count = 1;

	if ((ret = git_remote_push(remote, &refspecs, &options)) < 0) {
		cli_error_git();
		goto done;
	}

done:
	git_repository_free(repo);
	return ret;
}
