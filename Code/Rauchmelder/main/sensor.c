#include "sensor.h"
#include "app_config.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "SENSOR";
static adc_oneshot_unit_handle_t adc1_handle = NULL;
static adc_cali_handle_t adc1_cali_handle = NULL;
static bool adc_calibrated = false;

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel,
                                       adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
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
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
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
        ESP_LOGI(TAG, "ADC Kalibrierung erfolgreich");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse nicht gebrannt oder Kalibrierung nicht unterstützt");
    } else {
        ESP_LOGE(TAG, "ADC Kalibrierung fehlgeschlagen: %s", esp_err_to_name(ret));
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Curve Fitting Kalibrierung löschen");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "Line Fitting Kalibrierung löschen");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}

bool sensor_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    if (adc_oneshot_new_unit(&init_config, &adc1_handle) != ESP_OK) {
        ESP_LOGE(TAG, "ADC Einheit konnte nicht initialisiert werden");
        return false;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = EXAMPLE_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &chan_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "ADC Kanal konnte nicht konfiguriert werden");
        return false;
    }

    adc_calibrated = example_adc_calibration_init(
        ADC_UNIT_1,
        EXAMPLE_ADC1_CHAN0,
        EXAMPLE_ADC_ATTEN,
        &adc1_cali_handle
    );
    return true;
}

bool sensor_read(sensor_data_t *out_data)
{
    if (out_data == NULL || adc1_handle == NULL) {
        return false;
    }

    if (adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &out_data->adc_raw) != ESP_OK) {
        ESP_LOGE(TAG, "ADC Lesen fehlgeschlagen");
        return false;
    }

    if (adc_calibrated) {
        if (adc_cali_raw_to_voltage(adc1_cali_handle, out_data->adc_raw, &out_data->voltage_mv) != ESP_OK) {
            ESP_LOGW(TAG, "ADC Kalibrierung fehlgeschlagen, verwende Rohdaten");
            out_data->voltage_mv = 0;
        }
    } else {
        out_data->voltage_mv = 0;
    }

    out_data->vout = out_data->voltage_mv / 1000.0f;
    if (out_data->vout > 0.01f) {
        out_data->rs = MQ2_RL * (MQ2_VCC - out_data->vout) / out_data->vout;
        out_data->ratio = out_data->rs / MQ2_R0;
    } else {
        out_data->rs = 0;
        out_data->ratio = 0;
    }

    return (out_data->vout > 0.01f);
}
