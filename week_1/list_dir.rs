use std::fs;
use std::error::Error;

fn main() {
    if let Err(e) = run() {
        eprintln!("Error listing directory: {e}");
        std::process::exit(1);
    }
}

fn run() -> Result<(), Box<dyn Error>> {
    for entry in fs::read_dir(".")? {
        let entry = entry?;
        println!("{}", entry.file_name().to_string_lossy());
    }
    Ok(())
}
