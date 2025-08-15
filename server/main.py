"""
hawkBit DDI (Direct Device Integration) API Mock Server

English
-------
This module implements a simplified version of Eclipse hawkBit's DDI API for
educational purposes. It demonstrates modern FastAPI patterns, async programming,
and IoT device management concepts.

Eclipse hawkBit is an open-source update server for IoT devices that provides
a standardized way to manage firmware and software updates across distributed
device fleets.

Key Learning Points:
- RESTful API design for IoT device management
- Asynchronous Python programming with FastAPI
- HTTP status codes and proper error handling
- File serving and binary data transfer
- JSON data validation with Pydantic models
- Type hints for better code documentation

한국어
-----
이 모듈은 Eclipse hawkBit의 DDI(Direct Device Integration) API를 간소화하여
학습용으로 구현한 예제입니다. FastAPI의 현대적인 사용 패턴, 비동기 프로그래밍,
그리고 IoT 기기 관리 개념을 보여줍니다.

Eclipse hawkBit은 분산된 기기 플릿의 펌웨어/소프트웨어 업데이트를 표준화된 방식으로
관리할 수 있도록 도와주는 오픈소스 업데이트 서버입니다.

핵심 학습 포인트:
- IoT 기기 관리를 위한 RESTful API 설계
- FastAPI 기반 비동기(Async) 프로그래밍
- HTTP 상태 코드와 올바른 예외 처리
- 파일 제공 및 바이너리 데이터 전송
- Pydantic 모델을 활용한 JSON 데이터 검증
- 타입 힌트를 통한 문서화와 IDE 지원 향상
"""

# Standard library imports
import os
import uvicorn

# Third-party imports - FastAPI ecosystem
from fastapi import FastAPI, HTTPException
from fastapi.responses import FileResponse
from pydantic import BaseModel

# Type hints for better code documentation and IDE support
from typing import Dict, Any, List

# FastAPI application instance with OpenAPI documentation
# The title parameter automatically generates API documentation
app = FastAPI(
    title="hawkBit DDI API Mock Server",
    description="Educational implementation of Eclipse hawkBit DDI API",
    version="1.0.0",
    docs_url="/docs",  # Swagger UI documentation endpoint
    redoc_url="/redoc"  # ReDoc documentation endpoint
)


class StatusReport(BaseModel):
    """
    Pydantic model for device status reports

    English:
    Pydantic provides automatic JSON validation, serialization/deserialization,
    and generates OpenAPI schemas. This model represents the data structure
    that devices send when reporting their update status.

    한국어:
    Pydantic은 JSON 유효성 검사, 직렬화/역직렬화를 자동으로 처리하고
    OpenAPI 스키마를 생성합니다. 이 모델은 기기가 업데이트 상태를 보고할 때
    전송하는 데이터 구조를 표현합니다.

    Modern Python features demonstrated / 현대 파이썬 특징:
    - 타입 주석을 통한 런타임 검증 (Type annotations)
    - 적절한 타입의 기본값 처리
    - 자동 OpenAPI 스키마 생성
    """
    id: str  # Deployment ID that this status report refers to
    time: str  # Timestamp when the status was recorded (ISO format recommended)
    status: str  # Status value: "SUCCESS", "FAILURE", "RUNNING", etc.
    details: List[str] = []  # Optional list of detailed status messages


@app.get("/rest/v1/ddi/v1/controller/device/{controller_id}")
async def poll_controller(controller_id: str) -> Dict[str, Any]:
    """
    DDI Polling Endpoint - Core of hawkBit's pull-based architecture

    English:
    Implements the "polling" pattern where devices regularly check for updates
    instead of server-side push. Preferred in IoT due to controllability, NAT-friendliness,
    and flexible scheduling on devices.

    한국어:
    서버가 푸시하는 대신, 기기가 주기적으로 업데이트를 확인하는 "폴링" 패턴을 구현합니다.
    이 방식은 다음 이유로 IoT 환경에서 선호됩니다:
    1) 기기가 업데이트 확인 시점을 스스로 제어 가능
    2) NAT/방화벽 환경에서도 포트 포워딩 없이 동작
    3) 기기 자체의 스케줄링 로직을 적용 가능

    URL Structure / URL 구성:
    - /rest/v1/ddi/v1/ : hawkBit DDI API v1 네임스페이스
    - controller/device/ : 컨트롤러 리소스 경로
    - {controller_id} : 기기 고유 식별자

    HTTP Method: GET (멱등 연산)
    Response: 배포 정보가 포함된 JSON

    Args:
        controller_id (str): 기기 식별자 (예: "device001", MAC 주소 등)

    Returns:
        Dict[str, Any]: hawkBit 호환 배포 설명자

    Modern Python features / 현대 파이썬 특징:
    - async/await 비동기 I/O
    - 타입 힌트 기반 정적 분석/문서화
    - f-string을 통한 가독성 높은 문자열 인터폴레이션
    """
    
    # In a real hawkBit server, this would:
    # 1. Query database for pending deployments for this controller_id
    # 2. Check device capabilities and compatibility
    # 3. Return appropriate deployment or empty response
    
    # Simulated deployment response following hawkBit DDI specification
    deployment_response = {
        "deploymentBase": {
            # Unique identifier for this deployment
            "id": "12345",
            
            # Download section contains all artifacts to be downloaded
            "download": {
                "links": {
                    # Named artifact links - "firmware" is a common artifact name
                    "firmware": {
                        # Absolute URL where device can download the file
                        "href": f"http://localhost:8000/files/firmware.bin",
                        
                        # File size in bytes - helps devices validate complete download
                        "size": 1048576  # 1MB = 1024 * 1024 bytes
                    }
                }
            }
        }
    }
    
    print(f"Device {controller_id} polled for updates - returning deployment 12345")
    return deployment_response


@app.get("/files/firmware.bin")
async def download_firmware():
    """
    File Download Endpoint - Serves binary firmware files

    English:
    Demonstrates efficient file serving using FileResponse (streaming) without loading
    entire content into memory.

    한국어:
    대용량 파일을 메모리에 모두 올리지 않고 스트리밍 방식으로 전송하는 예제입니다.
    IoT 환경에서 수 MB~GB의 펌웨어 파일을 다룰 때 필수적입니다.

    Security (prod) / 보안 고려사항(프로덕션):
    - 인증/인가
    - 파일 무결성 검증(체크섬)
    - Rate limiting
    - 업로드 파일 바이러스 스캔

    Returns:
        FileResponse: 적절한 헤더와 함께 스트리밍 전송

    Raises:
        HTTPException: 파일을 찾을 수 없을 때 404 반환
    """
    
    # Relative path to firmware file
    firmware_path = "files/firmware.bin"
    
    # File existence check - important for security and user experience
    if not os.path.exists(firmware_path):
        # HTTPException automatically returns proper HTTP error response
        # FastAPI converts this to JSON error format
        raise HTTPException(
            status_code=404, 
            detail="Firmware file not found"
        )
    
    # FileResponse streams file efficiently without loading into memory
    # This is superior to reading file into bytes for large files
    return FileResponse(
        path=firmware_path,
        filename="firmware.bin",  # Suggested filename for client
        media_type="application/octet-stream",  # Binary data MIME type
        
        # Optional headers for better client handling:
        headers={
            "Content-Disposition": "attachment; filename=firmware.bin",
            "Cache-Control": "no-cache",  # Prevent caching of firmware
            "X-Content-Type-Options": "nosniff"  # Security header
        }
    )


@app.post("/rest/v1/ddi/v1/controller/device/{controller_id}/deploymentBase/{deployment_id}")
async def report_status(
    controller_id: str, 
    deployment_id: str, 
    status_report: StatusReport
) -> Dict[str, str]:
    """
    Status Reporting Endpoint - Receives device feedback

    English:
    Completes the update cycle by receiving device outcomes.
    Useful for monitoring, debugging, auditing, and automation.

    한국어:
    기기의 배포 결과를 수집하여 업데이트 사이클을 완성합니다.
    모니터링, 디버깅, 컴플라이언스, 자동화(롤백 결정 등)에 필수적인 정보입니다.

    URL Path Parameters / 경로 매개변수:
    - controller_id: 배포를 수행한 기기 ID
    - deployment_id: 어떤 배포에 대한 상태인지 식별자

    Request Body / 요청 본문:
    - StatusReport 모델(JSON)로 구조화된 상태 정보 전달

    HTTP Method: POST (새 상태 레코드 생성)

    Args:
        controller_id (str): 기기 식별자
        deployment_id (str): 배포 식별자
        status_report (StatusReport): 상태 상세 정보

    Returns:
        Dict[str, str]: 수신 확인 메시지

    Modern Python patterns / 현대 파이썬 패턴:
    - Pydantic 기반 자동 검증
    - 구조화된 로깅
    - 명확한 반환 타입 주석
    """
    
    # In production, this would:
    # 1. Validate that deployment_id exists and belongs to controller_id
    # 2. Store status in database with timestamp
    # 3. Trigger notifications/webhooks for monitoring systems
    # 4. Update deployment state machine
    # 5. Log structured data for analytics
    
    # Structured logging with contextual information
    print(f"📊 Status Report Received:")
    print(f"   Device: {controller_id}")
    print(f"   Deployment: {deployment_id}")
    print(f"   Status: {status_report.status}")
    print(f"   Time: {status_report.time}")
    if status_report.details:
        print(f"   Details: {', '.join(status_report.details)}")
    
    # Return structured response following REST conventions
    return {
        "message": "Status report received successfully",
        "controller_id": controller_id,
        "deployment_id": deployment_id,
        "received_status": status_report.status
    }


@app.get("/")
async def root() -> Dict[str, str]:
    """
    Health check and API information endpoint

    English:
    Health check and API info for humans and machines.

    한국어:
    서버 상태 확인 및 API 정보를 제공하는 엔드포인트입니다.

    Returns:
        Dict[str, str]: 기본 서버 정보
    """
    return {
        "message": "hawkBit DDI API Mock Server",
        "version": "1.0.0",
        "api_docs": "/docs",
        "status": "running"
    }


# Python application entry point
# This pattern allows the module to be run directly or imported
if __name__ == "__main__":
    """
    Application entry point with uvicorn ASGI server

    English:
    Starts Uvicorn to serve this FastAPI application.

    한국어:
    이 FastAPI 애플리케이션을 Uvicorn으로 실행합니다.

    Configuration explanation / 설정 설명:
    - "main:app" : 이 파일의 app 객체를 지정
    - host="0.0.0.0" : 모든 네트워크 인터페이스 수신
    - port=8000 : 개발 기본 포트
    - reload=True : 코드 변경시 자동 재시작(개발용)
    """
    uvicorn.run(
        "main:app",  # Module:variable format for app discovery
        host="0.0.0.0",  # Bind to all interfaces (allows external connections)
        port=8000,  # Standard HTTP alternate port
        reload=True,  # Development feature: auto-reload on file changes
        log_level="info",  # Logging verbosity
        access_log=True  # Log all HTTP requests
    )