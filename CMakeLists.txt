cmake_minimum_required(VERSION 3.0.0)
project(tflitec_app)

# include packages
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

set(VENDOR_DIR ${CMAKE_SOURCE_DIR}/vendor)

set(tflitec_DIR ${VENDOR_DIR}/tflite_c)
set(imgui_docking_DIR ${VENDOR_DIR}/imgui_docking)
find_package(tflitec CONFIG REQUIRED)
find_package(imgui_docking CONFIG REQUIRED)

# define our target
set(CMAKE_CXX_STANDARD 17)

set(SRC_FILES
    src/main.cpp 
    # neural network
    src/model.cpp
    # app
    src/App.cpp src/AppTracer.cpp
    src/gui.cpp
    src/Texture.cpp
    src/wordblitz_models.cpp
    src/unified_model.cpp
    src/buffer_graphics.cpp
    # wordblitz 
    src/wordblitz.cpp
    src/wordtree.cpp
    # utility
    vendor/util/MSS.cpp    
    vendor/util/KeyListener.cpp
    vendor/util/AutoGui.cpp)

add_executable(main ${SRC_FILES})
include_directories(main ${VENDOR_DIR})
target_link_libraries(main PRIVATE 
    tflitec imgui_docking 
    fmt::fmt
    spdlog::spdlog spdlog::spdlog_header_only
    "d3d11.lib" "dxgi.lib" "d3dcompiler.lib") 
target_compile_options(main PRIVATE "/MP")

# vcpkg.cmake has internal stuff that autogenerates this
# we have to do this manually
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    add_custom_command(TARGET main 
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy 
        ${tflitec_DIR}/bin/x64/Release/tensorflowlite_c.dll
        $<TARGET_FILE_DIR:main>)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    add_custom_command(TARGET main 
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy 
        ${tflitec_DIR}/bin/x86/Release/tensorflowlite_c.dll
        $<TARGET_FILE_DIR:main>)
endif()
