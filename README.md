# Mutiple-wifi

ESP32-S3 + PlatformIO + Arduino。目前 `src/main.cpp` 还是 PlatformIO 的空模板，功能待写。

## 芯片

实测信息（esptool 从芯片读回）：

| | |
|---|---|
| 芯片 | ESP32-S3 (QFN56)，revision v0.2 |
| 内核 | 双核 Xtensa LX7 @ 240 MHz |
| 无线 | WiFi 2.4G + BLE 5 |
| 晶振 | 40 MHz |
| PSRAM | 8 MB，芯片内置 |
| Flash | 8 MB，QIO @ 80 MHz |
| MAC | 98:a3:16:e5:a3:64 |

## 配置

- **框架**：PlatformIO + espressif32 7.1.3 + Arduino-ESP32 **2.0.17**
- **板子定义**：`esp32-s3-devkitc-1`
- **分区表**：`default_8MB.csv` —— 双 app 分区（各 3.19 MB，可 OTA）+ 1.5 MB SPIFFS + coredump
- **占用**（空模板）：RAM 18 KB / 320 KB，Flash 228 KB / 3.19 MB
- **烧录**：esptool 4.11.0，460800 波特率；COM 号随插拔变化，PlatformIO 会自动识别
- **USB**：原生 USB-Serial-JTAG，VID:PID = `303A:1001`

### USB 那两行 build_flags

```ini
-DARDUINO_USB_MODE=1          ; 走硬件 USB-Serial-JTAG，不用 TinyUSB
-DARDUINO_USB_CDC_ON_BOOT=1   ; Serial 输出到 USB 口，而不是 UART0
```

第一项板子定义里本来就有，显式写出来是为了标明意图；第二项决定 `Serial` 往哪走。

之所以不碰 TinyUSB：USB-Serial-JTAG 在**下载态和运行态是同一个 USB 设备**（都是 `303A:1001`），esptool 靠 DTR/RTS 就能自己复位进下载模式。而 TinyUSB 运行时会变成 `303A:4001` —— 两个状态对应两个不同的设备，自动复位失效，每次烧录都得手动按住 BOOT、点一下 RST 才能进去。

### PSRAM 怎么开起来的

板子定义写的是 "N8, No PSRAM"，但实测芯片**带 8 MB PSRAM**，默认是不会被启用的。两处都要配：

```ini
board_build.arduino.memory_type = qio_opi   ; flash 走 QIO，PSRAM 走 8 线 OPI
build_flags = ... -DBOARD_HAS_PSRAM         ; 真正打开 PSRAM 的是这一行
```

`memory_type` 只决定 flash 模式和启动模式，**光配它不会启用 PSRAM**，必须同时加 `-DBOARD_HAS_PSRAM`。配完实测：

```
PSRAM: 8189 KB, psramFound=1
```

### 两个 USB 口

板子上两个 Type-C 口不一样，插错也能烧但看不到 `Serial` 输出：

| 口 | 芯片 | 能做的 |
|---|---|---|
| 原生 USB | ESP32-S3 内置 USB-Serial-JTAG，`303A:1001` | `Serial` 走这里；也是 JTAG 调试口 |
| UART 口 | 板载 **CH343** 桥接芯片，`1A86:55D3` | `Serial0`(UART0) 走这里；硬件自动复位，烧录更省事 |

`debug_tool = esp-builtin` 用的是内置 JTAG，不需要外接 esp-prog。
