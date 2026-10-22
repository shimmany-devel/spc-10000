# 작업 보고서: SPC-1000 키 입력 씹힘 방지 및 부드러운 키 래칭(Latching) 구현

- **일시**: 2026-10-22 23:00:00 (KST)
- **작업자**: Antigravity Pair Programming Agent
- **사용자 요청**: "키 입력이 매끄럽지 못함.. 안눌릴때도 많고.. 부드럽게 입력할 수 있게 수정하라."

---

## 1. 원인 분석 (왜 키가 안 눌리고 씹혔는가?)

### 1.1 프레임 경계 내 키 릴리즈 증발 (Same-Frame Drop)
- 기존 이벤트 루프는 매 60Hz 프레임(16.6ms) 시작 시 `while (SDL_PollEvent(&event))`로 모든 이벤트를 소비한 후 CPU 1프레임(66,666 사이클)을 한 번에 실행했습니다.
- 사용자가 키를 가볍게 탭(빠르게 타이핑)하면 OS는 수 밀리초 간격으로 `KEYDOWN`과 `KEYUP`을 보냅니다.
- 만약 한 프레임 주기 내에서 `KEYDOWN`과 `KEYUP`이 연달아 처리되면, `matrix_` 비트가 0으로 내려갔다가 **CPU가 키보드를 읽기도 전에 즉시 1로 원복**되었습니다.
- 그 결과, Z80 CPU는 키가 눌렸다는 사실을 전혀 인지하지 못하고 지나가버리는 치명적인 키 씹힘(Key Drop)이 발생했습니다.

### 1.2 Hu-BASIC 키보드 스캔 디바운스 타이밍
- Hu-BASIC의 키보드 스캔 루틴은 노이즈 필터링(Debounce)을 위해 키 상태가 일정 시간(수 프레임) 이상 안정적으로 유지되어야 정상 입력으로 판정합니다.

### 1.3 키보드 매핑 오류
- `ESC` 키가 실기 사양인 `Row 3, 0x04`가 아닌 `Row 0, 0x10`(BREAK)에 잘못 할당되어 있었습니다.
- 텐키(Numpad) 및 PC 특수 키(`Delete`, `CapsLock`) 매핑이 누락되어 있었습니다.
- PC 자판과 SPC-1000 자판 간 특수기호(`:`, `"`, `+`, `-`, `@` 등) 불일치로 기호 입력 시 오작동이 있었습니다.

---

## 2. 해결 및 개선 내역

1. **키 누름 최소 보장 래칭 (Key Hold Latch) 구현 ([keyboard.hpp](file:///home/shimmany/work/spc-10000/src/hw/keyboard.hpp), [keyboard.cpp](file:///home/shimmany/work/spc-10000/src/hw/keyboard.cpp))**:
   - `handle_key_down(sym)` 호출 시 일반 키에 대해 `hold_ticks = 3` (최소 3프레임, 약 50ms 강제 유지) 카운터를 부여합니다.
   - 키를 즉시 떼더라도 `hold_ticks > 0`인 동안은 매트릭스의 Active-Low 상태(0)를 강제로 유지하여, Hu-BASIC의 키보드 스캔 디바운스 루틴이 100%確実に 감지하도록 보장합니다.
   - 매 프레임 끝 `tick_frame()`에서 카운터가 소진되고 물리적으로 키가 떼어졌을 때 비로소 안전하게 릴리즈합니다.
   - 수식키(Shift, Ctrl, Alt, Caps) 및 게임용 방향키(Up, Down, Left, Right)는 홀드 카운터 없이 물리적 누름 상태를 즉시 실시간 반영하도록 차별화하였습니다.
2. **실기 키보드 매핑 정상화 및 확장**:
   - `ESC` 키를 실기 정규 위치인 `Row 3, 0x04`로 교정 (BREAK 오동작 해결).
   - `CapsLock`, `Delete` 키 추가.
   - 텐키(Keypad 0~9, Enter, +, -, *, /, .) 전체 매핑 추가.
3. **PC 키보드 특수기호 호환 ROM 패치 ([spc1000_board.cpp](file:///home/shimmany/work/spc-10000/src/sys_spc1000/spc1000_board.cpp))**:
   - `load_rom` 시 0x1300 테이블에 현대 PC 키보드 기호 매핑(`:`, `"`, `+`, `=`, `|`, `~` 등)을 보정하여 직관적으로 타이핑할 수 있도록 적용.

---

## 3. 검증 결과

- **동일 프레임 키 누름/뗌 테스트**:
  - `KEYDOWN`과 `KEYUP`이 같은 프레임에 도착해도 `0x61 (DETECTED)`로 100% 정상 인식 확인.
- **연속 고속 타이핑 테스트**:
  - `"print"` 5글자를 고속 주입했을 때 VRAM에 `[print]`가 단 1글자의 누락이나 중복 없이 완벽히 입력됨을 확인.
- **빌드 및 릴리즈 완료**:
  - `make build` 성공 및 `git push origin main` 반영 완료.
