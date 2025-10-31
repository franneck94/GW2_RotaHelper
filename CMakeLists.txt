cmake_minimum_required(VERSION 3.20)

project(GW2RotaHelper_Addon)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.80
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(nlohmann_json)

add_library(third_party_libs STATIC)
target_include_directories(third_party_libs PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
file(GLOB IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/*.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_sources(third_party_libs PRIVATE ${IMGUI_SOURCES})

add_library(nexus INTERFACE
    "nexus/Nexus.h"
)
add_library(mumble INTERFACE
    "mumble/Mumble.h"
)
add_library(rtapi INTERFACE
    "rtapi/RTAPI.hpp"
)
add_library(arcdps INTERFACE
    "arcdps/ArcDPS.h"
)

target_link_libraries(third_party_libs PRIVATE
    d3d11
    d3dcompiler
    comdlg32
    nexus
    mumble
    rtapi
    arcdps
    nlohmann_json::nlohmann_json
)
target_include_directories(third_party_libs PUBLIC
    "."
)

add_library(first_party_libs STATIC
    "src/Shared.cpp"
    "src/Settings.cpp"
    "src/Render.cpp"
    "src/LogData.cpp"
    "src/resource.rc"
    "src/ArcEvents.cpp"
    "src/Textures.cpp"
    "src/FileUtils.cpp"
    "src/MumbleUtils.cpp"
    $<$<CONFIG:Debug>:src/MemoryReader.cpp>
)
set_source_files_properties("src/resource.rc" PROPERTIES LANGUAGE RC)
target_include_directories(first_party_libs PUBLIC
    "src"
)
target_link_libraries(first_party_libs PRIVATE
    third_party_libs
)

add_executable(GW2RotaHelper_App
    "src/main.cpp"
)
target_compile_definitions(GW2RotaHelper_App PRIVATE GW2_STANDALONE_APP)
target_link_libraries(GW2RotaHelper_App PRIVATE
    third_party_libs
    first_party_libs
)

add_library(GW2RotaHelper SHARED
    "src/entry.cpp"
)
target_compile_definitions(GW2RotaHelper PUBLIC GW2_NEXUS_ADDON)
target_link_libraries(GW2RotaHelper PRIVATE
    third_party_libs
    first_party_libs
)
set_target_properties(GW2RotaHelper PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/debug"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/release"
)
add_custom_command(TARGET GW2RotaHelper POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
        "D:/GW2/addons" || (exit 0)
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
        "D:/GW2_2/addons" || (exit 0)
    COMMENT "Copying binary files to GW2 addons directory"
)
