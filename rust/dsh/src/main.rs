use std::io::*;
use std::fs::File;
use std::os::unix::io::AsRawFd;
use nix::unistd::ForkResult;
use nix::sys::wait::wait;
use nix::unistd::{fork,pipe,execv,dup,dup2,close};
use std::ffi::CString;

fn exec_command(command: &str) -> Result<()> {
    let mut words = command.split_ascii_whitespace();
    if let Some(program) = words.next() {
        let mut args : Vec<CString> = words.map(|i| CString::new(i).unwrap()).collect();
        args.insert(0,CString::new(program)?);

        if let Ok(_) = execv(&CString::new(program)?,&args) {
            panic!("this should never happen");
        }
        else {
            println!("dsh: error executing: {}",program);
            std::process::exit(1);
        }
    }    
    Ok(())
}
fn run_pipeline(_head: &str, _tail: &str) -> Result<()> {
   panic!("Don't know how to pipeline!");
}

fn run_sequence(_head: &str, _tail: &str) -> Result<()> {
    panic!("Don't know how to sequence!");
}

fn run(line: &str) -> Result<()> {
    if let Some((head,tail)) = line.split_once(";") {
        run_sequence(head,tail)?
    }
    else if let Some((head,tail)) = line.split_once("|") {
        run_pipeline(head,tail)?
    }
    else {
        unsafe { 
            if let Ok(ForkResult::Child) = fork() {
                exec_command(line)?;
            }
            else {
                wait().unwrap();
            }
        }
    }
    Ok(())
}

fn main() -> Result<()> {

    let _ = std::fs::create_dir(format!("{}/.dsh",std::env::var("HOME").unwrap()));
    
    let reader = BufReader::new(stdin());
    print!("dsh> ");
    stdout().flush()?;        

    let _origin=dup(0)?;
    let _origout=dup(1)?;
    let _origerr=dup(2)?;

    for line in reader.lines() {
        run(&line?)?;

        print!("dsh> ");
        stdout().flush()?; 
    }
    Ok(())
}
