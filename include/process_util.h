#ifndef PROCESS_UTIL_H
#define PROCESS_UTIL_H

#include <string>
#include <vector>

class ProcessUtil {
public:
    /**
     * 执行命令并等待其完成
     * @param command 要执行的命令
     * @param args 命令参数
     * @return 命令的退出码，-1表示执行失败
     */
    static int executeCommand(const std::string& command, const std::vector<std::string>& args);
};

#endif // PROCESS_UTIL_H