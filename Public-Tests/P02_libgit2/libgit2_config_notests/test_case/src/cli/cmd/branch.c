#include <stdio.h>
#include <string.h>
#include <git2.h>
#include "common.h"
#include "cmd.h"

#define COMMAND_NAME "branch"

static int show_all = 0;
static int show_remote = 0;
static int force = 0;
static char *delete_name = NULL;
static char *branch_name_arg = NULL;
static char *target_spec_arg = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "all", 'a', &show_all, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "list both remote-tracking and local branches" },
	{ CLI_OPT_TYPE_SWITCH, "remotes", 'r', &show_remote, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "list or delete remote-tracking branches" },
	{ CLI_OPT_TYPE_SWITCH, "force", 'f', &force, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "force branch creation" },
	{ CLI_OPT_TYPE_VALUE, "delete", 'd', &delete_name, 0,
	  CLI_OPT_USAGE_DEFAULT, "branch", "delete a branch" },
	{ CLI_OPT_TYPE_ARG, "branch", 0, &branch_name_arg, 0,
	  CLI_OPT_USAGE_DEFAULT, "branch", "branch name to create" },
	{ CLI_OPT_TYPE_ARG, "start-point", 0, &target_spec_arg, 0,
	  CLI_OPT_USAGE_DEFAULT, "start-point",
	  "commit or reference to branch from" },
	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("List, create, or delete branches.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

static int list_branches(git_repository *repo)
{
	git_branch_iterator *iter = NULL;
	git_reference *branch = NULL, *head = NULL;
	git_branch_t branch_type, list_flags;
	const char *name, *head_name = NULL;
	int ret = 0, error;

	if (show_all)
		list_flags = GIT_BRANCH_ALL;
	else if (show_remote)
		list_flags = GIT_BRANCH_REMOTE;
	else
		list_flags = GIT_BRANCH_LOCAL;

	error = git_repository_head(&head, repo);
	if (!error)
		head_name = git_reference_shorthand(head);
	else if (error != GIT_EUNBORNBRANCH && error != GIT_ENOTFOUND)
		return cli_error_git();

	if (git_branch_iterator_new(&iter, repo, list_flags) < 0) {
		ret = cli_error_git();
		goto done;
	}

	while ((error = git_branch_next(&branch, &branch_type, iter)) == 0) {
		if (git_branch_name(&name, branch) < 0) {
			ret = cli_error_git();
			goto done;
		}

		printf("%c %s\n",
		       branch_type == GIT_BRANCH_LOCAL && head_name &&
		                       strcmp(name, head_name) == 0 ?
		               '*' :
		               ' ',
		       name);

		git_reference_free(branch);
		branch = NULL;
	}

	if (error != GIT_ITEROVER)
		ret = cli_error_git();

done:
	git_reference_free(branch);
	git_reference_free(head);
	git_branch_iterator_free(iter);
	return ret;
}

static int create_branch(
        git_repository *repo,
        const char *branch_name,
        const char *target_spec)
{
	git_reference *branch = NULL;
	git_object *target = NULL;
	git_commit *commit = NULL;
	int ret = 0;

	if (!target_spec)
		target_spec = "HEAD";

	if (git_revparse_single(&target, repo, target_spec) < 0 ||
	    git_object_peel((git_object **)&commit, target, GIT_OBJECT_COMMIT) <
	            0 ||
	    git_branch_create(&branch, repo, branch_name, commit, force) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Created branch '%s'\n", branch_name);

done:
	git_commit_free(commit);
	git_object_free(target);
	git_reference_free(branch);
	return ret;
}

static int delete_branch(git_repository *repo)
{
	git_reference *branch = NULL;
	git_branch_t branch_type = show_remote ? GIT_BRANCH_REMOTE :
	                                         GIT_BRANCH_LOCAL;
	int ret = 0;

	if (git_branch_lookup(&branch, repo, delete_name, branch_type) < 0 ||
	    git_branch_delete(branch) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("Deleted branch '%s'\n", delete_name);

done:
	git_reference_free(branch);
	return ret;
}

int branch_cmd(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
	cli_opt invalid_opt;
	const char *branch_name, *target_spec;
	int ret = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	branch_name = branch_name_arg;
	target_spec = target_spec_arg;

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	if (delete_name) {
		if (branch_name || show_all || force) {
			ret = cli_error_usage_message(
			        "delete cannot be combined with create or list options");
			goto done;
		}

		ret = delete_branch(repo);
		goto done;
	}

	if (branch_name) {
		if (show_all || show_remote) {
			ret = cli_error_usage_message(
			        "create cannot be combined with list options");
			goto done;
		}

		ret = create_branch(repo, branch_name, target_spec);
		goto done;
	}

	ret = list_branches(repo);

done:
	git_repository_free(repo);
	return ret;
}
