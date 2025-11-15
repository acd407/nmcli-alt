#include <network_manager.h>
#include <iwd_manager.h>
#include <station.h>
#include <netlink/netlink.h>
#include <netlink/route/route.h>
#include <netlink/route/link.h>
#include <netlink/route/addr.h>
#include <netlink/route/rtnl.h>
#include <linux/rtnetlink.h>
#include <iostream>
#include <algorithm>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <string>
#include <sstream>
#include <iomanip>
#include <functional>
#include <regex>

static std::string stringToUUID(const std::string &input) {
    // 使用多个哈希函数模拟MD5的128位输出
    size_t hash1 = std::hash<std::string>{}(input);
    size_t hash2 = std::hash<std::string>{}(input + "_salt1");
    size_t hash3 = std::hash<std::string>{}(input + "_salt2");
    size_t hash4 = std::hash<std::string>{}(input + "_salt3");

    // 组合成16字节（128位）数据
    unsigned char digest[16];
    for (int i = 0; i < 8; i++) {
        digest[i] = (hash1 >> (i * 8)) & 0xFF;
    }
    for (int i = 0; i < 8; i++) {
        digest[i + 8] = (hash2 >> (i * 8)) & 0xFF;
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0');

    // 格式化为UUID
    for (int i = 0; i < 4; i++) {
        ss << std::setw(2) << static_cast<unsigned>(digest[i]);
    }
    ss << "-";

    for (int i = 4; i < 6; i++) {
        ss << std::setw(2) << static_cast<unsigned>(digest[i]);
    }
    ss << "-";

    // 设置版本4
    ss << "4";
    for (int i = 6; i < 8; i++) {
        ss << std::setw(2) << static_cast<unsigned>(digest[i] & 0x0F);
    }
    ss << "-";

    // 设置变体
    ss << std::setw(2) << static_cast<unsigned>((digest[8] & 0x3F) | 0x80);
    ss << std::setw(2) << static_cast<unsigned>(digest[9]);
    ss << "-";

    for (int i = 10; i < 16; i++) {
        ss << std::setw(2) << static_cast<unsigned>(digest[i]);
    }

    return ss.str();
}

NetworkManager::NetworkManager() = default;

NetworkManager::~NetworkManager() = default;

std::string NetworkManager::getConnectivity() {
    // 使用智能指针和自定义删除器管理资源
    struct SocketDeleter {
        void operator()(struct nl_sock *sock) const {
            if (sock) {
                nl_close(sock);
                nl_socket_free(sock);
            }
        }
    };

    struct CacheDeleter {
        void operator()(struct nl_cache *cache) const {
            if (cache) {
                nl_cache_put(cache);
            }
        }
    };

    // 使用unique_ptr管理netlink socket资源
    std::unique_ptr<struct nl_sock, SocketDeleter> sock(nl_socket_alloc());
    if (!sock) {
        return "unknown";
    }

    // Connect to netlink
    if (nl_connect(sock.get(), NETLINK_ROUTE) < 0) {
        return "unknown";
    }

    // 使用unique_ptr管理route cache资源
    std::unique_ptr<struct nl_cache, CacheDeleter> route_cache;
    struct nl_cache *route_cache_raw = nullptr;
    if (rtnl_route_alloc_cache(sock.get(), AF_UNSPEC, 0, &route_cache_raw) < 0) {
        return "unknown";
    }

    // 将原始指针包装到unique_ptr中
    route_cache.reset(route_cache_raw);

    // Check if there's a default route in the main table
    bool has_default_route = false;

    // 使用范围for循环替代传统的for循环
    for (struct nl_object *obj = nl_cache_get_first(route_cache.get()); obj != nullptr; obj = nl_cache_get_next(obj)) {

        struct rtnl_route *route = (struct rtnl_route *)obj;

        // Check if this route is in the main table (RT_TABLE_MAIN = 254)
        if (rtnl_route_get_table(route) != RT_TABLE_MAIN) {
            continue;
        }

        // Get destination address
        struct nl_addr *dst = rtnl_route_get_dst(route);

        // A default route has no destination or a prefix length of 0
        if (!dst || nl_addr_get_prefixlen(dst) == 0) {
            has_default_route = true;
            break;
        }
    }

    // Return connectivity status
    return has_default_route ? "full" : "none";
}

bool NetworkManager::setWifiRadio(bool enabled) {
    try {
        IwdManager iwdManager;
        return iwdManager.setWifiRadioState(enabled);
    } catch (const std::exception &e) {
        std::cerr << "Error setting WiFi radio state: " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::getWifiRadioState() {
    try {
        IwdManager iwdManager;
        return iwdManager.getWifiRadioState();
    } catch (const std::exception &e) {
        std::cerr << "Error getting WiFi radio state: " << e.what() << std::endl;
        return false;
    }
}

std::vector<NetworkManager::DeviceInfo> NetworkManager::listDevices() {
    std::vector<DeviceInfo> devices;

    // 使用在getConnectivity方法中定义的智能指针和自定义删除器管理资源
    struct SocketDeleter {
        void operator()(struct nl_sock *sock) const {
            if (sock) {
                nl_close(sock);
                nl_socket_free(sock);
            }
        }
    };

    struct CacheDeleter {
        void operator()(struct nl_cache *cache) const {
            if (cache) {
                nl_cache_free(cache);
            }
        }
    };

    // 使用unique_ptr管理netlink socket资源
    std::unique_ptr<struct nl_sock, SocketDeleter> sock(nl_socket_alloc());
    if (!sock) {
        return devices;
    }

    // Connect to netlink
    if (nl_connect(sock.get(), NETLINK_ROUTE) < 0) {
        return devices;
    }

    // 使用unique_ptr管理link cache资源
    std::unique_ptr<struct nl_cache, CacheDeleter> link_cache;
    struct nl_cache *link_cache_raw = nullptr;
    if (rtnl_link_alloc_cache(sock.get(), AF_UNSPEC, &link_cache_raw) < 0) {
        return devices;
    }

    // 将原始指针包装到unique_ptr中
    link_cache.reset(link_cache_raw);

    // 使用范围for循环替代传统的for循环
    for (struct nl_object *obj = nl_cache_get_first(link_cache.get()); obj != nullptr; obj = nl_cache_get_next(obj)) {

        struct rtnl_link *link = (struct rtnl_link *)obj;

        DeviceInfo device_info;

        // DEVICE: Device name
        const char *name = rtnl_link_get_name(link);
        device_info.name = name ? name : "unknown";

        // TYPE: Device type
        const char *type = rtnl_link_get_type(link);
        if (!type) {
            if (name && name[0] == 'l' && name[1] == 'o') {
                type = "loopback";
            } else if (name && name[0] == 'e') {
                type = "ethernet";
            } else if (name && name[0] == 'w') {
                type = "wifi";
            } else {
                type = "unknown";
            }
        }
        device_info.type = type ? type : "unknown";

        // STATE: Operational state
        uint8_t operstate = rtnl_link_get_operstate(link);
        char state_buf[32];
        rtnl_link_operstate2str(operstate, state_buf, sizeof(state_buf));
        device_info.state = state_buf;

        devices.push_back(device_info);
    }

    return devices;
}

bool NetworkManager::activateConnection(const std::string &ssid) {
    try {
        // Create IwdManager instance
        IwdManager iwdManager;

        // Try to connect to the network using IwdManager
        bool result = iwdManager.connectToNetwork(ssid);

        if (result) {
            std::cout << "Successfully connected to '" << ssid << "'" << std::endl;
            return true;
        } else {
            std::cerr << "Failed to connect to '" << ssid << "'" << std::endl;
            return false;
        }
    } catch (const sdbus::Error &e) {
        std::cerr << "D-Bus error connecting to '" << ssid << "': " << e.what() << std::endl;
        return false;
    } catch (const std::exception &e) {
        std::cerr << "Error connecting to '" << ssid << "': " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::deactivateConnection(const std::string &ssid) {
    try {
        // Create IwdManager instance
        IwdManager iwdManager;

        // Create Station instance
        auto station = iwdManager.createStation();
        if (!station) {
            std::cerr << "Failed to create Station instance" << std::endl;
            return false;
        }

        // Get the current connected network
        std::string connectedNetworkPath = station->getConnectedNetwork();

        // If not connected to any network, return success
        if (connectedNetworkPath.empty()) {
            std::cout << "Not connected to any network" << std::endl;
            return true;
        }

        // Get the SSID of the connected network
        std::string connectedSSID = station->getPropertyFromObjectPath<std::string>(
            sdbus::ObjectPath{connectedNetworkPath}, "net.connman.iwd.Network", "Name"
        );

        // Check if the connected network matches the requested SSID
        if (connectedSSID != ssid) {
            std::cerr << "Not connected to network '" << ssid << "'. Currently connected to '" << connectedSSID << "'"
                      << std::endl;
            return false;
        }

        // Disconnect from the network
        return station->disconnect();
    } catch (const std::exception &e) {
        std::cerr << "Error deactivating connection to '" << ssid << "': " << e.what() << std::endl;
        return false;
    }
}

bool NetworkManager::deleteConnection(const std::string &ssid) {
    try {
        // Create IwdManager instance
        IwdManager iwdManager;

        // Create Station instance
        auto station = iwdManager.createStation();
        if (!station) {
            std::cerr << "Failed to create Station instance" << std::endl;
            return false;
        }
        auto wifiConnections = station->getAllConnection();

        for (const auto &networkPath : wifiConnections) {
            std::string networkSSID = station->getPropertyFromObjectPath<std::string>(
                sdbus::ObjectPath{networkPath}, "net.connman.iwd.KnownNetwork", "Name"
            );

            if (networkSSID == ssid) {
                // Call the Forget method on the KnownNetwork object
                station->callMethodFromObjectPath<bool>(
                    sdbus::ObjectPath{networkPath}, "net.connman.iwd.KnownNetwork", "Forget"
                );
                std::cout << "Successfully deleted connection '" << ssid << "'" << std::endl;
                return true;
            }
        }

        std::cerr << "Network '" << ssid << "' not found in known networks" << std::endl;
        return false;
    } catch (const sdbus::Error &e) {
        std::cerr << "D-Bus error deleting connection '" << ssid << "': " << e.what() << std::endl;
        return false;
    } catch (const std::exception &e) {
        std::cerr << "Error deleting connection '" << ssid << "': " << e.what() << std::endl;
        return false;
    }
}

void NetworkManager::showConnections() {
    std::vector<ConnectionInfo> connections;
    IwdManager iwdManager;
    auto station = iwdManager.createStation();
    if (!station) {
        std::cerr << "Failed to create Station instance" << std::endl;
        return;
    }

    std::string currentSSID;
    auto wired_cnt = 0;
    auto devices = listDevices();
    for (const auto &device : devices) {
        ConnectionInfo conn;
        if (device.type == "ethernet") {
            conn.name = "Wired connection " + std::to_string(wired_cnt);
        } else if (device.type == "loopback") {
            conn.name = "lo";
        } else if (device.type == "wifi") {
            conn.name = station->getPropertyFromObjectPath<std::string>(
                sdbus::ObjectPath{station->getConnectedNetwork()}, "net.connman.iwd.Network", "Name"
            );
            currentSSID = conn.name;
        } else {
            conn.name = device.name;
        }
        conn.uuid = stringToUUID(conn.name);
        conn.type = device.type;
        conn.device = device.name;
        connections.push_back(conn);
    }

    auto wifiConnections = station->getAllConnection();
    for (const auto &networkPath : wifiConnections) {
        std::string networkSSID = station->getPropertyFromObjectPath<std::string>(
            sdbus::ObjectPath{networkPath}, "net.connman.iwd.KnownNetwork", "Name"
        );
        if (networkSSID == currentSSID)
            continue;
        ConnectionInfo conn;
        conn.name = networkSSID;
        conn.uuid = stringToUUID(conn.name);
        conn.type = "wifi";
        connections.push_back(conn);
    }

    // Determine which fields to display
    bool show_name = field_selection.empty() ||
                     std::find(field_selection.begin(), field_selection.end(), "NAME") != field_selection.end();
    bool show_uuid = field_selection.empty() ||
                     std::find(field_selection.begin(), field_selection.end(), "UUID") != field_selection.end();
    bool show_type = field_selection.empty() ||
                     std::find(field_selection.begin(), field_selection.end(), "TYPE") != field_selection.end();
    bool show_device = field_selection.empty() ||
                       std::find(field_selection.begin(), field_selection.end(), "DEVICE") != field_selection.end();

    // Prepare data for formatting
    std::vector<std::vector<std::string>> table_data;
    std::vector<std::string> headers;

    // Add headers based on field selection
    if (show_name)
        headers.push_back("NAME");
    if (show_uuid)
        headers.push_back("UUID");
    if (show_type)
        headers.push_back("TYPE");
    if (show_device)
        headers.push_back("DEVICE");

    // Add connection data
    for (const auto &conn : connections) {
        std::vector<std::string> row;
        if (show_name)
            row.push_back(conn.name);
        if (show_uuid)
            row.push_back(conn.uuid);
        if (show_type)
            row.push_back(conn.type);
        if (show_device)
            row.push_back(conn.device.length() == 0 ? "--" : conn.device);
        table_data.push_back(row);
    }

    // Print formatted table
    printFormattedTable(table_data, headers);
}

int NetworkManager::dbmToQualitySegmented(int rssi_dbm) {
    /*
     * 分段线性映射，更符合实际感知
     */
    if (rssi_dbm >= -50) {
        return 100;
    } else if (rssi_dbm >= -60) {
        // -50 到 -60: 100% -> 80%
        return 80 + ((rssi_dbm + 60) * 20) / 10;
    } else if (rssi_dbm >= -70) {
        // -60 到 -70: 80% -> 60%
        return 60 + ((rssi_dbm + 70) * 20) / 10;
    } else if (rssi_dbm >= -80) {
        // -70 到 -80: 60% -> 40%
        return 40 + ((rssi_dbm + 80) * 20) / 10;
    } else if (rssi_dbm >= -90) {
        // -80 到 -90: 40% -> 20%
        return 20 + ((rssi_dbm + 90) * 20) / 10;
    } else {
        return 0;
    }
}

void NetworkManager::listWifiNetworks(bool rescan) {
    try {
        // Create IwdManager instance
        IwdManager iwdManager;

        // Create Station instance
        auto station = iwdManager.createStation();
        if (!station) {
            std::cerr << "Failed to create Station instance" << std::endl;
            return;
        }

        // If rescan is requested, perform a scan
        if (rescan) {
            if (station->scan()) {
                // Wait for scan to complete by polling the Scanning property
                int attempts = 0;
                const int maxAttempts = 20; // Maximum 10 seconds (20 * 500ms)
                while (attempts < maxAttempts) {
                    if (!station->isScanning()) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    attempts++;
                }

                if (attempts >= maxAttempts) {
                    std::cerr << "Scan timeout" << std::endl;
                }
            } else {
                std::cerr << "Failed to initiate scan" << std::endl;
            }
        }

        // Get ordered networks
        auto networks = station->getOrderedNetworks();

        // Print "Found X networks" message in non-terse mode
        if (!terse_output) {
            std::cout << "Found " << networks.size() << " networks" << std::endl;
        }

        // Sort networks by signal strength in descending order (strongest first)
        // Note: The networks should already be ordered by signal strength from getOrderedNetworks,
        // but we'll sort them explicitly to ensure correct ordering
        std::sort(networks.begin(), networks.end(), [](const Station::NetworkInfo &a, const Station::NetworkInfo &b) {
            return a.signal_strength > b.signal_strength;
        });

        // Determine which fields to display
        bool show_ssid = field_selection.empty() ||
                         std::find(field_selection.begin(), field_selection.end(), "SSID") != field_selection.end();
        bool show_security =
            field_selection.empty() ||
            std::find(field_selection.begin(), field_selection.end(), "SECURITY") != field_selection.end();
        bool show_signal = field_selection.empty() ||
                           std::find(field_selection.begin(), field_selection.end(), "SIGNAL") != field_selection.end();
        bool show_inuse = field_selection.empty() ||
                          std::find(field_selection.begin(), field_selection.end(), "IN-USE") != field_selection.end();

        // Prepare data for formatting
        std::vector<std::vector<std::string>> table_data;
        std::vector<std::string> headers;

        // Add headers based on field selection
        if (show_ssid)
            headers.push_back("SSID");
        if (show_security)
            headers.push_back("SECURITY");
        if (show_signal)
            headers.push_back("SIGNAL");
        if (show_inuse)
            headers.push_back("IN-USE");

        // Add network data
        for (const auto &network : networks) {
            std::vector<std::string> row;
            if (show_ssid)
                row.push_back(network.ssid);
            if (show_security)
                row.push_back(network.security);
            if (show_signal) {
                // Convert from dBm*100 to dBm, then convert to quality percentage
                int signal_dbm = network.signal_strength / 100;
                int signal_quality = dbmToQualitySegmented(signal_dbm);
                row.push_back(std::to_string(signal_quality));
            }
            if (show_inuse)
                row.push_back(network.in_use ? "*" : "");
            table_data.push_back(row);
        }

        // Print formatted table
        printFormattedTable(table_data, headers);
    } catch (const std::exception &e) {
        std::cerr << "Error listing WiFi networks: " << e.what() << std::endl;
    }
}

void NetworkManager::printFormattedTable(
    const std::vector<std::vector<std::string>> &data, const std::vector<std::string> &headers
) const {
    if (data.empty()) {
        return;
    }

    if (terse_output) {
        // Terse output format
        for (const auto &row : data) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0)
                    std::cout << ":";
                std::cout << row[i];
            }
            std::cout << std::endl;
        }
    } else {
        // Calculate column widths
        std::vector<size_t> column_widths(headers.size(), 0);

        // Initialize with header widths
        for (size_t i = 0; i < headers.size(); ++i) {
            column_widths[i] = headers[i].length();
        }

        // Find maximum width for each column
        for (const auto &row : data) {
            for (size_t i = 0; i < std::min(row.size(), headers.size()); ++i) {
                if (row[i].length() > column_widths[i]) {
                    column_widths[i] = row[i].length();
                }
            }
        }

        // Print header
        for (size_t i = 0; i < headers.size(); ++i) {
            std::cout << headers[i];
            if (i < headers.size() - 1) {
                std::cout << std::string(column_widths[i] - headers[i].length() + 2, ' ');
            }
        }
        std::cout << std::endl;

        // Print data rows
        for (const auto &row : data) {
            for (size_t i = 0; i < std::min(row.size(), headers.size()); ++i) {
                std::cout << row[i];
                if (i < headers.size() - 1) {
                    std::cout << std::string(column_widths[i] - row[i].length() + 2, ' ');
                }
            }
            std::cout << std::endl;
        }
    }
}
