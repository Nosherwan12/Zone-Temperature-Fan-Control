#include "fan_control.h"
#include "driver/gpio.h"
#include "points.h"

#define HYSTERESIS_C       0.5f
#define ALARM_DEVIATION_C  5.0f

static bool fan_command = false;
static bool high_temp_alarm = false;

void fan_control_init(void)
{
    gpio_config_t io_config = {
        .pin_bit_mask = (1ULL << FAN_RELAY_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_config);

    gpio_set_level(FAN_RELAY_GPIO, 1);
}

void fan_control_update(float zone_temperature,
                        float setpoint)
{
    float on_threshold = setpoint + HYSTERESIS_C;
    float off_threshold = setpoint - HYSTERESIS_C;

    if (zone_temperature > on_threshold)
    {
        fan_command = true;
    }
    else if (zone_temperature < off_threshold)
    {
        fan_command = false;
    }

    if (zone_temperature > (setpoint + ALARM_DEVIATION_C))
    {
        high_temp_alarm = true;
    }
    else
    {
        high_temp_alarm = false;
    }

    gpio_set_level(FAN_RELAY_GPIO, !fan_command);
}

bool fan_is_commanded_on(void)
{
    return fan_command;
}

bool high_temperature_alarm(void)
{
    return high_temp_alarm;
}

void fan_force_off(void)
{
    fan_command = false;
    high_temp_alarm = false;

    gpio_set_level(FAN_RELAY_GPIO, 1);
}