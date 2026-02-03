# Définition de l'exécutable
add_executable(lsaa-core main.cpp)

# Inclusions
target_include_directories(lsaa-core PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)

# Link
target_link_libraries(lsaa-core PRIVATE imgui_lib glfw opengl32 nlohmann_json::nlohmann_json shell32 advapi32)
# Features C++20 spécifiques si nécessaire (ex: modules plus tard)
