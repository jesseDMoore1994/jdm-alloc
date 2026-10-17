#!/usr/bin/env bash

# This script formats the code in the repository using clang-format.

set -euo pipefail

clang-format -i -style=file $(find . -name '*.c' -o -name '*.h')
