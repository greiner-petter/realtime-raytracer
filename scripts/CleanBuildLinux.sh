#!/bin/bash
cd "$(dirname "$0")/.."
rm -rf bin build
cmake -B build
cmake --build build --config Debug
