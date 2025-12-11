// main.c
#include "stm32f10x.h"
#include "joystick.h"
#include "display.h"
#include "snake.h"
#include "pir.h"
#include "bt.h"        // 블루투스 추가
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

extern volatile uint32_t g_msTicks;

// 간단한 소프트웨어 딜레이 (게임 속도용)
static void delay_loop(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

// PIR 기반 절전 기능에서 사용할 사람 부재 시간(ms)
// 10초 이상 사람이 없으면 절전 모드 진입
#define PIR_IDLE_SLEEP_MS   10000U

// 블루투스 HEARTBEAT 주기(ms)
#define BT_HEARTBEAT_MS     5000U

// 게임 종료 시 결과 프레임 전송
// 포맷: RPL|yyyy-MM-dd HH:mm:ss|경과시간(초)|점수\r\n
static void BT_SendResultFrame(uint32_t duration_sec, int score)
{
    char buffer[128];

    // 실제 RTC가 없으므로 예시로 고정된 날짜/시간 사용
    // 필요하면 RTC 연동해서 실제 시각으로 교체
    int year  = 2025;
    int month = 11;
    int day   = 13;
    int hour  = 17;
    int min   = 0;
    int sec   = 0;

    sprintf(buffer,
            "RPL|%04d-%02d-%02d %02d:%02d:%02d|%lu|%d\r\n",
            year, month, day, hour, min, sec,
            (unsigned long)duration_sec,
            score);

    BT_SendString(buffer);
}

int main(void)
{
    SystemInit();

    // SysTick 1ms 설정
    SysTick_Config(SystemCoreClock / 1000);

    // BT_Init 안에서 SystemInit()을 호출하므로 별도 SystemInit()은 생략
    BT_Init();        // 블루투스(USART2) 및 PC 터미널(USART1) 초기화

    Joystick_Init();  // Changed from Button_Init
    Display_Init();
    PIR_Init();
    snake_setup();

    // 난수 시드 (필요시 고정값)
    srand(1);

    static char grid[GRID_ROWS][GRID_COLS];

    // PIR 관련 시간 관리
    uint32_t nowMs          = PIR_GetMillis();
    uint32_t lastMotionTime = nowMs;   // 마지막으로 사람이 감지된 시점

    // 게임 시간/HEARTBEAT 관리
    uint32_t gameStartTimeMs  = nowMs; // 현재 게임 시작 시각
    uint32_t lastHeartbeatMs  = nowMs; // 마지막 HEARTBEAT 전송 시각

    while (1)
    {
        // 공통 시간 값 (루프 시작 시점 기준)
        nowMs = PIR_GetMillis();

        // 0) PIR 센서 상태 확인 및 절전 모드 관리
        uint8_t pirPresent = PIR_IsPersonPresent();

        if (pirPresent) {
            // 사람이 감지되면 마지막 감지 시점을 갱신하고, 절전 중이면 해제
            lastMotionTime = nowMs;
            if (Display_IsSleep()) {
                Display_SetSleep(0);
            }
        } else {
            // 사람이 없고, 일정 시간 이상 부재 상태가 지속되면 절전 모드 진입
            if (!Display_IsSleep()) {
                if ((nowMs - lastMotionTime) >= PIR_IDLE_SLEEP_MS) {
                    Display_SetSleep(1);
                }
            }
        }

        // 0-1) LED 제어 (Joystick 기능)
        Joystick_HandleInput();

        // 1) 조이스틱 입력 → 방향 변경
        KeyInput key = Joystick_GetInput(); // Changed from Button_GetInput
        if (key != KEY_NONE) {
            snake_set_direction(key);

            // 간단 디바운스 (원하면 유지)
            delay_loop(50000);
        }

        // 2) 게임 상태 업데이트
        if (snake_update()) {
            // 게임 오버 발생 시점의 경과 시간(초) 계산
            uint32_t duration_ms  = nowMs - gameStartTimeMs;
            uint32_t duration_sec = duration_ms / 1000U;

            // 종료 시 결과 프레임 전송: RPL|yyyy-MM-dd HH:mm:ss|경과초|점수
            BT_SendResultFrame(duration_sec, snake_get_score());

            // 게임 다시 시작
            snake_setup();
            gameStartTimeMs = nowMs;   // 새 게임 시작 시각 갱신
        }

        // 3) 현재 상태를 char 그리드로 변환
        snake_to_grid(grid);

        // 4) 그리드를 LCD에 출력 (절전 모드라면 내부에서 자동으로 skip)
        Display_DrawGrid(grid);

        // 5) 게임 중 HEARTBEAT 전송 (5초 주기)
        if ((nowMs - lastHeartbeatMs) >= BT_HEARTBEAT_MS) {
            // 게임 중 keep-alive 용 단순 문자열
            BT_SendString("HEARTBEAT\r\n");
            lastHeartbeatMs = nowMs;
        }

        // 6) 게임 속도 조절 (값 조정해서 원하는 스피드 맞추면 됨)
        delay_loop(10000);
    }

    // 여기까지 도달하지 않음
    // return 0;
}
