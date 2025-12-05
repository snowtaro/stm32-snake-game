// snake.h
#ifndef SNAKE_H
#define SNAKE_H

#include <stdint.h>

// ������ũ ���� ũ��
#define BOARD_WIDTH     20
#define BOARD_HEIGHT    20
#define MAX_SNAKE_SIZE  100

// (���� ������� �ʿ�� ����)
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

// ���� ������ũ ���¸� 2D �׸���(char �迭)�� ����
// grid[y][x] �� ���� ä�� (����: ' ')
void snake_to_grid(char grid[BOARD_HEIGHT][BOARD_WIDTH]);

#endif
