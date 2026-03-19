#!/bin/sh
#if the number of supplied parameters is less than 2, print error message and exit
if [ $# -lt 2 ]
then
    echo "finder.sh rquires two arguments"
    exit 1
fi
#check if the first argument is a directory, if not print error message and exit
if [ ! -d $1 ]
then
    echo "First argument must be a directory"
    exit 1
fi

#set x to the number of files in the directory and its subdirectories given as first argument
x=$( ls -1 $1 | wc -l)

#set y to the number of lines found in respective files that contain the searchstring $2 found in respective files
y=$( grep -r $2 $1 | wc -l)

echo "The number of files are $x and the number of matching lines are $y"