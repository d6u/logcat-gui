cmake_minimum_required(VERSION 3.19.2)

project(main VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB SOURCES
  src/*.cpp
)

add_executable(main ${SOURCES})

find_package(Curses REQUIRED)
target_include_directories(main PUBLIC $ENV{HOME}/homebrew/include)
target_link_libraries(main $ENV{HOME}/homebrew/Cellar/folly/2021.02.08.00/lib/libfolly.dylib ${CURSES_LIBRARIES})
