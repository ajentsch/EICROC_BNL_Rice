#!/bin/bash

# Exit immediately if any command fails
set -e

PY_FILE="registers_values.py"
C_FILE="zcu106.C"
ANA_FILE="ana_zcu.C"
BINARY="program"
ANA_BINARY="ana_program"

# 1. Compile static binary ONCE before the loop
echo "Compiling $C_FILE..."
g++ "$C_FILE" -o "$BINARY"

# Backup original Python file so we can restore the template each iteration
cp "$PY_FILE" "${PY_FILE}.bak"

# Trap to ensure cleanup even if script exits early (due to set -e)
trap 'mv "${PY_FILE}.bak" "$PY_FILE"; rm -f temp_ana_program.c "$BINARY" "$ANA_BINARY"' EXIT

for val in {0..63..3}; do
    echo "Processing for iteration value: $val"

    outfile="pulse_result_${val}.csv"
    
    binary_val=$(python3 -c "print(f'{ $val :08b}')")

    # substitute in pulse value for each iteration
    sed "s/eic_clib.write_asic_indirect_reg(0x400C, 0bPLACEHOLDER)  #dacb pulser/eic_clib.write_asic_indirect_reg(0x400C, 0b$binary_val)  #dacb pulser/" "${PY_FILE}.bak" > "$PY_FILE"
    sed "s|FILENAME_PLACEHOLDER|$outfile|" "$ANA_FILE" > temp_ana_program.c

    # python3 "$PY_FILE"

    # run ana program
    g++ temp_ana_program.c -o "$ANA_BINARY"

    # take data
    ./"$BINARY" -d /dev/ttyUSB3 -n 10 -m 2 | ./"$ANA_BINARY"

    # Clean up temp source file for this iteration
    rm temp_ana_program.c
done

echo "All runs completed!"
