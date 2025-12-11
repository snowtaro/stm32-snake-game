#include "stm32f10x.h"
#include "joystick.h"

/*
 * 버튼 → 방향 테스트
 *  PC4  → UP
 *  PB10 → DOWN
 *  PC13 → LEFT
 *  PA0  → RIGHT
 *
 * LED
 *  PD2 → LED1
 *  PD3 → LED2
 */

#define RCC_APB2ENR     (*(volatile unsigned int *)(0x40021018))

#define GPIOA_CRL       (*(volatile unsigned int *)(0x40010800))
#define GPIOA_IDR       (*(volatile unsigned int *)(0x40010808))

#define GPIOB_CRH       (*(volatile unsigned int *)(0x40010C04))
#define GPIOB_IDR       (*(volatile unsigned int *)(0x40010C08))

#define GPIOC_CRL       (*(volatile unsigned int *)(0x40011000))
#define GPIOC_CRH       (*(volatile unsigned int *)(0x40011004))
#define GPIOC_IDR       (*(volatile unsigned int *)(0x40011008))

#define GPIOD_CRL       (*(volatile unsigned int *)(0x40011400))
#define GPIOD_BSRR      (*(volatile unsigned int *)(0x40011410))


void Joystick_RCCEnable()
{
    RCC_APB2ENR |= (1 << 2);   // GPIOA
    RCC_APB2ENR |= (1 << 3);   // GPIOB
    RCC_APB2ENR |= (1 << 4);   // GPIOC
    RCC_APB2ENR |= (1 << 5);   // GPIOD
}

void Joystick_PortConfiguration()
{
    /* -------- BUTTON INPUTS (Pull-up/down) -------- */

    // PC4 → UP
    GPIOC_CRL &= 0xFFF0FFFF;
    GPIOC_CRL |= 0x00080000;   // Input pull-up/down

    // PB10 → DOWN
    GPIOB_CRH &= 0xFFFFF0FF;
    GPIOB_CRH |= 0x00000800;   // Input pull-up/down

    // PC13 → LEFT
    GPIOC_CRH &= 0xFF0FFFFF;
    GPIOC_CRH |= 0x00800000;   // Input pull-up/down

    // PA0 → RIGHT
    GPIOA_CRL &= 0xFFFFFFF0;
    GPIOA_CRL |= 0x00000008;   // Input pull-up/down


    /* -------- LED OUTPUTS -------- */

    // PD2 (push-pull, 50 MHz)
    GPIOD_CRL &= 0xFFFFF0FF;
    GPIOD_CRL |= 0x00000300;

    // PD3 (push-pull, 50 MHz)
    GPIOD_CRL &= 0xFFFF0FFF;
    GPIOD_CRL |= 0x00003000;

    /* -------- INITIAL LED OFF -------- */
    // PD2 OFF, PD3 OFF   (BSRR lower 16bit → Set bit)
    GPIOD_BSRR = (1 << 2) | (1 << 3);
}


void Joystick_HandleInput()
{
    int up    = !(GPIOC_IDR & (1 << 4));   // PC4
    int down  = !(GPIOB_IDR & (1 << 10));  // PB10
    int left  = !(GPIOC_IDR & (1 << 13));  // PC13
    int right = !(GPIOA_IDR & (1 << 0));   // PA0

    // UP → LED1 ON
    if (up) {
        GPIOD_BSRR = (1 << (2 + 16));   // Reset PD2 → LED ON
    }

    // DOWN → LED2 ON
    if (down) {
        GPIOD_BSRR = (1 << (3 + 16));   // Reset PD3 → LED ON
    }

    // LEFT → LED1 OFF
    if (left) {
        GPIOD_BSRR = (1 << 2);          // Set PD2 → LED OFF
    }

    // RIGHT → LED2 OFF
    if (right) {
        GPIOD_BSRR = (1 << 3);          // Set PD3 → LED OFF
    }
}

void Joystick_Init(void)
{
    Joystick_RCCEnable();
    Joystick_PortConfiguration();
}

KeyInput Joystick_GetInput(void)
{
    // Logic from HandleInput but returning KeyInput
    // PC4 → UP
    if (!(GPIOC_IDR & (1 << 4))) return KEY_UP;
    // PB10 → DOWN
    if (!(GPIOB_IDR & (1 << 10))) return KEY_DOWN;
    // PC13 → LEFT
    if (!(GPIOC_IDR & (1 << 13))) return KEY_LEFT;
    // PA0 → RIGHT
    if (!(GPIOA_IDR & (1 << 0))) return KEY_RIGHT;

    return KEY_NONE;
}
