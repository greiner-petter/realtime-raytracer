#!/bin/bash
cd "$(dirname "$0")/.."
rm -rf bin
cmake -G Xcode -B build
cmake --build build --config Debug
