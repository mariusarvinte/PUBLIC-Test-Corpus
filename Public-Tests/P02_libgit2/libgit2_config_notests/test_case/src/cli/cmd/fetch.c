#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "fetch"

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

	printf("Download objects and refs from another repository.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

static int progress_cb(const char *str, int len, void *data)
{
	(void)data;
	printf("remote: %.*s", len, str);
	fflush(stdout); /* We don't have the \n to force the flush */
	return 0;
}

/**
 * This function gets called for each remote-tracking branch that gets
 * updated. The message we output depends on whether it's a new one or
 * an update.
 */
static int update_cb(const char *refname, const git_oid *a, const git_oid *b, git_refspec *spec, void *data)
{
	char a_str[GIT_OID_SHA1_HEXSIZE+1], b_str[GIT_OID_SHA1_HEXSIZE+1];
	git_buf remote_name = GIT_BUF_INIT;
	(void)data;

	if (git_refspec_rtransform(&remote_name, spec, refname) < 0)
		return -1;

	git_oid_fmt(b_str, b);
	b_str[GIT_OID_SHA1_HEXSIZE] = '\0';

	if (git_oid_is_zero(a)) {
		printf("[new]     %.20s %s -> %s\n", b_str, remote_name.ptr, refname);
	} else {
		git_oid_fmt(a_str, a);
		a_str[GIT_OID_SHA1_HEXSIZE] = '\0';
		printf("[updated] %.10s..%.10s %s -> %s\n", a_str, b_str, remote_name.ptr, refname);
	}

	return 0;
}

/**
 * This gets called during the download and indexing. Here we show
 * processed and total objects in the pack and the amount of received
 * data. Most frontends will probably want to show a percentage and
 * the download rate.
 */
static int transfer_progress_cb(const git_indexer_progress *stats, void *payload)
{
	(void)payload;

	if (stats->received_objects == stats->total_objects) {
		printf("Resolving deltas %u/%u\r",
		       stats->indexed_deltas, stats->total_deltas);
	} else if (stats->total_objects > 0) {
		printf("Received %u/%u objects (%u) in %" PRIuZ " bytes\r",
		       stats->received_objects, stats->total_objects,
		       stats->indexed_objects, stats->received_bytes);
	}
	return 0;
}

int fetch(int argc, char **argv) {
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1};
	git_remote *remote = NULL;
	git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
	int ret = 0;

	if (cli_opt_parse(&invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_DEFAULT))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (cli_repository_open(&repo, &open_opts) < 0)
		return cli_error_git();

	/* Figure out whether it's a named remote or a URL */
	printf("Fetching %s\n", remote_repo);
	if (git_remote_lookup(&remote, repo, remote_repo) < 0)
		if (git_remote_create_anonymous(&remote, repo, remote_repo) < 0)
			goto done;

	/* Set up the callbacks (only update_tips for now) */
	fetch_opts.callbacks.update_refs = &update_cb;
	fetch_opts.callbacks.sideband_progress = &progress_cb;
	fetch_opts.callbacks.transfer_progress = transfer_progress_cb;

	/**
	 * Perform the fetch with the configured refspecs from the
	 * config. Update the reflog for the updated references with
	 * "fetch".
	 */
	if ((ret = git_remote_fetch(remote, NULL, &fetch_opts, "fetch")) < 0)
		goto done;

done:
	git_remote_free(remote);
	git_repository_free(repo);
	return ret;
}
