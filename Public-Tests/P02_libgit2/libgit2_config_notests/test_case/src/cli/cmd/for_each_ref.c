#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "for-each-ref"

static const cli_opt_spec opts[] = { CLI_COMMON_OPT,

	                             { 0 } };

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Output information on each ref.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

static int show_ref(git_reference *ref, void *data)
{
	git_repository *repo = data;
	git_reference *resolved = NULL;
	char hex[GIT_OID_SHA1_HEXSIZE + 1];
	const git_oid *oid;
	git_object *obj = NULL;
	int ret = 0;

	if (git_reference_type(ref) == GIT_REFERENCE_SYMBOLIC)
		if ((ret = git_reference_resolve(&resolved, ref)) < 0) {
			fprintf(stderr,
			        "%s: Unable to resolve symbolic reference: %s\n",
			        PROGRAM_NAME, git_reference_name(ref));
			ret = CLI_EXIT_ERROR;
			goto done;
		}

	oid = git_reference_target(resolved ? resolved : ref);
	git_oid_fmt(hex, oid);
	hex[GIT_OID_SHA1_HEXSIZE] = 0;

	if ((ret = git_object_lookup(&obj, repo, oid, GIT_OBJECT_ANY)) < 0) {
		fprintf(stderr, "%s: Unable to lookup object: %s\n",
		        PROGRAM_NAME, hex);
		ret = CLI_EXIT_ERROR;
		goto done;
	}

	printf("%s %-6s\t%s\n", hex,
	       git_object_type2string(git_object_type(obj)),
	       git_reference_name(ref));

done:
	git_object_free(obj);
	git_reference_free(ref);
	if (resolved)
		git_reference_free(resolved);
	return 0;
}

int for_each_ref(int argc, char **argv)
{
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
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

	if ((ret = git_reference_foreach(repo, show_ref, repo)) < 0)
		cli_error_git();

	git_repository_free(repo);
	return ret;
}
