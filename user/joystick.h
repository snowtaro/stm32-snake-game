#ifndef JOYSTICK_H
#define JOYSTICK_H

// 키 입력 방향 정의 (Moved from button.h or duplicated)
typedef enum {
    KEY_NONE = 0,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT
} KeyInput;

void Joystick_RCCEnable(void);
void Joystick_PortConfiguration(void);
void Joystick_HandleInput(void);

// New functions for integration
void Joystick_Init(void);
KeyInput Joystick_GetInput(void);

#endif
