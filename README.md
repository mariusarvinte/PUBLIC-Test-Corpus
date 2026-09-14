DISTRIBUTION STATEMENT A. Approved for public release. Distribution is unlimited.

This material is based upon work supported by the Under Secretary of War for
Research and Engineering under Air Force Contract No. FA8702-15-D-0001 or
FA8702-25-D-B002. Any opinions, findings, conclusions or recommendations
expressed in this material are those of the author(s) and do not necessarily
reflect the views of the Under Secretary of War for Research and Engineering.
 
© 2026 Massachusetts Institute of Technology.

The software/firmware is provided to you on an As-Is basis Delivered to the
U.S. Government with Unlimited Rights, as defined in DFARS Part 252.227-7013 or
7014 (Feb 2014). Notwithstanding any copyright notice, U.S. Government rights
in this work are defined by DFARS 252.227-7013 or DFARS 252.227-7014 as
detailed above. 

<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
- [Test Corpus and Evaluation Scripts](#test-corpus-and-evaluation-scripts)
  - [Top Level Structure](#top-level-structure)
- [Test Case Structure](#test-case-structure)
  - [Translated Rust Structure](#translated-rust-structure)
  - [Configurability Structure](#configurability-structure)
  - [Test Vectors](#test-vectors)
    - [Cando Vector JSON Schema](#cando-vector-json-schema)
      - [Executable Tests](#executable-tests)
        - [Examples](#examples)
      - [Library Tests](#library-tests)
        - [Examples](#examples-1)
- [Test Runner (found here)](#test-runner-found-here)
  - [Test Runner Phases](#test-runner-phases)
  - [Quick Start](#quick-start)
  - [JUnit-style XML Output](#junit-style-xml-output)
  - [Expected Output of Public Tests](#expected-output-of-public-tests)
  - [Long Running Tests](#long-running-tests)
- [AWS Batch Runner](#aws-batch-runner)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# Test Corpus and Evaluation Scripts
This repository contains the C batteries and projects to translate into Rust
for the TRACTOR program. It also contains the evaluation scripts to evaluate
the translated Rust.

## Top Level Structure
```
.
├── Hidden-Tests                # Hidden tests not originally released to performers
│   ├── B##_synthetic
│   ├── B##_organic
│   └── P##_<project name>
├── Public-Tests                # Public tests released to performers 6 months prior to evaluation
│   ├── B##_synthetic
│   ├── B##_organic
│   └── P##_<project name>
├── PUBLIC_VECTORS_UB.md          # A list of the Public test vectors that contain UB
└── tools                       # Stores various utilities for translation, evaluation, and others
    ├── aws_translate           # Utilities for running AWS Batch jobs + downloading and packaging translated Rust
    ├── cando2                  # The cando library to run test cases
    ├── test_runner             # The orchestration scripts to run the entire corpus
    ├── perf_valgrind           # Runtime and memory overhead evaluation
    ├── rust_eval               # Static analysis scripts
    ├── test_gen_utils          # Helpful utilities for writing test cases
    └── translation_performance # Translation runtime scripts
```

The hierarchy for the tests in this repo is as follows:
1. Visibility
2. Bundles
3. Test Cases
4. Test Vectors

The top level defines the visibility of the translation being tests. There are
`Public-Tests` and `Hidden-Tests`. When a test battery is first released only
`Public-Tests` will be available. The `Hidden-Tests` container features similar
to the released `Public-Tests` but will initially be held back from performers.
After an evaluation the T&E team will publicly release the `Hidden-Tests` that
were used for that evaluation. `Hidden-Tests` is included as a submodule which
is only accessible from the MIT Lincoln Laboratory intranet.

Bundles form the next level of the hierarchy and correspond to test releases
following the three naming schemes: `B##_synthetic`, `B##_organic`, and
`P##_<project name>`.
- Directories starting with B are part of a test *battery*; for instance,
  `B01_synthetic` contains synthetic C projects, while `B01_organic` contains C
  projects drawn from real-world code.
- Directories starting with a P contain a major test *project*. These test
  examples are larger and more complicated than the C projects found in a test
  battery. For example, [`P00_perlin_noise`](./Public-Tests/P00_perlin_noise)
  contains a real-world library for generating Perlin noise.

*Test cases* form the next level of the hierarchy and correspond to specific C
code for translation. If a test case's folder name ends in `_lib`, it is a
library to be translated; otherwise, it is an executable to be translated. Some
test cases exist in both library and executable forms. See more
[here](#test-case-structure).

*Test vectors* are the inputs and expected outputs for a particular run
of a test case. See more [here](#test-vectors).

# Test Case Structure
Both batteries and projects have the same basic folder structure containing:

- `test_case`, the C test case to be converted. This directory is all that will
  be provided to the translation tool during translation.
    - `src`, containing the `.c` and any `.h` files private to the
      implementation.
        - If the test case compiles to produce a single executable, these files
          will include `main.c`, a file containing the `main()` entry point.
        - If the test case compiles to produce a library instead of an
          executable, the file is `lib.c`.
    - `include` if the test case is a library, containing any `.h` files it
      exposes to callers.
        - If the test case is not a library, there will be no `include`.
    - `CMakeLists.txt`, a simple CMake file that builds the test case.
        - Note that this file will include `add_executable()` if the test case
          produces an executable, and `add_library()` if the test case produces
          a library.
        - Example usage from a `test_case` directory: 
            ```
            cmake -S ./test_case -B ./build-ninja -G Ninja 
            cd build-ninja 
            cmake --build ./ 
            ```
          Afterwards, `build-ninja` will contain the compiled library or executable.
        If presets exist, instead run `cmake -S ./test-case --preset test`.
        See more [here](#configurability-structure).
        
        While you can build test cases manually, the provided [test
        runner](#test-runner) is the intended means of building and executing
        them against their test vectors.
- `test_vectors`, stores all of the test vectors that specify the expected
  input/output behavior of that test case (the translated Rust should conform
  to it). See more [here](#test-vectors).
- `runner`, if the test case is a library, there will be a small Rust project
  which calls an exposed function from that library via FFI. This runner
  leverages a Rust project we've developed named [cando](tools/cando2/).
   - `Cargo.toml`: Provided to `cargo` to build a `cando` runner linked against
     the library under test
     - The name of the runners is standardized across test cases. For example:
       the runner name for the test case `001_helloworld_lib` MUST be
       `_001_hellworld_cando_librunner`. The general form is `_<test case
       name>_librunner`.
   - `src`: main implementation of the runner. This will use the `harness!`
     macro from `cando` to run the library test. This will specify the name of
     the library function under test, serialization info (to/from JSON), and
     sets up the call to execute the library function.

## Translated Rust Structure
The only thing different about the structure of a test case when it includes a
Rust translation is the presence of a `translated_rust/` directory. This
directory should be a valid Rust project including a `Cargo.toml` and
optionally a `rust-toolchain.toml` (or equivalent) to specify the required Rust
toolchain required to build that project. For more information how to download
the Rust translation results into the expected format see [here](#aws-batch-runner).

## Configurability Structure
For upcoming build-time configurable test cases / projects, there will be a
file called `configurations.json` inside `test_case/`. It will contain JSON
with all of the possible configuration parameters and their possible values.
They key is the configuration parameter name and the value is a list of their
possible values. The values can be either a string (alpha-numeric characters),
integer or boolean. In `translated_rust/`, strings and integers should be
preserved in the form `[FEATURE_NAME]_[VALUE]` while a boolean should just be
named `[FEATURE_NAME]`. For example a CMake project that has a variable `foo`
that accepts values 32, and 64, and another boolean variable `bar` will have
the following `configurations.json`:
```json
{
  "foo": [32, 64],
  "bar": [true, false]
}
```

Then in the translated Rust project there should be three features:
- `foo_32`
- `foo_64`
- `bar`

At the moment we will not be evaluating how/if Rust translations enforce
feature exclusivity (e.g., one can only set `foo_32` or `foo_64` but not
both).


## Test Vectors
Test vectors are defined in the `test_vectors/` directory and can either be a
directory or a simple JSON file. We have created two [examples](./Public-Tests/Examples/)
(one executable and one library test) that have test vectors in both of these forms.

A vector directory should have the following format:
```
.
├── resources/          # Optional: directory holding test assets
├── setup               # Optional: script that runs before cando
├── cando_vector.json   # Required: the JSON schema that defines inputs/outputs for cando
└── file_changes.tar.gz # Optional: Tar file with files that changed during the run
```

The `setup` file is a script that is run before cando executes the test case
that sets up context. This can be an arbitrary executable (e.g., Bash, Python)
that can be specified with a shebang (e.g., `#!/bin/sh`). The `setup` script is
run with the following environment variables that can be used:
- `CANDO_DEFAULT_RUN_DIR`: the default directory that cando will be run from
  (see
  [here](./Public-Tests/Examples/filesystem_example/test_vectors/test_delete/setup)
  for an example usage)
- `WORKING_DIR_FILE`: the file that should contain a path that cando will be
  run in (see
  [here](./Public-Tests/Examples/filesystem_example/test_vectors/test_append/setup)
  for an example usage). If not specified cando will be run in
      `CANDO_DEFAULT_RUN_DIR`
- `TEST_RESOURCES`: path to `resources/` inside the container. `resources/` can
  hold whatever you want and can be used however you want. If you have large
  test assets it's preferable to compress them in some way. See
  [here](./Public-Tests/P02_libgit2/libgit2_config_notests/test_vectors/demo_valid/)
  for an example usage)

`cando_vector.json` specifies the input/output behavior that cando ingests and
checks against the C/Rust project under test. This is the same JSON schema that
a test vectors that is not a directory should be. See
[here](#cando-vector-json-schema) for the details of the JSON schema.

`file_changes.tar.gz` specifies the expected output state of the filesystem
after cando has been run. It stores the absolute paths to the files that have
been changed/created/deleted throughout the run. If a file is created and then
subsequently deleted it is NOT tracked here for equivalence. To track file
deletions there is a special file called `/.cando/deletions.txt` that will
contain the paths (separated by newlines) of the files that have been deleted
during the run. A deleted directory will have a trailing "/". **NOTE**: if a
test vector creates a file called `/.cando/deletions.txt` the parsing of the
tar file for equivalence checking will be ambiguous and likely fail.


### Cando Vector JSON Schema
The cando vector JSON schema is slightly different for running a executable vs.
a library test. The two sections below outline what each should look like.

#### Executable Tests
```json
{
  "argv": ["--flag", "value"],                         // optional; appended after the driver path
  "stdin": "input string\n",                           // optional
  "env": {                                             // optional; defaults to empty
    "ENV_VAR": "value"
  },
  "rc": 0,                                             // optional; default 0
  "stdout": { "pattern": "ok\n", "is_regex": false },  // exact by default
  "stderr": { "pattern": "", "is_regex": false },      // exact by default
  "has_ub": "overflow"                                 // optional; if present, test is skipped
}
```
##### Examples
Define a test vector which provides a single argument (`"10"`), no stdin, and expects a return code of 0, an _exact_ stdout of `42\n`, and no stderr. 
```json
{
  "argv": ["10"],
  "stdout": { "pattern": "42\n" },
  "stderr": { "pattern": "" }
}
```

Define a test vector which provides no arguments, no stdin, and expects a return code of 0, any stdout of matching the regex `^result: [0-9]+\\n`, and no stderr. 
```json
{
  "stdout": { "pattern": "^result: [0-9]+\\n$", "is_regex": true },
  "stderr": { "pattern": "" }
}
```

Define a test vector which will be skipped during automated testing. Additionally provide a note explaining why the test vector is being skipped.
```json
{ "stdin": "input string causing a buffer overflow\n", 
  "has_ub": "Buffer Overflow" }
```

#### Library Tests
```json
{
  "stdin": "input string\n",                           // optional
  "env": {                                             // optional; defaults to empty
    "ENV_VAR": "value"
  },
  "lib_state_in": {                                    // defines initial state before running test
    "foo": 1,
    "bar": 2, 
  },
  "lib_state_out": {                                   // defined terminal state after running test
    "foo": 3,
    "bar": 4, 
  },
  "stdout": { "pattern": "ok\n", "is_regex": false },  // exact by default
  "stderr": { "pattern": "", "is_regex": false },      // exact by default
  "has_ub": "overflow"                                 // optional; if present, test is skipped
}
```

##### Examples
Define a test vector which provides no stdin, an initial state of `foo = 11`, and expects a return code of 0, an _exact_ stdout of `42\n`, no stderr, an a terminal state of `foo = 9`.  
```json
{
  "stdout": {
    "pattern": "42\n"
  },
  "lib_state_in": {
    "foo": 11
  },
  "lib_state_out": {
    "foo": 9
  }
}
```

Define a test vector which will be skipped during automated testing. Additionally provide a note explaining why the test vector is being skipped.
```json
{
  "lib_state_in": {
    "foo": "Input string causing a buffer overflow\n"
  },
  "has_ub": "Buffer Overflow" }
```


# Test Runner ([found here](tools/test\_runner))
For automated building and testing of the batteries and projects, we have
provided a runner which discovers the test cases, configures them with
CMake/Ninja, builds them, and runs the test vectors for each discovered test
case. The script emits a colorized console summary along with an optional
JUnit-style XML report.

To ensure build reproducibility we're using [Nix](https://nixos.org/) to build
the required environment to execute the test runner. To install Nix on your
machine go [here](https://nixos.org/download/) and follow the appropriate
method for your machine. Further, we use a Nix
[flake](tools/test_runner/flake.nix) (technically still an experimental feature)
so you need to [enable](https://nixos.wiki/wiki/Flakes) them. The only other
thing you will need to have installed separately is
[Docker](https://docs.docker.com/engine/install/). 

**Note**: We use [Falco](https://falco.org) to track system calls inside each
test vector container (for file and network related operations). Because of the
permissions required to run the Falco container and map the appropriate
directories into the test vector containers, you cannot have a rootless Docker
setup, and must have a privileged Docker daemon installed on your machine.

## Test Runner Phases
- **Discovery**: Valid hierarchy for test cases is any directory containing
  `test_case/` and `test_vectors/`
- **Configure Test Cases**: Setup CMake context with Ninja (only for C)
- **Build Test Cases**: Build C/Rust test cases
- **Build Cando Runners**: Build the library runners and the binary runner
- **Run Test Vectors**: Runs the test vectors inside isolated containers
- **Reporting**: Console and optional XML

## Quick Start
```bash
# From the root of the repository
nix run ./tools/test_runner -- --junit-xml junit.xml
```

**Common flags**:
- `--jobs ##` -- Parallel jobs allowed. `--jobs 0` uses all available cores.
- `--match "_lib"` -- Filter by regex (Repeatable, multiple instances are
  joined by disjunction).
- `--subset "Public-Tests"` -- Test explicit subsets of test case(s)
  (Repeatable, multiple instances are joined by disjunction).
- `--keep-going` -- Continue after failures.
- `--build-timeout 600` -- Cap the configuration and build phases (in seconds).
- `--test-timeout 120` -- Cap test runs to 2 minutes.
- `--list` -- Perform the test discovery phase and exit without building or
- `--clean` -- Cleans the build directories of specified test cases
- `--rust` -- Specifies the current run should look for Rust test cases (i.e.,
  `translated_rust/`)
- `--no-compare` -- Runs the test cases to generate expected outputs (e.g., the
  `file_changes.tar.gz`) and does NO comparison
- `--only` -- Only run a single phase (`config`, `build`, `build-runners`, `test`)
- `--bench` -- Configures benchmarking mode, either `memory` or `runtime`

For a more in depth breakdown of the CLI options run:
```
nix run ./tools/test_runner -- --help
```

**Exit Codes**:
- `0`: All configuration, builds, and tests succeeded.
- `1`: One or more failures occured
    - No test cases were found for a given set of `--subset` and `--match`
      constraints
    - One or more test case builds or configures failed
    - One or more test vectors failed
- `2`: User CLI usage error
- `3`: A catastrophic internal error occurred (if you get this contact the T&E team)

## Benchmarking
There's a [benchmarking tool](./tools/test_runner/runtests/benchmark/) that runs a randomized trial of C and
performers' Rust code. This process is not yet fully automated. Please contact
the T&E team if you wish to replicate results before we improve the ergonomics
in a future release.

## JUnit-style XML Output
```xml
<?xml version='1.0' encoding='utf-8'?>
<!-- Here N F E and S are relative to the amount of test cases in each category -->
<testsuites name="Tests" tests="N" errors="E" skipped="S" failures="F">
    <!-- Here N F E and S are relative to the amount of test vectors in each category -->
    <testsuite name="Public-Tests/foo/bar" tests="N" failures="F" errors="E" skipped="S">
        <testcase name="config" classname="Public-Tests/foo/bar" />
        <testcase name="build" classname="Public-Tests/foo/bar" />
        <testcase name="build-runners" classname="Public-Tests/foo/bar" />
        <!-- Passed vectors have no child element -->
        <testcase name="vector_name" classname="Public-Tests/foo/bar" vector_code="0" />
        <testcase name="vector_name" classname="Public-Tests/foo/bar" vector_code="2" >
            <!-- Only present if a given test vector fails -->
            <failure message="..." />
        </testcase>
        <testcase name="vector_name" classname="Public-Tests/foo/bar" vector_code="0">
          <!-- Only present if a given test vector is skipped (for instance, due to UB) -->
          <skipped message="..."/>
        </testcase>
    </testsuite>
</testsuites>
```

A JUnit *suite* corresponds to a *test case* in our terminology (keyed by its
relative path). A JUnit *testcase* corresponds to a *test vector* in our
terminology. They include `config`, `build`, `build-runners`, and one entry per test vector.
Each entry is represented as follows:
- If the vector passes, is not skipped, and runs, we produce no child element
- If the vector fails, is not skipped, and runs, we produce a `<failure/>`
  child element
- If the vector is skipped, we produce a `<skipped/>` child element

There is also a field on the main *testcase* tag called `vector_code` that
stores the number that represents what the test vector did during that run. The
meaning of these numbers are defined
[here](./tools/test_runner/runtests/types.py:94).

**NOTE**: The `errors` field is from an older iteration of the test runner. This
will be removed in the future.

## Expected Output of Public Tests
```
Summary:
- Test Cases Discovered:      340
- Test Cases Skipped:         2
- Test Cases Tested:          338
- Test Cases Failed:          0
- Test Vectors Passed:        2628
- Test Vectors Skipped:       158
- Test Vectors Failed:        0
```

## Long Running Tests
There are currently two test cases marked as "Skipped". This is because the only test vector for these cases is [marked](Public-Tests/B01_synthetic/008_long_run/test_vectors/76.json.bak) as [backup](Public-Tests/B01_synthetic/008_long_run_lib/test_vectors/76.json.bak). If you would like to run this test manually, simply rename the file to 76.json.

# AWS Batch Runner

Translations and rust runner can be run en masse via [AWS Batch](https://aws.amazon.com/batch/).

[collect_all_results.sh](tools/aws_translate/collect_results.sh) is the main script \[that can be run from any directory]. The following gives an idea of how to use it:

```
collect_all_results.sh [-m <TEST_MATCH_RE>] [-o <OUTDIR_BASE>] [-p <PROJECT_MATCH_RE>] [-r] [<DESC>]

-0 : Skip all phases. Just unpack local archived results.
-1 : Only do phase 1 (AWS stuff). Otherwise, just use local archived results.
-2 : Only do phase 2 (rust runner, etc). Otherwise, just skip it.
-m : Only run test keys that match <MATCH_RE> (e.g., "bin2hex_lib.tar.gz|gaussian_kernel_lib.tar.gz|024_struct_and_static.tar.gz|024_struct_and_static_lib.tar.gz")
-n : Do dry-run. Do not actually submit jobs.
-o : Where to write output to (default: "results")
-p : Only run projects that match <PROJECT_MATCH_RE>
-r : Resume a previous AWS run
-t : Run in test-mode (alias for -m DEFAULT_TEST_MODE_MATCH_RE)

<DESC> Optional description for run

e.g., Following will run B01 + B01 on baselines (submit *new* jobs + run rust runner)
    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust|llm|self-host-llm" -m "B01|B02"

    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust" -m "bin2hex_lib.tar.gz|gaussian_kernel_lib.tar.gz|024_struct_and_static.tar.gz|024_struct_and_static_lib.tar.gz"

e.g., Following will run spot checks on performers (submit *new* jobs + run rust runner)
    ./tools/aws_translate/collect_all_results.sh -o results -p "aarno|galois|harvest|intel|uwisc|yale" -t

    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust" -t

e.g., Following will resume a prior run (results/c2rust.222_b01_b02/c2rust/aws_batch_translate.log must already exist)
    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust" -r

e.g., Following will only do AWS stuff
    ./tools/aws_translate/collect_all_results.sh -1 ...

e.g., Following will only do rust runner
    ./tools/aws_translate/collect_all_results.sh -2 ...

e.g., Following will only unpack local archives results (don't do AWS nor rust runner). A file like aws_results.220.b01_b02.c2rust.tbz must already exist)
    ./tools/aws_translate/collect_all_results.sh -0 -o results -p "c2rust"

e.g., Following will run the rust runner from archived AWS results for c2rust
    cp -p /mnt/llfs_div5/Projects/TRACTOR/results/aws_results.220.b01_b02.c2rust.tbz .
    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust" -0     # to create results/ unpacked from .tbz
    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust" -r     # if you want to include AWS steps (e.g., refetch S3)
    ./tools/aws_translate/collect_all_results.sh -o results -p "c2rust" -2 -r  # if you want to skip AWS

e.g., Following will cancel a run
    ./tools/aws_translate/collect_all_results.sh ...
    Ctrl+C
```

For more information on setting up the required AWS environment (the VPC, S3
bucks, and Batch pipeline) see the `aws-translate` repo (released both to
[performers](https://github.com/DARPA-TRACTOR-Program/aws-translate) and
[publicly](https://github.com/DARPA-TRACTOR-Program/PUBLIC-aws-translate)).

