#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

#include "esp_err.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DHT22 temperature and humidity measurement.
 */
typedef struct
{
    uint32_t id;
    float temperature;
    float humidity;
} dht22_data_t;


/**
 * @brief Initialize the DHT22 driver and RMT peripheral.
 *
 * @return
 *      - ESP_OK          Initialization successful.
 *      - ESP_ERR_NO_MEM  Memory allocation failed.
 *      - Other error     RMT initialization failed.
 */
esp_err_t dht22_init(void);


/**
 * @brief Read temperature and humidity from the DHT22 sensor.
 *
 * @param[out] data  Pointer to the structure where the measurement
 *                   will be stored.
 *
 * @return
 *      - ESP_OK                  Reading successful.
 *      - ESP_ERR_INVALID_ARG     Invalid argument.
 *      - ESP_ERR_INVALID_SIZE    Incomplete RMT frame.
 *      - ESP_ERR_INVALID_CRC     Checksum verification failed.
 *      - ESP_ERR_TIMEOUT         Sensor did not respond in time.
 *      - ESP_ERR_INVALID_STATE   DHT22 bus was not HIGH before reading.
 *      - Other error             Hardware/driver error.
 */
esp_err_t dht22_read(dht22_data_t *data);


#ifdef __cplusplus
}
#endif


#endif /* DHT22_H */

