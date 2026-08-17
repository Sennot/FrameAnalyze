cmake_minimum_required(VERSION 3.21)
project(FrameWindowAnalyzer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(FWA_CORE_ONLY "Build only the portable analyzer core" OFF)
option(FWA_BUILD_TESTS "Build portable core tests" OFF)

add_library(fwa_core STATIC
    src/core/FrameWindow.cpp
    src/core/NaNDLExport.cpp
    src/runtime/AnalysisSession.cpp
)
target_include_directories(fwa_core PUBLIC src)
target_compile_features(fwa_core PUBLIC cxx_std_20)

if(FWA_BUILD_TESTS)
    enable_testing()
    add_executable(fwa_core_tests tests/core_tests.cpp)
    target_link_libraries(fwa_core_tests PRIVATE fwa_core)
    add_test(NAME fwa_core_tests COMMAND fwa_core_tests)
endif()

if(NOT FWA_CORE_ONLY)
    if(NOT DEFINED ENV{GEODE_SDK})
        message(FATAL_ERROR "GEODE_SDK is not set. Use `geode sdk install`, set GEODE_SDK, or configure with -DFWA_CORE_ONLY=ON for portable tests.")
    endif()

    add_library(${PROJECT_NAME} SHARED
        src/main.cpp
        src/runtime/BotController.cpp
        src/runtime/FileLogger.cpp
        src/runtime/TrajectoryOverlay.cpp
        src/ui/BotMenu.cpp
        src/hooks/GameHooks.cpp
    )
    target_include_directories(${PROJECT_NAME} PRIVATE src)
    target_link_libraries(${PROJECT_NAME} PRIVATE fwa_core)

    add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
    setup_geode_mod(${PROJECT_NAME})
endif()
