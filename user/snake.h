// snake.h
#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>

// 스네이크 보드 크기
#define BOARD_WIDTH     20
#define BOARD_HEIGHT    20
#define MAX_SNAKE_SIZE  100

// (기존 상수들은 필요시 유지)
#define CELL_SIZE 10
#define WIDTH   40
#define HEIGHT  20

typedef struct {
    int x, y;
} Point;

typedef struct {
    Point body[MAX_SNAKE_SIZE];
    int length;
} Snake;

void snake_setup(void);
int snake_update(void);
void snake_set_direction(int newDir);

Snake* snake_get(void);
Point* snake_get_food(void);
int snake_get_score(void);

// 현재 스네이크 상태를 2D 그리드(char 배열)에 투영
// grid[y][x] 에 문자 채움 (공백: ' ')
void snake_to_grid(char grid[BOARD_HEIGHT][BOARD_WIDTH]);

#endif
