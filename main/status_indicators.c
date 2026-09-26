#include "status_indicators.h"
#include "driver/gpio.h"
#include "points.h"

#define SATISFACTION_BAND_C  0.5f

void status_indicators_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask =
            (1ULL << FAN_STATUS_GPIO) |
            (1ULL << SETPOINT_STATUS_GPIO) |
            (1ULL << ALARM_STATUS_GPIO),

        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_config);

    gpio_set_level(FAN_STATUS_GPIO, 0);
    gpio_set_level(SETPOINT_STATUS_GPIO, 0);
    gpio_set_level(ALARM_STATUS_GPIO, 0);
}

void status_update(bool fan_command,
                   float zone_temperature,
                   float setpoint,
                   bool high_temp_alarm,
                   bool inputs_valid)
{
    if (!inputs_valid)
    {
        gpio_set_level(FAN_STATUS_GPIO, 0);
        gpio_set_level(SETPOINT_STATUS_GPIO, 0);
        gpio_set_level(ALARM_STATUS_GPIO, 0);

        return;
    }

    bool setpoint_satisfied =
        (zone_temperature >= setpoint - SATISFACTION_BAND_C) &&
        (zone_temperature <= setpoint + SATISFACTION_BAND_C);

    gpio_set_level(FAN_STATUS_GPIO, fan_command);
    gpio_set_level(SETPOINT_STATUS_GPIO, setpoint_satisfied);
    gpio_set_level(ALARM_STATUS_GPIO, high_temp_alarm);
}