#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Settings.h>

// === НАСТРОЙКИ ДИСПЛЕЯ ===
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// === ПИНЫ НАГРУЗОК ===
const uint8_t PIN_PUMP    = 26;
const uint8_t PIN_LIGHT   = 27;
const uint8_t PIN_HEATER  = 14;
const uint8_t PIN_CO2     = 12;
const uint8_t PIN_AERATOR = 13;

// === ОБЪЕКТ НАСТРОЕК ===
Settings sett;

// === КЛЮЧИ БАЗЫ ДАННЫХ (DB_KEYS) ===
DB_KEYS(
    wifi_ssid,      // WiFi SSID
    wifi_pass,      // WiFi Password
    temp_target,    // Целевая температура
    temp_hyst,      // Гистерезис температуры
    light_start,    // Время включения света
    light_end,      // Время выключения света
    co2_enabled,    // Включена ли система CO2
    pump_enabled,   // Включен ли насос
    reboot_btn      // Кнопка перезагрузки (техническая)
);

void connectWiFi() {
    Serial.print("Connecting to ");
    Serial.println((const char*)wifi_ssid);
    WiFi.begin((const char*)wifi_ssid, (const char*)wifi_pass);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nIP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi Failed!");
    }
}

void controlLogic() {
    float currentTemp = 24.5f;

    if (currentTemp < (temp_target - temp_hyst)) {
        digitalWrite(PIN_HEATER, HIGH);
    } else if (currentTemp > temp_target) {
        digitalWrite(PIN_HEATER, LOW);
    }

    digitalWrite(PIN_LIGHT, HIGH);
    digitalWrite(PIN_PUMP, pump_enabled ? HIGH : LOW);
    digitalWrite(PIN_CO2, co2_enabled ? HIGH : LOW);

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    
    display.println("AQUA CONTROL");
    display.print("T: "); display.print(currentTemp); display.println(" C");
    display.print("Set: "); display.print(temp_target); display.println(" C");
    display.print("WiFi: "); display.println(WiFi.status() == WL_CONNECTED ? "OK" : "NO");
    
    display.display();
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_LIGHT, OUTPUT);
    pinMode(PIN_HEATER, OUTPUT);
    pinMode(PIN_CO2, OUTPUT);
    pinMode(PIN_AERATOR, OUTPUT);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 failed"));
        for(;;);
    }
    display.clearDisplay();
    display.display();
    delay(1000);

    // Инициализация настроек
    sett.begin();

    // Построение интерфейса через build с лямбда-функцией
    sett.build([]() {
        sett.label("Aqua Control Settings");
        sett.hr();

        // WiFi настройки
        sett.input(&wifi_ssid, "WiFi SSID");
        sett.input(&wifi_pass, "WiFi Pass");

        // Температура (intInput для целых чисел, но у нас float - используем input или конвертируем)
        // Для float в Settings нет прямого виджета, используем input как строку или int с масштабированием
        // В документации только intInput, checkbox, select, action, label, hr, input
        sett.intInput(&temp_target, "Target Temp x10", 100, 350); // храним как int * 10
        sett.intInput(&temp_hyst, "Hysteresis x10", 1, 50);

        // Время света
        sett.intInput(&light_start, "Light Start Hour", 0, 23);
        sett.intInput(&light_end, "Light End Hour", 0, 23);

        // Переключатели
        sett.checkbox(&co2_enabled, "CO2 System Enabled");
        sett.checkbox(&pump_enabled, "Main Pump Enabled");

        sett.hr();

        // Кнопка перезагрузки - действие выполняется сразу при нажатии внутри build
        sett.action("Reboot ESP", []() {
            Serial.println("Rebooting...");
            ESP.restart();
        });
        
        sett.action("Reset Settings", []() {
            sett.reset();
            Serial.println("Settings reset!");
        });
    });

    connectWiFi();

    display.setCursor(0, 0);
    display.println("System Ready");
    display.println("Open Web UI");
    display.display();
    
    Serial.println("System started");
}

void loop() {
    // Основной цикл обработки настроек
    sett.tick();

    static unsigned long timer = 0;
    if (millis() - timer > 1000) {
        timer = millis();
        controlLogic();
    }
}