### 소개
STM32F401RE + FreeRTOS 기반 서보 전력 모니터링 및 과부하 차단 시스템

---

<img width="800" height="600" alt="KakaoTalk_20260821_000313359-ezgif com-optimize (1)" src="https://github.com/user-attachments/assets/90233da4-a7d1-4028-9427-6c09674085a1" />

---

### 프로젝트 구조
```text
Core/
├── Inc/
│   ├── ina219.h        # INA219 드라이버 (직접 구현)
│   └── ssd1306.h       # SSD1306 OLED 드라이버 (직접 구현)
└── Src/
    ├── ina219.c        # 레지스터 R/W, Calibration, 전압·전류 변환
    ├── ssd1306.c       # 초기화 시퀀스, 페이지 전송, 5x7 폰트 렌더링
    └── freertos.c      # 태스크 8개, 동기화 요소
```
INA219·SSD1306 드라이버는 외부 라이브러리 없이 데이터시트를 참고해 직접 구현했습니다.

---

### 개요
INA219로 서보 모터의 소비 전류를 측정해 OLED·블루투스로 출력하고, 과부하가 지속되면 PWM을 차단합니다.

---

### 하드웨어
  - 부품 목록
    - STM32F401RE
    - INA219
    - HC-06
    - OLED
    - 서보모터
  - 배선표
    ```text
    - INA219    VCC -> 3.3V    GND -> GND    SDA -> PB9    SCL -> PB8
                VIN+ -> 5V 레일    VIN- -> 서보 VCC(션트 0.1Ω 경유)
    - OLED    VCC -> 3.3V    GND -> GND    SDA -> PB9    SCL -> PB8
    - HC-06    VCC -> 3.3V    GND -> GND    RXD -> PA9    TXD -> PA10
    - 서보 모터    신호 -> PA0    VCC -> INA219 VIN-    GND -> GND
    ```

---

### 핀 배정표
| 기능 | 핀 | 비고 |
| :--- | :--- | :--- |
| I2C1 | PB8/PB9 | 100kHz |
| USART1 | PA9/PA10 | 9600, 블루투스 |
| USART2 | PA2/PA3 | 디버그 로그 |
| TIM2_CH1 | PA0 | 서보 PWM 50Hz |
| EXTI13 | PC13 | 사용자 버튼 |

---

### 태스크 구성
| 태스크 | 우선순위 | 주기 | 역할 |
| :--- | :--- | :--- | :--- |
| CONTROL | 4 | 이벤트 | 과부하 판정·차단 |
| BUTTON | 3 | 이벤트 | 차단 해제 |
| SENSOR | 2 | 50ms | INA219 측정 |
| SERVO | 2 | 1s | PWM 각도 변경 |
| HEARTBEAT | 2 | 1s | 생존 확인 |
| DISPLAY | 1 | 500ms | OLED 갱신 |
| STATS | 1 | 5s | 스택·힙 진단 |
| BLINK | 1 | 1s | LED |

```text
CONTROL 태스크를 최상위로 둔 이유는 과부하 차단이 안전 관련 동작이기 때문입니다.
DISPLAY 태스크와 STATS 태스크는 늦어도 무방할 것이라 생각해 최하위로 두었습니다.
SENSOR 태스크와 SERVO 태스크는 주기 정확도가 필요해 중간 우선순위 입니다.
```

---

### 동기화 요소
```text
xI2CMutex - INA219와 OLED가 I2C 자원 공유로 인해 Mutex를 사용했습니다.
xBtnSem - EXTI ISR과 BUTTON 태스크 간 동기화를 위한 Semaphore를 사용했습니다.
xSensorQueue - SENSOR 태스크와 CONTROL 태스크간의 통신을 위한 Queue 메세지를 사용했습니다.
```

---

### 빌드 방법
STM32CubeIDE Version: 2.2.0
1. STM32CubeIDE에서 STM32F401RE.ioc 파일을 더블 클릭하여 엽니다.
2. 단축키 Alt + K (Mac: `Cmd + K`)를 눌러 **Code Generation**을 수행합니다.
3. 생성이 완료되면 Ctrl + B를 눌러 프로젝트를 전체 빌드합니다.

---

### 한계
```text
전원 임피던스
서보 기동시 374mA에서 전압이 4.94V -> 4.03V로 강하했고, 역산하면 경로 저항이 약 2.6Ω입니다.
이 때문에 스톨 상태에서도 전류가 지속적으로 높지 않아 과부하 판정에 어려움이 있었습니다.
서보 전원 분리로 해결 가능합니다.

빵판 접촉 불량
개발 중 원인 불명 동작 이상이 4회 발생했고 전부 접촉 불량이었습니다.
400 ~ 600mA가 흐르는 서보 전류 경로에서 반복 발생했습니다.
```
