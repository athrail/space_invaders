#! /bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb"
CLIBS="-lSDL2 -lSDL2_ttf -lSDL2_image"

clang $CFLAGS -o build/space_invaders main.c $CLIBS
