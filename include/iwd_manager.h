#ifndef IWD_MANAGER_H
#define IWD_MANAGER_H

#include <memory>
#include <string>
#include <vector>
#include "nmcli_exception.h"
#include <sdbus-c++/sdbus-c++.h>

// 前向声明
class Station;

class IwdManager {
public:
    IwdManager();
    ~IwdManager();
    
    // WiFi device commands
    struct WifiNetwork {
        std::string ssid;
        std::string security;
        int signal;
        bool in_use;
    };
    
    // Adapter methods
    std::string getAdapterObjectPath();
    std::string getDeviceObjectPath();
    
    // Radio methods
    bool getWifiRadioState();
    bool setWifiRadioState(bool enabled);
    
    // Station相关方法
    std::unique_ptr<Station> createStation();
    
    bool connectToNetwork(const std::string& ssid, const std::string& password = "");
    bool connectToNetworkViaDBus(const std::string& ssid, const std::string& password = "");
    bool connectToNetworkViaIWCTL(const std::string& ssid, const std::string& password = "");
    
private:
    // Private implementation details
    std::unique_ptr<sdbus::IConnection> connection_;
};

#endif // IWD_MANAGER_H