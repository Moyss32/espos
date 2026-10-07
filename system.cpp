#include "system.h"
#include "config.h"
#include <esp_timer.h>

void sysReboot() {
  Serial.print("Rebooting...\r\n");
  Serial.flush();
  delay(100);
  ESP.restart();
}

void sysUname(Print& out, bool full) {
  if (!full) { out.print(MINIOS_NAME "\n"); return; }
#ifdef CONFIG_IDF_TARGET_ARCH_RISCV
  const char* arch = "RISC-V";
#else
  const char* arch = "Xtensa";
#endif
  out.printf(MINIOS_NAME " v" MINIOS_VERSION "\nArchitecture: %s\nChip: %s rev %u @ %u MHz\nFilesystem: LittleFS\n",
             arch, ESP.getChipModel(), (unsigned)ESP.getChipRevision(), (unsigned)ESP.getCpuFreqMHz());
}

void sysFree(Print& out) {
  out.printf("RAM: total %u KB, free %u KB, min free %u KB\n",
             (unsigned)(ESP.getHeapSize() / 1024), (unsigned)(ESP.getFreeHeap() / 1024),
             (unsigned)(ESP.getMinFreeHeap() / 1024));
}

void sysUptime(Print& out) {
  unsigned long s = (unsigned long)(esp_timer_get_time() / 1000000ULL);
  out.printf("up %lud %02lu:%02lu:%02lu\n", s / 86400UL, (s / 3600UL) % 24, (s / 60UL) % 60, s % 60);
}

uint32_t sysRamKB() { return ESP.getHeapSize() / 1024; }
