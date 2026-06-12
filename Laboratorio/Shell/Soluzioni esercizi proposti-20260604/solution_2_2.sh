#!/bin/bash
# Define the GROCERIES array
GROCERIES=("bread" "milk" "eggs" "coffee")
# Print the total number of items in the list
echo "Total number of items in the grocery list: ${#GROCERIES[@]}"
# alterantive version using wc
TOTAL_WC=$(printf '%s\n' "${GROCERIES[@]}" | wc -l)
echo "Total number of items (using wc): $TOTAL_WC"
# Print a reminder indicating the first element in the list
echo "Reminder: The first item you need to get is ${GROCERIES[0]}."
