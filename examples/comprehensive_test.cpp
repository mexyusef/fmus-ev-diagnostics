#include <fmus/j2534.h>
#include <fmus/auto.h>
#include <fmus/logger.h>
#include <fmus/config.h>
#include <fmus/thread_pool.h>
#include <fmus/utils.h>
#include <fmus/protocols/can.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

/**
 * Comprehensive test of FMUS-AUTO library functionality
 *
 * This example demonstrates:
 * 1. Configuration management
 * 2. Thread pool usage
 * 3. Utility functions
 * 4. CAN protocol handling
 * 5. J2534 device management
 * 6. Logging system
 */
int main(int argc, char* argv[]) {
    std::cout << "=== FMUS-AUTO Comprehensive Test ===" << std::endl;
    
    // Initialize logger
    auto logger = fmus::Logger::getInstance();
    logger->setLogLevel(fmus::LogLevel::Debug);
    logger->enableConsoleLogging(true);
    
    if (argc > 1) {
        std::string logFile = argv[1];
        logger->info("Writing logs to file: " + logFile);
        if (!logger->enableFileLogging(logFile)) {
            logger->error("Failed to open log file: " + logFile);
        }
    }
    
    logger->info("Starting comprehensive FMUS-AUTO test");
    
    try {
        // Test 1: Configuration Management
        logger->info("=== Test 1: Configuration Management ===");
        auto config = fmus::Config::getInstance();
        
        // Test default values
        int canBaudRate = config->getInt(fmus::ConfigKeys::CAN_BAUDRATE);
        logger->info("Default CAN baud rate: " + std::to_string(canBaudRate));
        
        // Test setting and getting values
        config->setValue("test.string", std::string("Hello World"));
        config->setValue("test.int", 42);
        config->setValue("test.bool", true);
        config->setValue("test.double", 3.14159);
        
        logger->info("Test string: " + config->getString("test.string"));
        logger->info("Test int: " + std::to_string(config->getInt("test.int")));
        logger->info("Test bool: " + std::string(config->getBool("test.bool") ? "true" : "false"));
        logger->info("Test double: " + std::to_string(config->getDouble("test.double")));
        
        // Test 2: Utility Functions
        logger->info("=== Test 2: Utility Functions ===");
        
        // Hex utilities
        std::vector<uint8_t> testData = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
        std::string hexStr = fmus::utils::bytesToHex(testData);
        logger->info("Hex string: " + hexStr);
        
        auto decodedData = fmus::utils::hexToBytes(hexStr);
        bool hexMatch = (testData == decodedData);
        logger->info("Hex encode/decode test: " + std::string(hexMatch ? "PASS" : "FAIL"));
        
        // Checksum utilities
        uint8_t checksum8 = fmus::utils::calculateChecksum8(testData);
        uint32_t crc32 = fmus::utils::calculateCRC32(testData);
        logger->info("Checksum8: 0x" + fmus::utils::bytesToHex({checksum8}));
        logger->info("CRC32: 0x" + std::to_string(crc32));
        
        // String utilities
        std::string testStr = "  Hello, World!  ";
        std::string trimmed = fmus::utils::trim(testStr);
        logger->info("Trimmed string: '" + trimmed + "'");
        
        // Test 3: Thread Pool
        logger->info("=== Test 3: Thread Pool ===");
        auto threadPool = fmus::getGlobalThreadPool();
        logger->info("Thread pool threads: " + std::to_string(threadPool->getThreadCount()));
        
        // Submit some async tasks
        std::vector<std::future<int>> futures;
        for (int i = 0; i < 5; ++i) {
            auto future = threadPool->enqueue([i]() -> int {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                return i * i;
            });
            futures.push_back(std::move(future));
        }
        
        // Wait for results
        for (size_t i = 0; i < futures.size(); ++i) {
            int result = futures[i].get();
            logger->info("Task " + std::to_string(i) + " result: " + std::to_string(result));
        }
        
        // Test 4: CAN Protocol
        logger->info("=== Test 4: CAN Protocol ===");
        
        fmus::protocols::CANConfig canConfig;
        canConfig.baudRate = 500000;
        canConfig.extendedFrames = true;
        logger->info("CAN Config: " + canConfig.toString());
        
        fmus::protocols::CANProtocol canProtocol;
        if (canProtocol.initialize(canConfig)) {
            logger->info("CAN protocol initialized successfully");
            
            // Create test messages
            fmus::protocols::CANMessage msg1(0x123, {0x01, 0x02, 0x03, 0x04});
            fmus::protocols::CANMessage msg2(0x456, {0xAA, 0xBB, 0xCC, 0xDD}, true); // Extended
            
            logger->info("Test message 1: " + msg1.toString());
            logger->info("Test message 2: " + msg2.toString());
            
            // Test message validation
            logger->info("Message 1 valid: " + std::string(msg1.isValid() ? "Yes" : "No"));
            logger->info("Message 2 valid: " + std::string(msg2.isValid() ? "Yes" : "No"));
            
            // Test filters
            fmus::protocols::CANFilter filter(0x100, 0x700, false);
            logger->info("CAN Filter: " + filter.toString());
            
            bool msg1Matches = filter.matches(msg1);
            bool msg2Matches = filter.matches(msg2);
            logger->info("Message 1 matches filter: " + std::string(msg1Matches ? "Yes" : "No"));
            logger->info("Message 2 matches filter: " + std::string(msg2Matches ? "Yes" : "No"));
            
            // Send messages
            if (canProtocol.sendMessage(msg1)) {
                logger->info("Message 1 sent successfully");
            }
            
            if (canProtocol.sendMessage(msg2)) {
                logger->info("Message 2 sent successfully");
            }
            
            // Get statistics
            auto stats = canProtocol.getStatistics();
            logger->info("CAN Statistics - Sent: " + std::to_string(stats.messagesSent) + 
                        ", Received: " + std::to_string(stats.messagesReceived));
            
            canProtocol.shutdown();
        } else {
            logger->error("Failed to initialize CAN protocol");
        }
        
        // Test 5: J2534 Device Management
        logger->info("=== Test 5: J2534 Device Management ===");
        
        auto adapters = fmus::j2534::discoverAdapters();
        logger->info("Found " + std::to_string(adapters.size()) + " J2534 adapters");
        
        for (size_t i = 0; i < adapters.size(); ++i) {
            logger->info("Adapter " + std::to_string(i) + ": " + adapters[i].toString());
        }
        
        if (!adapters.empty()) {
            if (fmus::j2534::connectToDevice(adapters[0])) {
                logger->info("Connected to J2534 device successfully");
                
                // Test message builders
                auto j2534Msg = fmus::j2534::MessageBuilder()
                    .protocol(fmus::j2534::Protocol::CAN)
                    .id(0x7E0)
                    .data({0x02, 0x01, 0x00})
                    .build();
                
                logger->info("J2534 Message: " + j2534Msg.toString());
                
                if (fmus::j2534::sendMessage(j2534Msg)) {
                    logger->info("J2534 message sent successfully");
                }
                
                fmus::j2534::disconnectFromDevice();
                logger->info("Disconnected from J2534 device");
            }
        }
        
        // Test 6: Error Handling
        logger->info("=== Test 6: Error Handling ===");
        
        try {
            // Test J2534 error
            throw fmus::j2534::J2534Error(fmus::j2534::ErrorCode::ERR_DEVICE_NOT_CONNECTED, 
                                         "Test error message");
        } catch (const fmus::j2534::J2534Error& e) {
            logger->info("Caught J2534 error: " + std::string(e.what()));
            logger->info("Error code: " + std::to_string(e.getRawErrorCode()));
        }
        
        // Test utility functions
        logger->info("=== Test 7: Additional Utilities ===");
        
        // Platform detection
        logger->info("Platform: " + fmus::utils::getPlatformName());
        logger->info("Is Linux: " + std::string(fmus::utils::isLinux() ? "Yes" : "No"));
        logger->info("Is Windows: " + std::string(fmus::utils::isWindows() ? "Yes" : "No"));
        
        // Validation functions
        bool validVIN = fmus::utils::isValidVIN("1HGBH41JXMN109186");
        logger->info("Valid VIN test: " + std::string(validVIN ? "PASS" : "FAIL"));
        
        bool validCANId = fmus::utils::isValidCANId(0x123, false);
        logger->info("Valid CAN ID test: " + std::string(validCANId ? "PASS" : "FAIL"));
        
        // Hex dump
        fmus::utils::hexDump(testData, "Test Data Hex Dump");
        
    } catch (const std::exception& e) {
        logger->error("Test failed with exception: " + std::string(e.what()));
        return 1;
    }
    
    logger->info("=== All Tests Completed Successfully ===");
    std::cout << "Comprehensive test completed successfully!" << std::endl;
    return 0;
}
