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

// 디스플레이 절전 모드 설정/조회
// enable = 1 : 절전 모드 (화면 갱신 중단)
// enable = 0 : 정상 모드
void Display_SetSleep(uint8_t enable);
uint8_t Display_IsSleep(void);
void Display_Wait(void);

// 다크 모드 설정
void set_dark_mode(uint8_t);

#endif // DISPLAY_H_
