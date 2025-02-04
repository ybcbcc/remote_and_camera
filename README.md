# 智能家居远程监控遥控设备
## 监控装置（监控装置的代码基于 vscode platformio，请根据情况移植）
### 1. 使用的库
```
#include <WiFi.h>
#include <ArduinoWebsockets.h>
#include "esp_camera.h"
#include <Arduino.h>
#include "FS.h"
#include "SPI.h"
#include "SdFat.h"
```
后三个库，并没有在代码中使用到，因为我买的摄像头模块使用 ESP32-cam，其上自带了一个TF卡模块，所以如果不添加这三个库就会报错，添加就可以了
### 2. 摄像头引脚定义
摄像头的引脚定义根据 ESP32-cam 的默认引脚。
```
#define RESET_GPIO_NUM -1
```
这句话的控制复位未使用，置 -1 表示未使用这个复位引脚。

### 3. 连接信息
```
// WiFi credentials
const char *ssid = "ybc2";
const char *password = "20010219";
// WebSocket server details
const char *websocket_server = "121.43.141.184";
const uint16_t websocket_port = 8080;
```
初始化 WiFi 和 Web 的信息，IP地址用你的服务器的云服务器信息，8080 端口使用你对自己云服务器的安全组设置端口。

### 4. 状态变量定义 
```
WebsocketsClient client;
bool client_connected = false;
bool camera_active = false; // 控制摄像头状态

```
定义摄像头状态和服务器连接状态

### 5. 初始化摄像头参数(configCamera())
初始化摄像头参数可以参考库 esp_camera.h 的示例。先初始化一个结构体，然后对应赋值即可。  
提几行有用的：  
1. 设置 LEDC 时钟
    
    ```
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    ```
摄像头的 XCLK 用的 LEDC 驱动。这里设置 LEDC 的通道 0，使用 LEDC 计时器 0 生成时钟信号。

2. 摄像头控制信号
    ```
    config.pin_xclk = XCLK_GPIO_NUM;  // 外部时钟输入
    config.pin_pclk = PCLK_GPIO_NUM;  // 像素时钟
    config.pin_vsync = VSYNC_GPIO_NUM; // 垂直同步信号，保证帧同步
    config.pin_href = HREF_GPIO_NUM;  // 行同步信号
    ```
3. IIC通信
    ```
    config.pin_sccb_sda = SIOD_GPIO_NUM; // 数据线
    config.pin_sccb_scl = SIOC_GPIO_NUM; // 时钟线
    ```
4. 供电和复位
    ```
    config.pin_pwdn = PWDN_GPIO_NUM;  // 省电模式
    config.pin_reset = RESET_GPIO_NUM; // 复位引脚

    ```
5. 摄像头基本参数
    ```
    config.xclk_freq_hz = 20000000; // 设置 XCLK 时钟频率 20MHz
    config.pixel_format = PIXFORMAT_JPEG; // 设置图像格式
    config.frame_size = FRAMESIZE_QVGA; // 分辨率
    config.jpeg_quality = 10; // JPEG 质量（0 最高，63 最差）
    config.fb_count = 1; // 帧缓冲区数量
    ```
6. 初始化摄像头
    ```
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
    ```
    1. esp_camera_init(&config) → 初始化摄像头,如果 初始化失败，打印错误代码，例如：plaintext Camera init failed with error 0x20001
    2. 可能的错误：
    - 摄像头连接错误（排查接线）
    - 供电问题（检查 3.3V 供电）
    - 摄像头损坏
7. 传感器参数设置
    ```
    sensor_t *s = esp_camera_sensor_get();
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);
        s->set_brightness(s, 1);
        s->set_saturation(s, -2);
    }
    ```
    1. 获取传感器指针地址
    2. 传感器设置，详细内容可以参考 sensor_t 结构体。

### 6. WebSocket回调函数
这段代码相当简单，接收到信息后触发回调函数，先是将接收的信息打印出来，其次是判断信息内容，如果是开启摄像头或者关闭摄像头，就会激活相应的状态

### 7. WebSocket 事件回调函数
当与服务器连接或者断开连接时触发，并且不断进行 ping 测试，保证设备和服务器一直在线
- WebSocket 心跳机制（Ping/Pong）
    - Ping：服务器主动发送，检测设备是否在线。
    - Pong：设备回复服务器，确认连接正常。

### 8. 摄像头捕获并发送函数
captureAndSendFrame 函数解析：
这个函数的作用是 捕获摄像头图像，并通过 WebSocket 发送给服务器。
它会先检查 摄像头是否已启用 和 WebSocket 是否已连接，然后 拍照并发送数据。  
可以适当加上延迟缓解发送频率

### 9. setup()
1. 初始化串口通信（用于调试输出）。
2. 连接 WiFi，确保设备可以联网。
3. 初始化摄像头，为后续拍照做准备。
4. 配置 WebSocket 客户端，设置事件回调函数。
5. 连接 WebSocket 服务器，用于传输图像数据。
### 10. loop()
1. 自动重连 WiFi：如果 WiFi 断开，则尝试重新连接。
2. 处理 WebSocket 消息：调用 client.poll() 处理服务器的 WebSocket 消息。可以接收服务器的信息，并且获得与服务器的状态，触发回调函数。
3. 定时 ping：每 15 秒发送 ping，防止 WebSocket 连接超时。
4. 发送摄像头画面：
只有在 camera_active = true 时才会真正捕获并发送图像数据。
发送图像后会 delay(100); 控制帧率。
5. WebSocket 断线重连：如果 WebSocket 断开，尝试重新连接，连接失败则等待 5 秒后重试。

## 服务器设置
### 1.安装Node.js和npm
```
# 更新包管理器
sudo apt update
# 安装 Node.js（推荐使用包管理工具安装 Node.js）
sudo apt install nodejs npm
```
### 2.将 server.js 上传到服务器，复制也可以

### 3.安装依赖项
```
# 进入项目目录
cd /path/to/your/project
# 初始化npm项目（如果还没有package.json）
npm init -y
# 安装依赖包
npm install express ws
```
### 4. 运行服务器
```
node server.js
```
记得配置服务器的 8080 端口以及防火墙哦

## 控制端设置
控制端可简单了，将文件的当前文件夹的命令行打开后，执行
```
python -m http.server 8080
```
在浏览器localhost:8080即可访问控制端。
