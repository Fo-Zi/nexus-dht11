#include <cstdint>
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

class DHT11UtilsTest : public ::testing::Test {
protected:
    void SetUp() override {
        memset(&handle, 0, sizeof(handle));
        memset(&raw_data, 0, sizeof(raw_data));
        memset(&reading, 0, sizeof(reading));

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

        // Initialize handle for timing tests
        pin_ctx = (struct nhal_pin_context*)0x1000;
        dht11_init(&handle, pin_ctx);
    }

    dht11_handle_t handle;
    struct nhal_pin_context *pin_ctx;
    dht11_raw_data_t raw_data;
    dht11_reading_t reading;
    uint32_t mock_time_ms;
    uint64_t mock_time_us;
};

// Test dht11_verify_checksum function
TEST_F(DHT11UtilsTest, VerifyChecksumWithValidData) {
    // Valid data: humidity=55%, temp=25°C, checksum should be 55+0+25+0=80
    raw_data.humidity_integer = 55;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 25;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 80;  // 55 + 0 + 25 + 0 = 80

    bool result = dht11_verify_checksum(&raw_data);

    EXPECT_TRUE(result);
}

TEST_F(DHT11UtilsTest, VerifyChecksumWithInvalidData) {
    // Invalid checksum
    raw_data.humidity_integer = 55;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 25;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 99;  // Should be 80, not 99

    bool result = dht11_verify_checksum(&raw_data);

    EXPECT_FALSE(result);
}

TEST_F(DHT11UtilsTest, VerifyChecksumWithNullPointer) {
    bool result = dht11_verify_checksum(nullptr);

    EXPECT_FALSE(result);
}

TEST_F(DHT11UtilsTest, VerifyChecksumWithComplexData) {
    // More complex data with decimal values
    raw_data.humidity_integer = 60;
    raw_data.humidity_decimal = 5;
    raw_data.temperature_integer = 23;
    raw_data.temperature_decimal = 8;
    raw_data.checksum = 96;  // 60 + 5 + 23 + 8 = 96

    bool result = dht11_verify_checksum(&raw_data);

    EXPECT_TRUE(result);
}

TEST_F(DHT11UtilsTest, VerifyChecksumWithOverflowData) {
    // Test edge case with values that could cause overflow
    raw_data.humidity_integer = 255;
    raw_data.humidity_decimal = 255;
    raw_data.temperature_integer = 255;
    raw_data.temperature_decimal = 255;
    raw_data.checksum = (255 + 255 + 255 + 255) & 0xFF;  // Handle overflow

    bool result = dht11_verify_checksum(&raw_data);

    EXPECT_TRUE(result);
}

// Test dht11_convert_raw_to_reading function
TEST_F(DHT11UtilsTest, ConvertRawToReadingWithValidData) {
    raw_data.humidity_integer = 55;
    raw_data.humidity_decimal = 3;
    raw_data.temperature_integer = 25;
    raw_data.temperature_decimal = 7;
    raw_data.checksum = 55 + 3 + 25 + 7; // 90

    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);

    EXPECT_EQ(result, DHT11_OK);
    EXPECT_FLOAT_EQ(reading.humidity, 55.3f);
    EXPECT_FLOAT_EQ(reading.temperature, 25.7f);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithZeroValues) {
    raw_data.humidity_integer = 0;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 0;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 0 + 0 + 0 + 0; // 0

    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);

    EXPECT_EQ(result, DHT11_OK);
    EXPECT_FLOAT_EQ(reading.humidity, 0.0f);
    EXPECT_FLOAT_EQ(reading.temperature, 0.0f);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithMaxValues) {
    // DHT11 max ranges: humidity 0-80%, temperature 0-50°C
    raw_data.humidity_integer = 80;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 50;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 80 + 0 + 50 + 0; // 130

    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);

    EXPECT_EQ(result, DHT11_OK);
    EXPECT_FLOAT_EQ(reading.humidity, 80.0f);
    EXPECT_FLOAT_EQ(reading.temperature, 50.0f);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithNullRawData) {
    dht11_result_t result = dht11_convert_raw_to_reading(nullptr, &reading);

    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithNullReading) {
    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, nullptr);

    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithInvalidHumidity) {
    // DHT11 humidity should not exceed 100%
    raw_data.humidity_integer = 101;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 25;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 101 + 0 + 25 + 0; // 126

    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);

    EXPECT_EQ(result, DHT11_ERR_INVALID_DATA);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithInvalidTemperature) {
    // DHT11 temperature should not exceed reasonable limits
    raw_data.humidity_integer = 50;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 100;  // Too high for DHT11
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 50 + 0 + 100 + 0; // 150

    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);

    EXPECT_EQ(result, DHT11_ERR_INVALID_DATA);
}

// Test dht11_is_ready_for_reading function with timing mock understanding
TEST_F(DHT11UtilsTest, IsReadyForReadingWithNullHandle) {
    bool result = dht11_is_ready_for_reading(nullptr);

    EXPECT_FALSE(result);
}

TEST_F(DHT11UtilsTest, IsReadyForReadingImmediatelyAfterInit) {

    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(5)); // Time difference:  > 2000ms )
    bool result = dht11_is_ready_for_reading(&handle);

    EXPECT_FALSE(result);
}

TEST_F(DHT11UtilsTest, IsReadyForReadingTooSoon) {
    handle.last_reading_time_ms = 1000;
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(1100)); // Time difference:  100ms < 2000ms )

    bool result = dht11_is_ready_for_reading(&handle);

    // Should return false since not enough time has passed
    EXPECT_FALSE(result);
}

TEST_F(DHT11UtilsTest, IsReadyForReadingAfterEnoughTime) {
    // Set last reading time to 0 to ensure maximum elapsed time
    // This tests the edge case where elapsed time might be large
    handle.last_reading_time_ms = 0;
    uint16_t emulated_timestamp = 1100;

    // Make multiple calls to advance mock time significantly
    for (int i = 0; i < 21; i++) {
        emulated_timestamp += 100; // Adds +100ms per iteration (2100ms total at the end)
        EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
            .WillOnce(Return(emulated_timestamp));
        dht11_is_ready_for_reading(&handle);
    }

    // Now test - depending on how much time has "elapsed" in our mock
    emulated_timestamp += 100;
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(emulated_timestamp));
    bool result = dht11_is_ready_for_reading(&handle);

    EXPECT_TRUE(result);
}

TEST_F(DHT11UtilsTest, IsReadyForReadingDifferentDriverInstances) {
    // Test the timing logic more directly by controlling the values

    // Test case 1: last_reading_time_ms is much larger than current time (wraparound case)
    handle.last_reading_time_ms = UINT32_MAX - 100;  // Very large value
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(2000)); // Time difference: (2000 - (UINT32_MAX - 100) > 2000ms )
    bool result1 = dht11_is_ready_for_reading(&handle);

    EXPECT_TRUE(result1);

    // Test case 2: Normal case with reasonable values
    handle.last_reading_time_ms = 5;  // Small value
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(500)); // Time difference: (500 - 5 < 2000ms )
    bool result2 = dht11_is_ready_for_reading(&handle);

    EXPECT_FALSE(result2);
}

// Integration test combining multiple functions
TEST_F(DHT11UtilsTest, EndToEndDataProcessingValid) {
    // Valid raw data
    raw_data.humidity_integer = 45;
    raw_data.humidity_decimal = 5;
    raw_data.temperature_integer = 22;
    raw_data.temperature_decimal = 3;
    raw_data.checksum = 75;  // 45 + 5 + 22 + 3 = 75

    // Verify checksum
    bool checksum_valid = dht11_verify_checksum(&raw_data);
    EXPECT_TRUE(checksum_valid);

    // Convert to reading
    dht11_result_t convert_result = dht11_convert_raw_to_reading(&raw_data, &reading);
    EXPECT_EQ(convert_result, DHT11_OK);
    EXPECT_FLOAT_EQ(reading.humidity, 45.5f);
    EXPECT_FLOAT_EQ(reading.temperature, 22.3f);
}

TEST_F(DHT11UtilsTest, EndToEndDataProcessingInvalidChecksum) {
    // Invalid checksum
    raw_data.humidity_integer = 45;
    raw_data.humidity_decimal = 5;
    raw_data.temperature_integer = 22;
    raw_data.temperature_decimal = 3;
    raw_data.checksum = 99;  // Wrong checksum

    // Verify checksum should fail
    bool checksum_valid = dht11_verify_checksum(&raw_data);
    EXPECT_FALSE(checksum_valid);

    // Conversion should fail because checksum validation is done first
    dht11_result_t convert_result = dht11_convert_raw_to_reading(&raw_data, &reading);
    EXPECT_EQ(convert_result, DHT11_ERR_CHECKSUM);
}

// Additional tests to improve coverage of edge cases and utility functions
TEST_F(DHT11UtilsTest, IsReadyForReadingWithTimestampWraparound) {
    // Test edge case where current timestamp is smaller than last reading time
    // This happens when timestamp wraps around (overflow)
    handle.last_reading_time_ms = UINT32_MAX - 1000;  // Very large value
    
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(1500)); // Small value after wraparound
    
    bool result = dht11_is_ready_for_reading(&handle);
    
    // Time difference with unsigned wraparound:
    // 1500 - (UINT32_MAX - 1000) = 1500 - 4294966295 
    // In unsigned arithmetic: 1500 + 1000 + 1 = 2501ms > 2000ms
    // Should be ready for reading
    EXPECT_TRUE(result);
}

TEST_F(DHT11UtilsTest, IsReadyForReadingExactMinimumWaitTime) {
    // Test exact boundary condition
    handle.last_reading_time_ms = 1000;
    
    EXPECT_CALL(NhalCommonMock::instance(), nhal_get_timestamp_milliseconds())
        .WillOnce(Return(3000)); // Exactly 2000ms difference
    
    bool result = dht11_is_ready_for_reading(&handle);
    
    // Should be ready (>= 2000ms)
    EXPECT_TRUE(result);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithValidBoundaryHumidity) {
    // Test DHT11 valid humidity boundary (100% should be valid)
    raw_data.humidity_integer = 100;  
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 25;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 125;
    
    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);
    
    EXPECT_EQ(result, DHT11_OK);  // 100% humidity should be valid
    EXPECT_FLOAT_EQ(reading.humidity, 100.0f);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithInvalidHighHumidity) {
    // Test DHT11 invalid high humidity (>100%)
    raw_data.humidity_integer = 110;  // Invalid - exceeds 100%
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 25;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 135;
    
    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);
    
    EXPECT_EQ(result, DHT11_ERR_INVALID_DATA);  // Should exceed valid range
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithValidBoundaryTemperature) {
    // Test DHT11 valid temperature boundary (80°C should be valid)
    raw_data.humidity_integer = 50;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 80;  // Max valid temperature
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 130;
    
    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);
    
    EXPECT_EQ(result, DHT11_OK);  // 80°C should be valid
    EXPECT_FLOAT_EQ(reading.temperature, 80.0f);
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithInvalidHighTemperature) {
    // Test DHT11 invalid high temperature (>80°C)
    raw_data.humidity_integer = 50;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 90;  // Invalid - exceeds 80°C
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 140;
    
    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);
    
    EXPECT_EQ(result, DHT11_ERR_INVALID_DATA);  // Should exceed valid range
}

TEST_F(DHT11UtilsTest, ConvertRawToReadingWithNegativeTemperature) {
    // Test negative temperature (should be invalid for DHT11)
    raw_data.humidity_integer = 50;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 255;  // Interpreted as negative
    raw_data.temperature_decimal = 0;
    raw_data.checksum = (50 + 0 + 255 + 0) & 0xFF;  // Wrap checksum
    
    dht11_result_t result = dht11_convert_raw_to_reading(&raw_data, &reading);
    
    EXPECT_EQ(result, DHT11_ERR_INVALID_DATA);
}

TEST_F(DHT11UtilsTest, VerifyChecksumWithAllZeros) {
    // Edge case with all zero data
    raw_data.humidity_integer = 0;
    raw_data.humidity_decimal = 0;
    raw_data.temperature_integer = 0;
    raw_data.temperature_decimal = 0;
    raw_data.checksum = 0;
    
    bool result = dht11_verify_checksum(&raw_data);
    
    EXPECT_TRUE(result);
}

TEST_F(DHT11UtilsTest, VerifyChecksumWithMaxValues) {
    // Edge case with maximum possible values
    raw_data.humidity_integer = 255;
    raw_data.humidity_decimal = 255;
    raw_data.temperature_integer = 255;
    raw_data.temperature_decimal = 255;
    raw_data.checksum = (255 + 255 + 255 + 255) & 0xFF;  // Should wrap to 252
    
    bool result = dht11_verify_checksum(&raw_data);
    
    EXPECT_TRUE(result);
}
