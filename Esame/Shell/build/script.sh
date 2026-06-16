#!/bin/bash

if [ $# -ne 2 ]; then
    echo "- Parameter error"
    exit
fi

FILENAME="$1"

if [ -f "$1" ] && [ -r "$1" ]; then
    echo "+ Source OK"
else
    echo "- Source error"
fi

mkdir -p ./build

if [ ! -e ./build ]; then 
    echo "- Builddir creation error"
fi

cp ./script.sh ./build/script.sh
gcc -o ./build/script ./build/script.sh

if [ "$?" -ne 0]; then 
    echo "- Compiling ${FILENAME} error"
else 
    echo "+ Compiling ${FILENAME} OK"
fi

mkdir -p ./D/bin
gcc -o ./bin/script ./build/script
if [ "$?" -ne 0 ]; then
    echo "- Install error"
fi

./bin/script
PIDPROC="$!"

if [ "$?" -eq 0 ]: then
    echo "+ Execution OK"
else
    echo "- Timeout error"
fi

sleep 5

kill -9 "$PIDPROC"

echo "- Timeout error"