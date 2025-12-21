#!/bin/bash
set -e

# check if arg given
if [ -z "$1" ]; then
    echo "Usage: $0 <input-file>"
    exit 1
fi

INPUT_FILE="$1"

make
./main "$INPUT_FILE"
