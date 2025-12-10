// main.c
#include "stm32f10x.h"
#include "button.h"
#include "display.h"
#include "snake.h"
#include "bt.h"      // 블루투스 모듈 추가
#include <stdlib.h>

// 간단한 소프트웨어 딜레이 (게임 속도용)
static void delay_loop(volatile uint32_t count)
{
    while (count--) {
        __NOP();
    }
}

// 한 프레임당 경과 시간(ms) 가정값
// delay_loop(10000)에 대응해서 대략적인 값으로 사용
// (보드 주파수에 맞게 조정 가능)
#define SNAKE_FRAME_MS 200U

int main(void)
{
    char grid[BOARD_HEIGHT][BOARD_WIDTH];

    uint32_t play_time_ms = 0;  // 한 판 플레이 시간(ms) 누적

    SystemInit();

    // 블루투스 초기화 (USART1/2, GPIO, NVIC)
    BT_Init();

    // 버튼 / LCD / 스네이크 초기화
    Button_Init();
    Display_Init();
    snake_setup();

    // 난수 시드 (음식 위치용)
    srand(1);

    while (1)
    {
        // 1) 입력 처리
        KeyInput key = Button_GetInput();
        if (key != KEY_NONE)
        {
            snake_set_direction((int)key);

            // 간단 디바운스: 키가 뗄 때까지 대기
            delay_loop(50000);
            while (Button_GetInput() != KEY_NONE)
                ;
        }

        // 2) 게임 상태 업데이트
        if (snake_update())
        {
            // 게임 종료 시점: 점수 및 플레이 시간 블루투스로 전송
            int score = snake_get_score();
            BT_SendScoreFrame(score, play_time_ms);

            // 다음 판을 위해 시간/상태 리셋
            play_time_ms = 0;
            snake_setup();
            continue;
        }

        // 3) 현재 상태를 char 그리드로 변환
        snake_to_grid(grid);

        // 4) 그리드를 LCD에 출력
        Display_DrawGrid(grid);

        // 5) 게임 속도 조절 및 플레이 시간 누적
        delay_loop(10000);
        play_time_ms += SNAKE_FRAME_MS;
    }

    return 0;
}
