cmake_minimum_required(VERSION 3.10)

project(CreatureColorPlugin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set ASA API path (adjust if needed)
set(ASA_API_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Include directories
include_directories(
    "${ASA_API_DIR}/AsaApi/Core/Public"
)

# Add the plugin as a shared library
add_library(CreatureColorPlugin SHARED
    CreatureColorPlugin.cpp
)

# Link directories (adjust based on your build configuration)
link_directories(
    "${ASA_API_DIR}/x64/Release"
)

# Link against AsaApi.lib
target_link_libraries(CreatureColorPlugin
    AsaApi
)

# Set output directory
set_target_properties(CreatureColorPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Copy PluginInfo.json to output directory
add_custom_command(TARGET CreatureColorPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/PluginInfo.json"
    "$<TARGET_FILE_DIR:CreatureColorPlugin>/PluginInfo.json"
)
