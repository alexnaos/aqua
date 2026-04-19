#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "config.h"

// --- Настройки MQTT ---
#define MQTT_SERVER "192.168.1.100"  // Укажите IP вашего MQTT сервера
#define MQTT_PORT 1883
#define MQTT_USER ""                 // Укажите пользователя, если требуется
#define MQTT_PASS ""                 // Укажите пароль, если требуется
#define MQTT_CLIENT_ID "aqua_esp32"

// Топики для публикации данных датчиков (отправка)
#define MQTT_TOPIC_TEMP         "aqua/sensors/temperature"
#define MQTT_TOPIC_PUMP         "aqua/sensors/pump"
#define MQTT_TOPIC_CO2          "aqua/sensors/co2"
#define MQTT_TOPIC_LIGHT        "aqua/sensors/light"
#define MQTT_TOPIC_HUMIDITY     "aqua/sensors/humidity"
#define MQTT_TOPIC_PH           "aqua/sensors/ph"
#define MQTT_TOPIC_TDS          "aqua/sensors/tds"
#define MQTT_TOPIC_WATER_LEVEL  "aqua/sensors/water_level"
#define MQTT_TOPIC_STATUS       "aqua/status"

// Топики для подписки на команды (получение)
#define MQTT_TOPIC_CMD_PUMP     "aqua/commands/pump"
#define MQTT_TOPIC_CMD_CO2      "aqua/commands/co2"
#define MQTT_TOPIC_CMD_LIGHT    "aqua/commands/light"
#define MQTT_TOPIC_CMD_TARGET   "aqua/commands/temp_target"
#define MQTT_TOPIC_CMD_HYST     "aqua/commands/temp_hyst"

class MqttHandler {
private:
    WiFiClient espClient;
    PubSubClient client;
    unsigned long lastReconnectAttempt = 0;
    bool connected = false;

    void callback(char* topic, byte* payload, unsigned int length) {
        String message;
        for (unsigned int i = 0; i < length; i++) {
            message += (char)payload[i];
        }

        Serial.print("MQTT Message received on topic: ");
        Serial.println(topic);
        Serial.print("Message: ");
        Serial.println(message);

        // Обработка команд
        if (String(topic) == MQTT_TOPIC_CMD_PUMP) {
            bool state = (message == "ON" || message == "1" || message == "true");
            db[kk::pump_enabled] = state;
            Serial.println("Pump command processed");
        }
        else if (String(topic) == MQTT_TOPIC_CMD_CO2) {
            bool state = (message == "ON" || message == "1" || message == "true");
            db[kk::co2_enabled] = state;
            Serial.println("CO2 command processed");
        }
        else if (String(topic) == MQTT_TOPIC_CMD_LIGHT) {
            bool state = (message == "ON" || message == "1" || message == "true");
            // Можно добавить обработку для ручного управления светом
            Serial.println("Light command processed");
        }
        else if (String(topic) == MQTT_TOPIC_CMD_TARGET) {
            float target = message.toFloat();
            db[kk::temp_target] = (int32_t)(target * 10);
            Serial.println("Temperature target command processed");
        }
        else if (String(topic) == MQTT_TOPIC_CMD_HYST) {
            float hyst = message.toFloat();
            db[kk::temp_hyst] = (int32_t)(hyst * 10);
            Serial.println("Temperature hysteresis command processed");
        }
    }

    boolean reconnect() {
        Serial.print("Attempting MQTT connection...");
        if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
            Serial.println("connected");
            
            // Подписка на топики команд
            client.subscribe(MQTT_TOPIC_CMD_PUMP);
            client.subscribe(MQTT_TOPIC_CMD_CO2);
            client.subscribe(MQTT_TOPIC_CMD_LIGHT);
            client.subscribe(MQTT_TOPIC_CMD_TARGET);
            client.subscribe(MQTT_TOPIC_CMD_HYST);
            
            // Публикация статуса подключения
            client.publish(MQTT_TOPIC_STATUS, "connected");
            
            return true;
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
            return false;
        }
    }

public:
    MqttHandler() : client(espClient) {}

    void begin() {
        client.setServer(MQTT_SERVER, MQTT_PORT);
        client.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->callback(topic, payload, length);
        });
        lastReconnectAttempt = 0;
    }

    void tick() {
        if (!client.connected()) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 5000) {
                lastReconnectAttempt = now;
                if (reconnect()) {
                    connected = true;
                }
            }
        } else {
            client.loop();
        }
    }

    bool isConnected() {
        return client.connected();
    }

    // Отправка данных датчиков
    void publishTemperature(float value) {
        char buffer[10];
        dtostrf(value, 4, 2, buffer);
        client.publish(MQTT_TOPIC_TEMP, buffer);
    }

    void publishPumpState(bool state) {
        client.publish(MQTT_TOPIC_PUMP, state ? "ON" : "OFF");
    }

    void publishCo2State(bool state) {
        client.publish(MQTT_TOPIC_CO2, state ? "ON" : "OFF");
    }

    void publishLightState(bool state) {
        client.publish(MQTT_TOPIC_LIGHT, state ? "ON" : "OFF");
    }

    void publishHumidity(float value) {
        char buffer[10];
        dtostrf(value, 4, 2, buffer);
        client.publish(MQTT_TOPIC_HUMIDITY, buffer);
    }

    void publishPh(float value) {
        char buffer[10];
        dtostrf(value, 4, 2, buffer);
        client.publish(MQTT_TOPIC_PH, buffer);
    }

    void publishTds(int value) {
        char buffer[10];
        itoa(value, buffer, 10);
        client.publish(MQTT_TOPIC_TDS, buffer);
    }

    void publishWaterLevel(float value) {
        char buffer[10];
        dtostrf(value, 4, 2, buffer);
        client.publish(MQTT_TOPIC_WATER_LEVEL, buffer);
    }

    void publishStatus(const char* status) {
        client.publish(MQTT_TOPIC_STATUS, status);
    }

    void disconnect() {
        client.publish(MQTT_TOPIC_STATUS, "disconnected");
        client.disconnect();
        connected = false;
    }
};

#endif // MQTT_HANDLER_H
