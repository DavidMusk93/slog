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

add_executable(fmt_test fmt_test.cpp str.cc)
target_link_libraries(fmt_test pthread)

add_executable(log_test log_test.cpp log.cc file.cc str.cc config.cc)
target_link_libraries(log_test pthread)

add_executable(stat_test stat_test.cpp str.cc)
target_link_libraries(stat_test pthread)

add_executable(redirect_test redirect_test.cpp log.cc file.cc str.cc config.cc)
target_link_libraries(redirect_test pthread)

add_executable(redirect2_test redirect2_test.cpp log.cc file.cc str.cc
                              config.cc)
target_link_libraries(redirect2_test pthread)

add_executable(dl_test dl_test.cpp)

add_executable(rotate_stdout redirect3_test.cpp log.cc file.cc str.cc config.cc)
target_link_libraries(rotate_stdout pthread)

include(CTest)
add_test(NAME config_test COMMAND config_test)
add_test(NAME fmt_test COMMAND fmt_test)
add_test(NAME dl_test COMMAND dl_test)
