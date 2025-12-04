// display.h
#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "stm32f10x.h"
#include "lcd.h"
#include "snake.h"

// 스네이크 보드를 그대로 사용
#define GRID_ROWS   BOARD_HEIGHT
#define GRID_COLS   BOARD_WIDTH

void Display_Init(void);

// grid[y][x] 에 들어있는 문자(' ', 'O', 'o', '*')를 LCD에 출력
void Display_DrawGrid(char grid[GRID_ROWS][GRID_COLS]);

#endif // DISPLAY_H_
