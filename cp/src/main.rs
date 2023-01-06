#[allow(unused_imports)]
use io_uring::{IoUring,types,cqueue,squeue,Submitter, opcode};
use std::os::unix::io::AsRawFd;
use std::{fs};

fn main() {
    let mut rn = IoUring::new(8).expect("Error creating io_uring instance");
    
    let fname = std::env::args().skip(1).next().unwrap();
    let fd = fs::File::open(&fname).expect("error opening file");
    let mut buffr = vec![0;fs::metadata(&fname).unwrap().len() as usize];

    let readr = opcode::Read::new(types::Fd(fd.as_raw_fd()),buffr.as_mut_ptr(),buffr.len() as _)
        .build()
        .user_data(0x69);


    unsafe{
        rn.submission()
            .push(&readr)
            .expect("error submitting");
    }
    
    rn.submit_and_wait(1).expect("error waiting");

    let cqe = rn.completion().next().expect("completion queue empty");
     println!("{:?}", cqe.user_data());
     println!("{}",fs::metadata(&fname).unwrap().len());
     println!("{}",String::from_utf8_lossy(&buffr));

}
