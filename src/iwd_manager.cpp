#include "iwd_manager.h"
#include "station.h"
#include "process_util.h"

#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <regex>
#include <memory>

IwdManager::IwdManager() : connection_(sdbus::createSystemBusConnection()) {
    // 构造函数初始化列表中直接创建D-Bus连接
    // connection_会自动管理资源，无需手动释放
}

IwdManager::~IwdManager() = default;

bool IwdManager::connectToNetwork(const std::string &ssid, const std::string &password) {
    try {
        // 尝试使用 D-Bus API 连接网络
        if (!connectToNetworkViaDBus(ssid, password)) {
            // 如果 D-Bus 方法失败，回退到 iwctl 命令行工具
            std::cerr << "Falling back to iwctl method..." << std::endl;
            return connectToNetworkViaIWCTL(ssid, password);
        }
        return true;
    } catch (const NmcliException &e) {
        // 重新抛出自定义异常
        throw;
    } catch (const std::exception &e) {
        // 将标准异常转换为自定义异常
        throw NetworkException("Error connecting to network: " + std::string(e.what()));
    }
}

std::string IwdManager::getAdapterObjectPath() {
    // 根据iwd文档，Adapter的Object Path格式为/net/connman/iwd/{phy0,phy1,...}
    // 使用sdbus-c++简化实现

    // 检查D-Bus连接是否已初始化
    if (!connection_) {
        std::cerr << "D-Bus connection not initialized" << std::endl;
        return "";
    }

    try {
        // 直接尝试获取iwd服务的内省数据
        std::string introspectionData;
        // 使用智能指针创建iwd服务代理对象
        auto iwdProxy = sdbus::createProxy(
            *connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{"/net/connman/iwd"}
        );

        iwdProxy->callMethod("Introspect")
            .onInterface("org.freedesktop.DBus.Introspectable")
            .storeResultsTo(introspectionData);

        // 使用正则表达式查找第一个数字命名的节点（通常是phy索引）
        std::regex nodeRegex("<node name=\"(\\d+)\"");
        std::smatch match;

        if (std::regex_search(introspectionData, match, nodeRegex) && match.size() > 1) {
            return "/net/connman/iwd/" + match.str(1);
        }

        // 如果没找到数字命名的节点，尝试查找phy命名的节点
        std::regex phyRegex("<node name=\"(phy\\d+)\"");
        if (std::regex_search(introspectionData, match, phyRegex) && match.size() > 1) {
            return "/net/connman/iwd/" + match.str(1);
        }

        // 如果通过解析XML没有找到Adapter，返回默认的phy0
        return "/net/connman/iwd/0";

    } catch (const sdbus::Error &e) {
        std::cerr << "Failed to get adapter object path: " << e.what() << std::endl;
        // 返回默认路径作为后备
        return "/net/connman/iwd/0";
    }
}

std::string IwdManager::getDeviceObjectPath() {
    // 根据iwd文档，Device的Object Path格式为/net/connman/iwd/{phyX}/{deviceIndex}
    // 其中phyX是adapter路径的一部分，deviceIndex是设备索引

    // 检查D-Bus连接是否已初始化
    if (!connection_) {
        std::cerr << "D-Bus connection not initialized" << std::endl;
        return "";
    }

    try {
        // 首先获取adapter object path
        std::string adapterPath = getAdapterObjectPath();
        if (adapterPath.empty()) {
            std::cerr << "Failed to get adapter object path" << std::endl;
            return "";
        }

        // 获取adapter的内省数据以查找其下的设备
        std::string introspectionData;
        // 使用智能指针创建adapter代理对象
        auto adapterProxy =
            sdbus::createProxy(*connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{adapterPath});

        adapterProxy->callMethod("Introspect")
            .onInterface("org.freedesktop.DBus.Introspectable")
            .storeResultsTo(introspectionData);

        // 使用正则表达式查找设备节点（通常是数字）
        std::regex deviceRegex("<node name=\"(\\d+)\"");
        std::smatch match;

        if (std::regex_search(introspectionData, match, deviceRegex) && match.size() > 1) {
            return adapterPath + "/" + match.str(1);
        }

        // 如果没找到数字命名的节点，尝试其他可能的命名方式
        // 在实际实现中，我们可能需要更复杂的逻辑来查找设备

        // 作为后备方案，返回一个常见的设备路径
        return adapterPath + "/1";

    } catch (const sdbus::Error &e) {
        std::cerr << "Failed to get device object path: " << e.what() << std::endl;
        // 返回空字符串表示失败
        return "";
    }
}

std::unique_ptr<Station> IwdManager::createStation() {
    // 获取设备对象路径
    std::string devicePath = getDeviceObjectPath();
    if (devicePath.empty()) {
        std::cerr << "Failed to get device object path for Station" << std::endl;
        return nullptr;
    }

    try {
        // 使用智能指针创建Station对象
        return std::make_unique<Station>(devicePath);
    } catch (const std::exception &e) {
        std::cerr << "Failed to create Station: " << e.what() << std::endl;
        return nullptr;
    }
}

bool IwdManager::connectToNetworkViaDBus(const std::string &ssid, const std::string &password) {
    // 检查D-Bus连接是否已初始化
    if (!connection_) {
        throw DBusException("D-Bus connection not initialized");
    }

    try {
        // 创建Station对象
        auto station = createStation();
        if (!station) {
            throw NetworkException("Failed to create station");
        }

        // 扫描网络
        if (!station->scan()) {
            throw NetworkException("Failed to scan networks");
        }

        // 获取扫描结果
        auto networks = station->getOrderedNetworks();

        // 查找指定的网络
        std::string networkObjectPath;
        for (const auto &network : networks) {
            if (network.ssid == ssid) {
                networkObjectPath = network.object_path;
                break;
            }
        }

        if (networkObjectPath.empty()) {
            throw NetworkException("Network '" + ssid + "' not found");
        }

        // 创建网络代理对象
        auto networkProxy = sdbus::createProxy(
            *connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{networkObjectPath}
        );

        // 调用Connect方法连接网络
        networkProxy->callMethod("Connect").onInterface("net.connman.iwd.Network");

        return true;
    } catch (const sdbus::Error &e) {
        throw DBusException("D-Bus error connecting to network: " + std::string(e.what()));
    } catch (const NmcliException &e) {
        // 重新抛出自定义异常
        throw;
    } catch (const std::exception &e) {
        throw NetworkException("Error connecting to network: " + std::string(e.what()));
    }
}

bool IwdManager::connectToNetworkViaIWCTL(const std::string &ssid, const std::string &password) {
    try {
        // 使用自定义的 ProcessUtil 安全地执行 iwctl 命令
        std::vector<std::string> args;
        args.push_back("station");
        
        // 动态获取设备名称而不是硬编码"wlan0"
        auto station = createStation();
        if (!station) {
            throw CommandExecutionException("Failed to create station instance");
        }
        
        std::string deviceName = station->getDeviceName();
        if (deviceName.empty()) {
            throw CommandExecutionException("Failed to get device name");
        }
        
        args.push_back(deviceName);
        args.push_back("connect");
        args.push_back(ssid);

        // 如果提供了密码，则通过 --passphrase 参数传递
        if (!password.empty()) {
            args.push_back("--passphrase");
            args.push_back(password);
        }

        // 执行命令
        int result = ProcessUtil::executeCommand("iwctl", args);

        if (result == 0) {
            return true;
        } else {
            throw CommandExecutionException("Failed to connect to network '" + ssid + "' via iwctl");
        }
    } catch (const NmcliException &e) {
        // 重新抛出自定义异常
        throw;
    } catch (const std::exception &e) {
        throw CommandExecutionException("Error executing iwctl command: " + std::string(e.what()));
    }
}

bool IwdManager::getWifiRadioState() {
    // 检查D-Bus连接是否已初始化
    if (!connection_) {
        std::cerr << "D-Bus connection not initialized" << std::endl;
        return false;
    }

    try {
        // 获取适配器对象路径
        std::string adapterPath = getAdapterObjectPath();
        if (adapterPath.empty()) {
            std::cerr << "Failed to get adapter object path" << std::endl;
            return false;
        }

        // 创建适配器代理对象
        auto adapterProxy =
            sdbus::createProxy(*connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{adapterPath});

        // 获取Powered属性
        bool powered = adapterProxy->getProperty("Powered").onInterface("net.connman.iwd.Adapter").get<bool>();

        return powered;
    } catch (const sdbus::Error &e) {
        std::cerr << "Failed to get WiFi radio state: " << e.what() << std::endl;
        return false;
    }
}

bool IwdManager::setWifiRadioState(bool enabled) {
    // 检查D-Bus连接是否已初始化
    if (!connection_) {
        std::cerr << "D-Bus connection not initialized" << std::endl;
        return false;
    }

    try {
        // 获取适配器对象路径
        std::string adapterPath = getAdapterObjectPath();
        if (adapterPath.empty()) {
            std::cerr << "Failed to get adapter object path" << std::endl;
            return false;
        }

        // 创建适配器代理对象
        auto adapterProxy =
            sdbus::createProxy(*connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{adapterPath});

        // 设置Powered属性
        adapterProxy->setProperty("Powered").onInterface("net.connman.iwd.Adapter").toValue(enabled);

        return true;
    } catch (const sdbus::Error &e) {
        std::cerr << "Failed to set WiFi radio state via D-Bus: " << e.what() << std::endl;
        return false;
    }
}