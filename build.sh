#!/usr/bin/bash
set -e
: ${CFLAGS:="-Wall -Wextra -Wno-unused -Wpedantic -Werror -std=c11"}
: ${CC:="clang"}

FILES="test.c ir/*.c common/*.c ir/opt/*.c ir/transform/*.c cg/x86/*.c cg/x86_stupid/*.c"

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
  clang build.c -lpthread -o build.exe
  ./build.exe lib.a 
  ./build.exe x86.a 
  ./build.exe tests
fi

# shellcheck enable=SC2086
