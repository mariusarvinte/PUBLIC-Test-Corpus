TRACTOR implementation of libgit2 - the Git linkable library
==================================

This was taken from [here](https://github.com/libgit2/libgit2).

`libgit2` is a portable, pure C implementation of the Git core methods
provided as a linkable library with a solid API, allowing to build Git
functionality into your application.

`libgit2` is used in a variety of places, from GUI clients to hosting
providers ("forges") and countless utilities and applications in
between. 

`libgit2` is licensed under a **very permissive license** (GPLv2 with
a special Linking Exception). This means that you can link against
the library with any kind of software without making that software
fall under the GPL. Changes to libgit2 would still be covered under
its GPL license.


Quick Start (NEED TO UPDATE)
===========

**Prerequisites** for building libgit2:

1. [CMake](https://cmake.org/), and is recommended to be installed into
   your `PATH`.
2. [Python](https://www.python.org) is used by our test framework, and
   should be installed into your `PATH`.
3. C compiler: libgit2 is C90 and should compile on most compilers.
   * Windows: Visual Studio is recommended
   * Mac: Xcode is recommended
   * Unix: gcc or clang is recommended.

**Build**

1. Create a build directory beneath the libgit2 source directory,
   and change into it: `mkdir build && cd build`
2. Create the cmake build environment: `cmake ..`
3. Build libgit2: `cmake --build .`

Trouble with these steps?  Read our [troubleshooting guide](docs/troubleshooting.md).
More detailed build guidance is available below.


What It Can Do (UPDATE)
==============

libgit2 provides you with the ability to manage Git repositories in the
programming language of your choice.  It's used in production to power many
applications including GitHub.com, Plastic SCM and Azure DevOps.

It does not aim to replace the git tool or its user-facing commands. Some
APIs resemble the plumbing commands as those align closely with the
concepts of the Git system, but most commands a user would type are out
of scope for this library to implement directly.

The library provides:

* SHA conversions, formatting and shortening
* abstracted ODB backend system
* commit, tag, tree and blob parsing, editing, and write-back
* tree traversal
* revision walking
* index file (staging area) manipulation
* reference management (including packed references)
* config file management
* high level repository management
* thread safety and reentrancy
* descriptive and detailed error messages
* ...and more (over 175 different API calls)

As libgit2 is purely a consumer of the Git system, we have to
adjust to changes made upstream. This has two major consequences:

* Some changes may require us to change provided interfaces. While
  we try to implement functions in a generic way so that no future
  changes are required, we cannot promise a completely stable API.
* As we have to keep up with changes in behavior made upstream, we
  may lag behind in some areas. We usually to document these
  incompatibilities in our issue tracker with the label "git change".

Optional dependencies (UPDATE)
=====================

**NOTE: This will need to be deleted once we've stripped all or most of this**

While the library provides git functionality with very few
dependencies, some recommended dependencies are used for performance
or complete functionality.

- Hash generation: Git uses SHA1DC (collision detecting SHA1) for
  its default hash generation. SHA256 support is experimental, and
  optimized support is provided by system libraries on macOS and
  Windows, or by the HTTPS library on Unix systems when available.
- Threading: is provided by the system libraries on Windows, and
  pthreads on Unix systems.
- HTTPS: is provided by the system libraries on macOS and Windows,
  or by OpenSSL or mbedTLS on other Unix systems.
- SSH: is provided by [libssh2](https://libssh2.org/) or by invoking
  [OpenSSH](https://www.openssh.com).
- Unicode: is provided by the system libraries on Windows and macOS.

Initialization (UPDATE)
===============

**NOTE: Not sure if we should keep this here? Could be useful for LLMs**

The library needs to keep track of some global state. Call

    git_libgit2_init();

before calling any other libgit2 functions. You can call this function many times. A matching number of calls to

    git_libgit2_shutdown();

will free the resources.  Note that if you have worker threads, you should
call `git_libgit2_shutdown` *after* those threads have exited.  If you
require assistance coordinating this, simply have the worker threads call
`git_libgit2_init` at startup and `git_libgit2_shutdown` at shutdown.

Conventions (UPDATE)
===========

**NOTE: Not sure if we should keep this here? Could be useful for LLMs**

See [conventions](docs/conventions.md) for an overview of the external
and internal API/coding conventions we use.

Building libgit2 - Using CMake (UPDATE)
==============================

Building (UPDATE)
--------

`libgit2` builds cleanly on most platforms without any external
dependencies as a requirement. `libgit2` is built using
[CMake](<https://cmake.org/>) (version 2.8 or newer) on all platforms.

On most systems you can build the library using the following commands

	$ mkdir build && cd build
	$ cmake ..
	$ cmake --build .

To include the examples in the build, use `cmake -DBUILD_EXAMPLES=ON ..`
instead of `cmake ..`. The built executable for the examples can then
be found in `build/examples`, relative to the toplevel directory.

Alternatively you can point the CMake GUI tool to the CMakeLists.txt file and generate platform specific build project or IDE workspace.

If you're not familiar with CMake, [a more detailed explanation](https://preshing.com/20170511/how-to-build-a-cmake-based-project/) may be helpful.


**NOTE: Get rid of this once we've stripped**
Dependency options:

* `USE_SSH=type`: enables SSH support and optionally selects the provider;
  `type` can be set to `libssh2` or `exec` (which will execute an external
  OpenSSH command). `ON` implies `libssh2`; defaults to `OFF`.
* `USE_HTTPS=type`: enables HTTPS support and optionally selects the
  provider; `type` can be set to `OpenSSL`, `OpenSSL-Dynamic` (to not
  link against OpenSSL, but load it dynamically), `SecureTransport`,
  `Schannel` or `WinHTTP`; the default is `SecureTransport` on macOS,
  `WinHTTP` on Windows, and whichever of `OpenSSL` or `mbedTLS` is
  detected on other platforms. Defaults to `ON`.
* `USE_SHA1=type`: selects the SHA1 mechanism to use; `type` can be set
  to `CollisionDetection`, `HTTPS` to use the system or HTTPS provider,
  or one of `OpenSSL`, `OpenSSL-Dynamic`, `OpenSSL-FIPS` (to use FIPS
  compliant routines in OpenSSL), `CommonCrypto`, or `Schannel`.
  Defaults to `CollisionDetection`. This option is retained for
  backward compatibility and should not be changed.
* `USE_SHA256=type`: selects the SHA256 mechanism to use; `type` can be
  set to `HTTPS` to use the system or HTTPS driver, `builtin`, or one of
  `OpenSSL`, `OpenSSL-Dynamic`, `OpenSSL-FIPS` (to use FIPS compliant
  routines in OpenSSL), `CommonCrypto`, or `Schannel`. Defaults to `HTTPS`.
* `USE_GSSAPI=<on/off>`: enables GSSAPI for SPNEGO authentication on
  Unix. Defaults to `OFF`.
* `USE_HTTP_PARSER=type`: selects the HTTP Parser; either `http-parser`
  for an external
  [`http-parser`](https://github.com/nodejs/http-parser) dependency,
  `llhttp` for an external [`llhttp`](https://github.com/nodejs/llhttp)
  dependency, or `builtin`. Defaults to `builtin`.
* `REGEX_BACKEND=type`: selects the regular expression backend to use;
  one of `regcomp_l`, `pcre2`, `pcre`, `regcomp`, or `builtin`. The
  default is to use `regcomp_l` where available, PCRE if found, otherwise,
  to use the builtin.
* `USE_BUNDLED_ZLIB=type`: selects the bundled zlib; either `ON` or `OFF`.
  Defaults to using the system zlib if available, falling back to the
  bundled zlib.



Running Tests (UPDATE)
-------------
**NOTE: Not sure how this will look but we might be reusing a lot of this**

Once built, you can run the tests from the `build` directory with the command

	$ ctest -V

Alternatively you can run the test suite directly using,

	$ ./libgit2_tests

Invoking the test suite directly is useful because it allows you to execute
individual tests, or groups of tests using the `-s` flag.  For example, to
run the index tests:

	$ ./libgit2_tests -sindex

To run a single test named `index::racy::diff`, which corresponds to
the test function
[`test_index_racy__diff`](https://github.com/libgit2/libgit2/blob/main/tests/libgit2/index/racy.c#L22):

	$ ./libgit2_tests -sindex::racy::diff

The test suite will print a `.` for every passing test, and an `F` for any
failing test.  An `S` indicates that a test was skipped because it is not
applicable to your platform or is particularly expensive.

**Note:** There should be _no_ failing tests when you build an unmodified
source tree from a [release](https://github.com/libgit2/libgit2/releases),
or from the [main branch](https://github.com/libgit2/libgit2/tree/main).
Please contact us or
[open an issue](https://github.com/libgit2/libgit2/issues)
if you see test failures.
