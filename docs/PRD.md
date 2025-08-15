# 📝 PRD (Product Requirements Document)

## 1. 개요 (Introduction)

본 프로젝트는 Eclipse hawkBit의 **DDI(Direct Device Integration) API**를 모방한 서버-클라이언트 통신 예제 개발을 위한 **제품 요구사항 문서**입니다. 이 예제는 실제 hawkBit 시스템의 모든 기능을 구현하기보다, 기기가 서버에 상태를 보고하고 업데이트 작업을 폴링하는 기본적인 통신 흐름을 이해하는 데 중점을 둡니다.

특히, 실제 파일 전송 시나리오를 시뮬레이션하기 위해 **1MB 크기의 더미 파일**을 서버에서 클라이언트로 전송하는 기능을 포함합니다.

* **서버**: **Python FastAPI**를 사용하며, 프로젝트 관리는 **`pyproject.toml`** 파일, 실행은 **UVicorn**으로 합니다.
* **클라이언트**: **C++**로 개발되며, 빌드는 **CMake**와 **`build.sh`** 스크립트를 사용합니다.

-----

## 2. 기능 요구사항 (Functional Requirements)

### 2.1 서버 (FastAPI)

* **기기 상태 폴링 (Polling)**: 클라이언트로부터 특정 `controllerId`를 포함한 **GET** 요청을 받습니다.
* **업데이트 작업 제공**: 폴링 요청에 응답하여 1MB 더미 파일 다운로드 URL이 포함된 **JSON** 형식의 업데이트 정보를 반환합니다.
* **파일 엔드포인트 추가**: `/files/firmware.bin` 엔드포인트에 대한 **GET** 요청을 처리하여 미리 생성해 둔 1MB 더미 파일을 전송합니다.
* **상태 보고 (Reporting)**: 클라이언트로부터 업데이트 작업의 진행 상태를 보고받는 **POST** 요청을 처리합니다.

### 2.2 클라이언트 (C++)

* **서버 폴링**: 특정 `controllerId`를 사용하여 서버에 주기적으로 **GET** 요청을 보냅니다.
* **파일 다운로드**: 서버의 응답 JSON에서 다운로드 URL을 추출하여, 해당 URL로 **GET** 요청을 보내 1MB 더미 파일을 다운로드하고 로컬에 저장합니다.
* **업데이트 작업 시뮬레이션**: 실제 펌웨어 다운로드 및 설치 과정을 시뮬레이션하고, 그 결과를 서버에 보고합니다.
* **상태 보고**: 파일 다운로드 성공/실패 여부를 **POST** 요청으로 서버에 전달합니다.

-----

## 3. 기술 요구사항 (Technical Requirements)

### 3.1 서버

* **언어**: Python
* **프레임워크**: FastAPI
* **패키지 관리**: `uv`를 사용하며, 의존성은 **`pyproject.toml`** 파일에 명시합니다.
  ```toml
  [project]
  name = "hawkbit-server"
  version = "0.1.0"
  dependencies = [
      "fastapi",
      "uvicorn",
      "uv"
  ]
  ```
* **실행 방식**: `uv run uvicorn main:app --reload` 명령어를 사용합니다.
* **파일 전송**: FastAPI의 `FileResponse` 또는 `StreamingResponse`를 활용하여 1MB 더미 파일을 효율적으로 전송합니다.

### 3.2 클라이언트

* **언어**: C++11 이상
* **빌드 시스템**: **CMake**
* **빌드 스크립트**: **`build.sh`** 파일을 제공하여 빌드 과정을 자동화합니다.
  ```bash
  #!/bin/bash

  rm -rf build
  mkdir build
  cd build
  cmake ..
  make
  cd ..

  echo "Client build completed. Executable is at build/client"
  ```
* **HTTP 라이브러리**: **cURL** 또는 **cpp-httplib** 사용을 권장합니다.

-----

## 4. 통신 프로토콜 (Communication Protocol)

본 프로젝트는 hawkBit DDI API의 핵심적인 동작 방식을 모방합니다.

* **폴링**: 클라이언트가 `GET /rest/v1/ddi/v1/controller/device/{controllerId}`를 통해 업데이트 요청.
* **업데이트 정보**: 서버가 JSON 형식으로 응답하며, 다운로드할 파일의 `href`와 `size` 정보를 포함.
  ```json
  {
    "deploymentBase": {
      "id": "12345",
      "download": {
        "links": {
          "firmware": {
            "href": "http://<서버주소>/files/firmware.bin",
            "size": 1048576
          }
        }
      }
    }
  }
  ```
* **상태 보고**: 클라이언트가 `POST /rest/v1/ddi/v1/controller/device/{controllerId}/deploymentBase/{id}`로 작업 상태(예: `SUCCESS`, `FAILURE`)를 보고.