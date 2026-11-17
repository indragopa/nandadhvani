cmake_minimum_required(VERSION 3.20)
project(NandaDhvani VERSION 0.1.0 LANGUAGES CXX)

option(ND_BUILD_UI "Build the Qt 6 desktop application when Qt is available" ON)
option(ND_BUILD_TESTS "Build core unit tests" ON)
option(ND_ENABLE_WHISPER "Enable optional whisper.cpp integration (OFF by default)" OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ndcore
  core/src/audio_buffer.cpp
  core/src/edl.cpp
  core/src/wav.cpp
  core/src/dsp.cpp
  core/src/alignment.cpp
  core/src/script.cpp
  core/src/project.cpp
  core/src/recording.cpp
)
target_include_directories(ndcore PUBLIC core/include)
target_compile_features(ndcore PUBLIC cxx_std_20)
if(MSVC)
  target_compile_options(ndcore PRIVATE /W4 /permissive- /utf-8)
else()
  target_compile_options(ndcore PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
endif()

if(ND_BUILD_TESTS)
  enable_testing()
  add_executable(nd_tests tests/test_main.cpp)
  target_link_libraries(nd_tests PRIVATE ndcore)
  add_test(NAME core_tests COMMAND nd_tests)
endif()

if(ND_BUILD_UI)
  find_package(Qt6 QUIET COMPONENTS Widgets)
  if(Qt6_FOUND)
    set(CMAKE_AUTOMOC ON)
    qt_add_executable(NandaDhvani WIN32 ui/src/main.cpp ui/src/main_window.cpp ui/src/waveform_widget.cpp ui/include/main_window.hpp ui/include/waveform_widget.hpp)
    target_include_directories(NandaDhvani PRIVATE ui/include)
    target_link_libraries(NandaDhvani PRIVATE ndcore Qt6::Widgets)
    target_compile_definitions(NandaDhvani PRIVATE ND_VERSION="${PROJECT_VERSION}")
    if(MSVC)
      target_compile_options(NandaDhvani PRIVATE /W4 /permissive- /utf-8)
    else()
      target_compile_options(NandaDhvani PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    if(WIN32)
      target_link_libraries(NandaDhvani PRIVATE ole32)
    endif()
  else()
    message(STATUS "Qt 6 Widgets not found: building core and tests only")
  endif()
endif()

install(TARGETS ndcore ARCHIVE DESTINATION lib)
if(TARGET NandaDhvani)
  install(TARGETS NandaDhvani RUNTIME DESTINATION .)
endif()
