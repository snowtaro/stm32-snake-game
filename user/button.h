// button.h
#ifndef BUTTON_H_
#define BUTTON_H_

#include "stm32f10x.h"

// 키 입력 방향 정의
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT
} KeyInput;

// 버튼 및 관련 GPIO 초기화
void Button_Init(void);

// 키 입력 읽기 (풀업, 눌리면 LOW 기준)
KeyInput Button_GetInput(void);

#endif // BUTTON_H_
