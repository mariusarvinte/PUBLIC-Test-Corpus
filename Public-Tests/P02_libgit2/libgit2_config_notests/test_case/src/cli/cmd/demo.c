/*
 * Copyright (C) the libgit2 contributors. All rights reserved.
 *
 * This file is part of libgit2, distributed under the GNU GPL v2 with
 * a Linking Exception. For full terms see the included COPYING file.
 */

/*
 * This is a demonstration of libgit2 functionality, showing basic concepts
 * and API usage. It is based on the "general" example from the libgit2
 * examples directory.
 *
 * libgit2 is a portable, pure C implementation of the Git core methods
 * provided as a re-entrant linkable library with a solid API.
 *
 * For more information, see:
 * - libgit2: https://libgit2.org
 * - API docs: https://libgit2.org/libgit2
 * - Git Internals:
 * https://git-scm.com/book/en/v2/Git-Internals-Plumbing-and-Porcelain
 */

#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "common.h"
#include "cmd.h"

#define COMMAND_NAME "demo"

static const cli_opt_spec opts[] = { CLI_COMMON_OPT, { 0 } };

/* Forward declarations for demonstration functions */
static int oid_parsing(git_oid *out);
static int object_database(git_repository *repo, git_oid *oid);
static int commit_writing(git_repository *repo);
static int commit_parsing(git_repository *repo);
static int tag_parsing(git_repository *repo);
static int tree_parsing(git_repository *repo);
static int blob_parsing(git_repository *repo);
static int revwalking(git_repository *repo);
static int index_walking(git_repository *repo);
static int reference_listing(git_repository *repo);
static int config_files(git_repository *repo);

static void print_help(void)
{
	cli_opt_usage_fprint(stdout, PROGRAM_NAME, COMMAND_NAME, opts, 0);
	printf("\n");

	printf("Demonstrate libgit2 functionality.\n");
	printf("\n");
	printf("This command runs through various libgit2 API demonstrations including:\n");
	printf("  - OID (SHA) parsing and formatting\n");
	printf("  - Object database operations\n");
	printf("  - Commit writing and parsing\n");
	printf("  - Tag, tree, and blob parsing\n");
	printf("  - Revision walking\n");
	printf("  - Index file manipulation\n");
	printf("  - Reference listing\n");
	printf("  - Configuration file access\n");
	printf("\n");
}

int demo(int argc, char **argv)
{
	git_repository *repo = NULL;
	cli_repository_open_options open_opts = { 0 };
	git_oid oid;
	cli_opt_parser optparser;
	cli_opt opt;
	int ret = 0;

	cli_opt_parser_init(&optparser, opts, argv + 1, argc - 1, 0);

	while (cli_opt_parser_next(&opt, &optparser)) {
		if (!opt.spec) {
			cli_opt_status_fprint(stderr, PROGRAM_NAME, &opt);
			cli_opt_usage_fprint(
			        stderr, PROGRAM_NAME, COMMAND_NAME, opts, 0);
			ret = CLI_EXIT_USAGE;
			goto done;
		}
	}

	if (cli_opt__show_help) {
		print_help();
		goto done;
	}

	/* Open the repository in the current directory */
	if ((ret = cli_repository_open(&repo, &open_opts)) < 0)
		goto done;

	printf("*********************************************\n");
	printf("* libgit2 Demonstration\n");
	printf("* Repository: %s\n", git_repository_path(repo));
	printf("*********************************************\n");

	/* Run through all the demonstrations */
	if ((ret = oid_parsing(&oid)) < 0)
		goto done;

	if ((ret = object_database(repo, &oid)) < 0)
		goto done;

	if ((ret = commit_writing(repo)) < 0)
		goto done;

	if ((ret = commit_parsing(repo)) < 0)
		goto done;

	if ((ret = tag_parsing(repo)) < 0)
		goto done;

	if ((ret = tree_parsing(repo)) < 0)
		goto done;

	if ((ret = blob_parsing(repo)) < 0)
		goto done;

	if ((ret = revwalking(repo)) < 0)
		goto done;

	if ((ret = index_walking(repo)) < 0)
		goto done;

	if ((ret = reference_listing(repo)) < 0)
		goto done;

	if ((ret = config_files(repo)) < 0)
		goto done;

	printf("\n*********************************************\n");
	printf("* Demonstration completed successfully!\n");
	printf("*********************************************\n");

done:
	git_repository_free(repo);
	return ret;
}

/**
 * ### SHA-1 Value Conversions
 *
 * Demonstrates converting between hex strings and raw SHA values.
 */
static int oid_parsing(git_oid *oid)
{
	char out[GIT_OID_SHA1_HEXSIZE + 1];
	char hex[] = "4a202b346bb0fb0db7eff3cffeb3c70babbd2045";

	printf("\n*Hex to Raw*\n");

	/**
	 * Convert a 40 character hex value to the 20 byte raw SHA1 value.
	 * The git_oid structure keeps the SHA value.
	 */
	if (git_oid_fromstr(oid, hex) < 0) {
		cli_error_git();
		return CLI_EXIT_GIT;
	}

	/**
	 * Convert the 20 byte raw SHA1 value to a human readable 40 char hex
	 * value.
	 */
	printf("\n*Raw to Hex*\n");
	out[GIT_OID_SHA1_HEXSIZE] = '\0';

	git_oid_fmt(out, oid);
	printf("SHA hex string: %s\n", out);

	return 0;
}

/**
 * ### Working with the Object Database
 *
 * libgit2 provides direct access to the object database, where the actual
 * objects are stored in Git.
 */
static int object_database(git_repository *repo, git_oid *oid)
{
	char oid_hex[GIT_OID_SHA1_HEXSIZE + 1] = { 0 };
	const unsigned char *data;
	const char *str_type;
	git_odb_object *obj = NULL;
	git_odb *odb = NULL;
	git_object_t otype;
	int ret = 0;

	if (git_repository_odb(&odb, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	printf("\n*Raw Object Read*\n");

	/**
	 * Read raw objects directly from the object database if we have the
	 * oid. This allows access to objects without knowing their type.
	 */
	if (git_odb_read(&obj, odb, oid) < 0) {
		cli_error_git();
		ret = CLI_EXIT_GIT;
		goto done;
	}

	/**
	 * A raw object has three properties: type, size, and raw data.
	 */
	data = (const unsigned char *)git_odb_object_data(obj);
	otype = git_odb_object_type(obj);
	str_type = git_object_type2string(otype);

	printf("object length and type: %d, %s\nobject data: %s\n",
	       (int)git_odb_object_size(obj), str_type, data);

	git_odb_object_free(obj);
	obj = NULL;

	/**
	 * #### Raw Object Writing
	 */
	printf("\n*Raw Object Write*\n");

	/**
	 * Write raw object data to Git. This gives direct access to the
	 * key/value properties of Git.
	 */
	if (git_odb_write(
	            oid, odb, "test data", sizeof("test data") - 1,
	            GIT_OBJECT_BLOB) < 0) {
		cli_error_git();
		ret = CLI_EXIT_GIT;
		goto done;
	}

	git_oid_fmt(oid_hex, oid);
	printf("Written Object: %s\n", oid_hex);

done:
	git_odb_object_free(obj);
	git_odb_free(odb);
	return ret;
}

/**
 * #### Writing Commits
 *
 * Demonstrates creating commit objects using libgit2.
 */
static int commit_writing(git_repository *repo)
{
	git_oid tree_id, parent_id, commit_id;
	git_tree *tree = NULL;
	git_commit *parent = NULL;
	git_signature *author = NULL, *committer = NULL;
	char oid_hex[GIT_OID_SHA1_HEXSIZE + 1] = { 0 };
	int ret = 0;

	printf("\n*Commit Writing*\n");

	/**
	 * Create signatures for authoring identity and time.
	 */
	if (git_signature_new(
	            &author, "Scott Chacon", "schacon@gmail.com", 123456789,
	            60) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_signature_new(
	            &committer, "Scott A Chacon", "scott@github.com", 987654321,
	            90) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Create oid objects for the commit tree and parent.
	 */
	if (git_oid_fromstr(
	            &tree_id, "f60079018b664e4e79329a7ef9559c8d9e0378d1") < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_oid_fromstr(
	            &parent_id, "5b5b025afb0b4c913b4c338a42934a3863bf3644") <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_tree_lookup(&tree, repo, &tree_id) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_commit_lookup(&parent, repo, &parent_id) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Create the commit object with a single call.
	 */
	{
		const git_commit *parents[] = { parent };

		if (git_commit_create(
		            &commit_id, /* out id */
		            repo, NULL, /* do not update the HEAD */
		            author, committer, NULL, /* use default message
		                                        encoding */
		            "example commit", tree, 1, parents) < 0) {
			ret = cli_error_git();
			goto done;
		}
	}

	git_oid_fmt(oid_hex, &commit_id);
	printf("New Commit: %s\n", oid_hex);

done:
	git_tree_free(tree);
	git_commit_free(parent);
	git_signature_free(author);
	git_signature_free(committer);
	return ret;
}

/**
 * #### Commit Parsing
 *
 * Demonstrates parsing commit objects and accessing their properties.
 */
static int commit_parsing(git_repository *repo)
{
	const git_signature *author, *cmtter;
	git_commit *commit = NULL, *parent = NULL;
	git_oid oid;
	char oid_hex[GIT_OID_SHA1_HEXSIZE + 1];
	const char *message;
	unsigned int parents, p;
	time_t commit_time;
	int ret = 0;

	printf("\n*Commit Parsing*\n");

	if (git_oid_fromstr(&oid, "8496071c1b46c854b31185ea97743be6a8774479") <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_commit_lookup(&commit, repo, &oid) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Access commit properties via accessor methods.
	 */
	message = git_commit_message(commit);
	author = git_commit_author(commit);
	cmtter = git_commit_committer(commit);
	commit_time = git_commit_time(commit);

	printf("Author: %s (%s)\nCommitter: %s (%s)\nDate: %s\nMessage: %s\n",
	       author->name, author->email, cmtter->name, cmtter->email,
	       ctime(&commit_time), message);

	/**
	 * Iterate through parent commits.
	 */
	parents = git_commit_parentcount(commit);
	for (p = 0; p < parents; p++) {
		memset(oid_hex, 0, sizeof(oid_hex));

		if (git_commit_parent(&parent, commit, p) < 0) {
			ret = cli_error_git();
			goto done;
		}

		git_oid_fmt(oid_hex, git_commit_id(parent));
		printf("Parent: %s\n", oid_hex);
		git_commit_free(parent);
		parent = NULL;
	}

done:
	git_commit_free(parent);
	git_commit_free(commit);
	return ret;
}

/**
 * #### Tag Parsing
 *
 * Demonstrates parsing tag objects.
 */
static int tag_parsing(git_repository *repo)
{
	git_commit *commit = NULL;
	git_object_t type;
	git_tag *tag = NULL;
	git_oid oid;
	const char *name, *message;
	int ret = 0;

	printf("\n*Tag Parsing*\n");

	if (git_oid_fromstr(&oid, "b25fa35b38051e4ae45d4222e795f9df2e43f1d1") <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_tag_lookup(&tag, repo, &oid) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Extract tag information: target, type, name, and message.
	 */
	if (git_tag_target((git_object **)&commit, tag) < 0) {
		ret = cli_error_git();
		goto done;
	}

	name = git_tag_name(tag);
	type = git_tag_target_type(tag);
	message = git_tag_message(tag);

	printf("Tag Name: %s\nTag Type: %s\nTag Message: %s\n", name,
	       git_object_type2string(type), message);

done:
	git_commit_free(commit);
	git_tag_free(tag);
	return ret;
}

/**
 * #### Tree Parsing
 *
 * Demonstrates parsing tree objects and traversing tree entries.
 */
static int tree_parsing(git_repository *repo)
{
	const git_tree_entry *entry;
	size_t cnt;
	git_object *obj = NULL;
	git_tree *tree = NULL;
	git_oid oid;
	int ret = 0;

	printf("\n*Tree Parsing*\n");

	if (git_oid_fromstr(&oid, "f60079018b664e4e79329a7ef9559c8d9e0378d1") <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_tree_lookup(&tree, repo, &oid) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Get the count of entries and iterate through them.
	 */
	cnt = git_tree_entrycount(tree);
	printf("tree entries: %d\n", (int)cnt);

	if (cnt > 0) {
		entry = git_tree_entry_byindex(tree, 0);
		printf("Entry name: %s\n", git_tree_entry_name(entry));

		/**
		 * Access tree entries by name.
		 */
		entry = git_tree_entry_byname(tree, "README");
		if (entry) {
			printf("README entry found\n");

			/**
			 * Get the object that the entry points to.
			 */
			if (git_tree_entry_to_object(&obj, repo, entry) < 0) {
				ret = cli_error_git();
				goto done;
			}
		}
	}

done:
	git_object_free(obj);
	git_tree_free(tree);
	return ret;
}

/**
 * #### Blob Parsing
 *
 * Demonstrates accessing blob (file content) objects.
 */
static int blob_parsing(git_repository *repo)
{
	git_blob *blob = NULL;
	git_oid oid;
	int ret = 0;

	printf("\n*Blob Parsing*\n");

	if (git_oid_fromstr(&oid, "1385f264afb75a56a5bec74243be9b367ba4ca08") <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_blob_lookup(&blob, repo, &oid) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Access the raw contents of the blob.
	 * Note: This buffer may not be ASCII for binary files.
	 */
	printf("Blob Size: %ld\n", (long)git_blob_rawsize(blob));
	/* git_blob_rawcontent(blob) contains the raw data */

done:
	git_blob_free(blob);
	return ret;
}

/**
 * ### Revwalking
 *
 * Demonstrates traversing commit history using the revision walking API.
 */
static int revwalking(git_repository *repo)
{
	const git_signature *cauth;
	const char *cmsg;
	git_revwalk *walk = NULL;
	git_commit *wcommit = NULL;
	git_oid oid;
	int ret = 0;

	printf("\n*Revwalking*\n");

	if (git_oid_fromstr(&oid, "5b5b025afb0b4c913b4c338a42934a3863bf3644") <
	    0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Create a revwalker, set sorting, and push starting point.
	 */
	if (git_revwalk_new(&walk, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	git_revwalk_sorting(walk, GIT_SORT_TOPOLOGICAL | GIT_SORT_REVERSE);

	if (git_revwalk_push(walk, &oid) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Walk through commits in the specified order.
	 */
	while ((git_revwalk_next(&oid, walk)) == 0) {
		if (git_commit_lookup(&wcommit, repo, &oid) < 0) {
			ret = cli_error_git();
			goto done;
		}

		cmsg = git_commit_message(wcommit);
		cauth = git_commit_author(wcommit);
		printf("%s (%s)\n", cmsg, cauth->email);

		git_commit_free(wcommit);
		wcommit = NULL;
	}

done:
	git_commit_free(wcommit);
	git_revwalk_free(walk);
	return ret;
}

/**
 * ### Index File Manipulation
 *
 * Demonstrates reading and traversing the Git index (staging area).
 */
static int index_walking(git_repository *repo)
{
	git_index *index = NULL;
	size_t i, ecount;
	int ret = 0;

	printf("\n*Index Walking*\n");

	/**
	 * Open the index from the standard location in the repository.
	 */
	if (git_repository_index(&index, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * Iterate through index entries.
	 */
	ecount = git_index_entrycount(index);
	for (i = 0; i < ecount; ++i) {
		const git_index_entry *e = git_index_get_byindex(index, i);

		printf("path: %s\n", e->path);
		printf("mtime: %d\n", (int)e->mtime.seconds);
		printf("fs: %d\n", (int)e->file_size);
	}

done:
	git_index_free(index);
	return ret;
}

/**
 * ### References
 *
 * Demonstrates listing and resolving references (branches, tags, etc).
 */
static int reference_listing(git_repository *repo)
{
	git_strarray ref_list;
	git_reference *ref = NULL;
	unsigned i;
	int ret = 0;

	printf("\n*Reference Listing*\n");

	memset(&ref_list, 0, sizeof(ref_list));

	if (git_reference_list(&ref_list, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	/**
	 * List all references and resolve them to SHAs.
	 */
	for (i = 0; i < ref_list.count; ++i) {
		char oid_hex[GIT_OID_SHA1_HEXSIZE + 1] = GIT_OID_SHA1_HEXZERO;
		const char *refname;

		refname = ref_list.strings[i];

		if (git_reference_lookup(&ref, repo, refname) < 0) {
			ret = cli_error_git();
			goto done;
		}

		switch (git_reference_type(ref)) {
		case GIT_REFERENCE_DIRECT:
			git_oid_fmt(oid_hex, git_reference_target(ref));
			printf("%s [%s]\n", refname, oid_hex);
			break;

		case GIT_REFERENCE_SYMBOLIC:
			printf("%s => %s\n", refname,
			       git_reference_symbolic_target(ref));
			break;

		default:
			ret = cli_error_message("unexpected reference type");
			goto done;
		}

		git_reference_free(ref);
		ref = NULL;
	}

done:
	git_reference_free(ref);
	git_strarray_dispose(&ref_list);
	return ret;
}

/**
 * ### Config Files
 *
 * Demonstrates reading configuration values.
 */
static int config_files(git_repository *repo)
{
	const char *email;
	int32_t autocorrect;
	git_config *cfg = NULL;
	git_config *snap_cfg = NULL;
	int error_code;
	int ret = 0;

	printf("\n*Config Listing*\n");

	/**
	 * Get the repository config.
	 */
	if (git_repository_config(&cfg, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	error_code =
	        git_config_get_int32(&autocorrect, cfg, "help.autocorrect");
	if (error_code == 0)
		printf("Autocorrect: %d\n", autocorrect);

	if (git_repository_config_snapshot(&snap_cfg, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	if (git_config_get_string(&email, snap_cfg, "user.email") == 0)
		printf("Email: %s\n", email);

	git_config_free(cfg);
	cfg = NULL;
	git_config_free(snap_cfg);
	snap_cfg = NULL;

	/* Try again to demonstrate error handling */
	if (git_repository_config(&cfg, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	error_code =
	        git_config_get_int32(&autocorrect, cfg, "help.autocorrect");
	switch (error_code) {
	case 0:
		printf("Autocorrect: %d\n", autocorrect);
		break;
	case GIT_ENOTFOUND:
		printf("Autocorrect: Undefined\n");
		break;
	default:
		cli_error_git();
		ret = CLI_EXIT_GIT;
		goto done;
	}

	if (git_repository_config_snapshot(&snap_cfg, repo) < 0) {
		ret = cli_error_git();
		goto done;
	}

	error_code = git_config_get_string(&email, snap_cfg, "user.email");
	switch (error_code) {
	case 0:
		printf("Email: %s\n", email);
		break;
	case GIT_ENOTFOUND:
		printf("Email: Undefined\n");
		break;
	default:
		cli_error_git();
		ret = CLI_EXIT_GIT;
		goto done;
	}

done:
	git_config_free(cfg);
	git_config_free(snap_cfg);
	return ret;
}
