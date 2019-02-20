#!/usr/bin/env sh

function make_files () {
    for f in $(seq 1 20)
    do
	if (( $RANDOM > 32767/2 ))
	then
	    echo "    FILE: $f"
	    dd if=/dev/urandom of=$f.txt count=1 bs=10 2>/dev/null
	fi
    done
}

function make_dir () {
    local depth=$1;
    make_files

    if (( $depth > 0 ))
    then
	for d in $(seq 1 20)
	do
	    if (( $RANDOM > 16383 ))
	    then
		mkdir $d
		pushd $d &>/dev/null
		echo "DIR: $(pwd)"
		make_dir $(($depth-1))
		popd &>/dev/null
	    fi
	done
    fi
}

mkdir test_data
pushd test_data
make_dir 3
popd test_data



	 
