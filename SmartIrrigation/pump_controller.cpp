#include "pump_controller.h"

void PumpController::begin() {
    pinMode(PIN_RELAY, OUTPUT);
    digitalWrite(PIN_RELAY, LOW);      // Relay OFF at start
    _pumpActive = false;
    _startTime  = 0;
}

void PumpController::pumpOn() {
    if (!_pumpActive) {
        digitalWrite(PIN_RELAY, HIGH); // Active-HIGH relay
        _pumpActive = true;
        _startTime  = millis();
    }
}

void PumpController::pumpOff() {
    digitalWrite(PIN_RELAY, LOW);
    _pumpActive = false;
    _startTime  = 0;
}

uint32_t PumpController::getRunDuration() const {
    if (!_pumpActive) return 0;
    return millis() - _startTime;
}

void PumpController::reset() {
    pumpOff();
}
