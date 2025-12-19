#include "CLode_monet_new.h"
#include "network_protocol.h"
#include <iostream>

int main() {
    // initializing logger for client
    Logger& client_logger = get_client_logger();
    client_logger.info("========================================================");
    client_logger.info("Client application starting...");
    client_logger.info("========================================================");

    try {
        ClodeMonetClient client;
        client.run();

        client_logger.info("========================================================");
        client_logger.info("Client application shutting down normally");
        client_logger.info("========================================================");

    }
    catch (const std::exception& e) {
        get_client_logger().critical("Client application crashed: {}", e.what());
        return 1;
    }
    catch (...) {
        get_client_logger().critical("Client application crashed with unknown error");
        return 1;
    }

    return 0;
}
