@echo off
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
call .\bootstrap-vcpkg.bat
call .\vcpkg install glfw3 glm spdlog