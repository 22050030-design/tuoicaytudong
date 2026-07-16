#include "safety_manager.h"

extern EventGroupHandle_t xEventGroup;

void SafetyManager::begin(PumpController* pump) {
    _pump = pump;
    _alarmActive = false;
    _waterEmpty  = false;
    _sensorFault = false;
    _pumpTimeout = false;
    _alarmMsg    = "";
}

void SafetyManager::check(const SensorData& data) {
    // ── Water empty ──
    if (!data.waterAvailable && !_waterEmpty) {
        _waterEmpty  = true;
        _alarmActive = true;
        _alarmMsg    = "NO WATER";
        _alarmTime   = millis();
        if (xEventGroup)
            xEventGroupSetBits(xEventGroup, BIT_WATER_EMPTY);
    }

    // ── Sensor error ──
    if (data.temperature < -10 || data.temperature > 60 ||
        data.airHumidity < 0   || data.airHumidity > 100) {
        _sensorFault = true;
        _alarmActive = true;
        _alarmMsg    = "SENSOR ERR";
        _alarmTime   = millis();
        if (xEventGroup)
            xEventGroupSetBits(xEventGroup, BIT_SENSOR_ERROR);
    }

    // ── Pump timeout ──
    if (_pump->isPumping() &&
        _pump->getRunDuration() >= PUMP_TIMEOUT_MS) {
        _pump->pumpOff();
        _pumpTimeout = true;
        _alarmActive = true;
        _alarmMsg    = "PUMP TIMEOUT";
        _alarmTime   = millis();
        if (xEventGroup)
            xEventGroupSetBits(xEventGroup, BIT_PUMP_RUNNING);
    }
}

void SafetyManager::clearAlarm() {
    _alarmActive = false;
    _waterEmpty  = false;
    _sensorFault = false;
    _pumpTimeout = false;
    _alarmMsg    = "";
    _pump->pumpOff();
    if (xEventGroup)
        xEventGroupClearBits(xEventGroup,
                             BIT_WATER_EMPTY | BIT_SENSOR_ERROR);
}
