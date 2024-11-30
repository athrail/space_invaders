#! /bin/sh

set -xe

CFLAGS="-Wall -Wextra"

clang $CFLAGS -o build/space_invaders main.c
