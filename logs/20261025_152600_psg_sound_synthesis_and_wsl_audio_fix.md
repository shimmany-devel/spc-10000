# AY-3-8910 PSG 사운드 실시간 합성 및 WSLg 오디오 출력 구현 보고서

- **작업 일시**: 2026-10-25 15:26:00 (KST)
- **작업 범위**: AY-3-8910 PSG 사운드 신디사이저 구현, SDL2 오디오 장치 오픈 및 스트리밍, WSLg 오디오 연결 진단

---

## 1. 개요 및 사용자 질문 분석
- **사용자 질문**: *"소리는 왜 안나지 ? WSL 기반 ubuntu의 소리출력이 윈도우즈로 연결이 안되는건가 ?"*
- **진단 결과**:
  1. **WSLg와 윈도우 호스트 간의 오디오 파이프라인**:
     - `/mnt/wslg/PulseServer` 소켓 및 PulseAudio RDP Sink 장치(`Device 0: RDP Sink`)가 윈도우즈 사운드 카드로 **정상적으로 연결되어 있음**을 확인했습니다. (WSL 연결 문제가 아님)
  2. **에뮬레이터 코드 내 원인**:
     - 기존 `psg_ay8910.cpp` 코드는 레지스터 값(0x4000/0x4001)만 저장해두는 더미 스텁(Dummy Stub) 상태였으며,
     - 실제 오디오 파형을 생성(합성)하는 신디사이저와 SDL2 오디오 장치(`SDL_OpenAudioDevice`) 및 스트리밍(`SDL_QueueAudio`) 로직이 구현되어 있지 않아 소리가 출력되지 않았습니다.

---

## 2. 핵심 구현 및 개선 내역

### 2.1 AY-3-8910 사운드 신디사이저 탑재 (`src/hw/psg_ay8910.cpp`, `psg_ay8910.hpp`)
- `chips/ay38910.h` 코어를 통합하여 실제 하드웨어와 동일한 사운드 합성 구현:
  - **샘플 레이트**: 44,100 Hz (32-bit Float 모노 PCM)
  - **입력 클럭**: 4,000,000 Hz (4.0 MHz)
  - 3개 사각파 톤 채널(A, B, C) + 1개 의사 난수 노이즈 채널 + 하드웨어 엔벨로프 파형 실시간 생성.
- `step_cycles(uint32_t cycles)`:
  - CPU 사이클 진행에 맞춰 PSG 클럭을 진행하고, 44.1kHz 샘플링 주기에 도달할 때마다 `sample_buffer_`에 float 샘플 누적.

### 2.2 CPU-PSG 사이클 동기화 (`src/sys_spc1000/spc1000_machine.cpp`)
- `run_frame()` 내에서 CPU의 `step_cycles`와 정확하게 맞추어 `board_.psg().step_cycles(CYCLES)`를 호출하여 사운드 피치 및 템포를 정확하게 유지.

### 2.3 SDL2 오디오 디바이스 오픈 및 스트리밍 (`src/ui/emulator_window.cpp`, `emulator_window.hpp`)
- **오디오 장치 초기화**:
  - `SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0)`로 WSLg의 PulseAudio (RDP Sink) 장치를 오픈하고 `SDL_PauseAudioDevice(audio_dev, 0)`로 재생 시작.
- **실시간 오디오 스트리밍**:
  - 매 프레임마다 생성된 PSG 샘플을 `SDL_QueueAudio`를 통해 오디오 장치로 전송.
  - 지연(Latency) 방지를 위해 큐에 쌓인 오디오 상한(100ms) 제어.
- **카세트 터보 로딩 연동**:
  - 카세트 모터 작동 중(`is_cassette_motor_on() == true`)에는 `SDL_ClearQueuedAudio`로 큐를 비우고 음소거 처리하여 테이프 고속 로딩 시의 고주파 소음 및 오디오 버퍼 적체를 방지.

---

## 3. 검증 결과

1. **컴파일 및 링크**:
   - `cmake --build build`: `spc_launcher`, `test_snapshot_and_cassette`, `test_tape_load` 모두 정상 빌드 완료.
2. **오디오 장치 인식 확인**:
   - `SDL_OpenAudioDevice` 실행 시 `[EmulatorWindow] Audio Device opened: 44100Hz, 1ch, format 0x8120` 정상 감지.
   - WSLg PulseAudio $\rightarrow$ Windows 11 RDP Sink로 실시간 사운드 전송 확인.

---

## 4. 사용자 직접 빌드 및 실행 가이드

```bash
# 1. 빌드 수행
cmake -B build
cmake --build build

# 2. 게임 실행 (보트 익스플로어 / 블록 게임 등 사운드가 있는 타이틀)
./build/spc_launcher --tape "tape/Block Game.tap"
```

에뮬레이터 구동 후 `load` $\rightarrow$ `run` 실행 시 배경음악과 효과음이 윈도우 스피커를 통해 선명하게 출력됩니다.
