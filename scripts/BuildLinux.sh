#!/bin/bash
cd "$(dirname "$0")/.."
rm -rf bin
cmake -B build
cmake --build build --config Debug
