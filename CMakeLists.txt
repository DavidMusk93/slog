cmake_minimum_required(VERSION 3.12)
project(slog CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS
    ON
    CACHE INTERNAL "")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wextra")

option(SLOG_USE_ASAN "Build with address sanitizer" OFF)
if(SLOG_USE_ASAN)
  set(CMAKE_CXX_FLAGS
      "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer"
  )
endif()

add_executable(config_test config_test.cpp config.cc)

include(CTest)
add_test(NAME config_test COMMAND config_test)
