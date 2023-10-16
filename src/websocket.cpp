#include "../include/websocket.h"

#include <iostream>

int SocketServer::startWsServer(
    std::function<void(const std::shared_ptr<ix::ConnectionState>,
                       ix::WebSocket &, const ix::WebSocketMessagePtr &)>
        callback) {
    std::cout << "Starting server" << std::endl;
    server.setOnClientMessageCallback(callback);
    auto res = server.listen();
    if (!res.first) {
        // Error handling
        return 1;
    }

    // Per message deflate connection is enabled by default. It can be disabled
    // which might be helpful when running on low power devices such as a
    // Rasbery Pi
    server.disablePerMessageDeflate();

    // Run the server in the background. Server can be stoped by calling
    // server.stop()
    server.start();

    return 0;
}

// void SocketServer::callbackFunction(std::shared_ptr<ix::ConnectionState>
// connectionState, ix::WebSocket & webSocket, const ix::WebSocketMessagePtr &
// msg) { SocketServer::a = 0;
//}