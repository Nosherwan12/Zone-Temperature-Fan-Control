#include "setpoint.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"

#define SETPOINT_MIN_C       15.0f
#define SETPOINT_MAX_C       30.0f

#define SETPOINT_ADC_UNIT    ADC_UNIT_1
#define SETPOINT_ADC_CHANNEL ADC_CHANNEL_0

static adc_oneshot_unit_handle_t adc_handle;

esp_err_t setpoint_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = SETPOINT_ADC_UNIT,
    };

    esp_err_t err = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (err != ESP_OK)
    {
        return err;
    }
    adc_oneshot_chan_cfg_t channel_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    err = adc_oneshot_config_channel(
        adc_handle,
        SETPOINT_ADC_CHANNEL,
        &channel_config
    );
    if (err != ESP_OK)
    {
        return err;
    }

    return ESP_OK;
}

esp_err_t setpoint_read_temperature(float *setpoint)
{
    if (setpoint == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int adc_raw = 0;

    esp_err_t err = adc_oneshot_read(
        adc_handle,
        SETPOINT_ADC_CHANNEL,
        &adc_raw
    );

    if (err != ESP_OK)
    {
        return err;
    }

    float normalized = (float)adc_raw / 4095.0f;

    *setpoint = SETPOINT_MIN_C +
                normalized * (SETPOINT_MAX_C - SETPOINT_MIN_C);

    return ESP_OK;
}