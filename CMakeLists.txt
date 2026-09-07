cmake_minimum_required(VERSION 3.20)
project(minesweeper LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
set(SOURCES
    src/main.cpp
    src/app.cpp
    src/audio/voice_manager.cpp
    src/core/board.cpp
    src/core/save_manager.cpp
    src/core/ship.cpp
    src/net/network_manager.cpp
    src/net/steam_manager.cpp
    src/render/asset_manager.cpp
    src/render/camera_controller.cpp
    src/render/particles.cpp
    src/render/raylib_renderer.cpp
    src/render/scrap_system.cpp
    src/ui/widgets.cpp
    src/ui/hud.cpp
    src/ui/menu.cpp
)

add_executable(minesweeper ${SOURCES})

# Target Include directories
target_include_directories(minesweeper PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/miniaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamworks/public/steam
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamworks/public
)

# Compiler flags
if(MSVC)
    target_compile_options(minesweeper PRIVATE /W4 /EHsc /utf-8 /std:c++20)
    target_compile_definitions(minesweeper PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(minesweeper PRIVATE -Wall -Wextra -std=c++20)
endif()

# ==========================================
# macOS (Darwin) Configuration
# ==========================================
if(APPLE)
    message(STATUS "Configuring for macOS (Darwin)...")

    # Support Homebrew paths for both Apple Silicon (/opt/homebrew) and Intel (/usr/local)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")

    # Find Raylib
    find_package(raylib QUIET)
    if(NOT raylib_FOUND)
        find_library(RAYLIB_LIB raylib PATHS /opt/homebrew/lib /usr/local/lib)
        find_path(RAYLIB_INCLUDE raylib.h PATHS /opt/homebrew/include /usr/local/include)
        if(RAYLIB_LIB AND RAYLIB_INCLUDE)
            target_include_directories(minesweeper PRIVATE ${RAYLIB_INCLUDE})
            target_link_libraries(minesweeper PRIVATE ${RAYLIB_LIB})
            message(STATUS "Found Raylib: ${RAYLIB_LIB}")
        else()
            message(FATAL_ERROR "raylib not found! Please run: brew install raylib")
        endif()
    else()
        target_link_libraries(minesweeper PRIVATE raylib)
    endif()

    # Find ENet
    find_library(ENET_LIB enet PATHS /opt/homebrew/lib /usr/local/lib)
    find_path(ENET_INCLUDE enet/enet.h PATHS /opt/homebrew/include /usr/local/include)
    if(ENET_LIB AND ENET_INCLUDE)
        target_include_directories(minesweeper PRIVATE ${ENET_INCLUDE})
        target_link_libraries(minesweeper PRIVATE ${ENET_LIB})
        message(STATUS "Found ENet: ${ENET_LIB}")
    else()
        message(FATAL_ERROR "enet not found! Please run: brew install enet")
    endif()

    # Required macOS system frameworks
    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(IOKIT_FRAMEWORK IOKit REQUIRED)
    find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
    find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
    target_link_libraries(minesweeper PRIVATE
        ${COCOA_FRAMEWORK}
        ${IOKIT_FRAMEWORK}
        ${COREVIDEO_FRAMEWORK}
        ${OPENGL_FRAMEWORK}
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOTOOLBOX_FRAMEWORK}
    )

    # Steamworks macOS dylib
    set(STEAM_OSX_DYLIB "${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamworks/redistributable_bin/osx/libsteam_api.dylib")
    if(EXISTS "${STEAM_OSX_DYLIB}")
        target_link_libraries(minesweeper PRIVATE "${STEAM_OSX_DYLIB}")
        add_custom_command(TARGET minesweeper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${STEAM_OSX_DYLIB}"
                "$<TARGET_FILE_DIR:minesweeper>/libsteam_api.dylib"
        )
    endif()

    # Set rpath so executable looks for dylibs in same directory
    set_target_properties(minesweeper PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "@executable_path"
    )

    # Copy assets & steam_appid.txt
    add_custom_command(TARGET minesweeper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:minesweeper>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt"
            "$<TARGET_FILE_DIR:minesweeper>/steam_appid.txt"
    )

# ==========================================
# Windows Configuration
# ==========================================
elseif(WIN32)
    message(STATUS "Configuring for Windows...")

    target_include_directories(minesweeper PRIVATE
        "C:/raylib/raylib/src"
        "C:/Users/nucit/vcpkg/installed/x64-windows/include"
    )

    target_link_directories(minesweeper PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        "C:/raylib/raylib/src"
        "C:/Users/nucit/vcpkg/installed/x64-windows/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamworks/redistributable_bin/win64"
    )

    target_link_libraries(minesweeper PRIVATE
        raylib
        enet
        steam_api64
        ws2_32
        winmm
    )

    add_custom_command(TARGET minesweeper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:minesweeper>/assets
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/raylib.dll
            $<TARGET_FILE_DIR:minesweeper>/raylib.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamworks/redistributable_bin/win64/steam_api64.dll
            $<TARGET_FILE_DIR:minesweeper>/steam_api64.dll
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt
            $<TARGET_FILE_DIR:minesweeper>/steam_appid.txt
    )

# ==========================================
# Linux Configuration
# ==========================================
else()
    message(STATUS "Configuring for Linux...")

    find_package(raylib REQUIRED)
    find_library(ENET_LIB enet REQUIRED)
    target_link_libraries(minesweeper PRIVATE raylib ${ENET_LIB} pthread dl)

    set(STEAM_LINUX_SO "${CMAKE_CURRENT_SOURCE_DIR}/third_party/steamworks/redistributable_bin/linux64/libsteam_api.so")
    if(EXISTS "${STEAM_LINUX_SO}")
        target_link_libraries(minesweeper PRIVATE "${STEAM_LINUX_SO}")
        add_custom_command(TARGET minesweeper POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${STEAM_LINUX_SO}"
                "$<TARGET_FILE_DIR:minesweeper>/libsteam_api.so"
        )
    endif()

    add_custom_command(TARGET minesweeper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:minesweeper>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/steam_appid.txt"
            "$<TARGET_FILE_DIR:minesweeper>/steam_appid.txt"
    )
endif()
