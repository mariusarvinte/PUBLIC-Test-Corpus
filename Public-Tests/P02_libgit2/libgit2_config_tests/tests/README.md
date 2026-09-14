# libgit2 tests

Using commit: d83036a20f966461852ce4b768ce71ac837498cc from clar.

These are the unit and integration tests for the libgit2 projects.

* `benchmarks`
  These are benchmark tests that excercise the CLI.
* `clar`  
  This is [clar](https://github.com/clar-test/clar) the common test framework.
* `headertest`  
  This is a simple project that ensures that our public headers are
  compatible with extremely strict compilation options.
* `libgit2`  
  These tests exercise the core git functionality in libgit2 itself.
* `resources`  
  These are the resources for the tests, including files and git
  repositories.
* `util`  
  These are tests of the common utility library.

## Writing tests for libgit2

libgit2 uses the [clar test framework](http://github.com/clar-test/clar), a
C testing framework.

The best resources for learning clar are [clar itself](https://github.com/clar-test/clar)
and the existing tests within libgit2.  In general:

* If you place a `.c` file into a test directory, it is eligible to contain
test cases.
* The function name for your test is important; test function names begin
  with `test_`, followed by the folder path (underscore separated), two
  underscores as a delimiter, then the test name.  For example, a file
  `merge/analysis.c` may contain a test `uptodate`:

  ```
  void test_merge_analysis__uptodate(void)
  {
    ...
  }
  ```

* You can run an individual test by passing `-s` to the test runner.  Tests
  are referred to by their function names; for example, the function
  `test_merge_analysis__uptodate` is referred to as `merge::analysis::uptodate`.
  To run only that function you can use the `-s` option on the test runner:

  ```
  libgit2_tests -smerge::analysis::uptodate
  ```
