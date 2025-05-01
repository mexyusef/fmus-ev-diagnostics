#include <gtest/gtest.h>
#include <fmus/j2534.h>
#include <iostream>
#include <memory>
#include <string>
#include <chrono>
#include <thread>

class J2534DeviceTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "Setting up J2534 device test" << std::endl;
    }

    void TearDown() override {
        std::cout << "Tearing down J2534 device test" << std::endl;
    }
};

TEST_F(J2534DeviceTest, MessageBuilderTest) {
    // Test that we can create a message using the builder pattern
    fmus::j2534::Message message = fmus::j2534::MessageBuilder()
        .protocol(1) // CAN
        .id(0x7DF)
        .data({0x02, 0x01, 0x0C})
        .flags(0)
        .build();

    EXPECT_EQ(message.getProtocol(), 1);
    EXPECT_EQ(message.getId(), 0x7DF);
    EXPECT_EQ(message.getDataSize(), 3);
    EXPECT_EQ(message.getFlags(), 0);
}

TEST_F(J2534DeviceTest, FilterBuilderTest) {
    // Test that we can create a filter using the builder pattern
    fmus::j2534::Filter filter = fmus::j2534::FilterBuilder()
        .protocol(1) // CAN
        .filterType(0) // PASS_FILTER
        .maskId(0xFF00)
        .patternId(0x7DF00)
        .flags(0)
        .build();

    EXPECT_EQ(filter.getProtocol(), 1);
    EXPECT_EQ(filter.getFilterType(), 0);
    EXPECT_EQ(filter.getMaskId(), 0xFF00);
    EXPECT_EQ(filter.getPatternId(), 0x7DF00);
    EXPECT_EQ(filter.getFlags(), 0);
}

TEST_F(J2534DeviceTest, ConnectionOptionsTest) {
    // Test that we can create and manipulate connection options
    fmus::j2534::ConnectionOptions options("Test Vendor", 1, 500000, 0, 1000);

    EXPECT_EQ(options.getVendorName(), "Test Vendor");
    EXPECT_EQ(options.getProtocol(), 1);
    EXPECT_EQ(options.getBaudRate(), 500000);
    EXPECT_EQ(options.getFlags(), 0);
    EXPECT_EQ(options.getTimeout(), 1000);

    options.setVendorName("New Vendor");
    options.setProtocol(2);
    options.setBaudRate(250000);
    options.setFlags(1);
    options.setTimeout(2000);

    EXPECT_EQ(options.getVendorName(), "New Vendor");
    EXPECT_EQ(options.getProtocol(), 2);
    EXPECT_EQ(options.getBaudRate(), 250000);
    EXPECT_EQ(options.getFlags(), 1);
    EXPECT_EQ(options.getTimeout(), 2000);
}

TEST_F(J2534DeviceTest, ChannelConfigTest) {
    // Test that we can create and use a CAN channel configuration
    fmus::j2534::ChannelConfig config = fmus::j2534::ChannelConfig::forCAN(500000);

    EXPECT_EQ(config.getBaudRate(), 500000);
    EXPECT_EQ(config.getFlags(), 0);

    // Change some parameters
    config.setBaudRate(250000);
    config.setFlags(0x00000100); // Extended CAN ID
    config.setParameter(0x01, 42); // Set DATA_RATE parameter

    EXPECT_EQ(config.getBaudRate(), 250000);
    EXPECT_EQ(config.getFlags(), 0x00000100);
    EXPECT_EQ(config.getParameter(0x01), 42);
    EXPECT_TRUE(config.hasParameter(0x01));
    EXPECT_FALSE(config.hasParameter(0x02));

    // Check factory methods for different protocols
    auto canExt = fmus::j2534::ChannelConfig::forCANExtended(500000);
    EXPECT_EQ(canExt.getFlags(), 0x00000100); // Should have extended ID flag

    auto iso = fmus::j2534::ChannelConfig::forISO15765(500000);
    EXPECT_EQ(iso.getParameter(0x1E), 8); // Should have block size set
}

TEST_F(J2534DeviceTest, DiscoverAdaptersMock) {
    // Test that we can discover J2534 adapters
    fmus::j2534::Device device;

    // This should return at least the mock adapter
    auto adapters = device.discoverAdapters();

    // In a mock/test setup, we should have at least one adapter
    EXPECT_FALSE(adapters.empty());

    if (!adapters.empty()) {
        EXPECT_FALSE(adapters[0].getVendorName().empty());
        EXPECT_FALSE(adapters[0].getDeviceName().empty());
        EXPECT_FALSE(adapters[0].getLibraryPath().empty());
    }
}

TEST_F(J2534DeviceTest, DeviceErrorTest) {
    // Test the DeviceError class
    const std::string errorMessage = "Test error message";
    const int errorCode = 42;

    fmus::j2534::DeviceError error(errorMessage, errorCode);

    EXPECT_EQ(error.getErrorCode(), errorCode);
    EXPECT_STREQ(error.what(), errorMessage.c_str());
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}