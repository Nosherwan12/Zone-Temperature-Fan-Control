#ifndef STATUS_INDICATORS_H
#define STATUS_INDICATORS_H

#include <stdbool.h>

void status_indicators_init(void);

void status_update(bool fan_command,
                   float zone_temperature,
                   float setpoint,
                   bool high_temp_alarm,
                   bool inputs_valid);

#endif