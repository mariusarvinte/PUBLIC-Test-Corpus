/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

#ifndef CLI_cmd_h__
#define CLI_cmd_h__

/* Command definitions */
typedef struct {
	const char *name;
	int (*fn)(int argc, char **argv);
	const char *desc;
} cli_cmd_spec;

/* Options that are common to all commands (eg --help, --git-dir) */
extern const cli_opt_spec cli_common_opts[];

/* All the commands supported by the CLI */
extern const cli_cmd_spec cli_cmds[];

/* Find a command by name */
extern const cli_cmd_spec *cli_cmd_spec_byname(const char *name);

/* Commands */
extern int add(int argc, char **argv);
extern int blame(int argc, char **argv);
extern int branch_cmd(int argc, char **argv);
extern int cat_file(int argc, char **argv);
extern int cherry_pick_cmd(int argc, char **argv);
extern int checkout(int argc, char **argv);
extern int clone(int argc, char **argv);
extern int commit(int argc, char **argv);
extern int config(int argc, char **argv);
extern int demo(int argc, char **argv);
extern int describe(int argc, char **argv);
extern int diff(int argc, char **argv);
extern int fetch(int argc, char **argv);
extern int for_each_ref(int argc, char **argv);
extern int hash_object(int argc, char **argv);
extern int help(int argc, char **argv);
extern int index_pack(int argc, char **argv);
extern int init(int argc, char **argv);
extern int log_cmd(int argc, char **argv);
extern int ls_files(int argc, char **argv);
extern int ls_remote(int argc, char **argv);
extern int merge(int argc, char **argv);
extern int note_cmd(int argc, char **argv);
extern int push(int argc, char **argv);
extern int rebase_cmd(int argc, char **argv);
extern int remote(int argc, char **argv);
extern int reset_cmd(int argc, char **argv);
extern int rev_list(int argc, char **argv);
extern int rev_parse(int argc, char **argv);
extern int show_index(int argc, char **argv);
extern int stash(int argc, char **argv);
extern int status(int argc, char **argv);
extern int submodule_cmd(int argc, char **argv);
extern int tag(int argc, char **argv);
extern int version(int argc, char **argv);
extern int worktree_cmd(int argc, char **argv);

#endif /* CLI_cmd_h__ */
