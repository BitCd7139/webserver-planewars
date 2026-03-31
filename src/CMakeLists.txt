file(GLOB SRC_FILES
        "*.cpp"
        "log/*.cpp"
        "server/*.cpp"
        "buffer/*.cpp"
        "http/*.cpp"
        "timer/*.cpp"
)

# 2. 定义可执行程序目标
add_executable(webserver ${SRC_FILES})

# 3. 关键：设置头文件包含路径
# 我们把包含路径设为 ${CMAKE_CURRENT_SOURCE_DIR}，
# 这样在 main.cpp 里就可以写 #include "util/threadpool.h"
target_include_directories(webserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(Threads REQUIRED)
target_link_libraries(webserver PRIVATE Threads::Threads)