#ifndef WEBSERVER_GET_ROOT_H
#define WEBSERVER_GET_ROOT_H

#include <filesystem>
#include <unistd.h>
#include <linux/limits.h>
#include <string>
#include "log/logger.h"

namespace webserver {
    inline std::string GetProjectRoot() {
        char buf[PATH_MAX];
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len != -1) {
            buf[len] = '\0';
            std::filesystem::path exe_path(buf);

            // cwd: /tmp/.../webserver/bin/webserver
            // exe_path.parent_path(): /tmp/.../webserver/bin
            // parent_path()*2: /tmp/.../webserver (root)
            return exe_path.parent_path().parent_path().string();
        }
        LOG_WARN("Failed to read /proc/self/exe, fallback to current path");
        return std::filesystem::current_path().string();
    }

}

#endif //WEBSERVER_GET_ROOT_H