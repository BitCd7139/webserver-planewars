cmake_minimum_required(VERSION 3.10)
project(webserver LANGUAGES C CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 允许 src 目录被访问
add_subdirectory(src)

# 如果以后要加测试
# enable_testing()
# add_subdirectory(test)