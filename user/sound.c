#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_tim.h"
#include "misc.h"
#include "sound.h"

/*
volatile uint32_t beep_end_time = 0;
volatile uint8_t beep_active = 0;

volatile uint32_t ms_ticks = 0;

void SysTick_Handler(void)
{
    ms_ticks++;
}

uint32_t millis(void)
{
    return ms_ticks;
}
*/


void RCC_Configure(void)
{  
    /* GPIOB 포트 활성화 */    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}

void GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // PB10 : PWM 출력
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   // PWM
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void TIM_Configure(void)
{
    TIM_TimeBaseInitTypeDef TIM3_InitStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    /* 1MHz(1us tick) 타이머 클럭 만들기 */
    uint16_t prescaler = (uint16_t)((SystemCoreClock / 1000000) - 1);

    /* 1) 타이머 기본 설정 먼저 */
    TIM3_InitStructure.TIM_Period = 499;     // 2kHz PWM
    TIM3_InitStructure.TIM_Prescaler = prescaler;
    TIM3_InitStructure.TIM_ClockDivision = 0;
    TIM3_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM3_InitStructure);

    /* 2) PWM 채널 설정 */
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = 0; 
    TIM_OC3Init(TIM3, &TIM_OCInitStructure);

    /* Preload는 반드시 ENABLE */
    TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    /* 타이머 시작 */
    TIM_Cmd(TIM3, ENABLE);
}

/*
void beep_start(uint32_t duration_ms)
{
    TIM_SetCompare3(TIM3, 250);  // 소리 ON (50% duty)
    beep_active = 1;
    beep_end_time = millis() + duration_ms; 
}

void beep_stop(void)
{
    TIM_SetCompare3(TIM3, 0);   // 소리 OFF
    beep_active = 0;
}

void beep_update(void)
{
    if (beep_active && millis() >= beep_end_time)
    {
        beep_stop();
    }
}

*/

void delay(volatile uint8_t count) {
    while(count--) {
        __NOP();
    }
}

void beep_start() {
    TIM_SetCompare3(TIM3, 250);
    delay(10000);
    TIM_SetCompare3(TIM3, 0);
}

