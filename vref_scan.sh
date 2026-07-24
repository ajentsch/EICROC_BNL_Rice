#!/bin/bash

# Exit immediately if any command fails
set -e

C_FILE="zcu106.C"
ANA_FILE="ana_zcu.C"      # <-- Un-commented and removed spaces around '='
BINARY="program"
ANA_BINARY="ana_program"

for val in {10..160..5}; do
    echo "Processing for iteration value: $val"

    outfile="result_${val}.csv"  # <-- Defined $outfile for sed replacement
    hex_val=$(printf "0x%02X" "$val")
    # 1. Modify source files
    sed "s/v_ref_val = 0x00;/v_ref_val = $hex_val;/" "$C_FILE" > temp_program.c
    sed "s/FILENAME_PLACEHOLDER/$outfile/" "$ANA_FILE" > temp_ana_program.c

    # 2. Compile (Using g++ for .C files; switch to gcc if pure C)
    g++ temp_program.c -o "$BINARY"
    g++ temp_ana_program.c -o "$ANA_BINARY"

    # 3. Run pipeline
    ./"$BINARY" -d /dev/ttyUSB3 -n 10 -m 2 | ./"$ANA_BINARY"

    # Clean up temp source files
    rm temp_program.c temp_ana_program.c
done

# Clean up built binaries
rm "$BINARY" "$ANA_BINARY"

echo "All runs completed!"
