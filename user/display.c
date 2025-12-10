// display.c
#include "display.h"

#define BASE_X      10   // 좌측 여백
#define BASE_Y      10   // 상단 여백
#define CELL_W      10   // 가로 간격 (픽셀)
#define CELL_H      12   // 세로 간격 (픽셀)

static uint8_t s_sleepMode = 0;  // 1이면 절전 상태

void Display_Init(void)
{
    LCD_Init();
    LCD_Clear(WHITE);
    s_sleepMode = 0;
}

// 절전 모드 on/off
void Display_SetSleep(uint8_t enable)
{
    if (enable) {
        if (!s_sleepMode) {
            // 처음 절전 진입 시 화면을 어둡게/지우기
            LCD_Clear(BLACK);
        }
        s_sleepMode = 1;
    } else {
        if (s_sleepMode) {
            // 절전 해제 시 기본 배경으로 초기화
            LCD_Clear(WHITE);
        }
        s_sleepMode = 0;
    }
}

uint8_t Display_IsSleep(void)
{
    return s_sleepMode;
}

void Display_DrawGrid(char grid[GRID_ROWS][GRID_COLS])
{
    // 절전 모드에서는 화면을 갱신하지 않음
    if (s_sleepMode) {
        return;
    }

    LCD_Clear(WHITE);

    for (int y = 0; y < GRID_ROWS; y++) {
        for (int x = 0; x < GRID_COLS; x++) {
            char ch = grid[y][x];

            // 공백은 출력 생략
            if (ch == ' ' || ch == '\0')
                continue;

            char buf[2] = { ch, '\0' };
            int px = BASE_X + x * CELL_W;
            int py = BASE_Y + y * CELL_H;

            LCD_ShowString(px, py, buf, BLACK, WHITE);
        }
    }

    // 점수 등 추가 정보 출력 예시 (원하면)
    // char scoreStr[16];
    // sprintf(scoreStr, "Score:%d", snake_get_score());
    // LCD_ShowString(10, BASE_Y + GRID_ROWS * CELL_H + 5, scoreStr, BLUE, WHITE);
}
