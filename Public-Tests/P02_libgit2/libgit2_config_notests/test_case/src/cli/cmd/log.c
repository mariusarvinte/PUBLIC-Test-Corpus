/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <stdio.h>
#include <string.h>
#include <git2.h>
#include "common.h"
#include "cmd.h"

#define COMMAND_NAME "log"

/* Options */
static int show_diff = 0;
static int show_oneline = 0;
static int show_log_size = 0;
static char *skip_str = NULL;
static char *limit_str = NULL;
static int min_parents = 0;
static int max_parents = -1;
static int sorting = GIT_SORT_TIME;
static char *author = NULL;
static char *committer = NULL;
static char *grep_pattern = NULL;
static char **revisions = NULL;
static char **pathspecs = NULL;

static const cli_opt_spec opts[] = {
	CLI_COMMON_OPT,

	{ CLI_OPT_TYPE_SWITCH, "patch", 'p', &show_diff, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "generate patch" },
	{ CLI_OPT_TYPE_SWITCH, "oneline", 0, &show_oneline, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show commits on one line" },
	{ CLI_OPT_TYPE_SWITCH, "log-size", 0, &show_log_size, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show log message size" },
	{ CLI_OPT_TYPE_VALUE, "skip", 0, &skip_str, 0, CLI_OPT_USAGE_DEFAULT,
	  "n", "skip n commits" },
	{ CLI_OPT_TYPE_VALUE, "max-count", 0, &limit_str, 0,
	  CLI_OPT_USAGE_DEFAULT, "n", "limit to n commits" },
	{ CLI_OPT_TYPE_VALUE, "author", 0, &author, 0, CLI_OPT_USAGE_DEFAULT,
	  "pattern", "filter by author" },
	{ CLI_OPT_TYPE_VALUE, "committer", 0, &committer, 0,
	  CLI_OPT_USAGE_DEFAULT, "pattern", "filter by committer" },
	{ CLI_OPT_TYPE_VALUE, "grep", 0, &grep_pattern, 0,
	  CLI_OPT_USAGE_DEFAULT, "pattern", "filter by commit message" },
	{ CLI_OPT_TYPE_SWITCH, "merges", 0, &min_parents, 2,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show only merge commits" },
	{ CLI_OPT_TYPE_SWITCH, "no-merges", 0, &max_parents, 1,
	  CLI_OPT_USAGE_DEFAULT, NULL, "show only non-merge commits" },
	{ CLI_OPT_TYPE_SWITCH, "date-order", 0, &sorting, GIT_SORT_TIME,
	  CLI_OPT_USAGE_DEFAULT, NULL, "sort by date" },
	{ CLI_OPT_TYPE_SWITCH, "topo-order", 0, &sorting, GIT_SORT_TOPOLOGICAL,
	  CLI_OPT_USAGE_DEFAULT, NULL, "sort topologically" },
	{ CLI_OPT_TYPE_SWITCH, "reverse", 0, &sorting, GIT_SORT_REVERSE,
	  CLI_OPT_USAGE_DEFAULT, NULL, "reverse order" },
	{ CLI_OPT_TYPE_ARGS, "revisions", 0, &revisions, 0,
	  CLI_OPT_USAGE_DEFAULT, "revisions", "starting revisions" },
	{ CLI_OPT_TYPE_LITERAL },
	{ CLI_OPT_TYPE_ARGS, "paths", 0, &pathspecs, 0, CLI_OPT_USAGE_DEFAULT,
	  "paths", "limit to specific paths" },

	{ 0 }
};

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Show commit logs.\n");
	printf("\n");

	printf("Options:\n");
	cli_opt_help_fprint(stdout, opts);
}

/* Helper to print time */
static void print_time(const git_time *intime, const char *prefix)
{
	char sign, out[32];
	struct tm *intm;
	int offset, hours, minutes;
	time_t t;

	offset = intime->offset;
	if (offset < 0) {
		sign = '-';
		offset = -offset;
	} else {
		sign = '+';
	}

	hours = offset / 60;
	minutes = offset % 60;

	t = (time_t)intime->time + (intime->offset * 60);
	intm = gmtime(&t);
	strftime(out, sizeof(out), "%a %b %e %T %Y", intm);

	printf("%s%s %c%02d%02d\n", prefix, out, sign, hours, minutes);
}

/* Helper to print commit */
static void print_commit(git_commit *commit)
{
	char buf[GIT_OID_SHA1_HEXSIZE + 1];
	int i, count;
	const git_signature *sig;
	const char *scan, *eol;

	git_oid_tostr(buf, sizeof(buf), git_commit_id(commit));

	if (show_oneline) {
		printf("%s ", buf);
	} else {
		printf("commit %s\n", buf);

		if (show_log_size) {
			printf("log size %d\n",
			       (int)strlen(git_commit_message(commit)));
		}

		if ((count = (int)git_commit_parentcount(commit)) > 1) {
			printf("Merge:");
			for (i = 0; i < count; ++i) {
				git_oid_tostr(
				        buf, 8,
				        git_commit_parent_id(commit, i));
				printf(" %s", buf);
			}
			printf("\n");
		}

		if ((sig = git_commit_author(commit)) != NULL) {
			printf("Author: %s <%s>\n", sig->name, sig->email);
			print_time(&sig->when, "Date:   ");
		}
		printf("\n");
	}

	for (scan = git_commit_message(commit); scan && *scan;) {
		for (eol = scan; *eol && *eol != '\n'; ++eol) /* find eol */
			;

		if (show_oneline)
			printf("%.*s\n", (int)(eol - scan), scan);
		else
			printf("    %.*s\n", (int)(eol - scan), scan);
		scan = *eol ? eol + 1 : NULL;
		if (show_oneline)
			break;
	}
	if (!show_oneline)
		printf("\n");
}

/* Diff output callback */
static int diff_output(
        const git_diff_delta *delta,
        const git_diff_hunk *hunk,
        const git_diff_line *line,
        void *payload)
{
	(void)delta;
	(void)hunk;
	(void)payload;

	if (line->origin == GIT_DIFF_LINE_CONTEXT ||
	    line->origin == GIT_DIFF_LINE_ADDITION ||
	    line->origin == GIT_DIFF_LINE_DELETION)
		fputc(line->origin, stdout);

	fwrite(line->content, 1, line->content_len, stdout);
	return 0;
}

/* Check if signature matches pattern */
static int signature_matches(const git_signature *sig, const char *filter)
{
	if (filter == NULL)
		return 1;

	if (sig != NULL && (strstr(sig->name, filter) != NULL ||
	                    strstr(sig->email, filter) != NULL))
		return 1;

	return 0;
}

/* Check if commit message matches pattern */
static int log_message_matches(const git_commit *commit, const char *filter)
{
	const char *message = NULL;

	if (filter == NULL)
		return 1;

	if ((message = git_commit_message(commit)) != NULL &&
	    strstr(message, filter) != NULL)
		return 1;

	return 0;
}

/* Check if commit matches pathspec */
static int
match_with_parent(git_commit *commit, int i, git_diff_options *diffopts)
{
	git_commit *parent;
	git_tree *a, *b;
	git_diff *diff;
	int ndeltas;
	int ret;

	if ((ret = git_commit_parent(&parent, commit, (size_t)i)) < 0) {
		cli_error_git();
		return 0;
	}

	if ((ret = git_commit_tree(&a, parent)) < 0) {
		cli_error_git();
		git_commit_free(parent);
		return 0;
	}

	if ((ret = git_commit_tree(&b, commit)) < 0) {
		cli_error_git();
		git_tree_free(a);
		git_commit_free(parent);
		return 0;
	}

	ret = git_diff_tree_to_tree(
	        &diff, git_commit_owner(commit), a, b, diffopts);
	if (ret < 0) {
		cli_error_git();
		git_tree_free(b);
		git_tree_free(a);
		git_commit_free(parent);
		return 0;
	}

	ndeltas = (int)git_diff_num_deltas(diff);

	git_diff_free(diff);
	git_tree_free(b);
	git_tree_free(a);
	git_commit_free(parent);

	return ndeltas > 0;
}

/* Add revision to walker */
static int
add_revision(git_revwalk *walker, git_repository *repo, const char *revstr)
{
	git_revspec revs;
	int hide = 0;
	int ret;

	if (!revstr) {
		/* Default to HEAD */
		if ((ret = git_revwalk_push_head(walker)) < 0) {
			cli_error_git();
			return ret;
		}
		return 0;
	}

	if (*revstr == '^') {
		hide = 1;
		revstr++;
	}

	ret = git_revparse(&revs, repo, revstr);
	if (ret < 0) {
		cli_error_git();
		return ret;
	}

	if ((revs.flags & GIT_REVSPEC_SINGLE) != 0) {
		if (hide)
			ret = git_revwalk_hide(
			        walker, git_object_id(revs.from));
		else
			ret = git_revwalk_push(
			        walker, git_object_id(revs.from));
		git_object_free(revs.from);
	} else {
		ret = git_revwalk_push(walker, git_object_id(revs.to));
		if (ret == 0) {
			if ((revs.flags & GIT_REVSPEC_MERGE_BASE) != 0) {
				git_oid base;
				ret = git_merge_base(
				        &base, repo, git_object_id(revs.from),
				        git_object_id(revs.to));
				if (ret == 0) {
					ret = git_revwalk_push(walker, &base);
				}
			}
			if (ret == 0) {
				ret = git_revwalk_hide(
				        walker, git_object_id(revs.from));
			}
		}
		git_object_free(revs.to);
		git_object_free(revs.from);
	}

	if (ret < 0)
		cli_error_git();

	return ret;
}

int log_cmd(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	cli_opt invalid_opt;
	git_revwalk *walker = NULL;
	git_oid oid;
	git_commit *commit = NULL;
	git_pathspec *ps = NULL;
	git_diff_options diffopts = GIT_DIFF_OPTIONS_INIT;
	int count = 0, printed = 0;
	int parents, i;
	int ret = 0;
	int skip = 0;
	int limit = -1;
	size_t rev_count = 0;

	if (cli_opt_parse(
	            &invalid_opt, opts, argv + 1, argc - 1, CLI_OPT_PARSE_GNU))
		return cli_opt_usage_error(COMMAND_NAME, opts, &invalid_opt);

	if (cli_opt__show_help) {
		print_help();
		return 0;
	}

	/* Parse skip and limit */
	if (skip_str)
		skip = atoi(skip_str);
	if (limit_str)
		limit = atoi(limit_str);

	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	/* Create revwalker */
	if ((ret = git_revwalk_new(&walker, repo)) < 0) {
		cli_error_git();
		goto done;
	}

	git_revwalk_sorting(walker, sorting);

	/* Add revisions */
	if (revisions) {
		while (revisions[rev_count]) {
			if ((ret = add_revision(
			             walker, repo, revisions[rev_count])) < 0)
				goto done;
			rev_count++;
		}
	}

	if (rev_count == 0) {
		/* Default to HEAD */
		if ((ret = add_revision(walker, repo, NULL)) < 0)
			goto done;
	}

	/* Set up pathspec */
	if (pathspecs) {
		i = 0;
		while (pathspecs[i])
			i++;
		diffopts.pathspec.strings = pathspecs;
		diffopts.pathspec.count = i;

		if ((ret = git_pathspec_new(&ps, &diffopts.pathspec)) < 0) {
			cli_error_git();
			goto done;
		}
	}

	/* Walk commits */
	while (!git_revwalk_next(&oid, walker)) {
		if ((ret = git_commit_lookup(&commit, repo, &oid)) < 0) {
			cli_error_git();
			break;
		}

		parents = (int)git_commit_parentcount(commit);

		/* Parent count filtering */
		if (parents < min_parents)
			goto next_commit;
		if (max_parents > 0 && parents > max_parents)
			goto next_commit;

		/* Pathspec filtering */
		if (diffopts.pathspec.count > 0) {
			int unmatched = parents;

			if (parents == 0) {
				git_tree *tree;
				if (git_commit_tree(&tree, commit) == 0) {
					if (git_pathspec_match_tree(
					            NULL, tree,
					            GIT_PATHSPEC_NO_MATCH_ERROR,
					            ps) != 0)
						unmatched = 1;
					git_tree_free(tree);
				}
			} else if (parents == 1) {
				unmatched = match_with_parent(
				                    commit, 0, &diffopts) ?
				                    0 :
				                    1;
			} else {
				for (i = 0; i < parents; ++i) {
					if (match_with_parent(
					            commit, i, &diffopts))
						unmatched--;
				}
			}

			if (unmatched > 0)
				goto next_commit;
		}

		/* Author filtering */
		if (!signature_matches(git_commit_author(commit), author))
			goto next_commit;

		/* Committer filtering */
		if (!signature_matches(git_commit_committer(commit), committer))
			goto next_commit;

		/* Message filtering */
		if (!log_message_matches(commit, grep_pattern))
			goto next_commit;

		/* Skip handling */
		if (count++ < skip)
			goto next_commit;

		/* Limit handling */
		if (limit != -1 && printed++ >= limit) {
			git_commit_free(commit);
			break;
		}

		/* Print commit */
		print_commit(commit);

		/* Print diff if requested */
		if (show_diff) {
			git_tree *a = NULL, *b = NULL;
			git_diff *diff = NULL;

			if (parents > 1)
				goto next_commit;

			if ((ret = git_commit_tree(&b, commit)) < 0) {
				cli_error_git();
				goto next_commit;
			}

			if (parents == 1) {
				git_commit *parent;
				if ((ret = git_commit_parent(
				             &parent, commit, 0)) < 0) {
					cli_error_git();
					git_tree_free(b);
					goto next_commit;
				}
				if ((ret = git_commit_tree(&a, parent)) < 0) {
					cli_error_git();
					git_commit_free(parent);
					git_tree_free(b);
					goto next_commit;
				}
				git_commit_free(parent);
			}

			if ((ret = git_diff_tree_to_tree(
			             &diff, repo, a, b, &diffopts)) < 0) {
				cli_error_git();
				git_tree_free(a);
				git_tree_free(b);
				goto next_commit;
			}

			git_diff_print(
			        diff, GIT_DIFF_FORMAT_PATCH, diff_output, NULL);

			git_diff_free(diff);
			git_tree_free(a);
			git_tree_free(b);
		}

	next_commit:
		git_commit_free(commit);
		commit = NULL;
	}

done:
	git_commit_free(commit);
	git_pathspec_free(ps);
	git_revwalk_free(walker);
	git_repository_free(repo);
	return ret;
}
