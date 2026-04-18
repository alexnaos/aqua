#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <GyverDBFile.h>
#include <SettingsGyver.h>

// ========== WiFi Credentials (хранятся в БД) ==========
#define WIFI_SSID_DEFAULT "YourSSID"
#define WIFI_PASS_DEFAULT "YourPassword"

// ========== Pin Definitions ==========
#define PIN_PUMP 26
#define PIN_CO2 27
#define PIN_TEMP_SENSOR 34

// ========== Database Keys ==========
DB_KEYS(
    wifi_ssid,
    wifi_pass,
    temp_target,      // x10 для точности (25.0 = 250)
    temp_hyst,        // x10
    light_start,
    light_end,
    co2_enabled,
    pump_enabled
);

// ========== Global Objects ==========
GyverDBFile db(&LittleFS, "/aqua.db");
SettingsGyver sett("Aqua Control", &db);

// ========== Runtime Variables ==========
float currentTemp = 24.5;  // Заглушка, в реальности чтение с датчика
bool heaterOn = false;

// ========== Function Declarations ==========
void connectWiFi();
void controlLogic();
void build(sets::Builder& b);
void update(sets::Updater& u);

// ========== WiFi Connection ==========
void connectWiFi() {
    String ssid = db.get<String>(wifi_ssid);
    String pass = db.get<String>(wifi_pass);
    
    if (ssid.length() == 0) {
        ssid = WIFI_SSID_DEFAULT;
        pass = WIFI_PASS_DEFAULT;
    }
    
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int tries = 30;
    while (WiFi.status() != WL_CONNECTED && tries > 0) {
        delay(500);
        Serial.print(".");
        tries--;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect. Starting AP mode...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP("Aqua-Settings", "12345678");
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
    }
}

// ========== Main Control Logic ==========
void controlLogic() {
    // Чтение температуры (заглушка)
    // currentTemp = analogRead(PIN_TEMP_SENSOR) * ...;
    
    int target = db.get<int>(temp_target);
    int hyst = db.get<int>(temp_hyst);
    
    float targetTemp = target / 10.0;
    float hystTemp = hyst / 10.0;
    
    // Простая логика гистерезиса
    if (currentTemp < (targetTemp - hystTemp)) {
        heaterOn = true;
    } else if (currentTemp > (targetTemp + hystTemp)) {
        heaterOn = false;
    }
    
    bool pumpEn = db.get<bool>(pump_enabled);
    bool co2En = db.get<bool>(co2_enabled);
    
    digitalWrite(PIN_PUMP, pumpEn ? HIGH : LOW);
    digitalWrite(PIN_CO2, co2En ? HIGH : LOW);
    
    // Вывод на дисплей (заглушка)
    Serial.printf("Temp: %.1f C, Target: %.1f C, Heater: %s\n", 
                  currentTemp, targetTemp, heaterOn ? "ON" : "OFF");
}

// ========== Settings UI Builder ==========
void build(sets::Builder& b) {
    b.Label("Aqua Control System");
    b.Paragraph("WiFi and Device Settings");
    
    // WiFi Settings
    b.Input("WiFi SSID", &db.ref<String>(wifi_ssid));
    b.Pass("WiFi Password", &db.ref<String>(wifi_pass));
    
    b.hr();
    
    // Temperature Settings
    b.Number("Target Temp (x10)", &db.ref<int>(temp_target), 100, 350);
    b.Paragraph("Enter temperature * 10 (e.g., 250 = 25.0C)");
    
    b.Number("Hysteresis (x10)", &db.ref<int>(temp_hyst), 1, 50);
    b.Paragraph("Enter hysteresis * 10 (e.g., 5 = 0.5C)");
    
    b.hr();
    
    // Light Schedule
    b.Number("Light Start Hour", &db.ref<int>(light_start), 0, 23);
    b.Number("Light End Hour", &db.ref<int>(light_end), 0, 23);
    
    b.hr();
    
    // Device Toggles
    b.Switch("CO2 System Enabled", &db.ref<bool>(co2_enabled));
    b.Switch("Main Pump Enabled", &db.ref<bool>(pump_enabled));
    
    b.hr();
    
    // Actions
    if (b.Button("Reboot ESP")) {
        Serial.println("Rebooting...");
        ESP.restart();
    }
    
    if (b.Button("Reset Settings")) {
        Serial.println("Resetting settings...");
        db.reset();
        // Инициализация дефолтных значений
        db.init(wifi_ssid, String(WIFI_SSID_DEFAULT));
        db.init(wifi_pass, String(WIFI_PASS_DEFAULT));
        db.init(temp_target, 250);  // 25.0C
        db.init(temp_hyst, 5);      // 0.5C
        db.init(light_start, 8);
        db.init(light_end, 20);
        db.init(co2_enabled, false);
        db.init(pump_enabled, true);
        ESP.restart();
    }
}

// ========== Settings Updater (для динамических обновлений) ==========
void update(sets::Updater& u) {
    // Здесь можно обрабатывать изменения в реальном времени
    // Например, если нужно что-то сделать при изменении настроек
}

// ========== Setup ==========
void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== Aqua Control System ===");
    
    // Init Pins
    pinMode(PIN_PUMP, OUTPUT);
    pinMode(PIN_CO2, OUTPUT);
    pinMode(PIN_TEMP_SENSOR, INPUT);
    
    // Init File System
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS mount failed!");
        return;
    }
    Serial.println("LittleFS mounted");
    
    // Init Database
    db.begin();
    
    // Initialize default values if not exist
    db.init(wifi_ssid, String(WIFI_SSID_DEFAULT));
    db.init(wifi_pass, String(WIFI_PASS_DEFAULT));
    db.init(temp_target, 250);  // 25.0C
    db.init(temp_hyst, 5);      // 0.5C
    db.init(light_start, 8);
    db.init(light_end, 20);
    db.init(co2_enabled, false);
    db.init(pump_enabled, true);
    
    // Connect WiFi
    connectWiFi();
    
    // Init Settings UI
    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(update);
    
    Serial.println("System ready. Open http://" + WiFi.localIP().toString());
}

// ========== Loop ==========
void loop() {
    sett.tick();  // Обработка веб-интерфейса
    
    static unsigned long lastControl = 0;
    if (millis() - lastControl > 1000) {
        lastControl = millis();
        controlLogic();
    }
    
    delay(10);
}
