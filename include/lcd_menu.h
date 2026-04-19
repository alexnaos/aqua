#pragma once
#include <LiquidCrystal_I2C.h>
#include <GyverEncoder.h>
#include <GyverDB.h>

// Конфигурация LCD
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Конфигурация энкодера
#define ENC_PIN_A 16
#define ENC_PIN_B 17
#define ENC_BTN 18

// Глобальные объекты
extern LiquidCrystal_I2C lcd;
extern GyverEncoder encoder;

// Структура меню
struct MenuItem {
    const char* name;
    int value;
    int min;
    int max;
    bool editable;
};

// Класс управления LCD и меню
class LcdMenu {
private:
    int currentMenuIndex = 0;
    int totalMenus = 0;
    bool inEditMode = false;
    unsigned long lastUpdate = 0;
    
    // Массив пунктов меню
    MenuItem menus[10];
    
    // Указатель на базу данных
    GyverDBFile* dbPtr = nullptr;
    
    // Инициализация пунктов меню
    void initMenus() {
        totalMenus = 0;
        
        // Температура цель
        menus[totalMenus++] = {"Temp Target", 240, 150, 350, true};
        // Температура гистерезис
        menus[totalMenus++] = {"Temp Hyst", 5, 1, 20, true};
        // Помпа вкл/выкл
        menus[totalMenus++] = {"Pump", 1, 0, 1, true};
        // CO2 вкл/выкл
        menus[totalMenus++] = {"CO2", 0, 0, 1, true};
        // Свет старт
        menus[totalMenus++] = {"Light Start", 8, 0, 23, true};
        // Свет конец
        menus[totalMenus++] = {"Light End", 20, 0, 23, true};
    }
    
    // Загрузка значений из БД
    void loadValues() {
        if (!dbPtr) return;
        
        menus[0].value = (*dbPtr)[kk::temp_target].toInt32();
        menus[1].value = (*dbPtr)[kk::temp_hyst].toInt32();
        menus[2].value = (*dbPtr)[kk::pump_enabled].toBool() ? 1 : 0;
        menus[3].value = (*dbPtr)[kk::co2_enabled].toBool() ? 1 : 0;
        menus[4].value = (*dbPtr)[kk::light_start].toInt32();
        menus[5].value = (*dbPtr)[kk::light_end].toInt32();
    }
    
    // Сохранение значений в БД
    void saveValue(int index) {
        if (!dbPtr) return;
        
        switch(index) {
            case 0: (*dbPtr)[kk::temp_target] = menus[0].value; break;
            case 1: (*dbPtr)[kk::temp_hyst] = menus[1].value; break;
            case 2: (*dbPtr)[kk::pump_enabled] = (menus[2].value == 1); break;
            case 3: (*dbPtr)[kk::co2_enabled] = (menus[3].value == 1); break;
            case 4: (*dbPtr)[kk::light_start] = menus[4].value; break;
            case 5: (*dbPtr)[kk::light_end] = menus[5].value; break;
        }
        dbPtr->save();
    }
    
public:
    // Инициализация
    void begin(GyverDBFile* db) {
        dbPtr = db;
        
        // Инициализация LCD
        lcd.begin(LCD_COLS, LCD_ROWS);
        lcd.backlight();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Aqua Control");
        lcd.setCursor(0, 1);
        lcd.print("Initializing...");
        delay(1000);
        
        // Инициализация энкодера
        encoder.begin();
        encoder.setAccelTimeout(500);
        
        initMenus();
        loadValues();
        
        // Обработчики энкодера (используем лямбды GyverEncoder)
        encoder.setClickHandler([]() {
            getInstance().onEncoderClick();
        });
        
        encoder.setLongPressHandler([]() {
            getInstance().onEncoderLongPress();
        });
        
        encoder.setEncoderHandler([](int dir) {
            getInstance().onEncoderTurn(dir);
        });
    }
    
    // Синглтон паттерн для доступа из прерываний
    static LcdMenu& getInstance() {
        static LcdMenu instance;
        return instance;
    }
    
    // Обработка клика энкодера
    void onEncoderClick() {
        if (inEditMode) {
            // Сохраняем значение и выходим из режима редактирования
            saveValue(currentMenuIndex);
            inEditMode = false;
        } else {
            // Входим в режим редактирования текущего пункта
            inEditMode = true;
        }
    }
    
    // Обработка долгого нажатия
    void onEncoderLongPress() {
        // Выход в главное меню (сброс индекса)
        currentMenuIndex = 0;
        inEditMode = false;
        updateDisplay();
    }
    
    // Обработка вращения энкодера
    void onEncoderTurn(int direction) {
        if (inEditMode) {
            // Изменение значения текущего пункта
            int newValue = menus[currentMenuIndex].value + direction;
            if (newValue >= menus[currentMenuIndex].min && 
                newValue <= menus[currentMenuIndex].max) {
                menus[currentMenuIndex].value = newValue;
                updateDisplay();
            }
        } else {
            // Навигация по меню
            currentMenuIndex += direction;
            if (currentMenuIndex < 0) currentMenuIndex = totalMenus - 1;
            if (currentMenuIndex >= totalMenus) currentMenuIndex = 0;
            updateDisplay();
        }
    }
    
    // Обновление дисплея
    void updateDisplay() {
        lcd.clear();
        
        if (inEditMode) {
            // Режим редактирования
            lcd.setCursor(0, 0);
            lcd.print(menus[currentMenuIndex].name);
            lcd.setCursor(0, 1);
            lcd.print("> ");
            lcd.print(menus[currentMenuIndex].value);
            lcd.print(" <");
        } else {
            // Режим просмотра
            lcd.setCursor(0, 0);
            lcd.print(menus[currentMenuIndex].name);
            lcd.setCursor(0, 1);
            lcd.print("= ");
            lcd.print(menus[currentMenuIndex].value);
            
            // Индикатор прокрутки
            if (totalMenus > 1) {
                lcd.setCursor(LCD_COLS - 1, 0);
                lcd.print(currentMenuIndex + 1);
                lcd.print("/");
                lcd.print(totalMenus);
            }
        }
    }
    
    // Обновление данных с датчиков (температура, статус)
    void updateSensors(float temp, bool pumpState, bool co2State, bool lightState) {
        // Показываем температуру на втором экране (если не в режиме редактирования)
        if (!inEditMode && millis() - lastUpdate > 2000) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("T:");
            lcd.print(temp, 1);
            lcd.print("C P:");
            lcd.print(pumpState ? "1" : "0");
            
            lcd.setCursor(0, 1);
            lcd.print("C:");
            lcd.print(co2State ? "1" : "0");
            lcd.print(" L:");
            lcd.print(lightState ? "1" : "0");
            
            lastUpdate = millis();
        }
    }
    
    // Основной цикл (обработка энкодера)
    void tick() {
        encoder.tick();
    }
};

// Глобальные экземпляры
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
GyverEncoder encoder(ENC_PIN_A, ENC_PIN_B, ENC_BTN);
LcdMenu lcdMenu;
