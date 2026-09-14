#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "ls-remote"

// Default to origin
static char *remote_repo = "origin";

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_ARG, "repository", 0, &remote_repo, 1, CLI_OPT_USAGE_DEFAULT,
	  "repository", "The \"remote\" repository that is the source of a fetch or pull operation. Defaults to \"origin\"" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("List references in a remote repository.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

static int use_remote(git_repository *repo, char *name)
{
	git_remote *remote = NULL;
	int error;
	const git_remote_head **refs;
	size_t refs_len, i;
	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;

	/* Find the remote by name */
	error = git_remote_lookup(&remote, repo, name);
	if (error < 0) {
		error = git_remote_create_anonymous(&remote, repo, name);
		if (error < 0)
			goto cleanup;
	}

	error = git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks);
	if (error < 0)
		goto cleanup;

	/**
	 * Get the list of references on the remote and print out
	 * their name next to what they point to.
	 */
	if (git_remote_ls(&refs, &refs_len, remote) < 0)
		goto cleanup;

	for (i = 0; i < refs_len; i++) {
		char oid[GIT_OID_SHA1_HEXSIZE + 1] = {0};
		git_oid_fmt(oid, &refs[i]->oid);
		printf("%s\t%s\n", oid, refs[i]->name);
	}

cleanup:
	git_remote_free(remote);
	return error;
}

int ls_remote(int argc, char **argv) {
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1};
	int ret = 0;

	if (cli_opt_parse(&invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_DEFAULT))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (cli_repository_open(&repo, &open_opts) < 0)
		return cli_error_git();

	ret = use_remote(repo, remote_repo);

	git_repository_free(repo);
	return ret;
}
