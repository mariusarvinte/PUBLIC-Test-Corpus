#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "add"

static int dry_run, verbose, update;
static char **files;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH,    "dry-run",     'n', &dry_run,     1,
         CLI_OPT_USAGE_DEFAULT,    NULL,         "don’t actually add the file(s), just show if they exist and/or will be ignored"},
	{ CLI_OPT_TYPE_SWITCH,    "verbose",     'v', &verbose,     1,
         CLI_OPT_USAGE_DEFAULT,    NULL,         "be verbose"},
	{ CLI_OPT_TYPE_SWITCH,    "update",      'u', &update,      1,
         CLI_OPT_USAGE_DEFAULT,    NULL,         "update tracked files"},
	{ CLI_OPT_TYPE_ARGS,      "file-spec",    0, &files,    0,
	  CLI_OPT_USAGE_REQUIRED, "file-spec",    "the file (or files) to add to index" },

	{ 0 }
};


static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Add file contents to the index.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

enum index_mode {
	INDEX_NONE,
	INDEX_ADD
};

struct index_options {
	int dry_run;
	int verbose;
	git_repository *repo;
	enum index_mode mode;
	int add_update;
};

/*
 * This callback is called for each file under consideration by
 * git_index_(update|add)_all above.
 * It makes uses of the callback's ability to abort the action.
 */
int print_matched_cb(const char *path, const char *matched_pathspec, void *payload)
{
	struct index_options *opts = (struct index_options *)(payload);
	int ret;
	unsigned status;
	(void)matched_pathspec;

	/* Get the file status */
	if (git_status_file(&status, opts->repo, path) < 0)
		return -1;

	if ((status & GIT_STATUS_WT_MODIFIED) || (status & GIT_STATUS_WT_NEW)) {
		printf("add '%s'\n", path);
		ret = 0;
	} else {
		ret = 1;
	}

	if (opts->dry_run)
		ret = 1;

	return ret;
}

void strarray_from_array(git_strarray *out_array, char **array)
{
	size_t i = 0;

	if (!array) {
		out_array->strings = NULL;
		out_array->count = 0;
		return;
	}

	while (array[i]) {
		i++;
	}

	out_array->strings = array;
	out_array->count = i;
}

int add(int argc, char **argv) {
	git_repository *repo = NULL;
	git_index_matched_path_cb matched_cb = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1};
	git_index *index;
	struct index_options idx_opts = {0};
	git_strarray pathspec = {0};
	cli_opt invalid_opt;
	int ret = 0;

	if (cli_opt_parse(&invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);
	strarray_from_array(&pathspec, files);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	if (cli_repository_open(&repo, &open_opts) < 0)
		return cli_error_git();

	if ((ret = git_repository_index(&index, repo)) != 0)
		goto done;

	if (verbose || dry_run) {
		matched_cb = &print_matched_cb;
	}

	idx_opts.dry_run = dry_run;
	idx_opts.verbose = verbose;
	idx_opts.repo = repo;
	idx_opts.mode = INDEX_ADD;
	idx_opts.add_update = update;

	if (update) {
		git_index_update_all(index, &pathspec, matched_cb, &idx_opts);
	} else {
		git_index_add_all(index, &pathspec, 0, matched_cb, &idx_opts);
	}

	git_index_write(index);
	git_index_free(index);

done:
	git_repository_free(repo);
	return ret;
}
