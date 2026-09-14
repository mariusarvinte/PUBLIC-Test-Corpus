#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "commit"

static char *message = "";

static const cli_opt_spec opts[] = { CLI_COMMON_OPT,

	                             { CLI_OPT_TYPE_VALUE, "message", 'm',
	                               &message, 0, CLI_OPT_USAGE_DEFAULT,
	                               "msg", "message for commit" },

	                             { 0 } };

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Record changes to the repository.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

int commit(int argc, char **argv)
{
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
	git_oid commit_oid, tree_oid;
	git_tree *tree = NULL;
	git_index *index;
	git_object *parent = NULL;
	git_reference *ref = NULL;
	git_signature *author_signature = NULL, *committer_signature = NULL;
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

	ret = git_revparse_ext(&parent, &ref, repo, "HEAD");
	if (ret == GIT_ENOTFOUND) {
		printf("HEAD not found. Creating first commit\n");
		ret = 0;
	} else if (ret != 0) {
		const git_error *err = git_error_last();
		if (err)
			printf("ERROR %d: %s\n", err->klass, err->message);
		else
			printf("ERROR %d: no detailed info\n", ret);
	}

	if ((ret = git_repository_index(&index, repo)) < 0) {
		printf("Could not open repository index\n");
		goto done;
	}
	if ((ret = git_index_write_tree(&tree_oid, index)) < 0) {
		printf("Could not write tree\n");
		goto done;
	}
	if ((ret = git_index_write(index)) < 0) {
		printf("Could not write index\n");
		goto done;
	}

	if ((ret = git_tree_lookup(&tree, repo, &tree_oid)) < 0) {
		printf("Error looking up tree\n");
		goto done;
	}

	if ((ret = git_signature_default_from_env(
	             &author_signature, &committer_signature, repo)) < 0) {
		printf("Error creating signature\n");
		goto done;
	}

	{
		const git_commit *parents[] = { (git_commit *)parent };

		ret = git_commit_create(
		        &commit_oid, repo, "HEAD", author_signature,
		        committer_signature, NULL, message, tree,
		        parent ? 1 : 0, parent ? parents : NULL);
	}

	if (ret < 0) {
		printf("Error creating commit\n");
		goto done;
	}

done:
	git_repository_free(repo);
	git_index_free(index);
	git_signature_free(author_signature);
	git_signature_free(committer_signature);
	git_tree_free(tree);
	git_object_free(parent);
	git_reference_free(ref);
	return ret;
}
