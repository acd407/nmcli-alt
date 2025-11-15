#include "process_util.h"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <sstream>

int ProcessUtil::executeCommand(const std::string& command, const std::vector<std::string>& args) {
    // 创建子进程
    pid_t pid = fork();
    
    if (pid == -1) {
        // fork失败
        std::cerr << "Failed to fork process" << std::endl;
        return -1;
    } else if (pid == 0) {
        // 子进程
        
        // 构建参数数组
        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(command.c_str()));
        
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        
        // 参数数组必须以nullptr结尾
        argv.push_back(nullptr);
        
        // 执行命令
        execvp(command.c_str(), argv.data());
        
        // 如果execvp返回，说明执行失败
        std::cerr << "Failed to execute command: " << command << std::endl;
        _exit(1);
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            // 正常退出
            return WEXITSTATUS(status);
        } else {
            // 异常退出
            std::cerr << "Command exited abnormally" << std::endl;
            return -1;
        }
    }
}