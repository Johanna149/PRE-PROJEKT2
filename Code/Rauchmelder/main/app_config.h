#pragma once

#define SERVER_URL "http://172.20.10.3:5297/api/sensor-data"
//#define SERVER_URL "http://localhost:5297/api/sensor-data" // nur wenn ESP auf gleichem PC läuft
#define DEVICE_ID      "ESP32-001"
#define WIFI_SSID      "iPhone vonJohanna"
#define WIFI_PASSWORD  "tudasnicht"

#define MQ2_VCC     5.0f     // Versorgungsspannung vom MQ-2 Modul
#define MQ2_RL      5.0f     // Lastwiderstand in kOhm
#define MQ2_R0      10.0f    // Referenzwiderstand in kOhm

#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_6
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12
