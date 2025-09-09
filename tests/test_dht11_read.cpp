#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nhal_common.h"
#include "nhal_pin_mock.hpp"
#include "nhal_common_mock.hpp"

extern "C" {
    #include "dht11.h"
}

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::StrictMock;
using ::testing::Sequence;
using ::testing::AtLeast;

class DHT11ReadTest : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&handle, 0, sizeof(handle));
        memset(&reading, 0, sizeof(reading));
        memset(&raw_data, 0, sizeof(raw_data));

        mock_time_ms = 0;
        EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](){
                mock_time_ms += 20;  // Increment by 20ms each call (realistic for DHT11 operation)
                return mock_time_ms;
            });
        mock_time_us = 0;
        EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_microseconds())
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](){
                mock_time_us += 20;  // Increment by 20ms each call (realistic for DHT11 operation)
                return mock_time_us;
            });

        // Set up default timing behaviors
        EXPECT_CALL(NhalCommonMock::instance(), nhal_delay_microseconds(testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](uint32_t microseconds){
                mock_time_us += microseconds;
            });

        EXPECT_CALL(NhalCommonMock::instance(), nhal_delay_milliseconds(testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](uint32_t milliseconds){
                mock_time_us += milliseconds * 1000;
            });

        // Use EXPECT_CALL with AnyNumber() to suppress warnings
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(testing::_, testing::_, testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](struct nhal_pin_context * ctx, nhal_pin_dir_t direction, nhal_pin_pull_mode_t pull_mode){
                return NHAL_OK;
            });

        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(testing::_, testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](struct nhal_pin_context * ctx, nhal_pin_state_t state){
                return NHAL_OK;
            });

        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(testing::_, testing::_))
            .Times(testing::AnyNumber())
            .WillRepeatedly([this](struct nhal_pin_context * ctx, nhal_pin_state_t* state){
                *state = NHAL_PIN_HIGH;  // Default to HIGH
                return NHAL_OK;
            });

        pin_ctx = (struct nhal_pin_context*)0x1000;
        dht11_init(&handle, pin_ctx);
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(&NhalPinMock::instance());
        testing::Mock::VerifyAndClearExpectations(&NhalCommonMock::instance());
    }

    // Helper to simulate a complete successful DHT11 communication
    void SetupSuccessfulDHT11Communication(uint8_t humidity_int, uint8_t humidity_dec,
                                           uint8_t temp_int, uint8_t temp_dec) {
        // Calculate expected checksum
        uint8_t expected_checksum = humidity_int + humidity_dec + temp_int + temp_dec;

        // 1. Start signal setup
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP))
            .WillOnce(Return(NHAL_OK));
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_LOW))
            .WillOnce(Return(NHAL_OK));
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_HIGH))
            .WillOnce(Return(NHAL_OK));

        // 2. Switch to input mode
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_INPUT, NHAL_PIN_PMODE_PULL_UP))
            .WillOnce(Return(NHAL_OK));

        // 3. DHT11 response sequence
        // First wait_for_pin_state(NHAL_PIN_LOW) - DHT11 pulls low for response
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
            .WillOnce([](struct nhal_pin_context* ctx, nhal_pin_state_t* state) {
                *state = NHAL_PIN_LOW;
                return NHAL_OK;
            });

        // Second wait_for_pin_state(NHAL_PIN_HIGH) - DHT11 pulls high before data
        EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
            .WillOnce([](struct nhal_pin_context* ctx, nhal_pin_state_t* state) {
                *state = NHAL_PIN_HIGH;
                return NHAL_OK;
            });

        // 4. Setup data transmission for all 5 bytes
        uint8_t data_bytes[5] = {humidity_int, humidity_dec, temp_int, temp_dec, expected_checksum};

        for (int byte_idx = 0; byte_idx < 5; byte_idx++) {
            SetupDataByteTransmission(data_bytes[byte_idx]);
        }
    }

    void SetupDataByteTransmission(uint8_t byte_value) {
        // Each byte consists of 8 bits transmitted MSB first
        for (int bit_idx = 7; bit_idx >= 0; bit_idx--) {
            bool bit_value = (byte_value >> bit_idx) & 1;

            // Each bit transmission:
            // 1. wait_for_pin_state(NHAL_PIN_LOW) - bit starts with low signal (50μs)
            EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
                .WillOnce([](struct nhal_pin_context* ctx, nhal_pin_state_t* state) {
                    *state = NHAL_PIN_LOW;
                    return NHAL_OK;
                });

            // 2. measure_pulse_duration calls:
            //    a. wait_for_pin_state(NHAL_PIN_HIGH) - wait for high pulse to start
            EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
                .WillOnce([](struct nhal_pin_context* ctx, nhal_pin_state_t* state) {
                    *state = NHAL_PIN_HIGH;
                    return NHAL_OK;
                });

            //    b. wait_for_pin_state(NHAL_PIN_LOW) - wait for high pulse to end
            //       The timing mock increments by 50μs each call, so we need to simulate
            //       the right duration for bit 0 (~26μs) vs bit 1 (~70μs)
            EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
                .WillOnce([bit_value](struct nhal_pin_context* ctx, nhal_pin_state_t* state) {
                    // With our mock timing (50μs increment), we can't perfectly simulate
                    // DHT11 timing, but the driver uses 40μs threshold to distinguish bits
                    // Since mock increments by 50μs, duration will be ~50μs, which > 40μs
                    // So all bits will be read as '1'. We need to adjust our expectations.
                    *state = NHAL_PIN_LOW;
                    return NHAL_OK;
                });
        }
    }

    dht11_handle_t handle;
    struct nhal_pin_context *pin_ctx;
    dht11_reading_t reading;
    dht11_raw_data_t raw_data;
    uint32_t mock_time_ms;
    uint64_t mock_time_us;
};

// Basic parameter validation tests
TEST_F(DHT11ReadTest, ReadWithNullHandle) {
    dht11_result_t result = dht11_read(nullptr, &reading);
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11ReadTest, ReadWithNullReading) {
    dht11_result_t result = dht11_read(&handle, nullptr);
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11ReadTest, ReadRawWithNullHandle) {
    dht11_result_t result = dht11_read_raw(nullptr, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11ReadTest, ReadRawWithNullRawData) {
    dht11_result_t result = dht11_read_raw(&handle, nullptr);
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

// Rate limiting tests using controlled timing
TEST_F(DHT11ReadTest, ReadTooSoonAfterLastReading) {
    // The limits for the rate limiting are 2000ms between readings
    // and 200ms between retries.

    // Set last reading time
    handle.last_reading_time_ms = 5000;  // Last reading was at 5000ms (timestamp)

    // Override the default mock behavior for this test only
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(6500));  // Current time 6500ms (1500ms elapsed < 2000ms)

    dht11_result_t result = dht11_read_raw(&handle, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_TOO_SOON);
}

// Pin operation error tests
TEST_F(DHT11ReadTest, ReadWithPinDirectionError) {
    // Set timing to pass rate limiting
    handle.last_reading_time_ms = 2100;  // Long ago

    // Force pin direction setting to fail
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_ERR_HW_FAILURE));

    dht11_result_t result = dht11_read_raw(&handle, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_PIN_ERROR);
}

TEST_F(DHT11ReadTest, ReadWithPinStateError) {
    // Set timing to pass rate limiting using the correct approach
    handle.last_reading_time_ms = 2100;  // Long ago

    // Override the default mock behavior for this test only
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(6500));  // Current time 6500ms (1500ms elapsed < 2000ms)

    // Allow initial direction setting to succeed
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_OK));

    // Force pin state setting to fail
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_LOW))
        .WillOnce(Return(NHAL_ERR_HW_FAILURE));

    dht11_result_t result = dht11_read_raw(&handle, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_PIN_ERROR);
}

TEST_F(DHT11ReadTest, ReadWithInputDirectionError) {
    // Set timing to pass rate limiting
    handle.last_reading_time_ms = 2100;  // Long ago

    // Setup start signal sequence to succeed
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_LOW))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_HIGH))
        .WillOnce(Return(NHAL_OK));

    // Force input direction setting to fail
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_INPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_ERR_HW_FAILURE));

    dht11_result_t result = dht11_read_raw(&handle, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_PIN_ERROR);
}

// Timeout and communication error tests
TEST_F(DHT11ReadTest, ReadWithNoResponseFromSensor) {
    // Set timing to pass rate limiting
    handle.last_reading_time_ms = 2500;  // Long ago

    // Setup start signal to succeed
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_LOW))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_HIGH))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_INPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_OK));

    // DHT11 doesn't respond - pin stays HIGH when it should go LOW
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
        .WillRepeatedly([](struct nhal_pin_context* ctx, nhal_pin_state_t* state) {
            *state = NHAL_PIN_HIGH;  // Never goes low - no response
            return NHAL_OK;
        });

    dht11_result_t result = dht11_read_raw(&handle, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_NO_RESPONSE);
}

TEST_F(DHT11ReadTest, ReadWithPinGetStateError) {
    // Set timing to pass rate limiting
    handle.last_reading_time_ms = 2100;  // Long ago

    // Setup start signal to succeed
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_OUTPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_LOW))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_state(pin_ctx, NHAL_PIN_HIGH))
        .WillOnce(Return(NHAL_OK));
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_set_direction(pin_ctx, NHAL_PIN_DIR_INPUT, NHAL_PIN_PMODE_PULL_UP))
        .WillOnce(Return(NHAL_OK));

    // Force pin_get_state to fail
    EXPECT_CALL(NhalPinMock::instance(), nhal_pin_get_state(pin_ctx, _))
        .WillOnce(Return(NHAL_ERR_HW_FAILURE));

    dht11_result_t result = dht11_read_raw(&handle, &raw_data);
    EXPECT_EQ(result, DHT11_ERR_NO_RESPONSE);
}

// Test the complete read function (not just read_raw)
TEST_F(DHT11ReadTest, CompleteReadFunction) {
    // This will call dht11_read_raw internally and then convert
    // Test with rate limiting active
    handle.last_reading_time_ms = 15;  // Recent time to trigger rate limiting

    dht11_result_t result = dht11_read(&handle, &reading);

    // Should fail with rate limiting
    EXPECT_EQ(result, DHT11_ERR_TOO_SOON);
}
