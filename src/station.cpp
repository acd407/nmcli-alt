#include "station.h"
#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <regex>

Station::Station(const std::string &device_object_path)
    : device_object_path_(device_object_path), connection_(sdbus::createSystemBusConnection()),
      stationProxy_(
          sdbus::createProxy(
              *connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{device_object_path_}
          )
      ) {
    // 构造函数初始化列表中直接初始化connection_和stationProxy_
}

Station::~Station() = default;

// 移除了initializeConnection方法，因为现在在构造函数中直接初始化

bool Station::scan() {
    // 检查D-Bus连接和代理是否已初始化
    if (!connection_ || !stationProxy_) {
        throw DBusException("D-Bus connection or station proxy not initialized");
    }

    try {
        // 调用Scan方法
        stationProxy_->callMethod("Scan").onInterface("net.connman.iwd.Station").dontExpectReply();
        return true;
    } catch (const sdbus::Error &e) {
        throw DBusException("D-Bus error scanning networks: " + std::string(e.what()));
    } catch (const NmcliException &e) {
        // 重新抛出自定义异常
        throw;
    } catch (const std::exception &e) {
        throw NetworkException("Error scanning networks: " + std::string(e.what()));
    }
}

bool Station::disconnect() {
    // 检查D-Bus连接和代理是否已初始化
    if (!connection_ || !stationProxy_) {
        throw DBusException("D-Bus connection or station proxy not initialized");
    }

    try {
        // 调用Disconnect方法
        stationProxy_->callMethod("Disconnect").onInterface("net.connman.iwd.Station").dontExpectReply();
        return true;
    } catch (const sdbus::Error &e) {
        throw DBusException("D-Bus error disconnecting: " + std::string(e.what()));
    } catch (const NmcliException &e) {
        // 重新抛出自定义异常
        throw;
    } catch (const std::exception &e) {
        throw NetworkException("Error disconnecting: " + std::string(e.what()));
    }
}

std::vector<Station::NetworkInfo> Station::getOrderedNetworks() {
    std::vector<NetworkInfo> networks;

    // 检查D-Bus连接是否已初始化
    if (!connection_) {
        throw DBusException("D-Bus connection not initialized");
    }

    try {
        // 创建Station代理
        auto stationProxy = sdbus::createProxy(
            *connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{device_object_path_}
        );

        // 调用D-Bus方法并直接存储到目标类型，不使用Variant中转
        std::vector<sdbus::Struct<sdbus::ObjectPath, int16_t>> networkList;

        stationProxy->callMethod("GetOrderedNetworks")
            .onInterface("net.connman.iwd.Station")
            .storeResultsTo(networkList); // 直接存储到networkList

        // 不在这里打印"Found X networks"信息，而是在调用者那里根据terse_output标志决定是否打印

        // 获取当前连接的网络
        const std::string connectedNetwork = getConnectedNetwork();

        // 预分配空间以提高性能
        networks.reserve(networkList.size());

        // 使用变换算法而不是手写循环
        for (const auto &[objPath, signalStrength] : networkList) {
            NetworkInfo info;
            info.object_path = objPath.c_str();
            info.signal_strength = signalStrength;
            info.in_use = (info.object_path == connectedNetwork);

            // 获取网络详细信息
            // 使用新的模板方法获取SSID
            info.ssid = getPropertyFromObjectPath<std::string>(
                sdbus::ObjectPath{info.object_path}, "net.connman.iwd.Network", "Name"
            );

            // 使用新的模板方法获取安全类型
            info.security = getPropertyFromObjectPath<std::string>(
                sdbus::ObjectPath{info.object_path}, "net.connman.iwd.Network", "Type"
            );

            networks.push_back(std::move(info)); // 使用移动语义
        }
    } catch (const sdbus::Error &e) {
        throw DBusException("D-Bus error getting ordered networks: " + std::string(e.what()));
    } catch (const NmcliException &e) {
        // 重新抛出自定义异常
        throw;
    } catch (const std::exception &e) {
        throw NetworkException("Error getting ordered networks: " + std::string(e.what()));
    }

    return networks;
}

std::vector<std::string> Station::getAllConnection() {
    auto iwdProxy =
        sdbus::createProxy(*connection_, sdbus::ServiceName{"net.connman.iwd"}, sdbus::ObjectPath{"/net/connman/iwd"});

    std::string introspectionData;
    iwdProxy->callMethod("Introspect")
        .onInterface("org.freedesktop.DBus.Introspectable")
        .storeResultsTo(introspectionData);

    // Parse the introspection data to find known network objects
    // Known networks are directly under /net/connman/iwd/
    std::regex nodeRegex("<node name=\"([^\"]+)\"");
    std::smatch match;
    std::string::const_iterator searchStart(introspectionData.cbegin());

    std::vector<std::string> wifiConnections;
    // Find all network nodes
    while (std::regex_search(searchStart, introspectionData.cend(), match, nodeRegex)) {
        std::string networkNodeName = match[1].str();
        searchStart = match.suffix().first;

        // Skip non-network nodes (numeric nodes are adapters/devices)
        if (std::all_of(networkNodeName.begin(), networkNodeName.end(), ::isdigit)) {
            continue;
        }

        std::string networkPath = "/net/connman/iwd/" + networkNodeName;
        wifiConnections.push_back(networkPath);
    }
    return wifiConnections;
}

std::string Station::getState() const {
    return getProperty<std::string>("net.connman.iwd.Station", "State");
}

std::string Station::getConnectedNetwork() const {
    return getProperty<sdbus::ObjectPath>("net.connman.iwd.Station", "ConnectedNetwork").c_str();
}

bool Station::isScanning() const {
    return getProperty<bool>("net.connman.iwd.Station", "Scanning");
}

std::string Station::getDeviceName() const {
    return getPropertyFromObjectPath<std::string>(
        sdbus::ObjectPath{device_object_path_}, "net.connman.iwd.Device", "Name"
    );
}
