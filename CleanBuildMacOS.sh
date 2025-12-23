rm -rf bin
rm -rf build
cmake -G Xcode -B build
cmake --build build --config Debug