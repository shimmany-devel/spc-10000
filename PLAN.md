# Project: SPC-1000 Modernization & SPC-10000 (Z8000 16-bit Evolution)

## 1. Overview & Objectives
본 프로젝트는 삼성전자의 8비트 퍼스널 컴퓨터 **SPC-1000 (Z80A 기반)**의 에뮬레이터를 현대적인 구조로 재구축하고, 나아가 Zilog의 16비트 아키텍처인 **Z8000 (Z8001/Z8002)**으로 하드웨어와 시스템 ROM을 고도화한 가상의 대체 역사 컴퓨터 **SPC-10000**을 구현하는 것을 목표로 한다.

- **핵심 도구:** Antigravity IDE + `morluto/rea` (Reverse Engineer Anything MCP, Ghidra/Rizin 백엔드 연동)
- **개발 언어 및 환경:** C/C++ (C++20), SDL2 (또는 WebAssembly 확장 고려), CMake / Makefile
- **크로스 툴체인:** Z80 (`z88dk` / `sdcc`), Z8000 (`z8k-coff-gcc` / GNU Binutils `z8k`)

---

## 2. Target Directory Structure (D:\work\spc-1000)

    spc-1000/
    ├── PLAN.md                 # 마스터 아키텍처 및 작업 계획서 (본 문서)
    ├── CMakeLists.txt          # 통합 빌드 스크립트
    ├── Makefile                # 빠른 빌드 및 자동화 래퍼
    ├── reference/              # 레퍼런스 오픈소스 및 원본 바이너리 보관
    │   ├── roms/               # SPC-1000 원본 32KB IPL/BASIC ROM (spc1000.rom)
    │   ├── meeso-spc1000/      # 기존 SPC-1000 에뮬레이터 레퍼런스 코드
    │   └── mame-z8000/         # MAME에서 추출한 Z8000 CPU 코어 레퍼런스
    ├── rea-workspace/          # morluto/rea 역공학 분석 결과물
    │   ├── z80-rom-decompiled/ # SPC-1000 ROM의 C 언어 재구성 소스
    │   └── memory-io-map.md    # 역공학으로 도출된 하드웨어 I/O 및 타이밍 명세
    ├── src/
    │   ├── core/
    │   │   ├── z80/            # Z80 CPU 코어 (SPC-1000 호환 및 서브 I/O 프로세서용)
    │   │   ├── z8000/          # Z8001/Z8002 16-bit 메인 CPU 코어 (SPC-10000용)
    │   │   └── mmu/            # Z8010 MMU 및 뱅크 스위칭/세그먼트 메모리 컨트롤러
    │   ├── hw/
    │   │   ├── vdg_mc6847.cpp  # 비디오 디스플레이 제너레이터 (확장 고해상도 모드 포함)
    │   │   ├── psg_ay8910.cpp  # 사운드 칩셋 에뮬레이션
    │   │   ├── keyboard.cpp    # 키보드 매트릭스 (0x8000 I/O 매핑)
    │   │   └── cassette.cpp    # 카세트 테이프 I/O 및 Fast-Load 지원
    │   ├── sys_spc1000/        # Phase 1: 고도화된 SPC-1000 에뮬레이터 시스템 통합
    │   ├── sys_spc10000/       # Phase 3: Z8000 기반 SPC-10000 시스템 통합
    │   └── ui/                 # SDL2 렌더링, 내장 디버거, 상태 저장(State Save) UI
    └── firmware-spc10000/      # Phase 4: Z8000 네이티브로 포팅된 Super Hu-BASIC 및 Monitor ROM

---

## 3. Hardware Architecture Specification

### 3.1. Base System: SPC-1000 (8-bit Z80A)
- **CPU:** Zilog Z80A @ 4MHz (Little-Endian)
- **Memory:** 32KB IPL/Hu-BASIC ROM, 64KB RAM (Bank switching)
- **Video:** Motorola MC6847 VDG (6KB VRAM, Text & Semigraphics, 256x192 / 128x192)
- **Sound:** General Instrument AY-3-8910 PSG (3채널 사각형파 + 노이즈)
- **I/O Map:**
  - `0x0000 - 0x1FFF`: VRAM (6KB + Attribute)
  - `0x2000 - 0x3FFF`: VDG Mode Control
  - `0x4000 - 0x5FFF`: PSG Data / Register Control
  - `0x6000 - 0x7FFF`: Cassette Tape Control & Motor/Pulse I/O
  - `0x8000 - 0x9FFF`: Keyboard Matrix Scan (`0x8000` ~ `0x8009`)
  - `0xA000 - 0xBFFF`: IPL/RAM Bank Switching Flip-Flop

### 3.2. Target Evolution: SPC-10000 (16-bit Z8000)
- **CPU:** Zilog Z8001 (Segmented, 최대 8MB 주소 공간) 또는 Z8002 (Non-segmented 64KB 호환 모드) @ 10MHz+
  - 16개의 16비트 범용 레지스터 (`R0`~`R15`), 32비트(`RR`), 64비트(`RQ`) 페어 연산 활용
  - **System Mode / Normal Mode** 분리 및 전용 시스템 스택 포인터(`R15'`) 활용
  - **Endianness:** Big-Endian (기존 Z80 데이터 구조체 접근 시 16비트 워드 바이트 스왑 레이어 고려)
- **Architecture Option (Dual-Processor Co-existence):**
  - **Main CPU (Z8001):** 16비트 네이티브 OS/BASIC 구동 및 고속 메모리/그래픽 제어
  - **Sub CPU (Z80A):** 사운드/키보드/카세트 I/O 제어 및 레거시 SPC-1000 바이너리 100% 하위 호환 실행
- **Enhanced Video & Features:**
  - MC6847 레거시 모드 완벽 호환 + 세그먼트 메모리를 활용한 확장 슈퍼 그래픽 모드 (256컬러 프레임버퍼)
  - 호스트 파일시스템 직결 가상 디스크(`DISK` I/O) 및 실시간 상태 저장(Savestate), 내장 Z80/Z8000 디스어셈블러/디버거 탑재

---

## 4. Phased Execution Plan (For AI Agent)

### Phase 1: 프로젝트 스캐폴딩 및 오픈소스 레퍼런스 확보
1. Git 저장소 초기화 및 기본 디렉토리 트리 생성.
2. 레퍼런스 코드 클론 및 분석:
   - `meeso/spc1000` (또는 관련 오픈소스)에서 SPC-1000 I/O 매핑 및 타이밍 로직 확보.
   - `floooh/chips`에서 단일 헤더 `mc6847.h`, `ay38910.h` 확보.
   - `mamedev/mame` (`src/devices/cpu/z8000/`)에서 Z8000 CPU 에뮬레이터 코어 파일 추출.
3. `npx rea-agents@latest setup`을 통해 워크스페이스에 `morluto/rea` MCP 설정 완료.

### Phase 2: morluto/rea 기반 SPC-1000 ROM 역공학 및 에뮬레이터 재구축
1. `rea` (Ghidra/Rizin 백엔드)에 SPC-1000 32KB ROM 바이너리를 로드.
2. ROM 내부의 핵심 루틴 식별 및 C 코드로 디컴파일(`rea-workspace/z80-rom-decompiled/`):
   - 부트스트랩 및 하드웨어 초기화 시퀀스
   - 키보드 매트릭스 스캔 및 디바운싱 루틴
   - 카세트 테이프 FSK 펄스 리드/라이트 타이밍 루틴
   - Hu-BASIC 인터프리터 코어 루프 및 플로팅 포인트 연산기
3. 분석된 하드웨어 명세를 바탕으로 깔끔하게 모듈화된 **SPC-1000 베이스 에뮬레이터** 구현 및 커스텀 기능(배속, 강제 세이브, 테이프 고속 로딩, CRT 필터, 메모리 모니터) 추가.

### Phase 3: Z8000 코어 이식 및 SPC-10000 가상 하드웨어 구축
1. MAME에서 발라낸 Z8000 C++ 코어의 외부 매크로 의존성을 제거하여 독립적인 `src/core/z8000` 모듈로 리팩토링.
2. Z8001 세그먼트 메모리 버스 및 I/O 버스(`IN`/`OUT` 및 Special I/O 명령어)를 SPC-1000의 주변기기 컨트롤러와 브릿징.
3. Big-Endian(Z8000)과 Little-Endian(기존 주변기기/데이터) 간의 버스 인터페이스 정의.

### Phase 4: SPC-10000 전용 펌웨어 (Z8000 Native ROM) 포팅
1. Docker 또는 WSL 환경에 `z8k-coff` / `z8k-elf` 크로스 컴파일러 툴체인 구축.
2. Phase 2에서 `rea`로 추출한 SPC-1000 ROM의 C 소스를 Z8000 타겟에 맞게 리팩토링:
   - Z80 레지스터 종속 코드 제거 및 포인터/엔디안 정규화
   - Z8000 System/Normal 모드 초기화 부트로더(Startup assembly `crt0.s`) 작성
3. 크로스 컴파일된 Z8000 네이티브 ROM을 SPC-10000 에뮬레이터에 올려 부팅 및 `Super Hu-BASIC` 프롬프트 진입 검증.
