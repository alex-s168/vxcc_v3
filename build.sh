#!/usr/bin/bash

set -e

: ${CFLAGS:="-Wall -Wextra -Wno-unused -Wno-comment -Wno-format -Wno-sign-compare -Wno-analyzer-malloc-leak -Wno-char-subscripts -Wno-implicit-fallthrough -Wno-missing-braces -Wno-analyzer-deref-before-check -Werror -std=c11"}
: ${CC:=clang}

FILES="test.c ir/*.c common/*.c ir/opt/*.c ir/transform/*.c cg/x86_stupid/*.c"

# shellcheck disable=SC2086

function prepare() {
  if [ -z $python ]; then
    echo \# detecting python...
    if venv/bin/python3 --help &>/dev/null; then 
      python=venv/bin/python3
    elif venv/Scripts/python --help &>/dev/null; then
      python=venv/Scripts/python
    elif python --help &>/dev/null; then
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
elif [[ $1 == "build" ]]; then
  echo "compile Debug"
  prepare
  
  echo "# lib.a"
  ./build.exe lib.a 
elif [[ $1 == "test" ]]; then 
  echo "compile Debug"
  prepare
  
  echo "# lib.a"
  ./build.exe lib.a 

  echo "# tests"
  ./build.exe tests
else 
  echo "invalid arguments; usage: ./build.sh [ganalyze|analyze|build|test]"
fi

# shellcheck enable=SC2086
