/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

const static char *TAG = "MQ2";

/*---------------------------------------------------------------
        Server Konfiguration
---------------------------------------------------------------*/
#define SERVER_URL     "http://10.0.29.177:5000/api/sensor-data"
#define DEVICE_ID      "ESP32-001"
#define WIFI_SSID      "iPhone von Johanna"
#define WIFI_PASSWORD  "tudasnicht"

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
#if CONFIG_IDF_TARGET_ESP32
#define EXAMPLE_ADC1_CHAN0          ADC_CHANNEL_6
#endif

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

/*---------------------------------------------------------------
        MQ-2 Einstellungen
---------------------------------------------------------------*/
#define MQ2_VCC     5.0f     // Versorgungsspannung vom MQ-2 Modul
#define MQ2_RL      5.0f     // Lastwiderstand in kOhm, oft 5k oder 10k
#define MQ2_R0      10.0f    // Platzhalter, später kalibrieren!

static int adc_raw[10];
static int voltage[10];

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

/*---------------------------------------------------------------
        HTTP Client Handler
---------------------------------------------------------------*/
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch (evt->event_id) {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, Key=%s, Value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%zu", evt->data_len);
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void send_sensor_data_to_server(int adc_raw, int voltage, float vout, float rs, float ratio)
{
    char json_buffer[256];
    
    // JSON-Daten erstellen
    snprintf(json_buffer, sizeof(json_buffer),
        "{\"adc_raw\":%d,\"voltage\":%d,\"vout\":%.3f,\"rs\":%.3f,\"ratio\":%.3f,\"device_id\":\"%s\"}",
        adc_raw, voltage, vout, rs, ratio, DEVICE_ID);

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP Client Initialisierung fehlgeschlagen");
        return;
    }

    // Header setzen
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_buffer, strlen(json_buffer));

    // Anfrage senden
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d", status_code);
    } else {
        ESP_LOGE(TAG, "HTTP POST Fehler: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

/*---------------------------------------------------------------
        WiFi Initialisierung
---------------------------------------------------------------*/
static void wifi_init_sta(void)
{
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    strncpy((char *)wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, WIFI_PASSWORD, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta verbinde mit SSID: %s", WIFI_SSID);
}

void app_main(void)
{
    //-------------WiFi Init---------------//
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_sta();

    //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(
        ADC_UNIT_1,
        EXAMPLE_ADC1_CHAN0,
        EXAMPLE_ADC_ATTEN,
        &adc1_cali_chan0_handle
    );

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &adc_raw[0]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d",
                 ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, adc_raw[0]);

        if (do_calibration1_chan0) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(
                adc1_cali_chan0_handle,
                adc_raw[0],
                &voltage[0]
            ));

            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV",
                     ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN0, voltage[0]);

            //-------------MQ-2 Berechnung---------------//
            float vout = voltage[0] / 1000.0f;   // mV -> V

            if (vout > 0.01f) {
                float rs = MQ2_RL * (MQ2_VCC - vout) / vout;
                float ratio = rs / MQ2_R0;

                ESP_LOGI(TAG, "MQ2 Vout: %.3f V", vout);
                ESP_LOGI(TAG, "MQ2 Rs: %.3f kOhm", rs);
                ESP_LOGI(TAG, "MQ2 Rs/R0: %.3f", ratio);

                // Daten an Server senden
                send_sensor_data_to_server(adc_raw[0], voltage[0], vout, rs, ratio);
            } else {
                ESP_LOGW(TAG, "MQ2 Vout zu klein, Berechnung übersprungen");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    *out_handle = handle;

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}