#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "dht22.h"
#include "setpoint.h"
#include "fan_control.h"
#include "status_indicators.h"

void app_main(void)
{
    dht22_data_t sensor_data;
    float setpoint = 0.0f;

    /*
     * Initialize outputs first so the actuator
     * and status indicators have defined states.
     */
    fan_control_init();
    status_indicators_init();

    /*
     * Initialize DHT22.
     */
    esp_err_t dht_result = dht22_init();

    if (dht_result != ESP_OK)
    {
        printf("DHT22 initialization failed: %s\n",
               esp_err_to_name(dht_result));

        fan_force_off();

        status_update(
            false,
            0.0f,
            setpoint,
            false,
            false
        );

        while (1)
        {
            printf("SYSTEM FAULT: DHT22 unavailable | Fan OFF\n");

            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    /*
     * Initialize setpoint ADC.
     */
    esp_err_t setpoint_result = setpoint_init();

    if (setpoint_result != ESP_OK)
    {
        printf("Setpoint ADC initialization failed: %s\n",
               esp_err_to_name(setpoint_result));

        fan_force_off();

        status_update(
            false,
            0.0f,
            setpoint,
            false,
            false
        );

        while (1)
        {
            printf("SYSTEM FAULT: Setpoint input unavailable | Fan OFF\n");

            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    /*
     * Main control loop.
     */
    while (1)
    {
        esp_err_t result = dht22_read(&sensor_data);

        if (result == ESP_OK)
        {
            esp_err_t adc_result =
                setpoint_read_temperature(&setpoint);

            if (adc_result == ESP_OK)
            {
                /*
                 * Execute fan control using valid
                 * temperature and setpoint inputs.
                 */
                fan_control_update(
                    sensor_data.temperature,
                    setpoint
                );

                /*
                 * Update status indicators.
                 * Inputs are valid in this path.
                 */
                status_update(
                    fan_is_commanded_on(),
                    sensor_data.temperature,
                    setpoint,
                    high_temperature_alarm(),
                    true
                );

                printf("Temperature: %.1f C | "
                       "Humidity: %.1f %% | "
                       "Setpoint: %.1f C | "
                       "Fan: %s | "
                       "Alarm: %s\n",

                       sensor_data.temperature,
                       sensor_data.humidity,
                       setpoint,

                       fan_is_commanded_on() ? "ON" : "OFF",

                       high_temperature_alarm() ?
                       "ACTIVE" : "NORMAL"
                );
            }
            else
            {
                /*
                 * Setpoint input is unavailable.
                 * Do not execute normal fan control.
                 */
                printf("Setpoint ADC read failed: %s | Fan forced OFF\n",
                       esp_err_to_name(adc_result));

                fan_force_off();

                status_update(
                    false,
                    sensor_data.temperature,
                    setpoint,
                    false,
                    false
                );
            }
        }
        else
        {
            /*
             * Temperature input is unavailable.
             * Do not execute normal fan control.
             */
            printf("DHT22 read failed: %s | Fan forced OFF\n",
                   esp_err_to_name(result));

            fan_force_off();

            status_update(
                false,
                0.0f,
                setpoint,
                false,
                false
            );
        }

        /*
         * DHT22 requires approximately 2 seconds
         * between readings.
         */
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}