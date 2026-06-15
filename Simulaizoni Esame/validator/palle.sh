#!/bin/bash

FACCESS="access.log"
FLOG="auth.log"

COD1=$400
COD2=$401


failed(){

    grep "Failed password" "${FACCESS}" | grep -oE '[0-9]{1,3}(\.[0-9]{1,3}){3}' | sort -u > ssh.ip

}

#elaborazione(){

   # grep -E ' (4**|5**) ' "${FACCESS}" | sed -E 's/^[0-9]{1,3}(\.[0-9]{1,3}){3}$' | echo > web.ip

    
#}

#$( elaborazione )

$( failed )

echo "Success"

