#!/usr/bin/bash

set -e

: ${CFLAGS:="-Wall -Wextra -Wno-unused -Wno-comment -Wno-format -Wno-sign-compare -Wno-analyzer-malloc-leak -Wno-char-subscripts -Wno-implicit-fallthrough -Wno-missing-braces -Wno-analyzer-deref-before-check -Werror -std=c11"}
: ${CC:=clang}

FILES="test.c ir/*.c common/*.c ir/opt/*.c ir/transform/*.c cg/x86_stupid/*.c"

# shellcheck disable=SC2086

function prepare() {
  if [ -z $python ]; then
    echo \# detecting python...
    if python --help &>/dev/null; then
      python=python
    elif python3 --help &>/dev/null; then
      python=python3
    else
      echo "python not found! tried \"python\" and \"python3\"! Set \"python\" variable or install python!"
      exit 1
    fi
  fi
  echo "found python at: $python"
  $python -m pip install generic-lexer &>/dev/null
  $CC build.c -lpthread -DVERBOSE=1 -DPYTHON="\"$python\"" -DCC="\"$CC\"" -o build.exe
  echo "# build.exe compiled"
  echo "# gen cdef files"
  ./build.exe gen
}

if [[ $1 == "analyze" ]]; then
  echo "# analyzing..."
  prepare
  echo "# starting analyzer"
  clang --analyze -Xclang -analyzer-werror $CFLAGS $FILES
elif [[ $1 == "ganalyze" ]]; then
  echo "analyzing (gcc) ..."
  prepare
  echo "# starting analyzer (gcc)"
  gcc -fanalyzer -fsanitize-trap=undefined $CFLAGS $FILES
elif [[ $1 == "info" ]]; then
  echo clang:
  clang --version
  echo gcc:
  gcc --version
  echo cflags: $CFLAGS
  echo files: $FILES
  echo cc:
  "$CC" --version
else
  echo "compile Debug"
  prepare
  
  echo "# lib.a"
  ./build.exe lib.a 
  
  echo "# x86.a"
  ./build.exe x86.a 

  echo "# tests"
  ./build.exe tests
fi

# shellcheck enable=SC2086
