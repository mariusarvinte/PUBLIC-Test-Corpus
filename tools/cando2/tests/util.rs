// © 2026 Massachusetts Institute of Technology
// MIT License

#![allow(unused)]

use cando2::{
    runner::{CommonOutput, RunAllTestResult, TestOutcome},
    *,
};
use std::{fmt::Debug, fs, mem::discriminant, process::Command, sync::Once};

static INIT: Once = Once::new();

/// Initializes the environment for running tests.
///
/// Sets the `CANDO_TESTS` environment variable because this allows cando to detect if tests are
/// being run (because we need to run them as integration tests).
/// This also ensures the C and Rust projects are built.
///
/// This must be called before every test case because of the multi-threaded run of cargo tests,
/// but this will only actually get run once
pub fn init() {
    TEST_ROOT_DIR.get_or_init(|| None);

    INIT.call_once(|| {
        unsafe {
            env::set_var("CANDO_TESTS", "1");
        }

        let mock_candidate_root = Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("tests")
            .join("test_data")
            .join("mock_candidate_lib");

        // Build C
        let _ = fs::create_dir(mock_candidate_root.join("build-ninja"));
        Command::new("./make.sh")
            .current_dir(mock_candidate_root.join("test_case"))
            .spawn()
            .expect("Couldn't build C test case")
            .wait()
            .expect("Couldn't build C test case");

        // Build Rust
        let rust_buil = Command::new("cargo")
            .arg("build")
            .arg("--release")
            .arg("--quiet")
            .current_dir(mock_candidate_root.join("translated_rust"))
            .spawn()
            .expect("Couldn't build Rust test case")
            .wait()
            .expect("Couldn't build Rust test case");
    })
}

pub fn error_as_rc<S: RunnableState<E> + PartialEq + Clone, E: Clone + Serialize + Debug>(
    ret: &Result<RunAllTestResult<S, E>, CandoError>,
) -> i32 {
    match ret {
        Ok(res) => {
            if res.any_bench_failed {
                println!("{}", CandoError::AnyBenchFailed);
                CandoError::AnyBenchFailed.exit_code()
            } else if res.any_vector_failed {
                println!("{}", CandoError::AnyVectorFailed);
                1
            } else {
                0
            }
        }
        Err(e) => {
            println!("{}", e);
            e.exit_code()
        }
    }
}

pub fn assert_result<S: RunnableState<E> + PartialEq + Clone, E: Clone + Serialize + Debug>(
    got_res: Result<RunAllTestResult<S, E>, CandoError>,
    vector_name: Option<&str>,
    expected: GenericRunResult,
) {
    let v_name = vector_name.map(|s| s.to_string());
    let result = got_res.unwrap();
    let got = result.report.get(&v_name).unwrap();
    let expected = expected.to_run_result();
    assert_eq!(discriminant(got), discriminant(&expected));
}

/// `RunResult` but without any of the interior fields that we don't care about for test comparison
/// sake
pub enum GenericRunResult {
    Skip,
    NoCompare,
    Pass,
    VectorComparisonFailed,
    Panic,
    SegmentationFault,
    Timeout,
    BenchFail,
    UnknownFailure,
}

impl GenericRunResult {
    /// Converts the generic run result to a concrete run result with default values because we
    /// don't care about the actual values for a comparison just that the enum variants match
    pub fn to_run_result<S: RunnableState<E> + PartialEq + Clone, E: Clone + Serialize + Debug>(
        self,
    ) -> TestOutcome<S, E> {
        let dummy_out = CommonOutput {
            stdout: "".into(),
            stderr: "".into(),
        };

        match self {
            GenericRunResult::Skip => TestOutcome::Skip,
            GenericRunResult::NoCompare => TestOutcome::NoCompare(None),
            GenericRunResult::Pass => TestOutcome::Pass(dummy_out),
            GenericRunResult::VectorComparisonFailed => TestOutcome::VectorComparisonFailed {
                diff: "".into(),
                common_out: dummy_out,
            },
            GenericRunResult::Panic => TestOutcome::Panic(dummy_out),
            GenericRunResult::SegmentationFault => TestOutcome::SegmentationFault(dummy_out),
            GenericRunResult::Timeout => TestOutcome::Timeout(dummy_out),
            GenericRunResult::BenchFail => TestOutcome::BenchFail("".into()),
            GenericRunResult::UnknownFailure => TestOutcome::UnknownFailure {
                wait_status: 0,
                common_out: dummy_out,
            },
        }
    }
}
