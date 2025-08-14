#ifndef FMUS_WEB_WEB_SERVER_H
#define FMUS_WEB_WEB_SERVER_H

/**
 * @file web_server.h
 * @brief Web Server for Remote Diagnostics Interface
 */

#include <fmus/auto.h>
#include <string>
#include <memory>
#include <functional>
#include <map>
#include <vector>

// Define exports macro
#ifdef _WIN32
    #ifdef FMUS_AUTO_EXPORTS
        #define FMUS_AUTO_API __declspec(dllexport)
    #else
        #define FMUS_AUTO_API __declspec(dllimport)
    #endif
#else
    #define FMUS_AUTO_API
#endif

namespace fmus {
namespace web {

/**
 * @brief HTTP Request structure
 */
struct HTTPRequest {
    std::string method;                     ///< GET, POST, PUT, DELETE
    std::string path;                       ///< Request path
    std::string version;                    ///< HTTP version
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> parameters;
    std::string body;                       ///< Request body
    std::string clientIP;                   ///< Client IP address
    
    std::string toString() const;
};

/**
 * @brief HTTP Response structure
 */
struct HTTPResponse {
    int statusCode = 200;                   ///< HTTP status code
    std::string statusMessage = "OK";       ///< Status message
    std::map<std::string, std::string> headers;
    std::string body;                       ///< Response body
    std::string contentType = "text/html";  ///< Content type
    
    HTTPResponse() {
        headers["Server"] = "FMUS-Auto/1.0";
        headers["Connection"] = "close";
    }
    
    void setJSON(const std::string& json);
    void setHTML(const std::string& html);
    void setError(int code, const std::string& message);
    
    std::string toString() const;
};

/**
 * @brief Request handler function type
 */
using RequestHandler = std::function<HTTPResponse(const HTTPRequest&)>;

/**
 * @brief WebSocket message structure
 */
struct WebSocketMessage {
    enum class Type {
        TEXT,
        BINARY,
        PING,
        PONG,
        CLOSE
    };
    
    Type type = Type::TEXT;
    std::string data;
    bool isFinal = true;
    
    std::string toString() const;
};

/**
 * @brief WebSocket connection
 */
class FMUS_AUTO_API WebSocketConnection {
public:
    WebSocketConnection(int socketFd);
    ~WebSocketConnection();
    
    /**
     * @brief Send message
     */
    bool sendMessage(const WebSocketMessage& message);
    
    /**
     * @brief Receive message
     */
    WebSocketMessage receiveMessage();
    
    /**
     * @brief Check if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Close connection
     */
    void close();
    
    /**
     * @brief Get client IP
     */
    std::string getClientIP() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Web Server Configuration
 */
struct WebServerConfig {
    std::string address = "0.0.0.0";        ///< Bind address
    int port = 8080;                        ///< Port number
    int maxConnections = 100;               ///< Max concurrent connections
    int timeout = 30;                       ///< Connection timeout (seconds)
    bool enableSSL = false;                 ///< Enable HTTPS
    std::string sslCertFile;                ///< SSL certificate file
    std::string sslKeyFile;                 ///< SSL private key file
    std::string documentRoot = "./web";     ///< Document root directory
    bool enableWebSocket = true;            ///< Enable WebSocket support
    bool enableCORS = true;                 ///< Enable CORS headers
    
    std::string toString() const;
};

/**
 * @brief Web Server Implementation
 */
class FMUS_AUTO_API WebServer {
public:
    /**
     * @brief Constructor
     */
    WebServer();
    
    /**
     * @brief Destructor
     */
    ~WebServer();
    
    /**
     * @brief Initialize server
     */
    bool initialize(const WebServerConfig& config);
    
    /**
     * @brief Start server
     */
    bool start();
    
    /**
     * @brief Stop server
     */
    void stop();
    
    /**
     * @brief Check if running
     */
    bool isRunning() const;
    
    /**
     * @brief Set Auto instance
     */
    void setAutoInstance(std::shared_ptr<Auto> autoInstance);
    
    /**
     * @brief Add request handler
     */
    void addHandler(const std::string& path, const std::string& method, RequestHandler handler);
    
    /**
     * @brief Add WebSocket handler
     */
    void addWebSocketHandler(const std::string& path, 
                            std::function<void(std::shared_ptr<WebSocketConnection>)> handler);
    
    /**
     * @brief Serve static file
     */
    void serveStaticFiles(const std::string& path, const std::string& directory);
    
    /**
     * @brief Get server statistics
     */
    struct Statistics {
        uint64_t requestsHandled = 0;
        uint64_t bytesTransferred = 0;
        uint64_t activeConnections = 0;
        uint64_t totalConnections = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();
    
    /**
     * @brief Get current configuration
     */
    WebServerConfig getConfiguration() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief REST API Handler
 */
class FMUS_AUTO_API RestAPIHandler {
public:
    RestAPIHandler(std::shared_ptr<Auto> autoInstance);
    ~RestAPIHandler();
    
    /**
     * @brief Register API endpoints
     */
    void registerEndpoints(WebServer* server);
    
    /**
     * @brief Handle ECU list request
     */
    HTTPResponse handleECUList(const HTTPRequest& request);
    
    /**
     * @brief Handle ECU info request
     */
    HTTPResponse handleECUInfo(const HTTPRequest& request);
    
    /**
     * @brief Handle DTC request
     */
    HTTPResponse handleDTCs(const HTTPRequest& request);
    
    /**
     * @brief Handle live data request
     */
    HTTPResponse handleLiveData(const HTTPRequest& request);
    
    /**
     * @brief Handle diagnostic request
     */
    HTTPResponse handleDiagnostic(const HTTPRequest& request);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief WebSocket API Handler
 */
class FMUS_AUTO_API WebSocketAPIHandler {
public:
    WebSocketAPIHandler(std::shared_ptr<Auto> autoInstance);
    ~WebSocketAPIHandler();
    
    /**
     * @brief Handle WebSocket connection
     */
    void handleConnection(std::shared_ptr<WebSocketConnection> connection);
    
    /**
     * @brief Broadcast message to all connections
     */
    void broadcast(const WebSocketMessage& message);
    
    /**
     * @brief Start live data streaming
     */
    void startLiveDataStream();
    
    /**
     * @brief Stop live data streaming
     */
    void stopLiveDataStream();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Web Server Error
 */
class FMUS_AUTO_API WebServerError : public std::runtime_error {
public:
    enum class ErrorCode {
        INITIALIZATION_FAILED,
        BIND_FAILED,
        SSL_ERROR,
        HANDLER_ERROR,
        CONNECTION_ERROR
    };
    
    WebServerError(ErrorCode code, const std::string& message)
        : std::runtime_error(message), errorCode(code) {}
    
    ErrorCode getErrorCode() const { return errorCode; }

private:
    ErrorCode errorCode;
};

// Utility functions
FMUS_AUTO_API std::string urlDecode(const std::string& encoded);
FMUS_AUTO_API std::string urlEncode(const std::string& decoded);
FMUS_AUTO_API std::string mimeTypeFromExtension(const std::string& extension);
FMUS_AUTO_API std::map<std::string, std::string> parseQueryString(const std::string& query);
FMUS_AUTO_API std::string generateWebSocketKey();
FMUS_AUTO_API std::string calculateWebSocketAccept(const std::string& key);

} // namespace web
} // namespace fmus

#endif // FMUS_WEB_WEB_SERVER_H
