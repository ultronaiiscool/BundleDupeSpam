cmake_minimum_required(VERSION 3.20)
project(BundleDupeSpam LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(UNICODE _UNICODE)

include(FetchContent)

# Fetch MinHook automatically.
FetchContent_Declare(
    minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

# Dear ImGui does not provide a CMake target itself, so we fetch its sources
# and compile the Win32 + DX11 backends directly into this DLL.
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(minhook imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

add_library(BundleDupeSpam SHARED
    src/dllmain.cpp
    src/hooks.cpp
    src/packet.cpp
    src/gui.cpp
    src/memory.cpp
    ${IMGUI_SOURCES}
)

target_include_directories(BundleDupeSpam PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(BundleDupeSpam PRIVATE
    minhook
    d3d11
    dxgi
    dwmapi
    psapi
)

if(MSVC)
    target_compile_options(BundleDupeSpam PRIVATE /W4 /permissive-)
endif()

set_target_properties(BundleDupeSpam PROPERTIES
    OUTPUT_NAME "BundleDupeSpam"
    PREFIX ""
)
