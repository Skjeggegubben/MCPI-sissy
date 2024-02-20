cmake_minimum_required(VERSION 3.16.0)

# Build For ARM
set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Start Project
project(mcpisissy)

# Include SDK
include("$ENV{HOME}/.minecraft-pi/sdk_2.4.8/lib/minecraft-pi-reborn-client/sdk/sdk.cmake")

# Build
add_library(mcpisissy SHARED inventorysissy.cpp custompackets.cpp small_stuff.cpp)
target_link_libraries(mcpisissy mods-headers reborn-patch symbols misc home)
target_compile_features(mcpisissy PRIVATE cxx_std_17) 

# In order to use functions from libextrapi.so refered to by extern:
#find_library(LIBRARY_NAME libextrapi.so PATHS "$ENV{HOME}/.minecraft-pi/mods")
#target_link_libraries(mcpisissy ${LIBRARY_NAME})
