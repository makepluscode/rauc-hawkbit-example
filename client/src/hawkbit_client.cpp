/**
 * @file hawkbit_client.cpp
 * @brief hawkBit DDI 클라이언트 로직 구현 파일
 *
 * English:
 * Implements the methods declared in `hawkbit_client.h` using a simple
 * string-based JSON parsing approach and a blocking polling loop.
 *
 * 한국어:
 * `hawkbit_client.h`에 선언된 메서드를 구현합니다. 간단한 문자열 기반 JSON 파싱과
 * 블로킹 폴링 루프를 사용합니다. 실제 제품에서는 신뢰성/유지보수를 위해 JSON 라이브러리
 * 사용(nlohmann/json 등)과 견고한 에러 처리, 비동기/스레드 설계가 권장됩니다.
 */
#include "hawkbit_client.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>

/**
 * @brief 생성자: 서버 URL과 컨트롤러 ID를 저장
 *
 * - 멤버 이니셜라이저 리스트를 사용하여 `server_url_`, `controller_id_` 초기화
 * - `http_client_`는 기본 생성자 사용
 */
HawkbitClient::HawkbitClient(const std::string& server_url, const std::string& controller_id)
    : server_url_(server_url), controller_id_(controller_id) {
}

/**
 * @brief 폴링 엔드포인트 URL 생성
 *
 * 완전한 URL을 조합합니다 (예: http://host/rest/v1/ddi/v1/controller/device/{id}).
 */
std::string HawkbitClient::build_polling_url() {
    return server_url_ + "/rest/v1/ddi/v1/controller/device/" + controller_id_;
}

/**
 * @brief 상태 보고 엔드포인트 URL 생성
 *
 * 배포 ID를 포함한 완전한 URL을 조합합니다.
 */
std::string HawkbitClient::build_status_url(const std::string& deployment_id) {
    return server_url_ + "/rest/v1/ddi/v1/controller/device/" + controller_id_ + 
           "/deploymentBase/" + deployment_id;
}

/**
 * @brief 서버의 배포 응답(JSON 문자열)에서 핵심 필드 추출
 *
 * 반환값: 파싱된 `DeploymentInfo` (없으면 `has_deployment=false`).
 * 주의: 단순 문자열 탐색 기반 구현으로 포맷 변경에 취약합니다.
 */
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

/**
 * @brief 서버에 업데이트 폴링 요청 수행
 *
 * 반환값: 배포 정보 (`has_deployment`가 false면 업데이트 없음).
 */
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

/**
 * @brief 펌웨어를 다운로드하여 로컬 경로에 저장
 *
 * 반환값: 성공 여부.
 */
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

/**
 * @brief 배포 결과 상태를 서버에 보고
 *
 * 반환값: 성공 여부.
 */
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

/**
 * @brief 무한 폴링 루프 실행 (학습용 구현)
 *
 * 루프: poll → download → report → sleep(10s)
 * - 실제 환경에서는 종료 조건, 신호 처리, 백오프 전략 등을 추가하세요.
 */
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