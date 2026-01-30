cmake_minimum_required(VERSION 3.5)
project(CG1_Tracer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Configuration for Homebrew ---
if(APPLE AND EXISTS "/opt/homebrew")
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

# --- Set output dirs ---
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin")

# --- Find System Packages ---
if(NOT WIN32)
    find_package(Threads REQUIRED)
endif()
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(spdlog REQUIRED)
find_package(Vulkan REQUIRED)

# --- Sources ---
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
file(GLOB_RECURSE cpp_src "src/*.cpp")
file(GLOB intermediate_src "Intermediate/Resources/*.cpp")

# --- Exclude third-party from glob, then add only what we need ---
list(FILTER cpp_src EXCLUDE REGEX "src/third-party/.*")

# --- ImGui Sources ---
set(IMGUI_SOURCES
    src/third-party/imgui/imgui.cpp
    src/third-party/imgui/imgui_draw.cpp
    src/third-party/imgui/imgui_tables.cpp
    src/third-party/imgui/imgui_widgets.cpp
    src/third-party/imgui/backends/imgui_impl_glfw.cpp
    src/third-party/imgui/backends/imgui_impl_vulkan.cpp
)

# --- Executable ---
add_executable(tracey_rt ${cpp_src} ${intermediate_src} ${IMGUI_SOURCES})
target_include_directories(tracey_rt PUBLIC src Intermediate/Resources src/third-party/imgui)

# --- Linking ---
target_link_libraries(tracey_rt PRIVATE glfw glm::glm spdlog::spdlog Vulkan::Vulkan)

# --- Platform Specific Linking ---
if (APPLE)
    # macOS frameworks
    target_link_libraries(tracey_rt 
        PUBLIC
        "-framework Cocoa" 
        "-framework IOKit" 
        "-framework CoreFoundation" 
        "-framework CoreVideo"
        "-framework Metal"
        "-framework QuartzCore" 
        "-framework Foundation" 
        "-framework IOSurface" 
        "-framework CoreGraphics"
        "-framework AppKit"
        "/usr/local/lib/libMoltenVK.dylib"
        "/usr/local/lib/libvulkan.dylib"
    )
elseif (UNIX)
    # Linux specific settings
    find_package(X11 REQUIRED)
    target_link_libraries(tracey_rt PRIVATE ${X11_LIBRARIES} dl pthread)
endif()