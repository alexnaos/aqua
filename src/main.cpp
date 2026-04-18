#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SettingsGyver.h>

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

// === ПЕРЕМЕННЫЕ НАСТРОЕК ===
char wifi_ssid[32] = "MyAquarium";
char wifi_pass[32] = "12345678";

float temp_target = 25.0f;
float temp_hyst   = 0.5f;

int light_start = 8;
int light_end   = 20;

bool co2_enabled = true;
bool pump_enabled = true;

// === ОБЪЕКТ НАСТРОЕК ===
SettingsGyver sett("AquaControlDB");

// === ФУНКЦИЯ ПОСТРОЕНИЯ ИНТЕРФЕЙСА ===
// Используем PTR() для передачи указателей на переменные
void build(sets::Builder& b) {
    // WiFi
    b.Input("WiFi SSID", PTR(wifi_ssid));
    b.Input("WiFi Pass", PTR(wifi_pass));

    // Температура
    // Сигнатура: Slider(ID, Label, Min, Max, Step, Unit, Ptr, Color)
    // ID можно опустить (автогенерация), если не нужно специфическое управление
    b.Slider("Target Temp", 10.0f, 35.0f, 0.1f, "", PTR(temp_target));
    b.Slider("Hysteresis", 0.1f, 5.0f, 0.1f, "", PTR(temp_hyst));

    // Время света (Number для int)
    // Сигнатура: Number(ID, Label, Min, Max, Ptr, Color)
    b.Number("Light Start", 0, 23, PTR(light_start));
    b.Number("Light End", 0, 23, PTR(light_end));

    // Переключатели
    // Сигнатура: Switch(ID, Label, Ptr, Color)
    b.Switch("CO2 System", PTR(co2_enabled));
    b.Switch("Main Pump", PTR(pump_enabled));

    // Кнопка перезагрузки
    // Кнопка не имеет привязки к переменной, но требует ID для обработки события
    b.Button("Reboot ESP"); 
}

void connectWiFi() {
    Serial.print("Connecting to ");
    Serial.println(wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_pass);

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

// Обработчик событий (для кнопки Reboot)
void eventHandler(sets::Event& ev) {
    if (ev.isButton("Reboot ESP")) {
        Serial.println("Rebooting...");
        ESP.restart();
    }
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

    sett.begin();
    sett.onBuild(build);
    sett.onEvent(eventHandler); // Подключаем обработчик событий

    connectWiFi();

    display.setCursor(0, 0);
    display.println("System Ready");
    display.println("Open Web UI");
    display.display();
}

void loop() {
    sett.tick();

    static unsigned long timer = 0;
    if (millis() - timer > 1000) {
        timer = millis();
        controlLogic();
    }
}