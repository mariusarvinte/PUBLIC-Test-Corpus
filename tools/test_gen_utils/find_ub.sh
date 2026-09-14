#!/bin/bash

# © 2026 Massachusetts Institute of Technology
# MIT License

set -euo pipefail

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <directory>" >&2
    exit 1
fi

TARGET_DIR=$1

if [ ! -d "$TARGET_DIR" ]; then
    echo "Directory not found: $TARGET_DIR" >&2
    exit 1
fi

rg -n --glob '*.json' 'has_ub' "$TARGET_DIR"
