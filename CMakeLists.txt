cmake_minimum_required(VERSION 3.21)
project(FWBot VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FWBOT_CORE_ONLY "Build only the portable analyzer core" OFF)
option(FWBOT_BUILD_TESTS "Build portable core tests" OFF)

# Backward-compatible flags used by the v0.1 workflow/readme.
if(DEFINED FWA_CORE_ONLY)
    set(FWBOT_CORE_ONLY ${FWA_CORE_ONLY})
endif()
if(DEFINED FWA_BUILD_TESTS)
    set(FWBOT_BUILD_TESTS ${FWA_BUILD_TESTS})
endif()

add_library(fwa_core STATIC
    src/core/FrameWindow.cpp
    src/core/NaNDLExport.cpp
    src/runtime/AnalysisSession.cpp
)
target_include_directories(fwa_core PUBLIC src)
target_compile_features(fwa_core PUBLIC cxx_std_23)

if(FWBOT_BUILD_TESTS)
    enable_testing()
    add_executable(fwa_core_tests tests/core_tests.cpp)
    target_link_libraries(fwa_core_tests PRIVATE fwa_core)
    add_test(NAME fwa_core_tests COMMAND fwa_core_tests)
endif()

if(NOT FWBOT_CORE_ONLY)
    if(NOT DEFINED ENV{GEODE_SDK})
        message(FATAL_ERROR "GEODE_SDK is not set. Install Geode SDK or configure with -DFWBOT_CORE_ONLY=ON.")
    endif()

    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.92.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(fwbot_imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    target_include_directories(fwbot_imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_compile_definitions(fwbot_imgui PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

    add_library(${PROJECT_NAME} SHARED
        src/main.cpp
        src/runtime/BotController.cpp
        src/runtime/FileLogger.cpp
        src/runtime/PracticeAnchor.cpp
        src/runtime/Speedhack.cpp
        src/runtime/TrajectoryOverlay.cpp
        src/ui/ImGuiOverlay.cpp
        src/hooks/GameHooks.cpp
    )
    target_include_directories(${PROJECT_NAME} PRIVATE src)
    # setup_geode_mod uses the plain target_link_libraries signature, so keep this plain too.
    target_link_libraries(${PROJECT_NAME} fwa_core fwbot_imgui opengl32 user32 gdi32 imm32)

    add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
    setup_geode_mod(${PROJECT_NAME})
endif()
