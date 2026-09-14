/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include "diff_stats.h"

#include "buf.h"
#include "common.h"
#include "vector.h"
#include "diff.h"
#include "patch_generate.h"

#define DIFF_RENAME_FILE_SEPARATOR " => "
#define STATS_FULL_MIN_SCALE 7

typedef struct {
	size_t insertions;
	size_t deletions;
} diff_file_stats;

struct git_diff_stats {
	git_diff *diff;
	diff_file_stats *filestats;

	size_t files_changed;
	size_t insertions;
	size_t deletions;

	size_t max_name;
	size_t max_filestat;
	int max_digits;
};

static int digits_for_value(size_t val)
{
	int count = 1;
	size_t placevalue = 10;

	while (val >= placevalue) {
		++count;
		placevalue *= 10;
	}

	return count;
}

static int diff_stats_puts_field(git_str *out, const char *value, size_t width)
{
	size_t len = strlen(value);

	if (git_str_puts(out, value) < 0)
		return -1;

	if (len < width)
		return git_str_putcn(out, ' ', width - len);

	return 0;
}

static int
diff_stats_putui_field(git_str *out, size_t value, size_t width, bool left_align)
{
	git_str num = GIT_STR_INIT;
	int error;

	if ((error = git_str_putui(&num, value)) < 0)
		goto done;

	if (!left_align && num.size < width &&
	    (error = git_str_putcn(out, ' ', width - num.size)) < 0)
		goto done;

	if ((error = git_str_puts(out, num.ptr)) < 0)
		goto done;

	if (left_align && num.size < width)
		error = git_str_putcn(out, ' ', width - num.size);

done:
	git_str_dispose(&num);
	return error;
}

static int diff_file_stats_full_to_buf(
        git_str *out,
        const git_diff_delta *delta,
        const diff_file_stats *filestat,
        const git_diff_stats *stats,
        size_t width)
{
	const char *old_path = NULL, *new_path = NULL, *adddel_path = NULL;
	size_t padding;
	git_object_size_t old_size, new_size;

	old_path = delta->old_file.path;
	new_path = delta->new_file.path;
	old_size = delta->old_file.size;
	new_size = delta->new_file.size;

	if (old_path && new_path && strcmp(old_path, new_path) != 0) {
		size_t common_dirlen;
		int error;

		if ((common_dirlen =
		             git_fs_path_common_dirlen(old_path, new_path)) &&
		    common_dirlen <= INT_MAX) {
			error = 0;

			if ((error = git_str_putc(out, ' ')) < 0 ||
			    (error = git_str_put(
			             out, old_path, common_dirlen)) < 0 ||
			    (error = git_str_putc(out, '{')) < 0 ||
			    (error = git_str_puts(
			             out, old_path + common_dirlen)) < 0 ||
			    (error = git_str_puts(
			             out, DIFF_RENAME_FILE_SEPARATOR)) < 0 ||
			    (error = git_str_puts(
			             out, new_path + common_dirlen)) < 0 ||
			    (error = git_str_putc(out, '}')) < 0)
				goto on_error;

			padding = stats->max_name + common_dirlen -
			          strlen(old_path) - strlen(new_path) - 2 -
			          strlen(DIFF_RENAME_FILE_SEPARATOR);
		} else {
			error = 0;

			if ((error = git_str_putc(out, ' ')) < 0 ||
			    (error = git_str_puts(out, old_path)) < 0 ||
			    (error = git_str_puts(
			             out, DIFF_RENAME_FILE_SEPARATOR)) < 0 ||
			    (error = git_str_puts(out, new_path)) < 0)
				goto on_error;

			padding = stats->max_name - strlen(old_path) -
			          strlen(new_path) -
			          strlen(DIFF_RENAME_FILE_SEPARATOR);
		}
	} else {
		adddel_path = new_path ? new_path : old_path;
		if (git_str_putc(out, ' ') < 0 ||
		    git_str_puts(out, adddel_path) < 0)
			goto on_error;

		padding = stats->max_name - strlen(adddel_path);
	}

	if (git_str_putcn(out, ' ', padding) < 0 ||
	    git_str_puts(out, " | ") < 0)
		goto on_error;

	if (delta->flags & GIT_DIFF_FLAG_BINARY) {
		if (git_str_puts(out, "Bin ") < 0 ||
		    git_str_puti(out, old_size) < 0 ||
		    git_str_puts(out, " -> ") < 0 ||
		    git_str_puti(out, new_size) < 0 ||
		    git_str_puts(out, " bytes") < 0)
			goto on_error;
	} else {
		if (diff_stats_putui_field(
		            out, filestat->insertions + filestat->deletions,
		            (size_t)stats->max_digits, false) < 0)
			goto on_error;

		if (filestat->insertions || filestat->deletions) {
			if (git_str_putc(out, ' ') < 0)
				goto on_error;

			if (!width) {
				if (git_str_putcn(
				            out, '+', filestat->insertions) <
				            0 ||
				    git_str_putcn(
				            out, '-', filestat->deletions) < 0)
					goto on_error;
			} else {
				size_t total = filestat->insertions +
				               filestat->deletions;
				size_t full = (total * width +
				               stats->max_filestat / 2) /
				              stats->max_filestat;
				size_t plus =
				        full * filestat->insertions / total;
				size_t minus = full - plus;

				if (git_str_putcn(out, '+', max(plus, 1)) < 0 ||
				    git_str_putcn(out, '-', max(minus, 1)) < 0)
					goto on_error;
			}
		}
	}

	git_str_putc(out, '\n');

on_error:
	return (git_str_oom(out) ? -1 : 0);
}

static int diff_file_stats_number_to_buf(
        git_str *out,
        const git_diff_delta *delta,
        const diff_file_stats *filestats)
{
	int error;
	const char *path = delta->new_file.path;

	if (delta->flags & GIT_DIFF_FLAG_BINARY) {
		if ((error = diff_stats_puts_field(out, "-", 8)) < 0 ||
		    (error = diff_stats_puts_field(out, "-", 8)) < 0 ||
		    (error = git_str_puts(out, path)) < 0 ||
		    (error = git_str_putc(out, '\n')) < 0)
			return error;
	} else {
		if ((error = diff_stats_putui_field(
		             out, filestats->insertions, 8, true)) < 0 ||
		    (error = diff_stats_putui_field(
		             out, filestats->deletions, 8, true)) < 0 ||
		    (error = git_str_puts(out, path)) < 0 ||
		    (error = git_str_putc(out, '\n')) < 0)
			return error;
	}

	return error;
}

static int
diff_file_stats_summary_to_buf(git_str *out, const git_diff_delta *delta)
{
	if (delta->old_file.mode != delta->new_file.mode) {
		if (delta->old_file.mode == 0) {
			git_str_puts(out, " create mode ");
			git_str_putui_pad(out, delta->new_file.mode, 8, 6, '0');
			git_str_putc(out, ' ');
			git_str_puts(out, delta->new_file.path);
			git_str_putc(out, '\n');
		} else if (delta->new_file.mode == 0) {
			git_str_puts(out, " delete mode ");
			git_str_putui_pad(out, delta->old_file.mode, 8, 6, '0');
			git_str_putc(out, ' ');
			git_str_puts(out, delta->old_file.path);
			git_str_putc(out, '\n');
		} else {
			git_str_puts(out, " mode change ");
			git_str_putui_pad(out, delta->old_file.mode, 8, 6, '0');
			git_str_puts(out, " => ");
			git_str_putui_pad(out, delta->new_file.mode, 8, 6, '0');
			git_str_putc(out, ' ');
			git_str_puts(out, delta->new_file.path);
			git_str_putc(out, '\n');
		}
	}

	return 0;
}

int git_diff_get_stats(git_diff_stats **out, git_diff *diff)
{
	size_t i, deltas;
	size_t total_insertions = 0, total_deletions = 0;
	git_diff_stats *stats = NULL;
	int error = 0;

	GIT_ASSERT_ARG(out);
	GIT_ASSERT_ARG(diff);

	stats = git__calloc(1, sizeof(git_diff_stats));
	GIT_ERROR_CHECK_ALLOC(stats);

	deltas = git_diff_num_deltas(diff);

	stats->filestats = git__calloc(deltas, sizeof(diff_file_stats));
	if (!stats->filestats) {
		git__free(stats);
		return -1;
	}

	stats->diff = diff;
	GIT_REFCOUNT_INC(diff);

	for (i = 0; i < deltas && !error; ++i) {
		git_patch *patch = NULL;
		size_t add = 0, remove = 0, namelen;
		const git_diff_delta *delta;

		if ((error = git_patch_from_diff(&patch, diff, i)) < 0)
			break;

		/* Length calculation for renames mirrors the actual
		 * presentation format generated in diff_file_stats_full_to_buf;
		 * namelen is the full length of what will be printed, taking
		 * into account renames and common prefixes.
		 */
		delta = patch->delta;
		namelen = strlen(delta->new_file.path);
		if (delta->old_file.path &&
		    strcmp(delta->old_file.path, delta->new_file.path) != 0) {
			size_t common_dirlen;
			if ((common_dirlen = git_fs_path_common_dirlen(
			             delta->old_file.path,
			             delta->new_file.path)) &&
			    common_dirlen <= INT_MAX) {
				namelen += strlen(delta->old_file.path) + 2 +
				           strlen(DIFF_RENAME_FILE_SEPARATOR) -
				           common_dirlen;
			} else {
				namelen += strlen(delta->old_file.path) +
				           strlen(DIFF_RENAME_FILE_SEPARATOR);
			}
		}

		/* and, of course, count the line stats */
		error = git_patch_line_stats(NULL, &add, &remove, patch);

		git_patch_free(patch);

		stats->filestats[i].insertions = add;
		stats->filestats[i].deletions = remove;

		total_insertions += add;
		total_deletions += remove;

		if (stats->max_name < namelen)
			stats->max_name = namelen;
		if (stats->max_filestat < add + remove)
			stats->max_filestat = add + remove;
	}

	stats->files_changed = deltas;
	stats->insertions = total_insertions;
	stats->deletions = total_deletions;
	stats->max_digits = digits_for_value(stats->max_filestat + 1);

	if (error < 0) {
		git_diff_stats_free(stats);
		stats = NULL;
	}

	*out = stats;
	return error;
}

size_t git_diff_stats_files_changed(const git_diff_stats *stats)
{
	GIT_ASSERT_ARG(stats);

	return stats->files_changed;
}

size_t git_diff_stats_insertions(const git_diff_stats *stats)
{
	GIT_ASSERT_ARG(stats);

	return stats->insertions;
}

size_t git_diff_stats_deletions(const git_diff_stats *stats)
{
	GIT_ASSERT_ARG(stats);

	return stats->deletions;
}

int git_diff_stats_to_buf(
        git_buf *out,
        const git_diff_stats *stats,
        git_diff_stats_format_t format,
        size_t width)
{
	git_str str = GIT_STR_INIT;
	int error;

	if ((error = git_buf_tostr(&str, out)) == 0 &&
	    (error = git_diff__stats_to_buf(&str, stats, format, width)) == 0)
		error = git_buf_fromstr(out, &str);

	git_str_dispose(&str);
	return error;
}

int git_diff__stats_to_buf(
        git_str *out,
        const git_diff_stats *stats,
        git_diff_stats_format_t format,
        size_t width)
{
	int error = 0;
	size_t i;
	const git_diff_delta *delta;

	GIT_ASSERT_ARG(out);
	GIT_ASSERT_ARG(stats);

	if (format & GIT_DIFF_STATS_NUMBER) {
		for (i = 0; i < stats->files_changed; ++i) {
			if ((delta = git_diff_get_delta(stats->diff, i)) ==
			    NULL)
				continue;

			error = diff_file_stats_number_to_buf(
			        out, delta, &stats->filestats[i]);
			if (error < 0)
				return error;
		}
	}

	if (format & GIT_DIFF_STATS_FULL) {
		if (width > 0) {
			if (width > stats->max_name + stats->max_digits + 5)
				width -=
				        (stats->max_name + stats->max_digits +
				         5);
			if (width < STATS_FULL_MIN_SCALE)
				width = STATS_FULL_MIN_SCALE;
		}
		if (width > stats->max_filestat)
			width = 0;

		for (i = 0; i < stats->files_changed; ++i) {
			if ((delta = git_diff_get_delta(stats->diff, i)) ==
			    NULL)
				continue;

			error = diff_file_stats_full_to_buf(
			        out, delta, &stats->filestats[i], stats, width);
			if (error < 0)
				return error;
		}
	}

	if (format & GIT_DIFF_STATS_FULL || format & GIT_DIFF_STATS_SHORT) {
		git_str_putc(out, ' ');
		git_str_putui(out, stats->files_changed);
		git_str_puts(out, " file");
		git_str_puts(
		        out,
		        stats->files_changed != 1 ? "s changed" : " changed");

		if (stats->insertions || stats->deletions == 0)
			if (git_str_puts(out, ", ") < 0 ||
			    git_str_putui(out, stats->insertions) < 0 ||
			    git_str_puts(
			            out, stats->insertions != 1 ?
			                         " insertions(+)" :
			                         " insertion(+)") < 0)
				return -1;

		if (stats->deletions || stats->insertions == 0)
			if (git_str_puts(out, ", ") < 0 ||
			    git_str_putui(out, stats->deletions) < 0 ||
			    git_str_puts(
			            out, stats->deletions != 1 ?
			                         " deletions(-)" :
			                         " deletion(-)") < 0)
				return -1;

		git_str_putc(out, '\n');

		if (git_str_oom(out))
			return -1;
	}

	if (format & GIT_DIFF_STATS_INCLUDE_SUMMARY) {
		for (i = 0; i < stats->files_changed; ++i) {
			if ((delta = git_diff_get_delta(stats->diff, i)) ==
			    NULL)
				continue;

			error = diff_file_stats_summary_to_buf(out, delta);
			if (error < 0)
				return error;
		}
	}

	return error;
}

void git_diff_stats_free(git_diff_stats *stats)
{
	if (stats == NULL)
		return;

	git_diff_free(stats->diff); /* bumped refcount in constructor */
	git__free(stats->filestats);
	git__free(stats);
}
