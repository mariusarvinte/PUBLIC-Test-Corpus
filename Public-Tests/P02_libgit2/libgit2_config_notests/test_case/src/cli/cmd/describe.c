#include <git2.h>
#include "common.h"
#include "cmd.h"
#include "error.h"
#include "progress.h"

#define COMMAND_NAME "describe"

struct describe_options {
	const char **commits;
	size_t commit_count;
	git_describe_options describe_options;
	git_describe_format_options format_options;
};

static int all, tags, exact_match, opt_long, always, first_parent;
static int abbrev, candidates;
static char *match, *dirty;
static char **commit_ish;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "all", 0, &all, 1, CLI_OPT_USAGE_DEFAULT, NULL,
	  "Instead of using only the annotated tags, use any ref found in refs/ namespace" },
	{ CLI_OPT_TYPE_SWITCH, "tags", 0, &tags, 1, CLI_OPT_USAGE_DEFAULT, NULL,
	  "Instead of using only the annotated tags, use any tag found in refs/tags namespace" },
	{ CLI_OPT_TYPE_SWITCH, "exact-match", 0, &exact_match, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "Only output exact matches (a tag directly references the supplied commit)" },
	{ CLI_OPT_TYPE_SWITCH, "long", 0, &opt_long, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL,
	  "Always output the long format (the tag, the number of commits and the abbreviated commit name) even when it matches a tag" },
	{ CLI_OPT_TYPE_SWITCH, "always", 0, &always, 1, CLI_OPT_USAGE_DEFAULT,
	  NULL, "Show uniquely abbreviated commit object as fallback" },
	{ CLI_OPT_TYPE_SWITCH, "first-parent", 0, &first_parent, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL,
	  "Follow only the first parent commit upon seeing a merge commit" },
	{ CLI_OPT_TYPE_VALUE, "abbrev", 0, &abbrev, 1, CLI_OPT_USAGE_DEFAULT,
	  "n",
	  "Instead of using the default number of hexadecimal digits (which will vary according to the number of objects in the repository with a default of 7) of the abbreviated object name, use <n> digits, or as many digits as needed to form a unique object name" },
	{ CLI_OPT_TYPE_VALUE, "candidates", 0, &candidates, 1,
	  CLI_OPT_USAGE_DEFAULT, "n",
	  "Instead of considering only the 10 most recent tags as candidates to describe the input commit-ish consider up to <n> candidates" },
	{ CLI_OPT_TYPE_VALUE, "match", 0, &match, 1, CLI_OPT_USAGE_DEFAULT,
	  "pattern",
	  "Only consider tags matching the given glob(7) pattern, excluding the \"refs/tags/\" prefix" },

	{ CLI_OPT_TYPE_VALUE, "dirty", 0, &dirty, 1,
	  CLI_OPT_USAGE_VALUE_OPTIONAL, "match",
	  "Describe the state of the working tree" },
	{ CLI_OPT_TYPE_ARGS, "commit-ish", 0, &commit_ish, 1,
	  CLI_OPT_USAGE_CHOICE, "commit-ish",
	  "Commit-ish object names to describe. Defaults to HEAD if omitted" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Give an object a human readable name based on an available ref.\n");
	printf("\n");

	printf("Options:\n");

	cli_opt_help_fprint(stdout, opts);
}

static void opts_add_commit(struct describe_options *opts, const char *commit)
{
	size_t sz;

	assert(opts != NULL);

	sz = ++opts->commit_count * sizeof(opts->commits[0]);
	opts->commits = realloc((void *)opts->commits, sz);
	opts->commits[opts->commit_count - 1] = commit;
}

void opts_to_desc_opts(struct describe_options *desc_opts)
{
	if (all)
		desc_opts->describe_options.describe_strategy =
		        GIT_DESCRIBE_ALL;

	if (tags)
		desc_opts->describe_options.describe_strategy =
		        GIT_DESCRIBE_TAGS;

	if (exact_match)
		desc_opts->describe_options.max_candidates_tags = 0;

	if (opt_long)
		desc_opts->format_options.always_use_long_format = 1;

	if (always)
		desc_opts->describe_options.show_commit_oid_as_fallback = 1;

	if (first_parent)
		desc_opts->describe_options.only_follow_first_parent = 1;

	if (abbrev)
		desc_opts->format_options.abbreviated_size = abbrev;

	if (candidates)
		desc_opts->describe_options.max_candidates_tags = candidates;

	if (match)
		desc_opts->describe_options.pattern = match;

	if (commit_ish && dirty) {
		exit(cli_error_usage_message(
		        "--dirty is incompatible with commit-ish"));
	}

	if (commit_ish) {
		char **commit;
		for (commit = commit_ish; *commit != NULL; commit++) {
			opts_add_commit(desc_opts, *commit);
		}
	} else if (dirty) {
		opts_add_commit(desc_opts, dirty);
	} else {
		// If they're both not specified default to HEAD
		opts_add_commit(desc_opts, "HEAD");
	}
}

/** Initialize describe_options struct */
static void describe_options_init(struct describe_options *opts)
{
	memset(opts, 0, sizeof(*opts));

	opts->commits = NULL;
	opts->commit_count = 0;

#ifndef GIT_DEPRECATE_HARD
	git_describe_init_options(
	        &opts->describe_options, GIT_DESCRIBE_OPTIONS_VERSION);
	git_describe_init_format_options(
	        &opts->format_options, GIT_DESCRIBE_FORMAT_OPTIONS_VERSION);
#else
	git_describe_options_init(
	        &opts->describe_options, GIT_DESCRIBE_OPTIONS_VERSION);
	git_describe_format_options_init(
	        &opts->format_options, GIT_DESCRIBE_FORMAT_OPTIONS_VERSION);
#endif
}

static void do_describe_single(
        git_repository *repo,
        struct describe_options *opts,
        const char *rev)
{
	git_object *commit;
	git_describe_result *describe_result;
	git_buf buf = { 0 };

	if (rev) {
		if (git_revparse_single(&commit, repo, rev) < 0) {
			cli_error_git();
			return;
		}

		if (git_describe_commit(
		            &describe_result, commit, &opts->describe_options) <
		    0) {
			cli_error_git();
			return;
		}

	} else if (
	        git_describe_workdir(
	                &describe_result, repo, &opts->describe_options) < 0) {
		cli_error_git();
		return;
	}

	if (git_describe_format(&buf, describe_result, &opts->format_options) <
	    0) {
		cli_error_git();
		return;
	}

	printf("%s\n", buf.ptr);
}

static void do_describe(git_repository *repo, struct describe_options *opts)
{
	if (opts->commit_count == 0)
		do_describe_single(repo, opts, NULL);
	else {
		size_t i;
		for (i = 0; i < opts->commit_count; i++)
			do_describe_single(repo, opts, opts->commits[i]);
	}
}

int describe(int argc, char **argv)
{
	cli_opt invalid_opt;
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { argv + 1, argc - 1 };
	struct describe_options desc_opts;

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

	describe_options_init(&desc_opts);
	opts_to_desc_opts(&desc_opts);

	do_describe(repo, &desc_opts);

	git_repository_free(repo);
	return 0;
}
