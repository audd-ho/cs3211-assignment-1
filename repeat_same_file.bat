#!/bin/bash
#echo off #for windows(?)
#set arg1=$1 # for windows

echo "Running Same File for $1 times with $2 input"
"Hello World :D"
make
for i in `seq 1 $1`;
do
    ./grader ../cs3211-assignment-1-e1337202/engine < $2
    if [ "$?" -ne "0" ];
    then
        break
    fi
done;

#used using . repeat_same_file.bat 10 tests/medium-my-try.in 