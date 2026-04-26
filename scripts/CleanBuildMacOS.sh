#!/bin/bash
cd "$(dirname "$0")/.."
rm -rf bin build
cmake -G Xcode -B build
cmake --build build --config Debug
