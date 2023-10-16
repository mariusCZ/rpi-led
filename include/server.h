#include <Wt/WAnimation.h>
#include <Wt/WApplication.h>
#include <Wt/WBootstrap5Theme.h>
#include <Wt/WBreak.h>
#include <Wt/WColorPicker.h>
#include <Wt/WComboBox.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WEnvironment.h>
#include <Wt/WGridLayout.h>
#include <Wt/WHBoxLayout.h>
#include <Wt/WLayout.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPanel.h>
#include <Wt/WProgressBar.h>
#include <Wt/WPushButton.h>
#include <Wt/WServer.h>
#include <Wt/WSlider.h>
#include <Wt/WTable.h>
#include <Wt/WTemplate.h>
#include <Wt/WText.h>
#include <Wt/WVBoxLayout.h>

#include "conf.h"
#include "vis.h"
#include "websocket.h"

using namespace Wt;

// Class to store the config so as to persist settings
// throughout different web page sessions.
class ServerConfig {
   public:
    // Visualization mode selection index variable.
    int modeIndex = 0;
    // Audio device selection index variable.
    int devIndex = 0;
    // Pattern selection index variable.
    int patIndex = 0;
    // Audio visualization mode selection index variable
    int audIndex = 0;
    // Audio visualization color selection index variable.
    int audColorIndex = 0;

    // State of the LEDs running.
    int ledButtonState = 0;

    // State of the websocket server.
    int wsStarted = 0;

    // Which audio device is selected.
    std::string selectedDevice = "";

    // Store a message to be sent to the audio device.
    std::string devMsg = "";

    // First key is name and second is ip
    std::map<std::string, std::string> audioDevs;

   private:
};

// Main class definition.
class MainWidget : public WApplication {
   public:
    MainWidget(const WEnvironment &env);

   private:
    // Pointer definitions for widgets that need persistent access
    // throughout the session's lifetime.
    WPushButton *startButton_;
    WPushButton *startLedButton;
    WComboBox *modeSelect;
    WComboBox *deviceSelect;
    WComboBox *patternSelect;
    WColorPicker *colorPick;
    WComboBox *colorModeAudioSelect;
    WComboBox *audioModeSelect;
    WSlider *colorCoeffSlider;
    WSlider *minRateSlider;
    WSlider *maxRateSlider;
    WText *modesText;
    WApplication *app;

    // Static object for websocket server.
    static SocketServer ws;

    // Static object for the visualization.
    static VisObject vis;

    // Static object for server config and its mutex.
    static ServerConfig config;
    std::mutex serverMutex;

    void startButtonPress();
    void ledButtonPress();
    void onModeSelect();
    void onPatternSelect();
    void onDeviceSelect();
    void onColorSelect();
    void onMinRateSlider();
    void onMaxRateSlider();
    void onColorModeSelect();
    void onAudioModeSelect();
    void onColorCoeffSlider();
    void dataButtonPress();
    void updateModeDisplay(int modeInd);
    void updateDeviceList(std::string dev);
    void addDevicePost(std::string dev);
    void addHelper(std::string dev);
    void deleteAudioDevice(std::string remoteIp);
    void wsCallback(std::shared_ptr<ix::ConnectionState> connectionState,
                    ix::WebSocket &webSocket,
                    const ix::WebSocketMessagePtr &msg);
    void handleMessage(std::string message, std::string remoteIp);
    void sendTest();
    std::vector<std::string> split(const std::string &s, char delim);
};