#ifndef STATION_H
#define STATION_H

#include <string>
#include <vector>
#include <memory>
#include "nmcli_exception.h"
#include <sdbus-c++/sdbus-c++.h>

class Station {
  public:
    // 网络信息结构体
    struct NetworkInfo {
        std::string object_path;
        std::string ssid;
        std::string security;
        int signal_strength; // 信号强度，单位为dBm*100
        bool in_use;         // 是否正在使用
    };

    // 构造函数，通过device object path初始化
    explicit Station(const std::string &device_object_path);
    ~Station();

    // 禁止拷贝构造和赋值
    Station(const Station &) = delete;
    Station &operator=(const Station &) = delete;

    // Station相关方法
    bool scan();                                   // 扫描网络
    bool disconnect();                             // 断开连接
    std::vector<NetworkInfo> getOrderedNetworks(); // 获取排序后的网络列表

    // 属性获取方法
    std::string getState() const;            // 获取连接状态
    std::string getConnectedNetwork() const; // 获取当前连接的网络对象路径
    bool isScanning() const;                 // 是否正在扫描
    std::string getDeviceName() const;       // 获取设备名称

    std::vector<std::string> getAllConnection();

    template <typename T> T getProperty(const std::string &interface, const std::string &property) const {
        // 检查D-Bus连接和代理是否已初始化
        if (!connection_ || !stationProxy_) {
            throw std::runtime_error("D-Bus connection or station proxy not initialized");
        }

        // 使用已有的Station代理获取属性值
        const sdbus::Variant result = stationProxy_->getProperty(property).onInterface(interface);

        // 根据类型返回相应的值
        if (result.containsValueOfType<T>()) {
            return result.get<T>();
        }

        // 如果类型不匹配，返回默认构造的值
        return T{};
    }

    // 从ObjectPath获取属性的通用模板方法
    template <typename T>
    T getPropertyFromObjectPath(
        const sdbus::ObjectPath &objectPath, const std::string &interface, const std::string &property
    ) const {
        // 检查D-Bus连接是否已初始化
        if (!connection_) {
            throw std::runtime_error("D-Bus connection not initialized");
        }

        // 使用智能指针创建代理
        auto proxy = sdbus::createProxy(*connection_, sdbus::ServiceName{"net.connman.iwd"}, objectPath);

        // 获取属性值
        const sdbus::Variant result = proxy->getProperty(property).onInterface(interface);

        // 根据类型返回相应的值
        if (result.containsValueOfType<T>()) {
            return result.get<T>();
        }

        // 如果类型不匹配，返回默认构造的值
        return T{};
    }

    template <typename T>
    T callMethodFromObjectPath(
        const sdbus::ObjectPath &objectPath, const std::string &interface, const std::string &method
    ) const {
        // 检查D-Bus连接是否已初始化
        if (!connection_) {
            throw std::runtime_error("D-Bus connection not initialized");
        }

        // 使用智能指针创建代理
        auto proxy = sdbus::createProxy(*connection_, sdbus::ServiceName{"net.connman.iwd"}, objectPath);

        T result;
        proxy->callMethod(method).onInterface(interface).storeResultsTo(result);

        return result;
    }

    std::string device_object_path_; // 设备对象路径
    std::unique_ptr<sdbus::IConnection> connection_;
    std::unique_ptr<sdbus::IProxy> stationProxy_; // Station代理对象
};

#endif // STATION_H
