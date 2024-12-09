#! /bin/sh

# set -xe

CFLAGS="-Wall -Wextra -ggdb"
CINCLUDES="-I/home/athrail/probe/raylib/raylib-5.5_linux_amd64/include"
CLIBPATHS="-L/home/athrail/probe/raylib/raylib-5.5_linux_amd64/lib"
CLIBS="-lraylib -lm"

export LD_LIBRARY_PATH="/home/athrail/probe/raylib/raylib-5.5_linux_amd64/lib:$LD_LIBRARY_PATH"

clang $CFLAGS -o build/space_invaders main.c player.c enemy.c $CLIBS $CINCLUDES $CLIBPATHS
