cmake_minimum_required(VERSION 3.15)
project(BundleDupeSpam)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

# MinHook
add_subdirectory(minhook)

# ImGui sources
set(IMGUI_SOURCES
    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_impl_win32.cpp
    imgui/imgui_impl_dx11.cpp
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
    include
    imgui
    minhook/include
)

target_link_libraries(BundleDupeSpam PRIVATE
    minhook
    d3d11
    dxgi
    dwmapi
)

set_target_properties(BundleDupeSpam PROPERTIES
    OUTPUT_NAME "BundleDupeSpam"
    PREFIX ""
)
