#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);   // 原生 USB 口 (USB-Serial-JTAG)
  Serial0.begin(115200);  // UART0 -> 板载 CH343 那个 USB 口
  delay(500);

  char msg[64];
  snprintf(msg, sizeof(msg), "PSRAM: %u KB, psramFound=%d\n",
           (unsigned)(ESP.getPsramSize() / 1024), psramFound());
  Serial.print(msg);
  Serial0.print(msg);

  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}