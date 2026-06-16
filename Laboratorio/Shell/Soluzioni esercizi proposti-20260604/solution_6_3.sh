#!/bin/bash
G=$1
shift
TMP=$$.tmp
find $G -type d | while read D; do
    # in each directory $D
    FOUND='FALSE'
    COUNT=0
    for F in "$D"/*; do
        if [ -f "$F"  -a -r "$F" ]; then
            for C in "$@"; do
                # echo $C "$F"
                if grep $C "$F" > /dev/null; then
                    FOUND='TRUE'
                    ((COUNT++))
                    #echo "found character \"$C\" in file \"$F\" in dir \"$D\"; FOUND=$FOUND"
                    break 
                fi
            done
        fi
    done
    if [ "$FOUND" == 'TRUE' ]
    then 
        echo "+ $D"
    else
        echo "- $D"
    fi
    echo $COUNT >> $TMP
done
#cat $TMP
awk '{tot+=$1} END {print "Total files found: "tot}' $TMP
rm $TMP

