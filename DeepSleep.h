#pragma once

extern int PIR_PIN;

// Time constants
constexpr uint64_t MICROS_PER_SECOND = 1'000'000ULL;
constexpr uint64_t MICROS_PER_MINUTE = 60ULL * MICROS_PER_SECOND;
constexpr uint64_t MICROS_PER_HOUR   = 60ULL * MICROS_PER_MINUTE;

// Policy [default]
constexpr uint64_t SLEEP_TIME   = 8ULL * MICROS_PER_HOUR;    // 5 hours
constexpr uint64_t AWAKE_TIME = 1ULL * MICROS_PER_MINUTE; // 1 minute
// Policy [debug]
// constexpr uint64_t SLEEP_TIME   = 15ULL * MICROS_PER_SECOND;    // 15 seconds
// constexpr uint64_t AWAKE_TIME = 15ULL * MICROS_PER_SECOND; // 15 seconds


//Track awake window start
static uint64_t g_awake_start_us = 0;

static void armWakeSources() {
  // Timer wake
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);

  // Try to enable wake on the PIR GPIO if it's RTC-capable
  gpio_num_t wakeGpio = (gpio_num_t)PIR_PIN;
  if (rtc_gpio_is_valid_gpio(wakeGpio)) {
    // EXT0 wake: wake when pin goes HIGH (typical PIR idle LOW, active HIGH)
    esp_sleep_enable_ext0_wakeup(wakeGpio, 1);
  } else {
    // Not RTC-capable → cannot wake from deep sleep on this pin
    Serial.printf("NOTE: GPIO %d is not RTC-capable; deep-sleep motion wake disabled.\n", PIR_PIN);
  }
}

// ADDED: enter deep sleep cleanly
static void goDeepSleep() {
  // Put radio to sleep to save power (if it stays powered during ESP32 deep sleep)
  Radio.Sleep();
  Serial.println("Going to deep sleep...");
  Serial.flush();
  esp_deep_sleep_start();
}