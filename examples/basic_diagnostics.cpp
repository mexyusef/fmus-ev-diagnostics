#include <fmus/j2534.h>
#include <fmus/auto.h>
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
 * 2. Access an ECU
 * 3. Read and clear diagnostic trouble codes
 * 4. Monitor some live data parameters
 */
int main(int argc, char* argv[]) {
    // Inisialisasi logger
    auto logger = fmus::Logger::getInstance();
    logger->setLogLevel(fmus::LogLevel::Debug);
    logger->enableConsoleLogging(true);

    // Coba untuk enable file logging jika diinginkan
    if (argc > 1) {
        std::string logFile = argv[1];
        logger->info("Writing logs to file: " + logFile);
        if (!logger->enableFileLogging(logFile)) {
            logger->error("Failed to open log file: " + logFile);
        }
    }

    logger->info("FMUS Auto J2534 Basic Diagnostics Example");

    try {
        // Buat instance J2534 device
        fmus::j2534::Device device;

        // Discover available adapters
        logger->info("Discovering J2534 adapters...");
        auto adapters = device.discoverAdapters();

        if (adapters.empty()) {
            logger->error("No J2534 adapters found. Please connect a J2534 device and try again.");
            return 1;
        }

        // Show available adapters
        logger->info("Found " + std::to_string(adapters.size()) + " adapters:");
        for (size_t i = 0; i < adapters.size(); ++i) {
            logger->info("  " + std::to_string(i + 1) + ": " + adapters[i].toString());
        }

        // Select first adapter for this example
        auto& selectedAdapter = adapters[0];
        logger->info("Connecting to adapter: " + selectedAdapter.toString());

        // Connect to the adapter
        if (!device.connect(selectedAdapter)) {
            logger->error("Failed to connect to adapter");
            return 1;
        }

        logger->info("Successfully connected to adapter");

        // Create a CAN channel configuration (500 kbps)
        fmus::j2534::ChannelConfig config = fmus::j2534::ChannelConfig::forCAN(500000);
        logger->info("Using channel config: " + config.toString());

        // Open a CAN channel
        logger->info("Opening CAN channel...");
        unsigned long channelId = device.openChannel(1, config); // 1 = CAN
        logger->info("Successfully opened channel with ID: " + std::to_string(channelId));

        // Create a pass filter for all CAN messages
        fmus::j2534::Filter filter = fmus::j2534::FilterBuilder()
            .protocol(1)             // CAN
            .filterType(0)           // PASS_FILTER
            .maskId(0)               // Don't mask any bits
            .patternId(0)            // Match any ID
            .flags(0)                // No flags
            .build();

        // Start the filter
        logger->info("Setting up message filter...");
        unsigned long filterId = device.startMsgFilter(channelId, filter);
        logger->info("Successfully created filter with ID: " + std::to_string(filterId));

        // Build a test CAN message to send
        fmus::j2534::Message canMessage = fmus::j2534::MessageBuilder()
            .protocol(1)             // CAN
            .id(0x7DF)               // Standard OBD-II broadcast address
            .data({0x02, 0x01, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00}) // Request engine RPM
            .flags(0)                // No flags
            .build();

        logger->info("Sending CAN message: " + canMessage.toString());

        // Send the message
        device.sendMessage(channelId, canMessage, 1000); // 1000 ms timeout

        // Wait for responses
        logger->info("Waiting for responses...");

        // Loop for 3 seconds looking for responses
        for (int i = 0; i < 3; i++) {
            auto messages = device.receiveMessages(channelId, 1000, 10); // 1000 ms timeout, max 10 messages

            if (!messages.empty()) {
                logger->info("Received " + std::to_string(messages.size()) + " messages:");
                for (const auto& msg : messages) {
                    logger->info("  " + msg.toString());
                }
            } else {
                logger->info("No messages received in this interval");
            }

            // Sleep for a second before trying again
            if (i < 2) {
                logger->debug("Waiting 1 second before checking for more messages...");
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        // Cleanup
        logger->info("Stopping message filter...");
        device.stopMsgFilter(channelId, filterId);

        logger->info("Closing channel...");
        device.closeChannel(channelId);

        logger->info("Disconnecting from adapter...");
        device.disconnect();

        logger->info("Example completed successfully");

    } catch (const fmus::j2534::DeviceError& e) {
        logger->error("J2534 Error: " + std::string(e.what()) + " (code: " + std::to_string(e.getErrorCode()) + ")");
        return 1;
    } catch (const std::exception& e) {
        logger->error("Error: " + std::string(e.what()));
        return 1;
    }

    return 0;
}