#include "clar_libgit2.h"
#include "posix.h"
#include "fs_path.h"
#include "futils.h"
#include "git2/sys/repository.h"

void cl_git_report_failure(
        int error,
        int expected,
        const char *file,
        const char *func,
        int line,
        const char *fncall)
{
	char msg[4096];
	const git_error *last = git_error_last();

	if (expected)
		p_snprintf(
		        msg, 4096, "error %d (expected %d) - %s", error,
		        expected, last ? last->message : "<no message>");
	else if (error || last)
		p_snprintf(
		        msg, 4096, "error %d - %s", error,
		        last ? last->message : "<no message>");
	else
		p_snprintf(msg, 4096, "no error, expected non-zero return");

	clar__assert(0, file, func, line, fncall, msg, 1);
}

int git_str_vprintf(git_str *buf, const char *format, va_list ap)
{
	size_t expected_size, new_size;
	int len;

	GIT_ERROR_CHECK_ALLOC_MULTIPLY(&expected_size, strlen(format), 2);
	GIT_ERROR_CHECK_ALLOC_ADD(&expected_size, expected_size, buf->size);

	if (git_str_grow(buf, expected_size) < 0)
		return -1;

	while (1) {
		va_list args;

		va_copy(args, ap);
		len = p_vsnprintf(
		        buf->ptr + buf->size, buf->asize - buf->size, format,
		        args);
		va_end(args);

		if (len < 0) {
			git__free(buf->ptr);
			buf->ptr = git_str__oom;
			return -1;
		}

		if ((size_t)len + 1 <= buf->asize - buf->size) {
			buf->size += len;
			break;
		}

		GIT_ERROR_CHECK_ALLOC_ADD(&new_size, buf->size, len);
		GIT_ERROR_CHECK_ALLOC_ADD(&new_size, new_size, 1);

		if (git_str_grow(buf, new_size) < 0)
			return -1;
	}

	return 0;
}

int git_str_printf(git_str *buf, const char *format, ...)
{
	int error;
	va_list ap;

	va_start(ap, format);
	error = git_str_vprintf(buf, format, ap);
	va_end(ap);

	return error;
}

void cl_git_mkfile(const char *filename, const char *content)
{
	int fd;

	fd = p_creat(filename, 0666);
	cl_assert(fd != -1);

	if (content) {
		cl_must_pass(p_write(fd, content, strlen(content)));
	} else {
		cl_must_pass(p_write(fd, filename, strlen(filename)));
		cl_must_pass(p_write(fd, "\n", 1));
	}

	cl_must_pass(p_close(fd));
}

void cl_git_write2file(
        const char *path,
        const char *content,
        size_t content_len,
        int flags,
        unsigned int mode)
{
	int fd;
	cl_assert(path && content);
	cl_assert((fd = p_open(path, flags, mode)) >= 0);
	if (!content_len)
		content_len = strlen(content);
	cl_must_pass(p_write(fd, content, content_len));
	cl_must_pass(p_close(fd));
}

void cl_git_append2file(const char *path, const char *content)
{
	cl_git_write2file(
	        path, content, 0, O_WRONLY | O_CREAT | O_APPEND, 0644);
}

void cl_git_rewritefile(const char *path, const char *content)
{
	cl_git_write2file(path, content, 0, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

void cl_git_rmfile(const char *filename)
{
	cl_must_pass(p_unlink(filename));
}

char *cl_getenv(const char *name)
{
	git_str out = GIT_STR_INIT;
	int error = git__getenv(&out, name);

	cl_assert(error >= 0 || error == GIT_ENOTFOUND);

	if (error == GIT_ENOTFOUND)
		return NULL;

	if (out.size == 0) {
		char *dup = git__strdup("");
		cl_assert(dup);

		return dup;
	}

	return git_str_detach(&out);
}

bool cl_is_env_set(const char *name)
{
	char *env = cl_getenv(name);
	bool result = (env != NULL);
	git__free(env);
	return result;
}

#include <stdlib.h>

int cl_setenv(const char *name, const char *value)
{
	return (value == NULL) ? unsetenv(name) : setenv(name, value, 1);
}

int cl_rename(const char *source, const char *dest)
{
	return p_rename(source, dest);
}

static const char *_cl_sandbox = NULL;
static git_repository *_cl_repo = NULL;

git_repository *cl_git_sandbox_init(const char *sandbox)
{
	/* Get the name of the sandbox folder which will be created */
	const char *basename = cl_fixture_basename(sandbox);

	/* Copy the whole sandbox folder from our fixtures to our test sandbox
	 * area.  After this it can be accessed with `./sandbox`
	 */
	cl_fixture_sandbox(sandbox);
	_cl_sandbox = sandbox;

	cl_git_pass(p_chdir(basename));

	/* If this is not a bare repo, then rename `sandbox/.gitted` to
	 * `sandbox/.git` which must be done since we cannot store a folder
	 * named `.git` inside the fixtures folder of our libgit2 repo.
	 */
	if (p_access(".gitted", F_OK) == 0)
		cl_git_pass(cl_rename(".gitted", ".git"));

	/* If we have `gitattributes`, rename to `.gitattributes`.  This may
	 * be necessary if we don't want the attributes to be applied in the
	 * libgit2 repo, but just during testing.
	 */
	if (p_access("gitattributes", F_OK) == 0)
		cl_git_pass(cl_rename("gitattributes", ".gitattributes"));

	/* As with `gitattributes`, we may need `gitignore` just for testing. */
	if (p_access("gitignore", F_OK) == 0)
		cl_git_pass(cl_rename("gitignore", ".gitignore"));

	cl_git_pass(p_chdir(".."));

	/* Now open the sandbox repository and make it available for tests */
	cl_git_pass(git_repository_open(&_cl_repo, basename));

	/* Adjust configs after copying to new filesystem */
	cl_git_pass(git_repository_reinit_filesystem(_cl_repo, 0));

	return _cl_repo;
}

git_repository *cl_git_sandbox_init_new(const char *sandbox)
{
	cl_git_pass(git_repository_init(&_cl_repo, sandbox, false));
	_cl_sandbox = sandbox;

	return _cl_repo;
}

git_repository *cl_git_sandbox_reopen(void)
{
	if (_cl_repo) {
		git_repository_free(_cl_repo);
		_cl_repo = NULL;

		cl_git_pass(git_repository_open(
		        &_cl_repo, cl_fixture_basename(_cl_sandbox)));
	}

	return _cl_repo;
}

void cl_git_sandbox_cleanup(void)
{
	if (_cl_repo) {
		git_repository_free(_cl_repo);
		_cl_repo = NULL;
	}
	if (_cl_sandbox) {
		cl_fixture_cleanup(_cl_sandbox);
		_cl_sandbox = NULL;
	}
}

bool cl_toggle_filemode(const char *filename)
{
	struct stat st1, st2;

	cl_must_pass(p_stat(filename, &st1));
	cl_must_pass(p_chmod(filename, st1.st_mode ^ 0100));
	cl_must_pass(p_stat(filename, &st2));

	return (st1.st_mode != st2.st_mode);
}

bool cl_is_chmod_supported(void)
{
	static int _is_supported = -1;

	if (_is_supported < 0) {
		cl_git_mkfile("filemode.t", "Test if filemode can be modified");
		_is_supported = cl_toggle_filemode("filemode.t");
		cl_must_pass(p_unlink("filemode.t"));
	}

	return _is_supported;
}

const char *cl_git_fixture_url(const char *fixturename)
{
	return cl_git_path_url(cl_fixture(fixturename));
}

const char *cl_git_path_url(const char *path)
{
	static char url[4096 + 1];

	const char *in_buf;
	git_str path_buf = GIT_STR_INIT;
	git_str url_buf = GIT_STR_INIT;

	cl_git_pass(git_fs_path_prettify_dir(&path_buf, path, NULL));
	cl_git_pass(git_str_puts(&url_buf, "file://"));

	in_buf = git_str_cstr(&path_buf);

	/*
	 * A very hacky Url encoding that only takes care of escaping the spaces
	 */
	while (*in_buf) {
		if (*in_buf == ' ')
			cl_git_pass(git_str_puts(&url_buf, "%20"));
		else
			cl_git_pass(git_str_putc(&url_buf, *in_buf));

		in_buf++;
	}

	cl_assert(url_buf.size < sizeof(url) - 1);

	strncpy(url, git_str_cstr(&url_buf), sizeof(url) - 1);
	url[sizeof(url) - 1] = '\0';
	git_str_dispose(&url_buf);
	git_str_dispose(&path_buf);
	return url;
}

const char *cl_git_sandbox_path(int is_dir, ...)
{
	const char *path = NULL;
	static char _temp[GIT_PATH_MAX];
	git_str buf = GIT_STR_INIT;
	va_list arg;

	cl_git_pass(git_str_sets(&buf, clar_sandbox_path()));

	va_start(arg, is_dir);

	while ((path = va_arg(arg, const char *)) != NULL) {
		cl_git_pass(git_str_joinpath(&buf, buf.ptr, path));
	}
	va_end(arg);

	cl_git_pass(git_fs_path_prettify(&buf, buf.ptr, NULL));
	if (is_dir)
		git_fs_path_to_dir(&buf);

	/* make sure we won't truncate */
	cl_assert(git_str_len(&buf) < sizeof(_temp));
	git_str_copy_cstr(_temp, sizeof(_temp), &buf);

	git_str_dispose(&buf);

	return _temp;
}

typedef struct {
	const char *filename;
	size_t filename_len;
} remove_data;

static int remove_placeholders_recurs(void *_data, git_str *path)
{
	remove_data *data = (remove_data *)_data;
	size_t pathlen;

	if (git_fs_path_isdir(path->ptr) == true)
		return git_fs_path_direach(
		        path, 0, remove_placeholders_recurs, data);

	pathlen = path->size;

	if (pathlen < data->filename_len)
		return 0;

	/* if path ends in '/'+filename (or equals filename) */
	if (!strcmp(data->filename, path->ptr + pathlen - data->filename_len) &&
	    (pathlen == data->filename_len ||
	     path->ptr[pathlen - data->filename_len - 1] == '/'))
		return p_unlink(path->ptr);

	return 0;
}

int cl_git_remove_placeholders(const char *directory_path, const char *filename)
{
	int error;
	remove_data data;
	git_str buffer = GIT_STR_INIT;

	if (git_fs_path_isdir(directory_path) == false)
		return -1;

	if (git_str_sets(&buffer, directory_path) < 0)
		return -1;

	data.filename = filename;
	data.filename_len = strlen(filename);

	error = remove_placeholders_recurs(&data, &buffer);

	git_str_dispose(&buffer);

	return error;
}

#define CL_COMMIT_NAME "Libgit2 Tester"
#define CL_COMMIT_EMAIL "libgit2-test@github.com"
#define CL_COMMIT_MSG "Test commit of tree "

void cl_repo_commit_from_index(
        git_oid *out,
        git_repository *repo,
        git_signature *sig,
        git_time_t time,
        const char *msg)
{
	git_index *index;
	git_oid commit_id, tree_id;
	git_object *parent = NULL;
	git_reference *ref = NULL;
	git_tree *tree = NULL;
	char buf[128];
	int free_sig = (sig == NULL);

	/* it is fine if looking up HEAD fails - we make this the first commit
	 */
	git_revparse_ext(&parent, &ref, repo, "HEAD");

	/* write the index content as a tree */
	cl_git_pass(git_repository_index(&index, repo));
	cl_git_pass(git_index_write_tree(&tree_id, index));
	cl_git_pass(git_index_write(index));
	git_index_free(index);

	cl_git_pass(git_tree_lookup(&tree, repo, &tree_id));

	if (sig)
		cl_assert(sig->name && sig->email);
	else if (!time)
		cl_git_pass(git_signature_now(
		        &sig, CL_COMMIT_NAME, CL_COMMIT_EMAIL));
	else
		cl_git_pass(git_signature_new(
		        &sig, CL_COMMIT_NAME, CL_COMMIT_EMAIL, time, 0));

	if (!msg) {
		strcpy(buf, CL_COMMIT_MSG);
		git_oid_tostr(
		        buf + strlen(CL_COMMIT_MSG),
		        sizeof(buf) - strlen(CL_COMMIT_MSG), &tree_id);
		msg = buf;
	}

	{
		const git_commit *parents[] = { (git_commit *)parent };

		cl_git_pass(git_commit_create(
		        &commit_id, repo,
		        ref ? git_reference_name(ref) : "HEAD", sig, sig, NULL,
		        msg, tree, parent ? 1 : 0, parent ? parents : NULL));
	}

	if (out)
		git_oid_cpy(out, &commit_id);

	git_object_free(parent);
	git_reference_free(ref);
	if (free_sig)
		git_signature_free(sig);
	git_tree_free(tree);
}

void cl_repo_set_bool(git_repository *repo, const char *cfg, int value)
{
	git_config *config;
	cl_git_pass(git_repository_config(&config, repo));
	cl_git_pass(git_config_set_bool(config, cfg, value != 0));
	git_config_free(config);
}

int cl_repo_get_bool(git_repository *repo, const char *cfg)
{
	int val = 0;
	git_config *config;
	cl_git_pass(git_repository_config(&config, repo));
	if (git_config_get_bool(&val, config, cfg) < 0)
		git_error_clear();
	git_config_free(config);
	return val;
}

void cl_repo_set_int(git_repository *repo, const char *cfg, int value)
{
	git_config *config;
	cl_git_pass(git_repository_config(&config, repo));
	cl_git_pass(git_config_set_int32(config, cfg, value));
	git_config_free(config);
}

int cl_repo_get_int(git_repository *repo, const char *cfg)
{
	int val = 0;
	git_config *config;
	cl_git_pass(git_repository_config(&config, repo));
	if (git_config_get_int32(&val, config, cfg) < 0)
		git_error_clear();
	git_config_free(config);
	return val;
}

void cl_repo_set_string(git_repository *repo, const char *cfg, const char *value)
{
	git_config *config;
	cl_git_pass(git_repository_config(&config, repo));
	cl_git_pass(git_config_set_string(config, cfg, value));
	git_config_free(config);
}

/* this is essentially the code from git__unescape modified slightly */
static size_t strip_cr_from_buf(char *start, size_t len)
{
	char *scan, *trail, *end = start + len;

	for (scan = trail = start; scan < end; trail++, scan++) {
		while (*scan == '\r')
			scan++; /* skip '\r' */

		if (trail != scan)
			*trail = *scan;
	}

	*trail = '\0';

	return (trail - start);
}

void clar__assert_equal_file(
        const char *expected_data,
        size_t expected_bytes,
        int ignore_cr,
        const char *path,
        const char *file,
        const char *func,
        int line)
{
	char buf[4000];
	ssize_t bytes, total_bytes = 0;
	int fd = p_open(path, O_RDONLY | O_BINARY, 0);
	cl_assert(fd >= 0);

	if (expected_data && !expected_bytes)
		expected_bytes = strlen(expected_data);

	while ((bytes = p_read(fd, buf, sizeof(buf))) != 0) {
		clar__assert(
		        bytes > 0, file, func, line, "error reading from file",
		        path, 1);

		if (ignore_cr)
			bytes = strip_cr_from_buf(buf, bytes);

		if (memcmp(expected_data, buf, bytes) != 0) {
			int pos;
			for (pos = 0;
			     pos < bytes && expected_data[pos] == buf[pos];
			     ++pos)
				/* find differing byte offset */;
			p_snprintf(
			        buf, sizeof(buf),
			        "file content mismatch at byte %" PRIdZ,
			        (ssize_t)(total_bytes + pos));
			p_close(fd);
			clar__fail(file, func, line, path, buf, 1);
		}

		expected_data += bytes;
		total_bytes += bytes;
	}

	p_close(fd);

	clar__assert(
	        !bytes, file, func, line, "error reading from file", path, 1);
	clar__assert_equal(
	        file, func, line, "mismatched file length", 1, "%" PRIuZ,
	        (size_t)expected_bytes, (size_t)total_bytes);
}

#define FAKE_HOMEDIR_NAME "cl_fake_home"

static git_buf _cl_restore_homedir = GIT_BUF_INIT;

void cl_fake_homedir_cleanup(void *payload)
{
	GIT_UNUSED(payload);

	if (_cl_restore_homedir.ptr) {
		cl_git_pass(git_futils_rmdir_r(
		        FAKE_HOMEDIR_NAME, NULL, GIT_RMDIR_REMOVE_FILES));

		cl_git_pass(git_libgit2_set_homedir(_cl_restore_homedir.ptr));
		git_buf_dispose(&_cl_restore_homedir);
	}
}

void cl_fake_homedir(git_str *out)
{
	git_str path = GIT_STR_INIT;

	cl_git_pass(git_libgit2_get_homedir(&_cl_restore_homedir));

	cl_set_cleanup(cl_fake_homedir_cleanup, NULL);

	/* TOC/TOU but merely attempts to prevent accidental cleanup. */
	cl_assert(!git_fs_path_exists(FAKE_HOMEDIR_NAME));
	cl_must_pass(p_mkdir(FAKE_HOMEDIR_NAME, 0777));
	cl_git_pass(git_fs_path_prettify(&path, FAKE_HOMEDIR_NAME, NULL));
	cl_git_pass(git_libgit2_set_homedir(path.ptr));

	if (out)
		git_str_swap(out, &path);

	git_str_dispose(&path);
}

#define FAKE_GLOBALCONFIG_NAME "cl_fake_global"

static git_buf _cl_restore_globalconfig = GIT_BUF_INIT;

void cl_fake_globalconfig_cleanup(void *payload)
{
	GIT_UNUSED(payload);

	if (_cl_restore_globalconfig.ptr) {
		cl_git_pass(git_futils_rmdir_r(
		        FAKE_GLOBALCONFIG_NAME, NULL, GIT_RMDIR_REMOVE_FILES));

		cl_git_pass(git_libgit2_set_homedir(
		        _cl_restore_globalconfig.ptr));
		git_buf_dispose(&_cl_restore_globalconfig);
	}
}

void cl_fake_globalconfig(git_str *out)
{
	git_str path = GIT_STR_INIT;

	cl_git_pass(git_libgit2_get_search_path(
	        GIT_CONFIG_LEVEL_GLOBAL, &_cl_restore_globalconfig));

	cl_set_cleanup(cl_fake_globalconfig_cleanup, NULL);

	/* TOC/TOU but merely attempts to prevent accidental cleanup. */
	cl_assert(!git_fs_path_exists(FAKE_GLOBALCONFIG_NAME));
	cl_must_pass(p_mkdir(FAKE_GLOBALCONFIG_NAME, 0777));
	cl_git_pass(git_fs_path_prettify(&path, FAKE_GLOBALCONFIG_NAME, NULL));
	cl_git_pass(git_libgit2_set_search_path(
	        GIT_CONFIG_LEVEL_GLOBAL, path.ptr));

	if (out)
		git_str_swap(out, &path);

	git_str_dispose(&path);
}

void cl_sandbox_set_homedir(const char *home)
{
	git_str path = GIT_STR_INIT;

	if (home) {
		git_libgit2_set_homedir(home);
	} else {
		git_str_joinpath(&path, clar_tempdir_path(), "__home");

		if (!git_fs_path_exists(path.ptr))
			cl_must_pass(p_mkdir(path.ptr, 0777));

		git_libgit2_set_homedir(path.ptr);
	}

	git_str_dispose(&path);
}

void cl_sandbox_set_search_path_defaults(void)
{
	git_str path = GIT_STR_INIT;

	git_str_joinpath(&path, clar_tempdir_path(), "__config");

	if (!git_fs_path_exists(path.ptr))
		cl_must_pass(p_mkdir(path.ptr, 0777));

	git_libgit2_set_search_path(GIT_CONFIG_LEVEL_GLOBAL, path.ptr);
	git_libgit2_set_search_path(GIT_CONFIG_LEVEL_XDG, path.ptr);
	git_libgit2_set_search_path(GIT_CONFIG_LEVEL_SYSTEM, path.ptr);
	git_libgit2_set_search_path(GIT_CONFIG_LEVEL_PROGRAMDATA, path.ptr);

	git_str_dispose(&path);
}

void cl_sandbox_disable_ownership_validation(void)
{
	git_libgit2_set_owner_validation(0);
}
