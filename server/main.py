"""
hawkBit DDI (Direct Device Integration) API Mock Server

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
    
    Pydantic provides automatic JSON validation, serialization/deserialization,
    and generates OpenAPI schemas. This model represents the data structure
    that devices send when reporting their update status.
    
    Modern Python features demonstrated:
    - Type annotations for runtime validation
    - Default values with proper typing
    - Automatic JSON schema generation
    """
    id: str  # Deployment ID that this status report refers to
    time: str  # Timestamp when the status was recorded (ISO format recommended)
    status: str  # Status value: "SUCCESS", "FAILURE", "RUNNING", etc.
    details: List[str] = []  # Optional list of detailed status messages


@app.get("/rest/v1/ddi/v1/controller/device/{controller_id}")
async def poll_controller(controller_id: str) -> Dict[str, Any]:
    """
    DDI Polling Endpoint - Core of hawkBit's pull-based architecture
    
    This endpoint implements the "polling" pattern where devices regularly
    check for available updates rather than the server pushing updates.
    This approach is preferred in IoT because:
    1. Devices can control when they check for updates
    2. Works behind NAT/firewalls without port forwarding
    3. Devices can implement their own scheduling logic
    
    URL Structure:
    - /rest/v1/ddi/v1/ = hawkBit DDI API version 1 namespace
    - controller/device/ = Resource path for device controllers
    - {controller_id} = Unique identifier for each device
    
    HTTP Method: GET (idempotent operation)
    Response: JSON containing deployment information
    
    Args:
        controller_id (str): Unique device identifier (e.g., "device001", MAC address)
        
    Returns:
        Dict[str, Any]: hawkBit-compatible deployment descriptor
        
    Modern Python features:
    - async/await for non-blocking I/O operations
    - Type hints with Union types for complex return types
    - f-string formatting for readable string interpolation
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
    
    This endpoint demonstrates efficient file serving in FastAPI using
    FileResponse for streaming large files without loading them entirely
    into memory. This is crucial for IoT scenarios where firmware files
    can be large (several MB to GB).
    
    Security considerations in production:
    - Authentication/authorization required
    - File integrity verification (checksums)
    - Rate limiting to prevent abuse
    - Virus scanning for uploaded files
    
    Returns:
        FileResponse: Streamed binary file with appropriate headers
        
    Raises:
        HTTPException: 404 if file not found
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
    
    This endpoint completes the update cycle by allowing devices to report
    the outcome of deployment attempts. This feedback is crucial for:
    1. Monitoring deployment success rates
    2. Debugging failed deployments
    3. Compliance and audit trails
    4. Automated rollback decisions
    
    URL Path Parameters:
    - controller_id: Device that performed the deployment
    - deployment_id: Which deployment this status refers to
    
    Request Body: StatusReport model with structured status information
    
    HTTP Method: POST (creates new status record)
    
    Args:
        controller_id (str): Device identifier
        deployment_id (str): Deployment identifier  
        status_report (StatusReport): Pydantic model with status details
        
    Returns:
        Dict[str, str]: Acknowledgment message
        
    Modern Python patterns:
    - Pydantic model automatic validation
    - Structured logging for observability
    - Clear return type annotations
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
    
    This endpoint serves as:
    1. Health check for monitoring systems
    2. API discovery for clients
    3. Human-readable API information
    
    Returns:
        Dict[str, str]: Basic server information
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
    
    Uvicorn is a high-performance ASGI server that supports:
    - Async/await Python code
    - WebSocket connections
    - HTTP/2 support
    - Automatic reloading during development
    - Production-ready performance
    
    Configuration explanation:
    - "main:app" = Import 'app' from 'main' module (this file)
    - host="0.0.0.0" = Listen on all network interfaces
    - port=8000 = Standard development port
    - reload=True = Auto-restart on code changes (development only)
    """
    uvicorn.run(
        "main:app",  # Module:variable format for app discovery
        host="0.0.0.0",  # Bind to all interfaces (allows external connections)
        port=8000,  # Standard HTTP alternate port
        reload=True,  # Development feature: auto-reload on file changes
        log_level="info",  # Logging verbosity
        access_log=True  # Log all HTTP requests
    )