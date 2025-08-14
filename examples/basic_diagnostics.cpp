#include <fmus/j2534.h>
#include <fmus/auto.h>
#include <fmus/logger.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

/**
 * Basic diagnostics example using the FMUS-AUTO library
 *
 * This example shows how to:
 * 1. Connect to a J2534 adapter
 * 2. Send and receive CAN messages
 * 3. Use the logging system
 */
int main(int argc, char* argv[]) {
    // Initialize logger
    auto logger = fmus::Logger::getInstance();
    logger->setLogLevel(fmus::LogLevel::Debug);
    logger->enableConsoleLogging(true);

    // Enable file logging if requested
    if (argc > 1) {
        std::string logFile = argv[1];
        logger->info("Writing logs to file: " + logFile);
        if (!logger->enableFileLogging(logFile)) {
            logger->error("Failed to open log file: " + logFile);
        }
    }

    logger->info("FMUS Auto J2534 Basic Diagnostics Example");

    try {
        // Discover available adapters
        logger->info("Discovering J2534 adapters...");
        auto adapters = fmus::j2534::discoverAdapters();
        
        if (adapters.empty()) {
            logger->error("No J2534 adapters found!");
            return 1;
        }

        // Show available adapters
        logger->info("Found " + std::to_string(adapters.size()) + " adapters:");
        for (size_t i = 0; i < adapters.size(); ++i) {
            logger->info("  [" + std::to_string(i) + "] " + adapters[i].toString());
        }

        // Connect to the first available adapter
        if (!fmus::j2534::connectToDevice(adapters[0])) {
            logger->error("Failed to connect to adapter: " + adapters[0].toString());
            return 1;
        }

        logger->info("Successfully connected to adapter: " + adapters[0].toString());

        // Create a test CAN message
        fmus::j2534::Message testMessage;
        testMessage.protocol = fmus::j2534::Protocol::CAN;
        testMessage.id = 0x7E0; // Typical OBD-II request ID
        testMessage.data = {0x02, 0x01, 0x00}; // OBD-II Mode 1 PID 0 request
        testMessage.flags = 0;

        logger->info("Sending test message: " + testMessage.toString());

        // Send the message
        if (fmus::j2534::sendMessage(testMessage)) {
            logger->info("Message sent successfully");
        } else {
            logger->error("Failed to send message");
        }

        // Try to receive messages
        logger->info("Listening for messages...");
        auto receivedMessages = fmus::j2534::receiveMessages(1000); // 1 second timeout

        if (receivedMessages.empty()) {
            logger->info("No messages received (this is normal for a mock implementation)");
        } else {
            logger->info("Received " + std::to_string(receivedMessages.size()) + " messages:");
            for (const auto& msg : receivedMessages) {
                logger->info("  " + msg.toString());
            }
        }

        // Test message builder
        logger->info("Testing message builder...");
        auto builtMessage = fmus::j2534::MessageBuilder()
            .protocol(fmus::j2534::Protocol::CAN)
            .id(0x7E8)
            .data({0x06, 0x41, 0x00, 0xBE, 0x3F, 0xB8, 0x13})
            .flags(0)
            .build();

        logger->info("Built message: " + builtMessage.toString());

        // Test filter builder
        logger->info("Testing filter builder...");
        auto filter = fmus::j2534::FilterBuilder()
            .protocol(fmus::j2534::Protocol::CAN)
            .filterType(fmus::j2534::FilterType::PASS_FILTER)
            .maskId(0x7FF)
            .patternId(0x7E8)
            .build();

        logger->info("Built filter: " + filter.toString());

        // Disconnect
        fmus::j2534::disconnectFromDevice();
        logger->info("Disconnected from adapter");

    } catch (const fmus::j2534::J2534Error& e) {
        logger->error("J2534 Error: " + std::string(e.what()));
        logger->error("Error code: " + std::to_string(e.getRawErrorCode()));
        return 1;
    } catch (const std::exception& e) {
        logger->error("Unexpected error: " + std::string(e.what()));
        return 1;
    }

    logger->info("Example completed successfully");
    return 0;
}
