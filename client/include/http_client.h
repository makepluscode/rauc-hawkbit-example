/**
 * @file http_client.h
 * @brief HTTP Client for hawkBit DDI API Communication
 * @author hawkBit DDI Example Project
 * @version 1.0
 * @date 2024-08-15
 *
 * English:
 * Modern C++ wrapper around libcurl for HTTP. Demonstrates RAII, callbacks,
 * STL containers, and simple blocking I/O operations.
 *
 * 한국어:
 * libcurl을 현대 C++ 스타일로 래핑한 HTTP 클라이언트입니다. RAII, 콜백,
 * STL 컨테이너 사용, 블로킹 I/O 설계를 다룹니다.
 *
 * @dot
 * digraph HttpClientDependency {
 *   rankdir=TD;
 *   node [shape=rectangle, style=filled, fillcolor=lightblue];
 *
 *   "HttpClient" -> "libcurl" [label="uses"];
 *   "HttpClient" -> "std::string" [label="contains"];
 *   "HttpClient" -> "std::map" [label="contains"];
 *   "HttpResponse" -> "std::string" [label="contains"];
 *   "HttpResponse" -> "std::map" [label="contains"];
 *   "HawkbitClient" -> "HttpClient" [label="composition"];
 * }
 * @enddot
 *
 * Key C++ Learning Points / 핵심 C++ 학습 포인트:
 * - RAII (Resource Acquisition Is Initialization)
 * - Modern C++ class design (constructor/destructor)
 * - Static callbacks for C library integration
 * - STL containers (std::string, std::map)
 * - Default parameter values
 * - Opaque pointer (pimpl) for implementation hiding
 *
 * HTTP Client Design / HTTP 클라이언트 설계:
 * - Synchronous (blocking) operations
 * - Automatic memory management
 * - Header parsing/management
 * - Large-file streaming download
 * - Boolean-based error handling
 */

#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

// Standard library includes - modern C++ containers and types
#include <string>    // std::string - modern C++ string class (better than char*)
#include <map>       // std::map - associative container for key-value pairs

/**
 * @struct HttpResponse
 * @brief Container for HTTP response data / HTTP 응답 컨테이너
 *
 * English:
 * Holds status code, body, and headers using STL containers.
 *
 * 한국어:
 * HTTP 상태 코드, 본문, 헤더를 STL 컨테이너로 보관합니다.
 *
 * Modern C++ features / 현대 C++ 특징:
 * - Aggregate initialization
 * - 자동 메모리 관리(STL)
 */
struct HttpResponse {
    long status_code;                               // HTTP status code (200, 404, 500, etc.)
    std::string body;                               // Response body content as string
    std::map<std::string, std::string> headers;     // HTTP headers as key-value pairs
    
    // Note: This struct uses default copy/move semantics
    // C++11 and later provide efficient move operations automatically
};

/**
 * @class HttpClient
 * @brief Modern C++ HTTP client using RAII principles
 *
 * @dot
 * digraph HttpClientFlow {
 *   rankdir=LR;
 *   node [shape=box, style=filled];
 *
 *   start [label="Constructor\n(curl 초기화)", fillcolor=lightgreen];
 *   get [label="GET 요청", fillcolor=lightblue];
 *   post [label="POST 요청", fillcolor=lightblue];
 *   download [label="파일 다운로드", fillcolor=lightblue];
 *   end [label="Destructor\n(curl 정리)", fillcolor=lightcoral];
 *
 *   start -> get;
 *   start -> post;
 *   start -> download;
 *   get -> end;
 *   post -> end;
 *   download -> end;
 * }
 * @enddot
 *
 * English:
 * Wraps libcurl with RAII, pimpl-like opaque pointer, and static callbacks.
 *
 * 한국어:
 * libcurl을 RAII와 opaque 포인터, 정적 콜백으로 감싼 구현입니다.
 */
class HttpClient {
public:
    /**
     * @brief Constructor - Initializes curl handle and global curl state
     * 
     * RAII Pattern: Constructor acquires all necessary resources
     * - Calls curl_global_init() for library initialization
     * - Creates curl easy handle for this instance
     * - Sets up initial state for HTTP operations
     * 
     * Exception Safety: Strong guarantee - if construction fails,
     * no resources are leaked
     */
    HttpClient();
    
    /**
     * @brief Destructor - Cleans up curl resources
     * 
     * RAII Pattern: Destructor releases all acquired resources
     * - Cleans up curl easy handle if valid
     * - Calls curl_global_cleanup() for library cleanup
     * - Ensures no memory leaks regardless of how object is destroyed
     * 
     * Modern C++ Note: Destructor is automatically called when:
     * - Object goes out of scope (stack allocation)
     * - delete is called (heap allocation)
     * - Exception occurs during object lifetime
     */
    ~HttpClient();
    
    /**
     * @brief Performs HTTP GET request
     * 
     * @param url The URL to request (must be valid HTTP/HTTPS URL)
     * @return HttpResponse containing status, body, and headers
     * 
     * Modern C++ Features:
     * - const std::string& parameter (no unnecessary copying)
     * - Return by value (RVO/move semantics optimize this)
     * - STL containers handle memory automatically
     * 
     * HTTP Method: GET is idempotent and safe for polling operations
     * Used by devices to check for available updates
     */
    HttpResponse get(const std::string& url);
    
    /**
     * @brief Performs HTTP POST request with data
     * 
     * @param url Target URL for the POST request
     * @param data Request body data (JSON, form data, etc.)
     * @param content_type MIME type of the data (default: application/json)
     * @return HttpResponse containing server response
     * 
     * Default Parameters: C++ feature allowing optional parameters
     * - content_type defaults to "application/json"
     * - Caller can override for other data types
     * - Reduces API complexity while maintaining flexibility
     * 
     * HTTP Method: POST creates new resources (status reports)
     * Used by devices to report deployment outcomes
     */
    HttpResponse post(const std::string& url, 
                     const std::string& data, 
                     const std::string& content_type = "application/json");
    
    /**
     * @brief Downloads file from URL to local filesystem
     * 
     * @param url Source URL of the file to download
     * @param filepath Local path where file should be saved
     * @return true if download succeeded, false otherwise
     * 
     * Streaming Design: File is written directly to disk without
     * loading entire content into memory. This is crucial for:
     * - Large firmware files (can be hundreds of MB)
     * - Memory-constrained IoT devices
     * - Network interruption recovery
     * 
     * Boolean Return: Simple success/failure indication
     * More complex error handling could use std::optional or exceptions
     */
    bool download_file(const std::string& url, const std::string& filepath);

private:
    /**
     * @brief Opaque pointer to curl handle
     * 
     * Pimpl Idiom (Pointer to Implementation):
     * - Hides libcurl types from header file
     * - Reduces compilation dependencies
     * - Allows implementation changes without recompiling clients
     * - void* instead of CURL* avoids including curl headers
     * 
     * This is a common pattern when wrapping C libraries in C++
     */
    void* curl_handle;
    
    /**
     * @brief Static callback for writing response body data
     * 
     * Static Function Requirement: libcurl (C library) expects C-style
     * function pointers, not C++ member function pointers
     * 
     * @param contents Data buffer from curl
     * @param size Size of each data element
     * @param nmemb Number of data elements
     * @param userp User pointer (cast to std::string* for response body)
     * @return Number of bytes processed (must equal size * nmemb)
     * 
     * Callback Pattern: Common in C libraries for handling data streams
     * The userp parameter allows passing context (our std::string)
     */
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    /**
     * @brief Static callback for parsing HTTP headers
     * 
     * @param buffer Header line from HTTP response
     * @param size Size of each character (always 1 for headers)
     * @param nitems Number of characters in header line
     * @param userdata User pointer (cast to std::map* for headers)
     * @return Number of bytes processed
     * 
     * Header Format: "Key: Value\r\n"
     * This callback parses each header line and stores in std::map
     */
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata);
    
    /**
     * @brief Static callback for writing downloaded file data
     * 
     * @param contents File data buffer from curl
     * @param size Size of each data element
     * @param nmemb Number of data elements  
     * @param userp User pointer (cast to std::ofstream* for file writing)
     * @return Number of bytes written to file
     * 
     * File Streaming: Data is written directly to disk as received
     * This prevents memory buildup for large file downloads
     */
    static size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp);
    
    // Note: Copy constructor and assignment operator are implicitly deleted
    // because the class manages resources (curl handle) that shouldn't be shared
    // Modern C++ (C++11+): Can explicitly delete with = delete if desired
    // HttpClient(const HttpClient&) = delete;
    // HttpClient& operator=(const HttpClient&) = delete;
};

#endif // HTTP_CLIENT_H