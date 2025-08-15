/**
 * @file main.cpp
 * @brief hawkBit DDI C++ 클라이언트의 진입점(엔트리 포인트)
 *
 * English:
 * Minimal CLI that constructs a `HawkbitClient` and starts the polling loop.
 *
 * 한국어:
 * 간단한 CLI로 `HawkbitClient` 객체를 생성하여 폴링 루프를 시작합니다.
 * C++ 기본 문법 요소도 함께 확인할 수 있습니다:
 * - `int main(int argc, char* argv[])`: 프로그램 시작점 및 인자 처리
 * - `std::string`: 동적 길이 문자열 클래스
 * - 예외 처리(`try/catch`)와 표준 입출력(`std::cout`, `std::cerr`)
 */
#include "hawkbit_client.h"
#include <iostream>
#include <string>

/**
 * @brief 프로그램 시작 함수 (C++ 표준 시그니처)
 *
 * English:
 * Parses optional CLI arguments: `[server_url] [controller_id]` and runs the client.
 *
 * 한국어:
 * 선택적 CLI 인자 `[server_url] [controller_id]`를 파싱하여 클라이언트를 실행합니다.
 * - `argc`: 인자의 개수 (프로그램 경로 포함)
 * - `argv`: 인자 문자열 배열 (`argv[0]`는 실행 파일 경로)
 *
 * 예시:
 *   ./build/client http://localhost:8000 device001
 */
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