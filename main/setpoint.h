#ifndef SETPOINT_H
#define SETPOINT_H

#include "esp_err.h"

esp_err_t setpoint_init(void);
esp_err_t setpoint_read_temperature(float *setpoint);

#endif