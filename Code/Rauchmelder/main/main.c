#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi.h"
#include "sensor.h"
#include "http_client.h"

static const char *TAG = "RAUCHMELDER";

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    if (!wifi_init_sta()) {
        ESP_LOGE(TAG, "WLAN konnte nicht verbunden werden");
        return;
    }

    if (!sensor_init()) {
        ESP_LOGE(TAG, "Sensor-Initialisierung fehlgeschlagen");
        return;
    }

    while (1) {
        sensor_data_t data = {0};
        if (!sensor_read(&data)) {
            ESP_LOGW(TAG, "Sensorlesen fehlgeschlagen oder Messwert zu klein");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        ESP_LOGI(TAG, "ADC Rohdaten: %d, Spannung: %d mV, Vout: %.3f V, Rs: %.3f kOhm, Ratio: %.3f",
                 data.adc_raw, data.voltage_mv, data.vout, data.rs, data.ratio);

        if (!send_sensor_data_to_server(data.adc_raw, data.voltage_mv, data.vout, data.rs, data.ratio)) {
            ESP_LOGW(TAG, "Senden der Sensordaten fehlgeschlagen");
        }

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
