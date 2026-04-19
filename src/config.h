#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <GyverDBFile.h>
#include <LittleFS.h>
#include <SettingsGyver.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

// --- Настройки дисплея ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// --- Настройки пинов ---
#define PIN_PUMP  2
#define PIN_CO2   4
#define PIN_LIGHT 5

// --- Сеть ---
#define WIFI_SSID_DEFAULT "Sloboda100"
#define WIFI_PASS_DEFAULT "2716192023"

// --- Ключи базы данных ---
enum kk : size_t
{
    wifi_ssid,
    wifi_pass,
    temp_target,    // х10 (например, 240 = 24.0°C)
    temp_hyst,      // х10
    light_start,
    light_end,
    co2_enabled,
    pump_enabled,
    rtc_key         // Для синхронизации времени
};

// Глобальные объекты (объявляем как extern, определяем в main.cpp)
extern GyverDBFile db;
extern SettingsGyver sett;
extern Adafruit_SSD1306 display;

// Прототипы функций
void initTime();
void build(sets::Builder &b);
void update(sets::Updater &upd);

#endif
