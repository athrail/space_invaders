#! /bin/sh

# set -xe

CFLAGS="-Wall -Wextra -ggdb"
CLIBS="-lraylib -lm"

clang $CFLAGS -o build/space_invaders main.c player.c enemy.c $CLIBS #$CINCLUDES $CLIBPATHS
