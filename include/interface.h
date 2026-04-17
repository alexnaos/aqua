#pragma once
#include <GyverHub.h>

// Объявление виджетов
GH_BUTTON(btn_light, "Освещение", "💡");
GH_BUTTON(btn_pump, "Помпа", "🌊");
GH_BUTTON(btn_heater, "Нагрев", "🔥");
GH_SLIDER(sld_temp, "Цель темп.", 15, 35, 1);
GH_LABEL(lbl_temp, "Тек. темп.", "0.0 C");
GH_LABEL(lbl_status, "Статус", "ОК");

// Функция инициализации интерфейса
void setupInterface() {
  Hub.attach(btn_light);
  Hub.attach(btn_pump);
  Hub.attach(btn_heater);
  Hub.attach(sld_temp);
  Hub.attach(lbl_temp);
  Hub.attach(lbl_status);

  // Начальные значения
  sld_temp = 25;
}

// Функция обновления интерфейса (вызывать в loop)
void updateInterface(float currentTemp, bool lightState, bool pumpState, bool heaterState) {
  lbl_temp = String(currentTemp, 1) + " C";
  
  // Обновление иконок кнопок в зависимости от состояния
  btn_light.state(lightState ? GH_PRESSED : GH_RELEASED);
  btn_pump.state(pumpState ? GH_PRESSED : GH_RELEASED);
  btn_heater.state(heaterState ? GH_PRESSED : GH_RELEASED);
}
