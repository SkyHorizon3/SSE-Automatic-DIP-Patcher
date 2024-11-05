cmake_minimum_required(VERSION 3.25.1)

project(ImGui)

# Add the source files for ImGui
set(IMGUI_SOURCES
    imgui.cpp
	imgui_draw.cpp
	imgui_tables.cpp
	imgui_widgets.cpp
	imgui_demo.cpp
    backends/imgui_impl_dx11.cpp
    backends/imgui_impl_win32.cpp
)

# Add the header files for ImGui
set(IMGUI_HEADERS
    imgui.h
	imconfig.h
	imgui_internal.h
	imstb_rectpack.h
	imstb_textedit.h
	imstb_truetype.h
    backends/imgui_impl_win32.h
    backends/imgui_impl_dx11.h
)

# Create the ImGui static library
add_library(ImGui STATIC ${IMGUI_SOURCES} ${IMGUI_HEADERS})

# Add include directories for ImGui
target_include_directories(ImGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
