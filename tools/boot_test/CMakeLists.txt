cmake_minimum_required(VERSION 3.16)
project(boot_test CXX C)

# Headless runner built on the melonDS emulation core (GPLv3).
# Point MELONDS_DIR at a melonDS checkout with melonds-capture.patch applied
# (tools/setup.sh does this), or let CMake fetch the pinned commit and patch it.
set(MELONDS_DIR "" CACHE PATH "Path to a melonDS source checkout")
set(MELONDS_COMMIT 906e9eb CACHE STRING "melonDS commit to fetch when MELONDS_DIR is empty")

set(BUILD_QT_SDL OFF CACHE BOOL "" FORCE)
set(ENABLE_OGLRENDERER OFF CACHE BOOL "" FORCE)
set(ENABLE_GDBSTUB OFF CACHE BOOL "" FORCE)
set(ENABLE_LTO_RELEASE OFF CACHE BOOL "" FORCE)

if (MELONDS_DIR)
    add_subdirectory(${MELONDS_DIR} melonDS EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(melonDS
        GIT_REPOSITORY https://github.com/melonDS-emu/melonDS.git
        GIT_TAG ${MELONDS_COMMIT}
        PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/melonds-capture.patch)
    FetchContent_GetProperties(melonDS)
    if (NOT melonds_POPULATED)
        FetchContent_Populate(melonDS)
        add_subdirectory(${melonds_SOURCE_DIR} ${melonds_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
endif()

add_executable(boot_test boot_test.cpp)
target_link_libraries(boot_test PRIVATE core)
set_target_properties(boot_test PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
target_link_libraries(boot_test PRIVATE Threads::Threads ZLIB::ZLIB)
