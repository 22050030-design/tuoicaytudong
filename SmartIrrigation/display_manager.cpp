#include "display_manager.h"

void DisplayManager::begin() {
    _lcd = new LiquidCrystal_I2C(0x27, 16, 2);
    _lcd->init();
    _lcd->backlight();
    _lcd->clear();
    _lcd->setCursor(0, 0);
    _lcd->print("SmartIrrigation");
    _lcd->setCursor(0, 1);
    _lcd->print(FIRMWARE_VERSION);

    pinMode(PIN_LED_GREEN,  OUTPUT);
    pinMode(PIN_LED_YELLOW, OUTPUT);
    pinMode(PIN_LED_RED,    OUTPUT);
    pinMode(PIN_BUZZER,     OUTPUT);

    digitalWrite(PIN_LED_GREEN,  LOW);
    digitalWrite(PIN_LED_YELLOW, LOW);
    digitalWrite(PIN_LED_RED,    LOW);
    digitalWrite(PIN_BUZZER,     LOW);
}

void DisplayManager::update(const SensorData& data, FSMState state,
                            bool pumpOn, bool alarmActive,
                            const char* alarmMsg) {
    uint32_t now = millis();
    if (now - _lastUpdate < DISPLAY_UPDATE_MS) return;
    _lastUpdate = now;

    _updateLCD(data, state, pumpOn, alarmActive, alarmMsg);
    _updateLEDs(state, alarmActive);
    _updateBuzzer(alarmActive);
}

void DisplayManager::_updateLCD(const SensorData& data, FSMState state,
                                bool pumpOn, bool alarmActive,
                                const char* alarmMsg) {
    // Line 1: Soil + Temp
    _lcd->setCursor(0, 0);
    char buf[17];
    snprintf(buf, sizeof(buf), "S:%3d%% T:%4.1fC",
             (int)data.soilMoisture, data.temperature);
    _lcd->print(buf);

    // Line 2: State + Water
    _lcd->setCursor(0, 1);
    if (alarmActive && alarmMsg) {
        _alarmBlink++;
        if (_alarmBlink % 2 == 0) {
            snprintf(buf, sizeof(buf), "!! %s !!", alarmMsg);
        } else {
            snprintf(buf, sizeof(buf), "!!  ALARM    !!");
        }
    } else {
        const char* st = "??";
        switch (state) {
            case STATE_INIT:            st = "INIT";     break;
            case STATE_MONITORING:      st = "READY";    break;
            case STATE_AUTO_WATERING:   st = "AUTO";     break;
            case STATE_MANUAL_WATERING: st = "MANUAL";   break;
            case STATE_COOLDOWN:        st = "COOL";     break;
            case STATE_ALARM:           st = "ALARM";    break;
            case STATE_OFFLINE:         st = "OFFLINE";  break;
        }
        snprintf(buf, sizeof(buf), "%s W:%s",
                 st, data.waterAvailable ? "OK" : "EMPTY");
    }
    _lcd->print(buf);
}

void DisplayManager::_updateLEDs(FSMState state, bool alarmActive) {
    digitalWrite(PIN_LED_GREEN,  LOW);
    digitalWrite(PIN_LED_YELLOW, LOW);
    digitalWrite(PIN_LED_RED,    LOW);

    if (alarmActive) {
        digitalWrite(PIN_LED_RED, (millis() / 500) % 2);  // blink
        return;
    }
    switch (state) {
        case STATE_AUTO_WATERING:
        case STATE_MANUAL_WATERING:
            digitalWrite(PIN_LED_YELLOW, HIGH);
            break;
        case STATE_OFFLINE:
            digitalWrite(PIN_LED_GREEN,  (millis() / 1000) % 2);
            break;
        case STATE_MONITORING:
        case STATE_COOLDOWN:
        default:
            digitalWrite(PIN_LED_GREEN, HIGH);
            break;
    }
}

void DisplayManager::_updateBuzzer(bool alarmActive) {
    digitalWrite(PIN_BUZZER, alarmActive && ((millis() / 1000) % 2));
}
