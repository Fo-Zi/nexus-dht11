#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nhal_pin_mock.hpp"

extern "C" {
    #include "dht11.h"
}

class DHT11InitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup test fixtures
        memset(&handle, 0, sizeof(handle));
        memset(&pin_ctx, 0, sizeof(pin_ctx));
        
        // Reset mock for each test
        testing::Mock::VerifyAndClearExpectations(&NhalPinMock::instance());
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(&NhalPinMock::instance());
    }

    dht11_handle_t handle;
    struct nhal_pin_context pin_ctx;
};

TEST_F(DHT11InitTest, InitWithValidParameters) {
    dht11_result_t result = dht11_init(&handle, &pin_ctx);

    EXPECT_EQ(result, DHT11_OK);
    EXPECT_EQ(handle.pin_ctx, &pin_ctx);
    EXPECT_EQ(handle.last_reading_time_ms, 0);
}

TEST_F(DHT11InitTest, InitWithNullHandle) {
    dht11_result_t result = dht11_init(nullptr, &pin_ctx);
    
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11InitTest, InitWithNullPinContext) {
    dht11_result_t result = dht11_init(&handle, nullptr);
    
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11InitTest, InitWithBothNullParameters) {
    dht11_result_t result = dht11_init(nullptr, nullptr);
    
    EXPECT_EQ(result, DHT11_ERR_INVALID_ARG);
}

TEST_F(DHT11InitTest, MultipleInitializationsOverwritePreviousValues) {
    struct nhal_pin_context pin_ctx_2;
    memset(&pin_ctx_2, 0, sizeof(pin_ctx_2));
    
    // First init
    dht11_result_t result1 = dht11_init(&handle, &pin_ctx);
    EXPECT_EQ(result1, DHT11_OK);
    EXPECT_EQ(handle.pin_ctx, &pin_ctx);
    
    // Set some state to verify it gets reset
    handle.last_reading_time_ms = 12345;
    
    // Second init should overwrite
    dht11_result_t result2 = dht11_init(&handle, &pin_ctx_2);
    EXPECT_EQ(result2, DHT11_OK);
    EXPECT_EQ(handle.pin_ctx, &pin_ctx_2);
    EXPECT_EQ(handle.last_reading_time_ms, 0);  // Should be reset
}