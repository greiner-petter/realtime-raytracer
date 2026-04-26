@echo off
cd /d "%~dp0\.."
winget install Kitware.CMake
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
call .\bootstrap-vcpkg.bat
call .\vcpkg install glfw3 glm spdlog
