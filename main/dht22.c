#include "dht22.h"

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "driver/gpio.h"
#include "driver/rmt_rx.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"


/* ============================================================
 * Configuration
 * ============================================================ */

#define DHT22_GPIO                  GPIO_NUM_18

#define RMT_RESOLUTION_HZ           1000000
#define RMT_RX_SYMBOLS              64

#define DHT22_START_LOW_US          1000

#define DHT22_BITS                  40
#define DHT22_BYTES                 5

/*
 * DHT22 bit encoding:
 *
 *   0 -> HIGH approximately 26-28 us
 *   1 -> HIGH approximately 70 us
 *
 * 50 us provides a safe separation point.
 */
#define DHT22_BIT_THRESHOLD_US      50


static const char *TAG = "DHT22";


/* ============================================================
 * RMT and synchronization objects
 * ============================================================ */

static rmt_channel_handle_t dht22_rmt_rx = NULL;

static SemaphoreHandle_t dht22_rx_done_sem = NULL;

/* Buffer used by RMT to store the captured waveform. */
static rmt_symbol_word_t dht22_symbols[RMT_RX_SYMBOLS];

/*
 * Pointer to the symbols captured by the RMT driver.
 * These values are valid after reception completes.
 */
static rmt_symbol_word_t *dht22_received_symbols = NULL;

static size_t dht22_num_symbols = 0;


/* ============================================================
 * RMT RX callback
 * ============================================================ */

static bool dht22_rmt_rx_done(
    rmt_channel_handle_t channel,
    const rmt_rx_done_event_data_t *edata,
    void *user_data)
{
    (void)channel;
    (void)user_data;

    /*
     * Save the captured symbols and count.
     * Decoding is performed outside the ISR.
     */
    dht22_received_symbols = edata->received_symbols;
    dht22_num_symbols = edata->num_symbols;

    BaseType_t high_task_woken = pdFALSE;

    xSemaphoreGiveFromISR(
        dht22_rx_done_sem,
        &high_task_woken
    );

    return high_task_woken == pdTRUE;
}


/* ============================================================
 * Arm RMT receiver
 * ============================================================ */

static esp_err_t dht22_arm_receiver(void)
{
    rmt_receive_config_t receive_config = {
        /*
         * Minimum pulse duration to capture: 1 us.
         */
        .signal_range_min_ns = 1000,

        /*
         * Maximum pulse duration to capture: 2 ms.
         */
        .signal_range_max_ns = 2000000,
    };

    /* Clear information from the previous reception. */
    dht22_received_symbols = NULL;
    dht22_num_symbols = 0;

    /*
     * Make sure no previous semaphore signal remains
     * before starting a new transaction.
     */
    xSemaphoreTake(
        dht22_rx_done_sem,
        0
    );

    esp_err_t ret = rmt_receive(
        dht22_rmt_rx,
        dht22_symbols,
        sizeof(dht22_symbols),
        &receive_config
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "rmt_receive failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    return ESP_OK;
}


/* ============================================================
 * Decode 40 DHT22 data bits
 * ============================================================ */

static esp_err_t dht22_decode(
    uint8_t bytes[DHT22_BYTES])
{
    if (dht22_received_symbols == NULL) {
        ESP_LOGE(
            TAG,
            "No received symbols"
        );

        return ESP_ERR_INVALID_STATE;
    }

    /*
     * RMT captures two response symbols followed by
     * 40 data-bit symbols.
     */
    if (dht22_num_symbols < 42) {
        ESP_LOGE(
            TAG,
            "Not enough symbols: %d",
            (int)dht22_num_symbols
        );

        return ESP_ERR_INVALID_SIZE;
    }

    uint8_t decoded_bytes[DHT22_BYTES] = {0};

    /*
     * DHT22 sends each byte MSB first.
     * Shift left and append each received bit.
     */
    for (int bit = 0; bit < DHT22_BITS; bit++) {

        rmt_symbol_word_t *symbol =
            &dht22_received_symbols[bit + 2];

        /*
         * The bit value is encoded in the duration
         * of the HIGH portion of the pulse.
         */
        uint32_t high_time = symbol->duration1;

        uint8_t bit_value =
            (high_time > DHT22_BIT_THRESHOLD_US)
                ? 1
                : 0;

        decoded_bytes[bit / 8] <<= 1;
        decoded_bytes[bit / 8] |= bit_value;
    }

    /* Copy decoded bytes to the caller's buffer. */
    for (int i = 0; i < DHT22_BYTES; i++) {
        bytes[i] = decoded_bytes[i];
    }

    ESP_LOGI(
        TAG,
        "Bytes: %02X %02X %02X %02X %02X",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3],
        bytes[4]
    );

    return ESP_OK;
}


/* ============================================================
 * Checksum
 * ============================================================ */

static esp_err_t dht22_check_checksum(
    const uint8_t bytes[DHT22_BYTES])
{
    /*
     * DHT22 checksum:
     *
     * humidity MSB
     * + humidity LSB
     * + temperature MSB
     * + temperature LSB
     *
     * Only the lower 8 bits are used.
     */
    uint8_t checksum =
        bytes[0] +
        bytes[1] +
        bytes[2] +
        bytes[3];

    if (checksum != bytes[4]) {
        ESP_LOGE(
            TAG,
            "Checksum failed: calculated=%02X received=%02X",
            checksum,
            bytes[4]
        );

        return ESP_ERR_INVALID_CRC;
    }

    return ESP_OK;
}


/* ============================================================
 * Convert raw DHT22 bytes into measurements
 * ============================================================ */

static void dht22_convert_data(
    const uint8_t bytes[DHT22_BYTES],
    dht22_data_t *data)
{
    /*
     * Bytes 0-1 contain relative humidity.
     *
     * DHT22 reports humidity with a resolution of
     * 0.1 percent.
     */
    uint16_t humidity_raw =
        ((uint16_t)bytes[0] << 8) |
        bytes[1];

    /*
     * Bytes 2-3 contain temperature.
     *
     * Bit 15 is the sign bit, so remove it before
     * calculating the temperature magnitude.
     */
    uint16_t temperature_raw =
        ((uint16_t)(bytes[2] & 0x7F) << 8) |
        bytes[3];

    float humidity =
        humidity_raw / 10.0f;

    float temperature =
        temperature_raw / 10.0f;

    /* Apply the sign for negative temperatures. */
    if (bytes[2] & 0x80) {
        temperature = -temperature;
    }

    data->humidity = humidity;
    data->temperature = temperature;

    ESP_LOGI(
        TAG,
        "Humidity: %.1f %%",
        humidity
    );

    ESP_LOGI(
        TAG,
        "Temperature: %.1f C",
        temperature
    );
}


/* ============================================================
 * Initialization
 * ============================================================ */

esp_err_t dht22_init(void)
{
    esp_err_t ret;

    /* Create semaphore used to signal RMT reception complete. */
    dht22_rx_done_sem = xSemaphoreCreateBinary();

    if (dht22_rx_done_sem == NULL) {
        ESP_LOGE(
            TAG,
            "Semaphore creation failed"
        );

        return ESP_ERR_NO_MEM;
    }

    /*
     * Configure the RMT receiver.
     *
     * At 1 MHz resolution, one RMT tick represents 1 us.
     */
    rmt_rx_channel_config_t rx_config = {
        .gpio_num = DHT22_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .mem_block_symbols = RMT_RX_SYMBOLS,

        .flags = {
            .invert_in = false,
            .with_dma = false,
            .io_loop_back = false,
        },
    };

    ret = rmt_new_rx_channel(
        &rx_config,
        &dht22_rmt_rx
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "RX channel creation failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    /* Register the RMT reception-complete callback. */
    rmt_rx_event_callbacks_t callbacks = {
        .on_recv_done = dht22_rmt_rx_done,
    };

    ret = rmt_rx_register_event_callbacks(
        dht22_rmt_rx,
        &callbacks,
        NULL
    );

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "RX callback registration failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    /* Enable the RMT receiver. */
    ret = rmt_enable(dht22_rmt_rx);

    if (ret != ESP_OK) {
        ESP_LOGE(
            TAG,
            "RX enable failed: %s",
            esp_err_to_name(ret)
        );

        return ret;
    }

    ESP_LOGI(
        TAG,
        "DHT22 RMT initialized"
    );

    return ESP_OK;
}


/* ============================================================
 * Read DHT22
 * ============================================================ */

esp_err_t dht22_read(
    dht22_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /*
     * Release the bus.
     *
     * The DHT22 bus normally remains HIGH through the
     * pull-up resistor.
     */
    gpio_set_direction(
        DHT22_GPIO,
        GPIO_MODE_INPUT
    );

    gpio_set_pull_mode(
        DHT22_GPIO,
        GPIO_PULLUP_ONLY
    );

    int level =
        gpio_get_level(DHT22_GPIO);

    if (level != 1) {
        ESP_LOGW(
            TAG,
            "DHT22 bus is not HIGH"
        );

        return ESP_ERR_INVALID_STATE;
    }

    /*
     * Arm RMT before generating the start signal.
     * This ensures the sensor response is captured.
     */
    esp_err_t ret =
        dht22_arm_receiver();

    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * Host start signal:
     *
     * Pull the bus LOW for at least 1 ms.
     */
    gpio_set_direction(
        DHT22_GPIO,
        GPIO_MODE_OUTPUT_OD
    );

    gpio_set_level(
        DHT22_GPIO,
        0
    );

    esp_rom_delay_us(
        DHT22_START_LOW_US
    );

    /*
     * Release the bus.
     *
     * The input configuration allows the DHT22 to
     * control the line.
     */
    gpio_set_direction(
        DHT22_GPIO,
        GPIO_MODE_INPUT
    );

    gpio_set_pull_mode(
        DHT22_GPIO,
        GPIO_PULLUP_ONLY
    );

    /*
     * Wait for RMT reception to complete.
     */
    if (xSemaphoreTake(
            dht22_rx_done_sem,
            pdMS_TO_TICKS(100)) != pdTRUE) {

        ESP_LOGE(
            TAG,
            "RMT RX timeout"
        );

        return ESP_ERR_TIMEOUT;
    }

    /*
     * Decode the 40-bit DHT22 frame.
     */
    uint8_t bytes[DHT22_BYTES];

    ret = dht22_decode(bytes);

    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * Validate the received data.
     */
    ret = dht22_check_checksum(bytes);

    if (ret != ESP_OK) {
        return ret;
    }

    /*
     * Convert raw bytes into temperature and humidity.
     */
    dht22_convert_data(
        bytes,
        data
    );

    return ESP_OK;
}

