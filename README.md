# hawkBit DDI API 예제 프로젝트

Eclipse hawkBit의 DDI(Direct Device Integration) API를 모방한 서버-클라이언트 통신 예제입니다.

## 프로젝트 구조

```
├── docs/
│   └── PRD.md              # 제품 요구사항 문서
├── server/                 # Python FastAPI 서버
│   ├── pyproject.toml
│   ├── main.py
│   └── files/
│       └── firmware.bin    # 1MB 더미 파일
└── client/                 # C++ 클라이언트
    ├── CMakeLists.txt
    ├── build.sh
    ├── include/
    │   ├── hawkbit_client.h
    │   └── http_client.h
    └── src/
        ├── main.cpp
        ├── hawkbit_client.cpp
        └── http_client.cpp
```

## 빠른 실행 가이드

### 1단계: 서버 실행
```bash
cd server
uv sync
uv run main.py
```

### 2단계: 클라이언트 빌드 (새 터미널)
```bash
# 의존성 설치 (최초 1회만)
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev pkg-config

# 빌드
cd client
./build.sh
```

### 3단계: 클라이언트 실행
```bash
./build/client http://localhost:8000 device001
```

## 상세 실행 방법

### 서버 실행

#### 의존성 설치 및 서버 시작
```bash
cd server
uv sync
uv run main.py
```

#### 기존 방식 (대안)
```bash
cd server
uv venv && source .venv/bin/activate
uv pip install fastapi uvicorn
uvicorn main:app --reload --host 0.0.0.0 --port 8000
```

서버는 http://localhost:8000 에서 실행됩니다.

### 클라이언트 빌드 및 실행

#### 의존성 설치
Ubuntu/Debian:
```bash
sudo apt-get install -y build-essential cmake libcurl4-openssl-dev pkg-config
```

#### 빌드
```bash
cd client
./build.sh
```

#### 실행
```bash
./build/client [server_url] [controller_id]
```

예시:
```bash
./build/client http://localhost:8000 device001
```

## API 엔드포인트

### 서버 API

- `GET /` - 서버 상태 확인
- `GET /rest/v1/ddi/v1/controller/device/{controller_id}` - 업데이트 폴링
- `GET /files/firmware.bin` - 펌웨어 파일 다운로드
- `POST /rest/v1/ddi/v1/controller/device/{controller_id}/deploymentBase/{deployment_id}` - 상태 보고

### 동작 흐름

1. 클라이언트가 서버에 주기적으로 폴링 (10초 간격)
2. 서버가 업데이트 정보를 JSON으로 응답
3. 클라이언트가 펌웨어 파일 다운로드
4. 클라이언트가 다운로드 결과를 서버에 보고

## 테스트 방법

### 수동 서버 테스트
```bash
# 폴링 테스트
curl http://localhost:8000/rest/v1/ddi/v1/controller/device/test123

# 파일 다운로드 테스트
curl -O http://localhost:8000/files/firmware.bin

# 상태 보고 테스트
curl -X POST http://localhost:8000/rest/v1/ddi/v1/controller/device/test123/deploymentBase/12345 \
  -H "Content-Type: application/json" \
  -d '{"id":"12345","time":"2024-01-01 12:00:00","status":"SUCCESS","details":[]}'
```

### 통합 테스트

1. **터미널 1**: 서버 실행
   ```bash
   cd server && uv run main.py
   ```

2. **터미널 2**: 클라이언트 실행
   ```bash
   cd client && ./build/client http://localhost:8000 device001
   ```

3. **예상 결과**: 
   - 클라이언트가 10초마다 서버에 폴링
   - 1MB 펌웨어 파일 다운로드
   - 성공/실패 상태를 서버에 보고

## 문제 해결

### 서버 문제
- **포트 8000 사용 중**: `lsof -ti:8000 | xargs kill -9` 또는 다른 포트 사용
- **의존성 설치 실패**: `uv` 설치 확인 또는 `pip` 사용

### 클라이언트 문제  
- **빌드 실패**: libcurl 개발 라이브러리 설치 확인
- **연결 실패**: 서버 실행 상태 및 URL 확인