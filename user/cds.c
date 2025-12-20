#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "cds.h"

volatile uint8_t dark_mode_flag = 0;
volatile uint8_t light_event_occured = 0;

void CDS_RCC_Configure(void)
{  
    /* GPIOC 포트 활성화 */    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
}


void CDS_GPIO_Configure(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // PC10 : DO 입력
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;   // 풀다운
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // PC10을 EXTI10 라인에 연결
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource10);
}


void CDS_EXTI_Configure(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;

    EXTI_InitStructure.EXTI_Line = EXTI_Line10;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;

    // DO 신호가 HIGH → LOW로 떨어질 때 인터럽트
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;

    EXTI_Init(&EXTI_InitStructure);
}


void CDS_NVIC_Configure(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;

    NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);
}

void CDS_Init() {
    CDS_RCC_Configure();
    CDS_GPIO_Configure();
    CDS_NVIC_Configure();
}

// 주변 환경이 어두워지거나 밝아지면 인터럽트
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line10) != RESET)
    {
        uint8_t current_state = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_10);

        if (current_state != dark_mode_flag) {
            dark_mode_flag = current_state;
            light_event_occured = 1;
        }

        EXTI_ClearITPendingBit(EXTI_Line10);
    }
}

uint8_t get_dark_mode_flag(void) {
    return dark_mode_flag;
}

uint8_t is_light_event_occured(void) {
    return light_event_occured;
}

void set_light_event_flag(uint8_t mode) {
    light_event_occured = mode;
}