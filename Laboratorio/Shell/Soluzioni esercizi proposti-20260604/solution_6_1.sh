#!/bin/bash
# print just success and errors count
awk '{total++} $9 ~ /[45][0-9][0-9]/ {error++} $9 ~/2[0-9][0-9]/{success++} END {print "Success: " success "/" total, "Error: " error "/" total}' access.log
# status code breackdown
awk '{count[$9]++}  END {for (code in count) print "Status: " code, " => " count[code]}' access.log

