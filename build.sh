#!/usr/bin/bash
set -e
: ${CFLAGS:="-Wall -Wextra -Wno-unused -Wpedantic -Werror -std=c11"}
: ${CC:="clang"}

FILES="main.c ir/opt/*.c ir/*.c common/*.c ir/transform/*.c"

# shellcheck disable=SC2086

if [[ $1 == "analyze" ]]; then
  echo "analyzing..."
  clang --analyze -Xclang -analyzer-werror $CFLAGS $FILES
elif [[ $1 == "info" ]]; then
  echo clang:
  clang --version
  echo cflags: $CFLAGS
  echo files: $FILES
  echo cc:
  $CC --version
else
  echo "compile Debug"
  $CC -g -ggdb -O0 $CFLAGS $FILES -o vxcc
fi

# shellcheck enable=SC2086
