use std::{env, path::PathBuf};

use glob::glob;

fn main() -> Result<(), std::io::Error> {
    println!("cargo:rerun-if-changed=../../../common");

    let files = glob("../../../common/*.fbs")
        .unwrap()
        .into_iter()
        .filter_map(|e| e.ok())
        .collect::<Vec<_>>();
    let files = files.iter().map(|p| p.as_path()).collect::<Vec<_>>();

    let out_dir = PathBuf::from(env::var("OUT_DIR").unwrap());

    flatc_rust::run(flatc_rust::Args {
        inputs: &files,
        out_dir: &out_dir,
        extra: &["--rust-module-root-file"],
        ..Default::default()
    })
}
