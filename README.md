# nmcli-alt

nmcli-alt 是一个基于 C++ 编写的网络管理命令行工具，旨在提供类似 nmcli（NetworkManager 命令行工具）的功能，但使用 IWD (iNet Wireless Daemon) 作为后端而不是 NetworkManager。

## 功能特性

- 设备管理：列出网络设备及其状态
- WiFi 管理：
  - 扫描可用的 WiFi 网络
  - 连接到 WiFi 网络（支持 WPA/WPA2 加密）
  - 显示 WiFi 网络列表
- 网络连接管理：
  - 显示网络连接
  - 激活网络连接
  - 停用网络连接
  - 删除网络连接
- 网络连接性检查
- 无线电控制：启用/禁用 WiFi 无线电
- 结构化输出：支持简洁模式和字段选择

## 依赖项

- C++17 或更高版本
- CMake 3.10 或更高版本
- libnl 库 (libnl-3.0, libnl-route-3.0, libnl-genl-3.0)
- sdbus-c++ 库
- IWD (iNet Wireless Daemon)

## 构建说明

1. 克隆仓库：
   ```bash
   git clone <repository-url>
   cd nmcli-alt
   ```

2. 创建构建目录并进入：
   ```bash
   mkdir build
   cd build
   ```

3. 运行 CMake 配置：
   ```bash
   cmake ..
   ```

4. 编译项目：
   ```bash
   make
   ```

5. （可选）安装到系统：
   ```bash
   sudo make install
   ```

## 使用方法

编译后，可以运行生成的 `nmcli-alt` 可执行文件：

```bash
./nmcli-alt [选项] <命令> [参数]
```

### 支持的命令

#### 设备管理
- 列出网络设备：
  ```bash
  ./nmcli-alt device
  ```
  
- 列出 WiFi 网络：
  ```bash
  ./nmcli-alt device wifi list [--rescan]
  ```

- 连接到 WiFi 网络：
  ```bash
  ./nmcli-alt device wifi connect <SSID> [password <密码>]
  ```

#### 网络连接管理
- 显示网络连接：
  ```bash
  ./nmcli-alt connection show
  ./nmcli-alt con show
  ```

- 激活网络连接：
  ```bash
  ./nmcli-alt connection up <SSID>
  ./nmcli-alt con up <SSID>
  ```

- 停用网络连接：
  ```bash
  ./nmcli-alt connection down <SSID>
  ./nmcli-alt con down <SSID>
  ```

- 删除网络连接：
  ```bash
  ./nmcli-alt connection delete <SSID>
  ./nmcli-alt con delete <SSID>
  ```

#### 网络连接性检查
```bash
./nmcli-alt networking connectivity
```

#### 无线电控制
- 显示 WiFi 无线电状态：
  ```bash
  ./nmcli-alt radio wifi
  ```

- 启用/禁用 WiFi 无线电：
  ```bash
  ./nmcli-alt radio wifi on
  ./nmcli-alt radio wifi off
  ```

### 选项

- `-t`, `--terse`: 使用简洁格式输出
- `-f`, `--fields`: 指定要显示的字段（以逗号分隔）

示例：
```bash
./nmcli-alt -t -f DEVICE,STATE device
```

## 项目结构

```
nmcli-alt/
├── CMakeLists.txt             # CMake 构建配置
├── include/                   # 头文件目录
│   ├── iwd_manager.h          # IWD 管理器接口
│   ├── network_manager.h      # 网络管理器接口
│   ├── nmcli_exception.h      # 自定义异常类
│   ├── process_util.h         # 进程工具函数
│   └── station.h              # Station 接口
├── src/                       # 源代码目录
│   ├── main.cpp               # 主程序入口
│   ├── iwd_manager.cpp        # IWD 管理器实现
│   ├── network_manager.cpp    # 网络管理器实现
│   ├── process_util.cpp       # 进程工具函数实现
│   └── station.cpp            # Station 实现
└── iwd-doc/                   # IWD 相关文档
```

## 异常处理

项目定义了多种自定义异常类型用于处理不同类型的错误：

- `NmcliException`: 基础异常类
- `NetworkException`: 网络相关异常
- `DBusException`: D-Bus 通信异常
- `CommandExecutionException`: 命令执行异常
- `ConnectionException`: 连接异常

## 许可证

本项目采用 [MIT 许可证](LICENSE)。

## 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目。
