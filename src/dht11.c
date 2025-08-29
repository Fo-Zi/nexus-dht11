/**
 * @file dht11.c
 * @brief Implementation of DHT11 temperature and humidity sensor driver
 */

#include "dht11.h"
#include <string.h>


static bool wait_for_pin_state(struct nhal_pin_context *pin_ctx, nhal_pin_state_t expected_state, uint32_t timeout_us)
{
    uint32_t elapsed_us = 0;
    nhal_pin_state_t current_state;


    while (elapsed_us < timeout_us) {
        nhal_result_t result = nhal_pin_get_state(pin_ctx, &current_state);
        if (result != NHAL_OK) {
            return false;
        }

        if (current_state == expected_state) {
            return true;
        }

        nhal_delay_microseconds(1);
        elapsed_us++;
    }

    return false;
}


static uint32_t measure_pulse_duration(struct nhal_pin_context *pin_ctx, nhal_pin_state_t pulse_state, uint32_t timeout_us)
{
    uint32_t start_time, end_time;


    // Wait for pulse to start
    if (!wait_for_pin_state(pin_ctx, pulse_state, timeout_us)) {
        return 0;
    }

    start_time = nhal_get_timestamp_microseconds();

    // Wait for pulse to end
    nhal_pin_state_t opposite_state = (pulse_state == NHAL_PIN_HIGH) ? NHAL_PIN_LOW : NHAL_PIN_HIGH;
    if (!wait_for_pin_state(pin_ctx, opposite_state, timeout_us)) {
        return 0;
    }

    end_time = nhal_get_timestamp_microseconds();
    uint32_t duration = (uint32_t)(end_time - start_time);
    return duration;
}


dht11_result_t dht11_init(dht11_handle_t *handle, struct nhal_pin_context *pin_ctx)
{
    if (handle == NULL || pin_ctx == NULL) {
        return DHT11_ERR_INVALID_ARG;
    }

    handle->pin_ctx = pin_ctx;
    handle->last_reading_time_ms = 0;

    // Initialize pin as output with pull-up, set to HIGH
    nhal_result_t pin_result = nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP);
    if (pin_result != NHAL_OK) {
        return DHT11_ERR_PIN_ERROR;
    }

    pin_result = nhal_pin_set_state(pin_ctx, NHAL_PIN_HIGH);
    if (pin_result != NHAL_OK) {
        return DHT11_ERR_PIN_ERROR;
    }

    return DHT11_OK;
}

bool dht11_is_ready_for_reading(dht11_handle_t *handle)
{
    if (handle == NULL) {
        return false;
    }

    uint32_t current_time = nhal_get_timestamp_milliseconds();
    uint32_t time_since_last = current_time - handle->last_reading_time_ms;

    return (time_since_last >= DHT11_MIN_SAMPLING_PERIOD_MS);
}

bool dht11_verify_checksum(const dht11_raw_data_t *raw_data)
{
    if (raw_data == NULL) {
        return false;
    }

    uint8_t calculated_checksum = raw_data->humidity_integer +
                                  raw_data->humidity_decimal +
                                  raw_data->temperature_integer +
                                  raw_data->temperature_decimal;

    return (calculated_checksum == raw_data->checksum);
}

dht11_result_t dht11_convert_raw_to_reading(const dht11_raw_data_t *raw_data, dht11_reading_t *reading)
{
    if (raw_data == NULL || reading == NULL) {
        return DHT11_ERR_INVALID_ARG;
    }

    if (!dht11_verify_checksum(raw_data)) {
        return DHT11_ERR_CHECKSUM;
    }

    // DHT11 provides integer values only (decimal parts are always 0)
    reading->humidity = (float)raw_data->humidity_integer + (float)raw_data->humidity_decimal / 10.0f;
    reading->temperature = (float)raw_data->temperature_integer + (float)raw_data->temperature_decimal / 10.0f;

    // Validate ranges
    if (reading->humidity < DHT11_HUMIDITY_MIN || reading->humidity > DHT11_HUMIDITY_MAX) {
        return DHT11_ERR_INVALID_DATA;
    }

    if (reading->temperature < DHT11_TEMPERATURE_MIN || reading->temperature > DHT11_TEMPERATURE_MAX) {
        return DHT11_ERR_INVALID_DATA;
    }

    return DHT11_OK;
}

dht11_result_t dht11_read_raw(dht11_handle_t *handle, dht11_raw_data_t *raw_data)
{
    if (handle == NULL || raw_data == NULL) {
        return DHT11_ERR_INVALID_ARG;
    }

    if (!dht11_is_ready_for_reading(handle)) {
        return DHT11_ERR_TOO_SOON;
    }
    uint8_t data_bytes[DHT11_DATA_BYTES] = {0};

    // Step 1: Send start signal
    nhal_result_t pin_result = nhal_pin_set_direction(handle->pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP);
    if (pin_result != NHAL_OK) {
        return DHT11_ERR_PIN_ERROR;
    }

    // Pull low for 18ms
    nhal_pin_set_state(handle->pin_ctx, NHAL_PIN_LOW);
    nhal_delay_milliseconds(DHT11_START_SIGNAL_MS);

    // Pull high for 20-40us
    nhal_pin_set_state(handle->pin_ctx, NHAL_PIN_HIGH);
    nhal_delay_microseconds(DHT11_START_SIGNAL_HIGH_US);

    // Step 2: Switch to input mode and wait for DHT11 response
    pin_result = nhal_pin_set_direction(handle->pin_ctx, NHAL_PIN_DIR_INPUT, NHAL_PIN_PMODE_PULL_UP);
    if (pin_result != NHAL_OK) {
        return DHT11_ERR_PIN_ERROR;
    }

    // Wait for DHT11 to pull low (response signal)
    if (!wait_for_pin_state(handle->pin_ctx, NHAL_PIN_LOW, DHT11_TIMEOUT_US)) {
        return DHT11_ERR_NO_RESPONSE;
    }

    // Wait for DHT11 to pull high (preparation for data transmission)
    if (!wait_for_pin_state(handle->pin_ctx, NHAL_PIN_HIGH, DHT11_TIMEOUT_US)) {
        return DHT11_ERR_NO_RESPONSE;
    }

    // Step 3: Read 40 bits of data
    for (int byte_idx = 0; byte_idx < DHT11_DATA_BYTES; byte_idx++) {
        for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
            // Wait for bit transmission to start (low signal)
            if (!wait_for_pin_state(handle->pin_ctx, NHAL_PIN_LOW, DHT11_TIMEOUT_US)) {
                return DHT11_ERR_TIMEOUT;
            }

            // Measure the high pulse duration to determine bit value
            uint32_t high_duration = measure_pulse_duration(handle->pin_ctx, NHAL_PIN_HIGH, DHT11_TIMEOUT_US);
            if (high_duration == 0) {
                return DHT11_ERR_TIMEOUT;
            }

            // Bit decision: >threshold = '1', <threshold = '0'
            if (high_duration > DHT11_PULSE_THRESHOLD_US) {
                data_bytes[byte_idx] |= (1 << bit_idx);
            }
        }
    }

    // Step 4: Parse received data
    raw_data->humidity_integer = data_bytes[0];
    raw_data->humidity_decimal = data_bytes[1];
    raw_data->temperature_integer = data_bytes[2];
    raw_data->temperature_decimal = data_bytes[3];
    raw_data->checksum = data_bytes[4];

    // Update last reading time
    handle->last_reading_time_ms = nhal_get_timestamp_milliseconds();

    // Verify checksum
    if (!dht11_verify_checksum(raw_data)) {
        return DHT11_ERR_CHECKSUM;
    }

    return DHT11_OK;
}

dht11_result_t dht11_read(dht11_handle_t *handle, dht11_reading_t *reading)
{
    if (handle == NULL || reading == NULL) {
        return DHT11_ERR_INVALID_ARG;
    }

    dht11_raw_data_t raw_data;
    dht11_result_t result = dht11_read_raw(handle, &raw_data);
    if (result != DHT11_OK) {
        return result;
    }

    return dht11_convert_raw_to_reading(&raw_data, reading);
}
