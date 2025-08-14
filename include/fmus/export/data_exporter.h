#ifndef FMUS_EXPORT_DATA_EXPORTER_H
#define FMUS_EXPORT_DATA_EXPORTER_H

/**
 * @file data_exporter.h
 * @brief Data Export System for Diagnostic Results
 */

#include <fmus/ecu.h>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <chrono>
#include <functional>

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
namespace export_system {

/**
 * @brief Export formats
 */
enum class ExportFormat {
    CSV,            ///< Comma-separated values
    JSON,           ///< JavaScript Object Notation
    XML,            ///< Extensible Markup Language
    PDF,            ///< Portable Document Format
    HTML,           ///< HyperText Markup Language
    EXCEL,          ///< Microsoft Excel format
    TXT,            ///< Plain text
    CUSTOM          ///< Custom format
};

/**
 * @brief Export data types
 */
enum class ExportDataType {
    LIVE_DATA,      ///< Live parameter data
    DTC_DATA,       ///< Diagnostic trouble codes
    ECU_INFO,       ///< ECU identification data
    SESSION_LOG,    ///< Complete session log
    CUSTOM_DATA     ///< Custom data type
};

/**
 * @brief Export configuration
 */
struct ExportConfig {
    ExportFormat format = ExportFormat::CSV;
    std::string filePath;
    bool includeTimestamp = true;
    bool includeHeaders = true;
    bool compressOutput = false;
    std::string dateFormat = "%Y-%m-%d %H:%M:%S";
    std::string delimiter = ",";           ///< For CSV format
    std::string encoding = "UTF-8";
    std::map<std::string, std::string> customOptions;
    
    std::string toString() const;
};

/**
 * @brief Export data container
 */
struct ExportData {
    ExportDataType type;
    std::string name;
    std::string description;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> metadata;
    
    // Data containers for different types
    std::vector<LiveDataParameter> liveData;
    std::vector<DiagnosticTroubleCode> dtcData;
    std::vector<ECUIdentification> ecuInfo;
    std::vector<std::string> logEntries;
    std::map<std::string, std::string> customData;
    
    ExportData(ExportDataType dataType, const std::string& dataName = "")
        : type(dataType), name(dataName), timestamp(std::chrono::system_clock::now()) {}
    
    std::string toString() const;
};

/**
 * @brief Export result
 */
struct ExportResult {
    bool success = false;
    std::string filePath;
    std::string errorMessage;
    size_t recordsExported = 0;
    size_t fileSize = 0;
    std::chrono::milliseconds duration{0};
    
    ExportResult() = default;
    ExportResult(bool s, const std::string& path = "", const std::string& error = "")
        : success(s), filePath(path), errorMessage(error) {}
    
    std::string toString() const;
};

/**
 * @brief Progress callback for export operations
 */
using ExportProgressCallback = std::function<void(
    const std::string& operation,
    size_t current,
    size_t total,
    const std::string& message
)>;

/**
 * @brief Base exporter interface
 */
class FMUS_AUTO_API IExporter {
public:
    virtual ~IExporter() = default;
    
    /**
     * @brief Get supported formats
     */
    virtual std::vector<ExportFormat> getSupportedFormats() const = 0;
    
    /**
     * @brief Export data
     */
    virtual ExportResult exportData(const ExportData& data, const ExportConfig& config,
                                   ExportProgressCallback callback = nullptr) = 0;
    
    /**
     * @brief Validate configuration
     */
    virtual bool validateConfig(const ExportConfig& config) const = 0;
    
    /**
     * @brief Get format description
     */
    virtual std::string getFormatDescription(ExportFormat format) const = 0;
};

/**
 * @brief CSV Exporter
 */
class FMUS_AUTO_API CSVExporter : public IExporter {
public:
    std::vector<ExportFormat> getSupportedFormats() const override;
    ExportResult exportData(const ExportData& data, const ExportConfig& config,
                           ExportProgressCallback callback = nullptr) override;
    bool validateConfig(const ExportConfig& config) const override;
    std::string getFormatDescription(ExportFormat format) const override;

private:
    std::string formatLiveDataCSV(const std::vector<LiveDataParameter>& data, const ExportConfig& config);
    std::string formatDTCDataCSV(const std::vector<DiagnosticTroubleCode>& data, const ExportConfig& config);
    std::string formatECUInfoCSV(const std::vector<ECUIdentification>& data, const ExportConfig& config);
};

/**
 * @brief JSON Exporter
 */
class FMUS_AUTO_API JSONExporter : public IExporter {
public:
    std::vector<ExportFormat> getSupportedFormats() const override;
    ExportResult exportData(const ExportData& data, const ExportConfig& config,
                           ExportProgressCallback callback = nullptr) override;
    bool validateConfig(const ExportConfig& config) const override;
    std::string getFormatDescription(ExportFormat format) const override;

private:
    std::string formatLiveDataJSON(const std::vector<LiveDataParameter>& data, const ExportConfig& config);
    std::string formatDTCDataJSON(const std::vector<DiagnosticTroubleCode>& data, const ExportConfig& config);
    std::string formatECUInfoJSON(const std::vector<ECUIdentification>& data, const ExportConfig& config);
};

/**
 * @brief XML Exporter
 */
class FMUS_AUTO_API XMLExporter : public IExporter {
public:
    std::vector<ExportFormat> getSupportedFormats() const override;
    ExportResult exportData(const ExportData& data, const ExportConfig& config,
                           ExportProgressCallback callback = nullptr) override;
    bool validateConfig(const ExportConfig& config) const override;
    std::string getFormatDescription(ExportFormat format) const override;

private:
    std::string formatLiveDataXML(const std::vector<LiveDataParameter>& data, const ExportConfig& config);
    std::string formatDTCDataXML(const std::vector<DiagnosticTroubleCode>& data, const ExportConfig& config);
    std::string formatECUInfoXML(const std::vector<ECUIdentification>& data, const ExportConfig& config);
};

/**
 * @brief PDF Report Exporter
 */
class FMUS_AUTO_API PDFExporter : public IExporter {
public:
    std::vector<ExportFormat> getSupportedFormats() const override;
    ExportResult exportData(const ExportData& data, const ExportConfig& config,
                           ExportProgressCallback callback = nullptr) override;
    bool validateConfig(const ExportConfig& config) const override;
    std::string getFormatDescription(ExportFormat format) const override;

private:
    std::string generatePDFReport(const ExportData& data, const ExportConfig& config);
};

/**
 * @brief Data Export Manager
 */
class FMUS_AUTO_API DataExporter {
public:
    /**
     * @brief Constructor
     */
    DataExporter();
    
    /**
     * @brief Destructor
     */
    ~DataExporter();
    
    /**
     * @brief Register exporter
     */
    void registerExporter(ExportFormat format, std::unique_ptr<IExporter> exporter);
    
    /**
     * @brief Export data
     */
    ExportResult exportData(const ExportData& data, const ExportConfig& config,
                           ExportProgressCallback callback = nullptr);
    
    /**
     * @brief Get supported formats
     */
    std::vector<ExportFormat> getSupportedFormats() const;
    
    /**
     * @brief Get format description
     */
    std::string getFormatDescription(ExportFormat format) const;
    
    /**
     * @brief Validate export configuration
     */
    bool validateConfig(const ExportConfig& config) const;
    
    /**
     * @brief Create export data from live parameters
     */
    static ExportData createLiveDataExport(const std::vector<LiveDataParameter>& parameters,
                                          const std::string& name = "Live Data");
    
    /**
     * @brief Create export data from DTCs
     */
    static ExportData createDTCExport(const std::vector<DiagnosticTroubleCode>& dtcs,
                                     const std::string& name = "Diagnostic Trouble Codes");
    
    /**
     * @brief Create export data from ECU info
     */
    static ExportData createECUInfoExport(const std::vector<ECUIdentification>& ecuInfo,
                                         const std::string& name = "ECU Information");
    
    /**
     * @brief Get export statistics
     */
    struct Statistics {
        uint64_t exportsCompleted = 0;
        uint64_t exportsFailed = 0;
        uint64_t totalRecordsExported = 0;
        uint64_t totalBytesExported = 0;
        std::chrono::system_clock::time_point startTime;
    };
    
    Statistics getStatistics() const;
    void resetStatistics();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Export template system
 */
class FMUS_AUTO_API ExportTemplate {
public:
    struct TemplateInfo {
        std::string name;
        std::string description;
        ExportFormat format;
        ExportDataType dataType;
        ExportConfig config;
        std::map<std::string, std::string> customFields;
    };
    
    /**
     * @brief Load template from file
     */
    static TemplateInfo loadTemplate(const std::string& filePath);
    
    /**
     * @brief Save template to file
     */
    static bool saveTemplate(const TemplateInfo& templateInfo, const std::string& filePath);
    
    /**
     * @brief Get built-in templates
     */
    static std::vector<TemplateInfo> getBuiltInTemplates();
    
    /**
     * @brief Apply template to export config
     */
    static ExportConfig applyTemplate(const TemplateInfo& templateInfo, const ExportConfig& baseConfig);
};

/**
 * @brief Export Error
 */
class FMUS_AUTO_API ExportError : public std::runtime_error {
public:
    enum class ErrorCode {
        INVALID_FORMAT,
        FILE_WRITE_ERROR,
        DATA_CONVERSION_ERROR,
        TEMPLATE_ERROR,
        CONFIGURATION_ERROR
    };
    
    ExportError(ErrorCode code, const std::string& message)
        : std::runtime_error(message), errorCode(code) {}
    
    ErrorCode getErrorCode() const { return errorCode; }

private:
    ErrorCode errorCode;
};

// Utility functions
FMUS_AUTO_API std::string exportFormatToString(ExportFormat format);
FMUS_AUTO_API ExportFormat stringToExportFormat(const std::string& str);
FMUS_AUTO_API std::string exportDataTypeToString(ExportDataType type);
FMUS_AUTO_API ExportDataType stringToExportDataType(const std::string& str);
FMUS_AUTO_API std::string generateUniqueFileName(const std::string& baseName, ExportFormat format);
FMUS_AUTO_API bool isValidExportPath(const std::string& path);

} // namespace export_system
} // namespace fmus

#endif // FMUS_EXPORT_DATA_EXPORTER_H
