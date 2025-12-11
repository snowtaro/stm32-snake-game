// main.c
#include "stm32f10x.h"
#include "joystick.h"
#include "display.h"
#include "snake.h"
#include "pir.h"
#include <stdlib.h>
#include <stdint.h>

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

int main(void)
{
    SystemInit();

    Joystick_Init(); // Changed from Button_Init
    Display_Init();
    PIR_Init();
    snake_setup();

    // 난수 시드 (필요시 고정값)
    srand(1);

    static char grid[GRID_ROWS][GRID_COLS];

    // PIR 관련 시간 관리
    uint32_t lastMotionTime = PIR_GetMillis();  // 마지막으로 사람이 감지된 시점
    uint8_t  lastPirState   = 0;                // 직전 PIR 상태(0/1)

    while (1)
    {
        // 0) PIR 센서 상태 확인 및 절전 모드 관리
        uint32_t nowMs = PIR_GetMillis();
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
        lastPirState = pirPresent;
        // 0) LED 제어 (Joystick 기능)
        Joystick_HandleInput();

        // 1) 버튼 입력 → 방향 변경
        KeyInput key = Joystick_GetInput(); // Changed from Button_GetInput
        if (key != KEY_NONE) {
            snake_set_direction(key);

            // 간단 디바운스 (원하면 유지)
            delay_loop(50000);
            while (Joystick_GetInput() != KEY_NONE) {
                // 버튼 뗄 때까지 대기
                Joystick_HandleInput(); // Keep handling LEDs while waiting?
            }
        }

        // 2) 게임 상태 업데이트
        if (snake_update()) {
            // 게임 오버일 시 > 다시 시작
            snake_setup();
        }

        // 3) 현재 상태를 char 그리드로 변환
        snake_to_grid(grid);

        // 4) 그리드를 LCD에 출력 (절전 모드라면 내부에서 자동으로 skip)
        Display_DrawGrid(grid);

        // 5) 게임 속도 조절 (값 조정해서 원하는 스피드 맞추면 됨)
        delay_loop(10000);
    }

    return 0;
}
