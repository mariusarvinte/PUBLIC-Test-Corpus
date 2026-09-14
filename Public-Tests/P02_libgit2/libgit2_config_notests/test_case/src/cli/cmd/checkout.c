#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "checkout"

static int force, perf;
static int progress = 1;
static char *branch;

typedef struct {
	int force;
	int progress;
	int perf;
} checkout_options;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "force", 'f', &force, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "force the checkout to happen" },
	{ CLI_OPT_TYPE_BOOL, "progress", 0, &progress, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "show checkout progress, on by default" },
	{ CLI_OPT_TYPE_SWITCH, "perf", 'p', &perf, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "show performance data" },
	{ CLI_OPT_TYPE_ARG, "branch", 0, &branch, 0, CLI_OPT_USAGE_REQUIRED,
	  "branch", "branch to checkout" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Switches branches or restores working tree files.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

int resolve_refish(git_annotated_commit **commit, git_repository *repo, const char *refish)
{
	git_reference *ref;
	git_object *obj;
	int err = 0;

	assert(commit != NULL);

	err = git_reference_dwim(&ref, repo, refish);
	if (err == GIT_OK) {
		git_annotated_commit_from_ref(commit, repo, ref);
		git_reference_free(ref);
		return 0;
	}

	err = git_revparse_single(&obj, repo, refish);
	if (err == GIT_OK) {
		err = git_annotated_commit_lookup(commit, repo, git_object_id(obj));
		git_object_free(obj);
	}

	return err;
}

/**
 * This function is called when the checkout completes, and is used to report the
 * number of syscalls performed.
 */
static void print_perf_data(const git_checkout_perfdata *perfdata, void *payload)
{
	(void)payload;
	printf("perf: stat: %" PRIuZ " mkdir: %" PRIuZ " chmod: %" PRIuZ "\n",
	       perfdata->stat_calls, perfdata->mkdir_calls, perfdata->chmod_calls);
}

/**
 * This corresponds to `git switch --guess`: if a given ref does
 * not exist, git will by default try to guess the reference by
 * seeing whether any remote has a branch called <ref>. If there
 * is a single remote only that has it, then it is assumed to be
 * the desired reference and a local branch is created for it.
 *
 * The following is a simplified implementation. It will not try
 * to check whether the ref is unique across all remotes.
 */
static int guess_refish(git_annotated_commit **out, git_repository *repo, const char *ref)
{
	git_strarray remotes = { NULL, 0 };
	git_reference *remote_ref = NULL;
	int error;
	size_t i;

	if ((error = git_remote_list(&remotes, repo)) < 0)
		goto out;

	for (i = 0; i < remotes.count; i++) {
		char *refname = NULL;
		size_t reflen;

		reflen = snprintf(refname, 0, "refs/remotes/%s/%s", remotes.strings[i], ref);
		if ((refname = malloc(reflen + 1)) == NULL) {
			error = -1;
			goto next;
		}
		snprintf(refname, reflen + 1, "refs/remotes/%s/%s", remotes.strings[i], ref);

		if ((error = git_reference_lookup(&remote_ref, repo, refname)) < 0)
			goto next;

		break;
next:
		free(refname);
		if (error < 0 && error != GIT_ENOTFOUND)
			break;
	}

	if (!remote_ref) {
		error = GIT_ENOTFOUND;
		goto out;
	}

	if ((error = git_annotated_commit_from_ref(out, repo, remote_ref)) < 0)
		goto out;

out:
	git_reference_free(remote_ref);
	git_strarray_dispose(&remotes);
	return error;
}

/**
 * This function is called to report progression, ie. it's called once with
 * a NULL path and the number of total steps, then for each subsequent path,
 * the current completed_step value.
 */
static void print_checkout_progress(const char *path, size_t completed_steps, size_t total_steps, void *payload)
{
	(void)payload;
	if (path == NULL) {
		printf("checkout started: %" PRIuZ " steps\n", total_steps);
	} else {
		printf("checkout: %s %" PRIuZ "/%" PRIuZ "\n", path, completed_steps, total_steps);
	}
}

/**
 * This is the main "checkout <branch>" function, responsible for performing
 * a branch-based checkout.
 */
static int perform_checkout_ref(git_repository *repo, git_annotated_commit *target, const char *target_ref, checkout_options *opts)
{
	git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
	git_reference *ref = NULL, *branch = NULL;
	git_commit *target_commit = NULL;
	int err;

	/** Setup our checkout options from the parsed options */
	checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	if (opts->force)
		checkout_opts.checkout_strategy = GIT_CHECKOUT_FORCE;

	if (opts->progress)
		checkout_opts.progress_cb = print_checkout_progress;

	if (opts->perf)
		checkout_opts.perfdata_cb = print_perf_data;

	/** Grab the commit we're interested to move to */
	err = git_commit_lookup(&target_commit, repo, git_annotated_commit_id(target));
	if (err != 0) {
		cli_error_git();
		goto cleanup;
	}

	/**
	 * Perform the checkout so the workdir corresponds to what target_commit
	 * contains.
	 *
	 * Note that it's okay to pass a git_commit here, because it will be
	 * peeled to a tree.
	 */
	err = git_checkout_tree(repo, (const git_object *)target_commit, &checkout_opts);
	if (err != 0) {
		cli_error_git();
		goto cleanup;
	}

	/**
	 * Now that the checkout has completed, we have to update HEAD.
	 *
	 * Depending on the "origin" of target (ie. it's an OID or a branch name),
	 * we might need to detach HEAD.
	 */
	if (git_annotated_commit_ref(target)) {
		const char *target_head;

		if ((err = git_reference_lookup(&ref, repo, git_annotated_commit_ref(target))) < 0)
			goto error;

		if (git_reference_is_remote(ref)) {
			if ((err = git_branch_create_from_annotated(&branch, repo, target_ref, target, 0)) < 0)
				goto error;
			target_head = git_reference_name(branch);
		} else {
			target_head = git_annotated_commit_ref(target);
		}

		err = git_repository_set_head(repo, target_head);
	} else {
		err = git_repository_set_head_detached_from_annotated(repo, target);
	}

error:
	if (err != 0) {
		cli_error_git();
		goto cleanup;
	}

cleanup:
	git_commit_free(target_commit);
	git_reference_free(branch);
	git_reference_free(ref);

	return err;
}

int checkout(int argc, char **argv) {
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1};
	git_repository_state_t state;
	git_annotated_commit *checkout_target = NULL;
	checkout_options checkout_opts;
	int ret = 0;

	if (cli_opt_parse(&invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_DEFAULT))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (cli_repository_open(&repo, &open_opts) < 0)
		return cli_error_git();

	/** Make sure we're not about to checkout while something else is going on */
	state = git_repository_state(repo);
	if (state != GIT_REPOSITORY_STATE_NONE) {
		cli_error_git();
		goto done;
	}

	checkout_opts.force = force;
	checkout_opts.progress = progress;
	checkout_opts.perf = perf;

	/**
	 * Try to resolve a "refish" argument to a target libgit2 can use
	 */
	if ((ret = resolve_refish(&checkout_target, repo, branch)) < 0 &&
	    (ret = guess_refish(&checkout_target, repo, branch)) < 0) {
		cli_error_git();
		goto done;
	}
	ret = perform_checkout_ref(repo, checkout_target, branch, &checkout_opts);

done:
	git_repository_free(repo);
	git_annotated_commit_free(checkout_target);
	return ret;
}
