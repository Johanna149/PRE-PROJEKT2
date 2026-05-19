#pragma once

#include <stdbool.h>

typedef struct {
    int adc_raw;
    int voltage_mv;
    float vout;
    float rs;
    float ratio;
} sensor_data_t;

bool sensor_init(void);
bool sensor_read(sensor_data_t *out_data);
