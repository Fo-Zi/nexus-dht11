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
        pin_ctx = (struct nhal_pin_context*)0x1000;  // Dummy pointer for testing

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

        // // Reset mock for each test
        // testing::Mock::VerifyAndClearExpectations(&NhalPinMock::instance());
    }

    void TearDown() override {
        testing::Mock::VerifyAndClearExpectations(&NhalPinMock::instance());
    }

    dht11_handle_t handle;
    struct nhal_pin_context *pin_ctx;  // Just a pointer since it's opaque
};

TEST_F(DHT11InitTest, InitWithValidParameters) {
    dht11_result_t result = dht11_init(&handle, pin_ctx);

    EXPECT_EQ(result, DHT11_OK);
    EXPECT_EQ(handle.pin_ctx, pin_ctx);
    EXPECT_EQ(handle.last_reading_time_ms, 0);
}

TEST_F(DHT11InitTest, InitWithNullHandle) {
    dht11_result_t result = dht11_init(nullptr, pin_ctx);

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
    struct nhal_pin_context *pin_ctx_2 = (struct nhal_pin_context*)0x2000;  // Different dummy pointer

    // First init
    dht11_result_t result1 = dht11_init(&handle, pin_ctx);
    EXPECT_EQ(result1, DHT11_OK);
    EXPECT_EQ(handle.pin_ctx, pin_ctx);

    // Set some state to verify it gets reset
    handle.last_reading_time_ms = 12345;

    // Second init should overwrite
    dht11_result_t result2 = dht11_init(&handle, pin_ctx_2);
    EXPECT_EQ(result2, DHT11_OK);
    EXPECT_EQ(handle.pin_ctx, pin_ctx_2);
    EXPECT_EQ(handle.last_reading_time_ms, 0);  // Should be reset
}
