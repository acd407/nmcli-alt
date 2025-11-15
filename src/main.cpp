#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <network_manager.h>
#include <iwd_manager.h>
#include <nmcli_exception.h>

// Helper function to split string by delimiter
std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    for (char c : str) {
        if (c == delimiter) {
            tokens.push_back(token);
            token.clear();
        } else {
            token += c;
        }
    }
    tokens.push_back(token);
    return tokens;
}

int main(int argc, char *argv[]) {
    // Check if we have enough arguments
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [-t] [-f <fields>] <command> [options]" << std::endl;
        return 1;
    }

    // Initialize NetworkManager
    NetworkManager nm;

    // Parse command line arguments
    int i = 1;
    while (i < argc) {
        std::string arg = argv[i];

        if (arg == "-t" || arg == "--terse") {
            nm.terse_output = true;
            i++;
        } else if (arg == "-f" || arg == "--fields") {
            if (i + 1 < argc) {
                nm.field_selection = split(argv[i + 1], ',');
                i += 2;
            } else {
                std::cerr << "Error: -f option requires an argument" << std::endl;
                return 1;
            }
        } else {
            break;
        }
    }

    // If we've processed all arguments, that's an error
    if (i >= argc) {
        std::cerr << "Error: No command specified" << std::endl;
        return 1;
    }

    // Get the command
    std::string command = argv[i];

    if (command == "networking") {
        if (i + 1 < argc && std::string(argv[i + 1]) == "connectivity") {
            // Handle "nmcli networking connectivity" command
            std::cout << nm.getConnectivity() << std::endl;
            return 0;
        }
    } else if (command == "device") {
        // Handle "nmcli device" command
        if (i + 1 < argc) {
            std::string subcommand = argv[i + 1];
            if (subcommand == "status" || subcommand == "") {
                // List devices
                auto devices = nm.listDevices();

                // Determine which fields to display
                bool show_device = nm.field_selection.empty() ||
                                   std::find(nm.field_selection.begin(), nm.field_selection.end(), "DEVICE") !=
                                       nm.field_selection.end();
                bool show_type =
                    nm.field_selection.empty() ||
                    std::find(nm.field_selection.begin(), nm.field_selection.end(), "TYPE") != nm.field_selection.end();
                bool show_state = nm.field_selection.empty() ||
                                  std::find(nm.field_selection.begin(), nm.field_selection.end(), "STATE") !=
                                      nm.field_selection.end();

                // Prepare data for formatting
                std::vector<std::vector<std::string>> table_data;
                std::vector<std::string> headers;

                // Add headers based on field selection
                if (show_device)
                    headers.push_back("DEVICE");
                if (show_type)
                    headers.push_back("TYPE");
                if (show_state)
                    headers.push_back("STATE");

                // Add device data
                for (const auto &device : devices) {
                    std::vector<std::string> row;
                    if (show_device)
                        row.push_back(device.name);
                    if (show_type)
                        row.push_back(device.type);
                    if (show_state)
                        row.push_back(device.state);
                    table_data.push_back(row);
                }

                // Print formatted table
                nm.printFormattedTable(table_data, headers);
                return 0;
            } else if (subcommand == "wifi") {
                // Handle "nmcli device wifi" subcommands
                if (i + 2 < argc) {
                    std::string wifi_subcommand = argv[i + 2];
                    if (wifi_subcommand == "list") {
                        // Check for --rescan option
                        bool rescan = false;
                        for (int j = i + 3; j < argc; j++) {
                            if (std::string(argv[j]) == "--rescan" || std::string(argv[j]) == "--rescan=yes") {
                                rescan = true;
                                break;
                            } else if (std::string(argv[j]) == "--rescan=no") {
                                rescan = false;
                                break;
                            }
                        }

                        // Handle "nmcli device wifi list" command
                        nm.listWifiNetworks(rescan);
                        return 0;
                    } else if (wifi_subcommand == "connect") {
                        // Handle "nmcli device wifi connect" command
                        if (i + 3 < argc) {
                            std::string ssid = argv[i + 3];
                            std::string password = "";

                            // Check for password option
                            if (i + 5 < argc && std::string(argv[i + 4]) == "password") {
                                password = argv[i + 5];
                            }

                            // Create IwdManager and connect to network
                            try {
                                IwdManager iwdManager;
                                bool result = iwdManager.connectToNetwork(ssid, password);

                                if (result) {
                                    std::cout << "Successfully connected to '" << ssid << "'" << std::endl;
                                    return 0;
                                } else {
                                    std::cerr << "Failed to connect to '" << ssid << "'" << std::endl;
                                    return 1;
                                }
                            } catch (const NetworkException &e) {
                                std::cerr << "Network error: " << e.what() << std::endl;
                                return 1;
                            } catch (const CommandExecutionException &e) {
                                std::cerr << "Command execution error: " << e.what() << std::endl;
                                return 1;
                            } catch (const DBusException &e) {
                                std::cerr << "D-Bus error: " << e.what() << std::endl;
                                return 1;
                            } catch (const NmcliException &e) {
                                std::cerr << "Nmcli error: " << e.what() << std::endl;
                                return 1;
                            } catch (const std::exception &e) {
                                std::cerr << "Unexpected error: " << e.what() << std::endl;
                                return 1;
                            }
                        } else {
                            std::cerr << "Error: SSID required for connect command" << std::endl;
                            return 1;
                        }
                    }
                }
            }
        } else {
            // List devices (default action)
            auto devices = nm.listDevices();

            // Determine which fields to display
            bool show_device =
                nm.field_selection.empty() ||
                std::find(nm.field_selection.begin(), nm.field_selection.end(), "DEVICE") != nm.field_selection.end();
            bool show_type =
                nm.field_selection.empty() ||
                std::find(nm.field_selection.begin(), nm.field_selection.end(), "TYPE") != nm.field_selection.end();
            bool show_state =
                nm.field_selection.empty() ||
                std::find(nm.field_selection.begin(), nm.field_selection.end(), "STATE") != nm.field_selection.end();

            // Prepare data for formatting
            std::vector<std::vector<std::string>> table_data;
            std::vector<std::string> headers;

            // Add headers based on field selection
            if (show_device)
                headers.push_back("DEVICE");
            if (show_type)
                headers.push_back("TYPE");
            if (show_state)
                headers.push_back("STATE");

            // Add device data
            for (const auto &device : devices) {
                std::vector<std::string> row;
                if (show_device)
                    row.push_back(device.name);
                if (show_type)
                    row.push_back(device.type);
                if (show_state)
                    row.push_back(device.state);
                table_data.push_back(row);
            }

            // Print formatted table
            nm.printFormattedTable(table_data, headers);
            return 0;
        }
    } else if (command == "connection" || command == "con") {
        // Handle "nmcli connection" command
        if (i + 1 < argc) {
            std::string subcommand = argv[i + 1];
            if (subcommand == "show") {
                // Show connections
                nm.showConnections();
                return 0;
            } else if (subcommand == "up") {
                // Handle "nmcli connection up" command
                if (i + 2 < argc) {
                    std::string ssid = argv[i + 2];
                    if (nm.activateConnection(ssid)) {
                        std::cout << "Connection '" << ssid << "' activated successfully" << std::endl;
                        return 0;
                    } else {
                        std::cerr << "Failed to activate connection '" << ssid << "'" << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Error: SSID required for connection up command" << std::endl;
                    return 1;
                }
            } else if (subcommand == "down") {
                // Handle "nmcli connection down" command
                if (i + 2 < argc) {
                    std::string ssid = argv[i + 2];
                    if (nm.deactivateConnection(ssid)) {
                        std::cout << "Connection '" << ssid << "' deactivated successfully" << std::endl;
                        return 0;
                    } else {
                        std::cerr << "Failed to deactivate connection '" << ssid << "'" << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Error: SSID required for connection down command" << std::endl;
                    return 1;
                }
            } else if (subcommand == "delete") {
                // Handle "nmcli connection delete" command
                if (i + 2 < argc) {
                    std::string ssid = argv[i + 2];
                    if (nm.deleteConnection(ssid)) {
                        std::cout << "Connection '" << ssid << "' deleted successfully" << std::endl;
                        return 0;
                    } else {
                        std::cerr << "Failed to delete connection '" << ssid << "'" << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Error: SSID required for connection delete command" << std::endl;
                    return 1;
                }
            }
            // 其他子命令可以在这里添加
        } else {
            // Default action for connection command is show
            nm.showConnections();
            return 0;
        }
    } else if (command == "radio") {
        // Handle "nmcli radio" command
        if (i + 1 < argc) {
            std::string subcommand = argv[i + 1];
            if (subcommand == "wifi") {
                if (i + 2 < argc) {
                    std::string state = argv[i + 2];
                    if (state != "on" && state != "off") {
                        std::cerr << "Invalid radio state: " << state << ". Use 'on' or 'off'" << std::endl;
                        return 1;
                    }
                    bool setState = state == "on";
                    if (!nm.setWifiRadio(setState)) {
                        std::cerr << "Failed to set WiFi radio " << state << std::endl;
                        return 1;
                    }
                    return 0;
                } else {
                    // Show current WiFi radio state
                    bool state = nm.getWifiRadioState();
                    std::cout << (state ? "enabled" : "disabled") << std::endl;
                    return 0;
                }
            } else if (subcommand == "all" || subcommand == "wwan") {
                // For now, we only support WiFi radio
                std::cerr << "Unsupported radio type: " << subcommand << std::endl;
                return 1;
            } else {
                std::cerr << "Invalid radio subcommand: " << subcommand << std::endl;
                return 1;
            }
        } else {
            // Show all radio states
            bool wifiState = nm.getWifiRadioState();
            std::cout << (wifiState ? "enabled" : "disabled") << std::endl;
            // TODO: Add WWAN and other radio types when implemented
            return 0;
        }
    }

    std::cerr << "Unsupported command: " << command << std::endl;
    return 1;
}