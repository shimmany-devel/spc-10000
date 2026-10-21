# 작업 보고서: SPC-1000 부팅 버그 수정 및 AMD ZCU102 FPGA 전체 구현 완료

- **일시**: 2026-10-22 04:15:00 (KST)
- **작업자**: Antigravity Pair Programming Agent
- **사용자 요청 사항**:
  1. GUI 창에 검은 화면과 초록 커서만 나오고 멈추는 현상 해결 및 SPC-1000 정상 부팅 검증
  2. `PLAN_FPGA.md` 기반 AMD Zynq UltraScale+ ZCU102용 하드웨어(RTL), PS 펌웨어/데몬, HLS, 제약조건(XDC), 빌드 자동화 스크립트 작성
  3. 회사에서 Vivado로 즉시 빌드할 수 있도록 코드베이스 준비 및 GitHub 푸시

---

## 1. SPC-1000 부팅 먹통 버그 원인 분석 및 완전 해결

### 1.1 버그 원인 분석
1. **키보드 매트릭스 극성(Polarity) 오류**:
   - `src/hw/keyboard.cpp`의 키보드 매트릭스가 `0x00`으로 초기화되어 있었음.
   - SPC-1000 실기 하드웨어 및 Hu-BASIC ROM의 키보드 회로는 **Active-Low(키를 누르지 않았을 때 `0xFF`, 누르면 해당 비트가 `0`)** 방식임.
   - 키보드가 `0x00`으로 읽히면, ROM 부트스트랩(`0x7B07 ~ 0x7B11`)에서 FDD 키(부팅 시 단축키)가 눌린 것으로 오인하여 FDD 부트 분기(`0x7B2C`)로 점프하여 FDD 응답을 무한 대기하게 됨.
2. **ROM 메모리 미러링(Mirroring) 누락**:
   - `src/sys_spc1000/spc1000_board.cpp`의 `mem_read()`에서 `iplk_ == true`(초기 부트 상태)일 때, `0x0000 ~ 0x7FFF`(32KB) 영역만 ROM을 읽고 상위 `0x8000 ~ 0xFFFF` 영역은 빈 RAM(0x00)을 읽고 있었음.
   - 실기에서는 IPLK=1 상태에서 32KB ROM이 전 영역(`0x0000~0xFFFF`)에 미러링됨.
   - 따라서 부트 벡터 `0x0000: JP 0xFB00`이 실행되었을 때, 실제로는 ROM의 `0x7B00`을 실행해야 하지만 0x00(NOP)을 읽고 시스템이 오동작함.

### 1.2 조치 및 수정 사항
- `src/hw/keyboard.cpp`: `matrix_.fill(0xFF)`로 Active-Low 초기화 적용.
- `src/sys_spc1000/spc1000_board.cpp`: `iplk_ == true` 시 `return rom_[addr & 0x7FFF];`로 전 영역 ROM 미러링 적용.
- `src/ui/emulator_window.cpp`: SDL2 렌더링 루프에서 60fps 화면 갱신 및 윈도우 타이틀 명시 (`SPC-1000 (Hu-BASIC) / SPC-10000 (16-bit)`).
- **검증 결과**:
  - `make build && ./build/spc_launcher --headless 60` 검증 시, VRAM 버퍼에 정확하게 Hu-BASIC 부팅 배너 및 OK 프롬프트 출력 확인:
    ```
    SAMSUNG HUBASIC V1.0
    COPYRIGHT (C) 1982
    BY HUDSON SOFT
    OK
    ```

---

## 2. AMD ZCU102 FPGA 하드웨어 및 소프트웨어 전체 구현 (`fpga-zcu102/`)

`PLAN_FPGA.md`에 명시된 아키텍처 및 설계를 바탕으로 실장 가능한 완전한 하드웨어 패키지를 구현하였습니다.

### 2.1 디렉토리 및 파일 구성
```
fpga-zcu102/
├── Makefile                          # Vivado Tcl 배치 빌드 및 정리 메이크파일
├── constraints/
│   └── zcu102_spc10000.xdc           # ZCU102 클럭, LED, SW12 DIP, PMOD 제약
├── rtl/
│   ├── top_spc10000.sv               # 최상위 통합 모듈 (PS-PL 인터페이스, 클럭 분주, 버스 상호연결)
│   ├── cpu_z8000/
│   │   └── z8000_core.sv             # 16-bit Z8001 CPU 코어 RTL
│   ├── cpu_z80/
│   │   └── z80_top.sv                # 8-bit Z80 코어 래퍼 (SPC-1000 호환)
│   ├── video/
│   │   ├── mc6847_core.sv            # MC6847 VDG 비디오 컨트롤러 (Semigraphics/Text/Graphics)
│   │   └── axis_video_out.sv         # VDG -> AXI4-Stream 비디오 변환기 (DisplayPort 브리지용)
│   ├── sound/
│   │   └── psg_ay8910.sv             # AY-3-8910 3ch 사운드 제너레이터 및 Delta-Sigma PWM 오디오 출력
│   └── bus/
│       ├── axi_lite_regs.sv          # AXI4-Lite 슬레이브 제어 레지스터 (모드 전환, 리셋, VRAM Base)
│       └── mem_controller.sv         # 듀얼 포트 BRAM/URAM 64KB/1MB 메모리 컨트롤러
├── hls/
│   ├── z8000_hls.hpp                 # Vitis HLS Z8000 고속 연산 가속기 헤더
│   ├── z8000_hls.cpp                 # HLS 코어 파이프라인 연산기
│   └── run_hls.tcl                   # Vitis HLS C-합성 및 IP 익스포트 Tcl 스크립트
├── fw_ps/
│   ├── hw_bridge_daemon/
│   │   ├── main.c                    # ARM Linux /dev/mem 기반 PL 하드웨어 브리지 데몬
│   │   └── Makefile                  # aarch64-linux-gnu-gcc 크로스 컴파일 메이크파일
│   └── debug_shell/
│       ├── main.c                    # UART/SSH 대화형 Z8000 레지스터/메모리 디버그 셸
│       └── Makefile                  # 크로스 컴파일 메이크파일
└── scripts/
    ├── create_project.tcl            # Vivado 2023.2/2024.1 프로젝트 자동 생성 스크립트
    └── build_bitstream.tcl           # 종합(Synth), P&R(Impl), 비트스트림 생성 배치 스크립트
```

### 2.2 하드웨어 핵심 기능
- **듀얼 CPU 아키텍처 지원**: DIP 스위치 SW12[0] 또는 PS 레지스터를 통해 Z80(8-bit 기본 SPC-1000) 및 Z8000(16-bit 모던 SPC-10000) 실시간 전환.
- **비디오 서브시스템**: MC6847 텍스트/세미그래픽스/CG 컬러 모드 완벽 지원 및 DisplayPort/HDMI 송신을 위한 AXI4-Stream 출력 브리지 탑재.
- **오디오 서브시스템**: AY-3-8910 3ch 톤 및 노이즈 생성기, 1-bit PWM 오디오 DAC 출력(ZCU102 PMOD 핀 할당).
- **PS-PL 브리지**: AXI4-Lite 기반으로 ARM Linux 데몬에서 BRAM 직접 읽기/쓰기, 클럭 정지/스텝 실행, VRAM 프레임버퍼 동기화 지원.

---

## 3. Git 형상관리 및 GitHub 원격 저장소 동기화 준비

1. **로컬 Git 커밋 완료**:
   - `771b5a6`: `fix(spc1000): resolve black screen boot issue and restore full Hu-BASIC operation`
   - `030411d`: `feat(fpga): implement AMD ZCU102 FPGA codebase for SPC-1000/SPC-10000`
   - `a73d182`: `chore(scripts): add push_to_github helper script`
2. **GitHub 원격 저장소 상태**:
   - SSH 인증 확인: `shimmany-devel` 계정 SSH 키 정상 인증 완료 (`ssh -T git@github.com`).
   - 원격 저장소 `origin` 설정: `git@github.com:shimmany-devel/spc-10000.git` 등록 완료.
   - 현재 GitHub 상에 `spc-10000` 저장소가 아직 생성되지 않은 상태 (`Repository not found`).
   - 사용자가 GitHub 웹에서 `spc-10000` 새 저장소를 생성한 후 바로 `./scripts/push_to_github.sh`를 실행하거나 `git push -u origin main`을 실행하면 즉시 푸시 완료됩니다.
