# 1. 指定源码文件（显式列出通常比 GLOB 更安全，尤其是在增加/删除文件时）
# 如果你的 .hpp 里没有实现代码（只有声明），则不需要加入源文件列表
# 但如果 .hpp 包含了模板实现，加入列表可以方便 IDE（如 CLion）识别关联
set(SRC_FILES
        main.cpp
        log/logger.cpp
        # 如果 util 目录下有 .cpp 文件，也要写在这里：
        # util/socket.cpp
)

# 2. 定义可执行程序目标
add_executable(webserver ${SRC_FILES})

# 3. 关键：设置头文件包含路径
# 我们把包含路径设为 ${CMAKE_CURRENT_SOURCE_DIR}，
# 这样在 main.cpp 里就可以写 #include "util/threadpool.hpp"
target_include_directories(webserver PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 如果你的项目依赖了线程库（pthread），记得加上
find_package(Threads REQUIRED)
target_link_libraries(webserver PRIVATE Threads::Threads)