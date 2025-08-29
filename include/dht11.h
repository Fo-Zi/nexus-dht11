/**
 * @file dht11.h
 * @brief Driver for DHT11 temperature and humidity sensor
 *
 * This driver provides functions for reading temperature and humidity
 * from the DHT11 sensor using a single-wire protocol via HAL pin interface.
 */
#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "nhal_pin.h"
#include "nhal_pin_types.h"
#include "nhal_common.h"
#include "dht11_defs.h"

typedef enum {
    DHT11_OK = 0,                       /**< Operation completed successfully */
    DHT11_ERR_INVALID_ARG,              /**< Invalid arguments provided */
    DHT11_ERR_TIMEOUT,                  /**< Communication timeout */
    DHT11_ERR_CHECKSUM,                 /**< Data checksum mismatch */
    DHT11_ERR_NO_RESPONSE,              /**< No response from sensor */
    DHT11_ERR_INVALID_DATA,             /**< Invalid data received */
    DHT11_ERR_PIN_ERROR,                /**< HAL pin operation error */
    DHT11_ERR_TOO_SOON,                 /**< Reading attempted too soon after last reading */
} dht11_result_t;

typedef struct {
    uint8_t humidity_integer;           /**< Humidity integer part (%) */
    uint8_t humidity_decimal;           /**< Humidity decimal part (%) */
    uint8_t temperature_integer;        /**< Temperature integer part (°C) */
    uint8_t temperature_decimal;        /**< Temperature decimal part (°C) */
    uint8_t checksum;                   /**< Data checksum */
} dht11_raw_data_t;

typedef struct {
    float humidity;                     /**< Humidity in percentage (0.0 - 100.0) */
    float temperature;                  /**< Temperature in Celsius */
} dht11_reading_t;

typedef struct {
    struct nhal_pin_context *pin_ctx;   /**< HAL pin context */
    uint32_t last_reading_time_ms;      /**< Timestamp of last reading (for rate limiting) */
} dht11_handle_t;

/**
 * @brief Initialize DHT11 driver
 *
 * @param handle Pointer to DHT11 handle structure
 * @param pin_ctx Initialized NHAL pin context for the data pin
 * @return dht11_result_t Result of initialization
 */
dht11_result_t dht11_init(dht11_handle_t *handle, struct nhal_pin_context *pin_ctx);

/**
 * @brief Read temperature and humidity from DHT11 sensor
 *
 * This function performs a complete DHT11 communication cycle and returns
 * the processed temperature and humidity values.
 *
 * @param handle Pointer to initialized DHT11 handle
 * @param reading Pointer to store the temperature and humidity reading
 * @return dht11_result_t Result of the reading operation
 */
dht11_result_t dht11_read(dht11_handle_t *handle, dht11_reading_t *reading);

/**
 * @brief Read raw data from DHT11 sensor
 *
 * This function performs a complete DHT11 communication cycle and returns
 * the raw data bytes without processing.
 *
 * @param handle Pointer to initialized DHT11 handle
 * @param raw_data Pointer to store the raw data
 * @return dht11_result_t Result of the reading operation
 */
dht11_result_t dht11_read_raw(dht11_handle_t *handle, dht11_raw_data_t *raw_data);

/**
 * @brief Convert raw DHT11 data to processed reading
 *
 * @param raw_data Pointer to raw data structure
 * @param reading Pointer to store processed reading
 * @return dht11_result_t Result of conversion
 */
dht11_result_t dht11_convert_raw_to_reading(const dht11_raw_data_t *raw_data, dht11_reading_t *reading);

/**
 * @brief Verify checksum of raw DHT11 data
 *
 * @param raw_data Pointer to raw data structure
 * @return true if checksum is valid, false otherwise
 */
bool dht11_verify_checksum(const dht11_raw_data_t *raw_data);

/**
 * @brief Check if enough time has passed since last reading
 *
 * DHT11 requires at least 2 seconds between readings.
 *
 * @param handle Pointer to initialized DHT11 handle
 * @return true if ready for new reading, false if too soon
 */
bool dht11_is_ready_for_reading(dht11_handle_t *handle);


#endif /* DHT11_H */