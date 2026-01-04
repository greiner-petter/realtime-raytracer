@echo off
rmdir /s/q .\bin
rmdir /s/q .\build
cmake -G "Visual Studio 17 2022" -B build -D CMAKE_TOOLCHAIN_FILE=%CD%/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Debug