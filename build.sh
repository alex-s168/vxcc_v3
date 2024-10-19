#!/usr/bin/bash

set -e

: ${CFLAGS:="-Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-comment -Wno-format -Wno-sign-compare -Wno-char-subscripts -Wno-implicit-fallthrough -Wno-missing-braces -Werror"}

if [ -z $EMPATH ]; then
        : ${CC:=clang}
        : ${BUILD_CC:=$CC}
        : ${EX_CFLAGS:=""}
	: ${EX_LDFLAGS:=""}
        : ${AR:="ar"}
else
        : ${CC:=$EMPATH/emcc}
        : ${BUILD_CC:=clang}
        : ${EX_CFLAGS:="-O3"}
	: ${EX_LDFLAGS:=""}
        : ${AR:=$EMPATH/emar}
fi

ANALYZER_FLAGS="$CFLAGS -Wno-analyzer-malloc-leak -Wno-analyzer-deref-before-check"
CFLAGS="$CFLAGS $EX_CFLAGS"

#CFG_HASH=$(sum <<< "$CC $BUILD_CC $AR $CLAGS")
#CLEAN=1
#if [ -f "_settings.chksum" ]; then
#       OLD=$(cat _settings.chksum)
#       if [[ "$CFG_HASH" == "$OLD" ]]; then
#               CLEAN=0
#       fi
#fi
#echo $CFG_HASH > _settings.chksum
#
#if [ -f "build" ]; then
#       if (( $CLEAN == 1 )); then
#               echo "# completely re-building"
#               rm -r build
#       fi
#fi

FILES="ir/*.c common/*.c ir/opt/*.c ir/transform/*.c cg/x86_stupid/*.c irparser/*.c"

# shellcheck disable=SC2086

function prepare() {
  $BUILD_CC build.c -lpthread -DVERBOSE=1 -DPYTHON="\"$python\"" -DCC="\"$CC\"" -DCC_ARGS="\"$CFLAGS\"" -DLD_ARGS="\"$EX_LDFLAGS -lpthread\"" -DAR="\"$AR\"" -o build.exe
  echo "# build.exe compiled"
  echo "# gen cdef files"
  ./build.exe gen
}

if [[ $1 == "analyze" ]]; then
  echo "# analyzing..."
  prepare
  echo "# starting analyzer"
  clang --analyze -Xclang -analyzer-werror $ANALYZER_FLAGS $FILES
elif [[ $1 == "ganalyze" ]]; then
  echo "analyzing (gcc) ..."
  prepare
  echo "# starting analyzer (gcc)"
  gcc -fanalyzer -fsanitize-trap=undefined $ANALYZER_FLAGS $FILES
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
  echo "# compile Debug"
  prepare
  echo "# deps"
  ./build.exe deps
  echo "# lib.a"
  ./build.exe lib.a
elif [[ $1 == "lsp" ]]; then 
  prepare
elif [[ $1 == "libfiles" ]]; then
  echo "build/lib.a allib/build/all.a"
else
  echo "invalid arguments; usage: ./build.sh [ganalyze|analyze|build|lsp|libfiles]"
  echo "you can set CC, CFLAGS, BUILD_CC, python, EX_CFLAGS, AR, EX_LDFLAGS"
  echo "if you set EMPATH, these flags get added automatically (you can overwrite them manually): CC=\$EMPATH/emcc BUILD_CC=clang EX_CFLAGS="-O3" AR=\$EMPATH/emar"
fi

# shellcheck enable=SC2086
