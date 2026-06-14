#!/bin/bash

THRESHOLD=10

PARTITION="/home"

USAGE=$(df "$PARTITION" | tail -1 | tr -s ' ' | cut -d' ' -f5 | tr -d '%')

AVAILABLE=$((100-$USAGE))

if [ $AVAILABLE -lt $THRESHOLD ] ; then
    echo "Warning" >&2
else
    echo "Tutto nel billilong"
    exit 0
fi
