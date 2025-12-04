// display.c
#include "display.h"

#define BASE_X      10   // 좌측 여백
#define BASE_Y      10   // 상단 여백
#define CELL_W      10   // 가로 간격 (픽셀)
#define CELL_H      12   // 세로 간격 (픽셀)

void Display_Init(void)
{
    LCD_Init();
    LCD_Clear(WHITE);
}

void Display_DrawGrid(char grid[GRID_ROWS][GRID_COLS])
{
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
