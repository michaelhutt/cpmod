// modcopy - main.rs

use std::collections::VecDeque;
use std::fs;
use std::os::unix::fs::PermissionsExt;
use std::path;
use std::process::exit;

use libc::*;


fn main() {
    let mut _recurse:bool = false;

    // okay, the argument parsing is a little more awkward than I'd like.
    // Why? Because I don't have internet right now and getting the
    // docparse/optparse crates is more pain. This...will be fine.
    let args:Vec<String> = std::env::args().collect();

    if args.len() < 2 {
        print_usage();
        exit(1);
    }

    let need_help = args.iter().any(|w| { w == "--help" || w == "-h" } );
    if need_help {
        print_full_usage();
        exit(1);
    }

    let mut positionals: VecDeque<String> = VecDeque::new();
    for arg in args[1..].iter() {
        if arg == "--recursive" || arg == "-R" {
            _recurse = true;
        }
        else {
            positionals.push_back(arg.to_string());
        }
    }

    let _mode_spec = positionals.pop_front().unwrap();
    let (_from,_to) = match parse_mode_transfer_spec(&_mode_spec as &str) {
        Ok(x) => x,
        Err(x) => {
            eprintln!("[Error] Unknown mode specifier: {}", x);
            print_full_usage();
            exit(1);
        }
    };
    
    for file in positionals.iter() {
        modcopy(&file, &_from, &_to, _recurse);
    }
}

fn print_usage() {
    eprintln!("Usage: modcopy [-r] FROM-TO FILE [FILE...]");
}

fn print_full_usage() {
    print_usage();
    eprintln!("
{esc}[1;4mFROM{esc}[0m is a single letter specifying where the file mode should be copied from.
{esc}[1;4mTO{esc}[0m is one or two letters specifying where the copied file mode should be
   applied.

Both specifiers consist of 'u', 'g', or 'o' for 'user', 'group', and 'other'
respectively. i.e. to copy the permissions from the user to the group, use
'u-g'. To copy from the group to the user and other, use 'g-uo'.

{esc}[1;4mFILE{esc}[0m is the file on which to operate and can be specified multiple times.

optional arguments:
  --help, -h       Show this help message and exit.
  --recursive, -r  Operate recursively on the file/directory arguments.
",esc=27 as char);
}



fn modcopy(filename:&str, from: &(u32,u32), to: &Vec<(u32,u32)>, recurse:bool) -> std::io::Result<()> {
    // output format: filename [ now perms ] -> [ after perms ]
    print!("{} ", filename);

    // let filestat = mystat(filename);
    let metadata = fs::metadata(filename)?;
    let mut perms = metadata.permissions();
    print!("[ {} ] -> ", permission_string(perms.mode()));


    // do the actual mode copying
    let mut new_mode = perms.mode();
    for (_mask, _shift) in to {
        // clear existing perms in the spot
        new_mode = new_mode & !_mask;

        // copy in the new ones (mask out bits 'from', shift all the way to the right,
        // then shift left into correct place, finally bitwise-or the new parts at the
        // previously cleared area.
        new_mode = new_mode | ((new_mode & from.0) >> from.1) << _shift;
    }
    
    perms.set_mode(new_mode);
    
    let _pcopy = perms.clone();
    // TODO: check the result here...
    //fs::set_permissions(filename, perms);

    println!("[ {} ]", permission_string(_pcopy.mode()));

    if recurse && path::Path(filename).is_dir() {
        println!("it's a directory!");
    }
    Ok(())
}

fn parse_mode_transfer_spec(_mode_spec: &str) -> Result<((u32, u32), Vec<(u32, u32)>), String> {
    let normalized_spec = _mode_spec.replace("+", "-");
    let _specs:Vec<&str> = normalized_spec.split("-").collect();

    if _specs.len() < 2 {
        return Err(String::from(_mode_spec));
    }
    
    let _from = get_mask_and_shift(_specs[0])?;

    let mut _to_specs = Vec::<(u32, u32)>::new();
    for _c in _specs[1].chars() {
        _to_specs.push(get_mask_and_shift(&_c.to_string())?);
    }
    
    return Ok((_from, _to_specs))
}

fn get_mask_and_shift(spec: &str) -> Result<(u32,u32), String> {
    match spec {
        "u" => Ok((S_IRWXU, 6)),
        "g" => Ok((S_IRWXG, 3)),
        "o" => Ok((S_IRWXO, 0)),
        _ => Err(String::from(spec))
    }
}


fn permission_string(perm: u32) -> String {
    let user_perm = ((perm & S_IRWXU) >> 6) as u8;
    let group_perm = ((perm & S_IRWXG) >> 3) as u8;
    let other_perm = (perm & S_IRWXO) as u8;

    // gross
    return format!("{}{}{}{}{}{}{}{}{}",
                   if (user_perm & 4) != 0 {"r"} else {"-"},
                   if (user_perm & 2) != 0 {"w"} else {"-"},
                   if (user_perm & 1) != 0 {"x"} else {"-"},
                   if (group_perm & 4) != 0 {"r"} else {"-"},
                   if (group_perm & 2) != 0 {"w"} else {"-"},
                   if (group_perm & 1) != 0 {"x"} else {"-"},
                   if (other_perm & 4) != 0 {"r"} else {"-"},
                   if (other_perm & 2) != 0 {"w"} else {"-"},
                   if (other_perm & 1) != 0 {"x"} else {"-"}
                   );
}
