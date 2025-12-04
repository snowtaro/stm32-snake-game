// button.c
#include "button.h"

// 레지스터 매크로 (필요한 것만 사용)
#define RCC_APB2ENR (*(volatile unsigned int *) (0x40021000 + 0x18))

#define GPIOA_CRL   (*(volatile unsigned int *) (0x40010800))
#define GPIOA_IDR   (*(volatile unsigned int *) (0x40010808))

#define GPIOB_CRH   (*(volatile unsigned int *) (0x40010C04))
#define GPIOB_IDR   (*(volatile unsigned int *) (0x40010C08))

#define GPIOC_CRL   (*(volatile unsigned int *) (0x40011000))
#define GPIOC_CRH   (*(volatile unsigned int *) (0x40011004))
#define GPIOC_IDR   (*(volatile unsigned int *) (0x40011008))

#define GPIOD_CRL   (*(volatile unsigned int *) (0x40011400))
#define GPIOD_BSRR  (*(volatile unsigned int *) (0x40011410))

// 내부용 함수: RCC 및 포트 설정
static void RCCEnable(void)
{
    RCC_APB2ENR |= 0x00000004;   // PA
    RCC_APB2ENR |= 0x00000008;   // PB
    RCC_APB2ENR |= 0x00000010;   // PC
    RCC_APB2ENR |= 0x00000020;   // PD
}

static void PortConfiguration(void)
{
    // PC4 (Input with pull-up / pull-down) (KEY1 - UP)
    GPIOC_CRL &= 0xFFF0FFFF;
    GPIOC_CRL |= 0x00080000;

    // PB10 (Input with pull-up / pull-down) (KEY2 - DOWN)
    GPIOB_CRH &= 0xFFFFF0FF;
    GPIOB_CRH |= 0x00000800;

    // PC13 (Input with pull-up / pull-down) (KEY3 - LEFT)
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00800000;

    // PA0 (Input with pull-up / pull-down) (KEY4 - RIGHT)
    GPIOA_CRL &= 0xFFFFFFF0;
    GPIOA_CRL |= 0x00000008;

    // PD2, PD3, PD4, PD7 (LED 출력 설정 - 필요 시 그대로 유지)
    GPIOD_CRL &= 0xFFFFF0FF;
    GPIOD_CRL |= 0x00000300;     // PD2

    GPIOD_CRL &= 0xFFFF0FFF;
    GPIOD_CRL |= 0x00003000;     // PD3

    GPIOD_CRL &= 0xFFF0FFFF;
    GPIOD_CRL |= 0x00030000;     // PD4

    GPIOD_CRL &= 0x0FFFFFFF;
    GPIOD_CRL |= 0x30000000;     // PD7

    // LED OFF
    GPIOD_BSRR |= 0x0000009C;
}

void Button_Init(void)
{
    RCCEnable();
    PortConfiguration();
}

// 키 입력 읽기: 눌리면 LOW 기준
KeyInput Button_GetInput(void)
{
    // KEY1 (PC4) → 위
    if (!(GPIOC_IDR & 0x00000010)) {
        return KEY_UP;
    }
    // KEY2 (PB10) → 아래
    if (!(GPIOB_IDR & 0x00000400)) {
        return KEY_DOWN;
    }
    // KEY3 (PC13) → 왼쪽
    if (!(GPIOC_IDR & 0x00002000)) {
        return KEY_LEFT;
    }
    // KEY4 (PA0) → 오른쪽
    if (!(GPIOA_IDR & 0x00000001)) {
        return KEY_RIGHT;
    }

    return KEY_NONE;
}
