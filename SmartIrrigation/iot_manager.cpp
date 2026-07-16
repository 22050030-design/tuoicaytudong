#include "iot_manager.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

extern EventGroupHandle_t xEventGroup;

static IoTManager* _self = nullptr;

// ─── Blynk Callbacks ───
BLYNK_WRITE(V6) {
    int val = param.asInt();
    _self->setCmdWater(val != 0);
}

BLYNK_CONNECTED() {
    if (xEventGroup)
        xEventGroupSetBits(xEventGroup, BIT_IOT_CONNECTED);
}

BLYNK_DISCONNECTED() {
    if (xEventGroup)
        xEventGroupClearBits(xEventGroup, BIT_IOT_CONNECTED);
}

void IoTManager::begin() {
    _self = this;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect();
    _connected = false;
    _lastSend  = 0;
}

void IoTManager::_reconnect() {
    if (!Blynk.connected()) {
        Blynk.connect();
        _connected = false;
        if (xEventGroup)
            xEventGroupClearBits(xEventGroup, BIT_IOT_CONNECTED);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    } else {
        _connected = true;
        if (WiFi.status() == WL_CONNECTED && xEventGroup)
            xEventGroupSetBits(xEventGroup, BIT_WIFI_CONNECTED);
    }
}

void IoTManager::_sendData(const SensorData& data, FSMState state,
                           bool pumpOn) {
    if (WiFi.status() != WL_CONNECTED || !Blynk.connected()) return;

    Blynk.virtualWrite(V0, data.soilMoisture);
    Blynk.virtualWrite(V1, data.temperature);
    Blynk.virtualWrite(V2, data.airHumidity);
    Blynk.virtualWrite(V3, data.waterAvailable ? 1 : 0);
    Blynk.virtualWrite(V4, pumpOn ? 1 : 0);
    Blynk.virtualWrite(V5, (int)state);

    Serial.printf("[IoT] Soil:%.1f%% T:%.1f Water:%d Pump:%d State:%d\n",
                  data.soilMoisture, data.temperature,
                  data.waterAvailable, pumpOn, state);
}

void IoTManager::_receiveCommands() {
    Blynk.run();
}

void IoTManager::update(const SensorData& data, FSMState state, bool pumpOn) {
    _reconnect();

    uint32_t now = millis();
    if (now - _lastSend >= IOT_SEND_MS) {
        _lastSend = now;
        _sendData(data, state, pumpOn);
        _receiveCommands();
    }
}

void IoTManager::setCmdWater(bool val) {
    if (_self) _self->_cmdWater = val;
}
