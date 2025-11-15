#ifndef NMCLI_EXCEPTION_H
#define NMCLI_EXCEPTION_H

#include <stdexcept>
#include <string>

/**
 * nmcli 自定义异常基类
 */
class NmcliException : public std::runtime_error {
public:
    explicit NmcliException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * 网络相关异常
 */
class NetworkException : public NmcliException {
public:
    explicit NetworkException(const std::string& message) : NmcliException(message) {}
};

/**
 * D-Bus 通信异常
 */
class DBusException : public NmcliException {
public:
    explicit DBusException(const std::string& message) : NmcliException(message) {}
};

/**
 * 命令执行异常
 */
class CommandExecutionException : public NmcliException {
public:
    explicit CommandExecutionException(const std::string& message) : NmcliException(message) {}
};

/**
 * 连接异常
 */
class ConnectionException : public NmcliException {
public:
    explicit ConnectionException(const std::string& message) : NmcliException(message) {}
};

#endif // NMCLI_EXCEPTION_H