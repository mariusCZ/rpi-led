#include "../include/main.h"
#include <Wt/WApplication.h>

// std::unique_ptr<WApplication> createApplication(const WEnvironment &env) {
//     std::unique_ptr<WApplication> app = std::make_unique<WApplication>(env);
//     app->root()->addWidget(std::make_unique<MainWidget>());

//     return app;
// }

// Start the web server. The main widget is in the server.cpp/.h file.
int main(int argc, char *argv[]) {
    return Wt::WRun(argc, argv, [](const WEnvironment &env) {
        return std::make_unique<MainWidget>(env);
    });
}