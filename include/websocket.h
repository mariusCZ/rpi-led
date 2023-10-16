#include <ixwebsocket/IXWebSocketServer.h>

#include <atomic>
#include <iostream>

class SocketServer {
   public:
    SocketServer() { wsConnected = 0; }
    ~SocketServer() {
        if (wsConnected) {
            // Server stop segfaults without changing callback. Likely due to stopping
            // causing callback, which the one in server accesses non-available resources.
            server.setOnClientMessageCallback(
                [](std::shared_ptr<ix::ConnectionState> connectionState,
                   ix::WebSocket &webSocket,
                   const ix::WebSocketMessagePtr &msg) {});
            server.stop();
        }
    }

    int startWsServer(
        std::function<void(const std::shared_ptr<ix::ConnectionState>,
                           ix::WebSocket &, const ix::WebSocketMessagePtr &)>
            callback);

    std::atomic_int wsConnected;

   private:
    ix::WebSocketServer server{8008, "0.0.0.0"};
};