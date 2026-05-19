#include "http_client.h"
#include "app_config.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_err.h"
#include <string.h>

static const char *TAG = "HTTP";

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

bool send_sensor_data_to_server(int adc_raw, int voltage_mv, float vout, float rs, float ratio)
{
    char json_buffer[256];
    int len = snprintf(json_buffer, sizeof(json_buffer),
        "{\"adc_raw\":%d,\"voltage\":%d,\"vout\":%.3f,\"rs\":%.3f,\"ratio\":%.3f,\"device_id\":\"%s\"}",
        adc_raw, voltage_mv, vout, rs, ratio, DEVICE_ID);

    if (len < 0 || len >= sizeof(json_buffer)) {
        ESP_LOGE(TAG, "JSON Buffer overflow");
        return false;
    }

    esp_http_client_config_t config = {
        .url = SERVER_URL,
        .event_handler = http_event_handler,
        .method = HTTP_METHOD_POST,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP Client Initialisierung fehlgeschlagen");
        return false;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_buffer, strlen(json_buffer));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP POST Status = %d", status_code);
    } else {
        ESP_LOGE(TAG, "HTTP POST Fehler: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return err == ESP_OK;
}
