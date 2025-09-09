/**
 * @file dht11_defs.h
 * @brief DHT11 sensor protocol definitions and timing constants
 *
 * This file contains the protocol-specific timing constants and definitions
 * for the DHT11 temperature and humidity sensor. These values are based on
 * the DHT11 datasheet and define the communication protocol timing requirements.
 */
#ifndef DHT11_DEFS_H
#define DHT11_DEFS_H

/* DHT11 Communication Protocol Timing Definitions */

#define DHT11_START_SIGNAL_MS           18      /**< Start signal duration in milliseconds */
#define DHT11_START_SIGNAL_HIGH_US      40      /**< Start signal high duration in microseconds */
#define DHT11_RESPONSE_LOW_US           80      /**< DHT11 response low duration in microseconds */
#define DHT11_RESPONSE_HIGH_US          80      /**< DHT11 response high duration in microseconds */
#define DHT11_BIT_LOW_US                50      /**< Bit transmission low duration in microseconds */
#define DHT11_BIT_0_HIGH_US             26      /**< Bit '0' high duration in microseconds */
#define DHT11_BIT_1_HIGH_US             70      /**< Bit '1' high duration in microseconds */
#define DHT11_TIMEOUT_US                1000    /**< General timeout in microseconds */
#define DHT11_RESPONSE_TIMEOUT_US       200     /**< Timeout for DHT11 response signals in microseconds */
#define DHT11_BIT_TIMEOUT_US            200     /**< Timeout for bit transmission in microseconds */
#define DHT11_DATA_BITS                 40      /**< Total number of data bits */
#define DHT11_MIN_SAMPLING_PERIOD_MS    2000    /**< Minimum time between readings in milliseconds */

/* DHT11 Protocol Constants */

#define DHT11_PULSE_THRESHOLD_US        40      /**< Threshold for distinguishing '0' from '1' bits */
#define DHT11_DATA_BYTES                5       /**< Number of data bytes (humidity_int, humidity_dec, temp_int, temp_dec, checksum) */

/* DHT11 Data Validation Constants */

#define DHT11_HUMIDITY_MIN              0.0f    /**< Minimum valid humidity percentage */
#define DHT11_HUMIDITY_MAX              100.0f  /**< Maximum valid humidity percentage */
#define DHT11_TEMPERATURE_MIN           -40.0f  /**< Minimum valid temperature in Celsius */
#define DHT11_TEMPERATURE_MAX           80.0f   /**< Maximum valid temperature in Celsius */

#endif /* DHT11_DEFS_H */