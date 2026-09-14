/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#include <stdio.h>
#include <stdint.h>

#include "progress.h"
#include "error.h"

/*
 * Show updates to the percentage and number of objects received
 * separately from the throughput to give an accurate progress while
 * avoiding too much noise on the screen. (In milliseconds.)
 */
#define PROGRESS_UPDATE_TIME 60
#define THROUGHPUT_UPDATE_TIME 500

#define is_nl(c) ((c) == '\r' || (c) == '\n')

#define return_os_error(msg)                      \
	do {                                      \
		git_error_set(GIT_ERROR_OS, msg); \
		return -1;                        \
	} while (0)

GIT_INLINE(size_t) no_nl_len(const char *str, size_t len)
{
	size_t i = 0;

	while (i < len && !is_nl(str[i]))
		i++;

	return i;
}

GIT_INLINE(size_t) nl_len(bool *has_nl, const char *str, size_t len)
{
	size_t i = no_nl_len(str, len);

	*has_nl = false;

	while (i < len && is_nl(str[i])) {
		*has_nl = true;
		i++;
	}

	return i;
}

static int progress_write(cli_progress *progress, bool force, git_str *line)
{
	bool has_nl;
	size_t no_nl = no_nl_len(line->ptr, line->size);
	size_t nl = nl_len(&has_nl, line->ptr + no_nl, line->size - no_nl);
	uint64_t now = git_time_monotonic();
	size_t i;

	/* Avoid spamming the console with progress updates */
	if (!force && line->ptr[line->size - 1] != '\n' &&
	    progress->last_update) {
		if (now - progress->last_update < PROGRESS_UPDATE_TIME) {
			git_str_clear(&progress->deferred);
			git_str_put(&progress->deferred, line->ptr, line->size);
			return git_str_oom(&progress->deferred) ? -1 : 0;
		}
	}

	/*
	 * If there's something on this line already (eg, a progress line
	 * with only a trailing `\r` that we'll print over) then we need
	 * to really print over it in case we're writing a shorter line.
	 */
	if (printf("%.*s", (int)no_nl, line->ptr) < 0)
		return_os_error("could not print status");

	if (progress->onscreen.size) {
		for (i = no_nl; i < progress->onscreen.size; i++) {
			if (printf(" ") < 0)
				return_os_error("could not print status");
		}
	}

	if (printf("%.*s", (int)nl, line->ptr + no_nl) < 0 ||
	    fflush(stdout) != 0)
		return_os_error("could not print status");

	git_str_clear(&progress->onscreen);

	if (line->ptr[line->size - 1] == '\n') {
		progress->last_update = 0;
	} else {
		git_str_put(&progress->onscreen, line->ptr, line->size);
		progress->last_update = now;
	}

	git_str_clear(&progress->deferred);
	return git_str_oom(&progress->onscreen) ? -1 : 0;
}

static int progress_complete(cli_progress *progress)
{
	if (progress->deferred.size)
		progress_write(progress, true, &progress->deferred);

	if (progress->onscreen.size)
		if (printf("\n") < 0)
			return_os_error("could not print status");

	git_str_clear(&progress->deferred);
	git_str_clear(&progress->onscreen);
	progress->last_update = 0;
	progress->action_start = 0;
	progress->action_finish = 0;

	return 0;
}

GIT_INLINE(int) percent(size_t completed, size_t total)
{
	if (total == 0)
		return (completed == 0) ? 100 : 0;

	return (int)(((double)completed / (double)total) * 100);
}

int cli_progress_fetch_sideband(const char *str, int len, void *payload)
{
	cli_progress *progress = (cli_progress *)payload;
	size_t remain;

	if (len <= 0)
		return 0;

	/* Accumulate the sideband data, then print it line-at-a-time. */
	if (git_str_put(&progress->sideband, str, len) < 0)
		return -1;

	str = progress->sideband.ptr;
	remain = progress->sideband.size;

	while (remain) {
		bool has_nl;
		size_t line_len = nl_len(&has_nl, str, remain);

		if (!has_nl)
			break;

		if (line_len < INT_MAX) {
			git_str buf = GIT_BUF_INIT;
			int error;

			error = git_str_puts(&buf, "remote: ");
			if (!error)
				error = git_str_put(&buf, str, line_len);
			if (error == 0)
				error = progress_write(progress, true, &buf);
			git_str_dispose(&buf);

			if (error < 0)
				return error;
		}

		str += line_len;
		remain -= line_len;
	}

	git_str_consume_bytes(
	        &progress->sideband, (progress->sideband.size - remain));

	return 0;
}

static int
fetch_receiving(cli_progress *progress, const git_indexer_progress *stats)
{
	git_str buf = GIT_BUF_INIT;
	int error;
	char *recv_units[] = { "B", "KiB", "MiB", "GiB", "TiB", NULL };
	char *rate_units[] = { "B/s", "KiB/s", "MiB/s", "GiB/s", "TiB/s", NULL };
	uint64_t now, elapsed;
	char recv_len_buf[32], rate_buf[32];

	double recv_len, rate;
	size_t recv_unit_idx = 0, rate_unit_idx = 0;
	bool done = (stats->received_objects == stats->total_objects);

	if (!progress->action_start)
		progress->action_start = git_time_monotonic();

	if (done && progress->action_finish)
		now = progress->action_finish;
	else if (done)
		progress->action_finish = now = git_time_monotonic();
	else
		now = git_time_monotonic();

	if (progress->throughput_update &&
	    now - progress->throughput_update < THROUGHPUT_UPDATE_TIME) {
		elapsed = progress->throughput_update - progress->action_start;
		recv_len = progress->throughput_bytes;
	} else {
		elapsed = now - progress->action_start;
		recv_len = (double)stats->received_bytes;

		progress->throughput_update = now;
		progress->throughput_bytes = recv_len;
	}

	rate = elapsed ? recv_len / elapsed : 0;

	while (recv_len > 1024 && recv_units[recv_unit_idx + 1]) {
		recv_len /= 1024;
		recv_unit_idx++;
	}

	while (rate > 1024 && rate_units[rate_unit_idx + 1]) {
		rate /= 1024;
		rate_unit_idx++;
	}

	if (snprintf(recv_len_buf, sizeof(recv_len_buf), "%.2f", recv_len) <
	            0 ||
	    snprintf(rate_buf, sizeof(rate_buf), "%.2f", rate) < 0)
		return -1;

	error = git_str_puts(&buf, "Receiving objects: ");
	if (!error)
		error = git_str_putui_pad(
		        &buf,
		        (uintmax_t)percent(
		                stats->received_objects, stats->total_objects),
		        10, 3, ' ');
	if (!error)
		error = git_str_puts(&buf, "% (");
	if (!error)
		error = git_str_putui(&buf, stats->received_objects);
	if (!error)
		error = git_str_putc(&buf, '/');
	if (!error)
		error = git_str_putui(&buf, stats->total_objects);
	if (!error)
		error = git_str_puts_array(
		        &buf, 6,
		        (const char *[]){ "), ", recv_len_buf, " ",
		                          recv_units[recv_unit_idx], " | ",
		                          rate_buf });
	if (!error)
		error = git_str_puts_array(
		        &buf, 3,
		        (const char *[]){ " ", rate_units[rate_unit_idx],
		                          done ? ", done." : "" });
	if (!error)
		error = git_str_putc(&buf, '\r');

	if (error == 0)
		error = progress_write(progress, false, &buf);

	git_str_dispose(&buf);
	return error;
}

static int
indexer_indexing(cli_progress *progress, const git_indexer_progress *stats)
{
	git_str buf = GIT_BUF_INIT;
	int error;
	bool done = (stats->received_objects == stats->total_objects);

	error = git_str_puts(&buf, "Indexing objects: ");
	if (!error)
		error = git_str_putui_pad(
		        &buf,
		        (uintmax_t)percent(
		                stats->received_objects, stats->total_objects),
		        10, 3, ' ');
	if (!error)
		error = git_str_puts(&buf, "% (");
	if (!error)
		error = git_str_putui(&buf, stats->received_objects);
	if (!error)
		error = git_str_putc(&buf, '/');
	if (!error)
		error = git_str_putui(&buf, stats->total_objects);
	if (!error)
		error = git_str_puts(&buf, ")");
	if (!error && done)
		error = git_str_puts(&buf, ", done.");
	if (!error)
		error = git_str_putc(&buf, '\r');

	if (error == 0)
		error = progress_write(progress, false, &buf);

	git_str_dispose(&buf);
	return error;
}

static int
indexer_resolving(cli_progress *progress, const git_indexer_progress *stats)
{
	git_str buf = GIT_BUF_INIT;
	int error;
	bool done = (stats->indexed_deltas == stats->total_deltas);

	error = git_str_puts(&buf, "Resolving deltas: ");
	if (!error)
		error = git_str_putui_pad(
		        &buf,
		        (uintmax_t)percent(
		                stats->indexed_deltas, stats->total_deltas),
		        10, 3, ' ');
	if (!error)
		error = git_str_puts(&buf, "% (");
	if (!error)
		error = git_str_putui(&buf, stats->indexed_deltas);
	if (!error)
		error = git_str_putc(&buf, '/');
	if (!error)
		error = git_str_putui(&buf, stats->total_deltas);
	if (!error)
		error = git_str_puts(&buf, ")");
	if (!error && done)
		error = git_str_puts(&buf, ", done.");
	if (!error)
		error = git_str_putc(&buf, '\r');

	if (error == 0)
		error = progress_write(progress, false, &buf);

	git_str_dispose(&buf);
	return error;
}

int cli_progress_fetch_transfer(const git_indexer_progress *stats, void *payload)
{
	cli_progress *progress = (cli_progress *)payload;
	int error = 0;

	switch (progress->action) {
	case CLI_PROGRESS_NONE:
		progress->action = CLI_PROGRESS_RECEIVING;
		/* fall through */

	case CLI_PROGRESS_RECEIVING:
		if ((error = fetch_receiving(progress, stats)) < 0)
			break;

		/*
		 * Upgrade from receiving to resolving; do this after the
		 * final call to cli_progress_fetch_receiving (above) to
		 * ensure that we've printed a final "done" string after
		 * any sideband data.
		 */
		if (!stats->indexed_deltas)
			break;

		progress_complete(progress);
		progress->action = CLI_PROGRESS_RESOLVING;
		/* fall through */

	case CLI_PROGRESS_RESOLVING:
		error = indexer_resolving(progress, stats);
		break;

	default:
		/* should not be reached */
		GIT_ASSERT(!"unexpected progress state");
	}

	return error;
}

int cli_progress_indexer(const git_indexer_progress *stats, void *payload)
{
	cli_progress *progress = (cli_progress *)payload;
	int error = 0;

	switch (progress->action) {
	case CLI_PROGRESS_NONE:
		progress->action = CLI_PROGRESS_INDEXING;
		/* fall through */

	case CLI_PROGRESS_INDEXING:
		if ((error = indexer_indexing(progress, stats)) < 0)
			break;

		if (stats->indexed_deltas == stats->total_deltas)
			break;

		progress_complete(progress);
		progress->action = CLI_PROGRESS_RESOLVING;
		/* fall through */

	case CLI_PROGRESS_RESOLVING:
		error = indexer_resolving(progress, stats);
		break;

	default:
		/* should not be reached */
		GIT_ASSERT(!"unexpected progress state");
	}

	return error;
}

void cli_progress_checkout(
        const char *path,
        size_t completed_steps,
        size_t total_steps,
        void *payload)
{
	cli_progress *progress = (cli_progress *)payload;
	bool done = (completed_steps == total_steps);

	GIT_UNUSED(path);

	if (progress->action != CLI_PROGRESS_CHECKING_OUT) {
		progress_complete(progress);
		progress->action = CLI_PROGRESS_CHECKING_OUT;
	}

	{
		git_str buf = GIT_BUF_INIT;

		if (git_str_puts(&buf, "Checking out files: ") == 0 &&
		    git_str_putui_pad(
		            &buf,
		            (uintmax_t)percent(completed_steps, total_steps),
		            10, 3, ' ') == 0 &&
		    git_str_puts(&buf, "% (") == 0 &&
		    git_str_putui(&buf, completed_steps) == 0 &&
		    git_str_putc(&buf, '/') == 0 &&
		    git_str_putui(&buf, total_steps) == 0 &&
		    git_str_puts(&buf, ")") == 0 &&
		    (!done || git_str_puts(&buf, ", done.") == 0) &&
		    git_str_putc(&buf, '\r') == 0)
			progress_write(progress, false, &buf);

		git_str_dispose(&buf);
	}
}

int cli_progress_abort(cli_progress *progress)
{
	if (progress->onscreen.size > 0 && printf("\n") < 0)
		return_os_error("could not print status");

	return 0;
}

int cli_progress_finish(cli_progress *progress)
{
	int error = progress->action ? progress_complete(progress) : 0;

	progress->action = 0;
	return error;
}

void cli_progress_dispose(cli_progress *progress)
{
	if (progress == NULL)
		return;

	git_str_dispose(&progress->sideband);
	git_str_dispose(&progress->onscreen);
	git_str_dispose(&progress->deferred);

	memset(progress, 0, sizeof(cli_progress));
}
