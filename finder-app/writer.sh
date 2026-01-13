#!/bin/bash
#check if both arguments are supplied, if not print error message and exit
if [ $# -lt 2 ]
then
    echo "writer.sh requires two arguments"
    exit 1
fi


#write the string given as second argument to the file given as first argument, 
#overwriting any existing file and creating the path if necessary
mkdir -p "$(dirname "$1")"

echo $2 > $1 
if [ $? -ne 0 ]
then
    echo "Error writing to file"
    exit 1
fi