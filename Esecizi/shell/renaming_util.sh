#!/bin/bash

EXTENSION=".$1"
PREFIX="backup-"

if [ "${EXTENSION}" == "." ] ; then
    echo "Usage ./$0 .smth"
    exit 1
fi


for FILE in *"$EXTENSION"; do
    if [[ "${FILE}" == "${PREFIX}"* ]]; then
        echo "Skipping $FILE (alrleady has prefix)"
    else
        mv "${FILE}" "${PREFIX}${FILE}"
        echo "Renamed: ${FILE} -> ${PREFIX}${FILE}"
    fi
done

