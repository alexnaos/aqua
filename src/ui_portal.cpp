#include "ui_portal.h"

// Описываем интерфейс настроек
void build(sets::Builder &b)
{
    // Если это действие (например, установка времени), обрабатываем его
    if (b.build.isAction())
    {
        Serial.print("Set: 0x");
        Serial.println(b.build.id, HEX);
        b.DateTime("rtc"); // Дата и время в одном флаконе
    }
    
    // Отображение текущего времени
    b.Label("Текущее время RTC: " + sett.rtc.toString());

    // Группа WiFi
    if (b.beginGroup("WiFi Settings"))
    {
        b.Input(kk::wifi_ssid, "SSID");
        b.Pass(kk::wifi_pass, "Password");
        b.endGroup();
    }

    // Группа Температуры
    if (b.beginGroup("Temperature Control"))
    {
        b.Number(kk::temp_target, "Target Temp (x10)", 100, 350);
        b.Number(kk::temp_hyst, "Hysteresis (x10)", 1, 50);
        b.endGroup();
    }

    // Группа Освещение
    if (b.beginGroup("Light Timer"))
    {
        b.Number(kk::light_start, "Start Hour", 0, 23);
        b.Number(kk::light_end, "End Hour", 0, 23);
        b.endGroup();
    }

    // Группа Устройства
    if (b.beginGroup("Devices"))
    {
        b.Switch(kk::co2_enabled, "CO2 System");
        b.Switch(kk::pump_enabled, "Main Pump");
        b.endGroup();
    }

    // Системные кнопки
    if (b.beginGroup("System"))
    {
        if (b.beginButtons())
        {
            if (b.Button("reboot"))
            {
                Serial.println("Reboot ESP");
                ESP.restart();
            }
            if (b.Button("reset_db", sets::Colors::Red))
            {
                Serial.println("Reset Settings");
                db.clear();
                db.update();
                ESP.restart();
            }
            b.endButtons();
        }
        b.endGroup();
    }
}

// Функция обновления данных в реальном времени
void update(sets::Updater &upd)
{
    // Здесь можно обновлять значения, которые меняются часто
    // Например, текущую температуру с датчика
    // upd.update(kk::temp_current, currentTemp);
}
