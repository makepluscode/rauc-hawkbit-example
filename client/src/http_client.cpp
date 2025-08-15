/**
 * @file http_client.cpp
 * @brief HTTP 클라이언트 구현 파일
 * 
 * libcurl을 사용한 HTTP 통신 기능을 구현합니다.
 * Modern C++의 RAII 패턴과 STL container를 활용하여
 * C 라이브러리를 안전하게 래핑합니다.
 */

// 헤더 파일 포함 - 클래스 선언부
#include "http_client.h"
// C 라이브러리 - HTTP 통신을 위한 libcurl
#include <curl/curl.h>
// 표준 라이브러리 - 콘솔 출력 및 파일 입출력
#include <iostream>
#include <fstream>

/**
 * @brief HttpClient 생성자 - curl 리소스 초기화
 * 
 * RAII 패턴의 핵심: 생성자에서 모든 필요한 리소스를 획득합니다.
 * 
 * curl 초기화 과정:
 * 1. curl_global_init(): 전역 curl 라이브러리 초기화
 * 2. curl_easy_init(): 이 인스턴스만의 curl handle 생성
 * 
 * CURL_GLOBAL_DEFAULT는 다음을 포함합니다:
 * - SSL 라이브러리 초기화
 * - Win32 Winsock 초기화 (Windows)
 * - 기타 플랫폼별 네트워크 초기화
 * 
 * Modern C++ 특징:
 * - 생성자에서 예외 발생시 자동으로 이미 생성된 멤버들의 소멸자 호출
 * - 리소스 누수 방지를 위한 RAII 패턴 적용
 */
HttpClient::HttpClient() {
    // 전역 curl 라이브러리 초기화 (thread-safe하지 않으므로 주의 필요)
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    // 이 인스턴스용 curl easy handle 생성
    // 실패시 nullptr 반환, 성공시 유효한 포인터 반환
    curl_handle = curl_easy_init();
}

/**
 * @brief HttpClient 소멸자 - curl 리소스 정리
 * 
 * RAII 패턴의 핵심: 소멸자에서 획득한 모든 리소스를 해제합니다.
 * 
 * 정리 순서:
 * 1. curl easy handle 정리 (생성의 역순)
 * 2. 전역 curl 라이브러리 정리
 * 
 * 안전 장치:
 * - curl_handle이 nullptr인지 확인 후 정리
 * - 중복 호출되어도 안전하도록 설계
 * 
 * Modern C++ 보장:
 * - 예외 발생 중에도 소멸자는 반드시 호출됨
 * - Stack unwinding 과정에서 자동 리소스 정리
 */
HttpClient::~HttpClient() {
    // curl handle이 유효한 경우에만 정리
    if (curl_handle) {
        curl_easy_cleanup(curl_handle);
    }
    
    // 전역 curl 라이브러리 정리
    // 주의: 다른 curl 인스턴스가 있다면 문제가 될 수 있음
    // 실제 프로덕션에서는 reference counting 등의 기법 필요
    curl_global_cleanup();
}

/**
 * @brief HTTP 응답 body 데이터를 처리하는 static callback 함수
 * 
 * @param contents curl이 전달하는 데이터 버퍼
 * @param size 각 데이터 요소의 크기 (보통 1)
 * @param nmemb 데이터 요소의 개수 (실제 바이트 수)
 * @param userp 사용자 포인터 (std::string*로 캐스팅됨)
 * @return 처리한 바이트 수 (size * nmemb와 같아야 함)
 * 
 * Static 함수인 이유:
 * - libcurl(C 라이브러리)은 C++ 멤버 함수 포인터를 받을 수 없음
 * - C 스타일 함수 포인터만 허용
 * - userp 매개변수를 통해 클래스 인스턴스 데이터에 접근
 * 
 * 동작 방식:
 * 1. 실제 데이터 크기 계산 (size * nmemb)
 * 2. userp를 std::string* 타입으로 캐스팅
 * 3. 받은 데이터를 string에 추가 (append)
 * 4. 처리한 바이트 수 반환 (성공 표시)
 * 
 * 메모리 안전성:
 * - static_cast로 안전한 타입 변환
 * - std::string의 자동 메모리 관리 활용
 * - 버퍼 오버플로우 방지
 */
size_t HttpClient::WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    // 실제 데이터 크기 계산
    size_t realsize = size * nmemb;
    
    // userp를 std::string 포인터로 안전하게 캐스팅
    // 이 포인터는 HTTP 응답 body를 저장할 string 객체를 가리킴
    std::string* response = static_cast<std::string*>(userp);
    
    // 받은 데이터를 string 끝에 추가
    // append()는 메모리를 자동으로 확장하므로 안전함
    response->append(static_cast<char*>(contents), realsize);
    
    // curl에게 모든 데이터를 처리했음을 알림
    // 반환값이 realsize와 다르면 curl은 전송을 중단함
    return realsize;
}

/**
 * @brief HTTP 응답 header를 파싱하는 static callback 함수
 * 
 * @param buffer curl이 전달하는 header 라인 버퍼
 * @param size 각 문자의 크기 (항상 1)
 * @param nitems header 라인의 문자 수
 * @param userdata 사용자 포인터 (std::map*로 캐스팅됨)
 * @return 처리한 바이트 수
 * 
 * HTTP Header 형식:
 * - "Header-Name: Header-Value\r\n"
 * - 각 header는 별도의 라인으로 전달됨
 * - 마지막에 \r\n (CRLF) 포함
 * 
 * 파싱 과정:
 * 1. header 라인을 string으로 변환
 * 2. ':' 문자 위치 찾기 (key/value 구분자)
 * 3. key 부분 추출 (colon 앞)
 * 4. value 부분 추출 (colon 뒤, 공백 제거)
 * 5. CRLF 문자들 제거
 * 6. map에 key-value 쌍으로 저장
 * 
 * 에러 처리:
 * - colon이 없는 라인은 무시 (status line 등)
 * - 빈 값도 허용
 * - \r\n 문자 자동 제거
 */
size_t HttpClient::HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    // 실제 header 라인 크기 계산
    size_t realsize = size * nitems;
    
    // userdata를 header map 포인터로 캐스팅
    std::map<std::string, std::string>* headers = 
        static_cast<std::map<std::string, std::string>*>(userdata);
    
    // buffer를 std::string으로 변환 (안전한 문자열 처리를 위해)
    std::string header(buffer, realsize);
    
    // ':' 문자 위치 찾기 (header key와 value의 구분자)
    size_t colon_pos = header.find(':');
    
    if (colon_pos != std::string::npos) {
        // Key 부분 추출 (colon 앞부분)
        std::string key = header.substr(0, colon_pos);
        
        // Value 부분 추출 (colon 뒤부분, +2는 ": " 건너뛰기)
        std::string value = header.substr(colon_pos + 2);
        
        // 줄바꿈 문자 제거 (\n)
        if (!value.empty() && value.back() == '\n') {
            value.pop_back();
        }
        
        // 캐리지 리턴 문자 제거 (\r)
        if (!value.empty() && value.back() == '\r') {
            value.pop_back();
        }
        
        // map에 key-value 쌍 저장
        // operator[]는 새 key 삽입 또는 기존 value 업데이트
        (*headers)[key] = value;
    }
    
    // curl에게 header 라인을 성공적으로 처리했음을 알림
    return realsize;
}

/**
 * @brief 파일 다운로드를 위한 static callback 함수
 * 
 * @param contents curl이 전달하는 파일 데이터 버퍼
 * @param size 각 데이터 요소의 크기
 * @param nmemb 데이터 요소의 개수
 * @param userp 사용자 포인터 (std::ofstream*로 캐스팅됨)
 * @return 파일에 쓴 바이트 수
 * 
 * 스트리밍 다운로드:
 * - 받은 데이터를 즉시 파일에 기록
 * - 메모리에 전체 파일을 로드하지 않음
 * - 대용량 파일 다운로드에 적합
 * - 메모리 제약이 있는 IoT 기기에 필수적
 * 
 * 장점:
 * - 메모리 사용량 최소화
 * - 네트워크 중단시 부분 다운로드 보존 가능
 * - 실시간 처리 가능 (압축 해제 등)
 * 
 * 주의사항:
 * - 파일이 열려있는지 미리 확인 필요
 * - 디스크 공간 부족시 에러 처리 필요
 * - 파일 권한 확인 필요
 */
size_t HttpClient::WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    // 실제 데이터 크기 계산
    size_t realsize = size * nmemb;
    
    // userp를 파일 스트림 포인터로 캐스팅
    std::ofstream* file = static_cast<std::ofstream*>(userp);
    
    // 받은 데이터를 파일에 직접 기록
    // binary 모드로 열린 파일이므로 데이터 변환 없이 그대로 쓰기
    file->write(static_cast<char*>(contents), realsize);
    
    // 실제 쓴 바이트 수 반환
    // 파일 쓰기 실패시 실제 쓴 크기와 다를 수 있음
    return realsize;
}

/**
 * @brief HTTP GET 요청을 수행하는 메서드
 * 
 * @param url 요청할 URL (http:// 또는 https://)
 * @return HttpResponse 응답 데이터 (status code, body, headers)
 * 
 * HTTP GET의 특징:
 * - Idempotent: 여러 번 호출해도 동일한 결과
 * - Safe: 서버 상태를 변경하지 않음
 * - Cacheable: 응답을 캐시할 수 있음
 * - hawkBit polling에 적합한 메서드
 * 
 * Modern C++ 특징:
 * - const std::string& 파라미터 (불필요한 복사 방지)
 * - Return by value (move semantics로 최적화)
 * - RAII 스타일 에러 처리
 * 
 * curl 설정 순서:
 * 1. curl handle 초기화 확인
 * 2. 이전 설정 리셋 (clean state)
 * 3. 필요한 옵션들 설정
 * 4. 요청 실행
 * 5. 결과 확인 및 반환
 */
HttpResponse HttpClient::get(const std::string& url) {
    // 응답 데이터를 저장할 구조체 초기화
    HttpResponse response;
    
    // curl handle 유효성 검사 (생성자에서 실패했을 가능성)
    if (!curl_handle) {
        response.status_code = 0;  // 0은 curl 에러를 의미
        return response;
    }
    
    // 이전 설정을 모두 리셋 (clean state 보장)
    // 이는 이전 요청의 설정이 현재 요청에 영향주는 것을 방지
    curl_easy_reset(curl_handle);
    
    // 요청할 URL 설정
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    
    // 응답 body를 처리할 callback 함수 설정
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response.body);
    
    // HTTP header를 처리할 callback 함수 설정
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response.headers);
    
    // HTTP redirect 자동 처리 (3xx 응답시 Location header 따라가기)
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    
    // 요청 timeout 설정 (30초)
    // IoT 환경에서는 네트워크가 불안정할 수 있으므로 timeout 필수
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    // HTTP 요청 실행
    CURLcode res = curl_easy_perform(curl_handle);
    
    // 요청 결과 확인
    if (res == CURLE_OK) {
        // 성공시 HTTP status code 추출
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response.status_code);
    } else {
        // 실패시 에러 처리
        response.status_code = 0;
        std::cerr << "cURL GET 에러: " << curl_easy_strerror(res) << std::endl;
    }
    
    // 응답 구조체 반환 (move semantics로 효율적)
    return response;
}

HttpResponse HttpClient::post(const std::string& url, const std::string& data, 
                             const std::string& content_type) {
    HttpResponse response;
    
    if (!curl_handle) {
        response.status_code = 0;
        return response;
    }
    
    // Reset all options first
    curl_easy_reset(curl_handle);
    
    struct curl_slist* headers = nullptr;
    std::string content_type_header = "Content-Type: " + content_type;
    headers = curl_slist_append(headers, content_type_header.c_str());
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, data.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, HeaderCallback);
    curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, &response.headers);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl_handle);
    
    if (res == CURLE_OK) {
        curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response.status_code);
    } else {
        response.status_code = 0;
        std::cerr << "cURL error: " << curl_easy_strerror(res) << std::endl;
    }
    
    curl_slist_free_all(headers);
    return response;
}

bool HttpClient::download_file(const std::string& url, const std::string& filepath) {
    if (!curl_handle) {
        return false;
    }
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    // Reset all options first
    curl_easy_reset(curl_handle);
    
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &file);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl_handle);
    file.close();
    
    if (res != CURLE_OK) {
        std::cerr << "Download failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    
    long response_code;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &response_code);
    
    return response_code == 200;
}