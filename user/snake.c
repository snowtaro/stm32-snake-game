// snake.c
#include <stdio.h>
#include <stdlib.h>
#include "snake.h"
#include "button.h"

static Snake snake;
static Point food;
static int direction;
static int score;

// 무작위로 음식 생성
static void generate_food(void)
{
    // 경계선에서 생성되지 않게 (1 ~ BOARD_WIDTH-2, 1 ~ BOARD_HEIGHT-2)
    food.x = rand() % (BOARD_WIDTH - 2) + 1;
    food.y = rand() % (BOARD_HEIGHT - 2) + 1;
}

// 게임 시작 전 초기화
void snake_setup(void)
{
    snake.length = 3;
    direction = KEY_RIGHT;

    int startX = BOARD_WIDTH / 2;
    int startY = BOARD_HEIGHT / 2;

    for (int i = 0; i < snake.length; i++) {
        snake.body[i].x = startX - i;
        snake.body[i].y = startY;
    }

    score = 0;
    generate_food();
}

// 게임 진행 중 상태 업데이트 - 출력이 1이면 게임 종료
int snake_update(void)
{
    // 현재 머리 위치
    Point head = snake.body[0];
    Point newHead = head;

    // 방향에 따른 이동
    switch (direction)
    {
        case KEY_UP:    newHead.y -= 1; break;
        case KEY_DOWN:  newHead.y += 1; break;
        case KEY_LEFT:  newHead.x -= 1; break;
        case KEY_RIGHT: newHead.x += 1; break;
        default: break;
    }

    // 벽 충돌 체크
    if (newHead.x < 0 || newHead.x >= BOARD_WIDTH ||
        newHead.y < 0 || newHead.y >= BOARD_HEIGHT)
    {
        // GAME OVER → 다시 시작
        return 1;
    }

    // 자기몸 충돌 체크
    for (int i = 0; i < snake.length; i++) {
        if (snake.body[i].x == newHead.x &&
            snake.body[i].y == newHead.y)
        {
            // GAME OVER 
            return 1;
        }
    }

    // 먹이 먹기 여부
    int ateFood = (newHead.x == food.x && newHead.y == food.y);

    // 몸통 이동 (뒤에서부터 앞으로 당김)
    for (int i = snake.length - 1; i > 0; i--) {
        snake.body[i] = snake.body[i - 1];
    }

    // 머리 갱신
    snake.body[0] = newHead;

    // 먹이를 먹었으면 길이 증가
    if (ateFood) {
        if (snake.length < MAX_SNAKE_SIZE) {
            snake.body[snake.length] = snake.body[snake.length - 1];
            snake.length++;
        }
        score++;
        generate_food();
    }

    return 0;
}

void snake_set_direction(int newDir)
{
    // 반대 방향으로는 못 돌게 막기
    if ((direction == KEY_UP    && newDir == KEY_DOWN) ||
        (direction == KEY_DOWN  && newDir == KEY_UP)   ||
        (direction == KEY_LEFT  && newDir == KEY_RIGHT)||
        (direction == KEY_RIGHT && newDir == KEY_LEFT))
    {
        return; // 반대 방향 무시
    }
    direction = newDir;
}

Snake* snake_get(void)
{
    return &snake;
}

Point* snake_get_food(void)
{
    return &food;
}

int snake_get_score(void)
{
    return score;
}

// 스네이크 상태를 char 그리드로 변환
// 빈 칸: ' '
// 뱀 머리: 'O', 몸통: 'o'
// 먹이: '*'
void snake_to_grid(char grid[BOARD_HEIGHT][BOARD_WIDTH])
{
    // 1) 전체를 공백으로 초기화
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            grid[y][x] = ' ';
        }
    }

    // 2) 먹이 표시
    if (food.x >= 0 && food.x < BOARD_WIDTH &&
        food.y >= 0 && food.y < BOARD_HEIGHT)
    {
        grid[food.y][food.x] = '*';
    }

    // 3) 뱀 표시 (머리/몸통)
    for (int i = snake.length - 1; i >= 0; i--) {
        Point p = snake.body[i];
        if (p.x < 0 || p.x >= BOARD_WIDTH ||
            p.y < 0 || p.y >= BOARD_HEIGHT) {
            continue;   // 방어적 체크
        }
        if (i == 0) {
            grid[p.y][p.x] = 'O'; // 머리
        } else {
            grid[p.y][p.x] = 'o'; // 몸통
        }
    }
}
