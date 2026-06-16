#!/bin/bash

clear
STAT1=$(grep '^cpu ' /proc/stat)
sleep 1
while true; do
    # Take both snapshots and feed them as a 2-line stream into a single awk command
    STAT2=$(grep '^cpu ' /proc/stat)
    echo -e "$STAT1\n$STAT2"
    (echo -e "$STAT1\n$STAT2") | awk '
    # first line
    NR==1 {
        for(i=2; i<=8; i++) total1 += $i;
        idle1 = $5 + $6;
    }
    # second line
    NR==2 {
        for(i=2; i<=8; i++) total2 += $i;
        idle2 = $5 + $6;
    }
    END {    
        total_delta = total2 - total1;
        idle_delta = idle2 - idle1;        
        if (total_delta > 0) {
            cpu_load = ((total_delta - idle_delta) / total_delta) * 100;
            printf "\rCPU Load: %.2f%%   \033[K", cpu_load;
        }
    }'
    STAT1="$STAT2"
    sleep 1
done
