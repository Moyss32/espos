#include <Arduino.h>
#include <esp_log.h>
#include "config.h"
#include "filesystem.h"
#include "shell.h"
#include "system.h"

void setup() {
  Serial.begin(SERIAL_BAUD);
  esp_log_level_set("*", ESP_LOG_NONE);     // mantém o terminal limpo
  delay(500);

  Serial.print("\r\nBooting MiniOS...\r\nMounting filesystem...\r\n");
  if (!fsMount()) {
    Serial.print("Filesystem mount failed. Halted.\r\n");
    for (;;) delay(1000);
  }
  Serial.print(fsCreateBaseTree() > 0 ? "Creating base directories...\r\n" : "Filesystem OK.\r\n");
  Serial.print("System ready.\r\n\r\n");

  Serial.printf(MINIOS_NAME " v" MINIOS_VERSION "\r\nFilesystem: LittleFS\r\nRAM: %u KB\r\n"
                "Type 'help' for available commands.\r\n\r\n", (unsigned)sysRamKB());
  shellPrompt();
}

void loop() {
  shellPoll();
  delay(1);
}
