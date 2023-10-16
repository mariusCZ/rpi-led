#include "../include/server.h"

#include <stdexcept>  // std::out_of_range

#include "../include/guihandle.h"

/*
    TODO: refactor the way the widgets are implemented.
    As it is right now, it's quite a mess and it can be significantly cleaned
   up.
*/

// Definitions for static objects. They need to be static since
// for each connection to the web page, a new thread with a new
// Main widget class is started.
ServerConfig MainWidget::config;
VisObject MainWidget::vis;
SocketServer MainWidget::ws;

// Main widget constructor with WApplication parent. This parent
// was chosen to allow for real time update of all sessions using
// post function.
MainWidget::MainWidget(const WEnvironment &env) : WApplication(env) {
    // Get application instance and enable updates of all pages outside
    // main thread.
    app = WApplication::instance();
    app->enableUpdates(true);
    app->setTheme(std::make_shared<WBootstrap5Theme>());
    // Create a container where all widgets will be placed.
    auto container = std::make_unique<Wt::WContainerWidget>();
    // Create a grid where all widgets will be distributed.
    auto grid = container->setLayout(std::make_unique<Wt::WGridLayout>());

    // Button which starts the separate websocket server on 8008 port.
    startButton_ =
        grid->addWidget(std::make_unique<WPushButton>("Start Server"), 0, 0);
    startButton_->setStyleClass("btn-primary");
    startButton_->clicked().connect(startButton_, &WPushButton::disable);
    startButton_->clicked().connect(this, &MainWidget::startButtonPress);
    startButton_->setMargin(2);
    startButton_->resize(WLength::Auto, MAIN_BTN_SIZE);
    startButton_->decorationStyle().font().setSize(FontSize::XXLarge);

    // Button which enables the LED visualization.
    startLedButton =
        grid->addWidget(std::make_unique<WPushButton>("Start LEDs"), 0, 1);
    startLedButton->setStyleClass("btn-secondary");
    startLedButton->clicked().connect(this, &MainWidget::ledButtonPress);
    startLedButton->setMargin(2);
    startLedButton->resize(WLength::Auto, MAIN_BTN_SIZE);
    startLedButton->decorationStyle().font().setSize(FontSize::XXLarge);

    // A virtual box layout to add both text and a combobox to a single grid
    // point.
    auto vbox = std::make_unique<WVBoxLayout>();
    vbox->addWidget(std::make_unique<WText>("Select a mode:"))
        ->decorationStyle()
        .font()
        .setSize(FontSize::Large);

    // Add a combobox with these items. This is abstracted and full
    // implementation is in guihandle.h
    std::vector<std::string> mItems{"Music", "Patterns", "Theater",
                                    "Single Color"};
    modeSelect =
        GUI::addComboBox(vbox, mItems, &MainWidget::onModeSelect, this);

    // Add previous to grid.
    grid->addLayout(std::move(vbox), 1, 0);

    // Add a horizontal box, in which different comboboxes are displayed.
    // A hbox allows to hide widgets and for other to show up and it auto
    // resizes the widgets. Doing this in a grid breaks things.
    auto hbox = std::make_unique<WHBoxLayout>();
    deviceSelect = GUI::addComboBox(hbox, &MainWidget::onDeviceSelect, this);

    std::vector<std::string> pItems{"Pat 1", "Pat 2", "Pat 3"};
    patternSelect =
        GUI::addComboBox(hbox, pItems, &MainWidget::onPatternSelect, this);

    // Color picker widget for single color lighting.
    colorPick = hbox->addWidget(std::make_unique<WColorPicker>());
    colorPick->resize(WLength::Auto, COMBO_SIZE);
    colorPick->colorInput().connect(this, &MainWidget::onColorSelect);

    // Second vbox for the different comboboxes in the hbox. Vbox is used again
    // to have text above the combobox. The text is changed in callbacks.
    auto vbox2 = std::make_unique<WVBoxLayout>();
    modesText = vbox2->addWidget(std::make_unique<WText>("Select a device:"));
    modesText->decorationStyle().font().setSize(FontSize::Large);
    vbox2->addLayout(std::move(hbox));

    grid->addLayout(std::move(vbox2), 1, 1);

    // First table for audio visualization settings.
    auto table = std::make_unique<Wt::WTable>();
    table->setHeaderCount(0);
    table->setWidth(Wt::WLength("100%"));

    table->elementAt(0, 0)
        ->addWidget(std::make_unique<WText>("Visualization mode"))
        ->decorationStyle()
        .font()
        .setSize(FontSize::Large);
    table->elementAt(1, 0)
        ->addWidget(std::make_unique<WText>("Color mode (if it applies)"))
        ->decorationStyle()
        .font()
        .setSize(FontSize::Large);
    table->elementAt(2, 0)
        ->addWidget(
            std::make_unique<WText>("Color coefficient (if it applies)"))
        ->decorationStyle()
        .font()
        .setSize(FontSize::Large);

    table->elementAt(3, 0)
        ->addWidget(std::make_unique<WText>("Min Decay Rate"))
        ->decorationStyle()
        .font()
        .setSize(FontSize::Large);
    table->elementAt(4, 0)
        ->addWidget(std::make_unique<WText>("Max Decay Rate"))
        ->decorationStyle()
        .font()
        .setSize(FontSize::Large);

    // Audio visualization mode selection.
    std::vector<std::string> aItems{"Center spectrum", "Popping spectrum"};
    audioModeSelect = GUI::addComboBox(std::move(table), aItems, 0, 1,
                                       &MainWidget::onAudioModeSelect, this);
    // Audio visualization color selection (will be changed eventually).
    std::vector<std::string> cItems{"Main Red", "Main Green", "Main Blue"};
    colorModeAudioSelect = GUI::addComboBox(
        std::move(table), cItems, 1, 1, &MainWidget::onColorModeSelect, this);
    // A color slider (also will be changed).
    colorCoeffSlider =
        GUI::addSliderToTable(std::move(table), 2, 1, 0, 255, 5, vis.colorCoeff,
                              &MainWidget::onColorCoeffSlider, this);
    // Sliders controlling FFT bin min and max decay rates.
    minRateSlider = GUI::addSliderToTable(std::move(table), 3, 1, 1, 40, 5,
                                          vis.decay_min * 10,
                                          &MainWidget::onMinRateSlider, this);
    maxRateSlider = GUI::addSliderToTable(std::move(table), 4, 1, 1, 40, 5,
                                          vis.decay_max * 10,
                                          &MainWidget::onMaxRateSlider, this);

    // Button that resets the peak audio value.
    auto peakButton = table->elementAt(5, 0)->addWidget(
        std::make_unique<WPushButton>("Reset Peak"));
    peakButton->clicked().connect([=] { vis.maxPeakReset = true; });

    // Put all these settings in the table in a collapsible panel.
    auto panel = std::make_unique<Wt::WPanel>();
    panel->setTitle("Audio visualization settings.");
    panel->addStyleClass("centered-example");
    panel->setCollapsible(true);

    Wt::WAnimation animation(Wt::AnimationEffect::SlideInFromTop,
                             Wt::TimingFunction::EaseOut, 100);

    panel->setAnimation(animation);
    panel->setCentralWidget(std::move(table));
    grid->addWidget(std::move(panel), 2, 0);

    // Second table for advanced audio vis controls.
    auto table2 = std::make_unique<Wt::WTable>();

    // Auto gain control button (in development for ESP32).
    auto agcButton = table2->elementAt(0, 0)->addWidget(
        std::make_unique<WPushButton>("Perform AGC"));
    agcButton->clicked().connect([=] {
        std::string devMsg = "";
        config.devMsg = "AGC";
    });

    // Put all these settings in the table in a collapsible panel.
    auto panel2 = std::make_unique<Wt::WPanel>();
    panel2->setTitle("Advanced audio settings.");
    panel2->addStyleClass("centered-example");
    panel2->setCollapsible(true);
    panel2->setAnimation(animation);
    panel2->setCentralWidget(std::move(table2));
    grid->addWidget(std::move(panel2), 3, 0);

    // Display all of these widgets.
    root()->addWidget(std::move(container));

    // Mutex to access a static config object used to have persisting
    // settings between sessions.
    std::lock_guard<std::mutex> lck(serverMutex);
    // Set all of the settings for the new sessions.
    if (config.wsStarted) {
        startButton_->disable();
    }

    if (!config.ledButtonState) {
        startLedButton->decorationStyle().setBackgroundColor(
            WColor(108, 117, 125));
        startLedButton->setText("Start LEDs");
    } else {
        startLedButton->decorationStyle().setBackgroundColor(
            WColor(220, 53, 69));
        startLedButton->setText("Stop LEDs");
    }

    modeSelect->setCurrentIndex(config.modeIndex);
    updateModeDisplay(config.modeIndex);
    patternSelect->setCurrentIndex(config.patIndex);
    deviceSelect->setCurrentIndex(config.devIndex);
    audioModeSelect->setCurrentIndex(config.audIndex);
    colorModeAudioSelect->setCurrentIndex(config.audColorIndex);
    updateDeviceList(config.selectedDevice);
}

// Callback function for the start server button press.
void MainWidget::startButtonPress() {
    // Set up to pass the callback function to the separate websocket server.
    std::function<void(
        const std::shared_ptr<ix::ConnectionState> connectionState,
        ix::WebSocket &webSocket, const ix::WebSocketMessagePtr &msg)>
        callback =
            std::bind(&MainWidget::wsCallback, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3);
    // If server not already started, start it.
    if (!config.wsStarted)
        ws.startWsServer(callback);
    else {
        return;
    }
    config.wsStarted = 1;
}

// Callback for when the mode is switched in the mode combobox
void MainWidget::onModeSelect() {
    {
        // Lock config mutex and write to it so that selected mode persists in
        // new sessions.
        std::lock_guard<std::mutex> lck(serverMutex);
        config.modeIndex = modeSelect->currentIndex();
    }
    // Change the display mode in the actual visualization object.
    // This variable is atomic so no need for mutex.
    vis.displayMode = modeSelect->currentIndex();
    // Change which widgets are displayed depending on mode selected.
    switch (modeSelect->currentIndex()) {
        case 0:
            modesText->setText("Select a device:");
            colorPick->hide();
            patternSelect->hide();
            deviceSelect->show();
            break;
        case 1:
            modesText->setText("Select a pattern:");
            colorPick->hide();
            deviceSelect->hide();
            patternSelect->show();
            break;
        case 2:
            modesText->setText("Select a placeholder:");
            colorPick->hide();
            deviceSelect->hide();
            patternSelect->hide();
            break;
        case 3:
            modesText->setText("Select a color:");
            deviceSelect->hide();
            patternSelect->hide();
            colorPick->show();
            break;
    }
}
// Same as function above except used to update new session, so no config
// update. There is probably a better way to do this.
void MainWidget::updateModeDisplay(int modeInd) {
    switch (modeInd) {
        case 0:
            modesText->setText("Select a device:");
            colorPick->hide();
            patternSelect->hide();
            deviceSelect->show();
            break;
        case 1:
            modesText->setText("Select a pattern:");
            colorPick->hide();
            deviceSelect->hide();
            patternSelect->show();
            break;
        case 2:
            modesText->setText("Select a placeholder:");
            colorPick->hide();
            deviceSelect->hide();
            patternSelect->hide();
            break;
        case 3:
            modesText->setText("Select a color:");
            deviceSelect->hide();
            patternSelect->hide();
            colorPick->show();
            break;
    }
}

// Pattern combobox selection callback.
void MainWidget::onPatternSelect() {
    std::lock_guard<std::mutex> lck(serverMutex);
    config.patIndex = patternSelect->currentIndex();
}

// Audio device combobox selection callback.
void MainWidget::onDeviceSelect() {
    std::lock_guard<std::mutex> lck(serverMutex);
    config.devIndex = deviceSelect->currentIndex();
    // This is used for ensuring that messages are accepted from the selected
    // device.
    config.selectedDevice = deviceSelect->currentText().toUTF8();
}

// Color picker callback.
void MainWidget::onColorSelect() {
    std::lock_guard<std::mutex> lck(serverMutex);
    if (config.ledButtonState) {
        int r = colorPick->color().red();
        int g = colorPick->color().green();
        int b = colorPick->color().blue();
        unsigned long int colors = 0;
        // Colors need to be stored in a single variable for the library we use.
        colors = (b << 16) | (g << 8) | r;
        // Send the colors to the visualization object.
        vis.ledColorSet(colors);
    }
}

// Minimum decay rate slider callback
void MainWidget::onMinRateSlider() {
    vis.decay_min = (float)minRateSlider->value() / 10.0F;
}

// Maximum decay rate slider callback
void MainWidget::onMaxRateSlider() {
    vis.decay_max = (float)maxRateSlider->value() / 10.0F;
}

// Audio visualization color mode combobox select callback.
void MainWidget::onColorModeSelect() {
    std::lock_guard<std::mutex> lck(serverMutex);
    vis.colorMode = colorModeAudioSelect->currentIndex();
    config.audColorIndex = vis.colorMode;
}

// Audio visualization mode combobox select callback.
void MainWidget::onAudioModeSelect() {
    std::lock_guard<std::mutex> lck(serverMutex);
    vis.audioMode = audioModeSelect->currentIndex();
    config.audIndex = vis.audioMode;
}

// Audio color slider callback.
void MainWidget::onColorCoeffSlider() {
    vis.colorCoeff = colorCoeffSlider->value();
}

// Websocket server callback function
void MainWidget::wsCallback(
    std::shared_ptr<ix::ConnectionState> connectionState,
    ix::WebSocket &webSocket, const ix::WebSocketMessagePtr &msg) {
    // Get the currently selected audio device's IP.
    std::string selectedIP;
    try {
        selectedIP = config.audioDevs.at(config.selectedDevice);
    } catch (const std::out_of_range &oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        selectedIP = "";
    }

    // Callback if a new connection is opened.
    if (msg->type == ix::WebSocketMessageType::Open) {
        ws.wsConnected = 1;
        std::cout << "New connection" << std::endl;

        // A connection state object is available, and has a default id
        // You can subclass ConnectionState and pass an alternate factory
        // to override it. It is useful if you want to store custom
        // attributes per connection (authenticated bool flag, attributes,
        // etc...)
        std::cout << "id: " << connectionState->getId() << std::endl;

        // The uri the client did connect to.
        std::cout << "Uri: " << msg->openInfo.uri << std::endl;

        std::cout << "Headers:" << std::endl;
        for (auto it : msg->openInfo.headers) {
            std::cout << "\t" << it.first << ": " << it.second << std::endl;
        }
        // Callback if a connection is closed.
    } else if (msg->type == ix::WebSocketMessageType::Close) {
        std::lock_guard<std::mutex> lck(serverMutex);
        // Delete the audio device associated with the closed connection.
        deleteAudioDevice(connectionState->getRemoteIp());
        // Update all sessions with device change.
        updateDeviceList(config.selectedDevice);
        // Select the first audio device if one exists.
        if (!selectedIP.compare(connectionState->getRemoteIp()) &&
            !config.audioDevs.empty())
            config.selectedDevice = config.audioDevs.begin()->first;
        // Callback if a message is received.
    } else if (msg->type == ix::WebSocketMessageType::Message) {
        // std::cout << "Message: " << msg->str << std::endl;
        std::lock_guard<std::mutex> lck(serverMutex);
        // First check if there is any pending message (like AGC) to be sent to
        // the audio device and if there is, send it.
        if (!config.devMsg.empty() &&
            !selectedIP.compare(connectionState->getRemoteIp())) {
            webSocket.send(config.devMsg);
            config.devMsg = "";
        }
        // Handle incoming message.
        handleMessage(msg->str, connectionState->getRemoteIp());
    }
}

// Function to handle incoming messages from websocket server.
void MainWidget::handleMessage(std::string message, std::string remoteIp) {
    // Split messages to tokens.
    std::vector<std::string> tokens;
    tokens = split(message, ',');
    // Grab selected device's IP.
    std::string selectedIP;
    try {
        selectedIP = config.audioDevs.at(config.selectedDevice);
    } catch (const std::out_of_range &oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        selectedIP = "";
    }
    // If first token is OCT, it means it is audio data.
    if (tokens[0] == "OCT" && !selectedIP.compare(remoteIp)) {
        const std::unique_lock<std::mutex> lck(vis.oct_lock);
        for (int i = 1; i <= OCT_N; i++) {  // Add mismatch checking
            // Convert text to int and store it for the visualization object to
            // use.
            float output = std::stoi(tokens[i]);
            vis.oct_bins[i - 1] =
                (output > vis.oct_bins[i - 1]) ? output : vis.oct_bins[i - 1];
        }
        // If first token is AUD, it means it's an audio device introducing
        // itself.
    } else if (tokens[0] == "AUD") {
        // Get the name of the device.
        config.audioDevs.emplace(tokens[1], remoteIp);
        // Set selected device to the newly added one.
        config.selectedDevice = tokens[1];
        // Update all sessions with the new device.
        updateDeviceList(tokens[1]);
    }
}

// Function that updates all sessions with the new device
void MainWidget::updateDeviceList(std::string dev) {
    // Get server instance and post new device to all sessions.
    auto server = WServer::instance();
    server->postAll(std::bind(&MainWidget::addDevicePost, this, dev));
}

// Function to post the data to sessions.
void MainWidget::addDevicePost(std::string dev) {
    // Get application instance
    auto appc = dynamic_cast<MainWidget *>(Wt::WApplication::instance());
    assert(appc != nullptr);
    // Add the device to the session's object instance.
    appc->addHelper(dev);

    // Push the changes to the sessions
    appc->triggerUpdate();
}

// Helper function to add (or remove) a device.
void MainWidget::addHelper(std::string dev) {
    // First clear all devices and then iterate through them and add them.
    deviceSelect->clear();
    for (auto &element : config.audioDevs) {
        deviceSelect->addItem(element.first);
    }
    // Update device selection.
    if (!dev.empty()) deviceSelect->setValueText(dev);
}

// Function for deleting an audio device from config.
void MainWidget::deleteAudioDevice(std::string remoteIp) {
    for (auto const &item : config.audioDevs) {
        std::cout << "Enter for loop in delete" << std::endl;
        if (item.second == remoteIp) {
            config.audioDevs.erase(item.first);
            break;
        }
    }
}

// Helper function to split a string in to tokens.
std::vector<std::string> MainWidget::split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

// Callback function for when the LED start button is pressed.
void MainWidget::ledButtonPress() {
    std::lock_guard<std::mutex> lck(serverMutex);
    if (!config.ledButtonState) {
        vis.startVis();
        config.ledButtonState = 1;
        startLedButton->setText("Stop LEDs");
        startLedButton->decorationStyle().setBackgroundColor(
            WColor(220, 53, 69));
    } else {
        vis.stopVis();
        config.ledButtonState = 0;
        startLedButton->setText("Start LEDs");
        startLedButton->decorationStyle().setBackgroundColor(
            WColor(108, 117, 125));
    }
}