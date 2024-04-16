#!/usr/bin/bash
set -e
FILES="main.c ir/opt/*.c ir/*.c common/*.c ir/transform/*.c"
CFLAGS="-Wall -Wextra -Wno-unused -Wpedantic -Werror -std=c11"

# shellcheck disable=SC2086

if [[ $1 == "analyze" ]]; then
  echo "analyzing..."
  clang --analyze -Xclang -analyzer-werror $CFLAGS $FILES
elif [[ $1 == "info" ]]; then
  clang --version
  echo cflags: $CFLAGS
  echo files: $FILES
else
  echo "compile DebugOpt"
  clang -g -ggdb -O3 $CFLAGS $FILES -o vxcc
fi

# shellcheck enable=SC2086
