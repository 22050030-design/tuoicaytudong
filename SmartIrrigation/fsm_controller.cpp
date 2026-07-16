#include "fsm_controller.h"
#include <WiFi.h>

extern EventGroupHandle_t xEventGroup;

void FsmController::begin(PumpController* pump) {
    _pump = pump;
    _state = STATE_INIT;
    _stateEntryTime = millis();
    _onEnterState(STATE_INIT);
}

void FsmController::_changeState(FSMState next) {
    _prevState = _state;
    _state = next;
    _stateEntryTime = millis();
    _onEnterState(next);
}

void FsmController::_onEnterState(FSMState s) {
    switch (s) {
        case STATE_AUTO_WATERING:
        case STATE_MANUAL_WATERING:
            _pump->pumpOn();
            if (xEventGroup)
                xEventGroupSetBits(xEventGroup, BIT_PUMP_RUNNING);
            break;

        case STATE_COOLDOWN:
        case STATE_MONITORING:
        case STATE_INIT:
            _pump->pumpOff();
            if (xEventGroup)
                xEventGroupClearBits(xEventGroup, BIT_PUMP_RUNNING);
            break;

        case STATE_ALARM:
            _pump->pumpOff();
            if (xEventGroup) {
                xEventGroupClearBits(xEventGroup, BIT_PUMP_RUNNING);
                xEventGroupSetBits(xEventGroup, BIT_WATER_EMPTY);
            }
            break;

        default:
            break;
    }
}

const char* FsmController::getStateName() const {
    switch (_state) {
        case STATE_INIT:            return "INIT";
        case STATE_MONITORING:      return "MONITORING";
        case STATE_AUTO_WATERING:   return "AUTO";
        case STATE_MANUAL_WATERING: return "MANUAL";
        case STATE_COOLDOWN:        return "COOLDOWN";
        case STATE_ALARM:           return "ALARM";
        case STATE_OFFLINE:         return "OFFLINE";
    }
    return "?";
}

bool FsmController::_isScheduleTime() {
    if (!_scheduleEnabled) return false;
    // Placeholder – requires RTC or NTP time source
    return false;
}

// ─── State handlers ───────────────────────────────

void FsmController::_handleInit(const SensorData& data) {
    _changeState(STATE_MONITORING);
}

void FsmController::_handleMonitoring(const SensorData& data,
                                      bool cmdWater, bool cmdStop) {
    bool wifiOk = (WiFi.status() == WL_CONNECTED);
    if (!wifiOk) {
        _changeState(STATE_OFFLINE);
        return;
    }

    if (data.soilMoisture < MOISTURE_LOW && data.waterAvailable) {
        _changeState(STATE_AUTO_WATERING);
        return;
    }

    if (cmdWater) {
        _changeState(STATE_MANUAL_WATERING);
        return;
    }

    if (data.soilMoisture < MOISTURE_LOW && !data.waterAvailable) {
        _changeState(STATE_ALARM);
        return;
    }
}

void FsmController::_handleAutoWatering(const SensorData& data) {
    uint32_t elapsed = millis() - _stateEntryTime;

    if (elapsed >= PUMP_TIMEOUT_MS) {
        _changeState(STATE_ALARM);
        return;
    }
    if (!data.waterAvailable) {
        _changeState(STATE_ALARM);
        return;
    }
    if (data.soilMoisture >= MOISTURE_HIGH) {
        _changeState(STATE_COOLDOWN);
        return;
    }
}

void FsmController::_handleManualWatering(const SensorData& data,
                                          bool cmdStop) {
    uint32_t elapsed = millis() - _stateEntryTime;

    if (elapsed >= PUMP_TIMEOUT_MS) {
        _changeState(STATE_ALARM);
        return;
    }
    if (!data.waterAvailable) {
        _changeState(STATE_ALARM);
        return;
    }
    if (cmdStop) {
        _changeState(STATE_COOLDOWN);
        return;
    }
}

void FsmController::_handleCooldown() {
    if (millis() - _stateEntryTime >= COOLDOWN_MS) {
        _changeState(STATE_MONITORING);
    }
}

void FsmController::_handleAlarm(bool cmdStop) {
    if (cmdStop) {
        clearAlarm();
    }
}

void FsmController::_handleOffline(const SensorData& data, bool cmdWater) {
    if (WiFi.status() == WL_CONNECTED) {
        _changeState(STATE_MONITORING);
        return;
    }
    // Continue watering locally even without Internet
    if (data.soilMoisture < MOISTURE_LOW && data.waterAvailable) {
        _pump->pumpOn();
    } else {
        _pump->pumpOff();
    }
    if (cmdWater) {
        _pump->pumpOn();
    }
}

// ─── Main update ──────────────────────────────────

void FsmController::update(const SensorData& data,
                           bool cmdWater, bool cmdStop) {
    switch (_state) {
        case STATE_INIT:
            _handleInit(data);
            break;
        case STATE_MONITORING:
            _handleMonitoring(data, cmdWater, cmdStop);
            break;
        case STATE_AUTO_WATERING:
            _handleAutoWatering(data);
            break;
        case STATE_MANUAL_WATERING:
            _handleManualWatering(data, cmdStop);
            break;
        case STATE_COOLDOWN:
            _handleCooldown();
            break;
        case STATE_ALARM:
            _handleAlarm(cmdStop);
            break;
        case STATE_OFFLINE:
            _handleOffline(data, cmdWater);
            break;
    }
}

void FsmController::enterAlarm() {
    if (_state != STATE_ALARM) {
        _changeState(STATE_ALARM);
    }
}

void FsmController::clearAlarm() {
    _pump->pumpOff();
    if (xEventGroup)
        xEventGroupClearBits(xEventGroup, BIT_WATER_EMPTY | BIT_SENSOR_ERROR);
    _changeState(STATE_MONITORING);
}

void FsmController::startManual() {
    if (_state == STATE_MONITORING) {
        _changeState(STATE_MANUAL_WATERING);
    }
}

void FsmController::stopManual() {
    if (_state == STATE_MANUAL_WATERING) {
        _changeState(STATE_COOLDOWN);
    }
}
