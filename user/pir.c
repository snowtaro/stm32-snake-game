// pir.c
#include "pir.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

extern volatile uint32_t g_msTicks;

uint32_t PIR_GetMillis(void)
{
    return g_msTicks;
}

void PIR_Init(void)
{
    // GPIOC 클럭 활성화
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_StructInit(&GPIO_InitStructure);

    // PC5 를 입력(Pull-down)으로 사용
    // DYP-ME003 모듈 출력이 HIGH = 감지, LOW = 미감지라고 가정
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD; // Input pull-down
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // 주의:
    // SysTick_Config(SystemCoreClock / 1000)는 main에서 1회만 수행
    // 여기서 SysTick를 다시 설정하지 않음
}

uint8_t PIR_IsPersonPresent(void)
{
    // PC5 가 HIGH 이면 사람/움직임 감지
    return (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) != Bit_RESET) ? 1 : 0;
}