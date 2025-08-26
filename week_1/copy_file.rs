use std::{env, fs, process};

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() != 3 {
        eprintln!("Usage: {} <source> <destination>", args[0]);
        process::exit(2);
    }
    if let Err(e) = fs::copy(&args[1], &args[2]) {
        eprintln!("Copy failed: {e}");
        process::exit(1);
    }
}
