#! /bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb"
CLIBS="-lSDL2"

clang $CFLAGS -o build/space_invaders main.c $CLIBS
