# SmartIrrigation – Hệ thống tưới cây thông minh

**Mã đề 11** – Xây dựng hệ thống tưới cây thông minh sử dụng ESP32, IoT, FSM và FreeRTOS

---

## Kiến trúc tổng thể

```
Cảm biến độ ẩm đất ──┐
Cảm biến mực nước ───┤
DHT22 ───────────────┤
Nút điều khiển ──────┤
                     ▼
                   ESP32
         ┌───────────┼─────────────┐
         ▼           ▼             ▼
   FSM điều khiển  FreeRTOS     Wi-Fi/IoT
         │           │             │
         ▼           ▼             ▼
    Relay/Máy bơm  LCD/LED      Blynk
```

## Cấu trúc thư mục

```
SmartIrrigation/
├── SmartIrrigation.ino       // File chính – FreeRTOS tasks, ISR
├── config.h                  // Cấu hình GPIO, ngưỡng, FreeRTOS
├── sensor_manager.h/.cpp     // Đọc cảm biến đất, DHT22, mực nước
├── pump_controller.h/.cpp    // Điều khiển relay / máy bơm
├── fsm_controller.h/.cpp     // Finite State Machine (7 trạng thái)
├── display_manager.h/.cpp    // LCD I2C, LED, Buzzer
├── iot_manager.h/.cpp        // Wi-Fi, IoT (ERa/Blynk)
├── safety_manager.h/.cpp     // Kiểm tra an toàn, timeout
└── README.md
```

## Yêu cầu phần cứng

| Thành phần               | Ghi chú                           |
|--------------------------|-----------------------------------|
| ESP32 DevKit V4          | Bộ điều khiển trung tâm          |
| Cảm biến độ ẩm đất v2.0 | Loại điện dung – GPIO 34         |
| Float switch / công tắc phao | GPIO 35                      |
| DHT22                    | GPIO 4                           |
| Relay 1 kênh             | GPIO 12 → Relay IN               |
| LCD I2C 16×2             | Addr 0x27, SDA=21, SCL=22       |
| Nút nhấn ×3              | GPIO 32, 33, 26 (INPUT_PULLUP)  |
| LED ×3 + resistor 220Ω   | GPIO 14 (xanh), 27 (vàng), 25 (đỏ) |
| Active buzzer            | GPIO 23                          |
| Nguồn 5 V / 12 V riêng  | Cấp cho relay + máy bơm        |

## Bảng GPIO

| GPIO | Chức năng        |
|------|------------------|
| 4    | DHT22 DATA       |
| 12   | Relay IN         |
| 14   | LED xanh         |
| 21   | SDA (LCD)        |
| 22   | SCL (LCD)        |
| 23   | Buzzer           |
| 25   | LED đỏ           |
| 26   | Nút Reset        |
| 27   | LED vàng         |
| 32   | Nút Auto/Manual  |
| 33   | Nút tưới         |
| 34   | Cảm biến đất (A) |
| 35   | Mực nước (D/A)   |

## FSM – 7 trạng thái

```
INIT → MONITORING → AUTO_WATERING → COOLDOWN → MONITORING
                ↕                ↘
          MANUAL_WATERING        ALARM → MONITORING
                ↕
           OFFLINE (mất Internet)
```

| Trạng thái       | Ý nghĩa                                          |
|------------------|--------------------------------------------------|
| INIT             | Khởi tạo cảm biến, LCD, Wi-Fi, IoT              |
| MONITORING       | Theo dõi độ ẩm, chờ điều kiện tưới               |
| AUTO_WATERING    | Tưới tự động khi đất khô + bồn còn nước          |
| MANUAL_WATERING  | Tưới theo lệnh nút nhấn / IoT                    |
| COOLDOWN         | Chờ 30 s nước ngấm sau khi tưới                  |
| ALARM            | Hết nước / lỗi cảm biến / bơm timeout            |
| OFFLINE          | Mất Internet – vẫn tưới tự động tại chỗ          |

## FreeRTOS Tasks

| Task           | Chu kỳ  | Priority | Core |
|----------------|---------|----------|------|
| SensorTask     | 1 s     | 2        | 0    |
| ControlTask    | 100 ms  | 3        | 1    |
| SafetyTask     | 100 ms  | 4        | 1    |
| DisplayTask    | 500 ms  | 1        | 0    |
| IoTTask        | 2 s     | 2        | 0    |

## Synchronisation

- **Queue** – SensorData từ SensorTask → ControlTask
- **Mutex** – Bảo vệ g_sensorData, g_fsmState, g_pumpOn
- **EventGroup** – BIT_WIFI_CONNECTED, BIT_WATER_EMPTY, BIT_SENSOR_ERROR, BIT_PUMP_RUNNING

## Cài đặt Arduino IDE

1. Cài board ESP32: `https://dl.espressif.com/dl/package_esp32_index.json`
2. Cài thư viện: `LiquidCrystal_I2C`, `DHT sensor library`
3. Chọn board: **DOIT ESP32 DEVKIT V1**
4. Thay `WIFI_SSID` / `WIFI_PASSWORD` trong `config.h`
5. Upload và mở Serial Monitor 115200 baud

## IoT Platform

Hệ thống sử dụng **Blynk 2.0** (`BlynkSimpleEsp32.h`). Cấu hình Template ID, Auth Token trong `config.h`. Xem README của thư viện Blynk để biết cách tạo Template trên App. Virtual Pin mapping: V0-V5 gửi sensor/state data, V6 nhận lệnh tưới thủ công từ App.

## Chế độ hoạt động

| Chế độ | Mô tả |
|--------|-------|
| Tự động | Đất < 35% → bơm ON; đất ≥ 60% → bơm OFF; timeout 15 s |
| Thủ công | Nút nhấn hoặc IoT command → bơm ON; nút Stop hoặc timeout → bơm OFF |
| Lịch    | Cài giờ tưới (6 h / 17 h) – đất phải khô + bồn có nước |
| Alarm   | LED đỏ nhấp nháy + buzzer; nút Reset để xóa |
| Offline | Mất Wi-Fi → FSM chuyển OFFLINE, vẫn tưới tự động |
