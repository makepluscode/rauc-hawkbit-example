#include "hawkbit_client.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string server_url = "http://localhost:8000";
    std::string controller_id = "device001";
    
    // Parse command line arguments
    if (argc >= 2) {
        server_url = argv[1];
    }
    if (argc >= 3) {
        controller_id = argv[2];
    }
    
    std::cout << "hawkBit DDI Client" << std::endl;
    std::cout << "==================" << std::endl;
    
    try {
        HawkbitClient client(server_url, controller_id);
        client.run_polling_loop();
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}