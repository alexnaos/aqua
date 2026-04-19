#include <Arduino.h>
#include "config.h"
#include "ui_portal.h"
#include "mqtt_handler.h"
#include <ESPmDNS.h>

// --- Глобальные объекты (определение) ---
GyverDBFile db(&LittleFS, "/data.db");
SettingsGyver sett("Aqua Control", &db);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MqttHandler mqtt;

// Переменные состояния
bool pumpState = false;
unsigned long lastMillis = 0;

// Инициализация времени
void initTime()
{
    // Для ESP32 можно использовать SNTP или просто установить время при старте
    // Здесь заглушка, можно расширить при необходимости
    setStampZone(3); // Часовой пояс
    sett.rtc = time(NULL);
}

// Логика управления устройствами
void controlLogic()
{
    // Чтение значений из базы данных
    int target = db[kk::temp_target].toInt32();
    int hyst = db[kk::temp_hyst].toInt32();
    
    // Эмуляция чтения датчика температуры (заглушка)
    int currentTemp = 250; // 25.0°C * 10

    // Логика насоса
    bool pumpEnabled = db[kk::pump_enabled].toBool();
    if (!pumpEnabled) {
        pumpState = false;
    } else {
        if (currentTemp > (target + hyst)) pumpState = true;
        if (currentTemp < (target - hyst)) pumpState = false;
    }
    digitalWrite(PIN_PUMP, pumpState ? HIGH : LOW);

    // Логика CO2
    bool co2Enabled = db[kk::co2_enabled].toBool();
    digitalWrite(PIN_CO2, co2Enabled ? HIGH : LOW);

    // Логика света (по времени)
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    int hour = timeinfo->tm_hour;
    
    int lightStart = db[kk::light_start].toInt32();
    int lightEnd = db[kk::light_end].toInt32();
    bool lightState = (hour >= lightStart && hour < lightEnd);
    digitalWrite(PIN_LIGHT, lightState ? HIGH : LOW);

    // Обновление дисплея
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Temp: "); display.print(currentTemp / 10.0); display.println(" C");
    display.print("Set: ");  display.print(target / 10.0); display.println(" C");
    display.print("Pump: "); display.println(pumpState ? "ON" : "OFF");
    display.display();
}

void setup()
{
    Serial.begin(115200);
    Serial.println("\n--- Aqua System Start ---");

    // Инициализация пинов
    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_CO2, OUTPUT);
    pinMode(PIN_LIGHT, OUTPUT);

    // Инициализация дисплея
    Wire.begin();
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("SSD1306 allocation failed"));
    }
    display.display();
    delay(100);
    display.clearDisplay();

    // Инициализация файловой системы
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Инициализация базы данных
    db.begin();

    // Инициализация значений по умолчанию (если ключей еще нет)
    db.init(kk::wifi_ssid, WIFI_SSID_DEFAULT);
    db.init(kk::wifi_pass, WIFI_PASS_DEFAULT);
    db.init(kk::temp_target, 240);  // 24.0°C
    db.init(kk::temp_hyst, 5);      // 0.5°C
    db.init(kk::light_start, 8);
    db.init(kk::light_end, 20);
    db.init(kk::co2_enabled, false);
    db.init(kk::pump_enabled, true);

    // Инициализация времени
    initTime();

    // Подключение к WiFi
    WiFi.mode(WIFI_STA);
    String ssid = db[kk::wifi_ssid].toString();
    String pass = db[kk::wifi_pass].toString();
    
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        
        // Запуск mDNS (опционально)
        if (MDNS.begin("aqua")) {
            Serial.println("MDNS responder started");
        }
    } else {
        Serial.println("\nWiFi failed");
    }

    // Настройка интерфейса
    sett.begin();
    sett.config.theme = sets::Colors::Green;
    sett.onBuild(build);
    sett.onUpdate(update);

    // Инициализация MQTT
    mqtt.begin();

    Serial.println("Setup complete");
}

void loop()
{
    // Системные задачи
    sett.tick();     // Обслуживание веб-интерфейса
    db.tick();       // Сохранение настроек в базу
    mqtt.tick();     // Обработка MQTT сообщений

    // Логика устройства (каждую секунду)
    if (millis() - lastMillis >= 1000) {
        lastMillis = millis();
        controlLogic();
        
        // Публикация данных датчиков через MQTT
        if (mqtt.isConnected()) {
            int currentTemp = 250; // Заглушка, заменить на реальное чтение датчика
            mqtt.publishTemperature(currentTemp / 10.0);
            mqtt.publishPumpState(pumpState);
            
            bool co2Enabled = db[kk::co2_enabled].toBool();
            mqtt.publishCo2State(co2Enabled);
            
            time_t now = time(NULL);
            struct tm *timeinfo = localtime(&now);
            int hour = timeinfo->tm_hour;
            int lightStart = db[kk::light_start].toInt32();
            int lightEnd = db[kk::light_end].toInt32();
            bool lightState = (hour >= lightStart && hour < lightEnd);
            mqtt.publishLightState(lightState);
            
            mqtt.publishStatus("online");
        }
    }
}
