#include <Arduino.h>
#include <WiFi.h>

extern "C" {
#include "esp_wifi.h"
}

// ESP32 只有一个射频，没法真的同时起多个 SoftAP。
// 所以这里不"起 AP"，而是自己拼 802.11 beacon 帧丢出去 ——
// 在别人扫描时，这些帧看起来就跟一堆独立的路由器一样。

// ===== 想广播的假 WiFi 名字，随便改（每个最多 32 字节）=====
static const char *FAKE_SSIDS[] = {
    "睡",
    "神",
    "我",
    "喜",
    "欢",
    "你",
    "世界No.1",
    "💓💓💓",
    "❤️‍🔥❤️‍🔥❤️‍🔥",
    "❤️❤️❤️",
};
static const size_t FAKE_COUNT = sizeof(FAKE_SSIDS) / sizeof(FAKE_SSIDS[0]);

static const uint8_t CHANNEL = 1;         // 2.4G 只有 1/6/11 互不重叠
static const uint16_t GAP_MS = 5;         // 每 5ms 发一个，轮流转圈

static uint8_t frame[128];

// 同时往两个 USB 口打印：原生 USB(Type-C 1) 和 CH343(UART0, Type-C 2)
static void say(const char *s) {
  Serial.print(s);
  Serial0.print(s);
}

// 拼一个 802.11 beacon 帧，返回长度
static size_t buildBeacon(const char *ssid, uint8_t idx, uint8_t channel, uint8_t *p) {
  size_t n = strlen(ssid);
  if (n > 32) n = 32;

  // 本地管理地址，最后一位用来让每个假 AP 看起来是不同的设备
  const uint8_t bssid[6] = {0x02, 0x00, 0x00, 0x13, 0x37, idx};
  const uint8_t rates[] = {0x82, 0x84, 0x8b, 0x96, 0x0c, 0x12, 0x18, 0x24};

  uint8_t *q = p;
  *q++ = 0x80; *q++ = 0x00;              // Frame control：管理帧 / beacon
  *q++ = 0x00; *q++ = 0x00;              // Duration
  memset(q, 0xFF, 6); q += 6;            // DA：广播
  memcpy(q, bssid, 6); q += 6;           // SA
  memcpy(q, bssid, 6); q += 6;           // BSSID
  *q++ = 0x00; *q++ = 0x00;              // Sequence control
  memset(q, 0x00, 8); q += 8;            // Timestamp（固定值，无所谓）
  *q++ = 0x64; *q++ = 0x00;              // Beacon interval = 100 TU
  *q++ = 0x01; *q++ = 0x04;              // Capability：ESS + Short Slot，不加密

  *q++ = 0x00; *q++ = (uint8_t)n;        // SSID（tag 0，规范要求排最前面）
  memcpy(q, ssid, n); q += n;
  *q++ = 0x01; *q++ = sizeof(rates);     // Supported rates（tag 1）
  memcpy(q, rates, sizeof(rates)); q += sizeof(rates);
  *q++ = 0x03; *q++ = 0x01; *q++ = channel;  // DS Parameter（tag 3）= 所在信道

  return q - p;
}

void setup() {
  Serial.begin(115200);   // 原生 USB 口
  Serial0.begin(115200);  // UART0 -> 板载 CH343 那个 USB 口
  delay(300);

  // 单靠 esp_wifi_80211_tx 不行，得先把 WiFi 跑起来才有 WIFI_IF_AP 这个接口。
  // mode() 里会调 esp_wifi_start()，所以这一步等价于"开射频并锁在 CHANNEL"。
  WiFi.mode(WIFI_MODE_AP);

  // 自己起一个隐藏 AP 占住信道，顺手把它的 beacon 周期拉到 60s，
  // 免得它自己也在广播、干扰演示效果。
  wifi_config_t ap = {};
  memcpy(ap.ap.ssid, "cfg", 3);
  ap.ap.ssid_len = 3;
  ap.ap.channel = CHANNEL;
  ap.ap.authmode = WIFI_AUTH_OPEN;
  ap.ap.beacon_interval = 60000;
  ap.ap.ssid_hidden = 1;
  ap.ap.max_connection = 1;
  esp_wifi_set_config(WIFI_IF_AP, &ap);

  char buf[80];
  snprintf(buf, sizeof(buf), "广播 %u 个假 WiFi，信道 %u\n",
           (unsigned)FAKE_COUNT, (unsigned)CHANNEL);
  say(buf);
}

void loop() {
  static size_t i = 0;
  static esp_err_t lastErr = ESP_OK;

  size_t len = buildBeacon(FAKE_SSIDS[i], (uint8_t)i, CHANNEL, frame);
  esp_err_t err = esp_wifi_80211_tx(WIFI_IF_AP, frame, len, false);
  if (err != lastErr) {  // 只在错误码变化时打印，不然会刷屏
    char buf[64];
    snprintf(buf, sizeof(buf), "80211_tx: %s\n", esp_err_to_name(err));
    say(buf);
    lastErr = err;
  }

  i = (i + 1) % FAKE_COUNT;
  delay(GAP_MS);
}
