#ifndef FAN_CONTROL_H
#define FAN_CONTROL_H

#include <stdbool.h>

void fan_control_init(void);

void fan_control_update(float zone_temperature,
                        float setpoint);

bool fan_is_commanded_on(void);

bool high_temperature_alarm(void);

void fan_force_off(void);

#endif