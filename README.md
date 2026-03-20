## Infineon의 AURIX™ 보드로 MCU 프로그래밍 익히기

- 개발 보드: ShieldBuuddy TC275
- 개발 환경: AURIX Development Studio

`tricore` 보드로 3개의 코어가 있지만 CPU0의 한 개만 사용하여 실습

---
### 260219_INTERRUPT

🎯 과제

S3 버튼 입력 인터럽트를 활용하여 LED 깜빡임을 On/Off

⚙️ 특징
- `iLLD` 드라이버를 사용하지 않고 **레지스터 직접 제어** 방식
- `상태 기반(FSM)`으로 LED 동작 제어
  - STATE_BLINKING(깜빡임)
  - STATE_IDLE(정지) 
- `ERU(External Request Unit)`를 이용한 외부 인터럽트 설정
- `Timer` 대신 for문 기반 **Busy-wait** 방식으로 LED 깜빡임 주기를 제어

### 260223_FND

### 260223_TIMER

### 260224_ADC

### 260224_SCHEDULE

### 260225_MINI_PROJECT
