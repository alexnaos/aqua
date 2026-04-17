/*
 * Скетч для управления аквариумом на ESP32
 * Использует библиотеки GyverHub и GyverControl
 * 
 * Функционал:
 * - Управление освещением (вкл/выкл, яркость)
 * - Управление помпой/фильтром
 * - Управление нагревателем
 * - Мониторинг температуры (симуляция)
 * - Веб-интерфейс через GyverHub
 */

#include <WiFi.h>
#include <GyverHub.h>
#include <GyverControl.h>

// ================= НАСТРОЙКИ WiFi =================
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ================= ПИНЫ ESP32 =================
#define LED_PIN 2           // Встроенный светодиод или подключение реле света
#define PUMP_PIN 4          // Реле помпы/фильтра
#define HEATER_PIN 5        // Реле нагревателя
#define TEMP_SENSOR_PIN 34  // Пин для датчика температуры (ADC)

// ================= ПАРАМЕТРЫ АКВАРИУМА =================
float targetTemp = 25.0;    // Целевая температура
float hysteresis = 0.5;     // Гистерезис для нагревателя
int lightBrightness = 100;  // Яркость света (0-100%)
bool autoMode = true;       // Автоматический режим

// ================= ОБЪЕКТЫ =================
GyverHub hub("AquaControl");
GyverControl gc;

// Переменные состояния
bool lightState = false;
bool pumpState = true;      // Помпа обычно работает постоянно
bool heaterState = false;
float currentTemp = 24.5;   // Текущая температура (симуляция)

// ================= ИНИЦИАЛИЗАЦИЯ =================
void setup() {
    Serial.begin(115200);
    
    // Настройка пинов
    pinMode(LED_PIN, OUTPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(HEATER_PIN, OUTPUT);
    
    digitalWrite(LED_PIN, LOW);
    digitalWrite(PUMP_PIN, HIGH);  // Реле часто инвертированы
    digitalWrite(HEATER_PIN, HIGH);
    
    // Подключение к WiFi
    connectToWiFi();
    
    // Настройка GyverHub
    setupGyverHub();
    
    Serial.println("\n=== AquaControl запущен ===");
}

// ================= ПОДКЛЮЧЕНИЕ К WiFi =================
void connectToWiFi() {
    Serial.print("Подключение к ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi подключен!");
        Serial.print("IP адрес: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nНе удалось подключиться к WiFi!");
        // Создаем точку доступа если не удалось подключиться
        WiFi.softAP("AquaControl_AP", "12345678");
        Serial.print("Создана точка доступа. IP: ");
        Serial.println(WiFi.softAPIP());
    }
}

// ================= НАСТРОЙКА GYVERHUB =================
void setupGyverHub() {
    // Инициализация GyverHub
    hub.begin();
    
    // Создание интерфейса
    hub.interface([]() {
        // Заголовок
        GH_TEXT("🐠 Управление аквариумом", GH_H2);
        
        // Блок освещения
        GH_PANEL_BEGIN("💡 Освещение");
        GH_SWITCH(lightState, "Свет", "light_sw");
        GH_SLIDER(lightBrightness, 0, 100, "Яркость", "light_slider", "%");
        GH_PANEL_END();
        
        // Блок оборудования
        GH_PANEL_BEGIN("⚙️ Оборудование");
        GH_SWITCH(pumpState, "Помпа", "pump_sw");
        GH_SWITCH(heaterState, "Нагреватель (ручной)", "heater_sw");
        GH_CHECK(autoMode, "Авто-режим температуры", "auto_check");
        GH_PANEL_END();
        
        // Блок температуры
        GH_PANEL_BEGIN("🌡️ Температура");
        GH_VALUE(currentTemp, "Текущая", "temp_val", "°C");
        GH_NUMBER(targetTemp, 15.0, 35.0, 0.1, "Целевая", "target_num", "°C");
        GH_PANEL_END();
        
        // Информация
        GH_PANEL_BEGIN("ℹ️ Информация");
        GH_TEXT("ESP32 AquaControl v1.0", GH_SMALL);
        GH_LINK("https://github.com/alexnaos/aqua", "Репозиторий проекта");
        GH_PANEL_END();
    });
    
    // Обработчики событий
    hub.onEvent([](GH_EVENT_TYPE type, const char* id) {
        if (type == GH_EVENT_CHANGE) {
            if (strcmp(id, "light_sw") == 0) {
                setLight(lightState);
            }
            else if (strcmp(id, "light_slider") == 0) {
                // Здесь можно реализовать ШИМ для яркости
                Serial.printf("Яркость изменена: %d%%\n", lightBrightness);
            }
            else if (strcmp(id, "pump_sw") == 0) {
                setPump(pumpState);
            }
            else if (strcmp(id, "heater_sw") == 0) {
                if (!autoMode) {
                    setHeater(heaterState);
                }
            }
            else if (strcmp(id, "auto_check") == 0) {
                if (autoMode) {
                    heaterState = false;
                    setHeater(false);
                }
            }
            else if (strcmp(id, "target_num") == 0) {
                Serial.printf("Целевая температура: %.1f°C\n", targetTemp);
            }
        }
    });
}

// ================= УПРАВЛЕНИЕ УСТРОЙСТВАМИ =================
void setLight(bool state) {
    lightState = state;
    digitalWrite(LED_PIN, state ? HIGH : LOW);
    Serial.printf("Свет: %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

void setPump(bool state) {
    pumpState = state;
    digitalWrite(PUMP_PIN, state ? HIGH : LOW);
    Serial.printf("Помпа: %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

void setHeater(bool state) {
    heaterState = state;
    digitalWrite(HEATER_PIN, state ? HIGH : LOW);
    Serial.printf("Нагреватель: %s\n", state ? "ВКЛ" : "ВЫКЛ");
}

// ================= ЧТЕНИЕ ТЕМПЕРАТУРЫ =================
float readTemperature() {
    // В реальном проекте здесь будет чтение с датчика (DS18B20, DHT22 и т.д.)
    // Для демонстрации - симуляция с небольшим разбросом
    static float baseTemp = 24.5;
    
    #ifdef TEMP_SENSOR_PIN
    // Если подключен реальный датчик
    // int sensorValue = analogRead(TEMP_SENSOR_PIN);
    // return sensorValue * (3.3 / 4095.0) * 100; // Пример конвертации
    #endif
    
    // Симуляция изменения температуры
    baseTemp += (random(-10, 11) / 100.0);
    if (baseTemp > 30.0) baseTemp = 30.0;
    if (baseTemp < 20.0) baseTemp = 20.0;
    
    return baseTemp;
}

// ================= АВТОМАТИЧЕСКОЕ УПРАВЛЕНИЕ =================
void autoControl() {
    if (!autoMode) return;
    
    currentTemp = readTemperature();
    
    // Простая логика включения/выключения нагревателя
    if (currentTemp < (targetTemp - hysteresis)) {
        setHeater(true);
    }
    else if (currentTemp > (targetTemp + hysteresis)) {
        setHeater(false);
    }
}

// ================= ОСНОВНОЙ ЦИКЛ =================
void loop() {
    // Обработка GyverHub
    hub.tick();
    
    // Автоматическое управление температурой
    autoControl();
    
    // Обновление значений в интерфейсе каждые 2 секунды
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 2000) {
        lastUpdate = millis();
        
        // Обновляем значение температуры в интерфейсе
        currentTemp = readTemperature();
        hub.updateValue("temp_val", currentTemp);
        
        // Синхронизируем переключатель нагревателя в авто-режиме
        if (autoMode) {
            hub.updateSwitch("heater_sw", heaterState);
        }
        
        Serial.printf("T: %.1f°C | Свет: %s | Помпа: %s | Нагрев: %s\n", 
                     currentTemp,
                     lightState ? "ВКЛ" : "ВЫКЛ",
                     pumpState ? "ВКЛ" : "ВЫКЛ",
                     heaterState ? "ВКЛ" : "ВЫКЛ");
    }
    
    delay(50); // Небольшая задержка для стабильности
}
