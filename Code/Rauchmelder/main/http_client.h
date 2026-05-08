#pragma once

#include <stdbool.h>

bool send_sensor_data_to_server(int adc_raw, int voltage_mv, float vout, float rs, float ratio);
