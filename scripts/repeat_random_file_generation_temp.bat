#!/bin/bash
#echo off #for windows(?)
#set arg1=$1 # for windows

test_file_name="random-test_temp.in"

echo "Running $2 (Random File $1 times with $test_file_name input) each"
"Hello World :D"
make
for j in `seq 1 $2`;
do
    python3 make_random_tests_temp.py
    for i in `seq 1 $1`;
    do
        ./grader ../cs3211-assignment-1-e1337202/engine < $test_file_name
        if [ "$?" -ne "0" ];
        then
            return 1 ## idk if 1 got significance here anot but yeaaaa
        fi
    done;
done;

## arg1 is no. of runs per random file, arg2 is no. of random files generated for the arg1 runs
#used using . repeat_random_file_generation_temp.bat 30 10 