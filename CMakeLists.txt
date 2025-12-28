cmake_minimum_required(VERSION 3.5)
project(CG1_Tracer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output dirs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin")

if(NOT WIN32)
  find_package(Threads REQUIRED)
  find_package(X11 REQUIRED)
endif()

# This directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
if(APPLE)
   # Assuming Homebrew installation path on Apple Silicon
   include_directories(AFTER "/opt/homebrew/include")
   list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
endif()

# which source files to use
file(GLOB_RECURSE cpp_src "src/*.cpp")
file(GLOB intermediate_src "Intermediate/Resources/*.cpp")

# the actual raytracer executable
add_executable(tracey_rt ${cpp_src} ${intermediate_src})
target_include_directories(tracey_rt PUBLIC src)
if(NOT WIN32)
    target_link_libraries(tracey_rt ${CMAKE_THREAD_LIBS_INIT} ${X11_LIBRARIES})
endif()
find_package(Vulkan REQUIRED)
target_link_libraries(tracey_rt Vulkan::Vulkan)
target_include_directories(tracey_rt PUBLIC Intermediate/Resources)

# Add GLFW (from local source)
add_subdirectory(third-party/glfw-3.4)
target_include_directories(tracey_rt PUBLIC third-party/glfw-3.4/include)
target_link_libraries(tracey_rt glfw)

# glm
add_subdirectory(third-party/glm)
target_include_directories(tracey_rt PUBLIC third-party/glm)
target_link_libraries(tracey_rt glm)

# spdlog
add_subdirectory(third-party/spdlog-1.15.2)
target_include_directories(tracey_rt PUBLIC third-party/spdlog-1.15.2/include)
target_link_libraries(tracey_rt spdlog::spdlog)

if (APPLE)
    # macOS frameworks
    target_link_libraries(tracey_rt 
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
    target_link_libraries(tracey_rt ${X11_LIBRARIES} dl pthread)
endif()