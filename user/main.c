// main.c
#include "stm32f10x.h"
#include "joystick.h"
#include "display.h"
#include "snake.h"
#include <stdlib.h>

// 간단한 소프트웨어 딜레이 (게임 속도용)
static void delay_loop(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

int main(void)
{
    SystemInit();

    Joystick_Init(); // Changed from Button_Init
    Display_Init();
    snake_setup();

    // 난수 시드 (필요시 고정값)
    srand(1);

    static char grid[GRID_ROWS][GRID_COLS];

    while (1)
    {
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
        snake_update();

        // 3) 현재 상태를 char 그리드로 변환
        snake_to_grid(grid);

        // 4) 그리드를 LCD에 출력
        Display_DrawGrid(grid);

        // 5) 게임 속도 조절 (값 조정해서 원하는 스피드 맞추면 됨)
        delay_loop(10000);
    }

    return 0;
}
