#include "hawkbit_client.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>

HawkbitClient::HawkbitClient(const std::string& server_url, const std::string& controller_id)
    : server_url_(server_url), controller_id_(controller_id) {
}

std::string HawkbitClient::build_polling_url() {
    return server_url_ + "/rest/v1/ddi/v1/controller/device/" + controller_id_;
}

std::string HawkbitClient::build_status_url(const std::string& deployment_id) {
    return server_url_ + "/rest/v1/ddi/v1/controller/device/" + controller_id_ + 
           "/deploymentBase/" + deployment_id;
}

DeploymentInfo HawkbitClient::parse_deployment_response(const std::string& json_response) {
    DeploymentInfo deployment;
    deployment.has_deployment = false;
    
    // Simple JSON parsing (in production, use a proper JSON library)
    size_t deployment_pos = json_response.find("\"deploymentBase\"");
    if (deployment_pos == std::string::npos) {
        return deployment;
    }
    
    // Extract deployment ID
    size_t id_pos = json_response.find("\"id\":", deployment_pos);
    if (id_pos != std::string::npos) {
        size_t id_start = json_response.find("\"", id_pos + 5) + 1;
        size_t id_end = json_response.find("\"", id_start);
        deployment.id = json_response.substr(id_start, id_end - id_start);
    }
    
    // Extract download URL
    size_t href_pos = json_response.find("\"href\":");
    if (href_pos != std::string::npos) {
        size_t url_start = json_response.find("\"", href_pos + 7) + 1;
        size_t url_end = json_response.find("\"", url_start);
        deployment.download_url = json_response.substr(url_start, url_end - url_start);
    }
    
    // Extract file size
    size_t size_pos = json_response.find("\"size\":");
    if (size_pos != std::string::npos) {
        size_t size_start = size_pos + 7;
        size_t size_end = json_response.find_first_of(",}", size_start);
        std::string size_str = json_response.substr(size_start, size_end - size_start);
        deployment.file_size = std::stoull(size_str);
    }
    
    deployment.has_deployment = !deployment.id.empty() && !deployment.download_url.empty();
    return deployment;
}

DeploymentInfo HawkbitClient::poll_for_updates() {
    std::cout << "Polling for updates..." << std::endl;
    
    HttpResponse response = http_client_.get(build_polling_url());
    
    if (response.status_code == 200) {
        std::cout << "Poll response: " << response.body << std::endl;
        return parse_deployment_response(response.body);
    } else {
        std::cout << "Poll failed with status code: " << response.status_code << std::endl;
        DeploymentInfo empty_deployment;
        empty_deployment.has_deployment = false;
        return empty_deployment;
    }
}

bool HawkbitClient::download_firmware(const DeploymentInfo& deployment, const std::string& local_path) {
    std::cout << "Downloading firmware from: " << deployment.download_url << std::endl;
    std::cout << "Expected file size: " << deployment.file_size << " bytes" << std::endl;
    
    bool success = http_client_.download_file(deployment.download_url, local_path);
    
    if (success) {
        std::cout << "Firmware downloaded successfully to: " << local_path << std::endl;
    } else {
        std::cout << "Firmware download failed!" << std::endl;
    }
    
    return success;
}

bool HawkbitClient::report_status(const std::string& deployment_id, const std::string& status) {
    std::cout << "Reporting status: " << status << " for deployment: " << deployment_id << std::endl;
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&time_t);
    time_str.pop_back(); // Remove newline
    
    // Create JSON payload
    std::ostringstream json_payload;
    json_payload << "{"
                 << "\"id\":\"" << deployment_id << "\","
                 << "\"time\":\"" << time_str << "\","
                 << "\"status\":\"" << status << "\","
                 << "\"details\":[]"
                 << "}";
    
    HttpResponse response = http_client_.post(build_status_url(deployment_id), 
                                             json_payload.str(), 
                                             "application/json");
    
    if (response.status_code == 200) {
        std::cout << "Status reported successfully" << std::endl;
        return true;
    } else {
        std::cout << "Status report failed with code: " << response.status_code << std::endl;
        return false;
    }
}

void HawkbitClient::run_polling_loop() {
    std::cout << "Starting hawkBit client polling loop..." << std::endl;
    std::cout << "Controller ID: " << controller_id_ << std::endl;
    std::cout << "Server URL: " << server_url_ << std::endl;
    
    while (true) {
        try {
            DeploymentInfo deployment = poll_for_updates();
            
            if (deployment.has_deployment) {
                std::cout << "New deployment found: " << deployment.id << std::endl;
                
                // Download firmware
                std::string firmware_path = "downloaded_firmware.bin";
                bool download_success = download_firmware(deployment, firmware_path);
                
                // Report status
                std::string status = download_success ? "SUCCESS" : "FAILURE";
                report_status(deployment.id, status);
                
                if (download_success) {
                    std::cout << "Firmware update completed successfully!" << std::endl;
                } else {
                    std::cout << "Firmware update failed!" << std::endl;
                }
            } else {
                std::cout << "No updates available" << std::endl;
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Error in polling loop: " << e.what() << std::endl;
        }
        
        // Wait 10 seconds before next poll
        std::cout << "Waiting 10 seconds before next poll..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}