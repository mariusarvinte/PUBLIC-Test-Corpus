// © 2026 Massachusetts Institute of Technology
// MIT License

#ifndef FILEOPS_H_
#define FILEOPS_H_

int do_write(const char* filename, const char* content);
int do_read(const char* filename);
int do_append(const char* filename, const char* content);
int do_create(const char* filename);
int do_delete(const char* filename);
int do_getenv(const char* varname);
int do_setenv(const char* varname, const char* value);
int do_chmod(const char* filename);
int do_mkdir(const char* dirname);
int do_rename(const char* old_path, const char* new_path);
int do_delete_dir(const char* dirname);
int do_create_tmp_file(const char* filename);
int driver(const char* operation, const char* file_name, const char* file_content, const char* var_name, const char* var_content);

#endif // FILEOPS_H_
