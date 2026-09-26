#ifndef POINTS_H
#define POINTS_H

#include "driver/gpio.h"
#include "driver/adc.h"

/* Sensors */
#define DHT22_GPIO              GPIO_NUM_18

/* Setpoint potentiometer */
#define SETPOINT_ADC_CHANNEL    ADC_CHANNEL_0

/* Outputs */
#define FAN_RELAY_GPIO          GPIO_NUM_4
#define FAN_STATUS_GPIO         GPIO_NUM_5
#define SETPOINT_STATUS_GPIO    GPIO_NUM_6
#define ALARM_STATUS_GPIO       GPIO_NUM_7

#endif