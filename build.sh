#!/bin/bash

DIR="$(dirname "$0")"

if [[ ! -f "$DIR/build/build.ninja" ]]; then
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B "$DIR/build"
fi

ninja -C "$DIR/build"
