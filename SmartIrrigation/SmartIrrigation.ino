// ══════════════════════════════════════════════════════════════════════════════
// SmartIrrigation.ino
// Mã đề 11 – Hệ thống tưới cây thông minh sử dụng ESP32, IoT, FSM, FreeRTOS
// ══════════════════════════════════════════════════════════════════════════════

#include "config.h"
#include "sensor_manager.h"
#include "pump_controller.h"
#include "fsm_controller.h"
#include "display_manager.h"
#include "iot_manager.h"
#include "safety_manager.h"

// ─── Module instances ───
SensorManager   sensorMgr;
PumpController  pump;
FsmController   fsm;
DisplayManager  display;
IoTManager      iotMgr;
SafetyManager   safety;

// ─── FreeRTOS handles ───
QueueHandle_t   xSensorQueue   = NULL;
SemaphoreHandle_t xBtnSem      = NULL;
EventGroupHandle_t xEventGroup = NULL;

// ─── Shared state (protected by mutex) ───
SemaphoreHandle_t xStateMutex = NULL;
SensorData g_sensorData;
FSMState   g_fsmState    = STATE_INIT;
bool       g_pumpOn      = false;
bool       g_alarmActive = false;
const char*g_alarmMsg    = "";

// ─── Button state ───
volatile bool g_btnWater      = false;
volatile bool g_btnStop       = false;
volatile bool g_btnResetAlarm = false;
uint32_t      g_lastBtnWater = 0;
uint32_t      g_lastBtnStop  = 0;
uint32_t      g_lastBtnReset = 0;

// ══════════════════════════════════════════════════════════════════════════════
//  ISR – Button interrupts
// ══════════════════════════════════════════════════════════════════════════════

void IRAM_ATTR ISR_BtnWater() {
    uint32_t now = millis();
    if (now - g_lastBtnWater > DEBOUNCE_MS) {
        g_btnWater = true;
        g_lastBtnWater = now;
    }
}

void IRAM_ATTR ISR_BtnStop() {
    uint32_t now = millis();
    if (now - g_lastBtnStop > DEBOUNCE_MS) {
        g_btnStop = true;
        g_lastBtnStop = now;
    }
}

void IRAM_ATTR ISR_BtnReset() {
    uint32_t now = millis();
    if (now - g_lastBtnReset > DEBOUNCE_MS) {
        g_btnResetAlarm = true;
        g_lastBtnReset = now;
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  FreeRTOS Tasks
// ══════════════════════════════════════════════════════════════════════════════

void vSensorTask(void* pvParameters) {
    SensorData data;
    for (;;) {
        data = sensorMgr.read();

        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        g_sensorData = data;
        xSemaphoreGive(xStateMutex);

        xQueueOverwrite(xSensorQueue, &data);

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_MS));
    }
}

void vControlTask(void* pvParameters) {
    SensorData data;
    for (;;) {
        if (xQueueReceive(xSensorQueue, &data, portMAX_DELAY) == pdTRUE) {

            bool cmdWater = false;
            bool cmdStop  = false;

            if (g_btnWater) {
                cmdWater = true;
                g_btnWater = false;
            }
            if (g_btnStop) {
                cmdStop = true;
                g_btnStop = false;
            }

            fsm.update(data, cmdWater, cmdStop);

            xSemaphoreTake(xStateMutex, portMAX_DELAY);
            g_fsmState    = fsm.getState();
            g_pumpOn      = pump.isPumping();
            g_alarmActive = safety.isAlarmActive();
            g_alarmMsg    = safety.getAlarmMessage();
            xSemaphoreGive(xStateMutex);
        }
    }
}

void vSafetyTask(void* pvParameters) {
    SensorData data;
    for (;;) {
        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        data = g_sensorData;
        xSemaphoreGive(xStateMutex);

        safety.check(data);

        if (safety.isAlarmActive()) {
            fsm.enterAlarm();
        }

        if (g_btnResetAlarm && safety.isAlarmActive()) {
            safety.clearAlarm();
            fsm.clearAlarm();
            g_btnResetAlarm = false;
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void vDisplayTask(void* pvParameters) {
    for (;;) {
        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        SensorData data = g_sensorData;
        FSMState   st   = g_fsmState;
        bool       pOn  = g_pumpOn;
        bool       alm  = g_alarmActive;
        const char* msg = g_alarmMsg;
        xSemaphoreGive(xStateMutex);

        display.update(data, st, pOn, alm, msg);

        vTaskDelay(pdMS_TO_TICKS(DISPLAY_UPDATE_MS));
    }
}

void vIoTTask(void* pvParameters) {
    for (;;) {
        xSemaphoreTake(xStateMutex, portMAX_DELAY);
        SensorData data = g_sensorData;
        FSMState   st   = g_fsmState;
        bool       pOn  = g_pumpOn;
        xSemaphoreGive(xStateMutex);

        iotMgr.update(data, st, pOn);

        if (iotMgr.isCommandWater()) {
            g_btnWater = true;
        }

        vTaskDelay(pdMS_TO_TICKS(IOT_SEND_MS));
    }
}

// ══════════════════════════════════════════════════════════════════════════════
//  setup() & loop()
// ══════════════════════════════════════════════════════════════════════════════

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== SmartIrrigation v" FIRMWARE_VERSION " ===");

    Wire.begin(PIN_SDA, PIN_SCL);

    // ── Buttons ──
    pinMode(PIN_BTN_AUTO_MANUAL, INPUT_PULLUP);
    pinMode(PIN_BTN_WATER,       INPUT_PULLUP);
    pinMode(PIN_BTN_RESET,       INPUT_PULLUP);

    attachInterrupt(digitalPinToInterrupt(PIN_BTN_WATER), ISR_BtnWater, FALLING);
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_RESET), ISR_BtnReset, FALLING);

    // ── FreeRTOS objects ──
    xSensorQueue  = xQueueCreate(1, sizeof(SensorData));
    xStateMutex   = xSemaphoreCreateMutex();
    xEventGroup   = xEventGroupCreate();

    // ── Module init ──
    sensorMgr.begin();
    pump.begin();
    fsm.begin(&pump);
    display.begin();
    safety.begin(&pump);
    iotMgr.begin();

    Serial.println("[INIT] Modules ready");

    // ── Create tasks ──
    xTaskCreatePinnedToCore(vSensorTask,  "Sensor",  STACK_SENSOR,  NULL, PRI_SENSOR,  NULL, 0);
    xTaskCreatePinnedToCore(vControlTask, "Control", STACK_CONTROL, NULL, PRI_CONTROL, NULL, 1);
    xTaskCreatePinnedToCore(vSafetyTask,  "Safety",  STACK_SAFETY,  NULL, PRI_SAFETY,  NULL, 1);
    xTaskCreatePinnedToCore(vDisplayTask, "Display", STACK_DISPLAY, NULL, PRI_DISPLAY, NULL, 0);
    xTaskCreatePinnedToCore(vIoTTask,     "IoT",     STACK_IOT,     NULL, PRI_IOT,     NULL, 0);

    Serial.println("[INIT] FreeRTOS tasks created on dual cores");
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}
