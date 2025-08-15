/**
 * @file hawkbit_client.h
 * @brief hawkBit DDI API 클라이언트 구현을 위한 헤더 파일
 * 
 * 이 헤더는 Eclipse hawkBit의 Device Direct Integration (DDI) API를 구현하는
 * 클라이언트 클래스를 정의합니다. IoT 기기의 firmware 업데이트 과정을 시뮬레이션합니다.
 * 
 * 주요 C++ 학습 포인트:
 * - 객체지향 설계 패턴 (class 설계)
 * - 멤버 변수 naming convention (trailing underscore)
 * - const correctness와 reference parameter 활용
 * - STL string과 size_t 타입 활용
 * - 복합 데이터 구조체 (struct) 설계
 * - 캡슐화 (public/private 접근 제어)
 * 
 * hawkBit DDI API 학습 포인트:
 * - Polling 기반 업데이트 확인 방식
 * - RESTful API 엔드포인트 구조
 * - JSON 응답 데이터 파싱
 * - Binary 파일 다운로드 처리
 * - 상태 보고 (status reporting) 메커니즘
 */

#ifndef HAWKBIT_CLIENT_H
#define HAWKBIT_CLIENT_H

// 의존성 포함 - HTTP 통신을 위한 클라이언트
#include "http_client.h"
// 표준 라이브러리 - 문자열 처리
#include <string>

/**
 * @struct DeploymentInfo
 * @brief 배포 정보를 담는 데이터 구조체
 * 
 * hawkBit 서버로부터 받은 배포(deployment) 정보를 저장하는 구조체입니다.
 * 이 구조체는 Plain Old Data (POD) 타입으로 설계되어 간단한 초기화와
 * 복사가 가능합니다.
 * 
 * Modern C++ 특징:
 * - 멤버 초기화 (member initialization) 지원
 * - 자동 복사/이동 생성자 (copy/move constructor)
 * - STL container와 호환 가능
 * - Aggregate initialization 지원: DeploymentInfo{id, url, size, true}
 */
struct DeploymentInfo {
    std::string id;             ///< 배포 고유 식별자 (deployment ID)
    std::string download_url;   ///< firmware 다운로드 URL
    size_t file_size;          ///< 파일 크기 (bytes 단위)
    bool has_deployment;       ///< 유효한 배포 정보 여부를 나타내는 flag
    
    // 구조체는 기본적으로 모든 멤버가 public이며
    // 자동으로 default constructor, copy constructor, assignment operator가 생성됨
};

/**
 * @class HawkbitClient
 * @brief hawkBit DDI 클라이언트의 메인 클래스
 * 
 * 이 클래스는 IoT 기기 역할을 하며 hawkBit 서버와 통신하여
 * firmware 업데이트를 수행합니다. 
 * 
 * 클래스 설계 원칙:
 * - Single Responsibility: hawkBit DDI 프로토콜만 담당
 * - Composition over Inheritance: HttpClient를 포함하여 사용
 * - Encapsulation: private 멤버로 내부 구현 숨김
 * - RAII: 생성자에서 초기화, 소멸자에서 정리
 * 
 * 동작 방식:
 * 1. 서버 polling으로 업데이트 확인
 * 2. 업데이트 있으면 firmware 다운로드
 * 3. 설치 시뮬레이션 수행
 * 4. 결과를 서버에 보고
 */
class HawkbitClient {
public:
    /**
     * @brief 생성자 - hawkBit 클라이언트 초기화
     * 
     * @param server_url hawkBit 서버의 base URL (예: "http://localhost:8000")
     * @param controller_id 이 기기의 고유 식별자 (예: "device001")
     * 
     * 생성자에서 하는 일:
     * - 서버 URL과 controller ID 저장
     * - HttpClient 인스턴스 초기화
     * - 내부 상태 변수들 초기화
     * 
     * Modern C++ 특징:
     * - const std::string& 파라미터로 불필요한 복사 방지
     * - 멤버 초기화 리스트 (member initializer list) 사용 권장
     * - explicit 키워드로 암시적 변환 방지 가능
     */
    HawkbitClient(const std::string& server_url, const std::string& controller_id);
    
    /**
     * @brief 서버에 업데이트 polling 요청 수행
     * 
     * @return DeploymentInfo 배포 정보 (없으면 has_deployment = false)
     * 
     * 이 메서드는 hawkBit DDI API의 핵심인 polling 동작을 구현합니다.
     * GET 요청으로 서버에서 대기 중인 배포가 있는지 확인합니다.
     * 
     * HTTP 엔드포인트: GET /rest/v1/ddi/v1/controller/device/{controller_id}
     * 
     * 응답 처리:
     * 1. HTTP 응답 코드 확인 (200 OK)
     * 2. JSON 응답 body 파싱
     * 3. 배포 정보 추출 및 DeploymentInfo 구조체로 변환
     * 
     * 에러 처리:
     * - 네트워크 오류시 has_deployment = false
     * - JSON 파싱 실패시 has_deployment = false
     * - 404 응답시 (업데이트 없음) has_deployment = false
     */
    DeploymentInfo poll_for_updates();
    
    /**
     * @brief firmware 파일을 다운로드하고 로컬에 저장
     * 
     * @param deployment 다운로드할 배포 정보
     * @param local_path 저장할 로컬 파일 경로
     * @return true 다운로드 성공, false 실패
     * 
     * 대용량 파일 다운로드를 위한 streaming 방식을 사용합니다:
     * 1. HTTP GET 요청으로 파일 다운로드 시작
     * 2. 데이터를 chunk 단위로 받아서 즉시 파일에 쓰기
     * 3. 메모리 사용량을 최소화하여 IoT 기기에 적합
     * 
     * 검증 과정:
     * - HTTP 응답 코드 확인 (200 OK)
     * - 파일 크기 검증 (deployment.file_size와 비교)
     * - 파일 쓰기 권한 확인
     * 
     * 실제 IoT 환경에서 추가할 사항:
     * - 체크섬 검증 (MD5, SHA256)
     * - 재시도 로직 (네트워크 불안정 대응)
     * - 진행률 콜백 (progress callback)
     */
    bool download_firmware(const DeploymentInfo& deployment, const std::string& local_path);
    
    /**
     * @brief 배포 결과를 서버에 보고
     * 
     * @param deployment_id 보고할 배포의 ID
     * @param status 배포 상태 ("SUCCESS", "FAILURE", "RUNNING" 등)
     * @return true 보고 성공, false 실패
     * 
     * hawkBit DDI 프로토콜의 마지막 단계로, 기기가 배포 결과를
     * 서버에 알리는 역할을 합니다.
     * 
     * HTTP 엔드포인트: POST /rest/v1/ddi/v1/controller/device/{controller_id}/deploymentBase/{deployment_id}
     * 
     * 요청 body (JSON):
     * {
     *   "id": "deployment_id",
     *   "time": "2024-01-01T12:00:00Z",
     *   "status": "SUCCESS",
     *   "details": ["Optional detail messages"]
     * }
     * 
     * 상태 값 의미:
     * - SUCCESS: 배포 성공적으로 완료
     * - FAILURE: 배포 실패 (에러 발생)
     * - RUNNING: 배포 진행 중 (중간 상태 보고)
     */
    bool report_status(const std::string& deployment_id, const std::string& status);
    
    /**
     * @brief 메인 polling loop 실행
     * 
     * 이 메서드는 실제 IoT 기기의 동작을 시뮬레이션하는 무한 루프입니다:
     * 
     * 루프 동작 순서:
     * 1. 서버에 업데이트 polling (poll_for_updates)
     * 2. 업데이트가 있으면 firmware 다운로드 (download_firmware)
     * 3. 설치 과정 시뮬레이션 (실제로는 파일 검증만)
     * 4. 결과를 서버에 보고 (report_status)
     * 5. 일정 시간 대기 후 1번부터 반복
     * 
     * 이 패턴은 실제 IoT 기기에서 사용되는 일반적인 방식입니다:
     * - Pull 방식: 기기가 능동적으로 업데이트 확인
     * - 비동기적: 기기의 일정에 따라 업데이트 수행
     * - 장애 복구: 네트워크 문제 발생시 자동 재시도
     * 
     * 프로덕션 환경에서는 추가 고려사항:
     * - 지수 백오프 (exponential backoff) 재시도
     * - 배터리 상태에 따른 업데이트 일정 조절
     * - 사용자 정의 maintenance window
     */
    void run_polling_loop();

private:
    // 멤버 변수들 - 모두 trailing underscore naming convention 사용
    // 이는 Google C++ Style Guide에서 권장하는 방식으로
    // 멤버 변수와 지역 변수를 구별하기 쉽게 만듭니다
    
    /**
     * @brief hawkBit 서버의 base URL
     * 
     * 예: "http://localhost:8000" 또는 "https://update.company.com"
     * 모든 API 엔드포인트는 이 URL을 base로 구성됩니다
     */
    std::string server_url_;
    
    /**
     * @brief 이 기기의 고유 식별자
     * 
     * hawkBit에서 각 기기를 구별하는 ID입니다.
     * 일반적으로 MAC 주소, 시리얼 번호, 또는 UUID를 사용합니다.
     */
    std::string controller_id_;
    
    /**
     * @brief HTTP 통신을 담당하는 클라이언트 객체
     * 
     * Composition 패턴으로 HttpClient를 포함합니다.
     * 이는 상속보다 더 유연한 설계를 제공합니다:
     * - 테스트 시 mock 객체로 교체 가능
     * - HttpClient 구현 변경이 HawkbitClient에 영향 없음
     */
    HttpClient http_client_;
    
    /**
     * @brief polling URL을 생성하는 헬퍼 메서드
     * 
     * @return 완전한 polling URL 문자열
     * 
     * 서버 URL과 controller ID를 조합하여 polling 엔드포인트의
     * 완전한 URL을 생성합니다.
     * 
     * 예: "http://localhost:8000/rest/v1/ddi/v1/controller/device/device001"
     * 
     * 이런 헬퍼 메서드를 만드는 이유:
     * - URL 구성 로직의 중앙화
     * - 오타 방지 및 유지보수 용이
     * - URL 형식 변경시 한 곳만 수정
     */
    std::string build_polling_url();
    
    /**
     * @brief 상태 보고 URL을 생성하는 헬퍼 메서드
     * 
     * @param deployment_id 상태를 보고할 배포 ID
     * @return 완전한 상태 보고 URL 문자열
     * 
     * 특정 deployment에 대한 상태 보고 엔드포인트 URL을 생성합니다.
     * 
     * 예: "http://localhost:8000/rest/v1/ddi/v1/controller/device/device001/deploymentBase/12345"
     */
    std::string build_status_url(const std::string& deployment_id);
    
    /**
     * @brief JSON 응답을 파싱하여 배포 정보 추출
     * 
     * @param json_response 서버로부터 받은 JSON 문자열
     * @return 파싱된 DeploymentInfo 구조체
     * 
     * 간단한 JSON 파싱 로직을 구현합니다. 프로덕션 환경에서는
     * rapidjson, nlohmann/json 같은 전문 라이브러리 사용을 권장합니다.
     * 
     * 파싱할 JSON 구조:
     * {
     *   "deploymentBase": {
     *     "id": "12345",
     *     "download": {
     *       "links": {
     *         "firmware": {
     *           "href": "http://server/files/firmware.bin",
     *           "size": 1048576
     *         }
     *       }
     *     }
     *   }
     * }
     * 
     * 에러 처리:
     * - JSON 형식 오류시 has_deployment = false 반환
     * - 필수 필드 누락시 has_deployment = false 반환
     * - 부분적 파싱 성공시에도 검증 후 결정
     */
    DeploymentInfo parse_deployment_response(const std::string& json_response);
};

#endif // HAWKBIT_CLIENT_H