#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <string>
#include <vector>

class NetworkManager {
  public:
    NetworkManager();
    ~NetworkManager();

    // Command line options
    bool terse_output = false;
    std::vector<std::string> field_selection;

    // Formatting methods
    void printFormattedTable(
        const std::vector<std::vector<std::string>> &data, const std::vector<std::string> &headers
    ) const;

    // Networking commands
    std::string getConnectivity();

    // Radio commands
    bool setWifiRadio(bool enabled);
    bool getWifiRadioState();

    // Device commands
    struct DeviceInfo {
        std::string name;
        std::string type;
        std::string state;
    };

    std::vector<DeviceInfo> listDevices();

    // Connection commands
    struct ConnectionInfo {
        std::string name;
        std::string uuid;
        std::string type;
        std::string device;
    };

    void showConnections();
    void listWifiNetworks(bool rescan = false);
    int dbmToQualitySegmented(int rssi_dbm);
    bool activateConnection(const std::string &ssid);
    bool deactivateConnection(const std::string &ssid);
    bool deleteConnection(const std::string &ssid);

  private:
    // Private implementation details
};

#endif // NETWORK_MANAGER_H
