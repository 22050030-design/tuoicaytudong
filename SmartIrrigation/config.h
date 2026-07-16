#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define FIRMWARE_VERSION "1.0.0"
#define DEVICE_NAME "SmartIrrigation"

// ─── Wi-Fi ───
#define WIFI_SSID         "OPPO A60"
#define WIFI_PASSWORD     "33333333"
#define WIFI_RECONNECT_MS 5000

// ─── IoT Platform – Blynk 2.0 ───
#define BLYNK_TEMPLATE_ID   "TMPL6-5Bsscw0"
#define BLYNK_TEMPLATE_NAME "tuoicaytudong"
#define BLYNK_AUTH_TOKEN    "Mn-34B7mWGxaNz_Yiu_71Cc5j-G1a78T"
#define BLYNK_PRINT Serial

// ─── GPIO Pin Mapping ───
// Sensors
#define PIN_SOIL_MOISTURE   34   // ADC1_CH6 – Capacitive soil sensor
#define PIN_WATER_LEVEL     35   // ADC1_CH7 – Float switch / water level sensor
#define PIN_DHT             4    // DHT22 data

// Buttons (INPUT_PULLUP – active LOW)
#define PIN_BTN_AUTO_MANUAL 32
#define PIN_BTN_WATER       33
#define PIN_BTN_RESET       26

// Outputs
#define PIN_RELAY           12   // Relay IN – Pump control
#define PIN_LED_GREEN       14   // System OK
#define PIN_LED_YELLOW      27   // Pumping
#define PIN_LED_RED         25   // Alarm
#define PIN_BUZZER          23   // Active buzzer

// I2C
#define PIN_SDA             21
#define PIN_SCL             22

// ─── Sensor Calibration ───
#define SOIL_ADC_DRY        3200   // Raw ADC when completely dry  (v2.0 capacitive)
#define SOIL_ADC_WET        1400   // Raw ADC when fully saturated
#define SOIL_MOISTURE_MIN   0
#define SOIL_MOISTURE_MAX   100

#define WATER_LEVEL_DRY     500    // ADC threshold – tank empty
#define WATER_LEVEL_WET     3000   // ADC threshold – tank has water

// ─── Irrigation Thresholds ───
#define MOISTURE_LOW        35     // Start watering when below this %
#define MOISTURE_HIGH       60     // Stop watering when above this %

// ─── Timing (ms) ───
#define PUMP_TIMEOUT_MS     15000  // Max pump runtime 15 s
#define COOLDOWN_MS         30000  // Wait 30 s after watering
#define SENSOR_READ_MS      1000
#define DISPLAY_UPDATE_MS   500
#define IOT_SEND_MS         2000
#define DEBOUNCE_MS         50

// ─── FreeRTOS Config ───
#define STACK_SENSOR   2048
#define STACK_CONTROL  4096
#define STACK_DISPLAY  2048
#define STACK_IOT      8192
#define STACK_SAFETY   2048

#define PRI_SENSOR     2
#define PRI_CONTROL    3
#define PRI_DISPLAY    1
#define PRI_IOT        2
#define PRI_SAFETY     4   // Highest – safety first

#define QUEUE_LENGTH   10

// ─── Event Group Bits ───
#define BIT_WIFI_CONNECTED   (1 << 0)
#define BIT_IOT_CONNECTED    (1 << 1)
#define BIT_WATER_EMPTY      (1 << 2)
#define BIT_SENSOR_ERROR     (1 << 3)
#define BIT_PUMP_RUNNING     (1 << 4)

// ─── FSM States ───
enum FSMState {
    STATE_INIT,
    STATE_MONITORING,
    STATE_AUTO_WATERING,
    STATE_MANUAL_WATERING,
    STATE_COOLDOWN,
    STATE_ALARM,
    STATE_OFFLINE
};

// ─── Sensor Data Structure ───
struct SensorData {
    uint16_t soilRaw;
    float    soilMoisture;   // 0-100 %
    float    temperature;    // °C
    float    airHumidity;    // %
    bool     waterAvailable; // true = water in tank
    uint32_t timestamp;
};

#endif // CONFIG_H
