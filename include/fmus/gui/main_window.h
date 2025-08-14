#ifndef FMUS_GUI_MAIN_WINDOW_H
#define FMUS_GUI_MAIN_WINDOW_H

/**
 * @file main_window.h
 * @brief Main GUI Window for FMUS Auto Diagnostics
 */

#include <fmus/auto.h>
#include <fmus/ecu.h>
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <map>

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
namespace gui {

/**
 * @brief GUI Widget base class
 */
class FMUS_AUTO_API Widget {
public:
    Widget(const std::string& name) : widgetName(name) {}
    virtual ~Widget() = default;
    
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update() = 0;
    virtual bool isVisible() const = 0;
    
    const std::string& getName() const { return widgetName; }
    void setName(const std::string& name) { widgetName = name; }

protected:
    std::string widgetName;
};

/**
 * @brief ECU List Widget
 */
class FMUS_AUTO_API ECUListWidget : public Widget {
public:
    ECUListWidget();
    ~ECUListWidget() override;
    
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    /**
     * @brief Set ECU list
     */
    void setECUs(const std::vector<ECU>& ecus);
    
    /**
     * @brief Get selected ECU
     */
    ECU* getSelectedECU();
    
    /**
     * @brief Set selection callback
     */
    void setSelectionCallback(std::function<void(ECU*)> callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Live Data Widget
 */
class FMUS_AUTO_API LiveDataWidget : public Widget {
public:
    LiveDataWidget();
    ~LiveDataWidget() override;
    
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    /**
     * @brief Add parameter to display
     */
    void addParameter(const std::string& name, const std::string& unit);
    
    /**
     * @brief Update parameter value
     */
    void updateParameter(const std::string& name, double value);
    
    /**
     * @brief Clear all parameters
     */
    void clearParameters();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief DTC List Widget
 */
class FMUS_AUTO_API DTCListWidget : public Widget {
public:
    DTCListWidget();
    ~DTCListWidget() override;
    
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    /**
     * @brief Set DTC list
     */
    void setDTCs(const std::vector<DiagnosticTroubleCode>& dtcs);
    
    /**
     * @brief Clear DTCs
     */
    void clearDTCs();
    
    /**
     * @brief Set clear callback
     */
    void setClearCallback(std::function<void()> callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Console Widget for logs and commands
 */
class FMUS_AUTO_API ConsoleWidget : public Widget {
public:
    ConsoleWidget();
    ~ConsoleWidget() override;
    
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
    /**
     * @brief Add log message
     */
    void addLogMessage(const std::string& level, const std::string& message);
    
    /**
     * @brief Execute command
     */
    void executeCommand(const std::string& command);
    
    /**
     * @brief Set command callback
     */
    void setCommandCallback(std::function<std::string(const std::string&)> callback);
    
    /**
     * @brief Clear console
     */
    void clear();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief Main Application Window
 */
class FMUS_AUTO_API MainWindow {
public:
    /**
     * @brief Constructor
     */
    MainWindow();
    
    /**
     * @brief Destructor
     */
    ~MainWindow();
    
    /**
     * @brief Initialize GUI
     */
    bool initialize();
    
    /**
     * @brief Shutdown GUI
     */
    void shutdown();
    
    /**
     * @brief Show window
     */
    void show();
    
    /**
     * @brief Hide window
     */
    void hide();
    
    /**
     * @brief Run main event loop
     */
    int run();
    
    /**
     * @brief Check if window is visible
     */
    bool isVisible() const;
    
    /**
     * @brief Set Auto instance
     */
    void setAutoInstance(std::shared_ptr<Auto> autoInstance);
    
    /**
     * @brief Get widget by name
     */
    Widget* getWidget(const std::string& name);
    
    /**
     * @brief Add custom widget
     */
    void addWidget(const std::string& name, std::unique_ptr<Widget> widget);
    
    /**
     * @brief Set status message
     */
    void setStatusMessage(const std::string& message);
    
    /**
     * @brief Update window title
     */
    void setTitle(const std::string& title);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief GUI Application class
 */
class FMUS_AUTO_API Application {
public:
    /**
     * @brief Get singleton instance
     */
    static Application* getInstance();
    
    /**
     * @brief Initialize application
     */
    bool initialize(int argc, char* argv[]);
    
    /**
     * @brief Run application
     */
    int run();
    
    /**
     * @brief Shutdown application
     */
    void shutdown();
    
    /**
     * @brief Get main window
     */
    MainWindow* getMainWindow();

private:
    Application() = default;
    ~Application() = default;
    
    class Impl;
    std::unique_ptr<Impl> pImpl;
    static Application* instance;
};

/**
 * @brief GUI Theme configuration
 */
struct GUITheme {
    std::string name = "Default";
    std::map<std::string, std::string> colors;
    std::map<std::string, std::string> fonts;
    std::map<std::string, int> sizes;
    
    static GUITheme getDefaultTheme();
    static GUITheme getDarkTheme();
    static GUITheme getLightTheme();
    
    std::string toString() const;
};

/**
 * @brief GUI Configuration
 */
struct GUIConfig {
    std::string windowTitle = "FMUS Auto Diagnostics";
    int windowWidth = 1200;
    int windowHeight = 800;
    bool maximized = false;
    bool showStatusBar = true;
    bool showToolBar = true;
    GUITheme theme = GUITheme::getDefaultTheme();
    
    std::string toString() const;
};

// Utility functions
FMUS_AUTO_API bool initializeGUI(int argc, char* argv[]);
FMUS_AUTO_API void shutdownGUI();
FMUS_AUTO_API MainWindow* createMainWindow(const GUIConfig& config = GUIConfig{});

} // namespace gui
} // namespace fmus

#endif // FMUS_GUI_MAIN_WINDOW_H
