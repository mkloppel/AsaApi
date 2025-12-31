cmake_minimum_required(VERSION 3.10)

project(RconAckPlugin)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set ASA API path (adjust if needed)
set(ASA_API_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")

# Include directories
include_directories(
    "${ASA_API_DIR}/AsaApi/Core/Public"
)

# Add the plugin as a shared library
add_library(RconAckPlugin SHARED
    RconAckPlugin.cpp
)

# Link directories (adjust based on your build configuration)
link_directories(
    "${ASA_API_DIR}/x64/Release"
)

# Link against AsaApi.lib
target_link_libraries(RconAckPlugin
    AsaApi
)

# Set output directory
set_target_properties(RconAckPlugin PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin"
)

# Copy config files to output directory
add_custom_command(TARGET RconAckPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/PluginInfo.json"
    "$<TARGET_FILE_DIR:RconAckPlugin>/PluginInfo.json"
)

add_custom_command(TARGET RconAckPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    "${CMAKE_CURRENT_SOURCE_DIR}/config.json"
    "$<TARGET_FILE_DIR:RconAckPlugin>/config.json"
)
