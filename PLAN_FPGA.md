# Project: SPC-10000 Hardware Implementation on AMD Zynq UltraScale+ ZCU102

## 1. Overview & Objectives
본 문서는 소프트웨어로 검증된 `SPC-1000 / SPC-10000` 시스템을 **AMD Xilinx ZCU102 Evaluation Board (XCZU9EG-2FFVB1156E)** 위에 실제 동작하는 하드웨어(SoC/FPGA)로 구현하기 위한 아키텍처 및 작업 계획서이다.

- **타겟 보드:** AMD Zynq UltraScale+ MPSoC ZCU102
- **개발 툴체인:** Vivado, Vitis (또는 PetaLinux), Vitis HLS
- **설계 철학:** 
  - **PL (Programmable Logic):** 사이클 정확도(Cycle-accurate)를 요구하는 CPU(Z8001, Z80A), 메모리 버스, 비디오(MC6847+), 사운드(AY-3-8910)를 하드웨어 로직으로 구현.
  - **PS (Processing System):** 현대적인 주변기기(DisplayPort 모니터 출력, USB HID 키보드, SD카드 파일시스템, UART 콘솔 제어)를 처리하고 AXI 버스를 통해 PL과 연동.

---

## 2. Directory Structure Extension (D:\work\spc-1000\fpga-zcu102)

    spc-1000/
    ├── PLAN.md                     # 소프트웨어 에뮬레이터 & ROM 역공학 마스터 플랜
    ├── PLAN_FPGA.md                # ZCU102 FPGA 하드웨어 구현 플랜 (본 문서)
    └── fpga-zcu102/                # FPGA 및 SoC 관련 워크스페이스
        ├── Makefile                # Vivado/Vitis 배치 모드(Tcl) 빌드 자동화
        ├── scripts/
        │   ├── create_project.tcl  # ZCU102 Vivado 프로젝트 및 Block Design 자동 생성
        │   └── build_bitstream.tcl # Synthesis, Implementation, XSA 추출 스크립트
        ├── rtl/                    # 순수 Verilog / SystemVerilog / VHDL 소스
        │   ├── top_spc10000.sv     # PL 최상위 래퍼 모듈
        │   ├── cpu_z80/            # T80 오픈소스 코어 (Sub/Legacy CPU)
        │   ├── cpu_z8000/          # Z8001/Z8002 16-bit Soft Core (RTL 또는 HLS 추출물)
        │   ├── video/              # MC6847 VDG + AXI4-Stream Video Out 변환기
        │   ├── sound/              # JT49 (AY-3-8910) PSG 코어
        │   └── bus/                # Z8000/Z80 버스 중재기 및 AXI-Lite 슬레이브 브릿지
        ├── hls/                    # Vitis HLS 프로젝트 (C++ Z8000 코어 -> RTL IP 변환용)
        ├── constraints/
        │   └── zcu102_spc10000.xdc # 클럭, GPIO LED, DIP 스위치(SW12), PMOD 핀 제약 조건
        └── fw_ps/                  # PS(ARM Cortex-A53) 구동용 펌웨어/데몬
            ├── hw_bridge_daemon/   # USB 키보드 -> AXI 매트릭스 주입 및 SD카드 ROM/TAP 로더
            └── debug_shell/        # UART 시리얼 콘솔 기반 실시간 하드웨어 모니터/디버거

---

## 3. ZCU102 PS-PL System Architecture

### 3.1. Clock & Reset Architecture
- **PL Fabric Clock (`pl_clk0`):** 100MHz 마스터 클럭 생성.
- **Clock Enable (CE) 분주기:** 물리적 클럭 도메인은 100MHz 단일 도메인으로 유지하되, Clock Enable 스트로브를 사용하여 타이밍 클로저(Timing Closure) 문제 원천 차단:
  - `ce_4mhz`: 오리지널 SPC-1000 (Z80A) 및 카세트 타이밍용
  - `ce_3m58`: MC6847 비디오 및 AY-3-8910 사운드용 (3.579545MHz 근사 분주)
  - `ce_z8000`: 10MHz / 25MHz / 50MHz 가변 스트로브 (보드 `SW12` DIP 스위치 또는 AXI 레지스터로 실시간 터보 배속 제어)

### 3.2. True Dual-Port BRAM / URAM Memory Map
FPGA 재합성 없이 펌웨어(ROM)와 프로그램(RAM)을 즉시 교체하기 위해 모든 시스템 메모리는 **True Dual-Port BRAM/URAM**으로 구성한다.
- **Port A (PL Side):** Z8001 (최대 8MB 세그먼트 공간 중 초기 1MB URAM 할당) 및 Z80A (64KB BRAM) 로컬 버스에 직결.
- **Port B (PS Side):** `AXI BRAM Controller`를 통해 PS 메모리 맵(`0xA000_0000` 대역)에 노출. PS에서 리눅스 `mmap` 또는 베어메탈 포인터로 즉시 ROM/RAM 읽기·쓰기 및 상태 덤프(Savestate) 가능.

### 3.3. Peripheral & I/O Bridging (AXI4-Lite Slave)
PS가 현대적인 입출력을 받아 PL 내부의 SPC-10000 하드웨어 레지스터에 꽂아주는 브릿지 구조 (`0xA001_0000` 대역):
- `REG_SYS_CTRL (0x00)`: Z80/Z8000 CPU Reset, Halt, Single-Step, Turbo Clock 선택
- `REG_KBD_MATRIX (0x10 ~ 0x1C)`: 10바이트 키보드 매트릭스 상태 레지스터. PS가 USB 키보드 이벤트를 읽어 해당 비트를 Clear/Set 하면, PL의 CPU가 `0x8000~0x8009`를 읽을 때 즉시 반영됨.
- `REG_TAPE_CTRL (0x20)`: 카세트 모터 상태 감지 및 FSK 펄스 스트림 FIFO 버퍼.

### 3.4. Video & Audio Output Pipeline (ZCU102 DisplayPort)
- PL의 `mc6847_vdg` 모듈이 Dual-Port VRAM을 읽어 픽셀 데이터를 생성하고, 이를 업스케일러(Line Doubler/Scaler)를 거쳐 **1080p 또는 720p RGB `AXI4-Stream` 신호**로 변환.
- Zynq UltraScale+ PS의 **DisplayPort Controller (`dp_live_video_in`)** 포트에 연결하여 별도의 외부 컨버터 없이 ZCU102 온보드 DP 포트로 선명한 화면과 사운드 출력.

---

## 4. Phased Implementation Roadmap

### Step 1: Z8000 하드웨어 IP 확보 (Two-Track Approach)
- **Track A (Fast Bring-up via Vitis HLS):** 소프트웨어 에뮬레이터에서 검증된 C++ Z8000 코어를 정적 배열 및 AXI-Master/BRAM 인터페이스 함수로 다듬어 Vitis HLS를 통해 Verilog IP로 합성.
- **Track B (Pure RTL Design):** Z8001/Z8002 명령어 셋을 처리하는 클린룸 SystemVerilog 상태 머신 및 16비트 ALU, 16x16비트 레지스터 파일(`R0`~`R15`) 직접 설계. 소프트웨어 에뮬레이터와 Co-simulation(Verilator)으로 명령어 단위 검증.

### Step 2: PL 서브시스템 통합 및 Verilator 시뮬레이션
1. `T80`(Z80 코어), `Z8000` 코어, `MC6847` 비디오, `JT49` 사운드, Dual-Port BRAM을 엮어 `top_spc10000.sv` 작성.
2. Vivado 합성 전, PC에서 **Verilator**를 이용해 RTL 코드를 C++ 테스트벤치로 컴파일한 뒤 소프트웨어 에뮬레이터와 동일한 ROM을 돌려 부팅 화면이 프레임버퍼에 찍히는지 초고속 검증.

### Step 3: Vivado Block Design (Tcl 자동화) 및 타이밍 검증
1. `scripts/create_project.tcl`을 작성하여 ZCU102 보드 프리셋 로드, Zynq UltraScale+ MPSoC IP 설정(DisplayPort Live Video 활성화, AXI HPM0 FPD 활성화).
2. `top_spc10000` RTL 모듈과 AXI BRAM Controller, Video Timing Controller(VTC) 연결.
3. 주요 버스 신호(Z8000 Address, Data, Status, MREQ, IORQ)에 **System ILA** 프로브 연결.
4. 비트스트림 생성 및 `.xsa` 하드웨어 플랫폼 파일 추출.

### Step 4: ZCU102 보드 브링업 및 PS 브릿지 데몬 구동
1. ZCU102 부팅 후 PS 단에서 `hw_bridge_daemon` 실행:
   - CPU Reset Hold -> SD카드에서 `spc10000_rom.bin`을 읽어 AXI BRAM(`Port B`)에 적재 -> CPU Reset Release.
2. DisplayPort 모니터로 `Super Hu-BASIC` 부팅 화면 확인 및 USB 키보드 입력 테스트.
3. UART 시리얼 콘솔을 통해 실시간으로 Z8000 레지스터 상태 조회, 브레이크포인트 제어, 테이프 이미지(`.tap`) 즉시 메모리 인젝션(Fast-Load) 기능 검증.

